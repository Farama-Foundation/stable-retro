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

#include "compat/strl.h"
#include "libretro.h"
#include "libretro_core_options.h"
#include "perf_timing.h"
#include "rhi.h"
#include <retro_miscellaneous.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <string/stdstring.h>
#include <retro_dirent.h>

#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#elif defined(_WIN32) && defined(_XBOX)
#include <xtl.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "cmd.h"
#include "common.h"
#include "quakedef.h"
#include "d_local.h"
#include "sys.h"

#include "qtypes.h"
#include "sound.h"
#include "bgmusic.h"
#include "keys.h"
#include "cdaudio_driver.h"
#include "input.h"

#include "client.h"
#include "host.h"

qboolean isDedicated;

#ifdef GEKKO
#include <ogc/lwp_watchdog.h>
#endif

#ifdef __PSL1GHT__
#include <lv2/systime.h>
#elif defined (__PS3__)
#include <sys/sys_time.h>
#include <sys/timer.h>
#endif

#include "sbar.h"

#define SURFCACHE_SIZE 10485760

#define RETRO_DEVICE_MODERN  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 2)
#define RETRO_DEVICE_JOYPAD_ALT  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

extern void CDAudio_Update(void);

unsigned width       = 320;
unsigned height      = 200;
unsigned device_type = 0;

unsigned MEMSIZE_MB;

bool shutdown_core = false;

static bool libretro_supports_bitmasks = false;

#if defined(HW_DOL)
#define DEFAULT_MEMSIZE_MB 8
#elif defined(WIIU)
#define DEFAULT_MEMSIZE_MB 32
#elif defined(HW_RVL) || defined(_XBOX1) 
#define DEFAULT_MEMSIZE_MB 24
#else
#define DEFAULT_MEMSIZE_MB 32
#endif

#ifdef RHI_HAVE_VULKAN
/* Phase 5b-06 follow-up (root-cause side): widen the Quake hunk
 * for builds that can stand up the GPU compute renderer.  The
 * extra pressure relative to a SW-only build comes from:
 *
 *   - Alias subdiv caches.  r_polysubdiv 3 keeps three
 *     concurrent mesh resolutions per alias model in the cache
 *     -- around 10 MiB peak across a typical Quake level, which
 *     against a 32 MiB hunk meant constant Cache_FreeLow churn
 *     and the qplaque-stripes corruption (since fixed source-
 *     side by 4f5a634's invalidate-on-Cache_Free hook).  More
 *     headroom keeps that pressure off the critical path.
 *   - Sky-texture atlas pair per level (~32 KiB, tiny but
 *     additive).
 *   - Misc compute-side cached data (alias skin uploads, etc.;
 *     tens of KiB).
 *
 * 64 MiB is a 2x bump on the PC default and gives ~30 MiB
 * headroom over the worst-case 3-pass alias resident set.  SW-
 * only builds (no RHI_HAVE_VULKAN) keep their previous footprint
 * -- important on platforms that track per-core memory (the Wii /
 * Xbox / GameCube defaults above stay at 8 / 24 / 32 MiB
 * regardless).  The RHI is a compile-time switch: a Vulkan-
 * capable build running its SW fallback (frontend without Vulkan
 * support, or user-selected) still gets the bigger pool, which
 * is benign -- Cache_FreeLow just fires less often. */
#undef  DEFAULT_MEMSIZE_MB
#define DEFAULT_MEMSIZE_MB 64
#endif

/* Bounds for frontend-driven hunk sizing via
 * RETRO_ENVIRONMENT_GET_MEMORY_STATUS: take at most 1/4 of the memory the
 * frontend reports free, cap it, and never shrink below the per-platform
 * DEFAULT_MEMSIZE_MB chosen above. The cap keeps the reservation sane on
 * desktops with gigabytes free while still covering the largest mods; the
 * floor preserves the current tuned behaviour on constrained targets and
 * on any frontend that cannot answer the query. */
#define HUNK_FREE_SHIFT 2      /* free >> 2  == a quarter of reported free RAM */
#define HUNK_MAX_MB     256u   /* upper bound; ample for the biggest Quake mods */

/* Use 44.1 kHz by default (matches CD
 * audio tracks) */
#define AUDIO_SAMPLERATE_DEFAULT 44100
/* SFX resampling fails with certain fps/
 * sample rate combinations (seems to be an
 * internal limitation...). When running at
 * the affected framerates, we must fall back
 * to lower or higher sample rates to maintain
 * acceptable audio quality. */
#define AUDIO_SAMPLERATE_22KHZ 22050
#define AUDIO_SAMPLERATE_48KHZ 48000
/* Widened from uint16_t: the "Sound Samplerate (Hint)" option can select
 * 96000, which does not fit in 16 bits. */
static int audio_samplerate = AUDIO_SAMPLERATE_DEFAULT;

/* Backported from prboom's "Sound Samplerate (Hint)" option.  Lets the core
 * render directly at the host's preferred rate.  Guarded so this builds
 * against an older in-tree libretro.h as well as a newer one that already
 * defines it (data is unsigned* Hz). */
#ifndef RETRO_ENVIRONMENT_GET_TARGET_SAMPLE_RATE
#define RETRO_ENVIRONMENT_GET_TARGET_SAMPLE_RATE (81 | RETRO_ENVIRONMENT_EXPERIMENTAL)
#endif

/* Linear output buffer.  S_PaintChannels writes
 * exactly samples_per_frame stereo frames into this
 * buffer per S_Update; audio_step then hands it
 * straight to audio_batch_cb.  No ring, no wrap. */
static int16_t audio_out_buffer[AUDIO_BUFFER_SIZE];

/* Float output buffer, used only when float output was negotiated.  The engine
 * writes normalized float [-1,1] here (snd_float_buffer points at it) instead
 * of audio_out_buffer; audio_step then pushes it via audio_batch_cb_float. */
static float audio_out_buffer_f[AUDIO_BUFFER_SIZE];

/* Initial cap on stereo frames per audio_batch_cb
 * invocation.  RetroArch typically accepts up to
 * ~4096 per call; if the frontend partial-writes
 * we shrink this dynamically (see audio_step). */
static unsigned audio_batch_frames_max = 4096;

/* System analog stick range is -0x8000 to 0x8000 */
#define ANALOG_RANGE 0x8000
/* Default deadzone: 15% */
static int analog_deadzone = (int)(0.15f * ANALOG_RANGE);

#define GP_MAXBINDS 32

typedef struct {
   struct retro_input_descriptor desc[GP_MAXBINDS];
   struct {
      char *key;
      char *com;
   } bind[GP_MAXBINDS];
} gp_layout_t;

gp_layout_t modern = {
   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Swim Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Strafe Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Strafe Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Swim Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Show Scores" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+moveleft"},     {"JOY_RIGHT", "+moveright"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+movedown"},     {"JOY_A",     "+moveright"},
      {"JOY_X",     "+moveup"},       {"JOY_Y",     "+moveleft"},
      {"JOY_L",     "impulse 12"},    {"JOY_R",     "impulse 10"},
      {"JOY_L2",    "+jump"},         {"JOY_R2",    "+attack"},
      {"JOY_SELECT","+showscores"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};

gp_layout_t classic = {
   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Cycle Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Freelook" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Strafe Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Strafe Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Look Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Look Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Move Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Swim Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Toggle Run Mode" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+left"},         {"JOY_RIGHT", "+right"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+jump"} ,        {"JOY_A",     "impulse 10"},
      {"JOY_X",     "+klook"},        {"JOY_Y",     "+attack"},
      {"JOY_L",     "+moveleft"},     {"JOY_R",     "+moveright"},
      {"JOY_L2",    "+lookup"},       {"JOY_R2",    "+lookdown"},
      {"JOY_L3",    "+movedown"},     {"JOY_R3",    "+moveup"},
      {"JOY_SELECT","+togglewalk"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};

gp_layout_t classic_alt = {

   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Look Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Look Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Look Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Look Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Run" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Move Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Toggle Run Mode" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+moveleft"},     {"JOY_RIGHT", "+moveright"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+lookdown"},     {"JOY_A",     "+right"},
      {"JOY_X",     "+lookup"},       {"JOY_Y",     "+left"},
      {"JOY_L",     "+jump"},         {"JOY_R",     "+attack"},
      {"JOY_L2",    "+speed"},          {"JOY_R2",    "impulse 10"},
      {"JOY_L3",    "+movedown"},     {"JOY_R3",    "impulse 12"},
      {"JOY_SELECT","+togglewalk"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};

gp_layout_t *gp_layoutp = NULL;

static float framerate      = 60.0f;
static float frametime_usec = 1000.0f / 60.0f;

static bool initial_resolution_set = false;
static bool has_set_username       = false;
static int invert_y_axis = 1;

unsigned char *heap;

#define MAX_PADS 1
static unsigned quake_devices[1];

static struct retro_rumble_interface rumble = {0};
static bool rumble_enabled                  = false;
static uint16_t rumble_damage_strength      = 0;
static uint16_t rumble_touch_strength       = 0;
static int16_t rumble_touch_counter         = -1;

void retro_set_rumble_damage(int damage)
{
   /* Rumble scales linearly from 0xFFF to 0xFFFF
    * as damage increases from 1 to 50 */
   int capped_damage = (damage < 50) ? damage : 50;
   uint16_t strength = 0;

   if (!rumble.set_rumble_state ||
       (!rumble_enabled && (capped_damage > 0)))
      return;

   if (capped_damage > 0)
      strength = 0xFFF + (capped_damage * 0x4CC);

   /* Return early if strength matches last
    * set value */
   if (strength == rumble_damage_strength)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength);
   rumble_damage_strength = strength;
}

