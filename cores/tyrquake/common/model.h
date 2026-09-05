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

#ifndef MODEL_H
#define MODEL_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef MAX_DLIGHTS
#define MAX_DLIGHTS 32
#endif

#if defined(_WIN32) || defined(__SNC__)
static INLINE int ffsl (long bits)
{
	unsigned long int shift;
	unsigned long int ind = (bits & -bits);
	
	static const unsigned char lookup[] = {
		0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	};
	
	shift = (ind > 0xffff) ? (ind > 0xffffff ? 24 : 16)
				 : (ind > 0xff ? 8 : 0);
	return lookup[ind >> shift] + shift;
}

static INLINE int ffs (int x)
{
   return ffsl(x);
}
#else
/* Use the GCC builtin ffsl function */
#ifndef ffsl
#define ffsl __builtin_ffsl
#endif
#endif

#include "bspfile.h"
#include "modelgen.h"
#include "spritegn.h"
#include "zone.h"

#include "quakedef.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/* entity effects */
#define EF_BRIGHTFIELD	1
#define EF_MUZZLEFLASH 	2
#define EF_BRIGHTLIGHT 	4
#define EF_DIMLIGHT 	8
#define EF_FLAG1	16
#define EF_FLAG2	32
#define EF_BLUE		64
#define EF_RED		128

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


/**/
/* in memory representation */
/**/
/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct {
    vec3_t position;
} mvertex_t;

typedef struct texture_s {
    char name[16];
    unsigned width, height;
    int anim_total;		/* total tenths in sequence ( 0 = no) */
    int anim_min, anim_max;	/* time for this frame min <=time< max */
    struct texture_s *anim_next;	/* in the animation sequence */
    struct texture_s *alternate_anims;	/* bmodels in frmae 1 use these */
    unsigned offsets[MIPLEVELS];	/* four mip maps stored */
} texture_t;


#define	SURF_PLANEBACK		(1 << 1)
#define	SURF_DRAWSKY		(1 << 2)
#define SURF_DRAWSPRITE		(1 << 3)
#define SURF_DRAWTURB		(1 << 4)
#define SURF_DRAWTILED		(1 << 5)
#define SURF_DRAWBACKGROUND	(1 << 6)

/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct {
    unsigned int v[2];
    unsigned int cachededgeoffset;
} medge_t;

typedef struct {
    float vecs[2][4];
    float mipadjust;
    texture_t *texture;
    int flags;
} mtexinfo_t;

typedef struct msurface_s {
    /* Cache-line 0 (offsets 0-63): "hot" fields touched during the
     * per-frame BSP traversal / PVS expansion / frustum culling /
     * dynamic-light association phase.  Every surface in the camera
     * PVS gets these fields read or written before the engine
     * decides whether to actually render the surface, so packing
     * them into one cache line keeps frustum-culled surfaces from
     * paying a line-1 load.  Layout below totals exactly 64 bytes
     * with no waste padding.  Access sites:
     *   visframe     -- R_MarkSurfaces, R_RecursiveWorldNode
     *   clipflags    -- R_CullSurfaces, R_RecursiveWorldNode
     *   flags        -- SURF_DRAWTURB / SURF_PLANEBACK gates throughout
     *   dlightframe, dlightbits -- R_MarkLights per-frame dlight bitset
     *   mins, maxs   -- BoxOnPlaneSide frustum test
     *   plane        -- backface test (plane->normal, plane->dist)
     *   styles       -- R_BuildLightMap inner gate
     * styles is in line 0 because R_AnimateLight + R_BuildLightMap
     * read it for every visible surface; the byte[4] also fits the
     * 4-byte gap before the next 8-byte-aligned pointer. */
    int             visframe;     /* should be drawn when node is crossed */
    int             clipflags;    /* flags for clipping against frustum */
    int             flags;        /* SURF_DRAWTURB et al */
    int             dlightframe;
    vec3_t          mins;         /* bounding box for frustum culling */
    byte            styles[MAXLIGHTMAPS];
    vec3_t          maxs;
    mplane_t       *plane;
    unsigned        dlightbits[(MAX_DLIGHTS + 31) >> 5]; /* qbism from MH - increase max_dlights */

    /* Cache-line 1 (offsets 64-127): "cold" fields touched only when
     * the surface is actually being rendered -- after frustum +
     * backface culling have passed and dispatch reaches the edge
     * walker / lightmap builder / rasterizer.  Surfaces that fail
     * culling never read this line.
     *   firstedge, numedges -- R_RenderFace edge walker
     *   texturemins, extents -- lightmap + UV setup
     *   texinfo      -- texture pointer + u/v vecs
     *   samples      -- per-style lightmap byte data
     *   cachespots   -- surface cache lookup per mip */
    int             firstedge;    /* look up in model->surfedges[], negative numbers */
    int             numedges;     /* are backwards edges */
    short           texturemins[2];
    short           extents[2];
    mtexinfo_t     *texinfo;
    byte           *samples;      /* [numstyles*surfsize] */
    struct surfcache_s *cachespots[MIPLEVELS];
} msurface_t;

