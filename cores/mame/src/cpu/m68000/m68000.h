#ifndef M68000__HEADER
#define M68000__HEADER

#include "osd_cpu.h"
#include "mamedbg.h"

enum
{
	/* NOTE: M68K_SP fetches the current SP, be it USP, ISP, or MSP */
	M68K_PC=1, M68K_SP, M68K_ISP, M68K_USP, M68K_MSP, M68K_SR, M68K_VBR,
	M68K_SFC, M68K_DFC, M68K_CACR, M68K_CAAR, M68K_PREF_ADDR, M68K_PREF_DATA,
	M68K_D0, M68K_D1, M68K_D2, M68K_D3, M68K_D4, M68K_D5, M68K_D6, M68K_D7,
	M68K_A0, M68K_A1, M68K_A2, M68K_A3, M68K_A4, M68K_A5, M68K_A6, M68K_A7
};

extern int m68k_ICount;

/* Redirect memory calls */

struct m68k_memory_interface
{
	offs_t		opcode_xor;						/* Address Calculation */
	data8_t		(*read8)(offs_t);				/* Normal read 8 bit */
	data16_t	(*read16)(offs_t);				/* Normal read 16 bit */
	data32_t	(*read32)(offs_t);				/* Normal read 32 bit */
	void		(*write8)(offs_t, data8_t);		/* Write 8 bit */
	void		(*write16)(offs_t, data16_t);	/* Write 16 bit */
	void		(*write32)(offs_t, data32_t);	/* Write 32 bit */
	void		(*changepc)(offs_t);			/* Change PC routine */

    /* For Encrypted Stuff */

	data8_t		(*read8pc)(offs_t);				/* PC Relative read 8 bit */
	data16_t	(*read16pc)(offs_t);			/* PC Relative read 16 bit */
	data32_t	(*read32pc)(offs_t);			/* PC Relative read 32 bit */

	data16_t	(*read16d)(offs_t);				/* Direct read 16 bit */
	data32_t	(*read32d)(offs_t);				/* Direct read 32 bit */
};

struct m68k_encryption_interface
{
	data8_t		(*read8pc)(offs_t);				/* PC Relative read 8 bit */
	data16_t	(*read16pc)(offs_t);			/* PC Relative read 16 bit */
	data32_t	(*read32pc)(offs_t);			/* PC Relative read 32 bit */

	data16_t	(*read16d)(offs_t);				/* Direct read 16 bit */
	data32_t	(*read32d)(offs_t);				/* Direct read 32 bit */
};

/* The MAME API for MC68000 */

#define MC68000_IRQ_1    1
#define MC68000_IRQ_2    2
#define MC68000_IRQ_3    3
#define MC68000_IRQ_4    4
#define MC68000_IRQ_5    5
#define MC68000_IRQ_6    6
#define MC68000_IRQ_7    7

#define MC68000_INT_ACK_AUTOVECTOR    -1
#define MC68000_INT_ACK_SPURIOUS      -2

#define m68000_ICount                   m68k_ICount
extern void m68000_init(void);
extern void m68000_reset(void *param);
extern void m68000_exit(void);
extern int	m68000_execute(int cycles);
extern unsigned m68000_get_context(void *dst);
extern void m68000_set_context(void *src);
extern unsigned m68000_get_reg(int regnum);
extern void m68000_set_reg(int regnum, unsigned val);
extern void m68000_set_irq_line(int irqline, int state);
extern void m68000_set_irq_callback(int (*callback)(int irqline));
extern const char *m68000_info(void *context, int regnum);
extern unsigned m68000_dasm(char *buffer, unsigned pc);
extern void m68000_memory_interface_set(int Entry,void * memory_routine);

/****************************************************************************
 * M68010 section
 ****************************************************************************/
