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
* R3000A CPU functions.
*/

#include "r3000a.h"
#include "cdrom.h"
#include "mdec.h"
#include "gte.h"
#include "psxinterpreter.h"
#include "psxbios.h"
#include "psxevents.h"
#include "../include/compiler_features.h"
#include <assert.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

R3000Acpu *psxCpu = NULL;
#ifdef DRC_DISABLE
psxRegisters psxRegs;
#endif

int psxInit() {
	assert(PSXINT_COUNT <= ARRAY_SIZE(psxRegs.intCycle));
	assert(ARRAY_SIZE(psxRegs.intCycle) == ARRAY_SIZE(psxRegs.event_cycles));

#ifndef DRC_DISABLE
	if (Config.Cpu == CPU_INTERPRETER) {
		psxCpu = &psxInt;
	} else psxCpu = &psxRec;
#else
	Config.Cpu = CPU_INTERPRETER;
	psxCpu = &psxInt;
#endif

	Log = 0;

	if (psxMemInit() == -1) return -1;

	return psxCpu->Init();
}

void psxReset() {
	boolean introBypassed = FALSE;
	boolean oldhle = Config.HLE;

	psxMemReset();

	memset(&psxRegs, 0, sizeof(psxRegs));

	psxRegs.pc = 0xbfc00000; // Start in bootstrap

	psxRegs.CP0.n.SR   = 0x10600000; // COP0 enabled | BEV = 1 | TS = 1
	psxRegs.CP0.n.PRid = 0x00000002; // PRevID = Revision ID, same as R3000A
	if (Config.HLE) {
		psxRegs.CP0.n.SR |= 1u << 30;    // COP2 enabled
		psxRegs.CP0.n.SR &= ~(1u << 22); // RAM exception vector
	}

	if (Config.HLE != oldhle) {
		// at least ari64 drc compiles differently so hard reset
		psxCpu->Shutdown();
		psxCpu->Init();
	}
	psxCpu->ApplyConfig();
	psxCpu->Reset();

	psxHwReset();
	psxBiosInit();

	if (!Config.HLE) {
		psxExecuteBios();
		if (psxRegs.pc == 0x80030000 && !Config.SlowBoot) {
			introBypassed = BiosBootBypass();
		}
	}
	if (Config.HLE || introBypassed)
		psxBiosSetupBootState();

#ifdef EMU_LOG
	EMU_LOG("*BIOS END*\n");
#endif
	Log = 0;
}

void psxShutdown() {
	psxBiosShutdown();

	psxCpu->Shutdown();

	psxMemShutdown();
}

// cp0 is passed separately for lightrec to be less messy
void psxException(u32 cause, enum R3000Abdt bdt, psxCP0Regs *cp0) {
	u32 opcode = intFakeFetch(psxRegs.pc);
	
	if (unlikely(!Config.HLE && (opcode >> 25) == 0x25)) {
		// "hokuto no ken" / "Crash Bandicot 2" ...
		// BIOS does not allow to return to GTE instructions
		// (just skips it, supposedly because it's scheduled already)
		// so we execute it here
		psxCP2Regs *cp2 = (psxCP2Regs *)(cp0 + 1);
		psxRegs.code = opcode;
		psxCP2[opcode & 0x3f](cp2);
	}

	// Set the Cause
	cp0->n.Cause = (bdt << 30) | (cp0->n.Cause & 0x700) | cause;

	// Set the EPC & PC
	cp0->n.EPC = bdt ? psxRegs.pc - 4 : psxRegs.pc;

	if (cp0->n.SR & 0x400000)
		psxRegs.pc = 0xbfc00180;
	else
		psxRegs.pc = 0x80000080;

	// Set the SR
	cp0->n.SR = (cp0->n.SR & ~0x3f) | ((cp0->n.SR & 0x0f) << 2);
}

