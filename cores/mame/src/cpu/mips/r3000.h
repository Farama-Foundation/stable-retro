/*###################################################################################################
**
**
**		r3000.h
**		Interface file for the portable MIPS R3000 emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef _R3000_H
#define _R3000_H

#include "memory.h"
#include "osd_cpu.h"


/*###################################################################################################
**	COMPILE-TIME DEFINITIONS
**#################################################################################################*/


/*###################################################################################################
**	REGISTER ENUMERATION
**#################################################################################################*/

enum
{
	R3000_PC=1,R3000_SR,
	R3000_R0,R3000_R1,R3000_R2,R3000_R3,R3000_R4,R3000_R5,R3000_R6,R3000_R7,
	R3000_R8,R3000_R9,R3000_R10,R3000_R11,R3000_R12,R3000_R13,R3000_R14,R3000_R15,
	R3000_R16,R3000_R17,R3000_R18,R3000_R19,R3000_R20,R3000_R21,R3000_R22,R3000_R23,
	R3000_R24,R3000_R25,R3000_R26,R3000_R27,R3000_R28,R3000_R29,R3000_R30,R3000_R31
};


/*###################################################################################################
**	INTERRUPT CONSTANTS
**#################################################################################################*/

#define R3000_IRQ0		0		/* IRQ0 */
#define R3000_IRQ1		1		/* IRQ1 */
#define R3000_IRQ2		2		/* IRQ2 */
#define R3000_IRQ3		3		/* IRQ3 */
#define R3000_IRQ4		4		/* IRQ4 */
#define R3000_IRQ5		5		/* IRQ5 */


/*###################################################################################################
**	STRUCTURES
**#################################################################################################*/

struct r3000_config
{
	UINT8		hasfpu;			/* 1 if we have an FPU, 0 otherwise */
	size_t		icache;			/* code cache size */
	size_t		dcache;			/* data cache size */
};


/*###################################################################################################
**	PUBLIC GLOBALS
**#################################################################################################*/

extern int r3000_icount;


/*###################################################################################################
**	PUBLIC FUNCTIONS
**#################################################################################################*/

extern void r3000_init(void);
extern void r3000be_reset(void *param);
extern void r3000le_reset(void *param);
extern void r3000_exit(void);
extern int r3000_execute(int cycles);
extern unsigned r3000_get_context(void *dst);
extern void r3000_set_context(void *src);
extern unsigned r3000_get_reg(int regnum);
extern void r3000_set_reg(int regnum, unsigned val);
extern void r3000_set_irq_line(int irqline, int state);
extern void r3000_set_irq_callback(int (*callback)(int irqline));
extern const char *r3000_info(void *context, int regnum);
extern unsigned r3000_dasm(char *buffer, unsigned pc);

#define r3000be_init				r3000_init
#define r3000be_exit				r3000_exit
#define r3000be_execute				r3000_execute
#define r3000be_get_context			r3000_get_context
#define r3000be_set_context			r3000_set_context
#define r3000be_get_reg				r3000_get_reg
#define r3000be_set_reg				r3000_set_reg
#define r3000be_set_irq_line		r3000_set_irq_line
#define r3000be_set_irq_callback	r3000_set_irq_callback
#define r3000be_info 				r3000_info
#define r3000be_dasm 				r3000_dasm

#define r3000le_init				r3000_init
#define r3000le_exit				r3000_exit
#define r3000le_execute				r3000_execute
#define r3000le_get_context			r3000_get_context
#define r3000le_set_context			r3000_set_context
#define r3000le_get_reg				r3000_get_reg
#define r3000le_set_reg				r3000_set_reg
#define r3000le_set_irq_line		r3000_set_irq_line
#define r3000le_set_irq_callback	r3000_set_irq_callback
#define r3000le_info 				r3000_info
#define r3000le_dasm 				r3000_dasm



#endif /* _JAGUAR_H */
