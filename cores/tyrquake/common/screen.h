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

#ifndef SCREEN_H
#define SCREEN_H

#include "qtypes.h"
#include "cvar.h"
#include "vid.h"

/* screen.h */

void SCR_Init(void);
void SCR_UpdateScreen(void);
void SCR_UpdateWholeScreen(void);
void SCR_CenterPrint(const char *str);
void SCR_BeginLoadingPlaque(void);
void SCR_EndLoadingPlaque(void);
void SCR_AgeLoadingPlaque(float dt);

/* Integer multiplier applied to all 320x200-native UI elements (menu,
 * status bar, console). 0 in the cvar means "auto", which resolves to
 * min(vid.width/320, vid.height/200) clamped to >= 1. Non-zero values
 * are clamped to 1..MAX_UI_SCALE. Cheap (a handful of arith ops); safe
 * to call per draw. */
#define MAX_UI_SCALE 8
int SCR_GetUIScale(void);
extern cvar_t scr_uiscale;

extern float scr_con_current;
extern float scr_centertime_off;
extern float scr_centertime_start;
extern int scr_fullupdate;	/* set to 0 to force full redraw */
extern int clearnotify;		/* set to 0 whenever notify text is drawn */
extern qboolean scr_disabled_for_loading;
extern qboolean scr_skipupdate;
extern qboolean scr_block_drawing;
extern cvar_t scr_viewsize;
extern cvar_t scr_fov;
extern vrect_t scr_vrect;

/* only the refresh window will be updated unless these variables are flagged */
extern int scr_copytop;
extern int scr_copyeverything;

#endif /* SCREEN_H */
