/*** konami: Portable Konami cpu emulator ******************************************/

#ifndef _KONAMI_H
#define _KONAMI_H

#include "memory.h"
#include "osd_cpu.h"

enum {
	KONAMI_PC=1, KONAMI_S, KONAMI_CC ,KONAMI_A, KONAMI_B, KONAMI_U, KONAMI_X, KONAMI_Y,
	KONAMI_DP, KONAMI_NMI_STATE, KONAMI_IRQ_STATE, KONAMI_FIRQ_STATE };

#define KONAMI_IRQ_LINE	0	/* IRQ line number */
#define KONAMI_FIRQ_LINE 1   /* FIRQ line number */

/* PUBLIC GLOBALS */
extern int  konami_ICount;
extern void (*konami_cpu_setlines_callback)( int lines ); /* callback called when A16-A23 are set */

/* PUBLIC FUNCTIONS */
extern void konami_init(void);
extern void konami_reset(void *param);
extern void konami_exit(void);
extern int konami_execute(int cycles);  /* NS 970908 */
extern unsigned konami_get_context(void *dst);
extern void konami_set_context(void *src);
extern unsigned konami_get_reg(int regnum);
extern void konami_set_reg(int regnum, unsigned val);
extern void konami_set_irq_line(int irqline, int state);
extern void konami_set_irq_callback(int (*callback)(int irqline));
extern const char *konami_info(void *context,int regnum);
extern unsigned konami_dasm(char *buffer, unsigned pc);

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
#define KONAMI_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define KONAMI_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* Z80_RDOP() is identical to Z80_RDMEM() except it is used for reading     */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define KONAMI_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* Z80_RDOP_ARG() is identical to Z80_RDOP() except it is used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define KONAMI_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef MAME_DEBUG
extern unsigned Dasmknmi (char *buffer, unsigned pc);
#endif

#endif /* _KONAMI_H */
