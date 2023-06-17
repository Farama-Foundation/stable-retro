/*** konami: Portable Konami cpu emulator ******************************************

	Copyright (C) The MAME Team 1999

	Based on M6809 cpu core	copyright (C) John Butler 1997

	References:

		6809 Simulator V09, By L.C. Benschop, Eidnhoven The Netherlands.

		m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from
			the 6809 Simulator V09)

		6809 Microcomputer Programming & Interfacing with Experiments"
			by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.

	System dependencies:	UINT16 must be 16 bit unsigned int
							UINT8 must be 8 bit unsigned int
							UINT32 must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

	History:
991022 HJB:
	Tried to improve speed: Using bit7 of cycles1 as flag for multi
	byte opcodes is gone, those opcodes now instead go through opcode2().
	Inlined fetch_effective_address() into that function as well.
	Got rid of the slow/fast flags for stack (S and U) memory accesses.
	Minor changes to use 32 bit values as arguments to memory functions
	and added defines for that purpose (e.g. X = 16bit XD = 32bit).

990720 EHC:
	Created this file

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "konami.h"

static UINT8 konami_reg_layout[] = {
	KONAMI_PC, KONAMI_S, KONAMI_CC, KONAMI_A, KONAMI_B, KONAMI_X, -1,
	KONAMI_Y, KONAMI_U, KONAMI_DP, KONAMI_NMI_STATE, KONAMI_IRQ_STATE, KONAMI_FIRQ_STATE, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 konami_win_layout[] = {
	27, 0,53, 4,	/* register window (top, right rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 5,53, 8,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/* Konami Registers */
typedef struct
{
	PAIR	pc; 		/* Program counter */
    PAIR    ppc;        /* Previous program counter */
    PAIR    d;          /* Accumulator a and b */
    PAIR    dp;         /* Direct Page register (page in MSB) */
	PAIR	u, s;		/* Stack pointers */
	PAIR	x, y;		/* Index registers */
    UINT8   cc;
	UINT8	ireg;		/* first opcode */
    UINT8   irq_state[2];
    int     extra_cycles; /* cycles used up by interrupts */
    int     (*irq_callback)(int irqline);
    UINT8   int_state;  /* SYNC and CWAI flags */
	UINT8	nmi_state;
} konami_Regs;

/* flag bits in the cc register */
#define CC_C    0x01        /* Carry */
#define CC_V    0x02        /* Overflow */
#define CC_Z    0x04        /* Zero */
#define CC_N    0x08        /* Negative */
#define CC_II   0x10        /* Inhibit IRQ */
#define CC_H    0x20        /* Half (auxiliary) carry */
#define CC_IF   0x40        /* Inhibit FIRQ */
#define CC_E    0x80        /* entire state pushed */

/* Konami registers */
static konami_Regs konami;

#define	pPPC    konami.ppc
#define pPC 	konami.pc
#define pU		konami.u
#define pS		konami.s
#define pX		konami.x
#define pY		konami.y
#define pD		konami.d

#define	PPC		konami.ppc.w.l
#define PC  	konami.pc.w.l
#define PCD 	konami.pc.d
#define U		konami.u.w.l
#define UD		konami.u.d
#define S		konami.s.w.l
#define SD		konami.s.d
#define X		konami.x.w.l
#define XD		konami.x.d
#define Y		konami.y.w.l
#define YD		konami.y.d
#define D   	konami.d.w.l
#define A   	konami.d.b.h
#define B		konami.d.b.l
#define DP		konami.dp.b.h
#define DPD 	konami.dp.d
#define CC  	konami.cc

static PAIR ea;         /* effective address */
#define EA	ea.w.l
#define EAD ea.d

#define KONAMI_CWAI		8	/* set when CWAI is waiting for an interrupt */
#define KONAMI_SYNC		16	/* set when SYNC is waiting for an interrupt */
#define KONAMI_LDS		32	/* set when LDS occured at least once */

