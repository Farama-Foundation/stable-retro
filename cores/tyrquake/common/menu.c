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

#include "client.h"
#include "cmd.h"
#include "console.h"
#include "draw.h"
#include "host.h"
#include "input.h"
#include "keys.h"
#include "menu.h"
#include "net.h"
#include "quakedef.h"
#include "r_subdiv.h"
#include "render.h"
#include "screen.h"
#include "server.h"
#include "sound.h"
#include "vid.h"
#include "view.h"

#include <streams/file_stream.h>

/* forward declarations */
RFILE* rfopen(const char *path, const char *mode);
int rfscanf(RFILE * stream, const char * format, ...);
int rfclose(RFILE* stream);

extern void D_SetupFrame(void);

void (*vid_menudrawfn) (void);
void (*vid_menukeyfn) (int key);

void M_Menu_Options_f(void);
void M_Menu_OptionsInput_f(void);
void M_Menu_OptionsVideo_f(void);
void M_Menu_OptionsAudio_f(void);
void M_Menu_OptionsGame_f(void);
void M_Menu_Quit_f(void);

static void M_Menu_Main_f(void);
static void M_Menu_SinglePlayer_f(void);
static void M_Menu_Load_f(void);
static void M_Menu_Save_f(void);
static void M_Menu_MultiPlayer_f(void);
static void M_Menu_Setup_f(void);
static void M_Menu_Keys_f(void);
static void M_Menu_Video_f(void);
static void M_Menu_Help_f(void);
static void M_Menu_LanConfig_f(void);
static void M_Menu_GameOptions_f(void);
static void M_Menu_Search_f(void);
static void M_Menu_ServerList_f(void);

static void M_Main_Draw(void);
static void M_SinglePlayer_Draw(void);
static void M_Load_Draw(void);
static void M_Save_Draw(void);
static void M_MultiPlayer_Draw(void);
static void M_Setup_Draw(void);
static void M_Options_Draw(void);
static void M_OptionsInput_Draw(void);
static void M_OptionsVideo_Draw(void);
static void M_OptionsAudio_Draw(void);
static void M_OptionsGame_Draw(void);
static void M_Keys_Draw(void);
static void M_Video_Draw(void);
static void M_Help_Draw(void);
static void M_Quit_Draw(void);
static void M_LanConfig_Draw(void);
static void M_GameOptions_Draw(void);
static void M_Search_Draw(void);
static void M_ServerList_Draw(void);

static void M_Main_Key(int key);
static void M_SinglePlayer_Key(int key);
static void M_Load_Key(int key);
static void M_Save_Key(int key);
static void M_MultiPlayer_Key(int key);
static void M_Setup_Key(int key);
static void M_Options_Key(int key);
static void M_OptionsInput_Key(int key);
static void M_OptionsVideo_Key(int key);
static void M_OptionsAudio_Key(int key);
static void M_OptionsGame_Key(int key);
static void M_Keys_Key(int key);
static void M_Video_Key(int key);
static void M_Help_Key(int key);
static void M_Quit_Key(int key);
static void M_LanConfig_Key(int key);
static void M_GameOptions_Key(int key);
static void M_Search_Key(int key);
static void M_ServerList_Key(int key);

static qboolean m_recursiveDraw;
static qboolean m_entersound;	/* play after drawing a frame, so caching */
				/* won't disrupt the sound */

qboolean m_return_onerror;
char m_return_reason[32];
int m_return_state;

enum m_state_enum {
    m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup,
    m_options, m_optionsinput, m_optionsvideo, m_optionsaudio, m_optionsgame,
    m_video, m_keys, m_help, m_quit, m_lanconfig, m_gameoptions,
    m_search, m_slist
};

int m_state;

#include "libretro.h"
extern retro_environment_t environ_cb;

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame	(m_multiplayer_cursor == 0)

static void M_ConfigureNetSubsystem(void);

/*
================
M_DrawCharacter

Draws one solid graphics character. (cx, line) are logical
320x200-space coordinates; the wrapper applies scr_uiscale and
horizontal centering against the physical screen width.
================
*/
void
M_DrawCharacter(int cx, int line, int num)
{
    int scale = SCR_GetUIScale();
    Draw_CharacterScaled(cx * scale + ((vid.width - 320 * scale) >> 1),
			 line * scale, num, scale);
}

void
M_Print(int cx, int cy, const char *str)
{
    while (*str) {
	M_DrawCharacter(cx, cy, (*str) + 128);
	str++;
	cx += 8;
    }
}

void
M_PrintWhite(int cx, int cy, const char *str)
{
    while (*str) {
	M_DrawCharacter(cx, cy, *str);
	str++;
	cx += 8;
    }
}

static void
M_DrawTransPic(int x, int y, const qpic_t *pic)
{
    int scale = SCR_GetUIScale();
    Draw_TransPicScaled(x * scale + ((vid.width - 320 * scale) >> 1),
			y * scale, pic, scale);
}

void
M_DrawPic(int x, int y, const qpic_t *pic)
{
    int scale = SCR_GetUIScale();
    Draw_PicScaled(x * scale + ((vid.width - 320 * scale) >> 1),
		   y * scale, pic, scale);
}

static byte identityTable[256];
static byte translationTable[256];

static void
M_BuildTranslationTable(int top, int bottom)
{
    int j;
    byte *dest, *source;

    for (j = 0; j < 256; j++)
	identityTable[j] = j;
    dest = translationTable;
    source = identityTable;
    memcpy(dest, source, 256);

    if (top < 128)		/* the artists made some backwards ranges.  sigh. */
	memcpy(dest + TOP_RANGE, source + top, 16);
    else
	for (j = 0; j < 16; j++)
	    dest[TOP_RANGE + j] = source[top + 15 - j];

    if (bottom < 128)
	memcpy(dest + BOTTOM_RANGE, source + bottom, 16);
    else
	for (j = 0; j < 16; j++)
	    dest[BOTTOM_RANGE + j] = source[bottom + 15 - j];
}


static void
M_DrawTransPicTranslate(int x, int y, const qpic_t *pic)
{
    int scale = SCR_GetUIScale();
    Draw_TransPicTranslateScaled(x * scale + ((vid.width - 320 * scale) >> 1),
				 y * scale, pic, translationTable, scale);
}


void
M_DrawTextBox(int x, int y, int width, int lines)
{
    const qpic_t *p;
    int cx, cy;
    int n;

    /* draw left side */
    cx = x;
    cy = y;
    p = Draw_CachePic("gfx/box_tl.lmp");
    M_DrawTransPic(cx, cy, p);
    p = Draw_CachePic("gfx/box_ml.lmp");
    for (n = 0; n < lines; n++) {
	cy += 8;
	M_DrawTransPic(cx, cy, p);
    }
    p = Draw_CachePic("gfx/box_bl.lmp");
    M_DrawTransPic(cx, cy + 8, p);

    /* draw middle */
    cx += 8;
    while (width > 0) {
	cy = y;
	p = Draw_CachePic("gfx/box_tm.lmp");
	M_DrawTransPic(cx, cy, p);
	p = Draw_CachePic("gfx/box_mm.lmp");
	for (n = 0; n < lines; n++) {
	    cy += 8;
	    if (n == 1)
		p = Draw_CachePic("gfx/box_mm2.lmp");
	    M_DrawTransPic(cx, cy, p);
	}
	p = Draw_CachePic("gfx/box_bm.lmp");
	M_DrawTransPic(cx, cy + 8, p);
	width -= 2;
	cx += 16;
    }

    /* draw right side */
    cy = y;
    p = Draw_CachePic("gfx/box_tr.lmp");
    M_DrawTransPic(cx, cy, p);
    p = Draw_CachePic("gfx/box_mr.lmp");
    for (n = 0; n < lines; n++) {
	cy += 8;
	M_DrawTransPic(cx, cy, p);
    }
    p = Draw_CachePic("gfx/box_br.lmp");
    M_DrawTransPic(cx, cy + 8, p);
}

/* ============================================================================= */

static int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void
M_ToggleMenu_f(void)
{
    m_entersound = true;

    if (key_dest == key_menu) {
	if (m_state != m_main) {
	    M_Menu_Main_f();
	    return;
	}
	key_dest = key_game;
	m_state = m_none;
	return;
    }
    if (key_dest == key_console) {
	Con_ToggleConsole_f();
    } else {
	M_Menu_Main_f();
    }
}


/* ============================================================================= */
/* MAIN MENU */

static int m_main_cursor;

#define MAIN_ITEMS 5

static void
M_Menu_Main_f(void)
{
    if (key_dest != key_menu) {
	m_save_demonum = cls.demonum;
	cls.demonum = -1;
    }
    key_dest = key_menu;
    m_state = m_main;
    m_entersound = true;
}


static void
M_Main_Draw(void)
{
    int f;
    const qpic_t *p;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/ttl_main.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);
    M_DrawTransPic(72, 32, Draw_CachePic("gfx/mainmenu.lmp"));

    f = (int)(host_time * 10) % 6;

    M_DrawTransPic(54, 32 + m_main_cursor * 20,
		   Draw_CachePic(va("gfx/menudot%i.lmp", f + 1)));
}


static void
M_Main_Key(int key)
{
    switch (key) {
    case K_JOY_START:
    case K_JOY_B:
    case K_ESCAPE:
	key_dest = key_game;
	m_state = m_none;
	cls.demonum = m_save_demonum;
	if (cls.demonum != -1 && !cls.demoplayback
	    && cls.state <= ca_connected)
	    CL_NextDemo();
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	if (++m_main_cursor >= MAIN_ITEMS)
	    m_main_cursor = 0;
	break;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	if (--m_main_cursor < 0)
	    m_main_cursor = MAIN_ITEMS - 1;
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;

	switch (m_main_cursor) {
	case 0:
	    M_Menu_SinglePlayer_f();
	    break;

	case 1:
	    M_Menu_MultiPlayer_f();
	    break;

	case 2:
	    M_Menu_Options_f();
	    break;

	case 3:
	    M_Menu_Help_f();
	    break;

	case 4:
	    M_Menu_Quit_f();
	    break;
	}
    }
}

/* ============================================================================= */
/* SINGLE PLAYER MENU */

int m_singleplayer_cursor;

#define	SINGLEPLAYER_ITEMS	3


static void
M_Menu_SinglePlayer_f(void)
{
    key_dest = key_menu;
    m_state = m_singleplayer;
    m_entersound = true;
}


static void
M_SinglePlayer_Draw(void)
{
    int f;
    const qpic_t *p;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/ttl_sgl.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);
    M_DrawTransPic(72, 32, Draw_CachePic("gfx/sp_menu.lmp"));

    f = (int)(host_time * 10) % 6;

    M_DrawTransPic(54, 32 + m_singleplayer_cursor * 20,
		   Draw_CachePic(va("gfx/menudot%i.lmp", f + 1)));
}


static void
M_SinglePlayer_Key(int key)
{
    switch (key) {

    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Main_f();
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
	    m_singleplayer_cursor = 0;
	break;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	if (--m_singleplayer_cursor < 0)
	    m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;

	switch (m_singleplayer_cursor) {
	case 0:
	    key_dest = key_game;
	    if (sv.active)
		Cbuf_AddText("disconnect\n");
	    Cbuf_AddText("maxplayers 1\n");
	    Cbuf_AddText("map start\n");

	    break;

	case 1:
	    M_Menu_Load_f();
	    break;

	case 2:
	    M_Menu_Save_f();
	    break;
	}
    }
}

