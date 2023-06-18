/*****************************************************************************
 *
 *	 m6502.h
 *	 Portable 6502/65c02/65sc02/6510/n2a03 emulator interface
 *
 *	 Copyright (c) 1998,1999,2000 Juergen Buchmueller, all rights reserved.
 *	 65sc02 core Copyright (c) 2000 Peter Trauner.
 *	 Deco16 portions Copyright (c) 2001 Bryan McPhail.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
/* 2.February 2000 PeT added 65sc02 subtype */

#ifndef _M6502_H
#define _M6502_H

#include "cpuintrf.h"
#include "osd_cpu.h"

#ifdef RUNTIME_LOADER
# ifdef __cplusplus
	extern "C" void m6502_runtime_loader_init(void);
# else
	extern void m6502_runtime_loader_init(void);
# endif
#endif

/* set to 1 to test cur_mrhard/cur_wmhard to avoid calls */
#define FAST_MEMORY 0

#define SUBTYPE_6502	0
#if (HAS_M65C02)
#define SUBTYPE_65C02	1
#endif
#if (HAS_M6510)
#define SUBTYPE_6510	2
#endif
#if (HAS_N2A03)
#define SUBTYPE_2A03	3
#endif
#if (HAS_M65SC02)
#define SUBTYPE_65SC02	4
#endif
#if (HAS_DECO16)
#define SUBTYPE_DECO16	5
#endif

enum {
	M6502_PC=1, M6502_S, M6502_P, M6502_A, M6502_X, M6502_Y,
	M6502_EA, M6502_ZP, M6502_NMI_STATE, M6502_IRQ_STATE, M6502_SO_STATE,
	M6502_SUBTYPE
};

#define M6502_IRQ_LINE		0
/* use cpu_set_irq_line(cpu, M6502_SET_OVERFLOW, level)
   to change level of the so input line
   positiv edge sets overflow flag */
#define M6502_SET_OVERFLOW	1

extern int m6502_ICount;				/* cycle count */

extern void m6502_init(void);
extern void m6502_reset(void *param);
extern void m6502_exit(void);
extern int	m6502_execute(int cycles);
extern unsigned m6502_get_context(void *dst);
extern void m6502_set_context(void *src);
extern unsigned m6502_get_reg(int regnum);
extern void m6502_set_reg(int regnum, unsigned val);
extern void m6502_set_irq_line(int irqline, int state);
extern void m6502_set_irq_callback(int (*callback)(int irqline));
extern const char *m6502_info(void *context, int regnum);
extern unsigned m6502_dasm(char *buffer, unsigned pc);

/****************************************************************************
 * The 6510
 ****************************************************************************/
#if (HAS_M6510)
#define M6510_A 						M6502_A
#define M6510_X 						M6502_X
#define M6510_Y 						M6502_Y
#define M6510_S 						M6502_S
#define M6510_PC						M6502_PC
#define M6510_P 						M6502_P
#define M6510_EA						M6502_EA
#define M6510_ZP						M6502_ZP
#define M6510_NMI_STATE 				M6502_NMI_STATE
#define M6510_IRQ_STATE 				M6502_IRQ_STATE

#define M6510_IRQ_LINE					M6502_IRQ_LINE

#define m6510_ICount					m6502_ICount

extern void m6510_init(void);
extern void m6510_reset(void *param);
extern void m6510_exit(void);
extern int	m6510_execute(int cycles);
extern unsigned m6510_get_context(void *dst);
extern void m6510_set_context(void *src);
extern unsigned m6510_get_reg(int regnum);
extern void m6510_set_reg(int regnum, unsigned val);
extern void m6510_set_irq_line(int irqline, int state);
extern void m6510_set_irq_callback(int (*callback)(int irqline));
extern const char *m6510_info(void *context, int regnum);
extern unsigned m6510_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned int Dasm6510( char *dst, unsigned pc );
#endif

#endif

#ifdef HAS_M6510T
#define M6510T_A						M6502_A
#define M6510T_X						M6502_X
#define M6510T_Y						M6502_Y
#define M6510T_S						M6502_S
#define M6510T_PC						M6502_PC
#define M6510T_P						M6502_P
#define M6510T_EA						M6502_EA
#define M6510T_ZP						M6502_ZP
#define M6510T_NMI_STATE				M6502_NMI_STATE
#define M6510T_IRQ_STATE				M6502_IRQ_STATE

#define M6510T_IRQ_LINE					M6502_IRQ_LINE

#define m6510t_ICount                   m6502_ICount