#if HAS_M68010
#define MC68010_IRQ_1					MC68000_IRQ_1
#define MC68010_IRQ_2					MC68000_IRQ_2
#define MC68010_IRQ_3					MC68000_IRQ_3
#define MC68010_IRQ_4					MC68000_IRQ_4
#define MC68010_IRQ_5					MC68000_IRQ_5
#define MC68010_IRQ_6					MC68000_IRQ_6
#define MC68010_IRQ_7					MC68000_IRQ_7
#define MC68010_INT_ACK_AUTOVECTOR		MC68000_INT_ACK_AUTOVECTOR
#define MC68010_INT_ACK_SPURIOUS		MC68000_INT_ACK_SPURIOUS

#define m68010_ICount                   m68k_ICount
extern void m68010_init(void);
extern void m68010_reset(void *param);
extern void m68010_exit(void);
extern int	m68010_execute(int cycles);
extern unsigned m68010_get_context(void *dst);
extern void m68010_set_context(void *src);
extern unsigned m68010_get_reg(int regnum);
extern void m68010_set_reg(int regnum, unsigned val);
extern void m68010_set_irq_line(int irqline, int state);
extern void m68010_set_irq_callback(int (*callback)(int irqline));
const char *m68010_info(void *context, int regnum);
extern unsigned m68010_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * M68EC020 section
 ****************************************************************************/
#if HAS_M68EC020
#define MC68EC020_IRQ_1					MC68000_IRQ_1
#define MC68EC020_IRQ_2					MC68000_IRQ_2
#define MC68EC020_IRQ_3					MC68000_IRQ_3
#define MC68EC020_IRQ_4					MC68000_IRQ_4
#define MC68EC020_IRQ_5					MC68000_IRQ_5
#define MC68EC020_IRQ_6					MC68000_IRQ_6
#define MC68EC020_IRQ_7					MC68000_IRQ_7
#define MC68EC020_INT_ACK_AUTOVECTOR	MC68000_INT_ACK_AUTOVECTOR
#define MC68EC020_INT_ACK_SPURIOUS		MC68000_INT_ACK_SPURIOUS

#define m68ec020_ICount                 m68k_ICount
extern void m68ec020_init(void);
extern void m68ec020_reset(void *param);
extern void m68ec020_exit(void);
extern int	m68ec020_execute(int cycles);
extern unsigned m68ec020_get_context(void *dst);
extern void m68ec020_set_context(void *src);
extern unsigned m68ec020_get_reg(int regnum);
extern void m68ec020_set_reg(int regnum, unsigned val);
extern void m68ec020_set_irq_line(int irqline, int state);
extern void m68ec020_set_irq_callback(int (*callback)(int irqline));
const char *m68ec020_info(void *context, int regnum);
extern unsigned m68ec020_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * M68020 section
 ****************************************************************************/
#if HAS_M68020
#define MC68020_IRQ_1					MC68000_IRQ_1
#define MC68020_IRQ_2					MC68000_IRQ_2
#define MC68020_IRQ_3					MC68000_IRQ_3
#define MC68020_IRQ_4					MC68000_IRQ_4
#define MC68020_IRQ_5					MC68000_IRQ_5
#define MC68020_IRQ_6					MC68000_IRQ_6
#define MC68020_IRQ_7					MC68000_IRQ_7
#define MC68020_INT_ACK_AUTOVECTOR		MC68000_INT_ACK_AUTOVECTOR
#define MC68020_INT_ACK_SPURIOUS		MC68000_INT_ACK_SPURIOUS

#define m68020_ICount                   m68k_ICount
extern void m68020_init(void);
extern void m68020_reset(void *param);
extern void m68020_exit(void);
extern int	m68020_execute(int cycles);
extern unsigned m68020_get_context(void *dst);
extern void m68020_set_context(void *src);
extern unsigned m68020_get_reg(int regnum);
extern void m68020_set_reg(int regnum, unsigned val);
extern void m68020_set_irq_line(int irqline, int state);
extern void m68020_set_irq_callback(int (*callback)(int irqline));
const char *m68020_info(void *context, int regnum);
extern unsigned m68020_dasm(char *buffer, unsigned pc);
#endif

/* C Core header */
#include "m68kmame.h"

#endif /* M68000__HEADER */