void retro_set_rumble_touch(unsigned intensity, float duration)
{
   /* Rumble scales linearly from 0xFF to 0xFFFF
    * as intensity increases from 1 to 20 */
   unsigned capped_intensity = (intensity < 20) ? intensity : 20;
   uint16_t strength         = 0;

   if (!rumble.set_rumble_state ||
       (!rumble_enabled && (capped_intensity > 0)))
      return;

   if ((capped_intensity > 0) && (duration > 0.0f))
   {
      strength             = 0xFF + (capped_intensity * 0xCC0);
      rumble_touch_counter = (int16_t)((duration / frametime_usec) + 1.0f);
   }

   /* Return early if strength matches last
    * set value */
   if (strength == rumble_touch_strength)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, strength);
   rumble_touch_strength = strength;
}

/* ======================================================================= */
/* General routines */
/* ======================================================================= */
/**/
/* log_cb is non-static so other TUs (rhi.c, future backend
 * files) can log init-time events through the libretro
 * frontend's log rather than Quake's in-game console.  Same
 * extern convention as environ_cb below. */
retro_log_printf_t log_cb;
/* video_cb is non-static so HW backends (backend_vulkan.c) can
 * call it directly from their end_frame with
 * RETRO_HW_FRAME_BUFFER_VALID -- the frontend reads the HW
 * image set via the per-API interface, so the core just needs
 * to signal "frame ready" through the usual video callback.
 * Same extern convention as environ_cb / log_cb. */
retro_video_refresh_t video_cb;
/* per-sample audio_cb is required by the libretro
 * API but never invoked: this core only ever uses
 * the batch callback (audio_batch_cb).  See
 * retro_set_audio_sample below -- we accept the
 * callback registration but throw it away. */
static retro_audio_sample_batch_t audio_batch_cb;
/* Float audio output, negotiated once in retro_load_game via
 * RETRO_ENVIRONMENT_GET_AUDIO_SAMPLE_BATCH_FLOAT.  use_float_output stays 0
 * (and audio_batch_cb_float NULL) on any frontend that doesn't support it,
 * which keeps the deterministic int16 path.  s_float_output / snd_float_buffer
 * live in snd_mix.c and are declared in sound.h (included above). */
static retro_audio_sample_batch_float_t audio_batch_cb_float = NULL;
static int use_float_output = 0;
retro_environment_t environ_cb;
static retro_input_poll_t poll_cb;
static retro_input_state_t input_cb;

void Sys_Printf(const char *fmt, ...) { }
void Sys_Quit(void) { Host_Shutdown(); }

void Sys_Init(void)
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
   }
}

void Sys_Error(const char *error, ...)
{
   char buffer[256];
   va_list ap;

   va_start(ap, error);
   /* vsnprintf instead of vsprintf -- buffer is fixed size,
    * and even though all current Sys_Error callers pass
    * compile-time format strings with bounded %s args, a
    * future caller passing a long argument (or any caller
    * forgetting to cap a %s arg) would silently overflow
    * the stack frame. */
   vsnprintf(buffer, sizeof(buffer), error, ap);
   if (log_cb)
      log_cb(RETRO_LOG_ERROR, "%s\n", buffer);
   va_end(ap);

   /* Historical behaviour: log and return.  An earlier
    * round added a longjmp-out via sys_abort to give
    * Sys_Error proper no-return semantics matching stock
    * Quake; that broke the renderer because R_RenderView
    * had three vestigial alignment guards firing
    * Sys_Error every frame.  The longjmp was reverted
    * (3301dcf) and the alignment guards were removed in
    * a follow-up.  Properly fixing the remaining call
    * sites that DO depend on no-return semantics
    * (SZ_GetSpace overflow, Hunk_Alloc failure) needs
    * per-site review rather than a blanket abort. */
}

char * Sys_ConsoleInput(void) { return NULL; }

viddef_t vid;			/* global video state */

void retro_init(void)
{
   struct retro_log_callback log;

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   Sys_Init();
}

void retro_deinit(void)
{
   if (!shutdown_core)
   {
      CL_Disconnect();
      Host_ShutdownServer(false);
   }

   Sys_Quit();
   if (heap)
      free(heap);

   libretro_supports_bitmasks = false;

   retro_set_rumble_damage(0);
   retro_set_rumble_touch(0, 0.0f);

   memset(&rumble, 0, sizeof(struct retro_rumble_interface));
   rumble_enabled         = false;
   rumble_damage_strength = 0;
   rumble_touch_strength  = 0;
   rumble_touch_counter   = -1;

   /* Reset the file-static "set once" flags so a subsequent retro_init
    * on a statically-linked target re-reads frontend options instead
    * of carrying values over from the previous session. */
   initial_resolution_set = false;
   has_set_username       = false;
   width                  = 320;
   height                 = 200;
   shutdown_core          = false;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

static void gp_layout_set_bind(gp_layout_t gp_layout)
{
   char buf[100];
   unsigned i;
   for (i=0; gp_layout.bind[i].key; ++i)
   {
      snprintf(buf, sizeof(buf), "bind %s \"%s\"", gp_layout.bind[i].key,
                                                   gp_layout.bind[i].com);
      Cmd_ExecuteString(buf, src_command);
   }
}


void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if (port == 0)
   {
      switch (device)
      {
         case RETRO_DEVICE_JOYPAD:
            quake_devices[port] = RETRO_DEVICE_JOYPAD;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, classic.desc);
            gp_layout_set_bind(classic);
            break;
         case RETRO_DEVICE_JOYPAD_ALT:
            quake_devices[port] = RETRO_DEVICE_JOYPAD;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, classic_alt.desc);
            gp_layout_set_bind(classic_alt);
            break;
         case RETRO_DEVICE_MODERN:
            quake_devices[port] = RETRO_DEVICE_MODERN;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, modern.desc);
            gp_layout_set_bind(modern);
            break;
         case RETRO_DEVICE_KEYBOARD:
            quake_devices[port] = RETRO_DEVICE_KEYBOARD;
            break;
         case RETRO_DEVICE_NONE:
         default:
            quake_devices[port] = RETRO_DEVICE_NONE;
            if (log_cb)
               log_cb(RETRO_LOG_ERROR, "[libretro]: Invalid device.\n");
      }
   }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "TyrQuake";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v" stringify(TYR_VERSION) GIT_VERSION;
   info->need_fullpath    = true;
   info->valid_extensions = "pak";
}

/* Aspect-ratio table indexed by the r_aspect cvar (0..R_ASPECT_MAX).
 * Index 0 is the vanilla 4:3 and must stay first so existing configs
 * (and a freshly-defaulted "0") keep the historical display. */
float R_AspectRatioForIndex(int idx)
{
   static const float ratios[R_ASPECT_NUM_RATIOS] = {
      4.0f  / 3.0f,   /* 0: 4:3   (default) */
      16.0f / 9.0f,   /* 1: 16:9            */
      16.0f / 10.0f,  /* 2: 16:10           */
      64.0f / 27.0f,  /* 3: 21:9  (DCI/UW)  */
      32.0f / 9.0f    /* 4: 32:9            */
   };
   if (idx < 0)             idx = 0;
   if (idx > R_ASPECT_MAX)  idx = R_ASPECT_MAX;
   return ratios[idx];
}

