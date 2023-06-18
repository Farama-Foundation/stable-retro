/*###################################################################################################
**
**
**		tms32031.h
**		Interface file for the portable TMS32C031 emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef _TMS32031_H
#define _TMS32031_H

#include "memory.h"
#include "osd_cpu.h"


/*###################################################################################################
**	TYPE DEFINITIONS
**#################################################################################################*/

struct tms32031_config
{
	UINT32		bootoffset;
	void		(*xf0_w)(UINT8 val);
	void		(*xf1_w)(UINT8 val);
};


/*###################################################################################################
**	REGISTER ENUMERATION
**#################################################################################################*/

enum
{
	TMS32031_PC=1,
	TMS32031_R0,TMS32031_R1,TMS32031_R2,TMS32031_R3,
	TMS32031_R4,TMS32031_R5,TMS32031_R6,TMS32031_R7,
	TMS32031_R0F,TMS32031_R1F,TMS32031_R2F,TMS32031_R3F,
	TMS32031_R4F,TMS32031_R5F,TMS32031_R6F,TMS32031_R7F,
	TMS32031_AR0,TMS32031_AR1,TMS32031_AR2,TMS32031_AR3,
	TMS32031_AR4,TMS32031_AR5,TMS32031_AR6,TMS32031_AR7,
	TMS32031_DP,TMS32031_IR0,TMS32031_IR1,TMS32031_BK,
	TMS32031_SP,TMS32031_ST,TMS32031_IE,TMS32031_IF,
	TMS32031_IOF,TMS32031_RS,TMS32031_RE,TMS32031_RC
};


/*###################################################################################################
**	INTERRUPT CONSTANTS
**#################################################################################################*/

#define TMS32031_IRQ0		0		/* IRQ0 */
#define TMS32031_IRQ1		1		/* IRQ1 */
#define TMS32031_IRQ2		2		/* IRQ2 */
#define TMS32031_IRQ3		3		/* IRQ3 */
#define TMS32031_XINT0		4		/* serial 0 transmit interrupt */
#define TMS32031_RINT0		5		/* serial 0 receive interrupt */
#define TMS32031_XINT1		6		/* serial 1 transmit interrupt */
#define TMS32031_RINT1		7		/* serial 1 receive interrupt */
#define TMS32031_TINT0		8		/* timer 0 interrupt */
#define TMS32031_TINT1		9		/* timer 1 interrupt */
#define TMS32031_DINT		10		/* DMA interrupt */


/*###################################################################################################
**	PUBLIC GLOBALS
**#################################################################################################*/

extern int tms32031_icount;


/*###################################################################################################
**	PUBLIC FUNCTIONS
**#################################################################################################*/

extern void tms32031_init(void);
extern void tms32031_reset(void *param);
extern void tms32031_exit(void);
extern int tms32031_execute(int cycles);
extern unsigned tms32031_get_context(void *dst);
extern void tms32031_set_context(void *src);
extern unsigned tms32031_get_reg(int regnum);
extern void tms32031_set_reg(int regnum, unsigned val);
extern void tms32031_set_irq_line(int irqline, int state);
extern void tms32031_set_irq_callback(int (*callback)(int irqline));
extern const char *tms32031_info(void *context, int regnum);

extern unsigned tms32031_dasm(char *buffer, unsigned pc);

#endif /* _TMS32031_H */
