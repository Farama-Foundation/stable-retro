/*****************************************************************************
 *
 *	 opsce02.h
 *	 Addressing mode and opcode macros for 65ce02 CPU
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
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

#define m6502 m65ce02
#define m6502_ICount m65ce02_ICount

/* stack disable extended (word) mode */
#undef F_T
#define F_E 0x20

/* some shortcuts for improved readability */
#define Z	m65ce02.z
#define B	m65ce02.zp.b.h
#define SW m6502.sp.w.l
#define SPL m6502.sp.b.l
#define SPH m6502.sp.b.h

#define PEEK_OP() cpu_readop(PCW)

#define RDMEM_WORD(addr, pair)			\
   pair.b.l=RDMEM( addr );				\
   pair.b.h=RDMEM( (addr+1) & 0xffff )

#define WRMEM_WORD(addr,pair)			\
   WRMEM(addr,pair.b.l);				\
   WRMEM((addr+1)&0xffff,pair.b.h)

#define WB_EA_WORD	WRMEM_WORD(EAD, tmp)


#define SET_NZ_WORD(n)											\
	if( n.w.l == 0 )											\
		P = (P & ~F_N) | F_Z;									\
	else														\
		P = (P & ~(F_N | F_Z)) | ((n.b.h) & F_N)

/* EA_IDY 
 *  ???? subtract 1 cycle if page boundary is crossed */


/***************************************************************
 *	EA = zero page indirect + Z (post indexed)
 *  ???? subtract 1 cycle if page boundary is crossed
 ***************************************************************/
#define EA_IDZ													\
	ZPL = RDOPARG();											\
	EAL = RDMEM(ZPD);											\
	ZPL++;														\
	EAH = RDMEM(ZPD);											\
	if (EAL + Z > 0xff) 										\
		m6502_ICount--; 										\
	EAW += Z

/***************************************************************
 *	EA = zero page indexed stack, indirect + Y (post indexed)
 *	??? subtract 1 cycle if page boundary is crossed
 ***************************************************************/
/* i think its based on stack high byte instead of of bank register */
#define EA_ZP_INSP_INY											\
{																\
	PAIR pair={{0}};											\
	pair.b.l = SPL+RDOPARG();									\
	pair.b.h = SPH; 											\
	EAL = RDMEM(pair.d);										\
	if( P & F_E )												\
		pair.b.l++; 											\
	else														\
		pair.w.l++; 											\
	EAH = RDMEM(pair.d);										\
	if( EAL + Y > 0xff )										\
		m6502_ICount--; 										\
	EAW += Y;													\
}

#define RD_IMM_WORD tmp.b.l = RDOPARG(); tmp.b.h=RDOPARG()
#define RD_IDZ	EA_IDZ; tmp = RDMEM(EAD)
#define WR_IDZ	EA_IDZ; WRMEM(EAD, tmp)

#define RD_INSY EA_ZP_INSP_INY; tmp = RDMEM(EAD)
#define WR_INSY EA_ZP_INSP_INY; WRMEM(EAD, tmp)

#define RD_ABS_WORD EA_ABS; RDMEM_WORD(EAD, tmp)
#define WR_ABS_WORD EA_ABS; WRMEM_WORD(EAD, tmp)

#define RD_ZPG_WORD EA_ZPG; RDMEM_WORD(EAD, tmp)
#define WR_ZPG_WORD EA_ZPG; WRMEM_WORD(EAD, tmp)

/***************************************************************
 * push a register onto the stack
 ***************************************************************/
#undef PUSH
#define PUSH(Rg) WRMEM(SPD, Rg); if (P&F_E) { S--; } else { SW--; }

/***************************************************************
 * pull a register from the stack
 ***************************************************************/
#undef PULL
#define PULL(Rg) if (P&F_E) { S++; } else { SW++; } Rg = RDMEM(SPD)

/* the order in which the args are pushed is correct! */
#define PUSH_WORD(pair) PUSH(pair.b.l);PUSH(pair.b.h)

/***************************************************************
 *	BRA  branch relative
 *	extra cycle if page boundary is crossed
 ***************************************************************/
#undef BRA
#define BRA(cond)												\
	if (cond)													\
	{															\
		tmp = RDOPARG();										\
		EAW = PCW + (signed char)tmp;							\
		PCD = EAD;												\
		CHANGE_PC;												\
	}															\
	else														\
	{															\
		PCW++;													\
	}

/***************************************************************
 *	BRA  branch relative
 ***************************************************************/
#define BRA_WORD(cond)											\
	if (cond)													\
	{															\
		EAL = RDOPARG();										\
		EAH = RDOPARG();										\
		EAW = PCW + (short)(EAW-1); 							\
		PCD = EAD;												\
		CHANGE_PC;												\
	}															\
	else														\
	{															\
		PCW += 2;												\
	}

