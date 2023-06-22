/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include <math.h>

#include "Gfx_1.3.h"
#include "../../libretro/libretro_private.h"
#include "rdp.h"

#include "3dmath.h"

#include "../../Graphics/3dmath.h"
#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RSP/gSP_state.h"

void InverseTransformVector (float *src, float *dst, float mat[4][4])
{
   dst[0] = mat[0][0]*src[0] + mat[0][1]*src[1] + mat[0][2]*src[2];
   dst[1] = mat[1][0]*src[0] + mat[1][1]*src[1] + mat[1][2]*src[2];
   dst[2] = mat[2][0]*src[0] + mat[2][1]*src[1] + mat[2][2]*src[2];
}

void MulMatrices(float m0[4][4], float m1[4][4], float dest[4][4])
{
    float row[4][4];
    register unsigned int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            row[i][j] = m1[i][j];
    for (i = 0; i < 4; i++)
    {
        float summand[4][4];

        for (j = 0; j < 4; j++)
        {
            summand[0][j] = m0[i][0] * row[0][j];
            summand[1][j] = m0[i][1] * row[1][j];
            summand[2][j] = m0[i][2] * row[2][j];
            summand[3][j] = m0[i][3] * row[3][j];
        }
        for (j = 0; j < 4; j++)
            dest[i][j] =
                summand[0][j]
              + summand[1][j]
              + summand[2][j]
              + summand[3][j]
        ;
    }
}

void math_init(void)
{
   unsigned cpu = 0;

   if (perf_get_cpu_features_cb)
      cpu = perf_get_cpu_features_cb();
}

void calc_sphere (VERTEX *v)
{
   float x, y;
   int s_scale = gSP.texture.org_scales >> 6;
   int t_scale = gSP.texture.org_scalet >> 6;

   if (settings.hacks&hack_Chopper)
   {
      s_scale = MIN(gSP.texture.org_scales >> 6, g_gdp.tile[rdp.cur_tile].sl);
      t_scale = MIN(gSP.texture.org_scalet >> 6, g_gdp.tile[rdp.cur_tile].tl);
   }

   TransformVectorNormalize(v->vec, rdp.model);
   x = v->vec[0];
   y = v->vec[1];

   if (gSP.lookatEnable)
   {
      x = DotProduct (rdp.lookat[0], v->vec);
      y = DotProduct (rdp.lookat[1], v->vec);
   }
   v->ou        = (x * 0.5f + 0.5f) * s_scale;
   v->ov        = (y * 0.5f + 0.5f) * t_scale;
   v->uv_scaled = 1;
}

void calc_linear (VERTEX *v)
{
   float x, y;

   if (settings.force_calc_sphere)
   {
      calc_sphere(v);
      return;
   }

   TransformVectorNormalize(v->vec, rdp.model);
   x = v->vec[0];
   y = v->vec[1];
   if (gSP.lookatEnable)
   {
      x = DotProduct (rdp.lookat[0], v->vec);
      y = DotProduct (rdp.lookat[1], v->vec);
   }

   if (x > 1.0f)
      x = 1.0f;
   else if (x < -1.0f)
      x = -1.0f;
   if (y > 1.0f)
      y = 1.0f;
   else if (y < -1.0f)
      y = -1.0f;

   if (rdp.cur_cache[0])
   {
      // scale >> 6 is size to map to
      v->ou     = (acosf(-x)/3.141592654f) * (gSP.texture.org_scales >> 6);
      v->ov     = (acosf(-y)/3.141592654f) * (gSP.texture.org_scalet >> 6);
   }
   v->uv_scaled = 1;
}

