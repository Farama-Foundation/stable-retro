/*
	99xxcore.h : generic tms99xx emulation

	The TMS99XX_MODEL switch tell which emulator we want to build.  Set the switch, then include
	99xxcore.h, and you will have an emulator for this processor.

	Only tms9900, tms9980a/9981, and tms9995 work OK for now.  Note that tms9995 has not been tested
	extensively.

	I think all software aspects of tms9940, tms9985 and tms9989 are implemented (though there
	must be some mistakes, particularily in tms9940 BCD support).  You'll just have to implement
	bus interfaces, provided you know them.  (I don't...)

	Original tms9900 emulator by Edward Swartz
	Smoothed out by Raphael Nabet
	Originally converted for Mame by M.Coates
	Processor timing, support for tms9980 and tms9995, and many bug fixes by R Nabet
*/

/*
	tms9900 is derived from the TI990/x minicomputer series (with multi-chip processors, except with
	cheaper or later models, which used microprocessors).  However, tms99xx (and even extension-less
	tms99xxx) only implement a reduced subset of the huge instruction set available on big TI990
	systems.

	AFAIK, tms9900, tms9980, and tms9985 have exactly the same programming model, and are actually
	the same CPU with different bus interfaces.  The former affirmation is (almost) true with
	tms9940, so I guess the later is true, too.  (The only problem with tms9940 is I have no idea
	what its bus interfaces are - I have a pinout, but it does not help much, although it is obvious
	that the beast is a microcontroller.)  tms9985 had on-chip RAM and timer, like tms9995, although
	I don't know how the memory and CRU were mapped exactly.

	tms9989 is mostly alien to me.  I assumed it was more related to tms9900 than tms9995, although
	it has most of the additionnal features tms9995 has.

	tms9995 belongs to another generation.  As a matter of fact, it quite faster than tms9900.

	tms99xxx include all tms9995 advances, and add some advances of their own.  I know they support
	many extra opcodes (84 instructions types on tms99105a, vs. 69 on tms9900), and support
	privileges (i.e. supervisor mode), flags in multiprocessor environment, 32-bit operations...
	There was yet other features (Macrostore to define custom instructions, op code compression...),
	which are completely alien to me.

	I have written code to recognize every tms99xxx opcode, and all other TI990 instructions I have
	heard of.  I cannot complete this code, since I have no documentation on tms99xxx.  I am not
	even sure the instruction set I guessed for tms99xxx is correct.  Also, the proposed meaning for
	these extra memnonics could be wrong.

References :
* 9900 family systems design, chapter 6, 7, 8
* TMS 9980A/ TMS 9981 Product Data Book
* TMS 9995 16-Bit Microcomputer Data Manual

Tons of thanks to the guy who posted these, whoever he is...
<http://www.stormaster.com/Spies/arcade/simulation/processors/index.html>
*/

/* Set this to 1 to support HOLD_LINE */
/* This is a weird HOLD_LINE, actually : we hold the interrupt line only until IAQ
	(instruction acquisition) is enabled.  Well, this scheme could possibly exist on
	a tms9900-based system, unlike a real HOLD_LINE.  (OK, this is just a pretext, I was just too
	lazy to implement a true HOLD_LINE ;-) .) */
/* BTW, this only works with tms9900 ! */
#define SILLY_INTERRUPT_HACK 0

#if SILLY_INTERRUPT_HACK
	#define IRQ_MAGIC_LEVEL -2
#endif


#include "memory.h"
#include "mamedbg.h"
#include "tms9900.h"
#include <math.h>


#if (TMS99XX_MODEL == TMS9900_ID)

	#define TMS99XX_ICOUNT tms9900_ICount
	#define TMS99XX_INIT tms9900_init
	#define TMS99XX_RESET tms9900_reset
	#define TMS99XX_EXIT tms9900_exit
	#define TMS99XX_EXECUTE tms9900_execute
	#define TMS99XX_GET_CONTEXT tms9900_get_context
	#define TMS99XX_SET_CONTEXT tms9900_set_context
	#define TMS99XX_GET_REG tms9900_get_reg
	#define TMS99XX_SET_REG tms9900_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms9900_set_irq_callback
	#define TMS99XX_INFO tms9900_info
	#define TMS99XX_DASM tms9900_dasm

	#define TMS99XX_CPU_NAME "TMS9900"

#elif (TMS99XX_MODEL == TMS9940_ID)

	#define TMS99XX_ICOUNT tms9940_ICount
	#define TMS99XX_INIT tms9940_init
	#define TMS99XX_RESET tms9940_reset
	#define TMS99XX_EXIT tms9940_exit
	#define TMS99XX_EXECUTE tms9940_execute
	#define TMS99XX_GET_CONTEXT tms9940_get_context
	#define TMS99XX_SET_CONTEXT tms9940_set_context
	#define TMS99XX_GET_REG tms9940_get_reg
	#define TMS99XX_SET_REG tms9940_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms9940_set_irq_callback
	#define TMS99XX_INFO tms9940_info
	#define TMS99XX_DASM tms9940_dasm

	#define TMS99XX_CPU_NAME "TMS9940"

	#error "tms9940 is not yet supported"

#elif (TMS99XX_MODEL == TMS9980_ID)

	#define TMS99XX_ICOUNT tms9980a_ICount
	#define TMS99XX_INIT tms9980a_init
	#define TMS99XX_RESET tms9980a_reset
	#define TMS99XX_EXIT tms9980a_exit
	#define TMS99XX_EXECUTE tms9980a_execute
	#define TMS99XX_GET_CONTEXT tms9980a_get_context
	#define TMS99XX_SET_CONTEXT tms9980a_set_context
	#define TMS99XX_GET_REG tms9980a_get_reg
	#define TMS99XX_SET_REG tms9980a_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms9980a_set_irq_callback
	#define TMS99XX_INFO tms9980a_info
	#define TMS99XX_DASM tms9980a_dasm

	#define TMS99XX_CPU_NAME "TMS9980A/TMS9981"

#elif (TMS99XX_MODEL == TMS9985_ID)

	#define TMS99XX_ICOUNT tms9985_ICount
	#define TMS99XX_INIT tms9985_init
	#define TMS99XX_RESET tms9985_reset
	#define TMS99XX_EXIT tms9985_exit
	#define TMS99XX_EXECUTE tms9985_execute
	#define TMS99XX_GET_CONTEXT tms9985_get_context
	#define TMS99XX_SET_CONTEXT tms9985_set_context
	#define TMS99XX_GET_REG tms9985_get_reg
	#define TMS99XX_SET_REG tms9985_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms9985_set_irq_callback
	#define TMS99XX_INFO tms9985_info
	#define TMS99XX_DASM tms9985_dasm

	#define TMS99XX_CPU_NAME "TMS9985"

	#error "tms9985 is not yet supported"

#elif (TMS99XX_MODEL == TMS9989_ID)

	#define TMS99XX_ICOUNT tms9989_ICount
	#define TMS99XX_INIT tms9989_init
	#define TMS99XX_RESET tms9989_reset
	#define TMS99XX_EXIT tms9989_exit
	#define TMS99XX_EXECUTE tms9989_execute
	#define TMS99XX_GET_CONTEXT tms9989_get_context
	#define TMS99XX_SET_CONTEXT tms9989_set_context
	#define TMS99XX_GET_REG tms9989_get_reg
	#define TMS99XX_SET_REG tms9989_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms9989_set_irq_callback
	#define TMS99XX_INFO tms9989_info
	#define TMS99XX_DASM tms9989_dasm

	#define TMS99XX_CPU_NAME "TMS9989"

	#error "tms9989 is not yet supported"

#elif (TMS99XX_MODEL == TMS9995_ID)

	#define TMS99XX_ICOUNT tms9995_ICount
	#define TMS99XX_INIT tms9995_init
	#define TMS99XX_RESET tms9995_reset
	#define TMS99XX_EXIT tms9995_exit
	#define TMS99XX_EXECUTE tms9995_execute
	#define TMS99XX_GET_CONTEXT tms9995_get_context
	#define TMS99XX_SET_CONTEXT tms9995_set_context
	#define TMS99XX_GET_REG tms9995_get_reg
	#define TMS99XX_SET_REG tms9995_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms9995_set_irq_callback
	#define TMS99XX_INFO tms9995_info
	#define TMS99XX_DASM tms9995_dasm

	#define TMS99XX_CPU_NAME "TMS9995"

#elif (TMS99XX_MODEL == TMS99105A_ID)

	#define TMS99XX_ICOUNT tms99105a_ICount
	#define TMS99XX_INIT tms99105a_init
	#define TMS99XX_RESET tms99105a_reset
	#define TMS99XX_EXIT tms99105a_exit
	#define TMS99XX_EXECUTE tms99105a_execute
	#define TMS99XX_GET_CONTEXT tms99105a_get_context
	#define TMS99XX_SET_CONTEXT tms99105a_set_context
	#define TMS99XX_GET_REG tms99105a_get_reg
	#define TMS99XX_SET_REG tms99105a_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms99105a_set_irq_callback
	#define TMS99XX_INFO tms99105a_info
	#define TMS99XX_DASM tms99105a_dasm

	#define TMS99XX_CPU_NAME "TMS99105A"

	#error "tms99105a is not yet supported"

#elif (TMS99XX_MODEL == TMS99110A_ID)

	#define TMS99XX_ICOUNT tms99110a_ICount
	#define TMS99XX_INIT tms99110a_init
	#define TMS99XX_RESET tms99110a_reset
	#define TMS99XX_EXIT tms99110a_exit
	#define TMS99XX_EXECUTE tms99110a_execute
	#define TMS99XX_GET_CONTEXT tms99110a_get_context
	#define TMS99XX_SET_CONTEXT tms99110a_set_context
	#define TMS99XX_GET_REG tms99110a_get_reg
	#define TMS99XX_SET_REG tms99110a_set_reg
	#define TMS99XX_SET_IRQ_CALLBACK tms99110a_set_irq_callback
	#define TMS99XX_INFO tms99110a_info
	#define TMS99XX_DASM tms99110a_dasm

	#define TMS99XX_CPU_NAME "TMS99110A"

	#error "tms99110a is not yet supported"

#endif


static INLINE void execute(UINT16 opcode);

static void external_instruction_notify(int ext_op_ID);
static UINT16 fetch(void);
static UINT16 decipheraddr(UINT16 opcode);
static UINT16 decipheraddrbyte(UINT16 opcode);
static void contextswitch(UINT16 addr);

static void field_interrupt(void);

/***************************/
/* Mame Interface Routines */
/***************************/


