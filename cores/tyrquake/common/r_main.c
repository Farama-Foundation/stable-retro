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
/* r_main.c */

#include <stdint.h>

#include "cmd.h"
#include "console.h"
#include "perf_timing.h"
#include "quakedef.h"
#include "r_local.h"
#include "r_subdiv.h"
#include "screen.h"
#include "sound.h"
#include "sys.h"
#include "view.h"
#include "rhi.h"      /* g_rhi / g_rhi_compute_rendering /
                       * dispatch_3d_warp_screen -- when GPU warp is
                       * active R_RenderView calls it instead of
                       * D_WarpScreen (Phase 5b-03). */

void *colormap;
int r_numallocatededges;

qboolean r_recursiveaffinetriangles = true;

float r_aliasuvscale = 1.0;

static vec3_t viewlightvec;
static alight_t r_viewlighting = { 128, 192, viewlightvec };

qboolean r_dowarp, r_dowarpold, r_viewchanged;

mvertex_t *r_pcurrentvertbase;

int c_surf;
int r_maxsurfsseen, r_maxedgesseen;

static int r_cnumsurfs;
static qboolean r_surfsonstack;

byte *r_warpbuffer;
/* Warp buffer must be sized for the largest vid.height the
 * software renderer can drive, not WARP_HEIGHT, because the
 * rasterizers walk d_scantable[0..vid.height-1] regardless of
 * which buffer d_viewbuffer points at.  D_ViewChanged builds
 * d_scantable[i] = i * WARP_WIDTH when r_dowarp is true, but
 * iterates 0..vid.height-1.  At modern resolutions where
 * vid.height (capped at MAXHEIGHT) exceeds WARP_HEIGHT (720),
 * every frame the player spends in water/lava/slime stomps
 * (vid.height - WARP_HEIGHT) * WARP_WIDTH bytes of whatever
 * static data follows this buffer in .bss -- a class of heap-
 * stomp-shaped corruption masquerading as bogus pointers in
 * unrelated subsystems.  Sizing for MAXHEIGHT closes the
 * window cleanly; cost is ~600 KB extra .bss on top of the
 * existing ~900 KB allocation. */
static byte r_warpbuffer_storage[WARP_WIDTH * MAXHEIGHT];

static byte *r_stack_start;

entity_t r_worldentity;

/**/
/* view origin */
/**/
vec3_t vup, base_vup;
vec3_t vpn, base_vpn;
vec3_t vright, base_vright;
vec3_t r_origin;

/**/
/* screen size info */
/**/
refdef_t r_refdef;
float xcenter, ycenter;
float xscale, yscale;
float xscaleinv, yscaleinv;
float xscaleshrink, yscaleshrink;
float aliasxscale, aliasyscale, aliasxcenter, aliasycenter;

int screenwidth;

float pixelAspect;
static float screenAspect;
static float verticalFieldOfView;
static float xOrigin, yOrigin;

mplane_t screenedge[4];

/**/
/* refresh flags */
/**/
int r_framecount = 1;		/* so frame counts initialized to 0 don't match */
int r_visframecount;
int r_drawnpolycount;

/* Two-pass rendering for translucent liquids.
 *   r_renderpass = 0: single pass, vanilla rendering.  Used when
 *                     liquid blend is off or all alphas = 1.0.
 *   r_renderpass = 1: pass 1 of two-pass mode, opaque world only
 *                     (R_RenderFace skips SURF_DRAWTURB).
 *   r_renderpass = 2: pass 2 of two-pass mode, liquids only
 *                     (R_RenderFace skips non-SURF_DRAWTURB).
 * Set by R_EdgeDrawing, consulted by R_RenderFace, R_RenderBmodelFace,
 * R_EmitCachedEdge, D_DrawSurfaces, and D_DrawTurbulent8Span. */
int r_renderpass;

/* Set by R_RenderFace/R_RenderBmodelFace whenever a SURF_DRAWTURB
 * face is filtered out in pass 1.  R_EdgeDrawing checks this after
 * pass 1 and skips pass 2 entirely if no liquid was ever in view --
 * which is the common case for most rooms in most maps.  Saves the
 * full BSP walk + edge emit + scan that pass 2 would otherwise
 * perform. */
int r_renderpass_seen_liquid;

/* Set by R_MarkSurfaces during its PVS walk: true iff liquids are
 * configured translucent AND the camera's PVS contains a liquid
 * surface.  When false, R_MarkSurfaces skips the no-cull marking
 * (and the second efrag pass), and R_EdgeDrawing skips two-pass
 * rendering -- vanilla performance is restored even with liquid
 * blend on, as long as no liquid is in view. */
int r_nocull_active;

mleaf_t *r_viewleaf, *r_oldviewleaf;

texture_t *r_notexture_mip;

float r_aliastransition, r_resfudge;

int d_lightstylevalue[256];	/* 8.8 fraction of base light value */

cvar_t r_draworder = { "r_draworder", "0" };
cvar_t r_clearcolor = { "r_clearcolor", "2" };
cvar_t r_waterwarp = { "r_waterwarp", "1" };
cvar_t r_waterwarp_scale = { "r_waterwarp_scale", "0.5" };
cvar_t r_wateralpha   = { "r_wateralpha",   "1", true };
cvar_t r_lavaalpha    = { "r_lavaalpha",    "1", true };
cvar_t r_slimealpha   = { "r_slimealpha",   "1", true };
cvar_t r_telealpha    = { "r_telealpha",    "1", true };
cvar_t r_liquidblend  = { "r_liquidblend",  "0", true };
cvar_t r_phongshading = { "r_phongshading", "0", true };
cvar_t r_coloredlight = { "r_coloredlight", "0", true };
cvar_t r_lightdither  = { "r_lightdither",  "0", true };
cvar_t r_persistgibs  = { "r_persistgibs",  "0", true };
cvar_t r_shadows      = { "r_shadows",      "0", true };
/* Display aspect ratio selector.  Index into the table in
 * R_AspectRatioForIndex (libretro.c): 0 = 4:3 (vanilla), 1 = 16:9,
 * 2 = 16:10, 3 = 21:9, 4 = 32:9.  Archived.  The frontend display
 * geometry is repushed via RETRO_ENVIRONMENT_SET_GEOMETRY from the
 * cvar callback (R_AspectRatioChanged); the rendered framebuffer
 * resolution is unchanged -- only the aspect the frontend stretches
 * it to. */