/* r_aspect cvar callback: clamp the index and repush the display
 * geometry to the frontend.  Only the aspect the frontend stretches
 * the framebuffer to changes -- base/max width/height (the actual
 * rendered resolution) are left alone, so this is cheap and takes
 * effect on the next presented frame. */
void R_AspectRatioChanged(cvar_t *var)
{
   struct retro_system_av_info av;
   int idx = (int)var->value;

   if (idx < 0)             Cvar_SetValue("r_aspect", 0.0f);
   if (idx > R_ASPECT_MAX)  Cvar_SetValue("r_aspect", (float)R_ASPECT_MAX);

   /* Force the renderer to recompute the view.  SCR_CalcRefdef (which
    * derives fov_x via the Hor+ path) and R_ViewChanged (which applies
    * the aspect-compensated pixelAspect) only run when
    * vid.recalc_refdef is set -- otherwise the projection stays frozen
    * at whatever fov/viewsize last triggered it, and changing the
    * aspect ratio has no visible effect.  This is the trigger that was
    * missing: without it both the fov_x widening and the pixelAspect
    * override are computed but never re-evaluated on an r_aspect
    * change. */
   vid.recalc_refdef = true;

   if (!environ_cb)
      return;

   memset(&av, 0, sizeof(av));
   retro_get_system_av_info(&av);
   environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av);
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps            = framerate;
   info->timing.sample_rate    = audio_samplerate;

   info->geometry.base_width   = width;
   info->geometry.base_height  = height;
   info->geometry.max_width    = width;
   info->geometry.max_height   = height;
   {
      cvar_t *cv = Cvar_FindVar("r_aspect");
      int idx = cv ? (int)cv->value : 0;
      info->geometry.aspect_ratio = R_AspectRatioForIndex(idx);
   }
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;

   static const struct retro_controller_description port_1[] = {
      { "Gamepad Classic", RETRO_DEVICE_JOYPAD },
      { "Gamepad Classic Alt", RETRO_DEVICE_JOYPAD_ALT },
      { "Gamepad Modern", RETRO_DEVICE_MODERN },
      { "Keyboard + Mouse", RETRO_DEVICE_KEYBOARD },
   };

   static const struct retro_controller_info ports[] = {
      { port_1, 4 },
      { 0 },
   };

   environ_cb = cb;

   libretro_set_core_options(environ_cb);
   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
   {
      filestream_vfs_init(&vfs_iface_info);
      dirent_vfs_init(&vfs_iface_info);
   }
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   /* No-op: this core uses only the batch callback.
    * The libretro API requires this entry point but
    * we never invoke the per-sample callback. */
   (void)cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

extern void M_Game_StartNewGame(void);

void retro_reset(void)
{
   M_Game_StartNewGame();
}

void Sys_SendKeyEvents(void)
{
   int port;

   if (!poll_cb)
      return;

   poll_cb();

   if (!input_cb)
      return;

   for (port = 0; port < MAX_PADS; port++)
   {
      if (!input_cb)
         break;

      switch (quake_devices[port])
      {
         case RETRO_DEVICE_JOYPAD:
         case RETRO_DEVICE_JOYPAD_ALT:
         case RETRO_DEVICE_MODERN:
            {
               unsigned i;
               int16_t ret    = 0;
               if (libretro_supports_bitmasks)
                  ret = input_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
               else
               {
                  for (i=RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R3; ++i)
                  {
                     if (input_cb(port, RETRO_DEVICE_JOYPAD, 0, i))
                        ret |= (1 << i);
                  }
               }

               for (i=RETRO_DEVICE_ID_JOYPAD_B; 
                     i <= RETRO_DEVICE_ID_JOYPAD_R3; ++i)
               {
                  if (ret & (1 << i))
                     Key_Event(K_JOY_B + i, 1);
                  else
                     Key_Event(K_JOY_B + i, 0);
               }
            }
            break;
         case RETRO_DEVICE_KEYBOARD:
            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT))
               Key_Event(K_MOUSE1, 1);
            else
               Key_Event(K_MOUSE1, 0);

            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT))
               Key_Event(K_MOUSE2, 1);
            else
               Key_Event(K_MOUSE2, 0);

            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE))
               Key_Event(K_MOUSE3, 1);
            else
               Key_Event(K_MOUSE3, 0);

            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP))
               Key_Event(K_MOUSE4, 1);
            else
               Key_Event(K_MOUSE4, 0);

            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN))
               Key_Event(K_MOUSE5, 1);
            else
               Key_Event(K_MOUSE5, 0);

            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP))
               Key_Event(K_MOUSE6, 1);
            else
               Key_Event(K_MOUSE6, 0);

            if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN))
               Key_Event(K_MOUSE7, 1);
            else
               Key_Event(K_MOUSE7, 0);

            if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
               if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP))
                  Key_Event(K_UPARROW, 1);
               else
                  Key_Event(K_UPARROW, 0);

               if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN))
                  Key_Event(K_DOWNARROW, 1);
               else
                  Key_Event(K_DOWNARROW, 0);

               if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT))
                  Key_Event(K_LEFTARROW, 1);
               else
                  Key_Event(K_LEFTARROW, 0);

               if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT))
                  Key_Event(K_RIGHTARROW, 1);
               else
                  Key_Event(K_RIGHTARROW, 0);
               }
            break;
         case RETRO_DEVICE_NONE:
            break;
      }
   }
}

static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
  /* character-only events are discarded */
  if (keycode != RETROK_UNKNOWN) {
      /* Key_Event indexes keydown[key], key_repeats[key], and
       * keybindings[key] directly with no internal bound check;
       * each of those arrays is sized K_LAST. The full RETROK
       * enum currently caps at RETROK_LAUNCH_APP2 = 341 and
       * K_LAST evaluates to 383, so all real keycodes from a
       * conformant frontend land inside. The cast 'unsigned'
       * keycode -> 'knum_t' that Key_Event takes silently
       * accepts any 32-bit value though, so a buggy or hostile
       * frontend feeding RETROK_DUMMY or any out-of-range
       * value would walk straight off the end of the keydown
       * table and stomp adjacent globals (keybindings strings
       * Z_Malloc'd at Key_SetBinding time, key_lastpress,
       * etc.). Reject anything outside the enum range. */
      if (keycode >= K_LAST)
         return;
      if (down)
         Key_Event((knum_t) keycode, 1);
      else
         Key_Event((knum_t) keycode, 0);
   }
}

const char *argv[MAX_NUM_ARGVS];
static const char *empty_string = "";

static const float supported_framerates[] = {
   10.0f,
   15.0f,
   20.0f,
   25.0f,
   30.0f,
   40.0f,
   50.0f,
   60.0f,
   72.0f,
   75.0f,
   90.0f,
   100.0f,
   119.0f,
   120.0f,
   144.0f,
   155.0f,
   160.0f,
   165.0f,
   180.0f,
   200.0f,
   240.0f,
   244.0f,
   300.0f,
   360.0f,
   480.0f,
   540.0f,
};
#define NUM_SUPPORTED_FRAMERATES (sizeof(supported_framerates) / sizeof(supported_framerates[0]))

static float sanitise_framerate(float target)
{
   unsigned i = 1;

   /* NaN comparisons are all false, so the two early-return
    * guards below (target <= first and target >= last) would
    * both miss it, and the while-loop predicate
    * 'supported_framerates[i] > target' is also false for NaN,
    * so i walks past NUM_SUPPORTED_FRAMERATES-1 and the
    * subsequent supported_framerates[i] read goes off the end
    * of the table. +Inf takes the 'target >= last' branch
    * correctly, but the NaN gap is real. Pin both to a sane
    * default before the table walk. */
   if (IS_NAN(target))
      target = 60.0f;

   if (target <= supported_framerates[0])
      return supported_framerates[0];

   if (target >= supported_framerates[NUM_SUPPORTED_FRAMERATES - 1])
      return supported_framerates[NUM_SUPPORTED_FRAMERATES - 1];

   while (i < NUM_SUPPORTED_FRAMERATES)
   {
      if (supported_framerates[i] > target)
         break;

      i++;
   }

   if ((supported_framerates[i] - target) <=
       (target - supported_framerates[i - 1]))
      return supported_framerates[i];

   return supported_framerates[i - 1];
}

