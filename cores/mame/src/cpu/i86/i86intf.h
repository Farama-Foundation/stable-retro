/* ASG 971222 -- rewrote this interface */
#ifndef __I86INTRF_H_
#define __I86INTRF_H_

#include "memory.h"
#include "osd_cpu.h"

enum {
	I86_IP=1, I86_AX, I86_CX, I86_DX, I86_BX, I86_SP, I86_BP, I86_SI, I86_DI,
	I86_FLAGS, I86_ES, I86_CS, I86_SS, I86_DS,
	I86_VECTOR, I86_PENDING, I86_NMI_STATE, I86_IRQ_STATE
};

/* Public variables */
extern int i86_ICount;

/* Public functions */

extern void i86_init(void);
extern void i86_reset(void *param);
extern void i86_exit(void);
extern int i86_execute(int cycles);
extern unsigned i86_get_context(void *dst);
extern void i86_set_context(void *src);
extern unsigned i86_get_reg(int regnum);
extern void i86_set_reg(int regnum, unsigned val);
extern void i86_set_irq_line(int irqline, int state);
extern void i86_set_irq_callback(int (*callback)(int irqline));
extern unsigned i86_dasm(char *buffer, unsigned pc);
extern const char *i86_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern unsigned DasmI86(char* buffer, unsigned pc);
#endif

#endif
