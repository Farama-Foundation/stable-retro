
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
#include "Util.h"
#include "Combine.h"
#include "3dmath.h"
#include "TexCache.h"
#include "Framebuffer_glide64.h"
#include "../../../Graphics/GBI.h"
#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RSP/gSP_funcs_C.h"
#include "../../Graphics/RSP/gSP_state.h"
#include "../../Graphics/RSP/RSP_state.h"

extern int dzdx;
static int deltaZ;
static VERTEX **org_vtx;

typedef struct
{
   float d;
   float x;
   float y;
} LineEquationType;

#define EvaLine(li, x, y) ((li->x) * (x) + (li->y) * (y) + (li->d))

static INLINE void glideSetVertexPrimShading(VERTEX *v, uint32_t prim_color)
{
   v->r = (uint8_t)g_gdp.prim_color.r;
   v->g = (uint8_t)g_gdp.prim_color.g;
   v->b = (uint8_t)g_gdp.prim_color.b;
   v->a = (uint8_t)g_gdp.prim_color.a;
}

static INLINE void glideSetVertexFlatShading(VERTEX *v, VERTEX **vtx, uint32_t w1)
{
   int flag = MIN(2, (w1 >> 24) & 3);
   v->a = vtx[flag]->a;
   v->b = vtx[flag]->b;
   v->g = vtx[flag]->g;
   v->r = vtx[flag]->r;
}

void apply_shade_modulation(VERTEX *v)
{
   if (rdp.cmb_flags)
   {
      if (v->shade_mod == 0)
         v->color_backup = *(uint32_t*)(&(v->b));
      else
         *(uint32_t*)(&(v->b)) = v->color_backup;

      if (rdp.cmb_flags & CMB_SET)
      {
         v->r = (uint8_t)(255.0f * clamp_float(rdp.col[0], 0.0, 1.0));
         v->g = (uint8_t)(255.0f * clamp_float(rdp.col[1], 0.0, 1.0));
         v->b = (uint8_t)(255.0f * clamp_float(rdp.col[2], 0.0, 1.0));
      }

      if (rdp.cmb_flags & CMB_A_SET)
         v->a = (uint8_t)(255.0f * clamp_float(rdp.col[3], 0.0, 1.0));

      if (rdp.cmb_flags & CMB_SETSHADE_SHADEALPHA)
         v->r = v->g = v->b = v->a;

      if (rdp.cmb_flags & CMB_MULT_OWN_ALPHA)
      {
         float percent = v->a / 255.0f;
         v->r = (uint8_t)(v->r * percent);
         v->g = (uint8_t)(v->g * percent);
         v->b = (uint8_t)(v->b * percent);
      }

      if (rdp.cmb_flags & CMB_MULT)
      {
         v->r = (uint8_t)(v->r * clamp_float(rdp.col[0], 0.0, 1.0));
         v->g = (uint8_t)(v->g * clamp_float(rdp.col[1], 0.0, 1.0));
         v->b = (uint8_t)(v->b * clamp_float(rdp.col[2], 0.0, 1.0));
      }

      if (rdp.cmb_flags & CMB_A_MULT)
         v->a = (uint8_t)(v->a * clamp_float(rdp.col[3], 0.0, 1.0));
      if (rdp.cmb_flags & CMB_SUB)
      {
         int r = v->r - (int)(255.0f * rdp.coladd[0]);
         int g = v->g - (int)(255.0f * rdp.coladd[1]);
         int b = v->b - (int)(255.0f * rdp.coladd[2]);
         if (r < 0) r = 0;
         if (g < 0) g = 0;
         if (b < 0) b = 0;
         v->r = (uint8_t)r;
         v->g = (uint8_t)g;
         v->b = (uint8_t)b;
      }
      if (rdp.cmb_flags & CMB_A_SUB)
      {
         int a = v->a - (int)(255.0f * rdp.coladd[3]);
         if (a < 0) a = 0;
         v->a = (uint8_t)a;
      }
      if (rdp.cmb_flags & CMB_ADD)
      {
         int r = v->r + (int)(255.0f * rdp.coladd[0]);
         int g = v->g + (int)(255.0f * rdp.coladd[1]);
         int b = v->b + (int)(255.0f * rdp.coladd[2]);
         if (r > 255) r = 255;
         if (g > 255) g = 255;
         if (b > 255) b = 255;
         v->r = (uint8_t)r;
         v->g = (uint8_t)g;
         v->b = (uint8_t)b;
      }
      if (rdp.cmb_flags & CMB_A_ADD)
      {
         int a = v->a + (int)(255.0f * rdp.coladd[3]);
         if (a > 255) a = 255;
         v->a = (uint8_t)a;
      }
      if (rdp.cmb_flags & CMB_COL_SUB_OWN)
      {
         int r = (uint8_t)(255.0f * rdp.coladd[0]) - v->r;
         int g = (uint8_t)(255.0f * rdp.coladd[1]) - v->g;
         int b = (uint8_t)(255.0f * rdp.coladd[2]) - v->b;
         if (r < 0) r = 0;
         if (g < 0) g = 0;
         if (b < 0) b = 0;
         v->r = (uint8_t)r;
         v->g = (uint8_t)g;
         v->b = (uint8_t)b;
      }
      v->shade_mod = cmb.shade_mod_hash;
   }

   if (rdp.cmb_flags_2 & CMB_INTER)
   {
      v->r = (uint8_t)(rdp.col_2[0] * rdp.shade_factor * 255.0f + v->r * (1.0f - rdp.shade_factor));
      v->g = (uint8_t)(rdp.col_2[1] * rdp.shade_factor * 255.0f + v->g * (1.0f - rdp.shade_factor));
      v->b = (uint8_t)(rdp.col_2[2] * rdp.shade_factor * 255.0f + v->b * (1.0f - rdp.shade_factor));
      v->shade_mod = cmb.shade_mod_hash;
   }
}

void apply_shading(void *data)
{
   int i;
   VERTEX *vptr = (VERTEX*)data;

   for (i = 0; i < 4; i++)
   {
      vptr[i].shade_mod = 0;
      apply_shade_modulation(&vptr[i]);
   }
}

static void Create1LineEquation(LineEquationType *l, VERTEX *v1, VERTEX *v2, VERTEX *v3)
{
   float x = v3->sx;
   float y = v3->sy;

   // Line between (x1,y1) to (x2,y2)
   l->x    = v2->sy-v1->sy;
   l->y    = v1->sx-v2->sx;
   l->d    = -(l->x * v2->sx+ (l->y) * v2->sy);

   if (EvaLine(l,x,y) * v3->oow < 0)
   {
      l->x = -l->x;
      l->y = -l->y;
      l->d = -l->d;
   }
}

