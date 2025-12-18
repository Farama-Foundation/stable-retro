/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - main.c                                                  *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 CasualJames                                        *
 *   Copyright (C) 2008-2009 Richard Goedeken                              *
 *   Copyright (C) 2008 Ebenblues Nmn Okaygo Tillin9                       *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This is MUPEN64's main entry point. It contains code that is common
 * to both the gui and non-gui versions of mupen64. See
 * gui subdirectories for the gui-specific code.
 * if you want to implement an interface, you should look here
 */

#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdarg.h>

#define M64P_CORE_PROTOTYPES 1
#include "../api/m64p_types.h"
#include "../api/callbacks.h"
#include "../api/config.h"
#include "../api/m64p_config.h"
#include "../api/debugger.h"
#include "../api/m64p_vidext.h"
#include "../api/vidext.h"

#include "main.h"
#include "cheat.h"
#include "device.h"
#include "eventloop.h"
#include "rom.h"
#include "savestates.h"
#include "util.h"

#include "../ai/ai_controller.h"
#include "../memory/memory.h"
#include "../osal/preproc.h"
#include "../pi/pi_controller.h"
#include "../plugin/plugin.h"
#include "../plugin/emulate_game_controller_via_input_plugin.h"
#include "../plugin/get_time_using_C_localtime.h"
#include "../plugin/rumble_via_input_plugin.h"
#include "../pifbootrom/pifbootrom.h"
#include "../r4300/r4300.h"
#include "../r4300/r4300_core.h"
#include "../r4300/reset.h"
#include "../rdp/rdp_core.h"
#include "../rsp/rsp_core.h"
#include "../ri/ri_controller.h"
#include "../si/si_controller.h"
#include "../vi/vi_controller.h"
#include "../dd/dd_controller.h"
#include "../dd/dd_rom.h"
#include "../dd/dd_disk.h"

#ifdef DBG
#include "../debugger/dbg_types.h"
#include "../debugger/debugger.h"
#endif

#include <libretro.h>

void set_audio_format_via_libretro(void* user_data,
      unsigned int frequency, unsigned int bits);

void push_audio_samples_via_libretro(void* user_data, const void* buffer, size_t size);

extern retro_input_poll_t poll_cb;

/* version number for Core config section */
#define CONFIG_PARAM_VERSION 1.01

/** globals **/
m64p_handle g_CoreConfig = NULL;

m64p_frame_callback g_FrameCallback = NULL;

int         g_MemHasBeenBSwapped = 0;   /* store byte-swapped flag so we don't swap twice when re-playing game */
int        g_DDMemHasBeenBSwapped = 0; /* store byte-swapped flag so we don't swap twice when re-playing game */
int         g_EmulatorRunning = 0;      /* need separate boolean to tell if emulator is running, since --nogui doesn't use a thread */

/* XXX: only global because of new dynarec linkage_x86.asm and plugin.c */
/* Need some guardbands so we can import g_rdram in parallel-RDP on Intel Windows which requires 64k alignment.
 * Windows obj files don't seem to support more than 8 KiB alignment ... */
uint32_t rdram_pre_guardband[16 * 1024];
ALIGN(4096, uint32_t g_rdram[RDRAM_MAX_SIZE/4]);
uint32_t rdram_post_guardband[16 * 1024];
struct device g_dev;
struct r4300_core g_r4300;

int g_delay_si = 0;

int g_gs_vi_counter = 0;

/** static (local) variables **/
static int   l_CurrentFrame = 0;         // frame counter

/*********************************************************************************************************
* helper functions
*/

void main_message(m64p_msg_level level, unsigned int corner, const char *format, ...)
{
   va_list ap;
   char buffer[2049];
   va_start(ap, format);
   vsnprintf(buffer, 2047, format, ap);
   buffer[2048]='\0';
   va_end(ap);

   /* send message to front-end */
   DebugMessage(level, "%s", buffer);
}

/*********************************************************************************************************
* global functions, for adjusting the core emulator behavior
*/

