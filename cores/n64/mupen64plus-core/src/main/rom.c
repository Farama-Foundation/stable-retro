/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rom.c                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <boolean.h>

#define M64P_CORE_PROTOTYPES 1
#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "api/config.h"
#include "api/m64p_config.h"

#include "md5.h"
#include "rom.h"
#include "main.h"
#include "util.h"

#include "memory/memory.h"
#include "osal/preproc.h"

#include "../r4300/r4300.h"

#define DEFAULT 16

/* Amount of cpu cycles per vi scanline - empirically determined */
enum { DEFAULT_COUNT_PER_SCANLINE = 1500 };
/* by default, alternate VI timing is disabled */
enum { DEFAULT_ALTERNATE_VI_TIMING = 0 };
/* by default, fixed audio position is disabled */
enum { DEFAULT_FIXED_AUDIO_POS = 0 };
/* by default, Audio Signal is disabled */
enum { DEFAULT_AUDIO_SIGNAL = 0 };

/* Global loaded rom memory space. */
unsigned char* g_rom = NULL;
/* Global loaded rom size. */
int g_rom_size = 0;
unsigned alternate_vi_timing = 0;
int           g_vi_refresh_rate = DEFAULT_COUNT_PER_SCANLINE;

extern bool frame_dupe;

m64p_rom_header   ROM_HEADER;
rom_params        ROM_PARAMS;
m64p_rom_settings ROM_SETTINGS = { "", "", 0, 0, 0, 0, 0x900 };


static const uint8_t Z64_SIGNATURE[4] = { 0x80, 0x37, 0x12, 0x40 };
static const uint8_t V64_SIGNATURE[4] = { 0x37, 0x80, 0x40, 0x12 };
static const uint8_t N64_SIGNATURE[4] = { 0x40, 0x12, 0x37, 0x80 };

// Get the system type associated to a ROM country code.
static m64p_system_type rom_country_code_to_system_type(char country_code)
{
   switch (country_code)
   {
      // PAL codes
      case 0x44:
      case 0x46:
      case 0x49:
      case 0x50:
      case 0x53:
      case 0x55:
      case 0x58:
      case 0x59:
         return SYSTEM_PAL;

         // NTSC codes
      case 0x37:
      case 0x41:
      case 0x45:
      case 0x4a:
      default: // Fallback for unknown codes
         return SYSTEM_NTSC;
   }
}

/* Tests if a file is a valid N64 rom by checking the first 4 bytes. */
static int is_valid_rom(const unsigned char *buffer)
{
   if (memcmp(buffer, Z64_SIGNATURE, sizeof(Z64_SIGNATURE)) == 0
         || memcmp(buffer, V64_SIGNATURE, sizeof(V64_SIGNATURE)) == 0
         || memcmp(buffer, N64_SIGNATURE, sizeof(N64_SIGNATURE)) == 0)
      return 1;
   else
      return 0;
}

/* If rom is a .v64 or .n64 image, byteswap or wordswap loadlength amount of
 * rom data to native .z64 before forwarding. Makes sure that data extraction
 * and MD5ing routines always deal with a .z64 image.
 */
static void swap_rom(unsigned char* localrom, unsigned char* imagetype, int loadlength)
{
   unsigned char temp;
   int i;

   /* Btyeswap if .v64 image. */
   if(localrom[0]==0x37)
   {
      *imagetype = V64IMAGE;
      for (i = 0; i < loadlength; i+=2)
      {
         temp=localrom[i];
         localrom[i]=localrom[i+1];
         localrom[i+1]=temp;
      }
   }
   /* Wordswap if .n64 image. */
   else if(localrom[0]==0x40)
   {
      *imagetype = N64IMAGE;
      for (i = 0; i < loadlength; i+=4)
      {
         temp=localrom[i];
         localrom[i]=localrom[i+3];
         localrom[i+3]=temp;
         temp=localrom[i+1];
         localrom[i+1]=localrom[i+2];
         localrom[i+2]=temp;
      }
   }
   else
      *imagetype = Z64IMAGE;
}