static INLINE void glide64_interpolate_colors(VERTEX *va, VERTEX *vb, VERTEX *res, float percent,
      float va_oow, float vb_oow, float w)
{
   float ba = va->b * va_oow;
   float bb = vb->b * vb_oow;
   float ga = va->g * va_oow;
   float gb = vb->g * vb_oow;
   float ra = va->r * va_oow;
   float rb = vb->r * vb_oow;
   float aa = va->a * va_oow;
   float ab = vb->a * vb_oow;
   float fa = va->f * va_oow;
   float fb = vb->f * vb_oow;

   res->r   = (uint8_t)((ra + (rb - ra) * percent) * w);
   res->g   = (uint8_t)((ga + (gb - ga) * percent) * w);
   res->b   = (uint8_t)((ba + (bb - ba) * percent) * w);
   res->a   = (uint8_t)((aa + (ab - aa) * percent) * w);
   res->f   = (fa + (fb - fa) * percent) * w;
}

#define interp3p(a, b, c, r1, r2) ((a)+(((b)+((c)-(b))*(r2))-(a))*(r1))

static void InterpolateColors3(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *out)
{
   LineEquationType line;
   float aDot, bDot, scale1, tx, ty, s1, s2, den, w;
   Create1LineEquation(&line, v2, v3, v1);

   aDot   = (out->x * line.x + out->y * line.y);
   bDot   = (v1->sx * line.x + v1->sy * line.y);
   scale1 = ( -line.d - aDot) / ( bDot - aDot );
   tx     = out->x + scale1 * (v1->sx - out->x);
   ty     = out->y + scale1 * (v1->sy - out->y);
   s1     = 101.0;
   s2     = 101.0;
   den    = tx - v1->sx;

   if (fabs(den) > 1.0)
      s1 = (out->x-v1->sx)/den;
   if (s1 > 100.0f)
      s1 = (out->y-v1->sy)/(ty-v1->sy);

   den = v3->sx - v2->sx;
   if (fabs(den) > 1.0)
      s2 = (tx-v2->sx)/den;
   if (s2 > 100.0f)
      s2 =(ty-v2->sy)/(v3->sy-v2->sy);

   w = 1.0f / interp3p(v1->oow,v2->oow,v3->oow,s1,s2);

   out->r = (uint8_t)
      (w * interp3p(v1->r*v1->oow,v2->r*v2->oow,v3->r*v3->oow,s1,s2));
   out->g = (uint8_t)
      (w * interp3p(v1->g*v1->oow,v2->g*v2->oow,v3->g*v3->oow,s1,s2));
   out->b = (uint8_t)
      (w * interp3p(v1->b*v1->oow,v2->b*v2->oow,v3->b*v3->oow,s1,s2));
   out->a = (uint8_t)
      (w * interp3p(v1->a*v1->oow,v2->a*v2->oow,v3->a*v3->oow,s1,s2));
   out->f = interp3p(v1->f*v1->oow,v2->f*v2->oow,v3->f*v3->oow,s1,s2) * w;
}


static INLINE void CalculateLODValues(VERTEX *v, int32_t i, int32_t j, float *lodFactor, float s_scale, float t_scale)
{
   float deltaS      = (v[j].u[0]/v[j].q - v[i].u[0]/v[i].q) * s_scale;
   float deltaT      = (v[j].v[0]/v[j].q - v[i].v[0]/v[i].q) * t_scale;
   float deltaTexels = sqrtf( deltaS * deltaS + deltaT * deltaT );

   float deltaX      = (v[j].x - v[i].x)/rdp.scale_x;
   float deltaY      = (v[j].y - v[i].y)/rdp.scale_y;
   float deltaPixels = sqrtf( deltaX * deltaX + deltaY * deltaY );

   *lodFactor       += deltaTexels / deltaPixels;
}

static void CalculateLOD(VERTEX *v, int n, uint32_t lodmode)
{
   float intptr, lod_fraction, detailmax;
   int i, j, ilod, lod_tile;
   float s_scale   = gDP.tiles[rdp.cur_tile].width / 255.0f;
   float t_scale   = gDP.tiles[rdp.cur_tile].height / 255.0f;
   float lodFactor = 0;

   if (lodmode == G_TL_LOD)
   {
      n = 1;
      j = 1;
   }

   for (i = 0; i < n; i++)
   {
      if (lodmode == G_TL_TILE)
         j = (i < n-1) ? (i + 1) : 0;
      CalculateLODValues(v, i, j, &lodFactor, s_scale, t_scale);
   }

   if (lodmode == G_TL_TILE)
      lodFactor = lodFactor / n; /* Divide by n (n edges) to find average */

   ilod         = (int)lodFactor;
   lod_tile     = MIN((int)(log10f((float)ilod)/log10f(2.0f)), rdp.cur_tile + gDP.otherMode.textureDetail);
   lod_fraction = 1.0f;
   detailmax    = 1.0f - lod_fraction;

   if (lod_tile < rdp.cur_tile + gDP.otherMode.textureDetail)
      lod_fraction = MAX((float)modff(lodFactor / pow(2.,lod_tile),&intptr), g_gdp.primitive_lod_min / 255.0f);

   if (cmb.dc0_detailmax < 0.5f)
      detailmax = lod_fraction;

   grTexDetailControl (GR_TMU0, cmb.dc0_lodbias, cmb.dc0_detailscale, detailmax);
   grTexDetailControl (GR_TMU1, cmb.dc1_lodbias, cmb.dc1_detailscale, detailmax);
}

