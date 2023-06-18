 /**************************************************************************\
 *				   Texas Instruments TMS32010 DSP Emulator					*
 *																			*
 *					Copyright (C) 1999-2002+ Tony La Porta					*
 *		You are not allowed to distribute this software commercially.		*
 *						Written for the MAME project.						*
 *																			*
 *																			*
 *		Note :	This is a word based microcontroller, with addressing		*
 *				architecture based on the Harvard addressing scheme.		*
 *																			*
 \**************************************************************************/

#ifndef _TMS32010_H
#define _TMS32010_H


#include "osd_cpu.h"
#include "cpuintrf.h"
#include "memory.h"


/****************************************************************************
 * Use this in the I/O port address fields of your driver for the BIO pin
 * i.e,
 *	{ TMS32010_PORT_RANGE(TMS32010_BIO, TMS32010_BIO), tms32010_bio_line_r },
 */

#define TMS32010_BIO			0x100		/* BIO input */


#define TMS32010_DATA_OFFSET	0x0000
#define TMS32010_PGM_OFFSET		0x8000

#define TMS32010_INT_PENDING	0x80000000
#define TMS32010_INT_NONE		0

#define  TMS32010_ADDR_MASK  0x0fff		/* TMS32010 can only address 0x0fff */
										/* however other TMS3201x devices	*/
										/* can address up to 0xffff (incase */
										/* their support is ever added).	*/


enum {
	TMS32010_PC=1, TMS32010_SP,   TMS32010_STR,  TMS32010_ACC,
	TMS32010_PREG, TMS32010_TREG, TMS32010_AR0,  TMS32010_AR1,
	TMS32010_STK0, TMS32010_STK1, TMS32010_STK2, TMS32010_STK3
};


/****************************************************************************
 *	Public Functions
 */

void tms32010_init(void);
void tms32010_reset(void *param);				/* Reset processor & registers	*/
void tms32010_exit(void);						/* Shutdown CPU core		*/
int tms32010_execute(int cycles);				/* Execute cycles T-States	*/
unsigned tms32010_get_context(void *dst);		/* Get registers			*/
void tms32010_set_context(void *src);			/* Set registers			*/
unsigned tms32010_get_reg(int regnum); 			/* Get specific register	*/
void tms32010_set_reg(int regnum, unsigned val);/* Set specific register	*/
void tms32010_set_irq_line(int irqline, int state);
void tms32010_set_irq_callback(int (*callback)(int irqline));
const char *tms32010_info(void *context, int regnum);
unsigned tms32010_dasm(char *buffer, unsigned pc);

extern int tms32010_icount;						/* T-state count */



/****************************************************************************
 *	Helpers for memory ranges
 */

#define TMS32010_DATA_ADDR_RANGE(start, end)	(TMS32010_DATA_OFFSET + ((start) << 1)), (TMS32010_DATA_OFFSET + ((end) << 1) + 1)
#define TMS32010_PGM_ADDR_RANGE(start, end)		(TMS32010_PGM_OFFSET + ((start) << 1)), (TMS32010_PGM_OFFSET + ((end) << 1) + 1)
#define TMS32010_PORT_RANGE(start, end)			((start) << 1), (((end) << 1) + 1)



/****************************************************************************
 *	Read the state of the BIO pin
 */

#define TMS32010_BIO_In (cpu_readport16bew_word(TMS32010_BIO<<1))


/****************************************************************************
 *	Input a word from given I/O port
 */

#define TMS32010_In(Port) (cpu_readport16bew_word((Port)<<1))


/****************************************************************************
 *	Output a word to given I/O port
 */

#define TMS32010_Out(Port,Value) (cpu_writeport16bew_word((Port)<<1,Value))



/****************************************************************************
 *	Read a word from given ROM memory location
 */

#define TMS32010_ROM_RDMEM(A) (cpu_readmem16bew_word(((A)<<1)+TMS32010_PGM_OFFSET))


/****************************************************************************
 *	Write a word to given ROM memory location
 */

#define TMS32010_ROM_WRMEM(A,V) (cpu_writemem16bew_word(((A)<<1)+TMS32010_PGM_OFFSET,V))



/****************************************************************************
 *	Read a word from given RAM memory location
 *	The following adds 8000h to the address, since MAME doesnt support
 *	RAM and ROM living in the same address space. RAM really starts at
 *	address 0 and are word entities.
 */

#define TMS32010_RAM_RDMEM(A) (cpu_readmem16bew_word(((A)<<1)+TMS32010_DATA_OFFSET))


/****************************************************************************
 *	Write a word to given RAM memory location
 *	The following adds 8000h to the address, since MAME doesnt support
 *	RAM and ROM living in the same address space. RAM really starts at
 *	address 0 and word entities.
 */

#define TMS32010_RAM_WRMEM(A,V) (cpu_writemem16bew_word(((A)<<1)+TMS32010_DATA_OFFSET,V))



/****************************************************************************
 *	TMS32010_RDOP() is identical to TMS32010_RDMEM() except it is used for reading
 *	opcodes. In case of system with memory mapped I/O, this function can be
 *	used to greatly speed up emulation
 */

#define TMS32010_RDOP(A) (cpu_readop16(((A)<<1)+TMS32010_PGM_OFFSET))


/****************************************************************************
 *	TMS32010_RDOP_ARG() is identical to TMS32010_RDOP() except it is used
 *	for reading opcode arguments. This difference can be used to support systems
 *	that use different encoding mechanisms for opcodes and opcode arguments
 */

#define TMS32010_RDOP_ARG(A) (cpu_readop_arg16(((A)<<1)+TMS32010_PGM_OFFSET))



#ifdef	MAME_DEBUG
extern unsigned Dasm32010(char *buffer, unsigned pc);
#endif

#endif	/* _TMS32010_H */
