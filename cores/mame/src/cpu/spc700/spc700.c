/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*


*/
/* ======================================================================== */
/* ================================= NOTES ================================ */
/* ======================================================================== */
/*
Questions:
	- How is H handled for ADDW and SUBW?
	- How is TCall handled?
	- What are the vectors?
	- Are there interrupts? how do they work?
	- Does I=1 mean interrupts enabled or disabled?
	- How are N and Z handled in DIV on divide by zero?
	- How is H handled in DIV and MUL?
	- What's the difference between SLEEP and STOP?

Memory:
0000-01ff: RAM (zero pages)
0200-1fff: RAM (max addressable for bit instructions)
2000-7fff: RAM
8000-ffbf: RAM (not connected in snes - only 32k of ram connected to SPC)
ffc0-ffff: initial program loader ROM

snes mapped ports: f0-ff
Address  Function Register  R/W  When Reset          Remarks

00F0H     (test)            ---  ------             Installed in sound-CPU
00F1H     Control            W   Control = "00-000"
00F2H    Register Add.      R/W  Indeterminate      Installed in DSP
00F3H    Register Data      R/W  Indeterminate      Installed in DSP
00F4H    Port-0             R/W  Port0r = "00"      Installed in sound-CPU
                                 Port0w = "00"
00F5H    Port-1             R/W  Port1r = "00"      Installed in sound-CPU
                                 Port1w = "00"
00F6H    Port-2             R/W  Port2r = "00"      Installed in sound-CPU
                                 Port2w = "00"
00F7H    Port-3             R/W  Port3r = "00"      Installed in sound-CPU
                                 Port3w = "00"
00F8H    ------             ---  ----------         -------------------
00F9H    ------             ---  ----------         -------------------
00FAH    Timer-0             W   Indeterminate      Installed in sound-CPU
00FBH    Timer-1             W   Indeterminate      Installed in sound-CPU
00FCH    Timer-2             W   Indeterminate      Installed in sound-CPU
00FDH    Counter-0           W   Indeterminate      Installed in sound-CPU
00FEH    Counter-1           W   Indeterminate      Installed in sound-CPU 
00FFH    Counter-2           W   Indeterminate      Installed in sound-CPU


SR bits B and I may not be present in snes version of spc...
instruction summary shows B and I being affected by BRK, EI, and DI
on reset SR is set to 000-0-00
spctodo mentions that V and H are linked???
IPL located in mask rom at fffc-ffff in SPC
spctodo says sleep and standby modes cannot be used.

*/
/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include <limits.h>
#include <stdio.h>
#include <retro_inline.h>
#include "spc700.h"


/* ======================================================================== */
/* ==================== ARCHITECTURE-DEPENDANT DEFINES ==================== */
/* ======================================================================== */

/* Fallback on static if we don't have inline */

/* This should be set to the default size of your processor (min 16 bit) */
#undef uint
#define uint unsigned int

#undef uint8
#define uint8 unsigned char

#undef int8

/* Allow for architectures that don't have 8-bit sizes */
#if UCHAR_MAX == 0xff
#define int8 char
#define MAKE_INT_8(A) (int8)((A)&0xff)
#else
#define int8   int
static INLINE int MAKE_INT_8(int A) {return (A & 0x80) ? A | ~0xff : A & 0xff;}
#endif /* UCHAR_MAX == 0xff */

#define MAKE_UINT_8(A) ((A)&0xff)
#define MAKE_UINT_16(A) ((A)&0xffff)



/* ======================================================================== */
/* =============================== PROTOTYPES ============================= */
/* ======================================================================== */

/* CPU Structure */
typedef struct
{
	uint a;			/* Accumulator */
	uint x;			/* Index Register X */
	uint y;			/* Index Register Y */
	uint s;			/* Stack Pointer */
	uint pc;		/* Program Counter */
	uint ppc;		/* Previous Program Counter */
	uint flag_nz;	/* Negative Flag and inverted Zero flag */
	uint flag_v;	/* Overflow Flag */
	uint flag_p;	/* Direct Page Flag */
	uint flag_b;	/* BRK Instruction Flag */
	uint flag_h;	/* Half-carry Flag */
	uint flag_i;	/* Interrupt Mask Flag */
	uint flag_c;	/* Carry Flag */
	uint line_irq;	/* Status of the IRQ line */
	uint line_nmi;	/* Status of the NMI line */
	uint line_rst;	/* Status of the RESET line */
	uint ir;		/* Instruction Register */
	int (*int_ack)(int); /* Interrupt Acknowledge */
	uint stopped;	/* stopped status */
} spc700i_cpu_struct;



/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

/* Our CPU structure */
static spc700i_cpu_struct spc700i_cpu = {0};

int spc700_ICount = 0;

/* Temporary Variables */
static uint spc700i_source;
static uint spc700i_destination;


/* Layout of the registers in the MAME debugger */
static unsigned char spc700_register_layout[] =
{
#if SPC700_OPTIMIZE_SNES
	SPC700_PC, SPC700_S, SPC700_P, SPC700_A, SPC700_X, SPC700_Y, 0,
#else
	SPC700_PC, SPC700_S, SPC700_P, SPC700_A, SPC700_X, SPC700_Y, -1,
	SPC700_NMI_STATE, SPC700_IRQ_STATE, 0
#endif /* SPC700_OPTIMIZE_SNES */
};

/* Layout of the MAME debugger windows x,y,w,h */
static unsigned char spc700_window_layout[] = {
	25, 0,55, 2, /* register window (top, right rows) */
	 0, 0,24,22, /* disassembler window (left colums) */
	25, 3,55, 9, /* memory #1 window (right, upper middle) */
	25,13,55, 9, /* memory #2 window (right, lower middle) */
	 0,23,80, 1, /* command line window (bottom rows) */
};



unsigned char daa_table[1024] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x66,0x66,0x66,0x66,0x66,0x66,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
	0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66
};



/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */
/* ======================================================================== */

/* Bits */
#define BIT_0		0x01
#define BIT_1		0x02
#define BIT_2		0x04
#define BIT_3		0x08
#define BIT_4		0x10
#define BIT_5		0x20
#define BIT_6		0x40
#define BIT_7		0x80
#define BIT_8		0x100

/* Flag positions in Processor Status Register */
#define FLAGPOS_N		BIT_7		/* Negative             */
#define FLAGPOS_V		BIT_6		/* Overflow             */
#define FLAGPOS_P		BIT_5		/* Direct Page Selector */
#define FLAGPOS_B		BIT_4		/* Break                */
#define FLAGPOS_H		BIT_3		/* Half-carry           */
#define FLAGPOS_I 		BIT_2		/* Interrupt            */
#define FLAGPOS_Z		BIT_1		/* Zero                 */
#define FLAGPOS_C		BIT_0		/* Carry                */

#define NFLAG_SET		FLAGPOS_N
#define VFLAG_SET		BIT_7
#define PFLAG_SET		BIT_8
#define BFLAG_SET		FLAGPOS_B
#define HFLAG_SET		0x10
#define IFLAG_SET		FLAGPOS_I
#define ZFLAG_SET		0
#define CFLAG_SET		BIT_8
#define NZFLAG_CLEAR	1
#define VFLAG_CLEAR		0
#define PFLAG_CLEAR		0
#define BFLAG_CLEAR		0
#define HFLAG_CLEAR		0
#define IFLAG_CLEAR		0
#define CFLAG_CLEAR 	0

#define NMI_SET		1
#define NMI_CLEAR	0
#define IRQ_SET		IFLAG_CLEAR
#define IRQ_CLEAR	IFLAG_SET

#define STACK_PAGE	0x100				/* Stack Page Offset */

#define VECTOR_RST	0xfffe				/* Reset */
#define VECTOR_BRK	0xfffc				/* Break Instruction ??? what is real vector? */
#define VECTOR_IRQ	0xfffc				/* IRQ ??? what is real vector? */
#define VECTOR_NMI	0xfffa				/* NMI ??? what is real vector? */

#define REG_A		spc700i_cpu.a		/* Accumulator */
#define REG_X		spc700i_cpu.x		/* Index X Register */
#define REG_Y		spc700i_cpu.y		/* Index Y Register */
#define REG_S		spc700i_cpu.s		/* Stack Pointer */
#define REG_PC		spc700i_cpu.pc		/* Program Counter */
#define REG_PPC		spc700i_cpu.ppc		/* Previous Program Counter */
#define REG_P		spc700i_cpu.p		/* Processor Status Register */
#define FLAG_NZ		spc700i_cpu.flag_nz	/* Negative Flag and inverted Zero flag */
#define FLAG_V		spc700i_cpu.flag_v	/* Overflow Flag */
#define FLAG_P		spc700i_cpu.flag_p	/* Direct Page Flag */
#define FLAG_B		spc700i_cpu.flag_b	/* BRK Instruction Flag */
#define FLAG_H		spc700i_cpu.flag_h	/* Decimal Mode Flag */
#define FLAG_I		spc700i_cpu.flag_i	/* Interrupt Mask Flag */
#define FLAG_C		spc700i_cpu.flag_c	/* Carry Flag */
#define LINE_IRQ	spc700i_cpu.line_irq	/* Status of the IRQ line */
#define LINE_NMI	spc700i_cpu.line_nmi	/* Status of the NMI line */
#define REG_IR		spc700i_cpu.ir		/* Instruction Register */
#define INT_ACK		spc700i_cpu.int_ack	/* Interrupt Acknowledge function pointer */
#define CLOCKS		spc700_ICount		/* Clock cycles remaining */
#define CPU_STOPPED	spc700i_cpu.stopped	/* Stopped status */

#define SRC			spc700i_source		/* Source Operand */
#define DST			spc700i_destination	/* Destination Operand */

#define STOP_LEVEL_STOP		1
#define STOP_LEVEL_SLEEP	2


/* ======================================================================== */
/* ============================ GENERAL MACROS ============================ */
/* ======================================================================== */

/* Codition code tests */
#define COND_CC()	(!(FLAG_C&0x100))	/* Carry Clear */
#define COND_CS()	(FLAG_C&0x100)		/* Carry Set */
#define COND_EQ()	(!FLAG_NZ)			/* Equal */
#define COND_NE()	FLAG_NZ				/* Not Equal */
#define COND_MI()	(FLAG_NZ&0x80)		/* Minus */
#define COND_PL()	(!(FLAG_NZ&0x80))	/* Plus */
#define COND_VC()	(!(FLAG_V&0x80))	/* Overflow Clear */
#define COND_VS()	(FLAG_V&0x80)		/* Overflow Set */

