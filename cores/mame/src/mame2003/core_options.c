/*********************************************************************

  core_options.c

  core option configurations used by mame2003-plus

*********************************************************************/

#include "mame.h"
#include "driver.h"
#include <libretro.h>
#include <string/stdstring.h>

static struct retro_core_option_v2_definition  default_options[OPT_end + 1];    /* need the plus one for the NULL entries at the end */
static struct retro_core_option_v2_definition  effective_defaults[OPT_end + 1];

/* prevent saving corrupt cfgs when the save type does not match the type initialized */
bool save_protection;

/******************************************************************************

  private function prototypes

******************************************************************************/

static void   set_variables(void);
static void   determine_core_options_version(struct retro_core_options_v2 *core_options_us);


/******************************************************************************

  external function prototypes and variables

******************************************************************************/

/* found in mame2003.c */
extern void   retro_set_audio_buff_status_cb(void);
extern void   retro_describe_controls(void);

extern const struct GameDriver *game_driver;
extern retro_set_led_state_t led_state_cb;



/*
 * Note that core options are not presented in order they are defined here,
 * but rather by their order in the OPT_ enum found in mame2003.h
 */


static struct retro_core_option_v2_definition option_def_four_way_emulation = {
   APPNAME"_four_way_emulation",
   "4-Way Joystick Emulation on 8-Way Joysticks",
   NULL,
   "Improves issues with hitting diagonals when playing 4-way games.",
   NULL,
   "cat_key_input",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_xy_device = {
   APPNAME"_xy_device",
   "X-Y Device",
   NULL,
   "Selects a specific x-y coordinates input device to read.",
   NULL,
   "cat_key_input",
   {
      { "mouse",    NULL },
      { "pointer",  NULL },
      { "lightgun", NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
#if defined(__IOS__)
   "pointer"
#else
   "mouse"
#endif
};

static struct retro_core_option_v2_definition option_def_crosshair_enabled = {
   APPNAME"_crosshair_enabled",
   "Show Lightgun Crosshairs",
   NULL,
   "Displays a generic crosshair for each player.",
   NULL,
   "cat_key_input",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_crosshair_appearance = {
   APPNAME"_crosshair_appearance",
   "Lightgun Crosshair Appearance",
   NULL,
   "Changes the appearance of the crosshairs for each player.",
   NULL,
   "cat_key_input",
   {
      { "simple",    NULL },
      { "enhanced",  NULL },
      { NULL, NULL },
   },
   "simple"
};

static struct retro_core_option_v2_definition option_def_skip_disclaimer = {
   APPNAME"_skip_disclaimer",
   "Skip Disclaimer",
   NULL,
   "Bypasses a copyright warning from being displayed when loading content.",
   NULL,
   "cat_key_system",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_skip_warnings = {
   APPNAME"_skip_warnings",
   "Skip Warnings",
   NULL,
   "Bypasses a warning message from being displayed when loading content containing known issues.",
   NULL,
   "cat_key_system",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_display_setup = {
   APPNAME"_display_setup",
   "Display MAME Menu",
   NULL,
   "Toggles the visibility of the internal MAME menu.",
   NULL,
   "cat_key_system",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_brightness = {
   APPNAME"_brightness",
   "Brightness",
   NULL,
   "Modifies the brightness level being used.",
   NULL,
   "cat_key_video",
   {
      { "0.2", NULL },
      { "0.3", NULL },
      { "0.4", NULL },
      { "0.5", NULL },
      { "0.6", NULL },
      { "0.7", NULL },
      { "0.8", NULL },
      { "0.9", NULL },
      { "1.0", NULL },
      { "1.1", NULL },
      { "1.2", NULL },
      { "1.3", NULL },
      { "1.4", NULL },
      { "1.5", NULL },
      { "1.6", NULL },
      { "1.7", NULL },
      { "1.8", NULL },
      { "1.9", NULL },
      { "2.0", NULL },
      { NULL, NULL },
   },
   "1.0"
};

static struct retro_core_option_v2_definition option_def_gamma = {
   APPNAME"_gamma",
   "Gamma Correction",
   NULL,
   "Modifies the gamma level being used.",
   NULL,
   "cat_key_video",
   {
      { "0.5", NULL },
      { "0.6", NULL },
      { "0.7", NULL },
      { "0.8", NULL },
      { "0.9", NULL },
      { "1.0", NULL },
      { "1.1", NULL },
      { "1.2", NULL },
      { "1.3", NULL },
      { "1.4", NULL },
      { "1.5", NULL },
      { "1.6", NULL },
      { "1.7", NULL },
      { "1.8", NULL },
      { "1.9", NULL },
      { "2.0", NULL },
      { NULL, NULL },
   },
   "1.0"
};

static struct retro_core_option_v2_definition option_def_display_artwork = {
   APPNAME"_display_artwork",
   "Display Artwork",
   NULL,
   "Restart core required. Used to display custom artwork when available.",
   NULL,
   "cat_key_artwork",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_art_resolution = {
   APPNAME"_art_resolution",
   "Artwork Resolution Multiplier",
   "Resolution Multiplier",
   "Restart core required. Increases the artwork resolution by the selected multiplier value.",
   NULL,
   "cat_key_artwork",
   {
      { "1",  NULL },
      { "2",  NULL },
      { "3",  NULL },
      { "4",  NULL },
      { "5",  NULL },
      { "6",  NULL },
      { "7",  NULL },
      { "8",  NULL },
      { NULL, NULL },
   },
   "1"
};

static struct retro_core_option_v2_definition option_def_art_overlay_opacity = {
   APPNAME"_art_overlay_opacity",
   "Artwork Hardcoded Overlay Opacity",
   "Hardcoded Overlay Opacity",
   "Restart core required.",
   NULL,
   "cat_key_artwork",
   {
      { "default", NULL },
      { "0",       NULL },
      { "1",       NULL },
      { "2",       NULL },
      { "3",       NULL },
      { "4",       NULL },
      { "5",       NULL },
      { "6",       NULL },
      { "7",       NULL },
      { "8",       NULL },
      { "9",       NULL },
      { "10",      NULL },
      { "11",      NULL },
      { "12",      NULL },
      { "13",      NULL },
      { "14",      NULL },
      { "15",      NULL },
      { "16",      NULL },
      { "17",      NULL },
      { "18",      NULL },
      { "19",      NULL },
      { "20",      NULL },
      { "21",      NULL },
      { "22",      NULL },
      { "23",      NULL },
      { "24",      NULL },
      { "25",      NULL },
      { "50",      NULL },
      { "70",      NULL },
      { NULL, NULL },
   },
   "default"
};

static struct retro_core_option_v2_definition option_def_neogeo_bios = {
   APPNAME"_neogeo_bios",
   "Specify Neo Geo BIOS",
   NULL,
   "Restart core required. Select alternative bios files.",
   NULL,
   NULL,
   {
      { "default",   NULL },
      { "euro",      NULL },
      { "euro-s1",   NULL },
      { "us",        NULL },
      { "us-e",      NULL },
      { "asia",      NULL },
      { "japan",     NULL },
      { "japan-s2",  NULL },
      { "unibios40", NULL },
      { "unibios33", NULL },
      { "unibios20", NULL },
      { "unibios13", NULL },
      { "unibios11", NULL },
      { "unibios10", NULL },
      { "debug",     NULL },
      { "asia-aes",  NULL },
      { NULL, NULL },
   },
   "default"
};

static struct retro_core_option_v2_definition option_def_stv_bios = {
   APPNAME"_stv_bios",
   "Specify Sega ST-V BIOS",
   NULL,
   "Restart core required. Select alternative bios files.",
   NULL,
   NULL,
   {
      { "default", NULL },
      { "japan",   NULL },
      { "japana",  NULL },
      { "us",      NULL },
      { "japan_b", NULL },
      { "taiwan",  NULL },
      { "europe",  NULL },
      { NULL, NULL },
   },
   "default"
};

static struct retro_core_option_v2_definition option_def_use_samples = {
   APPNAME"_use_samples",
   "Use Samples",
   NULL,
   "Restart core required. Allow audio sample files to be loaded when provided in the samples directory.",
   NULL,
   "cat_key_audio",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_use_alt_sound = {
   APPNAME"_use_alt_sound",
   "Use CD Soundtrack",
   NULL,
   "Restart core required. Replace original hardware sounds with optional audio sample files when provided in the samples directory.",
   NULL,
   "cat_key_audio",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_dialsharexy = {
   APPNAME"_dialsharexy",
   "Share 2 Player Dial Controls Across One X/Y Device",
   NULL,
   NULL,
   NULL,
   "cat_key_input",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_dial_swap_xy = {
   APPNAME"_dial_swap_xy",
   "Swap X and Y Dial Axis",
   NULL,
   "When enabled, the X axis will be returned for the Y axis and vice versa. This is useful when the device is improperly wired or the device does not support this feature in software.",
   NULL,
   "cat_key_input",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_tate_mode = {
   APPNAME"_tate_mode",
   "TATE Mode",
   NULL,
   "When enabled, the display will be rotated to the orientation used by actual hardware.",
   NULL,
   "cat_key_video",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_vector_resolution = {
   APPNAME"_vector_resolution",
   "Vector Resolution",
   "Resolution",
   "Restart core required.",
   NULL,
   "cat_key_vector",
   {
      { "640x480",   NULL },
      { "1024x768",  NULL },
      { "1280x960",  NULL },
      { "1440x1080", NULL },
      { "1600x1200", NULL },
      { "1707x1280", NULL },
      { "original",  NULL },
      { NULL, NULL },
   },
   "1024x768"
};

static struct retro_core_option_v2_definition option_def_vector_antialias = {
   APPNAME"_vector_antialias",
   "Vector Antialiasing",
   "Antialiasing",
   NULL,
   NULL,
   "cat_key_vector",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_vector_beam_width = {
   APPNAME"_vector_beam_width",
   "Vector Beam Width",
   "Beam Width",
   "Only used with antialiasing.",
   NULL,
   "cat_key_vector",
   {
      { "1",   NULL },
      { "1.2", NULL },
      { "1.4", NULL },
      { "1.6", NULL },
      { "1.8", NULL },
      { "2",   NULL },
      { "2.5", NULL },
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
      { NULL,  NULL },
   },
   "2"
};

static struct retro_core_option_v2_definition option_def_vector_translucency = {
   APPNAME"_vector_translucency",
   "Vector Translucency",
   "Translucency",
   NULL,
   NULL,
   "cat_key_vector",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_vector_flicker = {
   APPNAME"_vector_flicker",
   "Vector Flicker",
   "Flicker",
   NULL,
   NULL,
   "cat_key_vector",
   {
      { "0",   NULL },
      { "10",  NULL },
      { "20",  NULL },
      { "30",  NULL },
      { "40",  NULL },
      { "50",  NULL },
      { "60",  NULL },
      { "70",  NULL },
      { "80",  NULL },
      { "90",  NULL },
      { "100", NULL },
      { NULL, NULL },
   },
   "20"
};

static struct retro_core_option_v2_definition option_def_vector_intensity = {
   APPNAME"_vector_intensity",
   "Vector Intensity",
   "Intensity",
   NULL,
   NULL,
   "cat_key_vector",
   {
      { "0.5", NULL },
      { "1",   NULL },
      { "1.5", NULL },
      { "2",   NULL },
      { "2.5", NULL },
      { "3",   NULL },
      { NULL, NULL },
   },
   "1.5"
};

static struct retro_core_option_v2_definition option_def_nvram_bootstraps = {
   APPNAME"_nvram_bootstraps",
   "NVRAM Bootstraps",
   NULL,
   "Used to automatically initialize games that otherwise require special startup procedures or configurations.",
   NULL,
   "cat_key_system",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_sample_rate = {
   APPNAME"_sample_rate",
   "Sample Rate",
   NULL,
   "Number of audio samples taken per second. Higher rates provide better quality audio.",
   NULL,
   "cat_key_audio",
   {
      { "8000",   "8000 Hz" },
      { "11025", "11025 Hz" },
      { "22050", "22050 Hz" },
      { "30000", "30000 Hz" },
      { "44100", "44100 Hz" },
      { "48000", "48000 Hz" },
      { NULL, NULL },
   },
   "48000"
};

static struct retro_core_option_v2_definition option_def_input_interface = {
   APPNAME"_input_interface",
   "Input Interface",
   NULL,
   "Configures which input types are being read.",
   NULL,
   "cat_key_input",
   {
      { "simultaneous", NULL },
      { "retropad",     NULL },
      { "keyboard",     NULL },
      { NULL, NULL },
   },
   "simultaneous"
};

static struct retro_core_option_v2_definition option_def_mame_remapping = {
   APPNAME"_mame_remapping",
   "Legacy Remapping",
   NULL,
   "Restart core required. Enables MAME menu input remapping.",
   NULL,
   "cat_key_system",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_frameskip = {
   APPNAME"_frameskip",
   "Frameskip",
   NULL,
   "Skips a number of frames from being displayed. Can be used to squeeze performance out of lower spec platforms.",
   NULL,
   "cat_key_video",
   {
      { "disabled",        NULL },
      { "1",               NULL },
      { "2",               NULL },
      { "3",               NULL },
      { "4",               NULL },
      { "5",               NULL },
      { "6",               NULL },
      { "7",               NULL },
      { "8",               NULL },
      { "9",               NULL },
      { "10",              NULL },
      { "11",              NULL },
      { "auto",            NULL },
      { "auto_aggressive", "auto aggressive" },
      { "auto_max",        "auto max"        },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_core_sys_subfolder = {
   APPNAME"_core_sys_subfolder",
   "Locate System Files Within a Subfolder",
   NULL,
   NULL,
   NULL,
   "cat_key_system",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_core_save_subfolder = {
   APPNAME"_core_save_subfolder",
   "Locate Save Files Within a Subfolder",
   NULL,
   NULL,
   NULL,
   "cat_key_system",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_autosave_hiscore = {
   APPNAME"_autosave_hiscore",
   "Autosave Hiscore",
   NULL,
   "Recommended to use default which will save the hiscore when closing content. Recursively will save repeatedly the entire time during gameplay. Disabled will bypass the use of the hiscore.dat file entirely.",
   NULL,
   "cat_key_system",
   {
      { "default",     NULL },
      { "recursively", NULL },
      { "disabled",    NULL },
      { NULL, NULL },
   },
   "default"
};

static struct retro_core_option_v2_definition option_def_cheat_input_ports = {
   APPNAME"_cheat_input_ports",
   "Dip Switch/Cheat Input Ports",
   NULL,
   "Used to display fake dip switches and input control options within the MAME menu when available.",
   NULL,
   "cat_key_input",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_digital_joy_centering = {
   APPNAME"_digital_joy_centering",
   "Center Joystick Axis for Digital Controls",
   NULL,
   "Emulates the center position of an analog joystick when using digital controls. Automatically returns the center position when no direction is being applied.",
   NULL,
   "cat_key_input",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_cpu_clock_scale = {
   APPNAME"_cpu_clock_scale",
   "CPU Clock Scale",
   NULL,
   "Used to under or over clock the emulated CPU by a specified percentage.",
   NULL,
   NULL,
   {
      { "default", NULL },
      { "25",     "25%" },
      { "30",     "30%" },
      { "35",     "35%" },
      { "40",     "40%" },
      { "45",     "45%" },
      { "50",     "50%" },
      { "55",     "55%" },
      { "60",     "60%" },
      { "65",     "65%" },
      { "70",     "70%" },
      { "75",     "75%" },
      { "80",     "80%" },
      { "85",     "85%" },
      { "90",     "90%" },
      { "95",     "95%" },
      { "105",   "105%" },
      { "110",   "110%" },
      { "115",   "115%" },
      { "120",   "120%" },
      { "125",   "125%" },
      { "200",   "200%" },
      { "250",   "250%" },
      { "300",   "300%" },
      { NULL, NULL },
   },
   "default"
};

static struct retro_core_option_v2_definition option_def_cyclone_mode = {
   APPNAME"_cyclone_mode",
   "Cyclone Mode",
   NULL,
   "Restart core required. Forces the selected cyclone mode to be used.",
   NULL,
   NULL,
   {
      { "default",            NULL },
      { "disabled",           NULL },
      { "Cyclone",            NULL },
      { "DrZ80",              NULL },
      { "Cyclone+DrZ80",      NULL },
      { "DrZ80(snd)",         NULL },
      { "Cyclone+DrZ80(snd)", NULL },
      { NULL, NULL },
   },
   "default"
};

static struct retro_core_option_v2_definition option_def_override_ad_stick = {
   APPNAME"_override_ad_stick",
   "Use Lightgun as an Analog Stick",
   NULL,
   "Restart core required. Allows the input from a lightgun to override games which use analog sticks.",
   NULL,
   "cat_key_input",
   {
      { "disabled", NULL },
      { "enabled",  NULL },
      { NULL, NULL },
   },
   "disabled"
};

static struct retro_core_option_v2_definition option_def_input_toggle = {
   APPNAME"_input_toggle",
   "Allow Input Button to Act as a Toggle Switch",
   NULL,
   "Disable to use actual hardware such as a fixed position high/low shifter.",
   NULL,
   "cat_key_input",
   {
      { "enabled",  NULL },
      { "disabled", NULL },
      { NULL, NULL },
   },
   "enabled"
};

static struct retro_core_option_v2_definition option_def_null = {
   NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL
};


struct retro_core_option_v2_category option_cats_us[] = {
   {
      "cat_key_system",
      "System",
      "Configure system options."
   },
   {
      "cat_key_input",
      "Input",
      "Configure input options."
   },
   {
      "cat_key_audio",
      "Audio",
      "Configure audio options."
   },
   {
      "cat_key_video",
      "Video",
      "Configure video options."
   },
   {
      "cat_key_artwork",
      "Artwork",
      "Configure artwork options."
   },
   {
      "cat_key_vector",
      "Vector",
      "Configure vector options."
   },
   { NULL, NULL, NULL },
};


struct retro_core_options_v2 options_us = {
   option_cats_us,
   effective_defaults
};


void init_core_options(void)
{
  default_options[OPT_4WAY]                      = option_def_four_way_emulation;
  default_options[OPT_XY_DEVICE]                 = option_def_xy_device;
  default_options[OPT_CROSSHAIR_ENABLED]         = option_def_crosshair_enabled;
  default_options[OPT_CROSSHAIR_APPEARANCE]      = option_def_crosshair_appearance;
  default_options[OPT_SKIP_DISCLAIMER]           = option_def_skip_disclaimer;
  default_options[OPT_SKIP_WARNINGS]             = option_def_skip_warnings;
  default_options[OPT_DISPLAY_SETUP]             = option_def_display_setup;
  default_options[OPT_BRIGHTNESS]                = option_def_brightness;
  default_options[OPT_GAMMA]                     = option_def_gamma;
  default_options[OPT_ARTWORK]                   = option_def_display_artwork;
  default_options[OPT_ART_RESOLUTION]            = option_def_art_resolution;
  default_options[OPT_ART_OVERLAY_OPACITY]       = option_def_art_overlay_opacity;
  default_options[OPT_NEOGEO_BIOS]               = option_def_neogeo_bios;
  default_options[OPT_STV_BIOS]                  = option_def_stv_bios;
  default_options[OPT_USE_SAMPLES]               = option_def_use_samples;
  default_options[OPT_USE_ALT_SOUND]             = option_def_use_alt_sound;
  default_options[OPT_SHARE_DIAL]                = option_def_dialsharexy;
  default_options[OPT_DIAL_SWAP_XY]              = option_def_dial_swap_xy;
  default_options[OPT_TATE_MODE]                 = option_def_tate_mode;
  default_options[OPT_VECTOR_RESOLUTION]         = option_def_vector_resolution;
  default_options[OPT_VECTOR_ANTIALIAS]          = option_def_vector_antialias;
  default_options[OPT_VECTOR_BEAM]               = option_def_vector_beam_width;
  default_options[OPT_VECTOR_TRANSLUCENCY]       = option_def_vector_translucency;
  default_options[OPT_VECTOR_FLICKER]            = option_def_vector_flicker;
  default_options[OPT_VECTOR_INTENSITY]          = option_def_vector_intensity;
  default_options[OPT_NVRAM_BOOTSTRAP]           = option_def_nvram_bootstraps;
  default_options[OPT_SAMPLE_RATE]               = option_def_sample_rate;
  default_options[OPT_INPUT_INTERFACE]           = option_def_input_interface;
  default_options[OPT_MAME_REMAPPING]            = option_def_mame_remapping;
  default_options[OPT_FRAMESKIP]                 = option_def_frameskip;
  default_options[OPT_CORE_SYS_SUBFOLDER]        = option_def_core_sys_subfolder;
  default_options[OPT_CORE_SAVE_SUBFOLDER]       = option_def_core_save_subfolder;
  default_options[OPT_AUTOSAVE_HISCORE]          = option_def_autosave_hiscore;
  default_options[OPT_CHEAT_INPUT_PORTS]         = option_def_cheat_input_ports;
  default_options[OPT_DIGITAL_JOY_CENTERING]     = option_def_digital_joy_centering;
  default_options[OPT_CPU_CLOCK_SCALE]           = option_def_cpu_clock_scale;
  default_options[OPT_OVERRIDE_AD_STICK]         = option_def_override_ad_stick;
  default_options[OPT_INPUT_TOGGLE]              = option_def_input_toggle;
#if (HAS_CYCLONE || HAS_DRZ80)
  default_options[OPT_CYCLONE_MODE]              = option_def_cyclone_mode;
#endif
  default_options[OPT_end]                       = option_def_null;
  set_variables();
}


static void set_variables(void)
{
  static unsigned effective_options_count;         /* the number of core options in effect for the current content */
  int option_index   = 0;

  for(option_index = 0; option_index < (OPT_end + 1); option_index++)
  {
    switch(option_index)
    {
      case OPT_4WAY:
         if(options.content_flags[CONTENT_JOYSTICK_DIRECTIONS] != 4)
           continue;
         break;
      case OPT_CROSSHAIR_ENABLED:
      case OPT_CROSSHAIR_APPEARANCE:
         if(!options.content_flags[CONTENT_LIGHTGUN])
           continue;
         break;
      case OPT_STV_BIOS:
         if(!options.content_flags[CONTENT_STV])
           continue; /* only offer BIOS selection when it is relevant */
         break;
      case OPT_NEOGEO_BIOS:
          if(!options.content_flags[CONTENT_NEOGEO])
            continue; /* only offer BIOS selection when it is relevant */
          break;
      case OPT_USE_ALT_SOUND:
         if(!options.content_flags[CONTENT_ALT_SOUND])
           continue;
         break;
      case OPT_SHARE_DIAL:
      case OPT_DIAL_SWAP_XY:
         if(options.content_flags[CONTENT_DIAL] || options.content_flags[CONTENT_PADDLE])
           break;
         else
           continue;
         break;
      case OPT_VECTOR_RESOLUTION:
      case OPT_VECTOR_ANTIALIAS:
      case OPT_VECTOR_TRANSLUCENCY:
      case OPT_VECTOR_BEAM:
      case OPT_VECTOR_FLICKER:
      case OPT_VECTOR_INTENSITY:
         if(!options.content_flags[CONTENT_VECTOR])
           continue;
         break;
      case OPT_NVRAM_BOOTSTRAP:
         if(!options.content_flags[CONTENT_NVRAM_BOOTSTRAP])
           continue;
         break;
      case OPT_CHEAT_INPUT_PORTS:
         if(!options.content_flags[CONTENT_CHEAT_INPUT_PORT])
           continue;
         break;
      case OPT_OVERRIDE_AD_STICK:
         if(!options.content_flags[CONTENT_AD_STICK])
           continue;
         break;
    }

    effective_defaults[effective_options_count] = default_options[option_index];
    effective_options_count++;
  }

  determine_core_options_version( &options_us );
}


void update_variables(bool first_time)
{
  struct retro_led_interface ledintf;
  struct retro_variable var;
  int index;
  bool reset_control_descriptions = false;

  for(index = 0; index < OPT_end; index++)
  {
    var.value = NULL;
    var.key = default_options[index].key;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && !string_is_empty(var.value)) /* the frontend sends a value for this core option */
    {
      switch(index)
      {
        case OPT_INPUT_INTERFACE:
          if(strcmp(var.value, "retropad") == 0)
            options.input_interface = RETRO_DEVICE_JOYPAD;
          else if(strcmp(var.value, "keyboard") == 0)
            options.input_interface = RETRO_DEVICE_KEYBOARD;
          else
            options.input_interface = RETRO_DEVICE_KEYBOARD + RETRO_DEVICE_JOYPAD;
          break;

        case OPT_4WAY:
          if( (strcmp(var.value, "enabled") == 0) && (options.content_flags[CONTENT_JOYSTICK_DIRECTIONS] == 4) )
          {
            if(!options.restrict_4_way)           /* the option has just been toggled to "enabled" */
            {
              options.restrict_4_way = true;
              reset_control_descriptions = true;  /* games with rotated joysticks send different control descriptions in 4-way restrictor mode */
            }
          }
          else
          {
            if(options.restrict_4_way)            /* the option has just been toggled to "disabled" */
            {
              options.restrict_4_way = false;
              reset_control_descriptions = true;  /* games with rotated joysticks send different control descriptions in 4-way restrictor mode */
            }
          }
          break;

        case OPT_XY_DEVICE:
          if(strcmp(var.value, "pointer") == 0)
            options.xy_device = RETRO_DEVICE_POINTER;
          else if(strcmp(var.value, "mouse") == 0)
            options.xy_device = RETRO_DEVICE_MOUSE;
          else if(strcmp(var.value, "lightgun") == 0)
            options.xy_device = RETRO_DEVICE_LIGHTGUN;
          else
            options.xy_device = RETRO_DEVICE_NONE;
          break;

        case OPT_CROSSHAIR_ENABLED:
          if(strcmp(var.value, "enabled") == 0)
            options.crosshair_enable = 1;
          else
            options.crosshair_enable = 0;
          break;

        case OPT_CROSSHAIR_APPEARANCE:
          if(strcmp(var.value, "enhanced") == 0)
            options.crosshair_appearance = 1;
          else
            options.crosshair_appearance = 0;
          break;

        case OPT_SKIP_DISCLAIMER:
          if(strcmp(var.value, "enabled") == 0)
            options.skip_disclaimer = true;
          else
            options.skip_disclaimer = false;
          break;

        case OPT_SKIP_WARNINGS:
          if(strcmp(var.value, "enabled") == 0)
            options.skip_warnings = true;
          else
            options.skip_warnings = false;
          break;

        case OPT_DISPLAY_SETUP:
          if(strcmp(var.value, "enabled") == 0)
            options.display_setup = 1;
          else
            options.display_setup = 0;
          break;

        case OPT_BRIGHTNESS:
          options.brightness = atof(var.value);
          if(!first_time)
            palette_set_global_brightness(options.brightness);
          break;

        case OPT_GAMMA:
          options.gamma = atof(var.value);
          if(!first_time)
            palette_set_global_gamma(options.gamma);
          break;

        case OPT_ARTWORK:
          if(strcmp(var.value, "enabled") == 0)
            options.use_artwork = ARTWORK_USE_ALL;
          else
            options.use_artwork = ARTWORK_USE_NONE;
          break;

        case OPT_ART_RESOLUTION:
          options.artwork_res = atoi(var.value);
          break;

        case OPT_ART_OVERLAY_OPACITY:
          if(strcmp(var.value, "default") == 0)
            options.overlay_opacity = ARTWORK_OVERLAY_DEFAULT;
          else {
            options.overlay_opacity = atoi(var.value);
            if (options.overlay_opacity < 0 )
              options.overlay_opacity = 0;
            else if (options.overlay_opacity > 255)
              options.overlay_opacity = 255;
          }
          break;

        case OPT_STV_BIOS:
          if(!options.content_flags[CONTENT_STV])
            break;
          if(options.content_flags[CONTENT_DIEHARD]) /* catch required bios for this one game. */
            options.bios = "us";
          else
            options.bios = (strcmp(var.value, "default") == 0) ? NULL : var.value;
          break;

        case OPT_NEOGEO_BIOS:
          if(!options.content_flags[CONTENT_NEOGEO])
            break;
          options.bios = (strcmp(var.value, "default") == 0) ? NULL : var.value;
          break;

        case OPT_USE_SAMPLES:
          if(strcmp(var.value, "enabled") == 0)
            options.use_samples = true;
          else
            options.use_samples = false;
          break;

        case OPT_USE_ALT_SOUND:
          if(strcmp(var.value, "enabled") == 0)
            options.use_alt_sound = true;
          else
            options.use_alt_sound = false;
          break;

        case OPT_SHARE_DIAL:
          if(strcmp(var.value, "enabled") == 0)
            options.dial_share_xy = 1;
          else
            options.dial_share_xy = 0;
          break;

        case OPT_DIAL_SWAP_XY:
          if(strcmp(var.value, "enabled") == 0)
            options.dial_swap_xy = true;
          else
            options.dial_swap_xy = false;
          break;

        case OPT_TATE_MODE:
          if(strcmp(var.value, "enabled") == 0)
            options.tate_mode = 1;
          else
            options.tate_mode = 0;
          break;

        case OPT_DIGITAL_JOY_CENTERING:
          if(strcmp(var.value, "enabled") == 0)
            options.digital_joy_centering = 1;
          else
            options.digital_joy_centering = 0;
          break;

        case OPT_VECTOR_RESOLUTION:
          {
            int width = 0;
            int height = 0;
            sscanf(var.value, "%dx%d", &width, &height);
            /* if they are still 0, mame will set from driver resolution set */
            options.vector_width = width;
            options.vector_height = height;
          }
          break;

        case OPT_VECTOR_ANTIALIAS:
          if(strcmp(var.value, "enabled") == 0)
            options.antialias = 1; /* integer: 1 to enable antialiasing on vectors _ does not work as of 2018/04/17*/
          else
            options.antialias = 0;
          break;

        case OPT_VECTOR_BEAM:
          options.beam = atof(var.value); /* float: vector beam width */
          break;

        case OPT_VECTOR_TRANSLUCENCY:
          if(strcmp(var.value, "enabled") == 0)
            options.translucency = 1; /* integer: 1 to enable translucency on vectors */
          else
            options.translucency = 0;
          break;

        case OPT_VECTOR_FLICKER:
          options.vector_flicker = atof(var.value);
          break;

        case OPT_VECTOR_INTENSITY:
          options.vector_intensity_correction = atof(var.value); /* float: vector beam intensity */
          break;

        case OPT_NVRAM_BOOTSTRAP:
          if(strcmp(var.value, "enabled") == 0)
            options.nvram_bootstrap = true;
          else
            options.nvram_bootstrap = false;
          break;

        case OPT_SAMPLE_RATE:
          options.samplerate = atoi(var.value);
          break;

        case OPT_MAME_REMAPPING:
          if(strcmp(var.value, "enabled") == 0)
            options.mame_remapping = true;
          else
            options.mame_remapping = false;

          if(first_time)
            save_protection = options.mame_remapping;
          else
            setup_menu_init();
          break;

        case OPT_FRAMESKIP:
          if (strcmp(var.value, "auto") == 0)
            options.frameskip = 12;
          else if (strcmp(var.value, "auto_aggressive") == 0)
            options.frameskip = 13;
          else if(strcmp(var.value, "auto_max") == 0)
            options.frameskip = 14;
          else
            options.frameskip = atoi(var.value);

          retro_set_audio_buff_status_cb();
          break;

        case OPT_CPU_CLOCK_SCALE:
          if(strcmp(var.value, "default") == 0)
            options.cpu_clock_scale = 1;
          else
            options.cpu_clock_scale = (double) atoi(var.value) / 100;
          break;

        case OPT_CORE_SYS_SUBFOLDER:
          if(strcmp(var.value, "enabled") == 0)
            options.system_subfolder = true;
          else
            options.system_subfolder = false;
          break;

        case OPT_CORE_SAVE_SUBFOLDER:
          if(strcmp(var.value, "enabled") == 0)
            options.save_subfolder = true;
          else
            options.save_subfolder = false;
          break;

        case OPT_AUTOSAVE_HISCORE:
          if(strcmp(var.value, "default") == 0)
            options.autosave_hiscore = 1;
          else if(strcmp(var.value, "recursively") == 0)
            options.autosave_hiscore = 2;
          else
            options.autosave_hiscore = 0;
          break;

        case OPT_CHEAT_INPUT_PORTS:
          if(strcmp(var.value, "enabled") == 0)
            options.cheat_input_ports = true;
          else
            options.cheat_input_ports = false;
          break;

        case OPT_OVERRIDE_AD_STICK:
          if(strcmp(var.value, "enabled") == 0)
            options.override_ad_stick = 1;
          else
            options.override_ad_stick = 0;
          break;

        case OPT_INPUT_TOGGLE:
          if(strcmp(var.value, "enabled") == 0)
            options.input_toggle = true;
          else
            options.input_toggle = false;
          break;

#if (HAS_CYCLONE || HAS_DRZ80)
        case OPT_CYCLONE_MODE:
          if(strcmp(var.value, "default") == 0)
            options.cyclone_mode = 1;
          else if(strcmp(var.value, "Cyclone") == 0)
            options.cyclone_mode = 2;
          else if(strcmp(var.value, "DrZ80") == 0)
            options.cyclone_mode = 3;
          else if(strcmp(var.value, "Cyclone+DrZ80") == 0)
            options.cyclone_mode = 4;
          else if(strcmp(var.value, "DrZ80(snd)") == 0)
            options.cyclone_mode = 5;
          else if(strcmp(var.value, "Cyclone+DrZ80(snd)") == 0)
            options.cyclone_mode = 6;
          else /* disabled */
            options.cyclone_mode = 0;
          break;
#endif
      }
    }
  }

  ledintf.set_led_state = NULL;
  environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &ledintf);
  led_state_cb = ledintf.set_led_state;

  if(reset_control_descriptions) /* one of the option changes has flagged a need to re-describe the controls */
  {
    retro_describe_controls();
    reset_control_descriptions = false;
  }
}


void set_content_flags(void)
{
  int i = 0;
  const struct InputPortTiny *input = game_driver->input_ports;


  /************ DRIVERS WITH ALTERNATE SOUNDTRACKS ************/
  for( i = 0; Machine->drv->sound[i].sound_type && i < MAX_SOUND; i++ )
  {
    if (Machine->drv->sound[i].tag)
      if (strcmp("OST Samples",  Machine->drv->sound[i].tag) == 0)
      {
        options.content_flags[CONTENT_ALT_SOUND] = true;
      }
  }

  /************ DRIVERS WITH MULTIPLE BIOS OPTIONS ************/
#ifndef SPLIT_CORE
 {
  extern struct GameDriver driver_neogeo;
  extern struct GameDriver driver_stvbios;

  if (game_driver->clone_of == &driver_neogeo
   ||(game_driver->clone_of && game_driver->clone_of->clone_of == &driver_neogeo))
  {
    options.content_flags[CONTENT_NEOGEO] = true;
  }
  else if (game_driver->clone_of == &driver_stvbios
   ||(game_driver->clone_of && game_driver->clone_of->clone_of == &driver_stvbios))
  {
    options.content_flags[CONTENT_STV] = true;
  }
}
#endif

  /************ DIE HARD: ARCADE ************/
  if(strcasecmp(game_driver->name, "diehard") == 0)
    options.content_flags[CONTENT_DIEHARD] = true;

  /************ DRIVERS WITH VECTOR VIDEO DISPLAYS ************/
  if(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
    options.content_flags[CONTENT_VECTOR] = true;

  /************ INPUT-BASED CONTENT FLAGS ************/
  options.content_flags[CONTENT_JOYSTICK_DIRECTIONS] = 8; /* default behavior is 8-way joystick, even for 2-way games */

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		/* skip analog extension fields */
		if ((input->type & ~IPF_MASK) != IPT_EXTENSION)
		{
			switch (input->type & IPF_PLAYERMASK)
			{
				case IPF_PLAYER1:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 1) options.content_flags[CONTENT_PLAYER_COUNT] = 1;
					break;
				case IPF_PLAYER2:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 2) options.content_flags[CONTENT_PLAYER_COUNT] = 2;
					break;
				case IPF_PLAYER3:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 3) options.content_flags[CONTENT_PLAYER_COUNT] = 3;
					break;
				case IPF_PLAYER4:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 4) options.content_flags[CONTENT_PLAYER_COUNT] = 4;
					break;
				case IPF_PLAYER5:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 5) options.content_flags[CONTENT_PLAYER_COUNT] = 5;
					break;
				case IPF_PLAYER6:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 6) options.content_flags[CONTENT_PLAYER_COUNT] = 6;
					break;
				case IPF_PLAYER7:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 7) options.content_flags[CONTENT_PLAYER_COUNT] = 7;
					break;
				case IPF_PLAYER8:
					if (options.content_flags[CONTENT_PLAYER_COUNT] < 8) options.content_flags[CONTENT_PLAYER_COUNT] = 8;
					break;
			}

			if (input->type & IPF_4WAY) /* the controls use a 4-way joystick */
				options.content_flags[CONTENT_JOYSTICK_DIRECTIONS] = 4;

			if (input->type & IPF_CHEAT) /* uses a cheat input port */
				options.content_flags[CONTENT_CHEAT_INPUT_PORT] = true;

			switch (input->type & ~IPF_MASK)
			{
				case IPT_JOYSTICKRIGHT_UP:
				case IPT_JOYSTICKRIGHT_DOWN:
				case IPT_JOYSTICKRIGHT_LEFT:
				case IPT_JOYSTICKRIGHT_RIGHT:
				case IPT_JOYSTICKLEFT_UP:
				case IPT_JOYSTICKLEFT_DOWN:
				case IPT_JOYSTICKLEFT_LEFT:
				case IPT_JOYSTICKLEFT_RIGHT:
					options.content_flags[CONTENT_DUAL_JOYSTICK] = true;
					break;
				case IPT_BUTTON1:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 1) options.content_flags[CONTENT_BUTTON_COUNT] = 1;
					break;
				case IPT_BUTTON2:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 2) options.content_flags[CONTENT_BUTTON_COUNT] = 2;
					break;
				case IPT_BUTTON3:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 3) options.content_flags[CONTENT_BUTTON_COUNT] = 3;
					break;
				case IPT_BUTTON4:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 4) options.content_flags[CONTENT_BUTTON_COUNT] = 4;
					break;
				case IPT_BUTTON5:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 5) options.content_flags[CONTENT_BUTTON_COUNT] = 5;
					break;
				case IPT_BUTTON6:
					if (options.content_flags[CONTENT_BUTTON_COUNT] <6 ) options.content_flags[CONTENT_BUTTON_COUNT] = 6;
					break;
				case IPT_BUTTON7:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 7) options.content_flags[CONTENT_BUTTON_COUNT] = 7;
					break;
				case IPT_BUTTON8:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 8) options.content_flags[CONTENT_BUTTON_COUNT] = 8;
					break;
				case IPT_BUTTON9:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 9) options.content_flags[CONTENT_BUTTON_COUNT] = 9;
					break;
				case IPT_BUTTON10:
					if (options.content_flags[CONTENT_BUTTON_COUNT] < 10) options.content_flags[CONTENT_BUTTON_COUNT] = 10;
					break;
				case IPT_PADDLE:
					options.content_flags[CONTENT_PADDLE] = true;
					break;
				case IPT_DIAL:
					options.content_flags[CONTENT_DIAL] = true;
					break;
				case IPT_TRACKBALL_X:
				case IPT_TRACKBALL_Y:
					options.content_flags[CONTENT_TRACKBALL] = true;
					break;
				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
					options.content_flags[CONTENT_AD_STICK] = true;
					break;
				case IPT_LIGHTGUN_X:
					options.content_flags[CONTENT_LIGHTGUN_COUNT]++; /* only count x to prevent double counting lightguns */
				case IPT_LIGHTGUN_Y:
					options.content_flags[CONTENT_LIGHTGUN] = true;
					break;
				case IPT_SERVICE :
					options.content_flags[CONTENT_HAS_SERVICE] = true;
					break;
				case IPT_TILT :
					options.content_flags[CONTENT_HAS_TILT] = true;
					break;
			}
		}
		++input;
	}

  /************ DRIVERS FLAGGED IN CONTROLS.C WITH ALTERNATING CONTROLS ************/
  if(game_driver->ctrl_dat->alternating_controls)
  {
    options.content_flags[CONTENT_ALTERNATING_CTRLS] = true;
  }

  /************ NUMBER OF DISTINCT CONTROL LAYOUTS ************/
  options.content_flags[CONTENT_CTRL_COUNT] = options.content_flags[CONTENT_PLAYER_COUNT];

  /* There may be a future need to have a ctrl_count different than player_count,
   * perhaps because of some alternating controls layout. This is a place to check
   * some condition and make the two numbers different if that should ever prove useful.
   */
  if(false/*options.content_flags[CONTENT_ALTERNATING_CTRLS]*/)
  {
    options.content_flags[CONTENT_CTRL_COUNT] = 1;
  }

  /************ DRIVERS FLAGGED IN CONTROLS.C WITH 45-DEGREE JOYSTICK ROTATION ************/
  if(game_driver->ctrl_dat->rotate_joy_45)
    options.content_flags[CONTENT_ROTATE_JOY_45] = true;

  /************ DRIVERS FLAGGED IN CONTROLS.C WITH MIRRORED CONTROLS ************/
  if(game_driver->ctrl_dat->mirrored_controls)
    options.content_flags[CONTENT_MIRRORED_CTRLS] = true;

  /************ DRIVERS WITH NVRAM BOOTSTRAP PATCHES ************/
  if(game_driver->bootstrap != NULL)
    options.content_flags[CONTENT_NVRAM_BOOTSTRAP] = true;


  /************ LOG THE STATE OF THE CONTENT FLAGS ************/

  log_cb(RETRO_LOG_INFO, LOGPRE "==== BEGIN DRIVER CONTENT ATTRIBUTES ====\n");

  if(options.content_flags[CONTENT_NEOGEO])     log_cb(RETRO_LOG_INFO, LOGPRE "* Neo Geo BIOS required.\n");
  if(options.content_flags[CONTENT_STV])        log_cb(RETRO_LOG_INFO, LOGPRE "* STV BIOS required.\n");
  if(options.content_flags[CONTENT_DIEHARD])    log_cb(RETRO_LOG_INFO, LOGPRE "* Die Hard: Arcade BIOS required.\n");
  if(options.content_flags[CONTENT_ALT_SOUND])  log_cb(RETRO_LOG_INFO, LOGPRE "* Alternative soundtrack available.\n");
  if(options.content_flags[CONTENT_VECTOR])     log_cb(RETRO_LOG_INFO, LOGPRE "* Vector display.\n");

  log_cb(RETRO_LOG_INFO, LOGPRE "* Supports %i players with %i distinct controls.\n", options.content_flags[CONTENT_PLAYER_COUNT], options.content_flags[CONTENT_CTRL_COUNT]);
  log_cb(RETRO_LOG_INFO, LOGPRE "* Supports %i distinct button controls.\n", options.content_flags[CONTENT_BUTTON_COUNT]);

  if(options.content_flags[CONTENT_DIAL])               log_cb(RETRO_LOG_INFO, LOGPRE "* Uses a dial.\n");
  if(options.content_flags[CONTENT_TRACKBALL])          log_cb(RETRO_LOG_INFO, LOGPRE "* Uses a trackball.\n");
  if(options.content_flags[CONTENT_LIGHTGUN])           log_cb(RETRO_LOG_INFO, LOGPRE "* Uses %i lightgun(s).\n", options.content_flags[CONTENT_LIGHTGUN_COUNT]);
  if(options.content_flags[CONTENT_PADDLE])             log_cb(RETRO_LOG_INFO, LOGPRE "* Uses an paddle.\n");
  if(options.content_flags[CONTENT_AD_STICK])           log_cb(RETRO_LOG_INFO, LOGPRE "* Uses an analog joystick.\n");
  if(options.content_flags[CONTENT_HAS_SERVICE])        log_cb(RETRO_LOG_INFO, LOGPRE "* Uses a service button.\n");
  if(options.content_flags[CONTENT_HAS_TILT])           log_cb(RETRO_LOG_INFO, LOGPRE "* Uses a tilt function.\n");

  if(options.content_flags[CONTENT_ALTERNATING_CTRLS])  log_cb(RETRO_LOG_INFO, LOGPRE "* Uses alternating controls.\n");
  if(options.content_flags[CONTENT_MIRRORED_CTRLS])     log_cb(RETRO_LOG_INFO, LOGPRE "* Uses multiplayer control labels.\n");
  if(options.content_flags[CONTENT_ROTATE_JOY_45])      log_cb(RETRO_LOG_INFO, LOGPRE "* Uses joysticks rotated 45-degrees with respect to the cabinet.\n");
  if(options.content_flags[CONTENT_DUAL_JOYSTICK])      log_cb(RETRO_LOG_INFO, LOGPRE "* Uses dual joysticks.\n");
  if(options.content_flags[CONTENT_JOYSTICK_DIRECTIONS] == 4)
      log_cb(RETRO_LOG_INFO, LOGPRE "* Uses 4-way joystick controls.\n");
  else
      log_cb(RETRO_LOG_INFO, LOGPRE "* Uses 8-way joystick controls.\n");

  if(options.content_flags[CONTENT_NVRAM_BOOTSTRAP])    log_cb(RETRO_LOG_INFO, LOGPRE "* Uses an NVRAM bootstrap controlled via core option.\n");
  if(options.content_flags[CONTENT_CHEAT_INPUT_PORT])   log_cb(RETRO_LOG_INFO, LOGPRE "* Uses a cheat input port / dip switch.\n");

  log_cb(RETRO_LOG_INFO, LOGPRE "==== END DRIVER CONTENT ATTRIBUTES ====\n");
}


static void determine_core_options_version(struct retro_core_options_v2 *core_options_us)
{
   unsigned version  = 0;

   if (!environ_cb)
      return;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
      version = 0;

   if (version >= 2)
   {
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
            core_options_us);
   }
   else
   {
      size_t i, j;
      size_t option_index              = 0;
      size_t num_options               = 0;
      struct retro_core_option_v2_definition
            *option_defs_us            = core_options_us->definitions;
      struct retro_core_option_definition
            *option_v1_defs_us         = NULL;
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

         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
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
