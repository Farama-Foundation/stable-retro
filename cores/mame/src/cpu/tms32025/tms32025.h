 /**************************************************************************\
 *				  Texas Instruments TMS320x25 DSP Emulator					*
 *																			*
 *				   Copyright (C) 2001-2002+ Tony La Porta					*
 *						Written for the MAME project.						*
 *																			*
 *		Note :	This is a word based microcontroller, with addressing		*
 *				architecture based on the Harvard addressing scheme.		*
 *																			*
 *	Three versions of the chip are available, and they are: 				*
 *	TMS320C25   Internal ROM one time programmed at TI						*
 *	TMS320E25   Internal ROM programmable as a normal EPROM					*
 *	TMS320P25   Internal ROM programmable once as a normal EPROM only		*
 *	These devices can also be used as a MicroController with external ROM	*
 *																			*
 \***************************************************************************/

#ifndef _TMS32025_H
#define _TMS32025_H


#include "osd_cpu.h"
#include "cpuintrf.h"
#include "memory.h"




/****************************************************************************
 *	Use some of these helpers in the memory and I/O port address fields
 *	of your driver - i.e.

 *	  { TMS32025_INTERNAL_MEMORY_BLOCKS_READ },		// TMS320C25 internal Memory (Read)
 *	  { TMS32025_INTERNAL_MEMORY_BLOCKS_WRITE },	// TMS320C25 internal Memory (Write)
 *	  { TMS32025_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_ROM },
 *	  { TMS32025_PORT_RANGE (TMS32025_BIO, TMS32025_BIO), tms32025_bio_line_r },
 *	  { TMS32025_PORT_RANGE (TMS32025_HOLD, TMS32025_HOLD), tms32025_hold_line_r },
 *	  { TMS32025_PORT_RANGE (TMS32025_HOLDA, TMS32025_HOLDA), tms32025_hold_ack_line_w },
 */



#define TMS32025_BIO		0x100		/* BIO input  */
#define TMS32025_HOLD		0x102		/* HOLD input */
#define TMS32025_HOLDA		0x102		/* HOLD Acknowledge output */
#define TMS32025_XF			0x104		/* XF output  */
#define TMS32025_DR			0x108		/* Serial Data  Receive  input  */
#define TMS32025_DX			0x108		/* Serial Data  Transmit output */
#define TMS32025_CLKR		0x10a		/* Serial Clock Receive  input  */
#define TMS32025_CLKX		0x10a		/* Serial Clock Transmit output */
#define TMS32025_FSR		0x10c		/* Serial Frame Receive  pulse input  */
#define TMS32025_FSX		0x10c		/* Serial Frame Transmit pulse output */

#define TMS32025_DATA_OFFSET	0x00000
#define TMS32025_PGM_OFFSET		0x10000

#define TMS32025_DATA_ADDR_RANGE(start, end)	(((TMS32025_DATA_OFFSET + (start)) << 1)), (((TMS32025_DATA_OFFSET + (end)) << 1) + 1)
#define TMS32025_PRGM_ADDR_RANGE(start, end)	(((TMS32025_PGM_OFFSET + (start)) << 1)), (((TMS32025_PGM_OFFSET + (end)) << 1) + 1)
#define TMS32025_PORT_RANGE(start, end)			((start) << 1), (((end) << 1) + 1)

/* PRGM breaks for ROMs larger than 0x0fff. Needs a rethink on the approach */
#define TMS32025_PRGM_BANK(addr)		((TMS32025_PRGM_BANK[((addr & 0x0f00) >> 8)] | (addr & 0x00ff)) << 1)
#define TMS32025_DATA_BANK(addr)		((TMS32025_DATA_BANK[((addr & 0x0f00) >> 8)] | (addr & 0xf0ff)) << 1)


#define TMS32025_INTERNAL_MEMORY_BLOCKS_READ				 \
	  TMS32025_DATA_ADDR_RANGE(0x0000, 0x0005), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0060, 0x007f), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0200, 0x02ff), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0300, 0x03ff), MRA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xff00, 0xffff), MRA16_RAM

#define TMS32025_INTERNAL_MEMORY_BLOCKS_WRITE				 \
	  TMS32025_DATA_ADDR_RANGE(0x0000, 0x0005), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0060, 0x007f), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0200, 0x02ff), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0300, 0x03ff), MWA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xff00, 0xffff), MWA16_RAM

#define TMS32026_INTERNAL_MEMORY_BLOCKS_READ				 \
	  TMS32025_DATA_ADDR_RANGE(0x0000, 0x0005), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0060, 0x007f), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0200, 0x03ff), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0400, 0x05ff), MRA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0600, 0x07ff), MRA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xfa00, 0xfbff), MRA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xfc00, 0xfdff), MRA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xfe00, 0xffff), MRA16_RAM

