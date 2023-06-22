/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include "Gfx_1.3.h"
#include "Util.h"
#include "3dmath.h"
#include "Combine.h"
#include "TexCache.h"
#include "CRC.h"
#include "Framebuffer_glide64.h"
#include "Glide64_Ini.h"
#include "GlideExtensions.h"
#include <libretro.h>

#include "../../../Graphics/RDP/gDP_funcs_C.h"

extern void CRC_BuildTable();
extern retro_log_printf_t log_cb;
extern uint32_t screen_aspectmodehint;

#if defined(__GNUC__)
#include <sys/time.h>
#elif defined(__MSC__)
#include <time.h>
#define PATH_MAX MAX_PATH
#endif

#define G64_VERSION "G64 Mk2"
#define RELTIME "Date: " __DATE__// " Time: " __TIME__

int romopen = false;
int exception = false;

/* custom macros made up by cxd4 for tracking the system type better */
#define OS_TV_TYPE_PAL          0
#define OS_TV_TYPE_NTSC         1
#define OS_TV_TYPE_MPAL         2
unsigned int region;

// ref rate
// 60=0x0, 70=0x1, 72=0x2, 75=0x3, 80=0x4, 90=0x5, 100=0x6, 85=0x7, 120=0x8, none=0xff

uint32_t BMASK = 0x7FFFFF;
// Reality display processor structure
struct RDP rdp;

SETTINGS settings = { false, 640, 480, 0, 0 };

VOODOO voodoo = {0, 0};

uint32_t   offset_textures = 0;

// SOME FUNCTION DEFINITIONS

void glide_set_filtering(unsigned value);

static void (*l_DebugCallback)(void *, int, const char *) = NULL;
static void *l_DebugCallContext = NULL;

static void _ChangeSize(void)
{
   float res_scl_y;
   uint32_t dwHStartReg, dwVStartReg;
   uint32_t hstart, hend, vstart, vend, scale_y, scale_x;
   float fscale_x       = 0.0;
   float fscale_y       = 0.0;
   float aspect         = 0.0;
   if (!gfx_info.VI_H_START_REG)
      return;

   res_scl_y      = (float)settings.res_y / 240.0f;
   dwHStartReg = *gfx_info.VI_H_START_REG;
   dwVStartReg = *gfx_info.VI_V_START_REG;
   hstart      = dwHStartReg >> 16;
   hend        = dwHStartReg & 0xFFFF;
   vstart      = dwVStartReg >> 16;
   vend        = dwVStartReg & 0xFFFF;
   scale_x     = *gfx_info.VI_X_SCALE_REG & 0xFFF;
   scale_y     = *gfx_info.VI_Y_SCALE_REG & 0xFFF;

   if (!scale_x || !scale_y)
      return;

   fscale_x             = (float)scale_x / 1024.0f;
   fscale_y             = (float)scale_y / 2048.0f;

   // dunno... but sometimes this happens
   if (hend == hstart)
      hend              = (int)(*gfx_info.VI_WIDTH_REG / fscale_x);

   rdp.vi_width         = (hend - hstart) * fscale_x;
   rdp.vi_height        = (vend - vstart) * fscale_y * 1.0126582f;
   aspect               = (settings.adjust_aspect && (fscale_y > fscale_x) && (rdp.vi_width > rdp.vi_height)) ? fscale_x/fscale_y : 1.0f;
   rdp.scale_x          = (float)settings.res_x / rdp.vi_width;

   if (region != OS_TV_TYPE_NTSC && settings.pal230)
   {
      // odd... but pal games seem to want 230 as height...
      rdp.scale_y = res_scl_y * (230.0f / rdp.vi_height)  * aspect;
   }
   else
   {
      rdp.scale_y = (float)settings.res_y / rdp.vi_height * aspect;
   }

   rdp.offset_y   = ((float)settings.res_y - rdp.vi_height * rdp.scale_y) * 0.5f;
   if (((uint32_t)rdp.vi_width <= (*gfx_info.VI_WIDTH_REG)/2) && (rdp.vi_width > rdp.vi_height))
      rdp.scale_y *= 0.5f;

   gDPSetScissor(
         0,                         /* mode */
         0,                         /* ulx  */
         0,                         /* uly  */
         (uint32_t)rdp.vi_width,    /* lrx  */
         (uint32_t)rdp.vi_height    /* lry  */
         );

   g_gdp.flags |= UPDATE_VIEWPORT | UPDATE_SCISSOR;
}