#define m6510t_init m6510_init
#define m6510t_reset m6510_reset
#define m6510t_exit m6510_exit
#define m6510t_execute m6510_execute
#define m6510t_get_context m6510_get_context
#define m6510t_set_context m6510_set_context
#define m6510t_get_reg m6510_get_reg
#define m6510t_set_reg m6510_set_reg
#define m6510t_set_irq_line m6510_set_irq_line
#define m6510t_set_irq_callback m6510_set_irq_callback
#define m6510t_state_save m6510_state_save
#define m6510t_state_load m6510_state_load
extern const char *m6510t_info(void *context, int regnum);
#define m6510t_dasm m6510_dasm
#endif

#ifdef HAS_M7501
#define M7501_A 						M6502_A
#define M7501_X 						M6502_X
#define M7501_Y 						M6502_Y
#define M7501_S 						M6502_S
#define M7501_PC						M6502_PC
#define M7501_P 						M6502_P
#define M7501_EA						M6502_EA
#define M7501_ZP						M6502_ZP
#define M7501_NMI_STATE 				M6502_NMI_STATE
#define M7501_IRQ_STATE 				M6502_IRQ_STATE

#define M7501_IRQ_LINE					M6502_IRQ_LINE

#define m7501_ICount                    m6502_ICount

#define m7501_init m6510_init
#define m7501_reset m6510_reset
#define m7501_exit m6510_exit
#define m7501_execute m6510_execute
#define m7501_get_context m6510_get_context
#define m7501_set_context m6510_set_context
#define m7501_get_reg m6510_get_reg
#define m7501_set_reg m6510_set_reg
#define m7501_set_irq_line m6510_set_irq_line
#define m7501_set_irq_callback m6510_set_irq_callback
#define m7501_state_save m6510_state_save
#define m7501_state_load m6510_state_load
extern const char *m7501_info(void *context, int regnum);
#define m7501_dasm m6510_dasm
#endif

#ifdef HAS_M8502
#define M8502_A 						M6502_A
#define M8502_X 						M6502_X
#define M8502_Y 						M6502_Y
#define M8502_S 						M6502_S
#define M8502_PC						M6502_PC
#define M8502_P 						M6502_P
#define M8502_EA						M6502_EA
#define M8502_ZP						M6502_ZP
#define M8502_NMI_STATE 				M6502_NMI_STATE
#define M8502_IRQ_STATE 				M6502_IRQ_STATE

#define M8502_IRQ_LINE					M6502_IRQ_LINE

#define m8502_ICount                    m6502_ICount

#define m8502_init m6510_init
#define m8502_reset m6510_reset
#define m8502_exit m6510_exit
#define m8502_execute m6510_execute
#define m8502_get_context m6510_get_context
#define m8502_set_context m6510_set_context
#define m8502_get_reg m6510_get_reg
#define m8502_set_reg m6510_set_reg
#define m8502_set_irq_line m6510_set_irq_line
#define m8502_set_irq_callback m6510_set_irq_callback
#define m8502_state_save m6510_state_save
#define m8502_state_load m6510_state_load
extern const char *m8502_info(void *context, int regnum);
#define m8502_dasm m6510_dasm
#endif


/****************************************************************************
 * The 2A03 (NES 6502 without decimal mode ADC/SBC)
 ****************************************************************************/
#if (HAS_N2A03)
#define N2A03_A 						M6502_A
#define N2A03_X 						M6502_X
#define N2A03_Y 						M6502_Y
#define N2A03_S 						M6502_S
#define N2A03_PC						M6502_PC
#define N2A03_P 						M6502_P
#define N2A03_EA						M6502_EA
#define N2A03_ZP						M6502_ZP
#define N2A03_NMI_STATE 				M6502_NMI_STATE
#define N2A03_IRQ_STATE 				M6502_IRQ_STATE

#define N2A03_IRQ_LINE					M6502_IRQ_LINE

#define n2a03_ICount					m6502_ICount

extern void n2a03_init(void);
extern void n2a03_reset(void *param);
extern void n2a03_exit(void);
extern int	n2a03_execute(int cycles);
extern unsigned n2a03_get_context(void *dst);
extern void n2a03_set_context(void *src);
extern unsigned n2a03_get_reg(int regnum);
extern void n2a03_set_reg (int regnum, unsigned val);
extern void n2a03_set_irq_line(int irqline, int state);
extern void n2a03_set_irq_callback(int (*callback)(int irqline));
extern const char *n2a03_info(void *context, int regnum);
extern unsigned n2a03_dasm(char *buffer, unsigned pc);


