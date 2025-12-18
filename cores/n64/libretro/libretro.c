#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#ifndef NO_LIBCO
#include <libco.h>
#endif

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include <glsm/glsmsym.h>
#endif

#include "api/m64p_frontend.h"
#include "plugin/plugin.h"
#include "api/m64p_types.h"
#include "r4300/r4300.h"
#include "memory/memory.h"
#include "main/main.h"
#include "main/cheat.h"
#include "main/version.h"
#include "main/savestates.h"
#include "dd/dd_disk.h"
#include "pi/pi_controller.h"
#include "si/pif.h"
#include "libretro_memory.h"

/* Cxd4 RSP */
#include "../mupen64plus-rsp-cxd4/config.h"
#include "plugin/audio_libretro/audio_plugin.h"
#include "../Graphics/plugin.h"

#ifdef HAVE_THR_AL
#include "../mupen64plus-video-angrylion/vdac.h"
#endif

#ifndef PRESCALE_WIDTH
#define PRESCALE_WIDTH  640
#endif

#ifndef PRESCALE_HEIGHT
#define PRESCALE_HEIGHT 625
#endif

#if defined(NO_LIBCO) && defined(DYNAREC)
#error cannot currently use dynarecs without libco
#endif

/* forward declarations */
int InitGfx(void);
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
int glide64InitGfx(void);
void gles2n64_reset(void);
#endif

#if defined(HAVE_PARALLEL)
#include "../mupen64plus-video-paraLLEl/parallel.h"

static struct retro_hw_render_callback hw_render;
static struct retro_hw_render_context_negotiation_interface_vulkan hw_context_negotiation;
static const struct retro_hw_render_interface_vulkan *vulkan;
#endif

#define ISHEXDEC ((codeLine[cursor]>='0') && (codeLine[cursor]<='9')) || ((codeLine[cursor]>='a') && (codeLine[cursor]<='f')) || ((codeLine[cursor]>='A') && (codeLine[cursor]<='F'))

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;

retro_log_printf_t log_cb                         = NULL;
retro_video_refresh_t video_cb                    = NULL;
retro_input_poll_t poll_cb                        = NULL;
retro_input_state_t input_cb                      = NULL;
retro_audio_sample_batch_t audio_batch_cb         = NULL;
retro_environment_t environ_cb                    = NULL;

struct retro_rumble_interface rumble;

#define SUBSYSTEM_CART_DISK 0x0101

static const struct retro_subsystem_rom_info n64_cart_disk[] = {
   { "Cartridge", "n64|z64|v64|bin", false, false, false, NULL, 0 },
   { "Disk",      "ndd|bin",         false, false, false, NULL, 0 },
   { NULL }
};

static const struct retro_subsystem_info subsystems[] = {
   { "Cartridge and Disk", "n64_cart_disk", n64_cart_disk, 2, SUBSYSTEM_CART_DISK},
   { NULL }
};

save_memory_data saved_memory;

#ifdef NO_LIBCO
static bool stop_stepping;
#else
cothread_t main_thread;
static cothread_t game_thread;
#endif

float polygonOffsetFactor           = 0.0f;
float polygonOffsetUnits            = 0.0f;

static bool vulkan_inited           = false;
static bool gl_inited               = false;

int astick_deadzone                       = 0;
int astick_snap_active                    = 0;
int astick_snap_max_angle                 = 15;
int astick_snap_min_displacement_percent  = 70;
int astick_sensitivity                    = 100;
int first_time                            = 1;
bool flip_only                            = false;

static uint8_t* cart_data           = NULL;
static uint32_t cart_size           = 0;
static uint8_t* disk_data           = NULL;
static uint32_t disk_size           = 0;

static bool     emu_initialized     = false;
static unsigned initial_boot        = true;
static unsigned audio_buffer_size   = 2048;

static unsigned retro_filtering     = 0;
static unsigned retro_dithering     = 0;
static bool     reinit_screen       = false;
static bool     first_context_reset = false;
static bool     pushed_frame        = false;

bool frame_dupe                     = false;

uint32_t gfx_plugin_accuracy        = 2;
static enum rsp_plugin_type
                 rsp_plugin;
uint32_t screen_width               = 640;
uint32_t screen_height              = 480;
uint32_t screen_pitch               = 0;
uint32_t screen_aspectmodehint;
uint32_t send_allist_to_hle_rsp     = 0;

unsigned int BUFFERSWAP             = 0;
unsigned int FAKE_SDL_TICKS         = 0;

bool alternate_mapping;

static bool initializing            = true;

extern int g_vi_refresh_rate;

/* after the controller's CONTROL* member has been assigned we can update
 * them straight from here... */
extern struct
{
    CONTROL *control;
    BUTTONS buttons;
} controller[4];

/* ...but it won't be at least the first time we're called, in that case set
 * these instead for input_plugin to read. */
int pad_pak_types[4];
int pad_present[4] = {1, 1, 1, 1};

static void n64DebugCallback(void* aContext, int aLevel, const char* aMessage)
{
    char buffer[1024];
    if (!log_cb)
       return;

    sprintf(buffer, "mupen64plus: %s\n", aMessage);

    switch (aLevel)
    {
       case M64MSG_ERROR:
          log_cb(RETRO_LOG_ERROR, buffer);
          break;
       case M64MSG_INFO:
          log_cb(RETRO_LOG_INFO, buffer);
          break;
       case M64MSG_WARNING:
          log_cb(RETRO_LOG_WARN, buffer);
          break;
       case M64MSG_VERBOSE:
       case M64MSG_STATUS:
          log_cb(RETRO_LOG_DEBUG, buffer);
          break;
       default:
          break;
    }
}

extern m64p_rom_header ROM_HEADER;

static void core_settings_autoselect_gfx_plugin(void)
{
   struct retro_variable gfx_var = { "parallel-n64-gfxplugin", 0 };

   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &gfx_var);

   if (gfx_var.value && strcmp(gfx_var.value, "auto") != 0)
      return;

#if defined(HAVE_PARALLEL)
   if (vulkan_inited)
   {
      gfx_plugin = GFX_PARALLEL;
      return;
   }
#endif

#if (defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)) && defined(HAVE_GLIDE64)
   if (gl_inited)
   {
      gfx_plugin = GFX_GLIDE64;
      return;
   }
#endif

#ifdef HAVE_THR_AL
   gfx_plugin = GFX_ANGRYLION;
#endif
}

unsigned libretro_get_gfx_plugin(void)
{
   return gfx_plugin;
}

static void core_settings_autoselect_rsp_plugin(void);

