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

#ifndef D_IFACE_H
#define D_IFACE_H

#include "cvar.h"
#include "mathlib.h"
#include "model.h"
#include "qtypes.h"
#include "vid.h"

/* d_iface.h: interface header file for rasterization driver modules */

/*
 * Maximum dimensions of the underwater-warp render buffer. Sized to
 * MAXWIDTH/2 x MAXHEIGHT/2 (currently 1280x720) so the runtime warp
 * scale (controlled by r_waterwarp_scale) can render at up to half
 * of MAXWIDTH x MAXHEIGHT before having to clamp. The actual render
 * size is set in vid.maxwarpwidth/maxwarpheight at vid init and may
 * be smaller depending on the active resolution and scale cvar.
 */
#define WARP_WIDTH		1280
#define WARP_HEIGHT		720

/* FIXME - was NQ=480, QW=200 - does it matter? */
#define MAX_LBM_HEIGHT	480

typedef struct {
    float u, v;
    float s, t;
    float zi;
} emitpoint_t;

typedef enum {
    pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2,
    pt_blob, pt_blob2,
    ENSURE_INT_PTYPE = 0x70000000
} ptype_t;

/* !!! if this is changed, it must be changed in d_ifacea.h too !!! */
typedef struct particle_s {
/* driver-usable fields */
    vec3_t org;
    float color;
/* drivers never touch the following fields */
    struct particle_s *next;
    vec3_t vel;
    float ramp;
    float die;		/* remaining lifetime in seconds; expires at <= 0 */
    ptype_t type;
} particle_t;

#define PARTICLE_Z_CLIP	8.0

/* !!! if this is changed, it must be changed in d_ifacea.h too !!! */
typedef struct finalvert_s {
    int v[6];			/* u, v, s, t, l, 1/z */
    int flags;
    float reserved;
    float n[3];			/* vertex normal in model space (Phong shading) */
} finalvert_t;

/* !!! if this is changed, it must be changed in d_ifacea.h too !!! */
typedef struct {
    void *pskin;
    int skinwidth;
    int skinheight;
    mtriangle_t *ptriangles;
    finalvert_t *pfinalverts;
    int numtriangles;
    int drawtype;
    int seamfixupX16;
} affinetridesc_t;

/* !!! if this is changed, it must be changed in d_ifacea.h too !!! */
typedef struct {
    float u, v, zi, color;
} screenpart_t;

typedef struct {
    int nump;
    emitpoint_t *pverts;
    /* there's room for an extra element at [nump], */
    /*  if the driver wants to duplicate element [0] at */
    /*  element [nump] to avoid dealing with wrapping */
    mspriteframe_t *pspriteframe;
    vec3_t vup, vright, vpn;	/* in worldspace */
    float nearzi;
} spritedesc_t;

extern int r_framecount;	/* sequence # of current frame since Quake started */
extern qboolean r_recursiveaffinetriangles;	/* true if a driver wants to use */

					    /*  recursive triangular subdivison */
					    /*  and vertex drawing via */
					    /*  D_PolysetDrawFinalVerts() past */
					    /*  a certain distance (normally */
					    /*  only used by the software */
					    /*  driver) */
extern float r_aliasuvscale;	/* scale-up factor for screen u and v */

				/*  on Alias vertices passed to driver */
extern qboolean r_dowarp;

extern affinetridesc_t r_affinetridesc;
extern spritedesc_t r_spritedesc;

extern vec3_t r_pright, r_pup, r_ppn;

void D_Aff8Patch(void *pcolormap);
void D_BeginDirectRect(int x, int y, const byte *pbitmap, int width,
		       int height);
void D_EndDirectRect(int x, int y, int width, int height);
void D_PolysetDraw(void);
void D_PolysetDrawFinalVerts(finalvert_t *fv, int numverts);
void D_DrawParticle(particle_t *pparticle);
void D_DrawSprite(void);
void D_DrawSurfaces(void);
void D_EndParticles(void);
void D_Init(void);
void D_ViewChanged(void);
void D_SetupFrame(void);
void D_StartParticles(void);
void D_TurnZOn(void);
void D_WarpScreen(void);

void D_DrawRect(void);
void D_UpdateRects(vrect_t *prect);

/* currently for internal use only, and should be a do-nothing function in */
/* hardware drivers */
/* FIXME: this should go away */
void D_PolysetUpdateTables(void);

/* these are currently for internal use only, and should not be used by drivers */
extern int r_skydirect;
extern byte   *skyunderlay, *skyoverlay; /* Manoel Kasimier - smooth sky */

/* transparency types for D_DrawRect () */
#define DR_SOLID		0
#define DR_TRANSPARENT	1

/* !!! must be kept the same as in quakeasm.h !!! */
#define TRANSPARENT_COLOR	0xFF

extern void *acolormap;		/* FIXME: should go away */

/* =======================================================================// */

/* callbacks to Quake */

typedef struct {
    pixel_t *surfdat;		/* destination for generated surface */
    int rowbytes;		/* destination logical width in bytes */
    msurface_t *surf;		/* description for surface to generate */
    fixed8_t lightadj[MAXLIGHTMAPS];

    /* adjust for lightmap levels for dynamic lighting */
    texture_t *texture;		/* corrected for animating textures */
    int surfmip;		/* mipmapped ratio surface texels/world pixels */
    int surfwidth;		/* in mipmapped texels */
    int surfheight;		/* in mipmapped texels */
} drawsurf_t;

extern drawsurf_t r_drawsurf;

void R_DrawSurface(void);


/* !!! if this is changed, it must be changed in d_ifacea.h too !!! */
#define TURB_TEX_SIZE	64	/* base turbulent texture size */

/* !!! if this is changed, it must be changed in d_ifacea.h too !!! */
#define	TURB_CYCLE	128	/* turbulent cycle size */

#define TILE_SIZE	128	/* R_GenTurbTile{16}, R_GenSkyTile16 */

#define SKYSHIFT	7
#define	SKYSIZE		(1 << SKYSHIFT)
#define SKYMASK		(SKYSIZE - 1)

extern float skyspeed, skyspeed2;
extern float skytime;

extern int c_surf;
extern vrect_t scr_vrect;

extern byte *r_warpbuffer;

#endif /* D_IFACE_H */