/* ============================================================================= */
/* LOAD/SAVE MENU */

#define	MAX_SAVEGAMES 12

static char m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
static int loadable[MAX_SAVEGAMES];
static int load_cursor;		/* 0 < load_cursor < MAX_SAVEGAMES */

static void M_ScanSaves(void)
{
   int i;
#ifdef _WIN32
   char slash = '\\';
#else
   char slash = '/';
#endif

   for (i = 0; i < MAX_SAVEGAMES; i++)
   {
      int j, version;
      size_t cmt_len;
      char slot[24]; /* "s" + INT_MAX (10) + ".sav" + NUL = 16, round up */
      char name[MAX_OSPATH];
      char comment[MAX_OSPATH];
      RFILE *f;

      strlcpy(m_filenames[i], "--- UNUSED SLOT ---", sizeof(m_filenames[i]));
      loadable[i] = false;
      /* Build the slot suffix into a small bounded local first, then
       * use COM_JoinPath which performs an explicit overflow check —
       * GCC otherwise cannot see com_savedir is bounded enough to fit
       * and -Wformat-truncation= fires on a direct snprintf. */
      snprintf(slot, sizeof(slot), "s%i.sav", i);
      if (COM_JoinPath(name, sizeof(name), com_savedir, slash, slot) < 0)
         continue;
      f = rfopen(name, "r");
      if (!f)
         continue;
      rfscanf(f, "%i\n", &version);
      rfscanf(f, "%79s\n", comment);
      /* memcpy with explicit length avoids both -Wstringop-truncation
       * (which fires on strncpy(..., sizeof(dst)-1) as a known-truncate
       * idiom) and the missing-NUL footgun strncpy has when the source
       * is at least as long as the count. */
      cmt_len = strlen(comment);
      if (cmt_len >= sizeof(m_filenames[i]))
         cmt_len = sizeof(m_filenames[i]) - 1;
      memcpy(m_filenames[i], comment, cmt_len);
      m_filenames[i][cmt_len] = '\0';

      /* change _ back to space */
      for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
         if (m_filenames[i][j] == '_')
            m_filenames[i][j] = ' ';
      loadable[i] = true;
      rfclose(f);
   }
}

static void M_Menu_Load_f(void)
{
    m_entersound = true;
    m_state = m_load;
    key_dest = key_menu;
    M_ScanSaves();
}


static void M_Menu_Save_f(void)
{
   if (!sv.active)
      return;
   if (cl.intermission)
      return;
   if (svs.maxclients != 1)
      return;
   m_entersound = true;
   m_state = m_save;
   key_dest = key_menu;
   M_ScanSaves();
}


