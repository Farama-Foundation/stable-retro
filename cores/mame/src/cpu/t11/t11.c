/*** t11: Portable DEC T-11 emulator ******************************************

	Copyright (C) Aaron Giles 1998-2001

	System dependencies:	long must be at least 32 bits
	                        word must be 16 bit unsigned int
							byte must be 8 bit unsigned int
							long must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpuintrf.h"
#include "mamedbg.h"
#include "t11.h"



/*************************************
 *
 *	Debugger layouts
 *
 *************************************/

static UINT8 t11_reg_layout[] =
{
	T11_PC, T11_SP, T11_PSW, T11_IRQ0_STATE, T11_IRQ1_STATE, T11_IRQ2_STATE, T11_IRQ3_STATE, -1,
	T11_R0,T11_R1,T11_R2,T11_R3,T11_R4,T11_R5, -1,
	T11_BANK0,T11_BANK1,T11_BANK2,T11_BANK3, T11_BANK4,T11_BANK5,T11_BANK6,T11_BANK7, 0
};


static UINT8 t11_win_layout[] =
{
	 0, 0,80, 4,	/* register window (top rows) */
	 0, 5,31,17,	/* disassembler window (left colums) */
	32, 5,48, 8,	/* memory #1 window (right, upper middle) */
	32,14,48, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};



/*************************************
 *
 *	Internal state representation
 *
 *************************************/

typedef struct
{
	PAIR	ppc;	/* previous program counter */
    PAIR	reg[8];
    PAIR	psw;
    UINT16	op;
    UINT8	wait_state;
    UINT8 *	bank[8];
    UINT8	irq_state;
    int		interrupt_cycles;
    int		(*irq_callback)(int irqline);
} t11_Regs;

static t11_Regs t11;



/*************************************
 *
 *	Global variables
 *
 *************************************/

int	t11_ICount;



/*************************************
 *
 *	Macro shortcuts
 *
 *************************************/

/* registers of various sizes */
#define REGD(x) t11.reg[x].d
#define REGW(x) t11.reg[x].w.l
#define REGB(x) t11.reg[x].b.l

/* PC, SP, and PSW definitions */
#define SP REGW(6)
#define PC REGW(7)
#define SPD REGD(6)
#define PCD REGD(7)
#define PSW t11.psw.b.l



/*************************************
 *
 *	Low-level memory operations
 *
 *************************************/

static INLINE int ROPCODE(void)
{
	int pc = PCD;
	PC += 2;
	return *(UINT16 *)(&t11.bank[pc >> 13][pc & 0x1fff]);
}


static INLINE int RBYTE(int addr)
{
	return T11_RDMEM(addr);
}


static INLINE void WBYTE(int addr, int data)
{
	T11_WRMEM(addr, data);
}


static INLINE int RWORD(int addr)
{
	return T11_RDMEM_WORD(addr & 0xfffe);
}


static INLINE void WWORD(int addr, int data)
{
	T11_WRMEM_WORD(addr & 0xfffe, data);
}



/*************************************
 *
 *	Low-level stack operations
 *
 *************************************/

static INLINE void PUSH(int val)
{
	SP -= 2;
	WWORD(SPD, val);
}


static INLINE int POP(void)
{
	int result = RWORD(SPD);
	SP += 2;
	return result;
}



/*************************************
 *
 *	Flag definitions and operations
 *
 *************************************/

/* flag definitions */
#define CFLAG 1
#define VFLAG 2
#define ZFLAG 4
#define NFLAG 8

/* extracts flags */
#define GET_C (PSW & CFLAG)
#define GET_V (PSW & VFLAG)
#define GET_Z (PSW & ZFLAG)
#define GET_N (PSW & NFLAG)

/* clears flags */
#define CLR_C (PSW &= ~CFLAG)
#define CLR_V (PSW &= ~VFLAG)
#define CLR_Z (PSW &= ~ZFLAG)
#define CLR_N (PSW &= ~NFLAG)

/* sets flags */
#define SET_C (PSW |= CFLAG)
#define SET_V (PSW |= VFLAG)
#define SET_Z (PSW |= ZFLAG)
#define SET_N (PSW |= NFLAG)



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

struct irq_table_entry
{
	UINT8	priority;
	UINT8	vector;
};

