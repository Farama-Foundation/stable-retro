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
/* r_misc.c */

#include "console.h"
#include "draw.h"
#include "menu.h"
#include "quakedef.h"
#include "r_local.h"
#include "render.h"
#include "sbar.h"
#include "host.h"
#include "server.h"
#include "sys.h"
#include "rhi.h"          /* g_rhi / g_rhi_compute_rendering -- when the
                           * active backend exposes a GPU warp dispatch
                           * and compute rendering is enabled,
                           * R_SetupFrame suppresses the SW r_dowarp
                           * flag so the SW raster doesn't render to
                           * r_warpbuffer at downscaled resolution; the
                           * backend handles the warp at full
                           * resolution from vk_texture (Phase 5b-03). */

/*
===============
R_CheckVariables
===============
*/
static void
R_CheckVariables(void)
{
    /* FIXME - do it right (cvar callback) */
    static float oldbright;

    if (r_fullbright.value != oldbright) {
	oldbright = r_fullbright.value;
	D_FlushCaches();	/* so all lighting changes */
    }
}

/*
=============
R_PrintAliasStats
=============
*/
void
R_PrintAliasStats(void)
{
    Con_Printf("%3i polygon model drawn\n", r_amodels_drawn);
}

/*
===================
R_TransformFrustum
===================
*/
void
R_TransformFrustum(void)
{
    int i;
    vec3_t v, v2;
    mplane_t *plane;

    if (r_lockfrustum.value)
	return;

    for (i = 0; i < 4; i++) {
	v[0] = screenedge[i].normal[2];
	v[1] = -screenedge[i].normal[0];
	v[2] = screenedge[i].normal[1];

	v2[0] = v[1] * vright[0] + v[2] * vup[0] + v[0] * vpn[0];
	v2[1] = v[1] * vright[1] + v[2] * vup[1] + v[0] * vpn[1];
	v2[2] = v[1] * vright[2] + v[2] * vup[2] + v[0] * vpn[2];

	plane = &view_clipplanes[i].plane;
	VectorCopy(v2, plane->normal);
	plane->dist = DotProduct(modelorg, v2);
	plane->signbits = SignbitsForPlane(plane);
    }
}

/*
================
TransformVector
================
*/
void
TransformVector(vec3_t in, vec3_t out)
{
    out[0] = DotProduct(in, vright);
    out[1] = DotProduct(in, vup);
    out[2] = DotProduct(in, vpn);
}

/*
================
R_TransformPlane
================
*/
void
R_TransformPlane(mplane_t *p, float *normal, float *dist)
{
    float d;

    d = DotProduct(r_origin, p->normal);
    *dist = p->dist - d;
/* TODO: when we have rotating entities, this will need to use the view matrix */
    TransformVector(p->normal, normal);
}

/*
===============
R_PrepareFrame

Compute the per-frame camera basis vectors (r_origin and the
vpn/vright/vup orthonormal triad) from the refdef.  These four
globals are read by non-renderer code that runs every frame
regardless of which backend is active -- in particular, S_Update
in libretro.c hands them to the audio mixer as the listener's
position and orientation for 3D spatialization.

Historically these assignments lived inside R_SetupFrame, which
runs only as part of the SW backend's R_RenderView pipeline.
With the RHI in place, R_RenderView is bypassed whenever a HW
backend is active (Vulkan, future GL/D3D11/D3D12) -- the
backend's draw_view does its own thing and the four globals
freeze at whatever values the last SW frame computed.  Audio
spatializes against a frozen listener, sounds get the wrong
stereo pan, and 3D sounds outside the frozen hearing range
drop out of the mix entirely.  The demo-loop case (which is
deterministic player input) makes the divergence audible: the
SW path's audio differs from any HW backend's audio for the
same recorded inputs.

Fix: split the assignments into this function, which V_RenderView
calls before dispatching to g_rhi->draw_view.  Every backend now
sees up-to-date listener state regardless of what draw_view does
internally.  R_SetupFrame (called from R_RenderView, i.e. SW
backend only) delegates to R_PrepareFrame so the SW path keeps
the same effective behaviour without code duplication.
===============
*/
void
R_PrepareFrame(void)
{
    /* Listener / camera position. */
    VectorCopy(r_refdef.vieworg, r_origin);

    /* Camera basis vectors.  AngleVectors writes the
     * orthonormal triad (forward = vpn, right = vright,
     * up = vup) from the Euler angles. */
    AngleVectors(r_refdef.viewangles, vpn, vright, vup);
}