static UINT8 tms9900_reg_layout[] = {
	TMS9900_PC, TMS9900_WP, TMS9900_STATUS, TMS9900_IR
#ifdef MAME_DEBUG
	, -1,
	TMS9900_R0, TMS9900_R1, TMS9900_R2, TMS9900_R3,
	TMS9900_R4, TMS9900_R5, TMS9900_R6, TMS9900_R7, -1,
	TMS9900_R8, TMS9900_R9, TMS9900_R10, TMS9900_R11,
	TMS9900_R12, TMS9900_R13, TMS9900_R14, TMS9900_R15, -1,
#endif
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 tms9900_win_layout[] = {
	 0, 0,80, 4,	/* register window (top rows) */
	 0, 5,31,17,	/* disassembler window (left colums) */
	32, 5,48, 8,	/* memory #1 window (right, upper middle) */
	32,14,48, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

int TMS99XX_ICOUNT = 0;


/* tms9900 ST register bits. */

/* These bits are set by every compare, move and arithmetic or logical operation : */
/* (Well, COC, CZC and TB only set the E bit, but these are kind of exceptions.) */
#define ST_LGT 0x8000 /* Logical Greater Than (strictly) */
#define ST_AGT 0x4000 /* Arithmetical Greater Than (strictly) */
#define ST_EQ  0x2000 /* Equal */

/* These bits are set by arithmetic operations, when it makes sense to update them. */
#define ST_C   0x1000 /* Carry */
#define ST_OV  0x0800 /* OVerflow (overflow with operations on signed integers, */
                      /* and when the result of a 32bits:16bits division cannot fit in a 16-bit word.) */

/* This bit is set by move and arithmetic operations WHEN THEY USE BYTE OPERANDS. */
#define ST_OP  0x0400 /* Odd Parity */

#if (TMS99XX_MODEL != TMS9940_ID)

/* This bit is set by the XOP instruction. */
#define ST_X   0x0200 /* Xop */

#endif

#if (TMS99XX_MODEL == TMS9940_ID)

/* This bit is set by arithmetic operations to support BCD */
#define ST_DC  0x0100 /* Digit Carry */

#endif

#if (TMS99XX_MODEL == TMS99105A_ID)

/* This bit is set in user (i.e. non-supervisor) mode */
#define ST_PRIVILEGED 0x0100

#endif

#if (TMS99XX_MODEL >= TMS9995_ID)

/* This bit is set in TMS9995 and later chips to generate a level-2 interrupt when the Overflow
status bit is set */
#define ST_OV_EN 0x0020 /* OVerflow interrupt ENable */

#endif

/* On models before TMS9995 (TMS9989 ?), unused ST bits are always forced to 0, so we define
a ST_MASK */
#if TMS99XX_MODEL == TMS9940_ID

#define ST_MASK 0xFD03

#elif TMS99XX_MODEL <= TMS9985_ID

#define ST_MASK 0xFE0F

#endif


/* Offsets for registers. */
#define R0   0
#define R1   2
#define R2   4
#define R3   6
#define R4   8
#define R5  10
#define R6  12
#define R7  14
#define R8  16
#define R9  18
#define R10 20
#define R11 22
#define R12 24
#define R13 26
#define R14 28
#define R15 30

typedef struct
{
/* "actual" tms9900 registers : */
	UINT16 WP;  /* Workspace pointer */
	UINT16 PC;  /* Program counter */
	UINT16 STATUS;  /* STatus register */

/* Now, data used for emulation */
	UINT16 IR;  /* Instruction register, with the currently parsed opcode */

	int interrupt_pending;  /* true if an interrupt must be honored... */

	int load_state; /* nonzero if the LOAD* line is active (low) */

#if ((TMS99XX_MODEL == TMS9900_ID) || (TMS99XX_MODEL == TMS9980_ID))
	/* On tms9900, we cache the state of INTREQ* and IC0-IC3 here */
	/* On tms9980/9981, we translate the state of IC0-IC2 to the equivalent state for a tms9900,
	and store the result here */
	int irq_level;	/* when INTREQ* is active, interrupt level on IC0-IC3 ; else always 16 */
	int irq_state;	/* nonzero if the INTREQ* line is active (low) */
#elif (TMS99XX_MODEL == TMS9995_ID)
	/* tms9995 is quite different : it latches the interrupt inputs */
	int irq_level;    /* We store the level of the request with the highest level here */
	int int_state;    /* interrupt lines state */
	int int_latch;	  /* interrupt latches state */
#endif

	/* interrupt callback */
	/* note that this callback is used by tms9900_set_irq_line() and tms9980a_set_irq_line() to
	retreive the value on IC0-IC3 (non-standard behaviour) */
	int (*irq_callback)(int irq_line);

	int IDLE;       /* nonzero if processor is IDLE - i.e waiting for interrupt while writing
	                    special data on CRU bus */

#ifdef MAME_DEBUG
	UINT16 FR[16];  /* contains a copy of the workspace for the needs of the debugger */
#endif

#if (TMS99XX_MODEL == TMS9985_ID) || (TMS99XX_MODEL == TMS9995_ID)
	unsigned char RAM[256]; /* on-chip RAM (yes, sir !) */
#endif

#if (TMS99XX_MODEL == TMS9995_ID)
	/* on-chip event counter/timer*/
	int decrementer_enabled;
	UINT16 decrementer_interval;
	UINT16 decrementer_count;	/* used in event counter mode*/
	void *timer;  /* used in timer mode */
#endif

#if (TMS99XX_MODEL == TMS9995_ID)
	/* additionnal registers */
	UINT16 flag; 	  /* flag register */
	int MID_flag;   /* MID flag register */
#endif

#if (TMS99XX_MODEL == TMS9995_ID)
	/* chip config, which can be set on reset */
	int memory_wait_states_byte;
	int memory_wait_states_word;
#endif
}	tms99xx_Regs;

static tms99xx_Regs I =
{
	0,0,0,0,  /* don't care */
	0,        /* no pending interrupt */
	0,        /* LOAD* inactive */
  16, 0,    /* INTREQ* inactive */
};
static UINT8 lastparity;  /* rather than handling ST_OP directly, we copy the last value which
                                  would set it here */
/* Some instructions (i.e. XOP, BLWP, and MID) disable interrupt recognition until another
instruction is executed : so they set this flag */
static int disable_interrupt_recognition = 0;

#if (TMS99XX_MODEL == TMS9995_ID)
static void reset_decrementer(void);
#endif

#if (TMS99XX_MODEL == TMS9900_ID)
	/*16-bit data bus, 16-bit address bus*/
	/*Note that tms9900 actually never accesses a single byte : when performing byte operations,
	it reads a 16-bit word, changes the revelant byte, then write a complete word.  You should
	remember this when writing memory handlers.*/
	/*This does not apply to tms9995 and tms99xxx, but does apply to tms9980 (see below).*/

	#define readword(addr)        cpu_readmem16bew_word(addr)
	#define writeword(addr,data)  cpu_writemem16bew_word((addr), (data))

	#define readbyte(addr)        cpu_readmem16bew(addr)
	#define writebyte(addr,data)  cpu_writemem16bew((addr),(data))

#elif (TMS99XX_MODEL == TMS9980_ID)
	/*8-bit data bus, 14-bit address*/
	/*Note that tms9980 never accesses a single byte (however crazy it may seem).  Although this
	makes memory access slower, I have emulated this feature, because if I did otherwise,
	there would be some implementation problems in some driver sooner or later.*/

	/*Macros instead of true 14-bit handlers.  You may want to change this*/
	#define cpu_readmem14(addr) cpu_readmem16((addr) & 0x3fff)
	#define cpu_writemem14(addr, data) cpu_writemem16((addr) & 0x3fff, data)

	#define readword(addr)        ( TMS99XX_ICOUNT -= 2, (cpu_readmem14(addr) << 8) + cpu_readmem14((addr)+1) )
	#define writeword(addr,data)  { TMS99XX_ICOUNT -= 2; cpu_writemem14((addr), (data) >> 8); cpu_writemem14((addr) + 1, (data) & 0xff); }

#if 0
	#define readbyte(addr)        (TMS99XX_ICOUNT -= 2, cpu_readmem14(addr))
	#define writebyte(addr,data)  { TMS99XX_ICOUNT -= 2; cpu_writemem14((addr),(data)); }
#else
	/*This is how it really works*/
	/*Note that every writebyte must match a readbyte (which is the case on a real-world tms9980)*/
	static int extra_byte;

	static int readbyte(int addr)
	{
		TMS99XX_ICOUNT -= 2;
		if (addr & 1)
		{
			extra_byte = cpu_readmem14(addr-1);
			return cpu_readmem14(addr);
		}
		else
		{
			int val = cpu_readmem14(addr);
			extra_byte = cpu_readmem14(addr+1);
			return val;
		}
	}
	static void writebyte (int addr, int data)
	{
		TMS99XX_ICOUNT -= 2;
		if (addr & 1)
		{
			extra_byte = cpu_readmem14(addr-1);

			cpu_writemem14(addr-1, extra_byte);
			cpu_writemem14(addr, data);
		}
		else
		{
			extra_byte = cpu_readmem14(addr+1);

			cpu_writemem14(addr, data);
			cpu_writemem14(addr+1, extra_byte);
		}
	}
#endif

#elif (TMS99XX_MODEL == TMS9995_ID)
	/*8-bit external data bus, with on-chip 16-bit RAM, and 16-bit address bus*/
	/*The code is complex, so we use functions rather than macros*/

	static int readword(int addr)
	{
		if (addr < 0xf000)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_word;
			return (cpu_readmem16(addr) << 8) + cpu_readmem16(addr + 1);
		}
		else if (addr < 0xf0fc)
		{
			return *(UINT16 *)(& I.RAM[addr - 0xf000]);
		}
		else if (addr < 0xfffa)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_word;
			return (cpu_readmem16(addr) << 8) + cpu_readmem16(addr + 1);
		}
		else if (addr < 0xfffc)
		{
			/* read decrementer */
			if (I.flag & 1)
				/* event counter mode */
				return I.decrementer_count;
			else if (I.decrementer_enabled && !(I.flag & 1))
				/* timer mode, timer enabled */
				return ceil(TIME_TO_CYCLES(cpu_getactivecpu(), timer_timeleft(I.timer)) / 16);
			else
				/* timer mode, timer disabled */
				return 0;
		}
		else
		{
			return *(UINT16 *)(& I.RAM[addr - 0xff00]);
		}
	}

	static void writeword (int addr, int data)
	{
		if (addr < 0xf000)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_word;
			cpu_writemem16(addr, data >> 8);
			cpu_writemem16(addr + 1, data & 0xff);
		}
		else if (addr < 0xf0fc)
		{
			*(UINT16 *)(& I.RAM[addr - 0xf000]) = data;
		}
		else if (addr < 0xfffa)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_word;
			cpu_writemem16(addr, data >> 8);
			cpu_writemem16(addr + 1, data & 0xff);
		}
		else if (addr < 0xfffc)
		{
			/* write decrementer */
			I.decrementer_interval = data;
			reset_decrementer();
		}
		else
		{
			*(UINT16 *)(& I.RAM[addr - 0xff00]) = data;
		}
	}

	static int readbyte(int addr)
	{
		if (addr < 0xf000)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_byte;
			return cpu_readmem16(addr);
		}
		else if (addr < 0xf0fc)
		{
			return I.RAM[BYTE_XOR_BE(addr - 0xf000)];
		}
		else if (addr < 0xfffa)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_byte;
			return cpu_readmem16(addr);;
		}
		else if (addr < 0xfffc)
		{
			/* read decrementer */
			int value;

			if (I.flag & 1)
				/* event counter mode */
				value = I.decrementer_count;
			else if (I.decrementer_enabled && !(I.flag & 1))
				/* timer mode, timer enabled */
				value = ceil(TIME_TO_CYCLES(cpu_getactivecpu(), timer_timeleft(I.timer)) / 16);
			else
				/* timer mode, timer disabled */
				value = 0;

			if (addr & 1)
				return (value & 0xFF);
			else
				return (value >> 8);
		}
		else
		{
			return I.RAM[BYTE_XOR_BE(addr - 0xff00)];
		}
	}

	static void writebyte (int addr, int data)
	{
		if (addr < 0xf000)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_byte;
			cpu_writemem16(addr, data);
		}
		else if (addr < 0xf0fc)
		{
			I.RAM[BYTE_XOR_BE(addr - 0xf000)] = data;
		}
		else if (addr < 0xfffa)
		{
			TMS99XX_ICOUNT -= I.memory_wait_states_byte;
			cpu_writemem16(addr, data);
		}
		else if (addr < 0xfffc)
		{
			/* write decrementer */
			/* Note that a byte write to tms9995 timer messes everything up. */
			I.decrementer_interval = (data << 8) | data;
			reset_decrementer();
		}
		else
		{
			I.RAM[BYTE_XOR_BE(addr - 0xff00)] = data;
		}
	}

#else

	#error "memory access not implemented"

#endif

#define READREG(reg)          readword(I.WP+reg)
#define WRITEREG(reg,data)    writeword(I.WP+reg,data)

/* Interrupt mask */
#if (TMS99XX_MODEL != TMS9940_ID)
	#define IMASK       (I.STATUS & 0x0F)
#else
	#define IMASK       (I.STATUS & 0x03)
#endif

/*
	CYCLES macro : you provide timings for tms9900 and tms9995, and the macro chooses for you.

	BTW, I have no idea what the timings are for tms9989 and tms99xxx...
*/
#if TMS99XX_MODEL <= TMS9989_ID
	/* Use TMS9900/TMS9980 timings*/
	#define CYCLES(a,b) TMS99XX_ICOUNT -= a
#else
	/* Use TMS9995 timings*/
	#define CYCLES(a,b) TMS99XX_ICOUNT -= b*4
#endif

#if (TMS99XX_MODEL == TMS9995_ID)

static void set_flag0(int val);
static void set_flag1(int val);

#endif

/************************************************************************
 * Status register functions
 ************************************************************************/
#include "99xxstat.h"

/**************************************************************************/

/*
	TMS9900 hard reset
*/
void TMS99XX_INIT(void)
{
#if (TMS99XX_MODEL == TMS9995_ID)
	void decrementer_callback(int ignored);
	I.timer = timer_alloc(decrementer_callback);
#endif
}

void TMS99XX_RESET(void *param)
{
	contextswitch(0x0000);

	I.STATUS = 0; /* TMS9980 and TMS9995 Data Book say so */
	setstat();

	I.IDLE = 0;   /* clear IDLE condition */

#if (TMS99XX_MODEL == TMS9995_ID)
	/* we can ask at reset time that the CPU always generates one wait state automatically */
	if (param == NULL)
	{	/* if no param, the default is currently "wait state added" */
		I.memory_wait_states_byte = 4;
		I.memory_wait_states_word = 12;
	}
	else
	{
		I.memory_wait_states_byte = (((tms9995reset_param *) param)->auto_wait_state) ? 4 : 0;
		I.memory_wait_states_word = (((tms9995reset_param *) param)->auto_wait_state) ? 12 : 4;
	}

	I.MID_flag = 0;

	/* Clear flag bits 0 & 1 */
	set_flag0(0);
	set_flag1(0);

	/* Clear internal interupt latches */
	I.int_latch = 0;
	I.flag &= 0xFFE3;
#endif

	/* The ST register and interrupt latches changed, didn't it ? */
	field_interrupt();

	CYCLES(26, 14);
}