/* The framerate-appropriate sample rate, used as the "auto" fallback when the
 * frontend can't report a target rate.  Preserves the prior behaviour for the
 * fps/rate combinations the SFX resampler is sensitive to (see the
 * AUDIO_SAMPLERATE_* notes above). */
static int framerate_to_samplerate(float fr)
{
   if (fr == 40.0f || fr == 72.0f || fr == 119.0f)
      return AUDIO_SAMPLERATE_22KHZ;
   if (fr == 120.0f)
      return AUDIO_SAMPLERATE_48KHZ;
   return AUDIO_SAMPLERATE_DEFAULT;
}

/* Snap a host target rate to the nearest value the option advertises.
 * Identical thresholds to the prboom backport this came from. */
static int nearest_supported_rate(unsigned host_rate)
{
   if      (host_rate <= (32000u + 44100u) / 2) return 32000;
   else if (host_rate <= (44100u + 48000u) / 2) return 44100;
   else if (host_rate <= (48000u + 96000u) / 2) return 48000;
   return 96000;
}

/* Resolve the "Sound Samplerate (Hint)" core option to a concrete rate.
 * "auto" asks the frontend for its target rate via
 * RETRO_ENVIRONMENT_GET_TARGET_SAMPLE_RATE and snaps to the nearest advertised
 * value; if the frontend doesn't implement the call, it falls back to the
 * framerate-appropriate rate (the prior default), so frontends without the
 * call see no change.  An explicit "32000".."96000" is taken verbatim.
 * Resolved at startup, before SNDDMA_Init reads audio_samplerate. */
static void update_audio_samplerate(void)
{
   struct retro_variable var;
   int chosen = framerate_to_samplerate(framerate);

   var.key   = "tyrquake_sound_samplerate";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "auto"))
      {
         unsigned host_rate = 0;
         if (environ_cb(RETRO_ENVIRONMENT_GET_TARGET_SAMPLE_RATE, &host_rate)
               && host_rate > 0)
            chosen = nearest_supported_rate(host_rate);
         /* else: keep the framerate-derived fallback above */
      }
      else
         chosen = atoi(var.value);  /* "32000".."96000" */
   }

   audio_samplerate = chosen;
}

static void update_variables(bool startup)
{
   struct retro_variable var;

   var.key = "tyrquake_framerate";
   var.value = NULL;

   if (startup)
   {
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         if (!strcmp(var.value, "auto"))
         {
            float target_framerate = 0.0f;

            if (!environ_cb(RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE,
                  &target_framerate))
               target_framerate = 60.0f;

            framerate = sanitise_framerate(target_framerate);
         }
         else
         {
            /* Run the manual path through the same sanitiser as
             * 'auto'.  atof returns 0.0 for unparseable input
             * (which a misconfigured frontend or a hand-edited
             * options file can supply -- the core-options system
             * doesn't strictly enforce the enumerated set we
             * advertise), and 0.0 would propagate to
             * frametime_usec = 1000.0f / 0 = +inf and
             * shm->samples_per_frame = (int)(audio_samplerate /
             * 0) which is an undefined float->int cast.  Snap to
             * the closest entry in supported_framerates[] just
             * like the auto branch above so the rest of this
             * function and SNDDMA_Init never see a bad value. */
            framerate = sanitise_framerate((float)atof(var.value));
         }
      }
      else
         framerate = 60.0f;

      frametime_usec = 1000.0f / framerate;

      /* Resolve the sound-samplerate hint now that framerate is known (it is
       * the "auto" fallback).  Supersedes the old framerate-only derivation. */
      update_audio_samplerate();
   }

   var.key = "tyrquake_resolution";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value
       && !initial_resolution_set)
   {
      char *pch;
      char str[100];
      unsigned long w_in = 0, h_in = 0;
      snprintf(str, sizeof(str), "%s", var.value);

      pch = strtok(str, "x");
      if (pch)
         w_in = strtoul(pch, NULL, 0);
      pch = strtok(NULL, "x");
      if (pch)
         h_in = strtoul(pch, NULL, 0);

      /* Clamp to the renderer's compile-time scratch limits.
       * VID_Init does the final clamp before allocating, but
       * doing it here keeps the values reasonable in case other
       * code reads the globals between now and VID_Init. */
      if (w_in == 0)            w_in = 320;
      if (h_in == 0)            h_in = 200;
      if (w_in > MAXWIDTH)      w_in = MAXWIDTH;
      if (h_in > MAXHEIGHT)     h_in = MAXHEIGHT;

      width  = (unsigned)w_in;
      height = (unsigned)h_in;

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "Got size: %u x %u.\n", width, height);

      initial_resolution_set = true;
   }

   var.key = "tyrquake_rumble";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         rumble_enabled = false;
      else
         rumble_enabled = true;
   }

   if (!rumble_enabled)
   {
      retro_set_rumble_damage(0);
      retro_set_rumble_touch(0, 0.0f);
   }

   /* Phase 5 scaffolding: read the user's compute-vs-
    * graphics preference for the 3D view.  Marked as
    * requires-restart in the core-options manifest --
    * subsequent re-reads here (when the user toggles
    * the option mid-run) update the global but the
    * backend's already-allocated resources aren't torn
    * down, so the active session keeps the path it
    * stood up at retro_load_game time.  The global is
    * declared in rhi.h and defaults to `true` if this
    * read fails or returns an unknown value. */
   var.key = "tyrquake_compute_rendering";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         g_rhi_compute_rendering = false;
      else
         g_rhi_compute_rendering = true;
   }

   var.key = "tyrquake_invert_y_axis";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         invert_y_axis = 1;
      else
         invert_y_axis = -1;
   }
   
   var.key = "tyrquake_analog_deadzone";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		int pct = atoi(var.value);
		/* The core-options manifest advertises 0..30%, but
		 * frontends don't strictly enforce the enumerated set
		 * and a hand-edited options file can supply any
		 * string.  atoi returns 0 for unparseable input
		 * (harmless: deadzone disabled) but also accepts
		 * values >= 100 verbatim.  At pct == 100,
		 * analog_deadzone == ANALOG_RANGE == 32767 and the
		 * subsequent IN_Move divisor 'ANALOG_RANGE -
		 * analog_deadzone' becomes 0.  The deadzone gate
		 * 'lsx > analog_deadzone || lsx < -analog_deadzone'
		 * normally screens us off at that point (lsx can't
		 * exceed 32767), but at lsx == -32768 (stick
		 * physically fully one direction) the second
		 * comparison fires and we land on a
		 * 'cl_sidespeed.value * lsx / 0' divide-by-zero in
		 * the float math.  At pct > 100, analog_deadzone >
		 * ANALOG_RANGE and the divisor goes negative, which
		 * silently inverts the stick mapping rather than
		 * crashing -- still wrong behaviour.  Clamp pct so
		 * the divisor stays strictly positive. */
		if (pct < 0)  pct = 0;
		if (pct > 99) pct = 99;
		analog_deadzone = (int)(pct * 0.01f * ANALOG_RANGE);
   }

}

static void update_env_variables(void)
{
   const char *default_username = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_USERNAME, &default_username))
   {
      if (default_username && default_username[0] != '\0')
      {
         char setplayer[256];
         snprintf(setplayer, sizeof(setplayer), "name %s", default_username);
         retro_cheat_set(0, true, setplayer);
      }
   }
}

byte *vid_buffer;
short *zbuffer;
short *finalimage;
byte* surfcache;

static void audio_step(void);

/* did_flip is non-static so HW backends (backend_vulkan.c) can
 * set it from their end_frame after handing a frame to the
 * frontend via video_cb(RETRO_HW_FRAME_BUFFER_VALID, ...) --
 * suppresses the dupe-last-frame path below in retro_run. */
bool did_flip;

