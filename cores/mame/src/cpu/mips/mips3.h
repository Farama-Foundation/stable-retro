/*###################################################################################################
**
**
**		mips3.h
**		Interface file for the portable MIPS III/IV emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef _MIPS3_H
#define _MIPS3_H

#include "memory.h"
#include "osd_cpu.h"


/*###################################################################################################
**	REGISTER ENUMERATION
**#################################################################################################*/

enum
{
	MIPS3_PC=1,MIPS3_SR,
	MIPS3_R0HI,  MIPS3_R0LO,  MIPS3_R0,
	MIPS3_R1HI,  MIPS3_R1LO,  MIPS3_R1,
	MIPS3_R2HI,  MIPS3_R2LO,  MIPS3_R2,
	MIPS3_R3HI,  MIPS3_R3LO,  MIPS3_R3,
	MIPS3_R4HI,  MIPS3_R4LO,  MIPS3_R4,
	MIPS3_R5HI,  MIPS3_R5LO,  MIPS3_R5,
	MIPS3_R6HI,  MIPS3_R6LO,  MIPS3_R6,
	MIPS3_R7HI,  MIPS3_R7LO,  MIPS3_R7,
	MIPS3_R8HI,  MIPS3_R8LO,  MIPS3_R8,
	MIPS3_R9HI,  MIPS3_R9LO,  MIPS3_R9,
	MIPS3_R10HI, MIPS3_R10LO, MIPS3_R10,
	MIPS3_R11HI, MIPS3_R11LO, MIPS3_R11,
	MIPS3_R12HI, MIPS3_R12LO, MIPS3_R12,
	MIPS3_R13HI, MIPS3_R13LO, MIPS3_R13,
	MIPS3_R14HI, MIPS3_R14LO, MIPS3_R14,
	MIPS3_R15HI, MIPS3_R15LO, MIPS3_R15,
	MIPS3_R16HI, MIPS3_R16LO, MIPS3_R16,
	MIPS3_R17HI, MIPS3_R17LO, MIPS3_R17,
	MIPS3_R18HI, MIPS3_R18LO, MIPS3_R18,
	MIPS3_R19HI, MIPS3_R19LO, MIPS3_R19,
	MIPS3_R20HI, MIPS3_R20LO, MIPS3_R20,
	MIPS3_R21HI, MIPS3_R21LO, MIPS3_R21,
	MIPS3_R22HI, MIPS3_R22LO, MIPS3_R22,
	MIPS3_R23HI, MIPS3_R23LO, MIPS3_R23,
	MIPS3_R24HI, MIPS3_R24LO, MIPS3_R24,
	MIPS3_R25HI, MIPS3_R25LO, MIPS3_R25,
	MIPS3_R26HI, MIPS3_R26LO, MIPS3_R26,
	MIPS3_R27HI, MIPS3_R27LO, MIPS3_R27,
	MIPS3_R28HI, MIPS3_R28LO, MIPS3_R28,
	MIPS3_R29HI, MIPS3_R29LO, MIPS3_R29,
	MIPS3_R30HI, MIPS3_R30LO, MIPS3_R30,
	MIPS3_R31HI, MIPS3_R31LO, MIPS3_R31,
	MIPS3_HIHI,  MIPS3_HILO, MIPS3_HI,
	MIPS3_LOHI,  MIPS3_LOLO, MIPS3_LO,
	MIPS3_EPC,   MIPS3_CAUSE,
	MIPS3_COUNT, MIPS3_COMPARE
};


/*###################################################################################################
**	INTERRUPT CONSTANTS
**#################################################################################################*/

#define MIPS3_IRQ0		0		/* IRQ0 */
#define MIPS3_IRQ1		1		/* IRQ1 */
#define MIPS3_IRQ2		2		/* IRQ2 */
#define MIPS3_IRQ3		3		/* IRQ3 */
#define MIPS3_IRQ4		4		/* IRQ4 */
#define MIPS3_IRQ5		5		/* IRQ5 */


/*###################################################################################################
**	STRUCTURES
**#################################################################################################*/

struct mips3_config
{
	size_t		icache;							/* code cache size */
	size_t		dcache;							/* data cache size */
};


/*###################################################################################################
**	PUBLIC GLOBALS
**#################################################################################################*/

extern int mips3_icount;


