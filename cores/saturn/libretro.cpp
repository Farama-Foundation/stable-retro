#include <stdarg.h>
#include <compat/msvc.h>
#include <libretro.h>
#include <rthreads/rthreads.h>
#include <string/stdstring.h>
#include <streams/file_stream.h>

#include <ctype.h>
#include <time.h>

#include "mednafen/mednafen-types.h"
#include "mednafen/git.h"
#include "mednafen/general.h"
#include "mednafen/mempatcher.h"
#ifdef NEED_DEINTERLACER
#include "mednafen/video/surface.h"
#include "mednafen/video/Deinterlacer.h"
#endif
#include "mednafen/cdrom/CDUtility.h"

#include "mednafen/ss/ss.h"
#include "mednafen/ss/cart.h"
#include "mednafen/ss/db.h"
#include "mednafen/ss/smpc.h"
#include "mednafen/ss/sound.h"

#include "libretro_core_options.h"
#include "libretro_settings.h"
#include "input.h"
#include "disc.h"


#define MEDNAFEN_CORE_NAME                   "Beetle Saturn"
#define MEDNAFEN_CORE_VERSION                "v1.29.0"
#define MEDNAFEN_CORE_VERSION_NUMERIC        0x00102900
#define MEDNAFEN_CORE_EXTENSIONS             "cue|ccd|chd|toc|m3u"
#define MEDNAFEN_CORE_TIMING_FPS             59.82
#define MEDNAFEN_CORE_GEOMETRY_BASE_W        320
#define MEDNAFEN_CORE_GEOMETRY_BASE_H        240
#define MEDNAFEN_CORE_GEOMETRY_MAX_W         704
#define MEDNAFEN_CORE_GEOMETRY_MAX_H         576
#define MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO  (4.0 / 3.0)
#define FB_WIDTH                             MEDNAFEN_CORE_GEOMETRY_MAX_W

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;
retro_log_printf_t log_cb                         = NULL;
static retro_audio_sample_t audio_cb              = NULL;
static retro_audio_sample_batch_t audio_batch_cb  = NULL;
static retro_input_poll_t input_poll_cb           = NULL;
static retro_input_state_t input_state_cb         = NULL;
static retro_environment_t environ_cb             = NULL;
static retro_video_refresh_t video_cb             = NULL;

static unsigned frame_count = 0;
static unsigned internal_frame_count = 0;
static bool failed_init = false;
static unsigned image_offset = 0;
static unsigned image_crop = 0;

static unsigned h_mask = 0;
static unsigned first_sl = 0;
static unsigned last_sl = 239;
static unsigned first_sl_pal = 0;
static unsigned last_sl_pal = 287;
bool is_pal = false;

// Sets how often (in number of output frames/retro_run invocations)
// the internal framerace counter should be updated if
// display_internal_framerate is true.
#define INTERNAL_FPS_SAMPLE_PERIOD 32

char retro_save_directory[4096];
char retro_base_directory[4096];
static char retro_cd_base_directory[4096];
static char retro_cd_path[4096];
char retro_cd_base_name[4096];

#ifndef RETRO_SLASH
#ifdef _WIN32
#define RETRO_SLASH "\\"
#else
#define RETRO_SLASH "/"
#endif
#endif

static bool libretro_supports_option_categories = false;
static bool libretro_supports_bitmasks = false;

MDFNGI *MDFNGameInfo = NULL;

extern MDFNGI EmulatedSS;

// forward declarations
 bool MDFN_COLD InitCommon(const unsigned cpucache_emumode, const unsigned horrible_hacks, const unsigned cart_type, const unsigned smpc_area);
 MDFN_COLD void CloseGame(void);
 void Emulate(EmulateSpecStruct* espec_arg);

//forward decls
static bool overscan;
static double last_sound_rate;


#ifdef NEED_DEINTERLACER
static bool PrevInterlaced;
static Deinterlacer deint;
#endif

static MDFN_Surface *surf = NULL;

