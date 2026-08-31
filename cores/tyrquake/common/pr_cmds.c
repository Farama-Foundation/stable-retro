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

#include <float.h>

#include <compat/strl.h>

#include "cmd.h"
#include "console.h"
#include "model.h"
#include "progs.h"
#include "server.h"
#include "world.h"

#include "client.h"        /* CL_PersistGib for r_persistgibs cvar */
#include "host.h"
#include "protocol.h"
#include "quakedef.h"
#include "sys.h"
/* FIXME - quick hack to enable merging of NQ/QWSV shared code */
#define SV_Error Host_Error

extern cvar_t r_persistgibs;

/*
 * Returns true iff the given model name string starts with one of
 * the known Quake gib model prefixes.  Used to decide whether to
 * snapshot an entity into the persistent-gib pool just before
 * PF_Remove frees it.  Lives here next to PF_Remove rather than in
 * cl_main.c so the model-name check is co-located with its only
 * caller; the pool itself and the snapshot routine live with the
 * other client-side state in cl_main.c.
 */
static qboolean
SV_IsGibModelName(const char *name)
{
    if (!name)
	return false;
    if (!strncmp(name, "progs/gib",     9))  return true;
    if (!strncmp(name, "progs/h_",      8))  return true;
    if (!strncmp(name, "progs/zom_gib", 13)) return true;
    return false;
}

#define	RETURN_EDICT(e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))
#define	RETURN_STRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_SetString(s))

/*
===============================================================================

						BUILT-IN FUNCTIONS

===============================================================================
*/

