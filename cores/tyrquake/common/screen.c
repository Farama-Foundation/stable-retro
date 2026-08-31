/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "compat/strl.h"

#include <string.h>

#include "client.h"
#include "cmd.h"
#include "common.h"
#include "console.h"
#include "draw.h"
#include "keys.h"
#include "menu.h"
#include "quakedef.h"
#include "sbar.h"
#include "screen.h"
#include "sound.h"
#include "sys.h"
#include "view.h"

#include "d_iface.h"
#include "r_local.h"

#include "host.h"
#include "perf_timing.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions

syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?

async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint();
SlowPrint();
Screen_Update();
Con_Printf();

net
turn off messages option

the refresh is always rendered, unless the console is full screen

console is:
	notify lines
	half
	full
*/

static qboolean scr_initialized;	/* ready to draw */

/* only the refresh window will be updated unless these variables are flagged */
int scr_copytop;
int scr_copyeverything;

float scr_con_current;
static float scr_conlines;		/* lines of console to display */

int scr_fullupdate;
static int clearconsole;
int clearnotify;

vrect_t scr_vrect;

qboolean scr_disabled_for_loading;
qboolean scr_block_drawing;

static cvar_t scr_centertime = { "scr_centertime", "2" };
static cvar_t scr_printspeed = { "scr_printspeed", "8" };

cvar_t scr_viewsize = { "viewsize", "100", true };
cvar_t scr_fov = { "fov", "90" };	/* 10 - 170 */
static cvar_t scr_conspeed = { "scr_conspeed", "300" };
/* scr_uiscale: integer multiplier applied to all 320x200-native UI
 * elements (menu, status bar, console, intermission text). 0 = auto,
 * which picks min(vid.width/320, vid.height/200) clamped to >= 1.
 * Manual override values are clamped to 1..MAX_UI_SCALE. */
cvar_t scr_uiscale = { "scr_uiscale", "0", true };
static vrect_t *pconupdate;
qboolean scr_skipupdate;

static const qpic_t *scr_ram;
static const qpic_t *scr_net;

static char scr_centerstring[1024];
/* Elapsed time since the most recent SCR_CenterPrint, in seconds.
 * Aged once per frame in CL_ReadFromServer so the precision stays
 * bounded even after long single-level engine times. Read by the
 * cl.intermission slow-print branch in SCR_CheckDrawCenterString. */
float scr_centertime_start;	/* for slow victory printing */
float scr_centertime_off;
static int scr_center_lines;
static int scr_erase_lines;
static int scr_erase_center;

static qboolean scr_drawloading;
/* Elapsed time (in seconds) since SCR_BeginLoadingPlaque disabled
 * the rendering pipeline. Used as a 5-second safety net to recover
 * from a load that never completed. Aged once per frame in Host_Frame
 * alongside the other realtime-domain countdowns; reset to 0 at the
 * top of each loading sequence. */
static float scr_disabled_time;


/* ============================================================================= */

/*
==============
SCR_GetUIScale

Resolve the scr_uiscale cvar to a usable integer multiplier. The cvar
value of 0 means auto: pick the largest integer scale that fits both
dimensions of a 320x200 reference. Manual overrides are clamped to
1..MAX_UI_SCALE.
==============
*/
int
SCR_GetUIScale(void)
{
    int s;

    /* (int)NaN and (int)Inf are UB; on x86 typically yield
     * INT_MIN, but the C standard does not require any
     * particular value.  Treat any non-finite cvar value as
     * "auto" so the platform-derived path picks a sane scale. */
    if (IS_NAN(scr_uiscale.value))
	s = 0;
    else
	s = (int)scr_uiscale.value;

    if (s <= 0) {
	int sx = vid.width  / 320;
	int sy = vid.height / 200;
	s = sx < sy ? sx : sy;
    }
    if (s < 1)
	s = 1;
    if (s > MAX_UI_SCALE)
	s = MAX_UI_SCALE;
    return s;
}