cvar_t r_aspect       = { "r_aspect",       "0", true };
cvar_t r_drawentities = { "r_drawentities", "1" };
cvar_t r_drawviewmodel = { "r_drawviewmodel", "1" };
cvar_t r_ambient = { "r_ambient", "0" };
cvar_t r_numsurfs = { "r_numsurfs", "0" };
cvar_t r_numedges = { "r_numedges", "0" };

cvar_t r_lockpvs = { "r_lockpvs", "0" };
cvar_t r_lockfrustum = { "r_lockfrustum", "0" };

cvar_t r_fullbright = { "r_fullbright", "0" };

static cvar_t r_aliasstats = { "r_polymodelstats", "0" };
static cvar_t r_maxsurfs = { "r_maxsurfs", "0" };
static cvar_t r_maxedges = { "r_maxedges", "0" };
static cvar_t r_aliastransbase = { "r_aliastransbase", "200" };
static cvar_t r_aliastransadj = { "r_aliastransadj", "100" };

/*
 * r_phongshading callback: Phong shading itself is a per-pixel
 * rasterizer choice picked up automatically on the next frame, so
 * by itself the toggle needs no special handling.  But when
 * r_polysubdiv is also on, the same vertex normals drive Phong
 * tessellation at model-load time (see r_subdiv.c PhongMidpoint),
 * which means changing this flag changes the baked geometry.  In
 * that case we flush the alias cache so models reload with the
 * new midpoint positions.  When r_polysubdiv is 0 the geometry
 * doesn't depend on Phong shading and we leave the cache alone.
 */
static void
R_PhongShadingChanged(cvar_t *var)
{
    (void)var;
    if (R_PolySubdivPasses() > 0)
	Mod_FlushAlias();
}

/*
 * r_polysubdiv callback: clamp the user-entered value to the
 * supported range and force every cached alias model to reload, so
 * the new subdivision level takes effect for currently-visible
 * entities without waiting for cache pressure to evict them.
 *
 * The clamp is one-shot: if the user typed an out-of-range value at
 * the console (e.g. "r_polysubdiv 7"), we write back the clamped
 * value via Cvar_SetValue, which re-enters this callback.  We detect
 * the recursive entry by comparing the float value to the integer
 * round-trip and skip the flush the second time around, so a single
 * user-driven change produces a single Mod_FlushAlias call.
 */
static void
R_PolySubdivChanged(cvar_t *var)
{
    int v = (int)var->value;
    int clamped = v;
    if (clamped < 0)
	clamped = 0;
    if (clamped > R_POLYSUBDIV_MAX_PASSES)
	clamped = R_POLYSUBDIV_MAX_PASSES;
    if ((float)clamped != var->value) {
	Cvar_SetValue(var->name, (float)clamped);
	return;	/* re-entered through the Set; the second pass does the flush */
    }
    Mod_FlushAlias();
    Con_Printf("polygon subdivision: %d pass%s\n",
	       clamped, clamped == 1 ? "" : "es");
}

/*
==================
R_InitTextures
==================
*/
void
R_InitTextures(void)
{
    int x, y, m;
    byte *dest;

/* create a simple checkerboard texture for the default */
    r_notexture_mip = (texture_t*)Hunk_Alloc(sizeof(texture_t) + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2);

    r_notexture_mip->width = r_notexture_mip->height = 16;
    r_notexture_mip->offsets[0] = sizeof(texture_t);
    r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16 * 16;
    r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8 * 8;
    r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4 * 4;

    for (m = 0; m < 4; m++) {
	dest = (byte *)r_notexture_mip + r_notexture_mip->offsets[m];
	for (y = 0; y < (16 >> m); y++) {
	    for (x = 0; x < (16 >> m); x++) {
		if ((y < (8 >> m)) ^ (x < (8 >> m)))
		    *dest++ = 0;
		else
		    *dest++ = 0xff;
	    }
	}
    }
}


/*
================
R_InitTurb
================
*/
static void
R_InitTurb(void)
{
    int i;

    for (i = 0; i < TURB_TABLE_SIZE; ++i) {
	sintable[i] = TURB_SURF_AMP
	    + sinf((float)i * 3.14159f * 2.0f / TURB_CYCLE) * TURB_SURF_AMP;
	intsintable[i] = TURB_SCREEN_AMP
	    + sinf((float)i * 3.14159f * 2.0f / TURB_CYCLE) * TURB_SCREEN_AMP;
    }
}


/*
===============
R_Init
===============
*/
void
R_Init(void)
{
    int dummy;

    /* get stack position so we can guess if we are going to overflow */
    r_stack_start = (byte *)&dummy;

    R_InitTurb();

    Cvar_RegisterVariable(&r_draworder);
    Cvar_RegisterVariable(&r_clearcolor);
    Cvar_RegisterVariable(&r_waterwarp);
    Cvar_RegisterVariable(&r_waterwarp_scale);
    Cvar_RegisterVariable(&r_wateralpha);
    Cvar_RegisterVariable(&r_lavaalpha);
    Cvar_RegisterVariable(&r_slimealpha);
    Cvar_RegisterVariable(&r_telealpha);
    Cvar_RegisterVariable(&r_liquidblend);
    Cvar_RegisterVariable(&r_phongshading);
    Cvar_RegisterVariable(&r_coloredlight);
    Cvar_RegisterVariable(&r_lightdither);
    Cvar_RegisterVariable(&r_persistgibs);
    Cvar_RegisterVariable(&r_shadows);
    Cvar_RegisterVariable(&r_drawentities);
    Cvar_RegisterVariable(&r_drawviewmodel);
    Cvar_RegisterVariable(&r_ambient);
    Cvar_RegisterVariable(&r_numsurfs);
    Cvar_RegisterVariable(&r_numedges);
    Cvar_RegisterVariable(&r_lerpmodels);
    Cvar_RegisterVariable(&r_lerpmove);
    Cvar_RegisterVariable(&r_lockpvs);
    Cvar_RegisterVariable(&r_lockfrustum);

    /* perf_timing module owns its own cvar (r_perf), but
     * registers it from the same R_Init hook so all r_*
     * cvars come up together. */
    perf_timing_register_cvars();

    Cvar_RegisterVariable(&r_fullbright);

    Cvar_RegisterVariable(&r_aliasstats);
    Cvar_RegisterVariable(&r_maxsurfs);
    Cvar_RegisterVariable(&r_maxedges);
    Cvar_RegisterVariable(&r_aliastransbase);
    Cvar_RegisterVariable(&r_aliastransadj);

    Cvar_RegisterVariable(&r_polysubdiv);
    Cvar_SetCallback(&r_polysubdiv, R_PolySubdivChanged);
    Cvar_SetCallback(&r_phongshading, R_PhongShadingChanged);

    Cvar_RegisterVariable(&r_aspect);
    Cvar_SetCallback(&r_aspect, R_AspectRatioChanged);

    Cvar_SetValue("r_maxedges", (float)NUMSTACKEDGES);
    Cvar_SetValue("r_maxsurfs", (float)NUMSTACKSURFACES);

    view_clipplanes[0].leftedge = true;
    view_clipplanes[1].rightedge = true;
    view_clipplanes[1].leftedge = view_clipplanes[2].leftedge =
	view_clipplanes[3].leftedge = false;
    view_clipplanes[0].rightedge = view_clipplanes[2].rightedge =
	view_clipplanes[3].rightedge = false;

    r_refdef.xOrigin = XCENTERING;
    r_refdef.yOrigin = YCENTERING;

    R_InitParticles();

    D_Init();
}

