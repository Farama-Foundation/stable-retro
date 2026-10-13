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
/* d_polyset.c: routines for drawing sets of polygons sharing the same */
/* texture (used for Alias models) */

#include <stdint.h>

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"
#include "rhi.h"

/* TODO: put in span spilling to shrink list size */
/* !!! if this is changed, it must be changed in d_polysa.s too !!! */
#define DPS_MAXSPANS MAXHEIGHT+1
			/* 1 extra for spanpackage that marks end */

/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct {
    void *pdest;
    short *pz;
    int count;
    byte *ptex;
    int sfrac, tfrac, light, zi;
    float nx, ny, nz;       /* Phong: vertex normal at the start of this row */
} spanpackage_t;

typedef struct {
    int isflattop;
    int numleftedges;
    int *pleftedgevert0;
    int *pleftedgevert1;
    int *pleftedgevert2;
    int numrightedges;
    int *prightedgevert0;
    int *prightedgevert1;
    int *prightedgevert2;
} edgetable;

int r_p0[6], r_p1[6], r_p2[6];

/* Phong shading state. All of this is set up only when r_phongshading.value
 * is non-zero. When the cvar is off, none of these are touched and the
 * existing Gouraud (per-vertex) light interpolant via r_p0[4]/r_p1[4]/r_p2[4]
 * carries the rasterizer as before. */
static float r_n0[3], r_n1[3], r_n2[3];                      /* per-triangle vertex normals */
static float r_nxstepx, r_nxstepy;                           /* nx gradient (per pixel x, per row y) */
static float r_nystepx, r_nystepy;                           /* ny gradient */
static float r_nzstepx, r_nzstepy;                           /* nz gradient */
static float d_nx, d_nxbasestep, d_nxextrastep;              /* nx state along left edge */
static float d_ny, d_nybasestep, d_nyextrastep;              /* ny state */
static float d_nz, d_nzbasestep, d_nzextrastep;              /* nz state */

/* Light setup data shared with r_alias.c. r_plightvec holds the
 * model-space light direction set up in R_AliasSetupLighting; the
 * Phong inner loop computes per-pixel dot(N, L) using it.
 * r_phalfvec is the Blinn-Phong halfway vector L+V, also in model
 * space, used by the optional specular term. */
extern vec3_t r_plightvec;
extern vec3_t r_phalfvec;
extern int    r_ambientlight;
extern float  r_shadelight;

/* Specular LUT.  Indexed by clamp(dot(N, H), 0, 1) * 255 to give a
 * brightness contribution in light-units.  Built in
 * D_PolysetUpdateSpecularLUT() whenever the r_phongshading cvar
 * value indicates a different exponent has been requested.
 *
 * SPECULAR_PEAK is the maximum highlight contribution in light-units
 * (one colormap-row jump is 256, so SPECULAR_PEAK = 5120 = 20 rows
 * worth, which lifts the brightest specular pixel by ~20 colormap
 * rows -- visible but not saturating).  PHONG_SPECULAR_EXPONENT is
 * the power for the Blinn-Phong term; 16 is a reasonable middle
 * ground for Quake's matte-but-not-flat materials. */
#define SPECULAR_PEAK            5120
#define PHONG_SPECULAR_EXPONENT  16.0f

static int   specular_lut[256];
static float specular_lut_exponent = -1.0f;       /* sentinel: not built */

static void D_BuildSpecularLUT(float exponent)
{
    int i;
    for (i = 0; i < 256; i++) {
	float dot = (float)i / 255.0f;
	float spec = (float)pow(dot, exponent);
	specular_lut[i] = (int)(spec * SPECULAR_PEAK);
    }
    specular_lut_exponent = exponent;
}

/* Colored-lighting state, consumed by the RGB-output rasterizers
 * (D_PolysetDrawSpansRGB and D_PolysetDrawSpansPhongRGB).
 * lightcolor is set per-frame per-model by R_LightPoint;
 * host_fullbrights is the count of self-illuminated palette entries
 * (set in host.c at startup). */
extern vec3_t lightcolor;
extern int    host_fullbrights;

/* 4x4 ordered (Bayer) dither matrix.  Values are pre-scaled additive
 * offsets in light-units, where one colormap-row jump is 256.  Adding
 * the table value to a Gouraud- or Phong-interpolated `light` before
 * the `& 0xFF00` row-select truncation breaks the visible 64-row
 * banding into a stippled threshold without changing the average
 * brightness.
 *
 * Construction: standard 4x4 Bayer matrix B = {0,8,2,10; 12,4,14,6;
 * 3,11,1,9; 15,7,13,5}, then offset = (B * 16) - 120, giving values
 * in the range [-120, +120] symmetrically around zero.  Used only
 * when r_lightdither.value is non-zero; the rasterizer pulls a
 * pointer to either dither_bayer4 (enabled) or dither_zero4
 * (disabled, all zero) once per call so the inner loop has a single
 * unconditional table lookup. */
