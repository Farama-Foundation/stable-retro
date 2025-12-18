/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rom.h                                                   *
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

#ifndef __ROM_H__
#define __ROM_H__

#include "api/m64p_types.h"
#include "md5.h"

/* ROM Loading and Saving functions */

m64p_error open_rom(const unsigned char* romimage, unsigned int size);
m64p_error close_rom(void);

extern unsigned char* g_rom;
extern int g_rom_size;
extern int g_vi_refresh_rate;
extern unsigned char g_fixed_audio_pos;

typedef struct _rom_params
{
   m64p_system_type systemtype;
   char headername[21];  /* ROM Name as in the header, removing trailing whitespace */
   int fixedaudiopos;
   int audiosignal;
   int special_rom;
} rom_params;

extern m64p_rom_header   ROM_HEADER;
extern rom_params        ROM_PARAMS;
extern m64p_rom_settings ROM_SETTINGS;

/* Supported rom image types. */
enum 
{
    Z64IMAGE,
    V64IMAGE,
    N64IMAGE
};

/* Supported CIC chips. */
enum
{
    CIC_NUS_6101,
    CIC_NUS_6102,
    CIC_NUS_6103,
    CIC_NUS_6105,
    CIC_NUS_6106,
    CIC_NUS_5167,
    CIC_NUS_8303,
    CIC_NUS_USDD,
    CIC_NUS_DVDD
};

/* Supported save types. */
enum
{
    EEPROM_4KB,
    EEPROM_16KB,
    SRAM,
    FLASH_RAM,
    CONTROLLER_PACK,
    NONE
};

/*ROM specific hacks */
enum
{
    NORMAL_ROM,
    GOLDEN_EYE,
    RAT_ATTACK,
    PERFECT_DARK
};

#endif /* __ROM_H__ */