static void M_Load_Draw(void)
{
   int i;
   const qpic_t *p = Draw_CachePic("gfx/p_load.lmp");

   M_DrawPic((320 - p->width) / 2, 4, p);

   for (i = 0; i < MAX_SAVEGAMES; i++)
      M_Print(16, 32 + 8 * i, m_filenames[i]);

   /* line cursor */
   M_DrawCharacter(8, 32 + load_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


static void M_Save_Draw(void)
{
   int i;
   const qpic_t *p = Draw_CachePic("gfx/p_save.lmp");

   M_DrawPic((320 - p->width) / 2, 4, p);

   for (i = 0; i < MAX_SAVEGAMES; i++)
      M_Print(16, 32 + 8 * i, m_filenames[i]);

   /* line cursor */
   M_DrawCharacter(8, 32 + load_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


static void M_Load_Key(int k)
{
   switch (k)
   {
 
      case K_JOY_B:
      case K_ESCAPE:
         M_Menu_SinglePlayer_f();
         break;
 
      case K_JOY_A:
      case K_ENTER:
         S_LocalSound("misc/menu2.wav");
         if (!loadable[load_cursor])
            return;
         m_state = m_none;
         key_dest = key_game;

         /* Host_Loadgame_f can't bring up the loading plaque because too much */
         /* stack space has been used, so do it now */
         SCR_BeginLoadingPlaque();

         /* issue the load command */
         Cbuf_AddText("load s%i\n", load_cursor);
         return;

      case K_JOY_UP:
      case K_JOY_LEFT:
      case K_UPARROW:
      case K_LEFTARROW:
         S_LocalSound("misc/menu1.wav");
         load_cursor--;
         if (load_cursor < 0)
            load_cursor = MAX_SAVEGAMES - 1;
         break;

      case K_JOY_DOWN:
      case K_JOY_RIGHT:
      case K_DOWNARROW:
      case K_RIGHTARROW:
         S_LocalSound("misc/menu1.wav");
         load_cursor++;
         if (load_cursor >= MAX_SAVEGAMES)
            load_cursor = 0;
         break;
   }
}


static void M_Save_Key(int k)
{
   switch (k) {

      case K_JOY_B:
      case K_ESCAPE:
         M_Menu_SinglePlayer_f();
         break;

      case K_JOY_A:
      case K_ENTER:
         m_state = m_none;
         key_dest = key_game;
         Cbuf_AddText("save s%i\n", load_cursor);
         return;

      case K_JOY_UP:
      case K_JOY_LEFT:
      case K_UPARROW:
      case K_LEFTARROW:
         S_LocalSound("misc/menu1.wav");
         load_cursor--;
         if (load_cursor < 0)
            load_cursor = MAX_SAVEGAMES - 1;
         break;

      case K_JOY_DOWN:
      case K_JOY_RIGHT:
      case K_DOWNARROW:
      case K_RIGHTARROW:
         S_LocalSound("misc/menu1.wav");
         load_cursor++;
         if (load_cursor >= MAX_SAVEGAMES)
            load_cursor = 0;
         break;
   }
}

/* ============================================================================= */
/* MULTIPLAYER MENU */

#define	MULTIPLAYER_ITEMS 3

static int m_multiplayer_cursor;

static void M_Menu_MultiPlayer_f(void)
{
   key_dest = key_menu;
   m_state = m_multiplayer;
   m_entersound = true;
}


static void M_MultiPlayer_Draw(void)
{
   int f;
   const qpic_t *p;

   M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
   p = Draw_CachePic("gfx/p_multi.lmp");
   M_DrawPic((320 - p->width) / 2, 4, p);
   M_DrawTransPic(72, 32, Draw_CachePic("gfx/mp_menu.lmp"));

   f = (int)(host_time * 10) % 6;

   M_DrawTransPic(54, 32 + m_multiplayer_cursor * 20,
         Draw_CachePic(va("gfx/menudot%i.lmp", f + 1)));

   if (tcpipAvailable)
      return;
   M_PrintWhite((320 / 2) - ((27 * 8) / 2), 148,
         "No Communications Available");
}


static void M_MultiPlayer_Key(int key)
{
   switch (key) {
      case K_JOY_B:
      case K_ESCAPE:
         M_Menu_Main_f();
         break;

      case K_JOY_DOWN:
      case K_DOWNARROW:
         S_LocalSound("misc/menu1.wav");
         if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
            m_multiplayer_cursor = 0;
         break;

      case K_JOY_UP:
      case K_UPARROW:
         S_LocalSound("misc/menu1.wav");
         if (--m_multiplayer_cursor < 0)
            m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
         break;

      case K_JOY_A:
      case K_ENTER:
         m_entersound = true;
         switch (m_multiplayer_cursor) {
            case 0:
               if (tcpipAvailable)
                  M_Menu_LanConfig_f();
               break;

            case 1:
               if (tcpipAvailable)
                  M_Menu_LanConfig_f();
               break;

            case 2:
               M_Menu_Setup_f();
               break;
         }
   }
}

/* ============================================================================= */
/* SETUP MENU */

static int setup_cursor = 4;
static int setup_cursor_table[] = { 40, 56, 80, 104, 140 };

static char setup_hostname[16];
static char setup_myname[16];
static int setup_oldtop;
static int setup_oldbottom;
static int setup_top;
static int setup_bottom;

#define	NUM_SETUP_CMDS	5

static void M_Menu_Setup_f(void)
{
   key_dest = key_menu;
   m_state = m_setup;
   m_entersound = true;
   strlcpy(setup_myname, cl_name.string, sizeof(setup_myname));
   strlcpy(setup_hostname, hostname.string, sizeof(setup_hostname));
   setup_top = setup_oldtop = ((int)cl_color.value) >> 4;
   setup_bottom = setup_oldbottom = ((int)cl_color.value) & 15;
}


static void M_Setup_Draw(void)
{
   const qpic_t *p;

   M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
   p = Draw_CachePic("gfx/p_multi.lmp");
   M_DrawPic((320 - p->width) / 2, 4, p);

   M_Print(64, 40, "Hostname");
   M_DrawTextBox(160, 32, 16, 1);
   M_Print(168, 40, setup_hostname);

   M_Print(64, 56, "Your name");
   M_DrawTextBox(160, 48, 16, 1);
   M_Print(168, 56, setup_myname);

   M_Print(64, 80, "Shirt color");
   M_Print(64, 104, "Pants color");

   M_DrawTextBox(64, 140 - 8, 14, 1);
   M_Print(72, 140, "Accept Changes");

   p = Draw_CachePic("gfx/bigbox.lmp");
   M_DrawTransPic(160, 64, p);
   p = Draw_CachePic("gfx/menuplyr.lmp");
   M_BuildTranslationTable(setup_top * 16, setup_bottom * 16);
   M_DrawTransPicTranslate(172, 72, p);

   M_DrawCharacter(56, setup_cursor_table[setup_cursor],
         12 + ((int)(realtime * 4) & 1));

   if (setup_cursor == 0)
      M_DrawCharacter(168 + 8 * strlen(setup_hostname),
            setup_cursor_table[setup_cursor],
            10 + ((int)(realtime * 4) & 1));

   if (setup_cursor == 1)
      M_DrawCharacter(168 + 8 * strlen(setup_myname),
            setup_cursor_table[setup_cursor],
            10 + ((int)(realtime * 4) & 1));
}


static void
M_Setup_Key(int k)
{
    int l;

    switch (k) {

    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_MultiPlayer_f();
	break;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	setup_cursor--;
	if (setup_cursor < 0)
	    setup_cursor = NUM_SETUP_CMDS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	setup_cursor++;
	if (setup_cursor >= NUM_SETUP_CMDS)
	    setup_cursor = 0;
	break;

    case K_JOY_LEFT:
    case K_LEFTARROW:
	if (setup_cursor < 2)
	    return;
	S_LocalSound("misc/menu3.wav");
	if (setup_cursor == 2)
	    setup_top = setup_top - 1;
	if (setup_cursor == 3)
	    setup_bottom = setup_bottom - 1;
	break;
    case K_JOY_RIGHT:
    case K_RIGHTARROW:
	if (setup_cursor < 2)
	    return;
      forward:
	S_LocalSound("misc/menu3.wav");
	if (setup_cursor == 2)
	    setup_top = setup_top + 1;
	if (setup_cursor == 3)
	    setup_bottom = setup_bottom + 1;
	break;

    case K_JOY_A:
    case K_ENTER:
	if (setup_cursor == 0 || setup_cursor == 1)
	    return;

	if (setup_cursor == 2 || setup_cursor == 3)
	    goto forward;

	/* setup_cursor == 4 (OK) */
	if (strcmp(cl_name.string, setup_myname) != 0)
	    Cbuf_AddText("name \"%s\"\n", setup_myname);
	if (strcmp(hostname.string, setup_hostname) != 0)
	    Cvar_Set("hostname", setup_hostname);
	if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
	    Cbuf_AddText("color %i %i\n", setup_top, setup_bottom);
	m_entersound = true;
	M_Menu_MultiPlayer_f();
	break;

    case K_JOY_X:
    case K_JOY_Y:
    case K_BACKSPACE:
	if (setup_cursor == 0) {
	    if (strlen(setup_hostname))
		setup_hostname[strlen(setup_hostname) - 1] = 0;
	}

	if (setup_cursor == 1) {
	    if (strlen(setup_myname))
		setup_myname[strlen(setup_myname) - 1] = 0;
	}
	break;

    default:
	if (k < 32 || k > 127)
	    break;
	if (setup_cursor == 0) {
	    l = strlen(setup_hostname);
	    if (l < 15) {
		setup_hostname[l + 1] = 0;
		setup_hostname[l] = k;
	    }
	}
	if (setup_cursor == 1) {
	    l = strlen(setup_myname);
	    if (l < 15) {
		setup_myname[l + 1] = 0;
		setup_myname[l] = k;
	    }
	}
    }

    if (setup_top > 13)
	setup_top = 0;
    if (setup_top < 0)
	setup_top = 13;
    if (setup_bottom > 13)
	setup_bottom = 0;
    if (setup_bottom < 0)
	setup_bottom = 13;
}

/* ============================================================================= */
/* OPTIONS SLIDER */

#define	SLIDER_RANGE	10

static void
M_DrawSlider(int x, int y, float range)
{
    int i;

    if (range < 0)
	range = 0;
    if (range > 1)
	range = 1;
    M_DrawCharacter(x - 8, y, 128);
    for (i = 0; i < SLIDER_RANGE; i++)
	M_DrawCharacter(x + i * 8, y, 129);
    M_DrawCharacter(x + i * 8, y, 130);
    M_DrawCharacter(x + (SLIDER_RANGE - 1) * 8 * range, y, 131);
}

static void
M_DrawCheckbox(int x, int y, int on)
{
    if (on)
	M_Print(x, y, "on");
    else
	M_Print(x, y, "off");
}


/* ============================================================================= */
/* INPUT OPTIONS MENU */

#define	OPTIONSINPUT_ITEMS 5

static int optionsinput_cursor;

void
M_Menu_OptionsInput_f(void)
{
    key_dest = key_menu;
    m_state = m_optionsinput;
    m_entersound = true;
}

static void
M_OptionsInput_AdjustSliders(int dir)
{
    S_LocalSound("misc/menu3.wav");

    switch (optionsinput_cursor) {
    case 3:			/* mouse speed */
	sensitivity.value += dir * 0.5;
	if (sensitivity.value < 1)
	    sensitivity.value = 1;
	if (sensitivity.value > 11)
	    sensitivity.value = 11;
	Cvar_SetValue("sensitivity", sensitivity.value);
	break;
    case 4:			/* invert mouse */
	Cvar_SetValue("m_pitch", -m_pitch.value);
	break;
    }
}

static void
M_OptionsInput_Draw(void)
{
    float r;
    const qpic_t *p;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(16, 32, "    Customize controls");
    M_Print(16, 40, "         Go to console");
    M_Print(16, 48, "     Reset to defaults");

    M_Print(16, 56, "           Mouse Speed");
    r = (sensitivity.value - 1) / 10;
    M_DrawSlider(220, 56, r);
    M_Print(16, 64, "          Invert Mouse");
    M_DrawCheckbox(220, 64, m_pitch.value < 0);

/* cursor */
    M_DrawCharacter(200, 32 + optionsinput_cursor * 8,
		    12 + ((int)(realtime * 4) & 1));
}


static void
M_OptionsInput_Key(int k)
{

    switch (k) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Options_f();
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;
	switch (optionsinput_cursor) {
	case 0:
	    M_Menu_Keys_f();
	    break;
	case 1:
	    m_state = m_none;
	    Con_ToggleConsole_f();
	    break;
	case 2:
	    Cbuf_AddText("exec default.cfg\n");
	    break;
	default:
	    M_OptionsInput_AdjustSliders(1);
	    break;
	}
	return;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	optionsinput_cursor--;
	if (optionsinput_cursor < 0)
	    optionsinput_cursor = OPTIONSINPUT_ITEMS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	optionsinput_cursor++;
	if (optionsinput_cursor >= OPTIONSINPUT_ITEMS)
	    optionsinput_cursor = 0;
	break;

    case K_JOY_LEFT:
    case K_LEFTARROW:
	M_OptionsInput_AdjustSliders(-1);
	break;

    case K_JOY_RIGHT:
    case K_RIGHTARROW:
	M_OptionsInput_AdjustSliders(1);
	break;
    }
}

/* ============================================================================= */
/* VIDEO OPTIONS MENU */

#define	OPTIONSVIDEO_ITEMS 20

static int optionsvideo_cursor;

void
M_Menu_OptionsVideo_f(void)
{
    key_dest = key_menu;
    m_state = m_optionsvideo;
    m_entersound = true;
}


static void
M_OptionsVideo_AdjustSliders(int dir)
{
    cvar_t *cvar = NULL;
    S_LocalSound("misc/menu3.wav");

    switch (optionsvideo_cursor) {
       case 0:			/* screen size */
          scr_viewsize.value += dir * 10;
          if (scr_viewsize.value < 30)
             scr_viewsize.value = 30;
          if (scr_viewsize.value > 120)
             scr_viewsize.value = 120;
          Cvar_SetValue("viewsize", scr_viewsize.value);
          break;
       case 1:			/* gamma */
          v_gamma.value -= dir * 0.05;
          if (v_gamma.value < 0.5)
             v_gamma.value = 0.5;
          if (v_gamma.value > 1)
             v_gamma.value = 1;
          Cvar_SetValue("gamma", v_gamma.value);
          break;
       case 2:			/* brightness */
          /* Range [-0.5, +0.5] in 0.05 steps, centred on 0.0
           * (vanilla, no shift).  Shifts every palette entry
           * by b*255 byte-counts before contrast and gamma
           * apply; the LUT is rebuilt by V_CheckGamma and
           * reaches both d_8to16table (SW) and
           * d_8to24table_shifted (Vulkan) the same way gamma
           * and contrast do. */
          v_brightness.value += dir * 0.05;
          if (v_brightness.value < -0.5)
             v_brightness.value = -0.5;
          if (v_brightness.value >  0.5)
             v_brightness.value =  0.5;
          Cvar_SetValue("brightness", v_brightness.value);
          break;
       case 3:			/* contrast */
          /* Range [0.5, 1.5] in 0.05 steps, centred on 1.0
           * (vanilla, no change).  view.c's V_CheckGamma
           * folds this into the same gammatable lookup
           * used by V_UpdatePalette, so the value reaches
           * both d_8to16table (SW present) and
           * d_8to24table_shifted (Vulkan compose) on the
           * next palette rebuild. */
          v_contrast.value += dir * 0.05;
          if (v_contrast.value < 0.5)
             v_contrast.value = 0.5;
          if (v_contrast.value > 1.5)
             v_contrast.value = 1.5;
          Cvar_SetValue("contrast", v_contrast.value);
          break;

       case 5:			/* _windowed_mouse */
          Cvar_SetValue("_windowed_mouse", !_windowed_mouse.value);
          break;
       case 6:
          cvar = Cvar_FindVar("dither_filter");
          Cvar_SetValue("dither_filter", cvar->value ? 0.0f : 1.0f);
          D_SetupFrame();
          break;
       case 7:
          cvar = Cvar_FindVar("d_mipscale");
          Cvar_SetValue("d_mipscale", cvar->value ? 0.0f : 1.0f);
          break;
       case 8:
          cvar = Cvar_FindVar("r_lerpmodels");
          Cvar_SetValue("r_lerpmodels", cvar->value ? 0.0f : 1.0f);
          break;
       case 9:
          cvar = Cvar_FindVar("r_lerpmove");
          Cvar_SetValue("r_lerpmove", cvar->value ? 0.0f : 1.0f);
          break;
       case 10: {
          /* Tri-state: 0 = off, 1 = Phong, 2 = Phong + specular.
           * Arrow keys cycle the value; dir is +1 for right,
           * -1 for left. */
          int v;
          cvar = Cvar_FindVar("r_phongshading");
          v = (int)cvar->value + dir;
          if (v < 0) v = 2;
          if (v > 2) v = 0;
          Cvar_SetValue("r_phongshading", (float)v);
          break;
       }
       case 11:
          cvar = Cvar_FindVar("r_coloredlight");
          Cvar_SetValue("r_coloredlight", cvar->value ? 0.0f : 1.0f);
          break;
       case 12:
          cvar = Cvar_FindVar("r_lightdither");
          Cvar_SetValue("r_lightdither", cvar->value ? 0.0f : 1.0f);
          break;
       case 13:
          cvar = Cvar_FindVar("r_shadows");
          Cvar_SetValue("r_shadows", cvar->value ? 0.0f : 1.0f);
          break;
       case 14: {
          /* Cycle r_liquidblend through 0 = Off, 1 = Stipple.
           * Future modes (colormap-blend etc) would extend the
           * upper bound; for now wrap at 1. */
          int v;
          cvar = Cvar_FindVar("r_liquidblend");
          v = (int)cvar->value + dir;
          if (v < 0) v = 1;
          if (v > 1) v = 0;
          Cvar_SetValue("r_liquidblend", (float)v);
          break;
       }
       case 15: {
          /* Slide r_wateralpha in 0.05 increments.
           * Range [0.10, 0.90]:
           *   - Below 0.10 the dither is so sparse the liquid
           *     surface effectively disappears.
           *   - Above 0.90 the 4x4 Bayer matrix (max threshold
           *     240) produces zero stipple holes -- the result is
           *     visually identical to fully opaque rendering, so
           *     the slider would appear to do nothing.  Worse, in
           *     the two-pass pipeline the per-pixel z-test still
           *     gates writes, producing inconsistencies between
           *     "slider near max" and "fully opaque vanilla
           *     path".  Capping at 0.90 keeps every step
           *     meaningful.
           * Fully opaque is reachable via Liquid Blend = Off, or
           * by setting a per-liquid cvar to 1.0 from console. */
          float v;
          cvar = Cvar_FindVar("r_wateralpha");
          v = cvar->value + dir * 0.05f;
          if (v < 0.10f) v = 0.10f;
          if (v > 0.90f) v = 0.90f;
          Cvar_SetValue("r_wateralpha", v);
          break;
       }
       case 16: {
          float v;
          cvar = Cvar_FindVar("r_lavaalpha");
          v = cvar->value + dir * 0.05f;
          if (v < 0.10f) v = 0.10f;
          if (v > 0.90f) v = 0.90f;
          Cvar_SetValue("r_lavaalpha", v);
          break;
       }
       case 17: {
          float v;
          cvar = Cvar_FindVar("r_slimealpha");
          v = cvar->value + dir * 0.05f;
          if (v < 0.10f) v = 0.10f;
          if (v > 0.90f) v = 0.90f;
          Cvar_SetValue("r_slimealpha", v);
          break;
       }
       case 18: {
          /* Polygon subdivision: cycle 0..R_POLYSUBDIV_MAX_PASSES.
           * The cvar callback (R_PolySubdivChanged) clamps and
           * flushes the alias model cache, so the change is visible
           * the next time each model is referenced -- typically
           * within a frame for any visible entity. */
          int v;
          cvar = Cvar_FindVar("r_polysubdiv");
          v = (int)cvar->value + dir;
          if (v < 0) v = R_POLYSUBDIV_MAX_PASSES;
          if (v > R_POLYSUBDIV_MAX_PASSES) v = 0;
          Cvar_SetValue("r_polysubdiv", (float)v);
          break;
       }
       case 19: {
          /* Display aspect ratio: cycle 0..R_ASPECT_MAX
           * (4:3, 16:9, 16:10, 21:9, 32:9).  The r_aspect cvar
           * callback repushes the frontend display geometry, so
           * the change is visible on the next presented frame
           * without touching the rendered resolution. */
          int v;
          cvar = Cvar_FindVar("r_aspect");
          v = (int)cvar->value + dir;
          if (v < 0) v = R_ASPECT_MAX;
          if (v > R_ASPECT_MAX) v = 0;
          Cvar_SetValue("r_aspect", (float)v);
          break;
       }
    }
}


static void
M_OptionsVideo_Draw(void)
{
    float r;
    const qpic_t *p;
    cvar_t *cvar = NULL;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(16, 32, "           Screen size");
    r = (scr_viewsize.value - 30) / (120 - 30);
    M_DrawSlider(220, 32, r);

    M_Print(16, 40, "            Gamma");
    r = (1.0 - v_gamma.value) / 0.5;
    M_DrawSlider(220, 40, r);

    /* Brightness: range [-0.5, +0.5] centred on 0.0 (vanilla).
     * Slider sweep is (v + 0.5) / 1.0 so -0.5 is left, 0.0 is
     * mid, +0.5 is right.  Folded into gammatable[] alongside
     * gamma and contrast; reaches every backend through the
     * VID_SetPalette palette LUTs. */
    M_Print(16, 48, "          Brightness");
    r = (v_brightness.value + 0.5f) / 1.0f;
    M_DrawSlider(220, 48, r);

    /* Contrast: range [0.5, 1.5] centred on 1.0 (vanilla).
     * Slider sweep is (v - 0.5) / 1.0 so 0.5 is left, 1.0 is
     * mid, 1.5 is right.  Folded into gammatable[] in
     * view.c's V_CheckGamma; reaches both SW and Vulkan
     * via VID_SetPalette's twin LUTs. */
    M_Print(16, 56, "          Contrast");
    r = (v_contrast.value - 0.5f) / 1.0f;
    M_DrawSlider(220, 56, r);

    if (vid_menudrawfn)
	M_Print(16, 64, "         Video Options");

    if (!VID_IsFullScreen()) {
       M_Print(16, 72, "             Use Mouse");
       M_DrawCheckbox(220, 72, _windowed_mouse.value);
    }

    cvar = Cvar_FindVar("dither_filter");
    M_Print(16, 80, "      Dither Filtering");
	M_DrawCheckbox(220, 80, cvar->value);

    cvar = Cvar_FindVar("d_mipscale");
    M_Print(16, 88, "      Level of Detail");
    M_DrawCheckbox(220, 88, cvar->value);

    cvar = Cvar_FindVar("r_lerpmodels");
    M_Print(16, 96, "      Smooth Animation");
    M_DrawCheckbox(220, 96, cvar->value);
    
    cvar = Cvar_FindVar("r_lerpmove");
    M_Print(16, 104, "      Smooth Movement");
    M_DrawCheckbox(220, 104, cvar->value);

    /* Phong shading is tri-state: 0 = off, 1 = Phong, 2 = Phong +
     * specular highlights.  Show the state name instead of a
     * checkbox; arrow-key adjustment in M_OptionsVideo_AdjustSliders
     * cycles through the three values. */
    {
       static const char *phong_state_names[3] = {
          "Off", "Phong", "Specular"
       };
       int phong_v;
       cvar = Cvar_FindVar("r_phongshading");
       phong_v = (int)cvar->value;
       if (phong_v < 0) phong_v = 0;
       if (phong_v > 2) phong_v = 2;
       M_Print(16, 112, "        Phong Shading");
       M_Print(220, 112, phong_state_names[phong_v]);
    }

    cvar = Cvar_FindVar("r_coloredlight");
    M_Print(16, 120, "    Colored Lighting");
    M_DrawCheckbox(220, 120, cvar->value);

    cvar = Cvar_FindVar("r_lightdither");
    M_Print(16, 128, "      Light Dither");
    M_DrawCheckbox(220, 128, cvar->value);

    cvar = Cvar_FindVar("r_shadows");
    M_Print(16, 136, "             Shadows");
    M_DrawCheckbox(220, 136, cvar->value);

    /* Liquid blend mode is a cycling text widget (currently
     * 0 = Off, 1 = Stipple).  Reserved values 2+ for future
     * blend modes will be added without disrupting saved cvars. */
    {
	static const char *blend_state_names[2] = { "Off", "Stipple" };
	int blend_v;
	cvar = Cvar_FindVar("r_liquidblend");
	blend_v = (int)cvar->value;
	if (blend_v < 0) blend_v = 0;
	if (blend_v > 1) blend_v = 1;
	M_Print(16, 144, "         Liquid Blend");
	M_Print(220, 144, blend_state_names[blend_v]);
    }

    /* Per-liquid alpha sliders.  The cvar's stipple-useful range
     * is [0.10, 0.90]; map that to the slider's [0, 1] visual
     * range so the indicator sweeps end-to-end as the user steps
     * through legal values.
     *
     * The cvar default is 1.0 (fully opaque, vanilla rendering).
     * Values >= 0.90 collapse onto "slider far right" visually
     * since M_DrawSlider clamps; the user's first click left from
     * the default snaps the cvar to 0.90 and the indicator
     * immediately moves visibly. */
    {
	const float lo = 0.10f, hi = 0.90f;
	float v;

	cvar = Cvar_FindVar("r_wateralpha");
	v = (cvar->value - lo) / (hi - lo);
	M_Print(16, 152, "         Water Alpha");
	M_DrawSlider(220, 152, v);

	cvar = Cvar_FindVar("r_lavaalpha");
	v = (cvar->value - lo) / (hi - lo);
	M_Print(16, 160, "          Lava Alpha");
	M_DrawSlider(220, 160, v);

	cvar = Cvar_FindVar("r_slimealpha");
	v = (cvar->value - lo) / (hi - lo);
	M_Print(16, 168, "         Slime Alpha");
	M_DrawSlider(220, 168, v);
    }

    /* Polygon Subdivision: cycling text widget showing the current
     * pass count (0..R_POLYSUBDIV_MAX_PASSES).  Each pass quadruples
     * the triangle count of every alias model, smoothing curvature
     * at the cost of memory and per-frame transform work; see
     * r_subdiv.c.  The cvar string mirrors the array indices so a
     * future bump of R_POLYSUBDIV_MAX_PASSES only requires extending
     * this table. */
    {
	static const char *subdiv_state_names[R_POLYSUBDIV_MAX_PASSES + 1] = {
	    "Off",
	    "1 pass (4x tris)",
	    "2 pass (16x tris)",
	    "3 pass (64x tris)"
	};
	int sub_v;
	cvar = Cvar_FindVar("r_polysubdiv");
	sub_v = (int)cvar->value;
	if (sub_v < 0) sub_v = 0;
	if (sub_v > R_POLYSUBDIV_MAX_PASSES) sub_v = R_POLYSUBDIV_MAX_PASSES;
	M_Print(16, 176, "   Polygon Subdivision");
	M_Print(220, 176, subdiv_state_names[sub_v]);
    }

    /* Display Aspect Ratio: cycling text widget.  Index order matches
     * R_AspectRatioForIndex in libretro.c; 4:3 is index 0 (vanilla
     * default).  Changing it repushes display geometry to the
     * frontend via the r_aspect cvar callback. */
    {
	static const char *aspect_state_names[R_ASPECT_NUM_RATIOS] = {
	    "4:3", "16:9", "16:10", "21:9", "32:9"
	};
	int asp_v;
	cvar = Cvar_FindVar("r_aspect");
	asp_v = (int)cvar->value;
	if (asp_v < 0) asp_v = 0;
	if (asp_v > R_ASPECT_MAX) asp_v = R_ASPECT_MAX;
	M_Print(16, 184, "         Aspect Ratio");
	M_Print(220, 184, aspect_state_names[asp_v]);
    }

/* cursor */
    M_DrawCharacter(200, 32 + optionsvideo_cursor * 8,
		    12 + ((int)(realtime * 4) & 1));
}


static void
M_OptionsVideo_Key(int k)
{

    switch (k) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Options_f();
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;
	switch (optionsvideo_cursor) {
	case 4:
	    M_Menu_Video_f();
	    break;
	default:
	    M_OptionsVideo_AdjustSliders(1);
	    break;
	}
	return;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	optionsvideo_cursor--;
	if (optionsvideo_cursor < 0)
	    optionsvideo_cursor = OPTIONSVIDEO_ITEMS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	optionsvideo_cursor++;
	if (optionsvideo_cursor >= OPTIONSVIDEO_ITEMS)
	    optionsvideo_cursor = 0;
	break;

    case K_JOY_LEFT:
    case K_LEFTARROW:
	M_OptionsVideo_AdjustSliders(-1);
	break;

    case K_JOY_RIGHT:
    case K_RIGHTARROW:
	M_OptionsVideo_AdjustSliders(1);
	break;
    }

    /* Skip the Video Options row (cursor 4) when no per-platform
     * video menu is wired up -- libretro never sets vid_menudrawfn.
     * Skip the Use Mouse row (cursor 5) when running fullscreen
     * (always true under libretro since the frontend owns the
     * window).  Indices bumped by one vs the contrast-only insert
     * after the Brightness row was added at cursor 2. */
    if (optionsvideo_cursor == 4 && !vid_menudrawfn) {
	if (k == K_UPARROW)
	    optionsvideo_cursor = 3;
	else
	    optionsvideo_cursor = 5;
    }
    if ((optionsvideo_cursor == 5) && VID_IsFullScreen()) {
	if (k == K_UPARROW) {
	    if (!vid_menudrawfn)
		optionsvideo_cursor = 3;
	    else
		optionsvideo_cursor = 4;
	} else
      optionsvideo_cursor = 6;
    }
}

/* ============================================================================= */
/* AUDIO OPTIONS MENU */

#define	OPTIONSAUDIO_ITEMS	2

static int optionsaudio_cursor;

void
M_Menu_OptionsAudio_f(void)
{
    key_dest = key_menu;
    m_state = m_optionsaudio;
    m_entersound = true;
}


static void
M_OptionsAudio_AdjustSliders(int dir)
{
    S_LocalSound("misc/menu3.wav");

    switch (optionsaudio_cursor) {
    case 0:			/* music volume */
	/* 0.1 steps like the sound slider.  The old _WIN32 path used a 1.0
	 * step (so only off/full) because WinQuake's music volume drove the
	 * OS CD-audio mixer, which was effectively on/off.  Here music is
	 * software-mixed (bgmvolume scales the streamed BGM), so it takes a
	 * fine step on every platform. */
	bgmvolume.value += dir * 0.1;
	if (bgmvolume.value < 0)
	    bgmvolume.value = 0;
	if (bgmvolume.value > 1)
	    bgmvolume.value = 1;
	Cvar_SetValue("bgmvolume", bgmvolume.value);
	break;
    case 1:			/* sfx volume */
	sfxvolume.value += dir * 0.1;
	if (sfxvolume.value < 0)
	    sfxvolume.value = 0;
	if (sfxvolume.value > 1)
	    sfxvolume.value = 1;
	Cvar_SetValue("volume", sfxvolume.value);
	break;
    }
}


static void
M_OptionsAudio_Draw(void)
{
    float r;
    const qpic_t *p;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(16, 32, "          Music Volume");
    r = bgmvolume.value;
    M_DrawSlider(220, 32, r);

    M_Print(16, 40, "          Sound Volume");
    r = sfxvolume.value;
    M_DrawSlider(220, 40, r);

/* cursor */
    M_DrawCharacter(200, 32 + optionsaudio_cursor * 8,
		    12 + ((int)(realtime * 4) & 1));
}


static void
M_OptionsAudio_Key(int k)
{

    switch (k) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Options_f();
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;
	switch (optionsaudio_cursor) {
	default:
	    M_OptionsAudio_AdjustSliders(1);
	    break;
	}
	return;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	optionsaudio_cursor--;
	if (optionsaudio_cursor < 0)
	    optionsaudio_cursor = OPTIONSAUDIO_ITEMS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	optionsaudio_cursor++;
	if (optionsaudio_cursor >= OPTIONSAUDIO_ITEMS)
	    optionsaudio_cursor = 0;
	break;

    case K_JOY_LEFT:
    case K_LEFTARROW:
	M_OptionsAudio_AdjustSliders(-1);
	break;

    case K_JOY_RIGHT:
    case K_RIGHTARROW:
	M_OptionsAudio_AdjustSliders(1);
	break;
    }
}

/* ============================================================================= */
/* GAME OPTIONS MENU */

#define	OPTIONSGAME_ITEMS	7

static int optionsgame_cursor;

void
M_Menu_OptionsGame_f(void)
{
    key_dest = key_menu;
    m_state = m_optionsgame;
    m_entersound = true;
}

static void
M_OptionsGame_AdjustSliders(int dir)
{
    cvar_t *cvar = NULL;
    S_LocalSound("misc/menu3.wav");

    switch (optionsgame_cursor) {
    case 0:			/* allways run */
	if (cl_forwardspeed.value > 200) {
	    Cvar_SetValue("cl_forwardspeed", 200);
	    Cvar_SetValue("cl_backspeed", 200);
	} else {
	    Cvar_SetValue("cl_forwardspeed", 400);
	    Cvar_SetValue("cl_backspeed", 400);
	}
	break;
    case 1:			/* lookspring */
	Cvar_SetValue("lookspring", !lookspring.value);
	break;

    case 2:			/* lookstrafe */
	Cvar_SetValue("lookstrafe", !lookstrafe.value);
	break;
   case 3:
       cvar = Cvar_FindVar("crosshair");
       Cvar_SetValue("crosshair", cvar->value ? 0.0f : 1.0f);
       break;
   case 4:
       cvar = Cvar_FindVar("chase_type");
       Cvar_SetValue("chase_type", (cvar->value) ? 0 : 1);
       break;
   case 5:
       cvar = Cvar_FindVar("chase_active");
       Cvar_SetValue("chase_active", (cvar->value) ? 0 : 1);
       break;
   case 6:
       cvar = Cvar_FindVar("r_persistgibs");
       Cvar_SetValue("r_persistgibs", cvar->value ? 0.0f : 1.0f);
       /* Toggling off clears the existing pool so the user gets
        * immediate feedback instead of waiting for the next level
        * change.  The capture path on the new state simply stops
        * adding more. */
       if (!cvar->value)
           CL_ClearPersistGibs();
       break;
    }
}


static void
M_OptionsGame_Draw(void)
{
    const qpic_t *p;
    cvar_t *cvar = NULL;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(16, 32, "            Always Run");
    M_DrawCheckbox(220, 32, cl_forwardspeed.value > 200);

    M_Print(16, 40, "            Lookspring");
    M_DrawCheckbox(220, 40, lookspring.value);

    M_Print(16, 48, "            Lookstrafe");
    M_DrawCheckbox(220, 48, lookstrafe.value);

    cvar = Cvar_FindVar("crosshair");
    M_Print(16, 56, "             Crosshair");
    M_DrawCheckbox(220, 56, cvar->value);

    cvar = Cvar_FindVar("chase_type");
    M_Print(16, 64, "           Camera Type");

    if (cvar->value == 1)
       M_Print(220,64, "Clamped");
    else
       M_Print(220,64,"Clipped");

    cvar = Cvar_FindVar("chase_active");
    M_Print(16, 72, "          First Person");
    M_DrawCheckbox(220, 72, cvar->value ? 0 : 1);

    cvar = Cvar_FindVar("r_persistgibs");
    M_Print(16, 80, "       Persistent Gibs");
    M_DrawCheckbox(220, 80, cvar->value);

/* cursor */
    M_DrawCharacter(200, 32 + optionsgame_cursor * 8,
		    12 + ((int)(realtime * 4) & 1));
}


static void
M_OptionsGame_Key(int k)
{

    switch (k) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Options_f();
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;
	switch (optionsgame_cursor) {
	default:
	    M_OptionsGame_AdjustSliders(1);
	    break;
	}
	return;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	optionsgame_cursor--;
	if (optionsgame_cursor < 0)
	    optionsgame_cursor = OPTIONSGAME_ITEMS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	optionsgame_cursor++;
	if (optionsgame_cursor >= OPTIONSGAME_ITEMS)
	    optionsgame_cursor = 0;
	break;

    case K_JOY_LEFT:
    case K_LEFTARROW:
	M_OptionsGame_AdjustSliders(-1);
	break;

    case K_JOY_RIGHT:
    case K_RIGHTARROW:
	M_OptionsGame_AdjustSliders(1);
	break;
    }
}


