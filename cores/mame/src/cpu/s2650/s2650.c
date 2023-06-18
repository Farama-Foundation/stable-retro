/*************************************************************************
 *
 *      Portable Signetics 2650 cpu emulation
 *
 *		Written by Juergen Buchmueller for use with MAME
 *
 *************************************************************************/

#include <stdio.h>
#include <string.h>

#include <retro_inline.h>

#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "s2650.h"
#include "s2650cpu.h"


/* define this to expand all EA calculations inline */
#define INLINE_EA	1

static UINT8 s2650_reg_layout[] = {
	S2650_PC, S2650_PS, S2650_R0, S2650_R1, S2650_R2, S2650_R3, -1,
	S2650_SI, S2650_FO, S2650_R1A, S2650_R2A, S2650_R3A, -1,
	S2650_HALT, S2650_IRQ_STATE, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 s2650_win_layout[] = {
	32, 0,48, 4,	/* register window (top rows) */
	 0, 0,31,22,	/* disassembler window (left colums) */
	32, 5,48, 8,	/* memory #1 window (right, upper middle) */
	32,14,48, 8,	/* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};

int s2650_ICount = 0;

typedef struct {
	UINT16	ppc;	/* previous program counter (page + iar) */
    UINT16  page;   /* 8K page select register (A14..A13) */
    UINT16  iar;    /* instruction address register (A12..A0) */
    UINT16  ea;     /* effective address (A14..A0) */
    UINT8   psl;    /* processor status lower */
    UINT8   psu;    /* processor status upper */
    UINT8   r;      /* absolute addressing dst/src register */
    UINT8   reg[7]; /* 7 general purpose registers */
    UINT8   halt;   /* 1 if cpu is halted */
    UINT8   ir;     /* instruction register */
    UINT16  ras[8]; /* 8 return address stack entries */
	UINT8	irq_state;
    int     (*irq_callback)(int irqline);
}   s2650_Regs;

static s2650_Regs S;

/* condition code changes for a byte */
static	UINT8 ccc[0x200] = {
	0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x04,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
	0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
};

#define CHECK_IRQ_LINE											\
	if (S.irq_state != CLEAR_LINE)								\
	{															\
		if( (S.psu & II) == 0 ) 								\
		{														\
			int vector; 										\
			if (S.halt) 										\
			{													\
				S.halt = 0; 									\
				S.iar = (S.iar + 1) & PMSK; 					\
			}													\
			vector = (*S.irq_callback)(0) & 0xff;				\
			/* build effective address within first 8K page */	\
			S.ea = S2650_relative[vector] & PMSK;				\
			if (vector & 0x80)		/* indirect bit set ? */	\
			{													\
				int addr = S.ea;								\
				s2650_ICount -= 2;								\
				/* build indirect 32K address */				\
				S.ea = RDMEM(addr) << 8;						\
				if (!(++addr & PMSK)) addr -= PLEN; 			\
				S.ea = (S.ea + RDMEM(addr)) & AMSK; 			\
			}													\
			log_cb(RETRO_LOG_DEBUG, LOGPRE "S2650 interrupt to $%04x\n", S.ea);\
			S.psu  = (S.psu & ~SP) | ((S.psu + 1) & SP) | II;	\
			S.ras[S.psu & SP] = S.page + S.iar;					\
			S.page = S.ea & PAGE;								\
			S.iar  = S.ea & PMSK;								\
		}														\
	}

/***************************************************************
 *
 * set condition code (zero,plus,minus) from result
 ***************************************************************/
#define SET_CC(result)                                          \
	S.psl = (S.psl & ~CC) | ccc[result]

/***************************************************************
 *
 * set condition code (zero,plus,minus) and overflow
 ***************************************************************/
#define SET_CC_OVF(result,value)                                \
	S.psl = (S.psl & ~(OVF+CC)) |								\
		ccc[result + ( ( (result^value) << 1) & 256 )]

#if 1
#define SET_CC_OVF_ADD(result,value1,value2) SET_CC_OVF(result,value1)
#define SET_CC_OVF_SUB(result,value1,value2) SET_CC_OVF(result,value1)
#else
#define SET_CC_OVF_ADD(result,value1,value2)                    \
	S.psl = (S.psl & ~(OVF+CC)) |								\
		ccc[result + ( ( (~(value1^value2) & (result^value1)) << 1) & 256 )]

#define SET_CC_OVF_SUB(result,value1,value2)                    \
	S.psl = (S.psl & ~(OVF+CC)) |								\
		ccc[result + ( ( ((value1^value2) & (result^value1)) << 1) & 256 )]
#endif

/***************************************************************
 * ROP
 * read next opcode
 ***************************************************************/
static INLINE UINT8 ROP(void)
{
	UINT8 result = cpu_readop(S.page + S.iar);
	S.iar = (S.iar + 1) & PMSK;
	return result;
}

/***************************************************************
 * ARG
 * read next opcode argument
 ***************************************************************/
static INLINE UINT8 ARG(void)
{
	UINT8 result = cpu_readop_arg(S.page + S.iar);
	S.iar = (S.iar + 1) & PMSK;
	return result;
}

/***************************************************************
 * RDMEM
 * read memory byte from addr
 ***************************************************************/
#define RDMEM(addr) cpu_readmem16(addr)

/***************************************************************
 * handy table to build PC relative offsets
 * from HR (holding register)
 ***************************************************************/
static	int 	S2650_relative[0x100] =
{
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
	-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
	-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
	-16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
	-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
	-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
	-16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
};

/***************************************************************
 * _REL_EA
 * build effective address with relative addressing
 ***************************************************************/
#define _REL_EA(page)											\
{																\
	UINT8 hr = ARG();	/* get 'holding register' */            \
	/* build effective address within current 8K page */		\
	S.ea = page + ((S.iar + S2650_relative[hr]) & PMSK);		\
	if (hr & 0x80) { /* indirect bit set ? */					\
		int addr = S.ea;										\
		s2650_ICount -= 2;										\
		/* build indirect 32K address */						\
		S.ea = RDMEM(addr) << 8;								\
		if( (++addr & PMSK) == 0 ) addr -= PLEN; /* page wrap */\
		S.ea = (S.ea + RDMEM(addr)) & AMSK; 					\
	}															\
}

/***************************************************************
 * _REL_ZERO
 * build effective address with zero relative addressing
 ***************************************************************/
#define _REL_ZERO(page)											\
{																\
	UINT8 hr = ARG();	/* get 'holding register' */            \
	/* build effective address from 0 */						\
	S.ea = (S2650_relative[hr] & PMSK);							\
	if (hr & 0x80) { /* indirect bit set ? */					\
		int addr = S.ea;										\
		s2650_ICount -= 2;										\
		/* build indirect 32K address */						\
		S.ea = RDMEM(addr) << 8;								\
		if( (++addr & PMSK) == 0 ) addr -= PLEN; /* page wrap */\
		S.ea = (S.ea + RDMEM(addr)) & AMSK; 					\
	}															\
}

/***************************************************************
 * _ABS_EA
 * build effective address with absolute addressing
 ***************************************************************/
#define _ABS_EA()												\
{																\
	UINT8 hr, dr;												\
	hr = ARG(); 	/* get 'holding register' */                \
	dr = ARG(); 	/* get 'data bus register' */               \
	/* build effective address within current 8K page */		\
	S.ea = S.page + (((hr << 8) + dr) & PMSK);					\
	/* indirect addressing ? */ 								\
	if (hr & 0x80) {											\
		int addr = S.ea;										\
		s2650_ICount -= 2;										\
		/* build indirect 32K address */						\
		/* build indirect 32K address */						\
		S.ea = RDMEM(addr) << 8;								\
		if( (++addr & PMSK) == 0 ) addr -= PLEN; /* page wrap */\
        S.ea = (S.ea + RDMEM(addr)) & AMSK;                     \
	}															\
	/* check indexed addressing modes */						\
	switch (hr & 0x60) {										\
		case 0x00: /* not indexed */							\
			break;												\
		case 0x20: /* auto increment indexed */ 				\
			S.reg[S.r] += 1;									\
			S.ea = (S.ea & PAGE)+((S.ea+S.reg[S.r]) & PMSK);	\
			S.r = 0; /* absolute addressing reg is R0 */		\
			break;												\
		case 0x40: /* auto decrement indexed */ 				\
			S.reg[S.r] -= 1;									\
			S.ea = (S.ea & PAGE)+((S.ea+S.reg[S.r]) & PMSK);	\
			S.r = 0; /* absolute addressing reg is R0 */		\
			break;												\
		case 0x60: /* indexed */								\
			S.ea = (S.ea & PAGE)+((S.ea+S.reg[S.r]) & PMSK);	\
			S.r = 0; /* absolute addressing reg is R0 */		\
			break;												\
	}															\
}

/***************************************************************
 * _BRA_EA
 * build effective address with absolute addressing (branch)
 ***************************************************************/
#define _BRA_EA()												\
{																\
	UINT8 hr, dr;												\
	hr = ARG(); 	/* get 'holding register' */                \
	dr = ARG(); 	/* get 'data bus register' */               \
	/* build address in 32K address space */					\
	S.ea = ((hr << 8) + dr) & AMSK; 							\
	/* indirect addressing ? */ 								\
	if (hr & 0x80) {											\
		int addr = S.ea;										\
		s2650_ICount -= 2;										\
		/* build indirect 32K address */						\
		S.ea = RDMEM(addr) << 8;								\
		if( (++addr & PMSK) == 0 ) addr -= PLEN; /* page wrap */\
        S.ea = (S.ea + RDMEM(addr)) & AMSK;                     \
	}															\
}

/***************************************************************
 * SWAP_REGS
 * Swap registers r1-r3 with r4-r6 (the second set)
 * This is done everytime the RS bit in PSL changes
 ***************************************************************/
#define SWAP_REGS												\
{																\
	UINT8 tmp;													\
	tmp = S.reg[1]; 											\
	S.reg[1] = S.reg[4];										\
	S.reg[4] = tmp; 											\
	tmp = S.reg[2]; 											\
	S.reg[2] = S.reg[5];										\
	S.reg[5] = tmp; 											\
	tmp = S.reg[3]; 											\
	S.reg[3] = S.reg[6];										\
	S.reg[6] = tmp; 											\
}

/***************************************************************
 * M_BRR
 * Branch relative if cond is true
 ***************************************************************/
#define M_BRR(cond) 											\
{																\
	if (cond)													\
	{															\
		REL_EA( S.page );										\
		S.page = S.ea & PAGE;									\
		S.iar  = S.ea & PMSK;									\
		change_pc16(S.ea);										\
	} else S.iar = (S.iar + 1) & PMSK;							\
}

/***************************************************************
 * M_ZBRR
 * Branch relative to page zero
 ***************************************************************/
#define M_ZBRR()												\
{																\
	REL_ZERO( 0 );												\
	S.page = S.ea & PAGE;										\
	S.iar  = S.ea & PMSK;										\
	change_pc16(S.ea);											\
}

/***************************************************************
 * M_BRA
 * Branch absolute if cond is true
 ***************************************************************/
#define M_BRA(cond) 											\
{																\
	if( cond )													\
	{															\
		BRA_EA();												\
		S.page = S.ea & PAGE;									\
		S.iar  = S.ea & PMSK;									\
		change_pc16(S.ea);										\
	} else S.iar = (S.iar + 2) & PMSK;							\
}

/***************************************************************
 * M_BXA
 * Branch indexed absolute (EA + R3)
 ***************************************************************/
#define M_BXA() 												\
{																\
	BRA_EA();													\
	S.ea   = (S.ea + S.reg[3]) & AMSK;							\
	S.page = S.ea & PAGE;										\
	S.iar  = S.ea & PMSK;										\
	change_pc16(S.ea);											\
}

/***************************************************************
 * M_BSR
 * Branch to subroutine relative if cond is true
 ***************************************************************/
#define M_BSR(cond) 											\
{																\
	if( cond )													\
	{															\
		REL_EA(S.page); 										\
		S.psu  = (S.psu & ~SP) | ((S.psu + 1) & SP);			\
		S.ras[S.psu & SP] = S.page + S.iar;						\
		S.page = S.ea & PAGE;									\
		S.iar  = S.ea & PMSK;									\
		change_pc16(S.ea);										\
	} else	S.iar = (S.iar + 1) & PMSK; 						\
}

/***************************************************************
 * M_ZBSR
 * Branch to subroutine relative to page zero
 ***************************************************************/
#define M_ZBSR()												\
{																\
	REL_ZERO(0); 											    \
	S.psu  = (S.psu & ~SP) | ((S.psu + 1) & SP);				\
	S.ras[S.psu & SP] = S.page + S.iar;							\
	S.page = S.ea & PAGE;										\
	S.iar  = S.ea & PMSK;										\
	change_pc16(S.ea);											\
}

/***************************************************************
 * M_BSA
 * Branch to subroutine absolute
 ***************************************************************/
#define M_BSA(cond) 											\
{																\
	if( cond )													\
	{															\
		BRA_EA();												\
		S.psu = (S.psu & ~SP) | ((S.psu + 1) & SP); 			\
		S.ras[S.psu & SP] = S.page + S.iar;						\
		S.page = S.ea & PAGE;									\
		S.iar  = S.ea & PMSK;									\
		change_pc16(S.ea);										\
	} else S.iar = (S.iar + 2) & PMSK;							\
}

/***************************************************************
 * M_BSXA
 * Branch to subroutine indexed absolute (EA + R3)
 ***************************************************************/
#define M_BSXA()												\
{																\
	BRA_EA();													\
	S.ea  = (S.ea + S.reg[3]) & AMSK;							\
	S.psu = (S.psu & ~SP) | ((S.psu + 1) & SP); 				\
	S.ras[S.psu & SP] = S.page + S.iar;							\
	S.page = S.ea & PAGE;										\
	S.iar  = S.ea & PMSK;										\
	change_pc16(S.ea);											\
}

/***************************************************************
 * M_RET
 * Return from subroutine if cond is true
 ***************************************************************/
#define M_RET(cond) 											\
{																\
	if( cond )													\
	{															\
		S.ea = S.ras[S.psu & SP];								\
		S.psu = (S.psu & ~SP) | ((S.psu - 1) & SP); 			\
		S.page = S.ea & PAGE;									\
		S.iar  = S.ea & PMSK;									\
		change_pc16(S.ea);										\
	}															\
}

/***************************************************************
 * M_RETE
 * Return from subroutine if cond is true
 * and enable interrupts; afterwards check IRQ line
 * state and eventually take next interrupt
 ***************************************************************/
#define M_RETE(cond)											\
{																\
	if( cond )													\
	{															\
		S.ea = S.ras[S.psu & SP];								\
		S.psu = (S.psu & ~SP) | ((S.psu - 1) & SP); 			\
		S.page = S.ea & PAGE;									\
		S.iar  = S.ea & PMSK;									\
		change_pc16(S.ea);										\
		S.psu &= ~II;											\
		CHECK_IRQ_LINE; 										\
	}															\
}

/***************************************************************
 * M_LOD
 * Load destination with source register
 ***************************************************************/
#define M_LOD(dest,source)										\
{																\
	dest = source;												\
	SET_CC(dest);												\
}

/***************************************************************
 * M_STR
 * Store source register to memory addr (CC unchanged)
 ***************************************************************/
#define M_STR(address,source)									\
	cpu_writemem16(address, source)

/***************************************************************
 * M_AND
 * Logical and destination with source
 ***************************************************************/
#define M_AND(dest,source)										\
{																\
	dest &= source; 											\
	SET_CC(dest);												\
}

/***************************************************************
 * M_IOR
 * Logical inclusive or destination with source
 ***************************************************************/
#define M_IOR(dest,source)										\
{																\
	dest |= source; 											\
	SET_CC(dest);												\
}

/***************************************************************
 * M_EOR
 * Logical exclusive or destination with source
 ***************************************************************/
#define M_EOR(dest,source)										\
{																\
	dest ^= source; 											\
	SET_CC(dest);												\
}

/***************************************************************
 * M_ADD
 * Add source to destination
 * Add with carry if WC flag of PSL is set
 ***************************************************************/
#define M_ADD(dest,source)										\
{																\
	UINT8 before = dest;										\
	/* add source; carry only if WC is set */					\
	UINT16 res = dest + source + ((S.psl >> 3) & S.psl & C);	\
	S.psl &= ~(C | OVF | IDC);									\
	if(res & 0x100) S.psl |= C; 							    \
    dest = res & 0xff;                                          \
	if( (dest & 15) < (before & 15) ) S.psl |= IDC; 			\
	SET_CC_OVF_ADD(dest,before,source);							\
}

/***************************************************************
 * M_SUB
 * Subtract source from destination
 * Subtract with borrow if WC flag of PSL is set
 ***************************************************************/
#define M_SUB(dest,source)										\
{																\
	UINT8 before = dest;										\
	/* subtract source; borrow only if WC is set */ 			\
	UINT16 res = dest - source - ((S.psl >> 3) & (S.psl ^ C) & C);	\
	S.psl &= ~(C | OVF | IDC);									\
	if((res & 0x100)==0) S.psl |= C; 							\
    dest = res & 0xff;                                          \
	if( (dest & 15) <= (before & 15) ) S.psl |= IDC; 			\
	SET_CC_OVF_SUB(dest,before,source);							\
}

/***************************************************************
 * M_COM
 * Compare register against value. If COM of PSL is set,
 * use unsigned, else signed comparison
 ***************************************************************/
#define M_COM(reg,val)											\
{																\
	int d;														\
	S.psl &= ~CC;												\
	if (S.psl & COM) d = (UINT8)reg - (UINT8)val;				\
				else d = (INT8)reg - (INT8)val; 				\
	if( d < 0 ) S.psl |= 0x80;									\
	else														\
	if( d > 0 ) S.psl |= 0x40;									\
}

/***************************************************************
 * M_DAR
 * Decimal adjust register
 ***************************************************************/
#define M_DAR(dest)												\
{																\
	if ((S.psl & C) == 0) dest += 0xA0;							\
	if ((S.psl & IDC) == 0) dest = (dest & 0xF0) | ((dest + 0x0A) & 0x0F);\
}

/***************************************************************
 * M_RRL
 * Rotate register left; If WC of PSL is set, rotate
 * through carry, else rotate circular
 ***************************************************************/
#define M_RRL(dest) 											\
{																\
	UINT8 before = dest;										\
	if( S.psl & WC )											\
	{															\
		UINT8 c = S.psl & C;									\
		S.psl &= ~(C + IDC);									\
		dest = (before << 1) | c;								\
		S.psl |= (before >> 7) + (dest & IDC);					\
	}															\
	else														\
	{															\
		dest = (before << 1) | (before >> 7);					\
	}															\
	SET_CC_OVF(dest,before);									\
}

/***************************************************************
 * M_RRR
 * Rotate register right; If WC of PSL is set, rotate
 * through carry, else rotate circular
 ***************************************************************/
#define M_RRR(dest) 											\
{																\
	UINT8 before = dest;										\
	if (S.psl & WC) 											\
	{															\
		UINT8 c = S.psl & C;									\
		S.psl &= ~(C + IDC);									\
		dest = (before >> 1) | (c << 7);						\
		S.psl |= (before & C) + (dest & IDC);					\
	} else	dest = (before >> 1) | (before << 7);				\
	SET_CC_OVF(dest,before);									\
}

/***************************************************************
 * M_SPSU
 * Store processor status upper (PSU) to register R0
 * Checks for External Sense IO port
 ***************************************************************/
#define M_SPSU()												\
{																\
	R0 = ((S.psu & ~PSU34) | (cpu_readport16(S2650_SENSE_PORT) & SI)); \
	SET_CC(R0); 												\
}

/***************************************************************
 * M_SPSL
 * Store processor status lower (PSL) to register R0
 ***************************************************************/
#define M_SPSL()												\
{																\
	R0 = S.psl; 												\
	SET_CC(R0); 												\
}

/***************************************************************
 * M_CPSU
 * Clear processor status upper (PSU), selective
 ***************************************************************/
#define M_CPSU()												\
{																\
	UINT8 cpsu = ARG(); 										\
	S.psu = S.psu & ~cpsu;										\
	CHECK_IRQ_LINE; 											\
}

/***************************************************************
 * M_CPSL
 * Clear processor status lower (PSL), selective
 ***************************************************************/
#define M_CPSL()												\
{																\
	UINT8 cpsl = ARG(); 										\
	/* select other register set now ? */						\
	if( (cpsl & RS) && (S.psl & RS) )							\
		SWAP_REGS;												\
	S.psl = S.psl & ~cpsl;										\
	CHECK_IRQ_LINE; 											\
}

/***************************************************************
 * M_PPSU
 * Preset processor status upper (PSU), selective
 * Unused bits 3 and 4 can't be set
 ***************************************************************/
#define M_PPSU()												\
{																\
	UINT8 ppsu = (ARG() & ~PSU34) & ~SI;						\
	S.psu = S.psu | ppsu;										\
}

/***************************************************************
 * M_PPSL
 * Preset processor status lower (PSL), selective
 ***************************************************************/
#define M_PPSL()												\
{																\
	UINT8 ppsl = ARG(); 										\
	/* select 2nd register set now ? */ 						\
	if ((ppsl & RS) && !(S.psl & RS))							\
		SWAP_REGS;												\
	S.psl = S.psl | ppsl;										\
}

/***************************************************************
 * M_TPSU
 * Test processor status upper (PSU)
 ***************************************************************/
#define M_TPSU()												\
{																\
	UINT8 tpsu = ARG(); 										\
    UINT8 rpsu = (S.psu | (cpu_readport16(S2650_SENSE_PORT) & SI)); \
	S.psl &= ~CC;												\
	if( (rpsu & tpsu) != tpsu )									\
		S.psl |= 0x80;											\
}

/***************************************************************
 * M_TPSL
 * Test processor status lower (PSL)
 ***************************************************************/
#define M_TPSL()												\
{																\
	UINT8 tpsl = ARG(); 										\
	if( (S.psl & tpsl) != tpsl )								\
		S.psl = (S.psl & ~CC) | 0x80;							\
	else														\
		S.psl &= ~CC;											\
}

/***************************************************************
 * M_TMI
 * Test under mask immediate
 ***************************************************************/
#define M_TMI(value)											\
{																\
	UINT8 tmi = ARG();											\
	S.psl &= ~CC;												\
	if( (value & tmi) != tmi )									\
		S.psl |= 0x80;											\
}

#if INLINE_EA
#define REL_EA(page) _REL_EA(page)
#define REL_ZERO(page) _REL_ZERO(page)
#define ABS_EA() _ABS_EA()
#define BRA_EA() _BRA_EA()
#else
static void REL_EA(unsigned short page) _REL_EA(page)
static void REL_ZERO(unsigned short page) _REL_ZERO(page)
static void ABS_EA(void) _ABS_EA()
static void BRA_EA(void) _BRA_EA()
#endif

void s2650_init(void)
{
}

void s2650_reset(void *param)
{
	memset(&S, 0, sizeof(S));
	S.psl = COM | WC;
	S.psu = 0;
}

void s2650_exit(void)
{
	/* nothing to do */
}

unsigned s2650_get_context(void *dst)
{
	if( dst )
		*(s2650_Regs*)dst = S;
	return sizeof(s2650_Regs);
}

void s2650_set_context(void *src)
{
	if( src )
	{
		S = *(s2650_Regs*)src;
		S.page = S.page & PAGE;
		S.iar = S.iar & PMSK;
        change_pc16(S.page + S.iar);
	}
}

unsigned s2650_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:
		case S2650_PC: return S.page + S.iar;
		case REG_SP: return S.psu & SP;
		case S2650_PS: return (S.psu << 8) | S.psl;
		case S2650_R0: return S.reg[0];
		case S2650_R1: return S.reg[1];
		case S2650_R2: return S.reg[2];
		case S2650_R3: return S.reg[3];
		case S2650_R1A: return S.reg[4];
		case S2650_R2A: return S.reg[5];
		case S2650_R3A: return S.reg[6];
		case S2650_HALT: return S.halt;
		case S2650_IRQ_STATE: return S.irq_state;
		case S2650_SI: return s2650_get_sense(); break;
		case S2650_FO: return s2650_get_flag(); break;
		case REG_PREVIOUSPC: return S.ppc; break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if( offset < 8 )
					return S.ras[offset];
			}
	}
	return 0;
}

