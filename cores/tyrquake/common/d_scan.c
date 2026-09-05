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
/* d_scan.c */
/**/
/* Portable C scan-level rasterization code, all pixel depths. */

#include <stdint.h>

/* SIMD vector paths for the no-gather inner loops in this file.
 * SSE2 is baseline on x86-64; NEON is baseline on AArch64 and
 * opt-in on ARMv7 (-mfpu=neon).  Everything else stays on the
 * scalar fallback.  These are guarded per-function: only the
 * D_DrawZSpans pure-arithmetic loop has been measured to win
 * under SSE2/NEON.  D_DrawSpans16 (texel gather) was measured
 * and is a LOSS under SIMD on both ISAs -- the byte gather
 * dominates and SIMD extract overhead beats any address-math
 * win -- so it stays scalar. */
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
#define DSCAN_SSE2 1
#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#define DSCAN_NEON 1
#include <arm_neon.h>
#endif

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

unsigned char *r_turb_pbase, *r_turb_pdest;
fixed16_t r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
int *r_turb_turb;
int r_turb_spancount;

/* Liquid blending state, set per-surface by the caller before
 * Turbulent8() runs.  r_turb_alpha is the desired opacity 0..255
 * (255 = fully opaque, 0 = fully invisible).  r_turb_blendmode
 * selects the rendering style: 0 = off (always opaque,
 * vanilla-equivalent), 1 = stipple (4x4 Bayer threshold vs
 * alpha), reserved values (2+) for future colormap-blend modes
 * which fall back to opaque until implemented.
 * r_turb_screen_x/y track the screen position of r_turb_pdest so
 * the stipple pattern indexes by screen-pixel rather than
 * texture-pixel coordinates. */
int r_turb_alpha     = 255;
int r_turb_blendmode = 0;
int r_turb_screen_x;
int r_turb_screen_y;

/* Z-test state for pass-2 (translucent liquid) rasterization.
 * In single-pass mode this is unused -- the edge rasterizer's
 * BSP ordering already handles occlusion.  But pass 2's edge
 * list contains ONLY liquid surfaces, so it has no opaque
 * occluders to clip behind walls/floors that block the view of
 * the liquid.  Without an explicit z-test, lava in a closed
 * room would smear across the player's foreground floor.
 *
 * r_turb_izi:     starting izi (1/z * 0x80000000) for the span,
 *                 in the same fixed-point format the z-buffer
 *                 stores after the >>16 reduction.
 * r_turb_izistep: per-pixel step in u, same units.
 * r_turb_ztest:   set to 1 by D_DrawSurfaces when in pass 2;
 *                 D_DrawTurbulent8Span gates its z-comparison
 *                 on this so the opaque single-pass path stays
 *                 byte-identical to vanilla. */
int r_turb_izi;
int r_turb_izistep;
int r_turb_ztest = 0;

void D_DrawTurbulent8Span(void);

/*
=============
D_WarpScreen

this performs a slight compression of the screen at the same time as
the sine warp, to keep the edges from wrapping
=============
*/
void
D_WarpScreen(void)
{
   int u, v;
   byte *dest;
   int *turb;
   byte **row;
   int *column;

   int w = r_refdef.vrect.width;
   int h = r_refdef.vrect.height;

   float wratio = w / (float)scr_vrect.width;
   float hratio = h / (float)scr_vrect.height;

   /* Cached row pointer / column tables. The values only depend on the
    * current refdef/screen rectangles, so rebuild the tables only when
    * those change. This avoids two malloc()/free() pairs every frame
    * while a turbulent (water/lava/slime/teleport) post-process is
    * active. */
   static byte **rowptr;
   static int  *column_cache;
   static int   cached_w, cached_h;
   static int   cached_scr_w, cached_scr_h;
   static int   cached_vrect_x, cached_vrect_y;
   static int   cached_screenwidth;

   int screen_h = scr_vrect.height + TURB_SCREEN_AMP * 2;
   int screen_w = scr_vrect.width  + TURB_SCREEN_AMP * 2;

   if (rowptr == NULL
       || cached_w != w || cached_h != h
       || cached_scr_w != scr_vrect.width  || cached_scr_h != scr_vrect.height
       || cached_vrect_x != r_refdef.vrect.x || cached_vrect_y != r_refdef.vrect.y
       || cached_screenwidth != screenwidth)
   {
      free(rowptr);
      free(column_cache);
      rowptr       = (byte **)malloc(screen_h * sizeof(byte *));
      column_cache = (int   *)malloc(screen_w * sizeof(int));

      for (v = 0; v < screen_h; v++)
      {
         rowptr[v] = d_viewbuffer + (r_refdef.vrect.y * screenwidth) +
            (screenwidth * (int)((float)v * hratio * h /
                                 (h + TURB_SCREEN_AMP * 2)));
      }
      for (u = 0; u < screen_w; u++)
      {
         column_cache[u] = r_refdef.vrect.x +
            (int)((float)u * wratio * w / (w + TURB_SCREEN_AMP * 2));
      }

      cached_w = w; cached_h = h;
      cached_scr_w = scr_vrect.width; cached_scr_h = scr_vrect.height;
      cached_vrect_x = r_refdef.vrect.x; cached_vrect_y = r_refdef.vrect.y;
      cached_screenwidth = screenwidth;
   }
   column = column_cache;

   turb = intsintable + ((int)(cl.time * TURB_SPEED) & (TURB_CYCLE - 1));
   dest = vid.buffer + scr_vrect.y * vid.rowbytes + scr_vrect.x;

   for (v = 0; v < scr_vrect.height; v++, dest += vid.rowbytes)
   {
      int *col = &column[turb[v & (TURB_CYCLE - 1)]];
      row = &rowptr[v];
      for (u = 0; u < scr_vrect.width; u += 4)
      {
         dest[u + 0] = row[turb[(u + 0) & (TURB_CYCLE - 1)]][col[u + 0]];
         dest[u + 1] = row[turb[(u + 1) & (TURB_CYCLE - 1)]][col[u + 1]];
         dest[u + 2] = row[turb[(u + 2) & (TURB_CYCLE - 1)]][col[u + 2]];
         dest[u + 3] = row[turb[(u + 3) & (TURB_CYCLE - 1)]][col[u + 3]];
      }
   }
}

