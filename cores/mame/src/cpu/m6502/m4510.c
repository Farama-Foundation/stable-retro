/*****************************************************************************
 *
 *	 m4510.c
 *	 Portable 4510 emulator V1.0beta1
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved
 *   documentation preliminary databook
 *	 documentation by michael steil mist@c64.org
 *	 available at ftp:/*ftp.funet.fi/pub/cbm/c65*/
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

/*
   c65 memory management
   (reset)
   c64 ff
   c65 20 (interface)

a   (c65 mode)
   a:00 x:e3 y:00 z:b3
   c65 64 (interface)
   c64 ff

b   (c65 dosmode?)
   c65 65 (interface, full colorram)
   a:00 x:11 y:80 z:31
   c64 ff

c   (?)
   c64 07
   a:00 x:00 y:00 z:00

   a c65 mode

   diskcontroller accesses


   monitor
   c64 ff
   a:a0 x:82 y:00 z:83

   c64 mode
   c65 0
   c65 2f:0 !
   c64 ff
   a:00 x:00 y:00 z:00

internal 8 mb to 64k switching (jmp routine in rom)
( seams to be incomplete, in chapter 1 1megabyte memory mapper )
         a  x  y  z
g      0 00 e0 00 f0
g  10000 00 e1 00 f1
g  20000 00 e2 00 f2
g  30000 00 e3 00 f3
g  40000 00 e4 00 f4
g  50000 00 e5 00 f5
g  60000 00 e6 00 f6
.
.
g  f0000 00 ef 00 ff
the same for 100000 .. 700000
g 800000 00 e3 00 b3

thesis:
a: ?0?0 0000
   ? ?       only in monitor mode set
x:      xxxx address bits a19 .. a16 for memory accesses with a15 0 ?
   0000      c64 mode
   0001      dosmode
   1110      c65 mode, plain ram access
             (0000-1fff contains the switching code, so not switchable!?)
   1000      monitor
   1         map 6000-7fff
    1        map 4000-5fff
     1       map 2000-3fff
      1      map 0000-1fff
y: ?000 0000
   ?         only in dos mode set
z:      xxxx address bits a19 .. a16 for memory accesses with a15 1 ?
   0000      c64 mode
   0011      dosmode
   1000      monitor
   1011      c65 mode
   1111      plain ram access
   1         map e000-ffff
    1        map c000-dfff
     1       map a000-bfff
      1      map 8000-9fff
 */

#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "m6502.h"
#include "m4510.h"

#include "ops02.h"
#include "opsc02.h"
#include "opsce02.h"
#include "ops4510.h"

#define M6502_NMI_VEC	0xfffa
#define M6502_RST_VEC	0xfffc
#define M6502_IRQ_VEC	0xfffe
#define M4510_RST_VEC	M6502_RST_VEC
#define M4510_IRQ_VEC	M6502_IRQ_VEC
#define M4510_NMI_VEC	M6502_NMI_VEC

#ifdef RUNTIME_LOADER
struct cpu_interface
m4510_interface=
CPU0(M4510,    m4510,    1,  0,1.00,M4510_INT_NONE,    M4510_INT_IRQ,  M4510_INT_NMI,  8, 20,     0,20,LE,1, 3);

extern void m4510_runtime_loader_init(void)
{
	cpuintf[CPU_M4510]=m4510_interface;
}
#endif