/* Set Overflow flag in math operations */
#define VFLAG_ADD_8(S, D, R)   ((S^R) & (D^R))
#define VFLAG_ADD_16(S, D, R) (((S^R) & (D^R))>>8)
#define VFLAG_SUB_8(S, D, R)   ((S^D) & (R^D))
#define VFLAG_SUB_16(S, D, R) (((S^D) & (R^D))>>8)

#define CFLAG_AS_1()     ((FLAG_C>>8)&1)
#define CFLAG_AS_NOT_1() (!(FLAG_C&CFLAG_SET))

#define NZFLAG_16(A) (((A)&0x7f) | (((A)>>1)&0x40) | (((A)>>8)&0xff))
#define CFLAG_16(A)  ((A)>>8)

/* ======================================================================== */
/* ============================ UTILITY MACROS ============================ */
/* ======================================================================== */

/* Use up clock cycles */
#define CLK(A) CLOCKS -= (A)
#define CLK_ALL() CLOCKS = 0

#define BREAKOUT break

static INLINE uint read_8_normal(uint address)
{
	address = MAKE_UINT_16(address);
	return spc700_read_8(address);
}

static INLINE uint read_8_immediate(uint address)
{
	address = MAKE_UINT_16(address);
	return spc700_read_8_immediate(address);
}

static INLINE uint read_8_instruction(uint address)
{
	address = MAKE_UINT_16(address);
	return spc700_read_8_instruction(address);
}

static INLINE uint read_8_direct(uint address)
{
	address = MAKE_UINT_8(address) | FLAG_P;
	return spc700_read_8_direct(address);
}

static INLINE void write_8_normal(uint address, uint value)
{
	address = MAKE_UINT_16(address);
	value = MAKE_UINT_8(value);
	spc700_write_8(address, value);
}

static INLINE void write_8_direct(uint address, uint value)
{
	address = MAKE_UINT_8(address) | FLAG_P;
	value = MAKE_UINT_8(value);
	spc700_write_8_direct(address, value);
}


static INLINE uint read_16_normal(uint address)
{
	return read_8_normal(address) | (read_8_normal(address+1)<<8);
}

static INLINE uint read_16_immediate(uint address)
{
	return read_8_immediate(address) | (read_8_immediate(address+1)<<8);
}

static INLINE uint read_16_direct(uint address)
{
	return read_8_direct(address) | (read_8_direct(address+1)<<8);
}

static INLINE void write_16_direct(uint address, uint value)
{
	write_8_direct(address, value);
	write_8_direct(address+1, value>>8);
}

/* Low level memory access macros */
#define read_8_NORM(A)		read_8_normal(A)
#define read_8_IMM(A)		read_8_immediate(A)
#define read_8_ABS(A)		read_8_normal(A)
#define read_8_ABX(A)		read_8_normal(A)
#define read_8_ABY(A)		read_8_normal(A)
#define read_8_AXI(A)		read_8_normal(A)
#define read_8_DP(A)		read_8_direct(A)
#define read_8_DPX(A)		read_8_direct(A)
#define read_8_DPY(A)		read_8_direct(A)
#define read_8_DPI(A)		read_8_normal(A)
#define read_8_DXI(A)		read_8_normal(A)
#define read_8_DIY(A)		read_8_normal(A)
#define read_8_STK(A)		read_8_normal(A)
#define read_8_XI(A)		read_8_direct(A)
#define read_8_XII(A)		read_8_direct(A)
#define read_8_YI(A)		read_8_direct(A)


#define read_16_NORM(A)		read_16_normal(A)
#define read_16_IMM(A)		read_16_immediate(A)
#define read_16_ABS(A)		read_16_absolute(A)
#define read_16_ABX(A)		read_16_normal(A)
#define read_16_DP(A)		read_16_direct(A)
#define read_16_DPX(A)		read_16_direct(A)
#define read_16_DPY(A)		read_16_direct(A)
#define read_16_DPI(A)		read_16_normal(A)
#define read_16_VEC(A)		read_16_normal(A)
#define read_16_XI(A)		read_16_direct(A)
#define read_16_XII(A)		read_16_direct(A)
#define read_16_YI(A)		read_16_direct(A)

#define write_8_NORM(A, V)	write_8_normal(A, V)
#define write_8_IMM(A, V)	write_8_normal(A, V)
#define write_8_ABS(A, V)	write_8_normal(A, V)
#define write_8_ABX(A, V)	write_8_normal(A, V)
#define write_8_ABY(A, V)	write_8_normal(A, V)
#define write_8_AXI(A, V)	write_8_normal(A, V)
#define write_8_DP(A, V)	write_8_direct(A, V)
#define write_8_DPX(A, V)	write_8_direct(A, V)
#define write_8_DPY(A, V)	write_8_direct(A, V)
#define write_8_DPI(A, V)	write_8_normal(A, V)
#define write_8_DXI(A, V)	write_8_normal(A, V)
#define write_8_DIY(A, V)	write_8_normal(A, V)
#define write_8_STK(A, V)	write_8_normal(A, V)
#define write_8_XI(A, V)	write_8_direct(A, V)
#define write_8_XII(A, V)	write_8_direct(A, V)
#define write_8_YI(A, V)	write_8_direct(A, V)

#define write_16_NORM(A, V)	write_16_normal(A, V)
#define write_16_ABS(A, V)	write_16_normal(A, V)
#define write_16_ABX(A, V)	write_16_normal(A, V)
#define write_16_ABY(A, V)	write_16_normal(A, V)
#define write_16_AXI(A, V)	write_16_normal(A, V)
#define write_16_DP(A, V)	write_16_direct(A, V)
#define write_16_DPX(A, V)	write_16_direct(A, V)
#define write_16_DPY(A, V)	write_16_direct(A, V)
#define write_16_DPI(A, V)	write_16_normal(A, V)
#define write_16_DXI(A, V)	write_16_normal(A, V)
#define write_16_DIY(A, V)	write_16_normal(A, V)
#define write_16_STK(A, V)	write_16_normal(A, V)
#define write_16_XI(A, V)	write_16_direct(A, V)
#define write_16_XII(A, V)	write_16_direct(A, V)
#define write_16_YI(A, V)	write_16_direct(A, V)


#define OPER_8_IMM()	read_8_IMM(EA_IMM())
#define OPER_8_ABS()	read_8_ABS(EA_ABS())
#define OPER_8_ABX()	read_8_ABX(EA_ABX())
#define OPER_8_ABY()	read_8_ABY(EA_ABY())
#define OPER_8_AXI()	read_8_IND(EA_IND())
#define OPER_8_DP()		read_8_DP(EA_DP())
#define OPER_8_DPX()	read_8_DPX(EA_DPX())
#define OPER_8_DPY()	read_8_DPY(EA_DPY())
#define OPER_8_DPI()	read_8_DPI(EA_DPI())
#define OPER_8_DXI()	read_8_DXI(EA_DXI())
#define OPER_8_DIY()	read_8_DIY(EA_DIY())
#define OPER_8_XI()		read_8_XI(EA_XI())
#define OPER_8_XII()	read_8_XI(EA_XII())
#define OPER_8_YI()		read_8_YI(EA_YI())

#define OPER_16_IMM()	read_16_IMM(EA_IMM16())
#define OPER_16_ABS()	read_16_ABS(EA_ABS())
#define OPER_16_ABX()	read_16_ABX(EA_ABX())
#define OPER_16_ABY()	read_16_ABY(EA_ABY())
#define OPER_16_AXI()	read_16_IND(EA_IND())
#define OPER_16_DP()	read_16_DP(EA_DP())
#define OPER_16_DPX()	read_16_DPX(EA_DPX())
#define OPER_16_DPY()	read_16_DPY(EA_DPY())
#define OPER_16_DPI()	read_16_DPI(EA_DXI())
#define OPER_16_DXI()	read_16_DXI(EA_DXI())
#define OPER_16_DIY()	read_16_DIY(EA_DIY())
#define OPER_16_XI()	read_16_XI(EA_XI())
#define OPER_16_XII()	read_16_XI(EA_XII())
#define OPER_16_YI()	read_16_YI(EA_YI())

/* Effective Address Caluclations */
static INLINE uint EA_IMM(void)   {return REG_PC++;}
static INLINE uint EA_IMM16(void) {REG_PC += 2; return REG_PC-2;}
static INLINE uint EA_ABS(void)   {return OPER_16_IMM();}
static INLINE uint EA_ABX(void)   {return EA_ABS() + REG_X;}
static INLINE uint EA_ABY(void)   {return EA_ABS() + REG_Y;}
static INLINE uint EA_AXI(void)   {return OPER_16_ABX();}
static INLINE uint EA_DP(void)   {return OPER_8_IMM();}
static INLINE uint EA_DPX(void)   {return (EA_DP() + REG_X)&0xff;}
static INLINE uint EA_DPY(void)   {return (EA_DP() + REG_Y)&0xff;}
static INLINE uint EA_DPI(void)   {return OPER_16_DP();}
static INLINE uint EA_DXI(void)   {return OPER_16_DPX();}
static INLINE uint EA_DIY(void)   {uint addr = OPER_16_DP(); if((addr&0xff00) != ((addr+REG_Y)&0xff00)) CLK(1); return addr + REG_Y;}
static INLINE uint EA_XI(void)    {return REG_X;}
static INLINE uint EA_XII(void)   {uint val = REG_X;REG_X = MAKE_UINT_8(REG_X+1);return val;}
static INLINE uint EA_YI(void)    {return REG_Y;}



/* Change the Program Counter */
static INLINE void JUMP(uint address)
{
	REG_PC = address;
	spc700_jumping(REG_PC);
}

static INLINE void BRANCH(uint offset)
{
	REG_PC = MAKE_UINT_16(REG_PC + MAKE_INT_8(offset));
	spc700_branching(REG_PC);
}


#define GET_REG_YA() (REG_A | (REG_Y<<8))

static INLINE void SET_REG_YA(uint value)
{
	REG_A = MAKE_UINT_8(value);
	REG_Y = MAKE_UINT_8(value>>8);
}