/*
=============
D_DrawTurbulent8Span

Inner-loop rasterizer for liquid (water/lava/slime/teleport)
surfaces.  Sets each pixel to a sintable-warped sample of the
surface texture.

When r_turb_alpha < 255, applies stipple-pattern transparency
keyed off a 4x4 ordered Bayer matrix indexed by screen position:
each pixel is written only if its dither threshold is below the
desired alpha, leaving the existing framebuffer pixel visible
otherwise.  This is what users see as "translucent water" -- a
checker/dither mix of liquid texels and whatever was drawn
beneath the liquid surface (background world geometry).

When r_turb_alpha >= 255 the loop falls through to a write on
every pixel, identical to the original opaque path.
=============
*/
void
D_DrawTurbulent8Span(void)
{
   /* 4x4 Bayer dither matrix scaled to 0..255.  A pixel at screen
    * cell (cx, cy) is drawn iff bayer[cy & 3][cx & 3] < alpha,
    * giving 16 evenly-spaced dither thresholds per 4x4 block. */
   static const int bayer4[4][4] = {
      {   0, 128,  32, 160 },
      { 192,  64, 224,  96 },
      {  48, 176,  16, 144 },
      { 240, 112, 208,  80 }
   };
   const int alpha = r_turb_alpha;

   if (alpha >= 255 || r_turb_blendmode != 1) {
      if (!r_turb_ztest) {
         /* Vanilla opaque fast path: byte-for-byte identical to
          * the original tyrquake inner loop.  No z-test, no
          * stipple, no per-pixel z-buffer walk -- just the texel
          * write.  This is the path taken in single-pass mode
          * (r_liquidblend = 0 or all liquid alpha = 1.0), which
          * is the default and the common case. */
         do
         {
            int tturb;
            int sturb = r_turb_s + r_turb_turb[(r_turb_t >> 16) & (TURB_CYCLE - 1)];
            sturb = (sturb >> 16) & (TURB_TEX_SIZE - 1);
            tturb = r_turb_t + r_turb_turb[(r_turb_s >> 16) & (TURB_CYCLE - 1)];
            tturb = (tturb >> 16) & (TURB_TEX_SIZE - 1);
            *r_turb_pdest++ = *(r_turb_pbase + (tturb * TURB_TEX_SIZE) + sturb);
            r_turb_s += r_turb_sstep;
            r_turb_t += r_turb_tstep;
         } while (--r_turb_spancount > 0);
      } else {
         /* Pass-2 opaque-with-z-test path: liquid at alpha 1.0
          * but still rendered in the second pass (because some
          * OTHER liquid type has alpha < 1.0 -- the master gate
          * R_LiquidsAreTransparent enables two-pass mode whenever
          * ANY liquid is translucent).  The z-test here keeps
          * fully-opaque liquid from smearing across foreground
          * walls in pass 2 where the edge list lacks opaque
          * occluders. */
         short    *pzbuf   = d_pzbuffer + (d_zwidth * r_turb_screen_y) + r_turb_screen_x;
         int       izi     = r_turb_izi;
         const int izistep = r_turb_izistep;
         do
         {
            int tturb;
            int sturb = r_turb_s + r_turb_turb[(r_turb_t >> 16) & (TURB_CYCLE - 1)];
            sturb = (sturb >> 16) & (TURB_TEX_SIZE - 1);
            tturb = r_turb_t + r_turb_turb[(r_turb_s >> 16) & (TURB_CYCLE - 1)];
            tturb = (tturb >> 16) & (TURB_TEX_SIZE - 1);
            if ((short)(izi >> 16) >= *pzbuf)
               *r_turb_pdest = *(r_turb_pbase + (tturb * TURB_TEX_SIZE) + sturb);
            r_turb_pdest++;
            pzbuf++;
            izi += izistep;
            r_turb_s += r_turb_sstep;
            r_turb_t += r_turb_tstep;
         } while (--r_turb_spancount > 0);
      }
   } else {
      /* Stipple path: read texel as before but write only when
       * the dither threshold permits, advancing the screen-X
       * cursor regardless.  Pass 2 also gates on z so liquid
       * surfaces behind opaque walls/floors don't bleed through.
       * The z compare uses (izi >> 16) >= *pzbuf because the
       * z-buffer holds 1/z (larger = closer) and D_DrawZSpans
       * wrote it with the same shift; we want to draw iff the
       * liquid is at or in front of the existing surface.
       *
       * Stipple is only ever taken when blend mode is 1 AND
       * alpha < 255 -- both imply r_liquidblend != 0 AND a
       * liquid alpha < 1.0, so r_turb_ztest is guaranteed to be
       * 1 (we're in pass 2).  Vanilla fall-through is via the
       * fast path above when alpha >= 255. */
      int        cx      = r_turb_screen_x;
      const int  cy      = r_turb_screen_y & 3;
      const int *drow    = bayer4[cy];
      short     *pzbuf   = d_pzbuffer + (d_zwidth * r_turb_screen_y) + cx;
      int        izi     = r_turb_izi;
      const int  izistep = r_turb_izistep;
      do
      {
         int tturb;
         int sturb = r_turb_s + r_turb_turb[(r_turb_t >> 16) & (TURB_CYCLE - 1)];
         sturb = (sturb >> 16) & (TURB_TEX_SIZE - 1);
         tturb = r_turb_t + r_turb_turb[(r_turb_s >> 16) & (TURB_CYCLE - 1)];
         tturb = (tturb >> 16) & (TURB_TEX_SIZE - 1);
         if (drow[cx & 3] < alpha && (short)(izi >> 16) >= *pzbuf)
            *r_turb_pdest = *(r_turb_pbase + (tturb * TURB_TEX_SIZE) + sturb);
         r_turb_pdest++;
         pzbuf++;
         izi += izistep;
         cx++;
         r_turb_s += r_turb_sstep;
         r_turb_t += r_turb_tstep;
      } while (--r_turb_spancount > 0);
      r_turb_screen_x = cx;
   }
}