/* ============================================================================= */
/* OPTIONS MENU */

#define	OPTIONS_ITEMS	4

static int options_cursor;

void
M_Menu_Options_f(void)
{
    key_dest = key_menu;
    m_state = m_options;
    m_entersound = true;
}


static void
M_Options_Draw(void)
{
    const qpic_t *p;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_option.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_Print(16, 32, "                 Input");
    M_Print(16, 40, "                 Video");
    M_Print(16, 48, "                 Audio");
    M_Print(16, 56, "                  Game");

/* cursor */
    M_DrawCharacter(200, 32 + options_cursor * 8,
		    12 + ((int)(realtime * 4) & 1));
}


static void
M_Options_Key(int k)
{
    switch (k) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Main_f();
	break;

    case K_JOY_A:
    case K_ENTER:
	m_entersound = true;
	switch (options_cursor) {
	case 0:
	    M_Menu_OptionsInput_f();
	    break;
	case 1:
	    M_Menu_OptionsVideo_f();
	    break;
	case 2:
	    M_Menu_OptionsAudio_f();
	    break;
	case 3:
	    M_Menu_OptionsGame_f();
	    break;
	}
	return;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	options_cursor--;
	if (options_cursor < 0)
	    options_cursor = OPTIONS_ITEMS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	options_cursor++;
	if (options_cursor >= OPTIONS_ITEMS)
	    options_cursor = 0;
	break;
    }
}