int main_set_core_defaults(void)
{
   float fConfigParamsVersion;
   int bSaveConfig = 0, bUpgrade = 0;

   if (ConfigGetParameter(g_CoreConfig, "Version", M64TYPE_FLOAT, &fConfigParamsVersion, sizeof(float)) != M64ERR_SUCCESS)
   {
      DebugMessage(M64MSG_WARNING, "No version number in 'Core' config section. Setting defaults.");
      ConfigDeleteSection("Core");
      ConfigOpenSection("Core", &g_CoreConfig);
      bSaveConfig = 1;
   }
   else if (((int) fConfigParamsVersion) != ((int) CONFIG_PARAM_VERSION))
   {
      DebugMessage(M64MSG_WARNING, "Incompatible version %.2f in 'Core' config section: current is %.2f. Setting defaults.", fConfigParamsVersion, (float) CONFIG_PARAM_VERSION);
      ConfigDeleteSection("Core");
      ConfigOpenSection("Core", &g_CoreConfig);
      bSaveConfig = 1;
   }
   else if ((CONFIG_PARAM_VERSION - fConfigParamsVersion) >= 0.0001f)
   {
      float fVersion = (float) CONFIG_PARAM_VERSION;
      ConfigSetParameter(g_CoreConfig, "Version", M64TYPE_FLOAT, &fVersion);
      DebugMessage(M64MSG_INFO, "Updating parameter set version in 'Core' config section to %.2f", fVersion);
      bUpgrade = 1;
      bSaveConfig = 1;
   }

   /* parameters controlling the operation of the core */
   ConfigSetDefaultFloat(g_CoreConfig, "Version", (float) CONFIG_PARAM_VERSION,  "Mupen64Plus Core config parameter set version number.  Please don't change this version number.");
   ConfigSetDefaultBool(g_CoreConfig, "OnScreenDisplay", 1, "Draw on-screen display if True, otherwise don't draw OSD");
#if defined(DYNAREC)
   ConfigSetDefaultInt(g_CoreConfig, "R4300Emulator", 2, "Use Pure Interpreter if 0, Cached Interpreter if 1, or Dynamic Recompiler if 2 or more");
#else
   ConfigSetDefaultInt(g_CoreConfig, "R4300Emulator", 1, "Use Pure Interpreter if 0, Cached Interpreter if 1, or Dynamic Recompiler if 2 or more");
#endif
   ConfigSetDefaultBool(g_CoreConfig, "NoCompiledJump", 0, "Disable compiled jump commands in dynamic recompiler (should be set to False) ");
   ConfigSetDefaultBool(g_CoreConfig, "DisableExtraMem", 0, "Disable 4MB expansion RAM pack. May be necessary for some games");
   ConfigSetDefaultBool(g_CoreConfig, "EnableDebugger", 0, "Activate the R4300 debugger when ROM execution begins, if core was built with Debugger support");
   ConfigSetDefaultInt(g_CoreConfig, "CountPerOp", 0, "Force number of cycles per emulated instruction.");
   ConfigSetDefaultBool(g_CoreConfig, "DelaySI", 1, "Delay interrupt after DMA SI read/write");

   if (bSaveConfig)
      ConfigSaveSection("Core");

   return 1;
}

m64p_error main_core_state_query(m64p_core_param param, int *rval)
{
   switch (param)
   {
      case M64CORE_EMU_STATE:
         if (!g_EmulatorRunning)
            *rval = M64EMU_STOPPED;
         else
            *rval = M64EMU_RUNNING;
         break;
      case M64CORE_INPUT_GAMESHARK:
         *rval = event_gameshark_active();
         break;
      default:
         return M64ERR_INPUT_INVALID;
   }

   return M64ERR_SUCCESS;
}

m64p_error main_core_state_set(m64p_core_param param, int val)
{
   switch (param)
   {
      case M64CORE_EMU_STATE:
         if (!g_EmulatorRunning)
            return M64ERR_INVALID_STATE;
         if (val == M64EMU_STOPPED)
         {
            /* this stop function is asynchronous.  The emulator may not terminate until later */
             mupen_main_stop();
            return M64ERR_SUCCESS;
         }
         else if (val == M64EMU_RUNNING)
            return M64ERR_SUCCESS;
         return M64ERR_INPUT_INVALID;
      case M64CORE_INPUT_GAMESHARK:
         if (!g_EmulatorRunning)
            return M64ERR_INVALID_STATE;
         event_set_gameshark(val);
         return M64ERR_SUCCESS;
      default:
         break;
   }

   return M64ERR_INPUT_INVALID;
}

m64p_error main_read_screen(void *pixels, int bFront)
{
   int width_trash, height_trash;
   gfx.readScreen(pixels, &width_trash, &height_trash, bFront);
   return M64ERR_SUCCESS;
}

m64p_error main_reset(int do_hard_reset)
{
   if (do_hard_reset)
      reset_hard_job |= 1;
   else
      reset_soft();
   return M64ERR_SUCCESS;
}

/*********************************************************************************************************
* global functions, callbacks from the r4300 core or from other plugins
*/

void new_frame(void)
{
   if (g_FrameCallback)
      (*g_FrameCallback)(l_CurrentFrame++);
}

void mupen_main_exit(void)
{
   /* now begin to shut down */
#ifdef DBG
   if (g_DebuggerActive)
      destroy_debugger();
#endif

   if (rsp.romClosed) rsp.romClosed();
   if (input.romClosed) input.romClosed();
   if (gfx.romClosed) gfx.romClosed();

   // clean up
   g_EmulatorRunning = 0;
   StateChanged(M64CORE_EMU_STATE, M64EMU_STOPPED);
}