/*
===============
R_LiquidAlphaForTexture

Returns the alpha value (0..1) for a liquid surface, picked based
on the texture name prefix.  Stock Quake liquid texture naming
is "*water*" for water, "*lava*" for lava, "*slime*" for slime,
"*tele*" for teleporter portals; everything else with a leading
'*' is treated as water by convention.

Returns 1.0 (fully opaque) for non-liquid textures or when the
relevant alpha cvar is at its default of 1.0 -- both cases
produce the original behaviour and let the caller skip any
transparency-related code paths.
===============
*/
float
R_LiquidAlphaForTexture(const texture_t *tex)
{
    const char *name;

    if (!tex || !tex->name[0] || tex->name[0] != '*')
	return 1.0f;
    name = tex->name + 1;     /* skip leading '*' */

    if (!strncmp(name, "lava",  4))  return r_lavaalpha.value;
    if (!strncmp(name, "slime", 5))  return r_slimealpha.value;
    if (!strncmp(name, "tele",  4))  return r_telealpha.value;
    return r_wateralpha.value;
}

extern void V_NewMap (void);

/*
===============
R_NewMap
===============
*/
void
R_NewMap(void)
{
    int i;

    memset(&r_worldentity, 0, sizeof(r_worldentity));
    r_worldentity.model = cl.worldmodel;

/* clear out efrags in case the level hasn't been reloaded */
/* FIXME: is this one short? */
    for (i = 0; i < cl.worldmodel->numleafs; i++)
	cl.worldmodel->leafs[i].efrags = NULL;

    r_viewleaf = NULL;
    R_ClearParticles();

    /*
     * Translucent-liquid no-cull rendering walks every non-solid leaf
     * in the frustum, not just the camera's PVS, which puts a much
     * larger surface and edge count into the buffers.  Bump the
     * runtime caps so bmodels (doors, pickups, platforms) don't get
     * silently dropped at the surface_p >= surf_max guard in
     * R_RenderFace/R_RenderBmodelFace when the buffers fill up
     * during pass 1.
     *
     * 8x because underwater views looking up into open air leaves
     * can emit very large numbers of surfaces -- the air leaf
     * geometry plus connected leaves through every door/window
     * within the frustum.  4x wasn't enough on some maps.
     *
     * Cost of the bigger allocation: ~1.3 MB extra hunk on map load
     * when liquid blend is on, zero when off.  Per-frame cost is
     * unchanged (the buffers aren't memset; they're populated from
     * surface 0 and we only iterate up to surface_p).
     *
     * The bump only takes effect on the next map load after the user
     * toggles the cvar -- mid-map toggling uses whatever was
     * allocated when the map loaded.  Acceptable since toggling is
     * a settings change, not a hot-path action.
     */
    {
	int requested_surfs = (int)r_maxsurfs.value;
	if ((int)r_liquidblend.value != 0) {
	    if (requested_surfs < NUMSTACKSURFACES * 8)
		requested_surfs = NUMSTACKSURFACES * 8;
	}
	r_cnumsurfs = qclamp(requested_surfs, MINSURFACES, MAXSURFACES);
    }
    if (r_cnumsurfs > NUMSTACKSURFACES) {
	surfaces = (surf_t*)Hunk_Alloc(r_cnumsurfs * sizeof(surf_t));
	surface_p = surfaces;
	surf_max = &surfaces[r_cnumsurfs];
	r_surfsonstack = false;
	/* surface 0 doesn't really exist; it's just a dummy because index 0 */
	/* is used to indicate no edge attached to surface */
	surfaces--;
    } else {
	r_surfsonstack = true;
    }

    r_maxedgesseen = 0;
    r_maxsurfsseen = 0;

    {
	int requested_edges = (int)r_maxedges.value;
	if ((int)r_liquidblend.value != 0) {
	    if (requested_edges < NUMSTACKEDGES * 8)
		requested_edges = NUMSTACKEDGES * 8;
	}
	r_numallocatededges = qclamp(requested_edges, MINEDGES, MAXEDGES);
    }
    if (r_numallocatededges <= NUMSTACKEDGES)
	auxedges = NULL;
    else
	auxedges = (edge_t*)Hunk_Alloc(r_numallocatededges * sizeof(edge_t));

    r_dowarpold = false;
    r_viewchanged = false;

    V_NewMap();
}


/*
===============
R_SetVrect
===============
*/
void
R_SetVrect(const vrect_t *pvrectin, vrect_t *pvrect, int lineadj)
{
    int h;
    float size;
    qboolean full;

    full = (scr_viewsize.value >= 120.0f);
    size = qmin(scr_viewsize.value, 100.0f);

    /* Hide the status bar during intermission */
    if (cl.intermission) {
	full = true;
	size = 100.0;
	lineadj = 0;
    }
    size /= 100.0;

    if (full)
	h = pvrectin->height;
    else
	h = pvrectin->height - lineadj;

    pvrect->width = pvrectin->width * size;
    if (pvrect->width < 96) {
	size = 96.0 / pvrectin->width;
	pvrect->width = 96;	/* min for icons */
    }
    pvrect->width &= ~7;

    pvrect->height = pvrectin->height * size;
    if (!full) {
	if (pvrect->height > pvrectin->height - lineadj)
	    pvrect->height = pvrectin->height - lineadj;
    } else if (pvrect->height > pvrectin->height)
	pvrect->height = pvrectin->height;
    pvrect->height &= ~1;

    pvrect->x = (pvrectin->width - pvrect->width) / 2;
    if (full)
	pvrect->y = 0;
    else
	pvrect->y = (h - pvrect->height) / 2;
}


