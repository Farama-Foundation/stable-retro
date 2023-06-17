/*****************************************************************************
 *
 *	 m6509.h
 *	 Portable 6509 emulator V1.0beta
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _M6509_H
#define _M6509_H

#include "cpuintrf.h"
#include "osd_cpu.h"
#include "m6502.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RUNTIME_LOADER
	extern void m6509_runtime_loader_init(void);
#endif

enum {
	M6509_PC=1, M6509_S, M6509_P, M6509_A, M6509_X, M6509_Y,
	M6509_EA, M6509_ZP, M6509_NMI_STATE, M6509_IRQ_STATE, M6509_SO_STATE,
	M6509_PC_BANK, M6509_IND_BANK
};

#define M6509_IRQ_LINE					M6502_IRQ_LINE
/* use cpu_set_irq_line(cpu, M6509_SET_OVERFLOW, level)
   to change level of the so input line
   positiv edge sets overflow flag */
#define M6509_SET_OVERFLOW 3

extern int m6509_ICount;				/* cycle count */

extern void m6509_init(void);
extern void m6509_reset(void *param);			/* Reset registers to the initial values */
extern void m6509_exit(void);					/* Shut down CPU core */
extern int	m6509_execute(int cycles);			/* Execute cycles - returns number of cycles actually run */
extern unsigned m6509_get_context (void *dst);	/* Get registers, return context size */
extern void m6509_set_context (void *src);		/* Set registers */
extern unsigned m6509_get_reg (int regnum);
extern void m6509_set_reg (int regnum, unsigned val);
extern void m6509_set_irq_line(int irqline, int state);
extern void m6509_set_irq_callback(int (*callback)(int irqline));
extern void m6509_state_save(void *file);
extern void m6509_state_load(void *file);
extern const char *m6509_info(void *context, int regnum);
extern unsigned m6509_dasm(char *buffer, unsigned pc);

READ_HANDLER( m6509_read_00000 );
READ_HANDLER( m6509_read_00001 );

WRITE_HANDLER( m6509_write_00000 );
WRITE_HANDLER( m6509_write_00001 );

#ifdef MAME_DEBUG
extern unsigned int Dasm6509( char *dst, unsigned pc );
#endif

#ifdef __cplusplus
}
#endif

#endif /* _M6509_H */