static void core_settings_set_defaults(void)
{
   /* Load GFX plugin core option */
   struct retro_variable gfx_var = { "parallel-n64-gfxplugin", 0 };
   struct retro_variable rsp_var = { "parallel-n64-rspplugin", 0 };
   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &gfx_var);
   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &rsp_var);

   if (gfx_var.value)
   {
      if (gfx_var.value && !strcmp(gfx_var.value, "auto"))
         core_settings_autoselect_gfx_plugin();
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
      if (gfx_var.value && !strcmp(gfx_var.value, "gln64") && gl_inited)
         gfx_plugin = GFX_GLN64;
#endif

#ifdef HAVE_RICE
      if (gfx_var.value && !strcmp(gfx_var.value, "rice") && gl_inited)
         gfx_plugin = GFX_RICE;
#endif
#ifdef HAVE_GLIDE64
      if(gfx_var.value && !strcmp(gfx_var.value, "glide64") && gl_inited)
         gfx_plugin = GFX_GLIDE64;
#endif
#endif
#ifdef HAVE_THR_AL
	  if(gfx_var.value && !strcmp(gfx_var.value, "angrylion"))
         gfx_plugin = GFX_ANGRYLION;
#endif
#ifdef HAVE_PARALLEL
	  if(gfx_var.value && !strcmp(gfx_var.value, "parallel") && vulkan_inited)
         gfx_plugin = GFX_PARALLEL;
#endif
   }

   gfx_var.key = "parallel-n64-gfxplugin-accuracy";
   gfx_var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &gfx_var) && gfx_var.value)
   {
       if (gfx_var.value && !strcmp(gfx_var.value, "veryhigh"))
          gfx_plugin_accuracy = 3;
       else if (gfx_var.value && !strcmp(gfx_var.value, "high"))
          gfx_plugin_accuracy = 2;
       else if (gfx_var.value && !strcmp(gfx_var.value, "medium"))
          gfx_plugin_accuracy = 1;
       else if (gfx_var.value && !strcmp(gfx_var.value, "low"))
          gfx_plugin_accuracy = 0;
   }

   /* Load RSP plugin core option */

   if (rsp_var.value)
   {
      if (rsp_var.value && !strcmp(rsp_var.value, "auto"))
         core_settings_autoselect_rsp_plugin();
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
      if (rsp_var.value && !strcmp(rsp_var.value, "hle") && !vulkan_inited)
         rsp_plugin = RSP_HLE;
#endif
      if (rsp_var.value && !strcmp(rsp_var.value, "cxd4"))
         rsp_plugin = RSP_CXD4;
      if (rsp_var.value && !strcmp(rsp_var.value, "parallel"))
         rsp_plugin = RSP_PARALLEL;
   }
}



static void core_settings_autoselect_rsp_plugin(void)
{
   struct retro_variable rsp_var = { "parallel-n64-rspplugin", 0 };

   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &rsp_var);

   if (rsp_var.value && strcmp(rsp_var.value, "auto") != 0)
      return;

   rsp_plugin = RSP_HLE;

   if (
          (!strcmp((const char*)ROM_HEADER.Name, "GAUNTLET LEGENDS"))
      )
   {
      rsp_plugin = RSP_CXD4;
   }

   if (!strcmp((const char*)ROM_HEADER.Name, "CONKER BFD"))
      rsp_plugin = RSP_HLE;

   if (vulkan_inited)
   {
#if defined(HAVE_PARALLEL_RSP)
      rsp_plugin = RSP_PARALLEL;
#else
      rsp_plugin = RSP_CXD4;
#endif
   }

#ifdef HAVE_THR_AL
   if (gfx_plugin == GFX_ANGRYLION)
      rsp_plugin = RSP_CXD4;
#endif
}

static void setup_variables(void)
{
   struct retro_variable variables[] = {
      { "parallel-n64-cpucore",
#ifdef DYNAREC
#if defined(IOS)
         "CPU Core; cached_interpreter|pure_interpreter|dynamic_recompiler" },
#else
         "CPU Core; dynamic_recompiler|cached_interpreter|pure_interpreter" },
#endif
#else
         "CPU Core; cached_interpreter|pure_interpreter" },
#endif
      {"parallel-n64-audio-buffer-size",
         "Audio Buffer Size (restart); 2048|1024"},
      {"parallel-n64-astick-deadzone",
        "Analog Deadzone (percent); 15|20|25|30|0|5|10"},
      {"parallel-n64-astick-sensitivity",
        "Analog Sensitivity (percent); 100|105|110|115|120|125|130|135|140|145|150|200|50|55|60|65|70|75|80|85|90|95"},
      { "parallel-n64-astick-snap-angle-active",
      "Snap Controller Angle; disabled|enabled" },
      { "parallel-n64-astick-snap-max-angle",
         "Maximum Snap Angle (value of 5 would mean 85 to 95 degrees are snapped to 90); 1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21" },
      { "parallel-n64-astick-snap-min-displacement-percent",
         "Which percentage of displacement of the stick from the center causes snap to activate (zero means always); 0|10|20|30|40|50|60|65|70|75|80|85|90|95" },
      {"parallel-n64-pak1",
#ifdef CLASSIC
        "Player 1 Pak; memory|rumble|none"},
#else
        "Player 1 Pak; none|memory|rumble"},	
#endif
      {"parallel-n64-pak2",
        "Player 2 Pak; none|memory|rumble"},
      {"parallel-n64-pak3",
        "Player 3 Pak; none|memory|rumble"},
      {"parallel-n64-pak4",
        "Player 4 Pak; none|memory|rumble"},
      { "parallel-n64-disable_expmem",
         "Enable Expansion Pak RAM; enabled|disabled" },
      { "parallel-n64-gfxplugin-accuracy",
#if defined(IOS) || defined(ANDROID)
         "GFX Accuracy (restart); medium|high|veryhigh|low" },
#else
         "GFX Accuracy (restart); veryhigh|high|medium|low" },
#endif
#ifdef HAVE_PARALLEL
      { "parallel-n64-parallel-rdp-synchronous",
         "ParaLLEl Synchronous RDP; enabled|disabled" },
      { "parallel-n64-parallel-rdp-overscan",
         "(ParaLLEl-RDP) Crop pixel border pixels; 0|2|4|6|8|10|12|14|16|18|20|22|24|26|28|30|32|34|36|38|40|42|44|46|48|50|52|54|56|58|60|62|64" },
      { "parallel-n64-parallel-rdp-divot-filter",
         "(ParaLLEl-RDP) VI divot filter; enabled|disabled" },
      { "parallel-n64-parallel-rdp-gamma-dither",
         "(ParaLLEl-RDP) VI gamma dither; enabled|disabled" },
      { "parallel-n64-parallel-rdp-vi-aa",
         "(ParaLLEl-RDP) VI AA; enabled|disabled" },
      { "parallel-n64-parallel-rdp-vi-bilinear",
         "(ParaLLEl-RDP) VI bilinear; enabled|disabled" },
      { "parallel-n64-parallel-rdp-dither-filter",
         "(ParaLLEl-RDP) VI dither filter; enabled|disabled" },
      { "parallel-n64-parallel-rdp-upscaling",
         "(ParaLLEl-RDP) Upscaling factor (restart); 1x|2x|4x|8x" },
      { "parallel-n64-parallel-rdp-downscaling",
         "(ParaLLEl-RDP) Downsampling; disable|1/2|1/4|1/8" },
      { "parallel-n64-parallel-rdp-native-texture-lod",
         "(ParaLLEl-RDP) Use native texture LOD when upscaling; disabled|enabled" },
      { "parallel-n64-parallel-rdp-native-tex-rect",
         "(ParaLLEl-RDP) Use native resolution for TEX_RECT; enabled|disabled" },
