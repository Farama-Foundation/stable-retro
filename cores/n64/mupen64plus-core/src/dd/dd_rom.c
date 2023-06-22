/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - dd_rom.c                                                *
*   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
*   Copyright (C) 2015 LuigiBlood                                         *
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

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dd_rom.h"
#include "../pi/pi_controller.h"

#include "../api/callbacks.h"
#include "../api/config.h"
#include "../api/m64p_config.h"
#include "../api/m64p_types.h"
#include "../main/main.h"
#include "../main/rom.h"
#include "../main/util.h"

/* Global loaded DDrom memory space. */
unsigned char* g_ddrom = NULL;
/* Global loaded DDrom size. */
int g_ddrom_size = 0;

static const uint8_t Z64_SIGNATURE[4] = { 0x80, 0x27, 0x07, 0x40 };
static const uint8_t V64_SIGNATURE[4] = { 0x27, 0x80, 0x40, 0x07 };
static const uint8_t N64_SIGNATURE[4] = { 0x40, 0x07, 0x27, 0x80 };

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

// Get the VI (vertical interrupt) limit associated to a ROM system type.
static int rom_system_type_to_vi_limit(m64p_system_type system_type)
{
   switch (system_type)
   {
      case SYSTEM_PAL:
      case SYSTEM_MPAL:
         return 50;

      case SYSTEM_NTSC:
      default:
         return 60;
   }
}

static int rom_system_type_to_ai_dac_rate(m64p_system_type system_type)
{
   switch (system_type)
   {
      case SYSTEM_PAL:
         return 49656530;
      case SYSTEM_MPAL:
         return 48628316;
      case SYSTEM_NTSC:
      default:
         return 48681812;
   }
}

/* Tests if a file is a valid 64DD IPL rom by checking the first 4 bytes. */
static int is_valid_rom(const unsigned char *buffer)
{
	if (memcmp(buffer, Z64_SIGNATURE, sizeof(Z64_SIGNATURE)) == 0
		|| memcmp(buffer, V64_SIGNATURE, sizeof(V64_SIGNATURE)) == 0
		|| memcmp(buffer, N64_SIGNATURE, sizeof(N64_SIGNATURE)) == 0)
		return 1;
	else
		return 0;
}

/* Copies the source block of memory to the destination block of memory while
* switching the endianness of .v64 and .n64 images to the .z64 format, which
* is native to the Nintendo 64. The data extraction routines and MD5 hashing
* function may only act on the .z64 big-endian format.
*
* IN: src: The source block of memory. This must be a valid Nintendo 64 ROM
*          image of 'len' bytes.
*     len: The length of the source and destination, in bytes.
* OUT: dst: The destination block of memory. This must be a valid buffer for
*           at least 'len' bytes.
*      imagetype: A pointer to a byte that gets updated with the value of
*                 V64IMAGE, N64IMAGE or Z64IMAGE according to the format of
*                 the source block. The value is undefined if 'src' does not
*                 represent a valid Nintendo 64 ROM image.
*/
static void swap_copy_rom(void* dst, const void* src, size_t len, unsigned char* imagetype)
{
	if (memcmp(src, V64_SIGNATURE, sizeof(V64_SIGNATURE)) == 0)
	{
		size_t i;
		const uint16_t* src16 = (const uint16_t*)src;
		uint16_t* dst16 = (uint16_t*)dst;

		*imagetype = V64IMAGE;
		/* .v64 images have byte-swapped half-words (16-bit). */
		for (i = 0; i < len; i += 2)
		{
			*dst16++ = m64p_swap16(*src16++);
		}
	}
	else if (memcmp(src, N64_SIGNATURE, sizeof(N64_SIGNATURE)) == 0)
	{
		size_t i;
		const uint32_t* src32 = (const uint32_t*)src;
		uint32_t* dst32 = (uint32_t*)dst;

		*imagetype = N64IMAGE;
		/* .n64 images have byte-swapped words (32-bit). */
		for (i = 0; i < len; i += 4)
		{
			*dst32++ = m64p_swap32(*src32++);
		}
	}
	else {
		*imagetype = Z64IMAGE;
		memcpy(dst, src, len);
	}
}

void init_dd_rom(struct dd_rom* dd_rom,
                    uint8_t* rom, size_t rom_size)
{
	dd_rom->rom = rom;
	dd_rom->rom_size = rom_size;
}

void poweron_dd_rom(struct dd_rom *dd_rom)
{
}

m64p_error open_ddrom(const unsigned char* romimage, unsigned int size)
{
	unsigned char imagetype;

	/* check input requirements */
	if (g_ddrom != NULL)
	{
		DebugMessage(M64MSG_ERROR, "open_ddrom(): previous ROM image was not freed");
		return M64ERR_INTERNAL;
	}
	if (romimage == NULL || !is_valid_rom(romimage))
	{
		DebugMessage(M64MSG_ERROR, "open_ddrom(): not a valid ROM image");
		return M64ERR_INPUT_INVALID;
	}

	/* Clear Byte-swapped flag, since ROM is now deleted. */
	g_DDMemHasBeenBSwapped = 0;
	/* allocate new buffer for ROM and copy into this buffer */
	g_ddrom_size = size;
	g_ddrom = (unsigned char *)malloc(size);
	if (g_ddrom == NULL)
		return M64ERR_NO_MEMORY;
	swap_copy_rom(g_ddrom, romimage, size, &imagetype);

	/* add some useful properties to ROM_PARAMS */
    ROM_PARAMS.systemtype = rom_country_code_to_system_type(ROM_HEADER.destination_code);

    memcpy(ROM_PARAMS.headername, ROM_HEADER.Name, 20);
    ROM_PARAMS.headername[20] = '\0';
    trim(ROM_PARAMS.headername); /* Remove trailing whitespace from ROM name. */

	DebugMessage(M64MSG_STATUS, "64DD IPL loaded!");

	return M64ERR_SUCCESS;
}

m64p_error close_ddrom(void)
{
	if (g_ddrom == NULL)
	   return M64ERR_INVALID_STATE;

	free(g_ddrom);
	g_ddrom = NULL;

	/* Clear Byte-swapped flag, since ROM is now deleted. */
	g_DDMemHasBeenBSwapped = 0;
	DebugMessage(M64MSG_STATUS, "Rom closed.");

	return M64ERR_SUCCESS;
}

int read_dd_ipl(void* opaque, uint32_t address, uint32_t* value)
{
	struct pi_controller* pi = (struct pi_controller*)opaque;
	uint32_t offset = address & 0x3FFFFC;

	if (pi->dd_rom.rom == NULL || pi->dd_rom.rom_size == 0)
   {
      *value = 0;
      return 0;
   }

	*value = *(uint32_t*)(pi->dd_rom.rom + offset);

	return 0;
}

int write_dd_ipl(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
	return 0;
}
