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

#ifndef RENDER_H
#define RENDER_H

#include "cvar.h"
#include "mathlib.h"
#include "model.h"
#include "vid.h"

#include "quakedef.h"

/* render.h -- public interface to refresh functions */

#define	TOP_RANGE	16	/* soldier uniform colors */
#define	BOTTOM_RANGE	96

/* ============================================================================= */

typedef struct efrag_s {
    struct mleaf_s *leaf;
    struct efrag_s *leafnext;
    struct entity_s *entity;
    struct efrag_s *entnext;
} efrag_t;


typedef struct entity_s {
    qboolean forcelink;		/* model changed */

    int update_type;

    entity_state_t baseline;	/* to fill in defaults in updates */

    double msgtime;		/* time of last update */
    vec3_t msg_origins[2];	/* last two updates (0 is newest) */
    vec3_t msg_angles[2];	/* last two updates (0 is newest) */
    vec3_t origin;
    vec3_t angles;
    struct model_s *model;	/* NULL = no model */
    int frame;
    byte *colormap;
    int skinnum;		/* for Alias models */
    float syncbase;		/* for client-side animations */

    struct efrag_s *efrag;	/* linked list of efrags (FIXME) */
    int visframe;		/* last frame this entity was */
    /* found in an active leaf */

    int effects;		/* light, particals, etc */
    int dlightframe;		/* dynamic lighting */
    /* qbism - not used here... */

    /* FIXME: could turn these into a union */
    int trivial_accept;
    struct mnode_s *topnode;	/* for bmodels, first world node */
				/*  that splits bmodel, or NULL if */
				/*  not split */

    /* Alias model lerping.
     *
     * The previousX/currentX time fields below use countdown semantics
     * to avoid fp32 precision drift once cl.time grows: currentX is the
     * elapsed time since the latest snapshot (incremented per frame in
     * CL_RelinkEntities, reset to 0 at each shift event), previousX is
     * the duration of the previous interval (set at each shift to the
     * outgoing currentX). The consumer's lerp is currentX / previousX,
     * clamped to [0,1]; previousX <= 0 means 'no completed interval'
     * and is treated as 'snap, don't lerp'. */
    short previouspose;
    short currentpose;
    short previousframe;
    short currentframe;
    float previousframetime;
    float currentframetime;
    vec3_t previousorigin;
    vec3_t currentorigin;
    float previousorigintime;
    float currentorigintime;
    vec3_t previousangles;
    vec3_t currentangles;
    float previousanglestime;
    float currentanglestime;
} entity_t;

extern cvar_t r_lerpmodels;
extern cvar_t r_lerpmove;

/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct {
    vrect_t vrect;		/* subwindow in video for refresh */
    /* FIXME: not need vrect next field here? */
    vrect_t aliasvrect;		/* scaled Alias version */
    int vrectright, vrectbottom;	/* right & bottom screen coords */
    int aliasvrectright, aliasvrectbottom;	/* scaled Alias versions */
    float vrectrightedge;	/* rightmost right edge we care about, */
    /*  for use in edge list */
    float fvrectx, fvrecty;	/* for floating-point compares */
    float fvrectx_adj, fvrecty_adj;	/* left and top edges, for clamping */
    int vrect_x_adj_shift20;	/* (vrect.x + 0.5 - epsilon) << 20 */
    int vrectright_adj_shift20;	/* (vrectright + 0.5 - epsilon) << 20 */
    float fvrectright_adj, fvrectbottom_adj;
    /* right and bottom edges, for clamping */
    float fvrectright;		/* rightmost edge, for Alias clamping */
    float fvrectbottom;		/* bottommost edge, for Alias clamping */
    float horizontalFieldOfView;	/* at Z = 1.0, this many X is visible */
    /* 2.0 = 90 degrees */
    float xOrigin;		/* should probably allways be 0.5 */
    float yOrigin;		/* between be around 0.3 to 0.5 */

    vec3_t vieworg;
    vec3_t viewangles;

    float fov_x, fov_y;

    int ambientlight;
} refdef_t;


/**/
/* refresh */
/**/

extern refdef_t r_refdef;
extern vec3_t r_origin, vpn, vright, vup;

extern struct texture_s *r_notexture_mip;

extern entity_t r_worldentity;

void R_Init(void);
void R_InitTextures(void);
void R_InitEfrags(void);
void R_RenderView(void);	/* must set r_refdef first */
void R_PrepareFrame(void);	/* per-frame camera basis: r_origin / vpn / vright / vup
				 * V_RenderView calls this before dispatching to the
				 * RHI backend so non-renderer code (S_Update) sees
				 * up-to-date listener state regardless of which
				 * backend is active. */
void R_ViewChanged(vrect_t *pvrect, int lineadj, float aspect);
				/* called whenever r_refdef or vid change */

void R_InitSky(struct texture_s *mt);	/* called at level load */

void R_AddEfrags(entity_t *ent);
void R_RemoveEfrags(entity_t *ent);

void R_NewMap(void);

void R_ParseParticleEffect(void);
void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail(vec3_t start, vec3_t end, int type);
void R_BlobExplosion(vec3_t org);
void R_ParticleExplosion(vec3_t org);
void R_LavaSplash(vec3_t org);
void R_TeleportSplash(vec3_t org);

void R_EntityParticles(const entity_t *ent);
void R_ParticleExplosion2(vec3_t org, int colorStart, int colorLength);

void R_InitParticles(void);
void R_ClearParticles(void);
void R_DrawParticles(void);

/*
 * The renderer supplies callbacks to the model loader
 */
const model_loader_t *R_ModelLoader(void);

/*
 * Display aspect-ratio selector backing the r_aspect cvar.
 * Implemented in libretro.c, which owns environ_cb and the display
 * geometry.  R_AspectRatioForIndex maps the 0..R_ASPECT_MAX cvar
 * value to a float aspect; R_AspectRatioChanged is the cvar callback
 * that clamps r_aspect and repushes the geometry to the frontend.
 */
#define R_ASPECT_NUM_RATIOS 5
#define R_ASPECT_MAX        (R_ASPECT_NUM_RATIOS - 1)
float R_AspectRatioForIndex(int idx);
void  R_AspectRatioChanged(cvar_t *var);

/**/
/* surface cache related */
/**/
int D_SurfaceCacheForRes(int width, int height);
void D_FlushCaches(void);
void D_DeleteSurfaceCache(void);
void D_InitCaches(void *buffer, int size);
void R_SetVrect(const vrect_t *pvrectin, vrect_t *pvrect, int lineadj);

#endif /* RENDER_H */
