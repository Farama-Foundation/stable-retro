/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - dd_rom.h                                                *
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

#ifndef __DDROM_H__
#define __DDROM_H__

#include <stdint.h>

#include "api/m64p_types.h"

struct dd_rom
{
	uint8_t* rom;
	size_t rom_size;
};

void init_dd_rom(struct dd_rom* dd_rom,
                    uint8_t* rom, size_t rom_size);

/* ROM Loading and Saving functions */

m64p_error open_ddrom(const unsigned char* romimage, unsigned int size);
m64p_error close_ddrom(void);

extern unsigned char* g_ddrom;
extern int g_ddrom_size;

void poweron_dd_rom(struct dd_rom *dd_rom);

int read_dd_ipl(void* opaque, uint32_t address, uint32_t* value);
int write_dd_ipl(void* opaque, uint32_t address, uint32_t value, uint32_t mask);

#endif