void TMS99XX_EXIT(void)
{
	/* nothing to do ? */
}

int TMS99XX_EXECUTE(int cycles)
{
	TMS99XX_ICOUNT = cycles;

	do
	{

		#ifdef MAME_DEBUG
		{
			if (mame_debug)
			{
				setstat();

				I.FR[ 0] = READREG(R0);
				I.FR[ 1] = READREG(R1);
				I.FR[ 2] = READREG(R2);
				I.FR[ 3] = READREG(R3);
				I.FR[ 4] = READREG(R4);
				I.FR[ 5] = READREG(R5);
				I.FR[ 6] = READREG(R6);
				I.FR[ 7] = READREG(R7);
				I.FR[ 8] = READREG(R8);
				I.FR[ 9] = READREG(R9);
				I.FR[10] = READREG(R10);
				I.FR[11] = READREG(R11);
				I.FR[12] = READREG(R12);
				I.FR[13] = READREG(R13);
				I.FR[14] = READREG(R14);
				I.FR[15] = READREG(R15);

				#if 0		/* Trace */
				logerror("> PC %4.4x :%4.4x %4.4x : R=%4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x%4.4x %4.4x %4.4x %4.4x %4.4x %4.4x :T=%d\n",I.PC,I.STATUS,I.WP,I.FR[0],I.FR[1],I.FR[2],I.FR[3],I.FR[4],I.FR[5],I.FR[6],I.FR[7],I.FR[8],I.FR[9],I.FR[10],I.FR[11],I.FR[12],I.FR[13],I.FR[14],I.FR[15],TMS99XX_ICOUNT);
					#if 0	/* useful with TI99/4a driver */
					{
						extern int gpl_addr;
						logerror("> GPL pointer %4.4x\n", gpl_addr);
					}
					#endif
				#endif

				MAME_Debug();
			}
		}
		#endif

		if (I.IDLE)
		{	/* IDLE instruction has halted execution */
			external_instruction_notify(2);
			CYCLES(2, 2); /* 2 cycles per CRU write */
		}
		else
		{	/* we execute an instruction */
			disable_interrupt_recognition = 0;  /* default value */
			I.IR = fetch();
			execute(I.IR);

#if (TMS99XX_MODEL >= TMS9995_ID)
			/* Note that TI had some problem implementing this...  I don't know if this feature works on
			a real-world TMS9995. */
			if ((I.STATUS & ST_OV_EN) && (I.STATUS & ST_OV) && (I.irq_level > 2))
				I.irq_level = 2;  /* interrupt request */
#endif
		}

		/*
		  We handle interrupts here because :
		  a) LOAD and level-0 (reset) interrupts are non-maskable, so, AFAIK, if the LOAD* line or
		     INTREQ* line (with IC0-3 == 0) remain active, we will execute repeatedly the first
		     instruction of the interrupt routine.
		  b) if we did otherwise, we could have weird, buggy behavior if IC0-IC3 changed more than
		     once in too short a while (i.e. before tms9900 executes another instruction).  Yes, this
		     is rather pedantic, the probability is really small.
		*/
		if (I.interrupt_pending)
		{
			int level;

#if SILLY_INTERRUPT_HACK
			if (I.irq_level == IRQ_MAGIC_LEVEL)
			{
				level = (* I.irq_callback)(0);
				if (I.irq_state)
				{ /* if callback didn't clear the line */
					I.irq_level = level;
					if (level > IMASK)
						I.interrupt_pending = 0;
				}
			}
			else
#endif
			level = I.irq_level;

			if (I.load_state)
			{	/* LOAD has the highest priority */

				contextswitch(0xFFFC);  /* load vector, save PC, WP and ST */

				I.STATUS &= 0xFFF0;     /* clear mask */

				/* clear IDLE status if necessary */
				I.IDLE = 0;

				CYCLES(22, 14);
			}
			/* all TMS9900 chips I know do not honor interrupts after XOP, BLWP or MID (after any
			  interrupt-like instruction, actually) */
			else if (! disable_interrupt_recognition)
			{
				if (level <= IMASK)
				{	/* a maskable interrupt is honored only if its level isn't greater than IMASK */

					contextswitch(level*4); /* load vector, save PC, WP and ST */

					/* change interrupt mask */
					if (level)
					{
						I.STATUS = (I.STATUS & 0xFFF0) | (level -1);  /* decrement mask */
						I.interrupt_pending = 0;  /* as a consequence, the interrupt request will be subsequently ignored */
					}
					else
						I.STATUS &= 0xFFF0; /* clear mask (is this correct ???) */

#if (TMS99XX_MODEL == TMS9995_ID)
					I.STATUS &= 0xFE00;
#endif

					/* clear IDLE status if necessary */
					I.IDLE = 0;

#if (TMS99XX_MODEL == TMS9995_ID)
					/* Clear bit in latch */
					/* I think tms9989 does this, too */
					if (level != 2)
					{	/* Only do this on level 1, 3, 4 interrupts */
						int mask = 1 << level;
						int flag_mask = (level == 1) ? 4 : mask;

						I.int_latch &= ~ mask;
						I.flag &= ~ flag_mask;

						/* unlike tms9900, we can call the callback */
						if (level == 1)
							(* I.irq_callback)(0);
						else if (level == 4)
							(* I.irq_callback)(1);
					}
#endif

					CYCLES(22, 14);
				}
				else
#if SILLY_INTERRUPT_HACK
				if (I.interrupt_pending)  /* we may have just cleared this */
#endif
				{
					logerror("tms9900.c : the interrupt_pending flag was set incorrectly\n");
					I.interrupt_pending = 0;
				}
			}
		}

	} while (TMS99XX_ICOUNT > 0);

	return cycles - TMS99XX_ICOUNT;
}

unsigned TMS99XX_GET_CONTEXT(void *dst)
{
	setstat();

	if( dst )
		*(tms99xx_Regs*)dst = I;

	return sizeof(tms99xx_Regs);
}

void TMS99XX_SET_CONTEXT(void *src)
{
	if( src )
	{
		I = *(tms99xx_Regs*)src;
		/* We have to make additionnal checks this, because Mame debugger can foolishly initialize
		the context to all 0s */
#if (TMS99XX_MODEL == TMS9900_ID)
		if (! I.irq_state)
			I.irq_level = 16;
#elif ((TMS99XX_MODEL == TMS9980_ID) || (TMS99XX_MODEL == TMS9995_ID))
		/* Our job is simpler, since there is no level-0 request... */
		if (! I.irq_level)
			I.irq_level = 16;
#else
		#warning "You may want to have a look at this problem"
#endif

		getstat();  /* set last_parity */
	}
}

unsigned TMS99XX_GET_REG(int regnum)
{
	switch( regnum )
	{
		case REG_PC:
		case TMS9900_PC: return I.PC;
		case TMS9900_IR: return I.IR;
		case REG_SP:
		case TMS9900_WP: return I.WP;
		case TMS9900_STATUS: return I.STATUS;
#ifdef MAME_DEBUG
		case TMS9900_R0: return I.FR[0];
		case TMS9900_R1: return I.FR[1];
		case TMS9900_R2: return I.FR[2];
		case TMS9900_R3: return I.FR[3];
		case TMS9900_R4: return I.FR[4];
		case TMS9900_R5: return I.FR[5];
		case TMS9900_R6: return I.FR[6];
		case TMS9900_R7: return I.FR[7];
		case TMS9900_R8: return I.FR[8];
		case TMS9900_R9: return I.FR[9];
		case TMS9900_R10: return I.FR[10];
		case TMS9900_R11: return I.FR[11];
		case TMS9900_R12: return I.FR[12];
		case TMS9900_R13: return I.FR[13];
		case TMS9900_R14: return I.FR[14];
		case TMS9900_R15: return I.FR[15];
#endif
	}
	return 0;
}

void TMS99XX_SET_REG(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:
		case TMS9900_PC: I.PC = val; break;
		case TMS9900_IR: I.IR = val; break;
		case REG_SP:
		case TMS9900_WP: I.WP = val; break;
		case TMS9900_STATUS: I.STATUS = val; break;
#ifdef MAME_DEBUG
		case TMS9900_R0: I.FR[0]= val; break;
		case TMS9900_R1: I.FR[1]= val; break;
		case TMS9900_R2: I.FR[2]= val; break;
		case TMS9900_R3: I.FR[3]= val; break;
		case TMS9900_R4: I.FR[4]= val; break;
		case TMS9900_R5: I.FR[5]= val; break;
		case TMS9900_R6: I.FR[6]= val; break;
		case TMS9900_R7: I.FR[7]= val; break;
		case TMS9900_R8: I.FR[8]= val; break;
		case TMS9900_R9: I.FR[9]= val; break;
		case TMS9900_R10: I.FR[10]= val; break;
		case TMS9900_R11: I.FR[11]= val; break;
		case TMS9900_R12: I.FR[12]= val; break;
		case TMS9900_R13: I.FR[13]= val; break;
		case TMS9900_R14: I.FR[14]= val; break;
		case TMS9900_R15: I.FR[15]= val; break;
#endif
	}
}

#if (TMS99XX_MODEL == TMS9900_ID)

/*
void tms9900_set_irq_line(IRQ_LINE_NMI, int state) : change the state of the LOAD* line

	state == 0 -> LOAD* goes high (inactive)
	state != 0 -> LOAD* goes low (active)

	While LOAD* is low, we keep triggering LOAD interrupts...

	A problem : some peripheral lower the LOAD* line for a fixed time interval (causing the 1st
	instruction of the LOAD interrupt routine to be repeated while the line is low), and will be
	perfectly happy with the current scheme, but others might be more clever and wait for the IAQ
	(Instruction acquisition) line to go high, and this needs a callback function to emulate.
*/

/*
void tms9900_set_irq_line(int irqline, int state) : sets the state of the interrupt line.

	irqline is ignored, and should always be 0.

	state == 0 -> INTREQ* goes high (inactive)
	state != 0 -> INTREQ* goes low (active)
*/
/*
	R Nabet 991020, revised 991218 :
	In short : interrupt code should call "cpu_set_irq_line(0, 0, ASSERT_LINE);" to set an
	interrupt request (level-triggered interrupts).  Also, there MUST be a call to
	"cpu_set_irq_line(0, 0, CLEAR_LINE);" in the machine code, when the interrupt line is released by
	the hardware (generally in response to an action performed by the interrupt routines).
	On tms9995 (9989 ?), you can use PULSE_LINE, too, since the processor latches the line...

	**Note** : HOLD_LINE *NEVER* makes sense on the TMS9900 (or 9980, 9995...).  The reason is the
	TMS9900 does NOT tell the world it acknoledges an interrupt, so no matter how much hardware you
	se, you cannot know when the CPU takes the interrupt, hence you cannot release the line when
	the CPU takes the interrupt.  Generally, the interrupt condition is cleared by the interrupt
	routine (with some CRU or memory access).

	Note that cpu_generate_interrupt uses HOLD_LINE, so your driver interrupt code
	should always use the new style, i.e. return "ignore_interrupt()" and call
	"cpu_set_irq_line(0, 0, ASSERT_LINE);" explicitely.

	Last, many TMS9900-based hardware use a TMS9901 interrupt-handling chip.  If anybody wants
	to emulate some hardware which uses it, note that I am writing some emulation in the TI99/4(A)
	driver in MESS, so you should ask me.
*/
/*
 * HJB 990430: changed to use irq_callback() to retrieve the vector
 * instead of using 16 irqlines.
 *
 * R Nabet 990830 : My mistake, I rewrote all these once again ; I think it is now correct.
 * A driver using the TMS9900 should do :
 *		cpu_0_irq_line_vector_w(0, level);
 *		cpu_set_irq_line(0,0,ASSERT_LINE);
 *
 * R Nabet 991108 : revised once again, with advice from Juergen Buchmueller, after a discussion
 * with Nicola...
 * We use the callback to retreive the interrupt level as soon as INTREQ* is asserted.
 * As a consequence, I do not support HOLD_LINE normally...  However, we do not really have to
 * support HOLD_LINE, since no real world TMS9900-based system can support this.
 * FYI, there are two alternatives to retreiving the interrupt level with the callback :
 * a) using 16 pseudo-IRQ lines.  Mostly OK, though it would require a few core changes.
 *    However, this could cause some problems if someone tried to set two lines simulteanously...
 *    And TMS9900 did NOT have 16 lines ! This is why Juergen and I did not retain this solution.
 * b) modifying the interrupt system in order to provide an extra int to every xxx_set_irq_line
 *    function.  I think this solution would be fine, but it would require quite a number of
 *    changes in the MAME core.  (And I did not feel the courage to check out 4000 drivers and 25
 *    cpu cores ;-) .)
 *
 * Note that this does not apply to tms9995.
*/
void tms9900_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		I.load_state = state;   /* save new state */

		field_interrupt();  /* interrupt status changed */
	}
	else
	{
		/*if (I.irq_state == state)
			return;*/

		I.irq_state = state;

		if (state == CLEAR_LINE)
			I.irq_level = 16;
			/* trick : 16 will always be bigger than the IM (0-15), so there will never be interrupts */
		else
		{
	#if SILLY_INTERRUPT_HACK
			I.irq_level = IRQ_MAGIC_LEVEL;
	#else
			I.irq_level = (* I.irq_callback)(0);
	#endif
		}

		field_interrupt();  /* interrupt state is likely to have changed */
	}
}