/* 65ce02 ******************************************************
 *	cle clear disable extended stack flag
 ***************************************************************/
#define CLE 													\
		P|=F_E

/* 65ce02 ******************************************************
 *	see set disable extended stack flag
 ***************************************************************/
#define SEE 													\
		P&=~F_E

/* 65ce02 ******************************************************
 *	augment
 ***************************************************************/
#define AUG 													\
 t1=RDOPARG(); t2=RDOPARG(); t3=RDOPARG(); \
 retro_log_cb(RETRO_LOG_DEBUG, LOGPRE "m65ce02 at pc:%.4x reserved op aug %.2x %.2x %.2x\n", \
  t1,t2,t3);

/* 65ce02 ******************************************************
 *	rts imm
 ***************************************************************/
#define RTN 												\
  if (P&F_E) { S+=tmp; } else { SW+=tmp; }

/* 65ce02 ******************************************************
 *	NEG accu
 * twos complement
 ***************************************************************/
#define NEG 													\
	A= (A^0xff)+1;												\
	SET_NZ(A)

/* 65ce02 ******************************************************
 *	ASR arithmetic (signed) shift right
 *	[7] -> [7][6][5][4][3][2][1][0] -> C
 ***************************************************************/
#define ASR_65CE02												\
	P = (P & ~F_C) | (tmp & F_C);								\
	tmp = (signed char)tmp >> 1;								\
	SET_NZ(tmp)

/* 65ce02 ******************************************************
 *	ASW arithmetic shift left word
 *	[c] <- [15]..[6][5][4][3][2][1][0]
 ***************************************************************/
/* not sure about how 16 bit memory modifying is executed */
#define ASW 													\
	tmp.w.l = tmp.w.l << 1; 									\
	P = (P & ~F_C) | (tmp.b.h2 & F_C);							\
	SET_NZ_WORD(tmp);											

/* 65ce02 ******************************************************
 *	ROW rotate left word
 *	[c] <- [15]..[6][5][4][3][2][1][0] <- C
 ***************************************************************/
/* not sure about how 16 bit memory modifying is executed */
#define ROW 													\
	tmp.d =(tmp.d << 1);										\
	tmp.w.l |= (P & F_C);										\
	P = (P & ~F_C) | (tmp.w.l & F_C);							\
	SET_NZ_WORD(tmp);											\

/* 65ce02 ******************************************************
 *	CPZ Compare index Z
 ***************************************************************/
#define CPZ 													\
	P &= ~F_C;													\
	if (Z >= tmp)												\
		P |= F_C;												\
	SET_NZ((UINT8)(Z - tmp))

/* 65ce02 ******************************************************
 *	DEZ Decrement index Z
 ***************************************************************/
#define DEZ 													\
	Z = (UINT8)--Z; 											\
	SET_NZ(Z)

/* 65ce02 ******************************************************
 *	DEC Decrement memory word
 ***************************************************************/
/* not sure about this */
#define DEW 													\
	tmp.w.l = --tmp.w.l;										\
	SET_NZ_WORD(tmp)

/* 65ce02 ******************************************************
 *	DEZ Decrement index Z
 ***************************************************************/
#define INZ 													\
	Z = (UINT8)++Z; 											\
	SET_NZ(Z)

/* 65ce02 ******************************************************
 *	INW Increment memory word
 ***************************************************************/
#define INW 													\
	tmp.w.l = ++tmp.w.l;										\
	SET_NZ_WORD(tmp)

/* 65ce02 ******************************************************
 *	LDZ Load index Z
 ***************************************************************/
#define LDZ 													\
	Z = (UINT8)tmp; 											\
	SET_NZ(Z)

/* 65ce02 ******************************************************
 * STZ	Store index Z
 ***************************************************************/
#define STZ_65CE02												\
	tmp = Z

/* 65ce02 ******************************************************
 *	PHZ Push index z
 ***************************************************************/
#define PHZ 													\
	PUSH(Z)

/* 65ce02 ******************************************************
 *	PLA Pull accumulator
 ***************************************************************/
#define PLZ 													\
	PULL(Z); \
    SET_NZ(Z)

/* 65ce02 ******************************************************
 * TAZ	Transfer accumulator to index z
 ***************************************************************/
#define TAZ 													\
	Z = A;														\
	SET_NZ(Z)

/* 65ce02 ******************************************************
 * TZA	Transfer index z to accumulator
 ***************************************************************/
#define TZA 													\
	A = Z;														\
	SET_NZ(A)

/* 65ce02 ******************************************************
 * TSY	Transfer stack pointer to index y
 ***************************************************************/
#define TSY 													\
	Y = SPH;													\
	SET_NZ(Y)

/* 65ce02 ******************************************************
 * TYS	Transfer index y to stack pointer
 ***************************************************************/
