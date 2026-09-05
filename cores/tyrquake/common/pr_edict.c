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
/* sv_edict.c -- entity dictionary */

#include "compat/strl.h"

#include "cmd.h"
#include "console.h"
#include "crc.h"
#include "pr_comp.h"
#include "progdefs.h"
#include "progs.h"
#include "server.h"
#include "world.h"
#include "zone.h"

#include "host.h"
#include "quakedef.h"
#include "sys.h"

/* FIXME - quick hack to enable merging of NQ/QWSV shared code */
#define SV_Error Sys_Error

#include <streams/file_stream.h>

/* forward declarations */
int rfprintf(RFILE * stream, const char * format, ...);

dprograms_t *progs;
dfunction_t *pr_functions;
char *pr_strings;
int pr_strings_size;
dstatement_t *pr_statements;
globalvars_t *pr_global_struct;
float *pr_globals;		/* same as pr_global_struct */
int pr_edict_size;		/* in bytes */

static ddef_t *pr_fielddefs;
static ddef_t *pr_globaldefs;

/*
 * These are the sizes of the types enumerated in etype_t (pr_comp.h)
 */
static int type_size[8] = {
    1,				/* ev_void */
    1,				/* ev_string */
    1,				/* ev_float */
    3,				/* ev_vector */
    1,				/* ev_entity */
    1,				/* ev_field */
    1,				/* ev_function */
    1				/* ev_pointer */
};

static qboolean ED_ParseEpair(void *base, ddef_t *key, const char *s);

#define	MAX_FIELD_LEN	64
#define GEFV_CACHESIZE	2