#endif
      { "parallel-n64-send_allist_to_hle_rsp",
         "Send audio lists to HLE RSP; disabled|enabled" },
      { "parallel-n64-gfxplugin",
         "GFX Plugin; auto"
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         "|glide64|gln64|rice"
#endif         
         "|angrylion"
#ifdef HAVE_PARALLEL
            "|parallel"
#endif
      },
      { "parallel-n64-rspplugin",
         "RSP Plugin; auto"
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
		 "|hle"
#endif
		 "|cxd4"
#ifdef HAVE_PARALLEL_RSP
         "|parallel"
#endif
         },
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES) || defined(HAVE_PARALLEL)
      { "parallel-n64-screensize",
#ifdef CLASSIC
         "Resolution (restart); 320x240|640x480|960x720|1280x960|1440x1080|1600x1200|1920x1440|2240x1680|2880x2160|5760x4320" },
#else
         "Resolution (restart); 640x480|960x720|1280x960|1440x1080|1600x1200|1920x1440|2240x1680|2880x2160|5760x4320|320x240" },	
#endif
      { "parallel-n64-aspectratiohint",
         "Aspect ratio hint (reinit); normal|widescreen" },
      { "parallel-n64-filtering",
		 "(Glide64) Texture Filtering; automatic|N64 3-point|bilinear|nearest" },
      { "parallel-n64-dithering",
		 "(Angrylion) Dithering; enabled|disabled" },
      { "parallel-n64-polyoffset-factor",
       "(Glide64) Polygon Offset Factor; -3.0|-2.5|-2.0|-1.5|-1.0|-0.5|0.0|0.5|1.0|1.5|2.0|2.5|3.0|3.5|4.0|4.5|5.0|-3.5|-4.0|-4.5|-5.0"
      },
      { "parallel-n64-polyoffset-units",
       "(Glide64) Polygon Offset Units; -3.0|-2.5|-2.0|-1.5|-1.0|-0.5|0.0|0.5|1.0|1.5|2.0|2.5|3.0|3.5|4.0|4.5|5.0|-3.5|-4.0|-4.5|-5.0"
      },
#endif
      { "parallel-n64-angrylion-vioverlay",
       "(Angrylion) VI Overlay; Filtered|AA+Blur|AA+Dedither|AA only|Unfiltered|Depth|Coverage"
      },
      { "parallel-n64-angrylion-sync",
       "(Angrylion) Thread sync level; Low|Medium|High"
      },
       { "parallel-n64-angrylion-multithread",
         "(Angrylion) Multi-threading; all threads|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32|33|34|35|36|37|38|39|40|41|42|43|44|45|46|47|48|49|50|51|52|53|54|55|56|57|58|59|60|61|62|63" },
       { "parallel-n64-angrylion-overscan",
         "(Angrylion) Hide overscan; disabled|enabled" },
      { "parallel-n64-virefresh",
         "VI Refresh (Overclock); auto|1500|2200" },
      { "parallel-n64-bufferswap",
         "Buffer Swap; disabled|enabled"
      },
      { "parallel-n64-framerate",
         "Framerate (restart); original|fullspeed" },

      { "parallel-n64-alt-map",
        "Independent C-button Controls; disabled|enabled" },

#if defined(HAVE_PARALLEL) || !defined(HAVE_OPENGL) || !defined(HAVE_OPENGLES)
      { "parallel-n64-vcache-vbo",
         "(Glide64) Vertex cache VBO (restart); disabled|enabled" },
#endif
      { "parallel-n64-boot-device",
         "Boot Device; Default|64DD IPL" },
      { "parallel-n64-64dd-hardware",
         "64DD Hardware; disabled|enabled" },
      { NULL, NULL },
   };

   static const struct retro_controller_description port[] = {
      { "Controller", RETRO_DEVICE_JOYPAD },
      { "Mouse", RETRO_DEVICE_MOUSE },
      { "RetroPad", RETRO_DEVICE_JOYPAD },
   };

   static const struct retro_controller_info ports[] = {
      { port, 3 },
      { port, 3 },
      { port, 3 },
      { port, 3 },
      { 0, 0 }
   };

   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
   environ_cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)subsystems);
}

bool is_cartridge_rom(const uint8_t* data)
{
   return (data != NULL && *((uint32_t *)data) != 0x16D348E8 && *((uint32_t *)data) != 0x56EE6322);
}

static bool emu_step_load_data()
{
   const char *dir;
   bool loaded = false;
   char slash;

   #if defined(_WIN32)
      slash = '\\';
   #else
      slash = '/';
   #endif

   if(CoreStartup(FRONTEND_API_VERSION, ".", ".", "Core", n64DebugCallback, 0, 0) && log_cb)
       log_cb(RETRO_LOG_ERROR, "mupen64plus: Failed to initialize core\n");

   if (cart_data != NULL && cart_size != 0)
   {
      /* N64 Cartridge loading */
      loaded = true;

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "EmuThread: M64CMD_ROM_OPEN\n");

      if(CoreDoCommand(M64CMD_ROM_OPEN, cart_size, (void*)cart_data))
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus: Failed to load ROM\n");
         goto load_fail;
      }

      free(cart_data);
      cart_data = NULL;

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "EmuThread: M64CMD_ROM_GET_HEADER\n");

      if(CoreDoCommand(M64CMD_ROM_GET_HEADER, sizeof(ROM_HEADER), &ROM_HEADER))
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus; Failed to query ROM header information\n");
         goto load_fail;
      }
   }
   if (disk_data != NULL && disk_size != 0)
   {
      /* 64DD Disk loading */
      char disk_ipl_path[256];
      FILE *fPtr;
      long romlength = 0;
      uint8_t* ipl_data = NULL;

      loaded = true;
      if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) || !dir)
         goto load_fail;

      /* connect saved_memory.disk to disk */
      g_dd_disk = saved_memory.disk;

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "EmuThread: M64CMD_DISK_OPEN\n");
      printf("M64CMD_DISK_OPEN\n");

      if(CoreDoCommand(M64CMD_DISK_OPEN, disk_size, (void*)disk_data))
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus: Failed to load DISK\n");
         goto load_fail;
      }

      free(disk_data);
      disk_data = NULL;

      /* 64DD IPL LOAD - assumes "64DD_IPL.bin" is in system folder */
      sprintf(disk_ipl_path, "%s%c64DD_IPL.bin", dir, slash);

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "64DD_IPL.bin path: %s\n", disk_ipl_path);

      fPtr = fopen(disk_ipl_path, "rb");
      if (fPtr == NULL)
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus: Failed to load DISK IPL\n");
         goto load_fail;
      }

      fseek(fPtr, 0L, SEEK_END);
      romlength = ftell(fPtr);
      fseek(fPtr, 0L, SEEK_SET);

      ipl_data = malloc(romlength);
      if (ipl_data == NULL)
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus: couldn't allocate DISK IPL buffer\n");
         fclose(fPtr);
         free(ipl_data);
         ipl_data = NULL;
         goto load_fail;
      }

      if (fread(ipl_data, 1, romlength, fPtr) != romlength)
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus: couldn't read DISK IPL file to buffer\n");
         fclose(fPtr);
         free(ipl_data);
         ipl_data = NULL;
         goto load_fail;
      }
      fclose(fPtr);

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "EmuThread: M64CMD_DDROM_OPEN\n");
      printf("M64CMD_DDROM_OPEN\n");

      if(CoreDoCommand(M64CMD_DDROM_OPEN, romlength, (void*)ipl_data))
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus: Failed to load DDROM\n");
         free(ipl_data);
         ipl_data = NULL;
         goto load_fail;
      }

      if (log_cb)
         log_cb(RETRO_LOG_INFO, "EmuThread: M64CMD_ROM_GET_HEADER\n");

      if(CoreDoCommand(M64CMD_ROM_GET_HEADER, sizeof(ROM_HEADER), &ROM_HEADER))
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "mupen64plus; Failed to query ROM header information\n");
         goto load_fail;
      }
   }
   return loaded;

