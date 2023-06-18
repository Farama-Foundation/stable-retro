/*###################################################################################################
**
**
**		asap.h
**		Interface file for the portable Atari ASAP emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef _ASAP_H
#define _ASAP_H

#include "memory.h"
#include "osd_cpu.h"


/*###################################################################################################
**	REGISTER ENUMERATION
**#################################################################################################*/

enum
{
	ASAP_PC=1,ASAP_PS,
	ASAP_R0,ASAP_R1,ASAP_R2,ASAP_R3,ASAP_R4,ASAP_R5,ASAP_R6,ASAP_R7,
	ASAP_R8,ASAP_R9,ASAP_R10,ASAP_R11,ASAP_R12,ASAP_R13,ASAP_R14,ASAP_R15,
	ASAP_R16,ASAP_R17,ASAP_R18,ASAP_R19,ASAP_R20,ASAP_R21,ASAP_R22,ASAP_R23,
	ASAP_R24,ASAP_R25,ASAP_R26,ASAP_R27,ASAP_R28,ASAP_R29,ASAP_R30,ASAP_R31
};


/*###################################################################################################
**	INTERRUPT CONSTANTS
**#################################################################################################*/

#define ASAP_IRQ0		0		/* IRQ0 */


/*###################################################################################################
**	PUBLIC GLOBALS
**#################################################################################################*/

extern int asap_icount;


/*###################################################################################################
**	PUBLIC FUNCTIONS
**#################################################################################################*/

extern void asap_init(void);
extern void asap_reset(void *param);
extern void asap_exit(void);
extern int asap_execute(int cycles);
extern unsigned asap_get_context(void *dst);
extern void asap_set_context(void *src);
extern unsigned asap_get_reg(int regnum);
extern void asap_set_reg(int regnum, unsigned val);
extern void asap_set_irq_line(int irqline, int state);
extern void asap_set_irq_callback(int (*callback)(int irqline));
extern const char *asap_info(void *context, int regnum);
extern unsigned asap_dasm(char *buffer, unsigned pc);

extern void asap_set_cpi(int cpi);

#endif /* _ASAP_H */
