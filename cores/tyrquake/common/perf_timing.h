/*
 * perf_timing -- diagnostic per-frame CPU timing + work counters
 *
 * Gated by the r_perf cvar (default 0 = off, zero overhead).
 * When r_perf = 1, samples the libretro frontend's
 * get_time_usec() at strategic section boundaries and
 * accumulates per-frame work counters; logs a compact
 * summary to Con_Printf once per second.
 *
 * Timing values are DIAGNOSTIC ONLY and never feed back
 * into renderer state, simulation, or determinism-
 * critical decisions.  Toggling r_perf at runtime changes
 * only what gets printed.
 *
 * Determinism note: this module asks the libretro frontend
 * for time via the official retro_perf_callback interface
 * (RETRO_ENVIRONMENT_GET_PERF_INTERFACE).  The frontend
 * owns the call and may mock it for replay/netplay/save-
 * state scenarios.  The core never calls clock_gettime
 * or any other system timer directly.  See libretro.h's
 * retro_perf_callback documentation for the contract.
 *
 * Intended use during Phase 6/7 design work: with r_perf
 * enabled, swapping the active backend (vulkan_compute /
 * software) at the same scene gives an apples-to-apples
 * "where does the time go" comparison without any user-
 * facing instrumentation in shipping builds.
 */

#ifndef _PERF_TIMING_H
#define _PERF_TIMING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Section IDs.  Append new sections; keep PERF_SECTION_COUNT last. */
enum {
   PERF_SECTION_FRAME = 0,        /* full retro_run body */
   PERF_SECTION_RENDERVIEW,       /* Host_Frame body (CPU rendering work) */
   PERF_SECTION_HOST_RV,          /* RENDERVIEW sub: V_RenderView (3D scene
                                   * -- SW rasterizer + HW dispatch builders) */
   PERF_SECTION_HOST_2D,          /* RENDERVIEW sub: post-V_RenderView 2D work
                                   * in SCR_UpdateScreen (Sbar, HUD, console,
                                   * menu, V_UpdatePalette, VID_Update) */
   PERF_SECTION_HOST_PT,          /* RENDERVIEW sub: CL_RunParticles
                                   * (particle physics update; runs after
                                   * SCR_UpdateScreen) */
   PERF_SECTION_UPLOAD_VID,       /* FRAME sub (not in RENDERVIEW): host
                                   * -> staging memcpy of vid.buffer; runs
                                   * in end_frame between RENDERVIEW end and
                                   * RECORD_FRAME begin */
   PERF_SECTION_UPLOAD_ZBUF,      /* FRAME sub (not in RENDERVIEW): host
                                   * -> staging memcpy of d_pzbuffer;
                                   * conditional on 3D-dispatch activity;
                                   * same location as UPLOAD_VID */
   PERF_SECTION_AUDIO,            /* FRAME sub (not in RENDERVIEW): mixer
                                   * run via audio_step in libretro.c
                                   * AFTER end_frame returns */
   PERF_SECTION_RECORD_FRAME,     /* backend's command-buffer recording */
   PERF_SECTION_SUBMIT_PRESENT,   /* submit + wait + set_image + video_cb */
   PERF_SECTION_QUEUE_SUBMIT,     /* SUBMIT_PRESENT sub: vkQueueSubmit only */
   PERF_SECTION_BEGIN_FRAME_WAIT, /* begin_frame fence wait (cross-frame sync;
                                   * formerly end_frame's vkQueueWaitIdle, dropped at 3b) */
   PERF_SECTION_SET_IMAGE,        /* SUBMIT_PRESENT sub: vk_iface->set_image */
   PERF_SECTION_VIDEO_CB,         /* SUBMIT_PRESENT sub: video_cb */
   PERF_SECTION_COUNT
};

/* Counter IDs.  Append new counters; keep PERF_COUNTER_COUNT last. */
enum {
   PERF_COUNTER_BRUSH_SURFACES = 0,
   PERF_COUNTER_TURB_SURFACES,
   PERF_COUNTER_PARTICLES,
   PERF_COUNTER_SPRITES,
   PERF_COUNTER_ALIAS_ENTITIES,
   PERF_COUNTER_DISPATCHES,
   PERF_COUNTER_VID_BUFFER_BYTES,
   PERF_COUNTER_ZBUF_UPLOAD_BYTES,
   PERF_COUNTER_COUNT
};

/*
 * Register the r_perf cvar with the cvar system.  Must
 * be called once before any retro_run can fire.  Safe to
 * call from R_Init alongside the other r_* cvars.
 */
void perf_timing_register_cvars(void);

/*
 * Mark the start / end of a timed section.  All section
 * markers early-out cheaply when r_perf == 0, so leaving
 * them in hot paths is essentially free when disabled.
 * begin/end pairs nest only at the section_id granularity
 * (each section has its own start timestamp slot); calling
 * begin twice without an intervening end overwrites the
 * earlier timestamp.
 */
void perf_timing_section_begin(int section_id);
void perf_timing_section_end(int section_id);

/*
 * Add to a per-frame counter.  Early-outs when r_perf == 0.
 * Counter values are accumulated across all frames in the
 * current 1-second log window and printed as per-frame
 * averages.
 */
void perf_timing_counter_add(int counter_id, uint64_t n);

/*
 * Call once at the end of every frame.  Bumps the frame
 * counter; if at least 1 second of wall time has elapsed
 * since the last log, prints the rolling summary to
 * Con_Printf and resets the window.  Early-outs when
 * r_perf == 0.
 */
void perf_timing_end_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* _PERF_TIMING_H */
