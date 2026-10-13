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

#ifndef SYS_H
#define SYS_H

#include <boolean.h>
#include "qtypes.h"


/* memory protection */
void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length);

/* system IO */

#define MAX_PRINTMSG 4096

void Sys_Printf(const char *fmt, ...);
void Sys_Error(const char *error, ...);

/* send text to the console */
/* Sys_Error logs the message and longjmps out to the
 * nearest retro_run / retro_load_game setjmp guard.
 * Callers must treat it as no-return -- if the longjmp
 * isn't live (early init, late shutdown), Sys_Error
 * falls through to log-and-return as a last resort, but
 * code paths must not depend on the return path. */

void Sys_Quit(void);

char *Sys_ConsoleInput(void);

/* Perform Key_Event() callbacks until the input queue is empty */
void Sys_SendKeyEvents(void);

void Sys_Init(void);

#endif /* SYS_H */