static const int dither_bayer4[4][4] = {
    { -120,    8,  -88,   40 },
    {   72,  -56,  104,  -24 },
    {  -72,   56, -104,   24 },
    {  120,   -8,   88,  -40 }
};
static const int dither_zero4[4][4] = {
    { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
};


byte *d_pcolormap;

int d_xdenom;

static edgetable *pedgetable;
static edgetable edgetables[12] = {
    {0, 1, r_p0, r_p2, NULL, 2, r_p0, r_p1, r_p2},
    {0, 2, r_p1, r_p0, r_p2, 1, r_p1, r_p2, NULL},
    {1, 1, r_p0, r_p2, NULL, 1, r_p1, r_p2, NULL},
    {0, 1, r_p1, r_p0, NULL, 2, r_p1, r_p2, r_p0},
    {0, 2, r_p0, r_p2, r_p1, 1, r_p0, r_p1, NULL},
    {0, 1, r_p2, r_p1, NULL, 1, r_p2, r_p0, NULL},
    {0, 1, r_p2, r_p1, NULL, 2, r_p2, r_p0, r_p1},
    {0, 2, r_p2, r_p1, r_p0, 1, r_p2, r_p0, NULL},
    {0, 1, r_p1, r_p0, NULL, 1, r_p1, r_p2, NULL},
    {1, 1, r_p2, r_p1, NULL, 1, r_p0, r_p1, NULL},
    {1, 1, r_p1, r_p0, NULL, 1, r_p2, r_p0, NULL},
    {0, 1, r_p0, r_p2, NULL, 1, r_p0, r_p1, NULL},
};

/* FIXME: some of these can become statics */
int a_sstepxfrac, a_tstepxfrac, r_lstepx, a_ststepxwhole;
int r_sstepx, r_tstepx, r_lstepy, r_sstepy, r_tstepy;
int r_zistepx, r_zistepy;
int d_aspancount, d_countextrastep;

spanpackage_t *a_spans;
spanpackage_t *d_pedgespanpackage;
static int ystart;
byte *d_pdest, *d_ptex;
short *d_pz;
int d_sfrac, d_tfrac, d_light, d_zi;
int d_ptexextrastep, d_sfracextrastep;
int d_tfracextrastep, d_lightextrastep, d_pdestextrastep;
int d_lightbasestep, d_pdestbasestep, d_ptexbasestep;
int d_sfracbasestep, d_tfracbasestep;
int d_ziextrastep, d_zibasestep;
int d_pzextrastep, d_pzbasestep;

typedef struct {
    int quotient;
    int remainder;
} adivtab_t;

static adivtab_t adivtab[32 * 32] = {
#include "adivtab.h"
};

byte *skintable[MAX_LBM_HEIGHT];
static int skinwidth;
static byte *skinstart;

void D_PolysetDrawSpans8(spanpackage_t *pspanpackage);
void D_PolysetDrawSpansPhong8(spanpackage_t *pspanpackage);
void D_PolysetDrawSpansPhongRGB(spanpackage_t *pspanpackage);
void D_PolysetCalcGradients(int skinwidth);
void D_PolysetSetEdgeTable(void);
void D_RasterizeAliasPolySmooth(void);
void D_PolysetScanLeftEdge(int height);

static void D_DrawSubdiv(void);
static void D_DrawNonSubdiv(void);
static void D_PolysetRecursiveTriangle(int *p1, int *p2, int *p3);

/*
================
D_PolysetDraw
================
*/
/* Module-static span scratch.  Previously a 76928-byte stack local in
 * D_PolysetDraw, large enough that the function prologue ran a stack
 * probe loop (subq $4096, %rsp + orq $0, (%rsp) + cmp + jne, 18
 * iterations) on every call before any actual work could begin.
 *
 * Sized exactly the same as the previous stack local
 * (CACHE_PAD_ARRAY(DPS_MAXSPANS + 1, spanpackage_t)) so the rasterizer
 * sees an identical scratch buffer.  Aligned to CACHE_SIZE directly so
 * a_spans can take the array base address without the runtime
 * (uintptr_t)align trick the stack version needed.
 *
 * Safe to make file-static because D_PolysetDraw is called only from
 * the rendering thread (libretro keeps GS/audio on separate threads
 * that never touch the SW polyset path) and never recurses -- the
 * rasterizer it dispatches to (D_DrawSubdiv / D_DrawNonSubdiv ->
 * D_PolysetRecursiveTriangle and friends) doesn't call back into
 * D_PolysetDraw. */
CACHE_ALIGN_DECL(static spanpackage_t
    d_polyset_spans[CACHE_PAD_ARRAY(DPS_MAXSPANS + 1, spanpackage_t)]);

void
D_PolysetDraw(void)
{
   a_spans = d_polyset_spans;

   /* Phase 5b-06: if the active backend exposes a GPU
    * alias dispatch and compute rendering is enabled,
    * compact the triangle batch (per-triangle clipped
    * calls reference only 3 verts out of an entity-sized
    * finalvert array; the per-model unclipped call
    * references most of them) into a contiguous vertex
    * subset + remapped triangle indices and ship it to
    * the backend.  This keeps the GPU vertex pool small
    * even when an entity fires hundreds of per-triangle
    * D_PolysetDraw calls due to partial clipping.
    *
    * Static scratch buffers sized for the largest stock
    * Quake alias model (Shambler: ~400 tris, ~200
    * verts).  Hexen 2 / mods with bigger models that
    * exceed these caps fall through to SW.
    *
    * The intercept handles ANY drawtype.  Quake's
    * drawtype != 0 setting triggers the subdivided
    * affine raster (D_DrawSubdiv) instead of plain
    * D_DrawNonSubdiv, but the subdivision is just a
    * means to bound affine error within a triangle's
    * span -- our per-pixel barycentric interpolation
    * does that implicitly per pixel, so the GPU path
    * is correct (and slightly more accurate than
    * D_DrawSubdiv) for both. */
   if (g_rhi
       && g_rhi->dispatch_3d_alias
       && g_rhi_compute_rendering
       && r_affinetridesc.numtriangles >= 1
       && r_affinetridesc.pfinalverts
       && r_affinetridesc.ptriangles
       && r_affinetridesc.pskin
       && acolormap) {
      enum { D_ALIAS_GPU_MAX_VERTS = 1024,
             D_ALIAS_GPU_MAX_TRIS  = 2048 };
      /* Two remap tables.  An on-seam vertex referenced
       * by both a facesfront=true and a facesfront=false
       * triangle in the same dispatch needs two compacted
       * entries: one using the original s coordinate (for
       * the front-skin-half triangle), one with seamfixup
       * X16 added to v[2] (for the back-skin-half
       * triangle).  Other vertices share a single entry
       * via idx_remap_front.  Worst-case duplication is
       * the seam vertex count of the model, well under
       * 2x; D_ALIAS_GPU_MAX_VERTS = 1024 covers it.
       *
       * The companion SW path applies the same fixup
       * inline per triangle (d_polyse.c D_DrawNonSubdiv,
       * lines 481-488: r_p[i][2] += seamfixupX16 when
       * !facesfront && (flags & ALIAS_ONSEAM)).  Doing it
       * at compaction time is the natural place for the
       * GPU port -- it's a per-(vertex, triangle-side)
       * adjustment that the shader can't recover without
       * carrying flags + facesfront through the dispatch
       * record.  Without this, every facesfront=false
       * triangle samples the front half of the skin and
       * renders with garbled UVs across the visible
       * back-skin geometry. */
      static int             idx_remap_front[D_ALIAS_GPU_MAX_VERTS];
      static int             idx_remap_back[D_ALIAS_GPU_MAX_VERTS];
      static rhi_alias_vert_t compact_verts[D_ALIAS_GPU_MAX_VERTS];
      static rhi_alias_tri_t  compact_tris[D_ALIAS_GPU_MAX_TRIS];
      const finalvert_t *pfv = r_affinetridesc.pfinalverts;
      const mtriangle_t *ptr = r_affinetridesc.ptriangles;
      int                ntri = r_affinetridesc.numtriangles;
      int                seamfixup = r_affinetridesc.seamfixupX16;
      int                i, j, new_v_count = 0;
      int                ok = 1;

      if (ntri > (int)D_ALIAS_GPU_MAX_TRIS)
         ok = 0;

      if (ok) {
         /* Initialise both remap tables to "unseen" for
          * this call.  Cost is two memsets per call (8
          * KiB at int width); cheap vs the dispatch
          * overhead. */
         for (i = 0; i < (int)D_ALIAS_GPU_MAX_VERTS; i++) {
            idx_remap_front[i] = -1;
            idx_remap_back[i]  = -1;
         }

         for (i = 0; i < ntri && ok; i++) {
            int facesfront = ptr[i].facesfront;

            for (j = 0; j < 3; j++) {
               int orig = ptr[i].vertindex[j];
               int *remap;
               int  adjust;

               if (orig < 0 || orig >= (int)D_ALIAS_GPU_MAX_VERTS) {
                  ok = 0;
                  break;
               }

               /* Pick the remap table + s adjustment:
                *   facesfront=true   -> shared front entry, no adjust
                *   facesfront=false && ONSEAM -> back entry, +seamfixupX16
                *   facesfront=false && !ONSEAM -> shared front entry, no adjust
                *
                * The clipped path (R_AliasClipTriangle)
                * already applied the fixup to its local
                * fv stack before calling D_PolysetDraw,
                * and cleared flags afterwards, so when
                * those calls reach us the ONSEAM check
                * below sees flags=0 and we don't double-
                * adjust. */
               if (!facesfront && (pfv[orig].flags & ALIAS_ONSEAM)) {
                  remap  = idx_remap_back;
                  adjust = seamfixup;
               } else {
                  remap  = idx_remap_front;
                  adjust = 0;
               }

               if (remap[orig] < 0) {
                  if (new_v_count >= (int)D_ALIAS_GPU_MAX_VERTS) {
                     ok = 0;
                     break;
                  }
                  remap[orig] = new_v_count;
                  /* Copy the whole 44-byte finalvert_t.
                   * rhi_alias_vert_t has the same layout
                   * so this is a direct struct copy; the
                   * backend reads only v[0..5]. */
                  memcpy(&compact_verts[new_v_count],
                         &pfv[orig],
                         sizeof(rhi_alias_vert_t));
                  compact_verts[new_v_count].v[2] += adjust;
                  new_v_count++;
               }

               compact_tris[i].vertindex[j] = remap[orig];
            }
            compact_tris[i].facesfront = facesfront;
         }
      }

      if (ok && new_v_count >= 3) {
         g_rhi->dispatch_3d_alias(
            compact_verts,
            new_v_count,
            compact_tris,
            ntri,
            (const byte *)r_affinetridesc.pskin,
            r_affinetridesc.skinwidth,
            r_affinetridesc.skinheight,
            (const byte *)acolormap);
         return;
      }
      /* Fall through to SW if compaction failed (model
       * exceeded scratch buffer caps, or referenced a
       * vertex index past our remap-table size). */
   }

   if (r_affinetridesc.drawtype)
      D_DrawSubdiv();
   else
      D_DrawNonSubdiv();
}


/*
================
D_PolysetDrawFinalVerts

TODO/FIXME - Needs updates/specialization for Hexen 2
================
*/
void
D_PolysetDrawFinalVerts(finalvert_t *fv, int nverts)
{
   int i, z;
   int16_t *zbuf;

   for (i = 0; i < nverts; i++, fv++)
   {
      /* valid triangle coordinates for filling can include the bottom and
       * right clip edges, due to the fill rule; these shouldn't be drawn */
      if ((fv->v[0] < r_refdef.vrectright) &&
            (fv->v[1] < r_refdef.vrectbottom))
      {
         /* Baker - in very rare occassions, these may have 
          * negative values and can result in a memory access violation */
         if (fv->v[0] >=0 && fv->v[1] >=0)
         {
            z = fv->v[5] >> 16;
            zbuf = zspantable[fv->v[1]] + fv->v[0];
            if (z >= *zbuf)
            {
               int pix;

               *zbuf = z;
               pix = skintable[fv->v[3] >> 16][fv->v[2] >> 16];
               pix = ((byte *)acolormap)[pix + (fv->v[4] & 0xFF00)];
               d_viewbuffer[d_scantable[fv->v[1]] + fv->v[0]] = pix;
            }
         }
      }
   }
}


/*
================
D_DrawSubdiv

TODO/FIXME - Needs updates/specialization for Hexen 2
================
*/
static void D_DrawSubdiv(void)
{
   int i;
   finalvert_t *pfv  = r_affinetridesc.pfinalverts;
   mtriangle_t *ptri = r_affinetridesc.ptriangles;
   int lnumtriangles = r_affinetridesc.numtriangles;

   for (i = 0; i < lnumtriangles; i++)
   {
      finalvert_t *index0 = pfv + ptri[i].vertindex[0];
      finalvert_t *index1 = pfv + ptri[i].vertindex[1];
      finalvert_t *index2 = pfv + ptri[i].vertindex[2];

      if (((index0->v[1] - index1->v[1]) *
               (index0->v[0] - index2->v[0]) -
               (index0->v[0] - index1->v[0]) *
               (index0->v[1] - index2->v[1])) >= 0)
         continue;

      d_pcolormap = &((byte *)acolormap)[index0->v[4] & 0xFF00];

      if (ptri[i].facesfront)
         D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
      else
      {
         int s0 = index0->v[2];
         int s1 = index1->v[2];
         int s2 = index2->v[2];

         if (index0->flags & ALIAS_ONSEAM)
            index0->v[2] += r_affinetridesc.seamfixupX16;
         if (index1->flags & ALIAS_ONSEAM)
            index1->v[2] += r_affinetridesc.seamfixupX16;
         if (index2->flags & ALIAS_ONSEAM)
            index2->v[2] += r_affinetridesc.seamfixupX16;

         D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

         index0->v[2] = s0;
         index1->v[2] = s1;
         index2->v[2] = s2;
      }
   }
}


/*
================
D_DrawNonSubdiv

TODO/FIXME - Needs updates/specialization for Hexen 2
================
*/
static void D_DrawNonSubdiv(void)
{
   int i;

   finalvert_t *pfv = r_affinetridesc.pfinalverts;
   mtriangle_t *ptri = r_affinetridesc.ptriangles;
   int lnumtriangles = r_affinetridesc.numtriangles;

   for (i = 0; i < lnumtriangles; i++, ptri++)
   {
      finalvert_t *index0 = pfv + ptri->vertindex[0];
      finalvert_t *index1 = pfv + ptri->vertindex[1];
      finalvert_t *index2 = pfv + ptri->vertindex[2];

      d_xdenom = (index0->v[1] - index1->v[1]) *
         (index0->v[0] - index2->v[0]) -
         (index0->v[0] - index1->v[0]) * (index0->v[1] - index2->v[1]);

      if (d_xdenom >= 0) {
         continue;
      }

      r_p0[0] = index0->v[0];	/* u */
      r_p0[1] = index0->v[1];	/* v */
      r_p0[2] = index0->v[2];	/* s */
      r_p0[3] = index0->v[3];	/* t */
      r_p0[4] = index0->v[4];	/* light */
      r_p0[5] = index0->v[5];	/* iz */

      r_p1[0] = index1->v[0];
      r_p1[1] = index1->v[1];
      r_p1[2] = index1->v[2];
      r_p1[3] = index1->v[3];
      r_p1[4] = index1->v[4];
      r_p1[5] = index1->v[5];

      r_p2[0] = index2->v[0];
      r_p2[1] = index2->v[1];
      r_p2[2] = index2->v[2];
      r_p2[3] = index2->v[3];
      r_p2[4] = index2->v[4];
      r_p2[5] = index2->v[5];

      if (r_phongshading.value) {
         r_n0[0] = index0->n[0]; r_n0[1] = index0->n[1]; r_n0[2] = index0->n[2];
         r_n1[0] = index1->n[0]; r_n1[1] = index1->n[1]; r_n1[2] = index1->n[2];
         r_n2[0] = index2->n[0]; r_n2[1] = index2->n[1]; r_n2[2] = index2->n[2];
      }

      if (!ptri->facesfront) {
         if (index0->flags & ALIAS_ONSEAM)
            r_p0[2] += r_affinetridesc.seamfixupX16;
         if (index1->flags & ALIAS_ONSEAM)
            r_p1[2] += r_affinetridesc.seamfixupX16;
         if (index2->flags & ALIAS_ONSEAM)
            r_p2[2] += r_affinetridesc.seamfixupX16;
      }

      D_PolysetSetEdgeTable();
      D_RasterizeAliasPolySmooth();
   }
}


/*
================
D_PolysetRecursiveTriangle

TODO/FIXME - Needs updates/specialization for Hexen 2
================
*/
static void D_PolysetRecursiveTriangle(int *lp1, int *lp2, int *lp3)
{
   int *temp;
   int newobj[6];
   int z;
   int16_t *zbuf;

   int d = lp2[0] - lp1[0];
   if (d < -1 || d > 1)
      goto split;
   d = lp2[1] - lp1[1];
   if (d < -1 || d > 1)
      goto split;

   d = lp3[0] - lp2[0];
   if (d < -1 || d > 1)
      goto split2;
   d = lp3[1] - lp2[1];
   if (d < -1 || d > 1)
      goto split2;

   d = lp1[0] - lp3[0];
   if (d < -1 || d > 1)
      goto split3;
   d = lp1[1] - lp3[1];
   if (d < -1 || d > 1) {
split3:
      temp = lp1;
      lp1 = lp3;
      lp3 = lp2;
      lp2 = temp;

      goto split;
   }

   return;			/* entire tri is filled */

split2:
   temp = lp1;
   lp1 = lp2;
   lp2 = lp3;
   lp3 = temp;

split:
   /* split this edge */
   newobj[0] = (lp1[0] + lp2[0]) >> 1;
   newobj[1] = (lp1[1] + lp2[1]) >> 1;
   newobj[2] = (lp1[2] + lp2[2]) >> 1;
   newobj[3] = (lp1[3] + lp2[3]) >> 1;
   newobj[5] = (lp1[5] + lp2[5]) >> 1;

   /* draw the point if splitting a leading edge */
   if (lp2[1] > lp1[1])
      goto nodraw;
   if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
      goto nodraw;


   z = newobj[5] >> 16;
   zbuf = zspantable[newobj[1]] + newobj[0];
   if (z >= *zbuf) {
      int pix;

      *zbuf = z;
      pix = d_pcolormap[skintable[newobj[3] >> 16][newobj[2] >> 16]];
      d_viewbuffer[d_scantable[newobj[1]] + newobj[0]] = pix;
   }

nodraw:
   /* recursively continue */
   D_PolysetRecursiveTriangle(lp3, lp1, newobj);
   D_PolysetRecursiveTriangle(lp3, newobj, lp2);
}

/*
================
D_PolysetUpdateTables
================
*/
void D_PolysetUpdateTables(void)
{
   if (  r_affinetridesc.skinwidth != skinwidth ||
         r_affinetridesc.pskin     != skinstart)
   {
      byte *s;
      int i;
      skinwidth = r_affinetridesc.skinwidth;
      skinstart = (byte*)r_affinetridesc.pskin;
      s = skinstart;

      for (i = 0; i < MAX_LBM_HEIGHT; i++, s += skinwidth)
         skintable[i] = s;
   }
}

/*
===================
D_PolysetScanLeftEdge
====================
*/
void D_PolysetScanLeftEdge(int height)
{
   do
   {
      d_pedgespanpackage->pdest = d_pdest;
      d_pedgespanpackage->pz    = d_pz;
      d_pedgespanpackage->count = d_aspancount;
      d_pedgespanpackage->ptex  = d_ptex;

      d_pedgespanpackage->sfrac = d_sfrac;
      d_pedgespanpackage->tfrac = d_tfrac;

      /* FIXME: need to clamp l, s, t, at both ends? */
      d_pedgespanpackage->light = d_light;
      d_pedgespanpackage->zi    = d_zi;

      if (r_phongshading.value) {
         d_pedgespanpackage->nx = d_nx;
         d_pedgespanpackage->ny = d_ny;
         d_pedgespanpackage->nz = d_nz;
      }

      d_pedgespanpackage++;

      errorterm += erroradjustup;
      if (errorterm >= 0) {
         d_pdest += d_pdestextrastep;
         d_pz += d_pzextrastep;
         d_aspancount += d_countextrastep;
         d_ptex += d_ptexextrastep;
         d_sfrac += d_sfracextrastep;
         d_ptex += d_sfrac >> 16;

         d_sfrac &= 0xFFFF;
         d_tfrac += d_tfracextrastep;
         if (d_tfrac & 0x10000) {
            d_ptex += r_affinetridesc.skinwidth;
            d_tfrac &= 0xFFFF;
         }
         d_light += d_lightextrastep;
         d_zi += d_ziextrastep;
         if (r_phongshading.value) {
            d_nx += d_nxextrastep;
            d_ny += d_nyextrastep;
            d_nz += d_nzextrastep;
         }
         errorterm -= erroradjustdown;
      } else {
         d_pdest += d_pdestbasestep;
         d_pz += d_pzbasestep;
         d_aspancount += ubasestep;
         d_ptex += d_ptexbasestep;
         d_sfrac += d_sfracbasestep;
         d_ptex += d_sfrac >> 16;
         d_sfrac &= 0xFFFF;
         d_tfrac += d_tfracbasestep;
         if (d_tfrac & 0x10000) {
            d_ptex += r_affinetridesc.skinwidth;
            d_tfrac &= 0xFFFF;
         }
         d_light += d_lightbasestep;
         d_zi += d_zibasestep;
         if (r_phongshading.value) {
            d_nx += d_nxbasestep;
            d_ny += d_nybasestep;
            d_nz += d_nzbasestep;
         }
      }
   } while (--height);
}

/*
===================
D_PolysetSetUpForLineScan
====================
*/
static void D_PolysetSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
			  fixed8_t endvertu, fixed8_t endvertv)
{
   int tm = endvertu - startvertu;
   int tn = endvertv - startvertv;

   errorterm = -1;

   if (((tm <= 16) && (tm >= -15)) && ((tn <= 16) && (tn >= -15)))
   {
      adivtab_t *ptemp = &adivtab[((tm + 15) << 5) + (tn + 15)];
      ubasestep = ptemp->quotient;
      erroradjustup = ptemp->remainder;
      erroradjustdown = tn;
   }
   else
   {
      double dm = (double)tm;
      double dn = (double)tn;

      FloorDivMod(dm, dn, &ubasestep, &erroradjustup);

      erroradjustdown = dn;
   }
}

