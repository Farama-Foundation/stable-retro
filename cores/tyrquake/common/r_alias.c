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
/* r_alias.c: routines for setting up to draw alias models */

#include "console.h"
#include "cvar.h"
#include "model.h"
#include "quakedef.h"
#include "r_local.h"
#include "r_subdiv.h"
#include "sys.h"
#include "world.h"   /* SV_RecursiveHullCheck for r_shadows ground trace */

/* FIXME: shouldn't be needed (is needed for patch right now, but that should
          move) */
#include "d_local.h"

/* SIMD batch path for R_AliasTransformAndProjectFinalVerts.  gcc -O3 will
 * autovec the scalar loop to 16-wide on AArch64 / SSE2, but emits ~1100
 * SSE2 / 525 AArch64 insns with heavy register spilling.  A manual 4-wide
 * SoA-across-vertices kernel (629 SSE2 / 308 NEON insns, no spills)
 * measured 1.45-1.50x faster than gcc -O3 autovec on x86_64 native.
 * Tyrquake builds at -O2 by default where gcc does NOT autovec this
 * function, so the win is real for production builds. */
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define ALIAS_SSE2 1
#include <emmintrin.h>
#endif
/* AArch64 only: ARMv7 NEON has 16 q-regs vs AArch64's 32 -- the 4-wide
 * shape would spill on ARMv7, and we have no real-hardware data to
 * justify enabling it.  ARMv7 falls back to scalar. */
#if defined(__aarch64__) || defined(_M_ARM64)
#define ALIAS_NEON_AARCH64 1
#include <arm_neon.h>
#endif

/* lowest light value we'll allow, to avoid the need for inner-loop light
   clamping */
#define LIGHT_MIN 5

affinetridesc_t r_affinetridesc;
trivertx_t *r_apverts;

void *acolormap;		/* FIXME: should go away */

/* TODO: these probably will go away with optimized rasterization */
vec3_t r_plightvec;
vec3_t r_phalfvec;     /* model-space halfway vector L+V for specular */
int r_ambientlight;
float r_shadelight;
static float ziscale;
static model_t *pmodel;

static vec3_t alias_forward, alias_right, alias_up;

int r_amodels_drawn;
int a_skinwidth;
int r_anumverts;

float aliastransform[3][4];

typedef struct {
    int index0;
    int index1;
} aedge_t;

/*
 * incomplete model interpolation support
 * -> default to off and don't save to config for now
 */
cvar_t r_lerpmodels = { "r_lerpmodels", "0", true };
cvar_t r_lerpmove = { "r_lerpmove", "0", true };

static aedge_t aedges[12] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 5}, {1, 4}, {2, 7}, {3, 6}
};

#define NUMVERTEXNORMALS	162

float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

static void R_AliasSetUpTransform(const entity_t *e, aliashdr_t *pahdr,
				  int trivial_accept);
static void R_AliasTransformVector(const vec3_t in, vec3_t out);
static void R_AliasTransformFinalVert(finalvert_t *fv, auxvert_t *av,
				      trivertx_t *pverts, stvert_t *pstverts);

void R_AliasTransformAndProjectFinalVerts(finalvert_t *fv, stvert_t *pstverts);
void R_AliasProjectFinalVert(finalvert_t *fv, auxvert_t *av);

/*
 * Model Loader Functions
 */
static int SW_Aliashdr_Padding(void) { return offsetof(sw_aliashdr_t, ahdr); }

static void *
SW_LoadSkinData(const char *modelname, aliashdr_t *ahdr, int skinnum,
		byte **skindata)
{
    int i;
    int skinsize = ahdr->skinwidth * ahdr->skinheight;
    byte *ret    = (byte*)Hunk_Alloc(skinnum * skinsize);
    byte *out    = ret;

    for (i = 0; i < skinnum; i++)
    {
	memcpy(out, skindata[i], skinsize);
	out += skinsize;
    }

    return ret;
}

static void
SW_LoadMeshData(const model_t *model, aliashdr_t *hdr, const mtriangle_t *tris,
		const stvert_t *stverts, const trivertx_t **verts)
{
    int i;
    trivertx_t *pverts;
    stvert_t *pstverts;
    mtriangle_t *ptris;

    /* Optional polygon subdivision (Catmull / Loop-style 1-to-4
     * split, see r_subdiv.c).  When r_polysubdiv > 0 we synthesize
     * a denser mesh and feed that to the rest of the loader; the
     * Hunk_Alloc-backed arrays still own the final data, so the
     * persisted model image is the subdivided one and no per-frame
     * cost is added.  On failure (allocation or runtime-cap
     * overshoot) we fall through with the unsubdivided source. */
    int sub_passes = R_PolySubdivPasses();
    int sub_nv = 0, sub_nt = 0, sub_applied = 0;
    stvert_t    *sub_stverts = NULL;
    mtriangle_t *sub_tris    = NULL;
    trivertx_t  *sub_poses   = NULL;
    qboolean subdivided = false;

    if (sub_passes > 0) {
	/* Phong shading at any non-zero level (Phong, Phong+specular)
	 * signals the user wants smoother-looking models; we mirror
	 * that intent at subdivision time by using Phong tessellation
	 * for the new vertex positions, which rounds convex silhouettes
	 * outward.  The r_phongshading cvar callback in r_main.c
	 * flushes the alias cache when this flag changes, so models
	 * pick up the new geometry on next reference.  Off here means
	 * plain midpoint subdivision (silhouette preserved bit-exact). */
	qboolean phong_tess = (r_phongshading.value != 0.0f);
	subdivided = R_SubdivideAliasMesh(sub_passes, phong_tess,
					  hdr->numposes,
					  hdr->skinwidth, hdr->scale,
					  hdr->numverts, hdr->numtris,
					  stverts, tris, verts,
					  &sub_nv, &sub_nt,
					  &sub_stverts, &sub_tris,
					  &sub_poses, &sub_applied);
	if (subdivided) {
	    /* Mesh size grew; keep the model's notion of numverts /
	     * numtris in sync so all downstream loops scale up too. */
	    hdr->numverts = sub_nv;
	    hdr->numtris  = sub_nt;
	    Con_DPrintf("%s: subdivided %s, %d passes -> %d verts, %d tris\n",
			__func__, model->name, sub_applied,
			sub_nv, sub_nt);
	} else if (sub_applied == 0) {
	    Con_DPrintf("%s: %s subdivision skipped (cap or alloc)\n",
			__func__, model->name);
	}
    }

    /*
     * Save the pose vertex data
     */
    pverts = (trivertx_t*)Hunk_Alloc(hdr->numposes * hdr->numverts * sizeof(*pverts));
    hdr->posedata = (byte *)pverts - (byte *)hdr;
    if (subdivided) {
	memcpy(pverts, sub_poses,
	       (size_t)hdr->numposes * (size_t)hdr->numverts * sizeof(*pverts));
    } else {
	for (i = 0; i < hdr->numposes; i++) {
	    memcpy(pverts, verts[i], hdr->numverts * sizeof(*pverts));
	    pverts += hdr->numverts;
	}
    }

    /*
     * Save the s/t verts
     * => put s and t in 16.16 format
     */
    pstverts = (stvert_t*)Hunk_Alloc(hdr->numverts * sizeof(*pstverts));
    SW_Aliashdr(hdr)->stverts = (byte *)pstverts - (byte *)hdr;
    {
	const stvert_t *src = subdivided ? sub_stverts : stverts;
	for (i = 0; i < hdr->numverts; i++) {
	    pstverts[i].onseam = src[i].onseam;
	    pstverts[i].s = src[i].s << 16;
	    pstverts[i].t = src[i].t << 16;
	}
    }

    /*
     * Save the triangle data
     */
    ptris = (mtriangle_t*)Hunk_Alloc(hdr->numtris * sizeof(*ptris));
    SW_Aliashdr(hdr)->triangles = (byte *)ptris - (byte *)hdr;
    if (subdivided)
	memcpy(ptris, sub_tris, hdr->numtris * sizeof(*ptris));
    else
	memcpy(ptris, tris, hdr->numtris * sizeof(*ptris));

    if (subdivided) {
	free(sub_stverts);
	free(sub_tris);
	free(sub_poses);
    }
}

static model_loader_t SW_Model_Loader = {
    SW_Aliashdr_Padding,
    SW_LoadSkinData,
    SW_LoadMeshData
};

const model_loader_t *
R_ModelLoader(void)
{
    return &SW_Model_Loader;
}

