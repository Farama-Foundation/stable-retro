/* ASG 971222 -- rewrote this interface */
#ifndef __NEC_H_
#define __NEC_H_

#include "memory.h"
#include "osd_cpu.h"

enum {
	NEC_IP=1, NEC_AW, NEC_CW, NEC_DW, NEC_BW, NEC_SP, NEC_BP, NEC_IX, NEC_IY,
	NEC_FLAGS, NEC_ES, NEC_CS, NEC_SS, NEC_DS,
	NEC_VECTOR, NEC_PENDING, NEC_NMI_STATE, NEC_IRQ_STATE};

/* Public variables */
extern int nec_ICount;

/* Public functions */

#define v20_ICount nec_ICount
extern void v20_init(void);
extern void v20_reset(void *param);
extern void v20_exit(void);
extern int v20_execute(int cycles);
extern unsigned v20_get_context(void *dst);
extern void v20_set_context(void *src);
extern unsigned v20_get_reg(int regnum);
extern void v20_set_reg(int regnum, unsigned val);
extern void v20_set_irq_line(int irqline, int state);
extern void v20_set_irq_callback(int (*callback)(int irqline));
extern const char *v20_info(void *context, int regnum);
extern unsigned v20_dasm(char *buffer, unsigned pc);

#define v30_ICount nec_ICount
extern void v30_init(void);
extern void v30_reset(void *param);
extern void v30_exit(void);
extern int v30_execute(int cycles);
extern unsigned v30_get_context(void *dst);
extern void v30_set_context(void *src);
extern unsigned v30_get_reg(int regnum);
extern void v30_set_reg(int regnum, unsigned val);
extern void v30_set_irq_line(int irqline, int state);
extern void v30_set_irq_callback(int (*callback)(int irqline));
extern const char *v30_info(void *context, int regnum);
extern unsigned v30_dasm(char *buffer, unsigned pc);

#define v33_ICount nec_ICount
extern void v33_init(void);
extern void v33_reset(void *param);
extern void v33_exit(void);
extern int v33_execute(int cycles);
extern unsigned v33_get_context(void *dst);
extern void v33_set_context(void *src);
extern unsigned v33_get_reg(int regnum);
extern void v33_set_reg(int regnum, unsigned val);
extern void v33_set_irq_line(int irqline, int state);
extern void v33_set_irq_callback(int (*callback)(int irqline));
extern const char *v33_info(void *context, int regnum);
extern unsigned v33_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasmnec(char* buffer, unsigned pc);
#endif

#endif