/*
================
D_PolysetCalcGradients
================
*/
void D_PolysetCalcGradients(int swidth)
{
   static float
      xstepdenominv
      ,   ystepdenominv
      ,   t0
      ,   t1
      ,   p01_minus_p21
      ,   p11_minus_p21
      ,   p00_minus_p20
      ,   p10_minus_p20
      ;

   xstepdenominv = 1.0 / (float)d_xdenom;
   ystepdenominv = -xstepdenominv;

   /* mankrip - optimization */
   p00_minus_p20 = (r_p0[0] - r_p2[0]) * ystepdenominv;
   p01_minus_p21 = (r_p0[1] - r_p2[1]) * xstepdenominv;
   p10_minus_p20 = (r_p1[0] - r_p2[0]) * ystepdenominv;
   p11_minus_p21 = (r_p1[1] - r_p2[1]) * xstepdenominv;

   t0 = r_p0[2] - r_p2[2];
   t1 = r_p1[2] - r_p2[2];
   r_sstepx  = (int)      (t1 * p01_minus_p21 - t0 * p11_minus_p21);
   r_sstepy  = (int)      (t1 * p00_minus_p20 - t0 * p10_minus_p20);

   t0 = r_p0[3] - r_p2[3];
   t1 = r_p1[3] - r_p2[3];
   r_tstepx  = (int)      (t1 * p01_minus_p21 - t0 * p11_minus_p21);
   r_tstepy  = (int)      (t1 * p00_minus_p20 - t0 * p10_minus_p20);

   /* ceil () for light so positive steps are exaggerated, negative steps diminished, */
   /* pushing us away from underflow toward overflow. */
   /* Underflow is very visible, overflow is very unlikely, because of ambient lighting */
   t0 = r_p0[4] - r_p2[4];
   t1 = r_p1[4] - r_p2[4];
   r_lstepx  = (int) ceilf (t1 * p01_minus_p21 - t0 * p11_minus_p21);
   r_lstepy  = (int) ceilf (t1 * p00_minus_p20 - t0 * p10_minus_p20);

   t0 = r_p0[5] - r_p2[5];
   t1 = r_p1[5] - r_p2[5];
   r_zistepx = (int)      (t1 * p01_minus_p21 - t0 * p11_minus_p21);
   r_zistepy = (int)      (t1 * p00_minus_p20 - t0 * p10_minus_p20);

   if (r_phongshading.value) {
      /* normal gradients (Phong shading): 3 axes x 2 directions each.
       * These are float because normals are unit-length floats and we
       * want the per-pixel interpolated normal to be representable. */
      float ft0, ft1;
      ft0 = r_n0[0] - r_n2[0];
      ft1 = r_n1[0] - r_n2[0];
      r_nxstepx = ft1 * p01_minus_p21 - ft0 * p11_minus_p21;
      r_nxstepy = ft1 * p00_minus_p20 - ft0 * p10_minus_p20;

      ft0 = r_n0[1] - r_n2[1];
      ft1 = r_n1[1] - r_n2[1];
      r_nystepx = ft1 * p01_minus_p21 - ft0 * p11_minus_p21;
      r_nystepy = ft1 * p00_minus_p20 - ft0 * p10_minus_p20;

      ft0 = r_n0[2] - r_n2[2];
      ft1 = r_n1[2] - r_n2[2];
      r_nzstepx = ft1 * p01_minus_p21 - ft0 * p11_minus_p21;
      r_nzstepy = ft1 * p00_minus_p20 - ft0 * p10_minus_p20;
   }

   a_sstepxfrac = r_sstepx & 0xFFFF;
   a_tstepxfrac = r_tstepx & 0xFFFF;

   a_ststepxwhole = swidth * (r_tstepx >> 16) + (r_sstepx >> 16);
}