#elif (TMS99XX_MODEL == TMS9980_ID)
/*
	interrupt system similar to tms9900, but only 3 interrupt pins (IC0-IC2)
*/

void tms9980a_set_irq_line(int irqline, int state)
{
	if (state == CLEAR_LINE)
	{
		I.load_state = 0;
		I.irq_state = 0;
		I.irq_level = 16;
		/* trick : 16 will always be bigger than the IM (0-15), so there will never be interrupts */
	}
	else
	{
#if SILLY_INTERRUPT_HACK
		#error "OK, this does not work with tms9980a"
		/*I.load_state = 0;
		I.irq_state = 1;
		I.irq_level = IRQ_MAGIC_LEVEL;*/
#else
		int level;

		level = (* I.irq_callback)(0);

		switch (level)
		{
		case 0:
		case 1:
			I.load_state = 0;
			I.irq_state = 0;
			I.irq_level = 16;
			tms9980a_reset(NULL);
			break;
		case 2:
			I.load_state = 1;
			I.irq_state = 0;
			I.irq_level = 16;
			break;
		case 7:
			I.load_state = 0;
			I.irq_state = 0;
			I.irq_level = 16;
			break;
		default:  /* external levels 1, 2, 3, 4 */
			I.load_state = 0;
			I.irq_state = 1;
			I.irq_level = level - 2;
			break;
		}
#endif
	}

	field_interrupt();  /* interrupt state is likely to have changed */
}

#elif (TMS99XX_MODEL == TMS9995_ID)
/*
  this call-back is called by MESS timer system when the timer reaches 0.
*/
void decrementer_callback(int ignored)
{
	/* request decrementer interrupt */
	I.int_latch |= 0x8;
	I.flag |= 0x8;

	field_interrupt();
}


/*
	reset and load the timer/decrementer

	Note that I don't know whether toggling flag0/flag1 causes the decrementer to be reloaded or not
*/
static void reset_decrementer(void)
{
	timer_adjust(I.timer, TIME_NEVER, 0, 0);

	/* decrementer / timer enabled ? */
	I.decrementer_enabled = ((I.flag & 2) && (I.decrementer_interval));

	if (I.decrementer_enabled)
	{
		if (I.flag & 1)
		{	/* decrementer */
			I.decrementer_count = I.decrementer_interval;
		}
		else
		{	/* timer */
			timer_adjust(I.timer, TIME_IN_CYCLES(I.decrementer_interval * 16L, cpu_getactivecpu()), 0, 0);
		}
	}
}

/*
	You have two interrupt line : one triggers level-1 interrupts, the other triggers level-4
	interrupts (or decrements the decrementer register).

	According to the hardware, you may use PULSE_LINE (edge-triggered interrupts), or ASSERT_LINE
	(level-triggered interrupts).  Edge-triggered interrupts are way simpler, but if multiple devices
	share the same line, they must use level-triggered interrupts.
*/
void tms9995_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		I.load_state = state;   /* save new state */

		field_interrupt();  /* interrupt status changed */
	}
	else
	{
		int mask = (irqline == 0) ? 0x2 : 0x10;
		int flag_mask = (irqline == 0) ? 0x4 : 0x10;

		if (((I.int_state & mask) != 0) ^ (state != 0))
		{	/* only if state changes */
			if (state)
			{
				I.int_state |= mask;

				if ((irqline == 1) && (I.flag & 1))
				{	/* event counter mode : INT4* triggers no interrupt... */
					if (I.decrementer_enabled)
					{	/* decrement, then interrupt if reach 0 */
						if ((-- I.decrementer_count) == 0)
						{
							decrementer_callback(0);
							I.decrementer_count = I.decrementer_interval;	/* reload */
						}
					}
				}
				else
				{	/* plain interrupt mode */
					I.int_latch |= mask;
					I.flag |= flag_mask;
				}
			}
			else
			{
				I.int_state &= ~ mask;
			}

			field_interrupt();  /* interrupt status changed */
		}
	}
}

#else

#error "interrupt system not implemented"

#endif

void TMS99XX_SET_IRQ_CALLBACK(int (*callback)(int irqline))
{
	I.irq_callback = callback;
}

/*
 * field_interrupt
 *
 * Determines whether if an interrupt is pending, and sets the revelant flag.
 *
 * Called when an interrupt pin (LOAD*, INTREQ*, IC0-IC3) is changed, and when the interrupt mask
 * is modified.
 *
 * By using this flag, we save some compares in the execution loop.  Subtle, isn't it ;-) ?
 *
 * R Nabet.
 */
#if (TMS99XX_MODEL == TMS9900_ID) || (TMS99XX_MODEL == TMS9980_ID)

static void field_interrupt(void)
{
	I.interrupt_pending = ((I.irq_level <= IMASK) || (I.load_state));
}

#elif (TMS99XX_MODEL == TMS9995_ID)

static void field_interrupt(void)
{
	if (I.load_state)
	{
		I.interrupt_pending = 1;
	}
	else
	{
		int current_int;
		int level;

		if (I.flag & 1)
			/* event counter mode : ignore int4* line... */
			current_int = (I.int_state & ~0x10) | I.int_latch;
		else
			/* normal behavior */
			current_int = I.int_state | I.int_latch;

		if (current_int)
			/* find first bit to 1 */
			/* possible values : 1, 3, 4 */
			for (level=0; ! (current_int & 1); current_int >>= 1, level++)
				;
		else
			level=16;

		I.irq_level = level;

		I.interrupt_pending = (level <= IMASK);
	}
}

#else

#error "field_interrupt() not written"

#endif

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *TMS99XX_INFO(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	tms99xx_Regs *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+TMS9900_PC: sprintf(buffer[which], "PC :%04X",  r->PC); break;
		case CPU_INFO_REG+TMS9900_IR: sprintf(buffer[which], "IR :%04X",  r->IR); break;
		case CPU_INFO_REG+TMS9900_WP: sprintf(buffer[which], "WP :%04X",  r->WP); break;
		case CPU_INFO_REG+TMS9900_STATUS: sprintf(buffer[which], "ST :%04X",  r->STATUS); break;
#ifdef MAME_DEBUG
		case CPU_INFO_REG+TMS9900_R0: sprintf(buffer[which], "R0 :%04X",  r->FR[0]); break;
		case CPU_INFO_REG+TMS9900_R1: sprintf(buffer[which], "R1 :%04X",  r->FR[1]); break;
		case CPU_INFO_REG+TMS9900_R2: sprintf(buffer[which], "R2 :%04X",  r->FR[2]); break;
		case CPU_INFO_REG+TMS9900_R3: sprintf(buffer[which], "R3 :%04X",  r->FR[3]); break;
		case CPU_INFO_REG+TMS9900_R4: sprintf(buffer[which], "R4 :%04X",  r->FR[4]); break;
		case CPU_INFO_REG+TMS9900_R5: sprintf(buffer[which], "R5 :%04X",  r->FR[5]); break;
		case CPU_INFO_REG+TMS9900_R6: sprintf(buffer[which], "R6 :%04X",  r->FR[6]); break;
		case CPU_INFO_REG+TMS9900_R7: sprintf(buffer[which], "R7 :%04X",  r->FR[7]); break;
		case CPU_INFO_REG+TMS9900_R8: sprintf(buffer[which], "R8 :%04X",  r->FR[8]); break;
		case CPU_INFO_REG+TMS9900_R9: sprintf(buffer[which], "R9 :%04X",  r->FR[9]); break;
		case CPU_INFO_REG+TMS9900_R10: sprintf(buffer[which], "R10:%04X",  r->FR[10]); break;
		case CPU_INFO_REG+TMS9900_R11: sprintf(buffer[which], "R11:%04X",  r->FR[11]); break;
		case CPU_INFO_REG+TMS9900_R12: sprintf(buffer[which], "R12:%04X",  r->FR[12]); break;
		case CPU_INFO_REG+TMS9900_R13: sprintf(buffer[which], "R13:%04X",  r->FR[13]); break;
		case CPU_INFO_REG+TMS9900_R14: sprintf(buffer[which], "R14:%04X",  r->FR[14]); break;
		case CPU_INFO_REG+TMS9900_R15: sprintf(buffer[which], "R15:%04X",  r->FR[15]); break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->WP & 0x8000 ? 'L':'.',
				r->WP & 0x4000 ? 'A':'.',
				r->WP & 0x2000 ? 'E':'.',
				r->WP & 0x1000 ? 'C':'.',
				r->WP & 0x0800 ? 'V':'.',
				r->WP & 0x0400 ? 'P':'.',
				r->WP & 0x0200 ? 'X':'.',
				r->WP & 0x0100 ? '?':'.',
				r->WP & 0x0080 ? '?':'.',
				r->WP & 0x0040 ? '?':'.',
				r->WP & 0x0020 ? '?':'.',
				r->WP & 0x0010 ? '?':'.',
				r->WP & 0x0008 ? 'I':'.',
				r->WP & 0x0004 ? 'I':'.',
				r->WP & 0x0002 ? 'I':'.',
				r->WP & 0x0001 ? 'I':'.');
			break;
		case CPU_INFO_NAME: return TMS99XX_CPU_NAME;
		case CPU_INFO_FAMILY: return "Texas Instruments 9900";
		case CPU_INFO_VERSION: return "2.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "C TMS9900 emulator by Edward Swartz, initially converted for Mame by M.Coates, updated by R. Nabet";
		case CPU_INFO_REG_LAYOUT: return (const char*)tms9900_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)tms9900_win_layout;
	}
	return buffer[which];
}

unsigned TMS99XX_DASM(char *buffer, unsigned pc)
{

#ifdef MAME_DEBUG
	return Dasm9900(buffer,pc);
#else
	sprintf( buffer, "$%04X", readword(pc) );
	return 2;
#endif
}

/**************************************************************************/

#if (TMS99XX_MODEL == TMS9995_ID)

/* set decrementer mode flag */
static void set_flag0(int val)
{
	if (val)
		I.flag |= 1;
	else
		I.flag &= ~ 1;

	reset_decrementer();
}

/* set decrementer enable flag */
static void set_flag1(int val)
{
	if (val)
		I.flag |= 2;
	else
		I.flag &= ~ 2;

	reset_decrementer();
}

#endif

#if (TMS99XX_MODEL == TMS9900_ID)
#define WRITEPORT(Port, data) cpu_writeport16bew_word((Port)<<1, (data))
#else
#define WRITEPORT(Port, data) cpu_writeport16(Port, data)
#endif


/*
	performs a normal write to CRU bus (used by SBZ, SBO, LDCR : address range 0 -> 0xFFF)
*/
static void writeCRU(int CRUAddr, int Number, UINT16 Value)
{
#if (TMS99XX_MODEL == TMS9900_ID)
	/* 3 MSBs are always 0 to support external instructions */
	#define wCRUAddrMask 0xFFF;
#elif (TMS99XX_MODEL == TMS9980_ID)
	/* 2 bits unused, and 2 MSBs are always 0 to support external instructions */
	#define wCRUAddrMask 0x7FF;
#elif (TMS99XX_MODEL == TMS9995_ID)
	/* no such problem here : data bus lines D0-D2 provide the external instruction code */
	#define wCRUAddrMask 0x7FFF;
#else
	#warning "I don't know how your processor handle CRU."
	#define wCRUAddrMask 0x7FFF;
#endif

	int count;

	logerror("PC %4.4x Write CRU %x for %x =%x\n",I.PC,CRUAddr,Number,Value);

	CRUAddr &= wCRUAddrMask;

	/* Write Number bits from CRUAddr */

	for(count=0; count<Number; count++)
	{
#if (TMS99XX_MODEL == TMS9995_ID)
		if (CRUAddr == 0xF70)
			set_flag0(Value & 0x01);
		else if (CRUAddr == 0xF71)
			set_flag1(Value & 0x01);
		else if ((CRUAddr >= 0xF72) && (CRUAddr < 0xF75))
			;     /* ignored */
		else if ((CRUAddr >= 0xF75) && (CRUAddr < 0xF80))
		{	/* user defined flags */
			int mask = 1 << (CRUAddr - 0xF70);
			if (Value & 0x01)
				I.flag |= mask;
			else
				I.flag &= ~ mask;
		}
		else if (CRUAddr == 0x0FED)
			/* MID flag */
			I.MID_flag = Value & 0x01;
		else
			/* External CRU */
#endif
		WRITEPORT(CRUAddr, (Value & 0x01));
		Value >>= 1;
		CRUAddr = (CRUAddr + 1) & wCRUAddrMask;
	}
}