/* Get the Processor Status Register */
#define GET_REG_P()				\
	((FLAG_NZ & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	(FLAG_P>>3)				|	\
	FLAG_B					|	\
	((FLAG_H&0x10)>>1)		|	\
	FLAG_I					|	\
	((!FLAG_NZ) << 1)		|	\
	CFLAG_AS_1())

/* Get Processor Status Register with B flag set (when executing BRK instruction) */
#define GET_REG_P_BRK()			\
	((FLAG_NZ & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	(FLAG_P>>3)				|	\
	FLAGPOS_B				|	\
	((FLAG_H&0x10)>>1)		|	\
	FLAG_I					|	\
	((!FLAG_NZ) << 1)		|	\
	CFLAG_AS_1())

/* Get Processor Status Register with B flag cleared (when servicing an interrupt) */
#define GET_REG_P_INT()			\
	((FLAG_NZ & 0x80)		|	\
	((FLAG_V & 0x80) >> 1)	|	\
	(FLAG_P>>3)				|	\
	((FLAG_H&0x10)>>1)		|	\
	FLAG_I					|	\
	((!FLAG_NZ) << 1)		|	\
	CFLAG_AS_1())

static INLINE void SET_FLAG_I(uint value);

/* Set the Process Status Register */
static INLINE void SET_REG_P(uint value)
{
	FLAG_NZ = (value & 0x80) | !(value & 2);
	FLAG_V = value<<1;
	FLAG_P = (value & FLAGPOS_P) << 3;
	FLAG_B = value & FLAGPOS_B;
	FLAG_H = value << 1;
	FLAG_C = value << 8;
	SET_FLAG_I(value);
}

/* Push/Pull data to/from the stack */
static INLINE void PUSH_8(uint value)
{
	write_8_STK(REG_S+STACK_PAGE, value);
	REG_S = MAKE_UINT_8(REG_S - 1);
}

static INLINE uint PULL_8(void)
{
	REG_S = MAKE_UINT_8(REG_S + 1);
	return read_8_STK(REG_S+STACK_PAGE);
}

static INLINE void PUSH_16(uint value)
{
	PUSH_8(value>>8);
	PUSH_8(value);
}

static INLINE uint PULL_16(void)
{
	uint value = PULL_8();
	return value | (PULL_8()<<8);
}

static INLINE void SERVICE_IRQ(void)
{
	CLK(7);
	PUSH_16(REG_PC);
	PUSH_8(GET_REG_P_INT());
	FLAG_I = IFLAG_SET;
	if(INT_ACK)
		INT_ACK(0);
	JUMP(read_16_VEC(VECTOR_IRQ));
}

#if !SPC700_OPTIMIZE_SNES
static INLINE void CHECK_IRQ(void)
{
	if(FLAG_I & LINE_IRQ)
		SERVICE_IRQ();
}
#else
#define CHECK_IRQ()
#endif /* SPC700_OPTIMIZE_SNES */

static INLINE void SET_FLAG_I(uint value)
{
	FLAG_I = value & IFLAG_SET;
	CHECK_IRQ();
}

/* ======================================================================== */
/* =========================== OPERATION MACROS =========================== */
/* ======================================================================== */

/* Add With Carry */
#define OP_ADC(BCLK, MODE)													\
			CLK(BCLK);														\
			SRC     = OPER_8_##MODE();										\
			FLAG_C  = REG_A + SRC + CFLAG_AS_1();							\
			FLAG_H  = (REG_A&15) + (SRC&15) + CFLAG_AS_1();					\
			FLAG_V  = VFLAG_ADD_8(SRC, REG_A, FLAG_C);						\
			REG_A   = FLAG_NZ = MAKE_UINT_8(FLAG_C)

/* Add With Carry to memory */
#define OP_ADCM(BCLK, SMODE, DMODE)											\
			CLK(BCLK);														\
			SRC     = OPER_8_##SMODE();										\
			DST     = EA_##DMODE();											\
			FLAG_NZ = read_8_##DMODE(DST);	/* used as a temp val */		\
			FLAG_C  = FLAG_NZ + SRC + CFLAG_AS_1();							\
			FLAG_V  = VFLAG_ADD_8(SRC, FLAG_NZ, FLAG_C);					\
			FLAG_NZ = MAKE_UINT_8(FLAG_C);									\
			write_8_##DMODE(DST, FLAG_NZ)

/* Add word */
#define OP_ADDW(BCLK)														\
			CLK(BCLK);														\
			SRC     = OPER_16_DP();											\
			DST     = GET_REG_YA();											\
			FLAG_C  = DST + SRC;											\
			FLAG_H  = (DST&0xff) + (SRC&0xff);								\
			FLAG_V  = VFLAG_ADD_16(SRC, DST, FLAG_C);						\
			SET_REG_YA(MAKE_UINT_16(FLAG_C));								\
			FLAG_NZ = NZFLAG_16(FLAG_C);									\
			FLAG_C  = CFLAG_16(FLAG_C);										\

/* Logical AND with accumulator */
#define OP_AND(BCLK, MODE)													\
			CLK(BCLK);														\
			FLAG_NZ = REG_A &= OPER_8_##MODE()

/* Logical AND operand */
#define OP_ANDM(BCLK, SMODE, DMODE)											\
			CLK(BCLK);														\
			FLAG_NZ = OPER_8_##SMODE();										\
			DST     = EA_##DMODE();											\
			FLAG_NZ &= read_8_##DMODE(DST);									\
			write_8_##DMODE(DST, FLAG_NZ)

/* Logical AND bit to C */
#define OP_AND1(BCLK)														\
			CLK(BCLK);														\
			DST = EA_IMM16();												\
			if(FLAG_C & CFLAG_SET)											\
			{																\
				DST = read_16_IMM(DST);										\
				SRC = 1 << (DST >> 13);										\
				DST &= 0x1fff;												\
				if(!(read_8_NORM(DST) & SRC))								\
					FLAG_C = CFLAG_CLEAR;									\
			}

/* AND negated bit to C */
#define OP_ANDN1(BCLK)														\
			CLK(BCLK);														\
			DST = EA_IMM16();												\
			if(FLAG_C & CFLAG_SET)											\
			{																\
				DST = read_16_IMM(DST);										\
				SRC = 1 << (DST >> 13);										\
				DST &= 0x1fff;												\
				if(read_8_NORM(DST) & SRC)									\
					FLAG_C = CFLAG_CLEAR;									\
			}

/* Arithmetic Shift Left accumulator */
#define OP_ASL(BCLK)														\
			CLK(BCLK);														\
			FLAG_C  = REG_A << 1;											\
			FLAG_NZ = REG_A = MAKE_UINT_8(FLAG_C)

/* Arithmetic Shift Left operand */
#define OP_ASLM(BCLK, MODE)													\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_C  = read_8_##MODE(DST) << 1;								\
			FLAG_NZ = MAKE_UINT_8(FLAG_C);									\
			write_8_##MODE(DST, FLAG_NZ)

/* Branch if Bit Reset */
#define OP_BBC(BCLK, BIT)													\
			CLK(BCLK);														\
			SRC     = OPER_8_DP();											\
			DST     = OPER_8_IMM();											\
			if(!(SRC & BIT))												\
			{																\
				CLK(1);														\
				BRANCH(DST);												\
			}

/* Branch if Bit Set */
#define OP_BBS(BCLK, BIT)													\
			CLK(BCLK);														\
			SRC     = OPER_8_DP();											\
			DST     = OPER_8_IMM();											\
			if(SRC & BIT)													\
			{																\
				CLK(1);														\
				BRANCH(DST);												\
			}

/* Branch on Condition Code */
#define OP_BCC(BCLK, COND)													\
			CLK(BCLK);														\
			DST     = OPER_8_IMM();											\
			if(COND)														\
			{																\
				CLK(1);														\
				BRANCH(DST);												\
			}

/* Branch Unconditional */
/* speed up busy loops */
#define OP_BRA(BCLK)														\
			CLK(BCLK);														\
			BRANCH(OPER_8_IMM());											\
			if(REG_PC == REG_PPC)											\
				CLK_ALL()

/* Cause a Break interrupt */
#define OP_BRK(BCLK)														\
			CLK(BCLK);														\
			PUSH_16(REG_PC);												\
			PUSH_8(GET_REG_P_BRK());										\
			JUMP(read_16_VEC(VECTOR_BRK))

/* Call subroutine */
#define OP_CALL(BCLK)														\
			CLK(BCLK);														\
			DST     = EA_ABS();												\
			PUSH_16(REG_PC);												\
			JUMP(DST)

/* Compare accumulator and branch if not equal */
#define OP_CBNE(BCLK, MODE)													\
			CLK(BCLK);														\
			SRC     = OPER_8_##MODE();										\
			DST     = EA_IMM();												\
			if(SRC != REG_A)												\
			{																\
				CLK(1);														\
				BRANCH(read_8_IMM(DST));									\
			}

/* Clear Carry flag */
#define OP_CLRC(BCLK)														\
			CLK(BCLK);														\
			FLAG_C  = CFLAG_CLEAR

/* Clear Memory Bit */
#define OP_CLR(BCLK, BIT)													\
			CLK(BCLK);														\
			DST     = EA_DP();												\
			SRC     = read_8_DP(DST) & ~BIT;								\
			write_8_DP(DST, SRC)

/* Clear oVerflow flag */
#define OP_CLRV(BCLK)														\
			CLK(BCLK);														\
			FLAG_V  = VFLAG_CLEAR

/* Clear the Page flag */
#define OP_CLRP(BCLK)														\
			CLK(BCLK);														\
			FLAG_P  = PFLAG_CLEAR

/* Compare operand to register */
/* Unusual behavior: C is inverted */
#define OP_CMPR(BCLK, REG, MODE)											\
			CLK(BCLK);														\
			SRC     = OPER_8_##MODE();										\
			FLAG_C  = REG - SRC;											\
			FLAG_NZ = MAKE_UINT_8(FLAG_C);									\
			FLAG_C  ^= FLAG_C

/* Compare memory */
/* Unusual behavior: C is inverted */
#define OP_CMPM(BCLK, SMODE, DMODE)											\
			CLK(BCLK);														\
			SRC     = OPER_8_##SMODE();										\
			FLAG_C  = OPER_8_##DMODE() - SRC;								\
			FLAG_NZ = MAKE_UINT_8(FLAG_C);									\
			FLAG_C ^= FLAG_C

/* Compare word */
/* Unusual behavior: C is inverted */
#define OP_CMPW(BCLK, MODE)													\
			CLK(BCLK);														\
			SRC     = OPER_16_##MODE();										\
			FLAG_C  = GET_REG_YA() - SRC;									\
			FLAG_NZ = NZFLAG_16(FLAG_C);									\
			FLAG_C  = ~CFLAG_16(FLAG_C)

/* Decimal adjust for addition */
#define OP_DAA(BCLK)														\
			CLK(BCLK);														\
			DST     = REG_A;												\
			SRC     = daa_table[REG_A | (FLAG_C&0x100) | ((FLAG_H<<5)&0x200)];	\
			FLAG_NZ = MAKE_UINT_8(REG_A) + SRC;								\
			FLAG_C  |= FLAG_NZ;												\
			FLAG_NZ = REG_A = MAKE_UINT_8(FLAG_NZ)

/* Decimal adjust for subtraction */
#define OP_DAS(BCLK)														\
			CLK(BCLK);														\
			DST     = REG_A;												\
			SRC     = daa_table[REG_A | (FLAG_C&0x100) | ((FLAG_H<<5)&0x200)];	\
			FLAG_NZ = MAKE_UINT_8(REG_A) - SRC;								\
			FLAG_C |= FLAG_NZ;												\
			FLAG_NZ = REG_A = MAKE_UINT_8(FLAG_NZ)

/* Decrement register and branch if not zero */
/* speed up busy loops */
#define OP_DBNZR(BCLK)														\
			CLK(BCLK);														\
			REG_Y  = MAKE_UINT_8(REG_Y - 1);								\
			DST    = EA_IMM();												\
			if(REG_Y != 0)													\
			{																\
				CLK(1);														\
				BRANCH(read_8_IMM(DST));									\
/* \
				if(REG_PC == REG_PPC)										\
				{															\
					CLK((BCLK+1)*(REG_Y+1));								\
					REG_Y = 0;												\
					if(CLOCKS < 0)											\
						CLOCKS = 0;											\
				}															\
*/ \
			}

/* Decrement operand and branch if not zero */
/* Speed up busy loops but do reads/writes for compatibility */
#define OP_DBNZM(BCLK)														\
			CLK(BCLK);														\
			DST     = EA_DP();												\
			SRC     = MAKE_UINT_8(read_8_DP(DST) - 1);						\
			write_8_DP(DST, SRC);											\
			DST = EA_IMM();													\
			if(SRC != 0)													\
			{																\
				CLK(1);														\
				BRANCH(read_8_IMM(DST));									\
/* \
				if(REG_PC == REG_PPC)										\
				{															\
					do														\
					{														\
						SRC = MAKE_UINT_8(read_8_DP(DST) - 1);				\
						write_8_DP(DST, SRC);								\
						CLK(BCLK+1);										\
					} while(SRC != 0 && CLOCKS > 0);						\
					if(CLOCKS > 0)											\
						BRANCH(3);											\
				}															\
*/ \
			}

/* Decrement register */
#define OP_DECR(BCLK, REG)													\
			CLK(BCLK);														\
			FLAG_NZ = REG = MAKE_UINT_8(REG - 1)

/* Decrement operand */
#define OP_DECM(BCLK, MODE)													\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_NZ = MAKE_UINT_8(read_8_##MODE(DST) - 1);					\
			write_8_##MODE(DST, FLAG_NZ)

/* Decrement word */
#define OP_DECW(BCLK)														\
			CLK(BCLK);														\
			DST     = EA_DP();												\
			FLAG_NZ = MAKE_UINT_16(read_16_DP(DST) - 1);					\
			write_16_DP(DST, FLAG_NZ);										\
			FLAG_NZ = NZFLAG_16(FLAG_NZ)

/* Disable interrupts */
#define OP_DI(BCLK)															\
			CLK(BCLK);														\
			FLAG_I  = IFLAG_CLEAR

/* Divide */
#define OP_DIV(BCLK)														\
			CLK(BCLK);														\
			if(REG_X != 0)													\
			{																\
				SRC     = GET_REG_YA();										\
				REG_A   = MAKE_UINT_8(SRC / REG_X);							\
				REG_Y   = SRC % REG_X;										\
				FLAG_NZ = REG_A;											\
				/* What about H flag? */									\
				FLAG_V  = VFLAG_CLEAR;										\
				BREAKOUT;													\
			}																\
			FLAG_NZ = REG_A = REG_Y = 0xff;									\
			/* What about N Z and H? */										\
			FLAG_V = VFLAG_SET												\

/* Enable interrupts */
#define OP_EI(BCLK)															\
			CLK(BCLK);														\
			FLAG_I  = IFLAG_SET

/* Exclusive Or operand to accumulator */
#define OP_EOR(BCLK, MODE)													\
			CLK(BCLK);														\
			FLAG_NZ = REG_A ^= OPER_8_##MODE()

/* Logical EOR operand */
#define OP_EORM(BCLK, SMODE, DMODE)											\
			CLK(BCLK);														\
			FLAG_NZ = OPER_8_##SMODE();										\
			DST     = EA_##DMODE();											\
			FLAG_NZ ^= read_8_##DMODE(DST);									\
			write_8_##DMODE(DST, FLAG_NZ)

/* Exclusive OR bit to C */
#define OP_EOR1(BCLK)														\
			CLK(BCLK);														\
			DST     = OPER_16_IMM();										\
			SRC     = 1 << (DST >> 13);										\
			DST     &= 0x1fff;												\
			if(read_8_NORM(DST) & SRC)										\
				FLAG_C = ~FLAG_C

/* Increment register */
#define OP_INCR(BCLK, REG)													\
			CLK(BCLK);														\
			FLAG_NZ = REG = MAKE_UINT_8(REG + 1)

/* Increment operand */
#define OP_INCM(BCLK, MODE)													\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_NZ = MAKE_UINT_8(read_8_##MODE(DST) + 1);					\
			write_8_##MODE(DST, FLAG_NZ)

/* Increment word */
#define OP_INCW(BCLK)														\
			CLK(BCLK);														\
			DST     = EA_DP();												\
			FLAG_NZ = MAKE_UINT_16(read_16_DP(DST) + 1);					\
			write_16_DP(DST, FLAG_NZ);										\
			FLAG_NZ = NZFLAG_16(FLAG_NZ)

/* Jump */
/* If we're in a busy loop, eat all clock cycles */
#define OP_JMP(BCLK, MODE)													\
			CLK(BCLK);														\
			JUMP(EA_##MODE());												\
			if(REG_PC == REG_PPC)											\
				CLK_ALL()

/* Jump to Subroutine */
#define OP_JSR(BCLK, MODE)													\
			CLK(BCLK);														\
			PUSH_16(REG_PC);												\
			JUMP(EA_##MODE())

/* Logical Shift Right accumulator */
#define OP_LSR(BCLK)														\
			CLK(BCLK);														\
			FLAG_C = REG_A << 8;											\
			FLAG_NZ = REG_A >>= 1

/* Logical Shift Right operand */
#define OP_LSRM(BCLK, MODE)													\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_NZ = read_8_##MODE(DST);									\
			FLAG_C  = FLAG_NZ << 8;											\
			FLAG_NZ >>= 1;													\
			write_8_##MODE(DST, FLAG_NZ)

/* Move from register to register */
#define OP_MOVRR(BCLK, SREG, DREG)											\
			CLK(BCLK);														\
			FLAG_NZ = DREG = SREG

/* Move from register to memory */
#define OP_MOVRM(BCLK, SREG, DMODE)											\
			CLK(BCLK);														\
			write_8_##DMODE(EA_##DMODE(), SREG)

/* Move from memory to register */
#define OP_MOVMR(BCLK, SMODE, DREG)											\
			CLK(BCLK);														\
			FLAG_NZ = DREG = OPER_8_##SMODE()

/* Move from memory to memory */
#define OP_MOVMM(BCLK, SMODE, DMODE)										\
			CLK(BCLK);														\
			SRC     = OPER_8_##SMODE();										\
			DST     = EA_##DMODE();											\
			write_8_##DMODE(DST, SRC)

/* Move word register to memory */
#define OP_MOVWRM(BCLK)														\
			CLK(BCLK);														\
			write_16_DP(EA_DP(), GET_REG_YA())

/* Move word memory to register */
#define OP_MOVWMR(BCLK)														\
			CLK(BCLK);														\
			FLAG_NZ = OPER_16_DP();											\
			SET_REG_YA(FLAG_NZ);											\
			FLAG_NZ = NZFLAG_16(FLAG_NZ)

/* Move from Stack pointer to X */
#define OP_MOVSX(BCLK)														\
			CLK(BCLK);														\
			FLAG_NZ = REG_X = REG_S

/* Move from X to Stack pointer */
#define OP_MOVXS(BCLK)														\
			CLK(BCLK);														\
			REG_S  = REG_X

/* Move bit from memory to C */
#define OP_MOV1C(BCLK)														\
			CLK(BCLK);														\
			DST     = OPER_16_IMM();										\
			SRC     = 1 << (DST >> 13);										\
			DST     &= 0x1fff;												\
			FLAG_C  = ((read_8_NORM(DST) & SRC) != 0) << 8

/* Move bit from C to memory */
#define OP_MOV1M(BCLK)														\
			CLK(BCLK);														\
			DST     = OPER_16_IMM();										\
			SRC     = 1 << (DST >> 13);										\
			DST     &= 0x1fff;												\
			if(FLAG_C & CFLAG_SET)											\
				write_8_NORM(DST, read_8_NORM(DST) | SRC);					\
			else															\
				write_8_NORM(DST, read_8_NORM(DST) & ~SRC)


/* Multiply A and Y and store result in YA */
#define OP_MUL(BCLK)														\
			CLK(BCLK);														\
			FLAG_NZ = REG_Y * REG_A;										\
			SET_REG_YA(FLAG_NZ);											\
			FLAG_NZ = NZFLAG_16(FLAG_NZ)

/* No Operation */
#define OP_NOP(BCLK)														\
			CLK(BCLK)

/* Invert the C flag */
#define OP_NOTC(BCLK)														\
			CLK(BCLK);														\
			FLAG_C  = ~FLAG_C

/* NOT bit */
#define OP_NOT1(BCLK)														\
			CLK(BCLK);														\
			DST     = OPER_16_IMM();										\
			SRC     = 1 << (DST >> 13);										\
			DST     &= 0x1fff;												\
			write_8_NORM(DST, read_8_NORM(DST) ^ SRC)

/* Logical OR operand to accumulator */
#define OP_OR(BCLK, MODE)													\
			CLK(BCLK);														\
			FLAG_NZ = REG_A |= OPER_8_##MODE()

/* Logical OR operand */
#define OP_ORM(BCLK, SMODE, DMODE)											\
			CLK(BCLK);														\
			FLAG_NZ = OPER_8_##SMODE();										\
			DST     = EA_##DMODE();											\
			FLAG_NZ |= read_8_##DMODE(DST);									\
			write_8_##DMODE(DST, FLAG_NZ)

/* Logical OR bit to C */
#define OP_OR1(BCLK)														\
			CLK(BCLK);														\
			DST = EA_IMM16();												\
			if(!(FLAG_C & CFLAG_SET))										\
			{																\
				DST = read_16_IMM(DST);										\
				SRC = 1 << (DST >> 13);										\
				DST &= 0x1fff;												\
				if(read_8_NORM(DST) & SRC)									\
					FLAG_C = CFLAG_SET;										\
			}

/* OR negated bit to C */
#define OP_ORN1(BCLK)														\
			CLK(BCLK);														\
			DST = EA_IMM16();												\
			if(!(FLAG_C & CFLAG_SET))										\
			{																\
				DST = read_16_IMM(DST);										\
				SRC = 1 << (DST >> 13);										\
				DST &= 0x1fff;												\
				if(!(read_8_NORM(DST) & SRC))								\
					FLAG_C = CFLAG_SET;										\
			}

/* UPage Call */
#define OP_PCALL(BCLK)														\
			CLK(BCLK);														\
			DST     = EA_DP();												\
			PUSH_16(REG_PC);												\
			JUMP(0xff00 | DST)

/* Push a register to the stack */
#define OP_PUSH(BCLK, REG)													\
			CLK(BCLK);														\
			PUSH_8(REG)

/* Push the Processor Status Register to the stack */
#define OP_PHP(BCLK)														\
			CLK(BCLK);														\
			PUSH_8(GET_REG_P())

/* Pull a register from the stack */
#define OP_PULL(BCLK, REG)													\
			CLK(BCLK);														\
			REG     = PULL_8()

/* Pull the Processor Status Register from the stack */
#define OP_PLP(BCLK)														\
			CLK(BCLK);														\
			SET_REG_P(PULL_8())

/* Return from Subroutine */
#define OP_RET(BCLK)														\
			CLK(BCLK);														\
			JUMP(PULL_16())

/* Return from Interrupt */
#define OP_RETI(BCLK)														\
			CLK(BCLK);														\
			SET_REG_P(PULL_8());											\
			JUMP(PULL_16())

/* Rotate Left the accumulator */
#define OP_ROL(BCLK)														\
			CLK(BCLK);														\
			FLAG_C  = (REG_A<<1) | CFLAG_AS_1();							\
			FLAG_NZ = REG_A = MAKE_UINT_8(FLAG_C)

/* Rotate Left an operand */
#define OP_ROLM(BCLK, MODE)													\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_C  = (read_8_##MODE(DST)<<1) | CFLAG_AS_1();				\
			FLAG_NZ = MAKE_UINT_8(FLAG_C);									\
			write_8_##MODE(DST, FLAG_NZ)

/* Rotate Right the accumulator */
#define OP_ROR(BCLK)														\
			CLK(BCLK);														\
			REG_A   |= FLAG_C & 0x100;										\
			FLAG_C  = REG_A << 8;											\
			FLAG_NZ = REG_A >>= 1

/* Rotate Right an operand */
#define OP_RORM(BCLK, MODE)													\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_NZ = read_8_##MODE(DST) | (FLAG_C & 0x100);				\
			FLAG_C  = FLAG_NZ << 8;											\
			FLAG_NZ >>= 1;													\
			write_8_##MODE(DST, FLAG_NZ)

/* Subtract with Carry */
/* Unusual behavior: C is inverted */
#define OP_SBC(BCLK, MODE)													\
			CLK(BCLK);														\
			SRC     = OPER_8_##MODE();										\
			FLAG_C  = REG_A - SRC - CFLAG_AS_NOT_1();						\
			FLAG_H  = (REG_A&15) - (SRC&15) - CFLAG_AS_NOT_1();				\
			FLAG_V  = VFLAG_SUB_8(SRC, REG_A, FLAG_C);						\
			FLAG_NZ = REG_A = MAKE_UINT_8(FLAG_C);							\
			FLAG_C ^= FLAG_C

/* Add With Carry to memory */
/* Unusual behavior: C is inverted */
#define OP_SBCM(BCLK, SMODE, DMODE)											\
			CLK(BCLK);														\
			SRC     = OPER_8_##SMODE();										\
			DST     = EA_##DMODE();											\
			FLAG_NZ = read_8_##DMODE(DST);	/* used as a temp val */		\
			FLAG_C  = FLAG_NZ - SRC - CFLAG_AS_NOT_1();						\
			FLAG_H  = (FLAG_NZ&15) - (SRC&15) - CFLAG_AS_NOT_1();			\
			FLAG_V  = VFLAG_SUB_8(SRC, FLAG_NZ, FLAG_C);					\
			FLAG_NZ = MAKE_UINT_8(FLAG_C);									\
			write_8_##DMODE(DST, FLAG_NZ);									\
			FLAG_C  ^= FLAG_C

/* Set Carry flag */
#define OP_SETC(BCLK)														\
			CLK(BCLK);														\
			FLAG_C  = CFLAG_SET

/* Set Page flag */
#define OP_SETP(BCLK)														\
			CLK(BCLK);														\
			FLAG_P  = PFLAG_SET

/* Set Memory Bit */
#define OP_SET(BCLK, BIT)													\
			CLK(BCLK);														\
			DST    = EA_DP();												\
			SRC    = read_8_DP(DST) | BIT;									\
			write_8_DP(DST, SRC)

/* Put the CPU to sleep */
#define OP_SLEEP(BCLK)														\
			CLK(BCLK);														\
			CPU_STOPPED |= STOP_LEVEL_SLEEP;								\
			CLK_ALL()

/* Stop the CPU */
#define OP_STOP(BCLK)														\
			CLK(BCLK);														\
			CPU_STOPPED |= STOP_LEVEL_STOP;									\
			CLK_ALL()

/* Subtract word */
#define OP_SUBW(BCLK)														\
			CLK(BCLK);														\
			SRC     = OPER_16_DP();											\
			DST     = GET_REG_YA();											\
			FLAG_C  = DST - SRC;											\
			FLAG_H  = (DST&0xff) - (SRC&0xff);								\
			FLAG_V  = VFLAG_SUB_16(SRC, DST, FLAG_C);						\
			SET_REG_YA(FLAG_C);												\
			FLAG_NZ = NZFLAG_16(FLAG_C);									\
			FLAG_C  = ~CFLAG_16(FLAG_C)

/* Table Call */
#define OP_TCALL(BCLK, NUM)													\
			CLK(BCLK);														\
			PUSH_16(REG_PC);												\
			JUMP(read_16_NORM(0xffc0 + ((15-NUM)<<1)))

/* Test and Clear Bits */
#define OP_TCLR1(BCLK, MODE)												\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_NZ = read_8_##MODE(DST);									\
			write_8_##MODE(DST, FLAG_NZ & ~REG_A);							\
			FLAG_NZ &= REG_A

/* Test and Set Bits */
#define OP_TSET1(BCLK, MODE)												\
			CLK(BCLK);														\
			DST     = EA_##MODE();											\
			FLAG_NZ = read_8_##MODE(DST);									\
			write_8_##MODE(DST, FLAG_NZ | REG_A);							\
			FLAG_NZ &= REG_A

/* Exchange high and low nybbles of accumulator */
#define OP_XCN(BCLK)														\
			CLK(BCLK);														\
			FLAG_NZ = REG_A = MAKE_UINT_8((REG_A<<4) | (REG_A>>4))

#define OP_ILLEGAL(BCLK)													\
			CLK(BCLK)


/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

void spc700_init(void) {}


void spc700_reset(void* param)
{
	CPU_STOPPED = 0;
#if !SPC700_OPTIMIZE_SNES
	LINE_IRQ = 0;
	LINE_NMI = 0;
#endif /* SPC700_OPTIMIZE_SNES */
	REG_S   = 0;
	FLAG_NZ = NZFLAG_CLEAR;
	FLAG_V  = VFLAG_CLEAR;
	FLAG_P  = PFLAG_CLEAR;
	FLAG_B  = BFLAG_CLEAR;
	FLAG_H  = HFLAG_CLEAR;
	FLAG_I  = IFLAG_CLEAR;
	FLAG_C  = CFLAG_CLEAR;
	JUMP(read_16_VEC(VECTOR_RST));
}

/* Exit and clean up */
void spc700_exit(void)
{
	/* nothing to do yet */
}


/* Get the current CPU context */
unsigned spc700_get_context(void *dst_context)
{
	if(dst_context)
		*(spc700i_cpu_struct*)dst_context = spc700i_cpu;
	return sizeof(spc700i_cpu);
}

/* Set the current CPU context */
void spc700_set_context(void *src_context)
{
	if(src_context)
	{
		spc700i_cpu = *(spc700i_cpu_struct*)src_context;
		JUMP(REG_PC);
	}
}

/* Get the current Program Counter */
unsigned spc700_get_pc(void)
{
	return REG_PC;
}

/* Set the Program Counter */
void spc700_set_pc(unsigned val)
{
	JUMP(val);
}

/* Get the current Stack Pointer */
unsigned spc700_get_sp(void)
{
	return REG_S + STACK_PAGE;
}

/* Set the Stack Pointer */
void spc700_set_sp(unsigned val)
{
	REG_S = MAKE_UINT_8(val);
}

/* Get a register from the CPU core */
unsigned spc700_get_reg(int regnum)
{
	switch(regnum)
	{
		case SPC700_PC: return REG_PC;
		case SPC700_S: return REG_S + STACK_PAGE;
		case SPC700_P: return GET_REG_P();
		case SPC700_A: return REG_A;
		case SPC700_X: return REG_X;
		case SPC700_Y: return REG_Y;
		case REG_PREVIOUSPC: return REG_PPC;
		default:
			if(regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = REG_S + STACK_PAGE + 2 * (REG_SP_CONTENTS - regnum);
				if(offset < 0x1ff)
					return read_8_STK(offset) | (read_8_STK(offset + 1) << 8);
			}
	}
	return 0;
}

/* Set a register in the CPU core */
void spc700_set_reg(int regnum, unsigned val)
{
	switch(regnum)
	{
		case SPC700_PC: REG_PC = MAKE_UINT_16(val); break;
		case SPC700_S: REG_S = MAKE_UINT_8(val); break;
		case SPC700_P: SET_REG_P(val); break;
		case SPC700_A: REG_A = MAKE_UINT_8(val); break;
		case SPC700_X: REG_X = MAKE_UINT_8(val); break;
		case SPC700_Y: REG_Y = MAKE_UINT_8(val); break;
		default:
			if(regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = REG_S + STACK_PAGE + 2 * (REG_SP_CONTENTS - regnum);
				if(offset < 0x1ff)
				{
					write_8_STK(offset, MAKE_UINT_8(val));
					write_8_STK(offset + 1, MAKE_UINT_8(val >> 8));
				}
			}
	 }
}


/* Assert or clear the NMI line of the CPU */
void spc700_set_nmi_line(int state)
{
#if !SPC700_OPTIMIZE_SNES
	if(state == CLEAR_LINE)
		LINE_NMI = 0;
	else if(!LINE_NMI)
	{
		LINE_NMI = 1;
		CLK(7);
		PUSH_16(REG_PC);
		PUSH_8(GET_REG_P_INT());
		JUMP(read_16_VEC(VECTOR_NMI));
	}
#endif /* SPC700_OPTIMIZE_SNES */
}

/* Assert or clear the IRQ line of the CPU */
void spc700_set_irq_line(int line, int state)
{
#if !SPC700_OPTIMIZE_SNES
	LINE_IRQ = (state != CLEAR_LINE) ? IRQ_SET : IRQ_CLEAR;
	CHECK_IRQ();
#endif /* SPC700_OPTIMIZE_SNES */
}

/* Set the callback that is called when servicing an interrupt */
void spc700_set_irq_callback(int (*callback)(int))
{
	INT_ACK = callback;
}

/* Save the current CPU state to disk */
void spc700_state_save(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	uint p = GET_REG_P();
	state_save_UINT16(file,"spc700",cpu,"PC",&REG_PC,2);
	state_save_UINT16(file,"spc700",cpu,"PPC",&REG_PPC,2);
	state_save_UINT8(file,"spc700",cpu,"SP",&REG_SP,1);
	state_save_UINT8(file,"spc700",cpu,"P",&p,1);
	state_save_UINT8(file,"spc700",cpu,"A",&REG_A,1);
	state_save_UINT8(file,"spc700",cpu,"X",&REG_X,1);
	state_save_UINT8(file,"spc700",cpu,"Y",&REG_Y,1);
	state_save_UINT8(file,"spc700",cpu,"IRQ",&LINE_IRQ,1);
	state_save_UINT8(file,"spc700",cpu,"NMI",&LINE_NMI,1);
#endif
}

/* Load a CPU state from disk */
void spc700_state_load(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	uint p;
	state_load_UINT16(file,"spc700",cpu,"PC",&REG_PC,2);
	state_load_UINT16(file,"spc700",cpu,"PPC",&REG_PPC,2);
	state_load_UINT8(file,"spc700",cpu,"SP",&REG_SP,1);
	state_load_UINT8(file,"spc700",cpu,"P",&p,1);
	state_load_UINT8(file,"spc700",cpu,"A",&REG_A,1);
	state_load_UINT8(file,"spc700",cpu,"X",&REG_X,1);
	state_load_UINT8(file,"spc700",cpu,"Y",&REG_Y,1);
	state_load_UINT8(file,"spc700",cpu,"IRQ",&LINE_IRQ,1);
	state_load_UINT8(file,"spc700",cpu,"NMI",&LINE_NMI,1);
	SET_REG_P(p);
#endif
}


/* Get a formatted string representing a register and its contents */
const char *spc700_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	spc700i_cpu_struct* r = context;
	uint p;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if(!context)
		r = &spc700i_cpu;

	p =  ((r->flag_nz & 0x80)			|
			((r->flag_v & 0x80) >> 1)	|
			r->flag_p>>3				|
			r->flag_b					|
			((r->flag_h&0x10) >> 1)		|
			r->flag_i					|
			((!r->flag_nz) << 1)		|
			((r->flag_c >> 8)&1));

	 switch(regnum)
	{
		case CPU_INFO_REG+SPC700_PC:		sprintf(buffer[which], "PC:%04X", r->pc); break;
		case CPU_INFO_REG+SPC700_S:			sprintf(buffer[which], "S:%02X", r->s); break;
		case CPU_INFO_REG+SPC700_P:			sprintf(buffer[which], "P:%02X", p); break;
		case CPU_INFO_REG+SPC700_A:			sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+SPC700_X:			sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+SPC700_Y:			sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				p & 0x80 ? 'N':'.',
				p & 0x40 ? 'V':'.',
				p & 0x20 ? 'P':'.',
				p & 0x10 ? 'B':'.',
				p & 0x08 ? 'H':'.',
				p & 0x04 ? 'I':'.',
				p & 0x02 ? 'Z':'.',
				p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "SPC700";
		case CPU_INFO_FAMILY: return "Sony SPC700";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) , all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)spc700_register_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)spc700_window_layout;
	}
	return buffer[which];
}