#define N2A03_DEFAULTCLOCK (21477272.724 / 12)

/* The N2A03 is integrally tied to its PSG (they're on the same die).
   Bit 7 of address $4011 (the PSG's DPCM control register), when set,
   causes an IRQ to be generated.  This function allows the IRQ to be called
   from the PSG core when such an occasion arises. */
extern void n2a03_irq(void);
#endif


/****************************************************************************
 * The 65C02
 ****************************************************************************/
#if (HAS_M65C02)
#define M65C02_A						M6502_A
#define M65C02_X						M6502_X
#define M65C02_Y						M6502_Y
#define M65C02_S						M6502_S
#define M65C02_PC						M6502_PC
#define M65C02_P						M6502_P
#define M65C02_EA						M6502_EA
#define M65C02_ZP						M6502_ZP
#define M65C02_NMI_STATE				M6502_NMI_STATE
#define M65C02_IRQ_STATE				M6502_IRQ_STATE

#define M65C02_IRQ_LINE					M6502_IRQ_LINE

#define m65c02_ICount					m6502_ICount

extern void m65c02_init(void);
extern void m65c02_reset(void *param);
extern void m65c02_exit(void);
extern int	m65c02_execute(int cycles);
extern unsigned m65c02_get_context(void *dst);
extern void m65c02_set_context(void *src);
extern unsigned m65c02_get_reg(int regnum);
extern void m65c02_set_reg(int regnum, unsigned val);
extern void m65c02_set_irq_line(int irqline, int state);
extern void m65c02_set_irq_callback(int (*callback)(int irqline));
extern const char *m65c02_info(void *context, int regnum);
extern unsigned m65c02_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * The 65SC02
 ****************************************************************************/
#if (HAS_M65SC02)
#define M65SC02_A						M6502_A
#define M65SC02_X						M6502_X
#define M65SC02_Y						M6502_Y
#define M65SC02_S						M6502_S
#define M65SC02_PC						M6502_PC
#define M65SC02_P						M6502_P
#define M65SC02_EA						M6502_EA
#define M65SC02_ZP						M6502_ZP
#define M65SC02_NMI_STATE				M6502_NMI_STATE
#define M65SC02_IRQ_STATE				M6502_IRQ_STATE

#define M65SC02_IRQ_LINE				M6502_IRQ_LINE

#define m65sc02_ICount					m6502_ICount

extern void m65sc02_init(void);
extern void m65sc02_reset(void *param);
extern void m65sc02_exit(void);
extern int	m65sc02_execute(int cycles);
extern unsigned m65sc02_get_context(void *dst);
extern void m65sc02_set_context(void *src);
extern unsigned m65sc02_get_reg(int regnum);
extern void m65sc02_set_reg(int regnum, unsigned val);
extern void m65sc02_set_irq_line(int irqline, int state);
extern void m65sc02_set_irq_callback(int (*callback)(int irqline));
extern const char *m65sc02_info(void *context, int regnum);
extern unsigned m65sc02_dasm(char *buffer, unsigned pc);
#endif

/****************************************************************************
 * The DECO CPU16
 ****************************************************************************/
#if (HAS_DECO16)
#define DECO16_A						M6502_A
#define DECO16_X						M6502_X
#define DECO16_Y						M6502_Y
#define DECO16_S						M6502_S
#define DECO16_PC						M6502_PC
#define DECO16_P						M6502_P
#define DECO16_EA						M6502_EA
#define DECO16_ZP						M6502_ZP
#define DECO16_NMI_STATE				M6502_NMI_STATE
#define DECO16_IRQ_STATE				M6502_IRQ_STATE

#define DECO16_IRQ_LINE					M6502_IRQ_LINE

#define deco16_ICount					m6502_ICount

extern void deco16_init(void);
extern void deco16_reset(void *param);
extern void deco16_exit(void);
extern int	deco16_execute(int cycles);
extern unsigned deco16_get_context(void *dst);
extern void deco16_set_context(void *src);
extern unsigned deco16_get_reg(int regnum);
extern void deco16_set_reg(int regnum, unsigned val);
extern void deco16_set_irq_line(int irqline, int state);
extern void deco16_set_irq_callback(int (*callback)(int irqline));
extern const char *deco16_info(void *context, int regnum);
extern unsigned deco16_dasm(char *buffer, unsigned pc);
#endif

#ifdef MAME_DEBUG
extern unsigned Dasm6502( char *dst, unsigned pc );
#endif

#endif /* _M6502_H */