#define CHECK_IRQ_LINES 												\
	if( konami.irq_state[KONAMI_IRQ_LINE] != CLEAR_LINE ||				\
		konami.irq_state[KONAMI_FIRQ_LINE] != CLEAR_LINE )				\
		konami.int_state &= ~KONAMI_SYNC; /* clear SYNC flag */			\
	if( konami.irq_state[KONAMI_FIRQ_LINE]!=CLEAR_LINE && !(CC & CC_IF) ) \
	{																	\
		/* fast IRQ */													\
		/* state already saved by CWAI? */								\
		if( konami.int_state & KONAMI_CWAI )							\
		{																\
			konami.int_state &= ~KONAMI_CWAI;  /* clear CWAI */			\
			konami.extra_cycles += 7;		 /* subtract +7 cycles */	\
        }                                                               \
		else															\
		{																\
			CC &= ~CC_E;				/* save 'short' state */        \
			PUSHWORD(pPC);												\
			PUSHBYTE(CC);												\
			konami.extra_cycles += 10;	/* subtract +10 cycles */		\
		}																\
		CC |= CC_IF | CC_II;			/* inhibit FIRQ and IRQ */		\
		PCD = RM16(0xfff6); 											\
		change_pc16(PC);					/* TS 971002 */ 				\
		(void)(*konami.irq_callback)(KONAMI_FIRQ_LINE);					\
	}																	\
	else																\
	if( konami.irq_state[KONAMI_IRQ_LINE]!=CLEAR_LINE && !(CC & CC_II) )\
	{																	\
		/* standard IRQ */												\
		/* state already saved by CWAI? */								\
		if( konami.int_state & KONAMI_CWAI )							\
		{																\
			konami.int_state &= ~KONAMI_CWAI;  /* clear CWAI flag */	\
			konami.extra_cycles += 7;		 /* subtract +7 cycles */	\
		}																\
		else															\
		{																\
			CC |= CC_E; 				/* save entire state */ 		\
			PUSHWORD(pPC);												\
			PUSHWORD(pU);												\
			PUSHWORD(pY);												\
			PUSHWORD(pX);												\
			PUSHBYTE(DP);												\
			PUSHBYTE(B);												\
			PUSHBYTE(A);												\
			PUSHBYTE(CC);												\
			konami.extra_cycles += 19;	 /* subtract +19 cycles */		\
		}																\
		CC |= CC_II;					/* inhibit IRQ */				\
		PCD = RM16(0xfff8); 											\
		change_pc16(PC);					/* TS 971002 */ 				\
		(void)(*konami.irq_callback)(KONAMI_IRQ_LINE);					\
	}

/* public globals */
int konami_ICount=50000;
int konami_Flags;	/* flags for speed optimization (obsolete!!) */
void (*konami_cpu_setlines_callback)( int lines ) = 0; /* callback called when A16-A23 are set */

/* these are re-defined in konami.h TO RAM, ROM or functions in memory.c */
#define RM(Addr)			KONAMI_RDMEM(Addr)
#define WM(Addr,Value)		KONAMI_WRMEM(Addr,Value)
#define ROP(Addr)			KONAMI_RDOP(Addr)
#define ROP_ARG(Addr)		KONAMI_RDOP_ARG(Addr)

#define SIGNED(a)	(UINT16)(INT16)(INT8)(a)

/* macros to access memory */
#define IMMBYTE(b)	{ b = ROP_ARG(PCD); PC++; }
#define IMMWORD(w)	{ w.d = (ROP_ARG(PCD)<<8) | ROP_ARG(PCD+1); PC += 2; }

#define PUSHBYTE(b) --S; WM(SD,b)
#define PUSHWORD(w) --S; WM(SD,w.b.l); --S; WM(SD,w.b.h)
#define PULLBYTE(b) b=KONAMI_RDMEM(SD); S++
#define PULLWORD(w) w=KONAMI_RDMEM(SD)<<8; S++; w|=KONAMI_RDMEM(SD); S++

