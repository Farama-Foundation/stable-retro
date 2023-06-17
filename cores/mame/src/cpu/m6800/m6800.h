/*** m6800: Portable 6800 class emulator *************************************/

#ifndef _M6800_H
#define _M6800_H

#include "osd_cpu.h"
#include "memory.h"
#include "cpuintrf.h"

enum {
	M6800_PC=1, M6800_S, M6800_A, M6800_B, M6800_X, M6800_CC,
	M6800_WAI_STATE, M6800_NMI_STATE, M6800_IRQ_STATE };

#define M6800_WAI		8			/* set when WAI is waiting for an interrupt */
#define M6800_SLP		0x10		/* HD63701 only */


#define M6800_IRQ_LINE	0			/* IRQ line number */
#define M6800_TIN_LINE	1			/* P20/Tin Input Capture line (eddge sense)     */
									/* Active eddge is selecrable by internal reg.  */
									/* raise eddge : CLEAR_LINE  -> ASSERT_LINE     */
									/* fall  eddge : ASSERT_LINE -> CLEAR_LINE      */
									/* it is usuali to use PULSE_LINE state         */
/* PUBLIC GLOBALS */
extern int m6800_ICount;

/* PUBLIC FUNCTIONS */
void m6800_init(void);
void m6800_reset(void *param);
void m6800_exit(void);
int	m6800_execute(int cycles);
unsigned m6800_get_context(void *dst);
void m6800_set_context(void *src);
unsigned m6800_get_reg(int regnum);
void m6800_set_reg(int regnum, unsigned val);
void m6800_set_irq_line(int irqline, int state);
void m6800_set_irq_callback(int (*callback)(int irqline));
const char *m6800_info(void *context, int regnum);
unsigned m6800_dasm(char *buffer, unsigned pc);

/****************************************************************************
 * For now make the 6801 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_M6801)
#define M6801_A 					M6800_A
#define M6801_B 					M6800_B
#define M6801_PC					M6800_PC
#define M6801_S 					M6800_S
#define M6801_X 					M6800_X
#define M6801_CC					M6800_CC
#define M6801_WAI_STATE 			M6800_WAI_STATE
#define M6801_NMI_STATE 			M6800_NMI_STATE
#define M6801_IRQ_STATE 			M6800_IRQ_STATE

#define M6801_WAI					M6800_WAI
#define M6801_IRQ_LINE				M6800_IRQ_LINE

#define m6801_ICount				m6800_ICount
void m6801_init(void);
void m6801_reset(void *param);
void m6801_exit(void);
int	m6801_execute(int cycles);
unsigned m6801_get_context(void *dst);
void m6801_set_context(void *src);
unsigned m6801_get_reg(int regnum);
void m6801_set_reg(int regnum, unsigned val);
void m6801_set_irq_line(int irqline, int state);
void m6801_set_irq_callback(int (*callback)(int irqline));
const char *m6801_info(void *context, int regnum);
unsigned m6801_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the 6802 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_M6802)
#define M6802_A 					M6800_A
#define M6802_B 					M6800_B
#define M6802_PC					M6800_PC
#define M6802_S 					M6800_S
#define M6802_X 					M6800_X
#define M6802_CC					M6800_CC
#define M6802_WAI_STATE 			M6800_WAI_STATE
#define M6802_NMI_STATE 			M6800_NMI_STATE
#define M6802_IRQ_STATE 			M6800_IRQ_STATE

#define M6802_WAI					M6800_WAI
#define M6802_IRQ_LINE				M6800_IRQ_LINE

#define m6802_ICount				m6800_ICount
void m6802_init(void);
void m6802_reset(void *param);
void m6802_exit(void);
int	m6802_execute(int cycles);
unsigned m6802_get_context(void *dst);
void m6802_set_context(void *src);
unsigned m6802_get_reg(int regnum);
void m6802_set_reg(int regnum, unsigned val);
void m6802_set_irq_line(int irqline, int state);
void m6802_set_irq_callback(int (*callback)(int irqline));
const char *m6802_info(void *context, int regnum);
unsigned m6802_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the 6803 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_M6803)
#define M6803_A 					M6800_A
#define M6803_B 					M6800_B
#define M6803_PC					M6800_PC
#define M6803_S 					M6800_S
#define M6803_X 					M6800_X
#define M6803_CC					M6800_CC
#define M6803_WAI_STATE 			M6800_WAI_STATE
#define M6803_NMI_STATE 			M6800_NMI_STATE
#define M6803_IRQ_STATE 			M6800_IRQ_STATE

#define M6803_WAI					M6800_WAI
#define M6803_IRQ_LINE				M6800_IRQ_LINE
#define M6803_TIN_LINE				M6800_TIN_LINE

#define m6803_ICount				m6800_ICount
void m6803_init(void);
void m6803_reset(void *param);
void m6803_exit(void);
int	m6803_execute(int cycles);
unsigned m6803_get_context(void *dst);
void m6803_set_context(void *src);
unsigned m6803_get_reg(int regnum);
void m6803_set_reg(int regnum, unsigned val);
void m6803_set_irq_line(int irqline, int state);
void m6803_set_irq_callback(int (*callback)(int irqline));
const char *m6803_info(void *context, int regnum);
unsigned m6803_dasm(char *buffer, unsigned pc);
#endif

#if (HAS_M6803||HAS_HD63701)
/* By default, on a port write port bits which are not set as output in the DDR */
/* are set to the value returned by a read from the same port. If you need to */
/* know the DDR for e.g. port 1, do m6803_internal_registers_r(M6801_DDR1) */