load_fail:
   free(cart_data);
   cart_data = NULL;
   free(disk_data);
   disk_data = NULL;
   stop = 1;

   return false;
}

#ifdef HAVE_THR_AL
extern struct rgba prescale[PRESCALE_WIDTH * PRESCALE_HEIGHT];
#endif

bool emu_step_render(void)
{
   if (flip_only)
   {
      switch (gfx_plugin)
      {
         case GFX_ANGRYLION:
#ifdef HAVE_THR_AL
            video_cb(prescale, screen_width, screen_height, screen_pitch);
#endif
            break;

         case GFX_PARALLEL:
#if defined(HAVE_PARALLEL)
            parallel_profile_video_refresh_begin();
            video_cb(parallel_frame_is_valid() ? RETRO_HW_FRAME_BUFFER_VALID : NULL,
                    parallel_frame_width(), parallel_frame_height(), 0);
            parallel_profile_video_refresh_end();
#endif
            break;

         default:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
            video_cb(RETRO_HW_FRAME_BUFFER_VALID, screen_width, screen_height, 0);
#elif defined(HAVE_THR_AL)
            video_cb((screen_pitch == 0) ? NULL : prescale, screen_width, screen_height, screen_pitch);
#else
            video_cb(NULL, screen_width, screen_height, screen_pitch);
#endif
            break;
      }

      pushed_frame = true;
      return true;
   }

   if (!pushed_frame && frame_dupe) /* Dupe. Not duping violates libretro API, consider it a speedhack. */
      video_cb(NULL, screen_width, screen_height, screen_pitch);

   return false;
}

static void emu_step_initialize(void)
{
   if (emu_initialized)
      return;

   emu_initialized = true;

   core_settings_set_defaults();
   core_settings_autoselect_gfx_plugin();
   core_settings_autoselect_rsp_plugin();

   plugin_connect_all(gfx_plugin, rsp_plugin);

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "EmuThread: M64CMD_EXECUTE.\n");

   CoreDoCommand(M64CMD_EXECUTE, 0, NULL);
}

void reinit_gfx_plugin(void)
{
    if(first_context_reset)
    {
        first_context_reset = false;
#ifndef NO_LIBCO
        co_switch(game_thread);
#endif
    }

    switch (gfx_plugin)
    {
       case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
          glide64InitGfx();
#endif
          break;
       case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
          gles2n64_reset();
#endif
          break;
       case GFX_RICE:
#ifdef HAVE_RICE
          /* TODO/FIXME */
#endif
          break;
       case GFX_ANGRYLION:
          /* Stub */
          break;
       case GFX_PARALLEL:
#ifdef HAVE_PARALLEL
          if (!environ_cb(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &vulkan) || !vulkan)
          {
             if (log_cb)
                log_cb(RETRO_LOG_ERROR, "Failed to obtain Vulkan interface.\n");
          }
          else
             parallel_init(vulkan);
#endif
          break;
    }
}

void deinit_gfx_plugin(void)
{
    switch (gfx_plugin)
    {
       case GFX_PARALLEL:
#if defined(HAVE_PARALLEL)
          parallel_deinit();
#endif
          break;
       default:
          break;
    }
}

#ifdef NO_LIBCO
static void EmuThreadInit(void)
{
    emu_step_initialize();

    initializing = false;

    main_pre_run();
}

static void EmuThreadStep(void)
{
    stop_stepping = false;
    main_run();
}
#else
static void EmuThreadFunction(void)
{
    if (!emu_step_load_data())
       goto load_fail;

    /* ROM is loaded, switch back to main thread
     * so retro_load_game can return (returning failure if needed).
     * We'll continue here once the context is reset. */
    co_switch(main_thread);

    emu_step_initialize();

    /*Context is reset too, everything is safe to use.
     * Now back to main thread so we don't start pushing
     * frames outside retro_run. */
    co_switch(main_thread);

    initializing = false;
    main_pre_run();
    main_run();
    if (log_cb)
       log_cb(RETRO_LOG_INFO, "EmuThread: co_switch main_thread.\n");

    co_switch(main_thread);

load_fail:
    /*NEVER RETURN! That's how libco rolls */
    while(1)
    {
       if (log_cb)
          log_cb(RETRO_LOG_ERROR, "Running Dead N64 Emulator\n");
       co_switch(main_thread);
    }
}
#endif

const char* retro_get_system_directory(void)
{
    const char* dir;
    environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir);

    return dir ? dir : ".";
}


void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb)   { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_cb = cb; }


void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   setup_variables();
}

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name = "ParaLLEl N64";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version = "2.0-rc2" GIT_VERSION;
   info->valid_extensions = "n64|v64|z64|bin|u1|ndd";
   info->need_fullpath = false;
   info->block_extract = false;
}

/* Get the system type associated to a ROM country code. */
static m64p_system_type rom_country_code_to_system_type(char country_code)
{
    switch (country_code)
    {
        /* PAL codes */
        case 0x44:
        case 0x46:
        case 0x49:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x58:
        case 0x59:
            return SYSTEM_PAL;

        /* NTSC codes */
        case 0x37:
        case 0x41:
        case 0x45:
        case 0x4a:
        default: /* Fallback for unknown codes */
            return SYSTEM_NTSC;
    }
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   m64p_system_type region = rom_country_code_to_system_type(ROM_HEADER.destination_code);

   info->geometry.base_width   = screen_width;
   info->geometry.base_height  = screen_height;
   info->geometry.max_width    = screen_width;
   info->geometry.max_height   = screen_height;
   info->geometry.aspect_ratio = 4.0 / 3.0;
   info->timing.fps = (region == SYSTEM_PAL) ? 50.0 : (60.13);                /* TODO: Actual timing  */
   info->timing.sample_rate = 44100.0;
}

unsigned retro_get_region (void)
{
   m64p_system_type region = rom_country_code_to_system_type(ROM_HEADER.destination_code);
   return ((region == SYSTEM_PAL) ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
}

#if defined(HAVE_PARALLEL) || defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
static void context_reset(void)
{
   switch (gfx_plugin)
   {
      case GFX_ANGRYLION:
      case GFX_PARALLEL:
         break;
      default:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         {
            static bool first_init = true;
            printf("context_reset.\n");
            glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);

            if (first_init)
            {
               glsm_ctl(GLSM_CTL_STATE_SETUP, NULL);
               first_init = false;
            }
         }
#endif
         break;
   }

   reinit_gfx_plugin();
}

static void context_destroy(void)
{
   deinit_gfx_plugin();
}
#endif

