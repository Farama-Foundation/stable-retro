/*
 * perf_timing.c -- see perf_timing.h for design notes.
 *
 * Implementation detail: the libretro frontend's
 * retro_perf_callback is fetched lazily on the first
 * timed section call (and once, regardless of success).
 * If the frontend doesn't provide the interface or
 * doesn't populate get_time_usec, the module silently
 * remains in "counters-only" mode -- section_begin /
 * section_end become no-ops and the once-per-second
 * log shows 0.0ms for every section but still reports
 * counters and frame rate (derived from frame count
 * alone is impossible without time, so FPS is also 0.0
 * in that mode -- counters are the meaningful output).
 *
 * All state is single-threaded process-global; libretro
 * cores run retro_run on a single thread by contract,
 * and no other call site for these APIs exists outside
 * that thread.  No locks needed.
 *
 * C89-clean: declarations before statements at every
 * block scope.
 */

#include "perf_timing.h"

#include <stddef.h>
#include <string.h>

#include "console.h"
#include "cvar.h"
#include "qtypes.h"

#include <libretro.h>

/* Pulled in from libretro.c.  Set during retro_set_environment
 * before any retro_run / retro_init activity, so it is non-NULL
 * by the time any section_begin can fire. */
extern retro_environment_t environ_cb;

/* The r_perf cvar.  Declared here (not in r_main.c) so the
 * cvar's lifetime is tied to this module.  Registered via
 * perf_timing_register_cvars. */
cvar_t r_perf = { "r_perf", "0" };

static struct retro_perf_callback perf_cb;
static qboolean perf_cb_ready = false;
static qboolean perf_init_attempted = false;

static double section_start_usec[PERF_SECTION_COUNT];
static double section_accum_usec[PERF_SECTION_COUNT];
static uint64_t counter_accum[PERF_COUNTER_COUNT];
static uint32_t frame_count_accum = 0;
static double log_window_start_usec = 0.0;