#define M6803_DDR1	0x00
#define M6803_DDR2	0x01

#define M6803_PORT1 0x100
#define M6803_PORT2 0x101
READ_HANDLER( m6803_internal_registers_r );
WRITE_HANDLER( m6803_internal_registers_w );
#endif

/****************************************************************************
 * For now make the 6808 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_M6808)
#define M6808_A 					M6800_A
#define M6808_B 					M6800_B
#define M6808_PC					M6800_PC
#define M6808_S 					M6800_S
#define M6808_X 					M6800_X
#define M6808_CC					M6800_CC
#define M6808_WAI_STATE 			M6800_WAI_STATE
#define M6808_NMI_STATE 			M6800_NMI_STATE
#define M6808_IRQ_STATE 			M6800_IRQ_STATE

#define M6808_WAI                   M6800_WAI
#define M6808_IRQ_LINE              M6800_IRQ_LINE

#define m6808_ICount                m6800_ICount
void m6808_init(void);
void m6808_reset(void *param);
void m6808_exit(void);
int	m6808_execute(int cycles);
unsigned m6808_get_context(void *dst);
void m6808_set_context(void *src);
unsigned m6808_get_reg(int regnum);
void m6808_set_reg(int regnum, unsigned val);
void m6808_set_irq_line(int irqline, int state);
void m6808_set_irq_callback(int (*callback)(int irqline));
const char *m6808_info(void *context, int regnum);
unsigned m6808_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * For now make the HD63701 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_HD63701)
#define HD63701_A					 M6800_A
#define HD63701_B					 M6800_B
#define HD63701_PC					 M6800_PC
#define HD63701_S					 M6800_S
#define HD63701_X					 M6800_X
#define HD63701_CC					 M6800_CC
#define HD63701_WAI_STATE			 M6800_WAI_STATE
#define HD63701_NMI_STATE			 M6800_NMI_STATE
#define HD63701_IRQ_STATE			 M6800_IRQ_STATE

#define HD63701_WAI 				 M6800_WAI
#define HD63701_SLP 				 M6800_SLP
#define HD63701_IRQ_LINE			 M6800_IRQ_LINE
#define HD63701_TIN_LINE			 M6800_TIN_LINE

#define hd63701_ICount				 m6800_ICount
void hd63701_init(void);
void hd63701_reset(void *param);
void hd63701_exit(void);
int	hd63701_execute(int cycles);
unsigned hd63701_get_context(void *dst);
void hd63701_set_context(void *src);
unsigned hd63701_get_reg(int regnum);
void hd63701_set_reg(int regnum, unsigned val);
void hd63701_set_irq_line(int irqline, int state);
void hd63701_set_irq_callback(int (*callback)(int irqline));
const char *hd63701_info(void *context, int regnum);
unsigned hd63701_dasm(char *buffer, unsigned pc);

void hd63701_trap_pc(void);

#define HD63701_DDR1 M6803_DDR1
#define HD63701_DDR2 M6803_DDR2

#define HD63701_PORT1 M6803_PORT1
#define HD63701_PORT2 M6803_PORT2

READ_HANDLER( hd63701_internal_registers_r );
WRITE_HANDLER( hd63701_internal_registers_w );

#endif

/****************************************************************************
 * For now make the NSC8105 using the m6800 variables and functions
 ****************************************************************************/
#if (HAS_NSC8105)
#define NSC8105_A					 M6800_A
#define NSC8105_B					 M6800_B
#define NSC8105_PC					 M6800_PC
#define NSC8105_S					 M6800_S
#define NSC8105_X					 M6800_X
#define NSC8105_CC					 M6800_CC
#define NSC8105_WAI_STATE			 M6800_WAI_STATE
#define NSC8105_NMI_STATE			 M6800_NMI_STATE
#define NSC8105_IRQ_STATE			 M6800_IRQ_STATE

#define NSC8105_WAI 				 M6800_WAI
#define NSC8105_IRQ_LINE			 M6800_IRQ_LINE
#define NSC8105_TIN_LINE			 M6800_TIN_LINE

#define nsc8105_ICount				 m6800_ICount
void nsc8105_init(void);
void nsc8105_reset(void *param);
void nsc8105_exit(void);
int	nsc8105_execute(int cycles);
unsigned nsc8105_get_context(void *dst);
void nsc8105_set_context(void *src);
unsigned nsc8105_get_reg(int regnum);
void nsc8105_set_reg(int regnum, unsigned val);
void nsc8105_set_irq_line(int irqline, int state);
void nsc8105_set_irq_callback(int (*callback)(int irqline));
const char *nsc8105_info(void *context, int regnum);
unsigned nsc8105_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6800_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6800_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* M6800_RDOP() is identical to M6800_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6800_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* M6800_RDOP_ARG() is identical to M6800_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6800_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef	MAME_DEBUG
unsigned Dasm680x(int subtype, char *buf, unsigned pc);
#endif

#endif /* _M6800_H */