/*
	Some opcodes perform a dummy write to a special CRU address, so that an external function may be
	triggered.

	Only the first 3 MSBs of the address matter : other address bits and the written value itself
	are undefined.

	How should we support this ? With callback functions ? Actually, as long as we do not support
	hardware which makes use of this feature, it does not really matter :-) .
*/
static void external_instruction_notify(int ext_op_ID)
{
#if 1
	/* I guess we can support this like normal CRU operations */
#if (TMS99XX_MODEL == TMS9900_ID)
	WRITEPORT(ext_op_ID << 12, 0); /* or is it 1 ??? */
#elif (TMS99XX_MODEL == TMS9980_ID)
	cpu_writeport16((ext_op_ID & 3) << 11, (ext_op_ID & 4) ? 1 : 0);
#elif (TMS99XX_MODEL == TMS9995_ID)
	cpu_writeport16(ext_op_ID << 15, 0); /* or is it 1 ??? */
#else
	#warning "I don't know how your processor handle external opcodes (maybe you don't need them, though)."
#endif

#else
	switch (ext_op_ID)
	{
		case 2: /* IDLE */

			break;
		case 3: /* RSET */

			break;
		case 5: /* CKON */

			break;
		case 6: /* CKOF */

			break;
		case 7: /* LREX */

			break;
		case 0:
			/* normal CRU write !!! */
			logerror("PC %4.4x : external_instruction_notify : wrong ext_op_ID",I.PC);
			break;
		default:
			/* unknown address */
			logerror("PC %4.4x : external_instruction_notify : unknown ext_op_ID",I.PC);
			break;
	}
#endif
}

/*
	performs a normal read to CRU bus (used by TB, STCR : address range 0->0xFFF)

	Note that on some hardware, e.g. TI99, all normal memory operations cause unwanted CRU
	read at the same address.
	This seems to be impossible to emulate efficiently.
*/
#if (TMS99XX_MODEL == TMS9900_ID)
#define READPORT(Port) cpu_readport16bew_word((Port)<<1)
#elif (TMS99XX_MODEL != TMS9995_ID)
#define READPORT(Port) cpu_readport16(Port)
#else
/* on tms9995, we have to handle internal CRU port */
int READPORT(int Port)
{
	if (Port == 0x1EE)
		/* flag, bits 0-7 */
		return I.flag & 0xFF;
	else if (Port == 0x1EF)
		/* flag, bits 8-15 */
		return (I.flag >> 8) & 0xFF;
	else if (Port == 0x1FD)
		/* MID flag, and extrernal devices */
		if (I.MID_flag)
			return cpu_readport16(Port) | 0x10;
		else
			return cpu_readport16(Port) & ~ 0x10;
	else
		/* extrernal devices */
		return cpu_readport16(Port);
}
#endif

static UINT16 readCRU(int CRUAddr, int Number)
{
#if (TMS99XX_MODEL == TMS9900_ID)
	/* 3 MSBs are always 0 to support external instructions */
	#define rCRUAddrMask 0x1FF
#elif (TMS99XX_MODEL == TMS9980_ID)
	/* 2 bits unused, and 2 MSBs are always 0 to support external instructions */
	#define rCRUAddrMask 0x0FF
#elif (TMS99XX_MODEL == TMS9995_ID)
	/* no such problem here : data bus lines D0-D2 provide the external instruction code */
	#define rCRUAddrMask 0xFFF
#else
	#warning "I don't know how your processor handle CRU."
	#define rCRUAddrMask 0xFFF
#endif

	static int BitMask[] =
	{
		0, /* filler - saves a subtract to find mask */
		0x0100,0x0300,0x0700,0x0F00,0x1F00,0x3F00,0x7F00,0xFF00,
		0x01FF,0x03FF,0x07FF,0x0FFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF
	};

	int Offset,Location,Value;

	logerror("Read CRU %x for %x\n",CRUAddr,Number);

	Location = CRUAddr >> 3;
	Offset   = CRUAddr & 07;

	if (Number <= 8)
	{
		/* Read 16 Bits */
		Value = (READPORT((Location + 1) & rCRUAddrMask) << 8)
		                                | READPORT(Location & rCRUAddrMask);

		/* Allow for Offset */
		Value >>= Offset;

		/* Mask out what we want */
		Value = (Value << 8) & BitMask[Number];

		/* And update */
		return (Value>>8);
	}
	else
	{
		/* Read 24 Bits */
		Value = (READPORT((Location + 2) & rCRUAddrMask) << 16)
		                                | (READPORT((Location + 1) & rCRUAddrMask) << 8)
		                                | READPORT(Location & rCRUAddrMask);

		/* Allow for Offset */
		Value >>= Offset;

		/* Mask out what we want */
		Value &= BitMask[Number];

		/* and Update */
		return Value;
	}
}

/**************************************************************************/

/* fetch : read one word at * PC, and increment PC. */

static UINT16 fetch(void)
{
	register UINT16 value = readword(I.PC);
	I.PC += 2;
	return value;
}

/* contextswitch : performs a BLWP, ie change PC, WP, and save PC, WP and ST... */
static void contextswitch(UINT16 addr)
{
	UINT16 oldWP, oldpc;

	/* save old state */
	oldWP = I.WP;
	oldpc = I.PC;

	/* load vector */
	I.WP = readword(addr) & ~1;
	I.PC = readword(addr+2) & ~1;

	/* write old state to regs */
	WRITEREG(R13, oldWP);
	WRITEREG(R14, oldpc);
	setstat();
	WRITEREG(R15, I.STATUS);
}

/*
 * decipheraddr : compute and return the effective adress in word instructions.
 *
 * NOTA : the LSB is always ignored in word adresses,
 * but we do not set to 0 because of XOP...
 */
static UINT16 decipheraddr(UINT16 opcode)
{
	register UINT16 ts = opcode & 0x30;
	register UINT16 reg = opcode & 0xF;

	reg += reg;

	if (ts == 0)
		/* Rx */
		return(reg + I.WP);
	else if (ts == 0x10)
	{	/* *Rx */
		CYCLES(4, 1);
		return(readword(reg + I.WP));
	}
	else if (ts == 0x20)
	{
		register UINT16 imm;

		imm = fetch();

		if (reg)
		{	/* @>xxxx(Rx) */
			CYCLES(8, 3);
			return(readword(reg + I.WP) + imm);
		}
		else
		{	/* @>xxxx */
			CYCLES(8, 1);
			return(imm);
		}
	}
	else /*if (ts == 0x30)*/
	{	/* *Rx+ */
		register UINT16 response;

		reg += I.WP;    /* reg now contains effective address */

		CYCLES(8, 3);

		response = readword(reg);
		writeword(reg, response+2); /* we increment register content */
		return(response);
	}
}

/* decipheraddrbyte : compute and return the effective adress in byte instructions. */
static UINT16 decipheraddrbyte(UINT16 opcode)
{
	register UINT16 ts = opcode & 0x30;
	register UINT16 reg = opcode & 0xF;

	reg += reg;

	if (ts == 0)
		/* Rx */
		return(reg + I.WP);
	else if (ts == 0x10)
	{	/* *Rx */
		CYCLES(4, 1);
		return(readword(reg + I.WP));
	}
	else if (ts == 0x20)
	{
		register UINT16 imm;

		imm = fetch();

		if (reg)
		{	/* @>xxxx(Rx) */
			CYCLES(8, 3);
			return(readword(reg + I.WP) + imm);
		}
		else
		{	/* @>xxxx */
			CYCLES(8, 1);
			return(imm);
		}
	}
	else /*if (ts == 0x30)*/
	{	/* *Rx+ */
		register UINT16 response;

		reg += I.WP;    /* reg now contains effective address */

		CYCLES(6, 3);

		response = readword(reg);
		writeword(reg, response+1); /* we increment register content */
		return(response);
	}
}


/*************************************************************************/

#if TMS99XX_MODEL <= TMS9989_ID
	/* TMS9900/TMS9980 merely ignore the instruction */
	#define HANDLE_ILLEGAL TMS99XX_ICOUNT -= 6
#elif TMS99XX_MODEL == TMS9995_ID
	/* TMS9995 generates a MID interrupt */
	#define HANDLE_ILLEGAL \
	{ \
		I.MID_flag = 1; \
		contextswitch(0x0008); \
		I.STATUS = (I.STATUS & 0xFE00) | 0x1; \
		disable_interrupt_recognition = 1; \
	}
#else
#define HANDLE_ILLEGAL
#warning "don't know"
#endif

/*==========================================================================
   Illegal instructions                                        >0000->01FF (not for 9989 and later)
                                                               >0C00->0FFF (not for 99xxx)
============================================================================*/

static void illegal(UINT16 opcode)
{
	HANDLE_ILLEGAL;
}


#if (TMS99XX_MODEL >= TMS99105A_ID)
/*==========================================================================
   Additionnal instructions,                                   >0000->002F
   Additionnal single-register instruction,                    >0030->003F
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |     o p c o d e               |
    |     o p c o d e       | reg # |
    ---------------------------------

tms99xxx : SRAM, SLAM, AM, SM
============================================================================*/
static void h0000(UINT16 opcode)
{
#if 0
	if (opcode & 0x30)
	{	/* STPC STore Program Counter */

	}
	else
#endif

	switch (opcode /*& 0x3F*/)
	{
	case 0x1C:  /* SRAM */
		/* SRAM -- Shift Right Arithmetic Multiple */
		/* right shift on a 32-bit operand */
		/* ... */
		break;
	case 0x1D:  /* SLAM */
		/* SLAM -- Shift Left Arithmetic Multiple */
		/* left shift on a 32-bit operand */
		/* ... */
		break;

	case 0x29:  /* AM (or SM ?) */
		/* AM ---- Add Multiple */
		/* add with 32-bit operands */
		/* ... */
		break;
	case 0x2A:  /* SM (or AM ?) */
		/* SM ---- Subtract Multiple */
		/* substract with 32-bit operands */
		/* ... */
		break;

#if 0
	case 0x1E:
		/* RTO --- Right Test for One */
	case 0x1F:
		/* LTO --- Left Test for One */
	case 0x20:
		/* CNTO -- CouNT Ones */
	case 0x21:
		/* SLSL -- Search LiSt Logical address */
	case 0x22:
		/* SLSP -- Search LiSt Physical address */
	case 0x23:
		/* BDC --- Binary to Decimal ascii Conversion */
	case 0x24:
		/* DBC --- Decimal ascii to Binary Conversion */
	case 0x25:
		/* SWPM -- SWaP Multiple precision */
	case 0x26:
		/* XORM -- eXclusive OR Multiple precision */
	case 0x27:
		/* ORM --- OR Multiple precision */
	case 0x28:
		/* ANDM -- AND Multiple precision */

	case 0x2B:
		/* MOVA -- MOVe Address */
	case 0x2D:
		/* EMD --- Execute Micro-Diagnostics */
	case 0x2E:
		/* EINT -- Enable INTerrupts */
	case 0x2F:
		/* DINT -- Disable INTerrupts */

		break;
#endif

	default:
		HANDLE_ILLEGAL;
		break;
	}
}
#endif


#if (TMS99XX_MODEL >= TMS9989_ID)
/*==========================================================================
   Additionnal single-register instructions,                   >0040->00FF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |     o p c o d e       | reg # |
    ---------------------------------

tms9989 and later : LST, LWP
tms99xxx : BLSK
============================================================================*/
static void h0040(UINT16 opcode)
{
	register UINT16 addr;

	addr = opcode & 0xF;
	addr = ((addr + addr) + I.WP) & ~1;

	switch ((opcode & 0xF0) >> 4)
	{
	case 8:   /* LST */
		/* LST --- Load STatus register */
		/* ST = *Reg */
		I.STATUS = readword(addr);
		break;
	case 9:   /* LWP */
		/* LWP --- Load Workspace Pointer register */
		/* WP = *Reg */
		I.WP = readword(addr);
		break;

#if (TMS99XX_MODEL >= TMS99105A_ID)
	case 11:  /* BLSK */
		/* BLSK -- Branch immediate & Link to StacK */

		break;
#endif

#if 0
	case 4:
		/* CS ---- Compare Strings */
	case 5:
		/* SEQB -- Search string for EQual Byte */
	case 6:
		/* MOVS -- MOVe String */
	case 7:
		/* LIM --- Load Interrupt Mask	*/

	case 10:
		/* LCS --- Load writable Control Store */

	case 12:
		/* MVSR -- MoVe String Reverse */
	case 13:
		/* MVSK -- MoVe String from stacK */
	case 14:
		/* POPS -- POP String from stack	*/
	case 15:
		/* PSHS -- PuSH String to stack */

		break;
#endif

	default:
		HANDLE_ILLEGAL;
		break;
	}
}