static bool retro_init_vulkan(void)
{
#if defined(HAVE_PARALLEL)
   hw_render.context_type    = RETRO_HW_CONTEXT_VULKAN;
   hw_render.version_major   = VK_MAKE_VERSION(1, 0, 12);
   hw_render.context_reset   = context_reset;
   hw_render.context_destroy = context_destroy;

   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "mupen64plus: libretro frontend doesn't have Vulkan support.\n");
      return false;
   }

   hw_context_negotiation.interface_type = RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN;
   hw_context_negotiation.interface_version = RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN_VERSION;
   hw_context_negotiation.get_application_info = parallel_get_application_info;
   hw_context_negotiation.create_device = parallel_create_device;
   hw_context_negotiation.destroy_device = NULL;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE, &hw_context_negotiation))
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "mupen64plus: libretro frontend doesn't have context negotiation support.\n");
   }

   return true;
#else
   return false;
#endif
}

static bool context_framebuffer_lock(void *data)
{
   if (!stop)
      return false;
   return true;
}

static bool retro_init_gl(void)
{
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   glsm_ctx_params_t params     = {0};

   params.context_reset         = context_reset;
   params.context_destroy       = context_destroy;
   params.environ_cb            = environ_cb;
   params.stencil               = false;

   params.framebuffer_lock      = context_framebuffer_lock;

   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "mupen64plus: libretro frontend doesn't have OpenGL support.\n");
      return false;
   }

   return true;
#else
   return false;
#endif
}

void retro_init(void)
{
   struct retro_log_callback log;
   unsigned colorMode = RETRO_PIXEL_FORMAT_XRGB8888;
   uint64_t serialization_quirks = RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE;

   screen_pitch = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;
   else
      perf_get_cpu_features_cb = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &colorMode);
   environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble);

   environ_cb(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &serialization_quirks);
   initializing = true;

   /* hacky stuff for Glide64 */
   polygonOffsetUnits = -3.0f;
   polygonOffsetFactor =  -3.0f;

#ifndef NO_LIBCO
   main_thread = co_active();
   game_thread = co_create(65536 * sizeof(void*) * 16, EmuThreadFunction);
#endif

}

void retro_deinit(void)
{
   mupen_main_stop();
   mupen_main_exit();

#ifndef NO_LIBCO
   co_delete(game_thread);
#endif

   deinit_audio_libretro();

   if (perf_cb.perf_log)
      perf_cb.perf_log();

   vulkan_inited     = false;
   gl_inited         = false;
}


#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
extern void glide_set_filtering(unsigned value);
#endif
extern void angrylion_set_vi(unsigned value);
extern void angrylion_set_filtering(unsigned value);
extern void angrylion_set_dithering(unsigned value);
extern void  angrylion_set_threads(unsigned value);
extern void  angrylion_set_overscan(unsigned value);
extern void  angrylion_set_vi_dedither(unsigned value);
extern void  angrylion_set_vi_blur(unsigned value);

extern void angrylion_set_synclevel(unsigned value);
extern void ChangeSize();

static void gfx_set_filtering(void)
{
     if (log_cb)
        log_cb(RETRO_LOG_DEBUG, "set filtering mode...\n");
     switch (gfx_plugin)
     {
        case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
           glide_set_filtering(retro_filtering);
#endif
           break;
        case GFX_ANGRYLION:
#ifdef HAVE_THR_AL
           angrylion_set_filtering(retro_filtering);
#endif
           break;
        case GFX_RICE:
#ifdef HAVE_RICE
           /* TODO/FIXME */
#endif
           break;
        case GFX_PARALLEL:
#ifdef HAVE_PARALLEL
           /* Stub */
#endif
           break;
        case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
           /* Stub */
#endif
           break;
     }
}

unsigned setting_get_dithering(void)
{
   return retro_dithering;
}

static void gfx_set_dithering(void)
{
   if (log_cb)
      log_cb(RETRO_LOG_DEBUG, "set dithering mode...\n");

   switch (gfx_plugin)
   {
      case GFX_GLIDE64:
#ifdef HAVE_GLIDE64
         /* Stub */
#endif
         break;
      case GFX_ANGRYLION:
#ifdef HAVE_THR_AL
         angrylion_set_vi_dedither(!retro_dithering);
         angrylion_set_dithering(retro_dithering);
#endif
         break;
      case GFX_RICE:
#ifdef HAVE_RICE
         /* Stub */
#endif
         break;
      case GFX_PARALLEL:
         break;
      case GFX_GLN64:
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         /* Stub */
#endif
         break;
     }
}

void update_variables(bool startup)
{
   struct retro_variable var;

#if defined(HAVE_PARALLEL)
   var.key = "parallel-n64-parallel-rdp-synchronous";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      parallel_set_synchronous_rdp(!strcmp(var.value, "enabled"));
   else
      parallel_set_synchronous_rdp(true);

   var.key = "parallel-n64-parallel-rdp-overscan";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   parallel_set_overscan_crop(strtol(var.value, NULL, 0));
   else
	   parallel_set_overscan_crop(0);

   var.key = "parallel-n64-parallel-rdp-divot-filter";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   parallel_set_divot_filter(!strcmp(var.value, "enabled"));
   else
	   parallel_set_divot_filter(true);

   var.key = "parallel-n64-parallel-rdp-gamma-dither";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   parallel_set_gamma_dither(!strcmp(var.value, "enabled"));
   else
	   parallel_set_gamma_dither(true);

   var.key = "parallel-n64-parallel-rdp-vi-aa";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   parallel_set_vi_aa(!strcmp(var.value, "enabled"));
   else
	   parallel_set_vi_aa(true);

   var.key = "parallel-n64-parallel-rdp-vi-bilinear";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   parallel_set_vi_scale(!strcmp(var.value, "enabled"));
   else
	   parallel_set_vi_scale(true);

   var.key = "parallel-n64-parallel-rdp-dither-filter";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   parallel_set_dither_filter(!strcmp(var.value, "enabled"));
   else
	   parallel_set_dither_filter(true);

   var.key = "parallel-n64-parallel-rdp-upscaling";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
       parallel_set_upscaling(strtol(var.value, NULL, 0));
   else
       parallel_set_upscaling(1);

   var.key = "parallel-n64-parallel-rdp-downscaling";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
       if (!strcmp(var.value, "disable"))
           parallel_set_downscaling_steps(0);
       else if (!strcmp(var.value, "1/2"))
           parallel_set_downscaling_steps(1);
       else if (!strcmp(var.value, "1/4"))
           parallel_set_downscaling_steps(2);
       else if (!strcmp(var.value, "1/8"))
           parallel_set_downscaling_steps(3);
   }
   else
       parallel_set_downscaling_steps(0);

   var.key = "parallel-n64-parallel-rdp-native-texture-lod";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
       parallel_set_native_texture_lod(!strcmp(var.value, "enabled"));
   else
       parallel_set_native_texture_lod(false);

   var.key = "parallel-n64-parallel-rdp-native-tex-rect";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
       parallel_set_native_tex_rect(!strcmp(var.value, "enabled"));
   else
       parallel_set_native_tex_rect(true);
