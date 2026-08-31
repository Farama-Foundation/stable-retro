/*
 * Software renderer backend.
 *
 * Wraps the existing r_*.c / d_*.c span-rasterizer behind
 * the RHI vtable.  The actual rendering code stays where it
 * is -- this file's only job is to expose a render_backend_t
 * instance whose function pointers route to the existing SW
 * entry points.
 *
 * Phase 1 is intentionally a thin forwarder: backend_sw_draw_view
 * simply calls R_RenderView, which still contains the full
 * SW per-frame pipeline (SetupFrame, PushDlights, MarkSurfaces,
 * CullSurfaces, EdgeDrawing, etc.).  This keeps the SW path
 * unchanged at runtime; the vtable just adds one indirection.
 * If a future phase wants to split per-stage hooks (e.g. so
 * a HW backend can reuse R_PushDlights but supply its own
 * surface rasterizer), R_RenderView's body can be unrolled
 * into the backend implementations at that point.
 *
 * Convention: every internal symbol in this file is static.
 * The only exported symbol is the g_rhi_backend_sw vtable
 * instance, declared in rhi.h.  This pattern carries over to
 * the future single-file HW backends (backend_vulkan.c etc.).
 */

#include "rhi.h"
#include "quakedef.h"
#include "render.h"

extern void R_RenderView(void);  /* r_main.c */

static qboolean
backend_sw_init(void)
{
    /* The software renderer's state is built up by
     * Host_Init's call chain (R_Init / R_InitTextures /
     * d_*.c initializers) before rhi_init runs.  Nothing
     * extra to set up here.
     *
     * If a future phase needs SW-backend-specific resources
     * (e.g. a colormap LUT or warp buffer allocated through
     * the RHI rather than R_Init), they get created here. */
    return true;
}

static void
backend_sw_shutdown(void)
{
    /* Mirror of init: the SW renderer's teardown is driven
     * by Host_Shutdown.  Nothing extra. */
}

static void
backend_sw_begin_frame(void)
{
    /* No-op.  The SW renderer paints into the linear output
     * buffer that the libretro video callback reads at end of
     * retro_run; there is no command-buffer acquire step to
     * perform here. */
}

static void
backend_sw_draw_view(const refdef_t *rd)
{
    /* r_refdef is a file-scope global in r_main.c, already
     * populated by V_CalcRefdef before V_RenderView reaches
     * this dispatch.  We accept it as a parameter for the
     * vtable's contract -- future HW backends will want to
     * snapshot it per-frame for command-list recording --
     * but the SW path reads the global directly, so we just
     * silence the unused-parameter warning here. */
    (void)rd;
    R_RenderView();
}

static void
backend_sw_end_frame(void)
{
    /* No-op.  See begin_frame. */
}

const render_backend_t g_rhi_backend_sw = {
    "software",
    RHI_BACKEND_SOFTWARE,
    backend_sw_init,
    backend_sw_shutdown,
    backend_sw_begin_frame,
    backend_sw_draw_view,
    backend_sw_end_frame,
    NULL,               /* queue_2d_pic:                  SW falls through to vid.buffer memcpy */
    NULL,               /* queue_2d_char:                 same */
    NULL,               /* queue_2d_pic_scaled:           same (scale > 1 stretched-memcpy stays SW) */
    NULL,               /* queue_2d_console_background:   same */
    NULL,               /* queue_2d_pic_translate_scaled: same (player-color preview stays SW) */
    NULL,               /* queue_2d_fill:                 same (Draw_Fill stays SW) */
    NULL,               /* queue_2d_fade_screen:          same (Draw_FadeScreen stays SW) */
    NULL,               /* dispatch_3d_particles:         SW renders particles via D_DrawParticle */
    NULL,               /* dispatch_3d_warp_screen:       SW renders warp via D_WarpScreen */
    NULL,               /* dispatch_3d_sprite:            SW renders sprites via D_SpriteDrawSpans */
    NULL,               /* dispatch_3d_alias:             SW renders alias via D_PolysetDrawSpans8 */
    NULL,               /* notify_cache_invalidate:       SW has no GPU-side pointer cache to invalidate */
    NULL,               /* notify_sky_texture:            SW reads skyoverlay/skyunderlay globals directly */
    NULL,               /* dispatch_3d_sky_span:          SW renders sky via D_DrawSkyScans8 in d_edge.c */
    NULL                /* dispatch_3d_turb_surface:      SW renders turb via Turbulent8 in d_edge.c */
};