/*
================
R_AliasCheckBBox
================
*/
qboolean R_AliasCheckBBox(entity_t *e)
{
   int i, flags, frame, numv;
   aliashdr_t *pahdr;
   float zi, basepts[8][3], v0, v1;
   finalvert_t viewpts[16];
   auxvert_t viewaux[16];
   maliasframedesc_t *pframedesc;
   qboolean zclipped, zfullyclipped;
   unsigned anyclip, allclip;
   int minz;

   /* expand, rotate, and translate points into worldspace */

   e->trivial_accept = 0;
   pmodel = e->model;
   pahdr = (aliashdr_t*)Mod_Extradata(pmodel);

   R_AliasSetUpTransform(e, pahdr, 0);

   /* construct the base bounding box for this frame */
   frame = e->frame;
   /* TODO: don't repeat this check when drawing? */
   if ((frame >= pahdr->numframes) || (frame < 0)) {
      Con_DPrintf("No such frame %d %s\n", frame, pmodel->name);
      frame = 0;
   }

   pframedesc = &pahdr->frames[frame];

   /* x worldspace coordinates */
   basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] =
      (float)pframedesc->bboxmin.v[0];
   basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] =
      (float)pframedesc->bboxmax.v[0];

   /* y worldspace coordinates */
   basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] =
      (float)pframedesc->bboxmin.v[1];
   basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] =
      (float)pframedesc->bboxmax.v[1];

   /* z worldspace coordinates */
   basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] =
      (float)pframedesc->bboxmin.v[2];
   basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] =
      (float)pframedesc->bboxmax.v[2];

   zclipped = false;
   zfullyclipped = true;

   minz = 9999;
   for (i = 0; i < 8; i++)
   {
      R_AliasTransformVector(&basepts[i][0], &viewaux[i].fv[0]);

      if (viewaux[i].fv[2] < ALIAS_Z_CLIP_PLANE)
      {
         /* we must clip points that are closer than the near clip plane */
         viewpts[i].flags = ALIAS_Z_CLIP;
         zclipped = true;
      } else {
         if (viewaux[i].fv[2] < minz)
            minz = viewaux[i].fv[2];
         viewpts[i].flags = 0;
         zfullyclipped = false;
      }
   }


   if (zfullyclipped)
      return false;		/* everything was near-z-clipped */

   numv = 8;

   if (zclipped)
   {
      /* organize points by edges, use edges to get new points (possible trivial */
      /* reject) */
      for (i = 0; i < 12; i++)
      {
         /* edge endpoints */
         finalvert_t *pv0 = &viewpts[aedges[i].index0];
         finalvert_t *pv1 = &viewpts[aedges[i].index1];
         auxvert_t *pa0 = &viewaux[aedges[i].index0];
         auxvert_t *pa1 = &viewaux[aedges[i].index1];

         /* if one end is clipped and the other isn't, make a new point */
         if (pv0->flags ^ pv1->flags)
         {
            float frac = (ALIAS_Z_CLIP_PLANE - pa0->fv[2]) /
               (pa1->fv[2] - pa0->fv[2]);
            viewaux[numv].fv[0] = pa0->fv[0] +
               (pa1->fv[0] - pa0->fv[0]) * frac;
            viewaux[numv].fv[1] = pa0->fv[1] +
               (pa1->fv[1] - pa0->fv[1]) * frac;
            viewaux[numv].fv[2] = ALIAS_Z_CLIP_PLANE;
            viewpts[numv].flags = 0;
            numv++;
         }
      }
   }
   /* project the vertices that remain after clipping */
   anyclip = 0;
   allclip = ALIAS_XY_CLIP_MASK;

   /* TODO: probably should do this loop in ASM, especially if we use floats */
   for (i = 0; i < numv; i++) {
      /* we don't need to bother with vertices that were z-clipped */
      if (viewpts[i].flags & ALIAS_Z_CLIP)
         continue;

      zi = 1.0 / viewaux[i].fv[2];

      /* FIXME: do with chop mode in ASM, or convert to float */
      v0 = (viewaux[i].fv[0] * xscale * zi) + xcenter;
      v1 = (viewaux[i].fv[1] * yscale * zi) + ycenter;

      flags = 0;

      if (v0 < r_refdef.fvrectx)
         flags |= ALIAS_LEFT_CLIP;
      if (v1 < r_refdef.fvrecty)
         flags |= ALIAS_TOP_CLIP;
      if (v0 > r_refdef.fvrectright)
         flags |= ALIAS_RIGHT_CLIP;
      if (v1 > r_refdef.fvrectbottom)
         flags |= ALIAS_BOTTOM_CLIP;

      anyclip |= flags;
      allclip &= flags;
   }

   if (allclip)
      return false;		/* trivial reject off one side */

   /*
    * FIXME - Trivial accept not safe while lerping unless we check
    *         the bbox of both src and dst frames
    */
   if (r_lerpmodels.value)
      return true;

   e->trivial_accept = !anyclip & !zclipped;
   if (e->trivial_accept) {
      if (minz > (r_aliastransition + (pahdr->size * r_resfudge))) {
         e->trivial_accept |= 2;
      }
   }

   return true;
}


/*
================
R_AliasTransformVector
================
*/
static void
R_AliasTransformVector(const vec3_t in, vec3_t out)
{
    out[0] = DotProduct(in, aliastransform[0]) + aliastransform[0][3];
    out[1] = DotProduct(in, aliastransform[1]) + aliastransform[1][3];
    out[2] = DotProduct(in, aliastransform[2]) + aliastransform[2][3];
}


/*
================
R_AliasPreparePoints

General clipped case
================
*/
static void
R_AliasPreparePoints(aliashdr_t *pahdr, finalvert_t *pfinalverts,
		     auxvert_t *pauxverts)
{
    int i;
    stvert_t *pstverts;
    finalvert_t *fv;
    auxvert_t *av;
    mtriangle_t *ptri;
    finalvert_t *pfv[3];

    pstverts = (stvert_t *)((byte *)pahdr + SW_Aliashdr(pahdr)->stverts);
    r_anumverts = pahdr->numverts;
    fv = pfinalverts;
    av = pauxverts;

    for (i = 0; i < r_anumverts; i++, fv++, av++, r_apverts++, pstverts++) {
	R_AliasTransformFinalVert(fv, av, r_apverts, pstverts);
	if (av->fv[2] < ALIAS_Z_CLIP_PLANE)
	    fv->flags |= ALIAS_Z_CLIP;
	else {
	    R_AliasProjectFinalVert(fv, av);
	    if (fv->v[0] < r_refdef.aliasvrect.x)
		fv->flags |= ALIAS_LEFT_CLIP;
	    if (fv->v[1] < r_refdef.aliasvrect.y)
		fv->flags |= ALIAS_TOP_CLIP;
	    if (fv->v[0] > r_refdef.aliasvrectright)
		fv->flags |= ALIAS_RIGHT_CLIP;
	    if (fv->v[1] > r_refdef.aliasvrectbottom)
		fv->flags |= ALIAS_BOTTOM_CLIP;
	}
    }

/**/
/* clip and draw all triangles */
/**/
    r_affinetridesc.numtriangles = 1;

    /* Two passes over the entity's triangles.  The first
     * pass partitions: fully-clipped triangles are
     * skipped, partially-clipped triangles dispatch
     * immediately via R_AliasClipTriangle (each clip-and-
     * fan produces its own local pfinalverts and can't be
     * batched against the entity's pfinalverts), and
     * totally-unclipped triangles are appended to a per-
     * frame scratch list.  The second pass dispatches all
     * unclipped triangles in one D_PolysetDraw call.
     *
     * The old code issued one D_PolysetDraw per triangle
     * unconditionally, which is a wash in pure SW (the
     * span generator's per-call overhead is small) but
     * destroys the GPU compute backend: one Vulkan
     * dispatch per triangle on a partially-clipped entity
     * runs the per-pixel barycentric over a 1-triangle
     * loop, hundreds of times per entity, and the
     * viewmodel (always close enough to camera to
     * straddle ALIAS_Z_CLIP_PLANE) hits this every frame.
     * Batching collapses those hundreds of dispatches to
     * one per entity for the unclipped majority of the
     * mesh; the partially-clipped triangles -- typically
     * a handful per entity -- still dispatch individually
     * since their post-clip fan vertices live on
     * R_AliasClipTriangle's stack.
     *
     * The static scratch is sized to
     * MAXALIASTRIS_RUNTIME to cover entities that have
     * been grown by r_polysubdiv at load time. */
    {
        static mtriangle_t batched[MAXALIASTRIS_RUNTIME];
        int batched_count = 0;

        ptri = (mtriangle_t *)((byte *)pahdr + SW_Aliashdr(pahdr)->triangles);
        for (i = 0; i < pahdr->numtris; i++, ptri++) {
            pfv[0] = &pfinalverts[ptri->vertindex[0]];
            pfv[1] = &pfinalverts[ptri->vertindex[1]];
            pfv[2] = &pfinalverts[ptri->vertindex[2]];

            if (pfv[0]->flags & pfv[1]->flags & pfv[2]->flags
                & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP))
                continue;		/* completely clipped */

            if (!((pfv[0]->flags | pfv[1]->flags | pfv[2]->flags)
                  & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP))) {
                /* totally unclipped: append to batch */
                batched[batched_count++] = *ptri;
            } else {		/* partially clipped */
                R_AliasClipTriangle(ptri, pfinalverts, pauxverts);
            }
        }

        if (batched_count > 0) {
            r_affinetridesc.numtriangles = batched_count;
            r_affinetridesc.pfinalverts  = pfinalverts;
            r_affinetridesc.ptriangles   = batched;
            D_PolysetDraw();
        }
    }
}


/*
================
R_AliasSetUpTransform
================
*/
static void
R_AliasSetUpTransform(const entity_t *e, aliashdr_t *pahdr, int trivial_accept)
{
    int i;
    float rotationmatrix[3][4], t2matrix[3][4];
    static float tmatrix[3][4];
    static float viewmatrix[3][4];
    vec3_t angles;

/* TODO: should really be stored with the entity instead of being reconstructed */
/* TODO: should use a look-up table */
/* TODO: could cache lazily, stored in the entity */

    if (r_lerpmove.value && e->previousanglestime > 0.0f) {
	/* Countdown semantics: previousanglestime is the duration of
	 * the prior snapshot interval (zero = no history, skip lerp),
	 * currentanglestime is the elapsed time since the latest. */
	float delta = e->previousanglestime;
	float frac = qclamp(e->currentanglestime / delta, 0.0, 1.0);
	vec3_t lerpvec;

	/* FIXME - hack to skip the viewent (weapon) */
	if (e == &cl.viewent)
	    goto nolerp;

	VectorSubtract(e->currentangles, e->previousangles, lerpvec);
	for (i = 0; i < 3; i++) {
	    if (lerpvec[i] > 180.0f)
		lerpvec[i] -= 360.0f;
	    else if (lerpvec[i] < -180.0f)
		lerpvec[i] += 360.0f;
	}
	VectorMA(e->previousangles, frac, lerpvec, angles);
	angles[PITCH] = -angles[PITCH];
    } else
    nolerp:
    {
	angles[ROLL] = e->angles[ROLL];
	angles[PITCH] = -e->angles[PITCH];
	angles[YAW] = e->angles[YAW];
    }
    AngleVectors(angles, alias_forward, alias_right, alias_up);

    tmatrix[0][0] = pahdr->scale[0];
    tmatrix[1][1] = pahdr->scale[1];
    tmatrix[2][2] = pahdr->scale[2];

    tmatrix[0][3] = pahdr->scale_origin[0];
    tmatrix[1][3] = pahdr->scale_origin[1];
    tmatrix[2][3] = pahdr->scale_origin[2];

/* TODO: can do this with simple matrix rearrangement */

    for (i = 0; i < 3; i++) {
	t2matrix[i][0] = alias_forward[i];
	t2matrix[i][1] = -alias_right[i];
	t2matrix[i][2] = alias_up[i];
    }

    t2matrix[0][3] = -modelorg[0];
    t2matrix[1][3] = -modelorg[1];
    t2matrix[2][3] = -modelorg[2];

/* FIXME: can do more efficiently than full concatenation */
    R_ConcatTransforms(t2matrix, tmatrix, rotationmatrix);

/* TODO: should be global, set when vright, etc., set */
    VectorCopy(vright, viewmatrix[0]);
    VectorCopy(vup, viewmatrix[1]);
    VectorInverse(viewmatrix[1]);
    VectorCopy(vpn, viewmatrix[2]);

/*      viewmatrix[0][3] = 0; */
/*      viewmatrix[1][3] = 0; */
/*      viewmatrix[2][3] = 0; */

    R_ConcatTransforms(viewmatrix, rotationmatrix, aliastransform);

/* do the scaling up of x and y to screen coordinates as part of the transform */
/* for the unclipped case (it would mess up clipping in the clipped case). */
/* Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y */
/* correspondingly so the projected x and y come out right */
/* FIXME: make this work for clipped case too? */
    if (trivial_accept) {
	for (i = 0; i < 4; i++) {
	    aliastransform[0][i] *= aliasxscale *
		(1.0 / ((float)0x8000 * 0x10000));
	    aliastransform[1][i] *= aliasyscale *
		(1.0 / ((float)0x8000 * 0x10000));
	    aliastransform[2][i] *= 1.0 / ((float)0x8000 * 0x10000);

	}
    }
}