#define PSHUBYTE(b) --U; WM(UD,b);
#define PSHUWORD(w) --U; WM(UD,w.b.l); --U; WM(UD,w.b.h)
#define PULUBYTE(b) b=KONAMI_RDMEM(UD); U++
#define PULUWORD(w) w=KONAMI_RDMEM(UD)<<8; U++; w|=KONAMI_RDMEM(UD); U++

#define CLR_HNZVC	CC&=~(CC_H|CC_N|CC_Z|CC_V|CC_C)
#define CLR_NZV 	CC&=~(CC_N|CC_Z|CC_V)
#define CLR_NZ		CC&=~(CC_N|CC_Z)
#define CLR_HNZC	CC&=~(CC_H|CC_N|CC_Z|CC_C)
#define CLR_NZVC	CC&=~(CC_N|CC_Z|CC_V|CC_C)
#define CLR_Z		CC&=~(CC_Z)
#define CLR_NZC 	CC&=~(CC_N|CC_Z|CC_C)
#define CLR_ZC		CC&=~(CC_Z|CC_C)

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)		if(!a)SEZ
#define SET_Z8(a)		SET_Z((UINT8)a)
#define SET_Z16(a)		SET_Z((UINT16)a)
#define SET_N8(a)		CC|=((a&0x80)>>4)
#define SET_N16(a)		CC|=((a&0x8000)>>12)
#define SET_H(a,b,r)	CC|=(((a^b^r)&0x10)<<1)
#define SET_C8(a)		CC|=((a&0x100)>>8)
#define SET_C16(a)		CC|=((a&0x10000)>>16)
#define SET_V8(a,b,r)	CC|=(((a^b^r^(r>>1))&0x80)>>6)
#define SET_V16(a,b,r)	CC|=(((a^b^r^(r>>1))&0x8000)>>14)

static UINT8 flags8i[256]=	 /* increment */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
CC_N|CC_V,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
static UINT8 flags8d[256]= /* decrement */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,CC_V,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
#define SET_FLAGS8I(a)		{CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* macros for addressing modes (postbytes have their own code) */
#define DIRECT	EAD = DPD; IMMBYTE(ea.b.l)
#define IMM8	EAD = PCD; PC++
#define IMM16	EAD = PCD; PC+=2
#define EXTENDED IMMWORD(ea)

/* macros to set status flags */
#define SEC CC|=CC_C
#define CLC CC&=~CC_C
#define SEZ CC|=CC_Z
#define CLZ CC&=~CC_Z
#define SEN CC|=CC_N
#define CLN CC&=~CC_N
#define SEV CC|=CC_V
#define CLV CC&=~CC_V
#define SEH CC|=CC_H
#define CLH CC&=~CC_H

/* macros for convenience */
#define DIRBYTE(b) DIRECT; b=RM(EAD)
#define DIRWORD(w) DIRECT; w.d=RM16(EAD)
#define EXTBYTE(b) EXTENDED; b=RM(EAD)
#define EXTWORD(w) EXTENDED; w.d=RM16(EAD)

/* macros for branch instructions */
#define BRANCH(f) { 					\
	UINT8 t;							\
	IMMBYTE(t); 						\
	if( f ) 							\
	{									\
		PC += SIGNED(t);				\
		change_pc16(PC);	/* TS 971002 */ \
	}									\
}

#define LBRANCH(f) {                    \
	PAIR t; 							\
	IMMWORD(t); 						\
	if( f ) 							\
	{									\
		konami_ICount -= 1;				\
		PC += t.w.l;					\
		change_pc16(PC);	/* TS 971002 */ \
	}									\
}

#define NXORV  ((CC&CC_N)^((CC&CC_V)<<2))