/*
=============
Turbulent8
=============
*/
void
Turbulent8(espan_t *pspan)
{
   fixed16_t snext, tnext;
   float sdivz16stepu, tdivz16stepu, zi16stepu;

   r_turb_turb = sintable + ((int)(cl.time * TURB_SPEED) & (TURB_CYCLE - 1));

   r_turb_sstep = 0;		/* keep compiler happy */
   r_turb_tstep = 0;		/* ditto */

   r_turb_pbase = (unsigned char *)cacheblock;

   sdivz16stepu = d_sdivzstepu * 16;
   tdivz16stepu = d_tdivzstepu * 16;
   zi16stepu = d_zistepu * 16;

   do
   {
      /* calculate the initial s/z, t/z, 1/z, s, and t and clamp */
      int count = pspan->count;
      float  du = (float)pspan->u;
      float  dv = (float)pspan->v;
      float sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
      float tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
      float zi    = d_ziorigin + dv * d_zistepv + du * d_zistepu;
      float z     = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */

      r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);
      r_turb_screen_x = pspan->u;
      r_turb_screen_y = pspan->v;
      r_turb_s     = (int)(sdivz * z) + sadjust;

      if (r_turb_s > bbextents)
         r_turb_s = bbextents;
      else if (r_turb_s < 0)
         r_turb_s = 0;

      r_turb_t = (int)(tdivz * z) + tadjust;
      if (r_turb_t > bbextentt)
         r_turb_t = bbextentt;
      else if (r_turb_t < 0)
         r_turb_t = 0;

      do
      {
         /* Publish the per-pixel z-interpolation state for the
          * stipple/z-test path in D_DrawTurbulent8Span before we
          * mutate zi for the next chunk.  r_turb_izi must match
          * the fixed-point format D_DrawZSpans uses so we can
          * compare against z-buffer values directly: izi after
          * the >>16 in the inner loop has the same scale as the
          * 16-bit z-buffer entries written by D_DrawZSpans.
          *
          * Only computed when r_turb_ztest is active (pass 2 of
          * the translucent-liquid pipeline).  Skipping the two
          * float-to-int conversions in single-pass mode keeps the
          * vanilla path's per-chunk overhead unchanged.
          *
          * Clamp zi before casting.  The expression
          * (int)(zi * 0x8000 * 0x10000) is (int)(zi * 2^31).  A
          * float value >= 2^31 cast to signed int is undefined
          * behaviour in C; on MSVC this can also raise the
          * _invalid_parameter handler ("Invalid parameter passed
          * to C runtime function") and on some FPU
          * configurations causes a hard crash.  Underwater play
          * regularly produces zi near or above 1.0 because the
          * camera intersects or grazes liquid surfaces (z very
          * small, zi = 1/z very large) -- this is the source of
          * the underwater crashes.
          *
          * Clamp to slightly below 1.0 so zi * 2^31 stays just
          * inside INT_MAX.  D_DrawZSpans has the same theoretical
          * vulnerability but happens not to be hit in practice
          * because vanilla gameplay rarely produces close-enough
          * geometry.  The pass-2 stipple path is more exposed
          * because liquid surfaces that the camera is right at
          * are exactly the geometry being rendered. */
         if (r_turb_ztest) {
            float zi_safe = zi;
            float zistepu_safe = d_zistepu;
            if (zi_safe >  0.999f) zi_safe =  0.999f;
            if (zi_safe < -0.999f) zi_safe = -0.999f;
            /* d_zistepu is per-pixel 1/z step; on legitimate
             * geometry this is tiny (~1e-6) but be defensive. */
            if (zistepu_safe >  0.001f) zistepu_safe =  0.001f;
            if (zistepu_safe < -0.001f) zistepu_safe = -0.001f;
            r_turb_izi     = (int)(zi_safe * (float)0x8000 * (float)0x10000);
            r_turb_izistep = (int)(zistepu_safe * (float)0x8000 * (float)0x10000);
         }

         /* calculate s and t at the far end of the span */
         if (count >= 16)
            r_turb_spancount = 16;
         else
            r_turb_spancount = count;

         count -= r_turb_spancount;

         if (count)
         {
            /* calculate s/z, t/z, zi->fixed s and t at far end of span, */
            /* calculate s and t steps across span by shifting */
            sdivz += sdivz16stepu;
            tdivz += tdivz16stepu;
            zi += zi16stepu;
            z = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */

            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext < 16)
               snext = 16;	/* prevent round-off error on <0 steps from */
            /*  from causing overstepping & running off the */
            /*  edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 16)
               tnext = 16;	/* guard against round-off error on <0 steps */

            r_turb_sstep = (snext - r_turb_s) >> 4;
            r_turb_tstep = (tnext - r_turb_t) >> 4;
         }
         else
         {
            /* calculate s/z, t/z, zi->fixed s and t at last pixel in span (so */
            /* can't step off polygon), clamp, calculate s and t steps across */
            /* span by division, biasing steps low so we don't run off the */
            /* texture */
            float spancountminus1 = (float)(r_turb_spancount - 1);
            sdivz += d_sdivzstepu * spancountminus1;
            tdivz += d_tdivzstepu * spancountminus1;
            zi += d_zistepu * spancountminus1;
            z = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */
            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext < 16)
               snext = 16;	/* prevent round-off error on <0 steps from */
            /*  from causing overstepping & running off the */
            /*  edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 16)
               tnext = 16;	/* guard against round-off error on <0 steps */

            if (r_turb_spancount > 1) {
               r_turb_sstep =
                  (snext - r_turb_s) / (r_turb_spancount - 1);
               r_turb_tstep =
                  (tnext - r_turb_t) / (r_turb_spancount - 1);
            }
         }

         r_turb_s = r_turb_s & ((TURB_CYCLE << 16) - 1);
         r_turb_t = r_turb_t & ((TURB_CYCLE << 16) - 1);

         D_DrawTurbulent8Span();

         r_turb_s = snext;
         r_turb_t = tnext;

      } while (count > 0);

   } while ((pspan = pspan->pnext) != NULL);
}