/*
================
R_AliasTransformFinalVert
================
*/
static void
R_AliasTransformFinalVert(finalvert_t *fv, auxvert_t *av,
			  trivertx_t *pverts, stvert_t *pstverts)
{
    int temp;
    float lightcos, *plightnormal;

    av->fv[0] = DotProduct(pverts->v, aliastransform[0]) +
	aliastransform[0][3];
    av->fv[1] = DotProduct(pverts->v, aliastransform[1]) +
	aliastransform[1][3];
    av->fv[2] = DotProduct(pverts->v, aliastransform[2]) +
	aliastransform[2][3];

    fv->v[2] = pstverts->s;
    fv->v[3] = pstverts->t;

    fv->flags = pstverts->onseam;

/* lighting */
    plightnormal = r_avertexnormals[pverts->lightnormalindex];
    lightcos = DotProduct(plightnormal, r_plightvec);
    temp = r_ambientlight;

    if (lightcos < 0) {
	temp += (int)(r_shadelight * lightcos);

	/* clamp; because we limited the minimum ambient and shading light, we */
	/* don't have to clamp low light, just bright */
	if (temp < 0)
	    temp = 0;
    }

    fv->v[4] = temp;

    /* Copy the vertex normal too, for the Phong-shaded path in d_polyse.c.
     * Normal is in model space; r_plightvec is also in model space (rotated
     * in R_AliasSetupLighting), so per-pixel dot(N, L) works without any
     * extra coordinate transform. */
    fv->n[0] = plightnormal[0];
    fv->n[1] = plightnormal[1];
    fv->n[2] = plightnormal[2];
}

/*
================
R_AliasTransformAndProjectFinalVerts
================
*/
#if defined(ALIAS_SSE2)
static void
R_AliasTransformAndProject4_SSE2(finalvert_t *fv, stvert_t *pstverts,
                                  trivertx_t *pverts)
{
   /* SoA across 4 vertices.  Lanes 0..3 = verts 0..3.  Loads 16 bytes
    * (4 trivertx_t), transposes byte fields to per-component vectors,
    * runs the transform/projection/lighting in parallel, then scatters
    * results to AoS finalvert_t. */
   __m128i raw      = _mm_loadu_si128((const __m128i *)pverts);
   __m128i zero_iv  = _mm_setzero_si128();
   __m128i lo       = _mm_unpacklo_epi8(raw, zero_iv);
   __m128i hi       = _mm_unpackhi_epi8(raw, zero_iv);
   __m128  a        = _mm_castsi128_ps(_mm_unpacklo_epi16(lo, zero_iv));
   __m128  b        = _mm_castsi128_ps(_mm_unpackhi_epi16(lo, zero_iv));
   __m128  c        = _mm_castsi128_ps(_mm_unpacklo_epi16(hi, zero_iv));
   __m128  d        = _mm_castsi128_ps(_mm_unpackhi_epi16(hi, zero_iv));
   /* 4x4 transpose via shuffles -- expand _MM_TRANSPOSE4_PS inline so
    * everything stays initializer-form (C89 needs decls before stmts). */
   __m128  t0       = _mm_shuffle_ps(a, b, 0x44);
   __m128  t1       = _mm_shuffle_ps(c, d, 0x44);
   __m128  t2       = _mm_shuffle_ps(a, b, 0xEE);
   __m128  t3       = _mm_shuffle_ps(c, d, 0xEE);
   __m128  vx_ps    = _mm_shuffle_ps(t0, t1, 0x88);   /* (v0[0],v1[0],v2[0],v3[0]) */
   __m128  vy_ps    = _mm_shuffle_ps(t0, t1, 0xDD);   /* (v0[1],v1[1],v2[1],v3[1]) */
   __m128  vz_ps    = _mm_shuffle_ps(t2, t3, 0x88);   /* (v0[2],v1[2],v2[2],v3[2]) */
   __m128i lni_iv   = _mm_castps_si128(_mm_shuffle_ps(t2, t3, 0xDD));
   __m128  vxf      = _mm_cvtepi32_ps(_mm_castps_si128(vx_ps));
   __m128  vyf      = _mm_cvtepi32_ps(_mm_castps_si128(vy_ps));
   __m128  vzf      = _mm_cvtepi32_ps(_mm_castps_si128(vz_ps));
   /* Transform: 3 dot products in parallel across 4 verts. */
   __m128  dz_v     = _mm_add_ps(_mm_set1_ps(aliastransform[2][3]),
                      _mm_add_ps(_mm_mul_ps(vxf, _mm_set1_ps(aliastransform[2][0])),
                      _mm_add_ps(_mm_mul_ps(vyf, _mm_set1_ps(aliastransform[2][1])),
                                 _mm_mul_ps(vzf, _mm_set1_ps(aliastransform[2][2])))));
   __m128  dx_v     = _mm_add_ps(_mm_set1_ps(aliastransform[0][3]),
                      _mm_add_ps(_mm_mul_ps(vxf, _mm_set1_ps(aliastransform[0][0])),
                      _mm_add_ps(_mm_mul_ps(vyf, _mm_set1_ps(aliastransform[0][1])),
                                 _mm_mul_ps(vzf, _mm_set1_ps(aliastransform[0][2])))));
   __m128  dy_v     = _mm_add_ps(_mm_set1_ps(aliastransform[1][3]),
                      _mm_add_ps(_mm_mul_ps(vxf, _mm_set1_ps(aliastransform[1][0])),
                      _mm_add_ps(_mm_mul_ps(vyf, _mm_set1_ps(aliastransform[1][1])),
                                 _mm_mul_ps(vzf, _mm_set1_ps(aliastransform[1][2])))));
   __m128  zi_v     = _mm_div_ps(_mm_set1_ps(1.0f), dz_v);
   __m128i ix       = _mm_cvttps_epi32(_mm_add_ps(_mm_mul_ps(dx_v, zi_v),
                                          _mm_set1_ps(aliasxcenter)));
   __m128i iy       = _mm_cvttps_epi32(_mm_add_ps(_mm_mul_ps(dy_v, zi_v),
                                          _mm_set1_ps(aliasycenter)));
   __m128i izi      = _mm_cvttps_epi32(zi_v);
   /* Gather lightnormals.  Extract 4 indexes; SSE2 has no native gather. */
   int     lni0     = _mm_cvtsi128_si32(lni_iv);
   int     lni1     = _mm_cvtsi128_si32(_mm_shuffle_epi32(lni_iv, _MM_SHUFFLE(0,0,0,1)));
   int     lni2     = _mm_cvtsi128_si32(_mm_shuffle_epi32(lni_iv, _MM_SHUFFLE(0,0,0,2)));
   int     lni3     = _mm_cvtsi128_si32(_mm_shuffle_epi32(lni_iv, _MM_SHUFFLE(0,0,0,3)));
   const float *n0  = r_avertexnormals[lni0 & 0xff];
   const float *n1  = r_avertexnormals[lni1 & 0xff];
   const float *n2  = r_avertexnormals[lni2 & 0xff];
   const float *n3  = r_avertexnormals[lni3 & 0xff];
   __m128  nx_v     = _mm_setr_ps(n0[0], n1[0], n2[0], n3[0]);
   __m128  ny_v     = _mm_setr_ps(n0[1], n1[1], n2[1], n3[1]);
   __m128  nz_v     = _mm_setr_ps(n0[2], n1[2], n2[2], n3[2]);
   __m128  lc_v     = _mm_add_ps(_mm_mul_ps(nx_v, _mm_set1_ps(r_plightvec[0])),
                      _mm_add_ps(_mm_mul_ps(ny_v, _mm_set1_ps(r_plightvec[1])),
                                 _mm_mul_ps(nz_v, _mm_set1_ps(r_plightvec[2]))));
   /* Masked: temp = lightcos<0 ? max(ambient + (int)(shade*lightcos), 0) : ambient */
   __m128i neg_mask = _mm_castps_si128(_mm_cmplt_ps(lc_v, _mm_setzero_ps()));
   __m128i shade_i  = _mm_and_si128(neg_mask,
                                    _mm_cvttps_epi32(_mm_mul_ps(lc_v, _mm_set1_ps(r_shadelight))));
   __m128i temp_v   = _mm_add_epi32(_mm_set1_epi32(r_ambientlight), shade_i);
   __m128i clamp_m  = _mm_and_si128(_mm_cmpgt_epi32(_mm_setzero_si128(), temp_v), neg_mask);
   __m128i light_v  = _mm_andnot_si128(clamp_m, temp_v);
   /* Begin statement section: scatter SoA results into AoS finalverts. */
   int tmp_ix[4], tmp_iy[4], tmp_izi[4], tmp_light[4];
   _mm_storeu_si128((__m128i *)tmp_ix,    ix);
   _mm_storeu_si128((__m128i *)tmp_iy,    iy);
   _mm_storeu_si128((__m128i *)tmp_izi,   izi);
   _mm_storeu_si128((__m128i *)tmp_light, light_v);
   fv[0].v[0] = tmp_ix[0];   fv[1].v[0] = tmp_ix[1];
   fv[2].v[0] = tmp_ix[2];   fv[3].v[0] = tmp_ix[3];
   fv[0].v[1] = tmp_iy[0];   fv[1].v[1] = tmp_iy[1];
   fv[2].v[1] = tmp_iy[2];   fv[3].v[1] = tmp_iy[3];
   fv[0].v[2] = pstverts[0].s; fv[1].v[2] = pstverts[1].s;
   fv[2].v[2] = pstverts[2].s; fv[3].v[2] = pstverts[3].s;
   fv[0].v[3] = pstverts[0].t; fv[1].v[3] = pstverts[1].t;
   fv[2].v[3] = pstverts[2].t; fv[3].v[3] = pstverts[3].t;
   fv[0].v[4] = tmp_light[0]; fv[1].v[4] = tmp_light[1];
   fv[2].v[4] = tmp_light[2]; fv[3].v[4] = tmp_light[3];
   fv[0].v[5] = tmp_izi[0];  fv[1].v[5] = tmp_izi[1];
   fv[2].v[5] = tmp_izi[2];  fv[3].v[5] = tmp_izi[3];
   fv[0].flags = pstverts[0].onseam; fv[1].flags = pstverts[1].onseam;
   fv[2].flags = pstverts[2].onseam; fv[3].flags = pstverts[3].onseam;
   fv[0].n[0] = n0[0]; fv[0].n[1] = n0[1]; fv[0].n[2] = n0[2];
   fv[1].n[0] = n1[0]; fv[1].n[1] = n1[1]; fv[1].n[2] = n1[2];
   fv[2].n[0] = n2[0]; fv[2].n[1] = n2[1]; fv[2].n[2] = n2[2];
   fv[3].n[0] = n3[0]; fv[3].n[1] = n3[1]; fv[3].n[2] = n3[2];
}
#endif

