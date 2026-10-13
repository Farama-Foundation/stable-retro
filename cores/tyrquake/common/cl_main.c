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
/* cl_main.c  -- client main loop */

#include "client.h"
#include "cmd.h"
#include "console.h"
#include "cvar.h"
#include "host.h"
#include "input.h"
#include "model.h"
#include "net.h"
#include "protocol.h"
#include "quakedef.h"
#include "render.h"
#include "screen.h"
#include "server.h"
#include "sound.h"
#include "bgmusic.h"
#include "cdaudio.h"

/* we need to declare some mouse variables here, 
 * because the menu system
 * references them even when on a unix system.
 */

/* these two are not intended to be set directly */
cvar_t cl_name = { "_cl_name", "player", true };
cvar_t cl_color = { "_cl_color", "0", true };

cvar_t cl_shownet = { "cl_shownet", "0" };	/* can be 0, 1, or 2 */
cvar_t cl_nolerp = { "cl_nolerp", "0" };

cvar_t lookspring = { "lookspring", "0", true };
cvar_t lookstrafe = { "lookstrafe", "0", true };
cvar_t sensitivity = { "sensitivity", "3", true };

cvar_t m_pitch = { "m_pitch", "0.022", true };
cvar_t m_yaw = { "m_yaw", "0.022", true };


client_static_t cls;
client_state_t cl;

/* FIXME: put these on hunk? */
efrag_t cl_efrags[MAX_EFRAGS];
entity_t cl_entities[MAX_EDICTS];
entity_t cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t cl_dlights[MAX_DLIGHTS];

int cl_numvisedicts;
entity_t cl_visedicts[MAX_VISEDICTS];

/* Persistent-gib pool.
 *
 * When r_persistgibs is non-zero, the server-side PF_Remove handler
 * intercepts gib edicts (recognized by model name prefix) just before
 * they would normally be freed, and snapshots them into this pool so
 * they remain visible after the QuakeC SUB_Remove timer fires.
 *
 * The captured entries are normal entity_t records; once added via
 * R_AddEfrags they participate in PVS/efrag rendering identically to
 * the BSP's static entities, so no special render path is needed.
 *
 * Pool is bounded; once full, the oldest entry is evicted in FIFO
 * order (ring-buffer style) to keep both memory and worst-case
 * render cost predictable.  A cap of 256 is plenty for typical
 * play -- a Quake monster yields 3 gibs on death, so 256 covers
 * roughly 85 gibbed monsters lying around at once. */
#define MAX_PERSIST_GIBS 256
entity_t cl_persist_gibs[MAX_PERSIST_GIBS];
int      cl_persist_gib_count;     /* slots currently in use, <= MAX_PERSIST_GIBS */
int      cl_persist_gib_next;      /* next slot index for ring eviction */

void
CL_ClearPersistGibs(void)
{
    /* Wipes the pool; called from CL_ClearState on level change /
     * disconnect, and is also safe to call when the cvar is toggled
     * off mid-play to free the existing gibs immediately rather than
     * waiting for the next level. */
    memset(cl_persist_gibs, 0, sizeof(cl_persist_gibs));
    cl_persist_gib_count = 0;
    cl_persist_gib_next  = 0;
}

/*
 * Snapshot a gib edict into the persistent pool.  Called from the
 * server-side PF_Remove just before ED_Free would wipe the entity.
 * Position, angles, model, frame, and skin are copied; physics state
 * is dropped (the captured gib is purely visual).  After capture the
 * entity is registered with the BSP via R_AddEfrags so subsequent
 * frames render it identically to a static entity.
 *
 * Caller has already verified r_persistgibs is on and the model name
 * looks like a gib.  This routine handles model_t lookup and the
 * pool eviction policy.
 */
