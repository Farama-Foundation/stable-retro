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
/* sv_user.c -- server code for moving users */

#include "client.h"
#include "cmd.h"
#include "console.h"
#include "host.h"
#include "keys.h"
#include "net.h"
#include "progs.h"
#include "protocol.h"
#include "quakedef.h"
#include "server.h"
#include "sys.h"
#include "view.h"
#include "world.h"

edict_t *sv_player;

cvar_t sv_edgefriction = { "edgefriction", "2" };

static vec3_t forward, right, up;

vec3_t wishdir;
float wishspeed;

/* world */
float *angles;
float *origin;
float *velocity;

qboolean onground;

usercmd_t cmd;

cvar_t sv_idealpitchscale = { "sv_idealpitchscale", "0.8" };


/*
===============
SV_SetIdealPitch
===============
*/
#define	MAX_FORWARD	6
void SV_SetIdealPitch(void)
{
   float angleval, sinval, cosval;
   trace_t tr;
   vec3_t top, bottom;
   float z[MAX_FORWARD];
   int i, j;
   int step, dir, steps;

   if (!((int)sv_player->v.flags & FL_ONGROUND))
      return;

   angleval = sv_player->v.angles[YAW] * M_PI * 2 / 360;
   sinval   = sinf(angleval);
   cosval   = cosf(angleval);

   for (i = 0; i < MAX_FORWARD; i++)
   {
      top[0]    = sv_player->v.origin[0] + cosval * (i + 3) * 12;
      top[1]    = sv_player->v.origin[1] + sinval * (i + 3) * 12;
      top[2]    = sv_player->v.origin[2] + sv_player->v.view_ofs[2];

      bottom[0] = top[0];
      bottom[1] = top[1];
      bottom[2] = top[2] - 160;

      tr = SV_Move(top, vec3_origin, vec3_origin, bottom, 1, sv_player);
      if (tr.allsolid)
         return;		/* looking at a wall, leave ideal the way is was */

      if (tr.fraction == 1)
         return;		/* near a dropoff */

      z[i] = top[2] + tr.fraction * (bottom[2] - top[2]);
   }

   dir   = 0;
   steps = 0;
   for (j = 1; j < i; j++)
   {
      step = z[j] - z[j - 1];
      if (step > -ON_EPSILON && step < ON_EPSILON)
         continue;

      if (dir && (step - dir > ON_EPSILON || step - dir < -ON_EPSILON))
         return;		/* mixed changes */

      steps++;
      dir = step;
   }

   if (!dir)
   {
      sv_player->v.idealpitch = 0;
      return;
   }

   if (steps < 2)
      return;
   sv_player->v.idealpitch = -dir * sv_idealpitchscale.value;
}


/*
==================
SV_UserFriction

==================
*/
static void SV_UserFriction(void)
{
   float newspeed, control;
   vec3_t start, stop;
   float friction;
   trace_t trace;
   float *vel = velocity;
   float speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
   if (!speed)
      return;

   /* if the leading edge is over a dropoff, increase friction */
   start[0] = stop[0] = origin[0] + vel[0] / speed * 16;
   start[1] = stop[1] = origin[1] + vel[1] / speed * 16;
   start[2] = origin[2] + sv_player->v.mins[2];
   stop[2]  = start[2] - 34;

   trace    = SV_Move(start, vec3_origin, vec3_origin, stop, true, sv_player);

   if (trace.fraction == 1.0)
      friction = sv_friction.value * sv_edgefriction.value;
   else
      friction = sv_friction.value;

   /* apply friction */
   control  = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
   newspeed = speed - host_frametime * control * friction;

   if (newspeed < 0)
      newspeed = 0;
   newspeed /= speed;

   vel[0] = vel[0] * newspeed;
   vel[1] = vel[1] * newspeed;
   vel[2] = vel[2] * newspeed;
}