#endif

   var.key   = "parallel-n64-send_allist_to_hle_rsp";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if(!strcmp(var.value, "enabled"))
         send_allist_to_hle_rsp = true;
      else
         send_allist_to_hle_rsp = false;
   }
   else
      send_allist_to_hle_rsp = false;

   var.key   = "parallel-n64-screensize";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      /* TODO/FIXME - hack - force screen width and height back to 640x480 in case
       * we change it with Angrylion. If we ever want to support variable resolution sizes in Angrylion
       * then we need to drop this. */
      if (
#ifdef HAVE_THR_AL
            gfx_plugin == GFX_ANGRYLION || 
#endif
            sscanf(var.value ? var.value : "640x480", "%dx%d", &screen_width, &screen_height) != 2)
      {
         screen_width = 640;
         screen_height = 480;
      }
   }
   else
   {
      screen_width  = 640;
      screen_height = 480;
   }

   if (startup)
   {
      var.key = "parallel-n64-audio-buffer-size";
      var.value = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         audio_buffer_size = atoi(var.value);

      var.key = "parallel-n64-gfxplugin";
      var.value = NULL;

      environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);

      if (var.value)
      {
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64) || defined(HAVE_RICE) || defined(HAVE_GLIDE64) || defined(HAVE_THR_AL) || defined(HAVE_PARALLEL)
         if (!strcmp(var.value, "auto"))
#if defined(HAVE_GLN64) || defined(HAVE_GLIDEN64)
         if (!strcmp(var.value, "gln64"))
            gfx_plugin = GFX_GLN64;
#endif
#ifdef HAVE_RICE
         if (!strcmp(var.value, "rice"))
            gfx_plugin = GFX_RICE;
#endif
#ifdef HAVE_GLIDE64
         if(!strcmp(var.value, "glide64"))
            gfx_plugin = GFX_GLIDE64;
#endif
#ifdef HAVE_THR_AL
         if(!strcmp(var.value, "angrylion"))
            gfx_plugin = GFX_ANGRYLION;
#endif
#ifdef HAVE_PARALLEL
         if(!strcmp(var.value, "parallel"))
            gfx_plugin = GFX_PARALLEL;
#endif
#endif
      }
      else
         core_settings_autoselect_gfx_plugin();
   }

   
#ifdef HAVE_THR_AL
   var.key = "parallel-n64-angrylion-vioverlay";
   var.value = NULL;

   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);

   if (var.value)
   {
      if(!strcmp(var.value, "Filtered"))
      {
         angrylion_set_vi(0);
         angrylion_set_vi_dedither(1);
         angrylion_set_vi_blur(1);
      }
      else if(!strcmp(var.value, "AA+Blur"))
      {
         angrylion_set_vi(0);
         angrylion_set_vi_dedither(0);
         angrylion_set_vi_blur(1);
      }
      else if(!strcmp(var.value, "AA+Dedither"))
      {
         angrylion_set_vi(0);
         angrylion_set_vi_dedither(1);
         angrylion_set_vi_blur(0);
      }
      else if(!strcmp(var.value, "AA only"))
      {
         angrylion_set_vi(0);
         angrylion_set_vi_dedither(0);
         angrylion_set_vi_blur(0);
      }
      else if(!strcmp(var.value, "Unfiltered"))
      {
         angrylion_set_vi(1);
         angrylion_set_vi_dedither(1);
         angrylion_set_vi_blur(1);
      }
      else if(!strcmp(var.value, "Depth"))
      {
         angrylion_set_vi(2);
         angrylion_set_vi_dedither(1);
         angrylion_set_vi_blur(1);
      }
      else if(!strcmp(var.value, "Coverage"))
      {
         angrylion_set_vi(3);
         angrylion_set_vi_dedither(1);
         angrylion_set_vi_blur(1);
      }
   }
   else
   {
      angrylion_set_vi(0);
      angrylion_set_vi_dedither(1);
      angrylion_set_vi_blur(1);
   }

   var.key = "parallel-n64-angrylion-sync";
   var.value = NULL;

   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);

   if (var.value)
   {
      if(!strcmp(var.value, "High"))
         angrylion_set_synclevel(2);
      else if(!strcmp(var.value, "Medium"))
         angrylion_set_synclevel(1);
      else if(!strcmp(var.value, "Low"))
         angrylion_set_synclevel(0);
   }
   else
      angrylion_set_synclevel(0);

   var.key = "parallel-n64-angrylion-multithread";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if(!strcmp(var.value, "all threads"))
         angrylion_set_threads(0);
      else
         angrylion_set_threads(atoi(var.value));
   }
   else
      angrylion_set_threads(0);

   var.key = "parallel-n64-angrylion-overscan";
   var.value = NULL;

   environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var);

   if (var.value)
   {
      if(!strcmp(var.value, "enabled"))
         angrylion_set_overscan(1);
      else if(!strcmp(var.value, "disabled"))
         angrylion_set_overscan(0);
   }
   else
      angrylion_set_overscan(0);
#endif


   CFG_HLE_GFX = 0;

#ifdef HAVE_THR_AL
   if (gfx_plugin != GFX_ANGRYLION)
      CFG_HLE_GFX = 1;
#endif

#ifdef HAVE_PARALLEL
   if (gfx_plugin != GFX_PARALLEL)
      CFG_HLE_GFX = 1;
#endif
   CFG_HLE_AUD = 0; /* There is no HLE audio code in libretro audio plugin. */

   var.key = "parallel-n64-filtering";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      static signed old_filtering = -1;
      if (!strcmp(var.value, "automatic"))
         retro_filtering = 0;
      else if (!strcmp(var.value, "N64 3-point"))
#ifdef DISABLE_3POINT
         retro_filtering = 3;
#else
         retro_filtering = 1;
