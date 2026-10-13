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
/* cl_tent.c -- client side temporary entities */

#include "client.h"
#include "console.h"
#include "host.h"
#include "model.h"
#include "protocol.h"
#include "quakedef.h"
#include "sound.h"
#include "sys.h"

#define	MAX_BEAMS	24
typedef struct
{
    int entity;
    struct model_s *model;
    float endtime;		/* (countdown) remaining lifetime in seconds; expires at <= 0 */
    vec3_t start, end;
} beam_t;

static beam_t cl_beams[MAX_BEAMS];

static sfx_t *cl_sfx_wizhit;
static sfx_t *cl_sfx_knighthit;
static sfx_t *cl_sfx_tink1;
static sfx_t *cl_sfx_ric1;
static sfx_t *cl_sfx_ric2;
static sfx_t *cl_sfx_ric3;
static sfx_t *cl_sfx_r_exp3;

/*
=================
CL_InitTEnts
=================
*/
void
CL_InitTEnts(void)
{
    cl_sfx_wizhit = S_PrecacheSound("wizard/hit.wav");
    cl_sfx_knighthit = S_PrecacheSound("hknight/hit.wav");
    cl_sfx_tink1 = S_PrecacheSound("weapons/tink1.wav");
    cl_sfx_ric1 = S_PrecacheSound("weapons/ric1.wav");
    cl_sfx_ric2 = S_PrecacheSound("weapons/ric2.wav");
    cl_sfx_ric3 = S_PrecacheSound("weapons/ric3.wav");
    cl_sfx_r_exp3 = S_PrecacheSound("weapons/r_exp3.wav");
}

/*
=================
CL_ClearTEnts
=================
*/
void
CL_ClearTEnts(void)
{
    memset(&cl_beams, 0, sizeof(cl_beams));
}

/*
=================
CL_ParseBeam
=================
*/
static void
CL_ParseBeam(model_t *m)
{
   vec3_t start, end;
   beam_t *b;
   int i;
   int ent = MSG_ReadShort();

   start[0] = MSG_ReadCoord();
   start[1] = MSG_ReadCoord();
   start[2] = MSG_ReadCoord();

   end[0] = MSG_ReadCoord();
   end[1] = MSG_ReadCoord();
   end[2] = MSG_ReadCoord();

   /* override any beam with the same entity */
   for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
      if (b->entity == ent)
      {
         b->entity = ent;
         b->model = m;
         b->endtime = 0.2;
         VectorCopy(start, b->start);
         VectorCopy(end, b->end);
         return;
      }

   /* find a free beam */
   for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
   {
      if (!b->model || b->endtime <= 0)
      {
         b->entity = ent;
         b->model = m;
         b->endtime = 0.2;
         VectorCopy(start, b->start);
         VectorCopy(end, b->end);
         return;
      }
   }
   Con_Printf("beam list overflow!\n");
}