void s2650_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:
			S.page = val & PAGE;
			S.iar = val & PMSK;
			change_pc16(S.page + S.iar);
			break;
		case S2650_PC: S.page = val & PAGE; S.iar = val & PMSK; break;
		case REG_SP: S.psu = (S.psu & ~SP) | (val & SP); break;
		case S2650_PS: S.psl = val & 0xff; S.psu = val >> 8; break;
		case S2650_R0: S.reg[0] = val; break;
		case S2650_R1: S.reg[1] = val; break;
		case S2650_R2: S.reg[2] = val; break;
		case S2650_R3: S.reg[3] = val; break;
		case S2650_R1A: S.reg[4] = val; break;
		case S2650_R2A: S.reg[5] = val; break;
		case S2650_R3A: S.reg[6] = val; break;
		case S2650_HALT: S.halt = val; break;
		case S2650_IRQ_STATE: s2650_set_irq_line(0, val); break;
		case S2650_SI: s2650_set_sense(val); break;
		case S2650_FO: s2650_set_flag(val); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if( offset < 8 )
					S.ras[offset] = val;
			}
    }
}

void s2650_set_irq_line(int irqline, int state)
{
	if (irqline == 1)
	{
		if (state == CLEAR_LINE)
			s2650_set_sense(0);
		else
			s2650_set_sense(1);
		return;
	}

	S.irq_state = state;
	CHECK_IRQ_LINE;
}

