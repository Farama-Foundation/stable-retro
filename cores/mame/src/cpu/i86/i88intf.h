#ifndef __I88INTR_H_
#define __I88INTR_H_

#include "memory.h"
#include "osd_cpu.h"

#include "i86intf.h"

/* Public variables */
#define i88_ICount i86_ICount

/* Public functions */
#define i88_init i86_init
#define i88_reset i86_reset
#define i88_exit i86_exit
#define i88_execute i86_execute
#define i88_get_context i86_get_context
#define i88_set_context i86_set_context
#define i88_get_reg i86_get_reg
#define i88_set_reg i86_set_reg
#define i88_set_irq_line i86_set_irq_line
#define i88_set_irq_callback i86_set_irq_callback
extern const char *i88_info(void *context, int regnum);
#define i88_dasm i86_dasm

#endif