static void alloc_surface(void)
{
  MDFN_PixelFormat pix_fmt(MDFN_COLORSPACE_RGB, 16, 8, 0, 24);
  uint32_t width  = MEDNAFEN_CORE_GEOMETRY_MAX_W;
  uint32_t height = MEDNAFEN_CORE_GEOMETRY_MAX_H;

  if (surf != NULL)
    delete surf;

  surf = new MDFN_Surface(NULL, width, height, width, pix_fmt);
}

static void check_system_specs(void)
{
   // Hints that we need a fairly powerful system to run this.
   unsigned level = 15;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   // stub
}

/* LED interface */
extern int Running; // variable in ss.cpp
extern uint8 GetDriveStatus(void);
static retro_set_led_state_t led_state_cb = NULL;
static unsigned int retro_led_state[2] = {0};
static void retro_led_interface(void)
{
   /* 0: Power
    * 1: CD */

   unsigned int led_state[2] = {0};
   unsigned int l            = 0;

   unsigned int drive_status = GetDriveStatus();
   /* Active values:
    * STATUS_BUSY	 = 0x00,
    * STATUS_PLAY	 = 0x03,
    * STATUS_SEEK	 = 0x04,
    * STATUS_SCAN	 = 0x05, */

   led_state[0] = (!Running) ? 1 : 0;
   led_state[1] = (drive_status == 0 || drive_status == 3 || drive_status == 4 || drive_status == 5) ? 1 : 0;

   for (l = 0; l < sizeof(led_state)/sizeof(led_state[0]); l++)
   {
      if (retro_led_state[l] != led_state[l])
      {
         retro_led_state[l] = led_state[l];
         led_state_cb(l, led_state[l]);
      }
   }
}