/*
==============
SV_Accelerate
==============
*/
cvar_t sv_maxspeed = { "sv_maxspeed", "320", false, true };
cvar_t sv_accelerate = { "sv_accelerate", "10" };

static void SV_Accelerate(void)
{
   int i;
   float accelspeed;
   float currentspeed = DotProduct(velocity, wishdir);
   float addspeed     = wishspeed - currentspeed;

   if (addspeed <= 0)
      return;

   accelspeed = sv_accelerate.value * host_frametime * wishspeed;
   if (accelspeed > addspeed)
      accelspeed = addspeed;

   for (i = 0; i < 3; i++)
      velocity[i] += accelspeed * wishdir[i];
}

static void SV_AirAccelerate(vec3_t wishveloc)
{
   int i;
   float addspeed, accelspeed, currentspeed;
   float wishspd  = VectorNormalize(wishveloc);

   if (wishspd > 30)
      wishspd = 30;

   currentspeed = DotProduct(velocity, wishveloc);
   addspeed     = wishspd - currentspeed;

   if (addspeed <= 0)
      return;

   accelspeed = sv_accelerate.value * wishspeed * host_frametime;
   if (accelspeed > addspeed)
      accelspeed = addspeed;

   for (i = 0; i < 3; i++)
      velocity[i] += accelspeed * wishveloc[i];
}


static void DropPunchAngle(void)
{
   float len = VectorNormalize(sv_player->v.punchangle);

   len -= 10 * host_frametime;
   if (len < 0)
      len = 0;
   VectorScale(sv_player->v.punchangle, len, sv_player->v.punchangle);
}

/*
===================
SV_WaterMove

===================
*/
static void SV_WaterMove(void)
{
   int i;
   vec3_t wishvel;
   float speed, newspeed, wspeed, addspeed, accelspeed;

   /* user intentions */
   AngleVectors(sv_player->v.v_angle, forward, right, up);

   for (i = 0; i < 3; i++)
      wishvel[i] = forward[i] * cmd.forwardmove + right[i] * cmd.sidemove;

   if (!cmd.forwardmove && !cmd.sidemove && !cmd.upmove)
      wishvel[2] -= 60;	/* drift towards bottom */
   else
      wishvel[2] += cmd.upmove;

   wspeed = Length(wishvel);
   if (wspeed > sv_maxspeed.value)
   {
      VectorScale(wishvel, sv_maxspeed.value / wspeed, wishvel);
      wspeed = sv_maxspeed.value;
   }

   wspeed *= 0.7;

   /* water friction */
   speed = Length(velocity);
   if (speed)
   {
      newspeed = speed - host_frametime * speed * sv_friction.value;
      if (newspeed < 0)
         newspeed = 0;
      VectorScale(velocity, newspeed / speed, velocity);
   } else
      newspeed = 0;

   /* water acceleration */
   if (!wspeed)
      return;

   addspeed = wspeed - newspeed;
   if (addspeed <= 0)
      return;

   VectorNormalize(wishvel);
   accelspeed = sv_accelerate.value * wspeed * host_frametime;
   if (accelspeed > addspeed)
      accelspeed = addspeed;

   for (i = 0; i < 3; i++)
      velocity[i] += accelspeed * wishvel[i];
}

static void SV_WaterJump(void)
{
   if (sv.time > sv_player->v.teleport_time || !sv_player->v.waterlevel)
   {
      sv_player->v.flags = (int)sv_player->v.flags & ~FL_WATERJUMP;
      sv_player->v.teleport_time = 0;
   }
   sv_player->v.velocity[0] = sv_player->v.movedir[0];
   sv_player->v.velocity[1] = sv_player->v.movedir[1];
}


