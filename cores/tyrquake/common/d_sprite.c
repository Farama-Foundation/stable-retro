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
/* d_sprite.c: software top-level rasterization driver module for drawing */
/* sprites */

#include <float.h>

#include "d_local.h"
#include "quakedef.h"
#include "render.h"
#include "rhi.h"

static int sprite_height;
static int minindex, maxindex;

/*
=====================
D_SpriteDrawSpans
=====================
*/
void
D_SpriteDrawSpans(sspan_t * pspan)
{
   int count, spancount;
   int izi;
   byte *pdest;
   fixed16_t s, t, snext, tnext;
   float sdivz, tdivz, zi, z, du, dv, spancountminus1;
   byte btemp;
   short *pz;

   fixed16_t sstep = 0;			/* keep compiler happy */
   fixed16_t tstep = 0;			/* ditto */

   byte *pbase = cacheblock;

   float sdivz8stepu = d_sdivzstepu * 8;
   float tdivz8stepu = d_tdivzstepu * 8;
   float zi8stepu = d_zistepu * 8;

   /* we count on FP exceptions being turned off to avoid range problems */
   int izistep = (int)(d_zistepu * 0x8000 * 0x10000);

   do {
      pdest = (byte *)d_viewbuffer + (screenwidth * pspan->v) + pspan->u;
      pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

      count = pspan->count;

      if (count <= 0)
         goto NextSpan;

      /* calculate the initial s/z, t/z, 1/z, s, and t and clamp */
      du = (float)pspan->u;
      dv = (float)pspan->v;

      sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
      tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
      zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
      z = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */
      /* we count on FP exceptions being turned off to avoid range problems */
      izi = (int)(zi * 0x8000 * 0x10000);

      s = (int)(sdivz * z) + sadjust;
      if (s > bbextents)
         s = bbextents;
      else if (s < 0)
         s = 0;

      t = (int)(tdivz * z) + tadjust;
      if (t > bbextentt)
         t = bbextentt;
      else if (t < 0)
         t = 0;

      do {
         /* calculate s and t at the far end of the span */
         if (count >= 8)
            spancount = 8;
         else
            spancount = count;

         count -= spancount;

         if (count) {
            /* calculate s/z, t/z, zi->fixed s and t at far end of span, */
            /* calculate s and t steps across span by shifting */
            sdivz += sdivz8stepu;
            tdivz += tdivz8stepu;
            zi += zi8stepu;
            z = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */

            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext < 8)
               snext = 8;	/* prevent round-off error on <0 steps from */
            /*  from causing overstepping & running off the */
            /*  edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 8)
               tnext = 8;	/* guard against round-off error on <0 steps */

            sstep = (snext - s) >> 3;
            tstep = (tnext - t) >> 3;
         } else {
            /* calculate s/z, t/z, zi->fixed s and t at last pixel in span (so */
            /* can't step off polygon), clamp, calculate s and t steps across */
            /* span by division, biasing steps low so we don't run off the */
            /* texture */
            spancountminus1 = (float)(spancount - 1);
            sdivz += d_sdivzstepu * spancountminus1;
            tdivz += d_tdivzstepu * spancountminus1;
            zi += d_zistepu * spancountminus1;
            z = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */
            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext < 8)
               snext = 8;	/* prevent round-off error on <0 steps from */
            /*  from causing overstepping & running off the */
            /*  edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 8)
               tnext = 8;	/* guard against round-off error on <0 steps */

            if (spancount > 1) {
               sstep = (snext - s) / (spancount - 1);
               tstep = (tnext - t) / (spancount - 1);
            }
         }

         {
            do {
               btemp = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
               if (btemp != 255) {
                  if (*pz <= (izi >> 16)) {
                     *pz = izi >> 16;
                     *pdest = btemp;
                  }
               }

               izi += izistep;
               pdest++;
               pz++;
               s += sstep;
               t += tstep;
            } while (--spancount > 0);
         }

         s = snext;
         t = tnext;

      } while (count > 0);

NextSpan:
      pspan++;

   } while (pspan->count != DS_SPAN_LIST_END);
}

