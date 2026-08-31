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

#include "console.h"
#include "model.h"
#include "quakedef.h"
#include "server.h"

#include "d_iface.h"
#include "r_local.h"
#include "rhi.h"        /* g_rhi / g_rhi_compute_rendering -- when
                         * the active backend exposes a GPU
                         * particle dispatch and compute rendering
                         * is enabled, R_DrawParticles routes
                         * through it and skips the CPU SW
                         * D_DrawParticle loop (Phase 5b-02). */

#define MAX_PARTICLES		2048	/* default max # of particles at one */
					/*  time */
#define ABSOLUTE_MIN_PARTICLES	512	/* no fewer than this no matter what's */
					/*  on the command line */

int ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
int ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
int ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };

particle_t *active_particles, *free_particles;

particle_t *particles;
int r_numparticles;

vec3_t r_pright, r_pup, r_ppn;


/*
===============
R_ValidParticle

Returns true iff p points to a slot inside the particles array.
The free and active particle lists are normally well-formed, but
heap or list corruption from an unrelated subsystem can leave
free_particles or some node's next pointer aimed at memory
outside the array.  Without validation, the first pop of the
free list (p = free_particles; free_particles = p->next;
p->next = active_particles;) crashes when the bogus p is
written to.

Used as a gate at every list-pop site.  Cheap (two pointer
compares) so safe to call on every iteration.  When the gate
trips, the caller bails out of the current particle effect --
visually equivalent to "no free particles" (a condition the
existing code already handles), so no user-visible artefact
beyond the missing effect for that frame.
===============
*/
static int
R_ValidParticle(const particle_t *p)
{
   const char *base = (const char *)particles;
   const char *end  = base + (size_t)r_numparticles * sizeof(particle_t);
   const char *q    = (const char *)p;
   if (!p)
      return 0;
   if (q < base || q >= end)
      return 0;
   /* Slot must be aligned on a particle_t boundary. */
   if (((size_t)(q - base) % sizeof(particle_t)) != 0)
      return 0;
   return 1;
}


/*
===============
R_InitParticles
===============
*/
void R_InitParticles(void)
{
   int i = COM_CheckParm("-particles");

   if (i)
   {
      r_numparticles = (int)(Q_atoi(com_argv[i + 1]));
      if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
         r_numparticles = ABSOLUTE_MIN_PARTICLES;
      /* r_numparticles * sizeof(particle_t) is allocated below and
       * also walked by R_ValidParticle (f744f3a) to validate
       * pointer ranges on every particle reference.  Cap well
       * below INT_MAX / sizeof(particle_t) so neither the
       * multiplication here nor the validator's range check
       * can overflow.  16 million is far above any legitimate
       * use of -particles. */
      if (r_numparticles > 16 * 1024 * 1024)
         r_numparticles = 16 * 1024 * 1024;
   }
   else
      r_numparticles = MAX_PARTICLES;

   particles = (particle_t *)
      Hunk_Alloc(r_numparticles * sizeof(particle_t));
}

/*
===============
R_EntityParticles
===============
*/

#define NUMVERTEXNORMALS	162
vec3_t avelocities[NUMVERTEXNORMALS];
float beamlength = 16;
vec3_t avelocity = { 23, 7, 3 };
float partstep = 0.01;
float timescale = 0.01;

void R_EntityParticles(const entity_t *ent)
{
   int i, j;
   particle_t *p;
   float angle;
   float sp, sy, cp, cy;
   vec3_t forward;
   float dist = 64;

   if (!avelocities[0][0])
   {
      for (i = 0; i < NUMVERTEXNORMALS; i++)
         for (j = 0; j < 3; j++)
            avelocities[i][j] = (rand() & 255) * 0.01;
   }

   for (i = 0; i < NUMVERTEXNORMALS; i++) {
      angle = cl.time * avelocities[i][0];
      sy = sinf(angle);
      cy = cosf(angle);
      angle = cl.time * avelocities[i][1];
      sp = sinf(angle);
      cp = cosf(angle);

      forward[0] = cp * cy;
      forward[1] = cp * sy;
      forward[2] = -sp;

      if (!R_ValidParticle(free_particles))
         return;
      p = free_particles;
      free_particles = p->next;
      p->next = active_particles;
      active_particles = p;

      p->die = 0.01;
      p->color = 0x6f;
      p->type = pt_explode;

      p->org[0] =
         ent->origin[0] + r_avertexnormals[i][0] * dist +
         forward[0] * beamlength;
      p->org[1] =
         ent->origin[1] + r_avertexnormals[i][1] * dist +
         forward[1] * beamlength;
      p->org[2] =
         ent->origin[2] + r_avertexnormals[i][2] * dist +
         forward[2] * beamlength;
   }
}

