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
/* cvar.c -- dynamic variable tracking */

#include "compat/strl.h"

#include "cmd.h"
#include "common.h"
#include "console.h"
#include "cvar.h"
#include "shell.h"
#include "zone.h"

#include "server.h"
#include "quakedef.h"
#include "host.h"

#include <streams/file_stream.h>

/* forward declarations */
int rfprintf(RFILE * stream, const char * format, ...);

#define cvar_entry(ptr) container_of(ptr, struct cvar_s, stree)
DECLARE_STREE_ROOT(cvar_tree);

/*
============
Cvar_FindVar
============
*/
cvar_t *
Cvar_FindVar(const char *var_name)
{
    struct cvar_s *ret = NULL;
    struct stree_node *n;

    n = STree_Find(&cvar_tree, var_name);
    if (n)
	ret = cvar_entry(n);

    return ret;
}

/*
 * Return a string tree with all possible argument completions of the given
 * buffer for the given cvar.
 */
struct stree_root *
Cvar_ArgCompletions(const char *name, const char *buf)
{
    cvar_t *cvar;
    struct stree_root *root = NULL;

    cvar = Cvar_FindVar(name);
    if (cvar && cvar->completion)
	root = cvar->completion(buf);

    return root;
}

/*
 * Call the argument completion function for cvar "name".
 * Returned result should be Z_Free'd after use.
 */
char *
Cvar_ArgComplete(const char *name, const char *buf)
{
    char *result = NULL;
    struct stree_root *root;

    root = Cvar_ArgCompletions(name, buf);
    if (root) {
	result = STree_MaxMatch(root, buf);
	Z_Free(root);
    }

    return result;
}


/*
 * For NQ/net_dgrm.c, command == CCREQ_RULE_INFO case
 */
cvar_t *
Cvar_NextServerVar(const char *var_name)
{
   cvar_t *ret = NULL;
   cvar_t *var;
   struct stree_node *n;

   if (var_name[0] == '\0')
      var_name = NULL;

   if (var_name)
   {
      STree_ForEach_Init__(&cvar_tree, &n);
      STree_ForEach_After__(&cvar_tree, &n, var_name);
      for (; STree_WalkLeft__(&cvar_tree, &n) ; STree_WalkRight__(&n)) {
         var = cvar_entry(n);
         if (var->server) {
            ret = var;
            STree_ForEach_Cleanup__(&cvar_tree);
            return ret;
         }
      }
   }
   else
   {
      STree_ForEach_After_NullStr(&cvar_tree, n) {
         var = cvar_entry(n);
         if (var->server) {
            ret = var;
            STree_ForEach_Cleanup__(&cvar_tree);
            return ret;
         }
      }
   }

   return ret;
}

/*
============
Cvar_VariableValue
============
*/
float
Cvar_VariableValue(const char *var_name)
{
    cvar_t *var;

    var = Cvar_FindVar(var_name);
    if (!var)
	return 0;
    return Q_atof(var->string);
}


/*
============
Cvar_VariableString
============
*/
const char *
Cvar_VariableString(const char *var_name)
{
    cvar_t *var;

    var = Cvar_FindVar(var_name);

    return var ? var->string : "";
}


/*
============
Cvar_Set
============
*/
void
Cvar_Set(const char *var_name, const char *value)
{
    cvar_t *var;
    char *newstring;
    qboolean changed;

    var = Cvar_FindVar(var_name);
    if (!var) {
	/* there is an error in C code if this happens */
	Con_Printf("Cvar_Set: variable %s not found\n", var_name);
	return;
    }

    if (var->flags & CVAR_OBSOLETE) {
	Con_Printf("%s is obsolete.\n", var_name);
	return;
    }

    changed = strcmp(var->string, value);

    /* Check for developer-only cvar */
    if (changed && (var->flags & CVAR_DEVELOPER) && !developer.value) {
	Con_Printf("%s is settable only in developer mode.\n", var_name);
	return;
    }

    /* Central length cap on cvar values.  Cvar_Set Z_Mallocs
     * a copy of `value` of strlen(value)+1 bytes per call;
     * left unbounded, hostile or buggy inputs (QC's
     * cvar_set, server svc_stufftext, autoexec config
     * lines, key-bind reload) can drive the zone into
     * fragmentation/exhaustion.  Stock Quake cvars are
     * short numeric or path-like strings; the longest
     * legitimate cases (like _cl_name) fit in MAX_QPATH.
     * Truncate rather than reject so a malformed config
     * line doesn't refuse-to-load the rest of the
     * settings.  PF_cvar_set additionally rejects
     * upstream so QC sees an error rather than silent
     * truncation. */
    if (strlen(value) >= MAX_QPATH) {
	Con_Printf("Cvar_Set: \"%s\" value too long (%u >= %d), truncating\n",
	           var_name, (unsigned)strlen(value), MAX_QPATH);
    }

    {
	/* The original implementation freed var->string and *then*
	 * read `value` to copy it.  If the caller passed
	 * var->string itself (or a pointer into it) as `value`,
	 * the read at strlen/memcpy would be a use-after-free.
	 * No caller in the current codebase does this -- the
	 * typical path is Cvar_Command which sources `value` from
	 * Cmd_Argv() -- but the alias is silently legal at the
	 * API level and a future caller could hit it.  Allocate
	 * and copy before freeing the old string, so the source
	 * bytes stay live across the operation.  Slightly higher
	 * peak footprint during the realloc, no UB. */
	size_t valuelen = strlen(value) + 1;
	if (valuelen > MAX_QPATH)
	    valuelen = MAX_QPATH;
	newstring = (char*)Z_Malloc(valuelen);
	memcpy(newstring, value, valuelen - 1);
	newstring[valuelen - 1] = 0;
    }
    Z_Free((void *)var->string);	/* free the old value string */
    var->string = newstring;
    var->value = Q_atof(var->string);

    if (var->server && changed) {
	if (sv.active)
	    SV_BroadcastPrintf("\"%s\" changed to \"%s\"\n", var->name,
			       var->string);
    }

    if (changed && var->callback)
	var->callback(var);

    /* Don't allow deathmatch and coop at the same time... */
    if ((var->value != 0) && (!strcmp(var->name, deathmatch.name)))
	Cvar_Set("coop", "0");
    if ((var->value != 0) && (!strcmp(var->name, coop.name)))
	Cvar_Set("deathmatch", "0");
}