void
CL_PersistGib(const vec3_t origin, const vec3_t angles,
              const char *model_name, int frame, int skin)
{
    entity_t *ent;
    model_t  *mod;
    int       slot;

    mod = Mod_ForName(model_name, false);
    if (!mod)
	return;     /* gib model not loaded, can't render */

    /* Pick a slot.  Below capacity: append; at capacity: overwrite
     * oldest (ring buffer).  When overwriting we must first detach
     * the old occupant from the efrag chain, otherwise the leaf
     * lists end up pointing at a record we are about to clobber. */
    if (cl_persist_gib_count < MAX_PERSIST_GIBS) {
	slot = cl_persist_gib_count++;
    } else {
	slot = cl_persist_gib_next;
	cl_persist_gib_next = (cl_persist_gib_next + 1) % MAX_PERSIST_GIBS;
	R_RemoveEfrags(&cl_persist_gibs[slot]);
    }

    ent = &cl_persist_gibs[slot];
    memset(ent, 0, sizeof(*ent));
    ent->model    = mod;
    ent->frame    = frame;
    ent->skinnum  = skin;
    ent->colormap = vid.colormap;
    VectorCopy(origin, ent->origin);
    VectorCopy(angles, ent->angles);
    /* Model-lerp fields: snap both halves to the same pose so the
     * renderer never tries to interpolate from a stale frame. */
    ent->currentframe       = frame;
    ent->previousframe      = frame;
    /* Lerp times use countdown semantics: currentX counts elapsed
     * since the latest snapshot, previousX stores the duration of
     * the previous interval. Both start at zero. */
    ent->currentframetime   = 0;
    ent->previousframetime  = 0;
    VectorCopy(origin, ent->currentorigin);
    VectorCopy(origin, ent->previousorigin);
    VectorCopy(angles, ent->currentangles);
    VectorCopy(angles, ent->previousangles);
    ent->currentorigintime  = 0;
    ent->previousorigintime = 0;

    R_AddEfrags(ent);
}

/*
 * FIXME - horribly hackish because we don't have a way to tell if the
 *         entity is a player just by looking at it's properties/pointer.
 *
 * CL_PlayerEntity()
 * Returns the player number if the entity is a player, 0 otherwise
 */