/*
   =============
   D_DrawSpans16

FIXME: actually make this subdivide by 16 instead of 8!!!  qb:  OK!!!!
=============
*/

/*==============================================
//unrolled- mh, MK, qbism
//============================================*/

static const int dither_kernel[2][2][2] = {
   { { 16384,     0 }, { 49152, 32768 } },
   { { 32768, 49152 }, {     0, 16384 } }
};

#define SOLID(i) pdest[i] = pbase[(s >> 16) + (t >> 16) * cachewidth]
#define DITHERED_SOLID(i) pdest[i] = pbase[idiths + iditht * cachewidth]
#define DITHERED_SOLID_B(i) pdest[i] = pbase[idiths_b + iditht_b * cachewidth]

#define DITHERED_SOLID_B_UPDATE() \
   idiths_b = (s + dither_val_s_b) >> 16; iditht_b = (t + dither_val_t_b) >> 16; \
idiths_b = idiths_b ? ((idiths_b) - 1) : idiths_b; \
iditht_b = iditht_b ? ((iditht_b) - 1) : iditht_b

#define DITHERED_SOLID_UPDATE() \
   idiths = (s + dither_val_s) >> 16; iditht = (t + dither_val_t) >> 16; \
idiths = idiths ? ((idiths) - 1) : idiths; \
iditht = iditht ? ((iditht) - 1) : iditht

/* qbism: pointer to pbase and macroize idea from mankrip */
#define WRITEPDEST(i) { pdest[i] = *(pbase + (s >> 16) + (t >> 16) * cachewidth); s+=sstep; t+=tstep;}

extern surfcache_t		*pcurrentcache;