void s2650_set_irq_callback(int (*callback)(int irqline))
{
	S.irq_callback = callback;
}

void s2650_set_flag(int state)
{
    if (state)
        S.psu |= FO;
    else
        S.psu &= ~FO;
}

int s2650_get_flag(void)
{
    return (S.psu & FO) ? 1 : 0;
}

void s2650_set_sense(int state)
{
    if (state)
        S.psu |= SI;
    else
        S.psu &= ~SI;
}

int s2650_get_sense(void)
{
	/* OR'd with Input to allow for external connections */

    return (((S.psu & SI) ? 1 : 0) | ((cpu_readport16(S2650_SENSE_PORT) & SI) ? 1 : 0));
}

static  int S2650_Cycles[0x100] = {
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	2,2,2,2, 2,2,2,2, 3,3,3,3, 4,4,4,4,
	2,2,2,2, 3,3,3,3, 3,3,3,3, 3,3,3,3
};

int s2650_execute(int cycles)
{
	s2650_ICount = cycles;
	do
	{
		S.ppc = S.page + S.iar;

		CALL_MAME_DEBUG;

		S.ir = ROP();
		s2650_ICount -= S2650_Cycles[S.ir];
		S.r = S.ir & 3; 		/* register / value */
		switch (S.ir) {
			case 0x00:		/* LODZ,0 */
			case 0x01:		/* LODZ,1 */
			case 0x02:		/* LODZ,2 */
			case 0x03:		/* LODZ,3 */
				M_LOD( R0, S.reg[S.r] );
				break;

			case 0x04:		/* LODI,0 v */
			case 0x05:		/* LODI,1 v */
			case 0x06:		/* LODI,2 v */
			case 0x07:		/* LODI,3 v */
				M_LOD( S.reg[S.r], ARG() );
				break;

			case 0x08:		/* LODR,0 (*)a */
			case 0x09:		/* LODR,1 (*)a */
			case 0x0a:		/* LODR,2 (*)a */
			case 0x0b:		/* LODR,3 (*)a */
				REL_EA( S.page );
				M_LOD( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x0c:		/* LODA,0 (*)a(,X) */
			case 0x0d:		/* LODA,1 (*)a(,X) */
			case 0x0e:		/* LODA,2 (*)a(,X) */
			case 0x0f:		/* LODA,3 (*)a(,X) */
				ABS_EA();
				M_LOD( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x10:		/* illegal */
			case 0x11:		/* illegal */
				break;
			case 0x12:		/* SPSU */
				M_SPSU();
				break;
			case 0x13:		/* SPSL */
				M_SPSL();
				break;

			case 0x14:		/* RETC,0	(zero)	*/
			case 0x15:		/* RETC,1	(plus)	*/
			case 0x16:		/* RETC,2	(minus) */
				M_RET( (S.psl >> 6) == S.r );
				break;
			case 0x17:		/* RETC,3	(always) */
				M_RET( 1 );
				break;

			case 0x18:		/* BCTR,0  (*)a */
			case 0x19:		/* BCTR,1  (*)a */
			case 0x1a:		/* BCTR,2  (*)a */
				M_BRR( (S.psl >> 6) == S.r );
				break;
			case 0x1b:		/* BCTR,3  (*)a */
				M_BRR( 1 );
				break;

			case 0x1c:		/* BCTA,0  (*)a */
			case 0x1d:		/* BCTA,1  (*)a */
			case 0x1e:		/* BCTA,2  (*)a */
				M_BRA( (S.psl >> 6) == S.r );
				break;
			case 0x1f:		/* BCTA,3  (*)a */
				M_BRA( 1 );
				break;

			case 0x20:		/* EORZ,0 */
			case 0x21:		/* EORZ,1 */
			case 0x22:		/* EORZ,2 */
			case 0x23:		/* EORZ,3 */
				M_EOR( R0, S.reg[S.r] );
				break;

			case 0x24:		/* EORI,0 v */
			case 0x25:		/* EORI,1 v */
			case 0x26:		/* EORI,2 v */
			case 0x27:		/* EORI,3 v */
				M_EOR( S.reg[S.r], ARG() );
				break;

			case 0x28:		/* EORR,0 (*)a */
			case 0x29:		/* EORR,1 (*)a */
			case 0x2a:		/* EORR,2 (*)a */
			case 0x2b:		/* EORR,3 (*)a */
				REL_EA( S.page );
				M_EOR( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x2c:		/* EORA,0 (*)a(,X) */
			case 0x2d:		/* EORA,1 (*)a(,X) */
			case 0x2e:		/* EORA,2 (*)a(,X) */
			case 0x2f:		/* EORA,3 (*)a(,X) */
				ABS_EA();
				M_EOR( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x30:		/* REDC,0 */
			case 0x31:		/* REDC,1 */
			case 0x32:		/* REDC,2 */
			case 0x33:		/* REDC,3 */
				S.reg[S.r] = cpu_readport16(S2650_CTRL_PORT);
				SET_CC( S.reg[S.r] );
				break;

			case 0x34:		/* RETE,0 */
			case 0x35:		/* RETE,1 */
			case 0x36:		/* RETE,2 */
				M_RETE( (S.psl >> 6) == S.r );
				break;
			case 0x37:		/* RETE,3 */
				M_RETE( 1 );
				break;

			case 0x38:		/* BSTR,0 (*)a */
			case 0x39:		/* BSTR,1 (*)a */
			case 0x3a:		/* BSTR,2 (*)a */
				M_BSR( (S.psl >> 6) == S.r );
				break;
			case 0x3b:		/* BSTR,R3 (*)a */
				M_BSR( 1 );
				break;

			case 0x3c:		/* BSTA,0 (*)a */
			case 0x3d:		/* BSTA,1 (*)a */
			case 0x3e:		/* BSTA,2 (*)a */
				M_BSA( (S.psl >> 6) == S.r );
				break;
			case 0x3f:		/* BSTA,3 (*)a */
				M_BSA( 1 );
				break;

			case 0x40:		/* HALT */
				S.iar = (S.iar - 1) & PMSK;
				S.halt = 1;
				if (s2650_ICount > 0)
					s2650_ICount = 0;
				break;
			case 0x41:		/* ANDZ,1 */
			case 0x42:		/* ANDZ,2 */
			case 0x43:		/* ANDZ,3 */
				M_AND( R0, S.reg[S.r] );
				break;

			case 0x44:		/* ANDI,0 v */
			case 0x45:		/* ANDI,1 v */
			case 0x46:		/* ANDI,2 v */
			case 0x47:		/* ANDI,3 v */
				M_AND( S.reg[S.r], ARG() );
				break;

			case 0x48:		/* ANDR,0 (*)a */
			case 0x49:		/* ANDR,1 (*)a */
			case 0x4a:		/* ANDR,2 (*)a */
			case 0x4b:		/* ANDR,3 (*)a */
				REL_EA( S.page );
				M_AND( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x4c:		/* ANDA,0 (*)a(,X) */
			case 0x4d:		/* ANDA,1 (*)a(,X) */
			case 0x4e:		/* ANDA,2 (*)a(,X) */
			case 0x4f:		/* ANDA,3 (*)a(,X) */
				ABS_EA();
				M_AND( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x50:		/* RRR,0 */
			case 0x51:		/* RRR,1 */
			case 0x52:		/* RRR,2 */
			case 0x53:		/* RRR,3 */
				M_RRR( S.reg[S.r] );
				break;

			case 0x54:		/* REDE,0 v */
			case 0x55:		/* REDE,1 v */
			case 0x56:		/* REDE,2 v */
			case 0x57:		/* REDE,3 v */
				S.reg[S.r] = cpu_readport16( ARG() );
				SET_CC(S.reg[S.r]);
				break;

			case 0x58:		/* BRNR,0 (*)a */
			case 0x59:		/* BRNR,1 (*)a */
			case 0x5a:		/* BRNR,2 (*)a */
			case 0x5b:		/* BRNR,3 (*)a */
				M_BRR( S.reg[S.r] );
				break;

			case 0x5c:		/* BRNA,0 (*)a */
			case 0x5d:		/* BRNA,1 (*)a */
			case 0x5e:		/* BRNA,2 (*)a */
			case 0x5f:		/* BRNA,3 (*)a */
				M_BRA( S.reg[S.r] );
				break;

			case 0x60:		/* IORZ,0 */
			case 0x61:		/* IORZ,1 */
			case 0x62:		/* IORZ,2 */
			case 0x63:		/* IORZ,3 */
				M_IOR( R0, S.reg[S.r] );
				break;

			case 0x64:		/* IORI,0 v */
			case 0x65:		/* IORI,1 v */
			case 0x66:		/* IORI,2 v */
			case 0x67:		/* IORI,3 v */
				M_IOR( S.reg[S.r], ARG() );
				break;

			case 0x68:		/* IORR,0 (*)a */
			case 0x69:		/* IORR,1 (*)a */
			case 0x6a:		/* IORR,2 (*)a */
			case 0x6b:		/* IORR,3 (*)a */
				REL_EA( S.page );
				M_IOR( S.reg[S. r],RDMEM(S.ea) );
				break;

			case 0x6c:		/* IORA,0 (*)a(,X) */
			case 0x6d:		/* IORA,1 (*)a(,X) */
			case 0x6e:		/* IORA,2 (*)a(,X) */
			case 0x6f:		/* IORA,3 (*)a(,X) */
				ABS_EA();
				M_IOR( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x70:		/* REDD,0 */
			case 0x71:		/* REDD,1 */
			case 0x72:		/* REDD,2 */
			case 0x73:		/* REDD,3 */
				S.reg[S.r] = cpu_readport16(S2650_DATA_PORT);
				SET_CC(S.reg[S.r]);
				break;

			case 0x74:		/* CPSU */
				M_CPSU();
				break;
			case 0x75:		/* CPSL */
				M_CPSL();
				break;
			case 0x76:		/* PPSU */
				M_PPSU();
				break;
			case 0x77:		/* PPSL */
				M_PPSL();
				break;

			case 0x78:		/* BSNR,0 (*)a */
			case 0x79:		/* BSNR,1 (*)a */
			case 0x7a:		/* BSNR,2 (*)a */
			case 0x7b:		/* BSNR,3 (*)a */
				M_BSR( S.reg[S.r] );
				break;

			case 0x7c:		/* BSNA,0 (*)a */
			case 0x7d:		/* BSNA,1 (*)a */
			case 0x7e:		/* BSNA,2 (*)a */
			case 0x7f:		/* BSNA,3 (*)a */
				M_BSA( S.reg[S.r] );
				break;

			case 0x80:		/* ADDZ,0 */
			case 0x81:		/* ADDZ,1 */
			case 0x82:		/* ADDZ,2 */
			case 0x83:		/* ADDZ,3 */
				M_ADD( R0,S.reg[S.r] );
				break;

			case 0x84:		/* ADDI,0 v */
			case 0x85:		/* ADDI,1 v */
			case 0x86:		/* ADDI,2 v */
			case 0x87:		/* ADDI,3 v */
				M_ADD( S.reg[S.r], ARG() );
				break;

			case 0x88:		/* ADDR,0 (*)a */
			case 0x89:		/* ADDR,1 (*)a */
			case 0x8a:		/* ADDR,2 (*)a */
			case 0x8b:		/* ADDR,3 (*)a */
				REL_EA(S.page);
				M_ADD( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x8c:		/* ADDA,0 (*)a(,X) */
			case 0x8d:		/* ADDA,1 (*)a(,X) */
			case 0x8e:		/* ADDA,2 (*)a(,X) */
			case 0x8f:		/* ADDA,3 (*)a(,X) */
				ABS_EA();
				M_ADD( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0x90:		/* illegal */
			case 0x91:		/* illegal */
				break;
			case 0x92:		/* LPSU */
				S.psu = R0 & ~PSU34;
				break;
			case 0x93:		/* LPSL */
				/* change register set ? */
				if ((S.psl ^ R0) & RS)
					SWAP_REGS;
				S.psl = R0;
				break;

			case 0x94:		/* DAR,0 */
			case 0x95:		/* DAR,1 */
			case 0x96:		/* DAR,2 */
			case 0x97:		/* DAR,3 */
				M_DAR( S.reg[S.r] );
				break;

			case 0x98:		/* BCFR,0 (*)a */
			case 0x99:		/* BCFR,1 (*)a */
			case 0x9a:		/* BCFR,2 (*)a */
				M_BRR( (S.psl >> 6) != S.r );
				break;
			case 0x9b:		/* ZBRR    (*)a */
				M_ZBRR();
				break;

			case 0x9c:		/* BCFA,0 (*)a */
			case 0x9d:		/* BCFA,1 (*)a */
			case 0x9e:		/* BCFA,2 (*)a */
				M_BRA( (S.psl >> 6) != S.r );
				break;
			case 0x9f:		/* BXA	   (*)a */
				M_BXA();
				break;

			case 0xa0:		/* SUBZ,0 */
			case 0xa1:		/* SUBZ,1 */
			case 0xa2:		/* SUBZ,2 */
			case 0xa3:		/* SUBZ,3 */
				M_SUB( R0, S.reg[S.r] );
				break;

			case 0xa4:		/* SUBI,0 v */
			case 0xa5:		/* SUBI,1 v */
			case 0xa6:		/* SUBI,2 v */
			case 0xa7:		/* SUBI,3 v */
				M_SUB( S.reg[S.r], ARG() );
				break;

			case 0xa8:		/* SUBR,0 (*)a */
			case 0xa9:		/* SUBR,1 (*)a */
			case 0xaa:		/* SUBR,2 (*)a */
			case 0xab:		/* SUBR,3 (*)a */
				REL_EA(S.page);
				M_SUB( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0xac:		/* SUBA,0 (*)a(,X) */
			case 0xad:		/* SUBA,1 (*)a(,X) */
			case 0xae:		/* SUBA,2 (*)a(,X) */
			case 0xaf:		/* SUBA,3 (*)a(,X) */
				ABS_EA();
				M_SUB( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0xb0:		/* WRTC,0 */
			case 0xb1:		/* WRTC,1 */
			case 0xb2:		/* WRTC,2 */
			case 0xb3:		/* WRTC,3 */
				cpu_writeport16(S2650_CTRL_PORT,S.reg[S.r]);
				break;

			case 0xb4:		/* TPSU */
				M_TPSU();
				break;
			case 0xb5:		/* TPSL */
				M_TPSL();
				break;
			case 0xb6:		/* illegal */
			case 0xb7:		/* illegal */
				break;

			case 0xb8:		/* BSFR,0 (*)a */
			case 0xb9:		/* BSFR,1 (*)a */
			case 0xba:		/* BSFR,2 (*)a */
				M_BSR( (S.psl >> 6) != S.r );
				break;
			case 0xbb:		/* ZBSR    (*)a */
				M_ZBSR();
				break;

			case 0xbc:		/* BSFA,0 (*)a */
			case 0xbd:		/* BSFA,1 (*)a */
			case 0xbe:		/* BSFA,2 (*)a */
				M_BSA( (S.psl >> 6) != S.r );
				break;
			case 0xbf:		/* BSXA    (*)a */
				M_BSXA();
				break;

			case 0xc0:		/* NOP */
				break;
			case 0xc1:		/* STRZ,1 */
			case 0xc2:		/* STRZ,2 */
			case 0xc3:		/* STRZ,3 */
				M_LOD( S.reg[S.r], R0 );
				break;

			case 0xc4:		/* illegal */
			case 0xc5:		/* illegal */
			case 0xc6:		/* illegal */
			case 0xc7:		/* illegal */
				break;

			case 0xc8:		/* STRR,0 (*)a */
			case 0xc9:		/* STRR,1 (*)a */
			case 0xca:		/* STRR,2 (*)a */
			case 0xcb:		/* STRR,3 (*)a */
				REL_EA(S.page);
				M_STR( S.ea, S.reg[S.r] );
				break;

			case 0xcc:		/* STRA,0 (*)a(,X) */
			case 0xcd:		/* STRA,1 (*)a(,X) */
			case 0xce:		/* STRA,2 (*)a(,X) */
			case 0xcf:		/* STRA,3 (*)a(,X) */
				ABS_EA();
				M_STR( S.ea, S.reg[S.r] );
				break;

			case 0xd0:		/* RRL,0 */
			case 0xd1:		/* RRL,1 */
			case 0xd2:		/* RRL,2 */
			case 0xd3:		/* RRL,3 */
				M_RRL( S.reg[S.r] );
				break;

			case 0xd4:		/* WRTE,0 v */
			case 0xd5:		/* WRTE,1 v */
			case 0xd6:		/* WRTE,2 v */
			case 0xd7:		/* WRTE,3 v */
				cpu_writeport16( ARG(), S.reg[S.r] );
				break;

			case 0xd8:		/* BIRR,0 (*)a */
			case 0xd9:		/* BIRR,1 (*)a */
			case 0xda:		/* BIRR,2 (*)a */
			case 0xdb:		/* BIRR,3 (*)a */
				M_BRR( ++S.reg[S.r] );
				break;

			case 0xdc:		/* BIRA,0 (*)a */
			case 0xdd:		/* BIRA,1 (*)a */
			case 0xde:		/* BIRA,2 (*)a */
			case 0xdf:		/* BIRA,3 (*)a */
				M_BRA( ++S.reg[S.r] );
				break;

			case 0xe0:		/* COMZ,0 */
			case 0xe1:		/* COMZ,1 */
			case 0xe2:		/* COMZ,2 */
			case 0xe3:		/* COMZ,3 */
				M_COM( R0, S.reg[S.r] );
				break;

			case 0xe4:		/* COMI,0 v */
			case 0xe5:		/* COMI,1 v */
			case 0xe6:		/* COMI,2 v */
			case 0xe7:		/* COMI,3 v */
				M_COM( S.reg[S.r], ARG() );
				break;

			case 0xe8:		/* COMR,0 (*)a */
			case 0xe9:		/* COMR,1 (*)a */
			case 0xea:		/* COMR,2 (*)a */
			case 0xeb:		/* COMR,3 (*)a */
				REL_EA(S.page);
				M_COM( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0xec:		/* COMA,0 (*)a(,X) */
			case 0xed:		/* COMA,1 (*)a(,X) */
			case 0xee:		/* COMA,2 (*)a(,X) */
			case 0xef:		/* COMA,3 (*)a(,X) */
				ABS_EA();
				M_COM( S.reg[S.r], RDMEM(S.ea) );
				break;

			case 0xf0:		/* WRTD,0 */
			case 0xf1:		/* WRTD,1 */
			case 0xf2:		/* WRTD,2 */
			case 0xf3:		/* WRTD,3 */
				cpu_writeport16(S2650_DATA_PORT, S.reg[S.r]);
				break;

			case 0xf4:		/* TMI,0  v */
			case 0xf5:		/* TMI,1  v */
			case 0xf6:		/* TMI,2  v */
			case 0xf7:		/* TMI,3  v */
				M_TMI( S.reg[S.r] );
				break;

			case 0xf8:		/* BDRR,0 (*)a */
			case 0xf9:		/* BDRR,1 (*)a */
			case 0xfa:		/* BDRR,2 (*)a */
			case 0xfb:		/* BDRR,3 (*)a */
				M_BRR( --S.reg[S.r] );
				break;

			case 0xfc:		/* BDRA,0 (*)a */
			case 0xfd:		/* BDRA,1 (*)a */
			case 0xfe:		/* BDRA,2 (*)a */
			case 0xff:		/* BDRA,3 (*)a */
				M_BRA( --S.reg[S.r] );
				break;
		}
	} while( s2650_ICount > 0 );

	return cycles - s2650_ICount;
}
#if 0
void s2650_state_save(void *file)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file,"s2650",cpu,"PAGE",&S.page,1);
	state_save_UINT16(file,"s2650",cpu,"IAR",&S.iar,1);
	state_save_UINT8(file,"s2650",cpu,"PSL",&S.psl,1);
	state_save_UINT8(file,"s2650",cpu,"PSU",&S.psu,1);
	state_save_UINT8(file,"s2650",cpu,"REG",S.reg,7);
	state_save_UINT8(file,"s2650",cpu,"HALT",&S.halt,1);
	state_save_UINT16(file,"s2650",cpu,"RAS",S.ras,8);
	state_save_UINT8(file,"s2650",cpu,"IRQ_STATE",&S.irq_state,1);
}

void s2650_state_load(void *file)
{
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file,"s2650",cpu,"PAGE",&S.page,1);
	state_load_UINT16(file,"s2650",cpu,"IAR",&S.iar,1);
	state_load_UINT8(file,"s2650",cpu,"PSL",&S.psl,1);
	state_load_UINT8(file,"s2650",cpu,"PSU",&S.psu,1);
	state_load_UINT8(file,"s2650",cpu,"REG",S.reg,7);
	state_load_UINT8(file,"s2650",cpu,"HALT",&S.halt,1);
	state_load_UINT16(file,"s2650",cpu,"RAS",S.ras,8);
	state_load_UINT8(file,"s2650",cpu,"IRQ_STATE",&S.irq_state,1);
}
#endif
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *s2650_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	s2650_Regs *r = context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';

    if( !context )
		r = &S;

    switch( regnum )
	{
		case CPU_INFO_FLAGS:
		case CPU_INFO_REG+S2650_PC: sprintf(buffer[which], "PC:%04X", r->page + r->iar); break;
		case CPU_INFO_REG+S2650_PS: sprintf(buffer[which], "PS:%02X%02X", r->psu, r->psl); break;
		case CPU_INFO_REG+S2650_R0: sprintf(buffer[which], "R0:%02X", r->reg[0]); break;
		case CPU_INFO_REG+S2650_R1: sprintf(buffer[which], "R1:%02X", r->reg[1]); break;
		case CPU_INFO_REG+S2650_R2: sprintf(buffer[which], "R2:%02X", r->reg[2]); break;
		case CPU_INFO_REG+S2650_R3: sprintf(buffer[which], "R3:%02X", r->reg[3]); break;
		case CPU_INFO_REG+S2650_R1A: sprintf(buffer[which], "R1'%02X", r->reg[4]); break;
		case CPU_INFO_REG+S2650_R2A: sprintf(buffer[which], "R2'%02X", r->reg[5]); break;
		case CPU_INFO_REG+S2650_R3A: sprintf(buffer[which], "R3'%02X", r->reg[6]); break;
		case CPU_INFO_REG+S2650_HALT: sprintf(buffer[which], "HALT:%X", r->halt); break;
		case CPU_INFO_REG+S2650_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
		case CPU_INFO_REG+S2650_SI: sprintf(buffer[which], "SI:%X", (r->psu & SI) ? 1 : 0); break;
		case CPU_INFO_REG+S2650_FO: sprintf(buffer[which], "FO:%X", (r->psu & FO) ? 1 : 0); break;
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->psu & 0x80 ? 'S':'.',
				r->psu & 0x40 ? 'O':'.',
				r->psu & 0x20 ? 'I':'.',
				r->psu & 0x10 ? '?':'.',
				r->psu & 0x08 ? '?':'.',
				r->psu & 0x04 ? 's':'.',
				r->psu & 0x02 ? 's':'.',
				r->psu & 0x01 ? 's':'.',
                r->psl & 0x80 ? 'M':'.',
				r->psl & 0x40 ? 'P':'.',
				r->psl & 0x20 ? 'H':'.',
				r->psl & 0x10 ? 'R':'.',
				r->psl & 0x08 ? 'W':'.',
				r->psl & 0x04 ? 'V':'.',
				r->psl & 0x02 ? '2':'.',
				r->psl & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "S2650";
		case CPU_INFO_FAMILY: return "Signetics 2650";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Written by Juergen Buchmueller for use with MAME";
		case CPU_INFO_REG_LAYOUT: return (const char *)s2650_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)s2650_win_layout;
	}
	return buffer[which];
}

unsigned s2650_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return Dasm2650(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