static void clip_tri(int interpolate_colors)
{
   int i,j,index,n=rdp.n_global;

   // Check which clipping is needed
   if (rdp.clip & X_CLIP_MAX) // right of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp     = rdp.vtxbuf2;
      rdp.vtxbuf2     = rdp.vtxbuf;
      rdp.vtxbuf      = tmp;
      rdp.vtx_buffer ^= 1;
      index           = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         bool save_inpoint = false;
         VERTEX *first, *second, *current = NULL, *current2 = NULL;
         j = i+1;
         if (j == n)
            j = 0;
         first  = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->x <= rdp.clip_max_x)
         {
            if (second->x <= rdp.clip_max_x)   // Both are in, save the last one
            {
               save_inpoint = true;
            }
            else      // First is in, second is out, save intersection
            {
               current  = first;
               current2 = second;
            }
         }
         else
         {
            if (second->x <= rdp.clip_max_x) // First is out, second is in, save intersection & in point
            {
               current  = second;
               current2 = first;

               save_inpoint = true;
            }
         }

         if (current && current2)
         {
            float percent       = (rdp.clip_max_x - current->x) / (current2->x - current->x);
            rdp.vtxbuf[index].x = rdp.clip_max_x;
            rdp.vtxbuf[index].y = current->y + (current2->y - current->y) * percent;
            rdp.vtxbuf[index].z = current->z + (current2->z - current->z) * percent;
            rdp.vtxbuf[index].q = current->q + (current2->q - current->q) * percent;
            clip_tri_uv(current, current2, index, percent);
            clip_tri_interp_colors(current, current2, index, percent, 8, interpolate_colors);

         }

         if (save_inpoint)
         {
            // Save the in point
            rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
         }
      }
      n = index;
   }

   if (rdp.clip & X_CLIP_MIN) // left of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         bool save_inpoint = false;
         VERTEX *first, *second, *current = NULL, *current2 = NULL;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->x >= rdp.clip_min_x)
         {
            if (second->x >= rdp.clip_min_x)   // Both are in, save the last one
            {
               save_inpoint = true;
            }
            else      // First is in, second is out, save intersection
            {
               current  = first;
               current2 = second;
            }
         }
         else
         {
            if (second->x >= rdp.clip_min_x) // First is out, second is in, save intersection & in point
            {

               current  = second;
               current2 = first;

               save_inpoint = true;
            }
         }

         if (current && current2)
         {
            float percent       = (rdp.clip_min_x - current->x) / (current2->x - current->x);
            rdp.vtxbuf[index].x = rdp.clip_min_x;
            rdp.vtxbuf[index].y = current->y + (current2->y - current->y) * percent;
            rdp.vtxbuf[index].z = current->z + (current2->z - current->z) * percent;
            rdp.vtxbuf[index].q = current->q + (current2->q - current->q) * percent;
            clip_tri_uv(current, current2, index, percent);
            clip_tri_interp_colors(current, current2, index, percent, 8, interpolate_colors);
         }

         if (save_inpoint)
         {
            // Save the in point
            rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
         }
      }
      n = index;
   }

   if (rdp.clip & Y_CLIP_MAX) // top of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         bool save_inpoint = false;
         VERTEX *first, *second, *current = NULL, *current2 = NULL;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->y <= rdp.clip_max_y)
         {
            if (second->y <= rdp.clip_max_y)   // Both are in, save the last one
            {
               save_inpoint = true;
            }
            else      // First is in, second is out, save intersection
            {
               current  = first;
               current2 = second;
            }
         }
         else
         {
            if (second->y <= rdp.clip_max_y) // First is out, second is in, save intersection & in point
            {
               current  = second;
               current2 = first;

               save_inpoint = true;
            }
         }

         if (current && current2)
         {
            float percent       = (rdp.clip_max_y - current->y) / (current2->y - current->y);
            rdp.vtxbuf[index].x = current->x + (current2->x - current->x) * percent;
            rdp.vtxbuf[index].y = rdp.clip_max_y;
            rdp.vtxbuf[index].z = current->z + (current2->z - current->z) * percent;
            rdp.vtxbuf[index].q = current->q + (current2->q - current->q) * percent;
            clip_tri_uv(current, current2, index, percent);
            clip_tri_interp_colors(current, current2, index, percent, 16, interpolate_colors);
         }

         if (save_inpoint)
         {
            // Save the in point
            rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
         }
      }
      n = index;
   }

   if (rdp.clip & Y_CLIP_MIN) // bottom of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         bool save_inpoint = false;
         VERTEX *first, *second, *current = NULL, *current2 = NULL;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->y >= rdp.clip_min_y)
         {
            if (second->y >= rdp.clip_min_y)   // Both are in, save the last one
            {
               save_inpoint = true;
            }
            else      // First is in, second is out, save intersection
            {
               current      = first;
               current2     = second;
            }
         }
         else
         {
            if (second->y >= rdp.clip_min_y) // First is out, second is in, save intersection & in point
            {
               current      = second;
               current2     = first;
               save_inpoint = true;
            }
         }

         if (current && current2)
         {
            float percent       = (rdp.clip_min_y - current->y) / (current2->y - current->y);
            rdp.vtxbuf[index].x = current->x + (current2->x - current->x) * percent;
            rdp.vtxbuf[index].y = rdp.clip_min_y;
            rdp.vtxbuf[index].z = current->z + (current2->z - current->z) * percent;
            rdp.vtxbuf[index].q = current->q + (current2->q - current->q) * percent;
            clip_tri_uv(current, current2, index, percent);
            clip_tri_interp_colors(current, current2, index, percent, 16, interpolate_colors);
         }

         if (save_inpoint)
         {
            // Save the in point
            rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
         }
      }
      n = index;
   }

   if (rdp.clip & CLIP_ZMAX) // far plane
   {
      // Swap vertex buffers
      VERTEX *tmp;
	  float maxZ;

      tmp = (VERTEX*)rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;
      maxZ = gSP.viewport.vtrans[2] + gSP.viewport.vscale[2];

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         bool save_inpoint = false;
         VERTEX *first, *second, *current = NULL, *current2 = NULL;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->z < maxZ)
         {
            if (second->z < maxZ)
            {
               /* Both are in, save the last one */
               save_inpoint = true;
            }
            else
            {
               /* First is in, second is out, save intersection */
               current  = first;
               current2 = second;
            }
         }
         else
         {
            if (second->z < maxZ)
            {
               /* First is out, second is in, save intersection & in point */
               current      = second;
               current2     = first;

               if (second->z < maxZ)
                  save_inpoint = true;
            }
         }

         if (current && current2)
         {
            float percent       = (maxZ - current->z) / (current2->z - current->z);
            rdp.vtxbuf[index].x = current->x + (current2->x - current->x) * percent;
            rdp.vtxbuf[index].y = current->y + (current2->y - current->y) * percent;
            rdp.vtxbuf[index].z = maxZ - 0.001f;;
            rdp.vtxbuf[index].q = current->q + (current2->q - current->q) * percent;
            clip_tri_uv(current, current2, index, percent);
            clip_tri_interp_colors(current, current2, index, percent, 0, interpolate_colors);
         }

         /* Save the in point */
         if (save_inpoint)
            rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
      }
      n = index;
   }