void retro_init(void)
{
   struct retro_log_callback log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = fallback_log;

   CDUtility_Init();

   const char *dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
   {
      snprintf(retro_base_directory, sizeof(retro_base_directory), "%s", dir);
   }
   else
   {
      /* TODO: Add proper fallback */
      log_cb(RETRO_LOG_WARN, "System directory is not defined. Fallback on using same dir as ROM for system directory later ...\n");
      failed_init = true;
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
   {
      // If save directory is defined use it, otherwise use system directory
      if (dir)
         snprintf(retro_save_directory, sizeof(retro_save_directory), "%s", dir);
      else
         snprintf(retro_save_directory, sizeof(retro_save_directory), "%s", retro_base_directory);
   }
   else
   {
      /* TODO: Add proper fallback */
      log_cb(RETRO_LOG_WARN, "Save directory is not defined. Fallback on using SYSTEM directory ...\n");
      snprintf(retro_save_directory, sizeof(retro_save_directory), "%s", retro_base_directory);
   }

   disc_init( environ_cb );

   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;
   else
      perf_get_cpu_features_cb = NULL;

   setting_region = 0; // auto
   setting_smpc_autortc = true;
   setting_smpc_autortc_lang = 0;
   setting_initial_scanline = 0;
   setting_last_scanline = 239;
   setting_initial_scanline_pal = 0;
   setting_last_scanline_pal = 287;

   check_system_specs();
}

void retro_reset(void)
{
   SS_Reset( true );
}

bool retro_load_game_special(unsigned, const struct retro_game_info *, size_t)
{
   return false;
}

static bool cdimagecache = false;

static bool boot = true;

// shared internal memory support
bool shared_intmemory = false;
bool shared_intmemory_toggle = false;


// shared backup memory support
bool shared_backup = false;
bool shared_backup_toggle = false;

static void check_variables(bool startup)
{
   struct retro_variable var = {0};

   if (startup)
   {
	   var.key      = "beetle_saturn_cdimagecache";
           cdimagecache = false;

	   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) 
			   && var.value)
		   if (!strcmp(var.value, "enabled"))
			   cdimagecache = true;

	   var.key = "beetle_saturn_shared_int";

	   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   {
		   if (!strcmp(var.value, "enabled"))
			   shared_intmemory_toggle = true;
		   else if (!strcmp(var.value, "disabled"))
			   shared_intmemory_toggle = false;

	   }

	   var.key = "beetle_saturn_shared_ext";

	   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	   {
		   if (!strcmp(var.value, "enabled"))
			   shared_backup_toggle = true;
		   else if (!strcmp(var.value, "disabled"))
			   shared_backup_toggle = false;

	   }
   }

   var.key = "beetle_saturn_region";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "Auto Detect") || !strcmp(var.value, "auto"))
         setting_region = 0;
      else if (!strcmp(var.value, "Japan") || !strcmp(var.value, "jp"))
         setting_region = SMPC_AREA_JP;
      else if (!strcmp(var.value, "North America") || !strcmp(var.value, "na"))
         setting_region = SMPC_AREA_NA;
      else if (!strcmp(var.value, "Europe") || !strcmp(var.value, "eu"))
         setting_region = SMPC_AREA_EU_PAL;
      else if (!strcmp(var.value, "South Korea") || !strcmp(var.value, "kr"))
         setting_region = SMPC_AREA_KR;
      else if (!strcmp(var.value, "Asia (NTSC)") || !strcmp(var.value, "tw"))
         setting_region = SMPC_AREA_ASIA_NTSC;
      else if (!strcmp(var.value, "Asia (PAL)") || !strcmp(var.value, "as"))
         setting_region = SMPC_AREA_ASIA_PAL;
      else if (!strcmp(var.value, "Brazil") || !strcmp(var.value, "br"))
         setting_region = SMPC_AREA_CSA_NTSC;
      else if (!strcmp(var.value, "Latin America") || !strcmp(var.value, "la"))
         setting_region = SMPC_AREA_CSA_PAL;
   }

   var.key = "beetle_saturn_cart";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "Auto Detect") || !strcmp(var.value, "auto"))
         setting_cart = CART__RESERVED;
      else if (!strcmp(var.value, "None") || !strcmp(var.value, "none"))
         setting_cart = CART_NONE;
      else if (!strcmp(var.value, "Backup Memory") || !strcmp(var.value, "backup"))
         setting_cart = CART_BACKUP_MEM;
      else if (!strcmp(var.value, "Extended RAM (1MB)") || !strcmp(var.value, "extram1"))
         setting_cart = CART_EXTRAM_1M;
      else if (!strcmp(var.value, "Extended RAM (4MB)") || !strcmp(var.value, "extram4"))
         setting_cart = CART_EXTRAM_4M;
      else if (!strcmp(var.value, "The King of Fighters '95") || !strcmp(var.value, "kof95"))
         setting_cart = CART_KOF95;
      else if (!strcmp(var.value, "Ultraman: Hikari no Kyojin Densetsu") || !strcmp(var.value, "ultraman"))
         setting_cart = CART_ULTRAMAN;
   }

   var.key = "beetle_saturn_multitap_port1";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool connected = false;
      if (!strcmp(var.value, "enabled"))
         connected = true;
      else if (!strcmp(var.value, "disabled"))
         connected = false;

      input_multitap( 1, connected );
   }

   var.key = "beetle_saturn_multitap_port2";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool connected = false;
      if (!strcmp(var.value, "enabled"))
         connected = true;
      else if (!strcmp(var.value, "disabled"))
         connected = false;

      input_multitap( 2, connected );
   }



   var.key = "beetle_saturn_opposite_directions";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         opposite_directions = true;
      else if (!strcmp(var.value, "disabled"))
         opposite_directions = false;
   }

   var.key = "beetle_saturn_midsync";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         setting_midsync = true;
      else if (!strcmp(var.value, "disabled"))
         setting_midsync = false;
   }

   var.key = "beetle_saturn_autortc";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         setting_smpc_autortc = 1;
      else if (!strcmp(var.value, "disabled"))
         setting_smpc_autortc = 0;
   }

   var.key = "beetle_saturn_autortc_lang";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
       if (!strcmp(var.value, "english"))
          setting_smpc_autortc_lang = 0;
       else if (!strcmp(var.value, "german"))
          setting_smpc_autortc_lang = 1;
       else if (!strcmp(var.value, "french"))
          setting_smpc_autortc_lang = 2;
       else if (!strcmp(var.value, "spanish"))
          setting_smpc_autortc_lang = 3;
       else if (!strcmp(var.value, "italian"))
          setting_smpc_autortc_lang = 4;
       else if (!strcmp(var.value, "japanese"))
          setting_smpc_autortc_lang = 5;
   }

   var.key = "beetle_saturn_horizontal_overscan";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      h_mask = atoi(var.value);
   }

   var.key = "beetle_saturn_initial_scanline";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      first_sl = atoi(var.value);
   }

   var.key = "beetle_saturn_last_scanline";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      last_sl = atoi(var.value);
   }

   var.key = "beetle_saturn_initial_scanline_pal";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      first_sl_pal = atoi(var.value);
   }

   var.key = "beetle_saturn_last_scanline_pal";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      last_sl_pal = atoi(var.value);
   }

   var.key = "beetle_saturn_horizontal_blend";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool newval = (!strcmp(var.value, "enabled"));
      DoHBlend = newval;
   }

   var.key = "beetle_saturn_analog_stick_deadzone";
   var.value = NULL;

   if ( environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value )
      input_set_deadzone_stick( atoi( var.value ) );

   var.key = "beetle_saturn_trigger_deadzone";
   var.value = NULL;

   if ( environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value )
      input_set_deadzone_trigger( atoi( var.value ) );

   var.key = "beetle_saturn_mouse_sensitivity";
   var.value = NULL;

   if ( environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value )
      input_set_mouse_sensitivity( atoi( var.value ) );

   var.key = "beetle_saturn_virtuagun_crosshair";
   var.value = NULL;

   if ( environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value )
   {
      if ( !strcmp(var.value, "Off") ) {
         setting_gun_crosshair = SETTING_GUN_CROSSHAIR_OFF;
      } else if ( !strcmp(var.value, "Cross") ) {
         setting_gun_crosshair = SETTING_GUN_CROSSHAIR_CROSS;
      } else if ( !strcmp(var.value, "Dot") ) {
         setting_gun_crosshair = SETTING_GUN_CROSSHAIR_DOT;
      }
   }

   var.key   = "beetle_saturn_virtuagun_input";
   var.value = NULL;

   if ( environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value )
   {
      if ( !strcmp(var.value, "Touchscreen") ) {
         setting_gun_input = SETTING_GUN_INPUT_POINTER;
      } else {
         setting_gun_input = SETTING_GUN_INPUT_LIGHTGUN;
      }
   }
}