void D_DrawSpans16Qb(espan_t *pspan) /* qb: up it from 8 to 16.  This + unroll = big speed gain! */
{
   int count, spancount;
   byte *pbase, *pdest;
   fixed16_t s, t, snext, tnext, sstep, tstep;
   float sdivz, tdivz, zi, z, du, dv, spancountminus1;
   float sdivzstepu, tdivzstepu, zistepu;

   sstep = 0;   /* keep compiler happy */
   tstep = 0;   /* ditto */

   pbase = (byte *)cacheblock;
   sdivzstepu = d_sdivzstepu * 16;
   tdivzstepu = d_tdivzstepu * 16;
   zistepu = d_zistepu * 16;

   do
   {
      pdest = (byte *)((byte *)d_viewbuffer + (screenwidth * pspan->v) + pspan->u);
      count = pspan->count >> 4;

      spancount = pspan->count % 16;

      /* calculate the initial s/z, t/z, 1/z, s, and t and clamp */
      du = (float)pspan->u;
      dv = (float)pspan->v;

      sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
      tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
      zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
      z = (float)0x10000 / zi;   /* prescale to 16.16 fixed-point */

      s = (int)(sdivz * z) + sadjust;
      if (s < 0) s = 0;
      else if (s > bbextents) s = bbextents;

      t = (int)(tdivz * z) + tadjust;
      if (t < 0) t = 0;
      else if (t > bbextentt) t = bbextentt;

      while (count-- > 0) /* Manoel Kasimier */
      {
         sdivz += sdivzstepu;
         tdivz += tdivzstepu;
         zi += zistepu;
         z = (float)0x10000 / zi;   /* prescale to 16.16 fixed-point */

         snext = (int)(sdivz * z) + sadjust;
         if (snext < 16) snext = 16;
         else if (snext > bbextents) snext = bbextents;

         tnext = (int)(tdivz * z) + tadjust;
         if (tnext < 16) tnext = 16;
         else if (tnext > bbextentt) tnext = bbextentt;

         sstep = (snext - s) >> 4;
         tstep = (tnext - t) >> 4;
         pdest += 16;

         WRITEPDEST(-16);
         WRITEPDEST(-15);
         WRITEPDEST(-14);
         WRITEPDEST(-13);
         WRITEPDEST(-12);
         WRITEPDEST(-11);
         WRITEPDEST(-10);
         WRITEPDEST(-9);
         WRITEPDEST(-8);
         WRITEPDEST(-7);
         WRITEPDEST(-6);
         WRITEPDEST(-5);
         WRITEPDEST(-4);
         WRITEPDEST(-3);
         WRITEPDEST(-2);
         WRITEPDEST(-1);

         s = snext;
         t = tnext;
      }
      if (spancount > 0)
      {
         spancountminus1 = (float)(spancount - 1);
         sdivz += d_sdivzstepu * spancountminus1;
         tdivz += d_tdivzstepu * spancountminus1;
         zi += d_zistepu * spancountminus1;
         z = (float)0x10000 / zi;   /* prescale to 16.16 fixed-point */

         snext = (int)(sdivz * z) + sadjust;
         if (snext < 16) snext = 16;
         else if (snext > bbextents) snext = bbextents;

         tnext = (int)(tdivz * z) + tadjust;
         if (tnext < 16) tnext = 16;
         else if (tnext > bbextentt) tnext = bbextentt;

         if (spancount > 1)
         {
            sstep = (snext - s) / (spancount - 1);
            tstep = (tnext - t) / (spancount - 1);
         }

         pdest += spancount;

         switch (spancount)
         {
            case 16:
               WRITEPDEST(-16);
            case 15:
               WRITEPDEST(-15);
            case 14:
               WRITEPDEST(-14);
            case 13:
               WRITEPDEST(-13);
            case 12:
               WRITEPDEST(-12);
            case 11:
               WRITEPDEST(-11);
            case 10:
               WRITEPDEST(-10);
            case  9:
               WRITEPDEST(-9);
            case  8:
               WRITEPDEST(-8);
            case  7:
               WRITEPDEST(-7);
            case  6:
               WRITEPDEST(-6);
            case  5:
               WRITEPDEST(-5);
            case  4:
               WRITEPDEST(-4);
            case  3:
               WRITEPDEST(-3);
            case  2:
               WRITEPDEST(-2);
            case  1:
               WRITEPDEST(-1);
               break;
         }
      }
   } while ((pspan = pspan->pnext) != NULL);
}