void ChangeSize(void)
{
   float offset_y;

   switch (screen_aspectmodehint)
   {
      case 0: //4:3
         if (settings.scr_res_x >= settings.scr_res_y * 4.0f / 3.0f)
         {
            settings.res_y = settings.scr_res_y;
            settings.res_x = (uint32_t)(settings.res_y * 4.0f / 3.0f);
         }
         else
         {
            settings.res_x = settings.scr_res_x;
            settings.res_y = (uint32_t)(settings.res_x / 4.0f * 3.0f);
         }
         break;
      case 1: //16:9
         if (settings.scr_res_x >= settings.scr_res_y * 16.0f / 9.0f)
         {
            settings.res_y = settings.scr_res_y;
            settings.res_x = (uint32_t)(settings.res_y * 16.0f / 9.0f);
         }
         else
         {
            settings.res_x = settings.scr_res_x;
            settings.res_y = (uint32_t)(settings.res_x / 16.0f * 9.0f);
         }
         break;
      default: //stretch or original
         settings.res_x = settings.scr_res_x;
         settings.res_y = settings.scr_res_y;
   }
   _ChangeSize ();
   rdp.offset_x = (settings.scr_res_x - settings.res_x) / 2.0f;
   offset_y = (settings.scr_res_y - settings.res_y) / 2.0f;
   settings.res_x += (uint32_t)rdp.offset_x;
   settings.res_y += (uint32_t)offset_y;
   rdp.offset_y += offset_y;
   if (settings.aspectmode == 3) // original
   {
      rdp.scale_x = rdp.scale_y = 1.0f;
      rdp.offset_x = (settings.scr_res_x - rdp.vi_width) / 2.0f;
      rdp.offset_y = (settings.scr_res_y - rdp.vi_height) / 2.0f;
   }
   //	settings.res_x = settings.scr_res_x;
   //	settings.res_y = settings.scr_res_y;
}

void WriteLog(m64p_msg_level level, const char *msg, ...)
{
#ifdef DEBUG_GLIDE2GL
   char buf[1024];
   va_list args;
   va_start(args, msg);
   vsnprintf(buf, 1023, msg, args);
   buf[1023]='\0';
   va_end(args);
   if (l_DebugCallback)
   {
      l_DebugCallback(l_DebugCallContext, level, buf);
   }
   else
      fprintf(stdout, buf);
#endif
}

int GetTexAddrUMA(int tmu, int texsize)
{
   int addr;

   addr = voodoo.tmem_ptr[0];
   voodoo.tmem_ptr[0] += texsize;
   voodoo.tmem_ptr[1] = voodoo.tmem_ptr[0];
   return addr;
}

void guLoadTextures(void)
{
   int tbuf_size = 0;

   bool log2_2048 = (settings.scr_res_x > 1024) ? true : false;

   tbuf_size = grTexCalcMemRequired(log2_2048 ? GR_LOD_LOG2_2048 : GR_LOD_LOG2_1024,
         GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565);

   offset_textures = tbuf_size + 16;
}

int glide64InitGfx(void)
{
   rdp_reset ();

   if (!grSstWinOpen())
   {
      ERRLOG("Error setting display mode");
      return false;
   }

   // get the # of TMUs available
   voodoo.tex_max_addr = grTexMaxAddress(GR_TMU0);

   grStipplePattern(settings.stipple_pattern);

   InitCombine();

   if (settings.fog)
   {
      guFogGenerateLinear(0.0f, 255.0f);
   }
   else
      settings.fog = false;

   grDepthBufferMode (GR_DEPTHBUFFER_ZBUFFER);
   grDepthBufferFunction(GR_CMP_LESS);
   grDepthMask(FXTRUE);

   settings.res_x = settings.scr_res_x;
   settings.res_y = settings.scr_res_y;
   ChangeSize();

   guLoadTextures ();
   ClearCache ();

   return true;
}

void ReleaseGfx(void)
{
   VLOG("ReleaseGfx ()\n");

   grSstWinClose (0);

   rdp_free();
}