static void
ensure_init(void)
{
   if (perf_init_attempted)
      return;
   perf_init_attempted = true;
   memset(&perf_cb, 0, sizeof(perf_cb));
   if (!environ_cb)
      return;
   if (!environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      return;
   if (!perf_cb.get_time_usec)
      return;
   perf_cb_ready = true;
}

void
perf_timing_register_cvars(void)
{
   Cvar_RegisterVariable(&r_perf);
}

void
perf_timing_section_begin(int section_id)
{
   if (r_perf.value == 0.0f)
      return;
   ensure_init();
   if (!perf_cb_ready)
      return;
   if (section_id < 0 || section_id >= PERF_SECTION_COUNT)
      return;
   section_start_usec[section_id] = (double)perf_cb.get_time_usec();
}

void
perf_timing_section_end(int section_id)
{
   double now;
   if (r_perf.value == 0.0f)
      return;
   if (!perf_cb_ready)
      return;
   if (section_id < 0 || section_id >= PERF_SECTION_COUNT)
      return;
   now = (double)perf_cb.get_time_usec();
   section_accum_usec[section_id] += (now - section_start_usec[section_id]);
}

void
perf_timing_counter_add(int counter_id, uint64_t n)
{
   if (r_perf.value == 0.0f)
      return;
   if (counter_id < 0 || counter_id >= PERF_COUNTER_COUNT)
      return;
   counter_accum[counter_id] += n;
}

void
perf_timing_end_frame(void)
{
   double now;
   double window_elapsed;
   double window_ms;
   double fps;
   double frame_ms_avg;
   double render_ms_avg;
   double host_rv_ms_avg;
   double host_2d_ms_avg;
   double host_pt_ms_avg;
   double upload_vid_ms_avg;
   double upload_zbuf_ms_avg;
   double audio_ms_avg;
   double record_ms_avg;
   double submit_ms_avg;
   double qsubmit_ms_avg;
   double bwait_ms_avg;
   double setimg_ms_avg;
   double vidcb_ms_avg;
   uint32_t fc;
   int i;

   if (r_perf.value == 0.0f)
      return;
   ensure_init();
   if (!perf_cb_ready) {
      /* No timer.  Bump frame count so counters still
       * average correctly when we hit some other trigger
       * later -- but we never will without a clock.  In
       * practice this path is dead; the libretro frontend
       * always supplies get_time_usec.  Keeping the bump
       * out is fine. */
      return;
   }

   frame_count_accum++;
   now = (double)perf_cb.get_time_usec();

   if (log_window_start_usec == 0.0) {
      log_window_start_usec = now;
      return;
   }

   window_elapsed = now - log_window_start_usec;
   if (window_elapsed < 1000000.0)
      return;

   fc = frame_count_accum;
   window_ms      = window_elapsed / 1000.0;
   fps            = (double)fc / (window_elapsed / 1000000.0);
   frame_ms_avg   = section_accum_usec[PERF_SECTION_FRAME]          / 1000.0 / (double)fc;
   render_ms_avg  = section_accum_usec[PERF_SECTION_RENDERVIEW]     / 1000.0 / (double)fc;
   host_rv_ms_avg = section_accum_usec[PERF_SECTION_HOST_RV]        / 1000.0 / (double)fc;
   host_2d_ms_avg = section_accum_usec[PERF_SECTION_HOST_2D]        / 1000.0 / (double)fc;
   host_pt_ms_avg = section_accum_usec[PERF_SECTION_HOST_PT]        / 1000.0 / (double)fc;
   upload_vid_ms_avg  = section_accum_usec[PERF_SECTION_UPLOAD_VID]  / 1000.0 / (double)fc;
   upload_zbuf_ms_avg = section_accum_usec[PERF_SECTION_UPLOAD_ZBUF] / 1000.0 / (double)fc;
   audio_ms_avg       = section_accum_usec[PERF_SECTION_AUDIO]       / 1000.0 / (double)fc;
   record_ms_avg  = section_accum_usec[PERF_SECTION_RECORD_FRAME]   / 1000.0 / (double)fc;
   submit_ms_avg  = section_accum_usec[PERF_SECTION_SUBMIT_PRESENT] / 1000.0 / (double)fc;
   qsubmit_ms_avg = section_accum_usec[PERF_SECTION_QUEUE_SUBMIT]      / 1000.0 / (double)fc;
   bwait_ms_avg   = section_accum_usec[PERF_SECTION_BEGIN_FRAME_WAIT]  / 1000.0 / (double)fc;
   setimg_ms_avg  = section_accum_usec[PERF_SECTION_SET_IMAGE]       / 1000.0 / (double)fc;
   vidcb_ms_avg   = section_accum_usec[PERF_SECTION_VIDEO_CB]        / 1000.0 / (double)fc;

   Con_Printf("PERF [%u frames %.1fms]: %.1f FPS (%.2fms) | "
              "frame %.2f render %.2f (rv %.2f 2d %.2f pt %.2f) "
              "upload (vb %.2f zb %.2f) "
              "record %.2f submit %.2f "
              "(qs %.2f si %.2f vc %.2f) audio %.2f bw %.2f | "
              "brush=%llu turb=%llu alias=%llu particles=%llu "
              "sprites=%llu dispatches=%llu vidbuf=%llu zbufup=%llu\n",
              (unsigned)fc, window_ms,
              fps, frame_ms_avg,
              frame_ms_avg, render_ms_avg,
              host_rv_ms_avg, host_2d_ms_avg, host_pt_ms_avg,
              upload_vid_ms_avg, upload_zbuf_ms_avg,
              record_ms_avg, submit_ms_avg,
              qsubmit_ms_avg, setimg_ms_avg, vidcb_ms_avg,
              audio_ms_avg, bwait_ms_avg,
              (unsigned long long)(counter_accum[PERF_COUNTER_BRUSH_SURFACES]   / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_TURB_SURFACES]    / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_ALIAS_ENTITIES]   / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_PARTICLES]        / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_SPRITES]          / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_DISPATCHES]       / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_VID_BUFFER_BYTES] / fc),
              (unsigned long long)(counter_accum[PERF_COUNTER_ZBUF_UPLOAD_BYTES]/ fc));

   for (i = 0; i < PERF_SECTION_COUNT; i++)
      section_accum_usec[i] = 0.0;
   for (i = 0; i < PERF_COUNTER_COUNT; i++)
      counter_accum[i] = 0;
   frame_count_accum = 0;
   log_window_start_usec = now;
}
