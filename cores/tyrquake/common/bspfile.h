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

#ifndef BSPFILE_H
#define BSPFILE_H

#include <stdint.h>

#include "qtypes.h"

/* upper design bounds */

#define	MAX_MAP_HULLS		4

#define	MAX_MAP_MODELS		256
#define	MAX_MAP_BRUSHES		4096
#define	MAX_MAP_ENTITIES	1024
#define	MAX_MAP_ENTSTRING	65536

#define	MAX_MAP_PLANES		16384	/* TYR - was 8192 */
#define	MAX_MAP_NODES		32767	/* negative shorts are contents */
#define	MAX_MAP_CLIPNODES	32767	/* negative shorts are contents */
#define	MAX_MAP_LEAFS		32767	/* negative shorts are contents */
#define	MAX_MAP_VERTS		65535
#define	MAX_MAP_FACES		65535
#define	MAX_MAP_MARKSURFACES	65535
#define	MAX_MAP_TEXINFO		4096
#define	MAX_MAP_EDGES		256000
#define	MAX_MAP_SURFEDGES	512000
#define	MAX_MAP_TEXTURES	512
#define	MAX_MAP_MIPTEX		0x200000
#define	MAX_MAP_LIGHTING	0x100000
#define	MAX_MAP_VISIBILITY	0x100000

#define	MAX_MAP_PORTALS		65536

/* key / value pair sizes */

#define	MAX_KEY		32
#define	MAX_VALUE	1024

/* ============================================================================= */

#define BSPVERSION	29
#define BSP2VERSION	(('B' << 24) | ('S' << 16) | ('P' << 8) | '2')

typedef struct {
    int32_t fileofs;
    int32_t filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES		1
#define	LUMP_TEXTURES		2
#define	LUMP_VERTEXES		3
#define	LUMP_VISIBILITY		4
#define	LUMP_NODES		5
#define	LUMP_TEXINFO		6
#define	LUMP_FACES		7
#define	LUMP_LIGHTING		8
#define	LUMP_CLIPNODES		9
#define	LUMP_LEAFS		10
#define	LUMP_MARKSURFACES	11
#define	LUMP_EDGES		12
#define	LUMP_SURFEDGES		13
#define	LUMP_MODELS		14

#define	HEADER_LUMPS	15

typedef struct {
    float mins[3], maxs[3];
    float origin[3];
    int32_t headnode[MAX_MAP_HULLS];
    int32_t visleafs;		/* not including the solid leaf 0 */
    int32_t firstface, numfaces;
} dmodel_t;

typedef struct {
    int32_t version;
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
    int32_t nummiptex;
    int32_t dataofs[4];		/* [nummiptex] */
} dmiptexlump_t;

#define	MIPLEVELS	4
typedef struct miptex_s {
    char name[16];
    uint32_t width, height;
    uint32_t offsets[MIPLEVELS];	/* four mip maps stored */
} miptex_t;

typedef struct {
    float point[3];
} dvertex_t;


/* 0-2 are axial planes */
#define	PLANE_X		0
#define	PLANE_Y		1
#define	PLANE_Z		2

/* 3-5 are non-axial planes snapped to the nearest */
#define	PLANE_ANYX	3
#define	PLANE_ANYY	4
#define	PLANE_ANYZ	5

typedef struct {
    float normal[3];
    float dist;
    int32_t type;	/* PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate */
} dplane_t;


#define	CONTENTS_EMPTY		-1
#define	CONTENTS_SOLID		-2
#define	CONTENTS_WATER		-3
#define	CONTENTS_SLIME		-4
#define	CONTENTS_LAVA		-5
#define	CONTENTS_SKY		-6
#define	CONTENTS_ORIGIN		-7	/* removed at csg time       */
#define	CONTENTS_CLIP		-8	/* changed to contents_solid */

#define	CONTENTS_CURRENT_0	-9
#define	CONTENTS_CURRENT_90	-10
#define	CONTENTS_CURRENT_180	-11
#define	CONTENTS_CURRENT_270	-12
#define	CONTENTS_CURRENT_UP	-13
#define	CONTENTS_CURRENT_DOWN	-14

typedef struct {
    int32_t planenum;
    int16_t children[2];	/* negative numbers are -(leafs+1), not nodes */
    int16_t mins[3];		/* for sphere culling */
    int16_t maxs[3];
    uint16_t firstface;
    uint16_t numfaces;	/* counting both sides */
} bsp29_dnode_t;

typedef struct {
    int32_t planenum;
    int32_t children[2];	/* negative numbers are -(leafs+1), not nodes */
    int16_t mins[3];		/* for sphere culling */
    int16_t maxs[3];
    uint32_t firstface;
    uint32_t numfaces;	/* counting both sides */
} bsp2_dnode_t;

/*
 * Note that children are interpreted as unsigned values now, so that we can
 * handle > 32k clipnodes. Values > 0xFFF0 can be assumed to be CONTENTS
 * values and can be read as the signed value to be compatible with the above
 * (i.e. simply subtract 65536).
 *
 * I should probably change the type here to uint16_t eventaully and fix up
 * the rest of the code.
 */
typedef struct {
    int32_t planenum;
    int16_t children[2];
} bsp29_dclipnode_t;

typedef struct {
    int32_t planenum;
    int32_t children[2];
} bsp2_dclipnode_t;

typedef struct {
    int32_t planenum;
    int32_t children[2];
} mclipnode_t;

typedef struct texinfo_s {
    float vecs[2][4];		/* [s/t][xyz offset] */
    int32_t miptex;
    int32_t flags;
} texinfo_t;

#define	TEX_SPECIAL	1	/* sky or slime, no lightmap or 256 subdivision */

/* note that edge 0 is never used, because negative edge nums are used for */
/* counterclockwise use of the edge in a face */
typedef struct {
    uint16_t v[2];	/* vertex numbers */
} bsp29_dedge_t;

typedef struct {
    uint32_t v[2];	/* vertex numbers */
} bsp2_dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct {
    int16_t planenum;
    int16_t side;

    int32_t firstedge;		/* we must support > 64k edges */
    int16_t numedges;
    int16_t texinfo;

    /* lighting info */
    uint8_t styles[MAXLIGHTMAPS];
    int32_t lightofs;		/* start of [numstyles*surfsize] samples */
} bsp29_dface_t;

typedef struct {
    int32_t planenum;
    int32_t side;

    int32_t firstedge;		/* we must support > 64k edges */
    int32_t numedges;
    int32_t texinfo;

    /* lighting info */
    uint8_t styles[MAXLIGHTMAPS];
    int32_t lightofs;		/* start of [numstyles*surfsize] samples */
} bsp2_dface_t;

#define	AMBIENT_WATER	0
#define	AMBIENT_SKY	1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS	4	/* automatic ambient sounds */

/* leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas */
/* all other leafs need visibility info */
typedef struct {
    int32_t contents;
    int32_t visofs;			/* -1 = no visibility info */

    int16_t mins[3];		/* for frustum culling */
    int16_t maxs[3];

    uint16_t firstmarksurface;
    uint16_t nummarksurfaces;

    uint8_t ambient_level[NUM_AMBIENTS];
} bsp29_dleaf_t;

typedef struct {
    int32_t contents;
    int32_t visofs;			/* -1 = no visibility info */

    int16_t mins[3];		/* for frustum culling */
    int16_t maxs[3];

    uint32_t firstmarksurface;
    uint32_t nummarksurfaces;

    uint8_t ambient_level[NUM_AMBIENTS];
} bsp2_dleaf_t;

#endif /* BSPFILE_H */
