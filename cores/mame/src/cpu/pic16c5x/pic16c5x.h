 /**************************************************************************\
 *						Microchip PIC16C5x Emulator							*
 *																			*
 *					  Copyright (C) 2003+ Tony La Porta						*
 *				   Originally written for the MAME project.					*
 *																			*
 *																			*
 *		Addressing architecture is based on the Harvard addressing scheme.	*
 *																			*
 \**************************************************************************/

#ifndef _PIC16C5X_H
#define _PIC16C5X_H


#include "osd_cpu.h"
#include "cpuintrf.h"
#include "memory.h"


/****************************************************************************
 *	Use these in the memory and/or IO port address fields of your driver
 *	for simplified memory and IO mapping.
 *	i.e,
 *		{ PIC16C5x_T0, PIC16C5x_T0, PIC16C5X_T0_clk_r },
 *		{ PIC16C55_MEMORY_READ },
 *		{ PIC16C55_MEMORY_WRITE },
 *		{ PIC16C57_MEMORY_READ },
 *		{ PIC16C57_MEMORY_WRITE },
 */

#define PIC16C5X_DATA_OFFSET	0x0000
#define PIC16C5X_PGM_OFFSET		0x1000



/**************************************************************************
 *	Internal Clock divisor
 *
 *	External Clock is divided internally by 4 for the instruction cycle
 *	times. (Each instruction cycle passes through 4 machine states).
 */

#define PIC16C5x_CLOCK_DIVIDER		4


enum {
	PIC16C5x_PC=1, PIC16C5x_STK0, PIC16C5x_STK1, PIC16C5x_FSR,
	PIC16C5x_W,    PIC16C5x_ALU,  PIC16C5x_STR,  PIC16C5x_OPT,
	PIC16C5x_TMR0, PIC16C5x_PRTA, PIC16C5x_PRTB, PIC16C5x_PRTC,
	PIC16C5x_WDT,  PIC16C5x_TRSA, PIC16C5x_TRSB, PIC16C5x_TRSC,
	PIC16C5x_PSCL
};


/****************************************************************************
 *	Public Data
 */

extern int pic16C5x_icount;						/* T-state count */


/****************************************************************************
 *	Function to configure the CONFIG register. This is actually hard-wired
 *	during ROM programming, so should be called in the driver INIT, with
 *	the value if known (available in HEX dumps of the ROM).
 */

void pic16c5x_config(int data);


/****************************************************************************
 *	Read the state of the T0 Clock input signal
 */

#define PIC16C5x_T0		0x10
#define PIC16C5x_T0_In (cpu_readport16(PIC16C5x_T0))


/****************************************************************************
 *	Input a word from given I/O port
 */

#define PIC16C5x_In(Port) ((UINT8)cpu_readport16((Port)))


/****************************************************************************
 *	Output a word to given I/O port
 */

#define PIC16C5x_Out(Port,Value) (cpu_writeport16((Port),Value))



/****************************************************************************
 *	Read a word from given RAM memory location
 *	The following adds 800h to the address, since MAME doesnt support
 *	RAM and ROM living in the same address space. RAM really starts at
 *	address 0 and are word entities.
 */

#define PIC16C5x_RAM_RDMEM(A) ((UINT8)cpu_readmem16((A)+PIC16C5X_DATA_OFFSET))


/****************************************************************************
 *	Write a word to given RAM memory location
 *	The following adds 800h to the address, since MAME doesnt support
 *	RAM and ROM living in the same address space. RAM really starts at
 *	address 0 and word entities.
 */

#define PIC16C5x_RAM_WRMEM(A,V) (cpu_writemem16((A)+PIC16C5X_DATA_OFFSET,V))



/****************************************************************************
 *	PIC16C5X_RDOP() is identical to PIC16C5X_RDMEM() except it is used for
 *	reading opcodes. In case of system with memory mapped I/O, this function
 *	can be used to greatly speed up emulation
 */

#define PIC16C5x_RDOP(A) (cpu_readop16(((A)<<1)+PIC16C5X_PGM_OFFSET))


/****************************************************************************
 *	PIC16C5X_RDOP_ARG() is identical to PIC16C5X_RDOP() except it is used
 *	for reading opcode arguments. This difference can be used to support systems
 *	that use different encoding mechanisms for opcodes and opcode arguments
 */

#define PIC16C5x_RDOP_ARG(A) (cpu_readop_arg16(((A)<<1)+PIC16C5X_PGM_OFFSET))




#if (HAS_PIC16C54)
/****************************************************************************
 *	PIC16C54
 ****************************************************************************/
#define pic16C54_icount			pic16C5x_icount
#define PIC16C54_RESET_VECTOR	0x1ff
#define PIC16C54_DATA_OFFSET	PIC16C5X_DATA_OFFSET
#define PIC16C54_PGM_OFFSET		PIC16C5X_PGM_OFFSET