/* ============================================================================= */
/* KEYS MENU */

static const char *const bindnames[][2] = {
    {"+attack", "attack"},
    {"impulse 10", "next weapon"},
    {"impulse 12", "prev weapon"},
    {"+jump", "jump / swim up"},
    {"+forward", "walk forward"},
    {"+back", "backpedal"},
    {"+left", "turn left"},
    {"+right", "turn right"},
    {"+speed", "run"},
    {"+moveleft", "step left"},
    {"+moveright", "step right"},
    {"+strafe", "sidestep"},
    {"+lookup", "look up"},
    {"+lookdown", "look down"},
    {"centerview", "center view"},
    {"+mlook", "mouse look"},
    {"+klook", "keyboard look"},
    {"+moveup", "swim up"},
    {"+movedown", "swim down"}
};

#define	NUMCOMMANDS (sizeof(bindnames)/sizeof(bindnames[0]))

static int keys_cursor;
static int bind_grab;

static void
M_Menu_Keys_f(void)
{
    key_dest = key_menu;
    m_state = m_keys;
    m_entersound = true;
}


static void
M_FindKeysForCommand(const char *const command, int *twokeys)
{
    int count;
    int j;
    int l;
    const char *b;

    twokeys[0] = twokeys[1] = -1;
    l = strlen(command);
    count = 0;

    for (j = 0; j < K_LAST; j++) {
	b = keybindings[j];
	if (!b)
	    continue;
	if (!strncmp(b, command, l)) {
	    twokeys[count] = j;
	    count++;
	    if (count == 2)
		break;
	}
    }
}

static void
M_UnbindCommand(const char *const command)
{
    int j;
    int l;
    const char *b;

    l = strlen(command);

    for (j = 0; j < K_LAST; j++) {
	b = keybindings[j];
	if (!b)
	    continue;
	if (!strncmp(b, command, l))
	    Key_SetBinding((knum_t)j, NULL);
    }
}