void retro_run(void)
{
   bool updated = false;

   perf_timing_section_begin(PERF_SECTION_FRAME);

   did_flip = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables(false);
   if (!has_set_username)
   {
      update_env_variables();
      has_set_username = true;
   }

   /* HW backend per-frame acquire.  The Vulkan backend
    * uses this to rotate its N-slot ring (waits on the
    * rotated-to slot's done_fence + resets it, reassigns
    * vk_cmd_buffer to that slot's CB).  SW backend's
    * begin_frame is a no-op.  This MUST run before
    * Host_Frame: draw_view inside Host_Frame writes into
    * the slot's host-visible mapped SSBOs (alias /
    * sprite / particles / turb / sky dispatch builders)
    * and the slot identity is set by begin_frame.
    * Without this call vk_active_slot stays at 0 forever
    * and the N-buffering does nothing -- which was fine
    * pre-step-3b (end_frame's QueueWaitIdle was the real
    * sync) but breaks cross-frame correctness now that
    * the wait is gone. */
   if (g_rhi && g_rhi->kind != RHI_BACKEND_SOFTWARE && g_rhi->begin_frame)
      g_rhi->begin_frame();

   perf_timing_section_begin(PERF_SECTION_RENDERVIEW);
   Host_Frame(1.0 / framerate);
   perf_timing_section_end(PERF_SECTION_RENDERVIEW);

   if (rumble_touch_counter > -1)
   {
      rumble_touch_counter--;

      if (rumble_touch_counter == 0)
         retro_set_rumble_touch(0, 0.0f);
   }

   if (shutdown_core)
   {
      perf_timing_section_end(PERF_SECTION_FRAME);
      return;
   }

   /* HW backends present their frame here.  draw_view (called
    * during Host_Frame's SCR_UpdateScreen) recorded the per-
    * frame commands; end_frame submits them and hands the
    * resulting image to the frontend via the per-API
    * interface's set_image + a video_cb call with
    * RETRO_HW_FRAME_BUFFER_VALID.  end_frame is responsible
    * for setting did_flip so the dupe path below is
    * suppressed.  The SW backend's end_frame is a no-op;
    * its present path runs inside VID_Update earlier in
    * SCR_UpdateScreen. */
   if (g_rhi && g_rhi->kind != RHI_BACKEND_SOFTWARE && g_rhi->end_frame)
      g_rhi->end_frame();

   if (!did_flip)
      video_cb(NULL, width, height, width << 1); /* dupe */
   /* Light decay is per-frame, not audio.  Was
    * historically bundled into audio_step because
    * that was the "do this once per frame" hook,
    * but it has nothing to do with the mixer and
    * must always run regardless of any audio
    * skip-path. */
   if (cls.state == ca_active)
      CL_DecayLights();
   perf_timing_section_begin(PERF_SECTION_AUDIO);
   audio_step();
   perf_timing_section_end(PERF_SECTION_AUDIO);

   perf_timing_section_end(PERF_SECTION_FRAME);
   perf_timing_end_frame();
}

static void extract_directory(char *out_dir, const char *in_dir, size_t size)
{
   size_t len;

   fill_pathname_parent_dir(out_dir, in_dir, size);

   /* Remove trailing slash, if required */
   len = strlen(out_dir);
   if ((len > 0) &&
       (out_dir[len - 1] == PATH_DEFAULT_SLASH_C()))
      out_dir[len - 1] = '\0';

   /* If parent directory is an empty string,
    * must set it to '.' */
   if (string_is_empty(out_dir))
      strlcpy(out_dir, ".", size);
}