// new API code begins here!

void glide64ReadScreen2(void *dest, int *width, int *height, int front)
{
   GrLfbInfo_t info;
   uint8_t *line = (uint8_t*)dest;

   *width = settings.res_x;
   *height = settings.res_y;

   if (!line)
      return;

   info.size = sizeof(GrLfbInfo_t);
   if (grLfbLock (GR_LFB_READ_ONLY,
            GR_BUFFER_FRONTBUFFER,
            GR_LFBWRITEMODE_888,
            GR_ORIGIN_UPPER_LEFT,
            FXFALSE,
            &info))
   {
      uint32_t y, x;
      // Copy the screen, let's hope this works.
      for (y = 0; y < settings.res_y; y++)
      {
         uint8_t *ptr = (uint8_t*) info.lfbPtr + (info.strideInBytes * y);
         for (x = 0; x < settings.res_x; x++)
         {
            line[x*3]   = ptr[2];  // red
            line[x*3+1] = ptr[1];  // green
            line[x*3+2] = ptr[0];  // blue
            ptr += 4;
         }
         line += settings.res_x * 3;
      }

      // Unlock the frontbuffer
      grLfbUnlock (GR_LFB_READ_ONLY, GR_BUFFER_FRONTBUFFER);
   }
}

m64p_error glide64PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                   void (*DebugCallback)(void *, int, const char *))
{
   l_DebugCallback = DebugCallback;
   l_DebugCallContext = Context;

   ReadSettings();
   return M64ERR_SUCCESS;
}

m64p_error glide64PluginShutdown(void)
{
   return M64ERR_SUCCESS;
}

m64p_error glide64PluginGetVersion(m64p_plugin_type *PluginType,
      int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
   /* set version info */
   if (PluginType != NULL)
      *PluginType = M64PLUGIN_GFX;

   if (PluginVersion != NULL)
      *PluginVersion = 0x016304;

   if (APIVersion != NULL)
      *APIVersion = VIDEO_PLUGIN_API_VERSION;

   if (PluginNamePtr != NULL)
      *PluginNamePtr = "Glide64mk2 Video Plugin";

   if (Capabilities != NULL)
      *Capabilities = 0;

   return M64ERR_SUCCESS;
}

/******************************************************************
Function: ChangeWindow
Purpose:  to change the window between fullscreen and window
mode. If the window was in fullscreen this should
change the screen to window mode and vice vesa.
input:    none
output:   none
*******************************************************************/
//#warning ChangeWindow unimplemented
void glide64ChangeWindow (void)
{
}

/******************************************************************
Function: CloseDLL
Purpose:  This function is called when the emulator is closing
down allowing the dll to de-initialise.
input:    none
output:   none
*******************************************************************/
void glide64CloseDLL (void)
{
   ZLUT_release();
   ClearCache ();
}

/******************************************************************
Function: DrawScreen
Purpose:  This function is called when the emulator receives a
WM_PAINT message. This allows the gfx to fit in when
it is being used in the desktop.
input:    none
output:   none
*******************************************************************/
void glide64DrawScreen (void)
{
}

/******************************************************************
Function: GetDllInfo
Purpose:  This function allows the emulator to gather information
about the dll by filling in the PluginInfo structure.
input:    a pointer to a PLUGIN_INFO stucture that needs to be
filled by the function. (see def above)
output:   none
*******************************************************************/
void glide64GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
   PluginInfo->Version = 0x0103;     // Set to 0x0103
   PluginInfo->Type  = PLUGIN_TYPE_GFX;  // Set to PLUGIN_TYPE_GFX
   sprintf (PluginInfo->Name, "Glide64mk2 "G64_VERSION RELTIME);  // Name of the DLL

   // If DLL supports memory these memory options then set them to TRUE or FALSE
   //  if it does not support it
   PluginInfo->NormalMemory = true;  // a normal uint8_t array
   PluginInfo->MemoryBswaped = true; // a normal uint8_t array where the memory has been pre
   // bswap on a dword (32 bits) boundry
}

