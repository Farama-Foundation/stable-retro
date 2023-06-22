/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - transferpak.c                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
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

#include "transferpak.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "gb/gb_cart.h"

#include <string.h>


static uint16_t gb_cart_address(unsigned int bank, uint16_t address)
{
    return (address & 0x3fff) | ((bank & 0x3) * 0x4000) ;
}


int init_transferpak(struct transferpak* tpk, uint8_t *rom, size_t rom_size)
{
    memset(tpk, 0, sizeof(*tpk));

    tpk->access_mode = (init_gb_cart(&tpk->gb_cart, rom, rom_size) != 0)
        ? CART_NOT_INSERTED
        : CART_ACCESS_MODE_0;
    tpk->access_mode_changed = 0x44;

    return 0;
}

void release_transferpak(struct transferpak* tpk)
{
    release_gb_cart(&tpk->gb_cart);
}

void transferpak_read_command(struct transferpak* tpk, uint16_t address, uint8_t* data, size_t size)
{
    uint8_t value;

    DebugMessage(M64MSG_WARNING, "tpak read: %04x", address);

    switch(address >> 12)
    {
    case 0x8:
        /* get gb cart state (enabled/disabled) */
        value = (tpk->enabled)
              ? 0x84
              : 0x00;

        DebugMessage(M64MSG_WARNING, "tpak get cart state: %02x", value);
        memset(data, value, size);
        break;

    case 0xb:
        /* get gb cart access mode */
        if (tpk->enabled)
        {
            DebugMessage(M64MSG_WARNING, "tpak get access mode: %02x", tpk->access_mode);
            memset(data, tpk->access_mode, size);
            if (tpk->access_mode != CART_NOT_INSERTED)
            {
                data[0] |= tpk->access_mode_changed;
            }
            tpk->access_mode_changed = 0;
        }
        break;

    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        /* read gb cart */
        if (tpk->enabled)
        {
            DebugMessage(M64MSG_WARNING, "tpak read cart: %04x", address);
            read_gb_cart(&tpk->gb_cart, gb_cart_address(tpk->bank, address), data);
        }
        break;

    default:
        DebugMessage(M64MSG_WARNING, "Unknown tpak read: %04x", address);
    }
}

void transferpak_write_command(struct transferpak* tpk, uint16_t address, const uint8_t* data, size_t size)
{
    DebugMessage(M64MSG_WARNING, "tpak write: %04x <- %02x", address, *data);

    switch(address >> 12)
    {
    case 0x8:
        /* enable / disable gb cart */
        switch(*data)
        {
        case 0xfe:
            tpk->enabled = 0;
            DebugMessage(M64MSG_WARNING, "tpak disabled");
            break;
        case 0x84:
            tpk->enabled = 1;
            DebugMessage(M64MSG_WARNING, "tpak enabled");
            break;
        default:
            DebugMessage(M64MSG_WARNING, "Unknown tpak write: %04x <- %02x", address, *data);
        }
        break;

    case 0xa:
        /* set gb cart bank */
        if (tpk->enabled)
        {
            tpk->bank = *data;
            DebugMessage(M64MSG_WARNING, "tpak set bank %02x", tpk->bank);
        }
        break;

    case 0xb:
        /* set gb cart access mode */
        if (tpk->enabled)
        {
            tpk->access_mode_changed = 0x04;

            tpk->access_mode = ((*data & 1) == 0)
                              ? CART_ACCESS_MODE_0
                              : CART_ACCESS_MODE_1;

            if ((*data & 0xfe) != 0)
            {
                DebugMessage(M64MSG_WARNING, "Unknwon tpak write: %04x <- %02x", address, *data);
            }

            DebugMessage(M64MSG_WARNING, "tpak set access mode %02x", tpk->access_mode);
        }
        break;

    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        /* write gb cart */
//        if (tpk->enabled)
        {
            DebugMessage(M64MSG_WARNING, "tpak write gb: %04x <- %02x", address, *data);
            write_gb_cart(&tpk->gb_cart, gb_cart_address(tpk->bank, address), data);
        }
        break;
    default:
        DebugMessage(M64MSG_WARNING, "Unknown tpak write: %04x <- %02x", address, *data);
    }
}
