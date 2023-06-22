/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - device.c                                                *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2016 Bobby Smiles                                       *
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

#include "device.h"

#include "ai/ai_controller.h"
#include "memory/memory.h"
#include "pi/pi_controller.h"
#include "r4300/r4300_core.h"
#include "rdp/rdp_core.h"
#include "ri/ri_controller.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"


void init_device(
      struct device *dev,
      /* r4300 */
      unsigned int emumode,
      unsigned int count_per_op,
      int special_rom,
      /* ai */
      void * ai_user_data, void (*ai_set_audio_format)(void*,unsigned int, unsigned int),
      void (*ai_push_audio_samples)(void*,const void*,size_t),
      unsigned int fixed_audio_pos,
      /* pi */
      uint8_t *rom,
      size_t rom_size,
      void* flashram_user_data, void (*flashram_save)(void*), uint8_t* flashram_data,
      void* sram_user_data, void (*sram_save)(void*), uint8_t* sram_data,
      /* ri */
      uint32_t* dram, size_t dram_size,
      /* si */
      void* eeprom_user_data, void (*eeprom_save)(void*), uint8_t* eeprom_data, size_t eeprom_size, uint16_t eeprom_id,
      void* af_rtc_user_data, const struct tm* (*af_rtc_get_time)(void*),
      /* sp */
      unsigned int audio_signal,
      /* vi */
      unsigned int vi_clock,
      unsigned int expected_refresh_rate,
      uint8_t *ddrom,
      size_t ddrom_size,
      uint8_t *dd_disk,
      size_t dd_disk_size
      )
{
   init_r4300(&dev->r4300, emumode, count_per_op, special_rom);
   init_rdp(&dev->dp, &dev->r4300, &dev->sp, &dev->ri);
   init_rsp(&dev->sp, &dev->r4300, &dev->dp, &dev->ri, audio_signal);
   init_ai(&dev->ai, ai_user_data,
         ai_set_audio_format,
         ai_push_audio_samples,
         &dev->r4300, &dev->ri, &dev->vi,
	 fixed_audio_pos);
   init_pi(&dev->pi,
         rom, rom_size,
         ddrom, ddrom_size,
         flashram_user_data, flashram_save, flashram_data,
         sram_user_data, sram_save, sram_data,
         &dev->r4300, &dev->ri);
   init_ri(&dev->ri, dram, dram_size);
   init_si(&dev->si,
         eeprom_user_data,
         eeprom_save,
         eeprom_data,
         eeprom_size,
         eeprom_id,
         af_rtc_user_data,
         af_rtc_get_time,
         ((ddrom != NULL) && (ddrom_size != 0) && (rom == NULL) && (rom_size == 0)) ?
         (ddrom + 0x40) : (rom + 0x40),                       /* ipl3 */
         &dev->r4300, &dev->ri);

   init_vi(&dev->vi, vi_clock, expected_refresh_rate, &dev->r4300);
   init_dd(&dev->dd, &dev->r4300, dd_disk, dd_disk_size);
}

void poweron_device(struct device* dev)
{
    poweron_r4300(&dev->r4300);
    poweron_rdp(&dev->dp);
    poweron_rsp(&dev->sp);
    poweron_ai(&dev->ai);
    poweron_pi(&dev->pi);
    poweron_ri(&dev->ri);
    poweron_si(&dev->si);
    poweron_vi(&dev->vi);
    poweron_dd(&dev->dd);
    poweron_memory();

    /* XXX: somewhat cheating to put it here but not really other option.
     * Proper fix would probably trigerring the first vi
     * when VI_CONTROL_REG[1:0] is set to non zero value */
    add_interrupt_event_count(VI_INT, dev->vi.next_vi);
}
