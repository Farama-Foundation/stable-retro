/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - pifbootrom.c                                            *
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

#include "pifbootrom.h"

#include <stdint.h>
#include <string.h>

#include "api/m64p_types.h"
#include "main/device.h"
#include "ai/ai_controller.h"
#include "pi/pi_controller.h"
#include "r4300/cp0_private.h"
#include "r4300/mi_controller.h"
#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"
#include "main/rom.h"

static unsigned int get_tv_type(void)
{
    switch(ROM_PARAMS.systemtype)
    {
       default:
       case SYSTEM_NTSC:
	  break;
       case SYSTEM_PAL:
	  return 0;
       case SYSTEM_MPAL:
	  return 2;
    }

    return 1;
}

/* Simulates end result of PIFBootROM execution */
void pifbootrom_hle_execute(struct device *dev)
{
    uint32_t bsd_dom1_config;
    unsigned int rom_type   = 0;              /* 0:Cart, 1:DD */
    unsigned int reset_type = 0;              /* 0:ColdReset, 1:NMI */
    unsigned int s7         = 0;              /* ??? */
    unsigned int tv_type    = get_tv_type();  /* 0:PAL, 1:NTSC, 2:MPAL */
    
    if ((g_ddrom != NULL) && (g_ddrom_size != 0) && (dev->pi.cart_rom.rom == NULL) && (dev->pi.cart_rom.rom_size == 0))
    {
      /* 64DD IPL */
      bsd_dom1_config = *(uint32_t*)g_ddrom;
      rom_type = 1;
    }
    else
    {
      //N64 ROM
      bsd_dom1_config = *(uint32_t*)dev->pi.cart_rom.rom;
    }

    g_cp0_regs[CP0_STATUS_REG] = 0x34000000;
    g_cp0_regs[CP0_CONFIG_REG] = 0x0006e463;

    dev->sp.regs[SP_STATUS_REG] = 1;
    dev->sp.regs2[SP_PC_REG] = 0;

    dev->pi.regs[PI_BSD_DOM1_LAT_REG] = (bsd_dom1_config      ) & 0xff;
    dev->pi.regs[PI_BSD_DOM1_PWD_REG] = (bsd_dom1_config >>  8) & 0xff;
    dev->pi.regs[PI_BSD_DOM1_PGS_REG] = (bsd_dom1_config >> 16) & 0x0f;
    dev->pi.regs[PI_BSD_DOM1_RLS_REG] = (bsd_dom1_config >> 20) & 0x03;
    dev->pi.regs[PI_STATUS_REG] = 0;

    dev->ai.regs[AI_DRAM_ADDR_REG] = 0;
    dev->ai.regs[AI_LEN_REG] = 0;

    dev->vi.regs[VI_V_INTR_REG] = 1023;
    dev->vi.regs[VI_CURRENT_REG] = 0;
    dev->vi.regs[VI_H_START_REG] = 0;

    dev->r4300.mi.regs[MI_INTR_REG] &= ~(MI_INTR_PI | MI_INTR_VI | MI_INTR_AI | MI_INTR_SP);

    if ((g_ddrom != NULL) && (g_ddrom_size != 0) && (dev->pi.cart_rom.rom == NULL) && (dev->pi.cart_rom.rom_size == 0))
    {
      //64DD IPL
      memcpy((unsigned char*)dev->sp.mem+0x40, g_ddrom+0x40, 0xfc0);
    }
    else
    {
      /* N64 ROM */
      memcpy((unsigned char*)dev->sp.mem+0x40, dev->pi.cart_rom.rom+0x40, 0xfc0);
    }

    reg[19] = rom_type;     /* s3 */
    reg[20] = tv_type;      /* s4 */
    reg[21] = reset_type;   /* s5 */
    reg[22] = dev->si.pif.cic.seed;/* s6 */
    reg[23] = s7;           /* s7 */

    /* required by CIC x105 */
    dev->sp.mem[0x1000/4] = 0x3c0dbfc0;
    dev->sp.mem[0x1004/4] = 0x8da807fc;
    dev->sp.mem[0x1008/4] = 0x25ad07c0;
    dev->sp.mem[0x100c/4] = 0x31080080;
    dev->sp.mem[0x1010/4] = 0x5500fffc;
    dev->sp.mem[0x1014/4] = 0x3c0dbfc0;
    dev->sp.mem[0x1018/4] = 0x8da80024;
    dev->sp.mem[0x101c/4] = 0x3c0bb000;

    /* required by CIC x105 */
    reg[11] = INT64_C(0xffffffffa4000040); /* t3 */
    reg[29] = INT64_C(0xffffffffa4001ff0); /* sp */
    reg[31] = INT64_C(0xffffffffa4001550); /* ra */

    /* ready to execute IPL3 */
}