#define PIC16C54_MEMORY_READ													\
	  (PIC16C54_DATA_OFFSET + 0x00), (PIC16C54_DATA_OFFSET + 0x1f), MRA_RAM },	\
	{ (PIC16C54_PGM_OFFSET + 0x000), (PIC16C54_PGM_OFFSET + ((0x1ff*2)+1)), MRA_ROM

#define PIC16C54_MEMORY_WRITE											 		\
	  (PIC16C54_DATA_OFFSET + 0x00), (PIC16C54_DATA_OFFSET + 0x1f), MWA_RAM },	\
	{ (PIC16C54_PGM_OFFSET + 0x000), (PIC16C54_PGM_OFFSET + ((0x1ff*2)+1)), MWA_ROM

extern void pic16C54_init(void);
extern void pic16C54_reset(void *param);
extern void pic16C54_exit(void);
extern int	pic16C54_execute(int cycles);
extern unsigned pic16C54_get_context(void *dst);
extern void pic16C54_set_context(void *src);
extern unsigned pic16C54_get_reg(int regnum);
extern void pic16C54_set_reg(int regnum, unsigned val);
extern void pic16C54_set_irq_line(int irqline, int state);
extern void pic16C54_set_irq_callback(int (*callback)(int irqline));
extern const char *pic16C54_info(void *context, int regnum);
extern unsigned pic16C54_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C55)
/****************************************************************************
 *	PIC16C55
 ****************************************************************************/
#define pic16C55_icount			pic16C5x_icount
#define PIC16C55_RESET_VECTOR	0x1ff
#define PIC16C55_DATA_OFFSET	PIC16C5X_DATA_OFFSET
#define PIC16C55_PGM_OFFSET		PIC16C5X_PGM_OFFSET

#define PIC16C55_MEMORY_READ													\
	  (PIC16C55_DATA_OFFSET + 0x00), (PIC16C55_DATA_OFFSET + 0x1f), MRA_RAM },	\
	{ (PIC16C55_PGM_OFFSET + 0x000), (PIC16C55_PGM_OFFSET + ((0x1ff*2)+1)), MRA_ROM

#define PIC16C55_MEMORY_WRITE													\
	  (PIC16C55_DATA_OFFSET + 0x00), (PIC16C55_DATA_OFFSET + 0x1f), MWA_RAM },	\
	{ (PIC16C55_PGM_OFFSET + 0x000), (PIC16C55_PGM_OFFSET + ((0x1ff*2)+1)), MWA_ROM

extern void pic16C55_init(void);
extern void pic16C55_reset(void *param);
extern void pic16C55_exit(void);
extern int	pic16C55_execute(int cycles);
extern unsigned pic16C55_get_context(void *dst);
extern void pic16C55_set_context(void *src);
extern unsigned pic16C55_get_reg(int regnum);
extern void pic16C55_set_reg(int regnum, unsigned val);
extern void pic16C55_set_irq_line(int irqline, int state);
extern void pic16C55_set_irq_callback(int (*callback)(int irqline));
extern const char *pic16C55_info(void *context, int regnum);
extern unsigned pic16C55_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C56)
/****************************************************************************
 *	PIC16C56
 ****************************************************************************/
#define pic16C56_icount			pic16C5x_icount
#define PIC16C56_RESET_VECTOR	0x3ff
#define PIC16C56_DATA_OFFSET	PIC16C5X_DATA_OFFSET
#define PIC16C56_PGM_OFFSET		PIC16C5X_PGM_OFFSET

#define PIC16C56_MEMORY_READ													\
	  (PIC16C56_DATA_OFFSET + 0x00), (PIC16C56_DATA_OFFSET + 0x1f), MRA_RAM },	\
	{ (PIC16C56_PGM_OFFSET + 0x000), (PIC16C56_PGM_OFFSET + ((0x3ff*2)+1)), MRA_ROM

#define PIC16C56_MEMORY_WRITE													\
	  (PIC16C56_DATA_OFFSET + 0x00), (PIC16C56_DATA_OFFSET + 0x1f), MWA_RAM },	\
	{ (PIC16C56_PGM_OFFSET + 0x000), (PIC16C56_PGM_OFFSET + ((0x3ff*2)+1)), MWA_ROM

extern void pic16C56_init(void);
extern void pic16C56_reset(void *param);
extern void pic16C56_exit(void);
extern int	pic16C56_execute(int cycles);
extern unsigned pic16C56_get_context(void *dst);
extern void pic16C56_set_context(void *src);
extern unsigned pic16C56_get_reg(int regnum);
extern void pic16C56_set_reg(int regnum, unsigned val);
extern void pic16C56_set_irq_line(int irqline, int state);
extern void pic16C56_set_irq_callback(int (*callback)(int irqline));
extern const char *pic16C56_info(void *context, int regnum);
extern unsigned pic16C56_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C57)
/****************************************************************************
 *	PIC16C57
 ****************************************************************************/
#define pic16C57_icount			pic16C5x_icount
#define PIC16C57_RESET_VECTOR	0x7ff
#define PIC16C57_DATA_OFFSET	PIC16C5X_DATA_OFFSET
#define PIC16C57_PGM_OFFSET		PIC16C5X_PGM_OFFSET

