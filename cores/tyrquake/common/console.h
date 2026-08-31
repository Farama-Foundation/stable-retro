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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "qtypes.h"
#include "shell.h"

/**/
/* console */
/**/
typedef struct {
    char *text;			/* Text buffer */
    int current;		/* line where next message will be printed */
    int x;			/* offset in current line for next print */
    int display;		/* bottom of console displays this line */
} console_t;

extern console_t *con;

extern int con_ormask;
extern int con_totallines;
extern int con_notifylines;	/* scan lines to clear for notify lines */

extern qboolean con_forcedup;
extern qboolean con_initialized;

void Con_DrawCharacter(int cx, int line, int num);
void Con_CheckResize(void);
void Con_Init(void);
void Con_DrawConsole(int lines);
void Con_Print(const char *txt);
void Con_Printf(const char *fmt, ...);
void Con_DPrintf(const char *fmt, ...);
void Con_SafePrintf(const char *fmt, ...);
void Con_Clear_f(void);
void Con_DrawNotify(void);
void Con_ClearNotify(void);
void Con_AgeNotifyTimes(float dt);
void Con_ToggleConsole_f(void);
void Con_ShowList(const char **list, int cnt, int maxlen);
void Con_ShowTree(struct stree_root *root);

int Con_GetWidth(void); /* return the printing width in chars*/

#endif /* CONSOLE_H */
