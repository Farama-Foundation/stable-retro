/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef __PSXHW_H__
#define __PSXHW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "sio.h"
#include "psxcounters.h"

#define HW_DMA0_MADR (psxHu32ref(0x1080)) // MDEC in DMA
#define HW_DMA0_BCR  (psxHu32ref(0x1084))
#define HW_DMA0_CHCR (psxHu32ref(0x1088))

#define HW_DMA1_MADR (psxHu32ref(0x1090)) // MDEC out DMA
#define HW_DMA1_BCR  (psxHu32ref(0x1094))
#define HW_DMA1_CHCR (psxHu32ref(0x1098))

#define HW_DMA2_MADR (psxHu32ref(0x10a0)) // GPU DMA
#define HW_DMA2_BCR  (psxHu32ref(0x10a4))
#define HW_DMA2_CHCR (psxHu32ref(0x10a8))

#define HW_DMA3_MADR (psxHu32ref(0x10b0)) // CDROM DMA
#define HW_DMA3_BCR  (psxHu32ref(0x10b4))
#define HW_DMA3_CHCR (psxHu32ref(0x10b8))

#define HW_DMA4_MADR (psxHu32ref(0x10c0)) // SPU DMA
#define HW_DMA4_BCR  (psxHu32ref(0x10c4))
#define HW_DMA4_CHCR (psxHu32ref(0x10c8))

#define HW_DMA6_MADR (psxHu32ref(0x10e0)) // GPU DMA (OT)
#define HW_DMA6_BCR  (psxHu32ref(0x10e4))
#define HW_DMA6_CHCR (psxHu32ref(0x10e8))

#define HW_DMA_PCR   (psxHu32ref(0x10f0))
#define HW_DMA_ICR   (psxHu32ref(0x10f4))

#define HW_DMA_ICR_BUS_ERROR     (1u << 15)
#define HW_DMA_ICR_GLOBAL_ENABLE (1u << 23)
#define HW_DMA_ICR_IRQ_SENT      (1u << 31)

#define DMA_INTERRUPT(n) { \
	u32 icr = SWAPu32(HW_DMA_ICR); \
	if (icr & (1u << (16 + n))) { \
		icr |= 1u << (24 + n); \
		if (icr & HW_DMA_ICR_GLOBAL_ENABLE && !(icr & HW_DMA_ICR_IRQ_SENT)) { \
			psxHu32ref(0x1070) |= SWAP32(8); \
			icr |= HW_DMA_ICR_IRQ_SENT; \
		} \
		HW_DMA_ICR = SWAP32(icr); \
	} \
}

void psxHwReset();
u8 psxHwRead8(u32 add);
u16 psxHwRead16(u32 add);
u32 psxHwRead32(u32 add);
void psxHwWrite8(u32 add, u32 value);
void psxHwWrite16(u32 add, u32 value);
void psxHwWrite32(u32 add, u32 value);
u32 sio1ReadStat16(void);
int psxHwFreeze(void *f, int Mode);

void psxHwWriteIstat(u32 value);
void psxHwWriteImask(u32 value);
void psxHwWriteChcr0(u32 value);
void psxHwWriteChcr1(u32 value);
void psxHwWriteChcr2(u32 value);
void psxHwWriteChcr3(u32 value);
void psxHwWriteChcr4(u32 value);
void psxHwWriteChcr6(u32 value);
void psxHwWriteDmaPcr32(u32 value);
void psxHwWriteDmaIcr32(u32 value);
void psxHwWriteGpuSR(u32 value);
u32  psxHwReadGpuSR(void);

#ifdef __cplusplus
}
#endif
#endif