bool retro_load_game(const struct retro_game_info *info)
{
   unsigned i;
   char g_rom_dir[PATH_MAX_LENGTH];
   char g_save_dir[PATH_MAX_LENGTH];
   char cfg_file[PATH_MAX_LENGTH];
   char *path_lower = NULL;
   quakeparms_t parms;
   bool use_external_savedir = false;
   const char *base_save_dir = NULL;
   struct retro_keyboard_callback cb = { keyboard_cb };

   g_rom_dir[0] = '\0';
   g_save_dir[0] = '\0';
   cfg_file[0] = '\0';

   if (!info)
      return false;

   path_lower = strdup(info->path);

   for (i=0; path_lower[i]; ++i)
       path_lower[i] = tolower(path_lower[i]);

   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);

   update_variables(true);

   extract_directory(g_rom_dir, info->path, sizeof(g_rom_dir));

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &base_save_dir) && base_save_dir)
   {
		if (!string_is_empty(base_save_dir))
		{
			/* Get game 'name' (i.e. subdirectory) */
			const char *game_name = path_basename(g_rom_dir);

			/* > Build final save path */
			fill_pathname_join(g_save_dir, base_save_dir, game_name, sizeof(g_save_dir));
			use_external_savedir = true;
			
			/* > Create save directory, if required */
			if (!path_is_directory(g_save_dir))
			{
				use_external_savedir = path_mkdir(g_save_dir);
			}
		}
   }
   /* > Error check */
   if (!use_external_savedir)
   {
		/* > Use ROM directory fallback... */
		strlcpy(g_save_dir, g_rom_dir, sizeof(g_save_dir));
	}
	else
	{
		/* > Final check: is the save directory the same as the 'rom' directory? */
		/*   (i.e. ensure logical behaviour if user has set a bizarre save path...) */
		use_external_savedir = (strcmp(g_save_dir, g_rom_dir) != 0);
	}

   if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
      log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
   else
      log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");

   MEMSIZE_MB = DEFAULT_MEMSIZE_MB;

   if ( strstr(path_lower, "id1") ||
        strstr(path_lower, "quoth") ||
        strstr(path_lower, "hipnotic") ||
        strstr(path_lower, "rogue") )
   {
      char tmp_dir[PATH_MAX_LENGTH];
      tmp_dir[0] = '\0';

#if (defined(HW_RVL) && !defined(WIIU)) || defined(_XBOX1)
      MEMSIZE_MB = 16;
#endif
      extract_directory(tmp_dir, g_rom_dir, sizeof(tmp_dir));
      strlcpy(g_rom_dir, tmp_dir, sizeof(g_rom_dir));
   }

   memset(&parms, 0, sizeof(parms));

   parms.argc = 1;
   parms.basedir = g_rom_dir;
   parms.savedir = g_save_dir;
   parms.use_exernal_savedir = use_external_savedir ? 1 : 0;

   /* Size the Quake hunk from the memory the frontend actually has, when it
    * can tell us. We only ever grow above the default and clamp hard, so a
    * frontend that does not implement the query (returns false) keeps the
    * compile-time default computed above. */
   {
      struct retro_memory_status memstat;
      memstat.free = memstat.total = 0;
      if (environ_cb(RETRO_ENVIRONMENT_GET_MEMORY_STATUS, &memstat) && memstat.free)
      {
         unsigned free_mb = (unsigned)(memstat.free / (1024 * 1024));
         unsigned budget  = free_mb >> HUNK_FREE_SHIFT;
         if (budget > HUNK_MAX_MB)
            budget = HUNK_MAX_MB;
         if (budget > MEMSIZE_MB)
            MEMSIZE_MB = budget;
         log_cb(RETRO_LOG_INFO,
                "Frontend reports %u MB free; sizing Quake hunk to %u MB.\n",
                free_mb, MEMSIZE_MB);
      }
      else
         log_cb(RETRO_LOG_INFO,
                "Frontend has no memory-status query; using default %u MB hunk.\n",
                MEMSIZE_MB);
   }

   parms.memsize = MEMSIZE_MB * 1024 * 1024;
   argv[0] = empty_string;

   if (strstr(path_lower, "rogue"))
   {
      parms.argc++;
      argv[1] = "-rogue";
   }
   else if (strstr(path_lower, "hipnotic"))
   {
      parms.argc++;
      argv[1] = "-hipnotic";
   }
   else if (strstr(path_lower, "quoth"))
   {
      parms.argc++;
      argv[1] = "-quoth";
   }
   else if (!strstr(path_lower, "id1"))
   {
      const char *basename = path_basename(g_rom_dir);
      char tmp_dir[PATH_MAX_LENGTH];
      tmp_dir[0] = '\0';

      parms.argc++;
      argv[1] = "-game";
      parms.argc++;
      argv[2] = basename;

      extract_directory(tmp_dir, g_rom_dir, sizeof(tmp_dir));
      strlcpy(g_rom_dir, tmp_dir, sizeof(g_rom_dir));
   }

   free(path_lower);
   path_lower = NULL;

   parms.argv = argv;

   COM_InitArgv(parms.argc, parms.argv);

   parms.argc = com_argc;
   parms.argv = com_argv;

   heap = (unsigned char*)malloc(parms.memsize);

   parms.membase = heap;

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "Quake Libretro -- TyrQuake Version %s\n", stringify(TYR_VERSION));

   if (!Host_Init(&parms))
   {
      struct retro_message msg;
      char msg_local[256];

      Host_Shutdown();

      snprintf(msg_local, sizeof(msg_local),
            "PAK archive loading failed...");
      msg.msg    = msg_local;
      msg.frames = 360;
      if (environ_cb)
         environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, (void*)&msg);
      return false;
   }

   {
      static const struct retro_memory_descriptor descriptors[] = {
         { 0, cl.stats, 0, 0, 0, 0, sizeof(cl.stats), "SRAM" },
         { 0, &cl.intermission, 0, sizeof(cl.stats), 0, 0,
            sizeof(cl.intermission) + sizeof(cl.completed_time), "SRAM" },
      };
      static const struct retro_memory_map memory_map = {
         descriptors, sizeof(descriptors) / sizeof(descriptors[0])
      };
      environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, (void*)&memory_map);
   }

   /* Override some default binds with more modern ones if we are booting the 
    * game for the first time. */
   fill_pathname_join(cfg_file, g_save_dir, "config.cfg", sizeof(cfg_file));

   if (!path_is_valid(cfg_file))
   {
       Cvar_Set("gamma", "0.95");
       Cmd_ExecuteString("bind ' \"toggleconsole\"", src_command);
       Cmd_ExecuteString("bind ~ \"toggleconsole\"", src_command);
       Cmd_ExecuteString("bind ` \"toggleconsole\"", src_command);

       Cmd_ExecuteString("bind f \"+moveup\"", src_command);
       Cmd_ExecuteString("bind c \"+movedown\"", src_command);

       Cmd_ExecuteString("bind a \"+moveleft\"", src_command);
       Cmd_ExecuteString("bind d \"+moveright\"", src_command);
       Cmd_ExecuteString("bind w \"+forward\"", src_command);
       Cmd_ExecuteString("bind s \"+back\"", src_command);

       Cmd_ExecuteString("bind e \"impulse 10\"", src_command);
       Cmd_ExecuteString("bind q \"impulse 12\"", src_command);
   }

   Cmd_ExecuteString("bind AUX1 \"+moveright\"", src_command);
   Cmd_ExecuteString("bind AUX2 \"+moveleft\"", src_command);
   Cmd_ExecuteString("bind AUX3 \"+back\"", src_command);
   Cmd_ExecuteString("bind AUX4 \"+forward\"", src_command);
   Cmd_ExecuteString("bind AUX5 \"+right\"", src_command);
   Cmd_ExecuteString("bind AUX6 \"+left\"", src_command);
   Cmd_ExecuteString("bind AUX7 \"+lookup\"", src_command);
   Cmd_ExecuteString("bind AUX8 \"+lookdown\"", src_command);

   /* Stand up the renderer backend selected by the
    * 'tyrquake_renderer' core option.  Must run after
    * update_variables (so the option is in the cache) and
    * after Host_Init (so the SW renderer's globals exist if
    * software is the active backend).  Failure here is
    * survivable: V_RenderView falls back to a direct
    * R_RenderView call when g_rhi is unset, so an init
    * failure degrades to SW behaviour rather than crashing
    * the load. */
   if (!rhi_init())
      log_cb(RETRO_LOG_WARN,
             "rhi_init failed -- falling back to inline R_RenderView dispatch\n");

   /* Phase 4m: bypass Quake's vid.numpages-gated update
    * counters.  Quake has five places (sbar.c::Sbar_Draw,
    * screen.c::SCR_UpdateScreen @scr_fullupdate,
    * ::clearconsole / ::clearnotify blocks, and
    * ::SCR_EraseCenterString) that use the idiom
    *
    *   if (counter++ < vid.numpages) { redraw stuff; }
    *
    * to stop re-drawing after `vid.numpages` frames.  In
    * the SW double-buffered era this was sound: each
    * page-flip handed the renderer a buffer whose status-
    * bar / border / centerstring region either already
    * held the previous draw (no work needed) or had just
    * been swapped in fresh (one draw catches it up).  In
    * libretro's single-buffered model the assumption
    * breaks two ways:
    *
    *   1. SW backend: vid.numpages = 1, so Sbar_Draw /
    *      Draw_TileClear / etc. run exactly once and
    *      never again until a state change resets the
    *      counter.  Works as long as nothing overwrites
    *      the status-bar / border region of vid.buffer.
    *      But when scr_viewsize is high enough that
    *      R_RenderView's 3D viewport extends into the
    *      status-bar area, R_RenderView paints the
    *      viewport edge on top of the previously-drawn
    *      inventory icons, and the optimization keeps
    *      Sbar_Draw from re-painting them.  User-visible:
    *      'i don't think selecting the weapon in the HUD
    *      is often accurately shown in software'.  Same
    *      story for the SCR_UpdateScreen full-screen
    *      Draw_TileClear that paints the wood-grain
    *      borders -- once one frame paints them, the
    *      optimization stops repainting, and the first
    *      thing that disturbs the border strips (e.g.
    *      the fresh malloc memory from a VID_Init
    *      re-alloc) shows through.
    *
    *   2. Vulkan backend (Phase 4l): queue_2d_pic
    *      rebuilds the per-frame overlay queue from
    *      scratch every retro_run.  When Sbar_Draw
    *      early-returns because sb_updates >=
    *      vid.numpages, zero Draw_Pic calls fire, the
    *      queue stays empty, and the HUD vanishes for
    *      that frame.
    *
    * Both paths want the same thing: the redraw should
    * happen every frame, not just N times.  Setting
    * vid.numpages to 0x40000000 (~1 billion, ~8 years
    * at 120 FPS, comfortably below INT_MAX so the
    * existing post-increments don't overflow) lets every
    * `counter++ < vid.numpages` stay true for the
    * lifetime of any plausible session.  The override
    * runs unconditionally (regardless of which backend
    * rhi_init picked) because the underlying assumption
    * the optimization makes -- that page-flipping
    * provides natural redundancy -- is invalid for both
    * backends, not just the Vulkan one. */
   vid.numpages = 0x40000000;

   /* Negotiate float audio output once, now that the game is loaded and the
    * audio path is up (Host_Init -> SNDDMA_Init ran above).  If the frontend
    * supports it we commit to float for this game's lifetime; otherwise the
    * int16 path is used unchanged.  Contract: negotiate once per loaded game,
    * never mix formats.  Older frontends return false here, so they keep the
    * int16 path transparently. */
   use_float_output     = 0;
   audio_batch_cb_float = NULL;
   {
      struct retro_audio_sample_float_callback fcb;
      fcb.batch = NULL;
      if (environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_SAMPLE_BATCH_FLOAT, &fcb)
            && fcb.batch)
      {
         audio_batch_cb_float = fcb.batch;
         use_float_output     = 1;
      }
   }
   s_float_output = use_float_output;

   return true;
}



void retro_unload_game(void)
{
   /* Tear down float-output negotiation; a fresh retro_load_game re-negotiates
    * against whatever frontend loads the next game. */
   audio_batch_cb_float = NULL;
   use_float_output     = 0;
   s_float_output       = 0;
   snd_float_buffer     = NULL;

   rhi_shutdown();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   if (id == RETRO_MEMORY_SYSTEM_RAM)
      return cl.stats;

   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   if (id == RETRO_MEMORY_SYSTEM_RAM)
      return sizeof(cl.stats);

   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;

   if (code)
      Cmd_ExecuteString(code, src_command);
}

/*
 * VIDEO
 */

unsigned short d_8to16table[256];
unsigned       d_8to24table_shifted[256];

#define MAKECOLOR(r, g, b) (((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3))


void VID_SetPalette(unsigned char *palette)
{
   unsigned        i, j;
   unsigned short *pal16 = &d_8to16table[0];
   unsigned       *pal32 = &d_8to24table_shifted[0];

   /* Populate both the 16bpp (RGB565) lookup the SW present
    * path uses in VID_Update and the 32bpp (RGBA8) lookup
    * HW backends like backend_vulkan.c use to display
    * vid.buffer.  Keeping both in sync means palette shifts
    * (damage flash, bonus flash, underwater, quad damage)
    * propagate to whichever backend is active.
    *
    * d_8to24table_shifted byte order matches VK_FORMAT_
    * R8G8B8A8_UNORM: bytes [r, g, b, ff] in memory, which
    * is 0xFF000000 | (b<<16) | (g<<8) | r as a little-
    * endian uint32. */
   for (i = 0, j = 0; i < 256; i++, j += 3)
   {
      *pal16++ = MAKECOLOR(palette[j], palette[j+1], palette[j+2]);
      *pal32++ = 0xFF000000u
              | ((unsigned)palette[j+2] << 16)
              | ((unsigned)palette[j+1] <<  8)
              | ((unsigned)palette[j+0] <<  0);
   }
}