static const char *PF_VarString(int first)
{
    static char out[512];
    int i;
    out[0] = 0;
    for (i = first; i < pr_argc; i++)
    {
	const char *arg = G_STRING(OFS_PARM0 + i * 3);
	/* strlcat appends as much of arg as fits before the
	 * sizeof(out)-1 boundary, returns the total length
	 * the result would have been.  >= sizeof(out) means
	 * truncation happened. */
	if (strlcat(out, arg, sizeof(out)) >= sizeof(out))
	{
	    Con_DPrintf("%s: overflow (string truncated)\n", __func__);
	    break;
	}
    }
    return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
static void PF_error(void)
{
    edict_t *ed;
    const char *s = PF_VarString(0);
    Con_Printf("======SERVER ERROR in %s:\n%s\n",
	       PR_GetString(pr_xfunction->s_name), s);
    ed = PROG_TO_EDICT(pr_global_struct->self);
    ED_Print(ed);

    SV_Error("Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
static void
PF_objerror(void)
{
    const char *s;
    edict_t *ed;

    s = PF_VarString(0);
    Con_Printf("======OBJECT ERROR in %s:\n%s\n",
	       PR_GetString(pr_xfunction->s_name), s);
    ed = PROG_TO_EDICT(pr_global_struct->self);
    ED_Print(ed);
    ED_Free(ed);

    SV_Error("Program error");
}



/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
static void
PF_makevectors(void)
{
    AngleVectors(G_VECTOR(OFS_PARM0), pr_global_struct->v_forward,
		 pr_global_struct->v_right, pr_global_struct->v_up);
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the
world (setting velocity and waiting).  Directly changing origin will not set
internal links correctly, so clipping would be messed up.  This should be
called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
static void
PF_setorigin(void)
{
    edict_t *e;
    float *org;

    e = G_EDICT(OFS_PARM0);
    org = G_VECTOR(OFS_PARM1);
    VectorCopy(org, e->v.origin);
    SV_LinkEdict(e, false);
}

static void
SetMinMaxSize(edict_t *e, float *min, float *max, qboolean rotate)
{
    float *angles;
    vec3_t rmin, rmax;
    float bounds[2][3];
    float xvector[2], yvector[2];
    float a;
    vec3_t base, transformed;
    int i, j, k, l;

    for (i = 0; i < 3; i++)
	if (min[i] > max[i])
	    PR_RunError("backwards mins/maxs");

    rotate = false;		/* FIXME: implement rotation properly again */

    if (!rotate) {
	VectorCopy(min, rmin);
	VectorCopy(max, rmax);
    } else {
	/* find min / max for rotations */
	angles = e->v.angles;

	a = angles[1] / 180 * M_PI;

	xvector[0] = cosf(a);
	xvector[1] = sinf(a);
	yvector[0] = -sinf(a);
	yvector[1] = cosf(a);

	VectorCopy(min, bounds[0]);
	VectorCopy(max, bounds[1]);

	rmin[0] = rmin[1] = rmin[2] = FLT_MAX;
	rmax[0] = rmax[1] = rmax[2] = -FLT_MAX;

	for (i = 0; i <= 1; i++) {
	    base[0] = bounds[i][0];
	    for (j = 0; j <= 1; j++) {
		base[1] = bounds[j][1];
		for (k = 0; k <= 1; k++) {
		    base[2] = bounds[k][2];

		    /* transform the point */
		    transformed[0] =
			xvector[0] * base[0] + yvector[0] * base[1];
		    transformed[1] =
			xvector[1] * base[0] + yvector[1] * base[1];
		    transformed[2] = base[2];

		    for (l = 0; l < 3; l++) {
			if (transformed[l] < rmin[l])
			    rmin[l] = transformed[l];
			if (transformed[l] > rmax[l])
			    rmax[l] = transformed[l];
		    }
		}
	    }
	}
    }

/* set derived values */
    VectorCopy(rmin, e->v.mins);
    VectorCopy(rmax, e->v.maxs);
    VectorSubtract(max, min, e->v.size);

    SV_LinkEdict(e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
static void
PF_setsize(void)
{
    edict_t *e;
    float *min, *max;

    e = G_EDICT(OFS_PARM0);
    min = G_VECTOR(OFS_PARM1);
    max = G_VECTOR(OFS_PARM2);
    SetMinMaxSize(e, min, max, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
Also sets size, mins, and maxs for inline bmodels
=================
*/
static void
PF_setmodel(void)
{
    int i;
    model_t *mod;
    const char **check;
    edict_t *e    = G_EDICT(OFS_PARM0);
    const char *m = G_STRING(OFS_PARM1);
    int max = max_models(sv.protocol);

    /* check to see if model was properly precached.  Defensive
     * upper bound on the walk: sv.model_precache[] is sized
     * MAX_MODELS and the precache builtins guarantee a trailing
     * NULL while there are free slots, but if every slot is
     * occupied (a path the precache builtins reject with
     * PR_RunError) the *check sentinel never trips and the
     * loop walks off the end of the array. */
    for (i = 0, check = sv.model_precache; i < max && *check; i++, check++)
	if (!strcmp(*check, m))
	    break;

    if (i == max || !*check)
	PR_RunError("no precache: %s\n", m);

    e->v.model = PR_SetString(m);
    e->v.modelindex = i;

    mod = sv.models[(int)e->v.modelindex];
    if (mod)
	SetMinMaxSize(e, mod->mins, mod->maxs, true);
    else
	SetMinMaxSize(e, vec3_origin, vec3_origin, true);
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
static void
PF_bprint(void)
{
    const char *s = PF_VarString(0);
    SV_BroadcastPrintf("%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
static void
PF_sprint(void)
{
    client_t *client;
    int entnum    = G_EDICTNUM(OFS_PARM0);
    const char *s = PF_VarString(1);

    if (entnum < 1 || entnum > svs.maxclients) {
	Con_Printf("tried to sprint to a non-client\n");
	return;
    }

    client = &svs.clients[entnum - 1];
    MSG_WriteChar(&client->message, svc_print);
    MSG_WriteString(&client->message, s);
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
static void
PF_centerprint(void)
{
    client_t *client;
    int entnum    = G_EDICTNUM(OFS_PARM0);
    const char *s = PF_VarString(1);

    if (entnum < 1 || entnum > svs.maxclients) {
	Con_Printf("tried to sprint to a non-client\n");
	return;
    }

    client = &svs.clients[entnum - 1];
    MSG_WriteChar(&client->message, svc_centerprint);
    MSG_WriteString(&client->message, s);
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
static void
PF_normalize(void)
{
    vec3_t newvalue;
    float *value1 = G_VECTOR(OFS_PARM0);
    float newval  =
	value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
    newval = sqrtf(newval);

    if (newval == 0)
	newvalue[0] = newvalue[1] = newvalue[2] = 0;
    else {
	newval = 1 / newval;
	newvalue[0] = value1[0] * newval;
	newvalue[1] = value1[1] * newval;
	newvalue[2] = value1[2] * newval;
    }

    VectorCopy(newvalue, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
static void
PF_vlen(void)
{
    float *value1 = G_VECTOR(OFS_PARM0);
    float newobj  =
	value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
    newobj = sqrtf(newobj);

    G_FLOAT(OFS_RETURN) = newobj;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
static void
PF_vectoyaw(void)
{
    float yaw;
    float *value1 = G_VECTOR(OFS_PARM0);

    if (value1[1] == 0 && value1[0] == 0)
	yaw = 0;
    else
    {
	yaw = (int)(atan2f(value1[1], value1[0]) * 180 / M_PI);
	if (yaw < 0)
	    yaw += 360;
    }

    G_FLOAT(OFS_RETURN) = yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
static void
PF_vectoangles(void)
{
    float forward;
    float yaw, pitch;
    float *value1 = G_VECTOR(OFS_PARM0);

    if (value1[1] == 0 && value1[0] == 0) {
	yaw = 0;
	if (value1[2] > 0)
	    pitch = 90;
	else
	    pitch = 270;
    } else {
	yaw = (int)(atan2f(value1[1], value1[0]) * 180 / M_PI);
	if (yaw < 0)
	    yaw += 360;

	forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
	pitch = (int)(atan2f(value1[2], forward) * 180 / M_PI);
	if (pitch < 0)
	    pitch += 360;
    }

    G_FLOAT(OFS_RETURN + 0) = pitch;
    G_FLOAT(OFS_RETURN + 1) = yaw;
    G_FLOAT(OFS_RETURN + 2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
static void
PF_random(void)
{
    float num = (rand() & 0x7fff) / ((float)0x7fff);

    G_FLOAT(OFS_RETURN) = num;
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
static void
PF_particle(void)
{
    float *org  = G_VECTOR(OFS_PARM0);
    float *dir  = G_VECTOR(OFS_PARM1);
    float color = G_FLOAT(OFS_PARM2);
    float count = G_FLOAT(OFS_PARM3);
    SV_StartParticle(org, dir, color, count);
}

static void
PF_WriteSoundNum_Static(sizebuf_t *sb, int c)
{
    switch (sv.protocol) {
    case PROTOCOL_VERSION_NQ:
    case PROTOCOL_VERSION_BJP:
    case PROTOCOL_VERSION_BJP3:
	MSG_WriteByte(sb, c);
	break;
    case PROTOCOL_VERSION_BJP2:
	MSG_WriteShort(sb, c);
	break;
    case PROTOCOL_VERSION_FITZ:
	if (c > 255)
	    MSG_WriteShort(sb, c);
	else
	    MSG_WriteByte(sb, c);
	break;
    default:
	Host_Error("%s: Unknown protocol version (%d)\n", __func__,
		   sv.protocol);
    }
}

/*
=================
PF_ambientsound

=================
*/
static void
PF_ambientsound(void)
{
    int i, soundnum;
    const char **check;
    float *pos        = G_VECTOR(OFS_PARM0);
    const char *samp  = G_STRING(OFS_PARM1);
    float vol         = G_FLOAT(OFS_PARM2);
    float attenuation = G_FLOAT(OFS_PARM3);

    /* check to see if samp was properly precached */
    for (soundnum = 0, check = sv.sound_precache; *check; check++, soundnum++)
	if (!strcmp(*check, samp))
	    break;

    if (!*check) {
	Con_Printf("no precache: %s\n", samp);
	return;
    }

/* add an svc_spawnambient command to the level signon packet */
    if (sv.protocol == PROTOCOL_VERSION_FITZ && soundnum > 255)
	MSG_WriteByte(&sv.signon, svc_fitz_spawnstaticsound2);
    else
	MSG_WriteByte(&sv.signon, svc_spawnstaticsound);
    for (i = 0; i < 3; i++)
	MSG_WriteCoord(&sv.signon, pos[i]);

    PF_WriteSoundNum_Static(&sv.signon, soundnum);
    MSG_WriteByte(&sv.signon, vol * 255);
    MSG_WriteByte(&sv.signon, attenuation * 64);
}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
static void PF_sound(void)
{
    edict_t *entity    = G_EDICT(OFS_PARM0);
    int channel        = G_FLOAT(OFS_PARM1);
    const char *sample = G_STRING(OFS_PARM2);
    int volume         = G_FLOAT(OFS_PARM3) * 255;
    float attenuation  = G_FLOAT(OFS_PARM4);

    if (volume < 0 || volume > 255)
	Sys_Error("%s: volume = %i", __func__, volume);

    if (attenuation < 0 || attenuation > 4)
	Sys_Error("%s: attenuation = %f", __func__, attenuation);

    if (channel < 0 || channel > 7)
	Sys_Error("%s: channel = %i", __func__, channel);

    SV_StartSound(entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

QC's break() builtin -- intended in the original
1996 code as a debug-only "drop into the debugger"
hook, originally implemented by writing to address
-4 to deliberately segfault.  tyrquake had this
calling abort(), which from a libretro core takes
down the entire frontend process when QC code
invokes break() -- a hostile or malformed
progs.dat could drop a "break();" anywhere in
its source and the user's whole RetroArch session
would die.

Treat as a programmer error in the QC: print a
diagnostic and drop into the standard PR_RunError
path which longjmps back to the host-frame
boundary (skips the rest of the QC frame, returns
control to the engine, the level state survives).
=================
*/
static void PF_break(void)
{
    PR_RunError("break statement in %s",
                PR_GetString(pr_xfunction->s_name));
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
static void
PF_traceline(void)
{
    float *v1, *v2;
    trace_t trace;
    int nomonsters;
    edict_t *ent;

    v1 = G_VECTOR(OFS_PARM0);
    v2 = G_VECTOR(OFS_PARM1);
    nomonsters = G_FLOAT(OFS_PARM2);
    ent = G_EDICT(OFS_PARM3);

    trace = SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

    pr_global_struct->trace_allsolid = trace.allsolid;
    pr_global_struct->trace_startsolid = trace.startsolid;
    pr_global_struct->trace_fraction = trace.fraction;
    pr_global_struct->trace_inwater = trace.inwater;
    pr_global_struct->trace_inopen = trace.inopen;
    VectorCopy(trace.endpos, pr_global_struct->trace_endpos);
    VectorCopy(trace.plane.normal, pr_global_struct->trace_plane_normal);
    pr_global_struct->trace_plane_dist = trace.plane.dist;
    if (trace.ent)
	pr_global_struct->trace_ent = EDICT_TO_PROG(trace.ent);
    else
	pr_global_struct->trace_ent = EDICT_TO_PROG(sv.edicts);
}

/* ============================================================================ */

static int
PF_newcheckclient(int check)
{
    int entnum;
    edict_t *ent;
    vec3_t org;

/* cycle to the next one */
    check = qclamp(check, 1, svs.maxclients);
    entnum = (check == svs.maxclients) ? 1 : check + 1;

    for (;; entnum++) {
	if (entnum == svs.maxclients + 1)
	    entnum = 1;

	ent = EDICT_NUM(entnum);

	if (entnum == check)
	    break;		/* didn't find anything else */

	if (ent->free)
	    continue;
	if (ent->v.health <= 0)
	    continue;
	if ((int)ent->v.flags & FL_NOTARGET)
	    continue;

	/* anything that is a client, or has a client as an enemy */
	break;
    }

/* get the current leaf for the entity */
    VectorAdd(ent->v.origin, ent->v.view_ofs, org);
    sv.checkleaf = Mod_PointInLeaf(sv.worldmodel, org);

    return entnum;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define	MAX_CHECK	16
static int c_invis, c_notvis;
static void
PF_checkclient(void)
{
    edict_t *ent, *self;
    mleaf_t *leaf;
    int leafnum;
    const leafbits_t *checkpvs;
    vec3_t view;

/* find a new check if on a new frame */
    if (sv.time - sv.lastchecktime >= 0.1) {
	sv.lastcheck = PF_newcheckclient(sv.lastcheck);
	sv.lastchecktime = sv.time;
    }
/* return check if it might be visible */
    ent = EDICT_NUM(sv.lastcheck);
    if (ent->free || ent->v.health <= 0) {
	RETURN_EDICT(sv.edicts);
	return;
    }
/* if current entity can't possibly see the check entity, return 0 */
    checkpvs = Mod_LeafPVS(sv.worldmodel, sv.checkleaf);
    self = PROG_TO_EDICT(pr_global_struct->self);
    VectorAdd(self->v.origin, self->v.view_ofs, view);
    leaf = Mod_PointInLeaf(sv.worldmodel, view);
    leafnum = (leaf - sv.worldmodel->leafs) - 1;
    if (leafnum < 0 || !Mod_TestLeafBit(checkpvs, leafnum)) {
	c_notvis++;
	RETURN_EDICT(sv.edicts);
	return;
    }
/* might be able to see it */
    c_invis++;
    RETURN_EDICT(ent);
}

/* ============================================================================ */


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
static void
PF_stuffcmd(void)
{
    const char *str;
    client_t *client;
    int entnum = G_EDICTNUM(OFS_PARM0);
    if (entnum < 1 || entnum > svs.maxclients)
	PR_RunError("Parm 0 not a client");
    str = G_STRING(OFS_PARM1);

    client = host_client;
    host_client = &svs.clients[entnum - 1];
    Host_ClientCommands("%s", str);
    host_client = client;
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
static void PF_localcmd(void)
{
    const char *text = G_STRING(OFS_PARM0);

    /* QC's localcmd stuffs arbitrary text into the
     * client console execution buffer.  cmd_text is
     * a fixed 8KB sizebuf, so a single oversized call
     * is bounded -- but a tight QC loop calling
     * localcmd with multi-KB strings each frame fills
     * cmd_text faster than Cbuf_Execute can drain it,
     * leading to overflow logs every frame and eventual
     * Cmd_TokenizeString Z_Malloc churn (per-arg
     * allocation, fragmenting the zone).
     *
     * Cap individual calls at 1KB; legitimate QC uses
     * are short ("centerprint", "setcvar deathmatch
     * 1", ...).  Sister to the clc_stringcmd 1024-byte
     * cap from the previous round. */
    if (strlen(text) >= 1024)
        PR_RunError("%s: text too long (%u, max 1023)",
                    __func__, (unsigned)strlen(text));

    Cbuf_AddText("%s", text);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
static void PF_cvar(void)
{
    const char *var = G_STRING(OFS_PARM0);

    G_FLOAT(OFS_RETURN) = Cvar_VariableValue(var);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
static void PF_cvar_set(void)
{
    const char *var = G_STRING(OFS_PARM0);
    const char *val = G_STRING(OFS_PARM1);

    /* Both args are QC-controlled.  Cvar_Set Z_Mallocs a
     * copy of `val` of strlen(val)+1 bytes on every
     * change.  A hostile or buggy progs.dat that passes
     * a multi-KB string here, or that calls cvar_set in
     * a tight loop with varying-length values, drives
     * the zone allocator into fragmentation/exhaustion
     * (sister to PF_lightstyle / PF_precache_*).  Stock
     * Quake cvar values are short numeric strings and
     * the longest legitimate use case (e.g. _cl_name) is
     * still under 64 bytes; cap at MAX_QPATH for the
     * same reasons as the precache path -- generous but
     * finite.  Also bound the cvar name itself; a QC-
     * supplied multi-KB name walks Cvar_FindVar's
     * strcmp loop pointlessly. */
    if (strlen(var) >= MAX_QPATH)
        PR_RunError("%s: cvar name too long (%u, max %d)",
                    __func__, (unsigned)strlen(var),
                    MAX_QPATH - 1);
    if (strlen(val) >= MAX_QPATH)
        PR_RunError("%s: cvar value too long (%u, max %d)",
                    __func__, (unsigned)strlen(val),
                    MAX_QPATH - 1);

    Cvar_Set(var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
static void PF_findradius(void)
{
    int i, j;
    vec3_t eorg;
    edict_t *chain = (edict_t *)sv.edicts;
    float *org     = G_VECTOR(OFS_PARM0);
    float rad      = G_FLOAT(OFS_PARM1);
    edict_t *ent   = NEXT_EDICT(sv.edicts);

    for (i = 1; i < sv.num_edicts; i++, ent = NEXT_EDICT(ent))
    {
	if (ent->free)
	    continue;
	if (ent->v.solid == SOLID_NOT)
	    continue;
	for (j = 0; j < 3; j++)
	    eorg[j] =
		org[j] - (ent->v.origin[j] +
			  (ent->v.mins[j] + ent->v.maxs[j]) * 0.5);
	if (Length(eorg) > rad)
	    continue;

	ent->v.chain = EDICT_TO_PROG(chain);
	chain = ent;
    }

    RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
static void PF_dprint(void)
{
    Con_DPrintf("%s", PF_VarString(0));
}

static char pr_string_temp[128];

static void PF_ftos(void)
{
    float v = G_FLOAT(OFS_PARM0);
    if (v == (int)v)
	snprintf(pr_string_temp, sizeof(pr_string_temp), "%d", (int)v);
    else
	snprintf(pr_string_temp, sizeof(pr_string_temp), "%5.1f", v);
    G_INT(OFS_RETURN) = PR_SetString(pr_string_temp);
}

static void PF_fabs(void)
{
    float v = G_FLOAT(OFS_PARM0);
    G_FLOAT(OFS_RETURN) = fabsf(v);
}

static void
PF_vtos(void)
{
    snprintf(pr_string_temp, sizeof(pr_string_temp), "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0],
	    G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
    G_INT(OFS_RETURN) = PR_SetString(pr_string_temp);
}

static void
PF_Spawn(void)
{
    edict_t *ed;

    ed = ED_Alloc();
    RETURN_EDICT(ed);
}

static void
PF_Remove(void)
{
    edict_t *ed;

    ed = G_EDICT(OFS_PARM0);

    /* If r_persistgibs is set, intercept the removal of gib edicts
     * and snapshot them into the persistent-gib pool first.  The
     * snapshot is purely visual: we copy the entity's final origin,
     * angles, model, frame, and skin, then let ED_Free proceed
     * normally so the server-side edict slot is still recovered.
     *
     * Detection is by model name; the QC `remove()` builtin is the
     * standard removal path used by SUB_Remove (the deferred
     * self-removal scheduled by ThrowGib in combat.qc).  Mid-flight
     * gibs that get killed early would also be captured at their
     * mid-flight position, but that's a rare edge case and they
     * just freeze in place -- still better than disappearing. */
    if (r_persistgibs.value != 0.0f && !ed->free && ed->v.model) {
	const char *name = PR_GetString(ed->v.model);
	if (SV_IsGibModelName(name)) {
	    CL_PersistGib(ed->v.origin, ed->v.angles, name,
	                  (int)ed->v.frame, (int)ed->v.skin);
	}
    }

    ED_Free(ed);
}


/* entity (entity start, .string field, string match) find = #5; */
static void
PF_Find(void)
{
    int e;
    int f;
    const char *s, *t;
    edict_t *ed;

    e = G_EDICTNUM(OFS_PARM0);
    f = G_INT(OFS_PARM1);
    s = G_STRING(OFS_PARM2);
    if (!s)
	PR_RunError("%s: bad search string", __func__);

    /* f is a field offset (in float-units, i.e. 4-byte slots)
     * into edict->v.  The valid range is [0, entityfields);
     * reading past that walks past ed->v into adjacent memory.
     * Bytecode store ops bound their own offsets but f comes
     * from a global / parameter and isn't checked at the call
     * site.  Add the explicit bound here. */
    if (f < 0 || f >= progs->entityfields)
	PR_RunError("%s: bad field offset %i (max %i)", __func__,
	            f, progs->entityfields);

    for (e++; e < sv.num_edicts; e++) {
	ed = EDICT_NUM(e);
	if (ed->free)
	    continue;
	t = E_STRING(ed, f);
	if (!t)
	    continue;
	if (!strcmp(t, s)) {
	    RETURN_EDICT(ed);
	    return;
	}
    }

    RETURN_EDICT(sv.edicts);
}

static void
PR_CheckEmptyString(const char *s)
{
    if (s[0] <= ' ')
	PR_RunError("%s: Bad string", __func__);
    /* Cap precache and similar QC-passed name strings at
     * MAX_QPATH.  Without this, a malicious or buggy
     * progs.dat passing a multi-KB string to precache_
     * model / precache_sound is later reflected into
     * every connecting client's message buffer via
     * SV_SendServerinfo's MSG_WriteString loop, blowing
     * past client->message.maxsize.  Models also try to
     * open the path via Mod_ForName, where stock Quake
     * has its own MAX_QPATH-sized buffers downstream.
     * Sister to PF_lightstyle's MAX_STYLESTRING cap. */
    if (strlen(s) >= MAX_QPATH)
	PR_RunError("%s: string too long (%u, max %d)",
	            __func__, (unsigned)strlen(s), MAX_QPATH - 1);
}

static void
PF_precache_file(void)
{
    /* precache_file is only used to copy files with qcc, it does nothing */
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

static void
PF_precache_sound(void)
{
    const char *s;
    int i;

    if (sv.state != ss_loading)
	PR_RunError("%s: Precache can only be done in spawn functions",
		    __func__);

    s = G_STRING(OFS_PARM0);
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
    PR_CheckEmptyString(s);

    for (i = 0; i < max_sounds(sv.protocol); i++) {
	if (!sv.sound_precache[i]) {
	    sv.sound_precache[i] = s;
	    return;
	}
	if (!strcmp(sv.sound_precache[i], s))
	    return;
    }
    PR_RunError("%s: overflow (max = %d)", __func__, max_sounds(sv.protocol));
}

static void
PF_precache_model(void)
{
    const char *s;
    int i;

    if (sv.state != ss_loading)
	PR_RunError("%s: Precache can only be done in spawn functions",
		    __func__);

    s = G_STRING(OFS_PARM0);
    G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
    PR_CheckEmptyString(s);

    for (i = 0; i < max_models(sv.protocol); i++) {
	if (!sv.model_precache[i]) {
	    sv.model_precache[i] = s;
	    sv.models[i] = Mod_ForName(s, true);
	    return;
	}
	if (!strcmp(sv.model_precache[i], s))
	    return;
    }
    PR_RunError("%s: overflow (max = %d)", __func__, max_models(sv.protocol));
}

/* QC's traceon()/traceoff() builtins enable a per-
 * bytecode-instruction PR_PrintStatement print loop
 * (see pr_exec.c).  Useful as a development aid in
 * 1996; in a libretro core a malicious or buggy
 * progs.dat that calls traceon() and never calls
 * traceoff() floods the console with millions of
 * Con_Printf calls per second and tanks framerate.
 * No legitimate shipped QC uses this -- stub both as
 * no-ops to keep the builtin index stable. */
static void
PF_traceon(void)
{
}

static void
PF_traceoff(void)
{
}

static void
PF_eprint(void)
{
    ED_PrintNum(G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
static void
PF_walkmove(void)
{
    edict_t *ent;
    float yaw, dist;
    vec3_t move;
    dfunction_t *oldf;
    int oldself;

    ent = PROG_TO_EDICT(pr_global_struct->self);
    yaw = G_FLOAT(OFS_PARM0);
    dist = G_FLOAT(OFS_PARM1);

    if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM))) {
	G_FLOAT(OFS_RETURN) = 0;
	return;
    }

    yaw = yaw * M_PI * 2 / 360;

    move[0] = cosf(yaw) * dist;
    move[1] = sinf(yaw) * dist;
    move[2] = 0;

/* save program state, because SV_movestep may call other progs */
    oldf = pr_xfunction;
    oldself = pr_global_struct->self;

    G_FLOAT(OFS_RETURN) = SV_movestep(ent, move, true);


/* restore program state */
    pr_xfunction = oldf;
    pr_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
static void
PF_droptofloor(void)
{
    edict_t *ent;
    vec3_t end;
    trace_t trace;

    ent = PROG_TO_EDICT(pr_global_struct->self);

    VectorCopy(ent->v.origin, end);
    end[2] -= 256;

    trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

    if (trace.fraction == 1 || trace.allsolid)
	G_FLOAT(OFS_RETURN) = 0;
    else {
	VectorCopy(trace.endpos, ent->v.origin);
	SV_LinkEdict(ent, false);
	ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
	ent->v.groundentity = EDICT_TO_PROG(trace.ent);
	G_FLOAT(OFS_RETURN) = 1;
    }
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
static void
PF_lightstyle(void)
{
    int style;
    const char *val;
    client_t *client;
    int i;

    style = G_FLOAT(OFS_PARM0);
    val = G_STRING(OFS_PARM1);

    /* style is a QC-controlled float cast to int; reject
     * out-of-range values rather than indexing past
     * sv.lightstyles[MAX_LIGHTSTYLES] and stomping adjacent
     * server state.  The svc_lightstyle network message also
     * encodes style as a single byte / char, so values >= 256
     * would silently truncate when sent. */
    if (style < 0 || style >= MAX_LIGHTSTYLES)
        PR_RunError("%s: bad style %d (max %d)", __func__,
                    style, MAX_LIGHTSTYLES);

    /* val is a QC-controlled string of unbounded length.
     * Stock Quake lightstyles are 4-char patterns ("amma"
     * etc.) but a malicious or buggy progs.dat could pass
     * an arbitrarily long string; the client snprintf-
     * truncates to MAX_STYLESTRING on receive (cl_parse.c
     * svc_lightstyle handler), but the SERVER side
     * MSG_WriteString below transmits the whole thing,
     * pushing a multi-KB svc_lightstyle into every active
     * client's message buffer.  That overflows host_client
     * ->message and trips Sys_Error -- a self-DoS the
     * local server's QC can drive against its own client
     * (sister to PF_break / PF_traceon hardening). */
    if (strlen(val) >= MAX_STYLESTRING)
        PR_RunError("%s: style %d string too long (%u, max %d)",
                    __func__, style, (unsigned)strlen(val),
                    MAX_STYLESTRING - 1);

/* change the string in sv */
    sv.lightstyles[style] = val;

/* send message to all clients on this server */
    if (sv.state != ss_active)
	return;

    for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	if (client->active || client->spawned) {
	    MSG_WriteChar(&client->message, svc_lightstyle);
	    MSG_WriteChar(&client->message, style);
	    MSG_WriteString(&client->message, val);
	}
}

static void
PF_rint(void)
{
    float f;

    f = G_FLOAT(OFS_PARM0);
    if (f > 0)
	G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
    else
	G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}

static void
PF_floor(void)
{
    G_FLOAT(OFS_RETURN) = floorf(G_FLOAT(OFS_PARM0));
}

static void
PF_ceil(void)
{
    G_FLOAT(OFS_RETURN) = ceilf(G_FLOAT(OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
static void
PF_checkbottom(void)
{
    edict_t *ent;

    ent = G_EDICT(OFS_PARM0);

    G_FLOAT(OFS_RETURN) = SV_CheckBottom(ent);
}

/*
=============
PF_pointcontents
=============
*/
static void
PF_pointcontents(void)
{
    float *v;

    v = G_VECTOR(OFS_PARM0);

    G_FLOAT(OFS_RETURN) = SV_PointContents(v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
static void
PF_nextent(void)
{
    int i;
    edict_t *ent;

    i = G_EDICTNUM(OFS_PARM0);
    while (1) {
	i++;
	if (i == sv.num_edicts) {
	    RETURN_EDICT(sv.edicts);
	    return;
	}
	ent = EDICT_NUM(i);
	if (!ent->free) {
	    RETURN_EDICT(ent);
	    return;
	}
    }
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
cvar_t sv_aim = { "sv_aim", "0.93" };

static void
PF_aim(void)
{
    edict_t *check, *bestent;
    vec3_t start, dir, end, bestdir;
    int i, j;
    trace_t tr;
    float dist, bestdist;
    /* NOTE: missilespeed parameter is ignored */
    /* float speed; */
    edict_t *ent = G_EDICT(OFS_PARM0);

    VectorCopy(ent->v.origin, start);
    start[2] += 20;

/* try sending a trace straight */
    VectorCopy(pr_global_struct->v_forward, dir);
    VectorMA(start, 2048, dir, end);
    tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
    if (tr.ent && tr.ent->v.takedamage == DAMAGE_AIM
	&& (!teamplay.value || ent->v.team <= 0
	    || ent->v.team != tr.ent->v.team)) {
	VectorCopy(pr_global_struct->v_forward, G_VECTOR(OFS_RETURN));
	return;
    }
/* try all possible entities */
    VectorCopy(dir, bestdir);
    bestdist = sv_aim.value;
    bestent = NULL;

    check = NEXT_EDICT(sv.edicts);
    for (i = 1; i < sv.num_edicts; i++, check = NEXT_EDICT(check)) {
	if (check->v.takedamage != DAMAGE_AIM)
	    continue;
	if (check == ent)
	    continue;
	if (teamplay.value && ent->v.team > 0 && ent->v.team == check->v.team)
	    continue;		/* don't aim at teammate */
	for (j = 0; j < 3; j++)
	    end[j] = check->v.origin[j]
		+ 0.5 * (check->v.mins[j] + check->v.maxs[j]);
	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dist = DotProduct(dir, pr_global_struct->v_forward);
	if (dist < bestdist)
	    continue;		/* to far to turn */
	tr = SV_Move(start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.ent == check) {	/* can shoot at this one */
	    bestdist = dist;
	    bestent = check;
	}
    }

    if (bestent) {
	VectorSubtract(bestent->v.origin, ent->v.origin, dir);
	dist = DotProduct(dir, pr_global_struct->v_forward);
	VectorScale(pr_global_struct->v_forward, dist, end);
	end[2] = dir[2];
	VectorNormalize(end);
	VectorCopy(end, G_VECTOR(OFS_RETURN));
    } else {
	VectorCopy(bestdir, G_VECTOR(OFS_RETURN));
    }
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void PF_changeyaw(void)
{
    float move;
    edict_t *ent  = PROG_TO_EDICT(pr_global_struct->self);
    float current = anglemod(ent->v.angles[1]);
    float ideal   = ent->v.ideal_yaw;
    float speed   = ent->v.yaw_speed;

    if (current == ideal)
	return;
    move = ideal - current;
    if (ideal > current) {
	if (move >= 180)
	    move = move - 360;
    } else {
	if (move <= -180)
	    move = move + 360;
    }
    if (move > 0) {
	if (move > speed)
	    move = speed;
    } else {
	if (move < -speed)
	    move = -speed;
    }

    ent->v.angles[1] = anglemod(current + move);
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define MSG_BROADCAST	0	/* unreliable to all            */
#define MSG_ONE		1	/* reliable to one (msg_entity) */
#define MSG_ALL		2	/* reliable to all              */
#define MSG_INIT	3	/* write to the init string     */

static sizebuf_t *
WriteDest(void)
{
    int entnum;
    edict_t *ent;
    int dest = G_FLOAT(OFS_PARM0);
    switch (dest) {
    case MSG_BROADCAST:
	return &sv.datagram;

    case MSG_ONE:
	ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
	entnum = NUM_FOR_EDICT(ent);
	if (entnum < 1 || entnum > svs.maxclients)
	    PR_RunError("%s: not a client", __func__);
	return &svs.clients[entnum - 1].message;

    case MSG_ALL:
	return &sv.reliable_datagram;

    case MSG_INIT:
	return &sv.signon;

    default:
	PR_RunError("%s: bad destination", __func__);
	break;
    }

    return NULL;
}

static void
PF_WriteByte(void)
{
    MSG_WriteByte(WriteDest(), G_FLOAT(OFS_PARM1));
}

static void
PF_WriteChar(void)
{
    MSG_WriteChar(WriteDest(), G_FLOAT(OFS_PARM1));
}

static void
PF_WriteShort(void)
{
    MSG_WriteShort(WriteDest(), G_FLOAT(OFS_PARM1));
}

static void
PF_WriteLong(void)
{
    MSG_WriteLong(WriteDest(), G_FLOAT(OFS_PARM1));
}

static void
PF_WriteAngle(void)
{
    MSG_WriteAngle(WriteDest(), G_FLOAT(OFS_PARM1));
}

static void
PF_WriteCoord(void)
{
    MSG_WriteCoord(WriteDest(), G_FLOAT(OFS_PARM1));
}

static void
PF_WriteString(void)
{
    MSG_WriteString(WriteDest(), G_STRING(OFS_PARM1));
}

static void
PF_WriteEntity(void)
{
    MSG_WriteShort(WriteDest(), G_EDICTNUM(OFS_PARM1));
}

/* ============================================================================= */

static void PF_makestatic(void)
{
    int i;
    unsigned int bits;
    edict_t *ent = G_EDICT(OFS_PARM0);

    bits = 0;
    if (sv.protocol == PROTOCOL_VERSION_FITZ)
    {
	if (SV_ModelIndex(PR_GetString(ent->v.model)) & 0xff00)
	    bits |= B_FITZ_LARGEMODEL;
	if ((int)ent->v.frame & 0xff00)
	    bits |= B_FITZ_LARGEFRAME;
    }

    if (bits) {
	MSG_WriteByte(&sv.signon, svc_fitz_spawnstatic2);
	MSG_WriteByte(&sv.signon, bits);
    } else {
	MSG_WriteByte(&sv.signon, svc_spawnstatic);
    }
    SV_WriteModelIndex(&sv.signon, SV_ModelIndex(PR_GetString(ent->v.model)), bits);

    MSG_WriteByte(&sv.signon, ent->v.frame);
    MSG_WriteByte(&sv.signon, ent->v.colormap);
    MSG_WriteByte(&sv.signon, ent->v.skin);

    for (i = 0; i < 3; i++) {
	MSG_WriteCoord(&sv.signon, ent->v.origin[i]);
	MSG_WriteAngle(&sv.signon, ent->v.angles[i]);
    }

    /* throw the entity away now */
    ED_Free(ent);
}

/* ============================================================================= */

/*
==============
PF_setspawnparms
==============
*/
static void PF_setspawnparms(void)
{
    client_t *client;
    edict_t *ent = G_EDICT(OFS_PARM0);
    int i        = NUM_FOR_EDICT(ent);
    if (i < 1 || i > svs.maxclients)
        PR_RunError("%s: Entity is not a client", __func__);

    /* copy spawn parms out of the client_t */
    client = svs.clients + (i - 1);

    for (i = 0; i < NUM_SPAWN_PARMS; i++)
	(&pr_global_struct->parm1)[i] = client->spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
static void
PF_changelevel(void)
{
    const char *s = G_STRING(OFS_PARM0);
    const char *p;

    /* QuakeC string is whatever progs.dat plants there.  Cbuf
     * interprets ';' and '\n' as command separators, so a
     * malicious progs that calls
     *   changelevel("e1m1; quit");
     *   changelevel("e1m1\nrcon_password foo");
     * gets arbitrary command execution at the host.  Reject any
     * separator-class character; map / changelevel arguments
     * are filenames and never legitimately need them.  Also
     * reject control characters and quotes which would corrupt
     * the Cbuf format on parse. */
    for (p = s; *p; p++) {
	unsigned char c = (unsigned char)*p;
	if (c == ';' || c == '\n' || c == '\r' || c == '"' || c == '\\' || c < 0x20)
	    PR_RunError("%s: bad map name", __func__);
    }
    /* make sure we don't issue two changelevels */
    if (svs.changelevel_issued)
	return;
    svs.changelevel_issued = true;

    Cbuf_AddText("changelevel %s\n", s);
}

static void PF_Fixme(void)
{
    PR_RunError("unimplemented bulitin");
}

builtin_t pr_builtin[] = {
    PF_Fixme,
    PF_makevectors,	/* void(entity e) makevectors                    = #1; */
    PF_setorigin,	/* void(entity e, vector o) setorigin            = #2; */
    PF_setmodel,	/* void(entity e, string m) setmodel             = #3; */
    PF_setsize,		/* void(entity e, vector min, vector max) setsize = #4; */
    PF_Fixme,		/* void(entity e, vector min, vector max) setabssize = #5; */
    PF_break,		/* void() break                                  = #6; */
    PF_random,		/* float() random                                = #7; */
    PF_sound,		/* void(entity e, float chan, string samp) sound = #8; */
    PF_normalize,	/* vector(vector v) normalize                    = #9; */
    PF_error,		/* void(string e) error                          = #10; */
    PF_objerror,	/* void(string e) objerror                       = #11; */
    PF_vlen,		/* float(vector v) vlen                          = #12; */
    PF_vectoyaw,	/* float(vector v) vectoyaw                      = #13; */
    PF_Spawn,		/* entity() spawn                                = #14; */
    PF_Remove,		/* void(entity e) remove                         = #15; */
    PF_traceline,	/* float(vector v1, vector v2, float tryents) traceline = #16; */
    PF_checkclient,	/* entity() clientlist                           = #17; */
    PF_Find,		/* entity(entity start, .string fld, string match) find = #18; */
    PF_precache_sound,	/* void(string s) precache_sound                 = #19; */
    PF_precache_model,	/* void(string s) precache_model                 = #20; */
    PF_stuffcmd,	/* void(entity client, string s)stuffcmd         = #21; */
    PF_findradius,	/* entity(vector org, float rad) findradius      = #22; */
    PF_bprint,		/* void(string s) bprint                         = #23; */
    PF_sprint,		/* void(entity client, string s) sprint          = #24; */
    PF_dprint,		/* void(string s) dprint                         = #25; */
    PF_ftos,		/* void(string s) ftos                           = #26; */
    PF_vtos,		/* void(string s) vtos                           = #27; */
    PF_Fixme,		/* PF_coredump - print edicts */
    PF_traceon,
    PF_traceoff,
    PF_eprint,		/* void(entity e) debug print an entire entity */
    PF_walkmove,	/* float(float yaw, float dist) walkmove */
    PF_Fixme,		/* float(float yaw, float dist) walkmove */
    PF_droptofloor,
    PF_lightstyle,
    PF_rint,
    PF_floor,
    PF_ceil,
    PF_Fixme,
    PF_checkbottom,
    PF_pointcontents,
    PF_Fixme,
    PF_fabs,
    PF_aim,
    PF_cvar,
    PF_localcmd,
    PF_nextent,
    PF_particle,
    PF_changeyaw,
    PF_Fixme,
    PF_vectoangles,

    PF_WriteByte,
    PF_WriteChar,
    PF_WriteShort,
    PF_WriteLong,
    PF_WriteCoord,
    PF_WriteAngle,
    PF_WriteString,
    PF_WriteEntity,

    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,
    PF_Fixme,

    SV_MoveToGoal,
    PF_precache_file,
    PF_makestatic,

    PF_changelevel,
    PF_Fixme,

    PF_cvar_set,
    PF_centerprint,

    PF_ambientsound,

    PF_precache_model,
    PF_precache_sound,	/* precache_sound2 is different only for qcc */
    PF_precache_file,

    PF_setspawnparms,

};

builtin_t *pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