static void
M_Keys_Draw(void)
{
    int i;
    int keys[2];
    const char *name;
    int x, y;
    const qpic_t *p;

    p = Draw_CachePic("gfx/ttl_cstm.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    if (bind_grab)
	M_Print(12, 32, "Press a key or button for this action");
    else
	M_Print(18, 32, "Enter to change, backspace to clear");

/* search for known bindings */
    for (i = 0; i < NUMCOMMANDS; i++) {
	y = 48 + 8 * i;

	M_Print(16, y, bindnames[i][1]);
	M_FindKeysForCommand(bindnames[i][0], keys);

	if (keys[0] == -1) {
	    M_Print(140, y, "???");
	} else {
	    name = Key_KeynumToString(keys[0]);
	    M_Print(140, y, name);
	    x = strlen(name) * 8;
	    if (keys[1] != -1) {
		M_Print(140 + x + 8, y, "or");
		M_Print(140 + x + 32, y, Key_KeynumToString(keys[1]));
	    }
	}
    }

    if (bind_grab)
	M_DrawCharacter(130, 48 + keys_cursor * 8, '=');
    else
	M_DrawCharacter(130, 48 + keys_cursor * 8,
			12 + ((int)(realtime * 4) & 1));
}


static void
M_Keys_Key(int k)
{
    char cmd[80];
    int keys[2];

    if (bind_grab) {		/* defining a key */
	S_LocalSound("misc/menu1.wav");
	if (k == K_ESCAPE) {
	    bind_grab = false;
	} else if (k != '`') {
	    snprintf(cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(k),
		    bindnames[keys_cursor][0]);
	    Cbuf_InsertText(cmd);
	}

	bind_grab = false;
	return;
    }

    switch (k) {

    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_OptionsInput_f();
	break;

    case K_JOY_LEFT:
    case K_JOY_UP:
    case K_LEFTARROW:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	keys_cursor--;
	if (keys_cursor < 0)
	    keys_cursor = NUMCOMMANDS - 1;
	break;

    case K_JOY_DOWN:
    case K_JOY_RIGHT:
    case K_DOWNARROW:
    case K_RIGHTARROW:
	S_LocalSound("misc/menu1.wav");
	keys_cursor++;
	if (keys_cursor >= NUMCOMMANDS)
	    keys_cursor = 0;
	break;

    case K_JOY_A:
    case K_ENTER:		/* go into bind mode */
	M_FindKeysForCommand(bindnames[keys_cursor][0], keys);
	S_LocalSound("misc/menu2.wav");
	if (keys[1] != -1)
	    M_UnbindCommand(bindnames[keys_cursor][0]);
	bind_grab = true;
	break;

    case K_JOY_X:
    case K_JOY_Y:
    case K_BACKSPACE:		/* delete bindings */
    case K_DEL:			/* delete bindings */
	S_LocalSound("misc/menu2.wav");
	M_UnbindCommand(bindnames[keys_cursor][0]);
	break;
    }
}

/* ============================================================================= */
/* VIDEO MENU */

static void
M_Menu_Video_f(void)
{
    key_dest = key_menu;
    m_state = m_video;
    m_entersound = true;
}


static void
M_Video_Draw(void)
{
    (*vid_menudrawfn) ();
}


static void
M_Video_Key(int key)
{
    (*vid_menukeyfn) (key);
}

/* ============================================================================= */
/* HELP MENU */

#define NUM_HELP_PAGES 6

static int help_page;

static void
M_Menu_Help_f(void)
{
    key_dest = key_menu;
    m_state = m_help;
    m_entersound = true;
    help_page = 0;
}


static void
M_Help_Draw(void)
{
    M_DrawPic(0, 0, Draw_CachePic(va("gfx/help%i.lmp", help_page)));
}


static void
M_Help_Key(int key)
{
    switch (key) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_Main_f();
	break;

    case K_JOY_UP:
    case K_JOY_RIGHT:
    case K_UPARROW:
    case K_RIGHTARROW:
	m_entersound = true;
	if (++help_page >= NUM_HELP_PAGES)
	    help_page = 0;
	break;

    case K_JOY_DOWN:
    case K_JOY_LEFT:
    case K_DOWNARROW:
    case K_LEFTARROW:
	m_entersound = true;
	if (--help_page < 0)
	    help_page = NUM_HELP_PAGES - 1;
	break;
    }

}

/* ============================================================================= */
/* QUIT MENU */

static int msgNumber;
static int m_quit_prevstate;
static qboolean wasInMenus;

static const char *const quitMessage[] = {
    "  Are you gonna quit    ",
    "  this game just like   ",
    "   everything else?     ",
    "                        ",

    " Milord, methinks that  ",
    "   thou art a lowly     ",
    " quitter. Is this true? ",
    "                        ",

    " Do I need to bust your ",
    "  face open for trying  ",
    "        to quit?        ",
    "                        ",

    " Man, I oughta smack you",
    "   for trying to quit!  ",
    "     Press Y to get     ",
    "      smacked out.      ",

    " Press Y to quit like a ",
    "   big loser in life.   ",
    "  Press N to stay proud ",
    "    and successful!     ",

    "   If you press Y to    ",
    "  quit, I will summon   ",
    "  Satan all over your   ",
    "      hard drive!       ",

    "  Um, Asmodeus dislikes ",
    " his children trying to ",
    " quit. Press Y to return",
    "   to your Tinkertoys.  ",

    "  If you quit now, I'll ",
    "  throw a blanket-party ",
    "   for you next time!   ",
    "                        "
};


void
M_Menu_Quit_f(void)
{
    if (m_state == m_quit)
	return;
    wasInMenus = (key_dest == key_menu);
    key_dest = key_menu;
    m_quit_prevstate = m_state;
    m_state = m_quit;
    m_entersound = true;
    msgNumber = rand() & 7;
}


static void
M_Quit_Key(int key)
{
#if 0
    switch (key) {
    case K_ESCAPE:
    case 'n':
    case 'N':
	if (wasInMenus) {
	    m_state = (m_state_enum)m_quit_prevstate;
	    m_entersound = true;
	} else {
	    key_dest = key_game;
	    m_state = m_none;
	}
	break;

    case 'Y':
    case 'y':
	key_dest = key_console;
	Host_Quit_f();
	break;

    default:
	break;
    }
#else
   extern bool shutdown_core;
	key_dest = key_console;
	Host_Quit_f();
   shutdown_core = true;
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
#endif
}


static void
M_Quit_Draw(void)
{
    if (wasInMenus) {
	m_state = m_quit_prevstate;
	m_recursiveDraw = true;
	M_Draw();
	m_state = m_quit;
    }

    M_DrawTextBox(56, 76, 24, 4);
    M_Print(64, 84, quitMessage[msgNumber * 4 + 0]);
    M_Print(64, 92, quitMessage[msgNumber * 4 + 1]);
    M_Print(64, 100, quitMessage[msgNumber * 4 + 2]);
    M_Print(64, 108, quitMessage[msgNumber * 4 + 3]);
}

/* ============================================================================= */
/* LAN CONFIG MENU */

static int lanConfig_cursor = -1;
static int lanConfig_cursor_table[] = { 72, 92, 124 };

#define NUM_LANCONFIG_CMDS 3

static int lanConfig_port;
static char lanConfig_portname[6];
static char lanConfig_joinname[22];

static void
M_Menu_LanConfig_f(void)
{
    key_dest = key_menu;
    m_state = m_lanconfig;
    m_entersound = true;
    if (lanConfig_cursor == -1) {
	if (JoiningGame)
	    lanConfig_cursor = 2;
	else
	    lanConfig_cursor = 1;
    }
    if (StartingGame && lanConfig_cursor == 2)
	lanConfig_cursor = 1;
    lanConfig_port = DEFAULTnet_hostport;
    snprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", lanConfig_port);

    m_return_onerror = false;
    m_return_reason[0] = 0;
}


static void
M_LanConfig_Draw(void)
{
    const qpic_t *p;
    int basex;
    const char *startJoin;
    const char *protocol;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_multi.lmp");
    basex = (320 - p->width) / 2;
    M_DrawPic(basex, 4, p);

    if (StartingGame)
	startJoin = "New Game";
    else
	startJoin = "Join Game";
    protocol = "TCP/IP";
    M_Print(basex, 32, va("%s - %s", startJoin, protocol));
    basex += 8;

    M_Print(basex, 52, "Address:");
    M_Print(basex + 9 * 8, 52, my_tcpip_address);

    M_Print(basex, lanConfig_cursor_table[0], "Port");
    M_DrawTextBox(basex + 8 * 8, lanConfig_cursor_table[0] - 8, 6, 1);
    M_Print(basex + 9 * 8, lanConfig_cursor_table[0], lanConfig_portname);

    if (JoiningGame) {
	M_Print(basex, lanConfig_cursor_table[1],
		"Search for local games...");
	M_Print(basex, 108, "Join game at:");
	M_DrawTextBox(basex + 8, lanConfig_cursor_table[2] - 8, 22, 1);
	M_Print(basex + 16, lanConfig_cursor_table[2], lanConfig_joinname);
    } else {
	M_DrawTextBox(basex, lanConfig_cursor_table[1] - 8, 2, 1);
	M_Print(basex + 8, lanConfig_cursor_table[1], "OK");
    }

    M_DrawCharacter(basex - 8, lanConfig_cursor_table[lanConfig_cursor],
		    12 + ((int)(realtime * 4) & 1));

    if (lanConfig_cursor == 0)
	M_DrawCharacter(basex + 9 * 8 + 8 * strlen(lanConfig_portname),
			lanConfig_cursor_table[0],
			10 + ((int)(realtime * 4) & 1));

    if (lanConfig_cursor == 2)
	M_DrawCharacter(basex + 16 + 8 * strlen(lanConfig_joinname),
			lanConfig_cursor_table[2],
			10 + ((int)(realtime * 4) & 1));

    if (*m_return_reason)
	M_PrintWhite(basex, 148, m_return_reason);
}


static void
M_LanConfig_Key(int key)
{
    int l;

    switch (key) {
    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_MultiPlayer_f();
	break;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	lanConfig_cursor--;
	if (lanConfig_cursor < 0)
	    lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	lanConfig_cursor++;
	if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
	    lanConfig_cursor = 0;
	break;

    case K_JOY_A:
    case K_ENTER:
	if (lanConfig_cursor == 0)
	    break;

	m_entersound = true;

	M_ConfigureNetSubsystem();

	if (lanConfig_cursor == 1) {
	    if (StartingGame) {
		M_Menu_GameOptions_f();
		break;
	    }
	    M_Menu_Search_f();
	    break;
	}

	if (lanConfig_cursor == 2) {
	    m_return_state = m_state;
	    m_return_onerror = true;
	    key_dest = key_game;
	    m_state = m_none;
	    Cbuf_AddText("connect \"%s\"\n", lanConfig_joinname);
	    break;
	}

	break;

    case K_JOY_X:
    case K_JOY_Y:
    case K_BACKSPACE:
	if (lanConfig_cursor == 0) {
	    if (strlen(lanConfig_portname))
		lanConfig_portname[strlen(lanConfig_portname) - 1] = 0;
	}

	if (lanConfig_cursor == 2) {
	    if (strlen(lanConfig_joinname))
		lanConfig_joinname[strlen(lanConfig_joinname) - 1] = 0;
	}
	break;

    default:
	if (key < 32 || key > 127)
	    break;

	if (lanConfig_cursor == 2) {
	    l = strlen(lanConfig_joinname);
	    if (l < 21) {
		lanConfig_joinname[l + 1] = 0;
		lanConfig_joinname[l] = key;
	    }
	}

	if (key < '0' || key > '9')
	    break;
	if (lanConfig_cursor == 0) {
	    l = strlen(lanConfig_portname);
	    if (l < 5) {
		lanConfig_portname[l + 1] = 0;
		lanConfig_portname[l] = key;
	    }
	}
    }

    if (StartingGame && lanConfig_cursor == 2) {
	if (key == K_UPARROW)
	    lanConfig_cursor = 1;
	else
	    lanConfig_cursor = 0;
    }

    l = Q_atoi(lanConfig_portname);
    lanConfig_port = qmin(l, 65535);
    snprintf(lanConfig_portname, sizeof(lanConfig_portname), "%u", lanConfig_port);
}

/* ============================================================================= */
/* GAME OPTIONS MENU */

typedef struct {
    const char *const name;
    const char *const description;
} level_t;