/*==========================================================================
   Additionnal single-operand instructions,                    >0100->01FF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |    o p c o d e    |TS |   S   |
    ---------------------------------

tms9989 and later : DIVS, MPYS
tms99xxx : BIND
============================================================================*/
static void h0100(UINT16 opcode)
{
	register UINT16 src;

	src = decipheraddr(opcode) & ~1;

  switch ((opcode & 0xC0) >> 6)
  {
#if (TMS99XX_MODEL >= TMS99105A_ID)
	case 1:
		/* BIND -- Branch INDirect */

		break;
#endif

  case 2:   /* DIVS */
		/* DIVS -- DIVide Signed */
		/* R0 = (R0:R1)/S   R1 = (R0:R1)%S */
		{
			INT16 d = readword(src);
			long divq = (READREG(R0) << 16) | READREG(R1);
			long q = divq/d;

			if ((q < -32768L) || (q > 32767L))
			{
				I.STATUS |= ST_OV;
				CYCLES(24 /*don't know*/, 10);
			}
			else
			{
				I.STATUS &= ~ST_OV;
				setst_lae(q);
				WRITEREG(R0, q);
				WRITEREG(R1, divq%d);
				/* tms9995 : 33 is the worst case */
				CYCLES(102 /*don't know*/, 33);
			}
		}
		break;

  case 3:   /* MPYS */
		/* MPYS -- MultiPlY Signed */
		/* Results:  R0:R1 = R0*S */
		{
			long prod = ((long) (INT16) READREG(R0)) * ((long) (INT16) readword(src));

			I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ);
			if (prod > 0)
				I.STATUS |= (ST_LGT | ST_AGT);
			else if (prod < 0)
				I.STATUS |= ST_LGT;
			else
				I.STATUS |= ST_EQ;

			WRITEREG(R0, prod >> 16);
			WRITEREG(R1, prod);
		}
		CYCLES(56 /*don't know*/, 25);
		break;

#if 0
	case 0:
		/* EVAD -- EValuate ADdress instruction */

		break;
#endif

	default:
		HANDLE_ILLEGAL;
		break;
  }
}
#endif


/*==========================================================================
   Immediate, Control instructions,                            >0200->03FF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |     o p c o d e     |0| reg # |
    ---------------------------------

  LI, AI, ANDI, ORI, CI, STWP, STST, LIMI, LWPI, IDLE, RSET, RTWP, CKON, CKOF, LREX
============================================================================*/
static void h0200(UINT16 opcode)
{
	register UINT16 addr;
	register UINT16 value;	/* used for anything */

	addr = opcode & 0xF;
	addr = ((addr + addr) + I.WP) & ~1;

#if 0
	if ((opcode >= 0x0320) && (opcode < 0x0340))
	{	/* LMF */
		/* LMF --- Load memory Map File */
		/* I don't know what it does exactly.  I guess it was used by some paging system on some
		TI990/10 variant.
		Syntax : LMF Rx,x; where Rx can be any workspace register from 0 thru 15, and x can be
		a `0' or a `1'. */
		/* ... */
		return;
	}
#endif

#if (TMS99XX_MODEL >= TMS9995_ID)
	/* better instruction decoding on tms9995 */
	if (((opcode < 0x2E0) && (opcode & 0x10)) || ((opcode >= 0x2E0) && (opcode & 0x1F)))
	{
#if 0
		if (opcode == 0x0301)
		{	/* CR ---- Compare Reals */
		}
		else if (opcode == 0x0301)
		{	/* MM ---- Multiply Multiple */
		}
		else if (opcode >= 0x03F0)
		{	/* EP ---- Extend Precision */
		}
		else
#endif
		HANDLE_ILLEGAL;
		return;
	}
#endif

	switch ((opcode & 0x1e0) >> 5)
	{
	case 0:   /* LI */
		/* LI ---- Load Immediate */
		/* *Reg = *PC+ */
		value = fetch();
		writeword(addr, value);
		setst_lae(value);
		CYCLES(12, 3);
		break;
	case 1:   /* AI */
		/* AI ---- Add Immediate */
		/* *Reg += *PC+ */
		value = fetch();
		wadd(addr, value);
		CYCLES(14, 4);
		break;
	case 2:   /* ANDI */
		/* ANDI -- AND Immediate */
		/* *Reg &= *PC+ */
		value = fetch();
		value = readword(addr) & value;
		writeword(addr, value);
		setst_lae(value);
		CYCLES(14, 4);
		break;
	case 3:   /* ORI */
		/* ORI --- OR Immediate */
		/* *Reg |= *PC+ */
		value = fetch();
		value = readword(addr) | value;
		writeword(addr, value);
		setst_lae(value);
		CYCLES(14, 4);
		break;
	case 4:   /* CI */
		/* CI ---- Compare Immediate */
		/* status = (*Reg-*PC+) */
		value = fetch();
		setst_c_lae(value, readword(addr));
		CYCLES(14, 4);
		break;
	case 5:   /* STWP */
		/* STWP -- STore Workspace Pointer */
		/* *Reg = WP */
		writeword(addr, I.WP);
		CYCLES(8, 3);
		break;
	case 6:   /* STST */
		/* STST -- STore STatus register */
		/* *Reg = ST */
		setstat();
		writeword(addr, I.STATUS);
		CYCLES(8, 3);
		break;
	case 7:   /* LWPI */
		/* LWPI -- Load Workspace Pointer Immediate */
		/* WP = *PC+ */
		I.WP = fetch();
		CYCLES(10, 4);
		break;
	case 8:   /* LIMI */
		/* LIMI -- Load Interrupt Mask Immediate */
		/* ST&15 |= (*PC+)&15 */
		value = fetch();
#if (TMS99XX_MODEL == TMS9940_ID)
		/* Interrupt mask is only two-bit-long on tms9940 */
		I.STATUS = (I.STATUS & ~ 0x3) | (value & 0x3);
#else
		I.STATUS = (I.STATUS & ~ 0xF) | (value & 0xF);
#endif
		field_interrupt();  /*IM has been modified.*/
		CYCLES(16, 5);
		break;
	case 9:   /* LMF is implemented elsewhere - when it is implemented */
		HANDLE_ILLEGAL;
		break;
	case 10:  /* IDLE */
		/* IDLE -- IDLE until a reset, interrupt, load */
		/* The TMS99000 locks until an interrupt happen (like with 68k STOP instruction),
		   and continuously performs a special CRU write (code 2). */
		I.IDLE = 1;
		external_instruction_notify(2);
		CYCLES(12, 7);
		/* we take care of further external_instruction_notify(2); in execute() */
		break;
	case 11:  /* RSET */
		/* RSET -- ReSET */
		/* Reset the Interrupt Mask, and perform a special CRU write (code 3). */
		/* Does not actually cause a reset, but an external circuitery could trigger one. */
		I.STATUS &= 0xFFF0; /*clear IM.*/
		field_interrupt();  /*IM has been modified.*/
		external_instruction_notify(3);
		CYCLES(12, 7);
		break;
	case 12:  /* RTWP */
		/* RTWP -- Return with Workspace Pointer */
		/* WP = R13, PC = R14, ST = R15 */
		I.STATUS = READREG(R15);
		getstat();  /* set last_parity */
		I.PC = READREG(R14);
		I.WP = READREG(R13);
		field_interrupt();  /*IM has been modified.*/
		CYCLES(14, 6);
		break;
	case 13:  /* CKON */
	case 14:  /* CKOF */
	case 15:  /* LREX */
		/* CKON -- ClocK ON */
		/* Perform a special CRU write (code 5). */
		/* An external circuitery could, for instance, enable a "decrement-and-interrupt" timer. */
		/* CKOF -- ClocK OFf */
		/* Perform a special CRU write (code 6). */
		/* An external circuitery could, for instance, disable a "decrement-and-interrupt" timer. */
		/* LREX -- Load or REstart eXecution */
		/* Perform a special CRU write (code 7). */
		/* An external circuitery could, for instance, activate the LOAD* line,
		   causing a non-maskable LOAD interrupt (vector -1). */
		external_instruction_notify((opcode & 0x00e0) >> 5);
		CYCLES(12, 7);
		break;
	}
}


/*==========================================================================
   Single-operand instructions,                                >0400->07FF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |    o p c o d e    |TS |   S   |
    ---------------------------------

  BLWP, B, X, CLR, NEG, INV, INC, INCT, DEC, DECT, BL, SWPB, SETO, ABS
tms99xxx : LDD, LDS
============================================================================*/
static void h0400(UINT16 opcode)
{
	register UINT16 addr = decipheraddr(opcode) & ~1;
	register UINT16 value;  /* used for anything */

	switch ((opcode & 0x3C0) >> 6)
	{
	case 0:   /* BLWP */
		/* BLWP -- Branch and Link with Workspace Pointer */
		/* Result: WP = *S+, PC = *S */
		/*         New R13=old WP, New R14=Old PC, New R15=Old ST */
		contextswitch(addr);
		CYCLES(26, 11);
		disable_interrupt_recognition = 1;
		break;
	case 1:   /* B */
		/* B ----- Branch */
		/* PC = S */
		I.PC = addr;
		CYCLES(8, 3);
		break;
	case 2:   /* X */
		/* X ----- eXecute */
		/* Executes instruction *S */
		execute(readword(addr));
		/* On tms9900, the X instruction actually takes 8 cycles, but we gain 4 cycles on the next
		instruction, as we don't need to fetch it. */
		CYCLES(4, 2);
		break;
	case 3:   /* CLR */
		/* CLR --- CLeaR */
		/* *S = 0 */
		writeword(addr, 0);
		CYCLES(10, 3);
		break;
	case 4:   /* NEG */
		/* NEG --- NEGate */
		/* *S = -*S */
		value = - (INT16) readword(addr);
		if (value)
			I.STATUS &= ~ ST_C;
		else
			I.STATUS |= ST_C;
#if (TMS99XX_MODEL == TMS9940_ID)
		if (value & 0x0FFF)
			I.STATUS &= ~ ST_DC;
		else
			I.STATUS |= ST_DC;
#endif
		setst_laeo(value);
		writeword(addr, value);
		CYCLES(12, 3);
		break;
	case 5:   /* INV */
		/* INV --- INVert */
		/* *S = ~*S */
		value = ~ readword(addr);
		writeword(addr, value);
		setst_lae(value);
		CYCLES(10, 3);
		break;
	case 6:   /* INC */
		/* INC --- INCrement */
		/* (*S)++ */
		wadd(addr, 1);
		CYCLES(10, 3);
		break;
	case 7:   /* INCT */
		/* INCT -- INCrement by Two */
		/* (*S) +=2 */
		wadd(addr, 2);
		CYCLES(10, 3);
		break;
	case 8:   /* DEC */
		/* DEC --- DECrement */
		/* (*S)-- */
		wsub(addr, 1);
		CYCLES(10, 3);
		break;
	case 9:   /* DECT */
		/* DECT -- DECrement by Two */
		/* (*S) -= 2 */
		wsub(addr, 2);
		CYCLES(10, 3);
		break;
	case 10:  /* BL */
		/* BL ---- Branch and Link */
		/* IP=S, R11=old IP */
		WRITEREG(R11, I.PC);
		I.PC = addr;
		CYCLES(12, 5);
		break;
	case 11:  /* SWPB */
		/* SWPB -- SWaP Bytes */
		/* *S = swab(*S) */
		value = readword(addr);
		value = logical_right_shift(value, 8) | (value << 8);
		writeword(addr, value);
		CYCLES(10, 13);
		break;
	case 12:  /* SETO */
		/* SETO -- SET Ones */
		/* *S = #$FFFF */
		writeword(addr, 0xFFFF);
		CYCLES(10, 3);
		break;
	case 13:  /* ABS */
		/* ABS --- ABSolute value */
		/* *S = |*S| */
		/* clearing ST_C seems to be necessary, although ABS will never set it. */
#if (TMS99XX_MODEL <= TMS9985_ID)
		/* tms9900/tms9980 only write the result if it has changed */
		I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV);
#if (TMS99XX_MODEL == TMS9940_ID)
		/* I guess ST_DC is cleared here, too*/
		I.STATUS &= ~ ST_DC;
#endif
		value = readword(addr);

		CYCLES(12, Mooof!);

		if (((INT16) value) > 0)
			I.STATUS |= ST_LGT | ST_AGT;
		else if (((INT16) value) < 0)
		{
			I.STATUS |= ST_LGT;
			if (value == 0x8000)
				I.STATUS |= ST_OV;
#if (TMS99XX_MODEL == TMS9940_ID)
			if (! (value & 0x0FFF))
				I.STATUS |= ST_DC;
#endif
			writeword(addr, - ((INT16) value));
			CYCLES(2, Mooof!);
		}
		else
			I.STATUS |= ST_EQ;

#else
		/* tms9995 always write the result */
		I.STATUS &= ~ (ST_LGT | ST_AGT | ST_EQ | ST_C | ST_OV);
		value = readword(addr);

		CYCLES(12 /*Don't know for tms9989*/, 3);
		if (((INT16) value) > 0)
			I.STATUS |= ST_LGT | ST_AGT;
		else if (((INT16) value) < 0)
		{
			I.STATUS |= ST_LGT;
			if (value == 0x8000)
				I.STATUS |= ST_OV;
			value = - ((INT16) value);
		}
		else
			I.STATUS |= ST_EQ;

		writeword(addr, value);
#endif

		break;
#if (TMS99XX_MODEL >= TMS99105A_ID)
	/* "These opcode are designed to support the 99610 memory mapper, to allow easy access to
	another page without the need of switching a page someplace." */
	case 14:  /* LDS */
		/* LDS --- Long Distance Source */
		/* ... */
		break;
	case 15:  /* LDD */
		/* LDD --- Long Distance Destination */
		/* ... */
		break;
#else
	default:
		/* illegal instructions */
		HANDLE_ILLEGAL;
		break;
#endif
	}
}