/* Layout of the registers in the debugger */
static UINT8 m4510_reg_layout[] = {
	M4510_A,M4510_X,M4510_Y,M4510_Z,M4510_S,M4510_PC,
	M4510_MEM_LOW,
	-1,
	M4510_EA,M4510_ZP,M4510_NMI_STATE,M4510_IRQ_STATE, M4510_B,
	M4510_P,
	M4510_MEM_HIGH,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 m4510_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 3,55, 9,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct {
	void	(**insn)(void); /* pointer to the function pointer table */
	PAIR	ppc;			/* previous program counter */
	PAIR	pc; 			/* program counter */
	PAIR	sp; 			/* stack pointer (always 100 - 1FF) */
	PAIR	zp; 			/* zero page address */
	/* contains B register zp.b.h */
	PAIR	ea; 			/* effective address */
	UINT8	a;				/* Accumulator */
	UINT8	x;				/* X index register */
	UINT8	y;				/* Y index register */
	UINT8	z;				/* Z index register */
	UINT8	p;				/* Processor status */
	UINT8	pending_irq;	/* nonzero if an IRQ is pending */
	UINT8	after_cli;		/* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	UINT8	irq_state;
	UINT16  low, high;
	UINT32	mem[8];
	int 	(*irq_callback)(int irqline);	/* IRQ callback */
}	m4510_Regs;

int m4510_ICount = 0;

static m4510_Regs m4510;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/

static INLINE int m4510_cpu_readop(void)
{
	register UINT16 t=m4510.pc.w.l++;
	return cpu_readop(M4510_MEM(t));
}

static INLINE int m4510_cpu_readop_arg(void)
{
	register UINT16 t=m4510.pc.w.l++;
	return cpu_readop_arg(M4510_MEM(t));
}

#define M4510
#include "t65ce02.c"

void m4510_reset (void *param)
{
	m4510.insn = insn4510;

	/* wipe out the rest of the m65ce02 structure */
	/* read the reset vector into PC */
	/* reset z index and b bank */
	PCL = RDMEM(M4510_RST_VEC);
	PCH = RDMEM(M4510_RST_VEC+1);

	/* after reset in 6502 compatibility mode */
	m4510.sp.d = 0x01ff; /* high byte descriped in databook */
	m4510.z = 0;
	B = 0;
	m4510.p = F_E|F_B|F_I|F_Z;	/* set E, I and Z flags */
	m4510.pending_irq = 0;	/* nonzero if an IRQ is pending */
	m4510.after_cli = 0;		/* pending IRQ and last insn cleared I */
	m4510.irq_callback = NULL;

	/* don't know */
	m4510.high=0x8200;
	m4510.mem[7]=0x20000;

	CHANGE_PC;
}

void m4510_exit(void)
{
	/* nothing to do yet */
}

unsigned m4510_get_context (void *dst)
{
	if( dst )
		*(m4510_Regs*)dst = m4510;
	return sizeof(m4510_Regs);
}

void m4510_set_context (void *src)
{
	if( src )
	{
		m4510 = *(m4510_Regs*)src;
		CHANGE_PC;
	}
}

unsigned m4510_get_reg (int regnum)
{
	switch( regnum )
	{
		case REG_PC: return M4510_MEM(PCD);
		case M4510_PC: return m4510.pc.w.l;
		case REG_SP: return S;
		case M4510_S: return m4510.sp.w.l;
		case M4510_P: return m4510.p;
		case M4510_A: return m4510.a;
		case M4510_X: return m4510.x;
		case M4510_Y: return m4510.y;
		case M4510_Z: return m4510.z;
		case M4510_B: return m4510.zp.b.h;
		case M4510_MEM_LOW: return m4510.low;
		case M4510_MEM_HIGH: return m4510.high;
		case M4510_MEM0: return m4510.mem[0];
		case M4510_MEM1: return m4510.mem[1];
		case M4510_MEM2: return m4510.mem[2];
		case M4510_MEM3: return m4510.mem[3];
		case M4510_MEM4: return m4510.mem[4];
		case M4510_MEM5: return m4510.mem[5];
		case M4510_MEM6: return m4510.mem[6];
		case M4510_MEM7: return m4510.mem[7];
		case M4510_EA: return m4510.ea.w.l;
		case M4510_ZP: return m4510.zp.b.l;
		case M4510_NMI_STATE: return m4510.nmi_state;
		case M4510_IRQ_STATE: return m4510.irq_state;
		case REG_PREVIOUSPC: return m4510.ppc.w.l;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
					return RDMEM( offset ) | ( RDMEM( offset + 1 ) << 8 );
			}
	}
	return 0;
}

void m4510_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: PCW = val; CHANGE_PC; break;
		case M4510_PC: m4510.pc.w.l = val; break;
		case REG_SP: S = val; break;
		case M4510_S: m4510.sp.w.l = val; break;
		case M4510_P: m4510.p = val; break;
		case M4510_MEM_LOW:
			m4510.low = val;
			/* change the memory registers*/
			break;
		case M4510_MEM_HIGH:
			m4510.high = val;
			/* change the memory registers*/
			break;
		case M4510_A: m4510.a = val; break;
		case M4510_X: m4510.x = val; break;
		case M4510_Y: m4510.y = val; break;
		case M4510_Z: m4510.z = val; break;
		case M4510_B: m4510.zp.b.h = val; break;
		case M4510_EA: m4510.ea.w.l = val; break;
		case M4510_ZP: m4510.zp.b.l = val; break;
		case M4510_NMI_STATE: m4510_set_irq_line( IRQ_LINE_NMI, val ); break;
		case M4510_IRQ_STATE: m4510_set_irq_line( 0, val ); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
				{
					WRMEM( offset, val & 0xfff );
					WRMEM( offset + 1, (val >> 8) & 0xff );
				}
			}
	}
}

