/*****************************************************************************
 *
 *	 m6502ops.h
 *	 Addressing mode and opcode macros for 6502,65c02,65sc02,6510,n2a03 CPUs
 *
 *	 Copyright (c) 1998,1999,2000 Juergen Buchmueller, all rights reserved.
 *	 65sc02 core Copyright (c) 2000 Peter Trauner, all rights reserved.
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

/***************************************************************
 *	EA = indirect (only used by JMP)
 * correct overflow handling
 ***************************************************************/
#undef EA_IND
#define EA_IND													\
	EA_ABS; 													\
	tmp = RDMEM(EAD);											\
	if (EAL==0xff) m6502_ICount++;								\
	EAD++;														\
	EAH = RDMEM(EAD);											\
	EAL = tmp

/***************************************************************
 *	EA = zero page indirect (65c02 pre indexed w/o X)
 ***************************************************************/
#define EA_ZPI													\
	ZPL = RDOPARG();											\
	EAL = RDMEM(ZPD);											\
	ZPL++;														\
	EAH = RDMEM(ZPD)

/***************************************************************
 *	EA = indirect plus x (only used by 65c02 JMP)
 ***************************************************************/
#define EA_IAX													\
	 EA_ABS;													\
	 if (EAL + X > 0xff) /* assumption; probably wrong ? */ 	\
		 m6502_ICount--;										\
	 EAW += X;													\
	 tmp = RDMEM(EAD);											\
	 if (EAL==0xff) m6502_ICount++; 							\
	 EAD++; 													\
	 EAH = RDMEM(EAD);											\
	 EAL = tmp

#define RD_ZPI	EA_ZPI; tmp = RDMEM(EAD)

/* write a value from tmp */
#define WR_ZPI	EA_ZPI; WRMEM(EAD, tmp)

/***************************************************************
 ***************************************************************
 *			Macros to emulate the 65C02 opcodes
 ***************************************************************
 ***************************************************************/


/* 65C02 ********************************************************
 *	ADC Add with carry
 * different setting of flags in decimal mode
 ***************************************************************/
#undef ADC
#define ADC 													\
	if (P & F_D)												\
	{															\
		int c = (P & F_C);										\
		int lo = (A & 0x0f) + (tmp & 0x0f) + c; 				\
		int hi = (A & 0xf0) + (tmp & 0xf0); 					\
		P &= ~(F_V | F_C);										\
		if( lo > 0x09 ) 										\
		{														\
			hi += 0x10; 										\
			lo += 0x06; 										\
		}														\
		if( ~(A^tmp) & (A^hi) & F_N )							\
			P |= F_V;											\
		if( hi > 0x90 ) 										\
			hi += 0x60; 										\
		if( hi & 0xff00 )										\
			P |= F_C;											\
		A = (lo & 0x0f) + (hi & 0xf0);							\
	}															\
	else														\
	{															\
		int c = (P & F_C);										\
		int sum = A + tmp + c;									\
		P &= ~(F_V | F_C);										\
		if( ~(A^tmp) & (A^sum) & F_N )							\
			P |= F_V;											\
		if( sum & 0xff00 )										\
			P |= F_C;											\
		A = (UINT8) sum;										\
	}															\
	SET_NZ(A)

/* 65C02 ********************************************************
 *	SBC Subtract with carry
 * different setting of flags in decimal mode
 ***************************************************************/