/*
=====================
D_SpriteScanLeftEdge
=====================
*/
static void
D_SpriteScanLeftEdge(sspan_t * pspan)
{
   int lmaxindex;
   float vtop;
   int i = minindex;

   if (i == 0)
      i = r_spritedesc.nump;

   lmaxindex = maxindex;
   if (lmaxindex == 0)
      lmaxindex = r_spritedesc.nump;

   vtop = ceilf(r_spritedesc.pverts[i].v);

   do
   {
      emitpoint_t *pvert = &r_spritedesc.pverts[i];
      emitpoint_t *pnext = pvert - 1;
      float vbottom = ceilf(pnext->v);

      if (vtop < vbottom)
      {
         int v;
         float du = pnext->u - pvert->u;
         float dv = pnext->v - pvert->v;
         float slope = du / dv;
         fixed16_t u_step = (int)(slope * 0x10000);
         /* adjust u to ceil the integer portion */
         fixed16_t u = (int)((pvert->u + (slope * (vtop - pvert->v))) * 0x10000) +
            (0x10000 - 1);
         int itop = (int)vtop;
         int ibottom = (int)vbottom;

         for (v = itop; v < ibottom; v++)
         {
            pspan->u = u >> 16;
            pspan->v = v;
            u += u_step;
            pspan++;
         }
      }

      vtop = vbottom;

      i--;
      if (i == 0)
         i = r_spritedesc.nump;

   } while (i != lmaxindex);
}


/*
=====================
D_SpriteScanRightEdge
=====================
*/
static void
D_SpriteScanRightEdge(sspan_t * pspan)
{
   float vtop;
   int          i = minindex;
   float    vvert = r_spritedesc.pverts[i].v;

   if (vvert < r_refdef.fvrecty_adj)
      vvert = r_refdef.fvrecty_adj;
   if (vvert > r_refdef.fvrectbottom_adj)
      vvert = r_refdef.fvrectbottom_adj;

   vtop = ceilf(vvert);

   do
   {
      float vbottom;
      emitpoint_t *pvert = &r_spritedesc.pverts[i];
      emitpoint_t *pnext = pvert + 1;
      float vnext = pnext->v;

      if (vnext < r_refdef.fvrecty_adj)
         vnext = r_refdef.fvrecty_adj;
      if (vnext > r_refdef.fvrectbottom_adj)
         vnext = r_refdef.fvrectbottom_adj;

      vbottom = ceilf(vnext);

      if (vtop < vbottom)
      {
         fixed16_t u, u_step;
         int v;
         int itop, ibottom;
         float du, dv, unext, slope;
         float uvert = pvert->u;

         if (uvert < r_refdef.fvrectx_adj)
            uvert = r_refdef.fvrectx_adj;
         if (uvert > r_refdef.fvrectright_adj)
            uvert = r_refdef.fvrectright_adj;

         unext = pnext->u;
         if (unext < r_refdef.fvrectx_adj)
            unext = r_refdef.fvrectx_adj;
         if (unext > r_refdef.fvrectright_adj)
            unext = r_refdef.fvrectright_adj;

         du = unext - uvert;
         dv = vnext - vvert;
         slope = du / dv;
         u_step = (int)(slope * 0x10000);
         /* adjust u to ceil the integer portion */
         u = (int)((uvert + (slope * (vtop - vvert))) * 0x10000) +
            (0x10000 - 1);
         itop = (int)vtop;
         ibottom = (int)vbottom;

         for (v = itop; v < ibottom; v++)
         {
            pspan->count = (u >> 16) - pspan->u;
            u += u_step;
            pspan++;
         }
      }

      vtop = vbottom;
      vvert = vnext;

      i++;
      if (i == r_spritedesc.nump)
         i = 0;

   } while (i != maxindex);

   pspan->count = DS_SPAN_LIST_END;	/* mark the end of the span list */
}