static const struct irq_table_entry irq_table[] =
{
	{ 0<<5, 0x00 },
	{ 4<<5, 0x38 },
	{ 4<<5, 0x34 },
	{ 4<<5, 0x30 },
	{ 5<<5, 0x5c },
	{ 5<<5, 0x58 },
	{ 5<<5, 0x54 },
	{ 5<<5, 0x50 },
	{ 6<<5, 0x4c },
	{ 6<<5, 0x48 },
	{ 6<<5, 0x44 },
	{ 6<<5, 0x40 },
	{ 7<<5, 0x6c },
	{ 7<<5, 0x68 },
	{ 7<<5, 0x64 },
	{ 7<<5, 0x60 }
};

static void t11_check_irqs(void)
{
	const struct irq_table_entry *irq = &irq_table[t11.irq_state & 15];
	int priority = PSW & 0xe0;

	/* compare the priority of the interrupt to the PSW */
	if (irq->priority > priority)
	{
		/* get the priority of this interrupt */
		int new_pc = RWORD(irq->vector);
		int new_psw = RWORD(irq->vector + 2);
		
		/* call the callback */
		if (t11.irq_callback)
		{
			int vector = 0;
			
			if (t11.irq_state & 8) vector = 3;
			else if (t11.irq_state & 4) vector = 2;
			else if (t11.irq_state & 2) vector = 1;
			(*t11.irq_callback)(vector);
		}

		/* kludge for 720 - fix me! */
		if (new_pc == 0)
			return;

		/* push the old state, set the new one */
		PUSH(PSW);
		PUSH(PC);
		PCD = new_pc;
		PSW = new_psw;
		t11_check_irqs();

		/* count cycles and clear the WAIT flag */
		t11.interrupt_cycles += 114;
		t11.wait_state = 0;
	}
}



/*************************************
 *
 *	Core opcodes
 *
 *************************************/

/* includes the static function prototypes and the master opcode table */
#include "t11table.c"

/* includes the actual opcode implementations */
#include "t11ops.c"



/*************************************
 *
 *	Fetch current context into buffer
 *
 *************************************/

unsigned t11_get_context(void *dst)
{
	if (dst)
		*(t11_Regs *)dst = t11;
	return sizeof(t11_Regs);
}



/*************************************
 *
 *	Retrieve context from buffer
 *
 *************************************/

void t11_set_context(void *src)
{
	if (src)
		t11 = *(t11_Regs *)src;
	t11_check_irqs();
}



/*************************************
 *
 *	External register getting
 *
 *************************************/

unsigned t11_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case T11_PC:			return PCD;
		case REG_SP:
		case T11_SP:			return SPD;
		case T11_PSW:			return PSW;
		case T11_R0:			return REGD(0);
		case T11_R1:			return REGD(1);
		case T11_R2:			return REGD(2);
		case T11_R3:			return REGD(3);
		case T11_R4:			return REGD(4);
		case T11_R5:			return REGD(5);
		case T11_IRQ0_STATE:	return (t11.irq_state & 1) ? ASSERT_LINE : CLEAR_LINE;
		case T11_IRQ1_STATE:	return (t11.irq_state & 2) ? ASSERT_LINE : CLEAR_LINE;
		case T11_IRQ2_STATE:	return (t11.irq_state & 4) ? ASSERT_LINE : CLEAR_LINE;
		case T11_IRQ3_STATE:	return (t11.irq_state & 8) ? ASSERT_LINE : CLEAR_LINE;
		case T11_BANK0:			return (unsigned)(t11.bank[0] - OP_RAM);
		case T11_BANK1:			return (unsigned)(t11.bank[1] - OP_RAM);
		case T11_BANK2:			return (unsigned)(t11.bank[2] - OP_RAM);
		case T11_BANK3:			return (unsigned)(t11.bank[3] - OP_RAM);
		case T11_BANK4:			return (unsigned)(t11.bank[4] - OP_RAM);
		case T11_BANK5:			return (unsigned)(t11.bank[5] - OP_RAM);
		case T11_BANK6:			return (unsigned)(t11.bank[6] - OP_RAM);
		case T11_BANK7:			return (unsigned)(t11.bank[7] - OP_RAM);
		case REG_PREVIOUSPC:	return t11.ppc.w.l;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = SPD + 2 * (REG_SP_CONTENTS - regnum);
				if (offset < 0xffff)
					return RWORD(offset);
			}
	}
	return 0;
}