/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles(void)
{
   int i;

   free_particles = &particles[0];
   active_particles = NULL;

   for (i = 0; i < r_numparticles; i++)
      particles[i].next = &particles[i + 1];
   particles[r_numparticles - 1].next = NULL;
}


/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect(void)
{
   vec3_t org, dir;
   int i, count, msgcount, color;

   for (i = 0; i < 3; i++)
      org[i] = MSG_ReadCoord();
   for (i = 0; i < 3; i++)
      dir[i] = MSG_ReadChar() * (1.0 / 16);
   msgcount = MSG_ReadByte();
   color = MSG_ReadByte();

   if (msgcount == 255)
      count = 1024;
   else
      count = msgcount;

   R_RunParticleEffect(org, dir, color, count);
}

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion(vec3_t org)
{
   int i, j;
   particle_t *p;

   for (i = 0; i < 1024; i++) {
      if (!R_ValidParticle(free_particles))
         return;
      p = free_particles;
      free_particles = p->next;
      p->next = active_particles;
      active_particles = p;

      p->die = 5;
      p->color = ramp1[0];
      p->ramp = rand() & 3;
      if (i & 1) {
         p->type = pt_explode;
         for (j = 0; j < 3; j++) {
            p->org[j] = org[j] + ((rand() % 32) - 16);
            p->vel[j] = (rand() % 512) - 256;
         }
      } else {
         p->type = pt_explode2;
         for (j = 0; j < 3; j++) {
            p->org[j] = org[j] + ((rand() % 32) - 16);
            p->vel[j] = (rand() % 512) - 256;
         }
      }
   }
}

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2(vec3_t org, int colorStart, int colorLength)
{
   int i, j;
   particle_t *p;
   int colorMod = 0;

   /* colorStart and colorLength come from MSG_ReadByte at
    * the TE_EXPLOSION2 caller (range [-1, 255]).
    * colorLength == 0 makes (colorMod % colorLength)
    * divide by zero (UB; SIGFPE on x86); colorLength < 0
    * yields an implementation-defined modulo result that
    * may be negative or zero, both of which would put
    * p->color out of palette range.  Reject up front; a
    * malformed packet just produces a less colorful
    * explosion. */
   if (colorLength <= 0)
      return;

   for (i = 0; i < 512; i++) {
      if (!R_ValidParticle(free_particles))
         return;
      p = free_particles;
      free_particles = p->next;
      p->next = active_particles;
      active_particles = p;

      p->die = 0.3;
      p->color = colorStart + (colorMod % colorLength);
      colorMod++;

      p->type = pt_blob;
      for (j = 0; j < 3; j++) {
         p->org[j] = org[j] + ((rand() % 32) - 16);
         p->vel[j] = (rand() % 512) - 256;
      }
   }
}

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion(vec3_t org)
{
   int i, j;
   particle_t *p;

   for (i = 0; i < 1024; i++) {
      if (!R_ValidParticle(free_particles))
         return;
      p = free_particles;
      free_particles = p->next;
      p->next = active_particles;
      active_particles = p;

      p->die = 1 + (rand() & 8) * 0.05;

      if (i & 1) {
         p->type = pt_blob;
         p->color = 66 + rand() % 6;
         for (j = 0; j < 3; j++) {
            p->org[j] = org[j] + ((rand() % 32) - 16);
            p->vel[j] = (rand() % 512) - 256;
         }
      } else {
         p->type = pt_blob2;
         p->color = 150 + rand() % 6;
         for (j = 0; j < 3; j++) {
            p->org[j] = org[j] + ((rand() % 32) - 16);
            p->vel[j] = (rand() % 512) - 256;
         }
      }
   }
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count)
{
   int i, j;
   particle_t *p;

   for (i = 0; i < count; i++) {
      if (!R_ValidParticle(free_particles))
         return;
      p = free_particles;
      free_particles = p->next;
      p->next = active_particles;
      active_particles = p;

      if (count == 1024) {	/* rocket explosion */
         p->die = 5;
         p->color = ramp1[0];
         p->ramp = rand() & 3;
         if (i & 1) {
            p->type = pt_explode;
            for (j = 0; j < 3; j++) {
               p->org[j] = org[j] + ((rand() % 32) - 16);
               p->vel[j] = (rand() % 512) - 256;
            }
         } else {
            p->type = pt_explode2;
            for (j = 0; j < 3; j++) {
               p->org[j] = org[j] + ((rand() % 32) - 16);
               p->vel[j] = (rand() % 512) - 256;
            }
         }
      } else {
         p->die = 0.1 * (rand() % 5);
         p->color = (color & ~7) + (rand() & 7);
         p->type = pt_slowgrav;
         for (j = 0; j < 3; j++) {
            p->org[j] = org[j] + ((rand() & 15) - 8);
            p->vel[j] = dir[j] * 15;	/* + (rand()%300)-150; */
         }
      }
   }
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash(vec3_t org)
{
   int i, j, k;
   particle_t *p;
   float vel;
   vec3_t dir;

   for (i = -16; i < 16; i++)
      for (j = -16; j < 16; j++)
         for (k = 0; k < 1; k++)
         {
            if (!R_ValidParticle(free_particles))
               return;
            p = free_particles;
            free_particles = p->next;
            p->next = active_particles;
            active_particles = p;

            p->die = 2 + (rand() & 31) * 0.02;
            p->color = 224 + (rand() & 7);
            p->type = pt_grav;

            dir[0] = j * 8 + (rand() & 7);
            dir[1] = i * 8 + (rand() & 7);
            dir[2] = 256;

            p->org[0] = org[0] + dir[0];
            p->org[1] = org[1] + dir[1];
            p->org[2] = org[2] + (rand() & 63);

            VectorNormalize(dir);
            vel = 50 + (rand() & 63);
            VectorScale(dir, vel, p->vel);
         }
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash(vec3_t org)
{
   int i, j, k;
   particle_t *p;
   float vel;
   vec3_t dir;

   for (i = -16; i < 16; i += 4)
      for (j = -16; j < 16; j += 4)
         for (k = -24; k < 32; k += 4)
         {
            if (!R_ValidParticle(free_particles))
               return;
            p = free_particles;
            free_particles = p->next;
            p->next = active_particles;
            active_particles = p;

            p->die = 0.2 + (rand() & 7) * 0.02;
            p->color = 7 + (rand() & 7);
            p->type = pt_grav;

            dir[0] = j * 8;
            dir[1] = i * 8;
            dir[2] = k * 8;

            p->org[0] = org[0] + i + (rand() & 3);
            p->org[1] = org[1] + j + (rand() & 3);
            p->org[2] = org[2] + k + (rand() & 3);

            VectorNormalize(dir);
            vel = 50 + (rand() & 63);
            VectorScale(dir, vel, p->vel);
         }
}

void R_RocketTrail(vec3_t start, vec3_t end, int type)
{
   static int tracercount;
   vec3_t vec;
   float len;
   int j;
   particle_t *p;
   int dec;

   VectorSubtract(end, start, vec);
   len = VectorNormalize(vec);
   if (type < 128)
      dec = 3;
   else {
      dec = 1;
      type -= 128;
   }

   while (len > 0) {
      len -= dec;
      if (!R_ValidParticle(free_particles))
         return;
      p = free_particles;
      free_particles = p->next;
      p->next = active_particles;
      active_particles = p;

      VectorCopy(vec3_origin, p->vel);
      p->die = 2;

      switch (type) {
         case 0:		/* rocket trail */
            p->ramp = (rand() & 3);
            p->color = ramp3[(int)p->ramp];
            p->type = pt_fire;
            for (j = 0; j < 3; j++)
               p->org[j] = start[j] + ((rand() % 6) - 3);
            break;

         case 1:		/* smoke smoke */
            p->ramp = (rand() & 3) + 2;
            p->color = ramp3[(int)p->ramp];
            p->type = pt_fire;
            for (j = 0; j < 3; j++)
               p->org[j] = start[j] + ((rand() % 6) - 3);
            break;

         case 2:		/* blood */
            p->type = pt_grav;
            p->color = 67 + (rand() & 3);
            for (j = 0; j < 3; j++)
               p->org[j] = start[j] + ((rand() % 6) - 3);
            break;

         case 3:
         case 5:		/* tracer */
            p->die = 0.5;
            p->type = pt_static;
            if (type == 3)
               p->color = 52 + ((tracercount & 4) << 1);
            else
               p->color = 230 + ((tracercount & 4) << 1);

            tracercount++;
            VectorCopy(start, p->org);
            if (tracercount & 1) {
               p->vel[0] = 30 * vec[1];
               p->vel[1] = 30 * -vec[0];
            } else {
               p->vel[0] = 30 * -vec[1];
               p->vel[1] = 30 * vec[0];
            }
            break;

         case 4:		/* slight blood */
            p->type = pt_grav;
            p->color = 67 + (rand() & 3);
            for (j = 0; j < 3; j++)
               p->org[j] = start[j] + ((rand() % 6) - 3);
            len -= 3;
            break;

         case 6:		/* voor trail */
            p->color = 9 * 16 + 8 + (rand() & 3);
            p->type = pt_static;
            p->die = 0.3;
            for (j = 0; j < 3; j++)
               p->org[j] = start[j] + ((rand() & 15) - 8);
            break;
      }

      VectorAdd(start, vec, start);
   }
}

/*
===============
CL_RunParticles
===============
*/
void CL_RunParticles(void)
{
   particle_t *p, *kill;
   int i;
   float frametime = cl.time - cl.oldtime;
   float grav      = frametime * sv_gravity.value * 0.05;
   float time3     = frametime * 15;
   float time2     = frametime * 10;	/* 15; */
   float time1     = frametime * 5;
   float dvel      = frametime * 4;

   for (;;) {
      kill = active_particles;
      if (kill && kill->die <= 0) {
         /* Defensive: if kill itself or its successor isn't a
          * real particle slot, the active list is corrupt;
          * truncate it here rather than splicing garbage onto
          * the free list. */
         if (!R_ValidParticle(kill)) {
            active_particles = NULL;
            break;
         }
         active_particles = kill->next;
         kill->next = free_particles;
         free_particles = kill;
         continue;
      }
      break;
   }

   for (p = active_particles; p; p = p->next) {
      /* Defensive: if walking the active list lands outside the
       * particles array, abort the iteration to avoid
       * dereferencing garbage. */
      if (!R_ValidParticle(p))
         break;
      for (;;) {
         kill = p->next;
         if (kill && kill->die <= 0) {
            if (!R_ValidParticle(kill)) {
               p->next = NULL;
               break;
            }
            p->next = kill->next;
            kill->next = free_particles;
            free_particles = kill;
            continue;
         }
         break;
      }

      p->org[0] += p->vel[0] * frametime;
      p->org[1] += p->vel[1] * frametime;
      p->org[2] += p->vel[2] * frametime;

      switch (p->type)
      {
         case pt_static:
            break;
         case pt_fire:
            p->ramp += time1;
            if (p->ramp >= 6)
               p->die = -1;
            else
               p->color = ramp3[(int)p->ramp];
            p->vel[2] += grav;
            break;

         case pt_explode:
            p->ramp += time2;
            if (p->ramp >= 8)
               p->die = -1;
            else
               p->color = ramp1[(int)p->ramp];
            for (i = 0; i < 3; i++)
               p->vel[i] += p->vel[i] * dvel;
            p->vel[2] -= grav;
            break;

         case pt_explode2:
            p->ramp += time3;
            if (p->ramp >= 8)
               p->die = -1;
            else
               p->color = ramp2[(int)p->ramp];
            for (i = 0; i < 3; i++)
               p->vel[i] -= p->vel[i] * frametime;
            p->vel[2] -= grav;
            break;

         case pt_blob:
            for (i = 0; i < 3; i++)
               p->vel[i] += p->vel[i] * dvel;
            p->vel[2] -= grav;
            break;

         case pt_blob2:
            for (i = 0; i < 2; i++)
               p->vel[i] -= p->vel[i] * dvel;
            p->vel[2] -= grav;
            break;

         case pt_slowgrav:
         case pt_grav:
            p->vel[2] -= grav;
            break;
         /* ENSURE_INT_PTYPE is a sentinel that forces the enum to int
          * size; it is never a real particle type. The default arm
          * keeps -Wswitch quiet without affecting behavior. */
         default:
            break;
      }

      /* Walk down remaining lifetime; the head/tail kill
       * sweeps above will retire this particle next frame
       * once die crosses zero. pt_fire/pt_explode* may have
       * already set die = -1 above to force an early kill;
       * the subtraction below is harmless in that case
       * (die stays <= 0). */
      p->die -= frametime;
   }
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles(void)
{
   particle_t *p;

   D_StartParticles();

   VectorScale(vright, xscaleshrink, r_pright);
   VectorScale(vup, yscaleshrink, r_pup);
   VectorCopy(vpn, r_ppn);

   /* Phase 5b-02: GPU compute particle rasterizer.  When
    * the active RHI backend exposes a dispatch entry AND
    * the user has compute rendering enabled, hand the
    * active linked-list head over and skip the CPU SW
    * for-loop entirely -- the backend stages the particles
    * for a GPU dispatch that runs as part of the per-frame
    * command buffer in end_frame.  The VectorScale /
    * VectorCopy above must still happen before the
    * dispatch call so the SW raster state the backend
    * snapshots (r_pright / r_pup / r_ppn) reflects this
    * frame's camera. */
   if (g_rhi && g_rhi->dispatch_3d_particles
            && g_rhi_compute_rendering) {
      g_rhi->dispatch_3d_particles(active_particles);
      D_EndParticles();
      return;
   }

   for (p = active_particles; p; p = p->next)
      D_DrawParticle(p);

   D_EndParticles();
}