/*
================
D_PolysetDrawSpansPhong8

Phong-shaded variant of D_PolysetDrawSpans8.  Per-pixel:
  - interpolate the vertex normal (nx, ny, nz)
  - renormalize via 1/sqrt(|N|^2)
  - recompute dot(N, L) and turn it into a colormap row index using
    the same pre-scaled r_ambientlight / r_shadelight as the Gouraud
    per-vertex setup.

Cost vs Gouraud: ~6x rasterizer ops per alias-model pixel.  Acceptable
on x86-64 / ARM64; punishing on 32-bit ARM.  Default off behind cvar.
================
*/
void D_PolysetDrawSpansPhong8(spanpackage_t *pspanpackage)
{
   byte *lpdest;
   byte *lptex;
   int lsfrac, ltfrac;
   int lzi;
   int16_t *lpz;
   float lnx, lny, lnz;
   const float Lx = r_plightvec[0];
   const float Ly = r_plightvec[1];
   const float Lz = r_plightvec[2];
   const int   ambient = r_ambientlight;
   const float shade   = r_shadelight;
   const int (*dtab)[4] = (r_lightdither.value != 0.0f) ? dither_bayer4 : dither_zero4;

   /* Specular setup.  When r_phongshading >= 2 the rasterizer adds a
    * Blinn-Phong specular term: the LUT is built lazily on demand
    * (and only rebuilt if the exponent changes), and the halfway
    * vector is hoisted to scalar locals for the inner loop.  When
    * specular is off, do_specular is 0 and the inner-loop branch is
    * skipped; the LUT and halfway vector aren't touched. */
   const int do_specular = (r_phongshading.value >= 2.0f) ? 1 : 0;
   const float Hx = r_phalfvec[0];
   const float Hy = r_phalfvec[1];
   const float Hz = r_phalfvec[2];
   if (do_specular && specular_lut_exponent != PHONG_SPECULAR_EXPONENT)
      D_BuildSpecularLUT(PHONG_SPECULAR_EXPONENT);

   do
   {
      int lcount = d_aspancount - pspanpackage->count;

      errorterm += erroradjustup;
      if (errorterm >= 0)
      {
         d_aspancount += d_countextrastep;
         errorterm -= erroradjustdown;
      }
      else
         d_aspancount += ubasestep;

      if (lcount > 0)
      {
         const int span_offset = (int)((byte*)pspanpackage->pdest - (byte*)d_viewbuffer);
         const int span_y      = span_offset / screenwidth;
         int       span_x      = span_offset - span_y * screenwidth;
         const int *drow       = dtab[span_y & 3];

         lpdest = (byte*)pspanpackage->pdest;
         lptex  = pspanpackage->ptex;
         lpz    = pspanpackage->pz;
         lsfrac = pspanpackage->sfrac;
         ltfrac = pspanpackage->tfrac;
         lzi    = pspanpackage->zi;
         lnx    = pspanpackage->nx;
         lny    = pspanpackage->ny;
         lnz    = pspanpackage->nz;

         do
         {
            if ((lzi >> 16) >= *lpz) {
               float nlen2 = lnx*lnx + lny*lny + lnz*lnz;
               int   light = ambient;
               if (nlen2 > 0.0f) {
                  float inv = 1.0f / sqrtf(nlen2);
                  float dot = (lnx*Lx + lny*Ly + lnz*Lz) * inv;
                  if (dot < 0.0f) {
                     light += (int)(shade * dot);
                     if (light < 0) light = 0;
                  }
                  /* Specular: dot(N_normalized, H) reuses the same
                   * 1/|N| inverse computed for the Lambert step.
                   * Positive ndoth means N points toward the
                   * halfway vector -- closer to a perfect specular
                   * reflection toward the camera.  Lower `light`
                   * = brighter colormap row, so subtract. */
                  if (do_specular) {
                     float ndoth = (lnx*Hx + lny*Hy + lnz*Hz) * inv;
                     if (ndoth > 0.0f) {
                        int idx = (int)(ndoth * 255.0f);
                        if (idx > 255) idx = 255;
                        light -= specular_lut[idx];
                        if (light < 0) light = 0;
                     }
                  }
               }
               light += drow[span_x & 3];
               *lpdest = ((byte *)acolormap)[*lptex + (light & 0xFF00)];
               *lpz = lzi >> 16;
            }
            lpdest++;
            span_x++;
            lzi += r_zistepx;
            lpz++;
            lnx += r_nxstepx;
            lny += r_nystepx;
            lnz += r_nzstepx;
            lptex += a_ststepxwhole;
            lsfrac += a_sstepxfrac;
            lptex += lsfrac >> 16;
            lsfrac &= 0xFFFF;
            ltfrac += a_tstepxfrac;
            if (ltfrac & 0x10000) {
               lptex += r_affinetridesc.skinwidth;
               ltfrac &= 0xFFFF;
            }
         } while (--lcount);
      }

      pspanpackage++;
   } while (pspanpackage->count != -999999);
}