/*************************************
 *
 *	External register setting
 *
 *************************************/

void t11_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
		case T11_PC:			PC = val; /* change_pc16 not needed */ break;
		case REG_SP:
		case T11_SP:			SP = val; break;
		case T11_PSW:			PSW = val; break;
		case T11_R0:			REGW(0) = val; break;
		case T11_R1:			REGW(1) = val; break;
		case T11_R2:			REGW(2) = val; break;
		case T11_R3:			REGW(3) = val; break;
		case T11_R4:			REGW(4) = val; break;
		case T11_R5:			REGW(5) = val; break;
		case T11_IRQ0_STATE:	t11_set_irq_line(T11_IRQ0, val); break;
		case T11_IRQ1_STATE:	t11_set_irq_line(T11_IRQ1, val); break;
		case T11_IRQ2_STATE:	t11_set_irq_line(T11_IRQ2, val); break;
		case T11_IRQ3_STATE:	t11_set_irq_line(T11_IRQ3, val); break;
		case T11_BANK0:			t11.bank[0] = &OP_RAM[val]; break;
		case T11_BANK1:			t11.bank[1] = &OP_RAM[val]; break;
		case T11_BANK2:			t11.bank[2] = &OP_RAM[val]; break;
		case T11_BANK3:			t11.bank[3] = &OP_RAM[val]; break;
		case T11_BANK4:			t11.bank[4] = &OP_RAM[val]; break;
		case T11_BANK5:			t11.bank[5] = &OP_RAM[val]; break;
		case T11_BANK6:			t11.bank[6] = &OP_RAM[val]; break;
		case T11_BANK7:			t11.bank[7] = &OP_RAM[val]; break;
		default:
			if (regnum < REG_SP_CONTENTS)
			{
				unsigned offset = SPD + 2 * (REG_SP_CONTENTS - regnum);
				if (offset < 0xffff)
					WWORD(offset, val & 0xffff);
			}
    }
}



/*************************************
 *
 *	Low-level initialization/cleanup
 *
 *************************************/

void t11_init(void)
{
}


void t11_exit(void)
{
	/* nothing to do */
}



/*************************************
 *
 *	CPU reset
 *
 *************************************/