/* ============================================================================= */

/*
==============
SCR_DrawNet
==============
*/
static void
SCR_DrawNet(void)
{
   if (cl.last_received_message < 0.3)
      return;

   if (cls.demoplayback)
      return;

   /* Same hazard as Sbar_Draw / Con_DrawConsole: when the menu is
    * up, M_Draw covers vid.buffer but can't reach into the Vulkan
    * overlay queue, and Draw_PicScaled below routes through the
    * Phase 4k intercept.  The net-pic blip then renders on top of
    * the menu pics' transparent regions instead of being hidden
    * behind the menu backdrop the SW path produces.  Skip. */
   if (key_dest == key_menu)
      return;

   {
      int scale = SCR_GetUIScale();
      Draw_PicScaled(scr_vrect.x + 64 * scale, scr_vrect.y, scr_net, scale);
   }
}

/* ============================================================================= */

/*
==================
SCR_SetUpToDrawConsole
==================
*/
static void SCR_SetUpToDrawConsole(void)
{
   Con_CheckResize();

   if (scr_drawloading)
      return;			/* never a console with loading plaque */

   /* decide on the height of the console */
   con_forcedup = !cl.worldmodel || cls.state != ca_active;

   if (con_forcedup) {
      scr_conlines = vid.height;	/* full screen */
      scr_con_current = scr_conlines;
   } else if (key_dest == key_console)
      scr_conlines = vid.height / 2;	/* half screen */
   else
      scr_conlines = 0;	/* none visible */

   if (scr_conlines < scr_con_current) {
      scr_con_current -= scr_conspeed.value * host_frametime;
      if (scr_conlines > scr_con_current)
         scr_con_current = scr_conlines;

   } else if (scr_conlines > scr_con_current) {
      scr_con_current += scr_conspeed.value * host_frametime;
      if (scr_conlines < scr_con_current)
         scr_con_current = scr_conlines;
   }

   if (clearconsole++ < vid.numpages) {
      Sbar_Changed();
   } else if (clearnotify++ < vid.numpages) {
      scr_copytop = 1;
      Draw_TileClear(0, 0, vid.width, con_notifylines);
   } else
      con_notifylines = 0;
}