/*==========================================================================
   Shift instructions,                                         >0800->0BFF
  --------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    | o p c o d e   |   C   |   W   |
    ---------------------------------

  SRA, SRL, SLA, SRC
============================================================================*/
static void h0800(UINT16 opcode)
{
	register UINT16 addr;
	register UINT16 cnt = (opcode & 0xF0) >> 4;
	register UINT16 value;

	addr = (opcode & 0xF);
	addr = ((addr+addr) + I.WP) & ~1;

	CYCLES(12, 5);

	if (cnt == 0)
	{
		CYCLES(8, 2);

		cnt = READREG(0) & 0xF;

		if (cnt == 0)
			cnt = 16;
	}

	CYCLES(cnt+cnt, cnt);

	switch ((opcode & 0x300) >> 8)
	{
	case 0:   /* SRA */
		/* SRA --- Shift Right Arithmetic */
		/* *W >>= C   (*W is filled on the left with a copy of the sign bit) */
		value = setst_sra_laec(readword(addr), cnt);
		writeword(addr, value);
		break;
	case 1:   /* SRL */
		/* SRL --- Shift Right Logical */
		/* *W >>= C   (*W is filled on the left with 0) */
		value = setst_srl_laec(readword(addr), cnt);
		writeword(addr, value);
		break;
	case 2:   /* SLA */
		/* SLA --- Shift Left Arithmetic */
		/* *W <<= C */
		value = setst_sla_laeco(readword(addr), cnt);
		writeword(addr, value);
		break;
	case 3:   /* SRC */
		/* SRC --- Shift Right Circular */
		/* *W = rightcircularshift(*W, C) */
		value = setst_src_laec(readword(addr), cnt);
		writeword(addr, value);
		break;
	}
}


#if (TMS99XX_MODEL >= TMS99105A_ID)
/*==========================================================================
   Additionnal instructions,                                   >0C00->0C0F
   Additionnal single-register instructions,                   >0C10->0C3F
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |     o p c o d e               |
    |     o p c o d e       | reg # |
    ---------------------------------

tms99xxx : TMB, TCMB, TSMB
tms99110a : CRI, NEGR, CRE, CER
============================================================================*/
static void h0c00(UINT16 opcode)
{
	if (opcode & 0x30)
	{
#if 0
		switch ((opcode & 0x30) >> 4)
		{
		case 1:
			/* INSF -- INSert Field */
		case 2:
			/* XV ---- eXtract Value */
		case 3:
			/* XF ---- eXtract Field */

			break;
		}
#else
		HANDLE_ILLEGAL;
#endif
	}
	else
	{
		switch (opcode & 0x0F)
		{
#if (TMS99XX_MODEL == TMS99110A_ID)
		/* floating point instructions */
		case 0:
			/* CRI --- Convert Real to Integer */

			break;
		case 2:
			/* NEGR -- NEGate Real */

			break;
		case 4:
			/* CRE --- Convert Real to Extended integer */

			break;
		case 6:
			/* CER --- Convert Extended integer to Real */

			break;
#endif

		/* The next three instructions allow to handle multiprocessor systems */
		case 9:
			/* TMB --- Test Memory Bit */

			break;
		case 10:
			/* TCMB -- Test and Clear Memory Bit */

			break;
		case 11:
			/* TSMB -- Test and Set Memory Bit */

			break;

#if 0
		/* the four next instructions support BCD */
		case 1:
			/* CDI --- Convert Decimal to Integer */
		case 3:
			/* NEGD -- NEGate Decimal */
		case 5:
			/* CDE --- Convert Decimal to Extended integer */
		case 7:
			/* CED --- Convert Extended integer to Decimal */

		case 8:
			/* NRM --- NoRMalize */

	case 12:
			/* SRJ --- Subtract from Register and Jump */
		case 13:
			/* ARJ --- Add to Register and Jump */

		case 14:
		case 15:
			/* XIT --- eXIT from floating point interpreter (???) */
			/* Completely alien to me.  Must have existed on weird TI990 variants. */

			break;
#endif
		default:
			HANDLE_ILLEGAL;
			break;
		}
	}
}


/*==========================================================================
   Additionnal single-operand instructions,                    >0C40->0FFF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |    o p c o d e    |TS |   S   |
    ---------------------------------

tms99110a : AR, CIR, SR, MR, DR, LR, STR
============================================================================*/
static void h0c40(UINT16 opcode)
{
	register UINT16 src;

	src = decipheraddr(opcode) & ~1;

	switch ((opcode & 0x03C0) >> 6)
	{
#if (TMS99XX_MODEL == TMS99110A_ID)
	case 1:
		/* AR ---- Add Real */
	case 2:
		/* CIR --- Convert Integer to Real */
	case 3:
		/* SR ---- Subtract Real */
	case 4:
		/* MR ---- Multiply Real */
	case 5:
		/* DR ---- Divide Real */
	case 6:
		/* LR ---- Load Real */
	case 7:
		/* STR --- STore Real */
#endif
#if 0
	case 9:
		/* AD ---- Add Decimal */
	case 10:
		/* CID --- Convert Integer to Decimal */
	case 11:
		/* SD ---- Subtract Decimal */
	case 12:
		/* MD ---- Multiply Decimal */
	case 13:
		/* DD ---- Divide Decimal  */
	case 14:
		/* LD ---- Load Decimal */
	case 15:
		/* SD ---- Store Decimal */
#endif
	default:
		HANDLE_ILLEGAL;
		break;
	}
}
#endif


/*==========================================================================
   Jump, CRU bit instructions,                                 >1000->1FFF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |  o p c o d e  | signed offset |
    ---------------------------------

  JMP, JLT, JLE, JEQ, JHE, JGT, JNE, JNC, JOC, JNO, JL, JH, JOP
  SBO, SBZ, TB
============================================================================*/
static void h1000(UINT16 opcode)
{
	/* we convert 8 bit signed word offset to a 16 bit effective word offset. */
	register INT16 offset = ((INT8) opcode);


	switch ((opcode & 0xF00) >> 8)
	{
	case 0:   /* JMP */
		/* JMP --- unconditional JuMP */
		/* PC += offset */
		I.PC += (offset + offset);
		CYCLES(10, 3);
		break;
	case 1:   /* JLT */
		/* JLT --- Jump if Less Than (arithmetic) */
		/* if (A==0 && EQ==0), PC += offset */
		if (! (I.STATUS & (ST_AGT | ST_EQ)))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 2:   /* JLE */
		/* JLE --- Jump if Lower or Equal (logical) */
		/* if (L==0 || EQ==1), PC += offset */
		if ((! (I.STATUS & ST_LGT)) || (I.STATUS & ST_EQ))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 3:   /* JEQ */
		/* JEQ --- Jump if EQual */
		/* if (EQ==1), PC += offset */
		if (I.STATUS & ST_EQ)
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 4:   /* JHE */
		/* JHE --- Jump if Higher or Equal (logical) */
		/* if (L==1 || EQ==1), PC += offset */
		if (I.STATUS & (ST_LGT | ST_EQ))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 5:   /* JGT */
		/* JGT --- Jump if Greater Than (arithmetic) */
		/* if (A==1), PC += offset */
		if (I.STATUS & ST_AGT)
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 6:   /* JNE */
		/* JNE --- Jump if Not Equal */
		/* if (EQ==0), PC += offset */
		if (! (I.STATUS & ST_EQ))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 7:   /* JNC */
		/* JNC --- Jump if No Carry */
		/* if (C==0), PC += offset */
		if (! (I.STATUS & ST_C))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 8:   /* JOC */
		/* JOC --- Jump On Carry */
		/* if (C==1), PC += offset */
		if (I.STATUS & ST_C)
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 9:   /* JNO */
		/* JNO --- Jump if No Overflow */
		/* if (OV==0), PC += offset */
		if (! (I.STATUS & ST_OV))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 10:  /* JL */
		/* JL ---- Jump if Lower (logical) */
		/* if (L==0 && EQ==0), PC += offset */
		if (! (I.STATUS & (ST_LGT | ST_EQ)))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 11:  /* JH */
		/* JH ---- Jump if Higher (logical) */
		/* if (L==1 && EQ==0), PC += offset */
		if ((I.STATUS & ST_LGT) && ! (I.STATUS & ST_EQ))
		{
			I.PC += (offset + offset);
			CYCLES(10, 3);
		}
		else
			CYCLES(8, 3);
		break;
	case 12:  /* JOP */
		/* JOP --- Jump On (odd) Parity */
		/* if (P==1), PC += offset */
		{
			/* Let's set ST_OP. */
			int i;
			UINT8 a;
				a = lastparity;
			i = 0;

			while (a != 0)
			{
				if (a & 1)  /* If current bit is set, */
					i++;      /* increment bit count. */
				a >>= 1U;   /* Next bit. */
			}

			/* Set ST_OP bit. */
			/*if (i & 1)
				I.STATUS |= ST_OP;
			else
				I.STATUS &= ~ ST_OP;*/

			/* Jump accordingly. */
			if (i & 1)  /*(I.STATUS & ST_OP)*/
			{
				I.PC += (offset + offset);
				CYCLES(10, 3);
			}
			else
				CYCLES(8, 3);
		}

		break;
	case 13:  /* SBO */
		/* SBO --- Set Bit to One */
		/* CRU Bit = 1 */
		writeCRU((READREG(R12) >> 1) + offset, 1, 1);
		CYCLES(12, 8);
		break;
	case 14:  /* SBZ */
		/* SBZ --- Set Bit to Zero */
		/* CRU Bit = 0 */
		writeCRU((READREG(R12) >> 1) + offset, 1, 0);
		CYCLES(12, 8);
		break;
	case 15:  /* TB */
		/* TB ---- Test Bit */
		/* EQ = (CRU Bit == 1) */
		setst_e(readCRU((READREG(R12)>> 1) + offset, 1) & 1, 1);
		CYCLES(12, 8);
		break;
	}
}


/*==========================================================================
   General and One-Register instructions                       >2000->3FFF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |  opcode   |   D   |TS |   S   |
    ---------------------------------

  COC, CZC, XOR, LDCR, STCR, XOP, MPY, DIV
tms9940 : DCA, DCS, LIIM
==========================================================================*/

/* xop, ldcr and stcr are handled elsewhere */
static void h2000(UINT16 opcode)
{
	register UINT16 dest = (opcode & 0x3C0) >> 6;
	register UINT16 src;
	register UINT16 value;

	src = decipheraddr(opcode);

	switch ((opcode & 0x1C00) >> 10)
	{
	case 0:   /* COC */
		/* COC --- Compare Ones Corresponding */
		/* status E bit = (S&D == S) */
		dest = ((dest+dest) + I.WP) & ~1;
		src &= ~1;
		value = readword(src);
		setst_e(value & readword(dest), value);
		CYCLES(14, 4);
		break;
	case 1:   /* CZC */
		/* CZC --- Compare Zeroes Corresponding */
		/* status E bit = (S&~D == S) */
		dest = ((dest+dest) + I.WP) & ~1;
		src &= ~1;
		value = readword(src);
		setst_e(value & (~ readword(dest)), value);
		CYCLES(14, 4);
		break;
	case 2:   /* XOR */
		/* XOR --- eXclusive OR */
		/* D ^= S */
		dest = ((dest+dest) + I.WP) & ~1;
		src &= ~1;
		value = readword(dest) ^ readword(src);
		setst_lae(value);
		writeword(dest,value);
		CYCLES(14, 4);
		break;
	/*case 3:*/   /* XOP is implemented elsewhere */
	/*case 4:*/   /* LDCR is implemented elsewhere */
	/*case 5:*/   /* STCR is implemented elsewhere */
	case 6:   /* MPY */
		/* MPY --- MultiPlY  (unsigned) */
		/* Results:  D:D+1 = D*S */
		/* Note that early TMS9995 reportedly perform an extra dummy read in PC space */
		dest = ((dest+dest) + I.WP) & ~1;
		src &= ~1;
		{
			unsigned long prod = ((unsigned long) readword(dest)) * ((unsigned long) readword(src));
			writeword(dest, prod >> 16);
			writeword(dest+2, prod);
		}
		CYCLES(52, 23);
		break;
	case 7:   /* DIV */
		/* DIV --- DIVide    (unsigned) */
		/* D = D/S    D+1 = D%S */
		dest = ((dest+dest) + I.WP) & ~1;
		src &= ~1;
		{
			UINT16 d = readword(src);
			UINT16 hi = readword(dest);
			unsigned long divq = (((unsigned long) hi) << 16) | readword(dest+2);

			if (d <= hi)
			{
				I.STATUS |= ST_OV;
				CYCLES(16, 6);
			}
			else
			{
				I.STATUS &= ~ST_OV;
				writeword(dest, divq/d);
				writeword(dest+2, divq%d);
				/* tms9900 : from 92 to 124, possibly 92 + 2*(number of bits to 1 (or 0?) in quotient) */
				/* tms9995 : 28 is the worst case */
				CYCLES(92, 28);
			}
		}
		break;
	}
}