#if 0
   if (rdp.clip & CLIP_ZMIN) // near Z
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         j = i+1;
         if (j == n) j = 0;

         if (rdp.vtxbuf2[i].z >= 0.0f)
         {
            if (rdp.vtxbuf2[j].z >= 0.0f)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               float       percent = (-rdp.vtxbuf2[i].z) / (rdp.vtxbuf2[j].z - rdp.vtxbuf2[i].z);
               rdp.vtxbuf[index].x = rdp.vtxbuf2[i].x + (rdp.vtxbuf2[j].x - rdp.vtxbuf2[i].x) * percent;
               rdp.vtxbuf[index].y = rdp.vtxbuf2[i].y + (rdp.vtxbuf2[j].y - rdp.vtxbuf2[i].y) * percent;
               rdp.vtxbuf[index].z = 0.0f;
               rdp.vtxbuf[index].q = rdp.vtxbuf2[i].q + (rdp.vtxbuf2[j].q - rdp.vtxbuf2[i].q) * percent;
               rdp.vtxbuf[index].u[0] = rdp.vtxbuf2[i].u[0] + (rdp.vtxbuf2[j].u[0] - rdp.vtxbuf2[i].u[0]) * percent;
               rdp.vtxbuf[index].v[0] = rdp.vtxbuf2[i].v[0] + (rdp.vtxbuf2[j].v[0] - rdp.vtxbuf2[i].v[0]) * percent;
               rdp.vtxbuf[index].u[1] = rdp.vtxbuf2[i].u[1] + (rdp.vtxbuf2[j].u[1] - rdp.vtxbuf2[i].u[1]) * percent;
               rdp.vtxbuf[index].v[1] = rdp.vtxbuf2[i].v[1] + (rdp.vtxbuf2[j].v[1] - rdp.vtxbuf2[i].v[1]) * percent;
               if (interpolate_colors)
                  glide64_interpolate_colors(&rdp.vtxbuf2[i], &rdp.vtxbuf2[j], &rdp.vtxbuf[index++], percent, 1.0f, 1.0f, 1.0f);
               else
                  rdp.vtxbuf[index++].number = rdp.vtxbuf2[i].number | rdp.vtxbuf2[j].number;
            }
         }
         else
         {
            //if (rdp.vtxbuf2[j].z < 0.0f)  // Both are out, save nothing
            if (rdp.vtxbuf2[j].z >= 0.0f) // First is out, second is in, save intersection & in point
            {
               float       percent = (-rdp.vtxbuf2[j].z) / (rdp.vtxbuf2[i].z - rdp.vtxbuf2[j].z);
               rdp.vtxbuf[index].x = rdp.vtxbuf2[j].x + (rdp.vtxbuf2[i].x - rdp.vtxbuf2[j].x) * percent;
               rdp.vtxbuf[index].y = rdp.vtxbuf2[j].y + (rdp.vtxbuf2[i].y - rdp.vtxbuf2[j].y) * percent;
               rdp.vtxbuf[index].z = 0.0f;;
               rdp.vtxbuf[index].q = rdp.vtxbuf2[j].q + (rdp.vtxbuf2[i].q - rdp.vtxbuf2[j].q) * percent;
               rdp.vtxbuf[index].u[0] = rdp.vtxbuf2[j].u[0] + (rdp.vtxbuf2[i].u[0] - rdp.vtxbuf2[j].u[0]) * percent;
               rdp.vtxbuf[index].v[0] = rdp.vtxbuf2[j].v[0] + (rdp.vtxbuf2[i].v[0] - rdp.vtxbuf2[j].v[0]) * percent;
               rdp.vtxbuf[index].u[1] = rdp.vtxbuf2[j].u[1] + (rdp.vtxbuf2[i].u[1] - rdp.vtxbuf2[j].u[1]) * percent;
               rdp.vtxbuf[index].v[1] = rdp.vtxbuf2[j].v[1] + (rdp.vtxbuf2[i].v[1] - rdp.vtxbuf2[j].v[1]) * percent;
               if (interpolate_colors)
                  glide64_interpolate_colors(&rdp.vtxbuf2[j], &rdp.vtxbuf2[i], &rdp.vtxbuf[index++], percent, 1.0f, 1.0f, 1.0f);
               else
                  rdp.vtxbuf[index++].number = rdp.vtxbuf2[i].number | rdp.vtxbuf2[j].number;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }
#endif

   rdp.n_global = n;
}

static void render_tri (uint16_t linew, int old_interpolate)
{
   int i, n;
   float fog;

   if (rdp.clip)
      clip_tri(old_interpolate);

   n = rdp.n_global;

   if ((rdp.clip & CLIP_ZMIN) && (gDP.otherMode.l & G_OBJLT_TLUT))
   {
      int to_render = false;

      for (i = 0; i < n; i++)
      {
         if (rdp.vtxbuf[i].z >= 0.0f)
         {
            to_render = true;
            break;
         }
      }

      if (!to_render) //all z < 0
         return;
   }

   if (rdp.clip && !old_interpolate)
   {
      for (i = 0; i < n; i++)
      {
         float percent = 101.0f;
         VERTEX * v1   = 0,  * v2 = 0;

         switch (rdp.vtxbuf[i].number&7)
         {
            case 1:
            case 2:
            case 4:
               continue;
               break;
            case 3:
               v1 = org_vtx[0];
               v2 = org_vtx[1];
               break;
            case 5:
               v1 = org_vtx[0];
               v2 = org_vtx[2];
               break;
            case 6:
               v1 = org_vtx[1];
               v2 = org_vtx[2];
               break;
            case 7:
               InterpolateColors3(org_vtx[0], org_vtx[1], org_vtx[2], &rdp.vtxbuf[i]);
               continue;
               break;
         }

         switch (rdp.vtxbuf[i].number&24)
         {
            case 8:
               percent = (rdp.vtxbuf[i].x-v1->sx)/(v2->sx-v1->sx);
               break;
            case 16:
               percent = (rdp.vtxbuf[i].y-v1->sy)/(v2->sy-v1->sy);
               break;
            default:
               {
                  float d = (v2->sx-v1->sx);
                  if (fabs(d) > 1.0)
                     percent = (rdp.vtxbuf[i].x-v1->sx)/d;
                  if (percent > 100.0f)
                     percent = (rdp.vtxbuf[i].y-v1->sy)/(v2->sy-v1->sy);
               }
         }

         {
            float w = 1.0f / (v1->oow + (v2->oow - v1->oow) * percent);
            glide64_interpolate_colors(v1, v2, &rdp.vtxbuf[i], percent, v1->oow, v2->oow, w);
         }
      }
   }

   ConvertCoordsConvert (rdp.vtxbuf, n);

   switch (rdp.fog_mode)
   {
      case FOG_MODE_ENABLED:
         for (i = 0; i < n; i++)
            rdp.vtxbuf[i].f = 1.0f/MAX(4.0f, rdp.vtxbuf[i].f);
         break;
      case FOG_MODE_BLEND:
         fog = 1.0f/MAX(1, g_gdp.fog_color.a);
         for (i = 0; i < n; i++)
            rdp.vtxbuf[i].f = fog;
         break;
      case FOG_MODE_BLEND_INVERSE:
         fog = 1.0f/MAX(1, (~g_gdp.fog_color.total) & 0xFF);
         for (i = 0; i < n; i++)
            rdp.vtxbuf[i].f = fog;
         break;
   }

   if (settings.lodmode && rdp.cur_tile < gDP.otherMode.textureDetail)
      CalculateLOD(rdp.vtxbuf, n, settings.lodmode);

   cmb.cmb_ext_use = cmb.tex_cmb_ext_use = 0;

   if (linew > 0)
   {
      VERTEX v[4];
      float width;

      VERTEX *V0 = &rdp.vtxbuf[0];
      VERTEX *V1 = &rdp.vtxbuf[1];

      if (fabs(V0->x - V1->x) < 0.01 && fabs(V0->y - V1->y) < 0.01)
         V1      = &rdp.vtxbuf[2];

      V0->z      = ScaleZ(V0->z);
      V1->z      = ScaleZ(V1->z);

      v[0]       = *V0;
      v[1]       = *V0;
      v[2]       = *V1;
      v[3]       = *V1;

      width      = linew * 0.25f;

      if (fabs(V0->y - V1->y) < 0.0001)
      {
         v[0].x = v[1].x  = V0->x;
         v[2].x = v[3].x  = V1->x;

         width           *= rdp.scale_y;
         v[0].y = v[2].y  = V0->y - width;
         v[1].y = v[3].y  = V0->y + width;
      }
      else if (fabs(V0->x - V1->x) < 0.0001)
      {
         v[0].y = v[1].y  = V0->y;
         v[2].y = v[3].y  = V1->y;

         width *= rdp.scale_x;
         v[0].x = v[2].x  = V0->x - width;
         v[1].x = v[3].x  = V0->x + width;
      }
      else
      {
         float dx  = V1->x - V0->x;
         float dy  = V1->y - V0->y;
         float len = sqrtf(dx*dx + dy*dy);
         float wx  = dy * width * rdp.scale_x / len;
         float wy  = dx * width * rdp.scale_y / len;
         v[0].x    = V0->x + wx;
         v[0].y    = V0->y - wy;
         v[1].x    = V0->x - wx;
         v[1].y    = V0->y + wy;
         v[2].x    = V1->x + wx;
         v[2].y    = V1->y - wy;
         v[3].x    = V1->x - wx;
         v[3].y    = V1->y + wy;
      }

      grDrawVertexArrayContiguous(GR_TRIANGLE_STRIP, 4, &v[0]);
   }
   else
   {
      DrawDepthBuffer(rdp.vtxbuf, n);

      if ((rdp.rm & 0xC10) == 0xC10)
         grDepthBiasLevel(-deltaZ);

      grDrawVertexArrayContiguous(GR_TRIANGLE_FAN, n,
            rdp.vtx_buffer ? rdp.vtx2 : rdp.vtx1);
   }
}

