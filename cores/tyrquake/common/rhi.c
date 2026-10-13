/*
 * RHI selection and dispatch.
 *
 * The 'tyrquake_renderer' core option selects a backend at
 * retro_load_game time.  Today only two values are valid:
 *
 *   auto      -> Future: try the frontend's preferred HW
 *                context via GET_PREFERRED_HW_RENDER, then a
 *                fallback chain (vulkan -> d3d12 -> d3d11 ->
 *                gl -> software).  Phase 1 has only the
 *                software backend built, so auto always
 *                lands there.
 *   software  -> Always the software backend, regardless of
 *                what the frontend offers.  Useful for
 *                debugging or when the user wants the SW
 *                rasterizer's specific look (per-span affine
 *                texturing, palette + colormap output) even
 *                though the frontend's video driver could
 *                support a HW path.
 *
 * The option is marked requires-restart in
 * libretro_core_options.h so any change tears down and
 * rebuilds at game-load.  No hot-swap.
 */

#include "rhi.h"
#include "common.h"
#include "zone.h"
#include <string.h>
#include <libretro.h>

const render_backend_t *g_rhi = NULL;

/* User preference (Phase 5 scaffolding) for the 3D-
 * rendering path on backends that support compute
 * shaders.  Default is `true` (Quake-on-compute, SW-look
 * via GPU) -- the libretro layer overwrites this from
 * the `tyrquake_compute_rendering` core option at
 * startup / update.  Read by individual backends in
 * their init / draw_view paths; backends without
 * compute support simply ignore it.
 *
 * Note: this is a *preference*, not a capability flag.
 * Future code that wants to know whether the active
 * backend actually honors the request should query the
 * backend rather than read this directly (no such
 * query exists yet -- the scaffolding lands the
 * preference; capability reporting comes when the
 * second compute-capable backend lands and there's
 * something to disambiguate). */
qboolean g_rhi_compute_rendering = true;

/* environ_cb / log_cb are owned by libretro.c.  Same extern
 * pattern menu.c uses for environ_cb.  log_cb is the
 * libretro frontend's logger; using it (rather than
 * Con_Printf) routes init-time messages to whatever the
 * frontend captures -- typically a log window or stdout --
 * instead of burying them in Quake's in-game console
 * scrollback. */
extern retro_environment_t environ_cb;
extern retro_log_printf_t  log_cb;

static const render_backend_t *
rhi_pick_backend(void)
{
    struct retro_variable var;
    const char *value = NULL;

    var.key   = "tyrquake_renderer";
    var.value = NULL;
    if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var))
        value = var.value;

    /* Explicit selection -- caller named a backend, honour
     * it unconditionally.  Returning a backend whose init()
     * will fail (e.g. the user asked for 'vulkan' on a build
     * without RHI_HAVE_VULKAN) is fine: rhi_init logs the
     * failure and the next launch picks something else. */
    if (value && strcmp(value, "software") == 0)
        return &g_rhi_backend_sw;
    if (value && strcmp(value, "vulkan") == 0)
        return &g_rhi_backend_vk;

    /* Auto.  Try HW backends in order, falling through to
     * software when their init() refuses.  The order is
     * roughly modern-first: Vulkan, then (future) D3D12,
     * D3D11, GL.  Each backend's init() is responsible for
     * actually negotiating the HW context with the frontend
     * -- a false return here means the frontend declined or
     * the build flag is off, and we should try the next.
     *
     * Note: the actual try-and-fall-through happens in
     * rhi_init below, not here.  This function picks the
     * preferred candidate; rhi_init drives the fallback
     * chain.  When more backends ship, the fallback chain
     * grows. */
    return &g_rhi_backend_vk;  /* preferred HW backend today */
}

qboolean
rhi_init(void)
{
    const render_backend_t *candidate;

    if (g_rhi)
        return true;  /* already up; idempotent */

    /* First try the preferred candidate (explicit user
     * selection or the head of the auto chain).  On init
     * failure, fall back to the software backend, which is
     * always available and whose init() is trivially
     * successful.  This keeps 'auto' robust: the user always
     * gets *some* renderer.
     *
     * The fallback step is unconditional rather than driven
     * by the option value: if the user explicitly asked for
     * 'vulkan' but the build flag is off (or the HW context
     * handshake fails), we still want a playable game.  The
     * log line documents which backend actually came up so
     * the user can see the fallback happened. */
    candidate = rhi_pick_backend();
    if (candidate && candidate->init && candidate->init()) {
        g_rhi = candidate;
        /* Phase 5b-06 follow-up: route Cache_Free / Cache_Move
         * notifications into the backend's pointer-cache
         * invalidator (Vulkan overlay slot cache, etc.).  NULL
         * for backends with no GPU-side pointer cache (SW). */
        Cache_SetInvalidateCallback(g_rhi->notify_cache_invalidate);
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "rhi: %s backend active\n", g_rhi->name);
        return true;
    }

    if (candidate && candidate != &g_rhi_backend_sw) {
        if (log_cb)
            log_cb(RETRO_LOG_WARN,
                   "rhi: %s backend init failed; falling back to software\n",
                   candidate->name ? candidate->name : "(unnamed)");
    }

    if (g_rhi_backend_sw.init && g_rhi_backend_sw.init()) {
        g_rhi = &g_rhi_backend_sw;
        Cache_SetInvalidateCallback(g_rhi->notify_cache_invalidate);
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "rhi: software backend active\n");
        return true;
    }

    if (log_cb)
        log_cb(RETRO_LOG_ERROR, "rhi: no backend stood up\n");
    return false;
}

void
rhi_shutdown(void)
{
    const render_backend_t *active = g_rhi;
    if (!active)
        return;
    /* Clear g_rhi before the backend tears down so concurrent
     * callers (there shouldn't be any in the libretro
     * single-threaded model, but be conservative) don't
     * dispatch into a half-destroyed backend.
     *
     * Phase 5b-06 follow-up: drop the cache invalidate callback
     * first so any Cache_Free that runs during teardown (host
     * shutdown unwinds models, sound, etc.) doesn't trampoline
     * into a backend that's mid-destroy. */
    Cache_SetInvalidateCallback(NULL);
    g_rhi = NULL;
    if (active->shutdown)
        active->shutdown();
}