/*
==================
SCR_DrawConsole
==================
*/
static void SCR_DrawConsole(void)
{
   if (scr_con_current)
   {
      scr_copyeverything = 1;
      Con_DrawConsole(scr_con_current);
      clearconsole = 0;
   }
   else
   {
      if (key_dest == key_game || key_dest == key_message)
         Con_DrawNotify();	/* only draw notify in game */
   }
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void
SCR_CenterPrint(const char *str)
{
   strlcpy(scr_centerstring, str, sizeof(scr_centerstring));
   scr_centertime_off = scr_centertime.value;
   scr_centertime_start = 0;

   /* count the number of lines for centering */
   scr_center_lines = 1;
   while (*str)
   {
      if (*str == '\n')
         scr_center_lines++;
      str++;
   }
}

static void
SCR_EraseCenterString(void)
{
    int y, height;
    int scale = SCR_GetUIScale();

    if (scr_erase_center++ > vid.numpages) {
	scr_erase_lines = 0;
	return;
    }

    if (scr_center_lines <= 4)
	y = vid.height * 0.35;
    else
	y = 48 * scale;

    /* Make sure we don't draw off the bottom of the screen*/
    height = qmin(8 * scale * scr_erase_lines, ((int)vid.height) - y - 1);

    scr_copytop = 1;
    Draw_TileClear(0, y, vid.width, height);
}

static void
SCR_DrawCenterString(void)
{
    char *start;
    int l;
    int j;
    int x, y;
    int remaining;
    int scale = SCR_GetUIScale();

    scr_copytop = 1;
    if (scr_center_lines > scr_erase_lines)
	scr_erase_lines = scr_center_lines;

    scr_centertime_off -= host_frametime;

    if (scr_centertime_off <= 0 && !cl.intermission)
	return;
    if (key_dest != key_game)
	return;

/* the finale prints the characters one at a time */
    if (cl.intermission) {
	float speed = scr_printspeed.value;
	float dt    = scr_centertime_start;
	/* (int)NaN and (int)Inf are UB; with a user-typeable
	 * scr_printspeed cvar, NaN times any dt is NaN.  Clamp
	 * the product to a sane range before casting. */
	if (IS_NAN(speed) || speed < 0.0f)
	    speed = 8.0f;	/* default */
	if (IS_NAN(dt) || dt < 0.0f)
	    dt = 0.0f;
	{
	    float r = speed * dt;
	    if (r > 9999.0f)
		r = 9999.0f;
	    remaining = (int)r;
	}
    } else
	remaining = 9999;

    scr_erase_center = 0;
    start = scr_centerstring;

    if (scr_center_lines <= 4)
	y = vid.height * 0.35;
    else
	y = 48 * scale;

    do {
	/* scan the width of the line */
	for (l = 0; l < 40; l++)
	    if (start[l] == '\n' || !start[l])
		break;
	x = (vid.width - l * 8 * scale) / 2;
	for (j = 0; j < l; j++, x += 8 * scale) {
	    Draw_CharacterScaled(x, y, start[j], scale);
	    if (!remaining--)
		return;
	}

	y += 8 * scale;

	while (*start && *start != '\n')
	    start++;

	if (!*start)
	    break;
	start++;		/* skip the \n */
    } while (1);
}

/* ============================================================================ */

/*
====================
CalcFov
====================
*/
static float
CalcFov(float fov_x, float width, float height)
{
    float a;
    float x;

    if (fov_x < 1 || fov_x > 179)
	Sys_Error("Bad fov: %f", fov_x);

    x = width / tan(fov_x / 360 * M_PI);
    a = atan(height / x);
    a = a * 360 / M_PI;

    return a;
}


/*
====================
CalcFovHorPlus

Hor+ widescreen FOV correction (the prboom-plus model).

scr_fov ("fov") is authored against the vanilla 4:3 display, so it
defines the *horizontal* FOV only at 4:3.  Naively feeding that fov_x
straight into a wider framebuffer (or a wider display aspect) and then
letting the frontend stretch the result just fattens the image -- the
vertical FOV shrinks relative to vanilla and nothing new becomes
visible.

Instead we anchor the *vertical* FOV to whatever 4:3 would have shown
for the user's chosen scr_fov, then re-derive a wider horizontal FOV
from that vertical anchor and the selected display aspect.  The result:
vertical framing is identical to vanilla at every aspect, and wider
ratios reveal more of the world to the left and right rather than
distorting it.

  aspect = selected display ratio (e.g. 16/9).  At aspect == 4/3 this
  is a no-op and fov_x == scr_fov, preserving the vanilla projection
  bit-for-bit.

Returns the corrected horizontal FOV in degrees.
====================
*/
static float
CalcFovHorPlus(float fov_x_43, float aspect)
{
    float fov_y, x, fov_x;

    /* Vertical FOV that the authored fov_x would produce at 4:3.
     * Using the canonical 4:3 here (not the framebuffer's real
     * width:height) keeps vertical framing pinned to vanilla even
     * when the user has picked a non-4:3 render resolution. */
    fov_y = CalcFov(fov_x_43, 4.0f, 3.0f);

    /* Re-expand horizontally for the requested aspect.  This is the
     * exact inverse of CalcFov with width/height roles swapped:
     *   x     = height / tan(fov_y/2)
     *   fov_x = 2 * atan((height * aspect) / x)
     * with height factored out it reduces to the form below. */
    x = 1.0f / tan(fov_y / 360.0f * (float)M_PI);
    fov_x = atan(aspect / x) * 360.0f / (float)M_PI;

    if (fov_x < 1.0f)   fov_x = 1.0f;
    if (fov_x > 179.0f) fov_x = 179.0f;

    return fov_x;
}


/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef(void)
{
   vrect_t vrect;
   float size;

   scr_fullupdate = 0;		/* force a background redraw */
   vid.recalc_refdef = 0;

   /* force the status bar to redraw */
   Sbar_Changed();

   /* ======================================== */

   /* bound viewsize / fov.  NaN compares false in any
    * direction, so the < / > tests below silently let NaN
    * pass through to the renderer; CalcFov then divides
    * width by tan(NaN) and stores NaN in r_refdef.fov_y,
    * which propagates through view setup.  Reset to the
    * default first so the subsequent range tests do their
    * normal job. */
   if (IS_NAN(scr_viewsize.value))
      Cvar_Set("viewsize", "100");
   if (IS_NAN(scr_fov.value))
      Cvar_Set("fov", "90");

   /* bound viewsize */
   if (scr_viewsize.value < 30)
      Cvar_Set("viewsize", "30");
   if (scr_viewsize.value > 120)
      Cvar_Set("viewsize", "120");

   /* bound field of view */
   if (scr_fov.value < 10)
      Cvar_Set("fov", "10");
   if (scr_fov.value > 170)
      Cvar_Set("fov", "170");

   /* intermission is always full screen */
   if (cl.intermission)
      size = 120;
   else
      size = scr_viewsize.value;

   /* sb_lines is in physical pixels: scale the logical 0/24/48 by the
    * UI scale so the 3D viewport leaves the right amount of room for
    * the (now-scaled) status bar. The Sbar_Draw* paths multiply the
    * same scale through their drawing logic so the status bar fills
    * exactly this region. */
   {
      int scale = SCR_GetUIScale();
      if (size >= 120)
	 sb_lines = 0;				/* no status bar at all */
      else if (size >= 110)
	 sb_lines = 24 * scale;			/* no inventory */
      else
	 sb_lines = (24 + 16 + 8) * scale;
   }

   /* these calculations mirror those in R_Init() for r_refdef, but take no */
   /* account of water warping */
   vrect.x = 0;
   vrect.y = 0;
   vrect.width = vid.width;
   vrect.height = vid.height;

   R_SetVrect(&vrect, &scr_vrect, sb_lines);

   /* Hor+ widescreen: r_aspect selects a display ratio; widen the
    * horizontal FOV to match it while keeping vertical FOV pinned to
    * the vanilla 4:3 value.  Index 0 (4:3) leaves fov_x == scr_fov,
    * so the default path is unchanged. */
   {
      int   asp_idx = (int)r_aspect.value;
      float aspect  = R_AspectRatioForIndex(asp_idx);

      if (asp_idx <= 0) {
         /* 4:3, vanilla: fov_x authored directly, fov_y follows the
          * real framebuffer dimensions exactly as before. */
         r_refdef.fov_x = scr_fov.value;
         r_refdef.fov_y =
            CalcFov(r_refdef.fov_x, r_refdef.vrect.width,
                    r_refdef.vrect.height);
      } else {
         /* Widen fov_x for the chosen aspect.  fov_y stays anchored to
          * the vanilla 4:3 vertical framing for the authored fov so any
          * consumer of r_refdef.fov_y (and the pixelAspect compensation
          * in R_ViewChanged) sees an unchanged vertical view. */
         r_refdef.fov_x = CalcFovHorPlus(scr_fov.value, aspect);
         r_refdef.fov_y = CalcFov(scr_fov.value, 4.0f, 3.0f);
      }
   }

   /* guard against going from one mode to another that's less than half the */
   /* vertical resolution */
   if (scr_con_current > vid.height)
      scr_con_current = vid.height;

   /* notify the refresh of the change */
   R_ViewChanged(&vrect, sb_lines, vid.aspect);
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
static void
SCR_SizeUp_f(void)
{
    Cvar_SetValue("viewsize", scr_viewsize.value + 10);
    vid.recalc_refdef = 1;
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
static void SCR_SizeDown_f(void)
{
   Cvar_SetValue("viewsize", scr_viewsize.value - 10);
   vid.recalc_refdef = 1;
}

/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque(void)
{
   S_StopAllSounds(true);

   if (cls.state != ca_active)
      return;

   /* redraw with no console and the loading plaque */
   Con_ClearNotify();
   scr_centertime_off = 0;
   scr_con_current = 0;

   scr_drawloading = true;
   scr_fullupdate = 0;
   Sbar_Changed();
   SCR_UpdateScreen();
   scr_drawloading = false;

   scr_disabled_for_loading = true;
   scr_disabled_time = 0;
   scr_fullupdate = 0;
}

/*
==============
SCR_DrawLoading
==============
*/
static void SCR_DrawLoading(void)
{
   const qpic_t *pic;
   int scale = SCR_GetUIScale();

   if (!scr_drawloading)
      return;

   pic = Draw_CachePic("gfx/loading.lmp");
   Draw_PicScaled((vid.width - pic->width * scale) / 2,
         (vid.height - 48 * scale - pic->height * scale) / 2, pic, scale);
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque(void)
{
   scr_disabled_for_loading = false;
   scr_fullupdate = 0;
   Con_ClearNotify();
}

/*
================
SCR_AgeLoadingPlaque

Advance scr_disabled_time by dt. Called once per Host_Frame after
Host_FilterTime, so the safety net stays in realtime-units regardless
of whether SCR_BeginLoadingPlaque has been called recently. The
counter is reset to 0 at every BeginLoadingPlaque, so its long-term
unbounded growth is harmless -- only the < 5 s window after a load
start is ever read.
================
*/
void SCR_AgeLoadingPlaque(float dt)
{
   scr_disabled_time += dt;
}

/* ============================================================================= */

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void
SCR_UpdateScreen(void)
{
   static float old_viewsize, old_fov;
   vrect_t vrect;

   if (scr_skipupdate)
      return;
   if (scr_block_drawing)
      return;

   if (scr_disabled_for_loading) {
      /*
       * FIXME - this really needs to be fixed properly.
       * Simply starting a new game and typing "changelevel foo" will hang
       * the engine for 5s (was 60s!) if foo.bsp does not exist.
       */
      if (scr_disabled_time > 5) {
         scr_disabled_for_loading = false;
         Con_Printf("load failed.\n");
      } else
         return;
   }

   if (cls.state == ca_dedicated)
      return;			/* stdout only */

   if (!scr_initialized || !con_initialized)
      return;			/* not initialized yet */

   scr_copytop = 0;
   scr_copyeverything = 0;

   /*
    * Check for vid setting changes
    */
   if (old_fov != scr_fov.value) {
      old_fov = scr_fov.value;
      vid.recalc_refdef = true;
   }
   if (old_viewsize != scr_viewsize.value) {
      old_viewsize = scr_viewsize.value;
      vid.recalc_refdef = true;
   }

   if (vid.recalc_refdef)
      SCR_CalcRefdef();

   /*
    * do 3D refresh drawing, and then update the screen
    */

   if (scr_fullupdate++ < vid.numpages) {
      /* Phase 4n: tile only the wood-grain border strips
       * around the 3D viewport, not the full screen.
       * Phase 4m bumped vid.numpages high so this branch
       * runs every frame; the original full-screen
       * Draw_TileClear painted ~1.2 MB at 1280x960 only
       * to have R_RenderView immediately overwrite the
       * 3D viewport pixels and Sbar_Draw immediately
       * overwrite the status-bar strip.  The pixels that
       * actually need the tile are the ones outside the
       * 3D viewport (scr_vrect) -- paint just those.  At
       * scr_viewsize 100 (default, viewport spans full
       * width and stops above the status bar) the
       * top/left/right strips collapse to zero-width and
       * only the below-viewport strip remains; at smaller
       * viewsizes the left/right/top strips kick in.
       *
       * scr_vrect is valid here: SCR_CalcRefdef has
       * already run at line 670 because vid.recalc_refdef
       * triggers on the first frame via old_fov /
       * old_viewsize being zero-initialised and not
       * matching the cvar defaults.  After SCR_CalcRefdef
       * the rect describes a viewport that is fully on-
       * screen (R_SetVrect clamps), so the below
       * comparisons against vid.width / vid.height are
       * sound. */
      int right_x;
      int below_y;

      scr_copyeverything = 1;

      if (scr_vrect.y > 0)
         Draw_TileClear(0, 0, vid.width, scr_vrect.y);

      if (scr_vrect.x > 0)
         Draw_TileClear(0, scr_vrect.y,
                        scr_vrect.x, scr_vrect.height);

      right_x = scr_vrect.x + scr_vrect.width;
      if (right_x < vid.width)
         Draw_TileClear(right_x, scr_vrect.y,
                        vid.width - right_x, scr_vrect.height);

      below_y = scr_vrect.y + scr_vrect.height;
      if (below_y < vid.height)
         Draw_TileClear(0, below_y,
                        vid.width, vid.height - below_y);

      Sbar_Changed();
   }
   pconupdate = NULL;
   SCR_SetUpToDrawConsole();
   SCR_EraseCenterString();

   perf_timing_section_begin(PERF_SECTION_HOST_RV);
   V_RenderView();
   perf_timing_section_end(PERF_SECTION_HOST_RV);

   perf_timing_section_begin(PERF_SECTION_HOST_2D);
   if (scr_drawloading) {
      SCR_DrawLoading();
      Sbar_Draw();
   } else if (cl.intermission == 1 && key_dest == key_game) {
      Sbar_IntermissionOverlay();
   } else if (cl.intermission == 2 && key_dest == key_game) {
      Sbar_FinaleOverlay();
      SCR_DrawCenterString();
   } else if (cl.intermission == 3 && key_dest == key_game) {
      SCR_DrawCenterString();
   } else {
      SCR_DrawNet();
      SCR_DrawCenterString();
      Sbar_Draw();
      SCR_DrawConsole();
      M_Draw();
   }

   if (pconupdate)
      D_UpdateRects(pconupdate);

   V_UpdatePalette();

   /*
    * update one of three areas
    */
   if (scr_copyeverything)
   {
      vrect.x = 0;
      vrect.y = 0;
      vrect.width = vid.width;
      vrect.height = vid.height;
   }
   else if (scr_copytop)
   {
      vrect.x = 0;
      vrect.y = 0;
      vrect.width = vid.width;
      vrect.height = vid.height - sb_lines;
   }
   else
   {
      vrect.x = scr_vrect.x;
      vrect.y = scr_vrect.y;
      vrect.width = scr_vrect.width;
      vrect.height = scr_vrect.height;
   }
   vrect.pnext = 0;
   VID_Update(&vrect);
   perf_timing_section_end(PERF_SECTION_HOST_2D);
}

/* ============================================================================= */

/*
==================
SCR_Init
==================
*/
void
SCR_Init(void)
{
    Cvar_RegisterVariable(&scr_fov);
    Cvar_RegisterVariable(&scr_viewsize);
    Cvar_RegisterVariable(&scr_conspeed);
    Cvar_RegisterVariable(&scr_centertime);
    Cvar_RegisterVariable(&scr_printspeed);
    Cvar_RegisterVariable(&scr_uiscale);

    Cmd_AddCommand("sizeup", SCR_SizeUp_f);
    Cmd_AddCommand("sizedown", SCR_SizeDown_f);

    scr_ram = (qpic_t*)Draw_PicFromWad("ram");
    scr_net = (qpic_t*)Draw_PicFromWad("net");

    scr_initialized = true;
}