int
CL_PlayerEntity(const entity_t *e)
{
    ptrdiff_t offset;
    int i;

    /* might be a pointer directly into cl_entities... */
    offset =  e - cl_entities;
    if (offset >= 1 && offset <= cl.maxclients)
	return offset;

    /* ...but if not, try to find a match */
    for (i = 1; i <= cl.maxclients; i++) {
	/* Compare just the top of the struct, up to the lerp info */
	if (!memcmp(e, &cl_entities[i], offsetof(entity_t, previouspose)))
	    return i;
    }

    return 0;
}

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState(void)
{
   int i;

   if (!sv.active)
      Host_ClearMemory();

   CL_ClearTEnts();

   /* wipe the entire cl structure */
   memset(&cl, 0, sizeof(cl));

   SZ_Clear(&cls.message);

   /* clear other arrays */
   memset(cl_efrags, 0, sizeof(cl_efrags));
   memset(cl_entities, 0, sizeof(cl_entities));
   memset(cl_dlights, 0, sizeof(cl_dlights));
   memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
   CL_ClearPersistGibs();

   /* allocate the efrags and chain together into a free list */
   cl.free_efrags = cl_efrags;
   for (i = 0; i < MAX_EFRAGS - 1; i++)
      cl.free_efrags[i].entnext = &cl.free_efrags[i + 1];
   cl.free_efrags[i].entnext = NULL;
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect(void)
{
   int i;

   /* stop sounds (especially looping!) */
   S_StopAllSounds(true);
   BGM_Stop();
   CDAudio_Stop();

   /* Clear up view, remove palette shift */
   scr_centertime_off = 0;

   for (i = 0; i < NUM_CSHIFTS; i++)
      cl.cshifts[i].percent = 0;

   VID_SetPalette(host_basepal);

   /* if running a local server, shut it down */
   if (cls.demoplayback)
      CL_StopPlayback();
   else if (cls.state >= ca_connected)
   {
      Con_DPrintf("Sending clc_disconnect\n");
      SZ_Clear(&cls.message);
      MSG_WriteByte(&cls.message, clc_disconnect);
      NET_SendUnreliableMessage(cls.netcon, &cls.message);
      SZ_Clear(&cls.message);
      NET_Close(cls.netcon);

      cls.state = ca_disconnected;
      if (sv.active)
         Host_ShutdownServer(false);
   }

   cls.demoplayback = false;
   cls.timedemo = false;
   cls.signon = 0;
}

void CL_Disconnect_f(void)
{
   CL_Disconnect();
   if (sv.active)
      Host_ShutdownServer(false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection(const char *host)
{
   if (cls.state == ca_dedicated)
      return;

   if (cls.demoplayback)
      return;

   CL_Disconnect();

   cls.netcon = NET_Connect(host);
   if (!cls.netcon)
      Host_Error("CL_Connect: connect failed");

   Con_DPrintf("CL_EstablishConnection: connected to %s\n", host);

   cls.demonum = -1;		/* not in the demo loop now */
   cls.state = ca_connected;
   cls.signon = 0;		/* need all the signon messages before playing */
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply(void)
{
   Con_DPrintf("CL_SignonReply: %i\n", cls.signon);

   switch (cls.signon)
   {
      case 1:
         MSG_WriteByte(&cls.message, clc_stringcmd);
         MSG_WriteString(&cls.message, "prespawn");
         break;

      case 2:
         MSG_WriteByte(&cls.message, clc_stringcmd);
         MSG_WriteStringf(&cls.message, "name \"%s\"\n", cl_name.string);

         MSG_WriteByte(&cls.message, clc_stringcmd);
         MSG_WriteStringf(&cls.message, "color %i %i\n",
               ((int)cl_color.value) >> 4,
               ((int)cl_color.value) & 15);

         MSG_WriteByte(&cls.message, clc_stringcmd);
         MSG_WriteStringf(&cls.message, "spawn %s", cls.spawnparms);
         break;

      case 3:
         MSG_WriteByte(&cls.message, clc_stringcmd);
         MSG_WriteString(&cls.message, "begin");
         Cache_Report();		/* print remaining memory */

         /* FIXME - this the right place for it? */
         cls.state = ca_firstupdate;
         break;

      case 4:
         SCR_EndLoadingPlaque();	/* allow normal screen updates */

         /* FIXME - this the right place for it? */
         cls.state = ca_active;
         break;
   }
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo(void)
{
   char str[1024];

   if (cls.demonum == -1)
      return;			/* don't play demos */

   SCR_BeginLoadingPlaque();

   if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
   {
      cls.demonum = 0;
      if (!cls.demos[cls.demonum][0])
      {
         Con_Printf("No demos listed with startdemos\n");
         cls.demonum = -1;
         return;
      }
   }

   snprintf(str, sizeof(str), "playdemo %s\n", cls.demos[cls.demonum]);
   Cbuf_InsertText(str);
   cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
static void CL_PrintEntities_f(void)
{
   const entity_t *ent;
   int i;

   for (i = 0, ent = cl_entities; i < cl.num_entities; i++, ent++)
   {
      Con_Printf("%3i:", i);
      if (!ent->model)
      {
         Con_Printf("EMPTY\n");
         continue;
      }
      Con_Printf("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n",
            ent->model->name, ent->frame, ent->origin[0],
            ent->origin[1], ent->origin[2], ent->angles[0],
            ent->angles[1], ent->angles[2]);
   }
}


float dl_colors[4][4] = {
    { 0.2, 0.1, 0.05, 0.7 },	/* FLASH */
    { 0.05, 0.05, 0.3, 0.7 },	/* BLUE */
    { 0.5, 0.05, 0.05, 0.7 },	/* RED */
    { 0.5, 0.05, 0.4, 0.7 }	/* PURPLE */
};

/*
===============
CL_AllocDlight

===============
*/
dlight_t *
CL_AllocDlight(int key)
{
   int i;
   dlight_t *dl;

   /* first look for an exact key match */
   if (key)
   {
      dl = cl_dlights;
      for (i = 0; i < MAX_DLIGHTS; i++, dl++)
      {
         if (dl->key == key)
         {
            memset(dl, 0, sizeof(*dl));
            dl->color = dl_colors[DLIGHT_FLASH];
            dl->key = key;
            return dl;
         }
      }
   }

   /* then look for anything else */
   dl = cl_dlights;
   for (i = 0; i < MAX_DLIGHTS; i++, dl++)
   {
      if (dl->die <= 0)
      {
         memset(dl, 0, sizeof(*dl));
         dl->color = dl_colors[DLIGHT_FLASH];
         dl->key = key;
         return dl;
      }
   }

   dl = &cl_dlights[0];
   memset(dl, 0, sizeof(*dl));
   dl->color = dl_colors[DLIGHT_FLASH];
   dl->key = key;
   return dl;
}


/*
===============
CL_DecayLights

===============
*/
void
CL_DecayLights(void)
{
   int i;
   dlight_t *dl;
   float time;

   time = cl.time - cl.oldtime;

   dl = cl_dlights;
   for (i = 0; i < MAX_DLIGHTS; i++, dl++)
   {
      if (dl->die <= 0 || !dl->radius)
         continue;

      dl->radius -= time * dl->decay;
      if (dl->radius < 0)
         dl->radius = 0;
      dl->die    -= time;
   }
}


/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
static float CL_LerpPoint(void)
{
   float frac;
   float f = cl.mtime[0] - cl.mtime[1];

   if (!f || cl_nolerp.value || cls.timedemo || sv.active)
   {
      cl.time = cl.mtime[0];
      return 1;
   }

   if (f > 0.1)
   {
      /* dropped packet, or start of demo */
      cl.mtime[1] = cl.mtime[0] - 0.1;
      f = 0.1;
   }

   frac = (cl.time - cl.mtime[1]) / f;

   if (frac < 0)
   {
      if (frac < -0.01) /* low frac */
         cl.time = cl.mtime[1];
      frac = 0;
   }
   else if (frac > 1)
   {
      if (frac > 1.01) /* high frac */
         cl.time = cl.mtime[0];
      frac = 1;
   }

   return frac;
}


/*
===============
CL_RelinkEntities
===============
*/
static void CL_RelinkEntities(void)
{
   entity_t *ent;
   int i, j;
   float frac, f, d;
   vec3_t delta;
   float bobjrotate;
   vec3_t oldorg;
   dlight_t *dl;

   /* determine partial update time */
   frac = CL_LerpPoint();

   cl_numvisedicts = 0;

   /* interpolate player info */
   for (i = 0; i < 3; i++)
      cl.velocity[i] = cl.mvelocity[1][i] +
         frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

   if (cls.demoplayback)
   {
      /* interpolate the angles */
      for (j = 0; j < 3; j++)
      {
         d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
         if (d > 180)
            d -= 360;
         else if (d < -180)
            d += 360;
         cl.viewangles[j] = cl.mviewangles[1][j] + frac * d;
      }
   }

   bobjrotate = anglemod(100 * cl.time);

   /* start on the entity after the world */
   for (i = 1, ent = cl_entities + 1; i < cl.num_entities; i++, ent++)
   {
      if (!ent->model)
      {	/* empty slot */
         if (ent->forcelink)
            R_RemoveEfrags(ent);	/* just became empty */
         continue;
      }

      /* if the object wasn't included in the last packet, remove it */
      if (ent->msgtime != cl.mtime[0])
      {
         ent->model = NULL;
         /* Reset lerp info as well */
         ent->previousframe = 0;
         ent->currentframe = 0;
         ent->previousorigintime = 0;
         ent->currentorigintime = 0;
         ent->previousanglestime = 0;
         ent->currentanglestime = 0;
         continue;
      }

      /* Age the countdown lerp timers for this active entity. The
       * inactive cleanup above already zeroes the time fields for
       * empty/dead slots, so we only increment slots that have a
       * live model and were in the latest server packet. */
      ent->currentframetime  += host_frametime;
      ent->currentorigintime += host_frametime;
      ent->currentanglestime += host_frametime;

      VectorCopy(ent->origin, oldorg);

      if (ent->forcelink)
      {
         /*
          * the entity was not updated in the last message
          * so move to the final spot
          */
         VectorCopy(ent->msg_origins[0], ent->origin);
         VectorCopy(ent->msg_angles[0], ent->angles);
      }
      else
      {
         f = frac;
         for (j = 0; j < 3; j++)
         {
            delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
            if (delta[j] > 100 || delta[j] < -100)
               f = 1;	/* assume a teleport and don't lerp */
         }

         /*
          * interpolate the origin and angles
          */
         for (j = 0; j < 3; j++)
         {
            ent->origin[j] = ent->msg_origins[1][j] + f * delta[j];
            d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
            if (d > 180)
               d -= 360;
            else if (d < -180)
               d += 360;
            ent->angles[j] = ent->msg_angles[1][j] + f * d;
         }
      }

      /* rotate binary objects locally */
      if (ent->model->flags & EF_ROTATE)
         ent->angles[1] = bobjrotate;

      /*
       * FIXME - Some of these entity effects may be mutually exclusive?
       * work out which bits can be done better (e.g. I've already done the
       * RED|BLUE bit a little better...)
       */
      if (ent->effects & EF_BRIGHTFIELD)
         R_EntityParticles(ent);

      if (ent->effects & EF_MUZZLEFLASH)
      {
         vec3_t fv, rv, uv;

         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->origin[2] += 16;
         AngleVectors(ent->angles, fv, rv, uv);

         VectorMA(dl->origin, 18, fv, dl->origin);
         dl->radius = 200 + (rand() & 31);
         dl->minlight = 32;
         dl->die = 0.1;
         dl->color = dl_colors[DLIGHT_FLASH];
      }

      if (ent->effects & EF_BRIGHTLIGHT)
      {
         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->origin[2] += 16;
         dl->radius = 400 + (rand() & 31);
         dl->die = 0.001;
         dl->color = dl_colors[DLIGHT_FLASH];
      }

      if (ent->effects & EF_DIMLIGHT)
      {
         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->radius = 200 + (rand() & 31);
         dl->die = 0.001;
         dl->color = dl_colors[DLIGHT_FLASH];
      }

      if ((ent->effects & (EF_RED | EF_BLUE)) == (EF_RED | EF_BLUE))
      {
         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->radius = 200 + (rand() & 31);
         dl->die = 0.001;
         dl->color = dl_colors[DLIGHT_PURPLE];
      } else if (ent->effects & EF_BLUE) {
         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->radius = 200 + (rand() & 31);
         dl->die = 0.001;
         dl->color = dl_colors[DLIGHT_BLUE];
      } else if (ent->effects & EF_RED) {
         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->radius = 200 + (rand() & 31);
         dl->die = 0.001;
         dl->color = dl_colors[DLIGHT_RED];
      }

      if (ent->model->flags & EF_GIB)
         R_RocketTrail(oldorg, ent->origin, 2);
      else if (ent->model->flags & EF_ZOMGIB)
         R_RocketTrail(oldorg, ent->origin, 4);
      else if (ent->model->flags & EF_TRACER)
         R_RocketTrail(oldorg, ent->origin, 3);
      else if (ent->model->flags & EF_TRACER2)
         R_RocketTrail(oldorg, ent->origin, 5);
      else if (ent->model->flags & EF_ROCKET) {
         R_RocketTrail(oldorg, ent->origin, 0);
         dl = CL_AllocDlight(i);
         VectorCopy(ent->origin, dl->origin);
         dl->radius = 200;
         dl->die = 0.01;
      } else if (ent->model->flags & EF_GRENADE)
         R_RocketTrail(oldorg, ent->origin, 1);
      else if (ent->model->flags & EF_TRACER3)
         R_RocketTrail(oldorg, ent->origin, 6);

      ent->forcelink = false;

      if (i == cl.viewentity && !chase_active.value)
         continue;

      if (cl_numvisedicts < MAX_VISEDICTS)
      {
         cl_visedicts[cl_numvisedicts] = *ent;
         cl_numvisedicts++;
      }
   }
}


/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer(void)
{
   int ret;

   cl.oldtime = cl.time;
   cl.time += host_frametime;

   /* Age client-side countdown timers in lockstep with cl.time so
    * their fp32 storage stays bounded (was previously a cl.time or
    * realtime snapshot, which loses precision once those grow):
    *   - faceanimtime: down-counter, remaining sbar pain-frame
    *     duration. Goes negative once expired; readers check > 0.
    *   - last_received_message: up-counter, elapsed since the last
    *     server message arrived. Reset to 0 on receive (below).
    *   - scr_centertime_start: up-counter, elapsed since the last
    *     SCR_CenterPrint. Reset to 0 by SCR_CenterPrint. */
   cl.faceanimtime          -= host_frametime;
   cl.last_received_message += host_frametime;
   scr_centertime_start     += host_frametime;
   {
      /* item_gettime[j] used to store cl.time at pickup; under the
       * countdown it stores elapsed-since-pickup, set to 0 by the
       * STAT_ITEMS-change handler in CL_ParseUpdate. Slots for items
       * the player never owned grow unbounded but are gated out by
       * the cl.stats[STAT_ITEMS] ownership test at every sbar read
       * site, so no sentinel is needed. */
      int j;
      for (j = 0; j < 32; j++)
         cl.item_gettime[j] += host_frametime;
   }

   do {
      ret = CL_GetMessage();
      if (ret == -1)
         Host_Error("CL_ReadFromServer: lost server connection");
      if (!ret)
         break;

      cl.last_received_message = 0;
      CL_ParseServerMessage();
   } while (ret && cls.state >= ca_connected);

   if (cl_shownet.value)
      Con_Printf("\n");

   CL_RelinkEntities();
   CL_UpdateTEnts();

   /* bring the links up to date */
   return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd(void)
{
   usercmd_t cmd;

   if (cls.state < ca_connected)
      return;

   if (cls.state == ca_active)
   {
      /* get basic movement from keyboard */
      CL_BaseMove(&cmd);

      /* allow mice or other external controllers to add to the move */
      IN_Move(&cmd);

      /* send the unreliable message */
      CL_SendMove(&cmd);
   }

   if (cls.demoplayback)
   {
      SZ_Clear(&cls.message);
      return;
   }

   /* send the reliable message */
   if (!cls.message.cursize)
      return;			/* no message at all */

   if (!NET_CanSendMessage(cls.netcon))
   {
      Con_DPrintf("CL_WriteToServer: can't send\n");
      return;
   }

   if (NET_SendMessage(cls.netcon, &cls.message) == -1)
      Host_Error("CL_WriteToServer: lost server connection");

   SZ_Clear(&cls.message);
}

/*
=================
CL_Init
=================
*/
void CL_Init(void)
{
   SZ_Alloc(&cls.message, 1024);

   CL_InitInput();
   CL_InitTEnts();

   /* register our commands */
   Cvar_RegisterVariable(&cl_name);
   Cvar_RegisterVariable(&cl_color);
   Cvar_RegisterVariable(&cl_upspeed);
   Cvar_RegisterVariable(&cl_forwardspeed);
   Cvar_RegisterVariable(&cl_backspeed);
   Cvar_RegisterVariable(&cl_sidespeed);
   Cvar_RegisterVariable(&cl_movespeedkey);
   Cvar_RegisterVariable(&cl_yawspeed);
   Cvar_RegisterVariable(&cl_pitchspeed);
   Cvar_RegisterVariable(&cl_anglespeedkey);
   Cvar_RegisterVariable(&cl_shownet);
   Cvar_RegisterVariable(&cl_nolerp);
   Cvar_RegisterVariable(&lookspring);
   Cvar_RegisterVariable(&lookstrafe);
   Cvar_RegisterVariable(&sensitivity);

   Cvar_RegisterVariable(&m_pitch);
   Cvar_RegisterVariable(&m_yaw);

   Cmd_AddCommand("entities", CL_PrintEntities_f);
   Cmd_AddCommand("disconnect", CL_Disconnect_f);
   Cmd_AddCommand("playdemo", CL_PlayDemo_f);
   Cmd_SetCompletion("playdemo", CL_Demo_Arg_f);
   Cmd_AddCommand("timedemo", CL_TimeDemo_f);
   Cmd_SetCompletion("timedemo", CL_Demo_Arg_f);

   Cmd_AddCommand("mcache", Mod_Print);
}