/* macros for setting/getting registers in TFR/EXG instructions */
#define GETREG(val,reg) 				\
	switch(reg) {						\
	case 0: val = A;	break;			\
	case 1: val = B; 	break; 			\
	case 2: val = X; 	break;			\
	case 3: val = Y;	break; 			\
	case 4: val = S; 	break; /* ? */	\
	case 5: val = U;	break;			\
	default: val = 0xff; log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown TFR/EXG idx at PC:%04x\n", PC ); break; \
}

#define SETREG(val,reg) 				\
	switch(reg) {						\
	case 0: A = val;	break;			\
	case 1: B = val;	break;			\
	case 2: X = val; 	break;			\
	case 3: Y = val;	break;			\
	case 4: S = val;	break; /* ? */	\
	case 5: U = val; 	break;			\
	default: log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown TFR/EXG idx at PC:%04x\n", PC ); break; \
}

/* opcode timings */
static UINT8 cycles1[] =
{
	/*	 0	1  2  3  4	5  6  7  8	9  A  B  C	D  E  F */
  /*0*/  1, 1, 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 5, 5, 5, 5,
  /*1*/  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  /*2*/  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  /*3*/  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 7, 6,
  /*4*/  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 4, 4,
  /*5*/  4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 1, 1, 1,
  /*6*/  3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5,
  /*7*/  3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5,
  /*8*/  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5,
  /*9*/  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 6,
  /*A*/  2, 2, 2, 4, 4, 4, 4, 4, 2, 2, 2, 2, 3, 3, 2, 1,
  /*B*/  3, 2, 2,11,22,11, 2, 4, 3, 3, 3, 3, 3, 3, 3, 3,
  /*C*/  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2,
  /*D*/  2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /*E*/  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /*F*/  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static INLINE UINT32 RM16( UINT32 Addr )
{
	UINT32 result = RM(Addr) << 8;
	return result | RM((Addr+1)&0xffff);
}

static INLINE void WM16( UINT32 Addr, PAIR *p )
{
	WM( Addr, p->b.h );
	WM( (Addr+1)&0xffff, p->b.l );
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned konami_get_context(void *dst)
{
	if( dst )
		*(konami_Regs*)dst = konami;
	return sizeof(konami_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void konami_set_context(void *src)
{
	if( src )
		konami = *(konami_Regs*)src;
    change_pc16(PC);    /* TS 971002 */

    CHECK_IRQ_LINES;
}

/****************************************************************************/
/* Return a specific register                                               */
/****************************************************************************/
unsigned konami_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:
		case KONAMI_PC: return PC;
		case REG_SP:
		case KONAMI_S: return S;
		case KONAMI_CC: return CC;
		case KONAMI_U: return U;
		case KONAMI_A: return A;
		case KONAMI_B: return B;
		case KONAMI_X: return X;
		case KONAMI_Y: return Y;
		case KONAMI_DP: return DP;
		case KONAMI_NMI_STATE: return konami.nmi_state;
		case KONAMI_IRQ_STATE: return konami.irq_state[KONAMI_IRQ_LINE];
		case KONAMI_FIRQ_STATE: return konami.irq_state[KONAMI_FIRQ_LINE];
		case REG_PREVIOUSPC: return PPC;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return ( RM( offset ) << 8 ) | RM( offset + 1 );
			}
	}
	return 0;
}


/****************************************************************************/
/* Set a specific register                                                  */
/****************************************************************************/
void konami_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:
		case KONAMI_PC: PC = val; change_pc16(PC); break;
		case REG_SP:
		case KONAMI_S: S = val; break;
		case KONAMI_CC: CC = val; CHECK_IRQ_LINES; break;
		case KONAMI_U: U = val; break;
		case KONAMI_A: A = val; break;
		case KONAMI_B: B = val; break;
		case KONAMI_X: X = val; break;
		case KONAMI_Y: Y = val; break;
		case KONAMI_DP: DP = val; break;
		case KONAMI_NMI_STATE: konami.nmi_state = val; break;
		case KONAMI_IRQ_STATE: konami.irq_state[KONAMI_IRQ_LINE] = val; break;
		case KONAMI_FIRQ_STATE: konami.irq_state[KONAMI_FIRQ_LINE] = val; break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, (val >> 8) & 0xff );
					WM( offset+1, val & 0xff );
				}
			}
    }
}


