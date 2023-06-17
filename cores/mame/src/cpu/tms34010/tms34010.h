/*###################################################################################################
**
**	TMS34010: Portable Texas Instruments TMS34010 emulator
**
**	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
**	 Parts based on code by Aaron Giles
**
**#################################################################################################*/

#ifndef _TMS34010_H
#define _TMS34010_H

#include "osd_cpu.h"


/* the TMS34010 input clock is divided by 8; the 34020 by 4 */
#define TMS34010_CLOCK_DIVIDER		8
#define TMS34020_CLOCK_DIVIDER		4


/* register indexes for get_reg and set_reg */
enum
{
	TMS34010_PC = 1,
	TMS34010_SP,
	TMS34010_ST,
	TMS34010_A0,
	TMS34010_A1,
	TMS34010_A2,
	TMS34010_A3,
	TMS34010_A4,
	TMS34010_A5,
	TMS34010_A6,
	TMS34010_A7,
	TMS34010_A8,
	TMS34010_A9,
	TMS34010_A10,
	TMS34010_A11,
	TMS34010_A12,
	TMS34010_A13,
	TMS34010_A14,
	TMS34010_B0,
	TMS34010_B1,
	TMS34010_B2,
	TMS34010_B3,
	TMS34010_B4,
	TMS34010_B5,
	TMS34010_B6,
	TMS34010_B7,
	TMS34010_B8,
	TMS34010_B9,
	TMS34010_B10,
	TMS34010_B11,
	TMS34010_B12,
	TMS34010_B13,
	TMS34010_B14
};


/* Configuration structure */
struct tms34010_config
{
	UINT8	halt_on_reset;						/* /HCS pin, which determines HALT state after reset */
	void	(*output_int)(int state);			/* output interrupt callback */
	void	(*to_shiftreg)(offs_t, data16_t *);	/* shift register write */
	void	(*from_shiftreg)(offs_t, data16_t *);/* shift register read */
	void	(*display_addr_changed)(UINT32 offs, int rowbytes, int scanline);/* display address changed */
	void	(*display_int_callback)(int scanline);/* display interrupt callback */
};


/* PUBLIC FUNCTIONS - 34010 */
void		tms34010_init(void);
void		tms34010_reset(void *param);
void		tms34010_exit(void);
int			tms34010_execute(int cycles);
unsigned	tms34010_get_context(void *dst);
void		tms34010_set_context(void *src);
unsigned	tms34010_get_reg(int regnum);
void		tms34010_set_reg(int regnum, unsigned val);
void 		tms34010_set_irq_line(int irqline, int linestate);
void 		tms34010_set_irq_callback(int (*callback)(int irqline));
void 		tms34010_internal_interrupt(int type);
const char *tms34010_info(void *context, int regnum);
unsigned 	tms34010_dasm(char *buffer, unsigned pc);

int 		tms34010_io_display_blanked(int cpu);
int 		tms34010_get_DPYSTRT(int cpu);


/* PUBLIC FUNCTIONS - 34020 */
void		tms34020_init(void);
void 		tms34020_reset(void *param);
void 		tms34020_exit(void);
int			tms34020_execute(int cycles);
unsigned 	tms34020_get_context(void *dst);
void 		tms34020_set_context(void *src);
unsigned 	tms34020_get_reg(int regnum);
void 		tms34020_set_reg(int regnum, unsigned val);
void 		tms34020_set_irq_line(int irqline, int linestate);
void 		tms34020_set_irq_callback(int (*callback)(int irqline));
void 		tms34020_internal_interrupt(int type);
const char *tms34020_info(void *context, int regnum);
unsigned 	tms34020_dasm(char *buffer, unsigned pc);

int 		tms34020_io_display_blanked(int cpu);
int 		tms34020_get_DPYSTRT(int cpu);


/* Host control interface */
#define TMS34010_HOST_ADDRESS_L		0
#define TMS34010_HOST_ADDRESS_H		1
#define TMS34010_HOST_DATA			2
#define TMS34010_HOST_CONTROL		3

void		tms34010_host_w(int cpunum, int reg, int data);
int			tms34010_host_r(int cpunum, int reg);


/* Reads & writes to the 34010 I/O registers; place at TOBYTE(0xc0000000) */
WRITE16_HANDLER( tms34010_io_register_w );
READ16_HANDLER( tms34010_io_register_r );

/* Reads & writes to the 34020 I/O registers; place at TOBYTE(0xc0000000) */
WRITE16_HANDLER( tms34020_io_register_w );
READ16_HANDLER( tms34020_io_register_r );


/* PUBLIC GLOBALS */
extern int tms34010_ICount;
#define tms34020_ICount tms34010_ICount


/* Use this macro in the memory definitions to specify bit-based addresses */
#define TOBYTE(bitaddr) ((offs_t)(bitaddr) >> 3)
#define TOWORD(bitaddr) ((offs_t)(bitaddr) >> 4)


#ifdef MAME_DEBUG
unsigned Dasm34010(char *buff, unsigned _pc);
unsigned Dasm34020(char *buff, unsigned _pc);
#endif

#endif /* _TMS34010_H */
