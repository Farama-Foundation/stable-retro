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
/* d_edge.c */

#include "d_local.h"
#include "quakedef.h"
#include "r_local.h"
#include "rhi.h"

#include "client.h"

static int miplevel;
static vec3_t transformed_modelorg;

float scale_for_mip;
int ubasestep, errorterm, erroradjustup, erroradjustdown;

/*
=============
D_MipLevelForScale
=============
*/
static int D_MipLevelForScale(float scale)
{
   int lmiplevel;

   if (scale >= d_scalemip[0])
      lmiplevel = 0;
   else if (scale >= d_scalemip[1])
      lmiplevel = 1;
   else if (scale >= d_scalemip[2])
      lmiplevel = 2;
   else
      lmiplevel = 3;

   if (lmiplevel < d_minmip)
      lmiplevel = d_minmip;

   return lmiplevel;
}


/*
==============
D_DrawSolidSurface
==============
*/

/* FIXME: clean this up */

static void D_DrawSolidSurface(surf_t *surf, int color)
{
   espan_t *span;
   /* Shift in unsigned to avoid UB when color has bit 7 set
    * ((color << 24) is 0x80000000 which doesn't fit in int32_t). */
   unsigned int upix = (unsigned int)(color & 0xff);
   int pix = (int)((upix << 24) | (upix << 16) | (upix << 8) | upix);

   for (span = surf->spans; span; span = span->pnext)
   {
      byte *pdest = (byte *)d_viewbuffer + screenwidth * span->v;
      int u = span->u;
      int u2 = span->u + span->count - 1;
      ((byte *)pdest)[u] = pix;

      if (u2 - u < 8)
      {
         for (u++; u <= u2; u++)
            ((byte *)pdest)[u] = pix;
      }
      else
      {
         for (u++; u & 3; u++)
            ((byte *)pdest)[u] = pix;

         u2 -= 4;
         for (; u <= u2; u += 4)
            *(int *)((byte *)pdest + u) = pix;
         u2 += 4;
         for (; u <= u2; u++)
            ((byte *)pdest)[u] = pix;
      }
   }
}


/*
==============
D_CalcGradients
==============
*/
static void D_CalcGradients(msurface_t *pface)
{
   vec3_t p_temp1;
   vec3_t p_saxis, p_taxis;
   float t;
   float mipscale = 1.0 / (float)(1 << miplevel);

   TransformVector(pface->texinfo->vecs[0], p_saxis);
   TransformVector(pface->texinfo->vecs[1], p_taxis);

   t = xscaleinv * mipscale;
   d_sdivzstepu = p_saxis[0] * t;
   d_tdivzstepu = p_taxis[0] * t;

   t = yscaleinv * mipscale;
   d_sdivzstepv = -p_saxis[1] * t;
   d_tdivzstepv = -p_taxis[1] * t;

   d_sdivzorigin = p_saxis[2] * mipscale - xcenter * d_sdivzstepu -
      ycenter * d_sdivzstepv;
   d_tdivzorigin = p_taxis[2] * mipscale - xcenter * d_tdivzstepu -
      ycenter * d_tdivzstepv;

   VectorScale(transformed_modelorg, mipscale, p_temp1);

   t = 0x10000 * mipscale;
   sadjust = ((fixed16_t)(DotProduct(p_temp1, p_saxis) * 0x10000 + 0.5)) -
      ((pface->texturemins[0] << 16) >> miplevel)
      + pface->texinfo->vecs[0][3] * t;
   tadjust = ((fixed16_t)(DotProduct(p_temp1, p_taxis) * 0x10000 + 0.5)) -
      ((pface->texturemins[1] << 16) >> miplevel)
      + pface->texinfo->vecs[1][3] * t;

   /* -1 (-epsilon) so we never wander off the edge of the texture */
   bbextents = ((pface->extents[0] << 16) >> miplevel) - 1;
   bbextentt = ((pface->extents[1] << 16) >> miplevel) - 1;
}