#ifdef MAME_DEBUG
#include "spc700ds.h"
#endif
/* Disassemble an instruction */
unsigned spc700_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return spc700_disassemble(buffer, pc);
#else
/*	sprintf(buffer, "$%02X", read_8_instruction(pc));*/
	return 1;
#endif
}



/* Execute instructions for <clocks> cycles */
int spc700_execute(int clocks)
{
	CLOCKS = CPU_STOPPED ? 0 : clocks;
	while(CLOCKS > 0)
	{
		REG_PPC = REG_PC;
		CALL_MAME_DEBUG;
		REG_PC++;

		switch(REG_IR = read_8_immediate(REG_PPC))
		{
			case 0x00: OP_NOP   ( 2               ); break; /* NOP           */
			case 0x01: OP_TCALL ( 8, 0            ); break; /* TCALL 0       */
			case 0x02: OP_SET   ( 4, BIT_0        ); break; /* SET 0         */
			case 0x03: OP_BBS   ( 5, BIT_0        ); break; /* BBS 0         */
			case 0x04: OP_OR    ( 3, DP           ); break; /* ORA dp        */
			case 0x05: OP_OR    ( 4, ABS          ); break; /* ORA abs       */
			case 0x06: OP_OR    ( 3, XI           ); break; /* ORA xi        */
			case 0x07: OP_OR    ( 6, DXI          ); break; /* ORA dxi       */
			case 0x08: OP_OR    ( 2, IMM          ); break; /* ORA imm       */
			case 0x09: OP_ORM   ( 6, DP , DP      ); break; /* ORM dp dp     */
			case 0x0a: OP_OR1   ( 5               ); break; /* OR1 bit       */
			case 0x0b: OP_ASLM  ( 4, DP           ); break; /* ASL dp        */
			case 0x0c: OP_ASLM  ( 4, ABS          ); break; /* ASL abs       */
			case 0x0d: OP_PHP   ( 4               ); break; /* PHP           */
			case 0x0e: OP_TSET1 ( 6, ABS          ); break; /* TSET1 abs     */
			case 0x0f: OP_BRK   ( 8               ); break; /* BRK           */
			case 0x10: OP_BCC   ( 2, COND_PL()    ); break; /* BPL           */
			case 0x11: OP_TCALL ( 8, 1            ); break; /* TCALL 1       */
			case 0x12: OP_CLR   ( 4, BIT_0        ); break; /* CLR 0         */
			case 0x13: OP_BBC   ( 5, BIT_0        ); break; /* BBC 0         */
			case 0x14: OP_OR    ( 4, DPX          ); break; /* ORA dpx       */
			case 0x15: OP_OR    ( 5, ABX          ); break; /* ORA abx       */
			case 0x16: OP_OR    ( 5, ABY          ); break; /* ORA aby       */
			case 0x17: OP_OR    ( 6, DIY          ); break; /* ORA diy       */
			case 0x18: OP_ORM   ( 6, IMM, DP      ); break; /* ORM dp, imm   */
			case 0x19: OP_ORM   ( 6, YI, XI       ); break; /* ORM xi, yi    */
			case 0x1a: OP_DECW  ( 6               ); break; /* DECW di       */
			case 0x1b: OP_ASLM  ( 5, DPX          ); break; /* ASL dpx       */
			case 0x1c: OP_ASL   ( 2               ); break; /* ASL a         */
			case 0x1d: OP_DECR  ( 2, REG_X        ); break; /* DEC x         */
			case 0x1e: OP_CMPR  ( 4, REG_X, ABS   ); break; /* CMP x, abs    */
			case 0x1f: OP_JMP   ( 6, AXI          ); break; /* JMP axi       */
			case 0x20: OP_CLRP  ( 2               ); break; /* CLRP          */
			case 0x21: OP_TCALL ( 8, 2            ); break; /* TCALL 2       */
			case 0x22: OP_SET   ( 4, BIT_1        ); break; /* SET 1         */
			case 0x23: OP_BBS   ( 5, BIT_1        ); break; /* BBS 1         */
			case 0x24: OP_AND   ( 3, DP           ); break; /* AND dp        */
			case 0x25: OP_AND   ( 4, ABS          ); break; /* AND abs       */
			case 0x26: OP_AND   ( 3, XI           ); break; /* AND xi        */
			case 0x27: OP_AND   ( 6, DXI          ); break; /* AND dxi       */
			case 0x28: OP_AND   ( 2, IMM          ); break; /* AND imm       */
			case 0x29: OP_ANDM  ( 6, DP , DP      ); break; /* AND dp, dp    */
			case 0x2a: OP_ORN1  ( 5               ); break; /* OR1 !bit      */
			case 0x2b: OP_ROLM  ( 4, DP           ); break; /* ROL dp        */
			case 0x2c: OP_ROLM  ( 5, ABS          ); break; /* ROL abs       */
			case 0x2d: OP_PUSH  ( 4, REG_A        ); break; /* PUSH a        */
			case 0x2e: OP_CBNE  ( 5, DP           ); break; /* CBNE dp       */
			case 0x2f: OP_BRA   ( 4               ); break; /* BRA           */
			case 0x30: OP_BCC   ( 2, COND_MI()    ); break; /* BMI           */
			case 0x31: OP_TCALL ( 8, 3            ); break; /* TCALL 3       */
			case 0x32: OP_CLR   ( 4, BIT_1        ); break; /* CLR 1         */
			case 0x33: OP_BBC   ( 5, BIT_1        ); break; /* BBC 1         */
			case 0x34: OP_AND   ( 4, DPX          ); break; /* AND dpx       */
			case 0x35: OP_AND   ( 5, ABX          ); break; /* AND abx       */
			case 0x36: OP_AND   ( 5, ABY          ); break; /* AND aby       */
			case 0x37: OP_AND   ( 6, DIY          ); break; /* AND diy       */
			case 0x38: OP_ANDM  ( 5, IMM, DP      ); break; /* AND dp, imm   */
			case 0x39: OP_ANDM  ( 5, YI , XI      ); break; /* AND xi, yi    */
			case 0x3a: OP_INCW  ( 6               ); break; /* INCW di       */
			case 0x3b: OP_ROLM  ( 5, DPX          ); break; /* ROL dpx       */
			case 0x3c: OP_ROL   ( 2               ); break; /* ROL acc       */
			case 0x3d: OP_INCR  ( 2, REG_X        ); break; /* INC x         */
			case 0x3e: OP_CMPR  ( 3, REG_X, DP    ); break; /* CMP x, dp     */
			case 0x3f: OP_CALL  ( 8               ); break; /* CALL abs      */
			case 0x40: OP_SETP  ( 2               ); break; /* RTI           */
			case 0x41: OP_TCALL ( 8, 4            ); break; /* TCALL 4       */
			case 0x42: OP_SET   ( 4, BIT_2        ); break; /* SET 2         */
			case 0x43: OP_BBS   ( 5, BIT_2        ); break; /* BBS 2         */
			case 0x44: OP_EOR   ( 3, DP           ); break; /* EOR dp        */
			case 0x45: OP_EOR   ( 4, ABS          ); break; /* EOR abs       */
			case 0x46: OP_EOR   ( 3, XI           ); break; /* EOR xi        */
			case 0x47: OP_EOR   ( 6, DXI          ); break; /* EOR dxi       */
			case 0x48: OP_EOR   ( 2, IMM          ); break; /* EOR imm       */
			case 0x49: OP_EORM  ( 6, DP, DP       ); break; /* EOR dp, dp    */
			case 0x4a: OP_AND1  ( 5               ); break; /* AND1 bit      */
			case 0x4b: OP_LSRM  ( 4, DP           ); break; /* LSR dp        */
			case 0x4c: OP_LSRM  ( 5, ABS          ); break; /* LSR abs       */
			case 0x4d: OP_PUSH  ( 4, REG_X        ); break; /* PUSH x        */
			case 0x4e: OP_TCLR1 ( 6, ABS          ); break; /* TCLR1 abs     */
			case 0x4f: OP_PCALL ( 6               ); break; /* PCALL         */
			case 0x50: OP_BCC   ( 2, COND_VC()    ); break; /* BVC           */
			case 0x51: OP_TCALL ( 8, 5            ); break; /* TCALL 5       */
			case 0x52: OP_CLR   ( 4, BIT_2        ); break; /* CLR 2         */
			case 0x53: OP_BBC   ( 5, BIT_2        ); break; /* BBC 2         */
			case 0x54: OP_EOR   ( 4, DPX          ); break; /* EOR dpx       */
			case 0x55: OP_EOR   ( 5, ABX          ); break; /* EOR abx       */
			case 0x56: OP_EOR   ( 5, ABY          ); break; /* EOR aby       */
			case 0x57: OP_EOR   ( 6, DIY          ); break; /* EOR diy       */
			case 0x58: OP_EORM  ( 5, IMM, DP      ); break; /* EOR dp, imm   */
			case 0x59: OP_EORM  ( 5, YI , XI      ); break; /* EOR xi, yi    */
			case 0x5a: OP_CMPW  ( 4, DP           ); break; /* CMPW dp       */
			case 0x5b: OP_LSRM  ( 5, DPX          ); break; /* LSR dpx       */
			case 0x5c: OP_LSR   ( 2               ); break; /* LSR           */
			case 0x5d: OP_MOVRR ( 2, REG_A, REG_X ); break; /* MOV X, A      */
			case 0x5e: OP_CMPR  ( 4, REG_Y, ABS   ); break; /* CMP Y, abs    */
			case 0x5f: OP_JMP   ( 3, ABS          ); break; /* JMP abs       */
			case 0x60: OP_CLRC  ( 2               ); break; /* CLRC          */
			case 0x61: OP_TCALL ( 8, 6            ); break; /* TCALL 6       */
			case 0x62: OP_SET   ( 4, BIT_3        ); break; /* SET 3         */
			case 0x63: OP_BBS   ( 5, BIT_3        ); break; /* BBS 3         */
			case 0x64: OP_CMPR  ( 3, REG_A, DP    ); break; /* CMP A, dp     */
			case 0x65: OP_CMPR  ( 4, REG_A, ABS   ); break; /* CMP A, abs    */
			case 0x66: OP_CMPR  ( 3, REG_A, XI    ); break; /* CMP A, xi     */
			case 0x67: OP_CMPR  ( 6, REG_A, DXI   ); break; /* CMP A, dxi    */
			case 0x68: OP_CMPR  ( 2, REG_A, IMM   ); break; /* CMP A, imm    */
			case 0x69: OP_CMPM  ( 6, DP, DP       ); break; /* CMP dp, dp    */
			case 0x6a: OP_ANDN1 ( 4               ); break; /* AND1 !bit     */
			case 0x6b: OP_RORM  ( 4, DP           ); break; /* ROR dp        */
			case 0x6c: OP_RORM  ( 5, ABS          ); break; /* ROR abs       */
			case 0x6d: OP_PUSH  ( 4, REG_Y        ); break; /* PUSH Y        */
			case 0x6e: OP_DBNZM ( 5               ); break; /* DBNZ dp       */
			case 0x6f: OP_RET   ( 5               ); break; /* RET           */
			case 0x70: OP_BCC   ( 2, COND_VS()    ); break; /* BVS           */
			case 0x71: OP_TCALL ( 8, 7            ); break; /* TCALL 7       */
			case 0x72: OP_CLR   ( 4, BIT_3        ); break; /* CLR 3         */
			case 0x73: OP_BBC   ( 5, BIT_3        ); break; /* BBC 3         */
			case 0x74: OP_CMPR  ( 4, REG_A, DPX   ); break; /* CMP A, dpx    */
			case 0x75: OP_CMPR  ( 5, REG_A, ABX   ); break; /* CMP A, abx    */
			case 0x76: OP_CMPR  ( 5, REG_A, ABY   ); break; /* CMP A, aby    */
			case 0x77: OP_CMPR  ( 6, REG_A, DIY   ); break; /* CMP A, diy    */
			case 0x78: OP_CMPM  ( 5, IMM, DP      ); break; /* CMP dp, imm   */
			case 0x79: OP_CMPM  ( 5, YI, XI       ); break; /* CMP xi, yi    */
			case 0x7a: OP_ADDW  ( 5               ); break; /* ADDW di       */
			case 0x7b: OP_RORM  ( 5, DPX          ); break; /* ROR dpx       */
			case 0x7c: OP_ROR   ( 2               ); break; /* ROR A         */
			case 0x7d: OP_MOVRR ( 2, REG_X, REG_A ); break; /* MOV A, X      */
			case 0x7e: OP_CMPR  ( 3, REG_Y, DP    ); break; /* CMP Y, dp     */
			case 0x7f: OP_RETI  ( 6               ); break; /* RETI          */
			case 0x80: OP_SETC  ( 2               ); break; /* SETC          */
			case 0x81: OP_TCALL ( 8, 8            ); break; /* TCALL 8       */
			case 0x82: OP_SET   ( 4, BIT_4        ); break; /* SET 4         */
			case 0x83: OP_BBS   ( 5, BIT_4        ); break; /* BBS 4         */
			case 0x84: OP_ADC   ( 3, DP           ); break; /* ADC dp        */
			case 0x85: OP_ADC   ( 4, ABS          ); break; /* ADC abs       */
			case 0x86: OP_ADC   ( 3, XI           ); break; /* ADC xi        */
			case 0x87: OP_ADC   ( 6, DXI          ); break; /* ADC dxi       */
			case 0x88: OP_ADC   ( 2, IMM          ); break; /* ADC imm       */
			case 0x89: OP_ADCM  ( 6, DP, DP       ); break; /* ADC dp, dp    */
			case 0x8a: OP_EOR1  ( 4               ); break; /* EOR1 bit      */
			case 0x8b: OP_DECM  ( 4, DP           ); break; /* DEC dp        */
			case 0x8c: OP_DECM  ( 5, ABS          ); break; /* DEC abs       */
			case 0x8d: OP_MOVMR ( 2, IMM, REG_Y   ); break; /* MOV Y, imm    */
			case 0x8e: OP_PLP   ( 4               ); break; /* POP PSW       */
			case 0x8f: OP_MOVMM ( 5, IMM, DP      ); break; /* MOV dp, imm   */
			case 0x90: OP_BCC   ( 2, COND_CC()    ); break; /* BCC           */
			case 0x91: OP_TCALL ( 8, 9            ); break; /* TCALL 9       */
			case 0x92: OP_CLR   ( 4, BIT_4        ); break; /* CLR 4         */
			case 0x93: OP_BBC   ( 5, BIT_4        ); break; /* BBC 4         */
			case 0x94: OP_ADC   ( 4, DPX          ); break; /* ADC dpx       */
			case 0x95: OP_ADC   ( 5, ABX          ); break; /* ADC abx       */
			case 0x96: OP_ADC   ( 5, ABY          ); break; /* ADC aby       */
			case 0x97: OP_ADC   ( 6, DIY          ); break; /* ADC diy       */
			case 0x98: OP_ADCM  ( 5, IMM, DP      ); break; /* ADC dp, imm   */
			case 0x99: OP_ADCM  ( 5, YI, XI       ); break; /* ADC xi, yi    */
			case 0x9a: OP_SUBW  ( 5               ); break; /* SUBW dp       */
			case 0x9b: OP_DECM  ( 5, DPX          ); break; /* DEC dpx       */
			case 0x9c: OP_DECR  ( 2, REG_A        ); break; /* DEC A         */
			case 0x9d: OP_MOVSX ( 2               ); break; /* MOV X, SP     */
			case 0x9e: OP_DIV   (12               ); break; /* DIV YA, X     */
			case 0x9f: OP_XCN   ( 5               ); break; /* XCN A         */
			case 0xa0: OP_EI    ( 3               ); break; /* EI            */
			case 0xa1: OP_TCALL ( 8, 10           ); break; /* TCALL 10      */
			case 0xa2: OP_SET   ( 4, BIT_5        ); break; /* SET 5         */
			case 0xa3: OP_BBS   ( 5, BIT_5        ); break; /* BBS 5         */
			case 0xa4: OP_SBC   ( 3, DP           ); break; /* SBC dp        */
			case 0xa5: OP_SBC   ( 4, ABS          ); break; /* SBC abs       */
			case 0xa6: OP_SBC   ( 3, XI           ); break; /* SBC xi        */
			case 0xa7: OP_SBC   ( 6, DXI          ); break; /* SBC dxi       */
			case 0xa8: OP_SBC   ( 2, IMM          ); break; /* SBC imm       */
			case 0xa9: OP_SBCM  ( 6, DP, DP       ); break; /* SBC dp, dp    */
			case 0xaa: OP_MOV1C ( 4               ); break; /* MOV1 bit->C   */
			case 0xab: OP_INCM  ( 4, DP           ); break; /* INC dp        */
			case 0xac: OP_INCM  ( 5, ABS          ); break; /* INC abs       */
			case 0xad: OP_CMPR  ( 2, REG_Y, IMM   ); break; /* CMP Y, imm    */
			case 0xae: OP_PULL  ( 4, REG_A        ); break; /* POP A         */
			case 0xaf: OP_MOVRM ( 4, REG_A, XII   ); break; /* MOV xii, A    */
			case 0xb0: OP_BCC   ( 2, COND_CS()    ); break; /* BCS           */
			case 0xb1: OP_TCALL ( 8, 11           ); break; /* TCALL 11      */
			case 0xb2: OP_CLR   ( 4, BIT_5        ); break; /* CLR 5         */
			case 0xb3: OP_BBC   ( 5, BIT_5        ); break; /* BBC 5         */
			case 0xb4: OP_SBC   ( 4, DPX          ); break; /* SBC dpx       */
			case 0xb5: OP_SBC   ( 5, ABX          ); break; /* SBC abx       */
			case 0xb6: OP_SBC   ( 5, ABY          ); break; /* SBC aby       */
			case 0xb7: OP_SBC   ( 6, DIY          ); break; /* SBC diy       */
			case 0xb8: OP_SBCM  ( 5, IMM, DP      ); break; /* SBC dp, imm   */
			case 0xb9: OP_SBCM  ( 5, YI, XI       ); break; /* SBC xi, yi    */
			case 0xba: OP_MOVWMR( 5               ); break; /* MOVW YA, dp   */
			case 0xbb: OP_INCM  ( 5, DPX          ); break; /* INC dpx       */
			case 0xbc: OP_INCR  ( 2, REG_A        ); break; /* INC A         */
			case 0xbd: OP_MOVXS ( 2               ); break; /* MOV SP, X     */
			case 0xbe: OP_DAS   ( 3               ); break; /* DAS A         */
			case 0xbf: OP_MOVMR ( 4, XII, REG_A   ); break; /* MOV A, xii    */
			case 0xc0: OP_DI    ( 3               ); break; /* DI            */
			case 0xc1: OP_TCALL ( 8, 12           ); break; /* TCALL 12      */
			case 0xc2: OP_SET   ( 4, BIT_6        ); break; /* SET 6         */
			case 0xc3: OP_BBS   ( 5, BIT_6        ); break; /* BBS 6         */
			case 0xc4: OP_MOVRM ( 4, REG_A, DP    ); break; /* MOV dp, A     */
			case 0xc5: OP_MOVRM ( 5, REG_A, ABS   ); break; /* MOV abs, A    */
			case 0xc6: OP_MOVRM ( 4, REG_A, XI    ); break; /* MOV xi, A     */
			case 0xc7: OP_MOVRM ( 7, REG_A, DXI   ); break; /* MOV dxi, A    */
			case 0xc8: OP_CMPR  ( 2, REG_X, IMM   ); break; /* CMP X, imm    */
			case 0xc9: OP_MOVRM ( 5, REG_X, ABS   ); break; /* MOV abs, X    */
			case 0xca: OP_MOV1M ( 6               ); break; /* MOV1 C->bit   */
			case 0xcb: OP_MOVRM ( 4, REG_Y, DP    ); break; /* MOV dp, Y     */
			case 0xcc: OP_MOVRM ( 5, REG_Y, ABS   ); break; /* MOV abs, Y    */
			case 0xcd: OP_MOVMR ( 2, IMM, REG_X   ); break; /* MOV X, imm    */
			case 0xce: OP_PULL  ( 4, REG_X        ); break; /* POP X         */
			case 0xcf: OP_MUL   ( 9               ); break; /* MUL YA        */
			case 0xd0: OP_BCC   ( 2, COND_NE()    ); break; /* BNE           */
			case 0xd1: OP_TCALL ( 8, 13           ); break; /* TCALL 13      */
			case 0xd2: OP_CLR   ( 4, BIT_6        ); break; /* CLR 6         */
			case 0xd3: OP_BBC   ( 5, BIT_6        ); break; /* BBC 6         */
			case 0xd4: OP_MOVRM ( 5, REG_A, DPX   ); break; /* MOV dpx, A    */
			case 0xd5: OP_MOVRM ( 6, REG_A, ABX   ); break; /* MOV abx, A    */
			case 0xd6: OP_MOVRM ( 6, REG_A, ABY   ); break; /* MOV aby, A    */
			case 0xd7: OP_MOVRM ( 7, REG_A, DIY   ); break; /* MOV diy, A    */
			case 0xd8: OP_MOVRM ( 4, REG_X, DP    ); break; /* MOV dp, X     */
			case 0xd9: OP_MOVRM ( 5, REG_X, DPY   ); break; /* MOV dpy, X    */
			case 0xda: OP_MOVWRM( 4               ); break; /* MOVW dp, YA   */
			case 0xdb: OP_MOVRM ( 5, REG_Y, DPX   ); break; /* MOV dpx, Y    */
			case 0xdc: OP_DECR  ( 2, REG_Y        ); break; /* DEC Y         */
			case 0xdd: OP_MOVRR ( 2, REG_Y, REG_A ); break; /* MOV A, Y      */
			case 0xde: OP_CBNE  ( 6, DPX          ); break; /* CBNE dpx      */
			case 0xdf: OP_DAA   ( 3               ); break; /* DAA           */
			case 0xe0: OP_CLRV  ( 2               ); break; /* CLRV          */
			case 0xe1: OP_TCALL ( 8, 14           ); break; /* TCALL 14      */
			case 0xe2: OP_SET   ( 4, BIT_7        ); break; /* SET 7         */
			case 0xe3: OP_BBS   ( 5, BIT_7        ); break; /* BBS 7         */
			case 0xe4: OP_MOVMR ( 3, DP, REG_A    ); break; /* MOV A, dp     */
			case 0xe5: OP_MOVMR ( 4, ABS, REG_A   ); break; /* MOV A, abs    */
			case 0xe6: OP_MOVMR ( 3, XI, REG_A    ); break; /* MOV A, xi     */
			case 0xe7: OP_MOVMR ( 6, DXI, REG_A   ); break; /* MOV A, dxi    */
			case 0xe8: OP_MOVMR ( 2, IMM, REG_A   ); break; /* CMP A, imm    */
			case 0xe9: OP_MOVMR ( 4, ABS, REG_X   ); break; /* MOV X, abs    */
			case 0xea: OP_NOT1  ( 5               ); break; /* NOT1          */
			case 0xeb: OP_MOVMR ( 3, DP, REG_Y    ); break; /* MOV Y, dp     */
			case 0xec: OP_MOVMR ( 4, ABS, REG_Y   ); break; /* MOV Y, abs    */
			case 0xed: OP_NOTC  ( 3               ); break; /* NOTC          */
			case 0xee: OP_PULL  ( 4, REG_Y        ); break; /* POP Y         */
			case 0xef: OP_SLEEP ( 3               ); break; /* SLEEP         */
			case 0xf0: OP_BCC   ( 2, COND_EQ()    ); break; /* BEQ           */
			case 0xf1: OP_TCALL ( 8, 15           ); break; /* TCALL1 5      */
			case 0xf2: OP_CLR   ( 4, BIT_7        ); break; /* CLR 7         */
			case 0xf3: OP_BBC   ( 5, BIT_7        ); break; /* BBC 7         */
			case 0xf4: OP_MOVMR ( 4, DPX, REG_A   ); break; /* MOV A, dpx    */
			case 0xf5: OP_MOVMR ( 5, ABX, REG_A   ); break; /* MOV A, abx    */
			case 0xf6: OP_MOVMR ( 5, ABY, REG_A   ); break; /* MOV A, aby    */
			case 0xf7: OP_MOVMR ( 6, DIY, REG_A   ); break; /* MOV A, diy    */
			case 0xf8: OP_MOVMR ( 3, DP, REG_X    ); break; /* MOV X, dp     */
			case 0xf9: OP_MOVMR ( 4, DPY, REG_X   ); break; /* MOV X, dpy    */
			case 0xfa: OP_MOVMM ( 5, DP, DP       ); break; /* MOV dp, dp    */
			case 0xfb: OP_MOVMR ( 4, DPX, REG_Y   ); break; /* MOV Y, DPX    */
			case 0xfc: OP_INCR  ( 2, REG_Y        ); break; /* INC Y         */
			case 0xfd: OP_MOVRR ( 2, REG_A, REG_Y ); break; /* MOV Y, A      */
			case 0xfe: OP_DBNZR ( 4               ); break; /* DBNZ Y        */
			case 0xff: OP_STOP  ( 3               ); break; /* STOP          */
		}
	}
	return clocks - CLOCKS;
}


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