/******************************************************************
Function: InitiateGFX
Purpose:  This function is called when the DLL is started to give
information from the emulator that the n64 graphics
uses. This is not called from the emulation thread.
Input:    Gfx_Info is passed to this function which is defined
above.
Output:   TRUE on success
FALSE on failure to initialise

** note on interrupts **:
To generate an interrupt set the appropriate bit in MI_INTR_REG
and then call the function CheckInterrupts to tell the emulator
that there is a waiting interrupt.
*******************************************************************/

int glide64InitiateGFX (GFX_INFO Gfx_Info)
{
   char name[21] = "DEFAULT";

   rdp_new();

   // Assume scale of 1 for debug purposes
   rdp.scale_x = 1.0f;
   rdp.scale_y = 1.0f;

   memset (&settings, 0, sizeof(SETTINGS));
   ReadSettings ();
   ReadSpecialSettings (name);

   math_init ();
   TexCacheInit ();
   CRC_BuildTable();
   CountCombine();
   if (fb_depth_render_enabled)
      ZLUT_init();

   return true;
}

/******************************************************************
Function: MoveScreen
Purpose:  This function is called in response to the emulator
receiving a WM_MOVE passing the xpos and ypos passed
from that message.
input:    xpos - the x-coordinate of the upper-left corner of the
client area of the window.
ypos - y-coordinate of the upper-left corner of the
client area of the window.
output:   none
*******************************************************************/
void glide64MoveScreen (int xpos, int ypos)
{
}

/******************************************************************
Function: RomClosed
Purpose:  This function is called when a rom is closed.
input:    none
output:   none
*******************************************************************/
void glide64RomClosed (void)
{
   romopen = false;
   ReleaseGfx ();
}

static void CheckDRAMSize(void)
{
   uint32_t test = gfx_info.RDRAM[0x007FFFFF] + 1;
   if (test)
      BMASK = 0x7FFFFF;
   else
      BMASK = 0x3FFFFF;

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "Detected RDRAM size: %08lx\n", BMASK);
}

/******************************************************************
Function: RomOpen
Purpose:  This function is called when a rom is open. (from the
emulation thread)
input:    none
output:   none
*******************************************************************/
int glide64RomOpen (void)
{
   int i;
   char name[21] = "DEFAULT";

   no_dlist = true;
   romopen = true;
   ucode_error_report = true;	// allowed to report ucode errors
   rdp_reset ();

   /* cxd4 -- Glide64 tries to predict PAL scaling based on the ROM header. */
   region = OS_TV_TYPE_NTSC; /* Invalid region codes are probably NTSC betas. */
   switch (gfx_info.HEADER[BYTE4_XOR_BE(0x3E)])
   {
   case 'A': /* generic NTSC, not documented, used by 1080 Snowboarding */
      region = OS_TV_TYPE_NTSC; break;
   case 'B': /* Brazilian */
      region = OS_TV_TYPE_MPAL; break;
   case 'C': /* Chinese */
      region = OS_TV_TYPE_NTSC; break;
   case 'D': /* German */
      region = OS_TV_TYPE_PAL ; break;
   case 'E': /* North America */
      region = OS_TV_TYPE_NTSC; break;
   case 'F': /* French */
      region = OS_TV_TYPE_PAL ; break;
   case 'G': /* Gateway 64 (NTSC) */
      region = OS_TV_TYPE_NTSC; break;
   case 'H': /* Dutch */
      region = OS_TV_TYPE_PAL ; break;
   case 'I': /* Italian */
      region = OS_TV_TYPE_PAL ; break;
   case 'J': /* Japanese */
      region = OS_TV_TYPE_NTSC; break;
   case 'K': /* Korean */
      region = OS_TV_TYPE_NTSC; break;
   case 'L': /* Gateway 64 (PAL) */
      region = OS_TV_TYPE_PAL ; break;
   case 'N': /* Canadian */
      region = OS_TV_TYPE_NTSC; break;
   case 'P': /* European (basic spec.) */
      region = OS_TV_TYPE_PAL ; break;
   case 'S': /* Spanish */
      region = OS_TV_TYPE_PAL ; break;
   case 'U': /* Australian */
      region = OS_TV_TYPE_PAL ; break;
   case 'W': /* Scandinavian */
      region = OS_TV_TYPE_PAL ; break;
   case 'X': case 'Y': case 'Z': /* documented "others", always PAL I think? */
      region = OS_TV_TYPE_PAL ; break;
   }

   ReadSpecialSettings (name);

   // get the name of the ROM
   for (i = 0; i < 20; i++)
      name[i] = gfx_info.HEADER[(32+i)^3];
   name[20] = 0;

   // remove all trailing spaces
   while (name[strlen(name)-1] == ' ')
      name[strlen(name)-1] = 0;

   strncpy(rdp.RomName, name, sizeof(name));
   ReadSpecialSettings (name);
   ClearCache ();

   CheckDRAMSize();

   OPEN_RDP_LOG ();
   OPEN_RDP_E_LOG ();

   glide64InitGfx ();
   rdp_setfuncs();

   // **
   return true;
}