typedef struct mnode_s {
/* common with leaf */
    int contents;		/* 0, to differentiate from leafs */
    int visframe;		/* node needs to be traversed if current */
    int clipflags;		/* frustum plane clip flags */

    vec3_t mins;		/* for bounding box culling */
    vec3_t maxs;

    struct mnode_s *parent;

/* node specific */
    mplane_t *plane;
    struct mnode_s *children[2];

    unsigned int firstsurface;
    unsigned int numsurfaces;
} mnode_t;

/* forward decls; can't include render.h/glquake.h */
struct efrag_s;
struct entity_s;

typedef struct mleaf_s {
/* common with node */
    int contents;		/* wil be a negative contents number */
    int visframe;		/* node needs to be traversed if current */
    int clipflags;		/* frustum plane clip flags */

    vec3_t mins;		/* for bounding box culling */
    vec3_t maxs;

    struct mnode_s *parent;

/* leaf specific */
    byte *compressed_vis;
    struct efrag_s *efrags;

    msurface_t **firstmarksurface;
    int nummarksurfaces;
    int key;			/* BSP sequence number for leaf's contents */
    byte ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

/* !!! if this is changed, it must be changed in asm_i386.h too !!! */
typedef struct {
    mclipnode_t *clipnodes;
    mplane_t *planes;
    int firstclipnode;
    int lastclipnode;
    vec3_t clip_mins;
    vec3_t clip_maxs;
} hull_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


/* FIXME: shorten these? */
typedef struct mspriteframe_s {
    int width;
    int height;
    float up, down, left, right;
    byte rdata[1];	/* Renderer data, variable sized */
} mspriteframe_t;

/*
 * Renderer provides this function to specify the amount of space it needs for
 * a sprite frame with given pixel count
 */
int R_SpriteDataSize(int numpixels);

/*
 * Renderer provides this function to translate and store the raw sprite data
 * from the model file as needed.
 */
void R_SpriteDataStore(mspriteframe_t *frame, int framenum, byte *pixels);

typedef struct {
    int numframes;
    float *intervals;
    mspriteframe_t *frames[1];	/* variable sized */
} mspritegroup_t;

typedef struct {
    spriteframetype_t type;
    mspriteframe_t *frameptr;
} mspriteframedesc_t;

typedef struct {
    int type;
    int maxwidth;
    int maxheight;
    int numframes;
    float beamlength;		/* remove? */
    mspriteframedesc_t frames[1];	/* variable sized */
} msprite_t;

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
    int firstpose;
    int numposes;
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    int frame;
} maliasframedesc_t;

typedef struct {
    int firstframe;
    int numframes;
} maliasskindesc_t;

/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct mtriangle_s {
    int facesfront;
    int vertindex[3];
} mtriangle_t;

typedef struct {
    vec3_t scale;
    vec3_t scale_origin;
    int numskins;
    int skindesc;
    int skinintervals;
    int skindata;
    int skinwidth;
    int skinheight;
    int numverts;
    int numtris;
    int numframes;
    float size;
    int numposes;
    int poseintervals;
    int posedata;	/* (numposes * numverts) trivertx_t */
    maliasframedesc_t frames[1];	/* variable sized */
} aliashdr_t;


typedef struct {
    int stverts;
    int triangles;
    aliashdr_t ahdr;
} sw_aliashdr_t;

static INLINE sw_aliashdr_t *SW_Aliashdr(aliashdr_t *h)
{
   return container_of(h, sw_aliashdr_t, ahdr);
}


#define	MAXALIASVERTS	2048
#define	MAXALIASFRAMES	512
#define	MAXALIASTRIS	4096

/* =================================================================== */

/**/
/* Whole model */
/**/

typedef enum { mod_brush, mod_sprite, mod_alias, ENSURE_INT_MODTYPE = 0x70000000 } modtype_t;

#define	EF_ROCKET	1	/* leave a trail */
#define	EF_GRENADE	2	/* leave a trail */
#define	EF_GIB		4	/* leave a trail */
#define	EF_ROTATE	8	/* rotate (bonus items) */
#define	EF_TRACER	16	/* green split trail */
#define	EF_ZOMGIB	32	/* small blood trail */
#define	EF_TRACER2	64	/* orange split trail + rotate */
#define	EF_TRACER3	128	/* purple trail */

