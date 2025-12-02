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

/*
* Functions for PSX hardware control.
*/

#include "psxhw.h"
#include "psxevents.h"
#include "mdec.h"
#include "cdrom.h"
#include "gpu.h"
#include "../include/compiler_features.h"

void psxHwReset() {
	memset(psxH, 0, 0x10000);

	mdecInit(); // initialize mdec decoder
	cdrReset();
	psxRcntInit();
	HW_GPU_STATUS = SWAP32(0x10802000);
}

void psxHwWriteIstat(u32 value)
{
	u32 stat = psxHu16(0x1070) & value;
	psxHu16ref(0x1070) = SWAPu16(stat);

	psxRegs.CP0.n.Cause &= ~0x400;
	if (stat & psxHu16(0x1074))
		psxRegs.CP0.n.Cause |= 0x400;
}

void psxHwWriteImask(u32 value)
{
	u32 stat = psxHu16(0x1070);
	psxHu16ref(0x1074) = SWAPu16(value);
	if (stat & value) {
		//if ((psxRegs.CP0.n.SR & 0x401) == 0x401)
		//	log_unhandled("irq on unmask @%08x\n", psxRegs.pc);
		set_event(PSXINT_NEWDRC_CHECK, 1);
	}
	psxRegs.CP0.n.Cause &= ~0x400;
	if (stat & value)
		psxRegs.CP0.n.Cause |= 0x400;
}