void do_triangle_stuff_2 (uint16_t linew, uint8_t no_clip, int old_interpolate)
{
   int i;
   if (no_clip)
      rdp.clip = 0;

   for (i = 0; i < rdp.n_global; i++)
   {
      if (rdp.vtxbuf[i].x > rdp.clip_max_x)
         rdp.clip |= X_CLIP_MAX;
      if (rdp.vtxbuf[i].x < rdp.clip_min_x)
         rdp.clip |= X_CLIP_MIN;
      if (rdp.vtxbuf[i].y > rdp.clip_max_y)
         rdp.clip |= Y_CLIP_MAX;
      if (rdp.vtxbuf[i].y < rdp.clip_min_y)
         rdp.clip |= Y_CLIP_MIN;
   }

   render_tri (linew, old_interpolate);
}

void do_triangle_stuff (uint16_t linew, int old_interpolate) // what else?? do the triangle stuff :P (to keep from writing code twice)
{
   int i;
   float maxZ = (g_gdp.other_modes.z_source_sel != G_ZS_PRIM) ? gSP.viewport.vtrans[2] + gSP.viewport.vscale[2] : g_gdp.prim_color.z;
   uint8_t no_clip = 2;

   for (i=0; i< rdp.n_global; i++)
   {
      VERTEX *vtx = (VERTEX*)&rdp.vtxbuf[i];

      if (vtx->not_zclipped)
      {
         //FRDP (" * NOT ZCLIPPPED: %d\n", vtx->number);
         vtx->x    = vtx->sx;
         vtx->y    = vtx->sy;
         vtx->z    = vtx->sz;
         vtx->q    = vtx->oow;
         vtx->u[0] = vtx->u_w[0];
         vtx->v[0] = vtx->v_w[0];
         vtx->u[1] = vtx->u_w[1];
         vtx->v[1] = vtx->v_w[1];
      }
      else
      {
         //FRDP (" * ZCLIPPED: %d\n", vtx->number);
         vtx->q = 1.0f / vtx->w;
         vtx->x = gSP.viewport.vtrans[0] + vtx->x * vtx->q * gSP.viewport.vscale[0] + rdp.offset_x;
         vtx->y = gSP.viewport.vtrans[1] + vtx->y * vtx->q * gSP.viewport.vscale[1] + rdp.offset_y;
         vtx->z = gSP.viewport.vtrans[2] + vtx->z * vtx->q * gSP.viewport.vscale[2];
         if (rdp.tex >= 1)
         {
            vtx->u[0] *= vtx->q;
            vtx->v[0] *= vtx->q;
         }
         if (rdp.tex >= 2)
         {
            vtx->u[1] *= vtx->q;
            vtx->v[1] *= vtx->q;
         }
      }

      if (g_gdp.other_modes.z_source_sel == G_ZS_PRIM)
         vtx->z = g_gdp.prim_color.z;

      // Don't remove clipping, or it will freeze
      if (vtx->x > rdp.clip_max_x)
         rdp.clip |= X_CLIP_MAX;
      if (vtx->x < rdp.clip_min_x)
         rdp.clip |= X_CLIP_MIN;
      if (vtx->y > rdp.clip_max_y)
         rdp.clip |= Y_CLIP_MAX;
      if (vtx->y < rdp.clip_min_y)
         rdp.clip |= Y_CLIP_MIN;
      if (vtx->z > maxZ)
         rdp.clip |= CLIP_ZMAX;
      if (vtx->z < 0.0f)
         rdp.clip |= CLIP_ZMIN;

      no_clip &= vtx->screen_translated;
   }
   if (!no_clip)
   {
      if (!settings.clip_zmin)
         rdp.clip &= ~CLIP_ZMIN;
   }

   do_triangle_stuff_2(linew, no_clip, old_interpolate);
}