#endif
      else if (!strcmp(var.value, "nearest"))
         retro_filtering = 2;
      else if (!strcmp(var.value, "bilinear"))
         retro_filtering = 3;

      if (retro_filtering != old_filtering)
	gfx_set_filtering();

      old_filtering      = retro_filtering;
   }

   var.key = "parallel-n64-dithering";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      static signed old_dithering = -1;

      if (!strcmp(var.value, "enabled"))
         retro_dithering = 1;
      else if (!strcmp(var.value, "disabled"))
         retro_dithering = 0;

      gfx_set_dithering();

      old_dithering      = retro_dithering;
   }
   else
   {
      retro_dithering = 1;
      gfx_set_dithering();
   }

   var.key = "parallel-n64-polyoffset-factor";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      float new_val = (float)atoi(var.value);
      polygonOffsetFactor = new_val;
   }

   var.key = "parallel-n64-polyoffset-units";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      float new_val = (float)atoi(var.value);
      polygonOffsetUnits = new_val;
   }

   var.key = "parallel-n64-astick-deadzone";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      astick_deadzone = (int)(atoi(var.value) * 0.01f * 0x8000);

   var.key = "parallel-n64-astick-snap-angle-active";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         astick_snap_active = 1;
      else if (!strcmp(var.value, "disabled"))
         astick_snap_active = 0;
   }

   var.key = "parallel-n64-astick-snap-angle";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      astick_snap_max_angle = (int)(atoi(var.value));
   }

   var.key = "parallel-n64-astick-snap-min-displacement-percent";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      astick_snap_min_displacement_percent = (int)(atoi(var.value));
   }

   var.key = "parallel-n64-astick-sensitivity";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      astick_sensitivity = atoi(var.value);

   var.key = "parallel-n64-gfxplugin-accuracy";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
       if (var.value && !strcmp(var.value, "veryhigh"))
          gfx_plugin_accuracy = 3;
       else if (var.value && !strcmp(var.value, "high"))
          gfx_plugin_accuracy = 2;
       else if (var.value && !strcmp(var.value, "medium"))
          gfx_plugin_accuracy = 1;
       else if (var.value && !strcmp(var.value, "low"))
          gfx_plugin_accuracy = 0;
   }

   var.key = "parallel-n64-virefresh";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "auto")) { }
      else if (!strcmp(var.value, "1500"))
         g_vi_refresh_rate = 1500;
      else if (!strcmp(var.value, "2200"))
         g_vi_refresh_rate = 2200;
   }

   var.key = "parallel-n64-bufferswap";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         BUFFERSWAP = true;
      else if (!strcmp(var.value, "disabled"))
         BUFFERSWAP = false;
   }

   var.key = "parallel-n64-framerate";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && initial_boot)
   {
      if (!strcmp(var.value, "original"))
         frame_dupe = false;
      else if (!strcmp(var.value, "fullspeed"))
         frame_dupe = true;
   }

   var.key = "parallel-n64-alt-map";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && startup)
   {
      if (!strcmp(var.value, "disabled"))
         alternate_mapping = false;
      else if (!strcmp(var.value, "enabled"))
         alternate_mapping = true;
   }


   {
      struct retro_variable pk1var = { "parallel-n64-pak1" };
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &pk1var) && pk1var.value)
      {
         int p1_pak = PLUGIN_NONE;
         if (!strcmp(pk1var.value, "rumble"))
            p1_pak = PLUGIN_RAW;
         else if (!strcmp(pk1var.value, "memory"))
            p1_pak = PLUGIN_MEMPAK;

         /* If controller struct is not initialised yet, set pad_pak_types instead
          * which will be looked at when initialising the controllers. */
         if (controller[0].control)
            controller[0].control->Plugin = p1_pak;
         else
            pad_pak_types[0] = p1_pak;

      }
   }

   {
      struct retro_variable pk2var = { "parallel-n64-pak2" };
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &pk2var) && pk2var.value)
      {
         int p2_pak = PLUGIN_NONE;
         if (!strcmp(pk2var.value, "rumble"))
            p2_pak = PLUGIN_RAW;
         else if (!strcmp(pk2var.value, "memory"))
            p2_pak = PLUGIN_MEMPAK;

         if (controller[1].control)
            controller[1].control->Plugin = p2_pak;
         else
            pad_pak_types[1] = p2_pak;

      }
   }

   {
      struct retro_variable pk3var = { "parallel-n64-pak3" };
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &pk3var) && pk3var.value)
      {
         int p3_pak = PLUGIN_NONE;
         if (!strcmp(pk3var.value, "rumble"))
            p3_pak = PLUGIN_RAW;
         else if (!strcmp(pk3var.value, "memory"))
            p3_pak = PLUGIN_MEMPAK;

         if (controller[2].control)
            controller[2].control->Plugin = p3_pak;
         else
            pad_pak_types[2] = p3_pak;

      }
   }

   {
      struct retro_variable pk4var = { "parallel-n64-pak4" };
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &pk4var) && pk4var.value)
      {
         int p4_pak = PLUGIN_NONE;
         if (!strcmp(pk4var.value, "rumble"))
            p4_pak = PLUGIN_RAW;
         else if (!strcmp(pk4var.value, "memory"))
            p4_pak = PLUGIN_MEMPAK;

         if (controller[3].control)
            controller[3].control->Plugin = p4_pak;
         else
            pad_pak_types[3] = p4_pak;
      }
   }


}

static void format_saved_memory(void)
{
   format_sram(saved_memory.sram);
   format_eeprom(saved_memory.eeprom, sizeof(saved_memory.eeprom));
   format_flashram(saved_memory.flashram);
   format_mempak(saved_memory.mempack[0]);
   format_mempak(saved_memory.mempack[1]);
   format_mempak(saved_memory.mempack[2]);
   format_mempak(saved_memory.mempack[3]);
   format_disk(saved_memory.disk);
}

bool retro_load_game(const struct retro_game_info *game)
{
   format_saved_memory();

   update_variables(true);
   initial_boot = false;

   init_audio_libretro(audio_buffer_size);

#ifdef HAVE_THR_AL
   if (gfx_plugin != GFX_ANGRYLION)
#endif
   {
      if (gfx_plugin == GFX_PARALLEL)
      {
         retro_init_vulkan();
         vulkan_inited = true;
      }
      else
      {
         retro_init_gl();
         gl_inited = true;
      }
   }

   if (vulkan_inited)
   {
      switch (gfx_plugin)
      {
         case GFX_GLIDE64:
         case GFX_GLN64:
         case GFX_RICE:
            gfx_plugin = GFX_PARALLEL;
            break;
         default:
            break;
      }

      switch (rsp_plugin)
      {
         case RSP_HLE:
#if defined(HAVE_PARALLEL_RSP)
            rsp_plugin = RSP_PARALLEL;
#else
            rsp_plugin = RSP_CXD4;
#endif
            break;
         default:
            break;
      }
   }
   else if (gl_inited)
   {
      switch (gfx_plugin)
      {
         case GFX_PARALLEL:
            gfx_plugin = GFX_GLIDE64;
            break;
         default:
            break;
      }

      switch (rsp_plugin)
      {
         case RSP_PARALLEL:
            rsp_plugin = RSP_HLE;
         default:
            break;
      }
   }

   if (is_cartridge_rom(game->data))
   {
      cart_data = malloc(game->size);
      cart_size = game->size;
      memcpy(cart_data, game->data, game->size);
   }
   else
   {
      disk_data = malloc(game->size);
      disk_size = game->size;
      memcpy(disk_data, game->data, game->size);
   }

   stop      = false;
   /* Finish ROM load before doing anything funny,
    * so we can return failure if needed. */
#ifdef NO_LIBCO
    emu_step_load_data();
#else
   co_switch(game_thread);
#endif

   if (stop)
      return false;

   first_context_reset = true;

   return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   if (game_type == SUBSYSTEM_CART_DISK)
   {
      if (!info[1].data || info[1].size == 0)
         return false;
      
      disk_size = info[1].size;
      disk_data = malloc(disk_size);
      memcpy(disk_data, info[1].data, disk_size);

      return retro_load_game(&info[0]);
   }
 
   return false;
}

void retro_unload_game(void)
{
    stop = 1;
    first_time = 1;

#ifdef NO_LIBCO
    EmuThreadStep();
#else
    co_switch(game_thread);
#endif

    CoreDoCommand(M64CMD_ROM_CLOSE, 0, NULL);
    emu_initialized = false;
}

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
static void glsm_exit(void)
{
#ifndef HAVE_SHARED_CONTEXT
   if (stop)
      return;
#ifdef HAVE_THR_AL
   if (gfx_plugin == GFX_ANGRYLION)
      return;
#endif
#ifdef HAVE_PARALLEL
   if (gfx_plugin == GFX_PARALLEL)
      return;
#endif
   glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);
#endif
}

static void glsm_enter(void)
{
#ifndef HAVE_SHARED_CONTEXT
   if (stop)
      return;
#ifdef HAVE_THR_AL
   if (gfx_plugin == GFX_ANGRYLION)
      return;
#endif
#ifdef HAVE_PARALLEL
   if (gfx_plugin == GFX_PARALLEL)
      return;
#endif
   glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
#endif
}
#endif

