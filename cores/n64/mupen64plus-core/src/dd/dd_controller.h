/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*   Mupen64plus - dd_controller.h                                         *
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

#ifndef DD_CONTROLLER_H
#define DD_CONTROLLER_H

struct r4300_core;

#include <stdint.h>

#include "dd_disk.h"

enum dd_registers
{
    ASIC_DATA,
    ASIC_MISC_REG,
    ASIC_CMD_STATUS,
    ASIC_CUR_TK,
    ASIC_BM_STATUS_CTL,
    ASIC_ERR_SECTOR,
    ASIC_SEQ_STATUS_CTL,
    ASIC_CUR_SECTOR,
    ASIC_HARD_RESET,
    ASIC_C1_S0,
    ASIC_HOST_SECBYTE,
    ASIC_C1_S2,
    ASIC_SEC_BYTE,
    ASIC_C1_S4,
    ASIC_C1_S6,
    ASIC_CUR_ADDR,
    ASIC_ID_REG,
    ASIC_TEST_REG,
    ASIC_TEST_PIN_SEL,
    ASIC_REGS_COUNT
};

struct dd_controller
{
    uint32_t regs[ASIC_REGS_COUNT];
    uint8_t c2_buf[0x400];
    uint8_t sec_buf[0x100];
    uint8_t mseq_buf[0x40];

    struct r4300_core* r4300;
    struct dd_disk disk;
};

static uint32_t dd_reg(uint32_t address)
{
    uint32_t temp = address & 0xffff;
    if (temp >= 0x0500 && temp < 0x054c)
        temp -= 0x0500;

    return temp >> 2;
}

void init_dd(struct dd_controller* dd,
                struct r4300_core* r4300,
                uint8_t *dd_disk,
                size_t dd_disk_size);

void poweron_dd(struct dd_controller* dd);

int read_dd_regs(void* opaque, uint32_t address, uint32_t* value);
int write_dd_regs(void* opaque, uint32_t address, uint32_t value, uint32_t mask);

int dd_end_of_dma_event(struct dd_controller* dd);
void dd_pi_test();

#endif