void update_scissor(bool set_scissor)
{
   if (!(g_gdp.flags & UPDATE_SCISSOR))
      return;

   if (set_scissor)
   {
      rdp.scissor.ul_x = (uint32_t)rdp.clip_min_x;
      rdp.scissor.lr_x = (uint32_t)rdp.clip_max_x;
      rdp.scissor.ul_y = (uint32_t)rdp.clip_min_y;
      rdp.scissor.lr_y = (uint32_t)rdp.clip_max_y;
   }
   else
   {
      rdp.scissor.ul_x = (uint32_t)(g_gdp.__clip.xh * rdp.scale_x + rdp.offset_x);
      rdp.scissor.lr_x = (uint32_t)(g_gdp.__clip.xl * rdp.scale_x + rdp.offset_x);
      rdp.scissor.ul_y = (uint32_t)(g_gdp.__clip.yh * rdp.scale_y + rdp.offset_y);
      rdp.scissor.lr_y = (uint32_t)(g_gdp.__clip.yl * rdp.scale_y + rdp.offset_y);
   }

   grClipWindow (rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);

   g_gdp.flags ^= UPDATE_SCISSOR;
}

static void glide64_z_compare(void)
{
   int depthbias_level = 0;
   int depthbuf_func   = GR_CMP_ALWAYS;
   int depthmask_val   = FXFALSE;
   g_gdp.flags ^= UPDATE_ZBUF_ENABLED;

   if (((rdp.flags & ZBUF_ENABLED) || ((g_gdp.other_modes.z_source_sel == G_ZS_PRIM) && (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) < G_CYC_COPY))))
   {
      if (rdp.flags & ZBUF_COMPARE)
      {
         switch (g_gdp.other_modes.z_mode)
         {
            case ZMODE_OPA:
               depthbuf_func   = settings.zmode_compare_less ? GR_CMP_LESS : GR_CMP_LEQUAL;
               break;
            case ZMODE_INTER:
               depthbias_level = -4;
               depthbuf_func   = settings.zmode_compare_less ? GR_CMP_LESS : GR_CMP_LEQUAL;
               break;
            case ZMODE_XLU:
               if (settings.ucode == 7)
                  depthbias_level = -4;
               depthbuf_func = GR_CMP_LESS;
               break;
            case ZMODE_DEC:
               // will be set dynamically per polygon
               //grDepthBiasLevel(-deltaZ);
               depthbuf_func = GR_CMP_LEQUAL;
               break;
         }
      }

      if (rdp.flags & ZBUF_UPDATE)
         depthmask_val = FXTRUE;
   }

   grDepthBiasLevel(depthbias_level);
   grDepthBufferFunction (depthbuf_func);
   grDepthMask(depthmask_val);
}

