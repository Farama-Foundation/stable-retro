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
/* console.c */

#include "compat/strl.h"
#include <string.h>

#include "client.h"
#include "cmd.h"
#include "console.h"
#include "draw.h"
#include "keys.h"
#include "quakedef.h"
#include "screen.h"
#include "sys.h"
#include "zone.h"

#include "host.h"
#include "sound.h"

#define CON_TEXTSIZE 16384
#define	NUM_CON_TIMES 4

console_t *con;			/* point to current console */
static console_t con_main;

int con_ormask = 0;
qboolean con_forcedup;
int con_totallines;		/* total lines in console scrollback */
int con_notifylines;		/* scan lines to clear for notify lines */

static int con_linewidth;	/* characters across screen */
static int con_vislines;

int
Con_GetWidth(void)
{
    return con_linewidth;
}

static float con_cursorspeed = 4;
static cvar_t con_notifytime = { "con_notifytime", "3" };	/* seconds */

/* Elapsed time (in seconds) since each notify slot was last printed
 * to. Set to 0 by Con_Print and aged by host_frametime each Host_Frame
 * via Con_AgeNotifyTimes; once an entry exceeds con_notifytime.value
 * the line is no longer drawn. Con_ClearNotify fills with the
 * CON_NOTIFY_NEVER sentinel rather than 0 so cleared/never-printed
 * slots stay above the notifytime threshold (and stay stable under
 * the per-frame increment, since fp32 saturates the addition). */
#define CON_NOTIFY_NEVER 1.0e9f
static float con_times[NUM_CON_TIMES];

qboolean con_initialized;

/*
====================
Con_ToggleConsole_f
====================
*/
void
Con_ToggleConsole_f(void)
{
    Key_ClearTyping();

    if (key_dest == key_console) {
	if (!con_forcedup) {
	    key_dest = key_game;
	    Key_ClearTyping();
	}
    } else
	key_dest = key_console;

    Con_ClearNotify();
}

/*
================
Con_Clear_f
================
*/
void
Con_Clear_f(void)
{
    memset(con_main.text, ' ', CON_TEXTSIZE);
}


/*
================
Con_ClearNotify
================
*/
void
Con_ClearNotify(void)
{
    int i;

    for (i = 0; i < NUM_CON_TIMES; i++)
	con_times[i] = CON_NOTIFY_NEVER;
}


/*
================
Con_AgeNotifyTimes

Advance every notify slot's elapsed-since-print counter by dt. Called
once per Host_Frame after Host_FilterTime, so the per-frame delta
matches the realtime delta the original code's (realtime - snapshot)
math used to recover. The CON_NOTIFY_NEVER sentinel is large enough
that fp32 addition of host_frametime rounds away to a no-op, so
cleared/never-printed slots stay above the notifytime threshold.
================
*/
void
Con_AgeNotifyTimes(float dt)
{
    int i;

    for (i = 0; i < NUM_CON_TIMES; i++)
	con_times[i] += dt;
}


/*
================
Con_MessageMode_f
================
*/
static void
Con_MessageMode_f(void)
{
    key_dest = key_message;
    chat_team = false;
}

/*
================
Con_MessageMode2_f
================
*/
static void
Con_MessageMode2_f(void)
{
    key_dest = key_message;
    chat_team = true;
}