void D_DrawSpans16QbDither (espan_t *pspan) /* qbism up it from 8 to 16. This + unroll = big speed gain! */
{
   int spancount;
   uint8_t *pbase;
   fixed16_t snext, tnext, sstep, tstep;
   float sdivzstepu, tdivzstepu, zistepu;

   /* mipmaps shouldn't be dithered */
   if (pcurrentcache->mipscale < 1.0f)
   {
      D_DrawSpans16Qb(pspan);
      return;
   }

   sstep = 0; /* keep compiler happy */
   tstep = 0; /* ditto */

   pbase      = (uint8_t*)cacheblock;
   sdivzstepu = d_sdivzstepu * 16;
   tdivzstepu = d_tdivzstepu * 16;
   zistepu    = d_zistepu * 16;

   do
   {
      fixed16_t t;
      int count = pspan->count;
      /* calculate the initial s/z, t/z, 1/z, s, and t and clamp */
      float du = (float)pspan->u;
      float dv = (float)pspan->v;

      float sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
      float tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
      float zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
      float z = (float)0x10000 / zi; /* prescale to 16.16 fixed-point */

      fixed16_t s = (int)(sdivz * z) + sadjust;

      uint8_t *pdest = (uint8_t*)((byte *)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);

      if (s > bbextents)
         s = bbextents;
      else if (s < 0)
         s = 0;

      t = (int)(tdivz * z) + tadjust;
      if (t > bbextentt)
         t = bbextentt;
      else if (t < 0)
         t = 0;

      do
      {
         /* calculate s and t at the far end of the span */
         if (count >= 16)
            spancount = 16;
         else
            spancount = count;

         count -= spancount;

         if (count)
         {
            /* calculate s/z, t/z, zi->fixed s and t at far end of span, */
            /* calculate s and t steps across span by shifting */
            sdivz += sdivzstepu;
            tdivz += tdivzstepu;
            zi += zistepu;
            z = (float)0x10000 / zi; /* prescale to 16.16 fixed-point */

            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext <= 16)
               snext = 16; /* prevent round-off error on <0 steps from */
            /* from causing overstepping & running off the */
            /* edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 16)
               tnext = 16; /* guard against round-off error on <0 steps */

            sstep = (snext - s) >> 4;
            tstep = (tnext - t) >> 4;
         }
         else
         {
            /* calculate s/z, t/z, zi->fixed s and t at last pixel in span (so */
            /* can't step off polygon), clamp, calculate s and t steps across */
            /* span by division, biasing steps low so we don't run off the */
            /* texture */
            float spancountminus1 = (float)(spancount - 1);
            sdivz += d_sdivzstepu * spancountminus1;
            tdivz += d_tdivzstepu * spancountminus1;
            zi += d_zistepu * spancountminus1;
            z = (float)0x10000 / zi; /* prescale to 16.16 fixed-point */
            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext < 16)
               snext = 16; /* prevent round-off error on <0 steps from */
            /* from causing overstepping & running off the */
            /* edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 16)
               tnext = 16; /* guard against round-off error on <0 steps */

            if (spancount > 1)
            {
               sstep = (snext - s) / (spancount - 1);
               tstep = (tnext - t) / (spancount - 1);
            }
         }

         {
            int X = (pspan->u + spancount) & 1;
            int Y = (pspan->v) & 1;
            int dither_val_s = dither_kernel[X][Y][0];
            int dither_val_t = dither_kernel[X][Y][1];
            int dither_val_s_b = dither_kernel[!X][Y][0];
            int dither_val_t_b = dither_kernel[!X][Y][1];
            int idiths, iditht, idiths_b, iditht_b;

            DITHERED_SOLID_UPDATE();
            DITHERED_SOLID_B_UPDATE();

            /* qbism- Duff's Device loop unroll per mh. */
            pdest += spancount;
            switch (spancount)
            {
               case 16: DITHERED_SOLID(-16); s += sstep; t += tstep;
                        DITHERED_SOLID_B_UPDATE();
               case 15: DITHERED_SOLID_B(-15); s += sstep; t += tstep;
                        DITHERED_SOLID_UPDATE();
               case 14: DITHERED_SOLID(-14); s += sstep; t += tstep;
                        DITHERED_SOLID_B_UPDATE();
               case 13: DITHERED_SOLID_B(-13); s += sstep; t += tstep;
                        DITHERED_SOLID_UPDATE();
               case 12: DITHERED_SOLID(-12); s += sstep; t += tstep;
                        DITHERED_SOLID_B_UPDATE();
               case 11: DITHERED_SOLID_B(-11); s += sstep; t += tstep;
                        DITHERED_SOLID_UPDATE();
               case 10: DITHERED_SOLID(-10); s += sstep; t += tstep;
                        DITHERED_SOLID_B_UPDATE();
               case 9: DITHERED_SOLID_B(-9); s += sstep; t += tstep;
                       DITHERED_SOLID_UPDATE();
               case 8: DITHERED_SOLID(-8); s += sstep; t += tstep;
                       DITHERED_SOLID_B_UPDATE();
               case 7: DITHERED_SOLID_B(-7); s += sstep; t += tstep;
                       DITHERED_SOLID_UPDATE();
               case 6: DITHERED_SOLID(-6); s += sstep; t += tstep;
                       DITHERED_SOLID_B_UPDATE();
               case 5: DITHERED_SOLID_B(-5); s += sstep; t += tstep;
                       DITHERED_SOLID_UPDATE();
               case 4: DITHERED_SOLID(-4); s += sstep; t += tstep;
                       DITHERED_SOLID_B_UPDATE();
               case 3: DITHERED_SOLID_B(-3); s += sstep; t += tstep;
                       DITHERED_SOLID_UPDATE();
               case 2: DITHERED_SOLID(-2); s += sstep; t += tstep;
                       DITHERED_SOLID_B_UPDATE();
               case 1: DITHERED_SOLID_B(-1); s += sstep; t += tstep;
                       DITHERED_SOLID_UPDATE();
            }
         }

         s = snext;
         t = tnext;

      } while (count > 0);

   } while ((pspan = pspan->pnext) != NULL);
}

