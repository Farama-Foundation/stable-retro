#ifndef ARM_H
#define ARM_H

#include "driver.h"

/****************************************************************************************************
 *	INTERRUPT CONSTANTS
 ***************************************************************************************************/

#define ARM_IRQ_LINE	0
#define ARM_FIRQ_LINE	1

/****************************************************************************************************
 *	PUBLIC GLOBALS
 ***************************************************************************************************/

extern int arm_ICount;

/****************************************************************************************************
 *	PUBLIC FUNCTIONS
 ***************************************************************************************************/

extern void arm_init(void);
extern void arm_reset(void *param);
extern void arm_exit(void);
extern int arm_execute(int cycles);
extern unsigned arm_get_context(void *dst);
extern void arm_set_context(void *src);
extern unsigned arm_get_pc(void);
extern void arm_set_pc(unsigned val);
extern unsigned arm_get_sp(void);
extern void arm_set_sp(unsigned val);
extern unsigned arm_get_reg(int regnum);
extern void arm_set_reg(int regnum, unsigned val);
extern void arm_interrupt( int type );
extern void arm_set_nmi_line(int state);
extern void arm_set_irq_line(int irqline, int state);
extern void arm_set_irq_callback(int (*callback)(int irqline));
extern const char *arm_info(void *context, int regnum);
extern unsigned arm_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern void arm_disasm( char *pBuf, data32_t pc, data32_t opcode );
#endif

enum
{
	ARM32_R0=1, ARM32_R1, ARM32_R2, ARM32_R3, ARM32_R4, ARM32_R5, ARM32_R6, ARM32_R7,
	ARM32_R8, ARM32_R9, ARM32_R10, ARM32_R11, ARM32_R12, ARM32_R13, ARM32_R14, ARM32_R15,
	ARM32_FR8, ARM32_FR9, ARM32_FR10, ARM32_FR11, ARM32_FR12, ARM32_FR13, ARM32_FR14,
	ARM32_IR13, ARM32_IR14, ARM32_SR13, ARM32_SR14
};

#endif /* ARM_H */