/*
===============
R_ViewChanged

Called every time the vid structure or r_refdef changes.
Guaranteed to be called before the first refresh
===============
*/
void
R_ViewChanged(vrect_t *pvrect, int lineadj, float aspect)
{
    int i;
    float res_scale;

    /* Hor+ vertical-anchor compensation.
     *
     * SCR_CalcRefdef widens r_refdef.fov_x for non-4:3 aspect ratios
     * (see CalcFovHorPlus).  In this renderer the vertical scale is
     * yscale = (vrect.width / horizontalFieldOfView) * pixelAspect and
     * the vertical clip planes use verticalFieldOfView =
     * horizontalFieldOfView / (vrect.width * pixelAspect / height).
     * Widening fov_x alone (k = tan(fov_x/2) ratio) shrinks xscale by
     * k -- the intended horizontal expansion -- but also shrinks yscale
     * by k, zooming the view out vertically too.  That is not Hor+.
     *
     * Scaling pixelAspect by the same k cancels the vertical change
     * exactly: yscale becomes invariant and verticalFieldOfView
     * (which carries 1/pixelAspect) is likewise pinned, while xscale
     * keeps its 1/k horizontal widening.  k reduces to the selected
     * display ratio over the 4:3 baseline, so at 4:3 k == 1 and the
     * vanilla projection is reproduced bit-for-bit. */
    {
	int asp_idx = (int)r_aspect.value;
	if (asp_idx > 0) {
	    float k = R_AspectRatioForIndex(asp_idx) / (4.0f / 3.0f);
	    aspect *= k;
	}
    }

    r_viewchanged = true;

    R_SetVrect(pvrect, &r_refdef.vrect, lineadj);

    r_refdef.horizontalFieldOfView = 2.0f * tanf(r_refdef.fov_x / 360.0f * (float)M_PI);
    r_refdef.fvrectx = (float)r_refdef.vrect.x;
    r_refdef.fvrectx_adj = (float)r_refdef.vrect.x - 0.5;
    r_refdef.vrect_x_adj_shift20 = (r_refdef.vrect.x << 20) + (1 << 19) - 1;
    r_refdef.fvrecty = (float)r_refdef.vrect.y;
    r_refdef.fvrecty_adj = (float)r_refdef.vrect.y - 0.5;
    r_refdef.vrectright = r_refdef.vrect.x + r_refdef.vrect.width;
    r_refdef.vrectright_adj_shift20 =
	(r_refdef.vrectright << 20) + (1 << 19) - 1;
    r_refdef.fvrectright = (float)r_refdef.vrectright;
    r_refdef.fvrectright_adj = (float)r_refdef.vrectright - 0.5;
    r_refdef.vrectrightedge = (float)r_refdef.vrectright - 0.99;
    r_refdef.vrectbottom = r_refdef.vrect.y + r_refdef.vrect.height;
    r_refdef.fvrectbottom = (float)r_refdef.vrectbottom;
    r_refdef.fvrectbottom_adj = (float)r_refdef.vrectbottom - 0.5;

    r_refdef.aliasvrect.x = (int)(r_refdef.vrect.x * r_aliasuvscale);
    r_refdef.aliasvrect.y = (int)(r_refdef.vrect.y * r_aliasuvscale);
    r_refdef.aliasvrect.width = (int)(r_refdef.vrect.width * r_aliasuvscale);
    r_refdef.aliasvrect.height =
	(int)(r_refdef.vrect.height * r_aliasuvscale);
    r_refdef.aliasvrectright =
	r_refdef.aliasvrect.x + r_refdef.aliasvrect.width;
    r_refdef.aliasvrectbottom =
	r_refdef.aliasvrect.y + r_refdef.aliasvrect.height;

    pixelAspect = aspect;
    xOrigin = r_refdef.xOrigin;
    yOrigin = r_refdef.yOrigin;

    screenAspect = r_refdef.vrect.width * pixelAspect / r_refdef.vrect.height;
/* 320*200 1.0 pixelAspect = 1.6 screenAspect */
/* 320*240 1.0 pixelAspect = 1.3333 screenAspect */
/* proper 320*200 pixelAspect = 0.8333333 */

    verticalFieldOfView = r_refdef.horizontalFieldOfView / screenAspect;

/* values for perspective projection */
/* if math were exact, the values would range from 0.5 to to range+0.5 */
/* hopefully they wll be in the 0.000001 to range+.999999 and truncate */
/* the polygon rasterization will never render in the first row or column */
/* but will definately render in the [range] row and column, so adjust the */
/* buffer origin to get an exact edge to edge fill */
    xcenter = ((float)r_refdef.vrect.width * XCENTERING) +
	r_refdef.vrect.x - 0.5;
    aliasxcenter = xcenter * r_aliasuvscale;
    ycenter = ((float)r_refdef.vrect.height * YCENTERING) +
	r_refdef.vrect.y - 0.5;
    aliasycenter = ycenter * r_aliasuvscale;

    xscale = r_refdef.vrect.width / r_refdef.horizontalFieldOfView;
    aliasxscale = xscale * r_aliasuvscale;
    xscaleinv = 1.0 / xscale;
    yscale = xscale * pixelAspect;
    aliasyscale = yscale * r_aliasuvscale;
    yscaleinv = 1.0 / yscale;
    xscaleshrink =
	(r_refdef.vrect.width - 6) / r_refdef.horizontalFieldOfView;
    yscaleshrink = xscaleshrink * pixelAspect;

/* left side clip */
    screenedge[0].normal[0] =
	-1.0 / (xOrigin * r_refdef.horizontalFieldOfView);
    screenedge[0].normal[1] = 0;
    screenedge[0].normal[2] = 1;
    screenedge[0].type = PLANE_ANYZ;

/* right side clip */
    screenedge[1].normal[0] =
	1.0 / ((1.0 - xOrigin) * r_refdef.horizontalFieldOfView);
    screenedge[1].normal[1] = 0;
    screenedge[1].normal[2] = 1;
    screenedge[1].type = PLANE_ANYZ;

/* top side clip */
    screenedge[2].normal[0] = 0;
    screenedge[2].normal[1] = -1.0 / (yOrigin * verticalFieldOfView);
    screenedge[2].normal[2] = 1;
    screenedge[2].type = PLANE_ANYZ;

/* bottom side clip */
    screenedge[3].normal[0] = 0;
    screenedge[3].normal[1] = 1.0 / ((1.0 - yOrigin) * verticalFieldOfView);
    screenedge[3].normal[2] = 1;
    screenedge[3].type = PLANE_ANYZ;

    for (i = 0; i < 4; i++)
	VectorNormalize(screenedge[i].normal);

    res_scale =
	sqrt((double)(r_refdef.vrect.width * r_refdef.vrect.height) /
	     (320.0 * 152.0)) * (2.0 / r_refdef.horizontalFieldOfView);
    r_aliastransition = r_aliastransbase.value * res_scale;
    r_resfudge = r_aliastransadj.value * res_scale;

    D_ViewChanged();
}


