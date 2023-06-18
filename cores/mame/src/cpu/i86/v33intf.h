#ifndef __V33INTRF_H_
#define __V33INTRF_H_

#include "memory.h"
#include "osd_cpu.h"

#include "i86intrf.h"
#include "v30intrf.h"

/* Public variables */
#define v33_ICount i86_ICount

/* Public functions */
#define v33_init v30_init
#define v33_reset v30_reset
#define v33_exit i86_exit
#define v33_execute v30_execute
#define v33_get_context i86_get_context
#define v33_set_context i86_set_context
#define v33_get_reg i86_get_reg
#define v33_set_reg i86_set_reg
#define v33_set_irq_line i86_set_irq_line
#define v33_set_irq_callback i86_set_irq_callback
extern const char *v33_info(void *context, int regnum);
#define v33_dasm v30_dasm

#endif