#define PIC16C57_MEMORY_READ													\
	  (PIC16C57_DATA_OFFSET + 0x00), (PIC16C57_DATA_OFFSET + 0x1f), MRA_RAM },	\
	{ (PIC16C57_DATA_OFFSET + 0x30), (PIC16C57_DATA_OFFSET + 0x3f), MRA_RAM },	\
	{ (PIC16C57_DATA_OFFSET + 0x50), (PIC16C57_DATA_OFFSET + 0x5f), MRA_RAM },	\
	{ (PIC16C57_DATA_OFFSET + 0x70), (PIC16C57_DATA_OFFSET + 0x7f), MRA_RAM },	\
	{ (PIC16C57_PGM_OFFSET + 0x000), (PIC16C57_PGM_OFFSET + ((0x7ff*2)+1)), MRA_ROM

#define PIC16C57_MEMORY_WRITE													\
	  (PIC16C57_DATA_OFFSET + 0x00), (PIC16C57_DATA_OFFSET + 0x1f), MWA_RAM },	\
	{ (PIC16C57_DATA_OFFSET + 0x30), (PIC16C57_DATA_OFFSET + 0x3f), MWA_RAM },	\
	{ (PIC16C57_DATA_OFFSET + 0x50), (PIC16C57_DATA_OFFSET + 0x5f), MWA_RAM },	\
	{ (PIC16C57_DATA_OFFSET + 0x70), (PIC16C57_DATA_OFFSET + 0x7f), MWA_RAM },	\
	{ (PIC16C57_PGM_OFFSET + 0x000), (PIC16C57_PGM_OFFSET + ((0x7ff*2)+1)), MWA_ROM

extern void pic16C57_init(void);
extern void pic16C57_reset(void *param);
extern void pic16C57_exit(void);
extern int	pic16C57_execute(int cycles);
extern unsigned pic16C57_get_context(void *dst);
extern void pic16C57_set_context(void *src);
extern unsigned pic16C57_get_reg(int regnum);
extern void pic16C57_set_reg(int regnum, unsigned val);
extern void pic16C57_set_irq_line(int irqline, int state);
extern void pic16C57_set_irq_callback(int (*callback)(int irqline));
extern const char *pic16C57_info(void *context, int regnum);
extern unsigned pic16C57_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif



#if (HAS_PIC16C58)
/****************************************************************************
 *	PIC16C58
 ****************************************************************************/
#define pic16C58_icount			pic16C5x_icount
#define PIC16C58_RESET_VECTOR	0x7ff
#define PIC16C58_DATA_OFFSET	PIC16C5X_DATA_OFFSET
#define PIC16C58_PGM_OFFSET		PIC16C5X_PGM_OFFSET

#define PIC16C58_MEMORY_READ													\
	  (PIC16C58_DATA_OFFSET + 0x00), (PIC16C58_DATA_OFFSET + 0x1f), MRA_RAM },	\
	{ (PIC16C58_DATA_OFFSET + 0x30), (PIC16C58_DATA_OFFSET + 0x3f), MRA_RAM },	\
	{ (PIC16C58_DATA_OFFSET + 0x50), (PIC16C58_DATA_OFFSET + 0x5f), MRA_RAM },	\
	{ (PIC16C58_DATA_OFFSET + 0x70), (PIC16C58_DATA_OFFSET + 0x7f), MRA_RAM },	\
	{ (PIC16C58_PGM_OFFSET + 0x000), (PIC16C58_PGM_OFFSET + ((0x7ff*2)+1)), MRA_ROM

#define PIC16C58_MEMORY_WRITE													\
	  (PIC16C58_DATA_OFFSET + 0x00), (PIC16C58_DATA_OFFSET + 0x1f), MWA_RAM },	\
	{ (PIC16C58_DATA_OFFSET + 0x30), (PIC16C58_DATA_OFFSET + 0x3f), MWA_RAM },	\
	{ (PIC16C58_DATA_OFFSET + 0x50), (PIC16C58_DATA_OFFSET + 0x5f), MWA_RAM },	\
	{ (PIC16C58_DATA_OFFSET + 0x70), (PIC16C58_DATA_OFFSET + 0x7f), MWA_RAM },	\
	{ (PIC16C58_PGM_OFFSET + 0x000), (PIC16C58_PGM_OFFSET + ((0x7ff*2)+1)), MWA_ROM

extern void pic16C58_init(void);
extern void pic16C58_reset(void *param);
extern void pic16C58_exit(void);
extern int	pic16C58_execute(int cycles);
extern unsigned pic16C58_get_context(void *dst);
extern void pic16C58_set_context(void *src);
extern unsigned pic16C58_get_reg(int regnum);
extern void pic16C58_set_reg(int regnum, unsigned val);
extern void pic16C58_set_irq_line(int irqline, int state);
extern void pic16C58_set_irq_callback(int (*callback)(int irqline));
extern const char *pic16C58_info(void *context, int regnum);
extern unsigned pic16C58_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm16C5x(char *buffer, unsigned pc);
#endif

#endif

#endif	/* _PIC16C5X_H */