/*
===============
R_LiquidsAreTransparent

Returns true iff at least one liquid type is currently rendered
non-opaque -- i.e. r_liquidblend selects an actual blend mode AND
at least one of the per-liquid alpha cvars is below 1.0.  Used to
gate the PVS-through-liquid extension below: when nothing is
transparent there's nothing to see through, so the extension's
extra leaf-marking work isn't worth the cost.
===============
*/
qboolean
R_LiquidsAreTransparent(void)
{
    if ((int)r_liquidblend.value == 0)
	return false;
    if (r_wateralpha.value < 1.0f) return true;
    if (r_lavaalpha.value  < 1.0f) return true;
    if (r_slimealpha.value < 1.0f) return true;
    if (r_telealpha.value  < 1.0f) return true;
    return false;
}

/*
===============
R_MarkSurfaces
===============
*/
static void
R_MarkSurfaces(void)
{
    const leafbits_t *pvs;
    leafblock_t check;
    int leafnum, i;
    mleaf_t *leaf;
    mnode_t *node;
    msurface_t **mark;
    qboolean pvs_changed;

    /*
     * If the PVS hasn't changed, no need to update bsp visframes,
     * just store the efrags.
     */
    pvs_changed = (r_viewleaf != r_oldviewleaf && !r_lockpvs.value);
    if (pvs_changed) {
	r_visframecount++;
	r_oldviewleaf = r_viewleaf;
    }

    pvs = Mod_LeafPVS(cl.worldmodel, r_viewleaf);
    {
	qboolean pvs_has_liquid = false;
	qboolean want_nocull = R_LiquidsAreTransparent();

	foreach_leafbit(pvs, leafnum, check) {
	    leaf = &cl.worldmodel->leafs[leafnum + 1];
	    if (leaf->efrags)
		R_StoreEfrags(&leaf->efrags);

	    /* Detect liquid surfaces in the camera's PVS so we can
	     * skip the expensive no-cull marking + two-pass rendering
	     * when no liquid is visible from here.  Cheap inline scan
	     * during the PVS walk we'd be doing anyway. */
	    if (want_nocull && !pvs_has_liquid) {
		mark = leaf->firstmarksurface;
		for (i = 0; i < leaf->nummarksurfaces; i++) {
		    if ((*mark)->flags & SURF_DRAWTURB) {
			pvs_has_liquid = true;
			break;
		    }
		    mark++;
		}
	    }

	    if (!pvs_changed)
		continue;

	    /* Mark the surfaces */
	    mark = leaf->firstmarksurface;
	    for (i = 0; i < leaf->nummarksurfaces; i++) {
		(*mark)->visframe = r_visframecount;
		mark++;
	    }

	    /* Mark the leaf and all parent nodes */
	    node = (mnode_t *)leaf;
	    do {
		if (node->visframe == r_visframecount)
		    break;
		node->visframe = r_visframecount;
		node = node->parent;
	    } while (node);
	}

	/* Cache the decision for R_EdgeDrawing -- the no-cull marks
	 * (below) and pass 2 only run when liquid is in the PVS. */
	r_nocull_active = (want_nocull && pvs_has_liquid);
    }

    /*
     * Translucent-liquid no-cull pass.  When any liquid alpha < 1.0
     * AND there's a liquid surface in the camera's PVS, we extend
     * visibility to every non-solid leaf so geometry behind the
     * translucent water/lava/slime is rendered.  PVS-based culling
     * would normally treat liquids as opaque and stop visibility
     * there; rather than trying to selectively patch the PVS (which
     * causes severe BSP-edge-sort artefacts -- earlier iterations
     * of this code spent many rounds trying), we mark every
     * non-solid leaf visible and let frustum culling reject what's
     * outside the view.
     *
     * Per-pixel z-test in D_DrawTurbulent8Span gates the liquid
     * surface so it doesn't smear over foreground walls.
     *
     * No-op when liquid blend is off, OR when no liquid is in the
     * camera's PVS -- in which case vanilla single-pass rendering
     * runs normally.
     */
    if (r_nocull_active && pvs_changed) {
	mleaf_t *leaf_base = cl.worldmodel->leafs + 1;
	int numleafs = cl.worldmodel->numleafs;
	for (i = 0; i < numleafs; i++) {
	    leaf = leaf_base + i;
	    if (leaf->contents == CONTENTS_SOLID)
		continue;

	    /* Mark the surfaces */
	    mark = leaf->firstmarksurface;
	    {
		int j;
		for (j = 0; j < leaf->nummarksurfaces; j++) {
		    (*mark)->visframe = r_visframecount;
		    mark++;
		}
	    }

	    /* Mark the leaf and all parent nodes (walk stops as soon
	     * as we hit a node already marked this frame, so this is
	     * cheap on average). */
	    node = (mnode_t *)leaf;
	    do {
		if (node->visframe == r_visframecount)
		    break;
		node->visframe = r_visframecount;
		node = node->parent;
	    } while (node);
	}
    }

    /*
     * R_StoreEfrags also runs for every non-PVS leaf when no-cull
     * is active so static entities (monsters, ammo, health) in
     * leaves outside the camera's PVS but visible through the
     * translucent water enter cl_visedicts.  Without this, an air
     * leaf above the camera (when underwater) would render its
     * world surfaces but its statics would silently vanish.
     *
     * Gated on r_nocull_active so we skip the per-leaf scan when
     * there's no liquid in view (vanilla performance preserved).
     */
    if (r_nocull_active) {
	mleaf_t *leaf_base = cl.worldmodel->leafs + 1;
	int numleafs = cl.worldmodel->numleafs;
	for (i = 0; i < numleafs; i++) {
	    leaf = leaf_base + i;
	    if (leaf->contents == CONTENTS_SOLID)
		continue;
	    if (Mod_TestLeafBit(pvs, i))
		continue; /* already R_StoreEfrags'd in main pass */
	    if (leaf->efrags)
		R_StoreEfrags(&leaf->efrags);
	}
    }
}