/*
================
D_PolysetDrawSpansPhongRGB

Combined Phong shading + colored lighting.  Active when both
r_phongshading.value and coloredlights are true.

Same per-pixel Phong dot product as D_PolysetDrawSpansPhong8 (so
silhouettes get smooth lighting instead of Gouraud-banded), then
the resulting shade factor is applied to the texel-RGB times
lightcolor[] modulation from D_PolysetDrawSpansRGB (so the model
also gets the local BSP lightmap tint).

Cost: ~6x rasterizer arithmetic vs Gouraud (Phong path) plus the
small extra work of integer RGB modulation.  Default off behind
both cvars.
================
*/
void D_PolysetDrawSpansPhongRGB(spanpackage_t *pspanpackage)
{
   byte *lpdest;
   byte *lptex;
   int lsfrac, ltfrac;
   int lzi;
   int16_t *lpz;
   float lnx, lny, lnz;
   const float Lx = r_plightvec[0];
   const float Ly = r_plightvec[1];
   const float Lz = r_plightvec[2];
   const int   ambient = r_ambientlight;
   const float shadelt = r_shadelight;
   byte ah;
   unsigned trans[3];
   unsigned char *pix24;
   const int (*dtab)[4] = (r_lightdither.value != 0.0f) ? dither_bayer4 : dither_zero4;
   /* See D_PolysetDrawSpansPhong8 for specular setup rationale. */
   const int do_specular = (r_phongshading.value >= 2.0f) ? 1 : 0;
   const float Hx = r_phalfvec[0];
   const float Hy = r_phalfvec[1];
   const float Hz = r_phalfvec[2];
   if (do_specular && specular_lut_exponent != PHONG_SPECULAR_EXPONENT)
      D_BuildSpecularLUT(PHONG_SPECULAR_EXPONENT);

   do
   {
      int lcount = d_aspancount - pspanpackage->count;

      errorterm += erroradjustup;
      if (errorterm >= 0)
      {
         d_aspancount += d_countextrastep;
         errorterm -= erroradjustdown;
      }
      else
         d_aspancount += ubasestep;

      if (lcount > 0)
      {
         const int span_offset = (int)((byte*)pspanpackage->pdest - (byte*)d_viewbuffer);
         const int span_y      = span_offset / screenwidth;
         int       span_x      = span_offset - span_y * screenwidth;
         const int *drow       = dtab[span_y & 3];

         lpdest = (byte*)pspanpackage->pdest;
         lptex  = pspanpackage->ptex;
         lpz    = pspanpackage->pz;
         lsfrac = pspanpackage->sfrac;
         ltfrac = pspanpackage->tfrac;
         lzi    = pspanpackage->zi;
         lnx    = pspanpackage->nx;
         lny    = pspanpackage->ny;
         lnz    = pspanpackage->nz;

         do
         {
            if ((lzi >> 16) >= *lpz) {
               if (*lptex < host_fullbrights) {
                  /* Per-pixel Phong: renormalize the interpolated normal
                   * and recompute dot(N, L). */
                  float nlen2 = lnx*lnx + lny*lny + lnz*lnz;
                  int   light = ambient;
                  int   shade;
                  if (nlen2 > 0.0f) {
                     float inv = 1.0f / sqrtf(nlen2);
                     float dot = (lnx*Lx + lny*Ly + lnz*Lz) * inv;
                     if (dot < 0.0f) {
                        light += (int)(shadelt * dot);
                        if (light < 0) light = 0;
                     }
                     /* Specular contribution: same dot-product
                      * formulation as the grayscale Phong path,
                      * subtracting from `light` to brighten. */
                     if (do_specular) {
                        float ndoth = (lnx*Hx + lny*Hy + lnz*Hz) * inv;
                        if (ndoth > 0.0f) {
                           int idx = (int)(ndoth * 255.0f);
                           if (idx > 255) idx = 255;
                           light -= specular_lut[idx];
                           if (light < 0) light = 0;
                        }
                     }
                  }

                  light += drow[span_x & 3];

                  /* Same shade-factor conversion as D_PolysetDrawSpansRGB:
                   * map colormap-row-space light into a 0..255 brightness
                   * scalar where 255 = brightest. */
                  shade = 255 - ((light & 0xFF00) >> 8);
                  if (shade < 0) shade = 0;

                  /* Texel through bright colormap row -> 24-bit RGB. */
                  ah = ((byte *)acolormap)[*lptex];
                  pix24 = (unsigned char *)&d_8to24table[ah];

                  /* Modulate texel by light color and Phong shade.
                   * Same scaling as the Gouraud RGB path: 255^3 -> >>18 -> 0..63. */
                  trans[0] = (pix24[0] * (int)lightcolor[0] * shade) >> 18;
                  trans[1] = (pix24[1] * (int)lightcolor[1] * shade) >> 18;
                  trans[2] = (pix24[2] * (int)lightcolor[2] * shade) >> 18;

                  if (trans[0] > 63) trans[0] = 63;
                  if (trans[1] > 63) trans[1] = 63;
                  if (trans[2] > 63) trans[2] = 63;

                  *lpdest = palmap2[trans[0]][trans[1]][trans[2]];
               }
               else {
                  /* Fullbright: skin pixel passes through unmodulated. */
                  *lpdest = *lptex;
               }
               *lpz = lzi >> 16;
            }
            lpdest++;
            span_x++;
            lzi += r_zistepx;
            lpz++;
            lnx += r_nxstepx;
            lny += r_nystepx;
            lnz += r_nzstepx;
            lptex += a_ststepxwhole;
            lsfrac += a_sstepxfrac;
            lptex += lsfrac >> 16;
            lsfrac &= 0xFFFF;
            ltfrac += a_tstepxfrac;
            if (ltfrac & 0x10000) {
               lptex += r_affinetridesc.skinwidth;
               ltfrac &= 0xFFFF;
            }
         } while (--lcount);
      }

      pspanpackage++;
   } while (pspanpackage->count != -999999);
}


/*
================
D_PolysetDrawSpans8
================
*/
void D_PolysetDrawSpansRGB(spanpackage_t *pspanpackage);