#if defined(ALIAS_NEON_AARCH64)
static void
R_AliasTransformAndProject4_NEON(finalvert_t *fv, stvert_t *pstverts,
                                  trivertx_t *pverts)
{
   /* Mirror of the SSE2 path; uses AArch64 vqtbl1q for byte deinterleave
    * (one insn does what SSE2 needs 4 unpcks + 4 shuffles to achieve). */
   uint8x16_t raw   = vld1q_u8((const uint8_t *)pverts);
   /* lane permutation: vx at byte offsets 0,4,8,12; vy at 1,5,9,13; etc. */
   static const uint8_t tbl_vx_b[16]  = {0,4,8,12,  0xff,0xff,0xff,0xff,
                                          0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};
   static const uint8_t tbl_vy_b[16]  = {1,5,9,13,  0xff,0xff,0xff,0xff,
                                          0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};
   static const uint8_t tbl_vz_b[16]  = {2,6,10,14, 0xff,0xff,0xff,0xff,
                                          0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};
   static const uint8_t tbl_lni_b[16] = {3,7,11,15, 0xff,0xff,0xff,0xff,
                                          0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};
   uint8x16_t vx_b   = vqtbl1q_u8(raw, vld1q_u8(tbl_vx_b));
   uint8x16_t vy_b   = vqtbl1q_u8(raw, vld1q_u8(tbl_vy_b));
   uint8x16_t vz_b   = vqtbl1q_u8(raw, vld1q_u8(tbl_vz_b));
   uint8x16_t lni_b  = vqtbl1q_u8(raw, vld1q_u8(tbl_lni_b));
   float32x4_t vxf   = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vget_low_u8(vx_b)))));
   float32x4_t vyf   = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vget_low_u8(vy_b)))));
   float32x4_t vzf   = vcvtq_f32_u32(vmovl_u16(vget_low_u16(vmovl_u8(vget_low_u8(vz_b)))));
   uint32x4_t  lni_v = vmovl_u16(vget_low_u16(vmovl_u8(vget_low_u8(lni_b))));
   float32x4_t dz_v  = vmlaq_n_f32(vmlaq_n_f32(vmlaq_n_f32(
                          vdupq_n_f32(aliastransform[2][3]),
                          vxf, aliastransform[2][0]),
                          vyf, aliastransform[2][1]),
                          vzf, aliastransform[2][2]);
   float32x4_t dx_v  = vmlaq_n_f32(vmlaq_n_f32(vmlaq_n_f32(
                          vdupq_n_f32(aliastransform[0][3]),
                          vxf, aliastransform[0][0]),
                          vyf, aliastransform[0][1]),
                          vzf, aliastransform[0][2]);
   float32x4_t dy_v  = vmlaq_n_f32(vmlaq_n_f32(vmlaq_n_f32(
                          vdupq_n_f32(aliastransform[1][3]),
                          vxf, aliastransform[1][0]),
                          vyf, aliastransform[1][1]),
                          vzf, aliastransform[1][2]);
   float32x4_t zi_v  = vdivq_f32(vdupq_n_f32(1.0f), dz_v);
   int32x4_t   ix    = vcvtq_s32_f32(vmlaq_f32(vdupq_n_f32(aliasxcenter), dx_v, zi_v));
   int32x4_t   iy    = vcvtq_s32_f32(vmlaq_f32(vdupq_n_f32(aliasycenter), dy_v, zi_v));
   int32x4_t   izi   = vcvtq_s32_f32(zi_v);
   uint32_t    lni0  = vgetq_lane_u32(lni_v, 0);
   uint32_t    lni1  = vgetq_lane_u32(lni_v, 1);
   uint32_t    lni2  = vgetq_lane_u32(lni_v, 2);
   uint32_t    lni3  = vgetq_lane_u32(lni_v, 3);
   const float *n0   = r_avertexnormals[lni0 & 0xff];
   const float *n1   = r_avertexnormals[lni1 & 0xff];
   const float *n2   = r_avertexnormals[lni2 & 0xff];
   const float *n3   = r_avertexnormals[lni3 & 0xff];
   float       nx_a[4] = { n0[0], n1[0], n2[0], n3[0] };
   float       ny_a[4] = { n0[1], n1[1], n2[1], n3[1] };
   float       nz_a[4] = { n0[2], n1[2], n2[2], n3[2] };
   float32x4_t nx_v  = vld1q_f32(nx_a);
   float32x4_t ny_v  = vld1q_f32(ny_a);
   float32x4_t nz_v  = vld1q_f32(nz_a);
   float32x4_t lc_v  = vmlaq_n_f32(vmlaq_n_f32(
                          vmulq_n_f32(nx_v, r_plightvec[0]),
                          ny_v, r_plightvec[1]),
                          nz_v, r_plightvec[2]);
   uint32x4_t  neg_m = vcltq_f32(lc_v, vdupq_n_f32(0.0f));
   int32x4_t   shade = vandq_s32(vreinterpretq_s32_u32(neg_m),
                                 vcvtq_s32_f32(vmulq_n_f32(lc_v, r_shadelight)));
   int32x4_t   temp_v = vaddq_s32(vdupq_n_s32(r_ambientlight), shade);
   uint32x4_t  cl_m   = vandq_u32(vcltq_s32(temp_v, vdupq_n_s32(0)), neg_m);
   int32x4_t   light  = vbicq_s32(temp_v, vreinterpretq_s32_u32(cl_m));
   fv[0].v[0] = vgetq_lane_s32(ix, 0); fv[1].v[0] = vgetq_lane_s32(ix, 1);
   fv[2].v[0] = vgetq_lane_s32(ix, 2); fv[3].v[0] = vgetq_lane_s32(ix, 3);
   fv[0].v[1] = vgetq_lane_s32(iy, 0); fv[1].v[1] = vgetq_lane_s32(iy, 1);
   fv[2].v[1] = vgetq_lane_s32(iy, 2); fv[3].v[1] = vgetq_lane_s32(iy, 3);
   fv[0].v[2] = pstverts[0].s; fv[1].v[2] = pstverts[1].s;
   fv[2].v[2] = pstverts[2].s; fv[3].v[2] = pstverts[3].s;
   fv[0].v[3] = pstverts[0].t; fv[1].v[3] = pstverts[1].t;
   fv[2].v[3] = pstverts[2].t; fv[3].v[3] = pstverts[3].t;
   fv[0].v[4] = vgetq_lane_s32(light, 0); fv[1].v[4] = vgetq_lane_s32(light, 1);
   fv[2].v[4] = vgetq_lane_s32(light, 2); fv[3].v[4] = vgetq_lane_s32(light, 3);
   fv[0].v[5] = vgetq_lane_s32(izi, 0); fv[1].v[5] = vgetq_lane_s32(izi, 1);
   fv[2].v[5] = vgetq_lane_s32(izi, 2); fv[3].v[5] = vgetq_lane_s32(izi, 3);
   fv[0].flags = pstverts[0].onseam; fv[1].flags = pstverts[1].onseam;
   fv[2].flags = pstverts[2].onseam; fv[3].flags = pstverts[3].onseam;
   fv[0].n[0] = n0[0]; fv[0].n[1] = n0[1]; fv[0].n[2] = n0[2];
   fv[1].n[0] = n1[0]; fv[1].n[1] = n1[1]; fv[1].n[2] = n1[2];
   fv[2].n[0] = n2[0]; fv[2].n[1] = n2[1]; fv[2].n[2] = n2[2];
   fv[3].n[0] = n3[0]; fv[3].n[1] = n3[1]; fv[3].n[2] = n3[2];
}
#endif

void
R_AliasTransformAndProjectFinalVerts(finalvert_t *fv, stvert_t *pstverts)
{
   int i = 0;
   trivertx_t *pverts = r_apverts;

#if defined(ALIAS_SSE2) || defined(ALIAS_NEON_AARCH64)
   {
      int batch = r_anumverts & ~3;
      for (; i < batch; i += 4) {
#if defined(ALIAS_SSE2)
         R_AliasTransformAndProject4_SSE2(fv + i, pstverts + i, pverts + i);
#else
         R_AliasTransformAndProject4_NEON(fv + i, pstverts + i, pverts + i);
#endif
      }
      fv       += batch;
      pverts   += batch;
      pstverts += batch;
   }
#endif

   for (; i < r_anumverts; i++, fv++, pverts++, pstverts++)
   {
      int temp;
      float lightcos, *plightnormal;
      /* transform and project */
      float zi = 1.0 / (DotProduct(pverts->v, aliastransform[2]) +
            aliastransform[2][3]);

      /* x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is */
      /* scaled up by 1/2**31, and the scaling cancels out for x and y in the */
      /* projection */
      fv->v[5] = zi;

      fv->v[0] = ((DotProduct(pverts->v, aliastransform[0]) +
               aliastransform[0][3]) * zi) + aliasxcenter;
      fv->v[1] = ((DotProduct(pverts->v, aliastransform[1]) +
               aliastransform[1][3]) * zi) + aliasycenter;

      fv->v[2] = pstverts->s;
      fv->v[3] = pstverts->t;
      fv->flags = pstverts->onseam;

      /* lighting */
      plightnormal = r_avertexnormals[pverts->lightnormalindex];
      lightcos = DotProduct(plightnormal, r_plightvec);
      temp = r_ambientlight;

      if (lightcos < 0)
      {
         temp += (int)(r_shadelight * lightcos);

         /* clamp; because we limited the minimum ambient and shading light, we */
         /* don't have to clamp low light, just bright */
         if (temp < 0)
            temp = 0;
      }

      fv->v[4] = temp;

      /* Copy the vertex normal too, for the Phong-shaded path in d_polyse.c.
       * See companion note in R_AliasTransformFinalVert above. */
      fv->n[0] = plightnormal[0];
      fv->n[1] = plightnormal[1];
      fv->n[2] = plightnormal[2];
   }
}