void t11_reset(void *param)
{
	static const UINT16 initial_pc[] =
	{
		0xc000, 0x8000, 0x4000, 0x2000,
		0x1000, 0x0000, 0xf600, 0xf400
	};
	struct t11_setup *setup = param;
	int i;

	/* reset the state */
	memset(&t11, 0, sizeof(t11));
	
	/* initial SP is 376 octal, or 0xfe */
	SP = 0x00fe;
	
	/* initial PC comes from the setup word */
	PC = initial_pc[setup->mode >> 13];

	/* PSW starts off at highest priority */
	PSW = 0xe0;

	/* initialize the banking */
	for (i = 0; i < 8; i++)
		t11.bank[i] = &OP_RAM[i * 0x2000];
	
	/* initialize the IRQ state */
	t11.irq_state = 0;
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

void t11_set_irq_line(int irqline, int state)
{
	/* set the appropriate bit */
	if (state == CLEAR_LINE)
		t11.irq_state &= ~(1 << irqline);
	else
		t11.irq_state |= 1 << irqline;

	/* recheck for interrupts */
   	t11_check_irqs();
}


void t11_set_irq_callback(int (*callback)(int irqline))
{
	t11.irq_callback = callback;
}



/*************************************
 *
 *	Core execution
 *
 *************************************/

int t11_execute(int cycles)
{
	t11_ICount = cycles;
	t11_ICount -= t11.interrupt_cycles;
	t11.interrupt_cycles = 0;

	if (t11.wait_state)
	{
		t11_ICount = 0;
		goto getout;
	}

	do
	{
		t11.ppc = t11.reg[7];	/* copy PC to previous PC */

		CALL_MAME_DEBUG;

		t11.op = ROPCODE();
		(*opcode_table[t11.op >> 3])();

	} while (t11_ICount > 0);

getout:

	t11_ICount -= t11.interrupt_cycles;
	t11.interrupt_cycles = 0;

	return cycles - t11_ICount;
}



/*************************************
 *
 *	Return formatted string
 *
 *************************************/

const char *t11_info( void *context, int regnum )
{
	static char buffer[16][47+1];
	static int which = 0;
	t11_Regs *r = context;

	which = (which+1) % 16;
    buffer[which][0] = '\0';

	if( !context )
		r = &t11;

    switch( regnum )
	{
		case CPU_INFO_REG+T11_PC: sprintf(buffer[which], "PC:%04X", r->reg[7].w.l); break;
		case CPU_INFO_REG+T11_SP: sprintf(buffer[which], "SP:%04X", r->reg[6].w.l); break;
		case CPU_INFO_REG+T11_PSW: sprintf(buffer[which], "PSW:%02X", r->psw.b.l); break;
		case CPU_INFO_REG+T11_R0: sprintf(buffer[which], "R0:%04X", r->reg[0].w.l); break;
		case CPU_INFO_REG+T11_R1: sprintf(buffer[which], "R1:%04X", r->reg[1].w.l); break;
		case CPU_INFO_REG+T11_R2: sprintf(buffer[which], "R2:%04X", r->reg[2].w.l); break;
		case CPU_INFO_REG+T11_R3: sprintf(buffer[which], "R3:%04X", r->reg[3].w.l); break;
		case CPU_INFO_REG+T11_R4: sprintf(buffer[which], "R4:%04X", r->reg[4].w.l); break;
		case CPU_INFO_REG+T11_R5: sprintf(buffer[which], "R5:%04X", r->reg[5].w.l); break;
		case CPU_INFO_REG+T11_IRQ0_STATE: sprintf(buffer[which], "IRQ0:%X", (r->irq_state & 1) ? ASSERT_LINE : CLEAR_LINE); break;
		case CPU_INFO_REG+T11_IRQ1_STATE: sprintf(buffer[which], "IRQ1:%X", (r->irq_state & 2) ? ASSERT_LINE : CLEAR_LINE); break;
		case CPU_INFO_REG+T11_IRQ2_STATE: sprintf(buffer[which], "IRQ2:%X", (r->irq_state & 4) ? ASSERT_LINE : CLEAR_LINE); break;
		case CPU_INFO_REG+T11_IRQ3_STATE: sprintf(buffer[which], "IRQ3:%X", (r->irq_state & 8) ? ASSERT_LINE : CLEAR_LINE); break;
		case CPU_INFO_REG+T11_BANK0: sprintf(buffer[which], "B0:%06X", (unsigned)(r->bank[0] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK1: sprintf(buffer[which], "B1:%06X", (unsigned)(r->bank[1] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK2: sprintf(buffer[which], "B2:%06X", (unsigned)(r->bank[2] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK3: sprintf(buffer[which], "B3:%06X", (unsigned)(r->bank[3] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK4: sprintf(buffer[which], "B4:%06X", (unsigned)(r->bank[4] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK5: sprintf(buffer[which], "B5:%06X", (unsigned)(r->bank[5] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK6: sprintf(buffer[which], "B6:%06X", (unsigned)(r->bank[6] - OP_RAM)); break;
		case CPU_INFO_REG+T11_BANK7: sprintf(buffer[which], "B7:%06X", (unsigned)(r->bank[7] - OP_RAM)); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->psw.b.l & 0x80 ? '?':'.',
				r->psw.b.l & 0x40 ? 'I':'.',
				r->psw.b.l & 0x20 ? 'I':'.',
				r->psw.b.l & 0x10 ? 'T':'.',
				r->psw.b.l & 0x08 ? 'N':'.',
				r->psw.b.l & 0x04 ? 'Z':'.',
				r->psw.b.l & 0x02 ? 'V':'.',
				r->psw.b.l & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "T11";
		case CPU_INFO_FAMILY: return "DEC T-11";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Aaron Giles 1998";
		case CPU_INFO_REG_LAYOUT: return (const char*)t11_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)t11_win_layout;
    }
	return buffer[which];
}



/*************************************
 *
 *	Disassembly hook
 *
 *************************************/

unsigned t11_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return DasmT11(buffer,pc);
#else
	sprintf( buffer, "$%04X", cpu_readmem16lew_word(pc) );
	return 2;
#endif
}