#undef SBC
#define SBC 													\
	if (P & F_D)												\
	{															\
		int c = (P & F_C) ^ F_C;								\
		int sum = A - tmp - c;									\
		int lo = (A & 0x0f) - (tmp & 0x0f) - c; 				\
		int hi = (A & 0xf0) - (tmp & 0xf0); 					\
		P &= ~(F_V | F_C);										\
		if( (A^tmp) & (A^sum) & F_N )							\
			P |= F_V;											\
		if( lo & 0xf0 ) 										\
			lo -= 6;											\
		if( lo & 0x80 ) 										\
			hi -= 0x10; 										\
		if( hi & 0x0f00 )										\
			hi -= 0x60; 										\
		if( (sum & 0xff00) == 0 )								\
			P |= F_C;											\
		A = (lo & 0x0f) + (hi & 0xf0);							\
	}															\
	else														\
	{															\
		int c = (P & F_C) ^ F_C;								\
		int sum = A - tmp - c;									\
		P &= ~(F_V | F_C);										\
		if( (A^tmp) & (A^sum) & F_N )							\
			P |= F_V;											\
		if( (sum & 0xff00) == 0 )								\
			P |= F_C;											\
		A = (UINT8) sum;										\
	}															\
	SET_NZ(A)

/* 65C02 *******************************************************
 *	BBR Branch if bit is reset
 ***************************************************************/
#define BBR(bit)												\
	BRA(!(tmp & (1<<bit)))

/* 65C02 *******************************************************
 *	BBS Branch if bit is set
 ***************************************************************/
#define BBS(bit)												\
	BRA(tmp & (1<<bit))

/* 65c02 ********************************************************
 *	BRK Break
 *	increment PC, push PC hi, PC lo, flags (with B bit set),
 *	set I flag, reset D flag and jump via IRQ vector
 ***************************************************************/
#undef BRK
#define BRK 													\
	PCW++;														\
	PUSH(PCH);													\
	PUSH(PCL);													\
	PUSH(P | F_B);												\
	P = (P | F_I) & ~F_D;										\
	PCL = RDMEM(M6502_IRQ_VEC); 								\
	PCH = RDMEM(M6502_IRQ_VEC+1);								\
	CHANGE_PC


/* 65C02 *******************************************************
 *	DEA Decrement accumulator
 ***************************************************************/
#define DEA 													\
	A = (UINT8)--A; 											\
	SET_NZ(A)

/* 65C02 *******************************************************
 *	INA Increment accumulator
 ***************************************************************/
#define INA 													\
	A = (UINT8)++A; 											\
	SET_NZ(A)

/* 65C02 *******************************************************
 *	PHX Push index X
 ***************************************************************/
#define PHX 													\
	PUSH(X)

/* 65C02 *******************************************************
 *	PHY Push index Y
 ***************************************************************/
#define PHY 													\
	PUSH(Y)

/* 65C02 *******************************************************
 *	PLX Pull index X
 ***************************************************************/
#define PLX 													\
	PULL(X); \
	SET_NZ(X)

/* 65C02 *******************************************************
 *	PLY Pull index Y
 ***************************************************************/
#define PLY 													\
	PULL(Y); \
	SET_NZ(Y)

/* 65C02 *******************************************************
 *	RMB Reset memory bit
 ***************************************************************/
#define RMB(bit)												\
	tmp &= ~(1<<bit)

/* 65C02 *******************************************************
 *	SMB Set memory bit
 ***************************************************************/
#define SMB(bit)												\
	tmp |= (1<<bit)

/* 65C02 *******************************************************
 * STZ	Store zero
 ***************************************************************/
#define STZ 													\
	tmp = 0

/* 65C02 *******************************************************
 * TRB	Test and reset bits
 ***************************************************************/
#define TRB 													\
	SET_Z(tmp&A);												\
	tmp &= ~A

/* 65C02 *******************************************************
 * TSB	Test and set bits
 ***************************************************************/
#define TSB 													\
	SET_Z(tmp&A);												\
	tmp |= A


/***************************************************************
 ***************************************************************
 *			Macros to emulate the 65sc02 opcodes
 ***************************************************************
 ***************************************************************/


/* 65sc02 ********************************************************
 *	BSR Branch to subroutine
 ***************************************************************/
#define BSR 													\
	EAL = RDOPARG();											\
	PUSH(PCH);													\
	PUSH(PCL);													\
	EAH = RDOPARG();											\
	EAW = PCW + (INT16)(EAW-1); 								\
	PCD = EAD;													\
	CHANGE_PC
