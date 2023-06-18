/*** m6805: Portable 6805 emulator ******************************************/

#ifndef _M6805_H
#define _M6805_H

#include "memory.h"
#include "osd_cpu.h"

enum { M6805_PC=1, M6805_S, M6805_CC, M6805_A, M6805_X, M6805_IRQ_STATE };

#define M6805_IRQ_LINE		0

/* PUBLIC GLOBALS */
extern int  m6805_ICount;

/* PUBLIC FUNCTIONS */
extern void m6805_init(void);
extern void m6805_reset(void *param);
extern void m6805_exit(void);
extern int	m6805_execute(int cycles);
extern unsigned m6805_get_context(void *dst);
extern void m6805_set_context(void *src);
extern unsigned m6805_get_reg(int regnum);
extern void m6805_set_reg(int regnum, unsigned val);
extern void m6805_set_irq_line(int irqline, int state);
extern void m6805_set_irq_callback(int (*callback)(int irqline));
extern const char *m6805_info(void *context, int regnum);
extern unsigned m6805_dasm(char *buffer, unsigned pc);

/****************************************************************************
 * 68705 section
 ****************************************************************************/
#if (HAS_M68705)
#define M68705_A					M6805_A
#define M68705_PC					M6805_PC
#define M68705_S					M6805_S
#define M68705_X					M6805_X
#define M68705_CC					M6805_CC
#define M68705_IRQ_STATE			M6805_IRQ_STATE

#define M68705_IRQ_LINE				M6805_IRQ_LINE

#define m68705_ICount				m6805_ICount
extern void m68705_init(void);
extern void m68705_reset(void *param);
extern void m68705_exit(void);
extern int	m68705_execute(int cycles);
extern unsigned m68705_get_context(void *dst);
extern void m68705_set_context(void *src);
extern unsigned m68705_get_reg(int regnum);
extern void m68705_set_reg(int regnum, unsigned val);
extern void m68705_set_irq_line(int irqline, int state);
extern void m68705_set_irq_callback(int (*callback)(int irqline));
extern const char *m68705_info(void *context, int regnum);
extern unsigned m68705_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * HD63705 section
 ****************************************************************************/
#if (HAS_HD63705)
#define HD63705_A					M6805_A
#define HD63705_PC					M6805_PC
#define HD63705_S					M6805_S
#define HD63705_X					M6805_X
#define HD63705_CC					M6805_CC
#define HD63705_NMI_STATE			M6805_IRQ_STATE
#define HD63705_IRQ1_STATE			M6805_IRQ_STATE+1
#define HD63705_IRQ2_STATE			M6805_IRQ_STATE+2
#define HD63705_ADCONV_STATE		M6805_IRQ_STATE+3

#define HD63705_INT_MASK			0x1ff

#define HD63705_INT_IRQ1			0x00
#define HD63705_INT_IRQ2			0x01
#define	HD63705_INT_TIMER1			0x02
#define	HD63705_INT_TIMER2			0x03
#define	HD63705_INT_TIMER3			0x04
#define	HD63705_INT_PCI				0x05
#define	HD63705_INT_SCI				0x06
#define	HD63705_INT_ADCONV			0x07
#define HD63705_INT_NMI				0x08

#define hd63705_ICount				m6805_ICount
extern void hd63705_init(void);
extern void hd63705_reset(void *param);
extern void hd63705_exit(void);
extern int	hd63705_execute(int cycles);
extern unsigned hd63705_get_context(void *dst);
extern void hd63705_set_context(void *src);
extern unsigned hd63705_get_reg(int regnum);
extern void hd63705_set_reg(int regnum, unsigned val);
extern void hd63705_set_irq_line(int irqline, int state);
extern void hd63705_set_irq_callback(int (*callback)(int irqline));
extern const char *hd63705_info(void *context, int regnum);
extern unsigned hd63705_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6805_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6805_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* M6805_RDOP() is identical to M6805_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6805_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* M6805_RDOP_ARG() is identical to M6805_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6805_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef MAME_DEBUG
extern unsigned Dasm6805(char *buf, unsigned pc);
#endif

#endif /* _M6805_H */