#define TYS 													\
	SPH = Y;

/* 65ce02 ******************************************************
 * TAB	Transfer accumulator to Direct Page
 ***************************************************************/
#define TAB 													\
	B = A;														\
	SET_NZ(B)

/* 65ce02 ******************************************************
 * TBA	Transfer direct page to accumulator
 ***************************************************************/
#define TBA 													\
	A = B;														\
	SET_NZ(A)

/* 65ce02 ******************************************************
 *	BCC Branch if carry clear
 ***************************************************************/
#define BCC_WORD BRA_WORD(!(P & F_C))

/* 65ce02 ******************************************************
 *	BCS Branch if carry set
 ***************************************************************/
#define BCS_WORD BRA_WORD(P & F_C)

/* 65ce02 ******************************************************
 *	BEQ Branch if equal
 ***************************************************************/
#define BEQ_WORD BRA_WORD(P & F_Z)

/* 65ce02 ******************************************************
 *	BMI Branch if minus
 ***************************************************************/
#define BMI_WORD BRA_WORD(P & F_N)

/* 65ce02 ******************************************************
 *	BNE Branch if not equal
 ***************************************************************/
#define BNE_WORD BRA_WORD(!(P & F_Z))

/* 65ce02 ******************************************************
 *	BPL Branch if plus
 ***************************************************************/
#define BPL_WORD BRA_WORD(!(P & F_N))

/* 65ce02 ******************************************************
 * BVC	Branch if overflow clear
 ***************************************************************/
#define BVC_WORD BRA_WORD(!(P & F_V))

/* 65ce02 ******************************************************
 * BVS	Branch if overflow set
 ***************************************************************/
#define BVS_WORD BRA_WORD(P & F_V)

/* 65ce02 ******************************************************
 *	JSR Jump to subroutine
 *	decrement PC (sic!) push PC hi, push PC lo and set
 *	PC to the effective address
 ***************************************************************/
#define JSR_IND 												\
	EAL = RDOPARG();											\
	PUSH(PCH);													\
	PUSH(PCL);													\
	EAH = RDOPARG();											\
	PCL = RDMEM(EAD);											\
	EAL++;	/* booby trap: stay in same page! ;-) */			\
	PCH = RDMEM(EAD);											\
	CHANGE_PC

/* 65ce02 ******************************************************
 *	JSR Jump to subroutine
 *	decrement PC (sic!) push PC hi, push PC lo and set
 *	PC to the effective address
 ***************************************************************/
#define JSR_INDX												\
	EAL = RDOPARG()+X;											\
	PUSH(PCH);													\
	PUSH(PCL);													\
	EAH = RDOPARG();											\
	PCL = RDMEM(EAD);											\
	EAL++;	/* booby trap: stay in same page! ;-) */			\
	PCH = RDMEM(EAD);											\
	CHANGE_PC

/* 65ce02 ******************************************************
 *	PLP Pull processor status (flags)
 ***************************************************************/
#undef PLP
#define PLP 													\
	if ( P & F_I )												\
	{															\
		UINT8 temp; \
		PULL(temp);												\
		P=(P&F_E)|F_B|(temp&~F_E); \
		if( m6502.irq_state != CLEAR_LINE && !(P & F_I) )		\
		{														\
			log_cb(RETRO_LOG_DEBUG, LOGPRE "M65ce02#%d PLP sets after_cli\n",             \
				cpu_getactivecpu());							\
			m6502.after_cli = 1;								\
		}														\
	}															\
	else														\
	{															\
		UINT8 temp; \
		PULL(temp);												\
		P=(P&F_E)|F_B|(temp&~F_E); \
	}															

/* 65ce02 ********************************************************
 * RTI	Return from interrupt
 * pull flags, pull PC lo, pull PC hi and increment PC
 *	PCW++;
 ***************************************************************/
#undef RTI
#define RTI 													\
{ UINT8 temp;PULL(temp);P=(P&F_E)|F_B|(temp&~F_E); } \
	PULL(PCL);													\
	PULL(PCH);													\
	if( m65ce02.irq_state != CLEAR_LINE && !(P & F_I) ) 		\
	{															\
		log_cb(RETRO_LOG_DEBUG, LOGPRE "M65ce02#%d RTI sets after_cli\n",                 \
			cpu_getactivecpu());								\
		m6502.after_cli = 1;									\
	}															\
	CHANGE_PC

/* 65ce02 ********************************************************
 * TXS	Transfer index X to stack LSB
 * no flags changed (sic!)
 * txs tys not interruptable
 ***************************************************************/
#undef TXS
#define TXS 													\
		S = X;													\
		if (PEEK_OP() == 0x2b /*TYS*/ ) 						\
		{														\
				UINT8 op = RDOP();								\
				(*m65ce02.insn[op])();							\
		}
