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

#ifndef CVAR_H
#define CVAR_H

#include <streams/file_stream.h>

#include "shell.h"
#include "qtypes.h"

/* cvar.h */

/*

cvar_t variables are used to hold scalar or string variables that can be
changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or you
can add a ,true flag for variables that you want saved to the configuration
file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",true};

Cvars must be registered before use, or they will have a 0 value instead of
the float interpretation of the string.  Generally, all cvar_t declarations
should be registered in the apropriate init function before any console
commands are executed:
	Cvar_RegisterVariable (&teamplay);

C code usually just references a cvar in place:
	if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
	if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or cvar_set (name,
value) internal functions:
	teamplay = cvar("teamplay");
	cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
	r_draworder		prints the current value
	r_draworder 0		sets the current value to 0

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

Callback is a function that is called when the value of the cvar is changed.

*/

struct cvar_s;

typedef void (*cvar_callback) (struct cvar_s *);

/*
 * Cvar argument completion function.
 * Pass in the argument string
 * Returns a string tree of possible completions
 * Requires STree_AllocInit() prior to calling
 */
typedef struct stree_root *(*cvar_arg_f)(const char *);

typedef struct cvar_s {
    const char *name;
    const char *string;
    qboolean archive;	/* set to true to cause it to be saved to vars.rc */

    /* FIXME - obviously... */
    qboolean server;	/* NQ: notifies players when changed */

    float value;
    cvar_callback callback;
    unsigned flags;
    struct stree_node stree; /* string tree for cvar names */
    cvar_arg_f completion;
    const char *default_string; /* captured compile-time default; survives Cvar_Shutdown */
} cvar_t;

#define CVAR_DEVELOPER (1U << 0) /* can't set during normal play */
#define CVAR_OBSOLETE  (1U << 1) /* cvar has no effect; basically removed */
#define CVAR_CALLBACK  (1U << 2)

/*
 * register a cvar that already has the name, string, and optionally the
 * archive elements set.
 */
void Cvar_RegisterVariable(cvar_t *variable);
void Cvar_Shutdown(void);

void Cvar_SetCallback(cvar_t *var, cvar_callback func);

/* equivelant to "<name> <variable>" typed at the console */
void Cvar_Set(const char *var_name, const char *value);

/* expands value to a string and calls Cvar_Set */
void Cvar_SetValue(const char *var_name, float value);

/* returns 0 if not defined or non numeric */
float Cvar_VariableValue(const char *var_name);

/* returns an empty string if not defined */
const char *Cvar_VariableString(const char *var_name);

/*
 * called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known command.
 * Returns true if the command was a variable reference that was
 * handled. (print or change)
 */
qboolean Cvar_Command(void);

/*
 * Writes lines containing "set variable value" for all variables with the
 * archive flag set to true.
 */
void Cvar_WriteVariables(RFILE *f);

/* */
cvar_t *Cvar_FindVar(const char *var_name);

char *Cvar_ArgComplete(const char *name, const char *buf);
struct stree_root *Cvar_ArgCompletions(const char *name, const char *buf);

cvar_t *Cvar_NextServerVar(const char *var_name);

extern struct stree_root cvar_tree;

#endif /* CVAR_H */