#define make_dma_func(n, abort_func) \
void psxHwWriteChcr##n(u32 value) \
{ \
	u32 old = SWAPu32(HW_DMA##n##_CHCR); \
	if (n == 6) { value &= 0x51000002; value |= 2; } \
	else        { value &= 0x71770703; } \
	if (value == old) \
		return; \
	if (old & 0x01000000) \
		log_unhandled("%u dma" #n " %08x -> %08x\n", psxRegs.cycle, old, value); \
	HW_DMA##n##_CHCR = SWAPu32(value); \
	if ((value ^ old) & 0x01000000) { \
		if (!(value & 0x01000000)) \
			abort_func; \
		else if (HW_DMA_PCR & SWAPu32(8u << (n * 4))) \
			psxDma##n(SWAPu32(HW_DMA##n##_MADR), SWAPu32(HW_DMA##n##_BCR), value); \
	} \
}

make_dma_func(0,)
make_dma_func(1,)
make_dma_func(2, psxAbortDma2())
make_dma_func(3,)
make_dma_func(4,)
make_dma_func(6,)

void psxHwWriteDmaPcr32(u32 value)
{
	// todo: can this also pause/stop live dma?
	u32 on = (SWAPu32(HW_DMA_PCR) ^ value) & value & 0x08888888;
	u32 chcr;
	HW_DMA_PCR = SWAPu32(value);
	if (likely(!on))
		return;
	#define DO(n) \
	chcr = SWAPu32(HW_DMA##n##_CHCR); \
	if ((on & (8u << 4*n)) && (chcr & 0x01000000)) \
		psxDma##n(SWAPu32(HW_DMA##n##_MADR), SWAPu32(HW_DMA##n##_BCR), chcr)
	DO(0);
	DO(1);
	// breaks Kyuutenkai. Probably needs better timing or
	// proper gpu side dma enable handling
	//DO(2);
	if ((on & (8u << 4*2)) && (SWAPu32(HW_DMA2_CHCR) & 0x01000000))
		log_unhandled("dma2 pcr write ignored\n");
	DO(3);
	DO(4);
	DO(6);
	#undef DO
}

void psxHwWriteDmaIcr32(u32 value)
{
	u32 tmp = value & 0x00ff803f;
	tmp |= (SWAPu32(HW_DMA_ICR) & ~value) & 0x7f000000;
	if ((tmp & HW_DMA_ICR_GLOBAL_ENABLE && tmp & 0x7f000000)
	    || tmp & HW_DMA_ICR_BUS_ERROR) {
		if (!(SWAPu32(HW_DMA_ICR) & HW_DMA_ICR_IRQ_SENT))
			psxHu32ref(0x1070) |= SWAP32(8);
		tmp |= HW_DMA_ICR_IRQ_SENT;
	}
	HW_DMA_ICR = SWAPu32(tmp);
}

void psxHwWriteGpuSR(u32 value)
{
	u32 old_sr = HW_GPU_STATUS, new_sr;
	GPU_writeStatus(value);
	gpuSyncPluginSR();
	new_sr = HW_GPU_STATUS;
	// "The Next Tetris" seems to rely on the field order after enable
	if ((old_sr ^ new_sr) & new_sr & SWAP32(PSXGPU_ILACE))
		frame_counter |= 1;
}

u32 psxHwReadGpuSR(void)
{
	u32 v, c = psxRegs.cycle;

	// meh2, syncing for img bit, might want to avoid it..
	gpuSyncPluginSR();
	v = SWAP32(HW_GPU_STATUS);
	v |= ((s32)(psxRegs.gpuIdleAfter - c) >> 31) & PSXGPU_nBUSY;

	// XXX: because of large timeslices can't use hSyncCount, using rough
	// approximization instead. Perhaps better use hcounter code here or something.
	if (hSyncCount < 240 && (v & PSXGPU_ILACE_BITS) != PSXGPU_ILACE_BITS)
		v |= PSXGPU_LCF & (c << 20);
	return v;
}

u32 sio1ReadStat16(void)
{
	// Armored Core, F1 Link cable misdetection
	return 0xa0;
}

u8 psxHwRead8(u32 add) {
	u8 hard;

	switch (add & 0xffff) {
	case 0x1040: hard = sioRead8(); break;
	case 0x1800: hard = cdrRead0(); break;
	case 0x1801: hard = cdrRead1(); break;
	case 0x1802: hard = cdrRead2(); break;
	case 0x1803: hard = cdrRead3(); break;

	case 0x1041: case 0x1042: case 0x1043:
	case 0x1044: case 0x1045:
	case 0x1046: case 0x1047:
	case 0x1048: case 0x1049:
	case 0x104a: case 0x104b:
	case 0x104c: case 0x104d:
	case 0x104e: case 0x104f:
	case 0x1050: case 0x1051:
	case 0x1054: case 0x1055:
	case 0x1058: case 0x1059:
	case 0x105a: case 0x105b:
	case 0x105c: case 0x105d:
	case 0x1100: case 0x1101:
	case 0x1104: case 0x1105:
	case 0x1108: case 0x1109:
	case 0x1110: case 0x1111:
	case 0x1114: case 0x1115:
	case 0x1118: case 0x1119:
	case 0x1120: case 0x1121:
	case 0x1124: case 0x1125:
	case 0x1128: case 0x1129:
	case 0x1810: case 0x1811:
	case 0x1812: case 0x1813:
	case 0x1814: case 0x1815:
	case 0x1816: case 0x1817:
	case 0x1820: case 0x1821:
	case 0x1822: case 0x1823:
	case 0x1824: case 0x1825:
	case 0x1826: case 0x1827:
		log_unhandled("unhandled r8  %08x @%08x\n", add, psxRegs.pc);
		// falthrough
	default:
		if (0x1f801c00 <= add && add < 0x1f802000) {
			u16 val = SPU_readRegister(add & ~1, psxRegs.cycle);
			hard = (add & 1) ? val >> 8 : val;
			break;
		}
		hard = psxHu8(add);
	}

	//printf("r8  %08x       %02x @%08x\n", add, hard, psxRegs.pc);
	return hard;
}

u16 psxHwRead16(u32 add) {
	unsigned short hard;

	switch (add & 0xffff) {
	case 0x1040: hard = sioRead8(); break;
	case 0x1044: hard = sioReadStat16(); break;
	case 0x1048: hard = sioReadMode16(); break;
	case 0x104a: hard = sioReadCtrl16(); break;
	case 0x104e: hard = sioReadBaud16(); break;
	case 0x1054: hard = sio1ReadStat16(); break;
	case 0x1100: hard = psxRcntRcount0(); break;
	case 0x1104: hard = psxRcntRmode(0); break;
	case 0x1108: hard = psxRcntRtarget(0); break;
	case 0x1110: hard = psxRcntRcount1(); break;
	case 0x1114: hard = psxRcntRmode(1); break;
	case 0x1118: hard = psxRcntRtarget(1); break;
	case 0x1120: hard = psxRcntRcount2(); break;
	case 0x1124: hard = psxRcntRmode(2); break;
	case 0x1128: hard = psxRcntRtarget(2); break;

	case 0x1042:
	case 0x1046:
	case 0x104c:
	case 0x1050:
	case 0x1058:
	case 0x105a:
	case 0x105c:
	case 0x1800:
	case 0x1802:
	case 0x1810:
	case 0x1812:
	case 0x1814:
	case 0x1816:
	case 0x1820:
	case 0x1822:
	case 0x1824:
	case 0x1826:
		log_unhandled("unhandled r16 %08x @%08x\n", add, psxRegs.pc);
		// falthrough
	default:
		if (0x1f801c00 <= add && add < 0x1f802000) {
			hard = SPU_readRegister(add, psxRegs.cycle);
			break;
		}
		hard = psxHu16(add);
	}
	
	//printf("r16 %08x     %04x @%08x\n", add, hard, psxRegs.pc);
	return hard;
}

u32 psxHwRead32(u32 add) {
	u32 hard;

	switch (add & 0xffff) {
	case 0x1040: hard = sioRead8(); break;
	case 0x1044: hard = sioReadStat16(); break;
	case 0x1100: hard = psxRcntRcount0(); break;
	case 0x1104: hard = psxRcntRmode(0); break;
	case 0x1108: hard = psxRcntRtarget(0); break;
	case 0x1110: hard = psxRcntRcount1(); break;
	case 0x1114: hard = psxRcntRmode(1); break;
	case 0x1118: hard = psxRcntRtarget(1); break;
	case 0x1120: hard = psxRcntRcount2(); break;
	case 0x1124: hard = psxRcntRmode(2); break;
	case 0x1128: hard = psxRcntRtarget(2); break;
	case 0x1810: hard = GPU_readData(); break;
	case 0x1814: hard = psxHwReadGpuSR(); break;
	case 0x1820: hard = mdecRead0(); break;
	case 0x1824: hard = mdecRead1(); break;

	case 0x1048:
	case 0x104c:
	case 0x1050:
	case 0x1054:
	case 0x1058:
	case 0x105c:
	case 0x1800:
		log_unhandled("unhandled r32 %08x @%08x\n", add, psxRegs.pc);
		// falthrough
	default:
		if (0x1f801c00 <= add && add < 0x1f802000) {
			hard = SPU_readRegister(add, psxRegs.cycle);
			hard |= SPU_readRegister(add + 2, psxRegs.cycle) << 16;
			break;
		}
		hard = psxHu32(add);
	}
	//printf("r32 %08x %08x @%08x\n", add, hard, psxRegs.pc);
	return hard;
}

void psxHwWrite8(u32 add, u32 value) {
	switch (add & 0xffff) {
	case 0x1040: sioWrite8(value); return;
	case 0x10f6:
		// nocash documents it as forced w32, but still games use this?
		break;
	case 0x1800: cdrWrite0(value); return;
	case 0x1801: cdrWrite1(value); return;
	case 0x1802: cdrWrite2(value); return;
	case 0x1803: cdrWrite3(value); return;
	case 0x2041: break; // "POST (external 7 segment display)"

	default:
		if (0x1f801c00 <= add && add < 0x1f802000) {
			log_unhandled("spu w8 %02x @%08x\n", value, psxRegs.pc);
			if (!(add & 1))
				SPU_writeRegister(add, value, psxRegs.cycle);
			return;
		}
		else
			log_unhandled("unhandled w8  %08x %08x @%08x\n",
				add, value, psxRegs.pc);
	}
	psxHu8(add) = value;
}

void psxHwWrite16(u32 add, u32 value) {
	switch (add & 0xffff) {
	case 0x1040: sioWrite8(value); return;
	case 0x1044: sioWriteStat16(value); return;
	case 0x1048: sioWriteMode16(value); return;
	case 0x104a: sioWriteCtrl16(value); return;
	case 0x104e: sioWriteBaud16(value); return;
	case 0x1070: psxHwWriteIstat(value); return;
	case 0x1074: psxHwWriteImask(value); return;
	case 0x1100: psxRcntWcount(0, value); return;
	case 0x1104: psxRcntWmode(0, value); return;
	case 0x1108: psxRcntWtarget(0, value); return;
	case 0x1110: psxRcntWcount(1, value); return;
	case 0x1114: psxRcntWmode(1, value); return;
	case 0x1118: psxRcntWtarget(1, value); return;
	case 0x1120: psxRcntWcount(2, value); return;
	case 0x1124: psxRcntWmode(2, value); return;
	case 0x1128: psxRcntWtarget(2, value); return;

	// forced write32:
	case 0x1088: // DMA0 chcr (MDEC in DMA)
	case 0x108c: psxHwWriteChcr0(value); return;
	case 0x1098: // DMA1 chcr (MDEC out DMA)
	case 0x109c: psxHwWriteChcr1(value); return;
	case 0x10a8: // DMA2 chcr (GPU DMA)
	case 0x10ac: psxHwWriteChcr2(value); return;
	case 0x10b8: // DMA3 chcr (CDROM DMA)
	case 0x10bc: psxHwWriteChcr3(value); return;
	case 0x10c8: // DMA4 chcr (SPU DMA)
	case 0x10cc: psxHwWriteChcr4(value); return;
	case 0x10e8: // DMA6 chcr (OT clear)
	case 0x10ec: psxHwWriteChcr6(value); return;
	case 0x10f0: psxHwWriteDmaPcr32(value); return;
	case 0x10f4: psxHwWriteDmaIcr32(value); return;

	// forced write32 with no immediate effect:
	case 0x1014:
	case 0x1060:
	case 0x1080:
	case 0x1090:
	case 0x10a0:
	case 0x10b0:
	case 0x10c0:
	case 0x10d0:
	case 0x10e0:
		psxHu32ref(add) = SWAPu32(value);
		return;

	case 0x1800:
	case 0x1802:
	case 0x1810:
	case 0x1812:
	case 0x1814:
	case 0x1816:
	case 0x1820:
	case 0x1822:
	case 0x1824:
	case 0x1826:
		log_unhandled("unhandled w16 %08x @%08x\n", add, psxRegs.pc);
		break;

	default:
		if (0x1f801c00 <= add && add < 0x1f802000) {
			SPU_writeRegister(add, value, psxRegs.cycle);
			return;
		}
		else if (0x1f801000 <= add && add < 0x1f801800)
			log_unhandled("unhandled w16 %08x %08x @%08x\n",
				add, value, psxRegs.pc);
	}
	psxHu16ref(add) = SWAPu16(value);
}

void psxHwWrite32(u32 add, u32 value) {
	switch (add & 0xffff) {
	case 0x1040: sioWrite8(value); return;
	case 0x1070: psxHwWriteIstat(value); return;
	case 0x1074: psxHwWriteImask(value); return;
	case 0x1088: // DMA0 chcr (MDEC in DMA)
	case 0x108c: psxHwWriteChcr0(value); return;
	case 0x1098: // DMA1 chcr (MDEC out DMA)
	case 0x109c: psxHwWriteChcr1(value); return;
	case 0x10a8: // DMA2 chcr (GPU DMA)
	case 0x10ac: psxHwWriteChcr2(value); return;
	case 0x10b8: // DMA3 chcr (CDROM DMA)
	case 0x10bc: psxHwWriteChcr3(value); return;
	case 0x10c8: // DMA4 chcr (SPU DMA)
	case 0x10cc: psxHwWriteChcr4(value); return;
	case 0x10e8: // DMA6 chcr (OT clear)
	case 0x10ec: psxHwWriteChcr6(value); return;
	case 0x10f0: psxHwWriteDmaPcr32(value); return;
	case 0x10f4: psxHwWriteDmaIcr32(value); return;

	case 0x1810: GPU_writeData(value); return;
	case 0x1814: psxHwWriteGpuSR(value); return;
	case 0x1820: mdecWrite0(value); break;
	case 0x1824: mdecWrite1(value); break;

	case 0x1100: psxRcntWcount(0, value & 0xffff); return;
	case 0x1104: psxRcntWmode(0, value); return;
	case 0x1108: psxRcntWtarget(0, value & 0xffff); return;
	case 0x1110: psxRcntWcount(1, value & 0xffff); return;
	case 0x1114: psxRcntWmode(1, value); return;
	case 0x1118: psxRcntWtarget(1, value & 0xffff); return;
	case 0x1120: psxRcntWcount(2, value & 0xffff); return;
	case 0x1124: psxRcntWmode(2, value); return;
	case 0x1128: psxRcntWtarget(2, value & 0xffff); return;

	case 0x1044:
	case 0x1048:
	case 0x104c:
	case 0x1050:
	case 0x1054:
	case 0x1058:
	case 0x105c:
	case 0x1800:
		log_unhandled("unhandled w32 %08x %08x @%08x\n", add, value, psxRegs.pc);
		break;

	default:
		if (0x1f801c00 <= add && add < 0x1f802000) {
			SPU_writeRegister(add, value&0xffff, psxRegs.cycle);
			SPU_writeRegister(add + 2, value>>16, psxRegs.cycle);
			return;
		}
	}
	psxHu32ref(add) = SWAPu32(value);
}

int psxHwFreeze(void *f, int Mode) {
	return 0;
}