/* TODO: make a GameShark module and move that there */
static void gs_apply_cheats(void)
{
   if(g_gs_vi_counter < 60)
   {
      if (g_gs_vi_counter == 0)
         cheat_apply_cheats(ENTRY_BOOT);
      g_gs_vi_counter++;
   }
   else
   {
      cheat_apply_cheats(ENTRY_VI);
   }
}

/* called on vertical interrupt.
 * Allow the core to perform various things */
void new_vi(void)
{
   gs_apply_cheats();

   main_check_inputs();

#if 0
   timed_sections_refresh();

   pause_loop();

   apply_speed_limiter();
#endif
}

static void dummy_save(void *user_data)
{
}

/*********************************************************************************************************
* emulation thread - runs the core
*/
m64p_error main_init(void)
{
   size_t i;
   unsigned int disable_extra_mem;
   /* take the r4300 emulator mode from the config file at this point and cache it in a global variable */
   unsigned int emumode = ConfigGetParamInt(g_CoreConfig, "R4300Emulator");

   /* set some other core parameters based on the config file values */
   no_compiled_jump = ConfigGetParamBool(g_CoreConfig, "NoCompiledJump");
   disable_extra_mem = ConfigGetParamInt(g_CoreConfig, "DisableExtraMem");
#if 0
   count_per_op = ConfigGetParamInt(g_CoreConfig, "CountPerOp");
#endif

   if (count_per_op <= 0)
      count_per_op = 2;
   if (g_vi_refresh_rate == 0)
      g_vi_refresh_rate = 1500;

   /* do byte-swapping if it's not been done yet */
   if (g_MemHasBeenBSwapped == 0)
   {
      swap_buffer(g_rom, 4, g_rom_size / 4);
      g_MemHasBeenBSwapped = 1;
   }

   if (g_DDMemHasBeenBSwapped == 0)
   {
      swap_buffer(g_ddrom, 4, g_ddrom_size / 4);
      g_DDMemHasBeenBSwapped = 1;
   }

   init_device(
         &g_dev,
         emumode,
         count_per_op,
	 ROM_PARAMS.special_rom,
         NULL,
         set_audio_format_via_libretro,
         push_audio_samples_via_libretro,
	 ROM_PARAMS.fixedaudiopos,
         g_rom, g_rom_size,
         NULL, dummy_save, saved_memory.flashram,
         NULL, dummy_save, saved_memory.sram,
         g_rdram, (disable_extra_mem == 0) ? 0x800000 : 0x400000,
         NULL,dummy_save,saved_memory.eeprom,
         ROM_SETTINGS.savetype != EEPROM_16KB ? 0x200  : 0x800,   /* eeprom_size */
         ROM_SETTINGS.savetype != EEPROM_16KB ? 0x8000 : 0xc000,  /* eeprom_id   */
         NULL,                                                    /* af_rtc_userdata */
         get_time_using_C_localtime,                              /* af_rtc_get_time */
	 ROM_PARAMS.audiosignal,
         vi_clock_from_tv_standard(ROM_PARAMS.systemtype),
         vi_expected_refresh_rate_from_tv_standard(ROM_PARAMS.systemtype),
         g_ddrom, g_ddrom_size, g_dd_disk, g_dd_disk_size);


   // Attach rom to plugins
   printf("Gfx RomOpen.\n");
   if (!gfx.romOpen())
   {
      printf("Gfx RomOpen failed.\n");
      return M64ERR_PLUGIN_FAIL;
   }

#ifdef DBG
   if (ConfigGetParamBool(g_CoreConfig, "EnableDebugger"))
      init_debugger();
#endif

   g_EmulatorRunning = 1;
   StateChanged(M64CORE_EMU_STATE, M64EMU_RUNNING);

   /* call r4300 CPU core and run the game */
   poweron_device(&g_dev);
   pifbootrom_hle_execute(&g_dev);

   return M64ERR_SUCCESS;
}

m64p_error main_pre_run(void)
{
   r4300_init();

   return M64ERR_SUCCESS;
}

m64p_error main_run(void)
{
   r4300_execute();

   return M64ERR_SUCCESS;
}

void mupen_main_stop(void)
{
   /* note: this operation is asynchronous.  It may be called from a thread other than the
      main emulator thread, and may return before the emulator is completely stopped */
   if (!g_EmulatorRunning)
      return;

   DebugMessage(M64MSG_STATUS, "Stopping emulation.");
   stop = 1;
#ifdef DBG
   if(g_DebuggerActive)
      debugger_step();
#endif
}

void main_check_inputs(void)
{
   poll_cb();
}