/*
==============
D_DrawSurfaces
==============
*/
surfcache_t *pcurrentcache;
void D_DrawSurfaces(void)
{
   surf_t *s;
   msurface_t *pface;
   vec3_t world_transformed_modelorg;
   vec3_t local_modelorg;
   const entity_t *e = &r_worldentity;

   TransformVector(modelorg, transformed_modelorg);
   VectorCopy(transformed_modelorg, world_transformed_modelorg);

   for (s = &surfaces[1]; s < surface_p; s++)
   {
      if (!s->spans)
         continue;

      r_drawnpolycount++;

      d_zistepu = s->d_zistepu;
      d_zistepv = s->d_zistepv;
      d_ziorigin = s->d_ziorigin;

      if (s->flags & SURF_DRAWSKY)
      {
         /* Phase 5b-07a: when the RHI exposes a sky-span
          * dispatch entry, queue each span for GPU compute
          * raster.  The SW D_DrawSkyScans8 is skipped in
          * that branch -- sky.comp writes directly into
          * vk_texture at end_frame, overwriting whatever
          * vid.buffer's sky pixels held (which is fine
          * because nothing else reads vid.buffer at those
          * pixels in the RHI compose path).
          *
          * Sky compute also imageStores Z=0 (the "infinity"
          * sentinel for the atomicMax Z-test) directly into
          * vk_zbuffer at each sky pixel, so the SW D_Draw-
          * ZSpans call is unneeded on this branch for the
          * downstream alias / particle / sprite compute Z-
          * tests.
          *
          * d_pzbuffer at sky pixels is then memset to 0
          * per span as a cheap follow-up: the pass-2
          * translucent SW raster (water / slime / lava
          * stippled over what's behind them) reads
          * d_pzbuffer to decide stipple pass/fail, so
          * leaving it stale would mis-stipple the rare
          * sky+water overlap case.  memset to 0 matches
          * the same "infinity" sentinel sky.comp writes to
          * vk_zbuffer and costs only the SIMD memset
          * bandwidth -- far cheaper than D_DrawZSpans'
          * per-pixel float-fma + cast + 16-bit store. */
         if (g_rhi && g_rhi->dispatch_3d_sky_span)
         {
            const espan_t *p;
            for (p = s->spans; p; p = p->pnext)
            {
               g_rhi->dispatch_3d_sky_span(p->u, p->v, p->count);
               memset(d_pzbuffer + (size_t)d_zwidth * (size_t)p->v
                      + (size_t)p->u,
                      0,
                      (size_t)p->count * sizeof(short));
            }
         }
         else
         {
            D_DrawSkyScans8(s->spans);
            D_DrawZSpans(s->spans);
         }
      }
      else if (s->flags & SURF_DRAWBACKGROUND)
      {
         /* In pass 2 (translucent liquid pass), the "background"
          * surface owns every screen pixel that isn't covered by a
          * liquid surface -- but those pixels already hold pass 1's
          * world rendering, which we want to keep visible behind
          * the stippled liquid.  Drawing the background here would
          * paint solid r_clearcolor over the whole opaque world. */
         if (r_renderpass == 2)
            continue;

         /* Set up a gradient for the background surface that places it
          * effectively at infinity distance from the viewpoint */
         d_zistepu = 0;
         d_zistepv = 0;
         d_ziorigin = -0.9;

         D_DrawSolidSurface(s, (int)r_clearcolor.value & 0xFF);
         D_DrawZSpans(s->spans);
      }
      else if (s->flags & SURF_DRAWTURB)
      {
         float alpha;
         qboolean liquids_translucent;
         pface = (msurface_t*)s->data;
         miplevel = 0;
         cacheblock = (pixel_t *)
            ((byte *)pface->texinfo->texture +
             pface->texinfo->texture->offsets[0]);
         cachewidth = 64;

         /* Pick the per-liquid-type alpha (water/lava/slime/tele)
          * from the surface texture name, then translate the user's
          * 0..1 cvar value into the 0..255 threshold the rasterizer
          * expects.  Also publish the active blend mode for the
          * rasterizer to consult.
          *
          * Two snap-to-safe clamps tied to the 4x4 Bayer matrix
          * actually used in D_DrawTurbulent8Span (max threshold
          * 240, min 0):
          *   - alpha >= 0.95 -> opaque path.  The Bayer matrix's
          *     max threshold is 240, so any cvar value above
          *     240/255 = 0.941 produces zero stipple holes -- the
          *     stipple branch ends up writing every pixel,
          *     visually identical to opaque rendering AND with
          *     extra per-pixel z-test cost.  Snapping anything
          *     >= 0.95 (the next 0.05 step above 0.941) to the
          *     vanilla fast path eliminates a dead zone where the
          *     slider appears to do nothing.
          *   - alpha <= 0.05 -> opaque too.  At zero alpha the
          *     liquid surface is invisible, exposing the
          *     non-rendering of stipple-hole framebuffer pixels.
          *     Below the smallest useful stipple threshold we
          *     just render normal opaque liquid; the user can
          *     set r_liquidblend = 0 to disable across the board.
          *
          * Always reset r_turb_alpha/r_turb_blendmode/r_turb_ztest
          * to vanilla defaults BEFORE deciding on stipple values.
          * These are file-scope state in d_scan.c and would
          * otherwise carry stale stipple values across frames -- in
          * particular if the user toggles Liquid Blend off after
          * having had it on, or if any liquid alpha cvar was set
          * from a config at startup but blend is currently off.
          * Without this reset, D_DrawTurbulent8Span would keep
          * taking the stipple path with no z-write, causing
          * ghosting/corruption of liquid surfaces in Off mode. */
         r_turb_alpha     = 255;
         r_turb_blendmode = 0;
         r_turb_ztest     = 0;

         liquids_translucent = R_LiquidsAreTransparent();
         if (liquids_translucent) {
            alpha = R_LiquidAlphaForTexture(pface->texinfo->texture);
            if (alpha < 0.0f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;
            if (alpha >= 0.95f || alpha <= 0.05f) {
               /* Out of the useful stipple range -- this particular
                * liquid type is at full opacity even though OTHER
                * liquid types are translucent (so two-pass mode is
                * active globally, gated by R_LiquidsAreTransparent).
                *
                * In pass 2 this surface still needs the per-pixel
                * z-test, otherwise it smears over walls/floors
                * drawn in pass 1: pass 2's edge list contains
                * liquid surfaces only, no opaque occluders, so
                * without z-test there's nothing to stop a liquid
                * pixel from winning every screen position the
                * surface covers.  This is what causes "sticky
                * teleporter through walls" ghosting -- a
                * teleporter at r_telealpha=1.0 in a map where
                * r_wateralpha<1.0 paints itself through walls.
                *
                * In pass 1 (or single-pass mode), take the
                * vanilla non-ztest fast path. */
               r_turb_alpha     = 255;
               r_turb_blendmode = 0;
               r_turb_ztest     = (r_renderpass == 2) ? 1 : 0;
               liquids_translucent = false; /* take vanilla z-write below */
            } else {
               r_turb_alpha     = (int)(alpha * 255.0f + 0.5f);
               r_turb_blendmode = (int)r_liquidblend.value;
               /* Pass-2 z-test: the edge list contains only liquid
                * surfaces in pass 2, so without per-pixel z-test
                * liquid would smear over foreground walls/floors
                * drawn into the z-buffer in pass 1. */
               r_turb_ztest = 1;
            }
         }

         if (s->insubmodel)
         {
            /* FIXME: we don't want to do all this for every polygon!
             * TODO: store once at start of frame
             */
            e = s->entity;	/* FIXME: make this passed in to // R_RotateBmodel () */
            VectorSubtract(r_origin, e->origin, local_modelorg);
            TransformVector(local_modelorg, transformed_modelorg);

            R_RotateBmodel(e);	/* FIXME: don't mess with the frustum, make entity passed in */
         }

         D_CalcGradients(pface);
         /* Phase 5b-07b + followup: route turb surfaces to GPU
          * compute when the RHI exposes a turb dispatch entry.
          * Two modes covered, discriminated by r_renderpass:
          *
          *   r_renderpass != 2 AND alpha == 255 AND blend == 0:
          *     single-pass opaque (Phase 5b-07b).  Backend
          *     writes color + per-pixel 1/z; downstream
          *     atomicMax-Z paths see the actual water Z.
          *
          *   r_renderpass == 2:
          *     pass-2 of two-pass translucent mode.  Backend
          *     z-tests against vk_zbuffer (= pass-1 opaque-
          *     world Z), optionally Bayer-stipples (alpha <
          *     255 with blendmode == 1), writes color only,
          *     never updates Z -- exactly mirroring SW pass-
          *     2's no-z-write semantics.  This case is
          *     entered only by the opaque-with-z-test branch
          *     (alpha == 255, blendmode != 1) and the
          *     stipple branch (alpha < 255, blendmode == 1)
          *     in D_DrawTurbulent8Span.
          *
          *   Pass 1 of two-pass (r_renderpass == 1) is
          *   filtered out before reaching SURF_DRAWTURB --
          *   r_main.c excludes liquids from the opaque-world
          *   pass entirely -- so we don't need to handle it.
          *
          * memset d_pzbuffer to 0 per span fires in single-
          * pass mode only (see the inline comment at the
          * memset site for the rationale).  Pass-2 mode
          * MUST preserve d_pzbuffer's pass-1 Z at lava-span
          * pixels so the GPU pass-2 turb shader's imageLoad-
          * based z-test can see closer walls and skip drawing
          * lava through them.
          *
          * SW-only mode (no RHI / vtable entry NULL) is
          * unaffected -- the else-branch runs Turbulent8 +
          * D_DrawZSpans exactly as before. */
         if (g_rhi && g_rhi->dispatch_3d_turb_surface
             && ((r_renderpass != 2
                  && r_turb_alpha == 255
                  && r_turb_blendmode == 0)
                 || r_renderpass == 2))
         {
            rhi_turb_gradient_t grad;
            const espan_t *p;
            int phase;
            int is_pass2 = (r_renderpass == 2) ? 1 : 0;
            /* Mirror the SW driver's blend-mode dispatch:
             * D_DrawTurbulent8Span treats `alpha >= 255 ||
             * blendmode != 1` as the no-stipple path, so any
             * `r_liquidblend` value other than 1 (currently 0 =
             * off and 2+ = reserved future colormap-blend modes
             * that fall back to opaque) must NOT stipple even
             * when `r_turb_alpha < 255`.  The shader's stipple
             * trigger is just `alpha < 255`, so clamp the alpha
             * we ship to 255 here whenever blendmode isn't 1.
             * Result: blendmode 1 stipples (SW + GPU agree),
             * blendmode 0 / 2+ takes the opaque-with-ztest
             * branch (SW + GPU agree).  Z-test gating in pass-2
             * is unaffected -- the shader z-tests whenever
             * is_pass2 is set, independent of alpha. */
            int gpu_alpha = (r_turb_blendmode == 1)
                            ? r_turb_alpha : 255;

            grad.sdivzorigin = d_sdivzorigin;
            grad.sdivzstepu  = d_sdivzstepu;
            grad.sdivzstepv  = d_sdivzstepv;
            grad.tdivzorigin = d_tdivzorigin;
            grad.tdivzstepu  = d_tdivzstepu;
            grad.tdivzstepv  = d_tdivzstepv;
            grad.ziorigin    = d_ziorigin;
            grad.zistepu     = d_zistepu;
            grad.zistepv     = d_zistepv;
            grad.sadjust     = (int32_t)sadjust;
            grad.tadjust     = (int32_t)tadjust;
            grad.bbextents   = (int32_t)bbextents;
            grad.bbextentt   = (int32_t)bbextentt;

            phase = (int)(cl.time * TURB_SPEED) & (TURB_CYCLE - 1);

            g_rhi->dispatch_3d_turb_surface(s->spans, &grad,
                                            (const byte *)cacheblock,
                                            phase,
                                            gpu_alpha,
                                            is_pass2);

            /* Single-pass mode only: clear d_pzbuffer at lava-
             * span pixels to 0 so subsequent SW raster of OTHER
             * liquid surfaces in the same frame (mixed alpha /
             * blend in single-pass) doesn't read stale Z at
             * compute-rendered turb pixels.  Mirrors sky.comp's
             * pattern.
             *
             * NEVER in pass-2 mode (r_renderpass == 2).  In
             * two-pass translucent rendering, R_MarkSurfaces
             * extends visibility to every non-solid leaf when
             * liquid is in the camera's PVS (r_main.c:740) --
             * the no-cull pass.  As a consequence the lava
             * surface's edge-rastered spans include pixels
             * BEHIND closer walls / floors (pass 2's edge list
             * contains liquids only, so the edge raster has no
             * opaque occluders to clip against).  Pass 1
             * already wrote the closer-surface Z into
             * d_pzbuffer at those very pixels (that's the
             * design -- the SW comment below in the else-branch
             * says it explicitly: "Pass 1 already wrote the
             * opaque world's z-buffer; we want to preserve
             * those depths").  Pass-2 turb's job, both in SW
             * (D_DrawTurbulent8Span's ztest path) and on GPU
             * (turb.comp's pass-2 imageLoad-based z-test), is
             * to z-TEST against that pass-1 Z and skip drawing
             * where the wall is closer.  A memset to 0 here
             * would destroy the wall Z that pass-2 turb needs
             * to z-test against, and lava would render through
             * every wall in view.  This is what Lib's screenshot
             * with r_liquidblend=1 was showing -- a full-screen
             * lava bleed past the difficulty-room arch in
             * start.bsp. */
            if (!is_pass2) {
               for (p = s->spans; p; p = p->pnext)
                  memset(d_pzbuffer + (size_t)d_zwidth * (size_t)p->v
                         + (size_t)p->u,
                         0,
                         (size_t)p->count * sizeof(short));
            }
         }
         else
         {
            Turbulent8(s->spans);
            /* Z-write semantics:
             *   - Single-pass mode (pass 0): write z so alias models
             *     behind opaque liquid are occluded normally.
             *   - Pass 1 of two-pass: SURF_DRAWTURB is filtered out
             *     before reaching here, so this case can't happen.
             *   - Pass 2 of two-pass: NEVER write z.  Pass 1 already
             *     wrote the opaque world's z-buffer; we want to
             *     preserve those depths so alias models drawn between
             *     passes (R_DrawEntitiesOnList runs after pass 1's
             *     R_EdgeDrawing) z-test correctly against opaque
             *     world.  Writing liquid z over wall z would corrupt
             *     the buffer at pixels where liquid lost the z-test
             *     (D_DrawZSpans writes every pixel in the span
             *     regardless of compare result). */
            if (!liquids_translucent && r_renderpass != 2)
               D_DrawZSpans(s->spans);
         }

         if (s->insubmodel)
         {
            /* restore the old drawing state
             *
             * FIXME: we don't want to do this every time!
             * TODO: speed up
             */
            e = &r_worldentity;
            VectorCopy(world_transformed_modelorg,
                  transformed_modelorg);
            VectorCopy(base_vpn, vpn);
            VectorCopy(base_vup, vup);
            VectorCopy(base_vright, vright);
            VectorCopy(base_modelorg, modelorg);
            R_TransformFrustum();
         }
      }
      else
      {
         if (s->insubmodel)
         {
            /* FIXME: we don't want to do all this for every polygon! */
            /* TODO: store once at start of frame */
            e = s->entity;	/* FIXME: make this passed in to */
            /* R_RotateBmodel () */
            VectorSubtract(r_origin, e->origin, local_modelorg);
            TransformVector(local_modelorg, transformed_modelorg);

            R_RotateBmodel(e);	/* FIXME: don't mess with the frustum, */
            /* make entity passed in */
         }

         pface = (msurface_t*)s->data;
         miplevel = D_MipLevelForScale(s->nearzi * scale_for_mip
               * pface->texinfo->mipadjust);

         /* FIXME: make this passed in to D_CacheSurface */
         pcurrentcache = D_CacheSurface(e, pface, miplevel);

         cacheblock = (pixel_t *)pcurrentcache->data;
         cachewidth = pcurrentcache->width;

         D_CalcGradients(pface);
         D_DrawSpans(s->spans);
         D_DrawZSpans(s->spans);

         if (s->insubmodel)
         {
            /* restore the old drawing state */
            /* FIXME: we don't want to do this every time! */
            /* TODO: speed up */
            e = &r_worldentity;
            VectorCopy(world_transformed_modelorg,
                  transformed_modelorg);
            VectorCopy(base_vpn, vpn);
            VectorCopy(base_vup, vup);
            VectorCopy(base_vright, vright);
            VectorCopy(base_modelorg, modelorg);
            R_TransformFrustum();
         }
      }
   }
}