/*
=============
R_CullSurfaces
=============
*/
static void
R_CullSurfaces(model_t *model, vec3_t vieworg)
{
    int i, j;
    int side;
    mnode_t *node;
    msurface_t *surf;
    mplane_t *plane;
    vec_t dist;

    node = model->nodes;
    node->clipflags = 15;

    for (;;) {
	if (node->visframe != r_visframecount)
	    goto NodeUp;

	if (node->clipflags) {
	    /* Clip the node against the frustum */
	    for (i = 0; i < 4; i++) {
		if (!(node->clipflags & (1 << i)))
		    continue;
		plane = &view_clipplanes[i].plane;
		side = BoxOnPlaneSide(node->mins, node->maxs, plane);
		if (side == PSIDE_BACK) {
		    node->clipflags = BMODEL_FULLY_CLIPPED;
		    goto NodeUp;
		}
		if (side == PSIDE_FRONT)
		    node->clipflags &= ~(1 << i);
	    }
	}

	if (node->contents < 0)
	    goto NodeUp;

	surf = model->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++) {
	    /* Clip the surfaces against the frustum */
	    surf->clipflags = node->clipflags;
	    for (j = 0; j < 4; j++) {
		if (!(node->clipflags & (1 << j)))
		    continue;
		plane = &view_clipplanes[j].plane;
		side = BoxOnPlaneSide(surf->mins, surf->maxs, plane);
		if (side == PSIDE_BACK) {
		    surf->clipflags = BMODEL_FULLY_CLIPPED;
		    break;
		}
		if (side == PSIDE_FRONT)
		    surf->clipflags &= ~(1 << j);
	    }
	    if (j < 4)
		continue;

	    /* Cull backward facing surfs */
	    if (surf->plane->type < 3) {
		dist = vieworg[surf->plane->type] - surf->plane->dist;
	    } else {
		dist = DotProduct(vieworg, surf->plane->normal);
		dist -= surf->plane->dist;
	    }
	    if (surf->flags & SURF_PLANEBACK) {
		if (dist > -BACKFACE_EPSILON)
		    surf->clipflags = BMODEL_FULLY_CLIPPED;
	    } else {
		if (dist < BACKFACE_EPSILON)
		    surf->clipflags = BMODEL_FULLY_CLIPPED;
	    }
	}

/*  DownLeft: */
	/* Don't descend into solid leafs because parent links are broken */
	if (node->children[0]->contents == CONTENTS_SOLID)
	    goto DownRight;
	node->children[0]->clipflags = node->clipflags;
	node = node->children[0];
	continue;

    DownRight:
	/* Don't descent into solid leafs because parent links are broken */
	if (node->children[1]->contents == CONTENTS_SOLID)
	    goto NodeUp;
	node->children[1]->clipflags = node->clipflags;
	node = node->children[1];
	continue;

    NodeUp:
	/* If we just processed the left node, go right */
	if (node->parent && node == node->parent->children[0]) {
	    node = node->parent;
	    goto DownRight;
	}

	/* If we were on a right branch, backtrack */
	while (node->parent && node == node->parent->children[1])
	    node = node->parent;

	/* If we still have a parent, we need to cross to the right */
	if (node->parent) {
	    node = node->parent;
	    goto DownRight;
	}

	/* If no more parents, we are done */
	break;
    }
}

/*
=============
R_CullSubmodelSurfaces
=============
*/
static void
R_CullSubmodelSurfaces(const model_t *submodel, const vec3_t vieworg,
		       int clipflags)
{
    int i, j, side;
    msurface_t *surf;
    mplane_t *plane;
    vec_t dist;

    surf = submodel->surfaces + submodel->firstmodelsurface;
    for (i = 0; i < submodel->nummodelsurfaces; i++, surf++) {
	/* Clip the surface against the frustum */
	surf->clipflags = clipflags;
	for (j = 0; j < 4; j++) {
	    if (!(surf->clipflags & (1 << j)))
		continue;
	    plane = &view_clipplanes[j].plane;
	    side = BoxOnPlaneSide(surf->mins, surf->maxs, plane);
	    if (side == PSIDE_BACK) {
		surf->clipflags = BMODEL_FULLY_CLIPPED;
		break;
	    }
	    if (side == PSIDE_FRONT)
		surf->clipflags &= ~(1 << j);
	}
	if (j < 4)
	    continue;

	/* Cull backward facing surfs */
	if (surf->plane->type < 3) {
	    dist = vieworg[surf->plane->type] - surf->plane->dist;
	} else {
	    dist = DotProduct(vieworg, surf->plane->normal);
	    dist -= surf->plane->dist;
	}
	if (surf->flags & SURF_PLANEBACK) {
	    if (dist > -BACKFACE_EPSILON)
		surf->clipflags = BMODEL_FULLY_CLIPPED;
	} else {
	    if (dist < BACKFACE_EPSILON)
		surf->clipflags = BMODEL_FULLY_CLIPPED;
	}
    }
}