static void xop(UINT16 opcode)
{	/* XOP */
	/* XOP --- eXtended OPeration */
	/* WP = *(40h+D), PC = *(42h+D) */
	/* New R13=old WP, New R14=Old IP, New R15=Old ST */
	/* New R11=S */
	/* Xop bit set */

	register UINT16 immediate = (opcode & 0x3C0) >> 6;
	register UINT16 operand;

#if (TMS99XX_MODEL == TMS9940_ID)
		switch (immediate)
		{
		case 0: /* DCA */
			/* DCA --- Decimal Correct Addition */
			operand = decipheraddrbyte(opcode);
			{
			int value = readbyte(operand);
			int X = (value >> 4) & 0xf;
			int Y = value & 0xf;

			if (Y >= 10)
			{
				Y -= 10;
				I.STATUS |= ST_DC;
				X++;
			}
			else if (I.STATUS & ST_DC)
			{
				Y += 6;
			}

			if (X >= 10)
			{
				X -= 10;
				I.STATUS |= ST_C;
			}
			else if (I.STATUS & ST_C)
			{
				X += 6;
			}

			writebyte(operand, (X << 4) | Y);
			}
			break;
		case 1:	/* DCS */
			/* DCS --- Decimal Correct Substraction */
			operand = decipheraddrbyte(opcode);
			{
			int value = readbyte(operand);

			if (! (I.STATUS & ST_DC))
			{
				value += 10;
			}

			if (! (I.STATUS & ST_C))
			{
				value += 10 << 4;
			}

			I.STATUS ^= ST_DC;

			writebyte(operand, value);
			}
			break;
		case 2: /* LIIM */
		case 3: /* LIIM */
			/* LIIM - Load Immediate Interrupt Mask */
			/* Does the same job as LIMI, with a different opcode format. */
			/* Note that, unlike TMS9900, the interrupt mask is only 2-bit long. */
			operand = decipheraddr(opcode);	/* dummy decode (personnal guess) */

			I.STATUS = (I.STATUS & 0xFFFC) | (opcode & 0x0003);
			break;
		default:  /* normal XOP */
#endif

	operand = decipheraddr(opcode);

#if (TMS99XX_MODEL <= TMS9989_ID)
		(void)readword(operand & ~1); /*dummy read (personnal guess)*/
#endif

		contextswitch(0x40 + (immediate << 2));
#if (TMS99XX_MODEL != TMS9940_ID)
		/* The bit is not set on tms9940 */
		I.STATUS |= ST_X;
#endif
		WRITEREG(R11, operand);
		CYCLES(36, 15);
		disable_interrupt_recognition = 1;

#if (TMS99XX_MODEL == TMS9940_ID)
			break;
		}
#endif
}

/* LDCR and STCR */
static void ldcr_stcr(UINT16 opcode)
{
	register UINT16 cnt = (opcode & 0x3C0) >> 6;
	register UINT16 addr;
	register UINT16 value;

	if (cnt == 0)
		cnt = 16;

	if (cnt <= 8)
		addr = decipheraddrbyte(opcode);
	else
		addr = decipheraddr(opcode) & ~1;

	if (opcode < 0x3400)
	{	/* LDCR */
		/* LDCR -- LoaD into CRu */
		/* CRU R12--CRU R12+D-1 set to S */
		if (cnt <= 8)
		{
#if (TMS99XX_MODEL != TMS9995_ID)
			value = readbyte(addr);
#else
			/* just for once, tms9995 behaves like earlier 8-bit tms99xx chips */
			/* this must be because instruction decoding is too complex */
			value = readword(addr);
			if (addr & 1)
				value &= 0xFF;
			else
				value = (value >> 8) & 0xFF;
#endif
			(void)READREG(cnt+cnt); /*dummy read (reasonnable guess, cf TMS9995)*/
			setst_byte_laep(value);
			writeCRU((READREG(R12) >> 1), cnt, value);
		}
		else
		{
			value = readword(addr);
			(void)READREG(cnt+cnt); /*dummy read (reasonnable guess, cf TMS9995)*/
			setst_lae(value);
			writeCRU((READREG(R12) >> 1), cnt, value);
		}
		CYCLES(20 + cnt+cnt, 9 + cnt+cnt);
	}
	else
	{	/* STCR */
		/* STCR -- STore from CRu */
		/* S = CRU R12--CRU R12+D-1 */
		if (cnt <= 8)
		{
#if (TMS99XX_MODEL != TMS9995_ID)
			(void)readbyte(addr); /*dummy read*/
			(void)READREG(cnt+cnt); /*dummy read (reasonnable guess, cf TMS9995)*/
			value = readCRU((READREG(R12) >> 1), cnt);
			setst_byte_laep(value);
			writebyte(addr, value);
			CYCLES((cnt != 8) ? 42 : 44, 19 + cnt);
#else
			/* just for once, tms9995 behaves like earlier 8-bit tms99xx chips */
			/* this must be because instruction decoding is too complex */
			int value2 = readword(addr);

			READREG(cnt+cnt); /*dummy read (guessed from timing table)*/
			value = readCRU((READREG(R12) >> 1), cnt);
			setst_byte_laep(value);

			if (addr & 1)
				writeword(addr, (value & 0x00FF) | (value2 & 0xFF00));
			else
				writeword(addr, (value2 & 0x00FF) | ((value << 8) & 0xFF00));

			CYCLES(Mooof!, 19 + cnt);
#endif
		}
		else
		{
			(void)readword(addr); /*dummy read*/
			(void)READREG(cnt+cnt); /*dummy read (reasonnable guess, cf TMS9995)*/
			value = readCRU((READREG(R12) >> 1), cnt);
			setst_lae(value);
			writeword(addr, value);
			CYCLES((cnt != 16) ? 58 : 60, 27 + cnt);
		}
	}
}


/*==========================================================================
   Two-Operand instructions                                    >4000->FFFF
 ---------------------------------------------------------------------------

      0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ----------------------------------
    |opcode|B|TD |   D   |TS |   S   |
    ----------------------------------

  SZC, SZCB, S, SB, C, CB, A, AB, MOV, MOVB, SOC, SOCB
============================================================================*/

/* word instructions */
static void h4000w(UINT16 opcode)
{
	register UINT16 src;
	register UINT16 dest;
	register UINT16 value;
	int a,b;

	src = decipheraddr(opcode) & ~1;
	dest = decipheraddr(opcode >> 6) & ~1;

	switch ((opcode >> 13) & 0x0007)    /* ((opcode & 0xE000) >> 13) */
	{
	case 2:   /* SZC */
		/* SZC --- Set Zeros Corresponding */
		/* D &= ~S */
		value = readword(dest) & (~ readword(src));
		setst_lae(value);
		writeword(dest, value);
		CYCLES(14, 4);
		break;
	case 3:   /* S */
		/* S ----- Subtract */
		/* D -= S */
		a = readword(dest);
		b = readword(src);
		value = setst_sub_laeco(a, b);
		writeword(dest, value);
		CYCLES(14, 4);
		break;
	case 4:   /* C */
		/* C ----- Compare */
		/* ST = (D - S) */
		a = readword(dest);
		b = readword(src);
		setst_c_lae(a, b);
		CYCLES(14, 4);
		break;
	case 5:   /* A */
		/* A ----- Add */
		/* D += S */
		a = readword(dest);
		b = readword(src);
		value = setst_add_laeco(a, b);
		writeword(dest, value);
		CYCLES(14, 4);
		break;
	case 6:   /* MOV */
		/* MOV --- MOVe */
		/* D = S */
		value = readword(src);
		setst_lae(value);
#if (TMS99XX_MODEL <= TMS9989_ID)
		/* MOV performs a dummy read... */
		(void)readword(dest);
#endif
		writeword(dest, value);
		CYCLES(14, 3);
		break;
	case 7:   /* SOC */
		/* SOC --- Set Ones Corresponding */
		/* D |= S */
		value = readword(dest) | readword(src);
		setst_lae(value);
		writeword(dest, value);
		CYCLES(14, 4);
		break;
	}
}

/* byte instruction */
static void h4000b(UINT16 opcode)
{
	register UINT16 src;
	register UINT16 dest;
	register UINT16 value;

	src = decipheraddrbyte(opcode);
	dest = decipheraddrbyte(opcode >> 6);

	switch ((opcode >> 13) & 0x0007)    /* ((opcode & 0xE000) >> 13) */
	{
	case 2:   /* SZCB */
		/* SZCB -- Set Zeros Corresponding, Byte */
		/* D &= ~S */
		value = readbyte(dest) & (~ readbyte(src));
		setst_byte_laep(value);
		writebyte(dest, value);
		CYCLES(14, 4);
		break;
	case 3:   /* SB */
		/* SB ---- Subtract, Byte */
		/* D -= S */
		value = setst_subbyte_laecop(readbyte(dest), readbyte(src));
		writebyte(dest, value);
		CYCLES(14, 4);
		break;
	case 4:   /* CB */
		/* CB ---- Compare Bytes */
		/* ST = (D - S) */
		value = readbyte(src);
		setst_c_lae(readbyte(dest)<<8, value<<8);
		lastparity = value;
		CYCLES(14, 4);
		break;
	case 5:   /* AB */
		/* AB ---- Add, Byte */
		/* D += S */
		value = setst_addbyte_laecop(readbyte(dest), readbyte(src));
		writebyte(dest, value);
		break;
	case 6:   /* MOVB */
		/* MOVB -- MOVe Bytes */
		/* D = S */
		value = readbyte(src);
		setst_byte_laep(value);
#if (TMS99XX_MODEL <= TMS9989_ID)
		/* on tms9900, MOVB needs to read destination, because it cannot actually read one single byte.
		  It reads a word, replaces the revelant byte, then write the result */
		/* A tms9980 theorically does not need to do so, but still does... */
		readbyte(dest);
#endif
		writebyte(dest, value);
		CYCLES(14, 3);
		break;
	case 7:   /* SOCB */
		/* SOCB -- Set Ones Corresponding, Byte */
		/* D |= S */
		value = readbyte(dest) | readbyte(src);
		setst_byte_laep(value);
		writebyte(dest, value);
		CYCLES(14, 4);
		break;
	}
}


static INLINE void execute(UINT16 opcode)
{
#if (TMS99XX_MODEL <= TMS9985_ID)

	/* tms9900-like instruction set*/

	static void (* jumptable[128])(UINT16) =
	{
		&illegal,&h0200,&h0400,&h0400,&h0800,&h0800,&illegal,&illegal,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&xop,&xop,
		&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&h2000,&h2000,&h2000,&h2000,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b
	};

	(* jumptable[opcode >> 9])(opcode);

#elif (TMS99XX_MODEL <= TMS9995_ID)

	/* tms9989 and tms9995 include 4 extra instructions, and one additionnal instruction type */

	static void (* jumptable[256])(UINT16) =
	{
		&h0040,&h0100,&h0200,&h0200,&h0400,&h0400,&h0400,&h0400,
		&h0800,&h0800,&h0800,&h0800,&illegal,&illegal,&illegal,&illegal,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&xop,&xop,&xop,&xop,
		&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b
	};

	(* jumptable[opcode >> 8])(opcode);

#elif (TMS99XX_MODEL >= TMS99105A_ID)

	/* tms99xxx include even more instruction types */

	static void (* jumptable[1024])(UINT16) =
	{
		&h0000,&h0040,&h0040,&illegal,&h0100,&h0100,&h0100,&h0100,
		&h0200,&h0200,&h0200,&h0200,&h0200,&h0200,&h0200,&h0200,
		&h0400,&h0400,&h0400,&h0400,&h0400,&h0400,&h0400,&h0400,
		&h0400,&h0400,&h0400,&h0400,&h0400,&h0400,&h0400,&h0400,
		&h0800,&h0800,&h0800,&h0800,&h0800,&h0800,&h0800,&h0800,
		&h0800,&h0800,&h0800,&h0800,&h0800,&h0800,&h0800,&h0800,
#if (TMS99XX_MODEL == TMS99110A_ID)
		&h0c00,&h0c40,&h0c40,&h0c40,&h0c40,&h0c40,&h0c40,&h0c40,
#else
		&h0c00,&illegal,&illegal,&illegal,&illegal,&illegal,&illegal,&illegal,
#endif
		&illegal,&illegal,&illegal,&illegal,&illegal,&illegal,&illegal,&illegal,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&xop,&xop,&xop,&xop,&xop,&xop,&xop,&xop,
		&xop,&xop,&xop,&xop,&xop,&xop,&xop,&xop,
		&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,
		&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,
		&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,
		&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,&ldcr_stcr,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,&h4000w,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,
		&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b,&h4000b
	};

	(* jumptable[opcode >> 6])(opcode);

#endif
}