/*
=====================
D_SpriteCalculateGradients
=====================
*/
static void
D_SpriteCalculateGradients(void)
{
   vec3_t p_normal, p_saxis, p_taxis, p_temp1;
   float distinv;

   TransformVector(r_spritedesc.vpn, p_normal);
   TransformVector(r_spritedesc.vright, p_saxis);
   TransformVector(r_spritedesc.vup, p_taxis);
   VectorInverse(p_taxis);

   distinv = 1.0 / (-DotProduct(modelorg, r_spritedesc.vpn));

   d_sdivzstepu = p_saxis[0] * xscaleinv;
   d_tdivzstepu = p_taxis[0] * xscaleinv;

   d_sdivzstepv = -p_saxis[1] * yscaleinv;
   d_tdivzstepv = -p_taxis[1] * yscaleinv;

   d_zistepu = p_normal[0] * xscaleinv * distinv;
   d_zistepv = -p_normal[1] * yscaleinv * distinv;

   d_sdivzorigin = p_saxis[2] - xcenter * d_sdivzstepu -
      ycenter * d_sdivzstepv;
   d_tdivzorigin = p_taxis[2] - xcenter * d_tdivzstepu -
      ycenter * d_tdivzstepv;
   d_ziorigin = p_normal[2] * distinv - xcenter * d_zistepu -
      ycenter * d_zistepv;

   TransformVector(modelorg, p_temp1);

   sadjust = ((fixed16_t)(DotProduct(p_temp1, p_saxis) * 0x10000 + 0.5)) -
      (-(cachewidth >> 1) << 16);
   tadjust = ((fixed16_t)(DotProduct(p_temp1, p_taxis) * 0x10000 + 0.5)) -
      (-(sprite_height >> 1) << 16);

   /* -1 (-epsilon) so we never wander off the edge of the texture */
   bbextents = (cachewidth << 16) - 1;
   bbextentt = (sprite_height << 16) - 1;
}


/*
=====================
D_DrawSprite
=====================
*/
void
D_DrawSprite(void)
{
   int i, nump;
   float ymin, ymax;
   emitpoint_t *pverts;
   /* Static buffer reused across all sprite draws. The previous code
    * called malloc(sizeof(sspan_t)*MAXHEIGHT+1) per sprite per frame,
    * which also had a precedence bug — the +1 was outside the
    * multiplication and only added one byte instead of one element. */
   static sspan_t spans[MAXHEIGHT + 1];

   /* Phase 5b-05: if the active backend exposes a GPU
    * sprite dispatch and compute rendering is enabled,
    * hand off the already-projected polygon
    * (r_spritedesc.pverts has nump 3..8 vertices with
    * u, v, s, t, zi -- exactly the rhi_sprite_vert_t
    * layout) and skip the CPU span generation +
    * D_SpriteDrawSpans entirely.
    *
    * The transparent index is hard-coded 255 in stock
    * Quake (TRANSPARENT_COLOR; matches the SW path's
    * `if (btemp != 255)` test in D_SpriteDrawSpans). */
   if (g_rhi
       && g_rhi->dispatch_3d_sprite
       && g_rhi_compute_rendering
       && r_spritedesc.nump >= 3
       && r_spritedesc.pspriteframe) {
      g_rhi->dispatch_3d_sprite(
         (const rhi_sprite_vert_t *)r_spritedesc.pverts,
         r_spritedesc.nump,
         (const byte *)&r_spritedesc.pspriteframe->rdata[0],
         r_spritedesc.pspriteframe->width,
         r_spritedesc.pspriteframe->height,
         255);
      return;
   }

   /* find the top and bottom vertices, and make sure there's at least one scan to */
   /* draw */
   ymin = FLT_MAX;
   ymax = -FLT_MAX;
   pverts = r_spritedesc.pverts;

   for (i = 0; i < r_spritedesc.nump; i++) {
      if (pverts->v < ymin) {
         ymin = pverts->v;
         minindex = i;
      }

      if (pverts->v > ymax) {
         ymax = pverts->v;
         maxindex = i;
      }

      pverts++;
   }

   ymin = ceilf(ymin);
   ymax = ceilf(ymax);

   if (ymin >= ymax)
      return;			/* doesn't cross any scans at all */

   cachewidth = r_spritedesc.pspriteframe->width;
   sprite_height = r_spritedesc.pspriteframe->height;
   cacheblock = (byte *)&r_spritedesc.pspriteframe->rdata[0];

   /* copy the first vertex to the last vertex, so we don't have to deal with */
   /* wrapping */
   nump = r_spritedesc.nump;
   pverts = r_spritedesc.pverts;
   pverts[nump] = pverts[0];

   D_SpriteCalculateGradients();
   D_SpriteScanLeftEdge(spans);
   D_SpriteScanRightEdge(spans);
   D_SpriteDrawSpans(spans);
}