/*###################################################################################################
**	PUBLIC FUNCTIONS
**#################################################################################################*/

extern void mips3_init(void);
extern void mips3_exit(void);
extern int mips3_execute(int cycles);
extern unsigned mips3_get_context(void *dst);
extern void mips3_set_context(void *src);
extern unsigned mips3_get_reg(int regnum);
extern void mips3_set_reg(int regnum, unsigned val);
extern void mips3_set_irq_line(int irqline, int state);
extern void mips3_set_irq_callback(int (*callback)(int irqline));
extern unsigned mips3_dasm(char *buffer, unsigned pc);

#if HAS_R4600
extern const char *r4600_info(void *context, int regnum);
extern void r4600be_reset(void *param);
extern void r4600le_reset(void *param);

#define r4600be_init				mips3_init
#define r4600be_exit				mips3_exit
#define r4600be_execute				mips3_execute
#define r4600be_get_context			mips3_get_context
#define r4600be_set_context			mips3_set_context
#define r4600be_get_reg				mips3_get_reg
#define r4600be_set_reg				mips3_set_reg
#define r4600be_set_irq_line		mips3_set_irq_line
#define r4600be_set_irq_callback	mips3_set_irq_callback
#define r4600be_info 				r4600_info
#define r4600be_dasm 				mips3_dasm

#define r4600le_init				mips3_init
#define r4600le_exit				mips3_exit
#define r4600le_execute				mips3_execute
#define r4600le_get_context			mips3_get_context
#define r4600le_set_context			mips3_set_context
#define r4600le_get_reg				mips3_get_reg
#define r4600le_set_reg				mips3_set_reg
#define r4600le_set_irq_line		mips3_set_irq_line
#define r4600le_set_irq_callback	mips3_set_irq_callback
#define r4600le_info 				r4600_info
#define r4600le_dasm 				mips3_dasm
#endif

#if HAS_R5000
extern const char *r5000_info(void *context, int regnum);
extern void r5000be_reset(void *param);
extern void r5000le_reset(void *param);

#define r5000be_init				mips3_init
#define r5000be_exit				mips3_exit
#define r5000be_execute				mips3_execute
#define r5000be_get_context			mips3_get_context
#define r5000be_set_context			mips3_set_context
#define r5000be_get_reg				mips3_get_reg
#define r5000be_set_reg				mips3_set_reg
#define r5000be_set_irq_line		mips3_set_irq_line
#define r5000be_set_irq_callback	mips3_set_irq_callback
#define r5000be_info 				r5000_info
#define r5000be_dasm 				mips3_dasm

#define r5000le_init				mips3_init
#define r5000le_exit				mips3_exit
#define r5000le_execute				mips3_execute
#define r5000le_get_context			mips3_get_context
#define r5000le_set_context			mips3_set_context
#define r5000le_get_reg				mips3_get_reg
#define r5000le_set_reg				mips3_set_reg
#define r5000le_set_irq_line		mips3_set_irq_line
#define r5000le_set_irq_callback	mips3_set_irq_callback
#define r5000le_info 				r5000_info
#define r5000le_dasm 				mips3_dasm
#endif


/*###################################################################################################
**	COMPILER-SPECIFIC OPTIONS
**#################################################################################################*/

/* fix me -- how do we make this work?? */
#define MIPS3DRC_STRICT_VERIFY		0x0001			/* verify all instructions */
#define MIPS3DRC_STRICT_COP0		0x0002			/* validate all COP0 instructions */
#define MIPS3DRC_STRICT_COP1		0x0004			/* validate all COP1 instructions */
#define MIPS3DRC_STRICT_COP2		0x0008			/* validate all COP2 instructions */
#define MIPS3DRC_DIRECT_RAM			0x0010			/* allow direct RAM access (no bankswitching!) */

#define MIPS3DRC_COMPATIBLE_OPTIONS	(MIPS3DRC_STRICT_VERIFY | MIPS3DRC_STRICT_COP0 | MIPS3DRC_STRICT_COP1 | MIPS3DRC_STRICT_COP2)
#define MIPS3DRC_FASTEST_OPTIONS	(MIPS3DRC_DIRECT_RAM)

void mips3drc_set_options(UINT8 cpunum, UINT32 opts);



#endif /* _MIPS3_H */