//
// update - update states if they need it
//
void update(void)
{
   bool set_scissor = false;

   // Check for rendermode changes
   // Z buffer
   if (rdp.render_mode_changed & 0x00000C30)
   {
      FRDP (" |- render_mode_changed zbuf - decal: %s, update: %s, compare: %s\n",
            str_yn[(gDP.otherMode.l & G_CULL_BACK)?1:0],
            str_yn[(gDP.otherMode.l & UPDATE_BIASLEVEL)?1:0],
            str_yn[(gDP.otherMode.alphaCompare)?1:0]);

      rdp.render_mode_changed &= ~0x00000C30;
      g_gdp.flags |= UPDATE_ZBUF_ENABLED;

      // Update?
      if (gDP.otherMode.depthUpdate)
         rdp.flags |= ZBUF_UPDATE;
      else
         rdp.flags &= ~ZBUF_UPDATE;

      // Compare?
      if (gDP.otherMode.depthCompare)
         rdp.flags |= ZBUF_COMPARE;
      else
         rdp.flags &= ~ZBUF_COMPARE;
   }

   // Alpha compare
   if (rdp.render_mode_changed & CULL_FRONT)
   {
      FRDP (" |- render_mode_changed alpha compare - on: %s\n",
            str_yn[(gDP.otherMode.l & CULL_FRONT)?1:0]);
      rdp.render_mode_changed &= ~CULL_FRONT;
      g_gdp.flags |= UPDATE_ALPHA_COMPARE;

      if (gDP.otherMode.l & CULL_FRONT)
         rdp.flags |= ALPHA_COMPARE;
      else
         rdp.flags &= ~ALPHA_COMPARE;
   }

   if (rdp.render_mode_changed & CULL_BACK) // alpha cvg sel
   {
      FRDP (" |- render_mode_changed alpha cvg sel - on: %s\n",
            str_yn[(gDP.otherMode.l & CULL_BACK)?1:0]);
      rdp.render_mode_changed &= ~CULL_BACK;
      g_gdp.flags |= UPDATE_COMBINE;
      g_gdp.flags |= UPDATE_ALPHA_COMPARE;
   }

   // Force blend
   if (rdp.render_mode_changed & 0xFFFF0000)
   {
      FRDP (" |- render_mode_changed force_blend - %08lx\n", gDP.otherMode.l&0xFFFF0000);
      rdp.render_mode_changed &= 0x0000FFFF;
      g_gdp.flags |= UPDATE_COMBINE;
   }

   // Combine MUST go before texture
   if ((g_gdp.flags & UPDATE_COMBINE) && rdp.allow_combine)
   {

      LRDP (" |-+ update_combine\n");
      Combine ();
   }

   if (g_gdp.flags & UPDATE_TEXTURE)  // note: UPDATE_TEXTURE and UPDATE_COMBINE are the same
   {
      rdp.tex_ctr ++;
      if (rdp.tex_ctr == 0xFFFFFFFF)
         rdp.tex_ctr = 0;

      TexCache ();
      if (rdp.noise == NOISE_MODE_NONE)
         g_gdp.flags ^= UPDATE_TEXTURE;
   }

   if (g_gdp.flags & UPDATE_ZBUF_ENABLED)
      glide64_z_compare();

   // Alpha compare
   if (g_gdp.flags & UPDATE_ALPHA_COMPARE)
   {
      g_gdp.flags ^= UPDATE_ALPHA_COMPARE;

      if (gDP.otherMode.alphaCompare == 1 && !(gDP.otherMode.alphaCvgSel) && (!gDP.otherMode.forceBlender || (g_gdp.blend_color.a)))
      {
         uint8_t reference = (uint8_t)g_gdp.blend_color.a;
         grAlphaTestFunction (reference ? GR_CMP_GEQUAL : GR_CMP_GREATER, reference, 1);
         FRDP (" |- alpha compare: blend: %02lx\n", reference);
      }
      else
      {
         if (rdp.flags & ALPHA_COMPARE)
         {
            bool cond_set = (gDP.otherMode.l & 0x5000) == 0x5000;
            grAlphaTestFunction (!cond_set ? GR_CMP_GEQUAL : GR_CMP_GREATER, 0x20, !cond_set ? 1 : 0);
            if (cond_set)
               grAlphaTestReferenceValue ((gDP.otherMode.alphaCompare == 3) ? (uint8_t)g_gdp.blend_color.a : 0x00);
         }
         else
         {
            grAlphaTestFunction (GR_CMP_ALWAYS, 0x00, 0);
            LRDP (" |- alpha compare: none\n");
         }
      }
      if (gDP.otherMode.alphaCompare == 3 && ((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) < G_CYC_COPY)
      {
         if (settings.old_style_adither || g_gdp.other_modes.alpha_dither_sel != 3)
         {
            LRDP (" |- alpha compare: dither\n");
            grStippleMode(settings.stipple_mode);
         }
         else
            grStippleMode(GR_STIPPLE_DISABLE);
      }
      else
      {
         //LRDP (" |- alpha compare: dither disabled\n");
         grStippleMode(GR_STIPPLE_DISABLE);
      }
   }

   // Cull mode (leave this in for z-clipped triangles)
   if (g_gdp.flags & UPDATE_CULL_MODE)
   {
      uint32_t mode;
      g_gdp.flags ^= UPDATE_CULL_MODE;
      mode = (rdp.flags & G_CULL_BOTH) >> CULLSHIFT;
      FRDP (" |- cull_mode - mode: %s\n", str_cull[mode]);
      switch (mode)
      {
         case 0: // cull none
         case 3: // cull both
            grCullMode(GR_CULL_DISABLE);
            break;
         case 1: // cull front
            grCullMode(GR_CULL_NEGATIVE);
            break;
         case 2: // cull back
            grCullMode (GR_CULL_POSITIVE);
            break;
      }
   }

   //Added by Gonetz.
   if (settings.fog && (g_gdp.flags & UPDATE_FOG_ENABLED))
   {
      uint16_t blender = (uint16_t)(gDP.otherMode.l >> 16);
      g_gdp.flags ^= UPDATE_FOG_ENABLED;

      if (rdp.flags & FOG_ENABLED)
      {
         if((gSP.fog.multiplier > 0) && (gDP.otherMode.c1_m1a==3 || gDP.otherMode.c1_m2a == 3 || gDP.otherMode.c2_m1a == 3 || gDP.otherMode.c2_m2a == 3))
         {
            grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT, g_gdp.fog_color.total);
            rdp.fog_mode = FOG_MODE_ENABLED;
            LRDP("fog enabled \n");
         }
         else
         {
            LRDP("fog disabled in blender\n");
            rdp.fog_mode = FOG_MODE_DISABLED;
            grFogMode (GR_FOG_DISABLE, g_gdp.fog_color.total);
         }
      }
      else if (blender == 0xc410 || blender == 0xc411 || blender == 0xf500)
      {
         grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT, g_gdp.fog_color.total);
         rdp.fog_mode = FOG_MODE_BLEND;
         LRDP("fog blend \n");
      }
      else if (blender == 0x04d1)
      {
         grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT, g_gdp.fog_color.total);
         rdp.fog_mode = FOG_MODE_BLEND_INVERSE;
         LRDP("fog blend \n");
      }
      else
      {
         LRDP("fog disabled\n");
         rdp.fog_mode = FOG_MODE_DISABLED;
         grFogMode (GR_FOG_DISABLE, g_gdp.fog_color.total);
      }
   }

   if (g_gdp.flags & UPDATE_VIEWPORT)
   {
      g_gdp.flags ^= UPDATE_VIEWPORT;
      {
         float scale_x = (float)fabs(gSP.viewport.vscale[0]);
         float scale_y = (float)fabs(gSP.viewport.vscale[1]);

         rdp.clip_min_x = MAX((gSP.viewport.vtrans[0] - scale_x + rdp.offset_x) / rdp.clip_ratio, 0.0f);
         rdp.clip_min_y = MAX((gSP.viewport.vtrans[1] - scale_y + rdp.offset_y) / rdp.clip_ratio, 0.0f);
         rdp.clip_max_x = MIN((gSP.viewport.vtrans[0] + scale_x + rdp.offset_x) * rdp.clip_ratio, settings.res_x);
         rdp.clip_max_y = MIN((gSP.viewport.vtrans[1] + scale_y + rdp.offset_y) * rdp.clip_ratio, settings.res_y);

         FRDP (" |- viewport - (%d, %d, %d, %d)\n", (uint32_t)rdp.clip_min_x, (uint32_t)rdp.clip_min_y, (uint32_t)rdp.clip_max_x, (uint32_t)rdp.clip_max_y);
         if (!rdp.scissor_set)
         {
            g_gdp.flags |= UPDATE_SCISSOR;
            set_scissor = true;
         }
      }
   }

   update_scissor(set_scissor);
}

static void draw_tri_depth(VERTEX **vtx)
{
   float X0 = vtx[0]->sx / rdp.scale_x;
   float Y0 = vtx[0]->sy / rdp.scale_y;
   float X1 = vtx[1]->sx / rdp.scale_x;
   float Y1 = vtx[1]->sy / rdp.scale_y;
   float X2 = vtx[2]->sx / rdp.scale_x;
   float Y2 = vtx[2]->sy / rdp.scale_y;
   float diffy_02 = Y0 - Y2;
   float diffy_12 = Y1 - Y2;
   float diffx_02 = X0 - X2;
   float diffx_12 = X1 - X2;
   float denom = (diffx_02 * diffy_12 - diffx_12 * diffy_02);

   if(denom * denom > 0.0)
   {
      float diffz_02 = vtx[0]->sz - vtx[2]->sz;
      float diffz_12 = vtx[1]->sz - vtx[2]->sz;
      float fdzdx = (diffz_02 * diffy_12 - diffz_12 * diffy_02) / denom;

      if ((rdp.rm & ZMODE_DECAL) == ZMODE_DECAL)
      {
         /* Calculate deltaZ per polygon for Decal z-mode */
         float fdzdy = (float)((diffz_02*diffx_12 - diffz_12*diffx_02) / denom);
         float fdz   = (float)(fabs(fdzdx) + fabs(fdzdy));
         deltaZ      = MAX(8, (int)fdz);
      }
      dzdx = (int)(fdzdx * 65536.0);
   }
}

