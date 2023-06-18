/*###################################################################################################
**
**
**		jaguar.h
**		Interface file for the portable Jaguar DSP emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef _JAGUAR_H
#define _JAGUAR_H

#include "memory.h"
#include "osd_cpu.h"


/*###################################################################################################
**	COMPILE-TIME DEFINITIONS
**#################################################################################################*/


/*###################################################################################################
**	GLOBAL CONSTANTS
**#################################################################################################*/

#define JAGUAR_VARIANT_GPU		0
#define JAGUAR_VARIANT_DSP		1


/*###################################################################################################
**	REGISTER ENUMERATION
**#################################################################################################*/

enum
{
	JAGUAR_PC=1,JAGUAR_FLAGS,
	JAGUAR_R0,JAGUAR_R1,JAGUAR_R2,JAGUAR_R3,JAGUAR_R4,JAGUAR_R5,JAGUAR_R6,JAGUAR_R7,
	JAGUAR_R8,JAGUAR_R9,JAGUAR_R10,JAGUAR_R11,JAGUAR_R12,JAGUAR_R13,JAGUAR_R14,JAGUAR_R15,
	JAGUAR_R16,JAGUAR_R17,JAGUAR_R18,JAGUAR_R19,JAGUAR_R20,JAGUAR_R21,JAGUAR_R22,JAGUAR_R23,
	JAGUAR_R24,JAGUAR_R25,JAGUAR_R26,JAGUAR_R27,JAGUAR_R28,JAGUAR_R29,JAGUAR_R30,JAGUAR_R31
};

enum
{
	G_FLAGS = 0,
	G_MTXC,
	G_MTXA,
	G_END,
	G_PC,
	G_CTRL,
	G_HIDATA,
	G_DIVCTRL,
	G_DUMMY,
	G_REMAINDER,
	G_CTRLMAX
};

enum
{
	D_FLAGS = 0,
	D_MTXC,
	D_MTXA,
	D_END,
	D_PC,
	D_CTRL,
	D_MOD,
	D_DIVCTRL,
	D_MACHI,
	D_REMAINDER,
	D_CTRLMAX
};


/*###################################################################################################
**	CONFIGURATION STRUCTURE
**#################################################################################################*/

struct jaguar_config
{
	void (*cpu_int_callback)(void);
};


/*###################################################################################################
**	INTERRUPT CONSTANTS
**#################################################################################################*/

#define JAGUAR_IRQ0		0		/* IRQ0 */
#define JAGUAR_IRQ1		1		/* IRQ1 */
#define JAGUAR_IRQ2		2		/* IRQ2 */
#define JAGUAR_IRQ3		3		/* IRQ3 */
#define JAGUAR_IRQ4		4		/* IRQ4 */
#define JAGUAR_IRQ5		5		/* IRQ5 */


/*###################################################################################################
**	PUBLIC GLOBALS
**#################################################################################################*/

extern int jaguar_icount;


/*###################################################################################################
**	PUBLIC FUNCTIONS - GPU
**#################################################################################################*/

extern void jaguargpu_init(void);
extern void jaguargpu_reset(void *param);
extern void jaguargpu_exit(void);
extern int jaguargpu_execute(int cycles);
extern unsigned jaguargpu_get_context(void *dst);
extern void jaguargpu_set_context(void *src);
extern unsigned jaguargpu_get_reg(int regnum);
extern void jaguargpu_set_reg(int regnum, unsigned val);
extern void jaguargpu_set_irq_line(int irqline, int state);
extern void jaguargpu_set_irq_callback(int (*callback)(int irqline));
extern const char *jaguargpu_info(void *context, int regnum);
extern unsigned jaguargpu_dasm(char *buffer, unsigned pc);

extern void jaguargpu_ctrl_w(int cpunum, offs_t offset, data32_t data, data32_t mem_mask);
extern data32_t jaguargpu_ctrl_r(int cpunum, offs_t offset);


/*###################################################################################################
**	PUBLIC FUNCTIONS - DSP
**#################################################################################################*/

extern void jaguardsp_init(void);
extern void jaguardsp_reset(void *param);
extern void jaguardsp_exit(void);
extern int jaguardsp_execute(int cycles);
extern unsigned jaguardsp_get_context(void *dst);
extern void jaguardsp_set_context(void *src);
extern unsigned jaguardsp_get_reg(int regnum);
extern void jaguardsp_set_reg(int regnum, unsigned val);
extern void jaguardsp_set_irq_line(int irqline, int state);
extern void jaguardsp_set_irq_callback(int (*callback)(int irqline));
extern const char *jaguardsp_info(void *context, int regnum);
extern unsigned jaguardsp_dasm(char *buffer, unsigned pc);

extern void jaguardsp_ctrl_w(int cpunum, offs_t offset, data32_t data, data32_t mem_mask);
extern data32_t jaguardsp_ctrl_r(int cpunum, offs_t offset);


#endif /* _JAGUAR_H */