typedef struct model_s {
    char name[MAX_QPATH];
    qboolean needload;		/* bmodels and sprites don't cache normally */

    modtype_t type;
    int numframes;
    synctype_t synctype;

    int flags;

/**/
/* volume occupied by the model graphics */
/**/
    vec3_t mins, maxs;
    float radius;

/**/
/* brush model */
/**/
    int firstmodelsurface, nummodelsurfaces;

    int numsubmodels;
    dmodel_t *submodels;

    int numplanes;
    mplane_t *planes;

    int numleafs;		/* number of visible leafs, not counting 0 */
    mleaf_t *leafs;

    int numvertexes;
    mvertex_t *vertexes;

    int numedges;
    medge_t *edges;

    int numnodes;
    mnode_t *nodes;

    int numtexinfo;
    mtexinfo_t *texinfo;

    int numsurfaces;
    msurface_t *surfaces;

    int numsurfedges;
    int *surfedges;

    int numclipnodes;
    mclipnode_t *clipnodes;

    int nummarksurfaces;
    msurface_t **marksurfaces;

    hull_t hulls[MAX_MAP_HULLS];

    int numtextures;
    texture_t **textures;

    byte *visdata;
    int visdatasize;
    byte *lightdata;
    int lightdatasize;
    char *entities;

/**/
/* additional model data */
/**/
    cache_user_t cache;		/* only access through Mod_Extradata */

} model_t;

typedef struct model_loader {
    int (*Aliashdr_Padding)(void);
    void *(*LoadSkinData)(const char *, aliashdr_t *, int, byte **);
    void (*LoadMeshData)(const model_t *, aliashdr_t *hdr, const mtriangle_t *,
			 const stvert_t *, const trivertx_t **);
} model_loader_t;

/* ============================================================================ */

void Mod_Init(const model_loader_t *loader);
void Mod_Shutdown(void);
void Mod_ClearAll(void);
void Mod_FlushAlias(void);
model_t *Mod_ForName(const char *name, qboolean crash);
void *Mod_Extradata(model_t *mod);	/* handles caching */
void Mod_TouchModel(char *name);
void Mod_Print(void);

/*
 * PVS/PHS information
 */
typedef unsigned long leafblock_t;
typedef struct {
    int numleafs;
    leafblock_t bits[1]; /* Variable Sized */
} leafbits_t;

mleaf_t *Mod_PointInLeaf(const model_t *model, const vec3_t point);
const leafbits_t *Mod_LeafPVS(const model_t *model, const mleaf_t *leaf);
const leafbits_t *Mod_FatPVS(const model_t *model, const vec3_t point);
const leafbits_t *Mod_FatPVSThroughLiquid(const model_t *model, const vec3_t point);

#ifdef _WIN32
static INLINE int __ERRORLONGSIZE(void)
{
   fprintf(stderr, "Error occurred.\n");
   return 0;
}
#else
int __ERRORLONGSIZE(void); /* to generate an error at link time */
#endif
#define QBYTESHIFT(x) ((x) == 8 ? 6 : ((x) == 4 ? 5 : __ERRORLONGSIZE() ))
#define LEAFSHIFT QBYTESHIFT(sizeof(leafblock_t))
#define LEAFMASK  ((sizeof(leafblock_t) << 3) - 1UL)

static INLINE qboolean
Mod_TestLeafBit(const leafbits_t *bits, int leafnum)
{
    return !!(bits->bits[leafnum >> LEAFSHIFT] & (1UL << (leafnum & LEAFMASK)));
}

static INLINE size_t
Mod_LeafbitsSize(int numleafs)
{
    return offsetof(leafbits_t, bits[(numleafs + LEAFMASK) >> LEAFSHIFT]);
}

static INLINE int
Mod_NextLeafBit(const leafbits_t *leafbits, int leafnum, leafblock_t *check)
{
    int bit;

    if (!*check) {
	leafnum += (1 << LEAFSHIFT);
	leafnum &= ~LEAFMASK;
	if (leafnum < leafbits->numleafs)
	    *check = leafbits->bits[leafnum >> LEAFSHIFT];
	while (!*check) {
	    leafnum += (1 << LEAFSHIFT);
	    if (leafnum < leafbits->numleafs)
		*check = leafbits->bits[leafnum >> LEAFSHIFT];
	    else
		return leafbits->numleafs;
	}
    }

    bit = ffsl(*check) - 1;
    leafnum = (leafnum & ~LEAFMASK) + bit;
    *check &= ~(1UL << bit);

    return leafnum;
}

/*
 * Macro to iterate over just the ones in the leaf bit array
 */
#define foreach_leafbit(leafbits, leafnum, check) \
    for (	check = 0, leafnum = Mod_NextLeafBit(leafbits, -1, &check); \
		leafnum < leafbits->numleafs;				    \
		leafnum = Mod_NextLeafBit(leafbits, leafnum, &check) )

/* 'OR' the bits of src into dst */
void Mod_AddLeafBits(leafbits_t *dst, const leafbits_t *src);

/* FIXME - surely this doesn't belong here? */
texture_t *R_TextureAnimation(const struct entity_s *e, texture_t *base);

void Mod_LoadAliasModel(const model_loader_t *loader, model_t *mod,
			void *buffer, const model_t *loadmodel);
void Mod_LoadSpriteModel(model_t *mod, void *buffer);

mspriteframe_t *Mod_GetSpriteFrame(const struct entity_s *e,
				   msprite_t *psprite, double time);

int Mod_FindInterval(const float *intervals, int numintervals, double time);

#endif /* MODEL_H */