/*
===================
SV_AirMove

===================
*/
static void SV_AirMove(void)
{
   int i;
   vec3_t wishvel;
   float fmove, smove;

   AngleVectors(sv_player->v.angles, forward, right, up);

   fmove = cmd.forwardmove;
   smove = cmd.sidemove;

   /* hack to not let you back into teleporter */
   if (sv.time < sv_player->v.teleport_time && fmove < 0)
      fmove = 0;

   for (i = 0; i < 3; i++)
      wishvel[i] = forward[i] * fmove + right[i] * smove;

   if ((int)sv_player->v.movetype != MOVETYPE_WALK)
      wishvel[2] = cmd.upmove;
   else
      wishvel[2] = 0;

   VectorCopy(wishvel, wishdir);
   wishspeed = VectorNormalize(wishdir);
   if (wishspeed > sv_maxspeed.value)
   {
      VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
      wishspeed = sv_maxspeed.value;
   }

   if (sv_player->v.movetype == MOVETYPE_NOCLIP)
   {
      /* noclip */
      VectorCopy(wishvel, velocity);
   }
   else if (onground)
   {
      SV_UserFriction();
      SV_Accelerate();
   } else {			/* not on ground, so little effect on velocity */
      SV_AirAccelerate(wishvel);
   }
}

/*
===================
SV_ClientThink

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
void SV_ClientThink(void)
{
   vec3_t v_angle;

   if (sv_player->v.movetype == MOVETYPE_NONE)
      return;

   onground = (int)sv_player->v.flags & FL_ONGROUND;
   origin   = sv_player->v.origin;
   velocity = sv_player->v.velocity;

   DropPunchAngle();

   /* if dead, behave differently */
   if (sv_player->v.health <= 0)
      return;

   /* angles
    * show 1/3 the pitch angle and all the roll angle */
   cmd    = host_client->cmd;
   angles = sv_player->v.angles;

   VectorAdd(sv_player->v.v_angle, sv_player->v.punchangle, v_angle);
   angles[ROLL] = V_CalcRoll(sv_player->v.angles, sv_player->v.velocity) * 4;
   if (!sv_player->v.fixangle)
   {
      angles[PITCH] = -v_angle[PITCH] / 3;
      angles[YAW] = v_angle[YAW];
   }

   if ((int)sv_player->v.flags & FL_WATERJUMP)
   {
      SV_WaterJump();
      return;
   }

   /* walk */
   if ((sv_player->v.waterlevel >= 2)
         && (sv_player->v.movetype != MOVETYPE_NOCLIP))
   {
      SV_WaterMove();
      return;
   }

   SV_AirMove();
}


/*
===================
SV_ReadClientMove
===================
*/
static void SV_ReadClientMove(usercmd_t *move)
{
   int i;
   vec3_t angle;
   int bits;

   /* read ping time */
   host_client->ping_times[host_client->num_pings % NUM_PING_TIMES]
   = sv.time - MSG_ReadFloat();
   host_client->num_pings++;

   /* read current angles */
   for (i = 0; i < 3; i++)
      if (sv.protocol == PROTOCOL_VERSION_FITZ)
         angle[i] = MSG_ReadAngle16();
      else
         angle[i] = MSG_ReadAngle();

   VectorCopy(angle, host_client->edict->v.v_angle);

   /* read movement */
   move->forwardmove = MSG_ReadShort();
   move->sidemove    = MSG_ReadShort();
   move->upmove      = MSG_ReadShort();

   /* read buttons */
   bits              = MSG_ReadByte();
   host_client->edict->v.button0 = bits & 1;
   host_client->edict->v.button2 = (bits & 2) >> 1;

   i = MSG_ReadByte();
   if (i)
      host_client->edict->v.impulse = i;
}