/****************************************************************************/
/* Reset registers to their initial values									*/
/****************************************************************************/
void konami_init(void)
{
}

void konami_reset(void *param)
{
	konami.int_state = 0;
	konami.nmi_state = CLEAR_LINE;
	konami.irq_state[0] = CLEAR_LINE;
	konami.irq_state[0] = CLEAR_LINE;

	DPD = 0;			/* Reset direct page register */

    CC |= CC_II;        /* IRQ disabled */
    CC |= CC_IF;        /* FIRQ disabled */

	PCD = RM16(0xfffe);
    change_pc16(PC);    /* TS 971002 */
}

void konami_exit(void)
{
	/* just make sure we deinit this, so the next game set its own */
	konami_cpu_setlines_callback = 0;
}

/* Generate interrupts */
/****************************************************************************
 * Set IRQ line state
 ****************************************************************************/
void konami_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if (konami.nmi_state == state) return;
		konami.nmi_state = state;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "KONAMI#%d set_nmi_line %d\n", cpu_getactivecpu(), state);
		if( state == CLEAR_LINE ) return;

		/* if the stack was not yet initialized */
	    if( !(konami.int_state & KONAMI_LDS) ) return;

	    konami.int_state &= ~KONAMI_SYNC;
		/* state already saved by CWAI? */
		if( konami.int_state & KONAMI_CWAI )
		{
			konami.int_state &= ~KONAMI_CWAI;
			konami.extra_cycles += 7;	/* subtract +7 cycles next time */
	    }
		else
		{
			CC |= CC_E; 				/* save entire state */
			PUSHWORD(pPC);
			PUSHWORD(pU);
			PUSHWORD(pY);
			PUSHWORD(pX);
			PUSHBYTE(DP);
			PUSHBYTE(B);
			PUSHBYTE(A);
			PUSHBYTE(CC);
			konami.extra_cycles += 19;	/* subtract +19 cycles next time */
		}
		CC |= CC_IF | CC_II;			/* inhibit FIRQ and IRQ */
		PCD = RM16(0xfffc);
		change_pc16(PC);					/* TS 971002 */
	}
	else if (irqline < 2)
	{
    log_cb(RETRO_LOG_DEBUG, LOGPRE "KONAMI#%d set_irq_line %d, %d\n", cpu_getactivecpu(), irqline, state);
		konami.irq_state[irqline] = state;
		if (state == CLEAR_LINE) return;
		CHECK_IRQ_LINES;
	}
}

/****************************************************************************
 * Set IRQ vector callback
 ****************************************************************************/
void konami_set_irq_callback(int (*callback)(int irqline))
{
	konami.irq_callback = callback;
}
#if 0
/****************************************************************************
 * Save CPU state
 ****************************************************************************/
static void state_save(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file, module, cpu, "PC", &PC, 1);
	state_save_UINT16(file, module, cpu, "U", &U, 1);
	state_save_UINT16(file, module, cpu, "S", &S, 1);
	state_save_UINT16(file, module, cpu, "X", &X, 1);
	state_save_UINT16(file, module, cpu, "Y", &Y, 1);
	state_save_UINT8(file, module, cpu, "DP", &DP, 1);
	state_save_UINT8(file, module, cpu, "CC", &CC, 1);
	state_save_UINT8(file, module, cpu, "INT", &konami.int_state, 1);
	state_save_UINT8(file, module, cpu, "NMI", &konami.nmi_state, 1);
	state_save_UINT8(file, module, cpu, "IRQ", &konami.irq_state[0], 1);
	state_save_UINT8(file, module, cpu, "FIRQ", &konami.irq_state[1], 1);
}

/****************************************************************************
 * Load CPU state
 ****************************************************************************/