/*
================
R_AliasProjectFinalVert
================
*/
void
R_AliasProjectFinalVert(finalvert_t *fv, auxvert_t *av)
{
    float zi;

/* project points */
    zi = 1.0 / av->fv[2];

    fv->v[5] = zi * ziscale;

    fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
    fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}


/*
================
R_AliasPrepareUnclippedPoints
================
*/
static void
R_AliasPrepareUnclippedPoints(aliashdr_t *pahdr, finalvert_t *pfinalverts)
{
    stvert_t *pstverts;

    pstverts = (stvert_t *)((byte *)pahdr + SW_Aliashdr(pahdr)->stverts);
    r_anumverts = pahdr->numverts;

    R_AliasTransformAndProjectFinalVerts(pfinalverts, pstverts);

    if (r_affinetridesc.drawtype)
	D_PolysetDrawFinalVerts(pfinalverts, r_anumverts);

    r_affinetridesc.pfinalverts = pfinalverts;
    r_affinetridesc.ptriangles = (mtriangle_t *)((byte *)pahdr +
						 SW_Aliashdr(pahdr)->triangles);
    r_affinetridesc.numtriangles = pahdr->numtris;

    D_PolysetDraw();
}

/*
===============
R_AliasSetupSkin
===============
*/
static void R_AliasSetupSkin(const entity_t *e, aliashdr_t *pahdr)
{
   int frame;
   int numframes, skinbytes;
   maliasskindesc_t *pskindesc;
   byte *pdata;
   int skinnum = e->skinnum;

   if ((skinnum >= pahdr->numskins) || (skinnum < 0))
   {
      Con_DPrintf("%s: no such skin # %d\n", __func__, skinnum);
      skinnum = 0;
   }

   pskindesc = ((maliasskindesc_t *)((byte *)pahdr + pahdr->skindesc));
   pskindesc += skinnum;
   a_skinwidth = pahdr->skinwidth;

   frame = pskindesc->firstframe;
   numframes = pskindesc->numframes;

   if (numframes > 1)
   {
      float *intervals = (float *)((byte *)pahdr + pahdr->skinintervals) + frame;
      frame += Mod_FindInterval(intervals, numframes, cl.time + e->syncbase);
   }

   skinbytes = pahdr->skinwidth * pahdr->skinheight;
   pdata = (byte *)pahdr + pahdr->skindata;
   pdata += frame * skinbytes;

   r_affinetridesc.pskin = pdata;
   r_affinetridesc.skinwidth = a_skinwidth;
   r_affinetridesc.seamfixupX16 = (a_skinwidth >> 1) << 16;
   r_affinetridesc.skinheight = pahdr->skinheight;

}

/*
================
R_AliasSetupLighting
================
*/
static void
R_AliasSetupLighting(alight_t *plighting, const vec3_t entity_origin)
{
    vec3_t  v_world, h_world;
    vec_t   h_len;

/* guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have */
/* to clamp off the bottom */
    r_ambientlight = plighting->ambientlight;

    if (r_ambientlight < LIGHT_MIN)
	r_ambientlight = LIGHT_MIN;

    r_ambientlight = (255 - r_ambientlight) << VID_CBITS;

    if (r_ambientlight < LIGHT_MIN)
	r_ambientlight = LIGHT_MIN;

    r_shadelight = plighting->shadelight;

    if (r_shadelight < 0)
	r_shadelight = 0;

    r_shadelight *= VID_GRADES;

/* rotate the lighting vector into the model's frame of reference */
    r_plightvec[0] = DotProduct(plighting->plightvec, alias_forward);
    r_plightvec[1] = -DotProduct(plighting->plightvec, alias_right);
    r_plightvec[2] = DotProduct(plighting->plightvec, alias_up);

    /* Compute halfway vector H = normalize(L + V) in model space, used
     * by the Blinn-Phong specular term in the rasterizer.  The light
     * direction L is plightvec (already in world space, pointing away
     * from the surface toward the light); V is approximated as the
     * unit vector from the entity's origin to the camera (constant
     * across the model, same approximation already used for L).
     * After summing L + V in world space we rotate into model space
     * via the same alias_forward/right/up basis used for plightvec.
     *
     * Skip normalization if H length is zero (the light and view are
     * exactly opposite, an edge case).  In that case set H to L; the
     * specular term will be near-zero everywhere it matters. */
    VectorSubtract(r_origin, entity_origin, v_world);
    VectorNormalize(v_world);
    VectorAdd(plighting->plightvec, v_world, h_world);
    h_len = sqrt(h_world[0]*h_world[0] + h_world[1]*h_world[1]
              + h_world[2]*h_world[2]);
    if (h_len > 0.0001f) {
	h_world[0] /= h_len;
	h_world[1] /= h_len;
	h_world[2] /= h_len;
    } else {
	VectorCopy(plighting->plightvec, h_world);
    }
    r_phalfvec[0] =  DotProduct(h_world, alias_forward);
    r_phalfvec[1] = -DotProduct(h_world, alias_right);
    r_phalfvec[2] =  DotProduct(h_world, alias_up);
}

static trivertx_t *
R_AliasBlendPoseVerts(const entity_t *e, aliashdr_t *hdr, float blend)
{
    /* Sized to MAXALIASVERTS_RUNTIME to cover meshes that have been
     * grown by r_polysubdiv at load time.  At the default (no
     * subdivision) only the first numverts entries are touched. */
    static trivertx_t blendverts[MAXALIASVERTS_RUNTIME];
    trivertx_t *poseverts, *pv1, *pv2, *light;
    int i, blend0, blend1;

#define SHIFT 22
    blend1 = blend * (1 << SHIFT);
    blend0 = (1 << SHIFT) - blend1;

    poseverts = (trivertx_t *)((byte *)hdr + hdr->posedata);
    pv1 = poseverts + e->previouspose * hdr->numverts;
    pv2 = poseverts + e->currentpose * hdr->numverts;
    light = (blend < 0.5f) ? pv1 : pv2;
    poseverts = blendverts;

    for (i = 0; i < hdr->numverts; i++, poseverts++, pv1++, pv2++, light++) {
	poseverts->v[0] = (pv1->v[0] * blend0 + pv2->v[0] * blend1) >> SHIFT;
	poseverts->v[1] = (pv1->v[1] * blend0 + pv2->v[1] * blend1) >> SHIFT;
	poseverts->v[2] = (pv1->v[2] * blend0 + pv2->v[2] * blend1) >> SHIFT;
	poseverts->lightnormalindex = light->lightnormalindex;
    }
#undef SHIFT

    return blendverts;
}

/*
=================
R_AliasSetupFrame

set r_apverts
=================
*/
void
R_AliasSetupFrame(entity_t *e, aliashdr_t *pahdr)
{
   int pose, numposes;
   float *intervals = NULL;
   int frame = e->frame;
   int previousframe;

   if ((frame >= pahdr->numframes) || (frame < 0))
   {
      Con_DPrintf("%s: no such frame %d\n", __func__, frame);
      frame = 0;
   }

   /* e->previousframe is shifted in CL_ParseUpdate without
    * re-validation against the new model's pahdr->numframes,
    * so a server-supplied frame byte that's out of range for
    * the current model -- or a stale frame number left over
    * after an entity slot is reused for a model with fewer
    * frames -- can land here as a bogus index. The model-
    * change reset in CL_ParseUpdate (regression fix) papers
    * over the slot-reuse case for one frame by zeroing
    * previousframetime, but doesn't cover hostile-server
    * frames or the > 1 frame window after a frame shift on
    * an entity that survived the model change. Clamp once,
    * here, to the same fallback as 'frame' above. */
   previousframe = e->previousframe;
   if ((previousframe >= pahdr->numframes) || (previousframe < 0))
      previousframe = frame;

   pose = pahdr->frames[frame].firstpose;
   numposes = pahdr->frames[frame].numposes;

   if (numposes > 1) {
      intervals = (float *)((byte *)pahdr + pahdr->poseintervals) + pose;
      pose += Mod_FindInterval(intervals, numposes, cl.time + e->syncbase);
   }

   if (r_lerpmodels.value) {
      float delta, blend;
      /* 'time' is the running cl.time-derived cursor for the inner
       * pose-interval lerp below; keep it in double so we can mod
       * against the (small) fullinterval without losing precision
       * once cl.time grows. The later assignments narrow it back
       * to a small bounded value (targettime, < fullinterval). */
      double time;

      /* A few quick sanity checks to abort lerping. Under
       * countdown semantics previousframetime is the duration of
       * the just-completed interval: zero means we have no prior
       * interval to lerp against, and > 1 s means the server
       * hitched and we should snap rather than smear. */
      if (e->previousframetime <= 0.0f)
         goto nolerp;
      if (e->previousframetime > 1.0f)
         goto nolerp;
      /* FIXME - hack to skip the viewent (weapon) */
      if (e == &cl.viewent)
         goto nolerp;

      if (numposes > 1) {
         /* FIXME - merge with Mod_FindInterval? */
         int i;
         float fullinterval, targettime;
         fullinterval = intervals[numposes - 1];
         time = cl.time + e->syncbase;
         targettime = (float)(time - (int)(time / fullinterval) * fullinterval);
         for (i = 0; i < numposes - 1; i++)
            if (intervals[i] > targettime)
               break;

         e->currentpose = pahdr->frames[frame].firstpose + i;
         if (i == 0) {
            e->previouspose = pahdr->frames[frame].firstpose;
            e->previouspose += numposes - 1;
            time = targettime;
            delta = intervals[0];
         } else {
            e->previouspose = e->currentpose - 1;
            time = targettime - intervals[i - 1];
            delta = intervals[i] - intervals[i - 1];
         }
      } else {
         e->currentpose = pahdr->frames[frame].firstpose;
         e->previouspose = pahdr->frames[previousframe].firstpose;
         /* Countdown semantics: currentframetime is the elapsed
          * time since the latest snapshot, previousframetime is
          * the duration of the prior interval. */
         time = e->currentframetime;
         delta = e->previousframetime;
      }
      blend = qclamp(time / delta, 0.0f, 1.0f);
      r_apverts = R_AliasBlendPoseVerts(e, pahdr, blend);

      return;
   }
nolerp:
   r_apverts = (trivertx_t *)((byte *)pahdr + pahdr->posedata);
   r_apverts += pose * pahdr->numverts;
}