m64p_error open_rom(const unsigned char* romimage, unsigned int size)
{
#include "rom_luts.c"
   md5_state_t state;
   md5_byte_t digest[16];
   char buffer[256];
   unsigned char imagetype;
   int i;
   uint64_t lut_id;
   int patch_applied = 0;

   /* check input requirements */
   if (g_rom != NULL)
   {
      DebugMessage(M64MSG_ERROR, "open_rom(): previous ROM image was not freed");
      return M64ERR_INTERNAL;
   }
   if (romimage == NULL || !is_valid_rom(romimage))
   {
      DebugMessage(M64MSG_ERROR, "open_rom(): not a valid ROM image");
      return M64ERR_INPUT_INVALID;
   }

   /* Clear Byte-swapped flag, since ROM is now deleted. */
   g_MemHasBeenBSwapped = 0;
   /* allocate new buffer for ROM and copy into this buffer */
   g_rom_size = size;
   g_rom = (unsigned char *) malloc(size);
   alternate_vi_timing = 0;
   g_vi_refresh_rate = DEFAULT_COUNT_PER_SCANLINE;
   if (g_rom == NULL)
      return M64ERR_NO_MEMORY;
   memcpy(g_rom, romimage, size);
   swap_rom(g_rom, &imagetype, g_rom_size);

   memcpy(&ROM_HEADER, g_rom, sizeof(m64p_rom_header));

   /* Calculate MD5 hash  */
   md5_init(&state);
   md5_append(&state, (const md5_byte_t*)g_rom, g_rom_size);
   md5_finish(&state, digest);
   for ( i = 0; i < 16; ++i )
      sprintf(buffer+i*2, "%02X", digest[i]);
   buffer[32] = '\0';
   strcpy(ROM_SETTINGS.MD5, buffer);
   
   ROM_SETTINGS.sidmaduration = 0x900;

   /* add some useful properties to ROM_PARAMS */
   ROM_PARAMS.systemtype = rom_country_code_to_system_type(ROM_HEADER.destination_code);
   ROM_PARAMS.fixedaudiopos = DEFAULT_FIXED_AUDIO_POS;
   ROM_PARAMS.audiosignal = DEFAULT_AUDIO_SIGNAL;

   memcpy(ROM_PARAMS.headername, ROM_HEADER.Name, 20);
   ROM_PARAMS.headername[20] = '\0';
   trim(ROM_PARAMS.headername); /* Remove trailing whitespace from ROM name. */

   lut_id = (((uint64_t)sl(ROM_HEADER.CRC1)) << 32) | sl(ROM_HEADER.CRC2);

   for (i = 0; i < sizeof(lut_ee16k)/sizeof(lut_ee16k[0]); ++i)
   {
      if (lut_ee16k[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         ROM_SETTINGS.savetype = EEPROM_16KB;
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_audiosignal)/sizeof(lut_audiosignal[0]); ++i)
   {
      if (lut_audiosignal[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         ROM_PARAMS.audiosignal = 1;
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_fixedaudiopos)/sizeof(lut_fixedaudiopos[0]); ++i)
   {
      if (lut_fixedaudiopos[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         ROM_PARAMS.fixedaudiopos = 1;
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_alternate_vi)/sizeof(lut_alternate_vi[0]); ++i)
   {
      if (lut_alternate_vi[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         alternate_vi_timing = 1;
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_vi_clock_1500)/sizeof(lut_vi_clock_1500[0]); ++i)
   {
      if (lut_vi_clock_1500[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);
	 g_vi_refresh_rate = 1500;

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_vi_clock_1600)/sizeof(lut_vi_clock_1600[0]); ++i)
   {
      if (lut_vi_clock_1600[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);
	 g_vi_refresh_rate = 1600;

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_vi_clock_2200)/sizeof(lut_vi_clock_2200[0]); ++i)
   {
      if (lut_vi_clock_2200[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);
	 g_vi_refresh_rate = 2200;

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_ee4k)/sizeof(lut_ee4k[0]); ++i)
   {
      if (lut_ee4k[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         ROM_SETTINGS.savetype = EEPROM_4KB;
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);

         patch_applied = 1;
         break;
      }
   }

   for (i = 0; i < sizeof(lut_flashram)/sizeof(lut_flashram[0]); ++i)
   {
      if (lut_flashram[i] == lut_id)
      {
         strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
         ROM_SETTINGS.savetype = FLASH_RAM;
         DebugMessage(M64MSG_INFO, "%s INI patches applied.", ROM_PARAMS.headername);

         patch_applied = 1;
         break;
      }
   }

   if (!patch_applied)
   {
      strcpy(ROM_SETTINGS.goodname, ROM_PARAMS.headername);
      strcat(ROM_SETTINGS.goodname, " (unknown rom)");
      ROM_SETTINGS.savetype = NONE;
      ROM_SETTINGS.sidmaduration = 0x900;
      ROM_SETTINGS.status = 0;
      ROM_SETTINGS.players = 0;
      ROM_SETTINGS.rumble = 0;
   }

   for (i = 0; i < sizeof(lut_cpop)/sizeof(lut_cpop[0]); ++i)
   {
      if (lut_cpop[i][0] == lut_id)
      {
         count_per_op = lut_cpop[i][1];
         DebugMessage(M64MSG_INFO, "CountPerOp set to %u.", count_per_op);
         break;
      }
   }

   for (i = 0; i < sizeof(lut_sidmaduration)/sizeof(lut_sidmaduration[0]); ++i)
   {
      if (lut_sidmaduration[i][0] == lut_id)
      {
         ROM_SETTINGS.sidmaduration = lut_sidmaduration[i][1];
         DebugMessage(M64MSG_INFO, "SI DMA Duration set to %u.", ROM_SETTINGS.sidmaduration);
         break;
      }
   }

   if (frame_dupe)
      count_per_op = 1;

   g_delay_si = 1; /* default */

   for (i = 0; i < sizeof(lut_delaysi)/sizeof(lut_delaysi[0]); ++i)
   {
      if (lut_delaysi[i][0] == lut_id)
      {
         g_delay_si = lut_delaysi[i][1];
         DebugMessage(M64MSG_INFO, "DelaySI set to %u.", g_delay_si);
         break;
      }
   }

   /* print out a bunch of info about the ROM */
   DebugMessage(M64MSG_INFO, "Goodname: %s", ROM_SETTINGS.goodname);
   DebugMessage(M64MSG_INFO, "Headername: %s", ROM_PARAMS.headername);
   DebugMessage(M64MSG_INFO, "Name: %s", ROM_HEADER.Name);
   imagestring(imagetype, buffer);
   DebugMessage(M64MSG_INFO, "MD5: %s", ROM_SETTINGS.MD5);
   DebugMessage(M64MSG_INFO, "CRC: %x %x", sl(ROM_HEADER.CRC1), sl(ROM_HEADER.CRC2));
   DebugMessage(M64MSG_INFO, "Imagetype: %s", buffer);
   DebugMessage(M64MSG_INFO, "Rom size: %d bytes (or %d Mb or %d Megabits)", g_rom_size, g_rom_size/1024/1024, g_rom_size/1024/1024*8);
   DebugMessage(M64MSG_VERBOSE, "ClockRate = %x", sl(ROM_HEADER.ClockRate));
   DebugMessage(M64MSG_INFO, "Version: %x", sl(ROM_HEADER.Release));
   if(sl(ROM_HEADER.Manufacturer_ID) == 'N')
      DebugMessage(M64MSG_INFO, "Manufacturer: Nintendo");
   else
      DebugMessage(M64MSG_INFO, "Manufacturer: %x", sl(ROM_HEADER.Manufacturer_ID));
   DebugMessage(M64MSG_VERBOSE, "Cartridge_ID: %x", ROM_HEADER.Cartridge_ID);
   countrycodestring(ROM_HEADER.destination_code, buffer);
   DebugMessage(M64MSG_INFO, "Country: %s", buffer);
   DebugMessage(M64MSG_VERBOSE, "PC = %x", sl((unsigned int)ROM_HEADER.PC));
   DebugMessage(M64MSG_VERBOSE, "Save type: %d", ROM_SETTINGS.savetype);

   if (!strcmp(ROM_PARAMS.headername, "GOLDENEYE"))
      ROM_PARAMS.special_rom = GOLDEN_EYE;
   else if (!strcmp(ROM_PARAMS.headername, "RAT ATTACK"))
      ROM_PARAMS.special_rom = RAT_ATTACK;
   else if (!strcmp(ROM_PARAMS.headername, "Perfect Dark"))
      ROM_PARAMS.special_rom = PERFECT_DARK;
   else
      ROM_PARAMS.special_rom = NORMAL_ROM;

   return M64ERR_SUCCESS;
}

m64p_error close_rom(void)
{
   if (g_rom == NULL)
      return M64ERR_INVALID_STATE;

   free(g_rom);
   g_rom = NULL;

   /* Clear Byte-swapped flag, since ROM is now deleted. */
   g_MemHasBeenBSwapped = 0;
   DebugMessage(M64MSG_STATUS, "Rom closed.");

   return M64ERR_SUCCESS;
}