/*
=============
R_DrawEntitiesOnList
=============
*/
static void
R_DrawEntitiesOnList(void)
{
    entity_t *e;
    int i, j;
    int lnum;
    alight_t lighting;

/* FIXME: remove and do real lighting */
    float lightvec[3] = { -1, 0, 0 };
    vec3_t dist;
    float add;

    if (!r_drawentities.value)
	return;

    for (i = 0; i < cl_numvisedicts; i++) {
	e = &cl_visedicts[i];
	if (e == &cl_entities[cl.viewentity])
	    continue;		/* don't draw the player */
	switch (e->model->type) {
	case mod_sprite:
	    VectorCopy(e->origin, r_entorigin);
	    VectorSubtract(r_origin, r_entorigin, modelorg);
	    R_DrawSprite(e);
	    break;

	case mod_alias:
	    if (r_lerpmove.value && e->previousorigintime > 0.0f) {
		/* Countdown semantics: previousorigintime is the duration
		 * of the prior snapshot interval (zero = no history, snap
		 * instead of lerp), currentorigintime is the elapsed time
		 * since the latest. */
		float delta = e->previousorigintime;
		float frac = qclamp(e->currentorigintime / delta, 0.0, 1.0);
		vec3_t lerpvec;

		/* FIXME - hack to skip the viewent (weapon) */
		if (e == &cl.viewent)
		    goto nolerp;

		VectorSubtract(e->currentorigin, e->previousorigin, lerpvec);
		VectorMA(e->previousorigin, frac, lerpvec, r_entorigin);
	    } else
	    nolerp:
	    {
		VectorCopy(e->origin, r_entorigin);
	    }
	    VectorSubtract(r_origin, r_entorigin, modelorg);

	    /* see if the bounding box lets us trivially reject, also sets */
	    /* trivial accept status */
	    if (R_AliasCheckBBox(e)) {
		j = R_LightPoint(e->origin);

		lighting.ambientlight = j;
		lighting.shadelight = j;

		lighting.plightvec = lightvec;

		for (lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
		    if (cl_dlights[lnum].die > 0) {
			VectorSubtract(e->origin, cl_dlights[lnum].origin,
				       dist);
			add = cl_dlights[lnum].radius - Length(dist);

			if (add > 0)
			    lighting.ambientlight += add;
		    }
		}

		/* clamp lighting so it doesn't overbright as much */
		if (lighting.ambientlight > 128)
		    lighting.ambientlight = 128;
		if (lighting.ambientlight + lighting.shadelight > 192)
		    lighting.shadelight = 192 - lighting.ambientlight;

		/* Cast a flat shadow on the floor below the entity, glquake
		 * r_shadows-style.  Drawn BEFORE the model so that the model's
		 * z-write naturally covers the shadow at any pixels where they
		 * overlap.  Drawing after-the-model would require z biasing the
		 * shadow above the floor by enough to clear any model vert that
		 * touches the floor -- which would also cause the shadow to
		 * draw OVER those low model verts.  Before-the-model side-steps
		 * the issue. */
		if (r_shadows.value != 0.0f)
		    R_AliasDrawShadow(e);

		R_AliasDrawModel(e, &lighting);
	    }
	    break;

	default:
	    break;
	}
    }
}

/*
=============
R_DrawViewModel
=============
*/
static void
R_DrawViewModel(void)
{
    entity_t *e;
/* FIXME: remove and do real lighting */
    float lightvec[3] = { -1, 0, 0 };
    int j;
    int lnum;
    vec3_t dist;
    float add;
    dlight_t *dl;

    if (!r_drawviewmodel.value || chase_active.value)
	return;

    if (cl.stats[STAT_ITEMS] & IT_INVISIBILITY)
	return;

    if (cl.stats[STAT_HEALTH] <= 0)
	return;

    e = &cl.viewent;
    if (!e->model)
	return;

    VectorCopy(e->origin, r_entorigin);
    VectorSubtract(r_origin, r_entorigin, modelorg);

    VectorCopy(vup, viewlightvec);
    VectorInverse(viewlightvec);

    j = R_LightPoint(e->origin);

    if (j < 24)
	j = 24;			/* allways give some light on gun */
    r_viewlighting.ambientlight = j;
    r_viewlighting.shadelight = j;

/* add dynamic lights */
    for (lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
	dl = &cl_dlights[lnum];
	if (!dl->radius)
	    continue;
	if (!dl->radius)
	    continue;
	if (dl->die <= 0)
	    continue;

	VectorSubtract(e->origin, dl->origin, dist);
	add = dl->radius - Length(dist);
	if (add > 0)
	    r_viewlighting.ambientlight += add;
    }

/* clamp lighting so it doesn't overbright as much */
    if (r_viewlighting.ambientlight > 128)
	r_viewlighting.ambientlight = 128;
    if (r_viewlighting.ambientlight + r_viewlighting.shadelight > 192)
	r_viewlighting.shadelight = 192 - r_viewlighting.ambientlight;

    r_viewlighting.plightvec = lightvec;

    R_AliasDrawModel(e, &r_viewlighting);
}


/*
=============
R_BmodelCheckBBox
=============
*/
static int
R_BmodelCheckBBox(const entity_t *e, model_t *clmodel,
		  const vec3_t mins, const vec3_t maxs)
{
    int i, side, clipflags;
    vec_t d;

    clipflags = 0;

    if (e->angles[0] || e->angles[1] || e->angles[2]) {
	for (i = 0; i < 4; i++) {
	    d = DotProduct(e->origin, view_clipplanes[i].plane.normal);
	    d -= view_clipplanes[i].plane.dist;

	    if (d <= -clmodel->radius)
		return BMODEL_FULLY_CLIPPED;

	    if (d <= clmodel->radius)
		clipflags |= (1 << i);
	}
    } else {
	for (i = 0; i < 4; i++) {
	    side = BoxOnPlaneSide(mins, maxs, &view_clipplanes[i].plane);
	    if (side == PSIDE_BACK)
		return BMODEL_FULLY_CLIPPED;
	    if (side == PSIDE_BOTH)
		clipflags |= (1 << i);
	}
    }

    return clipflags;
}


/*
=============
R_DrawBEntitiesOnList
=============
*/
static void R_DrawBEntitiesOnList(void)
{
    entity_t *e;
    int i, clipflags;
    vec3_t oldorigin;
    model_t *model;
    vec3_t mins, maxs;

    if (!r_drawentities.value)
	return;

    VectorCopy(modelorg, oldorigin);
    insubmodel = true;

    for (i = 0; i < cl_numvisedicts; i++) {
	e = &cl_visedicts[i];
	if (e->model->type != mod_brush)
	    continue;

	model = e->model;

	/* see if the bounding box lets us trivially reject, also sets */
	/* trivial accept status */
	VectorAdd(e->origin, model->mins, mins);
	VectorAdd(e->origin, model->maxs, maxs);
	clipflags = R_BmodelCheckBBox(e, model, mins, maxs);

	if (clipflags == BMODEL_FULLY_CLIPPED)
	    continue;

	VectorCopy(e->origin, r_entorigin);
	VectorSubtract(r_origin, r_entorigin, modelorg);
	r_pcurrentvertbase = model->vertexes;

	/* FIXME: stop transforming twice */
	R_RotateBmodel(e);

	/* calculate dynamic lighting for bmodel if it's not an */
	/* instanced model */
	if (model->firstmodelsurface != 0)
       R_PushDlights (model->nodes + model->hulls[0].firstclipnode);  /*qbism - from MH */

	r_pefragtopnode = NULL;
	VectorCopy(mins, r_emins);
	VectorCopy(maxs, r_emaxs);
	R_SplitEntityOnNode2(cl.worldmodel->nodes);
	R_CullSubmodelSurfaces(model, modelorg, clipflags);

	if (r_pefragtopnode) {
	    e->topnode = r_pefragtopnode;

	    if (r_pefragtopnode->contents >= 0) {
		/* not a leaf; has to be clipped to the world BSP */
		R_DrawSolidClippedSubmodelPolygons(e, model);
	    } else {
		/* falls entirely in one leaf, so we just put all */
		/* the edges in the edge list and let 1/z sorting */
		/* handle drawing order */
		R_DrawSubmodelPolygons(e, model, clipflags);
	    }
	    e->topnode = NULL;
	}

	/* put back world rotation and frustum clipping */
	/* FIXME: R_RotateBmodel should just work off base_vxx */
	VectorCopy(base_vpn, vpn);
	VectorCopy(base_vup, vup);
	VectorCopy(base_vright, vright);
	VectorCopy(base_modelorg, modelorg);
	VectorCopy(oldorigin, modelorg);
	R_TransformFrustum();
    }

    insubmodel = false;
}