static bool MDFNI_LoadGame( const char *name )
{
   unsigned horrible_hacks   = 0;
   // .. safe defaults
   unsigned region           = SMPC_AREA_NA;
   int cart_type             = CART_BACKUP_MEM;
   unsigned cpucache_emumode = CPUCACHE_EMUMODE_DATA;

   // always set this.
   MDFNGameInfo = &EmulatedSS;

   size_t name_len = strlen( name );

   // check for a valid file extension
   if ( name_len > 4 )
   {
      const char *ext = name + name_len - 4;

      // supported extension?
      if ((!strcasecmp( ext, ".ccd" )) ||
          (!strcasecmp( ext, ".chd" )) ||
          (!strcasecmp( ext, ".cue" )) ||
          (!strcasecmp( ext, ".toc" )) ||
          (!strcasecmp( ext, ".m3u" )) )
      {
         uint8 fd_id[16];
         char sgid[16 + 1]     = { 0 };
         char sgname[0x70 + 1] = { 0 };
         char sgarea[0x10 + 1] = { 0 };

         if ( disc_load_content( MDFNGameInfo, name, fd_id, sgid, sgname, sgarea, cdimagecache ) )
         {
            log_cb(RETRO_LOG_INFO, "Game ID is: %s\n", sgid );

            // test discs?
            bool discs_ok = true;
            if ( setting_disc_test )
               discs_ok = DiscSanityChecks();

            if ( discs_ok )
            {
               DetectRegion( &region );

               DB_Lookup(nullptr, sgid, sgname, sgarea, fd_id, &region, &cart_type, &cpucache_emumode );
               horrible_hacks = DB_LookupHH(sgid, fd_id);

               // forced region setting?
               if ( setting_region != 0 )
                  region = setting_region;

               // forced cartridge setting?
               if ( setting_cart != CART__RESERVED )
                  cart_type = setting_cart;

               // GO!
               if ( InitCommon( cpucache_emumode,
                    horrible_hacks, cart_type, region ) )
               {
                  MDFN_LoadGameCheats();
                  MDFNMP_InstallReadPatches();

                  return true;
               }

               // OK it's really bad. Probably don't 
               // have a BIOS if InitCommon
               // fails. We can't continue as an 
               // emulator and will show a blank
               // screen.

               disc_cleanup();

               return false;
            } // discs okay?

         } // load content

      } // supported extension?

   } // valid name?

   //
   // Drop to BIOS

   disc_cleanup();

   // forced region setting?
   if ( setting_region != 0 )
      region = setting_region;

   // forced cartridge setting?
   if ( setting_cart != CART__RESERVED )
      cart_type = setting_cart;

   // Initialise with safe parameters
   InitCommon( cpucache_emumode, horrible_hacks, cart_type, region );

   MDFN_LoadGameCheats();
   MDFNMP_InstallReadPatches();

   return true;
}