/*
================
Con_Resize

================
*/
static void Con_Resize(console_t * c)
{
   int width;
   int scale = SCR_GetUIScale();
   char tbuf[CON_TEXTSIZE];

   /* The console grid is in scaled-character cells: each cell is 8*scale
    * physical pixels wide. We subtract 2 cells of side margin, matching
    * the original 1996 layout. */
   width = (vid.width / (8 * scale)) - 2;

   if (width == con_linewidth)
      return;

   if (width < 1)		/* video hasn't been initialized yet */
   {
      width = 38;
      con_linewidth = width;
      con_totallines = CON_TEXTSIZE / con_linewidth;
      memset(c->text, ' ', CON_TEXTSIZE);
   }
   else
   {
      int i, j, numlines, numchars;
      int oldwidth = con_linewidth;
      int oldtotallines = con_totallines;

      con_linewidth  = width;
      con_totallines = CON_TEXTSIZE / con_linewidth;
      numlines = oldtotallines;

      if (con_totallines < numlines)
         numlines = con_totallines;

      numchars = oldwidth;

      if (con_linewidth < numchars)
         numchars = con_linewidth;

      memcpy(tbuf, c->text, CON_TEXTSIZE);
      memset(c->text, ' ', CON_TEXTSIZE);

      for (i = 0; i < numlines; i++)
      {
         for (j = 0; j < numchars; j++)
         {
            c->text[(con_totallines - 1 - i) * con_linewidth + j] =
               tbuf[((c->current - i + oldtotallines) %
                     oldtotallines) * oldwidth + j];
         }
      }
      Con_ClearNotify();
   }

   c->current = con_totallines - 1;
   c->display = c->current;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void
Con_CheckResize(void)
{
    Con_Resize(&con_main);
}

/*
===============
Con_Linefeed
===============
*/
static void
Con_Linefeed(void)
{
    con->x = 0;
    if (con->display == con->current)
	con->display++;
    con->current++;
    memset(&con->text[(con->current % con_totallines) * con_linewidth]
	   , ' ', con_linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void
Con_Print(const char *txt)
{
    int y;
    int c, l;
    static int cr;
    int mask;

    if (txt[0] == 1 || txt[0] == 2) {
	mask = 128;		/* go to colored text */
	txt++;
	if (txt[0] == 1)
	    S_LocalSound("misc/talk.wav");	/* play talk wav */
    } else
	mask = 0;

    while ((c = *txt)) {
	/* count word length */
	for (l = 0; l < con_linewidth; l++)
	    if (txt[l] <= ' ')
		break;

	/* word wrap */
	if (l != con_linewidth && (con->x + l > con_linewidth))
	    con->x = 0;

	txt++;

	if (cr) {
	    con->current--;
	    cr = false;
	}


	if (!con->x) {
	    Con_Linefeed();
	    /* mark time for transparent overlay */
	    if (con->current >= 0)
		con_times[con->current % NUM_CON_TIMES] = 0;
	}

	switch (c) {
	case '\n':
	    con->x = 0;
	    break;

	case '\r':
	    con->x = 0;
	    cr = 1;
	    break;

	default:		/* display character and advance */
	    y = con->current % con_totallines;
	    con->text[y * con_linewidth + con->x] = c | mask | con_ormask;
	    con->x++;
	    if (con->x >= con_linewidth)
		con->x = 0;
	    break;
	}
    }
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf(const char *fmt, ...)
{
   va_list argptr;
   char msg[MAX_PRINTMSG];

   va_start(argptr, fmt);
   vsnprintf(msg, sizeof(msg), fmt, argptr);
   va_end(argptr);

   /* also echo to debugging console */
   Sys_Printf("%s", msg);	/* also echo to debugging console */

   if (!con_initialized)
      return;

   if (cls.state == ca_dedicated)
      return;			/* no graphics mode */

   /* write it to the scrollable buffer */
   Con_Print(msg);

   /*
    * FIXME - not sure if this is ok, need to rework the screen update
    * criteria so it gets done once per frame unless explicitly flushed. For
    * now, don't update until we see a newline char.
    */
   if (!strchr(msg, '\n'))
      return;

   /* update the screen immediately if the console is displayed */
   if (cls.state != ca_active && !scr_disabled_for_loading)
      {
         static qboolean inupdate;
         /* protect against infinite loop if something in SCR_UpdateScreen calls */
         /* Con_Printd */
         if (!inupdate)
         {
            inupdate = true;
            SCR_UpdateScreen();
            inupdate = false;
         }
      }
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void
Con_DPrintf(const char *fmt, ...)
{
    va_list argptr;
    char msg[MAX_PRINTMSG];

    if (!developer.value)
	return;

    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Con_Printf("%s", msg);
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
static void
Con_DrawInput(void)
{
    int y;
    int i;
    char *text;
    int scale = SCR_GetUIScale();

    if (key_dest != key_console && !con_forcedup)
	return;			/* don't draw anything */

    text = key_lines[edit_line];

/* add the cursor frame */
    text[key_linepos] = 10 + ((int)(realtime * con_cursorspeed) & 1);

/* fill out remainder with spaces */
    for (i = key_linepos + 1; i < con_linewidth; i++)
	text[i] = ' ';

/*      prestep if horizontally scrolling */
    if (key_linepos >= con_linewidth)
	text += 1 + key_linepos - con_linewidth;

/* draw it */
    y = con_vislines - 22 * scale;
    for (i = 0; i < con_linewidth; i++)
	Draw_CharacterScaled(((i + 1) << 3) * scale, y, text[i], scale);

/* remove cursor */
    key_lines[edit_line][key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify(void)
{
   int i, x;
   char *text;
   float time;
   char *s;
   int v = 0;
   int scale = SCR_GetUIScale();

   for (i = con->current - NUM_CON_TIMES + 1; i <= con->current; i++)
   {
      if (i < 0)
         continue;
      /* Under countdown semantics con_times[] stores elapsed-since-
       * print directly, so no realtime subtraction is needed. The
       * single > con_notifytime test covers both 'too old' and
       * 'never logged' (the CON_NOTIFY_NEVER sentinel is far larger
       * than any plausible con_notifytime). */
      time = con_times[i % NUM_CON_TIMES];
      if (time > con_notifytime.value)
         continue;
      text = con->text + (i % con_totallines) * con_linewidth;

      clearnotify = 0;
      scr_copytop = 1;

      for (x = 0; x < con_linewidth; x++)
         Draw_CharacterScaled(((x + 1) << 3) * scale, v, text[x], scale);

      v += 8 * scale;
   }


   if (key_dest == key_message)
   {
      int skip;

      clearnotify = 0;
      scr_copytop = 1;

      if (chat_team)
      {
         Draw_StringScaled(8 * scale, v, "say_team:", scale);
         skip = 11;
      }
      else
      {
         Draw_StringScaled(8 * scale, v, "say:", scale);
         skip = 6;
      }

      s = chat_buffer;
      /* FIXME = Truncating? should be while, not if? */
      if (chat_bufferlen > (int)((vid.width / (8 * scale)) - (skip + 1)))
         s += chat_bufferlen - ((vid.width / (8 * scale)) - (skip + 1));

      x = 0;
      while (s[x])
      {
         Draw_CharacterScaled(((x + skip) << 3) * scale, v, s[x], scale);
         x++;
      }
      Draw_CharacterScaled(((x + skip) << 3) * scale, v,
            10 + ((int)(realtime * con_cursorspeed) & 1), scale);
      v += 8 * scale;
   }

   if (v > con_notifylines)
      con_notifylines = v;
}

static void
Con_DrawDLBar(void)
{
}

/*
================
Con_DrawConsole

Draws the console with the solid background
FIXME - The input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole(int lines)
{
   int i, x, y;
   int rows;
   char *text;
   int row;
   int scale = SCR_GetUIScale();
   int cellh = 8 * scale;	/* physical pixels per character row */

   if (lines <= 0)
      return;

   /* When the menu is open M_Draw is about to call
    * Draw_ConsoleBackground(vid.height) and a chain of
    * M_DrawTransPic / M_DrawPic / Draw_FadeScreen that
    * cover everything we'd lay down here.  Skipping the
    * body is a benign optimization on the SW path (a
    * couple hundred small memcpys avoided per frame while
    * the console scrolls past during the open-the-menu-
    * early window) and a correctness fix on the Vulkan
    * path.  The Phase 4o Draw_Character intercept queues
    * each glyph as an overlay quad; the SW
    * Draw_ConsoleBackground that M_Draw issues right
    * after us writes vid.buffer directly (no intercept,
    * just a memcpy of the conback pic over the whole
    * screen) so it covers the SW conback / chars in
    * vid.buffer, but it can't reach into the overlay
    * queue to mask out the already-queued char quads.
    * Those quads then render on top of the compute-shaded
    * vid.buffer the menu just painted, and the user sees
    * console text bleeding through the main-menu items.
    * Returning here keeps the overlay queue free of the
    * console glyphs in the first place.
    *
    * (When more SW-direct paths -- Draw_FadeScreen,
    * Draw_Fill, Draw_ConsoleBackground itself -- get
    * Vulkan intercepts in a future phase, this kind of
    * vid.buffer-vs-overlay-queue ordering bug will go
    * away wholesale; for now, suppressing at the source
    * is the cheapest correct fix.) */
   if (key_dest == key_menu)
      return;

   /* draw the background */
   Draw_ConsoleBackground(lines);

   /* draw the text. 'lines' is in physical pixels (it comes from
    * scr_con_current). The grid is in scale-aware character cells. */
   con_vislines = lines;
   rows = (lines - 22 * scale) / cellh;	/* rows of text to draw */
   y = lines - 30 * scale;

   /* draw from the bottom up */
   if (con->display != con->current) {
      /* draw arrows to show the buffer is backscrolled */
      for (x = 0; x < con_linewidth; x += 4)
         Draw_CharacterScaled(((x + 1) << 3) * scale, y, '^', scale);
      y -= cellh;
      rows--;
   }

   row = con->display;
   for (i = 0; i < rows; i++, y -= cellh, row--) {
      if (row < 0)
         break;
      if (con->current - row >= con_totallines)
         break;		/* past scrollback wrap point */

      text = con->text + (row % con_totallines) * con_linewidth;
      for (x = 0; x < con_linewidth; x++)
         Draw_CharacterScaled(((x + 1) << 3) * scale, y, text[x], scale);
   }

   /* draw the download bar, if needed */
   Con_DrawDLBar();

   /* draw the input prompt, user text, and cursor if desired */
   Con_DrawInput();
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void
Con_SafePrintf(const char *fmt, ...)
{
    va_list argptr;
    char msg[MAX_PRINTMSG];
    int temp;

    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    temp = scr_disabled_for_loading;
    scr_disabled_for_loading = true;
    Con_Printf("%s", msg);
    scr_disabled_for_loading = temp;
}

void
Con_ShowList(const char **list, int cnt, int maxlen)
{
    const char *s;
    unsigned i, j, len, cols, rows;
    char *line;
    size_t linesize;

    /* Lay them out in columns */
    linesize = Con_GetWidth() + 1;
    line = (char*)Z_Malloc(linesize);
    cols = Con_GetWidth() / (maxlen + 2);
    rows = cnt / cols + ((cnt % cols) ? 1 : 0);

    /* Looks better if we have a few rows before spreading out */
    if (rows < 5) {
	cols = cnt / 5 + ((cnt % 5) ? 1 : 0);
	rows = cnt / cols + ((cnt % cols) ? 1 : 0);
    }

    for (i = 0; i < rows; ++i) {
	line[0] = '\0';
	for (j = 0; j < cols; ++j) {
	    if (j * rows + i >= cnt)
		break;
	    s = list[j * rows + i];
	    len = strlen(s);

	    strlcat(line, s, linesize);
	    if (j < cols - 1) {
		while (len < maxlen) {
		    strlcat(line, " ", linesize);
		    len++;
		}
		strlcat(line, "  ", linesize);
	    }
	}
	Con_Printf("%s\n", line);
    }
    Z_Free(line);
}

static const char **showtree_list;
static unsigned showtree_idx;

static void
Con_ShowTree_Populate(struct rb_node *n)
{
    if (n) {
	Con_ShowTree_Populate(n->rb_left);
	showtree_list[showtree_idx++] = stree_entry(n)->string;
	Con_ShowTree_Populate(n->rb_right);
    }
}

void
Con_ShowTree(struct stree_root *root)
{
    /* FIXME - cheating with malloc */
    showtree_list = (const char**)malloc(root->entries * sizeof(char *));
    if (showtree_list) {
	showtree_idx = 0;
	Con_ShowTree_Populate(root->root.rb_node);
	Con_ShowList(showtree_list, root->entries, root->maxlen);
	free(showtree_list);
    }
}


static void
Con_Maplist_f(void)
{
    struct stree_root st_root;
    const char *pfx = NULL;

    st_root.entries = 0;
    st_root.maxlen = 0;
    st_root.minlen = -1;
    /* st_root.root = NULL; */
    st_root.stack = NULL;

    if (Cmd_Argc() == 2)
	pfx = Cmd_Argv(1);

    STree_AllocInit();
    COM_ScanDir(&st_root, "maps", pfx, ".bsp", true);
    Con_ShowTree(&st_root);
}


/*
================
Con_Init
================
*/
void
Con_Init(void)
{
    con_main.text = (char*)Hunk_Alloc(CON_TEXTSIZE);

    con = &con_main;
    con_linewidth = -1;
    Con_CheckResize();

    Con_Printf("Console initialized.\n");

    /* register our commands */
    Cvar_RegisterVariable(&con_notifytime);

    Cmd_AddCommand("toggleconsole", Con_ToggleConsole_f);
    Cmd_AddCommand("messagemode", Con_MessageMode_f);
    Cmd_AddCommand("messagemode2", Con_MessageMode2_f);
    Cmd_AddCommand("clear", Con_Clear_f);
    Cmd_AddCommand("maplist", Con_Maplist_f);

    con_initialized = true;
}