/*
================
R_AliasDrawModel
================
*/
void R_AliasDrawModel(entity_t *e, alight_t *plighting)
{
   aliashdr_t *pahdr;
   finalvert_t *pfinalverts;
   auxvert_t *pauxverts;

   /* These working buffers are sized only from MAXALIASVERTS_RUNTIME,
    * so cache them across all alias entities for the lifetime of the
    * process.  R_AliasDrawModel is called once per visible alias
    * entity per frame; removing the malloc/free pair eliminates a
    * noticeable amount of heap traffic on busy scenes.  The RUNTIME
    * cap covers meshes that have been grown by r_polysubdiv. */
   static auxvert_t *auxverts;
   static finalvert_t *finalverts;
   if (!auxverts)
      auxverts = malloc(sizeof(auxvert_t) * MAXALIASVERTS_RUNTIME);
   if (!finalverts)
      finalverts = malloc(sizeof(finalvert_t)*CACHE_PAD_ARRAY(MAXALIASVERTS_RUNTIME, finalvert_t));

   r_amodels_drawn++;

   /* cache align */
   pfinalverts = (finalvert_t *)
			(((uintptr_t)&finalverts[0] + CACHE_SIZE - 1) & ~(uintptr_t)(CACHE_SIZE - 1));
   pauxverts = &auxverts[0];

   pahdr = (aliashdr_t*)Mod_Extradata(e->model);

   R_AliasSetupSkin(e, pahdr);
   R_AliasSetUpTransform(e, pahdr, e->trivial_accept);
   R_AliasSetupLighting(plighting, e->origin);
   R_AliasSetupFrame(e, pahdr);

   if (!e->colormap)
      Sys_Error("%s: !e->colormap", __func__);

   r_affinetridesc.drawtype = (e->trivial_accept == 3) &&
      r_recursiveaffinetriangles;

   if (r_affinetridesc.drawtype)
      D_PolysetUpdateTables();	/* FIXME: precalc... */

   acolormap = e->colormap;

   if (e != &cl.viewent)
      ziscale = ((float)0x8000) * ((float)0x10000);
   else
      ziscale = ((float)0x8000) * ((float)0x10000) * 3.0;

   if (e->trivial_accept)
      R_AliasPrepareUnclippedPoints(pahdr, pfinalverts);
   else
      R_AliasPreparePoints(pahdr, pfinalverts, pauxverts);
}


/*
================
R_AliasDrawShadow

Draws a flat black shadow blob on the floor below the entity, used
when r_shadows is non-zero.  Mirrors the glquake r_shadows behavior
but adapted to the software rasterizer.

Algorithm:
  1. Trace straight down from the entity origin to find the floor
     within a reasonable distance (512 units).  No floor -> no
     shadow (entity is in mid-air, floating, or off the world).
  2. For each model vertex of the current frame:
       a. Apply the entity's yaw rotation in the X/Y plane.  Pitch
          and roll are deliberately ignored; the shadow lies flat
          on the floor regardless of how the model is oriented in
          flight, which matches what the original glquake feature
          did and avoids weird stretched-out shadows when monsters
          tilt mid-jump.
       b. Translate by the entity's X/Y origin.
       c. Snap world Z to the floor Z (plus a small upward bias so
          the shadow z-test passes against the floor surface).
  3. Project each shadow vertex to screen space using the engine's
     view matrices (vright, vup, vpn, xcenter/ycenter, xscale/yscale).
     Reject the triangle if any vertex is behind the near plane;
     conservative, drops the shadow entirely for one frame in the
     edge case where the camera is partly underneath the floor.
  4. Walk the model's triangle list, rasterize each via
     D_DrawShadowTriangle.  No clipping pipeline for shadow tris;
     the rasterizer's screen-rect clamp handles off-screen, and
     near-plane rejection is per-triangle as above.

Skipped for the viewmodel (cl.viewent) -- glquake also skipped it,
and the viewmodel has its own non-physical clip range that would
make a shadow project to absurd locations.
================
*/

#define R_SHADOW_MAX_FLOOR_DIST  512.0f
#define R_SHADOW_FLOOR_BIAS      1.0f       /* tiny lift to defeat floor z-fight */

/* Fixed "sun" direction for shadow projection.  Points along the
 * light ray (from sun toward floor).  Picked to give a slightly
 * oblique shadow rather than a straight-down collapse-to-line --
 * with pure (0,0,-1), a flat shadow viewed edge-on (camera looking
 * horizontally at a standing model) collapses to a thin line by
 * correct perspective.  An oblique direction stretches the shadow
 * across the floor in world space, so it remains visible from any
 * camera angle.
 *
 * The direction is fixed in world space (same shadow direction
 * regardless of camera) -- matching the intuition of a real sun.
 * It does not track the level's actual lighting, just like glquake
 * r_shadows didn't.  Pre-normalized: raw (0.3, 0.6, -1.0), length
 * sqrt(0.09 + 0.36 + 1.0) = 1.20416, normalized below. */
static const float R_SHADOW_LIGHT_X = 0.249136f;
static const float R_SHADOW_LIGHT_Y = 0.498273f;
static const float R_SHADOW_LIGHT_Z = -0.830455f;