static INLINE void m4510_take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M4510_IRQ_VEC;
		m4510_ICount -= 7;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog,"M4510#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD);
		/* call back the cpuintrf to let it clear the line */
		if (m4510.irq_callback) (*m4510.irq_callback)(0);
		CHANGE_PC;
	}
	m4510.pending_irq = 0;
}

int m4510_execute(int cycles)
{
	m4510_ICount = cycles;

	CHANGE_PC;

	do
	{
		UINT8 op;
		PPC = PCD;

		CALL_MAME_DEBUG;

		/* if an irq is pending, take it now */
		if( m4510.pending_irq )
			m4510_take_irq();

		op = RDOP();
		(*insn4510[op])();

		/* check if the I flag was just reset (interrupts enabled) */
		if( m4510.after_cli )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog,"M4510#%d after_cli was >0", cpu_getactivecpu());
			m4510.after_cli = 0;
			if (m4510.irq_state != CLEAR_LINE)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog,": irq line is asserted: set pending IRQ\n");
				m4510.pending_irq = 1;
			}
			else
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog,": irq line is clear\n");
			}
		}
		else
		if( m4510.pending_irq )
			m4510_take_irq();

	} while (m4510_ICount > 0);

	return cycles - m4510_ICount;
}

void m4510_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if (m4510.nmi_state == state) return;
		m4510.nmi_state = state;
		if( state != CLEAR_LINE )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog, "M4510#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu());
			EAD = M4510_NMI_VEC;
			m4510_ICount -= 7;
			PUSH(PCH);
			PUSH(PCL);
			PUSH(P & ~F_B);
			P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
			PCL = RDMEM(EAD);
			PCH = RDMEM(EAD+1);
			log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog,"M4510#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD);
			CHANGE_PC;
		}
	}
	else
	{
		m4510.irq_state = state;
		if( state != CLEAR_LINE )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE errorlog, "M4510#%d set_irq_line(ASSERT)\n", cpu_getactivecpu());
			m4510.pending_irq = 1;
		}
	}
}

void m4510_set_irq_callback(int (*callback)(int))
{
	m4510.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m4510_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m4510_Regs *r = context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &m4510;

	switch( regnum )
	{
		case CPU_INFO_REG+M4510_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+M4510_S: sprintf(buffer[which], "S:%04X", r->sp.w.l); break;
		case CPU_INFO_REG+M4510_P: sprintf(buffer[which], "P:%02X", r->p); break;
		case CPU_INFO_REG+M4510_MEM_LOW: sprintf(buffer[which], "LO:%04X", r->low); break;
		case CPU_INFO_REG+M4510_MEM_HIGH: sprintf(buffer[which], "HI:%04X", r->high); break;
		case CPU_INFO_REG+M4510_A: sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+M4510_X: sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+M4510_Y: sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+M4510_Z: sprintf(buffer[which], "Z:%02X", r->z); break;
		case CPU_INFO_REG+M4510_B: sprintf(buffer[which], "B:%02X", r->zp.b.h); break;
		case CPU_INFO_REG+M4510_EA: sprintf(buffer[which], "EA:%04X", r->ea.w.l); break;
		case CPU_INFO_REG+M4510_ZP: sprintf(buffer[which], "ZP:%04X", r->zp.w.l); break;
		case CPU_INFO_REG+M4510_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+M4510_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->p & 0x80 ? 'N':'.',
				r->p & 0x40 ? 'V':'.',
				r->p & 0x20 ? 'E':'.',
				r->p & 0x10 ? 'B':'.',
				r->p & 0x08 ? 'D':'.',
				r->p & 0x04 ? 'I':'.',
				r->p & 0x02 ? 'Z':'.',
				r->p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "M4510";
		case CPU_INFO_FAMILY: return "CBM Semiconductor Group CSG 65CE02";
		case CPU_INFO_VERSION: return "1.0beta";
		case CPU_INFO_CREDITS:
			return "Copyright (c) 1998 Juergen Buchmueller\n"
				"Copyright (c) 2000 Peter Trauner\n"
				"all rights reserved.";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_REG_LAYOUT: return (const char*)m4510_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m4510_win_layout;
	}
	return buffer[which];
}

unsigned m4510_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm4510( buffer, pc );
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}


extern void m4510_init(void){ return; }