unsigned 	d_8to24table[256];


void	VID_SetPalette2 (unsigned char *palette)
{
	unsigned r,g,b;
	unsigned v;
	unsigned short i;
	byte *pal       = palette;
	unsigned *table = d_8to24table;

	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		if (r>255) r = 255;
		if (g>255) g = 255;
		if (b>255) b = 255;
		pal += 3;
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
		
	}

	
	d_8to24table[255] &= 0xffffff;	/* 255 is transparent */
	d_8to24table[0] &= 0x000000;	/* black is black */

}

void VID_ShiftPalette(unsigned char *palette)
{
   VID_SetPalette(palette);
   Sbar_Changed();
}

void VID_Init(unsigned char *palette)
{
   size_t fb_pixels;

   /* Clamp to the renderer's compile-time scratch limits.  The
    * fixed-pipeline rasterizer indexes per-row state from
    * vid.rowbytes and walks vid.height rows; oversize buffers
    * past MAXWIDTH x MAXHEIGHT (1920 x 1200) cause out-of-range
    * writes into static r_main.c arrays (zspantable, scantable,
    * r_warpbuffer_storage etc., see commit 60dacb5).  Undersize
    * buffers (zero or near-zero) make every subsequent render
    * line-write a stomp into whatever malloc returned for a
    * 0-byte alloc.
    *
    * width / height come from the libretro 'tyrquake_resolution'
    * variable parsed at startup; clamping here is the
    * defence-of-last-resort in case the frontend hands us
    * a bad value, and also catches any future code path that
    * sets width/height from another untrusted source. */
   if (width  < 320)         width  = 320;
   if (height < 200)         height = 200;
   if (width  > MAXWIDTH)    width  = MAXWIDTH;
   if (height > MAXHEIGHT)   height = MAXHEIGHT;

   /* width and height are now bounded; width * height fits
    * comfortably in size_t (max 1920*1200 = 2.3M pixels = 4.6M
    * bytes for short, well under 32-bit).  Compute pixels in
    * size_t to avoid any chance of the multiply truncating to
    * a smaller size before the malloc cast. */
   fb_pixels = (size_t)width * (size_t)height;

   /* Guard against being re-entered (resolution change, soft
    * reset etc.) by freeing any pre-existing buffers first.
    * Without this, retro_reset paths leak the old buffers and
    * the various d_* / vid.* aliases keep dangling pointers
    * to free()d memory. */
   if (vid_buffer)  { free(vid_buffer);  vid_buffer  = NULL; }
   if (zbuffer)     { free(zbuffer);     zbuffer     = NULL; }
   if (finalimage)  { free(finalimage);  finalimage  = NULL; }
   if (surfcache)   { free(surfcache);   surfcache   = NULL; }

   vid_buffer = (byte*)malloc(fb_pixels * sizeof(byte));
   zbuffer    = (short*)malloc(fb_pixels * sizeof(short));
   finalimage = (short*)malloc(fb_pixels * sizeof(short));
   surfcache  = (byte*)malloc(SURFCACHE_SIZE);

   if (!vid_buffer || !zbuffer || !finalimage || !surfcache)
      Sys_Error("VID_Init: failed to allocate framebuffer "
                "(width=%u height=%u)", width, height);

    vid.width = width;
    vid.height = height;
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.conwidth = vid.width;
    vid.conheight = vid.height;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
    vid.buffer = vid.conbuffer = vid_buffer;
    vid.rowbytes = width;
    vid.conrowbytes = vid.rowbytes;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);

    d_pzbuffer = zbuffer;
    D_InitCaches(surfcache, SURFCACHE_SIZE);
}

void VID_Shutdown(void)
{
   if (vid_buffer)
      free(vid_buffer);
   if (zbuffer)
      free(zbuffer);
   if (finalimage)
      free(finalimage);
   if (surfcache)
      free(surfcache);
   vid_buffer = NULL;
   zbuffer    = NULL;
   finalimage = NULL;
   surfcache  = NULL;
}

void VID_Update(vrect_t *rects)
{
   unsigned x, y;
   unsigned pitch;
   uint8_t *ilineptr           = (uint8_t*)vid.buffer;
   uint16_t *olineptr;
   uint16_t *pal               = (uint16_t*)&d_8to16table;
   uint16_t *ptr;

   if (!video_cb || !rects || did_flip)
      return;

   /* When a HW backend is active, the SW pixel buffer
    * (vid.buffer) is not the source of truth -- the active
    * backend is rendering through its own command stream
    * and will call video_cb with RETRO_HW_FRAME_BUFFER_VALID
    * from its own end_frame.  Palette-converting stale
    * vid.buffer contents here and pushing them to video_cb
    * would either fight the HW backend for the video output
    * (interleaved SW + HW frames) or simply present
    * unrelated SW frames that the user never asked for.
    * Bail out of the SW present path and let the HW
    * backend own the video callback.
    *
    * Phase 3b: HW backends do not yet call video_cb either,
    * so the frontend's frame-dupe path takes over (the
    * 'if (!did_flip)' check in retro_run does
    * video_cb(NULL, ...)).  This produces a frozen frame
    * while Vulkan is active -- expected for the current
    * phase; Phase 3c lands the actual HW-backed frame
    * delivery and the frozen behaviour resolves. */
   if (g_rhi && g_rhi->kind != RHI_BACKEND_SOFTWARE)
      return;

   /* Palette-convert our 8bpp framebuffer into the 16bpp finalimage
    * buffer and hand that to video_cb. We deliberately do NOT use
    * RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER: the SW path
    * still wants vid.buffer to be a stable, persistent buffer across
    * frames -- the libretro frontend's swapchain buffer is not, so
    * direct-rendering into it produced visible cross-buffer artifacts
    * (e.g. a doubled status bar from previous-frame content showing
    * through unwritten regions).  The vid.numpages-gated partial-
    * update optimizations the original code relied on (sb_updates,
    * scr_fullupdate, clearnotify, etc.) are now bypassed at
    * retro_load_game (Phase 4m sets vid.numpages to 0x40000000),
    * because they assume page-flip redundancy that libretro's
    * single-buffered model never provides -- and that assumption
    * also breaks the moment R_RenderView's 3D viewport extends
    * into the status-bar area at high scr_viewsize, overwriting
    * the inventory icons between Sbar_Draw runs.  The right way
    * to claim the sw_fb speedup is to convert the renderer to
    * write 16bpp natively; until then, this single post-pass is
    * correct. */
   pitch    = width;
   ptr      = (uint16_t*)finalimage;
   olineptr = ptr;

   for (y = 0; y < rects->height; ++y)
   {
      for (x = 0; x < rects->width; ++x)
         *olineptr++ = pal[*ilineptr++];
      olineptr += pitch - rects->width;
   }

   video_cb(ptr, width, height, pitch << 1);
   did_flip = true;
}

qboolean VID_IsFullScreen(void)
{
    return true;
}

void VID_LockBuffer(void)
{
}

void VID_UnlockBuffer(void)
{
}

void D_BeginDirectRect(int x, int y, const byte *pbitmap, int w, int h)
{
   (void)x; (void)y; (void)pbitmap; (void)w; (void)h;
}

void D_EndDirectRect(int x, int y, int w, int h)
{
   (void)x; (void)y; (void)w; (void)h;
}

/*
 * SOUND (TODO)
 */

/*
 * Per-frame audio: mix the engine's channels into
 * audio_out_buffer, then push that buffer to the
 * frontend.  Called once at the end of every
 * retro_run, after Host_Frame has finished the game
 * tick (so any sounds triggered this frame are
 * already in the channel array when we mix them).
 *
 * The "audio_callback" / "audio_process" split was
 * organizational only -- nothing else ever called
 * either half, and the "callback" naming was
 * misleading (this is a regular synchronous call,
 * not the async push model that
 * RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK provides).
 */