bool retro_load_game(const struct retro_game_info *info)
{
   char tocbasepath[4096];
   bool ret = false;

   if (!info || failed_init)
      return false;

   input_init_env( environ_cb );

   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      return false;

   extract_basename(retro_cd_base_name,       info->path, sizeof(retro_cd_base_name));
   extract_directory(retro_cd_base_directory, info->path, sizeof(retro_cd_base_directory));

   snprintf(tocbasepath, sizeof(tocbasepath), "%s" RETRO_SLASH "%s.toc", retro_cd_base_directory, retro_cd_base_name);

   if (!strstr(tocbasepath, "cdrom://") && filestream_exists(tocbasepath))
      snprintf(retro_cd_path, sizeof(retro_cd_path), "%s", tocbasepath);
   else
      snprintf(retro_cd_path, sizeof(retro_cd_path), "%s", info->path);

   check_variables(true);
   //make sure shared memory cards and save states are enabled only at startup
   shared_intmemory = shared_intmemory_toggle;
   shared_backup = shared_backup_toggle;

   // Let's try to load the game. If this fails then things are very wrong.
   if (MDFNI_LoadGame(retro_cd_path) == false)
      return false;

   MDFN_LoadGameCheats();
   MDFNMP_InstallReadPatches();

   alloc_surface();

#ifdef NEED_DEINTERLACER
   PrevInterlaced = false;
   deint.ClearState();
#endif

   input_init();

   boot = false;

   disc_select(disk_get_image_index());

   frame_count = 0;
   internal_frame_count = 0;

   struct retro_core_option_display option_display;
   option_display.visible = false;
   if (is_pal)
   {
	   option_display.key = "beetle_saturn_initial_scanline";
	   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
	   option_display.key = "beetle_saturn_last_scanline";
	   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   }
   else
   {
	   option_display.key = "beetle_saturn_initial_scanline_pal";
	   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
	   option_display.key = "beetle_saturn_last_scanline_pal";
	   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
   }

   return true;
}

void retro_unload_game(void)
{
   if(!MDFNGameInfo)
      return;

   MDFN_FlushGameCheats();

   CloseGame();

   MDFNMP_Kill();

   MDFNGameInfo = NULL;

   disc_cleanup();

   retro_cd_base_directory[0] = '\0';
   retro_cd_path[0]           = '\0';
   retro_cd_base_name[0]      = '\0';
}

static uint64_t video_frames, audio_frames;
#define SOUND_CHANNELS 2