static void state_load(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file, module, cpu, "PC", &PC, 1);
	state_load_UINT16(file, module, cpu, "U", &U, 1);
	state_load_UINT16(file, module, cpu, "S", &S, 1);
	state_load_UINT16(file, module, cpu, "X", &X, 1);
	state_load_UINT16(file, module, cpu, "Y", &Y, 1);
	state_load_UINT8(file, module, cpu, "DP", &DP, 1);
	state_load_UINT8(file, module, cpu, "CC", &CC, 1);
	state_load_UINT8(file, module, cpu, "INT", &konami.int_state, 1);
	state_load_UINT8(file, module, cpu, "NMI", &konami.nmi_state, 1);
	state_load_UINT8(file, module, cpu, "IRQ", &konami.irq_state[0], 1);
	state_load_UINT8(file, module, cpu, "FIRQ", &konami.irq_state[1], 1);
}

void konami_state_save(void *file) { state_save(file, "konami"); }
void konami_state_load(void *file) { state_load(file, "konami"); }
#endif
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *konami_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	konami_Regs *r = context;

	which = (which+1) % 16;
    buffer[which][0] = '\0';
	if( !context )
		r = &konami;

	switch( regnum )
	{
		case CPU_INFO_NAME: return "KONAMI";
		case CPU_INFO_FAMILY: return "KONAMI 5000x";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) The MAME Team 1999";
		case CPU_INFO_REG_LAYOUT: return (const char*)konami_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)konami_win_layout;

		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->cc & 0x80 ? 'E':'.',
				r->cc & 0x40 ? 'F':'.',
                r->cc & 0x20 ? 'H':'.',
                r->cc & 0x10 ? 'I':'.',
                r->cc & 0x08 ? 'N':'.',
                r->cc & 0x04 ? 'Z':'.',
                r->cc & 0x02 ? 'V':'.',
                r->cc & 0x01 ? 'C':'.');
            break;
		case CPU_INFO_REG+KONAMI_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+KONAMI_S: sprintf(buffer[which], "S:%04X", r->s.w.l); break;
		case CPU_INFO_REG+KONAMI_CC: sprintf(buffer[which], "CC:%02X", r->cc); break;
		case CPU_INFO_REG+KONAMI_U: sprintf(buffer[which], "U:%04X", r->u.w.l); break;
		case CPU_INFO_REG+KONAMI_A: sprintf(buffer[which], "A:%02X", r->d.b.h); break;
		case CPU_INFO_REG+KONAMI_B: sprintf(buffer[which], "B:%02X", r->d.b.l); break;
		case CPU_INFO_REG+KONAMI_X: sprintf(buffer[which], "X:%04X", r->x.w.l); break;
		case CPU_INFO_REG+KONAMI_Y: sprintf(buffer[which], "Y:%04X", r->y.w.l); break;
		case CPU_INFO_REG+KONAMI_DP: sprintf(buffer[which], "DP:%02X", r->dp.b.h); break;
		case CPU_INFO_REG+KONAMI_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+KONAMI_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state[KONAMI_IRQ_LINE]); break;
		case CPU_INFO_REG+KONAMI_FIRQ_STATE: sprintf(buffer[which], "FIRQ:%X", r->irq_state[KONAMI_FIRQ_LINE]); break;
	}
	return buffer[which];
}

unsigned konami_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return Dasmknmi(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

/* includes the static function prototypes and the master opcode table */
#include "konamtbl.c"

/* includes the actual opcode implementations */
#include "konamops.c"

/* execute instructions on this CPU until icount expires */
int konami_execute(int cycles)
{
	konami_ICount = cycles - konami.extra_cycles;
	konami.extra_cycles = 0;

	if( konami.int_state & (KONAMI_CWAI | KONAMI_SYNC) )
	{
		konami_ICount = 0;
	}
	else
	{
		do
		{
			pPPC = pPC;

			CALL_MAME_DEBUG;

			konami.ireg = ROP(PCD);
			PC++;

            (*konami_main[konami.ireg])();

            konami_ICount -= cycles1[konami.ireg];

        } while( konami_ICount > 0 );

        konami_ICount -= konami.extra_cycles;
		konami.extra_cycles = 0;
    }

	return cycles - konami_ICount;
}