/* clip_w - clips aint the z-axis */
static void clip_w (void)
{
   int i;
   int index = 0;
   int n = rdp.n_global;
   VERTEX *tmp = (VERTEX*)rdp.vtxbuf2;

   /* Swap vertex buffers */
   rdp.vtxbuf2 = rdp.vtxbuf;
   rdp.vtxbuf = tmp;
   rdp.vtx_buffer ^= 1;

   // Check the vertices for clipping
   for (i=0; i < n; i++)
   {
      bool save_inpoint = false;
      VERTEX *first, *second, *current = NULL, *current2 = NULL;
      int j = i+1;
      if (j == 3)
         j = 0;
      first = (VERTEX*)&rdp.vtxbuf2[i];
      second = (VERTEX*)&rdp.vtxbuf2[j];

      if (first->w >= 0.01f)
      {
         if (second->w >= 0.01f)    // Both are in, save the last one
         {
            save_inpoint = true;
         }
         else      // First is in, second is out, save intersection
         {
            current  = first;
            current2 = second;
         }
      }
      else
      {
         if (second->w >= 0.01f)  // First is out, second is in, save intersection & in point
         {
            current  = second;
            current2 = first;

            save_inpoint = true;
         }
      }

      if (current && current2)
      {
         float percent = (-current->w) / (current2->w - current->w);
         rdp.vtxbuf[index].not_zclipped = 0;
         rdp.vtxbuf[index].x = current->x + (current2->x - current->x) * percent;
         rdp.vtxbuf[index].y = current->y + (current2->y - current->y) * percent;
         rdp.vtxbuf[index].z = current->z + (current2->z - current->z) * percent;
         rdp.vtxbuf[index].w = settings.depth_bias * 0.01f;

         clip_tri_uv(current, current2, index, percent);
         clip_tri_interp_colors(first, second, index, percent, 0, 0);
      }

      if (save_inpoint)
      {
         // Save the in point
         rdp.vtxbuf[index] = rdp.vtxbuf2[j];
         rdp.vtxbuf[index++].not_zclipped = 1;
      }
   }
   rdp.n_global = index;
}

static int cull_tri(VERTEX **v) // type changed to VERTEX** [Dave2001]
{
   int i, draw, iarea;
   unsigned int mode;
   float x1, y1, x2, y2, area;

   if (v[0]->scr_off & v[1]->scr_off & v[2]->scr_off)
      return true;

   // Triangle can't be culled, if it need clipping
   draw = false;

   for (i=0; i<3; i++)
   {
      if (!v[i]->screen_translated)
      {
         v[i]->sx = gSP.viewport.vtrans[0] + v[i]->x_w * gSP.viewport.vscale[0] + rdp.offset_x;
         v[i]->sy = gSP.viewport.vtrans[1] + v[i]->y_w * gSP.viewport.vscale[1] + rdp.offset_y;
         v[i]->sz = gSP.viewport.vtrans[2] + v[i]->z_w * gSP.viewport.vscale[2];
         v[i]->screen_translated = 1;
      }
      if (v[i]->w < 0.01f) //need clip_z. can't be culled now
         draw = 1;
   }

   rdp.u_cull_mode = (rdp.flags & G_CULL_BOTH);
   if (draw || rdp.u_cull_mode == 0 || rdp.u_cull_mode == G_CULL_BOTH) //no culling set
   {
      rdp.u_cull_mode >>= CULLSHIFT;
      return false;
   }

   x1 = v[0]->sx - v[1]->sx;
   y1 = v[0]->sy - v[1]->sy;
   x2 = v[2]->sx - v[1]->sx;
   y2 = v[2]->sy - v[1]->sy;
   area = y1 * x2 - x1 * y2;
   iarea = *(int*)&area;

   mode = (rdp.u_cull_mode << 19UL);
   rdp.u_cull_mode >>= CULLSHIFT;

   if ((iarea & 0x7FFFFFFF) == 0)
   {
      //LRDP (" zero area triangles\n");
      return true;
   }

   if ((rdp.flags & G_CULL_BOTH) && ((int)(iarea ^ mode)) >= 0)
   {
      //LRDP (" culled\n");
      return true;
   }

   return false;
}

static void draw_tri_uv_calculation(VERTEX **vtx, VERTEX *v)
{
   unsigned i;
   //FRDP(" * CALCULATING VERTEX U/V: %d\n", v->number);

   if (!(gSP.geometryMode & G_LIGHTING))
   {
      if (!(gSP.geometryMode & UPDATE_SCISSOR))
      {
         if (gSP.geometryMode & G_SHADE)
            glideSetVertexFlatShading(v, vtx, __RSP.w1);
         else
            glideSetVertexPrimShading(v, g_gdp.prim_color.total);
      }
   }

   // Fix texture coordinates
   if (!v->uv_scaled)
   {
      v->ou        *= gSP.texture.scales;
      v->ov        *= gSP.texture.scalet;
      v->uv_scaled  = 1;

      if (!gDP.otherMode.texturePersp)
      {
         v->ou *= 0.5f;
         v->ov *= 0.5f;
      }
   }
   v->u[1] = v->u[0] = v->ou;
   v->v[1] = v->v[0] = v->ov;

   for (i = 0; i < 2; i++)
   {
      unsigned index = i+1;
      if (rdp.tex >= index && rdp.cur_cache[i])
         draw_tri_uv_calculation_update_shift(rdp.cur_tile+i, i, v);
   }

   v->uv_calculated = rdp.tex_ctr;
}

static void draw_tri (VERTEX **vtx, uint16_t linew)
{
   int i;

   org_vtx = vtx;

   for (i = 0; i < 3; i++)
   {
      VERTEX *v = (VERTEX*)vtx[i];

      if (v->uv_calculated != rdp.tex_ctr)
         draw_tri_uv_calculation(vtx, v);
      if (v->shade_mod != cmb.shade_mod_hash)
         apply_shade_modulation(v);
   }

   rdp.clip = 0;

   vtx[0]->not_zclipped = vtx[1]->not_zclipped = vtx[2]->not_zclipped = 1;

   // Set vertex buffers
   rdp.vtxbuf = rdp.vtx1;  // copy from v to rdp.vtx1
   rdp.vtxbuf2 = rdp.vtx2;
   rdp.vtx_buffer = 0;
   rdp.n_global = 3;

   rdp.vtxbuf[0] = *vtx[0];
   rdp.vtxbuf[0].number = 1;
   rdp.vtxbuf[1] = *vtx[1];
   rdp.vtxbuf[1].number = 2;
   rdp.vtxbuf[2] = *vtx[2];
   rdp.vtxbuf[2].number = 4;

   if ((vtx[0]->scr_off & 16) ||
         (vtx[1]->scr_off & 16) ||
         (vtx[2]->scr_off & 16))
      clip_w();

   do_triangle_stuff (linew, false);
}

void cull_trianglefaces(VERTEX **v, unsigned iterations, bool do_update, bool do_cull, int32_t wd)
{
   uint32_t i;
   int32_t vcount = 0;

   if (do_update)
      update();

   for (i = 0; i < iterations; i++, vcount += 3)
   {
      if (do_cull)
         if (cull_tri(v + vcount))
            continue;

      deltaZ = dzdx = 0;
      if (wd == 0 && (fb_depth_render_enabled || (rdp.rm & ZMODE_DECAL) == ZMODE_DECAL))
         draw_tri_depth(v + vcount);
      draw_tri (v + vcount, wd);
   }
}