/*
===============
R_SetupFrame
===============
*/
void
R_SetupFrame(void)
{
    int edgecount;
    vrect_t vrect;
    float w, h;

/* don't allow cheats in multiplayer */
    if (cl.maxclients > 1) {
	Cvar_Set("r_draworder", "0");
	Cvar_Set("r_fullbright", "0");
	Cvar_Set("r_ambient", "0");
    }

    if (r_numsurfs.value) {
	if ((surface_p - surfaces) > r_maxsurfsseen)
	    r_maxsurfsseen = surface_p - surfaces;

	Con_Printf("Used %d of %d surfs; %d max\n",
		   (int)(surface_p - surfaces),
		   (int)(surf_max - surfaces), r_maxsurfsseen);
    }

    if (r_numedges.value) {
	edgecount = edge_p - r_edges;

	if (edgecount > r_maxedgesseen)
	    r_maxedgesseen = edgecount;

	Con_Printf("Used %d of %d edges; %d max\n", edgecount,
		   r_numallocatededges, r_maxedgesseen);
    }

    r_refdef.ambientlight = r_ambient.value;

    if (r_refdef.ambientlight < 0)
	r_refdef.ambientlight = 0;

    if (!sv.active)
	r_draworder.value = 0;	/* don't let cheaters look behind walls */

    R_CheckVariables();

    R_AnimateLight();

    r_framecount++;

    /* build the transformation matrix for the given view angles.
     * modelorg is renderer-internal; r_origin and the vpn/vright/vup
     * triad are computed by R_PrepareFrame (which V_RenderView calls
     * before dispatching to the backend, so they're already set by
     * the time we get here -- the explicit call below is defence
     * in depth in case R_SetupFrame ever gets invoked from a path
     * that skipped R_PrepareFrame). */
    VectorCopy(r_refdef.vieworg, modelorg);
    R_PrepareFrame();

    /* current viewleaf */
    r_oldviewleaf = r_viewleaf;
    r_viewleaf = Mod_PointInLeaf(cl.worldmodel, r_origin);

    r_dowarpold = r_dowarp;
    r_dowarp = r_waterwarp.value && (r_viewleaf->contents <= CONTENTS_WATER);

    /* Phase 5b-03: when the active backend can dispatch the
     * water warp on the GPU and the user has compute
     * rendering enabled, suppress the SW warp flag.  Effect
     * on the SW raster: it renders the 3D view at full
     * vid.width x vid.height into vid.buffer (instead of at
     * the downscaled r_waterwarp_scale resolution into
     * r_warpbuffer), then the backend's GPU warp dispatch
     * samples vk_texture at full resolution with the sin
     * offsets that D_WarpScreen would have applied on the
     * CPU.  R_RenderView's "if (r_dowarp) D_WarpScreen()"
     * call site doesn't fire because r_dowarp is now
     * false on this path; the GPU dispatch happens in the
     * mirroring "else if" added there in this commit. */
    if (r_dowarp
            && g_rhi && g_rhi->dispatch_3d_warp_screen
            && g_rhi_compute_rendering) {
        r_dowarp = false;
    }

    /* If we're entering water, recompute the per-frame warp render-rect
     * cap from the active resolution and r_waterwarp_scale. The runtime
     * cap drives the clamp logic below; the underlying buffer has a
     * compile-time ceiling of WARP_WIDTH x WARP_HEIGHT (960x600).
     *
     * Behavior:
     *  - Active resolution <= 640x400: warp at full active resolution
     *    (no downscale; below this point software warp cost is small
     *    enough that downsampling only loses fidelity).
     *  - Above 640x400: warp at active * scale, clamped to a floor of
     *    320x200 (matches the original behavior when scale is small)
     *    and a ceiling of WARP_WIDTH x WARP_HEIGHT (the buffer cap).
     *  - Scale itself is clamped to [0.125, 1.0]; 1.0 = full-resolution
     *    warp, 0.5 = half (default), 0.125 = aggressive downscale.
     */
    if (r_dowarp && !r_dowarpold) {
	float scale = r_waterwarp_scale.value;
	int   mw, mh;

	if (scale < 0.125f) scale = 0.125f;
	if (scale > 1.0f)   scale = 1.0f;

	if (vid.width <= 640 && vid.height <= 400) {
	    mw = vid.width;
	    mh = vid.height;
	} else {
	    mw = (int)(vid.width  * scale);
	    mh = (int)(vid.height * scale);
	    if (mw < 320) mw = 320;
	    if (mh < 200) mh = 200;
	}
	if (mw > WARP_WIDTH)  mw = WARP_WIDTH;
	if (mh > WARP_HEIGHT) mh = WARP_HEIGHT;

	vid.maxwarpwidth  = mw;
	vid.maxwarpheight = mh;
    }

    if ((r_dowarp != r_dowarpold) || r_viewchanged) {
	if (r_dowarp) {
	    if ((vid.width <= vid.maxwarpwidth) &&
		(vid.height <= vid.maxwarpheight)) {
		vrect.x = 0;
		vrect.y = 0;
		vrect.width = vid.width;
		vrect.height = vid.height;

		R_ViewChanged(&vrect, sb_lines, vid.aspect);
	    } else {
		w = vid.width;
		h = vid.height;

		if (w > vid.maxwarpwidth) {
		    h *= (float)vid.maxwarpwidth / w;
		    w = vid.maxwarpwidth;
		}

		if (h > vid.maxwarpheight) {
		    h = vid.maxwarpheight;
		    w *= (float)vid.maxwarpheight / h;
		}

		vrect.x = 0;
		vrect.y = 0;
		vrect.width = (int)w;
		vrect.height = (int)h;

		R_ViewChanged(&vrect,
			      (int)((float)sb_lines *
				    (h / (float)vid.height)),
			      vid.aspect * (h / w) * ((float)vid.width /
						      (float)vid.height));
	    }
	} else {
	    vrect.x = 0;
	    vrect.y = 0;
	    vrect.width = vid.width;
	    vrect.height = vid.height;

	    R_ViewChanged(&vrect, sb_lines, vid.aspect);
	}

	r_viewchanged = false;
    }
/* start off with just the four screen edge clip planes */
    R_TransformFrustum();

/* save base values */
    VectorCopy(vpn, base_vpn);
    VectorCopy(vright, base_vright);
    VectorCopy(vup, base_vup);
    VectorCopy(modelorg, base_modelorg);

    R_SetSkyFrame();

    /* clear frame counts */
    r_drawnpolycount = 0;
    r_amodels_drawn = 0;

    D_SetupFrame();
}
