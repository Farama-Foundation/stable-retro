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
/* d_init.c: rasterization driver initialization */

#include "quakedef.h"
#include "d_local.h"

#define NUM_MIPS	4

static cvar_t d_mipcap = { "d_mipcap", "0" };
static cvar_t d_mipscale = { "d_mipscale", "1", true };

cvar_t dither_filter = { "dither_filter", "0", true };

surfcache_t *d_initial_rover;
int d_minmip;
float d_scalemip[NUM_MIPS - 1];

static float basemip[NUM_MIPS - 1] = { 1.0, 0.5 * 0.8, 0.25 * 0.8 };

void (*D_DrawSpans)(espan_t *pspan);

/*
===============
D_Init
===============
*/
void
D_Init(void)
{

    r_skydirect = 1;

    Cvar_RegisterVariable(&d_mipcap);
    Cvar_RegisterVariable(&d_mipscale);
    Cvar_RegisterVariable(&dither_filter);

    r_recursiveaffinetriangles = true;
    r_aliasuvscale = 1.0;
}


/*
===============
D_TurnZOn
===============
*/
void
D_TurnZOn(void)
{
/* not needed for software version */
}


/*
===============
D_SetupFrame
===============
*/
void
D_SetupFrame(void)
{
   int i;

   if (r_dowarp)
      d_viewbuffer = r_warpbuffer;
   else
      d_viewbuffer = (pixel_t*)(void *)(byte *)vid.buffer;

   if (r_dowarp)
      screenwidth = WARP_WIDTH;
   else
      screenwidth = vid.rowbytes;

   d_initial_rover = sc_rover;

   d_minmip = d_mipcap.value;
   if (d_minmip > 3)
      d_minmip = 3;
   else if (d_minmip < 0)
      d_minmip = 0;

   for (i = 0; i < (NUM_MIPS - 1); i++)
      d_scalemip[i] = basemip[i] * d_mipscale.value;

   /* dither_filter is defined right above in this same
    * translation unit -- use its address directly instead
    * of Cvar_FindVar("dither_filter") which is an O(log n)
    * red-black tree walk per frame. */
   if (dither_filter.value == 1.0f)
      D_DrawSpans = D_DrawSpans16QbDither;
   else
      D_DrawSpans = D_DrawSpans16Qb;
}


/*
===============
D_UpdateRects

the software driver draws these directly to the vid buffer
===============
*/
void
D_UpdateRects(vrect_t *prect) { }