typedef struct {
    ddef_t *pcache;
    char field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache gefvCache[GEFV_CACHESIZE] = { {NULL, ""}, {NULL, ""} };

unsigned short pr_crc;
cvar_t nomonsters = { "nomonsters", "0" };
static cvar_t gamecfg = { "gamecfg", "0" };
static cvar_t scratch1 = { "scratch1", "0" };
static cvar_t scratch2 = { "scratch2", "0" };
static cvar_t scratch3 = { "scratch3", "0" };
static cvar_t scratch4 = { "scratch4", "0" };
static cvar_t savedgamecfg = { "savedgamecfg", "0", true };
static cvar_t saved1 = { "saved1", "0", true };
static cvar_t saved2 = { "saved2", "0", true };
static cvar_t saved3 = { "saved3", "0", true };
static cvar_t saved4 = { "saved4", "0", true };

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
static void
ED_ClearEdict(edict_t *e)
{
    memset(&e->v, 0, progs->entityfields * 4);
    e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *
ED_Alloc(void)
{
    int i;
    edict_t *e;

    for (i = svs.maxclients + 1; i < sv.num_edicts; i++) {
	e = EDICT_NUM(i);
	/* the first couple seconds of server time can involve a lot of */
	/* freeing and allocating, so relax the replacement policy */
	if (e->free && (e->freetime < 2 || sv.time - e->freetime > 0.5)) {
	    ED_ClearEdict(e);
	    return e;
	}
    }

    if (i == MAX_EDICTS)
	SV_Error("%s: no free edicts", __func__);
    sv.num_edicts++;

    e = EDICT_NUM(i);
    ED_ClearEdict(e);

    return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void
ED_Free(edict_t *ed)
{
    SV_UnlinkEdict(ed);		/* unlink from world bsp */

    ed->free = true;
    ed->v.model = 0;
    ed->v.takedamage = 0;
    ed->v.modelindex = 0;
    ed->v.colormap = 0;
    ed->v.skin = 0;
    ed->v.frame = 0;
    VectorCopy(vec3_origin, ed->v.origin);
    VectorCopy(vec3_origin, ed->v.angles);
    ed->v.nextthink = -1;
    ed->v.solid = 0;

    ed->freetime = sv.time;
}

/* =========================================================================== */

/*
============
ED_GlobalAtOfs
============
*/
static ddef_t *
ED_GlobalAtOfs(int ofs)
{
    ddef_t *def;
    int i;

    for (i = 0; i < progs->numglobaldefs; i++) {
	def = &pr_globaldefs[i];
	if (def->ofs == ofs)
	    return def;
    }
    return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
static ddef_t *
ED_FieldAtOfs(int ofs)
{
    ddef_t *def;
    int i;

    for (i = 0; i < progs->numfielddefs; i++) {
	def = &pr_fielddefs[i];
	if (def->ofs == ofs)
	    return def;
    }
    return NULL;
}

/*
============
ED_FindField
============
*/
static ddef_t *
ED_FindField(const char *name)
{
    ddef_t *def;
    int i;

    for (i = 0; i < progs->numfielddefs; i++) {
	def = &pr_fielddefs[i];
	if (!strcmp(PR_GetString(def->s_name), name))
	    return def;
    }
    return NULL;
}


/*
============
ED_FindGlobal
============
*/
static ddef_t *
ED_FindGlobal(const char *name)
{
    ddef_t *def;
    int i;

    for (i = 0; i < progs->numglobaldefs; i++) {
	def = &pr_globaldefs[i];
	if (!strcmp(PR_GetString(def->s_name), name))
	    return def;
    }
    return NULL;
}


/*
============
ED_FindFunction
============
*/
static dfunction_t *
ED_FindFunction(const char *name)
{
    dfunction_t *func;
    int i;

    for (i = 0; i < progs->numfunctions; i++) {
	func = &pr_functions[i];
	if (!strcmp(PR_GetString(func->s_name), name))
	    return func;
    }
    return NULL;
}

eval_t *
GetEdictFieldValue(edict_t *ed, const char *field)
{
    static int rep = 0;
    ddef_t *def = NULL;
    int i;

    for (i = 0; i < GEFV_CACHESIZE; i++) {
	if (!strcmp(field, gefvCache[i].field)) {
	    def = gefvCache[i].pcache;
	    goto Done;
	}
    }

    def = ED_FindField(field);

    if (strlen(field) < MAX_FIELD_LEN) {
	gefvCache[rep].pcache = def;
	strlcpy(gefvCache[rep].field, field, sizeof(gefvCache[rep].field));
	rep ^= 1;
    }

  Done:
    if (!def)
	return NULL;

    return (eval_t *)((char *)&ed->v + def->ofs * 4);
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
static char *
PR_ValueString(etype_t type, eval_t *val)
{
    static char line[128];
    ddef_t *def;
    dfunction_t *f;

    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
    case ev_string:
	snprintf(line, sizeof(line), "%s", PR_GetString(val->string));
	break;
    case ev_entity:
	snprintf(line, sizeof(line), "entity %i",
		 NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
	break;
    case ev_function:
	/* val->function is a QC-stored function index; bytecode
	 * can plant arbitrary values.  Bound against
	 * progs->numfunctions before computing the pointer; an
	 * out-of-range index would read OOB from pr_functions. */
	if (val->function < 0 || val->function >= progs->numfunctions) {
	    snprintf(line, sizeof(line), "bad function %i()", val->function);
	    break;
	}
	f = pr_functions + val->function;
	snprintf(line, sizeof(line), "%s()", PR_GetString(f->s_name));
	break;
    case ev_field:
	/* ED_FieldAtOfs returns NULL when no fielddef has the
	 * given offset.  A malicious progs.dat (or in-progress
	 * data corruption) can plant an ofs that misses every
	 * fielddef; the pre-existing code then dereferenced
	 * NULL via def->s_name. */
	def = ED_FieldAtOfs(val->_int);
	if (!def) {
	    snprintf(line, sizeof(line), ".bad field ofs %i", val->_int);
	    break;
	}
	snprintf(line, sizeof(line), ".%s", PR_GetString(def->s_name));
	break;
    case ev_void:
	snprintf(line, sizeof(line), "void");
	break;
    case ev_float:
	snprintf(line, sizeof(line), "%5.1f", val->_float);
	break;
    case ev_vector:
	snprintf(line, sizeof(line), "'%5.1f %5.1f %5.1f'",
		 val->vector[0], val->vector[1], val->vector[2]);
	break;
    case ev_pointer:
	snprintf(line, sizeof(line), "pointer");
	break;
    default:
	snprintf(line, sizeof(line), "bad type %i", type);
	break;
    }

    return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
static char *
PR_UglyValueString(etype_t type, eval_t *val)
{
    static char line[128];
    ddef_t *def;
    dfunction_t *f;

    type &= ~DEF_SAVEGLOBAL;

    switch (type) {
    case ev_string:
	snprintf(line, sizeof(line), "%s", PR_GetString(val->string));
	break;
    case ev_entity:
	snprintf(line, sizeof(line), "%i",
		 NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
	break;
    case ev_function:
	/* val->function is a QC-stored function index; bytecode
	 * can plant arbitrary values.  Bound against
	 * progs->numfunctions before computing the pointer; an
	 * out-of-range index would read OOB from pr_functions. */
	if (val->function < 0 || val->function >= progs->numfunctions) {
	    snprintf(line, sizeof(line), "bad function %i", val->function);
	    break;
	}
	f = pr_functions + val->function;
	snprintf(line, sizeof(line), "%s", PR_GetString(f->s_name));
	break;
    case ev_field:
	/* ED_FieldAtOfs returns NULL when no fielddef has the
	 * given offset.  A malicious progs.dat (or in-progress
	 * data corruption) can plant an ofs that misses every
	 * fielddef; the pre-existing code then dereferenced
	 * NULL via def->s_name. */
	def = ED_FieldAtOfs(val->_int);
	if (!def) {
	    snprintf(line, sizeof(line), "bad field ofs %i", val->_int);
	    break;
	}
	snprintf(line, sizeof(line), "%s", PR_GetString(def->s_name));
	break;
    case ev_void:
	snprintf(line, sizeof(line), "void");
	break;
    case ev_float:
	snprintf(line, sizeof(line), "%f", val->_float);
	break;
    case ev_vector:
	snprintf(line, sizeof(line), "%f %f %f",
		 val->vector[0], val->vector[1], val->vector[2]);
	break;
    default:
	snprintf(line, sizeof(line), "bad type %i", type);
	break;
    }

    return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char *PR_GlobalString(int ofs)
{
   static char line[128];
   char *s;
   int i;
   void *val;
   ddef_t *def;

   /* ofs is a global offset typically taken from a statement
    * operand (s->a, s->b, s->c).  The bytecode dispatch in
    * 1a8b98d validates these per-op for normal execution,
    * but pr_exec's trace path calls into here directly.  A
    * hostile progs.dat with an out-of-range operand makes
    * pr_globals[ofs] an OOB read.  Guard before deref. */
   if (ofs < 0 || ofs >= progs->numglobals)
   {
      snprintf(line, sizeof(line), "%i(BAD OFS)", ofs);
      goto pad;
   }
   val = (void *)&pr_globals[ofs];
   def = ED_GlobalAtOfs(ofs);

   if (!def)
      snprintf(line, sizeof(line), "%i(???"")", ofs);
   else
   {
      s = (char*)PR_ValueString((etype_t)def->type, (eval_t*)val);
      snprintf(line, sizeof(line), "%i(%s)%s", ofs,
            PR_GetString(def->s_name), s);
   }

pad:
   for (i = strlen(line); i < 20; i++)
      strlcat(line, " ", sizeof(line));
   strlcat(line, " ", sizeof(line));

   return line;
}

char *PR_GlobalStringNoContents(int ofs)
{
   static char line[128];
   int i;
   ddef_t *def;

   /* Same hostile-operand reasoning as PR_GlobalString;
    * ED_GlobalAtOfs already returns NULL for unknown offsets
    * so this path is safe for unbounded ofs, but be explicit
    * about the bound for parity. */
   if (ofs < 0 || ofs >= progs->numglobals)
   {
      snprintf(line, sizeof(line), "%i(BAD OFS)", ofs);
      goto pad;
   }
   def = ED_GlobalAtOfs(ofs);
   if (!def)
      snprintf(line, sizeof(line), "%i(???"")", ofs);
   else
      snprintf(line, sizeof(line), "%i(%s)", ofs, PR_GetString(def->s_name));

pad:
   i = strlen(line);
   for (; i < 20; i++)
      strlcat(line, " ", sizeof(line));
   strlcat(line, " ", sizeof(line));

   return line;
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print(edict_t *ed)
{
   int i;

   if (ed->free)
   {
      Con_Printf("FREE\n");
      return;
   }

   Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
   for (i = 1; i < progs->numfielddefs; i++)
   {
      int *v;
      int type, j, l;
      ddef_t        *d = &pr_fielddefs[i];
      const char *name = PR_GetString(d->s_name);

      if (name[strlen(name) - 2] == '_')
         continue;		/* skip _x, _y, _z vars */

      v = (int *)((char *)&ed->v + d->ofs * 4);

      /* if the value is still all 0, skip the field */
      type = d->type & ~DEF_SAVEGLOBAL;

      for (j = 0; j < type_size[type]; j++)
         if (v[j])
            break;
      if (j == type_size[type])
         continue;

      Con_Printf("%s", name);
      l = strlen(name);
      while (l++ < 15)
         Con_Printf(" ");

      Con_Printf("%s\n", PR_ValueString((etype_t)d->type, (eval_t *)v));
   }
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write(RFILE *f, edict_t *ed)
{
   ddef_t *d;
   int *v;
   int i, j;
   const char *name;
   int type;

   rfprintf(f, "{\n");

   if (ed->free)
   {
      rfprintf(f, "}\n");
      return;
   }

   for (i = 1; i < progs->numfielddefs; i++) {
      d = &pr_fielddefs[i];
      name = PR_GetString(d->s_name);
      if (name[strlen(name) - 2] == '_')
         continue;		/* skip _x, _y, _z vars */

      v = (int *)((char *)&ed->v + d->ofs * 4);

      /* if the value is still all 0, skip the field */
      type = d->type & ~DEF_SAVEGLOBAL;
      for (j = 0; j < type_size[type]; j++)
         if (v[j])
            break;
      if (j == type_size[type])
         continue;

      rfprintf(f, "\"%s\" ", name);
      rfprintf(f, "\"%s\"\n", PR_UglyValueString((etype_t)d->type, (eval_t *)v));
   }

   rfprintf(f, "}\n");
}

void ED_PrintNum(int ent)
{
   ED_Print(EDICT_NUM(ent));
}

/*
==============================================================================

ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void
ED_WriteGlobals(RFILE *f)
{
    ddef_t *def;
    int i;
    const char *name;
    int type;

    rfprintf(f, "{\n");
    for (i = 0; i < progs->numglobaldefs; i++) {
	def = &pr_globaldefs[i];
	type = def->type;
	if (!(def->type & DEF_SAVEGLOBAL))
	    continue;
	type &= ~DEF_SAVEGLOBAL;

	if (type != ev_string && type != ev_float && type != ev_entity)
	    continue;

	name = PR_GetString(def->s_name);
	rfprintf(f, "\"%s\" ", name);
	rfprintf(f, "\"%s\"\n",
		PR_UglyValueString((etype_t)type, (eval_t *)&pr_globals[def->ofs]));
    }
    rfprintf(f, "}\n");
}

/*
=============
ED_ParseGlobals
=============
*/
void
ED_ParseGlobals(const char *data)
{
    char keyname[64];
    ddef_t *key;

    while (1) {
	/* parse key */
	data = COM_Parse(data);
	if (com_token[0] == '}')
	    break;
	if (!data)
	    SV_Error("%s: EOF without closing brace", __func__);

	strlcpy(keyname, com_token, sizeof(keyname));

	/* parse value */
	data = COM_Parse(data);
	if (!data)
	    SV_Error("%s: EOF without closing brace", __func__);

	if (com_token[0] == '}')
	    SV_Error("%s: closing brace without data", __func__);

	key = ED_FindGlobal(keyname);
	if (!key) {
	    Con_Printf("'%s' is not a global\n", keyname);
	    continue;
	}

	if (!ED_ParseEpair((void *)pr_globals, key, com_token))
	    Host_Error("%s: parse error", __func__);
    }
}

/* ============================================================================ */


/*
=============
ED_NewString
=============
*/
static char *
ED_NewString(const char *string)
{
    int i;
    size_t l     = strlen(string) + 1;
    char *newobj = (char*)Hunk_Alloc(l);
    char *new_p  = newobj;

    for (i = 0; i < l; i++)
    {
	if (string[i] == '\\' && i < l - 1) {
	    i++;
	    if (string[i] == 'n')
		*new_p++ = '\n';
	    else
		*new_p++ = '\\';
	} else
	    *new_p++ = string[i];
    }

    return newobj;
}


/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
static qboolean
ED_ParseEpair(void *base, ddef_t *key, const char *s)
{
    int i;
    char string[128];
    ddef_t *def;
    char *v, *w;
    dfunction_t *func;
    void *d;
    int max_ofs;

    /* key->ofs is read from progs.dat (pr_globaldefs / pr_fielddefs).
     * An out-of-range ofs lets ED_ParseEpair write past the end of
     * the entity (when called from ED_ParseEdict with base=&ent->v)
     * or past the end of pr_globals (from ED_ParseGlobals).  The
     * caller can't easily compute the right limit in either case
     * because ddef_t doesn't carry it, so do the check here using
     * key->ofs vs the appropriate live size. */
    if (base == (void *)pr_globals)
	max_ofs = progs->numglobals;
    else
	max_ofs = progs->entityfields;

    /* Reject a pure overflow first.  ev_vector writes 3 floats so it
     * needs three slots starting at ofs; everything else writes one. */
    if (key->ofs < 0 || key->ofs >= max_ofs)
	return false;
    if ((key->type & ~DEF_SAVEGLOBAL) == ev_vector
	&& key->ofs > max_ofs - 3)
	return false;

    d = (void *)((int *)base + key->ofs);

    switch (key->type & ~DEF_SAVEGLOBAL) {
    case ev_string:
	*(string_t *)d = PR_SetString(ED_NewString(s));
	break;

    case ev_float:
	*(float *)d = atof(s);
	break;

    case ev_vector:
	strlcpy(string, s, sizeof(string));
	v = string;
	w = string;
	for (i = 0; i < 3; i++) {
	    while (*v && *v != ' ')
		v++;
	    *v = 0;
	    ((float *)d)[i] = atof(w);
	    w = v = v + 1;
	}
	break;

    case ev_entity:
	*(int *)d = EDICT_TO_PROG(EDICT_NUM(atoi(s)));
	break;

    case ev_field:
	def = ED_FindField(s);
	if (!def) {
	    Con_Printf("Can't find field %s\n", s);
	    return false;
	}
	*(int *)d = G_INT(def->ofs);
	break;

    case ev_function:
	func = ED_FindFunction(s);
	if (!func) {
	    Con_Printf("Can't find function %s\n", s);
	    return false;
	}
	*(func_t *)d = func - pr_functions;
	break;

    default:
	break;
    }
    return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
const char *
ED_ParseEdict(const char *data, edict_t *ent)
{
    int n;
    ddef_t *key;
    qboolean anglehack;
    char keyname[256];
    qboolean init = false;

    /* clear it */
    if (ent != sv.edicts)	/* hack */
	memset(&ent->v, 0, progs->entityfields * 4);

    /* go through all the dictionary pairs */
    for (;;)
    {
	/* parse key */
	data = COM_Parse(data);
	if (com_token[0] == '}')
	    break;
	if (!data)
	    SV_Error("%s: EOF without closing brace", __func__);

/* anglehack is to allow QuakeEd to write single scalar angles */
/* and allow them to be turned into vectors. (FIXME...) */
	if (!strcmp(com_token, "angle"))
	{
	    strlcpy(com_token, "angles", sizeof(com_token));
	    anglehack = true;
	}
	else
	    anglehack = false;

	/* FIXME: change light to _light to get rid of this hack */
	if (!strcmp(com_token, "light"))
	    strlcpy(com_token, "light_lev", sizeof(com_token));	/* hack for single light def */

	strlcpy(keyname, com_token, sizeof(keyname));

	/* another hack to fix keynames with trailing spaces */
	n = strlen(keyname);
	while (n && keyname[n - 1] == ' ')
	{
	    keyname[n - 1] = 0;
	    n--;
	}

	/* parse value */
	data = COM_Parse(data);
	if (!data)
	    SV_Error("%s: EOF without closing brace", __func__);

	if (com_token[0] == '}')
	    SV_Error("%s: closing brace without data", __func__);

	init = true;

/* keynames with a leading underscore are used for utility comments, */
/* and are immediately discarded by quake */
	if (keyname[0] == '_')
	    continue;

	key = ED_FindField(keyname);
	if (!key) {
	    Con_Printf("'%s' is not a field\n", keyname);
	    continue;
	}

	if (anglehack) {
	    char temp[32];

	    strlcpy(temp, com_token, sizeof(temp));
	    snprintf(com_token, sizeof(com_token), "0 %s 0", temp);
	}

	if (!ED_ParseEpair((void *)&ent->v, key, com_token))
	    Host_Error("%s: parse error", __func__);
    }

    if (!init)
	ent->free = true;

    return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void
ED_LoadFromFile(const char *data)
{
    edict_t *ent;
    int inhibit;
    dfunction_t *func;

    ent = NULL;
    inhibit = 0;
    pr_global_struct->time = sv.time;

    /* parse ents */
   for (;;)
   {
        /* parse the opening brace */
	data = COM_Parse(data);
	if (!data)
	    break;
	if (com_token[0] != '{')
	    SV_Error("%s: found %s when expecting {", __func__, com_token);

	if (!ent)
	    ent = EDICT_NUM(0);
	else
	    ent = ED_Alloc();
	data = ED_ParseEdict(data, ent);

	/* remove things from different skill levels or deathmatch */
	if (deathmatch.value) {
	    if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH)) {
		ED_Free(ent);
		inhibit++;
		continue;
	    }
	} else
	    if ((current_skill == 0
		 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
		|| (current_skill == 1
		    && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
		|| (current_skill >= 2
		    && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD))) {
	    ED_Free(ent);
	    inhibit++;
	    continue;
	}

/**/
/* immediately call spawn function */
/**/
	if (!ent->v.classname) {
	    Con_Printf("No classname for:\n");
	    ED_Print(ent);
	    ED_Free(ent);
	    continue;
	}
	/* look for the spawn function */
	func = ED_FindFunction(PR_GetString(ent->v.classname));

	if (!func) {
	    Con_Printf("No spawn function for:\n");
	    ED_Print(ent);
	    ED_Free(ent);
	    continue;
	}

	pr_global_struct->self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(func - pr_functions);
    }

    Con_DPrintf("%i entities inhibited\n", inhibit);
}


/*
===============
PR_LoadProgs
===============
*/
void
PR_LoadProgs(void)
{
   int i;

   /* flush the non-C variable lookup cache */
   for (i = 0; i < GEFV_CACHESIZE; i++)
      gefvCache[i].field[0] = 0;

   progs = (dprograms_t*)COM_LoadHunkFile("progs.dat");
   if (!progs)
      SV_Error("%s: couldn't load progs.dat", __func__);
   Con_DPrintf("Programs occupy %iK.\n", com_filesize / 1024);

   pr_crc = CRC_Block((byte *)progs, com_filesize);

   /* byte swap the header */
   for (i = 0; i < sizeof(*progs) / 4; i++)
      ((int *)progs)[i] = LittleLong(((int *)progs)[i]);

   if (progs->version != PROG_VERSION)
      SV_Error("progs.dat has wrong version number (%i should be %i)",
            progs->version, PROG_VERSION);
   if (progs->crc != PROGHEADER_CRC)
      SV_Error("progs.dat system vars have been modified, "
            "progdefs.h is out of date");

   pr_functions = (dfunction_t *)((byte *)progs + progs->ofs_functions);
   pr_strings = (char *)progs + progs->ofs_strings;
   pr_strings_size = progs->strings_size;
   if (progs->ofs_strings + pr_strings_size >= com_filesize)
      Host_Error("progs.dat strings extend past end of file\n");

   /* Bounds-check every (ofs, num) pair in the progs.dat header
    * against com_filesize.  Without this, a crafted progs.dat with
    * the matching PROGHEADER_CRC but malicious offsets would cause
    * the byte-swap and lookup loops below to read and write off
    * the end of the progs.dat hunk allocation.  Each lump's size
    * is implied by its element count times the on-disk struct
    * size; check ofs >= 0, count >= 0, and ofs + count*sizeof(elt)
    * fits within com_filesize.  Use a size_t cast and a subtraction
    * formulation to avoid signed overflow during the comparison. */
   {
      const size_t fsize = (size_t)com_filesize;
      const struct {
	 const char *name;
	 int32_t ofs;
	 int32_t count;
	 size_t elt_size;
      } lumps[] = {
	 { "statements",  progs->ofs_statements,  progs->numstatements,  sizeof(dstatement_t) },
	 { "globaldefs",  progs->ofs_globaldefs,  progs->numglobaldefs,  sizeof(ddef_t)       },
	 { "fielddefs",   progs->ofs_fielddefs,   progs->numfielddefs,   sizeof(ddef_t)       },
	 { "functions",   progs->ofs_functions,   progs->numfunctions,   sizeof(dfunction_t)  },
	 { "globals",     progs->ofs_globals,     progs->numglobals,     sizeof(int32_t)      },
      };
      size_t li;
      for (li = 0; li < sizeof(lumps)/sizeof(lumps[0]); li++) {
	 if (lumps[li].ofs < 0 || lumps[li].count < 0)
	    SV_Error("progs.dat: bad %s lump (ofs=%d, count=%d)",
		     lumps[li].name, lumps[li].ofs, lumps[li].count);
	 if ((size_t)lumps[li].count > (fsize - (size_t)lumps[li].ofs) / lumps[li].elt_size
	     || (size_t)lumps[li].ofs > fsize)
	    SV_Error("progs.dat: %s lump extends past end of file "
		     "(ofs=%d, count=%d, filesize=%i)",
		     lumps[li].name, lumps[li].ofs, lumps[li].count,
		     com_filesize);
      }
      if (progs->entityfields < 0)
	 SV_Error("progs.dat: bad entityfields (%d)", progs->entityfields);
      /* entityfields multiplied by 4 produces pr_edict_size,
       * which is then multiplied by MAX_EDICTS (8192) to size
       * sv.edicts.  Cap entityfields well below INT_MAX/(4 *
       * MAX_EDICTS) so the multiplication chain can't overflow.
       * Stock progs uses ~95; even heavily modded progs rarely
       * exceeds a few hundred. */
      if (progs->entityfields > 4096)
	 SV_Error("progs.dat: too many entityfields (%d, max 4096)",
		  progs->entityfields);
   }

   PR_InitStringTable();

   pr_globaldefs = (ddef_t *)((byte *)progs + progs->ofs_globaldefs);
   pr_fielddefs = (ddef_t *)((byte *)progs + progs->ofs_fielddefs);
   pr_statements = (dstatement_t *)((byte *)progs + progs->ofs_statements);

   pr_global_struct = (globalvars_t *)((byte *)progs + progs->ofs_globals);
   pr_globals = (float *)pr_global_struct;

   pr_edict_size =
      progs->entityfields * 4 + sizeof(edict_t) - sizeof(entvars_t);

   /* byte swap the lumps */
   for (i = 0; i < progs->numstatements; i++)
   {
      pr_statements[i].op = LittleShort(pr_statements[i].op);
      pr_statements[i].a = LittleShort(pr_statements[i].a);
      pr_statements[i].b = LittleShort(pr_statements[i].b);
      pr_statements[i].c = LittleShort(pr_statements[i].c);
   }

   for (i = 0; i < progs->numfunctions; i++) {
      pr_functions[i].first_statement =
         LittleLong(pr_functions[i].first_statement);
      pr_functions[i].parm_start = LittleLong(pr_functions[i].parm_start);
      pr_functions[i].s_name = LittleLong(pr_functions[i].s_name);
      pr_functions[i].s_file = LittleLong(pr_functions[i].s_file);
      pr_functions[i].numparms = LittleLong(pr_functions[i].numparms);
      pr_functions[i].locals = LittleLong(pr_functions[i].locals);
   }

   for (i = 0; i < progs->numglobaldefs; i++) {
      pr_globaldefs[i].type = LittleShort(pr_globaldefs[i].type);
      pr_globaldefs[i].ofs = LittleShort(pr_globaldefs[i].ofs);
      pr_globaldefs[i].s_name = LittleLong(pr_globaldefs[i].s_name);
   }

   for (i = 0; i < progs->numfielddefs; i++) {
      pr_fielddefs[i].type = LittleShort(pr_fielddefs[i].type);
      if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
         SV_Error("%s: pr_fielddefs[i].type & DEF_SAVEGLOBAL", __func__);
      pr_fielddefs[i].ofs    = LittleShort(pr_fielddefs[i].ofs);
      pr_fielddefs[i].s_name = LittleLong(pr_fielddefs[i].s_name);
   }

   for (i = 0; i < progs->numglobals; i++)
      ((int *)pr_globals)[i] = LittleLong(((int *)pr_globals)[i]);

   /* Bytecode validation pass.  By this point the lumps are
    * byte-swapped (on MSB_FIRST) and we know they fit within
    * com_filesize, but the indices stored inside them have
    * not been checked.  Without this, the inner interpreter
    * loop in PR_ExecuteProgram dereferences pr_globals[st->a]
    * etc. for indices 0..65535 with no bound, lets a corrupt
    * function jump to first_statement past numstatements,
    * lets PR_EnterFunction copy parm_size[i]*numparms entries
    * past numglobals, and lets s_name fields point past
    * pr_strings.  Catch all of those at load time so the
    * inner loop can stay hot and bounds-check-free for
    * st->a/b/c.  Runtime-only checks are added separately
    * for the values that are still computed at runtime
    * (OP_STOREP target, OP_CALL function index, dynamic
    * GOTO/IF jump targets). */
   {
      const int ng = progs->numglobals;
      const int ns = progs->numstatements;
      const int nf = progs->numfunctions;
      const int nfd = progs->numfielddefs;
      const int ngd = progs->numglobaldefs;
      const int ssz = pr_strings_size;

      /* Statements: a/b/c are int16 indices into pr_globals
       * for most opcodes.  OP_GOTO uses st->a and OP_IF/IFNOT
       * use st->b as a signed statement-relative jump offset
       * (typically negative for backward jumps in loops),
       * so they must NOT be treated as globals indices.  The
       * inner interpreter loop still computes pointers from
       * those operands at the top of each iteration before
       * the switch, but the resulting pointer is never
       * dereferenced for those ops, so the (technically UB)
       * pointer arithmetic causes no harm in practice.  For
       * jump ops we only verify the resolved statement target.
       * Vector ops touch [n], [n+1], [n+2] so the upper bound
       * for a globals index is numglobals - 3. */
      if (ng < 3)
         SV_Error("progs.dat: numglobals (%d) too small", ng);
      for (i = 0; i < ns; i++) {
         dstatement_t *st = &pr_statements[i];
         if (st->op >= OP_NUMOPS)
            SV_Error("progs.dat: bad opcode %u in statement %i",
                     (unsigned)st->op, i);
         if (st->op == OP_GOTO) {
            int target = i + (int)(int16_t)st->a;
            if (target < 0 || target >= ns)
               SV_Error("progs.dat: OP_GOTO target out of range "
                        "(statement %i + %d -> %d; numstatements=%d)",
                        i, (int)(int16_t)st->a, target, ns);
         } else if (st->op == OP_IF || st->op == OP_IFNOT) {
            int target = i + (int)(int16_t)st->b;
            if (target < 0 || target >= ns)
               SV_Error("progs.dat: OP_IF/IFNOT target out of range "
                        "(statement %i + %d -> %d; numstatements=%d)",
                        i, (int)(int16_t)st->b, target, ns);
            /* IF/IFNOT evaluates a as a scalar globals index. */
            if ((unsigned)st->a > (unsigned)(ng - 1))
               SV_Error("progs.dat: statement %i (OP_IF/IFNOT) has "
                        "bad globals index a=%u (numglobals=%d)",
                        i, (unsigned)st->a, ng);
         } else {
            /* Non-jump op: a/b/c must be valid globals indices. */
            if ((unsigned)st->a > (unsigned)(ng - 3) ||
                (unsigned)st->b > (unsigned)(ng - 3) ||
                (unsigned)st->c > (unsigned)(ng - 3))
               SV_Error("progs.dat: statement %i (op %u) has "
                        "globals index out of range "
                        "(a=%u b=%u c=%u; numglobals=%d)",
                        i, (unsigned)st->op,
                        (unsigned)st->a, (unsigned)st->b,
                        (unsigned)st->c, ng);
         }
      }

      /* Functions: first_statement is signed (negative = builtin),
       * parm_start, locals, numparms must fit in pr_globals. */
      for (i = 0; i < nf; i++) {
         dfunction_t *fp = &pr_functions[i];
         if (fp->first_statement >= ns)
            SV_Error("progs.dat: function %i first_statement %d "
                     "out of range (numstatements=%d)",
                     i, fp->first_statement, ns);
         if (fp->parm_start < 0 || fp->locals < 0 ||
             fp->parm_start > ng - fp->locals)
            SV_Error("progs.dat: function %i parm_start/locals "
                     "out of range (parm_start=%d locals=%d "
                     "numglobals=%d)",
                     i, fp->parm_start, fp->locals, ng);
         if (fp->numparms < 0 || fp->numparms > MAX_PARMS)
            SV_Error("progs.dat: function %i numparms %d out of "
                     "range (max=%d)", i, fp->numparms, MAX_PARMS);
         if (fp->s_name < 0 || fp->s_name >= ssz)
            SV_Error("progs.dat: function %i s_name %d out of "
                     "range (strings_size=%d)", i, fp->s_name, ssz);
         if (fp->s_file < 0 || fp->s_file >= ssz)
            SV_Error("progs.dat: function %i s_file %d out of "
                     "range (strings_size=%d)", i, fp->s_file, ssz);
      }

      /* Globaldefs / fielddefs: ofs is the index used by
       * ED_ParseEpair (already bounded at use, but cheap to
       * check here too) and s_name must point into strings. */
      for (i = 0; i < ngd; i++) {
         if (pr_globaldefs[i].s_name < 0 ||
             pr_globaldefs[i].s_name >= ssz)
            SV_Error("progs.dat: globaldef %i s_name out of range",
                     i);
      }
      for (i = 0; i < nfd; i++) {
         if (pr_fielddefs[i].s_name < 0 ||
             pr_fielddefs[i].s_name >= ssz)
            SV_Error("progs.dat: fielddef %i s_name out of range",
                     i);
      }
   }

}


/*
===============
PR_Init
===============
*/
void
PR_Init(void)
{
    Cmd_AddCommand("profile", PR_Profile_f);
    Cvar_RegisterVariable(&nomonsters);
    Cvar_RegisterVariable(&gamecfg);
    Cvar_RegisterVariable(&scratch1);
    Cvar_RegisterVariable(&scratch2);
    Cvar_RegisterVariable(&scratch3);
    Cvar_RegisterVariable(&scratch4);
    Cvar_RegisterVariable(&savedgamecfg);
    Cvar_RegisterVariable(&saved1);
    Cvar_RegisterVariable(&saved2);
    Cvar_RegisterVariable(&saved3);
    Cvar_RegisterVariable(&saved4);
}

edict_t *
EDICT_NUM(int n)
{
    /* sv.max_edicts is set at server init for NQ and to MAX_EDICTS
     * for QW.  Bound against whichever is the active limit; the
     * dead-fallthrough case (QW_HACK !SERVERONLY) was the only
     * path where this function returned an unbounded computation,
     * but pr_edict.c is built only on server-capable targets so
     * that fallthrough is currently unreachable.  Make the check
     * unconditional so any future build that links pr_edict.c
     * into a non-server target stays safe. */
    if (n < 0 || n >= sv.max_edicts)
        SV_Error("%s: bad number %i (max %i)", __func__, n, sv.max_edicts);
    return (edict_t *)((byte *)sv.edicts + (n) * pr_edict_size);
}

edict_t *
PROG_TO_EDICT(int e)
{
    /* e is a byte offset into sv.edicts as stored by
     * EDICT_TO_PROG.  The engine sets edict-typed globals
     * (self / other / world / msg_entity) and eval_t.edict
     * field values to EDICT_TO_PROG(known_good_edict), so the
     * "trusted" path is always within the array.  But QuakeC
     * bytecode can also write directly to an edict-typed
     * global or field; the bytecode store ops bound the offset
     * within pr_globals[] but not the *value* being stored.
     * A hostile progs.dat can plant any int, then cause a
     * builtin to receive the resulting wild pointer and
     * dereference (e.g. ED_Free, PF_setorigin's
     * ent->v.origin).  That gives bytecode an OOB write into
     * arbitrary host memory at sv.edicts + offset.
     *
     * Bound the offset before computing the pointer: must be
     * non-negative, a multiple of pr_edict_size (otherwise the
     * offset doesn't address an edict's start), and less than
     * sv.max_edicts * pr_edict_size. */
    int max_byte_offset;

    if (!sv.edicts)
        SV_Error("%s: sv.edicts not allocated", __func__);
    max_byte_offset = sv.max_edicts * pr_edict_size;
    if (e < 0 || e >= max_byte_offset || (e % pr_edict_size) != 0)
        SV_Error("%s: bad offset %i (max %i, edict_size %i)",
                 __func__, e, max_byte_offset, pr_edict_size);
    return (edict_t *)((byte *)sv.edicts + e);
}

int
NUM_FOR_EDICT(const edict_t *e)
{
    /* Pointer subtraction across distinct allocations is UB in
     * the C abstract machine.  e usually came from EDICT_NUM or
     * PROG_TO_EDICT and so is within the edicts array, but
     * defensively bound the pointer against the array's byte
     * extent before doing the subtraction.  Also reject
     * pointers that aren't on a pr_edict_size boundary -- a
     * misaligned pointer would yield a non-integer division and
     * silently round, producing the wrong edict index. */
    const byte *p = (const byte *)e;
    const byte *base = (const byte *)sv.edicts;
    int b;

    if (!sv.edicts || p < base
        || p >= base + (size_t)sv.max_edicts * (size_t)pr_edict_size
        || ((size_t)(p - base)) % (size_t)pr_edict_size != 0)
	SV_Error("%s: bad pointer", __func__);

    b = (p - base) / pr_edict_size;

    if (b < 0 || b >= sv.num_edicts)
	SV_Error("%s: bad pointer", __func__);
    return b;
}