/*
=============
D_DrawSpans8
=============
*/
void
D_DrawSpans8(espan_t *pspan)
{
   fixed16_t sstep = 0;			/* keep compiler happy */
   fixed16_t tstep = 0;			/* ditto */

   unsigned char *pbase = (unsigned char *)cacheblock;
   float sdivz8stepu    = d_sdivzstepu * 8;
   float tdivz8stepu    = d_tdivzstepu * 8;
   float zi8stepu       = d_zistepu * 8;

   do
   {
      fixed16_t t;
      uint8_t *pdest = (uint8_t*)((byte *)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);

      int count = pspan->count;

      /* calculate the initial s/z, t/z, 1/z, s, and t and clamp */
      float du = (float)pspan->u;
      float dv = (float)pspan->v;

      float sdivz = d_sdivzorigin + dv * d_sdivzstepv + du * d_sdivzstepu;
      float tdivz = d_tdivzorigin + dv * d_tdivzstepv + du * d_tdivzstepu;
      float zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
      float z = (float)0x10000 / zi;	/* prescale to 16.16 fixed-point */

      fixed16_t s = (int)(sdivz * z) + sadjust;
      if (s > bbextents)
         s = bbextents;
      else if (s < 0)
         s = 0;

      t = (int)(tdivz * z) + tadjust;
      if (t > bbextentt)
         t = bbextentt;
      else if (t < 0)
         t = 0;

      do
      {
         fixed16_t snext, tnext;
         int spancount = count;
         /* calculate s and t at the far end of the span */
         if (count >= 8)
            spancount = 8;

         count -= spancount;

         if (count)
         {
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
         }
         else
         {
            /* calculate s/z, t/z, zi->fixed s and t at last pixel in span (so */
            /* can't step off polygon), clamp, calculate s and t steps across */
            /* span by division, biasing steps low so we don't run off the */
            /* texture */
            float spancountminus1 = (float)(spancount - 1);
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

         do
         {
            *pdest++ = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
            s += sstep;
            t += tstep;
         } while (--spancount > 0);

         s = snext;
         t = tnext;

      } while (count);

   } while ((pspan = pspan->pnext) != NULL);
}

/*
=============
D_DrawSpans
=============
*/

void D_DrawSpans16 (espan_t *pspan) /* qbism up it from 8 to 16.  This + unroll = big speed gain! */
{
   fixed16_t sstep = 0;   /* keep compiler happy */
   fixed16_t tstep = 0;   /* ditto */

   uint8_t *pbase = (uint8_t*)cacheblock;

   float sdivzstepu = d_sdivzstepu * 16;
   float tdivzstepu = d_tdivzstepu * 16;
   float zistepu    = d_zistepu * 16;

   do
   {
      fixed16_t t;
      uint8_t *pdest = (uint8_t*)((byte *)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);

      int count = pspan->count;

      /* calculate the initial s/z, t/z, 1/z, s, and t and clamp */
      float du = (float)pspan->u;
      float dv = (float)pspan->v;

      float sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
      float tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
      float zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
      float z = (float)0x10000 / zi;   /* prescale to 16.16 fixed-point */
      fixed16_t s = (int)(sdivz * z) + sadjust;

      if (s > bbextents)
         s = bbextents;
      else if (s < 0)
         s = 0;

      t = (int)(tdivz * z) + tadjust;
      if (t > bbextentt)
         t = bbextentt;
      else if (t < 0)
         t = 0;

      do
      {
         fixed16_t snext, tnext;
         int spancount = count;

         /* calculate s and t at the far end of the span */
         if (count >= 16)
            spancount = 16;

         count -= spancount;

         if (count)
         {
            /* calculate s/z, t/z, zi->fixed s and t at far end of span, */
            /* calculate s and t steps across span by shifting */
            sdivz += sdivzstepu;
            tdivz += tdivzstepu;
            zi += zistepu;
            z = (float)0x10000 / zi;   /* prescale to 16.16 fixed-point */

            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext <= 16)
               snext = 16;   /* prevent round-off error on <0 steps from */
            /*  from causing overstepping & running off the */
            /*  edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 16)
               tnext = 16;   /* guard against round-off error on <0 steps */

            sstep = (snext - s) >> 4;
            tstep = (tnext - t) >> 4;
         }
         else
         {
            /* calculate s/z, t/z, zi->fixed s and t at last pixel in span (so */
            /* can't step off polygon), clamp, calculate s and t steps across */
            /* span by division, biasing steps low so we don't run off the */
            /* texture */
            float spancountminus1 = (float)(spancount - 1);
            sdivz += d_sdivzstepu * spancountminus1;
            tdivz += d_tdivzstepu * spancountminus1;
            zi += d_zistepu * spancountminus1;
            z = (float)0x10000 / zi;   /* prescale to 16.16 fixed-point */
            snext = (int)(sdivz * z) + sadjust;
            if (snext > bbextents)
               snext = bbextents;
            else if (snext < 16)
               snext = 16;   /* prevent round-off error on <0 steps from */
            /*  from causing overstepping & running off the */
            /*  edge of the texture */

            tnext = (int)(tdivz * z) + tadjust;
            if (tnext > bbextentt)
               tnext = bbextentt;
            else if (tnext < 16)
               tnext = 16;   /* guard against round-off error on <0 steps */

            if (spancount > 1)
            {
               sstep = (snext - s) / (spancount - 1);
               tstep = (tnext - t) / (spancount - 1);
            }
         }

         do {
            *pdest++ = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
            s += sstep;
            t += tstep;
         } while (--spancount > 0);

         s = snext;
         t = tnext;

      } while (count > 0);

   } while ((pspan = pspan->pnext) != NULL);
}

