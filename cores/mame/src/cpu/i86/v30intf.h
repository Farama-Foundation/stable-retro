#ifndef __V30INTRF_H_
#define __V30INTRF_H_

#include "memory.h"
#include "osd_cpu.h"

#include "i86intf.h"

/* Public variables */
#define v30_ICount i86_ICount

/* Public functions */
extern void v30_init(void);
extern void v30_reset(void *param);
#define v30_exit i86_exit
extern int v30_execute(int cycles);
#define v30_get_context i86_get_context
#define v30_set_context i86_set_context
#define v30_get_reg i86_get_reg
#define v30_set_reg i86_set_reg
#define v30_set_irq_line i86_set_irq_line
#define v30_set_irq_callback i86_set_irq_callback
extern const char *v30_info(void *context, int regnum);
extern unsigned v30_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned DasmV30(char* buffer, unsigned pc);
#endif

#endif