#define TMS32026_INTERNAL_MEMORY_BLOCKS_WRITE				 \
	  TMS32025_DATA_ADDR_RANGE(0x0000, 0x0005), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0060, 0x007f), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0200, 0x03ff), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0400, 0x05ff), MWA16_RAM }, \
	{ TMS32025_DATA_ADDR_RANGE(0x0600, 0x07ff), MWA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xfa00, 0xfbff), MWA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xfc00, 0xfdff), MWA16_RAM }, \
	{ TMS32025_PRGM_ADDR_RANGE(0xfe00, 0xffff), MWA16_RAM

/****************************************************************************
 *	Interrupt constants
 */

#define TMS32025_INT0			  0			/* External INT0 */
#define TMS32025_INT1			  1			/* External INT1 */
#define TMS32025_INT2			  2			/* External INT2 */
#define TMS32025_TINT			  3			/* Internal Timer interrupt */
#define TMS32025_RINT			  4			/* Serial Port receive  interrupt */
#define TMS32025_XINT			  5			/* Serial Port transmit interrupt */
#define TMS32025_TRAP			  6			/* Trap instruction */
#define TMS32025_INT_NONE		  -1


enum {
	TMS32025_PC=1,
	TMS32025_PFC,  TMS32025_STR0, TMS32025_STR1, TMS32025_IFR,
	TMS32025_RPTC, TMS32025_ACC,  TMS32025_PREG, TMS32025_TREG,
	TMS32025_AR0,  TMS32025_AR1,  TMS32025_AR2,  TMS32025_AR3,
	TMS32025_AR4,  TMS32025_AR5,  TMS32025_AR6,  TMS32025_AR7,
	TMS32025_STK0, TMS32025_STK1, TMS32025_STK2, TMS32025_STK3,
	TMS32025_STK4, TMS32025_STK5, TMS32025_STK6, TMS32025_STK7,
	TMS32025_DRR,  TMS32025_DXR,  TMS32025_TIM,  TMS32025_PRD,
	TMS32025_IMR,  TMS32025_GREG
};

extern int tms32025_icount;					/* T-state count */



/****************************************************************************
 *	Public Functions
 */

void tms32025_init(void);
void tms32025_reset(void *param);			/* Reset processor & registers	*/
void tms32025_exit(void);					/* Shutdown CPU core			*/
int tms32025_execute(int cycles);			/* Execute cycles T-States -	*/
											/* returns number of cycles actually run */
unsigned tms32025_get_context(void *dst);	/* Get registers				*/
void tms32025_set_context(void *src);		/* Set registers				*/
unsigned tms32025_get_reg(int regnum); 		/* Get specific register		*/
void tms32025_set_reg(int regnum, unsigned val);/* Set specific register	*/
void tms32025_set_irq_line(int irqline, int state);
void tms32025_set_irq_callback(int (*callback)(int irqline));
const char *tms32025_info(void *context, int regnum);
unsigned tms32025_dasm(char *buffer, unsigned pc);



/****************************************************************************
 *	Read the state of a signal pin
 */

#define TMS32025_Signal_In(Signal) (cpu_readport16bew_word(Signal))


/****************************************************************************
 *	Set the state of a signal pin
 */

#define TMS32025_Signal_Out(Signal,Level) (cpu_writeport16bew_word(Signal,Level))



/****************************************************************************
 *	Input a word from given I/O port
 */

#define TMS32025_In(Port) (cpu_readport16bew_word((Port)<<1))


/****************************************************************************
 *	Output a word to given I/O port.
 */

#define TMS32025_Out(Port,Value) (cpu_writeport16bew_word(((Port)<<1),Value))



/****************************************************************************
 *	Read a word from given ROM memory location.
 *	The following adds 20000h to the address, since MAME doesn't support
 *	RAM and ROM living in the same address space. RAM really starts at
 *	address 0 , and data is in word entities only.
 */

/* #define TMS32025_ROM_RDMEM(A) (cpu_readmem18bew_word((A<<1)+TMS32025_PGM_OFFSET))*/
#define TMS32025_ROM_RDMEM(A) (cpu_readmem18bew_word(TMS32025_PRGM_BANK(A)))

/****************************************************************************
 *	Write a word to given ROM memory location.
 *	The following adds 20000h to the address, since MAME doesn't support
 *	RAM and ROM living in the same address space. ROM really starts at
 *	address 0 , and data is in word entities only.
 */

/* #define TMS32025_ROM_WRMEM(A,V) (cpu_writemem18bew_word((A<<1)+TMS32025_PGM_OFFSET,V))*/
#define TMS32025_ROM_WRMEM(A,V) (cpu_writemem18bew_word(TMS32025_PRGM_BANK(A),V))



/****************************************************************************
 *	Read a 16 bit Opcode.
 */

/* #define TMS32025_RDOP(A) (cpu_readop16((A<<1)+TMS32025_PGM_OFFSET))*/
#define TMS32025_RDOP(A) (cpu_readop16(TMS32025_PRGM_BANK(A)))


/****************************************************************************
 *	TMS32025_RDOP_ARG() is identical to TMS32025_RDOP(), except it is used
 *	for reading opcode arguments.
 */

/* #define TMS32025_RDOP_ARG(A) (cpu_readop_arg16((A<<1)+TMS32025_PGM_OFFSET))*/
#define TMS32025_RDOP_ARG(A) (cpu_readop_arg16(TMS32025_PRGM_BANK(A)))



/****************************************************************************
 *	Read a word from given RAM memory location.
 */

/* #define TMS32025_RAM_RDMEM(A) (cpu_readmem18bew_word((A<<1)+TMS32025_DATA_OFFSET))*/
#define TMS32025_RAM_RDMEM(A) (cpu_readmem18bew_word(TMS32025_DATA_BANK(A)))


/****************************************************************************
 *	Write a word to given RAM memory location.
 */
/* #define TMS32025_RAM_WRMEM(A,V) (cpu_writemem18bew_word((A<<1)+TMS32025_DATA_OFFSET,V))*/
#define TMS32025_RAM_WRMEM(A,V) (cpu_writemem18bew_word(TMS32025_DATA_BANK(A),V))



#ifdef	MAME_DEBUG
extern unsigned Dasm32025(char *buffer, unsigned pc);
#endif

#endif	/* _TMS32025_H */