/*
=================
CL_ParseTEnt
=================
*/
void
CL_ParseTEnt(void)
{
   int type;
   vec3_t pos;

   dlight_t *dl;
   int rnd;
   int colorStart, colorLength;

   type = MSG_ReadByte();
   switch (type)
   {
      case TE_WIZSPIKE:		/* spike hitting wall */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_RunParticleEffect(pos, vec3_origin, 20, 30);
         S_StartSound(-1, 0, cl_sfx_wizhit, pos, 1, 1);
         break;

      case TE_KNIGHTSPIKE:	/* spike hitting wall */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_RunParticleEffect(pos, vec3_origin, 226, 20);
         S_StartSound(-1, 0, cl_sfx_knighthit, pos, 1, 1);
         break;

      case TE_SPIKE:		/* spike hitting wall */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();

         R_RunParticleEffect(pos, vec3_origin, 0, 10);

         if (rand() % 5)
            S_StartSound(-1, 0, cl_sfx_tink1, pos, 1, 1);
         else {
            rnd = rand() & 3;
            if (rnd == 1)
               S_StartSound(-1, 0, cl_sfx_ric1, pos, 1, 1);
            else if (rnd == 2)
               S_StartSound(-1, 0, cl_sfx_ric2, pos, 1, 1);
            else
               S_StartSound(-1, 0, cl_sfx_ric3, pos, 1, 1);
         }
         break;
      case TE_SUPERSPIKE:	/* super spike hitting wall */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_RunParticleEffect(pos, vec3_origin, 0, 20);

         if (rand() % 5)
            S_StartSound(-1, 0, cl_sfx_tink1, pos, 1, 1);
         else {
            rnd = rand() & 3;
            if (rnd == 1)
               S_StartSound(-1, 0, cl_sfx_ric1, pos, 1, 1);
            else if (rnd == 2)
               S_StartSound(-1, 0, cl_sfx_ric2, pos, 1, 1);
            else
               S_StartSound(-1, 0, cl_sfx_ric3, pos, 1, 1);
         }
         break;

      case TE_GUNSHOT:		/* bullet hitting wall */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_RunParticleEffect(pos, vec3_origin, 0, 20);
         break;

      case TE_EXPLOSION:		/* rocket explosion */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_ParticleExplosion(pos);
         dl = CL_AllocDlight(0);
         VectorCopy(pos, dl->origin);
         dl->radius = 350;
         dl->die = 0.5;
         dl->decay = 300;
         S_StartSound(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
         break;

      case TE_TAREXPLOSION:	/* tarbaby explosion */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_BlobExplosion(pos);

         S_StartSound(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
         break;

      case TE_LIGHTNING1:	/* lightning bolts */
         CL_ParseBeam(Mod_ForName("progs/bolt.mdl", true));
         break;

      case TE_LIGHTNING2:	/* lightning bolts */
         CL_ParseBeam(Mod_ForName("progs/bolt2.mdl", true));
         break;

      case TE_LIGHTNING3:	/* lightning bolts */
         CL_ParseBeam(Mod_ForName("progs/bolt3.mdl", true));
         break;

         /* PGM 01/21/97 */
      case TE_BEAM:		/* grappling hook beam */
         CL_ParseBeam(Mod_ForName("progs/beam.mdl", true));
         break;
         /* PGM 01/21/97 */

      case TE_LAVASPLASH:
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_LavaSplash(pos);
         break;

      case TE_TELEPORT:
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         R_TeleportSplash(pos);
         break;

      case TE_EXPLOSION2:	/* color mapped explosion */
         pos[0] = MSG_ReadCoord();
         pos[1] = MSG_ReadCoord();
         pos[2] = MSG_ReadCoord();
         colorStart = MSG_ReadByte();
         colorLength = MSG_ReadByte();
         R_ParticleExplosion2(pos, colorStart, colorLength);
         dl = CL_AllocDlight(0);
         VectorCopy(pos, dl->origin);
         dl->radius = 350;
         dl->die = 0.5;
         dl->decay = 300;
         S_StartSound(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
         break;

      default:
         Sys_Error("%s: bad type", __func__);
   }
}


/*
=================
CL_NewTempEntity
=================
*/
static entity_t *CL_NewTempEntity(void)
{
   entity_t *ent;

   if (cl_numvisedicts == MAX_VISEDICTS)
      return NULL;

   ent = &cl_visedicts[cl_numvisedicts];
   cl_numvisedicts++;

   memset(ent, 0, sizeof(*ent));

   ent->colormap = vid.colormap;
   return ent;
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts(void)
{
   int i;
   beam_t *b;
   vec3_t dist, org;
   float d;
   entity_t *ent;
   float yaw, pitch;
   float forward;

   /* update lightning */
   for (i = 0, b = cl_beams; i < MAX_BEAMS; i++, b++)
   {
      if (!b->model || b->endtime <= 0)
         continue;

      /* if coming from the player, update the start position */
      if (b->entity == cl.viewentity)
      {
         VectorCopy(cl_entities[cl.viewentity].origin, b->start);
      }

      /* calculate pitch and yaw */
      VectorSubtract(b->end, b->start, dist);

      if (dist[1] == 0 && dist[0] == 0)
      {
         yaw = 0;
         if (dist[2] > 0)
            pitch = 90;
         else
            pitch = 270;
      }
      else
      {
         yaw = (int)(atan2f(dist[1], dist[0]) * 180 / M_PI);
         if (yaw < 0)
            yaw += 360;

         forward = sqrtf(dist[0] * dist[0] + dist[1] * dist[1]);
         pitch = (int)(atan2f(dist[2], forward) * 180 / M_PI);
         if (pitch < 0)
            pitch += 360;
      }

      /* add new entities for the lightning */
      VectorCopy(b->start, org);
      d = VectorNormalize(dist);
      while (d > 0)
      {
         ent = CL_NewTempEntity();
         if (!ent)
            return;

         VectorCopy(org, ent->origin);
         ent->model = b->model;
         ent->angles[0] = pitch;
         ent->angles[1] = yaw;
         ent->angles[2] = rand() % 360;

         VectorMA(org, 30, dist, org);
         d -= 30;

         /* Initialize model lerp info */
         ent->frame = 0;
         ent->currentframe = 0;
         ent->previousframe = 0;
         /* Lerp times use countdown semantics (see render.h). */
         ent->currentframetime = 0;
         ent->previousframetime = 0;
         VectorCopy(ent->origin, ent->currentorigin);
         VectorCopy(ent->origin, ent->previousorigin);
         ent->currentorigintime = 0;
         ent->previousorigintime = 0;
      }

      /* Age the beam after using it this frame so the per-frame
       * progression matches the original (cl.time vs absolute
       * b->endtime) timeline exactly. */
      b->endtime -= host_frametime;
   }
}