/*
============
Cvar_SetValue
============
*/
void
Cvar_SetValue(const char *var_name, float value)
{
    char val[32];

    snprintf(val, sizeof(val), "%f", value);
    Cvar_Set(var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable(cvar_t *variable)
{
   char value[512];		/* FIXME - magic numbers... */
   float old_developer;

   /* first check to see if it has allready been defined */
   if (Cvar_FindVar(variable->name))
   {
      Con_Printf("Can't register variable %s, allready defined\n",
            variable->name);
      return;
   }
   /* check for overlap with a command */
   if (Cmd_Exists(variable->name)) {
      Con_Printf("Cvar_RegisterVariable: %s is a command\n",
            variable->name);
      return;
   }

   variable->stree.string = variable->name;
   STree_Insert(&cvar_tree, &variable->stree);

   /* Capture the compile-time default string the first time we see this
    * cvar.  Cvar_Shutdown will reset variable->string back to this, so
    * a subsequent Host_Init / re-registration sees a valid pointer
    * here instead of a dangle into a freed zone. */
   if (!variable->default_string)
       variable->default_string = variable->string;

   /* copy the value off, because future sets will Z_Free it */
   strlcpy(value, variable->string ? variable->string : variable->default_string, sizeof(value));
   variable->string = (const char*)Z_Malloc(1);

   if (!(variable->flags & CVAR_CALLBACK))
     variable->callback = NULL;

   /*
    * FIXME (BARF) - readonly cvars need to be initialised
    *                developer 1 allows set
    */
   /* set it through the function to be consistant */
   old_developer = developer.value;
   developer.value = 1;
   Cvar_Set(variable->name, value);
   developer.value = old_developer;
}

/*
============
Cvar_SetCallback

Set a callback function to the var
============
*/
void Cvar_SetCallback (cvar_t *var, cvar_callback func)
{
	var->callback = func;
	if (func)
		var->flags |= CVAR_CALLBACK;
	else	var->flags &= ~CVAR_CALLBACK;
}

/*
============
Cvar_Shutdown

Detach all cvars from the cvar_tree and reset their string fields back
to the compile-time default captured at first registration.  Call this
before tearing down the zone heap (e.g. from Host_Shutdown), otherwise
each cvar->string left over from the previous session points into the
freed zone -- and any subsequent Cvar_Set in the next session would
Z_Free that dangling pointer and corrupt the new mainzone.

The cvar_t structs themselves are file-scope statics in r_main.c,
host.c, etc., so we never free them; we just clear the tree links and
restore variable->string to a pointer that will still be valid after
the zone is wiped.
============
*/
void Cvar_Shutdown(void)
{
	struct stree_node *n;
	cvar_t *var;

	/* First pass: collect cvars and reset their string pointers.  We
	 * can't free the strings -- they're in the zone that's about to be
	 * (or has just been) wiped wholesale -- but we must overwrite the
	 * dangling pointer so the next Cvar_Set doesn't Z_Free it. */
	STree_ForEach_After_NullStr(&cvar_tree, n) {
		var = cvar_entry(n);
		var->string = var->default_string;
	}

	/* Reset the tree to the same empty layout that DECLARE_STREE_ROOT
	 * produces at file scope.  All node links inside cvar_t structs
	 * are stale; the next round of Cvar_RegisterVariable calls will
	 * re-insert them fresh. */
	memset(&cvar_tree, 0, sizeof(cvar_tree));
	cvar_tree.minlen = (unsigned int)-1;
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command(void)
{
   /* check variables */
   cvar_t *v = Cvar_FindVar(Cmd_Argv(0));
   if (!v)
      return false;

   /* perform a variable print or set */
   if (Cmd_Argc() == 1)
   {
      if (v->flags & CVAR_OBSOLETE)
         Con_Printf("%s is obsolete.\n", v->name);
      else
         Con_Printf("\"%s\" is \"%s\"\n", v->name, v->string);
      return true;
   }

   Cvar_Set(v->name, Cmd_Argv(1));
   return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables(RFILE *f)
{
   struct stree_node *n;

   STree_ForEach_After_NullStr(&cvar_tree, n)
   {
      cvar_t *var = cvar_entry(n);
      if (var->archive)
      {
         /* Cvar values can contain arbitrary bytes including
          * '"', '\n', and ';', which break out of the
          * "%s \"%s\"\n" framing on re-parse and inject
          * commands into the next exec config.cfg.  Hostile
          * sources for such bytes include QW server-pushed
          * setinfo commands that the client mirrors into a
          * local cvar, and in NQ any rcon / stuffcmd path that
          * sets a cvar.  Skip the offending chars on write
          * rather than corrupting the config file format. */
         const char *s;
         rfprintf(f, "%s \"", var->name);
         for (s = var->string; *s; s++) {
            unsigned char c = (unsigned char)*s;
            if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c < 0x20)
               continue;
            rfprintf(f, "%c", c);
         }
         rfprintf(f, "\"\n");
      }
   }
}
