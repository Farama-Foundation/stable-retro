#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition
 */

struct retro_core_option_v2_category option_cats_us[] = {
   {
      "system",
      "System",
      "Configure base hardware parameters: region, BIOS selection, memory cards, etc."
   },
   {
      "video",
      "Video",
      "Configure base display parameters."
   },
#ifdef GPU_NEON
   {
      "gpu_neon",
      "GPU Plugin",
      "Configure low-level settings of the NEON GPU plugin."
   },
#endif
#ifdef GPU_PEOPS
   {
      "gpu_peops",
      "GPU Plugin (Advanced)",
      "Configure low-level settings of the P.E.Op.S. GPU plugin."
   },
#endif
#ifdef GPU_UNAI
   {
      "gpu_unai",
      "GPU Plugin (Advanced)",
      "Configure low-level settings of the UNAI GPU plugin."
   },
#endif
   {
      "audio",
      "Audio",
      "Configure sound emulation: reverb, interpolation, CD audio decoding."
   },
   {
      "input",
      "Input",
      "Configure input devices: analog response, haptic feedback, Multitaps, light guns, etc."
   },
   {
      "compat_hack",
      "Compatibility Fixes",
      "Configure settings/workarounds required for correct operation of specific games."
   },
   {
      "speed_hack",
      "Speed Hacks (Advanced)",
      "Configure hacks that may improve performance at the expense of decreased accuracy/stability."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
   {
      "pcsx_rearmed_region",
      "Region",
      NULL,
      "Specify which region the system is from. 'NTSC' is 60 Hz while 'PAL' is 50 Hz. 'Auto' will detect the region of the currently loaded content. Games may run faster or slower than normal if the incorrect region is selected.",
      NULL,
      "system",
      {
         { "auto", "Auto" },
         { "NTSC", NULL },
         { "PAL",  NULL },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_bios",
      "BIOS Selection",
      NULL,
      "Specify which BIOS to use. 'Auto' will attempt to load a real bios file from the frontend 'system' directory, falling back to high level emulation if unavailable. 'HLE' forces high level BIOS emulation. It is recommended to use an official bios file for better compatibility.",
      NULL,
      "system",
      {
         { "auto", "Auto" },
         { "HLE",  NULL },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_show_bios_bootlogo",
      "Show BIOS Boot Logo",
      NULL,
      "When using an official BIOS file, specify whether to show the PlayStation logo upon starting or resetting content. Warning: Enabling the boot logo may reduce game compatibility.",
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_memcard2",
      "Second Memory Card (Shared)",
      NULL,
      "Emulate a second memory card in slot 2. This will be shared by all games.",
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#if defined(HAVE_CDROM) || defined(USE_ASYNC_CDROM)
#define V(x) { #x, NULL }
   {
      "pcsx_rearmed_cd_readahead",
      "CD read-ahead",
      NULL,
      "Reads the specified amount of sectors ahead of time to try to avoid later stalls. "
#ifdef HAVE_CDROM
      "Affects both physical CD-ROM and CD images. "
#endif
#if !defined(_3DS) && !defined(VITA)
      "333000 will try to read the complete disk (requires an additional 750MB of RAM)."
#endif
      ,
      NULL,
      "system",
      {
         V(0),  V(1),  V(2),  V(3),  V(4),  V(5),  V(6),  V(7),
         V(8),  V(9),  V(10), V(11), V(12), V(13), V(14), V(15),
         V(16), V(32), V(64), V(128), V(256), V(512), V(1024),
#if !defined(_3DS) && !defined(VITA)
	 V(333000),
#endif
         { NULL, NULL},
      },
      "12",
   },
#undef V
#endif
#ifndef DRC_DISABLE
   {
      "pcsx_rearmed_drc",
      "Dynamic Recompiler",
      NULL,
      "Dynamically recompile PSX CPU instructions to native instructions. Much faster than using an interpreter, but may be less accurate on some platforms.",
      NULL,
      "system",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
#if !defined(LIGHTREC) && defined(NDRC_THREAD)
   {
      "pcsx_rearmed_drc_thread",
      "DynaRec threading",
      NULL,
      "Run the dynarec on another thread.",
      NULL,
      "system",
      {
         { "auto", "Auto" },
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "auto",
   },
#endif
#endif // DRC_DISABLE
   {
      "pcsx_rearmed_psxclock",
      "PSX CPU Clock Speed (%)",
      NULL,
      "Overclock or under-clock the PSX CPU. Should be much less than 100 (something like 57) due to some real hardware slowdowns not being emulated. Usually should be left at 'Auto', else glitches or hangs are likely.",
      NULL,
      "system",
      {
         { "auto", "Auto" },
         { "30",  NULL },
         { "31",  NULL },
         { "32",  NULL },
         { "33",  NULL },
         { "34",  NULL },
         { "35",  NULL },
         { "36",  NULL },
         { "37",  NULL },
         { "38",  NULL },
         { "39",  NULL },
         { "40",  NULL },
         { "41",  NULL },
         { "42",  NULL },
         { "43",  NULL },
         { "44",  NULL },
         { "45",  NULL },
         { "46",  NULL },
         { "47",  NULL },
         { "48",  NULL },
         { "49",  NULL },
         { "50",  NULL },
         { "51",  NULL },
         { "52",  NULL },
         { "53",  NULL },
         { "54",  NULL },
         { "55",  NULL },
         { "56",  NULL },
         { "57",  NULL },
         { "58",  NULL },
         { "59",  NULL },
         { "60",  NULL },
         { "61",  NULL },
         { "62",  NULL },
         { "63",  NULL },
         { "64",  NULL },
         { "65",  NULL },
         { "66",  NULL },
         { "67",  NULL },
         { "68",  NULL },
         { "69",  NULL },
         { "70",  NULL },
         { "71",  NULL },
         { "72",  NULL },
         { "73",  NULL },
         { "74",  NULL },
         { "75",  NULL },
         { "76",  NULL },
         { "77",  NULL },
         { "78",  NULL },
         { "79",  NULL },
         { "80",  NULL },
         { "81",  NULL },
         { "82",  NULL },
         { "83",  NULL },
         { "84",  NULL },
         { "85",  NULL },
         { "86",  NULL },
         { "87",  NULL },
         { "88",  NULL },
         { "89",  NULL },
         { "90",  NULL },
         { "91",  NULL },
         { "92",  NULL },
         { "93",  NULL },
         { "94",  NULL },
         { "95",  NULL },
         { "96",  NULL },
         { "97",  NULL },
         { "98",  NULL },
         { "99",  NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_dithering",
      "Dithering Pattern",
      NULL,
      "Enable emulation of the dithering technique used by the PSX to smooth out color banding artifacts. \"Force\" enables it even if the game turns it off. Increases performance requirements.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { "force",    "Force" },
         { NULL, NULL },
      },
#if defined(_3DS)
      "disabled",
#else
      "enabled",
#endif
   },
#ifdef THREAD_RENDERING
   {
      "pcsx_rearmed_gpu_thread_rendering",
      "Threaded Rendering",
      NULL,
      "When enabled, runs GPU commands in a secondary thread. 'Synchronous' improves performance while maintaining proper frame pacing. 'Asynchronous' improves performance even further, but may cause dropped frames and increased latency. Produces best results with games that run natively at less than 60 frames per second.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "sync",     "Synchronous" },
         { "async",    "Asynchronous" },
         { NULL, NULL},
      },
      "disabled",
   },
#endif
   {
      "pcsx_rearmed_frameskip_type",
      "Frameskip",
      NULL,
      "Skip frames to avoid audio buffer under-run (crackling). Improves performance at the expense of visual smoothness. 'Auto' skips frames when advised by the frontend. 'Auto (Threshold)' utilises the 'Frameskip Threshold (%)' setting. 'Fixed Interval' utilises the 'Frameskip Interval' setting.",
      NULL,
      "video",
      {
         { "disabled",       NULL },
         { "auto",           "Auto" },
         { "auto_threshold", "Auto (Threshold)" },
         { "fixed_interval", "Fixed Interval" },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "pcsx_rearmed_frameskip_threshold",
      "Frameskip Threshold (%)",
      NULL,
      "When 'Frameskip' is set to 'Auto (Threshold)', specifies the audio buffer occupancy threshold (percentage) below which frames will be skipped. Higher values reduce the risk of crackling by causing frames to be dropped more frequently.",
      NULL,
      "video",
      {
         { "15", NULL },
         { "18", NULL },
         { "21", NULL },
         { "24", NULL },
         { "27", NULL },
         { "30", NULL },
         { "33", NULL },
         { "36", NULL },
         { "39", NULL },
         { "42", NULL },
         { "45", NULL },
         { "48", NULL },
         { "51", NULL },
         { "54", NULL },
         { "57", NULL },
         { "60", NULL },
         { NULL, NULL },
      },
      "33"
   },
   {
      "pcsx_rearmed_frameskip_interval",
      "Frameskip Interval",
      NULL,
      "Specify the maximum number of frames that can be skipped before a new frame is rendered.",
      NULL,
      "video",
      {
         { "1",  NULL },
         { "2",  NULL },
         { "3",  NULL },
         { "4",  NULL },
         { "5",  NULL },
         { "6",  NULL },
         { "7",  NULL },
         { "8",  NULL },
         { "9",  NULL },
         { "10", NULL },
         { NULL, NULL },
      },
      "3"
   },
   {
      "pcsx_rearmed_display_fps_v2",
      "Display Internal FPS",
      NULL,
      "Show the internal frame rate at which the emulated PlayStation system is rendering content. Note: Requires on-screen notifications to be enabled in the libretro frontend.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { "extra",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_fractional_framerate",
      "Use fractional frame rate",
      NULL,
      "Instead of the exact 50 or 60 (maximum) fps for PAL/NTSC the real console runs closer to something like 49.75 and 59.81fps (varies slightly between hw versions). PCSX-ReARMed uses the former \"round\" framerates to better match modern displays, however that may cause audio/video desync in games like DDR and Spyro 2 (intro). With this option you can try to use fractional framerates.",
      NULL,
      "video",
      {
         { "auto", "Auto" },
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_rgb32_output",
      "RGB32 output",
      NULL,
      "Improves color depth for true color modes (most FMVs and occasional title screens). Causes higher CPU usage due to double memory bandwidth requirement, even in 15bpp modes. Takes effect on game reload only (libretro limitation).",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_slow_llists",
      "(GPU) Slow linked list processing",
      NULL,
      "Slower but more accurate GPU linked list processing. Needed by only a few games like Vampire Hunter D. Should be autodetected in most cases.",
      NULL,
      "video",
      {
         { "auto", "Auto" },
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_show_overscan",
      "(GPU) Horizontal overscan",
      NULL,
      "The PSX can display graphics way into the horizontal borders, even if most screens would crop it. This option tries to display all such graphics. Note that this may result in unusual resolutions that your device might not handle well. The 'Hack' option is intended for the widescreen hacks.",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "auto", "Auto" },
         { "hack", "Hack" },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_screen_centering",
      "(GPU) Screen centering",
      NULL,
      "The PSX has a feature allowing it to shift the image position on screen. Some (mostly PAL) games used this feature in a strange way making the image miscentered and causing uneven borders to appear. With 'Auto' the emulator tries to correct this miscentering automatically. 'Game-controlled' uses the settings supplied by the game. 'Manual' allows to override those values with the settings below.",
      NULL,
      "video",
      {
         { "auto", "Auto" },
         { "game", "Game-controlled" },
         { "borderless", "Borderless" },
         { "manual", "Manual" },
         { NULL, NULL },
      },
      "auto",
   },
#define V(x) { #x, NULL }
   {
      "pcsx_rearmed_screen_centering_x",
      "(GPU) Manual position X",
      NULL,
      "X offset of the frame buffer. Only effective when 'Screen centering' is set to 'Manual'.",
      NULL,
      "video",
      {
         V(-16), V(-14), V(-12), V(-10), V(-8), V(-6), V(-4), V(-2), V(0), V(2), V(4), V(6), V(8), V(10), V(12), V(14), V(16),
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_screen_centering_y",
      "(GPU) Manual position Y",
      NULL,
      "Y offset of the frame buffer. Only effective when 'Screen centering' is set to 'Manual'.",
      NULL,
      "video",
      {
         V(-16), V(-15), V(-14), V(-13), V(-12), V(-11), V(-10), V(-9), V(-8), V(-7), V(-6), V(-5), V(-4), V(-3), V(-2), V(-1),
	 V(0), V(1), V(2), V(3), V(4), V(5), V(6), V(7), V(8), V(9), V(10), V(11), V(12), V(13), V(14), V(15), V(16),
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_screen_centering_h_adj",
      "(GPU) Manual height adjustment",
      NULL,
      "Height adjustment. Only effective when 'Screen centering' is set to 'Manual'.",
      NULL,
      "video",
      {
         V(-64), V(-48), V(-40), V(-32), V(-24), V(-16), V(-8), V(-7), V(-6), V(-5), V(-4), V(-3), V(-2), V(-1), V(0),
         { NULL, NULL },
      },
      "0",
   },
#undef V
#ifdef GPU_NEON
   {
      "pcsx_rearmed_neon_interlace_enable_v2",
      "(GPU) Show Interlaced Video",
      "Show Interlaced Video",
      "When enabled, games that run in high resolution video modes (480i, 512i) will produced interlaced video output. While this displays correctly on CRT televisions, it will produce artifacts on modern displays. When disabled, all video is output in progressive format. Note: there are games that will glitch is this is off.",
      NULL,
      "gpu_neon",
      {
         { "auto", NULL },
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "auto",
   },
   {
      "pcsx_rearmed_neon_enhancement_enable",
      "(GPU) Enhanced Resolution",
      "Enhanced Resolution",
      "Render games that do not already run in high resolution video modes (480i, 512i) at twice the native internal resolution. Improves the fidelity of 3D models at the expense of increased performance requirements. 2D elements are generally unaffected by this setting.",
      NULL,
      "gpu_neon",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_neon_enhancement_no_main",
      "(GPU) Enhanced Resolution Speed Hack",
      "Enh. Res. Speed Hack",
      "('Enhanced Resolution' Hack) Improves performance but reduces compatibility and may cause rendering errors.",
      NULL,
      "gpu_neon",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_neon_enhancement_tex_adj_v2",
      "(GPU) Enhanced Resolution Texture Adjustment",
      "Enh. Res. Texture Fixup",
      "('Enhanced Resolution' Hack) Solves some texturing issues in some games in Enhanced Resolution mode. May cause a small performance hit.",
      NULL,
      "gpu_neon",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
#endif /* GPU_NEON */
#ifdef GPU_PEOPS
   {
      "pcsx_rearmed_show_gpu_peops_settings",
      "Show Advanced P.E.Op.S. GPU Settings",
      NULL,
      "Show low-level configuration options for the P.E.Op.S. GPU plugin. Quick Menu may need to be toggled for this setting to take effect.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_odd_even_bit",
      "(GPU) Odd/Even Bit Hack",
      "Odd/Even Bit Hack",
      "A hack fix used to correct lock-ups that may occur in games such as Chrono Cross. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_expand_screen_width",
      "(GPU) Expand Screen Width",
      "Expand Screen Width",
      "Intended for use only with Capcom 2D fighting games. Enlarges the display area at the right side of the screen to show all background elements without cut-off. May cause rendering errors.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_ignore_brightness",
      "(GPU) Ignore Brightness Color",
      "Ignore Brightness Color",
      "A hack fix used to repair black screens in Lunar Silver Star Story Complete when entering a house or a menu. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_disable_coord_check",
      "(GPU) Disable Coordinate Check",
      "Disable Coordinate Check",
      "Legacy compatibility mode. May improve games that fail to run correctly on newer GPU hardware. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_lazy_screen_update",
      "(GPU) Lazy Screen Update",
      "Lazy Screen Update",
      "A partial fix to prevent text box flickering in Dragon Warrior VII. May also improve Pandemonium 2. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_repeated_triangles",
      "(GPU) Repeat Flat Tex Triangles",
      "Repeat Flat Tex Triangles",
      "A hack fix used to correct rendering errors in Star Wars: Dark Forces. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_quads_with_triangles",
      "(GPU) Draw Tex-Quads as Triangles",
      "Draw Tex-Quads as Triangles",
      "Corrects graphical distortions that may occur when games utilize Gouraud Shading, at the expense of reduced texture quality. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_peops_fake_busy_state",
      "(GPU) Fake 'GPU Busy' States",
      "Fake 'GPU Busy' States",
      "Emulate the 'GPU is busy' (drawing primitives) status flag of the original hardware instead of assuming the GPU is always ready for commands. May improve compatibility at the expense of reduced performance. Disable unless required.",
      NULL,
      "gpu_peops",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#endif /* GPU_PEOPS */
#ifdef GPU_UNAI
   {
      "pcsx_rearmed_show_gpu_unai_settings",
      "Show Advanced UNAI GPU Settings",
      NULL,
      "Show low-level configuration options for the UNAI GPU plugin. Quick Menu may need to be toggled for this setting to take effect.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled",
   },
#ifndef GPU_UNAI_NO_OLD
   {
      "pcsx_rearmed_gpu_unai_old_renderer",
      "(GPU) Old renderer",
      "Old renderer",
      "This enables faster, but less accurate code.",
      NULL,
      "gpu_unai",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled",
   },
#endif
   {
      "pcsx_rearmed_gpu_unai_blending",
      "(GPU) Texture Blending",
      "Texture Blending",
      "Enable alpha-based (and additive) texture blending. Required for various rendering effects, including transparency (e.g. water, shadows). Can be disabled to improve performance at the expense of severe display errors/inaccuracies.",
      NULL,
      "gpu_unai",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "enabled",
   },
   {
      "pcsx_rearmed_gpu_unai_skipline",
      "(GPU) Skip every 2nd line",
      "Skip every 2nd line",
      "Skips every second scanline. Can be enabled to improve performance at the expense of display inaccuracies (artifacts etc.).",
      NULL,
      "gpu_unai",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_unai_lighting",
      "(GPU) Lighting Effects",
      "Lighting Effects",
      "Enable simulated lighting effects (via vertex coloring combined with texture mapping). Required by almost all 3D games. Can be disabled to improve performance at the expense of severe display errors/inaccuracies (missing shadows, flat textures, etc.).",
      NULL,
      "gpu_unai",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "enabled",
   },
   {
      "pcsx_rearmed_gpu_unai_fast_lighting",
      "(GPU) Fast Lighting",
      "Fast Lighting",
      "Improves performance when 'Lighting Effects' are enabled, but may cause moderate/severe rendering errors.",
      NULL,
      "gpu_unai",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gpu_unai_scale_hires",
      "(GPU) Hi-Res Downscaling",
      "Hi-Res Downscaling",
      "When enabled, games that run in high resolution video modes (480i, 512i) will be downscaled to 320x240. Can improve performance, and is recommended on devices with native 240p display resolutions.",
      NULL,
      "gpu_unai",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL},
      },
#ifdef _MIYOO
      "enabled",
#else
      "disabled",
#endif
   },
#endif /* GPU_UNAI */
   {
      "pcsx_rearmed_spu_reverb",
      "Audio Reverb Effects",
      "Reverb Effects",
      "Enable emulation of the reverb feature provided by the PSX SPU. Can be disabled to improve performance at the expense of reduced audio quality/authenticity.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
#ifdef HAVE_PRE_ARMV7
      "disabled",
#else
      "enabled",
#endif
   },
   {
      "pcsx_rearmed_spu_interpolation",
      "Sound Interpolation",
      NULL,
      "Enable emulation of the in-built audio interpolation provided by the PSX SPU. 'Gaussian' sounds closest to original hardware. 'Simple' improves performance but reduces quality. 'Cubic' has the highest performance requirements but produces increased clarity. Can be disabled entirely for maximum performance, at the expense of greatly reduced audio quality.",
      NULL,
      "audio",
      {
         { "simple",   "Simple" },
         { "gaussian", "Gaussian" },
         { "cubic",    "Cubic" },
         { "off",      "disabled" },
         { NULL, NULL },
      },
#ifdef HAVE_PRE_ARMV7
      "off",
#else
      "simple",
#endif
   },
   {
      "pcsx_rearmed_nocdaudio",
      "CD Audio",
      NULL,
      "Enable playback of CD (CD-DA) audio tracks. Can be disabled to improve performance in games that include CD audio, at the expense of missing music.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      "pcsx_rearmed_noxadecoding",
      "XA Decoding",
      NULL,
      "Enable playback of XA (eXtended Architecture ADPCM) audio tracks. Can be disabled to improve performance in games that include XA audio, at the expense of missing music.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
#if P_HAVE_PTHREAD
   {
      "pcsx_rearmed_spu_thread",
      "Threaded SPU",
      NULL,
      "Emulates the PSX SPU on another CPU thread. May cause audio glitches in some games.",
      NULL,
      "audio",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#endif // P_HAVE_PTHREAD
   {
      "pcsx_rearmed_show_input_settings",
      "Show Input Settings",
      NULL,
      "Show configuration options for all input devices: analog response, Multitaps, light guns, etc. Quick Menu may need to be toggled for this setting to take effect.",
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_analog_axis_modifier",
      "Analog Axis Bounds",
      NULL,
      "Specify range limits for the left and right analog sticks when input device is set to 'analog' or 'dualshock'. 'Square' bounds improve input response when using controllers with highly circular ranges that are unable to fully saturate the X and Y axes at 45 degree deflections.",
      NULL,
      "input",
      {
         { "circle", "Circle" },
         { "square", "Square" },
         { NULL, NULL },
      },
      "square",
   },
   {
      "pcsx_rearmed_vibration",
      "Rumble Effects",
      NULL,
      "Enable haptic feedback when using a rumble-equipped gamepad with input device set to 'dualshock'.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      "pcsx_rearmed_analog_combo",
      "DualShock Analog Mode Toggle Key Combo",
      NULL,
      "When the input device type is DualShock, this option allows the emulated DualShock to be toggled between DIGITAL and ANALOG mode like original hardware. You can select the button combination for this.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "l1+r1+select",             "L1 + R1 + Select" },
         { "l1+r1+start",              "L1 + R1 + Start" },
         { "l1+r1+l3",                 "L1 + R1 + L3" },
         { "l1+r1+r3",                 "L1 + R1 + R3" },
         { "l3+r3",                    "L3 + R3" },
         { NULL, NULL },
      },
      "l1+r1+select"
   },
   {
      "pcsx_rearmed_multitap",
      "Multitap Mode",
      NULL,
      "Connect a virtual PSX Multitap peripheral to either controller 'Port 1' or controller 'Port 2' for 5 player simultaneous input, or to both 'Ports 1 and 2' for 8 player input. Mutlitap usage requires compatible games.",
      NULL,
      "input",
      {
         { "disabled",      NULL },
         { "port 1",        "Port 1" },
         { "port 2",        "Port 2" },
         { "ports 1 and 2", "Ports 1 and 2" },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_negcon_deadzone",
      "NegCon Twist Deadzone",
      NULL,
      "Set the deadzone of the RetroPad left analog stick when simulating the 'twist' action of emulated neGcon Controllers. Used to eliminate drift/unwanted input.",
      NULL,
      "input",
      {
         { "0",  "0%" },
         { "3",  "3%" },
         { "5",  "5%" },
         { "7",  "7%" },
         { "10", "10%" },
         { "13", "13%" },
         { "15", "15%" },
         { "17", "17%" },
         { "20", "20%" },
         { "23", "23%" },
         { "25", "25%" },
         { "27", "27%" },
         { "30", "30%" },
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_negcon_response",
      "NegCon Twist Response",
      NULL,
      "Specify the analog response when using a RetroPad left analog stick to simulate the 'twist' action of emulated neGcon Controllers.",
      NULL,
      "input",
      {
         { "linear",    "Linear" },
         { "quadratic", "Quadratic" },
         { "cubic",     "Cubic" },
         { NULL, NULL },
      },
      "linear",
   },
   {
      "pcsx_rearmed_input_sensitivity",
      "Mouse Sensitivity",
      NULL,
      "Adjust responsiveness of emulated 'mouse' input devices.",
      NULL,
      "input",
      {
         { "0.05", NULL },
         { "0.10", NULL },
         { "0.15", NULL },
         { "0.20", NULL },
         { "0.25", NULL },
         { "0.30", NULL },
         { "0.35", NULL },
         { "0.40", NULL },
         { "0.45", NULL },
         { "0.50", NULL },
         { "0.55", NULL },
         { "0.60", NULL },
         { "0.65", NULL },
         { "0.70", NULL },
         { "0.75", NULL },
         { "0.80", NULL },
         { "0.85", NULL },
         { "0.90", NULL },
         { "0.95", NULL },
         { "1.00", NULL },
         { "1.05", NULL },
         { "1.10", NULL },
         { "1.15", NULL },
         { "1.20", NULL },
         { "1.25", NULL },
         { "1.30", NULL },
         { "1.35", NULL },
         { "1.40", NULL },
         { "1.45", NULL },
         { "1.50", NULL },
         { "1.55", NULL },
         { "1.60", NULL },
         { "1.65", NULL },
         { "1.70", NULL },
         { "1.75", NULL },
         { "1.80", NULL },
         { "1.85", NULL },
         { "1.90", NULL },
         { "1.95", NULL },
         { "2.00", NULL },
      },
      "1.00",
   },
   {
      "pcsx_rearmed_crosshair1",
      "Player 1 Lightgun Crosshair",
      NULL,
      "Toggle player 1's crosshair for the Guncon or Konami Gun. Only works if RGB32 output is off (video options).",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "blue",  NULL },
         { "green",  NULL },
         { "red",  NULL },
         { "white",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_crosshair2",
      "Player 2 Lightgun Crosshair",
      NULL,
      "Toggle player 2's crosshair for the Guncon or Konami Gun. Only works if RGB32 output is off (video options).",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "blue",  NULL },
         { "green",  NULL },
         { "red",  NULL },
         { "white",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_konamigunadjustx",
      "Konami Gun X Axis Offset",
      NULL,
      "Apply an X axis offset to light gun input when emulating a Konami Gun (Hyper Blaster / Justifier) device. Can be used to correct aiming misalignments.",
      NULL,
      "input",
      {
         { "-40", NULL },
         { "-39", NULL },
         { "-38", NULL },
         { "-37", NULL },
         { "-36", NULL },
         { "-35", NULL },
         { "-34", NULL },
         { "-33", NULL },
         { "-32", NULL },
         { "-31", NULL },
         { "-30", NULL },
         { "-29", NULL },
         { "-28", NULL },
         { "-27", NULL },
         { "-26", NULL },
         { "-25", NULL },
         { "-24", NULL },
         { "-23", NULL },
         { "-22", NULL },
         { "-21", NULL },
         { "-20", NULL },
         { "-19", NULL },
         { "-18", NULL },
         { "-17", NULL },
         { "-16", NULL },
         { "-15", NULL },
         { "-14", NULL },
         { "-13", NULL },
         { "-12", NULL },
         { "-11", NULL },
         { "-10", NULL },
         { "-9",  NULL },
         { "-8",  NULL },
         { "-7",  NULL },
         { "-6",  NULL },
         { "-5",  NULL },
         { "-4",  NULL },
         { "-3",  NULL },
         { "-2",  NULL },
         { "-1",  NULL },
         { "0",   NULL },
         { "1",   NULL },
         { "2",   NULL },
         { "3",   NULL },
         { "4",   NULL },
         { "5",   NULL },
         { "6",   NULL },
         { "7",   NULL },
         { "8",   NULL },
         { "9",   NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { "25",  NULL },
         { "26",  NULL },
         { "27",  NULL },
         { "28",  NULL },
         { "29",  NULL },
         { "30",  NULL },
         { "31",  NULL },
         { "32",  NULL },
         { "33",  NULL },
         { "34",  NULL },
         { "35",  NULL },
         { "36",  NULL },
         { "37",  NULL },
         { "38",  NULL },
         { "39",  NULL },
         { "40",  NULL },
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_konamigunadjusty",
      "Konami Gun Y Axis Offset",
      NULL,
      "Apply a Y axis offset to light gun input when emulating a Konami Gun (Hyper Blaster / Justifier) device. Can be used to correct aiming misalignments.",
      NULL,
      "input",
      {
         { "-40", NULL },
         { "-39", NULL },
         { "-38", NULL },
         { "-37", NULL },
         { "-36", NULL },
         { "-35", NULL },
         { "-34", NULL },
         { "-33", NULL },
         { "-32", NULL },
         { "-31", NULL },
         { "-30", NULL },
         { "-29", NULL },
         { "-28", NULL },
         { "-27", NULL },
         { "-26", NULL },
         { "-25", NULL },
         { "-24", NULL },
         { "-23", NULL },
         { "-22", NULL },
         { "-21", NULL },
         { "-20", NULL },
         { "-19", NULL },
         { "-18", NULL },
         { "-17", NULL },
         { "-16", NULL },
         { "-15", NULL },
         { "-14", NULL },
         { "-13", NULL },
         { "-12", NULL },
         { "-11", NULL },
         { "-10", NULL },
         { "-9",  NULL },
         { "-8",  NULL },
         { "-7",  NULL },
         { "-6",  NULL },
         { "-5",  NULL },
         { "-4",  NULL },
         { "-3",  NULL },
         { "-2",  NULL },
         { "-1",  NULL },
         { "0",   NULL },
         { "1",   NULL },
         { "2",   NULL },
         { "3",   NULL },
         { "4",   NULL },
         { "5",   NULL },
         { "6",   NULL },
         { "7",   NULL },
         { "8",   NULL },
         { "9",   NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { "25",  NULL },
         { "26",  NULL },
         { "27",  NULL },
         { "28",  NULL },
         { "29",  NULL },
         { "30",  NULL },
         { "31",  NULL },
         { "32",  NULL },
         { "33",  NULL },
         { "34",  NULL },
         { "35",  NULL },
         { "36",  NULL },
         { "37",  NULL },
         { "38",  NULL },
         { "39",  NULL },
         { "40",  NULL },
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_gunconadjustx",
      "Guncon X Axis Offset",
      NULL,
      "Apply an X axis offset to light gun input when emulating a Guncon device. Can be used to correct aiming misalignments.",
      NULL,
      "input",
      {
         { "-40", NULL },
         { "-39", NULL },
         { "-38", NULL },
         { "-37", NULL },
         { "-36", NULL },
         { "-35", NULL },
         { "-34", NULL },
         { "-33", NULL },
         { "-32", NULL },
         { "-31", NULL },
         { "-30", NULL },
         { "-29", NULL },
         { "-28", NULL },
         { "-27", NULL },
         { "-26", NULL },
         { "-25", NULL },
         { "-24", NULL },
         { "-23", NULL },
         { "-22", NULL },
         { "-21", NULL },
         { "-20", NULL },
         { "-19", NULL },
         { "-18", NULL },
         { "-17", NULL },
         { "-16", NULL },
         { "-15", NULL },
         { "-14", NULL },
         { "-13", NULL },
         { "-12", NULL },
         { "-11", NULL },
         { "-10", NULL },
         { "-9",  NULL },
         { "-8",  NULL },
         { "-7",  NULL },
         { "-6",  NULL },
         { "-5",  NULL },
         { "-4",  NULL },
         { "-3",  NULL },
         { "-2",  NULL },
         { "-1",  NULL },
         { "0",   NULL },
         { "1",   NULL },
         { "2",   NULL },
         { "3",   NULL },
         { "4",   NULL },
         { "5",   NULL },
         { "6",   NULL },
         { "7",   NULL },
         { "8",   NULL },
         { "9",   NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { "25",  NULL },
         { "26",  NULL },
         { "27",  NULL },
         { "28",  NULL },
         { "29",  NULL },
         { "30",  NULL },
         { "31",  NULL },
         { "32",  NULL },
         { "33",  NULL },
         { "34",  NULL },
         { "35",  NULL },
         { "36",  NULL },
         { "37",  NULL },
         { "38",  NULL },
         { "39",  NULL },
         { "40",  NULL },
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_gunconadjusty",
      "Guncon Y Axis Offset",
      NULL,
      "Apply a Y axis offset to light gun input when emulating a Guncon device. Can be used to correct aiming misalignments.",
      NULL,
      "input",
      {
         { "-40", NULL },
         { "-39", NULL },
         { "-38", NULL },
         { "-37", NULL },
         { "-36", NULL },
         { "-35", NULL },
         { "-34", NULL },
         { "-33", NULL },
         { "-32", NULL },
         { "-31", NULL },
         { "-30", NULL },
         { "-29", NULL },
         { "-28", NULL },
         { "-27", NULL },
         { "-26", NULL },
         { "-25", NULL },
         { "-24", NULL },
         { "-23", NULL },
         { "-22", NULL },
         { "-21", NULL },
         { "-20", NULL },
         { "-19", NULL },
         { "-18", NULL },
         { "-17", NULL },
         { "-16", NULL },
         { "-15", NULL },
         { "-14", NULL },
         { "-13", NULL },
         { "-12", NULL },
         { "-11", NULL },
         { "-10", NULL },
         { "-9",  NULL },
         { "-8",  NULL },
         { "-7",  NULL },
         { "-6",  NULL },
         { "-5",  NULL },
         { "-4",  NULL },
         { "-3",  NULL },
         { "-2",  NULL },
         { "-1",  NULL },
         { "0",   NULL },
         { "1",   NULL },
         { "2",   NULL },
         { "3",   NULL },
         { "4",   NULL },
         { "5",   NULL },
         { "6",   NULL },
         { "7",   NULL },
         { "8",   NULL },
         { "9",   NULL },
         { "10",  NULL },
         { "11",  NULL },
         { "12",  NULL },
         { "13",  NULL },
         { "14",  NULL },
         { "15",  NULL },
         { "16",  NULL },
         { "17",  NULL },
         { "18",  NULL },
         { "19",  NULL },
         { "20",  NULL },
         { "21",  NULL },
         { "22",  NULL },
         { "23",  NULL },
         { "24",  NULL },
         { "25",  NULL },
         { "26",  NULL },
         { "27",  NULL },
         { "28",  NULL },
         { "29",  NULL },
         { "30",  NULL },
         { "31",  NULL },
         { "32",  NULL },
         { "33",  NULL },
         { "34",  NULL },
         { "35",  NULL },
         { "36",  NULL },
         { "37",  NULL },
         { "38",  NULL },
         { "39",  NULL },
         { "40",  NULL },
         { NULL, NULL },
      },
      "0",
   },
   {
      "pcsx_rearmed_gunconadjustratiox",
      "Guncon X Axis Response",
      NULL,
      "Adjust relative magnitude of horizontal light gun motion when emulating a Guncon device. Can be used to correct aiming misalignments.",
      NULL,
      "input",
      {
         { "0.75", NULL },
         { "0.76", NULL },
         { "0.77", NULL },
         { "0.78", NULL },
         { "0.79", NULL },
         { "0.80", NULL },
         { "0.81", NULL },
         { "0.82", NULL },
         { "0.83", NULL },
         { "0.84", NULL },
         { "0.85", NULL },
         { "0.86", NULL },
         { "0.87", NULL },
         { "0.88", NULL },
         { "0.89", NULL },
         { "0.90", NULL },
         { "0.91", NULL },
         { "0.92", NULL },
         { "0.93", NULL },
         { "0.94", NULL },
         { "0.95", NULL },
         { "0.96", NULL },
         { "0.97", NULL },
         { "0.98", NULL },
         { "0.99", NULL },
         { "1.00", NULL },
         { "1.01", NULL },
         { "1.02", NULL },
         { "1.03", NULL },
         { "1.04", NULL },
         { "1.05", NULL },
         { "1.06", NULL },
         { "1.07", NULL },
         { "1.08", NULL },
         { "1.09", NULL },
         { "1.10", NULL },
         { "1.11", NULL },
         { "1.12", NULL },
         { "1.13", NULL },
         { "1.14", NULL },
         { "1.15", NULL },
         { "1.16", NULL },
         { "1.17", NULL },
         { "1.18", NULL },
         { "1.19", NULL },
         { "1.20", NULL },
         { "1.21", NULL },
         { "1.22", NULL },
         { "1.23", NULL },
         { "1.24", NULL },
         { "1.25", NULL },
         { NULL, NULL },
      },
      "1.00",
   },
   {
      "pcsx_rearmed_gunconadjustratioy",
      "Guncon Y Axis Response",
      NULL,
      "Adjust relative magnitude of vertical light gun motion when emulating a Guncon device. Can be used to correct aiming misalignments.",
      NULL,
      "input",
      {
         { "0.75", NULL },
         { "0.76", NULL },
         { "0.77", NULL },
         { "0.78", NULL },
         { "0.79", NULL },
         { "0.80", NULL },
         { "0.81", NULL },
         { "0.82", NULL },
         { "0.83", NULL },
         { "0.84", NULL },
         { "0.85", NULL },
         { "0.86", NULL },
         { "0.87", NULL },
         { "0.88", NULL },
         { "0.89", NULL },
         { "0.90", NULL },
         { "0.91", NULL },
         { "0.92", NULL },
         { "0.93", NULL },
         { "0.94", NULL },
         { "0.95", NULL },
         { "0.96", NULL },
         { "0.97", NULL },
         { "0.98", NULL },
         { "0.99", NULL },
         { "1.00", NULL },
         { "1.01", NULL },
         { "1.02", NULL },
         { "1.03", NULL },
         { "1.04", NULL },
         { "1.05", NULL },
         { "1.06", NULL },
         { "1.07", NULL },
         { "1.08", NULL },
         { "1.09", NULL },
         { "1.10", NULL },
         { "1.11", NULL },
         { "1.12", NULL },
         { "1.13", NULL },
         { "1.14", NULL },
         { "1.15", NULL },
         { "1.16", NULL },
         { "1.17", NULL },
         { "1.18", NULL },
         { "1.19", NULL },
         { "1.20", NULL },
         { "1.21", NULL },
         { "1.22", NULL },
         { "1.23", NULL },
         { "1.24", NULL },
         { "1.25", NULL },
         { NULL, NULL },
      },
      "1.00",
   },
   {
      "pcsx_rearmed_icache_emulation",
      "Instruction Cache Emulation",
      NULL,
      "Enable emulation of the PSX CPU instruction cache. Improves accuracy at the expense of increased performance overheads. Required for Formula One 2001, Formula One Arcade and Formula One 99. [Interpreter only; partial on lightrec and ARM dynarecs]",
      NULL,
      "compat_hack",
      {
         { "enabled",  NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "enabled",
   },
   {
      "pcsx_rearmed_exception_emulation",
      "Exception and Breakpoint Emulation",
      NULL,
      "Enable emulation of some almost never used PSX's debug features. This causes a performance hit, is not useful for games and is intended for PSX homebrew and romhack developers only. Only enable if you know what you are doing. [Interpreter only]",
      NULL,
      "compat_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#ifdef _3DS
#define V(x) { #x, NULL }
   {
      "pcsx_rearmed_3ds_appcputime",
      "3DS AppCpuTimeLimit",
      NULL,
      "% of syscore (core #1) CPU time allocated to the emulator",
      NULL,
      "speed_hack",
      {
         V( 5),  V(10),
         V(15),  V(20),
         V(25),  V(30),
         V(35),  V(40),
         V(45),  V(50),
         V(55),  V(60),
         V(65),  V(70),
         V(75),  V(80),
         { NULL, NULL},
      },
      "35",
   },
#undef V
#endif // _3DS
   {
      "pcsx_rearmed_cd_turbo",
      "Turbo CD",
      NULL,
      "This makes the emulated CD-ROM extremely fast and can reduce loading times in some cases. Warning: many games were not programmed to handle such a speed. The game (or even the emulator) MAY CRASH at ANY TIME if this is enabled.",
      NULL,
      "speed_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#if !defined(DRC_DISABLE) && !defined(LIGHTREC)
   {
      "pcsx_rearmed_nocompathacks",
      "Disable Automatic Compatibility Hacks",
      NULL,
      "By default, PCSX-ReARMed will apply auxiliary compatibility hacks automatically, based on the currently loaded content. This behaviour is required for correct operation, but may be disabled if desired.",
      NULL,
      "compat_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_nosmccheck",
      "(Speed Hack) Disable SMC Checks",
      "Disable SMC Checks",
      "Will cause crashes when loading, and lead to memory card failure.",
      NULL,
      "speed_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_gteregsunneeded",
      "(Speed Hack) Assume GTE Registers Unneeded",
      "Assume GTE Registers Unneeded",
      "May cause rendering errors.",
      NULL,
      "speed_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   {
      "pcsx_rearmed_nogteflags",
      "(Speed Hack) Disable GTE Flags",
      "Disable GTE Flags",
      "Will cause rendering errors.",
      NULL,
      "speed_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
#endif /* !DRC_DISABLE && !LIGHTREC */
   {
      "pcsx_rearmed_nostalls",
      "Disable CPU/GTE Stalls",
      NULL,
      "Will cause some games to run too quickly. Should be disabled in almost all cases."
#if defined(LIGHTREC)
      " Interpreter only."
#endif
      ,
      NULL,
      "speed_hack",
      {
         { "disabled", NULL },
         { "enabled",  NULL },
         { NULL, NULL },
      },
      "disabled",
   },
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,        /* RETRO_LANGUAGE_JAPANESE */
   NULL,        /* RETRO_LANGUAGE_FRENCH */
   NULL,        /* RETRO_LANGUAGE_SPANISH */
   NULL,        /* RETRO_LANGUAGE_GERMAN */
   NULL,        /* RETRO_LANGUAGE_ITALIAN */
   NULL,        /* RETRO_LANGUAGE_DUTCH */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,        /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,        /* RETRO_LANGUAGE_RUSSIAN */
   NULL,        /* RETRO_LANGUAGE_KOREAN */
   NULL,        /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,        /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,        /* RETRO_LANGUAGE_ESPERANTO */
   NULL,        /* RETRO_LANGUAGE_POLISH */
   NULL,        /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,        /* RETRO_LANGUAGE_ARABIC */
   NULL,        /* RETRO_LANGUAGE_GREEK */
   &options_tr, /* RETRO_LANGUAGE_TURKISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb,
      bool *categories_supported)
{
   unsigned version  = 0;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language = 0;
#endif

   if (!environ_cb || !categories_supported)
      return;

   *categories_supported = false;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_v2_intl core_options_intl;

      core_options_intl.us    = &options_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = options_intl[language];

      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL,
            &core_options_intl);
#else
      *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            &options_us);
#endif
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
#ifndef HAVE_NO_LANGEXTRA
      size_t num_options_intl          = 0;
      struct retro_core_option_v2_definition
            *option_defs_intl          = NULL;
      struct retro_core_option_definition
            *option_v1_defs_intl       = NULL;
      struct retro_core_options_intl
            core_options_v1_intl;
#endif
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine total number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      if (version >= 1)
      {
         /* Allocate US array */
         option_v1_defs_us = (struct retro_core_option_definition *)
               calloc(num_options + 1, sizeof(struct retro_core_option_definition));

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
            struct retro_core_option_value *option_values         = option_def_us->values;
            struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
            struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

            option_v1_def_us->key           = option_def_us->key;
            option_v1_def_us->desc          = option_def_us->desc;
            option_v1_def_us->info          = option_def_us->info;
            option_v1_def_us->default_value = option_def_us->default_value;

            /* Values must be copied individually... */
            while (option_values->value)
            {
               option_v1_values->value = option_values->value;
               option_v1_values->label = option_values->label;

               option_values++;
               option_v1_values++;
            }
         }

#ifndef HAVE_NO_LANGEXTRA
         if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
             (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH) &&
             options_intl[language])
            option_defs_intl = options_intl[language]->definitions;

         if (option_defs_intl)
         {
            /* Determine number of intl options */
            while (true)
            {
               if (option_defs_intl[num_options_intl].key)
                  num_options_intl++;
               else
                  break;
            }

            /* Allocate intl array */
            option_v1_defs_intl = (struct retro_core_option_definition *)
                  calloc(num_options_intl + 1, sizeof(struct retro_core_option_definition));

            /* Copy parameters from option_defs_intl array */
            for (i = 0; i < num_options_intl; i++)
            {
               struct retro_core_option_v2_definition *option_def_intl = &option_defs_intl[i];
               struct retro_core_option_value *option_values           = option_def_intl->values;
               struct retro_core_option_definition *option_v1_def_intl = &option_v1_defs_intl[i];
               struct retro_core_option_value *option_v1_values        = option_v1_def_intl->values;

               option_v1_def_intl->key           = option_def_intl->key;
               option_v1_def_intl->desc          = option_def_intl->desc;
               option_v1_def_intl->info          = option_def_intl->info;
               option_v1_def_intl->default_value = option_def_intl->default_value;

               /* Values must be copied individually... */
               while (option_values->value)
               {
                  option_v1_values->value = option_values->value;
                  option_v1_values->label = option_values->label;

                  option_values++;
                  option_v1_values++;
               }
            }
         }

         core_options_v1_intl.us    = option_v1_defs_us;
         core_options_v1_intl.local = option_v1_defs_intl;

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_v1_intl);
#else
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
#endif
      }
      else
      {
         /* Allocate arrays */
         variables  = (struct retro_variable *)calloc(num_options + 1,
               sizeof(struct retro_variable));
         values_buf = (char **)calloc(num_options, sizeof(char *));

         if (!variables || !values_buf)
            goto error;

         /* Copy parameters from option_defs_us array */
         for (i = 0; i < num_options; i++)
         {
            const char *key                        = option_defs_us[i].key;
            const char *desc                       = option_defs_us[i].desc;
            const char *default_value              = option_defs_us[i].default_value;
            struct retro_core_option_value *values = option_defs_us[i].values;
            size_t buf_len                         = 3;
            size_t default_index                   = 0;

            values_buf[i] = NULL;

            /* Skip options that are irrelevant when using the
             * old style core options interface */
            if ((strcmp(key, "pcsx_rearmed_show_input_settings") == 0) ||
                (strcmp(key, "pcsx_rearmed_show_gpu_peops_settings") == 0) ||
                (strcmp(key, "pcsx_rearmed_show_gpu_unai_settings") == 0))
               continue;

            if (desc)
            {
               size_t num_values = 0;

               /* Determine number of values */
               while (true)
               {
                  if (values[num_values].value)
                  {
                     /* Check if this is the default value */
                     if (default_value)
                        if (strcmp(values[num_values].value, default_value) == 0)
                           default_index = num_values;

                     buf_len += strlen(values[num_values].value);
                     num_values++;
                  }
                  else
                     break;
               }

               /* Build values string */
               if (num_values > 0)
               {
                  buf_len += num_values - 1;
                  buf_len += strlen(desc);

                  values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                  if (!values_buf[i])
                     goto error;

                  strcpy(values_buf[i], desc);
                  strcat(values_buf[i], "; ");

                  /* Default value goes first */
                  strcat(values_buf[i], values[default_index].value);

                  /* Add remaining values */
                  for (j = 0; j < num_values; j++)
                  {
                     if (j != default_index)
                     {
                        strcat(values_buf[i], "|");
                        strcat(values_buf[i], values[j].value);
                     }
                  }
               }
            }

            variables[option_index].key   = key;
            variables[option_index].value = values_buf[i];
            option_index++;
         }

         /* Set variables */
         environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
      }

error:
      /* Clean up */

      if (option_v1_defs_us)
      {
         free(option_v1_defs_us);
         option_v1_defs_us = NULL;
      }

#ifndef HAVE_NO_LANGEXTRA
      if (option_v1_defs_intl)
      {
         free(option_v1_defs_intl);
         option_v1_defs_intl = NULL;
      }
#endif

      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