void D_PolysetDrawSpans8(spanpackage_t *pspanpackage)
{
   byte *lpdest;
   byte *lptex;
   int lsfrac, ltfrac;
   int llight;
   int lzi;
   int16_t *lpz;

   /* Pull the dither table once per draw call.  When r_lightdither
    * is off, dtab points at the all-zero matrix and the per-pixel
    * lookup folds out to a no-op addition. */
   const int (*dtab)[4] = (r_lightdither.value != 0.0f) ? dither_bayer4 : dither_zero4;

   do
   {
      int lcount = d_aspancount - pspanpackage->count;

      errorterm += erroradjustup;
      if (errorterm >= 0)
      {
         d_aspancount += d_countextrastep;
         errorterm -= erroradjustdown;
      }
      else
         d_aspancount += ubasestep;

      if (lcount > 0)
      {
         /* Compute the span's screen-space (x, y) once, then walk
          * x along with lpdest in the inner loop.  The framebuffer
          * is a flat byte array of size screenwidth * screenheight,
          * so y = offset / screenwidth and the span's starting x
          * is the remainder. */
         const int span_offset = (int)((byte*)pspanpackage->pdest - (byte*)d_viewbuffer);
         const int span_y      = span_offset / screenwidth;
         int       span_x      = span_offset - span_y * screenwidth;
         const int *drow       = dtab[span_y & 3];

         lpdest = (byte*)pspanpackage->pdest;
         lptex = pspanpackage->ptex;
         lpz = pspanpackage->pz;
         lsfrac = pspanpackage->sfrac;
         ltfrac = pspanpackage->tfrac;
         llight = pspanpackage->light;
         lzi = pspanpackage->zi;

         do
         {
            if ((lzi >> 16) >= *lpz) {
               int dlight = llight + drow[span_x & 3];
               *lpdest = ((byte *)acolormap)[*lptex + (dlight & 0xFF00)];
               *lpz = lzi >> 16;
            }
            lpdest++;
            span_x++;
            lzi += r_zistepx;
            lpz++;
            llight += r_lstepx;
            lptex += a_ststepxwhole;
            lsfrac += a_sstepxfrac;
            lptex += lsfrac >> 16;
            lsfrac &= 0xFFFF;
            ltfrac += a_tstepxfrac;
            if (ltfrac & 0x10000) {
               lptex += r_affinetridesc.skinwidth;
               ltfrac &= 0xFFFF;
            }
         } while (--lcount);
      }

      pspanpackage++;
   } while (pspanpackage->count != -999999);
}

/* leilei - quickly hacked colored lighting on models */
void D_PolysetDrawSpansRGB(spanpackage_t *pspanpackage)
{
   byte *lpdest;
   byte *lptex;
   byte ah;
   unsigned trans[3];
   unsigned char *pix24;	/* leilei - colored lighting */
   int lsfrac, ltfrac;
   int llight;
   int lzi;
   short *lpz;
   const int (*dtab)[4] = (r_lightdither.value != 0.0f) ? dither_bayer4 : dither_zero4;

   do
   {
      int lcount = d_aspancount - pspanpackage->count;

      errorterm += erroradjustup;
      if (errorterm >= 0)
      {
         d_aspancount += d_countextrastep;
         errorterm -= erroradjustdown;
      }
      else
         d_aspancount += ubasestep;

      if (lcount > 0)
      {
         const int span_offset = (int)((byte*)pspanpackage->pdest - (byte*)d_viewbuffer);
         const int span_y      = span_offset / screenwidth;
         int       span_x      = span_offset - span_y * screenwidth;
         const int *drow       = dtab[span_y & 3];

         lpdest = (byte*)pspanpackage->pdest;
         lptex = pspanpackage->ptex;
         lpz = pspanpackage->pz;
         lsfrac = pspanpackage->sfrac;
         ltfrac = pspanpackage->tfrac;
         llight = pspanpackage->light;
         lzi = pspanpackage->zi;

         do
         {
            if ((lzi >> 16) >= *lpz) {
				/* leilei - gross simple hack. it goes like this */
				/* lpdest = the skin...... */
				/* 	TIMES */
				/* Colored lighting color */
				/*      AND THEN */
				/* colormap is blended on it */
		if (*lptex < host_fullbrights)
		{
			int shade;
			int dlight = llight + drow[span_x & 3];

			/* Pull texel through the bright colormap row to get its base
			 * palette index, then look up the 24-bit RGB. */
			ah = ((byte *)acolormap)[*lptex];
			pix24 = (unsigned char *)&d_8to24table[ah];

			/* Lambertian shading factor.  llight is the Gouraud-interpolated
			 * light in colormap-row space: high byte = row index, where 0
			 * is brightest and 63 (== 63<<8 = 16128) is darkest.  Convert
			 * to a 0..255 brightness factor where 255 = brightest.  Without
			 * this the model gets a flat tint regardless of normal direction. */
			shade = 255 - ((dlight & 0xFF00) >> 8);
			if (shade < 0) shade = 0;

			/* Modulate texel by light color and Lambertian shade.
			 *   pix24[i]      0..255   (8 bits, texel RGB channel)
			 *   lightcolor[i] 0..255   (8 bits, BSP-sampled tint at model origin)
			 *   shade         0..255   (8 bits, Lambertian factor)
			 * Product max = 255^3 = ~16.6M.  palmap2 wants 0..63 per channel
			 * (it's a 64x64x64 LUT), so divide by 2^18 and clamp. */
			trans[0] = (pix24[0] * (int)lightcolor[0] * shade) >> 18;
			trans[1] = (pix24[1] * (int)lightcolor[1] * shade) >> 18;
			trans[2] = (pix24[2] * (int)lightcolor[2] * shade) >> 18;

			if (trans[0] > 63) trans[0] = 63;
			if (trans[1] > 63) trans[1] = 63;
			if (trans[2] > 63) trans[2] = 63;

			*lpdest = palmap2[trans[0]][trans[1]][trans[2]];
		}
		else
		{
			/* Fullbright: skin pixel passes through unmodulated. */
			*lpdest = *lptex;
		}
               *lpz = lzi >> 16;
            }
            lpdest++;
            span_x++;
            lzi += r_zistepx;
            lpz++;
            llight += r_lstepx;
            lptex += a_ststepxwhole;
            lsfrac += a_sstepxfrac;
            lptex += lsfrac >> 16;
            lsfrac &= 0xFFFF;
            ltfrac += a_tstepxfrac;
            if (ltfrac & 0x10000) {
               lptex += r_affinetridesc.skinwidth;
               ltfrac &= 0xFFFF;
            }
         } while (--lcount);
      }

      pspanpackage++;
   } while (pspanpackage->count != -999999);
}

/*
================
D_RasterizeAliasPolySmooth
================
*/

extern int coloredlights;