static void audio_step(void)
{
   unsigned audio_frames_remaining;
   int16_t *audio_out_ptr;
   int      av_flags = RETRO_AV_ENABLE_VIDEO | RETRO_AV_ENABLE_AUDIO;
   qboolean push_audio;

   /* RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE lets
    * the frontend tell us when audio output isn't
    * needed.  Two cases are interesting here:
    *
    * - HARD_DISABLE_AUDIO: a runahead secondary
    *   core whose audio is never consumed.  The
    *   docs allow skipping audio synthesis
    *   entirely "without compromising emulation
    *   accuracy".  In tyrquake the mixer state is
    *   purely output -- it does not feed back into
    *   game state -- so we can short-circuit the
    *   whole step.
    *
    * - AUDIO cleared (without HARD_DISABLE):
    *   frontend will discard the samples this
    *   frame (e.g. fast-forward).  We still update
    *   engine state (channel positions, BGM
    *   stream cursor, channel remaining_samples
    *   countdowns) so the next non-skipped frame
    *   mixes from the right position; we just
    *   don't push the buffer.
    *
    * Older frontends that don't implement env 47
    * fall through with all flags set as default. */
   environ_cb(RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE, &av_flags);
   if (av_flags & RETRO_AV_ENABLE_HARD_DISABLE_AUDIO)
      return;
   push_audio = (av_flags & RETRO_AV_ENABLE_AUDIO) != 0;

   /* Mix.  S_Update does the spatialization /
    * volume falloff for the listener position; the
    * BGM and CD updaters feed raw streaming samples
    * into the same paintbuffer.  S_Update_ at the
    * tail of S_Update writes exactly samples_per_
    * frame stereo frames into shm->buffer ==
    * audio_out_buffer. */
   BGM_Update();
   if (cls.state == ca_active)
      S_Update(r_origin, vpn, vright, vup);
   else
      S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);
   CDAudio_Update();

   if (!push_audio)
      return;

   /* Push.  At very low framerates one video frame's
    * worth of audio can exceed the frontend's
    * internal batch limit; chunk if so.  At 60fps /
    * 44.1kHz the count is 735 stereo frames -- well
    * below any reasonable limit -- and the loop runs
    * once.  On a partial write (frontend returned
    * fewer frames than asked), shrink our cap and
    * advance only by what was actually consumed --
    * the unwritten tail will be retried in the next
    * loop iteration. */
   audio_frames_remaining = audio_samplerate / framerate;

   /* Float output path: push the engine's normalized-float buffer through the
    * negotiated float batch callback.  Identical chunking / partial-write
    * backpressure to the int16 path below -- only the buffer type and the
    * callback differ. */
   if (use_float_output)
   {
      float *audio_out_ptr_f = audio_out_buffer_f;
      do
      {
         unsigned audio_frames_to_write =
               (audio_frames_remaining > audio_batch_frames_max) ?
                     audio_batch_frames_max : audio_frames_remaining;
         unsigned audio_frames_written  =
               audio_batch_cb_float(audio_out_ptr_f, audio_frames_to_write);

         if (audio_frames_written == 0)
            break;	/* frontend can't accept any more this frame */

         if (audio_frames_written < audio_frames_to_write)
            audio_batch_frames_max = audio_frames_written;

         audio_frames_remaining -= audio_frames_written;
         audio_out_ptr_f        += audio_frames_written << 1;
      }
      while (audio_frames_remaining > 0);
      return;
   }

   audio_out_ptr          = audio_out_buffer;
   do
   {
      unsigned audio_frames_to_write =
            (audio_frames_remaining > audio_batch_frames_max) ?
                  audio_batch_frames_max : audio_frames_remaining;
      unsigned audio_frames_written  =
            audio_batch_cb(audio_out_ptr, audio_frames_to_write);

      if (audio_frames_written == 0)
         break;	/* frontend can't accept any more this frame */

      if (audio_frames_written < audio_frames_to_write)
         audio_batch_frames_max = audio_frames_written;

      audio_frames_remaining -= audio_frames_written;
      audio_out_ptr          += audio_frames_written << 1;
   }
   while (audio_frames_remaining > 0);
}

qboolean SNDDMA_Init(dma_t *dma)
{
   shm                    = dma;
   shm->speed             = audio_samplerate;
   /* Stereo frames per video frame.  audio_samplerate
    * and framerate are both set by update_variables()
    * which runs before this in retro_load_game.
    * S_Update_ uses this to mix exactly one video
    * frame's worth of audio per call, matching what
    * audio_step drains. */
   shm->samples_per_frame = (int)(audio_samplerate / framerate);
   /* Engine writes mixed audio directly into our
    * linear output buffer; audio_step then hands
    * that buffer straight to audio_batch_cb. */
   shm->buffer            = (unsigned char *volatile)audio_out_buffer;

   /* Float output buffer shares the same one-frame linear layout.  Hand the
    * engine its pointer; s_float_output is (re)asserted from the negotiation
    * result so a reload picks up the current frontend's capability. */
   snd_float_buffer       = audio_out_buffer_f;
   s_float_output         = use_float_output;

   return true;
}

/*
 * CD (TODO)
 */

int CDDrv_IsAudioTrack(byte track) { return 0; }
int CDDrv_PlayTrack(byte track) { return 1; }
int CDDrv_IsPlaying(byte track) { return 0; }
int CDDrv_InitDevice(void) { return -1; }
void CDDrv_CloseDevice(void) { }
void CDDrv_Eject(void) { }
void CDDrv_CloseDoor(void) { }
void CDDrv_Stop(void) { }
void CDDrv_Pause(void) { }
void CDDrv_Resume(byte track) { }
int CDDrv_GetMaxTrack(byte *track) { return 0; }

/*
 * INPUT (TODO)
 */
static void windowed_mouse_f(struct cvar_s *var)
{
}

cvar_t _windowed_mouse = { "_windowed_mouse", "0", true, false, 0, windowed_mouse_f };

void
IN_Init(void)
{
   int i;
   for (i = 0; i < MAX_PADS; i++)
      quake_devices[i] = RETRO_DEVICE_JOYPAD;
}

void
IN_Shutdown(void)
{
}

void
IN_Commands(void)
{
}

void
IN_Move(usercmd_t *cmd)
{
   int mx, my, lsx, lsy, rsx, rsy;

   if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
      mx = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      my = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

      if (mx || my)
      {
         mx *= sensitivity.value;
         my *= sensitivity.value;

         cl.viewangles[YAW] -= m_yaw.value * mx;

         V_StopPitchDrift();

         cl.viewangles[PITCH] += m_pitch.value * my;

         if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
         if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
      }
   } else if (quake_devices[0] != RETRO_DEVICE_NONE && quake_devices[0] != RETRO_DEVICE_KEYBOARD) {
      /* Left stick move */
      lsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
               RETRO_DEVICE_ID_ANALOG_X);
      lsy = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (lsx > analog_deadzone || lsx < -analog_deadzone) {
         if (lsx > analog_deadzone)
            lsx = lsx - analog_deadzone;
         if (lsx < -analog_deadzone)
            lsx = lsx + analog_deadzone;
         cmd->sidemove += cl_sidespeed.value * lsx / (ANALOG_RANGE - analog_deadzone);
      }

      if (lsy > analog_deadzone || lsy < -analog_deadzone) {
         if (lsy > analog_deadzone)
            lsy = lsy - analog_deadzone;
         if (lsy < -analog_deadzone)
            lsy = lsy + analog_deadzone;
         cmd->forwardmove -= cl_forwardspeed.value * lsy / (ANALOG_RANGE - analog_deadzone);
      }

      /* Right stick Look */
      rsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_X);
      rsy = invert_y_axis * input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (rsx > analog_deadzone || rsx < -analog_deadzone)
      {
         if (rsx > analog_deadzone)
            rsx = rsx - analog_deadzone;
         if (rsx < -analog_deadzone)
            rsx = rsx + analog_deadzone;
         /* For now we are sharing the sensitivity with the mouse setting */
         cl.viewangles[YAW] -= (float)(sensitivity.value * rsx / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
      }

      V_StopPitchDrift();
      if (rsy > analog_deadzone || rsy < -analog_deadzone)
      {
         if (rsy > analog_deadzone)
            rsy = rsy - analog_deadzone;
         if (rsy < -analog_deadzone)
            rsy = rsy + analog_deadzone;
         cl.viewangles[PITCH] -= (float)(sensitivity.value * rsy / (ANALOG_RANGE - analog_deadzone)) / (framerate / 60.0f);
      }

      if (cl.viewangles[PITCH] > 80)
         cl.viewangles[PITCH] = 80;
      if (cl.viewangles[PITCH] < -70)
         cl.viewangles[PITCH] = -70;
   }
}

