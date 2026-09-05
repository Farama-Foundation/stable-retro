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
/* cmd.c -- Quake script command processing module */

#include "compat/strl.h"
#include <string.h>

#include "client.h"
#include "cmd.h"
#include "common.h"
#include "console.h"
#include "cvar.h"
#include "quakedef.h"
#include "shell.h"
#include "sys.h"
#include "zone.h"

#include "host.h"
#include "protocol.h"

#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s {
    char name[MAX_ALIAS_NAME];
    const char *value;
    struct stree_node stree;
} cmdalias_t;

#define cmdalias_entry(ptr) container_of(ptr, struct cmdalias_s, stree)
static DECLARE_STREE_ROOT(cmdalias_tree);

static qboolean cmd_wait;

cvar_t cl_warncmd = { "cl_warncmd", "0" };

/* ============================================================================= */

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
static void
Cmd_Wait_f(void)
{
    cmd_wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

static sizebuf_t cmd_text;

/*
============
Cbuf_Init
============
*/
void
Cbuf_Init(void)
{
    SZ_Alloc(&cmd_text, 8192);
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void
Cbuf_AddText(const char *fmt, ...)
{
    va_list ap;
    int len, maxlen;
    char *buf;

    buf = (char *)cmd_text.data + cmd_text.cursize;
    maxlen = cmd_text.maxsize - cmd_text.cursize;
    /* Defensive: never pass a negative maxlen to vsnprintf;
     * size_t cast turns it into a huge number and overflows
     * the buffer.  cursize > maxsize shouldn't occur but
     * could after corruption upstream. */
    if (maxlen <= 0) {
	Con_Printf("%s: overflow\n", __func__);
	return;
    }
    va_start(ap, fmt);
    len = vsnprintf(buf, maxlen, fmt, ap);
    va_end(ap);

    if (len >= 0 && len < maxlen)
	cmd_text.cursize += len;
    else
	Con_Printf("%s: overflow\n", __func__);
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command (i.e. at the start
of the command buffer). Adds a \n to the text.
============
*/
void
Cbuf_InsertText(const char *text)
{
    int len;

    len = strlen(text);
    if (cmd_text.cursize) {
	if (cmd_text.cursize + len + 1 > cmd_text.maxsize)
	    Sys_Error("%s: overflow", __func__);

	/* move any commands still remaining in the exec buffer */
	memmove(cmd_text.data + len + 1, cmd_text.data, cmd_text.cursize);
	memcpy(cmd_text.data, text, len);
	cmd_text.data[len] = '\n';
	cmd_text.cursize += len + 1;
    } else {
	Cbuf_AddText("%s\n", text);
    }
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute(void)
{
   int len;
   char line[1024];

   while (cmd_text.cursize)
   {
      /* find a \n or ; line break */
      char *text = (char *)cmd_text.data;
      int quotes = 0;
      int maxlen = qmin(cmd_text.cursize, (int)sizeof(line));
      qboolean truncated = false;

      for (len = 0; len < maxlen; len++)
      {
         if (text[len] == '"')
            quotes++;
         if (!(quotes & 1) && text[len] == ';')
            break;		/* don't break if inside a quoted string */
         if (text[len] == '\n')
            break;
      }
      if (len == sizeof(line)) {
         /* The command line is longer than our line[1024] copy
          * buffer.  Previously we just decremented len to fit and
          * advanced the buffer by 1024 bytes -- which left the
          * remaining ~N-1024 bytes of the same command still in
          * the buffer, where the next iteration parsed them as
          * fresh commands.  An attacker controlling an svc_stuff
          * text payload could exploit this to smuggle an
          * unintended command through the truncation tail.
          *
          * Instead, scan the rest of the command (still within
          * the buffer, just beyond our copy buffer's reach) up
          * to the real ';' or '\n' separator and discard the
          * whole thing.  We continue the same quote-state
          * tracking from above so quoted ';' inside the
          * truncated portion don't end the command early. */
         int scan;
         truncated = true;
         for (scan = (int)sizeof(line); scan < cmd_text.cursize; scan++) {
            if (text[scan] == '"')
               quotes++;
            if (!(quotes & 1) && text[scan] == ';')
               break;
            if (text[scan] == '\n')
               break;
         }
         len = scan;
         Con_Printf("%s: command too long, discarded\n", __func__);
      }

      if (!truncated) {
         memcpy(line, text, len);
         line[len] = 0;
      }

      /*
       * delete the text from the command buffer and move remaining commands
       * down this is necessary because commands (exec, alias) can insert
       * data at the beginning of the text buffer
       */
      if (len == cmd_text.cursize)
         cmd_text.cursize = 0;
      else {
         len++; /* skip the terminating character */
         cmd_text.cursize -= len;
         memmove(text, text + len, cmd_text.cursize);
      }

      /* execute the command line (skip the truncated one) */
      if (!truncated)
         Cmd_ExecuteString(line, src_command);

      if (cmd_wait)
      {
         /*
          * skip out while text still remains in buffer, leaving it for
          * next frame
          */
         cmd_wait = false;
         break;
      }
   }
}

/*
==============================================================================

				SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
static void
Cmd_StuffCmds_f(void)
{
    int i, j;
    int s;
    char *text, *build, c;

    if (Cmd_Argc() != 1) {
	Con_Printf("stuffcmds : execute command line parameters\n");
	return;
    }
/* build the combined string to parse from */
    s = 0;
    for (i = 1; i < com_argc; i++) {
	if (!com_argv[i])
	    continue;		/* NEXTSTEP nulls out -NXHost */
	s += strlen(com_argv[i]) + 1;
    }
    if (!s)
	return;

    text = (char*)Z_Malloc(s + 1);
    text[0] = 0;
    for (i = 1; i < com_argc; i++) {
	if (!com_argv[i])
	    continue;		/* NEXTSTEP nulls out -NXHost */
	strlcat(text, com_argv[i], s + 1);
	if (i != com_argc - 1)
	    strlcat(text, " ", s + 1);
    }

/* pull out the commands */
    build = (char*)Z_Malloc(s + 1);
    build[0] = 0;

    for (i = 0; i < s - 1; i++) {
	if (text[i] == '+') {
	    i++;

	    for (j = i;
		 (text[j] != '+') && (text[j] != '-') && (text[j] != 0); j++);

	    c = text[j];
	    text[j] = 0;

	    strlcat(build, text + i, s + 1);
	    strlcat(build, "\n", s + 1);
	    text[j] = c;
	    i = j - 1;
	}
    }

    if (build[0])
	Cbuf_InsertText(build);

    Z_Free(text);
    Z_Free(build);
}


/*
===============
Cmd_Exec_f
===============
*/
static void
Cmd_Exec_f(void)
{
    char *f;
    int mark;

    if (Cmd_Argc() != 2) {
	Con_Printf("exec <filename> : execute a script file\n");
	return;
    }
    /* FIXME: is this safe freeing the hunk here??? */
    mark = Hunk_LowMark();
    f = (char*)COM_LoadHunkFile(Cmd_Argv(1));
    if (!f) {
	Con_Printf("couldn't exec %s\n", Cmd_Argv(1));
	return;
    }
    if (cl_warncmd.value || developer.value)
	Con_Printf("execing %s\n", Cmd_Argv(1));

    Cbuf_InsertText(f);
    Hunk_FreeToLowMark(mark);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void
Cmd_Echo_f(void)
{
    int i;

    for (i = 1; i < Cmd_Argc(); i++)
	Con_Printf("%s ", Cmd_Argv(i));
    Con_Printf("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/

static struct cmdalias_s *
Cmd_Alias_Find(const char *name)
{
    struct cmdalias_s *ret = NULL;
    struct stree_node *n;

    n = STree_Find(&cmdalias_tree, name);
    if (n)
	ret = cmdalias_entry(n);

    return ret;
}

static void
Cmd_Alias_f(void)
{
    cmdalias_t *a;
    char cmd[1024];
    int i, c;
    const char *s;
    char *newval;
    size_t cmd_len;
    struct stree_node *node;

    if (Cmd_Argc() == 1) {
	Con_Printf("Current alias commands:\n");
	STree_ForEach_After_NullStr(&cmdalias_tree, node) {
	    a = cmdalias_entry(node);
	    Con_Printf("%s : %s\n", a->name, a->value);
	}
	return;
    }

    s = Cmd_Argv(1);
    if (strlen(s) >= MAX_ALIAS_NAME) {
	Con_Printf("Alias name is too long\n");
	return;
    }

    /* if the alias already exists, reuse it */
    a = Cmd_Alias_Find(s);
    if (a)
	Z_Free((void *)a->value);

    if (!a) {
	a = (cmdalias_t*)Z_Malloc(sizeof(cmdalias_t));
	strlcpy(a->name, s, sizeof(a->name));
	a->stree.string = a->name;
	STree_Insert(&cmdalias_tree, &a->stree);
    }

/* copy the rest of the command line */
    cmd[0] = 0;			/* start out with a null string */
    c = Cmd_Argc();
    cmd_len = 1;
    for (i = 2; i < c; i++) {
	cmd_len += strlen(Cmd_Argv(i));
	if (i != c - 1)
		cmd_len++;
	/* Reserve one byte for the trailing "\n" appended below. */
	if (cmd_len >= sizeof(cmd) - 1) {
	    Con_Printf("Alias value is too long\n");
	    cmd[0] = 0;	/* nullify the string */
	    break;
	}
	strlcat(cmd, Cmd_Argv(i), sizeof(cmd));
	if (i != c - 1)
	    strlcat(cmd, " ", sizeof(cmd));
    }
    strlcat(cmd, "\n", sizeof(cmd));

    newval = (char*)Z_Malloc(strlen(cmd) + 1);
    memcpy(newval, cmd, strlen(cmd) + 1);
    a->value = newval;
}

/*
=============================================================================

				COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s {
    const char *name;
    xcommand_t function;
    cmd_arg_f completion;
    struct stree_node stree;
} cmd_function_t;

#define cmd_entry(ptr) container_of(ptr, struct cmd_function_s, stree)
static DECLARE_STREE_ROOT(cmd_tree);

#define	MAX_ARGS		80
static int cmd_argc;
static const char *cmd_argv[MAX_ARGS];
static const char *cmd_null_string = "";
static const char *cmd_args = NULL;

cmd_source_t cmd_source;

/*
============
Cmd_Init
============
*/
void
Cmd_Init(void)
{
/**/
/* register our commands */
/**/
    Cmd_AddCommand("stuffcmds", Cmd_StuffCmds_f);
    Cmd_AddCommand("exec", Cmd_Exec_f);
    Cmd_AddCommand("echo", Cmd_Echo_f);
    Cmd_AddCommand("alias", Cmd_Alias_f);
    Cmd_AddCommand("wait", Cmd_Wait_f);
    Cmd_AddCommand("cmd", Cmd_ForwardToServer);
}

/*
============
Cmd_Shutdown

Reset cmd_tree and cmdalias_tree to their empty state.

Both trees hold nodes that live inside dynamically-allocated structs:
cmd_function_t is Hunk_Alloc'd by Cmd_AddCommand; cmdalias_t is
Z_Malloc'd by Cmd_Alias_f.  When the libretro frontend frees the
backing heap between deinit and the next load_game, those structs
disappear -- but the tree's internal pointers still reference them,
and the very first STree_Find on the next session would dereference
freed memory.

Resetting the trees to STREE_ROOT throws away the stale links.  The
freed memory is reclaimed by the heap free; we don't need to (and
cannot) Hunk_Free or Z_Free the individual structs.
============
*/
void
Cmd_Shutdown(void)
{
    /* Reset both trees to the same empty layout that
     * DECLARE_STREE_ROOT produces at file scope. */
    memset(&cmd_tree,      0, sizeof(cmd_tree));
    memset(&cmdalias_tree, 0, sizeof(cmdalias_tree));
    cmd_tree.minlen      = (unsigned int)-1;
    cmdalias_tree.minlen = (unsigned int)-1;
}

/*
============
Cmd_Argc
============
*/
int
Cmd_Argc(void)
{
    return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
const char *
Cmd_Argv(int arg)
{
    if (arg >= cmd_argc)
	return cmd_null_string;
    return cmd_argv[arg];
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
const char *
Cmd_Args(void)
{
    /* FIXME - check necessary? */
    if (!cmd_args)
	return "";
    return cmd_args;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void
Cmd_TokenizeString(const char *text)
{
    int i;
    char *arg;

/* clear the args from the last string */
    for (i = 0; i < cmd_argc; i++)
	Z_Free((void *)cmd_argv[i]);

    cmd_argc = 0;
    cmd_args = NULL;

    while (1) {
/* skip whitespace up to a /n */
	while (*text && *text <= ' ' && *text != '\n') {
	    text++;
	}

	if (*text == '\n') {	/* a newline seperates commands in the buffer */
	    text++;
	    break;
	}

	if (!*text)
	    return;

	if (cmd_argc == 1)
	    cmd_args = text;

	text = COM_Parse(text);
	if (!text)
	    return;

	if (cmd_argc < MAX_ARGS) {
	    size_t toklen = strlen(com_token) + 1;
	    arg = (char*)Z_Malloc(toklen);
	    memcpy(arg, com_token, toklen);
	    cmd_argv[cmd_argc] = arg;
	    cmd_argc++;
	}
    }
}

static struct cmd_function_s *
Cmd_FindCommand(const char *name)
{
    struct cmd_function_s *ret = NULL;
    struct stree_node *n;

    n = STree_Find(&cmd_tree, name);
    if (n)
	ret = cmd_entry(n);

    return ret;
}

/*
============
Cmd_AddCommand
============
*/
void
Cmd_AddCommand(const char *cmd_name, xcommand_t function)
{
    cmd_function_t *cmd;

    if (host_initialized)	/* because hunk allocation would get stomped */
	Sys_Error("%s: called after host_initialized", __func__);

/* fail if the command is a variable name */
    if (Cvar_VariableString(cmd_name)[0]) {
	Con_Printf("%s: %s already defined as a var\n", __func__, cmd_name);
	return;
    }
/* fail if the command already exists */
    cmd = Cmd_FindCommand(cmd_name);
    if (cmd) {
	Con_Printf("%s: %s already defined\n", __func__, cmd_name);
	return;
    }

    cmd = (cmd_function_t*)Hunk_Alloc(sizeof(cmd_function_t));
    cmd->name = cmd_name;
    cmd->function = function;
    cmd->completion = NULL;
    cmd->stree.string = cmd->name;
    STree_Insert(&cmd_tree, &cmd->stree);
}

void
Cmd_SetCompletion(const char *cmd_name, cmd_arg_f completion)
{
    cmd_function_t *cmd;

    cmd = Cmd_FindCommand(cmd_name);
    if (cmd)
	cmd->completion = completion;
    else
	Sys_Error("%s: no such command - %s", __func__, cmd_name);
}

/*
============
Cmd_Exists
============
*/
qboolean
Cmd_Exists(const char *cmd_name)
{
    return Cmd_FindCommand(cmd_name) != NULL;
}

qboolean
Cmd_Alias_Exists(const char *cmd_name)
{
    return Cmd_Alias_Find(cmd_name) != NULL;
}


/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void
Cmd_ForwardToServer(void)
{
    if (cls.state < ca_connected) {
	Con_Printf("Can't \"%s\", not connected\n", Cmd_Argv(0));
	return;
    }

    if (cls.demoplayback)
	return;			/* not really connected */

    MSG_WriteByte(&cls.message, clc_stringcmd);
    if (strcasecmp(Cmd_Argv(0), "cmd") != 0) {
	SZ_Print(&cls.message, Cmd_Argv(0));
	SZ_Print(&cls.message, " ");
    }
    if (Cmd_Argc() > 1)
	SZ_Print(&cls.message, Cmd_Args());
    else
	SZ_Print(&cls.message, "\n");
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void
Cmd_ExecuteString(const char *text, cmd_source_t src)
{
    cmd_function_t *cmd;
    cmdalias_t *a;

    cmd_source = src;
    Cmd_TokenizeString(text);

/* execute the command line */
    if (!Cmd_Argc())
	return;			/* no tokens */

/* check functions */
    cmd = Cmd_FindCommand(cmd_argv[0]);
    if (cmd) {
	if (cmd->function)
	    cmd->function();
	return;
    }

/* check alias */
    a = Cmd_Alias_Find(cmd_argv[0]);
    if (a) {
	Cbuf_InsertText(a->value);
	return;
    }

/* check cvars */
    if (!Cvar_Command() && (cl_warncmd.value || developer.value))
	Con_Printf("Unknown command \"%s\"\n", Cmd_Argv(0));
}

/*
 * Return a string tree with all possible argument completions of the given
 * buffer for the given command.
 */
struct stree_root *
Cmd_ArgCompletions(const char *name, const char *buf)
{
    cmd_function_t *cmd;
    struct stree_root *root = NULL;

    cmd = Cmd_FindCommand(name);
    if (cmd && cmd->completion)
	root = cmd->completion(buf);

    return root;
}

/*
 * Call the argument completion function for cmd "name".
 * Returned result should be Z_Free'd after use.
 */
const char *
Cmd_ArgComplete(const char *name, const char *buf)
{
    char *result = NULL;
    struct stree_root *root;

    root = Cmd_ArgCompletions(name, buf);
    if (root) {
	result = STree_MaxMatch(root, buf);
	Z_Free(root);
    }

    return result;
}


/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/
int
Cmd_CheckParm(const char *parm)
{
    int i;

    if (!parm)
	Sys_Error("Cmd_CheckParm: NULL");

    for (i = 1; i < Cmd_Argc(); i++)
	if (!strcasecmp(parm, Cmd_Argv(i)))
	    return i;

    return 0;
}

struct stree_root *Cmd_CommandCompletions(const char *buf)
{
    struct stree_root *root_tree;

    root_tree = (struct stree_root*)Z_Malloc(sizeof(struct stree_root));

    root_tree->entries = 0;
    root_tree->maxlen = 0;
    root_tree->minlen = -1;
    /* root_tree->root = {NULL}; */
    root_tree->stack = NULL;

    STree_AllocInit();

    STree_Completions(root_tree, &cmd_tree, buf);
    STree_Completions(root_tree, &cmdalias_tree, buf);
    STree_Completions(root_tree, &cvar_tree, buf);

    return root_tree;
}

const char *Cmd_CommandComplete(const char *buf)
{
    struct stree_root *root;
    char *ret;

    root = Cmd_CommandCompletions(buf);
    ret = STree_MaxMatch(root, buf);
    Z_Free(root);

    return ret;
}