void retro_run(void)
{
   bool updated = false;
   bool hires_h_mode;
   unsigned overscan_mask;
   unsigned linevisfirst, linevislast;
   static unsigned width, height;
   static unsigned game_width, game_height;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables(false);

   linevisfirst   =  is_pal ? first_sl_pal : first_sl;
   linevislast    =  is_pal ? last_sl_pal : last_sl;

   // Keep the counters at 0 so that they don't display a bogus
   // value if this option is enabled later on
   frame_count = 0;
   internal_frame_count = 0;

   input_poll_cb();

   if (libretro_supports_bitmasks)
	   input_update_with_bitmasks( input_state_cb);
   else
	   input_update( input_state_cb);

   static int32 rects[MEDNAFEN_CORE_GEOMETRY_MAX_H];
   rects[0] = ~0;

   EmulateSpecStruct spec;
   spec.surface = surf;
   spec.LineWidths = rects;
   spec.SoundBufSize = 0;

   EmulateSpecStruct *espec = (EmulateSpecStruct*)&spec;

   Emulate(espec);

#ifdef NEED_DEINTERLACER
   if (spec.InterlaceOn)
   {
      if (!PrevInterlaced)
         deint.ClearState();

      deint.Process(spec.surface, spec.DisplayRect, spec.LineWidths, spec.InterlaceField);

      PrevInterlaced = true;

      spec.InterlaceOn = false;
      spec.InterlaceField = 0;
   }
   else
      PrevInterlaced = false;

#endif
   const void *fb      = NULL;
   const uint32_t *pix = surf->pixels;
   size_t pitch        = FB_WIDTH * sizeof(uint32_t);

   hires_h_mode   =  (rects[0] == 704) ? true : false;
   overscan_mask  =  (h_mask >> 1) << hires_h_mode;
   width          =  rects[0] - (h_mask << hires_h_mode);
   height         =  (linevislast + 1 - linevisfirst) << PrevInterlaced;

   if (width != game_width || height != game_height)
   {
      struct retro_system_av_info av_info;

      // Change frontend resolution using  base width/height (+ overscan adjustments).
      // This avoids inconsistent frame scales when game switches between interlaced and non-interlaced modes.
      av_info.geometry.base_width   = 352 - h_mask;
      av_info.geometry.base_height  = linevislast + 1 - linevisfirst;
      av_info.geometry.max_width    = MEDNAFEN_CORE_GEOMETRY_MAX_W;
      av_info.geometry.max_height   = MEDNAFEN_CORE_GEOMETRY_MAX_H;
      av_info.geometry.aspect_ratio = MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO;
      environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);

      log_cb(RETRO_LOG_INFO, "Target framebuffer size : %dx%d\n", width, height);

      game_width  = width;
      game_height = height;

      input_set_geometry( width, height );
   }

   /* LED interface */
   if (led_state_cb)
      retro_led_interface();

   pix += surf->pitchinpix * (linevisfirst << PrevInterlaced) + overscan_mask;

   fb = pix;

   video_cb(fb, game_width, game_height, pitch);

   video_frames++;
   audio_frames += spec.SoundBufSize;

   int16_t *interbuf = (int16_t*)&IBuffer;

   audio_batch_cb(interbuf, spec.SoundBufSize);
}

void retro_get_system_info(struct retro_system_info *info)
{
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   memset(info, 0, sizeof(*info));
   info->library_name     = MEDNAFEN_CORE_NAME;
   info->library_version  = MEDNAFEN_CORE_VERSION GIT_VERSION;
   info->need_fullpath    = true;
   info->valid_extensions = MEDNAFEN_CORE_EXTENSIONS;
   info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.sample_rate    = 44100;
   info->geometry.base_width   = MEDNAFEN_CORE_GEOMETRY_BASE_W;
   info->geometry.base_height  = MEDNAFEN_CORE_GEOMETRY_BASE_H;
   info->geometry.max_width    = MEDNAFEN_CORE_GEOMETRY_MAX_W;
   info->geometry.max_height   = MEDNAFEN_CORE_GEOMETRY_MAX_H;
   info->geometry.aspect_ratio = MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO;

   if (retro_get_region() == RETRO_REGION_PAL)
      info->timing.fps            = 49.96;
   else
      info->timing.fps            = 59.88;
}

void retro_deinit(void)
{
   delete surf;
   surf = NULL;

   log_cb(RETRO_LOG_INFO, "[%s]: Samples / Frame: %.5f\n",
         MEDNAFEN_CORE_NAME, (double)audio_frames / video_frames);
   log_cb(RETRO_LOG_INFO, "[%s]: Estimated FPS: %.5f\n",
         MEDNAFEN_CORE_NAME, (double)video_frames * 44100 / audio_frames);
 
   libretro_supports_option_categories = false;
   libretro_supports_bitmasks          = false;
}