void D_RasterizeAliasPolySmooth(void)
{
   int working_lstepx, originalcount;

   int *plefttop = pedgetable->pleftedgevert0;
   int *prighttop = pedgetable->prightedgevert0;

   int *pleftbottom = pedgetable->pleftedgevert1;
   int *prightbottom = pedgetable->prightedgevert1;

   int initialleftheight = pleftbottom[1] - plefttop[1];
   int initialrightheight = prightbottom[1] - prighttop[1];

   /**/
   /* set the s, t, and light gradients, which are consistent across the triangle */
   /* because being a triangle, things are affine */
      D_PolysetCalcGradients(r_affinetridesc.skinwidth);

   /**/
   /* rasterize the polygon */
   /**/

   /**/
   /* scan out the top (and possibly only) part of the left edge */
   /**/
   D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
         pleftbottom[0], pleftbottom[1]);

   d_pedgespanpackage = a_spans;

   ystart = plefttop[1];
   d_aspancount = plefttop[0] - prighttop[0];

   d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
      (plefttop[3] >> 16) * r_affinetridesc.skinwidth;
   d_sfrac = plefttop[2] & 0xFFFF;
   d_tfrac = plefttop[3] & 0xFFFF;
   d_pzbasestep = d_zwidth + ubasestep;
   d_pzextrastep = d_pzbasestep + 1;
   d_light = plefttop[4];
   d_zi = plefttop[5];

   d_pdestbasestep = screenwidth + ubasestep;
   d_pdestextrastep = d_pdestbasestep + 1;
   d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
   d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

   /* TODO: can reuse partial expressions here */

   /*
    * for negative steps in x along left edge, bias toward overflow rather
    * than underflow (sort of turning the floor () we did in the gradient
    * calcs into ceil (), but plus a little bit)
    */
   if (ubasestep < 0)
      working_lstepx = r_lstepx - 1;
   else
      working_lstepx = r_lstepx;

   d_countextrastep = ubasestep + 1;
   d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
      ((r_tstepy + r_tstepx * ubasestep) >> 16) * r_affinetridesc.skinwidth;
   d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
   d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
   d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
   d_zibasestep = r_zistepy + r_zistepx * ubasestep;

   d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
      ((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
      r_affinetridesc.skinwidth;
   d_sfracextrastep = (r_sstepy + r_sstepx * d_countextrastep) & 0xFFFF;
   d_tfracextrastep = (r_tstepy + r_tstepx * d_countextrastep) & 0xFFFF;
   d_lightextrastep = d_lightbasestep + working_lstepx;
   d_ziextrastep = d_zibasestep + r_zistepx;

   if (r_phongshading.value) {
      /* Seed the normal interpolant at the start of the left edge.
       * plefttop is one of r_p0/p1/p2 depending on edgetable; the
       * companion normal is whichever of r_n0/n1/n2 matches the same
       * vertex.  Pick by pointer identity. No integer-rounding bias
       * (unlike the integer light's working_lstepx). */
      const float *seed_n = (plefttop == r_p0) ? r_n0
                          : (plefttop == r_p1) ? r_n1 : r_n2;
      d_nx = seed_n[0];
      d_ny = seed_n[1];
      d_nz = seed_n[2];

      d_nxbasestep = r_nxstepy + r_nxstepx * ubasestep;
      d_nybasestep = r_nystepy + r_nystepx * ubasestep;
      d_nzbasestep = r_nzstepy + r_nzstepx * ubasestep;

      d_nxextrastep = d_nxbasestep + r_nxstepx;
      d_nyextrastep = d_nybasestep + r_nystepx;
      d_nzextrastep = d_nzbasestep + r_nzstepx;
   }

   D_PolysetScanLeftEdge(initialleftheight);

   /**/
   /* scan out the bottom part of the left edge, if it exists */
   /**/
   if (pedgetable->numleftedges == 2) {
      int height;

      plefttop = pleftbottom;
      pleftbottom = pedgetable->pleftedgevert2;

      D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
            pleftbottom[0], pleftbottom[1]);

      height = pleftbottom[1] - plefttop[1];

      /* TODO: make this a function; modularize this function in general */

      ystart = plefttop[1];
      d_aspancount = plefttop[0] - prighttop[0];
      d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
         (plefttop[3] >> 16) * r_affinetridesc.skinwidth;
      d_sfrac = 0;
      d_tfrac = 0;
      d_light = plefttop[4];
      d_zi = plefttop[5];

      d_pdestbasestep = screenwidth + ubasestep;
      d_pdestextrastep = d_pdestbasestep + 1;
      d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
      d_pzbasestep = d_zwidth + ubasestep;
      d_pzextrastep = d_pzbasestep + 1;
      d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

      if (ubasestep < 0)
         working_lstepx = r_lstepx - 1;
      else
         working_lstepx = r_lstepx;

      d_countextrastep = ubasestep + 1;
      d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
         ((r_tstepy + r_tstepx * ubasestep) >> 16) *
         r_affinetridesc.skinwidth;
      d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
      d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
      d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
      d_zibasestep = r_zistepy + r_zistepx * ubasestep;

      d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
         ((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
         r_affinetridesc.skinwidth;
      d_sfracextrastep = (r_sstepy + r_sstepx * d_countextrastep) & 0xFFFF;
      d_tfracextrastep = (r_tstepy + r_tstepx * d_countextrastep) & 0xFFFF;
      d_lightextrastep = d_lightbasestep + working_lstepx;
      d_ziextrastep = d_zibasestep + r_zistepx;

      if (r_phongshading.value) {
         /* Reseed the normal at the apex of the bottom-edge segment.
          * Same pointer-identity trick as the top edge. */
         const float *seed_n = (plefttop == r_p0) ? r_n0
                             : (plefttop == r_p1) ? r_n1 : r_n2;
         d_nx = seed_n[0];
         d_ny = seed_n[1];
         d_nz = seed_n[2];

         d_nxbasestep = r_nxstepy + r_nxstepx * ubasestep;
         d_nybasestep = r_nystepy + r_nystepx * ubasestep;
         d_nzbasestep = r_nzstepy + r_nzstepx * ubasestep;

         d_nxextrastep = d_nxbasestep + r_nxstepx;
         d_nyextrastep = d_nybasestep + r_nystepx;
         d_nzextrastep = d_nzbasestep + r_nzstepx;
      }

      D_PolysetScanLeftEdge(height);
   }
   /* scan out the top (and possibly only) part of the right edge, updating the */
   /* count field */
   d_pedgespanpackage = a_spans;

   D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
         prightbottom[0], prightbottom[1]);
   d_aspancount = 0;
   d_countextrastep = ubasestep + 1;
   originalcount = a_spans[initialrightheight].count;
   a_spans[initialrightheight].count = -999999;	/* mark end of the spanpackages */

   if (r_phongshading.value && coloredlights)
      D_PolysetDrawSpansPhongRGB(a_spans);
   else if (r_phongshading.value)
      D_PolysetDrawSpansPhong8(a_spans);
   else if (coloredlights)
      D_PolysetDrawSpansRGB(a_spans);
   else
      D_PolysetDrawSpans8(a_spans);


   /* scan out the bottom part of the right edge, if it exists */
   if (pedgetable->numrightedges == 2) {
      int height;
      spanpackage_t *pstart;

      pstart = a_spans + initialrightheight;
      pstart->count = originalcount;

      d_aspancount = prightbottom[0] - prighttop[0];

      prighttop = prightbottom;
      prightbottom = pedgetable->prightedgevert2;

      height = prightbottom[1] - prighttop[1];

      D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
            prightbottom[0], prightbottom[1]);

      d_countextrastep = ubasestep + 1;
      a_spans[initialrightheight + height].count = -999999;
      /* mark end of the spanpackages */

      if (r_phongshading.value && coloredlights)
         D_PolysetDrawSpansPhongRGB(pstart);
      else if (r_phongshading.value)
         D_PolysetDrawSpansPhong8(pstart);
      else if (coloredlights)
         D_PolysetDrawSpansRGB(pstart);
      else
         D_PolysetDrawSpans8(pstart);
   }
}


/*
================
D_PolysetSetEdgeTable
================
*/
void D_PolysetSetEdgeTable(void)
{
   int edgetableindex = 0;		/* assume the vertices are already in */
   /*  top to bottom order */

   /* determine which edges are right & left, and the order in which */
   /* to rasterize them */
   if (r_p0[1] >= r_p1[1]) {
      if (r_p0[1] == r_p1[1]) {
         if (r_p0[1] < r_p2[1])
            pedgetable = &edgetables[2];
         else
            pedgetable = &edgetables[5];

         return;
      } else {
         edgetableindex = 1;
      }
   }

   if (r_p0[1] == r_p2[1]) {
      if (edgetableindex)
         pedgetable = &edgetables[8];
      else
         pedgetable = &edgetables[9];

      return;
   } else if (r_p1[1] == r_p2[1]) {
      if (edgetableindex)
         pedgetable = &edgetables[10];
      else
         pedgetable = &edgetables[11];

      return;
   }

   if (r_p0[1] > r_p2[1])
      edgetableindex += 2;

   if (r_p1[1] > r_p2[1])
      edgetableindex += 4;

   pedgetable = &edgetables[edgetableindex];
}