/*
===================
SV_ReadClientMessage

Returns false if the client should be killed
===================
*/
static qboolean SV_ReadClientMessage(void)
{
   int ret;
   int opcode;
   char *s;

   do
   {
nextmsg:
      ret = NET_GetMessage(host_client->netconnection);
      if (ret == -1)
      {
         Sys_Printf("%s: NET_GetMessage failed\n", __func__);
         return false;
      }
      if (!ret)
         return true;

      MSG_BeginReading();

      while (1)
      {
         if (!host_client->active)
            return false;	/* a command caused an error */

         if (msg_badread) {
            Sys_Printf("%s: badread\n", __func__);
            return false;
         }

         opcode = MSG_ReadChar();

         switch (opcode) {
            case -1:
               goto nextmsg;	/* end of message */

            default:
               Sys_Printf("%s: unknown command char\n", __func__);
               return false;

            case clc_nop:
               /* Sys_Printf ("clc_nop\n"); */
               break;

            case clc_stringcmd:
               s = MSG_ReadString();

               /* Exact-token whitelist of commands a client
                * is allowed to dispatch via clc_stringcmd.
                * Original code used strncasecmp prefix
                * matching, which was non-exploitable in
                * practice (Cmd_FindCommand on an unknown
                * extended-prefix token would fail and the
                * dispatch would be logged-and-rejected at
                * the cvar fallthrough), but the logic was
                * wrong in spirit -- "saywhatever" prefix-
                * matched "say" and was waved through to
                * Cmd_ExecuteString.
                *
                * We compare against the leading whitespace-
                * delimited token of s.  Each handler still
                * checks cmd_source for its own
                * sensitive-action gating (verified by
                * the Thread D audit); this whitelist is
                * the outer perimeter. */
               {
                  static const char * const allowed[] = {
                     "status", "god", "notarget", "fly",
                     "name", "noclip", "say", "say_team",
                     "tell", "color", "kill", "pause",
                     "spawn", "begin", "prespawn", "kick",
                     "ping", "give", "ban", NULL
                  };
                  const char *cmd_start;
                  size_t      cmd_len;
                  unsigned    ai;

                  /* Cap input length: MSG_ReadString already
                   * bounds at COM_STRBUF_LEN, but a 64KB
                   * command line is still a Cmd_Tokenize
                   * Z_Malloc storm and a Sys_Printf-of-
                   * verbose-rejection log spam.  Trim
                   * silently to a sane cap. */
                  if (strlen(s) >= 1024) {
                     Con_DPrintf("%s: oversized stringcmd from %s "
                                 "(%u bytes), dropping\n",
                                 __func__, host_client->name,
                                 (unsigned)strlen(s));
                     break;
                  }

                  /* Skip leading whitespace */
                  cmd_start = s;
                  while (*cmd_start && *cmd_start <= ' ')
                     cmd_start++;
                  /* Find token end */
                  cmd_len = 0;
                  while (cmd_start[cmd_len] > ' ')
                     cmd_len++;

                  ret = 0;
                  for (ai = 0; allowed[ai]; ai++) {
                     if (strlen(allowed[ai]) == cmd_len
                         && strncasecmp(cmd_start, allowed[ai],
                                        cmd_len) == 0) {
                        ret = 1;
                        break;
                     }
                  }
               }

               if (ret == 1)
                  Cmd_ExecuteString(s, src_client);
               else
                  Con_DPrintf("%s tried to %s\n", host_client->name, s);
               break;

            case clc_disconnect:
               /* Sys_Printf ("%s: client disconnected\n", __func__); */
               return false;

            case clc_move:
               SV_ReadClientMove(&host_client->cmd);
               break;

         }
      }
   }while (ret == 1);

   return true;
}


/*
==================
SV_RunClients
==================
*/
void SV_RunClients(void)
{
   int i;

   for (i = 0, host_client = svs.clients; i < svs.maxclients;
         i++, host_client++)
   {
      if (!host_client->active)
         continue;

      sv_player = host_client->edict;

      if (!SV_ReadClientMessage())
      {
         /* client misbehaved... */
         SV_DropClient(false);
         continue;
      }

      if (!host_client->spawned)
      {
         /* clear client movement until a new packet is received */
         memset(&host_client->cmd, 0, sizeof(host_client->cmd));
         continue;
      }

      /* always pause in single player if in console or menus */
      if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game))
         SV_ClientThink();
   }
}