unsigned retro_get_region(void)
{
   if (is_pal)
       return RETRO_REGION_PAL;  //Ben Swith PAL
   return RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_environment( retro_environment_t cb )
{
   struct retro_vfs_interface_info vfs_iface_info;
   struct retro_led_interface led_interface;
   environ_cb = cb;

   libretro_supports_option_categories = false;
   libretro_set_core_options(environ_cb,
           &libretro_supports_option_categories);

   vfs_iface_info.required_interface_version = 2;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);

   if(environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &led_interface))
   if (led_interface.set_led_state && !led_state_cb)
      led_state_cb = led_interface.set_led_state;

   input_set_env(cb);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

static size_t serialize_size = 0;

size_t retro_serialize_size(void)
{
   // Don't know yet?
   if ( serialize_size == 0 )
   {
      // Do a fake save to see.
      StateMem st;

      st.data_frontend  = NULL;
      st.data           = NULL;
      st.loc            = 0;
      st.len            = 0;
      st.malloced       = 0;
      st.initial_malloc = 0;

      if ( MDFNSS_SaveSM( &st, MEDNAFEN_CORE_VERSION_NUMERIC, NULL, NULL, NULL ) )
      {
         // Cache and tidy up.
         serialize_size = st.len;
         if (st.data)
            free(st.data);
      }
   }

   // Return cached value.
   return serialize_size;
}

bool retro_serialize(void *data, size_t size)
{
   StateMem st;
   bool ret          = false;

   st.data_frontend  = (uint8_t*)data;
   st.data           = st.data_frontend;
   st.loc            = 0;
   st.len            = 0;
   st.malloced       = size;
   st.initial_malloc = 0;

   /* MDFNSS_SaveSM will malloc separate memory for st.data to complete
    * the save if the passed-in size is too small */
   ret               = MDFNSS_SaveSM(&st, MEDNAFEN_CORE_VERSION_NUMERIC, NULL, NULL, NULL);

   if (st.len != size)
   {
      log_cb(RETRO_LOG_WARN, "Warning: Save state size has changed\n");

      if (st.data != st.data_frontend)
      {
         free(st.data);
         serialize_size = st.len;
         ret            = false;
      }
   }

   return ret;
}

bool retro_unserialize(const void *data, size_t size)
{
   StateMem st;

   st.data_frontend  = (uint8_t*)data;
   st.data           = st.data_frontend;
   st.loc            = 0;
   st.len            = size;
   st.malloced       = 0;
   st.initial_malloc = 0;

   return MDFNSS_LoadSM(&st, MEDNAFEN_CORE_VERSION_NUMERIC);
}

void *retro_get_memory_data(unsigned type)
{
   switch ( type & RETRO_MEMORY_MASK )
   {

   case RETRO_MEMORY_SYSTEM_RAM:
      return WorkRAM;

   }

   // not supported
   return NULL;
}

size_t retro_get_memory_size(unsigned type)
{
   switch ( type & RETRO_MEMORY_MASK )
   {

   case RETRO_MEMORY_SYSTEM_RAM:
      return sizeof(WorkRAM);

   }

   // not supported
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned, bool, const char *)
{}

// Use a simpler approach to make sure that things go right for libretro.
const char *MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
   static char fullpath[4096];

   fullpath[0] = '\0';

   switch (type)
   {
      case MDFNMKF_SAV:
         snprintf(fullpath, sizeof(fullpath), "%s" RETRO_SLASH "%s.%s",
               retro_save_directory,
               (!shared_intmemory) ? retro_cd_base_name : "mednafen_saturn_libretro_shared",
               cd1);
         break;
      case MDFNMKF_CART:
         snprintf(fullpath, sizeof(fullpath), "%s" RETRO_SLASH "%s.%s",
               retro_save_directory,
               (!shared_backup) ? retro_cd_base_name : "mednafen_saturn_libretro_shared",
               cd1);
         break;
      case MDFNMKF_FIRMWARE:
         snprintf(fullpath, sizeof(fullpath), "%s" RETRO_SLASH "%s", retro_base_directory, cd1);
         break;
      default:
         break;
   }

   return fullpath;
}

void MDFN_MidSync(void)
{
    input_poll_cb();
    if (libretro_supports_bitmasks)
	    input_update_with_bitmasks( input_state_cb);
    else
	    input_update( input_state_cb);
}