/* FIXME - make this dynamic? get strings from bsps, scanning map dirs? */
static const level_t levels[] = {
    {"start", "Entrance"},	/* 0 */

    {"e1m1", "Slipgate Complex"},	/* 1 */
    {"e1m2", "Castle of the Damned"},
    {"e1m3", "The Necropolis"},
    {"e1m4", "The Grisly Grotto"},
    {"e1m5", "Gloom Keep"},
    {"e1m6", "The Door To Chthon"},
    {"e1m7", "The House of Chthon"},
    {"e1m8", "Ziggurat Vertigo"},

    {"e2m1", "The Installation"},	/* 9 */
    {"e2m2", "Ogre Citadel"},
    {"e2m3", "Crypt of Decay"},
    {"e2m4", "The Ebon Fortress"},
    {"e2m5", "The Wizard's Manse"},
    {"e2m6", "The Dismal Oubliette"},
    {"e2m7", "Underearth"},

    {"e3m1", "Termination Central"},	/* 16 */
    {"e3m2", "The Vaults of Zin"},
    {"e3m3", "The Tomb of Terror"},
    {"e3m4", "Satan's Dark Delight"},
    {"e3m5", "Wind Tunnels"},
    {"e3m6", "Chambers of Torment"},
    {"e3m7", "The Haunted Halls"},

    {"e4m1", "The Sewage System"},	/* 23 */
    {"e4m2", "The Tower of Despair"},
    {"e4m3", "The Elder God Shrine"},
    {"e4m4", "The Palace of Hate"},
    {"e4m5", "Hell's Atrium"},
    {"e4m6", "The Pain Maze"},
    {"e4m7", "Azure Agony"},
    {"e4m8", "The Nameless City"},

    {"end", "Shub-Niggurath's Pit"},	/* 31 */

    {"dm1", "Place of Two Deaths"},	/* 32 */
    {"dm2", "Claustrophobopolis"},
    {"dm3", "The Abandoned Base"},
    {"dm4", "The Bad Place"},
    {"dm5", "The Cistern"},
    {"dm6", "The Dark Zone"}
};

/* MED 01/06/97 added hipnotic levels */
static const level_t hipnoticlevels[] = {
    {"start", "Command HQ"},	/* 0 */

    {"hip1m1", "The Pumping Station"},	/* 1 */
    {"hip1m2", "Storage Facility"},
    {"hip1m3", "The Lost Mine"},
    {"hip1m4", "Research Facility"},
    {"hip1m5", "Military Complex"},

    {"hip2m1", "Ancient Realms"},	/* 6 */
    {"hip2m2", "The Black Cathedral"},
    {"hip2m3", "The Catacombs"},
    {"hip2m4", "The Crypt"},
    {"hip2m5", "Mortum's Keep"},
    {"hip2m6", "The Gremlin's Domain"},

    {"hip3m1", "Tur Torment"},	/* 12 */
    {"hip3m2", "Pandemonium"},
    {"hip3m3", "Limbo"},
    {"hip3m4", "The Gauntlet"},

    {"hipend", "Armagon's Lair"},	/* 16 */

    {"hipdm1", "The Edge of Oblivion"}	/* 17 */
};

/* PGM 01/07/97 added rogue levels */
/* PGM 03/02/97 added dmatch level */
static const level_t roguelevels[] = {
    {"start", "Split Decision"},
    {"r1m1", "Deviant's Domain"},
    {"r1m2", "Dread Portal"},
    {"r1m3", "Judgement Call"},
    {"r1m4", "Cave of Death"},
    {"r1m5", "Towers of Wrath"},
    {"r1m6", "Temple of Pain"},
    {"r1m7", "Tomb of the Overlord"},
    {"r2m1", "Tempus Fugit"},
    {"r2m2", "Elemental Fury I"},
    {"r2m3", "Elemental Fury II"},
    {"r2m4", "Curse of Osiris"},
    {"r2m5", "Wizard's Keep"},
    {"r2m6", "Blood Sacrifice"},
    {"r2m7", "Last Bastion"},
    {"r2m8", "Source of Evil"},
    {"ctf1", "Division of Change"}
};

typedef struct {
    const char *const description;
    int firstLevel;
    int levels;
} episode_t;

static const episode_t episodes[] = {
    {"Welcome to Quake", 0, 1},
    {"Doomed Dimension", 1, 8},
    {"Realm of Black Magic", 9, 7},
    {"Netherworld", 16, 7},
    {"The Elder World", 23, 8},
    {"Final Level", 31, 1},
    {"Deathmatch Arena", 32, 6}
};

/* MED 01/06/97  added hipnotic episodes */
static const episode_t hipnoticepisodes[] = {
    {"Scourge of Armagon", 0, 1},
    {"Fortress of the Dead", 1, 5},
    {"Dominion of Darkness", 6, 6},
    {"The Rift", 12, 4},
    {"Final Level", 16, 1},
    {"Deathmatch Arena", 17, 1}
};

/* PGM 01/07/97 added rogue episodes */
/* PGM 03/02/97 added dmatch episode */
static const episode_t rogueepisodes[] = {
    {"Introduction", 0, 1},
    {"Hell's Fortress", 1, 7},
    {"Corridors of Time", 8, 8},
    {"Deathmatch Arena", 16, 1}
};

static int startepisode;
static int startlevel;
static int maxplayers;
static qboolean m_serverInfoMessage = false;
static double m_serverInfoMessageTime;

#define	NUM_GAMEOPTIONS	9
static int gameoptions_cursor;
static int gameoptions_cursor_table[] =
    { 40, 56, 64, 72, 80, 88, 96, 112, 120 };

static void
M_Menu_GameOptions_f(void)
{
    key_dest = key_menu;
    m_state = m_gameoptions;
    m_entersound = true;
    if (maxplayers == 0)
	maxplayers = svs.maxclients;
    if (maxplayers < 2)
	maxplayers = svs.maxclientslimit;
}


static void
M_GameOptions_Draw(void)
{
    const qpic_t *p;
    int x;

    M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
    p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);

    M_DrawTextBox(152, 32, 10, 1);
    M_Print(160, 40, "begin game");

    M_Print(0, 56, "      Max players");
    M_Print(160, 56, va("%i", maxplayers));

    M_Print(0, 64, "        Game Type");
    if (coop.value)
	M_Print(160, 64, "Cooperative");
    else
	M_Print(160, 64, "Deathmatch");

    M_Print(0, 72, "        Teamplay");
    if (rogue) {
	const char *msg;

	switch ((int)teamplay.value) {
	case 1:
	    msg = "No Friendly Fire";
	    break;
	case 2:
	    msg = "Friendly Fire";
	    break;
	case 3:
	    msg = "Tag";
	    break;
	case 4:
	    msg = "Capture the Flag";
	    break;
	case 5:
	    msg = "One Flag CTF";
	    break;
	case 6:
	    msg = "Three Team CTF";
	    break;
	default:
	    msg = "Off";
	    break;
	}
	M_Print(160, 72, msg);
    } else {
	const char *msg;

	switch ((int)teamplay.value) {
	case 1:
	    msg = "No Friendly Fire";
	    break;
	case 2:
	    msg = "Friendly Fire";
	    break;
	default:
	    msg = "Off";
	    break;
	}
	M_Print(160, 72, msg);
    }

    M_Print(0, 80, "            Skill");
    if (skill.value == 0)
	M_Print(160, 80, "Easy difficulty");
    else if (skill.value == 1)
	M_Print(160, 80, "Normal difficulty");
    else if (skill.value == 2)
	M_Print(160, 80, "Hard difficulty");
    else
	M_Print(160, 80, "Nightmare difficulty");

    M_Print(0, 88, "       Frag Limit");
    if (fraglimit.value == 0)
	M_Print(160, 88, "none");
    else
	M_Print(160, 88, va("%i frags", (int)fraglimit.value));

    M_Print(0, 96, "       Time Limit");
    if (timelimit.value == 0)
	M_Print(160, 96, "none");
    else
	M_Print(160, 96, va("%i minutes", (int)timelimit.value));

    M_Print(0, 112, "         Episode");
    /* MED 01/06/97 added hipnotic episodes */
    if (hipnotic)
	M_Print(160, 112, hipnoticepisodes[startepisode].description);
    /* PGM 01/07/97 added rogue episodes */
    else if (rogue)
	M_Print(160, 112, rogueepisodes[startepisode].description);
    else
	M_Print(160, 112, episodes[startepisode].description);

    M_Print(0, 120, "           Level");
    /* MED 01/06/97 added hipnotic episodes */
    if (hipnotic) {
	M_Print(160, 120,
		hipnoticlevels[hipnoticepisodes[startepisode].firstLevel +
			       startlevel].description);
	M_Print(160, 128,
		hipnoticlevels[hipnoticepisodes[startepisode].firstLevel +
			       startlevel].name);
    }
    /* PGM 01/07/97 added rogue episodes */
    else if (rogue) {
	M_Print(160, 120,
		roguelevels[rogueepisodes[startepisode].firstLevel +
			    startlevel].description);
	M_Print(160, 128,
		roguelevels[rogueepisodes[startepisode].firstLevel +
			    startlevel].name);
    } else {
	M_Print(160, 120,
		levels[episodes[startepisode].firstLevel +
		       startlevel].description);
	M_Print(160, 128,
		levels[episodes[startepisode].firstLevel + startlevel].name);
    }

/* line cursor */
    M_DrawCharacter(144, gameoptions_cursor_table[gameoptions_cursor],
		    12 + ((int)(realtime * 4) & 1));

    if (m_serverInfoMessage) {
	if ((realtime - m_serverInfoMessageTime) < 5.0) {
	    x = (320 - 26 * 8) / 2;
	    M_DrawTextBox(x, 138, 24, 4);
	    x += 8;
	    M_Print(x, 146, "  More than 4 players   ");
	    M_Print(x, 154, " requires using command ");
	    M_Print(x, 162, "line parameters; please ");
	    M_Print(x, 170, "   see techinfo.txt.    ");
	} else {
	    m_serverInfoMessage = false;
	}
    }
}


static void
M_NetStart_Change(int dir)
{
    int count;

    switch (gameoptions_cursor) {
    case 1:
	maxplayers += dir;
	if (maxplayers > svs.maxclientslimit) {
	    maxplayers = svs.maxclientslimit;
	    m_serverInfoMessage = true;
	    m_serverInfoMessageTime = realtime;
	}
	if (maxplayers < 2)
	    maxplayers = 2;
	break;

    case 2:
	Cvar_SetValue("coop", coop.value ? 0 : 1);
	break;

    case 3:
	if (rogue)
	    count = 6;
	else
	    count = 2;

	Cvar_SetValue("teamplay", teamplay.value + dir);
	if (teamplay.value > count)
	    Cvar_SetValue("teamplay", 0);
	else if (teamplay.value < 0)
	    Cvar_SetValue("teamplay", count);
	break;

    case 4:
	Cvar_SetValue("skill", skill.value + dir);
	if (skill.value > 3)
	    Cvar_SetValue("skill", 0);
	if (skill.value < 0)
	    Cvar_SetValue("skill", 3);
	break;

    case 5:
	Cvar_SetValue("fraglimit", fraglimit.value + dir * 10);
	if (fraglimit.value > 100)
	    Cvar_SetValue("fraglimit", 0);
	if (fraglimit.value < 0)
	    Cvar_SetValue("fraglimit", 100);
	break;

    case 6:
	Cvar_SetValue("timelimit", timelimit.value + dir * 5);
	if (timelimit.value > 60)
	    Cvar_SetValue("timelimit", 0);
	if (timelimit.value < 0)
	    Cvar_SetValue("timelimit", 60);
	break;

    case 7:
	startepisode += dir;
	/* MED 01/06/97 added hipnotic count */
	if (hipnotic)
	    count = 6;
	/* PGM 01/07/97 added rogue count */
	/* PGM 03/02/97 added 1 for dmatch episode */
	else if (rogue)
	    count = 4;
	else if (registered.value)
	    count = 7;
	else
	    count = 2;

	if (startepisode < 0)
	    startepisode = count - 1;

	if (startepisode >= count)
	    startepisode = 0;

	startlevel = 0;
	break;

    case 8:
	startlevel += dir;
	/* MED 01/06/97 added hipnotic episodes */
	if (hipnotic)
	    count = hipnoticepisodes[startepisode].levels;
	/* PGM 01/06/97 added hipnotic episodes */
	else if (rogue)
	    count = rogueepisodes[startepisode].levels;
	else
	    count = episodes[startepisode].levels;

	if (startlevel < 0)
	    startlevel = count - 1;

	if (startlevel >= count)
	    startlevel = 0;
	break;
    }
}

