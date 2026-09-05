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
/* r_sky.c */

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"
#include "rhi.h"


int iskyspeed = 8;
int iskyspeed2 = 2;
float skyspeed, skyspeed2;

float skytime;

byte *r_skysource;

int r_skydirect;		/* not used? */

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (texture_t *mt)
{

   skyoverlay = (byte *)mt + mt->offsets[0]; /* Manoel Kasimier - smooth sky */
   skyunderlay = skyoverlay+128; /* Manoel Kasimier - smooth sky */

   /* Phase 5b-07a: push the sky texture to the backend.  The
    * Vulkan backend caches the data and uploads it to a pair
    * of 128x128 R8_UINT storage images that sky.comp samples
    * (the SW backend's hook is NULL -- skyoverlay /
    * skyunderlay are all it needs, since d_sky.c reads them
    * directly).  Called from model.c at level load whenever
    * a sky surface texture is encountered; the same texture
    * stays live for the whole level, so this fires once per
    * level (twice on the rare maps with multiple sky
    * textures -- the last one wins, matching SW). */
   if (g_rhi && g_rhi->notify_sky_texture)
       g_rhi->notify_sky_texture(skyoverlay, skyunderlay);
}

/*
=============
R_SetSkyFrame
==============
*/
void
R_SetSkyFrame(void)
{
    int g, s1, s2;
    /* 'temp' is the modulus we wrap cl.time against. Keep it (and the
     * mod-then-multiply chain) in double so cl.time-derived inputs
     * keep full precision regardless of how long the engine has been
     * running; the final result is bounded below temp, so the cast
     * to fp32 for skytime storage is exact. Same antipattern as
     * V_CalcBob's bob-cycle math fixed earlier in the series. */
    double temp;

    skyspeed = iskyspeed;
    skyspeed2 = iskyspeed2;

    g = GreatestCommonDivisor(iskyspeed, iskyspeed2);
    s1 = iskyspeed / g;
    s2 = iskyspeed2 / g;
    temp = SKYSIZE * s1 * s2;

    skytime = (float)(cl.time - ((int)(cl.time / temp) * temp));
}