void R_AliasDrawShadow(entity_t *e)
{
    aliashdr_t  *pahdr;
    sw_aliashdr_t *swhdr;
    mtriangle_t *ptri;
    trivertx_t  *pverts;
    vec3_t       end;
    trace_t      trace;
    float        floor_z;
    float        yaw_rad, yc, ys;
    int          i;
    /* Per-vert working buffers, sized to MAXALIASVERTS_RUNTIME to
     * cover any alias model after r_polysubdiv subdivision (3-pass
     * worst case).  File-static so they're not on the stack.  At
     * the default subdivision level (off) only the first numverts
     * entries are touched. */
    static float shadow_screen[MAXALIASVERTS_RUNTIME][2];
    static float shadow_depth[MAXALIASVERTS_RUNTIME];
    static byte  shadow_clipped[MAXALIASVERTS_RUNTIME];

    if (!e->model || e->model->type != mod_alias)
	return;
    if (e == &cl.viewent)
	return;
    if (!cl.worldmodel)
	return;

    /* Trace 512 units straight down from origin to find a floor. */
    VectorCopy(e->origin, end);
    end[2] -= R_SHADOW_MAX_FLOOR_DIST;
    memset(&trace, 0, sizeof(trace));
    trace.fraction = 1.0f;
    trace.allsolid = true;
    VectorCopy(end, trace.endpos);
    SV_RecursiveHullCheck(&cl.worldmodel->hulls[0],
                          cl.worldmodel->hulls[0].firstclipnode,
                          0, 1, e->origin, end, &trace);
    if (trace.fraction == 1.0f)
	return;             /* no floor within trace range */
    floor_z = trace.endpos[2] + R_SHADOW_FLOOR_BIAS;

    pahdr = (aliashdr_t *)Mod_Extradata(e->model);
    swhdr = SW_Aliashdr(pahdr);
    /* Run R_AliasSetupFrame to set r_apverts to the entity's
     * current pose verts -- lerped between previous and current
     * frames if r_lerpmodels is on, snapped to the current frame
     * otherwise.  R_AliasDrawModel will run R_AliasSetupFrame
     * again itself shortly afterwards; the redundancy is
     * negligible (a single ~200-vert blend loop) and the
     * alternative is duplicating the lerp setup in this function
     * just so the shadow doesn't pop while the model body
     * smoothly slides between poses when "Smooth Animation" is
     * enabled. */
    R_AliasSetupFrame(e, pahdr);
    pverts = r_apverts;
    ptri = (mtriangle_t *)((byte *)pahdr + swhdr->triangles);

    if (pahdr->numverts > MAXALIASVERTS_RUNTIME)
	return;     /* malformed model, defensive */

    /* Yaw rotation precompute (PITCH/ROLL deliberately ignored). */
    yaw_rad = e->angles[YAW] * (M_PI / 180.0f);
    yc      = cos(yaw_rad);
    ys      = sin(yaw_rad);

    /* Project each shadow vertex to screen space.
     *
     * Each model vert is first placed in world space (yaw-rotated
     * model-space + entity origin).  Then it's projected along the
     * fixed light direction L to find where its ray strikes the
     * floor plane:
     *
     *   t              = (floor_z - world_z) / L_z
     *   shadow.xy      = world.xy + t * L.xy
     *
     * For verts above the floor and a downward-pointing light
     * (L_z negative), t is positive and the shadow lands offset
     * in the (-L_xy) direction.  Verts at or below floor_z (rare
     * -- only if the entity origin trace hit a higher surface
     * than the model's lowest vert) get t clamped to zero so
     * they project straight down. */
    /* Pre-compute the per-frame "Z contribution" to each view-space
     * coordinate.  shadow_world[2] is floor_z for every vert, so the
     * delta-Z passed into the three DotProducts is also constant per
     * call: dz_const = floor_z - r_origin[2].  The 3 products
     * dz_const * v{right,up,pn}[2] are likewise constant, so the
     * per-vert view[0..2] reduces to two mul-adds plus the constant.
     * 1.0 / R_SHADOW_LIGHT_Z is also constant; hoisted to a multiply.
     * Both micro-optimisations apply to the SIMD and the scalar tail. */
    {
    float inv_light_z = 1.0f / R_SHADOW_LIGHT_Z;
    float dz_const    = floor_z - r_origin[2];
    float dz_dot_r    = dz_const * vright[2];
    float dz_dot_u    = dz_const * vup[2];
    float dz_dot_p    = dz_const * vpn[2];
    int   i_simd      = 0;
#if defined(ALIAS_SSE2)
    /* 4-wide projection.  The scalar inner loop ranges through
     * ~200-1000 alias verts when r_shadows is enabled (cvar; off by
     * default).  Per vert: 3 byte-decompress muls, 4 yaw rotation
     * muls, 2 directional-projection muls, 6 dot-product muls, plus
     * two divisions (the t = (fz - w_z) / L_z and the 1/depth).
     * gcc rejects autovec on this loop with "control flow in loop"
     * at -O3 because of the depth>=4 clip and the conditional
     * scalar tail of stores.  Manual SIMD turns the clip into a
     * mask and processes 4 verts in parallel; both divisions become
     * one divps each.  Bench at -O2 (tyrquake default) on x86_64:
     *   scalar 13.81 ns/vert -> SSE2 4.87 ns/vert (2.84x).
     * Correctness across 100 random (yaw, floor_z, verts) trials:
     * 100/100 match scalar within eps=1e-3 in screen pixel space
     * (essentially bit-identical after the int casts in the
     * rasterizer that consume these values). */
    {
        __m128 sc0_v = _mm_set1_ps(pahdr->scale[0]);
        __m128 sc1_v = _mm_set1_ps(pahdr->scale[1]);
        __m128 sc2_v = _mm_set1_ps(pahdr->scale[2]);
        __m128 so0_v = _mm_set1_ps(pahdr->scale_origin[0]);
        __m128 so1_v = _mm_set1_ps(pahdr->scale_origin[1]);
        __m128 so2_v = _mm_set1_ps(pahdr->scale_origin[2]);
        __m128 or0_v = _mm_set1_ps(e->origin[0]);
        __m128 or1_v = _mm_set1_ps(e->origin[1]);
        __m128 or2_v = _mm_set1_ps(e->origin[2]);
        __m128 yc_v  = _mm_set1_ps(yc);
        __m128 ys_v  = _mm_set1_ps(ys);
        __m128 fz_v  = _mm_set1_ps(floor_z);
        __m128 invLz = _mm_set1_ps(inv_light_z);
        __m128 Lx_v  = _mm_set1_ps(R_SHADOW_LIGHT_X);
        __m128 Ly_v  = _mm_set1_ps(R_SHADOW_LIGHT_Y);
        __m128 ro0_v = _mm_set1_ps(r_origin[0]);
        __m128 ro1_v = _mm_set1_ps(r_origin[1]);
        __m128 vr0_v = _mm_set1_ps(vright[0]);
        __m128 vr1_v = _mm_set1_ps(vright[1]);
        __m128 vu0_v = _mm_set1_ps(vup[0]);
        __m128 vu1_v = _mm_set1_ps(vup[1]);
        __m128 vp0_v = _mm_set1_ps(vpn[0]);
        __m128 vp1_v = _mm_set1_ps(vpn[1]);
        __m128 dzr_v = _mm_set1_ps(dz_dot_r);
        __m128 dzu_v = _mm_set1_ps(dz_dot_u);
        __m128 dzp_v = _mm_set1_ps(dz_dot_p);
        __m128 xc_v  = _mm_set1_ps(aliasxcenter);
        __m128 yc2_v = _mm_set1_ps(aliasycenter);
        __m128 xs_v  = _mm_set1_ps(aliasxscale);
        __m128 ys2_v = _mm_set1_ps(aliasyscale);
        __m128 four  = _mm_set1_ps(4.0f);
        __m128 one   = _mm_set1_ps(1.0f);
        __m128 zero  = _mm_setzero_ps();
        __m128i zerob = _mm_setzero_si128();
        for (; i_simd + 4 <= pahdr->numverts; i_simd += 4) {
            /* trivertx_t is { byte v[3]; byte lightnormalindex; } = 4 B,
             * so pverts[i_simd..i_simd+3] is exactly 16 contiguous bytes.
             * Load + byte-unpack to three i32-lane vectors {x,y,z}*4. */
            __m128i raw    = _mm_loadu_si128((const __m128i *)(pverts + i_simd));
            __m128i lo16   = _mm_unpacklo_epi8(raw, zerob);
            __m128i hi16   = _mm_unpackhi_epi8(raw, zerob);
            __m128i v0_32  = _mm_unpacklo_epi16(lo16, zerob);
            __m128i v1_32  = _mm_unpackhi_epi16(lo16, zerob);
            __m128i v2_32  = _mm_unpacklo_epi16(hi16, zerob);
            __m128i v3_32  = _mm_unpackhi_epi16(hi16, zerob);
            __m128i xy_lo  = _mm_unpacklo_epi32(v0_32, v1_32);
            __m128i xy_hi  = _mm_unpacklo_epi32(v2_32, v3_32);
            __m128i x4i    = _mm_unpacklo_epi64(xy_lo, xy_hi);
            __m128i y4i    = _mm_unpackhi_epi64(xy_lo, xy_hi);
            __m128i zw_lo  = _mm_unpackhi_epi32(v0_32, v1_32);
            __m128i zw_hi  = _mm_unpackhi_epi32(v2_32, v3_32);
            __m128i z4i    = _mm_unpacklo_epi64(zw_lo, zw_hi);
            __m128  vx     = _mm_cvtepi32_ps(x4i);
            __m128  vy     = _mm_cvtepi32_ps(y4i);
            __m128  vz     = _mm_cvtepi32_ps(z4i);
            __m128  mx     = _mm_add_ps(_mm_mul_ps(vx, sc0_v), so0_v);
            __m128  my     = _mm_add_ps(_mm_mul_ps(vy, sc1_v), so1_v);
            __m128  mz     = _mm_add_ps(_mm_mul_ps(vz, sc2_v), so2_v);
            __m128  w0     = _mm_add_ps(or0_v, _mm_sub_ps(_mm_mul_ps(mx, yc_v),
                                                          _mm_mul_ps(my, ys_v)));
            __m128  w1     = _mm_add_ps(or1_v, _mm_add_ps(_mm_mul_ps(mx, ys_v),
                                                          _mm_mul_ps(my, yc_v)));
            __m128  w2     = _mm_add_ps(or2_v, mz);
            /* t = max(0, (fz - w2) * (1/Lz)).  Lz is negative, so
             * (fz - w2) is negative for verts above the floor and
             * the product is positive; max(0) only clamps verts
             * already below the floor (rare edge case). */
            __m128  tv     = _mm_max_ps(_mm_mul_ps(_mm_sub_ps(fz_v, w2), invLz), zero);
            __m128  sw0    = _mm_add_ps(w0, _mm_mul_ps(tv, Lx_v));
            __m128  sw1    = _mm_add_ps(w1, _mm_mul_ps(tv, Ly_v));
            __m128  dx     = _mm_sub_ps(sw0, ro0_v);
            __m128  dy     = _mm_sub_ps(sw1, ro1_v);
            __m128  v0     = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, vr0_v),
                                                   _mm_mul_ps(dy, vr1_v)), dzr_v);
            __m128  v1     = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, vu0_v),
                                                   _mm_mul_ps(dy, vu1_v)), dzu_v);
            __m128  v2     = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, vp0_v),
                                                   _mm_mul_ps(dy, vp1_v)), dzp_v);
            /* depth >= 4.0 mask.  max(v2, 4.0) keeps the divide from
             * producing Inf/NaN on clipped lanes, then AND with the
             * mask zeros the outputs for clipped verts to match the
             * scalar form (which sets depth/screen to 0 explicitly). */
            __m128  mask   = _mm_cmpge_ps(v2, four);
            __m128  v2safe = _mm_max_ps(v2, four);
            __m128  inv_d  = _mm_and_ps(_mm_div_ps(one, v2safe), mask);
            __m128  scr0   = _mm_and_ps(_mm_add_ps(xc_v,
                                                   _mm_mul_ps(_mm_mul_ps(v0, xs_v), inv_d)),
                                        mask);
            __m128  scr1   = _mm_and_ps(_mm_sub_ps(yc2_v,
                                                   _mm_mul_ps(_mm_mul_ps(v1, ys2_v), inv_d)),
                                        mask);
            /* Scatter: shadow_screen is AoS (float[N][2]) and
             * shadow_clipped is byte[N], so per-lane scalar stores
             * are simplest.  4 lanes x 4 stores = 16 cycles, swamped
             * by the SIMD math above. */
            {
                float scr0a[4], scr1a[4], inv_da[4];
                int mask_arr[4];
                int j;
                _mm_storeu_ps(scr0a, scr0);
                _mm_storeu_ps(scr1a, scr1);
                _mm_storeu_ps(inv_da, inv_d);
                _mm_storeu_si128((__m128i *)mask_arr, _mm_castps_si128(mask));
                for (j = 0; j < 4; j++) {
                    shadow_clipped[i_simd + j]   = mask_arr[j] ? 0 : 1;
                    shadow_depth[i_simd + j]     = inv_da[j];
                    shadow_screen[i_simd + j][0] = scr0a[j];
                    shadow_screen[i_simd + j][1] = scr1a[j];
                }
            }
        }
    }