/*
================
D_DrawShadowTriangle

Soft-fill rasterizer for projected alias-model shadows, used by
R_AliasDrawShadow.  Takes three screen-space vertices (x, y) plus
a per-vertex inverse-depth `zi`, and darkens each covered pixel
through the colormap (z-test, no z-write).

The shape is essentially the model's silhouette mashed flat onto
a floor plane, projected along a fixed light direction so the
shadow extends across the floor in world space rather than
collapsing to a line at the entity's feet when viewed edge-on.

Darkening: instead of solid black (palette 0), each shadow pixel
is read back and looked up through the colormap at a high
darkness row.  This produces a darkened version of whatever
texture was already on the floor underneath, so the shadow reads
as "this part of the floor is in shade" rather than as a black
silhouette pasted on top.  The cost is one extra byte read and
one extra colormap lookup per pixel vs. solid fill -- still
well within the budget of the per-entity rasterizer pass.

Z values are interpolated per-scanline and per-pixel across the
span, which is needed because a single shadow triangle can span
both nearby walls and far floor (a corpse near a doorway, etc.):
a per-triangle constant z would let shadow pixels bleed onto
walls that are physically in front of the floor area.  Linear
interpolation in screen space is not perspective-correct, but
the depth ranges within a single triangle are narrow enough that
the error is well below the z-buffer's quantization step.

Not writing z avoids polluting the depth buffer for geometry
drawn later in the frame.

The rasterizer is integer-screen-coord only and clips
conservatively to [0..screenwidth-1] x [0..vid.height-1].  Caller
is responsible for back-face culling and near-plane rejection.
================
*/
void
D_DrawShadowTriangle(const float v0[2], const float v1[2], const float v2[2],
                     float zi0, float zi1, float zi2)
{
    const float *p0 = v0, *p1 = v1, *p2 = v2, *tmp;
    float z0 = zi0, z1 = zi1, z2 = zi2, ztmp;
    float yspan02, yspan01, yspan12;
    float dx_long, dz_long;
    float dx_short_top, dz_short_top;
    float dx_short_bot, dz_short_bot;
    int   y, y_top, y_mid, y_bot;
    int   y_clamp_top, y_clamp_bot;

    /* Defensive: reject NaN, Inf, or absurd inputs.  Bad floats
     * here (e.g. from a degenerate projection where t blew up
     * because L_z went to zero, or from triangles fed garbage
     * coordinates due to vertex-index corruption upstream) would
     * cause undefined behaviour on the float->int casts below
     * and produce out-of-bounds y_top/y_mid/y_bot values that
     * survive the vid.height clamp.  The chosen bound (1.0e6)
     * is far past any legitimate screen coordinate or 1/depth.
     * NaN fails both the < and > tests so it's also caught. */
    if (!(v0[0] > -1.0e6f && v0[0] < 1.0e6f) ||
        !(v0[1] > -1.0e6f && v0[1] < 1.0e6f) ||
        !(v1[0] > -1.0e6f && v1[0] < 1.0e6f) ||
        !(v1[1] > -1.0e6f && v1[1] < 1.0e6f) ||
        !(v2[0] > -1.0e6f && v2[0] < 1.0e6f) ||
        !(v2[1] > -1.0e6f && v2[1] < 1.0e6f) ||
        !(zi0   > -1.0e6f && zi0   < 1.0e6f) ||
        !(zi1   > -1.0e6f && zi1   < 1.0e6f) ||
        !(zi2   > -1.0e6f && zi2   < 1.0e6f))
        return;

    /* Defensive: validate the vid pointers we'll dereference per
     * pixel.  vid.colormap is hunk-allocated (host_colormap, set
     * once at Host_Init) and so should be stable across map
     * transitions, but heap reallocations on a content reload
     * can leave it pointing at freed memory.  d_viewbuffer is
     * reseated by D_SetupFrame from vid.buffer, which is the
     * malloc'd framebuffer; if the rasterizer ever runs with a
     * stale pointer (e.g. one frame that slipped through during
     * the VID_Shutdown -> VID_Init window of a reload), the
     * inner-loop write to prow[x] / read from cmap[prow[x]]
     * lands in unmapped memory and crashes.
     *
     * Hunk_PointerInHunk checks colormap; for d_viewbuffer the
     * test is just non-NULL since malloc'd framebuffers don't
     * fall in the hunk. */
    if (!Hunk_PointerInHunk(vid.colormap))
        return;
    if (!d_viewbuffer)
        return;

    /* Sort verts top-to-bottom by Y, dragging per-vertex z values
     * along with the position pointers. */
    if (p0[1] > p1[1]) { tmp = p0; p0 = p1; p1 = tmp; ztmp = z0; z0 = z1; z1 = ztmp; }
    if (p1[1] > p2[1]) { tmp = p1; p1 = p2; p2 = tmp; ztmp = z1; z1 = z2; z2 = ztmp; }
    if (p0[1] > p1[1]) { tmp = p0; p0 = p1; p1 = tmp; ztmp = z0; z0 = z1; z1 = ztmp; }

    y_top = (int)p0[1];
    y_mid = (int)p1[1];
    y_bot = (int)p2[1];

    /* Clip Y to the 3D viewport (r_refdef.vrect), not the full
     * framebuffer.  The shadow rasterizer used to clamp to
     * [0, vid.height) and [0, screenwidth), which lets a long
     * shadow that projects past the bottom of the viewport keep
     * darkening pixels in the status-bar strip vid.buffer holds
     * below it -- visible as a moving silhouette over the static
     * HUD backdrop.  r_refdef.vrectbottom is exclusive (== vrect.y
     * + vrect.height, from r_main.c::R_ViewChanged), so the last
     * valid row is vrectbottom - 1; same for vrectright on the X
     * axis below. */
    if (y_bot < r_refdef.vrect.y || y_top >= r_refdef.vrectbottom)
        return;

    y_clamp_top = y_top;
    y_clamp_bot = y_bot;
    if (y_clamp_top < r_refdef.vrect.y)
        y_clamp_top = r_refdef.vrect.y;
    if (y_clamp_bot >= r_refdef.vrectbottom)
        y_clamp_bot = r_refdef.vrectbottom - 1;
    if (y_clamp_top > y_clamp_bot)   return;

    yspan02 = p2[1] - p0[1];
    if (yspan02 < 1.0f) return;
    dx_long = (p2[0] - p0[0]) / yspan02;
    dz_long = (z2     - z0   ) / yspan02;

    yspan01 = p1[1] - p0[1];
    yspan12 = p2[1] - p1[1];
    if (yspan01 >= 1.0f) {
	dx_short_top = (p1[0] - p0[0]) / yspan01;
	dz_short_top = (z1    - z0   ) / yspan01;
    } else {
	dx_short_top = 0.0f;
	dz_short_top = 0.0f;
    }
    if (yspan12 >= 1.0f) {
	dx_short_bot = (p2[0] - p1[0]) / yspan12;
	dz_short_bot = (z2    - z1   ) / yspan12;
    } else {
	dx_short_bot = 0.0f;
	dz_short_bot = 0.0f;
    }

    for (y = y_clamp_top; y <= y_clamp_bot; y++) {
	float fy = (float)y;
	float x_a, x_b, z_a, z_b;
	float z_left, dz_dx;
	int   xl, xr, x;
	int16_t *zrow = zspantable[y];
	byte    *prow = (byte *)d_viewbuffer + d_scantable[y];

	/* Long edge X / Z at this scanline. */
	x_a = p0[0] + (fy - p0[1]) * dx_long;
	z_a = z0    + (fy - p0[1]) * dz_long;

	/* Short edge: top half uses (p0->p1), bottom half uses (p1->p2). */
	if (y < y_mid) {
	    x_b = p0[0] + (fy - p0[1]) * dx_short_top;
	    z_b = z0    + (fy - p0[1]) * dz_short_top;
	} else {
	    x_b = p1[0] + (fy - p1[1]) * dx_short_bot;
	    z_b = z1    + (fy - p1[1]) * dz_short_bot;
	}

	/* Order so xl <= xr; pick z_left and the dz/dx step. */
	if (x_a <= x_b) {
	    xl = (int)x_a; xr = (int)x_b;
	    z_left = z_a;
	    dz_dx  = (x_b - x_a > 1.0f) ? (z_b - z_a) / (x_b - x_a) : 0.0f;
	} else {
	    xl = (int)x_b; xr = (int)x_a;
	    z_left = z_b;
	    dz_dx  = (x_a - x_b > 1.0f) ? (z_a - z_b) / (x_a - x_b) : 0.0f;
	}

	if (xl < r_refdef.vrect.x) {
	    z_left += dz_dx * (float)(r_refdef.vrect.x - xl);
	    xl = r_refdef.vrect.x;
	}
	if (xr >= r_refdef.vrectright) xr = r_refdef.vrectright - 1;
	if (xl > xr)           continue;

	{
	    /* Colormap row 52 (of 64) for shadow darkness.  Floor
	     * texture darkened to ~20% brightness -- reads as deep
	     * shadow but the texture pattern is faintly visible,
	     * keeping the shadow from looking like a black hole.
	     * One row = 256 entries (one per palette index). */
	    const byte *cmap = (const byte *)vid.colormap + 52 * 256;
	    float z_pix = z_left;
	    for (x = xl; x <= xr; x++) {
		int16_t z_int = (int16_t)(z_pix * (float)0x8000);
		if (z_int >= zrow[x]) {
		    prow[x] = cmap[prow[x]];
		}
		z_pix += dz_dx;
	    }
	}
    }
}