/*
================
R_EdgeDrawing
================
*/
static void R_EdgeDrawing(void)
{
   /* These two buffers are sized purely from the build-time
    * NUMSTACKEDGES / NUMSTACKSURFACES constants and don't change at
    * runtime, so allocate once on first call. Removes two malloc/free
    * pairs per rendered frame. Same shape as the R_ScanEdges fix. */
   static edge_t *ledges;
   static surf_t *lsurfs;
   if (!ledges)
      ledges = malloc(sizeof(edge_t)*CACHE_PAD_ARRAY(NUMSTACKEDGES, edge_t));
   if (!lsurfs)
      lsurfs = malloc(sizeof(surf_t)*CACHE_PAD_ARRAY(NUMSTACKSURFACES, surf_t));

   if (auxedges) {
      r_edges = auxedges;
   } else {
      r_edges =  (edge_t *)(((uintptr_t)&ledges[0] + CACHE_SIZE - 1) & ~(uintptr_t)(CACHE_SIZE - 1));
   }

   if (r_surfsonstack) {
      surfaces =  (surf_t *)(((uintptr_t)&lsurfs[0] + CACHE_SIZE - 1) & ~(uintptr_t)(CACHE_SIZE - 1));
      surf_max = &surfaces[r_cnumsurfs];
      /* surface 0 doesn't really exist; it's just a dummy because index 0 */
      /* is used to indicate no edge attached to surface */
      surfaces--;
   }

   /* Two-pass rendering when liquids are translucent:
    *   pass 1: opaque world only (skip SURF_DRAWTURB).  Because
    *           R_MarkSurfaces marked every non-solid leaf visible
    *           when liquids translucent, the edge list contains the
    *           full frustum-bounded set of opaque surfaces -- not
    *           just those in the camera's PVS -- so geometry behind
    *           translucent liquid is properly rendered.
    *   pass 2: liquids only.  Edge rasterizer rebuilds the edge list
    *           with only liquid surfaces and Turbulent8's stipple
    *           path paints them with per-pixel z-test on top of the
    *           pass 1 framebuffer; the dither holes leave the
    *           underwater geometry visible.
    *
    * Single-pass mode (r_renderpass = 0, the common case with no
    * liquid translucency) does not run pass 2 at all, R_RenderFace
    * doesn't filter, and rendering matches stock tyrquake. */
   /* Two-pass mode is gated by r_nocull_active, set by
    * R_MarkSurfaces.  When no liquid is in the camera's PVS, we
    * fall through to single-pass vanilla rendering even with
    * liquid blend on -- saves the second BSP walk for the common
    * case of being in a pure-land room. */
   if (r_nocull_active) {
      r_renderpass = 1;
      r_renderpass_seen_liquid = 0;
   } else {
      r_renderpass = 0;
   }

   R_BeginEdgeFrame();

   R_RenderWorld();

   /* only the world can be drawn back to front with no z reads or compares, */
   /* just z writes, so have the driver turn z compares on now */
   D_TurnZOn();

   R_DrawBEntitiesOnList();

   R_ScanEdges();

   /* Pass 2 only runs if pass 1 saw a liquid surface.  Most rooms
    * in most maps don't have liquid in view, so this skips the
    * second BSP walk + edge emit + scan when there's nothing to
    * stipple.  Big perf win on land-only sections of e1m1, e1m4,
    * e2m2, etc. */
   if (r_renderpass == 1 && r_renderpass_seen_liquid) {
      r_renderpass = 2;
      R_BeginEdgeFrame();
      R_RenderWorld();
      R_DrawBEntitiesOnList();
      R_ScanEdges();
   }
   r_renderpass = 0;
}


/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void
R_RenderView(void)
{
    r_warpbuffer = r_warpbuffer_storage;

    R_SetupFrame();
    R_PushDlights (cl.worldmodel->nodes);  /* qbism - moved here from view.c */
    R_MarkSurfaces();		/* done here so we know if we're in water */
    R_CullSurfaces(r_worldentity.model, r_refdef.vieworg);

    if (!r_worldentity.model || !cl.worldmodel)
	Sys_Error("%s: NULL worldmodel", __func__);

    R_EdgeDrawing();

    R_DrawEntitiesOnList();

    R_DrawViewModel();

    R_DrawParticles();

    if (r_dowarp)
	D_WarpScreen();
    else if (g_rhi && g_rhi->dispatch_3d_warp_screen
                   && g_rhi_compute_rendering
                   && r_waterwarp.value
                   && (r_viewleaf->contents <= CONTENTS_WATER))
	/* Phase 5b-03: GPU compute warp.  R_SetupFrame
	 * suppressed r_dowarp so the SW raster ran at full
	 * resolution into vid.buffer; the backend takes
	 * over here, snapshotting cl.time / scr_vrect into
	 * its push-constant block and dispatching a fused
	 * warp + palette compute pass that replaces this
	 * frame's regular palette dispatch. */
	g_rhi->dispatch_3d_warp_screen();

    V_SetContentsColor(r_viewleaf->contents);

    if (r_aliasstats.value)
	R_PrintAliasStats();

    /* Historical R_RenderView previously wrapped this body
     * with three alignment guards (Hunk_LowMark()&3,
     * &dummy_local&3, &r_warpbuffer&3) and Sys_Error'd on
     * any failure.  All three are vestigial DOS/Win16
     * paranoia: Hunk_Alloc rounds every allocation to 16
     * bytes ((size+15)&~15 in Hunk_Alloc) so Hunk_LowMark
     * is structurally 16-byte aligned, and modern x86/x64
     * ABIs guarantee >=4-byte alignment for stack ints
     * and globals of pointer type.  The checks fired
     * during the brief window when Sys_Error longjmp'd
     * (commit c992cb3) and broke the renderer entirely;
     * the longjmp was reverted in 3301dcf, but the
     * checks themselves are noise and have been removed. */
}