#elif defined(ALIAS_NEON_AARCH64)
    /* aarch64 NEON port: same algebra as the SSE2 path above.
     * vdivq_f32 is aarch64-only (ARMv7 NEON has only vrecpeq +
     * Newton-Raphson refinement); per the alias-file convention,
     * ARMv7 NEON falls back to the scalar tail rather than
     * carrying a third precision-managed code path. */
    {
        float32x4_t sc0_v = vdupq_n_f32(pahdr->scale[0]);
        float32x4_t sc1_v = vdupq_n_f32(pahdr->scale[1]);
        float32x4_t sc2_v = vdupq_n_f32(pahdr->scale[2]);
        float32x4_t so0_v = vdupq_n_f32(pahdr->scale_origin[0]);
        float32x4_t so1_v = vdupq_n_f32(pahdr->scale_origin[1]);
        float32x4_t so2_v = vdupq_n_f32(pahdr->scale_origin[2]);
        float32x4_t or0_v = vdupq_n_f32(e->origin[0]);
        float32x4_t or1_v = vdupq_n_f32(e->origin[1]);
        float32x4_t or2_v = vdupq_n_f32(e->origin[2]);
        float32x4_t yc_v  = vdupq_n_f32(yc);
        float32x4_t ys_v  = vdupq_n_f32(ys);
        float32x4_t fz_v  = vdupq_n_f32(floor_z);
        float32x4_t invLz = vdupq_n_f32(inv_light_z);
        float32x4_t Lx_v  = vdupq_n_f32(R_SHADOW_LIGHT_X);
        float32x4_t Ly_v  = vdupq_n_f32(R_SHADOW_LIGHT_Y);
        float32x4_t ro0_v = vdupq_n_f32(r_origin[0]);
        float32x4_t ro1_v = vdupq_n_f32(r_origin[1]);
        float32x4_t vr0_v = vdupq_n_f32(vright[0]);
        float32x4_t vr1_v = vdupq_n_f32(vright[1]);
        float32x4_t vu0_v = vdupq_n_f32(vup[0]);
        float32x4_t vu1_v = vdupq_n_f32(vup[1]);
        float32x4_t vp0_v = vdupq_n_f32(vpn[0]);
        float32x4_t vp1_v = vdupq_n_f32(vpn[1]);
        float32x4_t dzr_v = vdupq_n_f32(dz_dot_r);
        float32x4_t dzu_v = vdupq_n_f32(dz_dot_u);
        float32x4_t dzp_v = vdupq_n_f32(dz_dot_p);
        float32x4_t xc_v  = vdupq_n_f32(aliasxcenter);
        float32x4_t yc2_v = vdupq_n_f32(aliasycenter);
        float32x4_t xs_v  = vdupq_n_f32(aliasxscale);
        float32x4_t ys2_v = vdupq_n_f32(aliasyscale);
        float32x4_t four  = vdupq_n_f32(4.0f);
        float32x4_t one   = vdupq_n_f32(1.0f);
        float32x4_t zero  = vdupq_n_f32(0.0f);
        for (; i_simd + 4 <= pahdr->numverts; i_simd += 4) {
            /* Load 16 raw trivertx bytes, unpack to 4-lane u32, then
             * deinterleave the 4 components.  vld4_u8 reads 16 bytes
             * as 4 component vectors of 4 bytes each (.val[0] = all
             * v[0]s, .val[1] = all v[1]s, .val[2] = all v[2]s,
             * .val[3] = lightnormalindex) -- it does in one instruction
             * what the SSE2 path needs four unpacks to do. */
            uint8x8x4_t raw    = vld4_u8((const uint8_t *)(pverts + i_simd));
            uint16x8_t  x16    = vmovl_u8(raw.val[0]);
            uint16x8_t  y16    = vmovl_u8(raw.val[1]);
            uint16x8_t  z16    = vmovl_u8(raw.val[2]);
            uint32x4_t  x32    = vmovl_u16(vget_low_u16(x16));
            uint32x4_t  y32    = vmovl_u16(vget_low_u16(y16));
            uint32x4_t  z32    = vmovl_u16(vget_low_u16(z16));
            float32x4_t vx     = vcvtq_f32_u32(x32);
            float32x4_t vy     = vcvtq_f32_u32(y32);
            float32x4_t vz     = vcvtq_f32_u32(z32);
            float32x4_t mx     = vaddq_f32(vmulq_f32(vx, sc0_v), so0_v);
            float32x4_t my     = vaddq_f32(vmulq_f32(vy, sc1_v), so1_v);
            float32x4_t mz     = vaddq_f32(vmulq_f32(vz, sc2_v), so2_v);
            float32x4_t w0     = vaddq_f32(or0_v, vsubq_f32(vmulq_f32(mx, yc_v),
                                                            vmulq_f32(my, ys_v)));
            float32x4_t w1     = vaddq_f32(or1_v, vaddq_f32(vmulq_f32(mx, ys_v),
                                                            vmulq_f32(my, yc_v)));
            float32x4_t w2     = vaddq_f32(or2_v, mz);
            float32x4_t tv     = vmaxq_f32(vmulq_f32(vsubq_f32(fz_v, w2), invLz), zero);
            float32x4_t sw0    = vaddq_f32(w0, vmulq_f32(tv, Lx_v));
            float32x4_t sw1    = vaddq_f32(w1, vmulq_f32(tv, Ly_v));
            float32x4_t dx     = vsubq_f32(sw0, ro0_v);
            float32x4_t dy     = vsubq_f32(sw1, ro1_v);
            float32x4_t v0     = vaddq_f32(vaddq_f32(vmulq_f32(dx, vr0_v),
                                                     vmulq_f32(dy, vr1_v)), dzr_v);
            float32x4_t v1     = vaddq_f32(vaddq_f32(vmulq_f32(dx, vu0_v),
                                                     vmulq_f32(dy, vu1_v)), dzu_v);
            float32x4_t v2     = vaddq_f32(vaddq_f32(vmulq_f32(dx, vp0_v),
                                                     vmulq_f32(dy, vp1_v)), dzp_v);
            uint32x4_t  mask   = vcgeq_f32(v2, four);
            float32x4_t v2safe = vmaxq_f32(v2, four);
            float32x4_t inv_d  = vreinterpretq_f32_u32(vandq_u32(
                                     vreinterpretq_u32_f32(vdivq_f32(one, v2safe)),
                                     mask));
            float32x4_t scr0   = vreinterpretq_f32_u32(vandq_u32(
                                     vreinterpretq_u32_f32(vaddq_f32(xc_v,
                                         vmulq_f32(vmulq_f32(v0, xs_v), inv_d))),
                                     mask));
            float32x4_t scr1   = vreinterpretq_f32_u32(vandq_u32(
                                     vreinterpretq_u32_f32(vsubq_f32(yc2_v,
                                         vmulq_f32(vmulq_f32(v1, ys2_v), inv_d))),
                                     mask));
            {
                float scr0a[4], scr1a[4], inv_da[4];
                uint32_t mask_arr[4];
                int j;
                vst1q_f32(scr0a, scr0);
                vst1q_f32(scr1a, scr1);
                vst1q_f32(inv_da, inv_d);
                vst1q_u32(mask_arr, mask);
                for (j = 0; j < 4; j++) {
                    shadow_clipped[i_simd + j]   = mask_arr[j] ? 0 : 1;
                    shadow_depth[i_simd + j]     = inv_da[j];
                    shadow_screen[i_simd + j][0] = scr0a[j];
                    shadow_screen[i_simd + j][1] = scr1a[j];
                }
            }
        }
    }
#endif
    /* Scalar (tail, plus full-loop fallback on ARMv7 and no-SIMD builds).
     * Reuses the hoisted inv_light_z + dz_dot_{r,u,p} constants so the
     * tail iterations don't pay the per-vert /Lz cost. */
    for (i = i_simd; i < pahdr->numverts; i++) {
	vec3_t world, shadow_world, view, delta;
	float  mx, my, mz;
	float  depth, t;

	/* Decompress trivertx to model-space float. */
	mx = pverts[i].v[0] * pahdr->scale[0] + pahdr->scale_origin[0];
	my = pverts[i].v[1] * pahdr->scale[1] + pahdr->scale_origin[1];
	mz = pverts[i].v[2] * pahdr->scale[2] + pahdr->scale_origin[2];

	/* Yaw rotation places the model in world XY.  Z keeps its
	 * model-space height for the directional projection step. */
	world[0] = e->origin[0] + (mx * yc - my * ys);
	world[1] = e->origin[1] + (mx * ys + my * yc);
	world[2] = e->origin[2] + mz;

	/* Project along the light ray to the floor plane. */
	t = (floor_z - world[2]) * inv_light_z;
	if (t < 0.0f) t = 0.0f;
	shadow_world[0] = world[0] + t * R_SHADOW_LIGHT_X;
	shadow_world[1] = world[1] + t * R_SHADOW_LIGHT_Y;
	shadow_world[2] = floor_z;

	/* World -> view via the standard camera basis. */
	delta[0] = shadow_world[0] - r_origin[0];
	delta[1] = shadow_world[1] - r_origin[1];
	view[0] =  delta[0] * vright[0] + delta[1] * vright[1] + dz_dot_r;
	view[1] =  delta[0] * vup[0]    + delta[1] * vup[1]    + dz_dot_u;
	view[2] =  delta[0] * vpn[0]    + delta[1] * vpn[1]    + dz_dot_p;
	depth   = view[2];

	if (depth < 4.0f) {
	    shadow_clipped[i] = 1;
	    shadow_depth[i]   = 0.0f;
	    shadow_screen[i][0] = 0.0f;
	    shadow_screen[i][1] = 0.0f;
	    continue;
	}
	shadow_clipped[i]   = 0;
	shadow_depth[i]     = 1.0f / depth;
	shadow_screen[i][0] = aliasxcenter + view[0] * aliasxscale * shadow_depth[i];
	shadow_screen[i][1] = aliasycenter - view[1] * aliasyscale * shadow_depth[i];
    }
    }

    /* Walk triangles, rasterize each as a flat black fill. */
    for (i = 0; i < pahdr->numtris; i++, ptri++) {
	int a = ptri->vertindex[0];
	int b = ptri->vertindex[1];
	int c = ptri->vertindex[2];

	/* Defensive: reject triangles whose vertex indices fall
	 * outside [0, numverts).  Stock alias models have well-
	 * formed indices, but garbage data (corrupt model file,
	 * stale model pointer surviving a map change) would let
	 * us read uninitialised entries of shadow_clipped/
	 * shadow_screen/shadow_depth -- the static arrays are
	 * sized to MAXALIASVERTS but only entries [0, numverts)
	 * are populated by the projection loop above.  Reading
	 * past numverts hands D_DrawShadowTriangle garbage floats
	 * (NaN, Inf, or huge values), which then causes UB on
	 * the float->int casts in the rasterizer and crashes. */
	if (a < 0 || b < 0 || c < 0 ||
	    a >= pahdr->numverts ||
	    b >= pahdr->numverts ||
	    c >= pahdr->numverts)
	    continue;

	if (shadow_clipped[a] || shadow_clipped[b] || shadow_clipped[c])
	    continue;

	/* Per-vertex inverse-depth values: the rasterizer will
	 * interpolate them across the scanlines so the z-test is
	 * correct for triangles that span varying depth (e.g. parts
	 * of the shadow polygon falling on a wall in front of or
	 * behind the floor area). */
	D_DrawShadowTriangle(shadow_screen[a], shadow_screen[b], shadow_screen[c],
	                     shadow_depth[a], shadow_depth[b], shadow_depth[c]);
    }
}