void retro_run (void)
{
   static bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
   {
      static float last_aspect = 4.0 / 3.0;
      struct retro_variable var;

      update_variables(false);

      var.key = "parallel-n64-aspectratiohint";
      var.value = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      {
         float aspect_val = 4.0 / 3.0;
         float aspectmode = 0;

         if (!strcmp(var.value, "widescreen"))
         {
            aspect_val = 16.0 / 9.0;
            aspectmode = 1;
         }
         else if (!strcmp(var.value, "normal"))
         {
            aspect_val = 4.0 / 3.0;
            aspectmode = 0;
         }

         if (aspect_val != last_aspect)
         {
            screen_aspectmodehint = aspectmode;

            switch (gfx_plugin)
            {
               case GFX_GLIDE64:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
                  ChangeSize();
#endif
                  break;
               case GFX_RICE:
#ifdef HAVE_RICE
                  /* Stub */
#endif
                  break;
               case GFX_GLN64:
#ifdef HAVE_GLN64
                  /* Stub */
#endif
                  break;
               case GFX_PARALLEL:
#ifdef HAVE_PARALLEL
                  /* Stub */
#endif
                  break;
               case GFX_ANGRYLION:
                  /* Stub */
                  break;
            }

            last_aspect = aspect_val;
            reinit_screen = true;
         }
      }
   }

   FAKE_SDL_TICKS += 16;
   pushed_frame = false;

   if (reinit_screen)
   {
      bool ret;
      struct retro_system_av_info info;
      retro_get_system_av_info(&info);
      switch (screen_aspectmodehint)
      {
         case 0:
            info.geometry.aspect_ratio = 4.0 / 3.0;
            break;
         case 1:
            info.geometry.aspect_ratio = 16.0 / 9.0;
            break;
      }
      ret = environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
      reinit_screen = false;
   }

   do
   {
      switch (gfx_plugin)
      {
         case GFX_GLIDE64:
         case GFX_GLN64:
         case GFX_RICE:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
            glsm_enter();
#endif
            break;
         case GFX_PARALLEL:
#if defined(HAVE_PARALLEL)
            parallel_begin_frame();
#endif
            break;
         case GFX_ANGRYLION:
            break;
      }

      if (first_time)
      {
         first_time = 0;
         emu_step_initialize();
         /* Additional check for vioverlay not set at start */
         update_variables(false);
         gfx_set_filtering();
#ifdef NO_LIBCO
         EmuThreadInit();
#endif
      }

#ifdef NO_LIBCO
      EmuThreadStep();
#else
      co_switch(game_thread);
#endif

      switch (gfx_plugin)
      {
         case GFX_GLIDE64:
         case GFX_GLN64:
         case GFX_RICE:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
            glsm_exit();
#endif
            break;
         case GFX_PARALLEL:
         case GFX_ANGRYLION:
            break;
      }
   } while (emu_step_render());
}

void retro_reset (void)
{
    CoreDoCommand(M64CMD_RESET, 1, (void*)0);
}

void *retro_get_memory_data(unsigned type)
{
   switch (type)
   {
   case RETRO_MEMORY_SYSTEM_RAM: return g_rdram;
   case RETRO_MEMORY_SAVE_RAM:   return &saved_memory;
   }

   return NULL;
}

size_t retro_get_memory_size(unsigned type)
{
   switch (type)
   {
   case RETRO_MEMORY_SYSTEM_RAM:
      return RDRAM_MAX_SIZE;

   case RETRO_MEMORY_SAVE_RAM:
      if (type != RETRO_MEMORY_SAVE_RAM)
            return 0;

      if (g_dd_disk)
            return sizeof(saved_memory);

      return sizeof(saved_memory)-sizeof(saved_memory.disk);
   }

   return 0;
}

size_t retro_serialize_size (void)
{
    return 16788288 + 1024; /* < 16MB and some change... ouch */
}

bool retro_serialize(void *data, size_t size)
{
    if (initializing)
       return false;

    if (savestates_save_m64p(data, size))
        return true;

    return false;
}

bool retro_unserialize(const void * data, size_t size)
{
    if (initializing)
       return false;

    if (savestates_load_m64p(data, size))
        return true;

    return false;
}

/*Needed to be able to detach controllers
 * for Lylat Wars multiplayer
 *
 * Only sets if controller struct is
 * initialised as addon paks do.
 */
void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
   if (in_port < 4)
   {
      switch(device)
      {
         case RETRO_DEVICE_NONE:
            if (controller[in_port].control){
               controller[in_port].control->Present = 0;
               break;
            } else {
               pad_present[in_port] = 0;
               break;
            }

         case RETRO_DEVICE_MOUSE:
            if (controller[in_port].control){
               controller[in_port].control->Present = 2;
               break;
            } else {
               pad_present[in_port] = 2;
               break;
            }

         case RETRO_DEVICE_JOYPAD:
         default:
            if (controller[in_port].control){
               controller[in_port].control->Present = 1;
               break;
            } else {
               pad_present[in_port] = 1;
               break;
            }
      }
   }
}

/* Stubs */
unsigned retro_api_version(void) { return RETRO_API_VERSION; }

void retro_cheat_reset(void)
{
	cheat_delete_all();
}

void retro_cheat_set(unsigned index, bool enabled, const char* codeLine)
{
	char name[256];
	m64p_cheat_code mupenCode[256];
	int matchLength=0,partCount=0;
	uint32_t codeParts[256];
	int cursor;

	//Generate a name
	sprintf(name, "cheat_%u",index);

	//Break the code into Parts
	for (cursor=0;;cursor++)
   {
      if (ISHEXDEC)
         matchLength++;
      else
      {
         if (matchLength)
         {
            char *codePartS = (char*)calloc(matchLength + 1, sizeof(*codePartS));
            if (codePartS == NULL)
            {
               log_cb(RETRO_LOG_ERROR, "Unable to calloc memory in retro_cheat_set. Cheat will not be applied\n");
            }
            else 
            {
               strncpy(codePartS,codeLine+cursor-matchLength,matchLength);
               codePartS[matchLength]=0;
               codeParts[partCount++]=strtoul(codePartS,NULL,16);
               matchLength=0;

               free(codePartS);
            }
         }
      }
      if (!codeLine[cursor])
         break;
   }

	//Assign the parts to mupenCode
	for (cursor=0;2*cursor+1<partCount;cursor++)
   {
      mupenCode[cursor].address=codeParts[2*cursor];
      mupenCode[cursor].value=codeParts[2*cursor+1];
   }

	//Assign to mupenCode
	cheat_add_new(name,mupenCode,partCount/2);
	cheat_set_enabled(name,enabled);
}


void vbo_disable(void);

int retro_stop_stepping(void)
{
#ifdef NO_LIBCO
    return stop_stepping;
#else
    return false;
#endif
}

int retro_return(bool just_flipping)
{
   if (stop)
      return 0;

#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
   vbo_disable();
#endif

#ifdef NO_LIBCO
   if (just_flipping)
   {
      /* HACK: in case the VI comes before the render? is that possible?
       * remove this when we totally remove libco */
      flip_only = 1;
      emu_step_render();
      flip_only = 0;
   }
   else
      flip_only = just_flipping;

   stop_stepping = true;
#else
   flip_only = just_flipping;
   co_switch(main_thread);
#endif

   return 0;
}