/******************************************************************
Function: ShowCFB
Purpose:  Useally once Dlists are started being displayed, cfb is
ignored. This function tells the dll to start displaying
them again.
input:    none
output:   none
*******************************************************************/
bool no_dlist = true;

void glide64ShowCFB (void)
{
   no_dlist = true;
}

void glide64SetRenderingCallback(void (*callback)(int))
{
}

/******************************************************************
Function: UpdateScreen
Purpose:  This function is called in response to a vsync of the
screen were the VI bit in MI_INTR_REG has already been
set
input:    none
output:   none
*******************************************************************/
uint32_t update_screen_count = 0;

int retro_return(bool a);

void glide64UpdateScreen (void)
{
   bool forced_update = false;
   uint32_t width = (*gfx_info.VI_WIDTH_REG) << 1;

   if (*gfx_info.VI_ORIGIN_REG  > width)
      update_screen_count++;

   if (
         (settings.frame_buffer & fb_cpu_write_hack) &&
         (update_screen_count > ((settings.hacks&hack_Lego) ? 15U : 30U)) &&
         (rdp.last_bg == 0)
      )
   {
      //DirectCPUWrite hack
      update_screen_count = 0;
      no_dlist = true;
      ClearCache ();
      width = (*gfx_info.VI_WIDTH_REG) << 1;
      if (*gfx_info.VI_ORIGIN_REG  > width)
         update_screen_count++;
   }

   if( no_dlist )
   {
      if( *gfx_info.VI_ORIGIN_REG  > width )
      {
         ChangeSize();
         LRDP("ChangeSize done\n");
         if (rdp.vi_height != 0)
            drawViRegBG();
         rdp.updatescreen = 1;
         forced_update = true;
      }
      else
         return;
   }

   if (settings.swapmode == 0 || forced_update)
      newSwapBuffers ();

   if (settings.swapmode_retro && BUFFERSWAP)
      retro_return(true);
}

uint32_t curframe = 0;
extern int need_to_compile;
void glide_set_filtering(unsigned value)
{
	if(settings.filtering != value)
   {
		need_to_compile = 1;
		settings.filtering = value;
	}
}

void newSwapBuffers(void)
{
   if (!rdp.updatescreen)
      return;

   rdp.updatescreen = 0;

   g_gdp.flags |= UPDATE_SCISSOR | UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
   grClipWindow (0, 0, settings.scr_res_x, settings.scr_res_y);
   grDepthBufferFunction (GR_CMP_ALWAYS);
   grDepthMask (FXFALSE);

   if (settings.frame_buffer & fb_read_back_to_screen)
      DrawWholeFrameBufferToScreen();

   {
      grBufferSwap (settings.vsync);

      if  (settings.buff_clear || (settings.hacks & hack_PPL && settings.ucode == 6))
      {
         grDepthMask (FXTRUE);
         grBufferClear (0, 0, 0xFFFF);
      }
   }

   if (settings.frame_buffer & fb_read_back_to_screen2)
      DrawWholeFrameBufferToScreen();

   frame_count ++;
}

/******************************************************************
Function: ViStatusChanged
Purpose:  This function is called to notify the dll that the
ViStatus registers value has been changed.
input:    none
output:   none
*******************************************************************/
void glide64ViStatusChanged(void)
{
}

/******************************************************************
Function: ViWidthChanged
Purpose:  This function is called to notify the dll that the
ViWidth registers value has been changed.
input:    none
output:   none
*******************************************************************/
void  glide64ViWidthChanged(void)
{
}
