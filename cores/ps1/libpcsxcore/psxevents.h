#ifndef __PSXEVENTS_H__
#define __PSXEVENTS_H__

#include "psxcommon.h"

enum {
	PSXINT_SIO = 0,      // sioInterrupt
	PSXINT_CDR,          // cdrInterrupt
	PSXINT_CDREAD,       // cdrPlayReadInterrupt
	PSXINT_GPUDMA,       // gpuInterrupt
	PSXINT_MDECOUTDMA,   // mdec1Interrupt
	PSXINT_SPUDMA,       // spuInterrupt
	PSXINT_SPU_IRQ,      // spuDelayedIrq
	PSXINT_MDECINDMA,    // mdec0Interrupt
	PSXINT_GPUOTCDMA,    // gpuotcInterrupt
	PSXINT_CDRDMA,       // cdrDmaInterrupt
	PSXINT_NEWDRC_CHECK, // (none)
	PSXINT_RCNT,         // psxRcntUpdate
	PSXINT_CDRLID,       // cdrLidSeekInterrupt
	PSXINT_IRQ10,        // irq10Interrupt
	PSXINT_SPU_UPDATE,   // spuUpdate
	PSXINT_COUNT
};

#define set_event_raw_abs(e, abs) { \
	u32 abs_ = abs; \
	s32 di_ = psxRegs.next_interupt - abs_; \
	psxRegs.event_cycles[e] = abs_; \
	if (di_ > 0) { \
		/*printf("%u: next_interupt %u -> %u\n", psxRegs.cycle, psxRegs.next_interupt, abs_);*/ \
		psxRegs.next_interupt = abs_; \
	} \
}

#define set_event(e, c) do { \
	psxRegs.interrupt |= (1 << (e)); \
	psxRegs.intCycle[e].cycle = c; \
	psxRegs.intCycle[e].sCycle = psxRegs.cycle; \
	set_event_raw_abs(e, psxRegs.cycle + (c)) \
} while (0)

union psxCP0Regs_;
struct psxRegisters;

u32  schedule_timeslice(struct psxRegisters *regs);
void irq_test(union psxCP0Regs_ *cp0);
void gen_interupt(union psxCP0Regs_ *cp0);
void events_restore(void);

#endif // __PSXEVENTS_H__
