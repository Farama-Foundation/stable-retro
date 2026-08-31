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

#ifndef MENU_H
#define MENU_H

#include "wad.h"

/**/
/* menus */
/**/
void M_Init(void);
void M_Keydown(int key);
void M_Draw(void);

void M_ToggleMenu_f(void);
void M_Menu_Options_f(void);
void M_Menu_Quit_f(void);
void M_Game_StartNewGame(void);

void M_DrawPic(int x, int y, const qpic_t *pic);
void M_DrawCharacter(int cx, int line, int num);
void M_DrawTextBox(int x, int y, int width, int lines);
void M_Print(int cx, int cy, const char *str);
void M_PrintWhite(int cx, int cy, const char *str);

/* FIXME - These are only here for NQ/net_dgrm.c */
extern qboolean m_return_onerror;
extern char m_return_reason[32];
extern int m_return_state;
#ifndef _WIN32
typedef int m_state_enum;
#endif

#endif /* MENU_H */