void psxBranchTest() {
	if ((psxRegs.cycle - psxRegs.psxNextsCounter) >= psxRegs.psxNextCounter)
		psxRcntUpdate();

	irq_test(&psxRegs.CP0);

	if (unlikely(psxRegs.pc == psxRegs.biosBranchCheck))
		psxBiosCheckBranch();
}

void psxJumpTest() {
	if (!Config.HLE && Config.PsxOut) {
		u32 call = psxRegs.GPR.n.t1 & 0xff;
		switch (psxRegs.pc & 0x1fffff) {
			case 0xa0:
#ifdef PSXBIOS_LOG
				if (call != 0x28 && call != 0xe) {
					PSXBIOS_LOG("Bios call a0: %s (%x) %x,%x,%x,%x\n", biosA0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3); }
#endif
				if (biosA0[call])
					biosA0[call]();
				break;
			case 0xb0:
#ifdef PSXBIOS_LOG
				if (call != 0x17 && call != 0xb) {
					PSXBIOS_LOG("Bios call b0: %s (%x) %x,%x,%x,%x\n", biosB0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3); }
#endif
				if (biosB0[call])
					biosB0[call]();
				break;
			case 0xc0:
#ifdef PSXBIOS_LOG
				PSXBIOS_LOG("Bios call c0: %s (%x) %x,%x,%x,%x\n", biosC0n[call], call, psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
#endif
				if (biosC0[call])
					biosC0[call]();
				break;
		}
	}
}

int psxExecuteBiosEnded(void) {
	return (psxRegs.pc & 0xff800000) == 0x80000000;
}

void psxExecuteBios() {
	int i;
	for (i = 0; i < 5000000; i++) {
		psxCpu->ExecuteBlock(&psxRegs, EXEC_CALLER_BOOT);
		if (psxExecuteBiosEnded())
			break;
	}
	if (psxRegs.pc != 0x80030000)
		SysPrintf("non-standard BIOS detected (%d, %08x)\n", i, psxRegs.pc);
}

// irq10 stuff, very preliminary
static int irq10count;

static void psxScheduleIrq10One(u32 cycles_abs) {
	// schedule relative to frame start
	u32 c = cycles_abs - rcnts[3].cycleStart;
	assert((s32)c >= 0);
	psxRegs.interrupt |= 1 << PSXINT_IRQ10;
	psxRegs.intCycle[PSXINT_IRQ10].cycle = c;
	psxRegs.intCycle[PSXINT_IRQ10].sCycle = rcnts[3].cycleStart;
	set_event_raw_abs(PSXINT_IRQ10, cycles_abs);
}

void irq10Interrupt() {
	u32 prevc = psxRegs.intCycle[PSXINT_IRQ10].sCycle
		+ psxRegs.intCycle[PSXINT_IRQ10].cycle;

	psxHu32ref(0x1070) |= SWAPu32(0x400);

#if 0
	s32 framec = psxRegs.cycle - rcnts[3].cycleStart;
	printf("%d:%03d irq10 #%d %3d m=%d,%d\n", frame_counter,
		(s32)((float)framec / (PSXCLK / 60 / 263.0f)),
		irq10count, psxRegs.cycle - prevc,
		(psxRegs.CP0.n.SR & 0x401) != 0x401, !(psxHu32(0x1074) & 0x400));
#endif
	if (--irq10count > 0) {
		u32 cycles_per_line = Config.PsxType
			? PSXCLK / 50 / 314 : PSXCLK / 60 / 263;
		psxScheduleIrq10One(prevc + cycles_per_line);
	}
}

void psxScheduleIrq10(int irq_count, int x_cycles, int y) {
	//printf("%s %d, %d, %d\n", __func__, irq_count, x_cycles, y);
	u32 cycles_per_frame = Config.PsxType ? PSXCLK / 50 : PSXCLK / 60;
	u32 cycles = rcnts[3].cycleStart + cycles_per_frame;
	cycles += y * cycles_per_frame / (Config.PsxType ? 314 : 263);
	cycles += x_cycles;
	psxScheduleIrq10One(cycles);
	irq10count = irq_count;
}
