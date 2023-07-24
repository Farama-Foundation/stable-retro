#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

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
 *   frontend language definition */

struct retro_core_option_v2_category option_cats_us[] = {
   {
      "video",
      "Video",
      "Configure aspect ratio / display cropping / video filter / frame skipping parameters."
   },
   {
      "input",
      "Input",
      "Configure lightgun / mouse / NegCon input."
   },
   {
      "memcards",
      "Cartridge / Memory Card",
      "This lets you modify settings related to the Saturn cartridge and the virtual Memory Card(s) used by the system."
   },
   {
      "hacks",
      "Emulation hacks",
      "Configure processor overclocking and emulation accuracy parameters affecting low-level performance and compatibility."
   },
   { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {
   {
      "beetle_saturn_horizontal_overscan",
      "Horizontal Overscan Mask",
      NULL,
      "Self-explanatory.",
      NULL,
      "video",
      {
         { "0",       NULL },
         { "2",       NULL },
         { "4",       NULL },
         { "6",       NULL },
         { "8",       NULL },
         { "10",       NULL },
         { "12",       NULL },
         { "14",       NULL },
         { "16",       NULL },
         { "18",       NULL },
         { "20",       NULL },
         { "22",       NULL },
         { "24",       NULL },
         { "26",       NULL },
         { "28",       NULL },
         { "30",       NULL },
         { "32",       NULL },
         { "34",       NULL },
         { "36",       NULL },
         { "38",       NULL },
         { "40",       NULL },
         { "42",       NULL },
         { "44",       NULL },
         { "46",       NULL },
         { "48",       NULL },
         { "50",       NULL },
         { "52",       NULL },
         { "54",       NULL },
         { "56",       NULL },
         { "58",       NULL },
         { "60",       NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "beetle_saturn_initial_scanline",
      "Initial scanline",
      NULL,
      "Adjust the first displayed scanline in NTSC mode.",
      NULL,
      "video",
      {
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
         { "10",       NULL },
         { "11",       NULL },
         { "12",       NULL },
         { "13",       NULL },
         { "14",       NULL },
         { "15",       NULL },
         { "16",       NULL },
         { "17",       NULL },
         { "18",       NULL },
         { "19",       NULL },
         { "20",       NULL },
         { "21",       NULL },
         { "22",       NULL },
         { "23",       NULL },
         { "24",       NULL },
         { "25",       NULL },
         { "26",       NULL },
         { "27",       NULL },
         { "28",       NULL },
         { "29",       NULL },
         { "30",       NULL },
         { "31",       NULL },
         { "32",       NULL },
         { "33",       NULL },
         { "34",       NULL },
         { "35",       NULL },
         { "36",       NULL },
         { "37",       NULL },
         { "38",       NULL },
         { "39",       NULL },
         { "40",       NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "beetle_saturn_last_scanline",
      "Last scanline",
      NULL,
      "Adjust the last displayed scanline in NTSC mode.",
      NULL,
      "video",
      {
         { "210",       NULL },
         { "211",       NULL },
         { "212",       NULL },
         { "213",       NULL },
         { "214",       NULL },
         { "215",       NULL },
         { "216",       NULL },
         { "217",       NULL },
         { "218",       NULL },
         { "219",       NULL },
         { "220",       NULL },
         { "221",       NULL },
         { "222",       NULL },
         { "223",       NULL },
         { "224",       NULL },
         { "225",       NULL },
         { "226",       NULL },
         { "227",       NULL },
         { "228",       NULL },
         { "229",       NULL },
         { "230",       NULL },
         { "231",       NULL },
         { "232",       NULL },
         { "233",       NULL },
         { "234",       NULL },
         { "235",       NULL },
         { "236",       NULL },
         { "237",       NULL },
         { "238",       NULL },
         { "239",       NULL },
         { NULL, NULL },
      },
      "239"
   },
   {
      "beetle_saturn_initial_scanline_pal",
      "Initial scanline (PAL)",
      NULL,
      "Adjust the first displayed scanline in PAL mode.",
      NULL,
      "video",
      {
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
         { "10",       NULL },
         { "11",       NULL },
         { "12",       NULL },
         { "13",       NULL },
         { "14",       NULL },
         { "15",       NULL },
         { "16",       NULL },
         { "17",       NULL },
         { "18",       NULL },
         { "19",       NULL },
         { "20",       NULL },
         { "21",       NULL },
         { "22",       NULL },
         { "23",       NULL },
         { "24",       NULL },
         { "25",       NULL },
         { "26",       NULL },
         { "27",       NULL },
         { "28",       NULL },
         { "29",       NULL },
         { "30",       NULL },
         { "31",       NULL },
         { "32",       NULL },
         { "33",       NULL },
         { "34",       NULL },
         { "35",       NULL },
         { "36",       NULL },
         { "37",       NULL },
         { "38",       NULL },
         { "39",       NULL },
         { "40",       NULL },
         { "41",       NULL },
         { "42",       NULL },
         { "43",       NULL },
         { "44",       NULL },
         { "45",       NULL },
         { "46",       NULL },
         { "47",       NULL },
         { "48",       NULL },
         { "49",       NULL },
         { "50",       NULL },
         { "51",       NULL },
         { "52",       NULL },
         { "53",       NULL },
         { "54",       NULL },
         { "55",       NULL },
         { "56",       NULL },
         { "57",       NULL },
         { "58",       NULL },
         { "59",       NULL },
         { "60",       NULL },
         { NULL, NULL },
      },
      "0"
   },
   {
      "beetle_saturn_last_scanline_pal",
      "Last scanline (PAL)",
      NULL,
      "Adjust the last displayed scanline in PAL mode.",
      NULL,
      "video",
      {
         { "230",       NULL },
         { "231",       NULL },
         { "232",       NULL },
         { "233",       NULL },
         { "234",       NULL },
         { "235",       NULL },
         { "236",       NULL },
         { "237",       NULL },
         { "238",       NULL },
         { "239",       NULL },
         { "240",       NULL },
         { "241",       NULL },
         { "242",       NULL },
         { "243",       NULL },
         { "244",       NULL },
         { "245",       NULL },
         { "246",       NULL },
         { "247",       NULL },
         { "248",       NULL },
         { "249",       NULL },
         { "250",       NULL },
         { "251",       NULL },
         { "252",       NULL },
         { "253",       NULL },
         { "254",       NULL },
         { "255",       NULL },
         { "256",       NULL },
         { "257",       NULL },
         { "258",       NULL },
         { "259",       NULL },
         { "260",       NULL },
         { "261",       NULL },
         { "262",       NULL },
         { "263",       NULL },
         { "264",       NULL },
         { "265",       NULL },
         { "266",       NULL },
         { "267",       NULL },
         { "268",       NULL },
         { "269",       NULL },
         { "270",       NULL },
         { "271",       NULL },
         { "272",       NULL },
         { "273",       NULL },
         { "274",       NULL },
         { "275",       NULL },
         { "276",       NULL },
         { "277",       NULL },
         { "278",       NULL },
         { "279",       NULL },
         { "280",       NULL },
         { "281",       NULL },
         { "282",       NULL },
         { "283",       NULL },
         { "284",       NULL },
         { "285",       NULL },
         { "286",       NULL },
         { "287",       NULL },
         { NULL, NULL },
      },
      "271"
   },
   {
      "beetle_saturn_horizontal_blend",
      "Enable Horizontal Blend(blur)",
      NULL,
      "Enable horizontal blend(blur) filter. Has a more noticeable effect with the Saturn's higher horizontal resolution modes(640/704).",
      NULL,
      "video",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "beetle_saturn_multitap_port1",
      "6Player Adaptor on Port 1",
      NULL,
      "Enable multitap on Saturn port 1.",
      NULL,
      "input",
      {
         { "disabled",   NULL },
         { "enabled",   NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "beetle_saturn_multitap_port2",
      "6Player Adaptor on Port 2",
      NULL,
      "Enable multitap on Saturn port 2.",
      NULL,
      "input",
      {
         { "disabled",   NULL },
         { "enabled",   NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "beetle_saturn_opposite_directions",
      "Allow Up+Down and Left+Right",
      NULL,
      "Enabling this will allow pressing up and down or left and right directions at the same time. This may cause glitches.",
      NULL,
      "input",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "beetle_saturn_analog_stick_deadzone",
      "Analog Stick Deadzone",
      NULL,
      "Configure the '3D Control Pad' Device Type's analog deadzone.",
      NULL,
      "input",
      {
         { "0%",   NULL },
         { "5%",   NULL },
         { "10%",   NULL },
         { "15%",   NULL },
         { "20%",   NULL },
         { "25%",   NULL },
         { "30%",   NULL },
         { NULL, NULL },
      },
      "15%"
   },
   {
      "beetle_saturn_trigger_deadzone",
      "Trigger Deadzone",
      NULL,
      "Configure the '3D Control Pad' Device Type's trigger deadzone.",
      NULL,
      "input",
      {
         { "0%",   NULL },
         { "5%",   NULL },
         { "10%",   NULL },
         { "15%",   NULL },
         { "20%",   NULL },
         { "25%",   NULL },
         { "30%",   NULL },
         { NULL, NULL },
      },
      "15%"
   },
   {
      "beetle_saturn_mouse_sensitivity",
      "Mouse Sensitivity",
      NULL,
      "Configure the 'Mouse' device type's sensitivity.",
      NULL,
      "input",
      {
         { "5%",   NULL },
         { "10%",   NULL },
         { "15%",   NULL },
         { "20%",   NULL },
         { "25%",   NULL },
         { "30%",   NULL },
         { "35%",   NULL },
         { "40%",   NULL },
         { "45%",   NULL },
         { "50%",   NULL },
         { "55%",   NULL },
         { "60%",   NULL },
         { "65%",   NULL },
         { "70%",   NULL },
         { "75%",   NULL },
         { "80%",   NULL },
         { "85%",   NULL },
         { "90%",   NULL },
         { "95%",   NULL },
         { "100%",   NULL },
         { "105%",   NULL },
         { "110%",   NULL },
         { "115%",   NULL },
         { "120%",   NULL },
         { "125%",   NULL },
         { "130%",   NULL },
         { "135%",   NULL },
         { "140%",   NULL },
         { "145%",   NULL },
         { "150%",   NULL },
         { "155%",   NULL },
         { "160%",   NULL },
         { "165%",   NULL },
         { "170%",   NULL },
         { "175%",   NULL },
         { "180%",   NULL },
         { "185%",   NULL },
         { "190%",   NULL },
         { "195%",   NULL },
         { "200%",   NULL },
         { NULL, NULL },
      },
      "100%"
   },
   {
      "beetle_saturn_virtuagun_crosshair",
      "Gun Crosshair",
      NULL,
      "Choose the crosshair for the 'Stunner' and 'Virtua Gun' device types. Setting it to Off disables the crosshair.",
      NULL,
      "input",
      {
         { "Cross",   NULL },
         { "Dot",   NULL },
         { "Off",   NULL },
         { NULL, NULL },
      },
      "Cross"
   },
   {
      "beetle_saturn_virtuagun_input",
      "Gun Input Mode",
      NULL,
      "",
      NULL,
      "input",
      {
         { "Lightgun",   NULL },
         { "Touchscreen",   NULL },
         { NULL, NULL },
      },
      "Lightgun"
   },
   {
      "beetle_saturn_cart",
      "Cartridge",
      NULL,
      "Certain games require an external cartridge to run (ROM Cart, 1M RAM Cart, 4M RAM Cart).",
      NULL,
      "memcards",
      {
         { "Auto Detect",   NULL },
         { "None",   NULL },
         { "Backup Memory",   NULL },
         { "Extended RAM (1MB)",   NULL },
         { "Extended RAM (4MB)",   NULL },
         { "The King of Fighters '95",   NULL },
         { "Ultraman: Hikari no Kyojin Densetsu",   NULL },
         { NULL, NULL },
      },
      "Auto Detect"
   },
   {
      "beetle_saturn_shared_int",
      "Shared Internal Memory (Restart)",
      NULL,
      "Enables shared internal memory.",
      NULL,
      "memcards",
      {
         { "enabled", NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "beetle_saturn_shared_ext",
      "Shared Backup Memory (Restart)",
      NULL,
      "Enables shared backup memory.",
      NULL,
      "memcards",
      {
         { "enabled", NULL },
         { "disabled", NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "beetle_saturn_region",
      "System Region",
      NULL,
      "Choose which region the system is from.",
      NULL,
      NULL,
      {
         { "Auto Detect",   NULL },
         { "Japan",   NULL },
         { "North America",   NULL },
         { "Europe",   NULL },
         { "South Korea",   NULL },
         { "Asia (NTSC)",   NULL },
         { "Asia (PAL)",   NULL },
         { "Brazil",   NULL },
         { "Latin America",   NULL },
         { NULL, NULL },
      },
      "Auto Detect"
   },
   {
      "beetle_saturn_autortc_lang",
      "BIOS language",
      NULL,
      "Also affects language used in some games (e.g. the European release of 'Panzer Dragoon').",
      NULL,
      NULL,
      {
         { "english", "English" },
         { "german",  "German" },
         { "french",  "French" },
         { "spanish", "Spanish" },
         { "italian", "Italian" },
         { "japanese", "Japanese" },
         { NULL, NULL },
      },
      "english"
   },
   {
      "beetle_saturn_cdimagecache",
      "CD Image Cache (Restart)",
      NULL,
      "Loads the complete image in memory at startup. Can potentially decrease loading times at the cost of increased startup time. Requires a restart in order for a change to take effect.",
      NULL,
      NULL,
      {
         { "disabled",   NULL },
         { "enabled",   NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   
   
   {
      "beetle_saturn_midsync",
      "Mid-frame Input Synchronization",
      NULL,
      "Mid-frame synchronization can reduce input latency, but it will increase CPU requirements.",
      NULL,
      NULL,
      {
         { "disabled",   NULL },
         { "enabled",   NULL },
         { NULL, NULL },
      },
      "disabled"
   },
   {
      "beetle_saturn_autortc",
      "Automatically set RTC on game load",
      NULL,
      "Automatically set the SMPC's emulated Real-Time Clock to the host system's current time and date upon game load.",
      NULL,
      NULL,
      {
         { "enabled",   NULL },
         { "disabled",   NULL },
         { NULL, NULL },
      },
      "enabled"
   },

   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

/* RETRO_LANGUAGE_JAPANESE */

/* RETRO_LANGUAGE_FRENCH */

/* RETRO_LANGUAGE_SPANISH */

/* RETRO_LANGUAGE_GERMAN */

/* RETRO_LANGUAGE_ITALIAN */

/* RETRO_LANGUAGE_DUTCH */

/* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */

/* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */

/* RETRO_LANGUAGE_RUSSIAN */

/* RETRO_LANGUAGE_KOREAN */

/* RETRO_LANGUAGE_CHINESE_TRADITIONAL */

/* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */

/* RETRO_LANGUAGE_ESPERANTO */

/* RETRO_LANGUAGE_POLISH */

/* RETRO_LANGUAGE_VIETNAMESE */

/* RETRO_LANGUAGE_ARABIC */

/* RETRO_LANGUAGE_GREEK */

/* RETRO_LANGUAGE_TURKISH */

struct retro_core_options_v2 options_us = {
   option_cats_us,
   option_defs_us
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

struct retro_core_options_v2 *options_intl[RETRO_LANGUAGE_LAST] = {
   &options_us,    /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};

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
