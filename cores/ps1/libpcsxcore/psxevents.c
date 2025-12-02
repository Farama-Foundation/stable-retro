#include <stddef.h>
#include <stdio.h>
#include "r3000a.h"
#include "cdrom.h"
#include "psxdma.h"
#include "mdec.h"
#include "psxevents.h"

//#define evprintf printf
#define evprintf(...)

static psxRegisters *cp0TOpsxRegs(psxCP0Regs *cp0)
{
#ifndef LIGHTREC
	return (void *)((char *)cp0 - offsetof(psxRegisters, CP0));
#else
	// lightrec has it's own cp0
	return &psxRegs;
#endif
}

u32 schedule_timeslice(psxRegisters *regs)
{
	u32 i, c = regs->cycle;
	u32 irqs = regs->interrupt;
	s32 min, dif;

	min = PSXCLK;
	for (i = 0; irqs != 0; i++, irqs >>= 1) {
		if (!(irqs & 1))
			continue;
		dif = regs->event_cycles[i] - c;
		//evprintf("  ev %d\n", dif);
		if (0 < dif && dif < min)
			min = dif;
	}
	regs->next_interupt = c + min;
	return regs->next_interupt;
}

static void irqNoOp() {
}

typedef void (irq_func)();

static irq_func * const irq_funcs[] = {
	[PSXINT_SIO]	= sioInterrupt,
	[PSXINT_CDR]	= cdrInterrupt,
	[PSXINT_CDREAD]	= cdrPlayReadInterrupt,
	[PSXINT_GPUDMA]	= gpuInterrupt,
	[PSXINT_MDECOUTDMA] = mdec1Interrupt,
	[PSXINT_SPUDMA]	= spuInterrupt,
	[PSXINT_MDECINDMA] = mdec0Interrupt,
	[PSXINT_GPUOTCDMA] = gpuotcInterrupt,
	[PSXINT_CDRDMA] = cdrDmaInterrupt,
	[PSXINT_NEWDRC_CHECK] = irqNoOp,
	[PSXINT_CDRLID] = cdrLidSeekInterrupt,
	[PSXINT_IRQ10] = irq10Interrupt,
	[PSXINT_SPU_UPDATE] = spuUpdate,
	[PSXINT_SPU_IRQ] = spuDelayedIrq,
	[PSXINT_RCNT] = psxRcntUpdate,
};

void irq_test(psxCP0Regs *cp0)
{
	psxRegisters *regs = cp0TOpsxRegs(cp0);
	u32 cycle = regs->cycle;
	u32 irq, irq_bits;

	for (irq = 0, irq_bits = regs->interrupt; irq_bits != 0; irq++, irq_bits >>= 1) {
		if (!(irq_bits & 1))
			continue;
		if ((s32)(cycle - regs->event_cycles[irq]) >= 0) {
			// note: irq_funcs() also modify regs->interrupt
			regs->interrupt &= ~(1u << irq);
			irq_funcs[irq]();
		}
	}

	cp0->n.Cause &= ~0x400;
	if (psxHu32(0x1070) & psxHu32(0x1074))
		cp0->n.Cause |= 0x400;
	if (((cp0->n.Cause | 1) & cp0->n.SR & 0x401) == 0x401)
		psxException(0, 0, cp0);
}

void gen_interupt(psxCP0Regs *cp0)
{
	psxRegisters *regs = cp0TOpsxRegs(cp0);

	evprintf("  +ge %08x, %u->%u (%d)\n", regs->pc, regs->cycle,
		regs->next_interupt, regs->next_interupt - regs->cycle);

	irq_test(cp0);
	schedule_timeslice(regs);

	evprintf("  -ge %08x, %u->%u (%d)\n", regs->pc, regs->cycle,
		regs->next_interupt, regs->next_interupt - regs->cycle);
}

void events_restore(void)
{
	int i;
	for (i = 0; i < PSXINT_COUNT; i++)
		psxRegs.event_cycles[i] = psxRegs.intCycle[i].sCycle + psxRegs.intCycle[i].cycle;

	psxRegs.event_cycles[PSXINT_RCNT] = psxRegs.psxNextsCounter + psxRegs.psxNextCounter;
	psxRegs.interrupt |=  1 << PSXINT_RCNT;
	psxRegs.interrupt &= (1 << PSXINT_COUNT) - 1;
}