void M_Game_StartNewGame(void)
{
   if (sv.active)
      Cbuf_AddText("disconnect\n");
   Cbuf_AddText("listen 0\n");	/* so host_netport will be re-examined */
   Cbuf_AddText("maxplayers %u\n", maxplayers);
   SCR_BeginLoadingPlaque();

   if (hipnotic)
      Cbuf_AddText("map %s\n",
            hipnoticlevels[hipnoticepisodes[startepisode].
            firstLevel +
            startlevel].name);
   else if (rogue)
      Cbuf_AddText("map %s\n",
            roguelevels[rogueepisodes[startepisode].
            firstLevel + startlevel].name);
   else
      Cbuf_AddText("map %s\n",
            levels[episodes[startepisode].firstLevel +
            startlevel].name);

}

static void
M_GameOptions_Key(int key)
{
    switch (key) {

    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_MultiPlayer_f();
	break;

    case K_JOY_UP:
    case K_UPARROW:
	S_LocalSound("misc/menu1.wav");
	gameoptions_cursor--;
	if (gameoptions_cursor < 0)
	    gameoptions_cursor = NUM_GAMEOPTIONS - 1;
	break;

    case K_JOY_DOWN:
    case K_DOWNARROW:
	S_LocalSound("misc/menu1.wav");
	gameoptions_cursor++;
	if (gameoptions_cursor >= NUM_GAMEOPTIONS)
	    gameoptions_cursor = 0;
	break;

    case K_JOY_LEFT:
    case K_LEFTARROW:
	if (gameoptions_cursor == 0)
	    break;
	S_LocalSound("misc/menu3.wav");
	M_NetStart_Change(-1);
	break;

    case K_JOY_RIGHT:
    case K_RIGHTARROW:
	if (gameoptions_cursor == 0)
	    break;
	S_LocalSound("misc/menu3.wav");
	M_NetStart_Change(1);
	break;

    case K_JOY_A:
    case K_ENTER:
	S_LocalSound("misc/menu2.wav");
	if (gameoptions_cursor == 0) {
      M_Game_StartNewGame();
      return;
	}

	M_NetStart_Change(1);
	break;
    }
}

/* ============================================================================= */
/* SEARCH MENU */

static qboolean searchComplete = false;
static double searchCompleteTime;

static void
M_Menu_Search_f(void)
{
    key_dest = key_menu;
    m_state = m_search;
    m_entersound = false;
    slistSilent = true;
    slistLocal = false;
    searchComplete = false;
    NET_Slist_f();
}


static void
M_Search_Draw(void)
{
    const qpic_t *p;
    int x;

    p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);
    x = (320 / 2) - ((12 * 8) / 2) + 4;
    M_DrawTextBox(x - 8, 32, 12, 1);
    M_Print(x, 40, "Searching...");

    if (slistInProgress) {
	NET_Poll();
	return;
    }

    if (!searchComplete) {
	searchComplete = true;
	searchCompleteTime = realtime;
    }

    if (hostCacheCount) {
	M_Menu_ServerList_f();
	return;
    }

    M_PrintWhite((320 / 2) - ((22 * 8) / 2), 64, "No Quake servers found");
    if ((realtime - searchCompleteTime) < 3.0)
	return;

    M_Menu_LanConfig_f();
}


static void
M_Search_Key(int key)
{
}

/* ============================================================================= */
/* SLIST MENU */

static int slist_cursor;
static qboolean slist_sorted;

static void
M_Menu_ServerList_f(void)
{
    key_dest = key_menu;
    m_state = m_slist;
    m_entersound = true;
    slist_cursor = 0;
    m_return_onerror = false;
    m_return_reason[0] = 0;
    slist_sorted = false;
}


static void
M_ServerList_Draw(void)
{
    int n;
    char string[64];
    const qpic_t *p;

    if (!slist_sorted) {
	if (hostCacheCount > 1) {
	    int i, j;
	    hostcache_t temp;

	    for (i = 0; i < hostCacheCount; i++)
		for (j = i + 1; j < hostCacheCount; j++)
		    if (strcmp(hostcache[j].name, hostcache[i].name) < 0) {
			memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
			memcpy(&hostcache[j], &hostcache[i],
				 sizeof(hostcache_t));
			memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
		    }
	}
	slist_sorted = true;
    }

    p = Draw_CachePic("gfx/p_multi.lmp");
    M_DrawPic((320 - p->width) / 2, 4, p);
    for (n = 0; n < hostCacheCount; n++) {
	if (hostcache[n].maxusers)
	    snprintf(string, sizeof(string), "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name,
		    hostcache[n].map, hostcache[n].users,
		    hostcache[n].maxusers);
	else
	    snprintf(string, sizeof(string), "%-15.15s %-15.15s\n", hostcache[n].name,
		    hostcache[n].map);
	M_Print(16, 32 + 8 * n, string);
    }
    M_DrawCharacter(0, 32 + slist_cursor * 8, 12 + ((int)(realtime * 4) & 1));

    if (*m_return_reason)
	M_PrintWhite(16, 148, m_return_reason);
}


static void
M_ServerList_Key(int k)
{
    switch (k) {

    case K_JOY_B:
    case K_ESCAPE:
	M_Menu_LanConfig_f();
	break;

    case K_JOY_X:
    case K_JOY_Y:
    case K_SPACE:
	M_Menu_Search_f();
	break;

    case K_JOY_UP:
    case K_JOY_LEFT:
    case K_UPARROW:
    case K_LEFTARROW:
	S_LocalSound("misc/menu1.wav");
	slist_cursor--;
	if (slist_cursor < 0)
	    slist_cursor = hostCacheCount - 1;
	break;

    case K_JOY_DOWN:
    case K_JOY_RIGHT:
    case K_DOWNARROW:
    case K_RIGHTARROW:
	S_LocalSound("misc/menu1.wav");
	slist_cursor++;
	if (slist_cursor >= hostCacheCount)
	    slist_cursor = 0;
	break;

    case K_JOY_START:
    case K_ENTER:
	S_LocalSound("misc/menu2.wav");
	m_return_state = m_state;
	m_return_onerror = true;
	slist_sorted = false;
	key_dest = key_game;
	m_state = m_none;
	Cbuf_AddText("connect \"%s\"\n", hostcache[slist_cursor].cname);
	break;

    default:
	break;
    }

}

/* ============================================================================= */
/* Menu Subsystem */


void
M_Init(void)
{
    Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

    Cmd_AddCommand("menu_main", M_Menu_Main_f);
    Cmd_AddCommand("menu_singleplayer", M_Menu_SinglePlayer_f);
    Cmd_AddCommand("menu_load", M_Menu_Load_f);
    Cmd_AddCommand("menu_save", M_Menu_Save_f);
    Cmd_AddCommand("menu_multiplayer", M_Menu_MultiPlayer_f);
    Cmd_AddCommand("menu_setup", M_Menu_Setup_f);
    Cmd_AddCommand("menu_options", M_Menu_Options_f);
    Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
    Cmd_AddCommand("menu_video", M_Menu_Video_f);
    Cmd_AddCommand("help", M_Menu_Help_f);
    Cmd_AddCommand("menu_quit", M_Menu_Quit_f);
}


void
M_Draw(void)
{
   if (m_state == m_none || key_dest != key_menu)
      return;

   if (!m_recursiveDraw) {
      scr_copyeverything = 1;

      if (scr_con_current) {
         Draw_ConsoleBackground(vid.height);
      } else
         Draw_FadeScreen();

      scr_fullupdate = 0;
   } else {
      m_recursiveDraw = false;
   }

   switch (m_state) {
      case m_none:
         break;

      case m_main:
         M_Main_Draw();
         break;

      case m_singleplayer:
         M_SinglePlayer_Draw();
         break;

      case m_load:
         M_Load_Draw();
         break;

      case m_save:
         M_Save_Draw();
         break;

      case m_multiplayer:
         M_MultiPlayer_Draw();
         break;

      case m_setup:
         M_Setup_Draw();
         break;

      case m_options:
         M_Options_Draw();
         break;

      case m_optionsinput:
         M_OptionsInput_Draw();
         break;

      case m_optionsvideo:
         M_OptionsVideo_Draw();
         break;

      case m_optionsaudio:
         M_OptionsAudio_Draw();
         break;

      case m_optionsgame:
         M_OptionsGame_Draw();
         break;

      case m_keys:
         M_Keys_Draw();
         break;

      case m_video:
         M_Video_Draw();
         break;

      case m_help:
         M_Help_Draw();
         break;

      case m_quit:
         M_Quit_Draw();
         break;

      case m_lanconfig:
         M_LanConfig_Draw();
         break;

      case m_gameoptions:
         M_GameOptions_Draw();
         break;

      case m_search:
         M_Search_Draw();
         break;

      case m_slist:
         M_ServerList_Draw();
         break;
   }

   if (m_entersound) {
      S_LocalSound("misc/menu2.wav");
      m_entersound = false;
   }
}


void
M_Keydown(int key)
{
    switch (m_state) {
    case m_none:
	return;

    case m_main:
	M_Main_Key(key);
	return;

    case m_singleplayer:
	M_SinglePlayer_Key(key);
	return;

    case m_load:
	M_Load_Key(key);
	return;

    case m_save:
	M_Save_Key(key);
	return;

    case m_multiplayer:
	M_MultiPlayer_Key(key);
	return;

    case m_setup:
	M_Setup_Key(key);
	return;

    case m_options:
	M_Options_Key(key);
	return;

    case m_optionsinput:
	M_OptionsInput_Key(key);
	return;

    case m_optionsvideo:
	M_OptionsVideo_Key(key);
	return;

    case m_optionsaudio:
	M_OptionsAudio_Key(key);
	return;

    case m_optionsgame:
	M_OptionsGame_Key(key);
	return;

    case m_keys:
	M_Keys_Key(key);
	return;

    case m_video:
	M_Video_Key(key);
	return;

    case m_help:
	M_Help_Key(key);
	return;

    case m_quit:
	M_Quit_Key(key);
	return;

    case m_lanconfig:
	M_LanConfig_Key(key);
	return;

    case m_gameoptions:
	M_GameOptions_Key(key);
	return;

    case m_search:
	M_Search_Key(key);
	break;

    case m_slist:
	M_ServerList_Key(key);
	return;
    }
}


static void
M_ConfigureNetSubsystem(void)
{
    /* enable/disable net systems to match desired config */
    Cbuf_AddText("stopdemo\n");
    net_hostport = lanConfig_port;
}