/*
=============
D_DrawZSpans

The z-buffer holds (1/z * 2^31) values, truncated to int16
per pixel.  In real Quake content 1/z is in [2^19, 2^29],
always positive, so the (izi >> 16) sign extension never
sets the high 16 bits of the result.

The body is hand-unrolled to write two int16 values per
32-bit store in scalar form; the SSE2/NEON paths write
eight per 128-bit store.  Both vector ISAs measured ~2x
on synthetic 1080p-style span mixes (avg 160 px).
=============
*/
void
D_DrawZSpans(espan_t *pspan)
{
   /* FIXME: check for clamping/range problems */
   /* we count on FP exceptions being turned off to avoid range problems */
   int izistep = (int)(d_zistepu * 0x8000 * 0x10000);

   do
   {
      int16_t *pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;
      int count = pspan->count;

      /* calculate the initial 1/z */
      float du = (float)pspan->u;
      float dv = (float)pspan->v;

      double zi = d_ziorigin + dv * d_zistepv + du * d_zistepu;
      /* we count on FP exceptions being turned off to avoid range problems */
      int   izi = (int)(zi * 0x8000 * 0x10000);

      /* Head: align pdest to a 4-byte boundary so the
       * packed 32-bit stores below are aligned (they're
       * unaligned-tolerant on x86 but cheaper aligned). */
      if ((uintptr_t)pdest & 0x02)
      {
         *pdest++ = (short)(izi >> 16);
         izi += izistep;
         count--;
      }

#if defined(DSCAN_SSE2)
      /* 8 pixels per iter.  Two int32x4 vectors hold
       * {izi+0..3*step} and {izi+4..7*step}; PSRAD shifts
       * each lane right 16 (arithmetic), PACKSSDW narrows
       * two int32x4 into one int16x8 with signed saturate
       * (values already fit in int16 by construction, so
       * saturation never triggers).  PADDD advances by
       * 8*step. */
      if (count >= 8)
      {
         __m128i v0 = _mm_setr_epi32(izi + 0*izistep, izi + 1*izistep,
                                     izi + 2*izistep, izi + 3*izistep);
         __m128i v1 = _mm_setr_epi32(izi + 4*izistep, izi + 5*izistep,
                                     izi + 6*izistep, izi + 7*izistep);
         __m128i step8 = _mm_set1_epi32(izistep * 8);
         int batches = count >> 3;
         izi  += izistep * 8 * batches;
         count &= 7;
         do {
            __m128i hi0 = _mm_srai_epi32(v0, 16);
            __m128i hi1 = _mm_srai_epi32(v1, 16);
            _mm_storeu_si128((__m128i *)pdest, _mm_packs_epi32(hi0, hi1));
            pdest += 8;
            v0 = _mm_add_epi32(v0, step8);
            v1 = _mm_add_epi32(v1, step8);
         } while (--batches > 0);
      }
#elif defined(DSCAN_NEON)
      /* Same shape as SSE2.  gcc realizes that
       * vshrn_n_s32(v, 16) is equivalent to taking the
       * upper int16 of each int32 lane and emits a single
       * UZP2 to do the narrow.  Two int32x4 lanes -> one
       * int16x8 store. */
      if (count >= 8)
      {
         int32_t v0_init[4] = { izi + 0*izistep, izi + 1*izistep,
                                izi + 2*izistep, izi + 3*izistep };
         int32_t v1_init[4] = { izi + 4*izistep, izi + 5*izistep,
                                izi + 6*izistep, izi + 7*izistep };
         int32x4_t v0 = vld1q_s32(v0_init);
         int32x4_t v1 = vld1q_s32(v1_init);
         int32x4_t step8 = vdupq_n_s32(izistep * 8);
         int batches = count >> 3;
         izi  += izistep * 8 * batches;
         count &= 7;
         do {
            int16x4_t hi0 = vshrn_n_s32(v0, 16);
            int16x4_t hi1 = vshrn_n_s32(v1, 16);
            vst1q_s16(pdest, vcombine_s16(hi0, hi1));
            pdest += 8;
            v0 = vaddq_s32(v0, step8);
            v1 = vaddq_s32(v1, step8);
         } while (--batches > 0);
      }
#endif

      /* Scalar tail: pairs of pixels packed into a 32-bit
       * store, then one optional trailing pixel. */
      {
         int doublecount = count >> 1;
         if (doublecount > 0)
         {
            do
            {
#ifdef MSB_FIRST
               unsigned ltemp = izi & 0xFFFF0000;
               izi += izistep;
               ltemp |= izi >> 16;
#else
               unsigned ltemp = izi >> 16;
               izi += izistep;
               ltemp |= izi & 0xFFFF0000;
#endif
               izi += izistep;
               *(int *)pdest = ltemp;
               pdest += 2;
            } while (--doublecount > 0);
         }
         if (count & 1)
            *pdest = (short)(izi >> 16);
      }

   } while ((pspan = pspan->pnext));
}
