/*****************************************************************************
 *
 *	 i8x41.c
 *	 Portable UPI-41/8041/8741/8042/8742 emulator V0.2
 *
 *	 Copyright (c) 1999 Juergen Buchmueller, all rights reserved.
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
 *	This work is solely based on the
 *	'Intel(tm) UPI(tm)-41AH/42AH Users Manual'
 *
 *
 *	**** Change Log ****
 *
 *	TLP (10-Jan-2003) Changed ver from 0.1 to 0.2
 *	 -ÿChanged the internal RAM mask from 3Fh to FFh . The i8x41/i8x42 have
 *	   128/256 bytes of internal RAM respectively.
 *	 -ÿAdded output port data to the debug register view window.
 *	 -ÿAdded some missing break commands to the set_reg switch function.
 *	 -ÿChanged Ports 1 and 2 to latched types (Quasi-bidirectional).
 *	 -ÿStopped illegal access to Port 0 and 3 (they don't exist).
 *	 -ÿChanged ANLD, ORLD and MOVD instructions to act through Port 2 in
 *	   nibble mode.
 *	 -ÿCopied F0 and moved F1 flags to the STATE flag bits where they belong.
 *	 -ÿCorrected the 'addr' field by changing it from UINT8 to UINT16 for:
 *	   'INC @Rr' 'MOV @Rr,A' 'MOV @Rr,#N' 'XCH A,@Rr' 'XCHD A,@Rr'
 *	 -ÿAdded mask to TIMER when the TEST1 Counter overflows.
 *	 - Seperated the prescaler out of the timer/counter, in order to correct
 *	   the TEST1 input counter step.
 *	 -ÿMoved TEST0 and TEST1 status flags out of the STATE register.
 *	   STATE register uses these upper bits for user definable purposes.
 *	 -ÿTEST0 and TEST1 input lines are now sampled during the JTx/JNTx
 *	   instructions.
 *	 -ÿTwo methods for updating TEST1 input during counter mode are now
 *	   supported depending on the mode of use required.
 *	   You can use the Interrupt method, or input port read method.
 *	 -ÿTIMER is now only controlled by the timer or counter (not both)
 *	   ie, When Starting the Counter, Stop the Timer and viceversa.
 *	 -ÿNested IRQs of any sort are no longer allowed, however IRQs can
 *	   become pending while a current interrupt is being serviced.
 *	 -ÿIBF Interrupt now has priority over the Timer Interrupt, when they
 *	   occur simultaneously.
 *	 -ÿAdd the external Interrupt FLAGS (Port 24, Port 25).
 *	To Do:
 *	 -ÿAdd the external DMA FLAGS (Port 26, Port 27).  Page 4 and 37
 *
 *****************************************************************************/

#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "i8x41.h"

typedef struct {
	UINT16	ppc;
	UINT16	pc;
	UINT8	timer;
	UINT8	prescaler;
	UINT16	subtype;
	UINT8	a;
	UINT8	psw;
	UINT8	state;
	UINT8	enable;
	UINT8	control;
	UINT8	dbbi;
	UINT8	dbbo;
	UINT8	p1;
	UINT8	p2;
	UINT8	p2_hs;
	UINT8	*ram;
	int 	(*irq_callback)(int irqline);
}	I8X41;

int i8x41_ICount;

static I8X41 i8x41;

/* Layout of the registers in the debugger */
static UINT8 i8x41_reg_layout[] = {
	I8X41_PC, I8X41_SP, I8X41_PSW, I8X41_T, I8X41_DATA_DASM, I8X41_CMND_DASM, I8X41_STAT, I8X41_P1, I8X41_P2, -1,
	I8X41_A, I8X41_R0, I8X41_R1, I8X41_R2, I8X41_R3, I8X41_R4, I8X41_R5, I8X41_R6, I8X41_R7, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 i8x41_win_layout[] = {
	 0, 0,80, 2,	/* register window (top rows) */
	 0, 3,24,19,	/* disassembler window (left colums) */
	25, 3,55, 9,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

#define RM(a)	cpu_readmem16(a)
#define WM(a,v) cpu_writemem16(a,v)
#define RP(a)	cpu_readport16(a)
#define WP(a,v) cpu_writeport16(a,v)
#define ROP(pc) cpu_readop(pc)
#define ROP_ARG(pc) cpu_readop_arg(pc)

/* PC vectors */
#define V_RESET 0x000	/* power on address */
#define V_IBF	0x003	/* input buffer full interrupt vector */
#define V_TIMER 0x007	/* timer/counter interrupt vector */

/*
 * Memory locations
 * Note:
 * 000-3ff		internal ROM for 8x41 (1K)
 * 400-7ff		(more) internal for 8x42 type (2K)
 * 800-8ff		internal RAM
 */
#define M_IRAM	0x800	/* internal RAM is mapped here */
#define M_BANK0 0x800	/* register bank 0 (8 times 8 bits) */
#define M_STACK 0x808	/* stack (8 times 16 bits) */
#define M_BANK1 0x818	/* register bank 1 (8 times 8 bits) */
#define M_USER	0x820	/* user memory (224 times 8 bits) */

/* PSW flag bits */
#define FC		0x80	/* carry flag */
#define FA		0x40	/* auxiliary carry flag */
#define Ff0		0x20	/* flag 0 - same flag as F0 below */
#define BS		0x10	/* bank select */
#define FU		0x08	/* unused */
#define SP		0x07	/* lower three bits are used as stack pointer */

/* STATE flag bits */
#define OBF 	0x01	/* output buffer full */
#define IBF 	0x02	/* input buffer full */
#define F0		0x04	/* flag 0 - same flag as Ff0 above */
#define F1		0x08	/* flag 1 */

/* ENABLE flag bits */
#define IBFI	0x01	/* input buffer full interrupt */
#define TCNTI	0x02	/* timer/counter interrupt */
#define DMA 	0x04	/* DMA mode */
#define FLAGS	0x08	/* FLAGS mode */
#define T		0x10	/* timer */
#define CNT 	0x20	/* counter */

/* CONTROL flag bits */
#define IBFI_EXEC	0x01	/* IBFI is currently being serviced */
#define IBFI_PEND	0x02	/* IBFI is pending */
#define TIRQ_EXEC	0x04	/* Timer interrupt is currently being serviced */
#define TIRQ_PEND	0x08	/* Timer interrupt is pending */
#define TEST1		0x10	/* Test1 line mode */
#define TOVF		0x20	/* Timer Overflow Flag */

#define IRQ_EXEC	0x05	/* Mask for IRQs being serviced */
#define IRQ_PEND	0x0a	/* Mask for IRQs pending */


/* shorter names for the I8x41 structure elements */
#define PPC 		i8x41.ppc
#define PC			i8x41.pc
#define A			i8x41.a
#define PSW 		i8x41.psw
#define DBBI		i8x41.dbbi
#define DBBO		i8x41.dbbo
#define R(n)		i8x41.ram[((PSW & BS) ? M_BANK1:M_BANK0)+(n)]
#define STATE		i8x41.state
#define ENABLE		i8x41.enable
#define TIMER		i8x41.timer
#define PRESCALER	i8x41.prescaler
#define P1			i8x41.p1
#define P2			i8x41.p2
#define P2_HS		i8x41.p2_hs		/* Port 2 Hand Shaking */
#define CONTROL		i8x41.control



/************************************************************************
 *	Shortcuts
 ************************************************************************/

static INLINE void PUSH_PC_TO_STACK(void)
{
	WM( M_STACK + (PSW&SP) * 2 + 0, PC & 0xff);
	WM( M_STACK + (PSW&SP) * 2 + 1, ((PC >> 8) & 0x0f) | (PSW & 0xf0) );
	PSW = (PSW & ~SP) | ((PSW + 1) & SP);
}


/************************************************************************
 *	Emulate the Instructions
 ************************************************************************/

/***********************************
 *	illegal opcodes
 ***********************************/
static INLINE void illegal(void)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "i8x41 #%d: illegal opcode at 0x%03x: %02x\n", cpu_getactivecpu(), PC, ROP(PC));
}

/***********************************
 *	0110 1rrr *  ADD	 A,Rr
 ***********************************/
static INLINE void add_r(int r)
{
	UINT8 res = A + R(r);
	if( res < A ) PSW |= FC;
	if( (res & 0x0f) < (A & 0x0f) ) PSW |= FA;
	A = res;
}

/***********************************
 *	0110 000r
 *	ADD 	A,@Rr
 ***********************************/
static INLINE void add_rm(int r)
{
	UINT8 res = A + RM( M_IRAM + (R(r) & I8X42_intRAM_MASK) );
	if( res < A ) PSW |= FC;
	if( (res & 0x0f) < (A & 0x0f) ) PSW |= FA;
	A = res;
}

/***********************************
 *	0000 0011 7654 3210
 *	ADD 	A,#n
 ***********************************/
static INLINE void add_i(void)
{
	UINT8 res = A + ROP_ARG(PC);
	PC++;
	if( res < A ) PSW |= FC;
	if( (res & 0x0f) < (A & 0x0f) ) PSW |= FA;
	A = res;
}

/***********************************
 *	0111 1rrr
 *	ADDC	A,Rr
 ***********************************/
static INLINE void addc_r(int r)
{
	UINT8 res = A + R(r) + (PSW >> 7);
	if( res <= A ) PSW |= FC;
	if( (res & 0x0f) <= (A & 0x0f) ) PSW |= FA;
	A = res;
}

/***********************************
 *	0111 000r
 *	ADDC	A,@Rr
 ***********************************/
static INLINE void addc_rm(int r)
{
	UINT8 res = A + RM( M_IRAM + (R(r) & I8X42_intRAM_MASK) ) + (PSW >> 7);
	if( res <= A ) PSW |= FC;
	if( (res & 0x0f) <= (A & 0x0f) ) PSW |= FA;
	A = res;
}

/***********************************
 *	0001 0011 7654 3210
 *	ADDC	A,#n
 ***********************************/
static INLINE void addc_i(void)
{
	UINT8 res = A + ROP_ARG(PC);
	PC++;
	if( res < A ) PSW |= FC;
	if( (res & 0x0f) < (A & 0x0f) ) PSW |= FA;
	A = res;
}

/***********************************
 *	0101 1rrr
 *	ANL 	A,Rr
 ***********************************/
static INLINE void anl_r(int r)
{
	A = A & R(r);
}

/***********************************
 *	0101 000r
 *	ANL 	A,@Rr
 ***********************************/
static INLINE void anl_rm(int r)
{
	A = A & RM( M_IRAM + (R(r) & I8X42_intRAM_MASK) );
}

/***********************************
 *	0101 0011 7654 3210
 *	ANL 	A,#n
 ***********************************/
static INLINE void anl_i(void)
{
	A = A & ROP_ARG(PC);
	PC++;
}

/***********************************
 *	1001 10pp 7654 3210
 *	ANL 	Pp,#n
 ***********************************/
static INLINE void anl_p_i(int p)
{
	UINT8 val = ROP_ARG(PC);
	PC++;
	/* changed to latched port scheme */
	switch (p)
	{
		case 00:	break;	/* invalid port */
		case 01:	P1 &= val; WP(p, P1); break;
		case 02:	P2 &= val; WP(p, (P2 & P2_HS) ); break;
		case 03:	break;	/* invalid port */
		default:	break;
	}
}

/***********************************
 *	1001 11pp 7654 3210
 *	ANLD	Pp,A
 ***********************************/
static INLINE void anld_p_a(int p)
{
	/* added proper expanded port setup */
	WP(2, (P2 & 0xf0) | 0x0c | p); /* AND mode */
	WP(I8X41_ps, 0);	/* activate command strobe */
	WP(2, (A & 0x0f)); 	/* Expander to take care of AND function */
	WP(I8X41_ps, 1);	/* release command strobe */
}

/***********************************
 *	aaa1 0100 7654 3210
 *	CALL	addr
 ***********************************/
static INLINE void call_i(int page)
{
	UINT8 adr = ROP_ARG(PC);
	PC++;
	PUSH_PC_TO_STACK();
	PC = page | adr;
}

/***********************************
 *	0010 0111
 *	CLR 	A
 ***********************************/
static INLINE void clr_a(void)
{
	A = 0;
}

/***********************************
 *	1001 0111
 *	CLR 	C
 ***********************************/
static INLINE void clr_c(void)
{
	PSW &= ~FC;
}

/***********************************
 *	1000 0101
 *	CLR 	F0
 ***********************************/
static INLINE void clr_f0(void)
{
	PSW &= ~Ff0;
	STATE &= ~F0;
}

/***********************************
 *	1010 0101
 *	CLR 	F1
 ***********************************/
static INLINE void clr_f1(void)
{
	STATE &= ~F1;
}

/***********************************
 *	0011 0111
 *	CPL 	A
 ***********************************/
static INLINE void cpl_a(void)
{
	A = ~A;
}

/***********************************
 *	1010 0111
 *	CPL 	C
 ***********************************/
static INLINE void cpl_c(void)
{
	PSW ^= FC;
}

/***********************************
 *	1001 0101
 *	CPL 	F0
 ***********************************/
static INLINE void cpl_f0(void)
{
	PSW ^= Ff0;
	STATE ^= F0;
}

/***********************************
 *	1011 0101
 *	CPL 	F1
 ***********************************/
static INLINE void cpl_f1(void)
{
	STATE ^= F1;
}

/***********************************
 *	0101 0111
 *	DA		A
 ***********************************/
static INLINE void da_a(void)
{
	UINT8 res = A + ((PSW & FA) || ((A & 0x0f) > 0x09)) ? 0x06 : 0x00;
	if( (PSW & FC) || ((res & 0xf0) > 0x90) )
		res += 0x60;
	if( res < A )
		PSW |= FC;
	else
		PSW &= ~FC;
	A = res;
}

/***********************************
 *	0000 0111
 *	DEC 	A
 ***********************************/
static INLINE void dec_a(void)
{
	A -= 1;
}

/***********************************
 *	1100 1rrr
 *	DEC 	Rr
 ***********************************/
static INLINE void dec_r(int r)
{
	R(r) -= 1;
}

/***********************************
 *	0001 0101
 *	DIS 	I
 ***********************************/
static INLINE void dis_i(void)
{
	ENABLE &= ~IBFI;	/* disable input buffer full interrupt */
}

/***********************************
 *	0011 0101
 *	DIS 	TCNTI
 ***********************************/
static INLINE void dis_tcnti(void)
{
	ENABLE &= ~TCNTI;	/* disable timer/counter interrupt */
}

/***********************************
 *	0111 1rrr 7654 3210
 *	DJNZ	Rr,addr
 ***********************************/
static INLINE void djnz_r_i(int r)
{
	UINT8 adr = ROP_ARG(PC);
	PC++;
	R(r) -= 1;
	if( R(r) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1110 0101
 *	EN		DMA
 ***********************************/
static INLINE void en_dma(void)
{
	ENABLE |= DMA;		/* enable DMA handshake lines */
	P2_HS &= 0xbf;
	WP(0x02, (P2 & P2_HS) );
}

/***********************************
 *	1111 0101
 *	EN		FLAGS
 ***********************************/
static INLINE void en_flags(void)
{
	if( 0 == (ENABLE & FLAGS) )
	{
		/* Configure upper lines on Port 2 for IRQ handshaking (P24 and P25) */

		ENABLE |= FLAGS;
		if( STATE & OBF ) P2_HS |= 0x10;
		else P2_HS &= 0xef;
		if( STATE & IBF ) P2_HS |= 0x20;
		else P2_HS &= 0xdf;
		WP(0x02, (P2 & P2_HS) );
	}
}

/***********************************
 *	0000 0101
 *	EN		I
 ***********************************/
static INLINE void en_i(void)
{
	if( 0 == (ENABLE & IBFI) )
	{
		ENABLE |= IBFI;		/* enable input buffer full interrupt */
		if( STATE & IBF )	/* already got data in the buffer? */
			i8x41_set_irq_line(I8X41_INT_IBF, HOLD_LINE);
	}
}

/***********************************
 *	0010 0101
 *	EN		TCNTI
 ***********************************/
static INLINE void en_tcnti(void)
{
	ENABLE |= TCNTI;	/* enable timer/counter interrupt */
}

/***********************************
 *	0010 0010
 *	IN		A,DBB
 ***********************************/
static INLINE void in_a_dbb(void)
{
	if( i8x41.irq_callback )
		(*i8x41.irq_callback)(I8X41_INT_IBF);

	STATE &= ~IBF;					/* clear input buffer full flag */
	if( ENABLE & FLAGS )
	{
		P2_HS &= 0xdf;
		if( STATE & OBF ) P2_HS |= 0x10;
		else P2_HS &= 0xef;
		WP(0x02, (P2 & P2_HS) );	/* Clear the DBBI IRQ out on P25 */
	}
	A = DBBI;
}

/***********************************
 *	0000 10pp
 *	IN		A,Pp
 ***********************************/
static INLINE void in_a_p(int p)
{
	/* changed to latched port scheme */
	switch( p )
	{
		case 00:	break;	/* invalid port */
		case 01:	A = (RP(p) & P1); break;
		case 02:	A = (RP(p) & P2); break;
		case 03:	break;	/* invalid port */
		default:	break;
	}
}

/***********************************
 *	0001 0111
 *	INC 	A
 ***********************************/
static INLINE void inc_a(void)
{
	A += 1;
}

/***********************************
 *	0001 1rrr
 *	INC 	Rr
 ***********************************/
static INLINE void inc_r(int r)
{
	R(r) += 1;
}

/***********************************
 *	0001 000r
 *	INC  @	Rr
 ***********************************/
static INLINE void inc_rm(int r)
{
	UINT16 addr = M_IRAM + (R(r) & I8X42_intRAM_MASK);
	WM( addr, RM(addr) + 1 );
}

/***********************************
 *	bbb1 0010
 *	JBb 	addr
 ***********************************/
static INLINE void jbb_i(int bit)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( A & (1 << bit) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1111 0110
 *	JC		addr
 ***********************************/
static INLINE void jc_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( PSW & FC )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1011 0110
 *	JF0 	addr
 ***********************************/
static INLINE void jf0_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( STATE & F0 )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	0111 0110
 *	JF1 	addr
 ***********************************/
static INLINE void jf1_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( STATE & F1 )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	aaa0 0100
 *	JMP 	addr
 ***********************************/
static INLINE void jmp_i(int page)
{
	/* err.. do we have 10 or 11 PC bits?
	 * CALL is said to use 0aa1 (4 pages)
	 * JMP is said to use aaa0 (8 pages)
	 */
	UINT8 adr = ROP_ARG(PC);
	PC = page | adr;
}

/***********************************
 *	1011 0011
 *	JMP  @	A
 ***********************************/
static INLINE void jmpp_a(void)
{
	UINT16 adr = (PC & 0x700) | A;
	PC = (PC & 0x700) | RM(adr);
}

/***********************************
 *	1110 0110
 *	JNC 	addr
 ***********************************/
static INLINE void jnc_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( !(PSW & FC) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1101 0110
 *	JNIBF	addr
 ***********************************/
static INLINE void jnibf_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( 0 == (STATE & IBF) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	0010 0110
 *	JNT0	addr
 ***********************************/
static INLINE void jnt0_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( 0 == RP(I8X41_t0) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	0100 0110
 *	JNT1	addr
 ***********************************/
static INLINE void jnt1_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( !(ENABLE & CNT) )
	{
		UINT8 level = RP(I8X41_t1);
		if( level ) CONTROL |= TEST1;
		else CONTROL &= ~TEST1;
	}
	if( !(CONTROL & TEST1) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1001 0110
 *	JNZ 	addr
 ***********************************/
static INLINE void jnz_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( A )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1000 0110
 *	JOBF	addr
 ***********************************/
static INLINE void jobf_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( STATE & OBF )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	0001 0110
 *	JTF 	addr
 ***********************************/
static INLINE void jtf_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( CONTROL & TOVF )
		PC = (PC & 0x700) | adr;
	CONTROL &= ~TOVF;
}

/***********************************
 *	0011 0110
 *	JT0 	addr
 ***********************************/
static INLINE void jt0_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( RP(I8X41_t0) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	0101 0110
 *	JT1 	addr
 ***********************************/
static INLINE void jt1_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( !(ENABLE & CNT) )
	{
		UINT8 level = RP(I8X41_t1);
		if( level ) CONTROL |= TEST1;
		else CONTROL &= ~TEST1;
	}
	if( (CONTROL & TEST1) )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	1100 0110
 *	JZ		addr
 ***********************************/
static INLINE void jz_i(void)
{
	UINT8 adr = ROP_ARG(PC);
	PC += 1;
	if( !A )
		PC = (PC & 0x700) | adr;
}

/***********************************
 *	0010 0011
 *	MOV 	A,#n
 ***********************************/
static INLINE void mov_a_i(void)
{
	A = ROP(PC);
	PC += 1;
}

/***********************************
 *	1100 0111
 *	MOV 	A,PSW
 ***********************************/
static INLINE void mov_a_psw(void)
{
	A = PSW;
}

/***********************************
 *	1111 1rrr
 *	MOV 	A,Rr
 ***********************************/
static INLINE void mov_a_r(int r)
{
	A = R(r);
}

/***********************************
 *	1111 000r
 *	MOV 	A,Rr
 ***********************************/
static INLINE void mov_a_rm(int r)
{
	A = RM( M_IRAM + (R(r) & I8X42_intRAM_MASK) );
}

/***********************************
 *	0100 0010
 *	MOV 	A,T
 ***********************************/
static INLINE void mov_a_t(void)
{
	A = TIMER;
}

/***********************************
 *	1101 0111
 *	MOV 	PSW,A
 ***********************************/
static INLINE void mov_psw_a(void)
{
	PSW = A;
}

/***********************************
 *	1010 1rrr
 *	MOV 	Rr,A
 ***********************************/
static INLINE void mov_r_a(int r)
{
	R(r) = A;
}

/***********************************
 *	1011 1rrr
 *	MOV 	Rr,#n
 ***********************************/
static INLINE void mov_r_i(int r)
{
	UINT8 val = ROP_ARG(PC);
	PC += 1;
	R(r) = val;
}

/***********************************
 *	1010 000r
 *	MOV 	@Rr,A
 ***********************************/
static INLINE void mov_rm_a(int r)
{
	WM( M_IRAM + (R(r) & I8X42_intRAM_MASK), A );
}

/***********************************
 *	1011 000r
 *	MOV 	@Rr,#n
 ***********************************/
static INLINE void mov_rm_i(int r)
{
	UINT8 val = ROP_ARG(PC);
	PC += 1;
	WM( M_IRAM + (R(r) & I8X42_intRAM_MASK), val );
}

/***********************************
 *	1001 0000
 *	MOV 	STS,A
 ***********************************/
static INLINE void mov_sts_a(void)
{
	STATE = (STATE & 0x0f) | (A & 0xf0);
}

/***********************************
 *	0110 0010
 *	MOV 	T,A
 ***********************************/
static INLINE void mov_t_a(void)
{
	TIMER = A;
}

/***********************************
 *	0000 11pp
 *	MOVD	A,Pp
 ***********************************/
static INLINE void movd_a_p(int p)
{
	/* added proper expanded port setup */
	WP(2, (P2 & 0xf0) | 0x00 | p);	/* READ mode */
	WP(I8X41_ps, 0);		/* activate command strobe */
	A = RP(2) & 0xf;
	WP(I8X41_ps, 1);		/* release command strobe */
}

/***********************************
 *	0011 11pp
 *	MOVD	Pp,A
 ***********************************/
static INLINE void movd_p_a(int p)
{
	/* added proper expanded port setup */
	WP(2, (P2 & 0xf0) | 0x04 | p);	/* WRITE mode */
	WP(I8X41_ps, 0);		/* activate command strobe */
	WP(2, A & 0x0f);
	WP(I8X41_ps, 1);		/* release command strobe */
}

/***********************************
 *	1010 0011
 *	MOVP	A,@A
 ***********************************/
static INLINE void movp_a_am(void)
{
	UINT16 addr = (PC & 0x700) | A;
	A = RM(addr);
}

/***********************************
 *	1110 0011
 *	MOVP3	A,@A
 ***********************************/
static INLINE void movp3_a_am(void)
{
	UINT16 addr = 0x300 | A;
	A = RM(addr);
}

/***********************************
 *	0000 0000
 *	NOP
 ***********************************/
static INLINE void nop(void)
{
}

/***********************************
 *	0100 1rrr
 *	ORL 	A,Rr
 ***********************************/
static INLINE void orl_r(int r)
{
	A = A | R(r);
}

/***********************************
 *	0100 000r
 *	ORL 	A,@Rr
 ***********************************/
static INLINE void orl_rm(int r)
{
	A = A | RM( M_IRAM + (R(r) & I8X42_intRAM_MASK) );
}

/***********************************
 *	0100 0011 7654 3210
 *	ORL 	A,#n
 ***********************************/
static INLINE void orl_i(void)
{
	UINT8 val = ROP_ARG(PC);
	PC++;
	A = A | val;
}

/***********************************
 *	1000 10pp 7654 3210
 *	ORL 	Pp,#n
 ***********************************/
static INLINE void orl_p_i(int p)
{
	UINT8 val = ROP_ARG(PC);
	PC++;
	/* changed to latched port scheme */
	switch (p)
	{
		case 00:	break;	/* invalid port */
		case 01:	P1 |= val; WP(p, P1); break;
		case 02:	P2 |= val; WP(p, P2); break;
		case 03:	break;	/* invalid port */
		default:	break;
	}
}

/***********************************
 *	1000 11pp 7654 3210
 *	ORLD	Pp,A
 ***********************************/
static INLINE void orld_p_a(int p)
{
	/* added proper expanded port setup */
	WP(2, (P2 & 0xf0) | 0x08 | p);	/* OR mode */
	WP(I8X41_ps, 0);	/* activate command strobe */
	WP(2, A & 0x0f);	/* Expander to take care of OR function */
	WP(I8X41_ps, 1);	/* release command strobe */
}

/***********************************
 *	0000 0010
 *	OUT 	DBB,A
 ***********************************/
static INLINE void out_dbb_a(void)
{
	DBBO = A;			/* DBB output buffer */
	STATE |= OBF;		/* assert the output buffer full flag */
	if( ENABLE & FLAGS )
	{
		P2_HS |= 0x10;
		if( STATE & IBF ) P2_HS |= 0x20;
		else P2_HS &= 0xdf;
		WP(0x02, (P2 & P2_HS) );	/* Assert the DBBO IRQ out on P24 */
	}
}

/***********************************
 *	0011 10pp
 *	OUT 	Pp,A
 ***********************************/
static INLINE void out_p_a(int p)
{
	/* changed to latched port scheme */
	switch (p)
	{
		case 00:	break;	/* invalid port */
		case 01:	WP(p, A); P1 = A; break;
		case 02:	WP(p, A); P2 = A; break;
		case 03:	break;	/* invalid port */
		default:	break;
	}
}

/***********************************
 *	1000 0011
 *	RET
 ***********************************/
static INLINE void ret(void)
{
	UINT8 msb;
	PSW = (PSW & ~SP) | ((PSW - 1) & SP);
	msb = RM(M_STACK + (PSW&SP) * 2 + 1);
	PC = RM(M_STACK + (PSW&SP) * 2 + 0);
	PC |= (msb << 8) & 0x700;
}

/***********************************
 *	1001 0011
 *	RETR
 ***********************************/
static INLINE void retr(void)
{
	UINT8 msb;
	PSW = (PSW & ~SP) | ((PSW - 1) & SP);
	msb = RM(M_STACK + (PSW&SP) * 2 + 1);
	PC = RM(M_STACK + (PSW&SP) * 2 + 0);
	PC |= (msb << 8) & 0x700;
	PSW = (PSW & 0x0f) | (msb & 0xf0);
	CONTROL &= ~IBFI_EXEC;
	CONTROL &= ~TIRQ_EXEC;
}

/***********************************
 *	1110 0111
 *	RL		A
 ***********************************/
static INLINE void rl_a(void)
{
	A = (A << 1) | (A >> 7);
}

/***********************************
 *	1111 0111
 *	RLC 	A
 ***********************************/
static INLINE void rlc_a(void)
{
	UINT8 c = PSW >> 7;
	PSW = (PSW & ~FC) | (A >> 7);
	A = (A << 1) | c;
}

/***********************************
 *	0111 0111
 *	RR		A
 ***********************************/
static INLINE void rr_a(void)
{
	A = (A >> 1) | (A << 7);
}

/***********************************
 *	0110 0111
 *	RRC 	A
 ***********************************/
static INLINE void rrc_a(void)
{
	UINT8 c = PSW & 0x80;
	PSW = (PSW & ~FC) | (A << 7);
	A = (A >> 1) | c;
}

/***********************************
 *	1100 0101
 *	SEL 	RB0
 ***********************************/
static INLINE void sel_rb0(void)
{
	PSW &= ~BS;
}

/***********************************
 *	1101 0101
 *	SEL 	RB1
 ***********************************/
static INLINE void sel_rb1(void)
{
	PSW |= BS;
}

/***********************************
 *	0110 0101
 *	STOP	TCNT
 ***********************************/
static INLINE void stop_tcnt(void)
{
	ENABLE &= ~(T|CNT);
}

/***********************************
 *	0100 0101
 *	STRT	CNT
 ***********************************/
static INLINE void strt_cnt(void)
{
	ENABLE |= CNT;
	ENABLE &= ~T;
}

/***********************************
 *	0101 0101
 *	STRT	T
 ***********************************/
static INLINE void strt_t(void)
{
	ENABLE |= T;
	ENABLE &= ~CNT;
}

/***********************************
 *	0100 0111
 *	SWAP	A
 ***********************************/
static INLINE void swap_a(void)
{
	A = (A << 4) | (A >> 4);
}

/***********************************
 *	0010 1rrr
 *	XCH 	A,Rr
 ***********************************/
static INLINE void xch_a_r(int r)
{
	UINT8 tmp = R(r);
	R(r) = A;
	A = tmp;
}

/***********************************
 *	0010 000r
 *	XCH 	A,@Rr
 ***********************************/
static INLINE void xch_a_rm(int r)
{
	UINT16 addr = M_IRAM + (R(r) & I8X42_intRAM_MASK);
	UINT8 tmp = RM(addr);
	WM( addr, A );
	A = tmp;
}

/***********************************
 *	0011 000r
 *	XCHD	A,@Rr
 ***********************************/
static INLINE void xchd_a_rm(int r)
{
	UINT16 addr = M_IRAM + (R(r) & I8X42_intRAM_MASK);
	UINT8 tmp = RM(addr);
	WM( addr, (tmp & 0xf0) | (A & 0x0f) );
	A = (A & 0xf0) | (tmp & 0x0f);
}

/***********************************
 *	1101 1rrr
 *	XRL 	A,Rr
 ***********************************/
static INLINE void xrl_r(int r)
{
	A = A ^ R(r);
}

/***********************************
 *	1101 000r
 *	XRL 	A,@Rr
 ***********************************/
static INLINE void xrl_rm(int r)
{
	A = A ^ RM( M_IRAM + (R(r) & I8X42_intRAM_MASK) );
}

/***********************************
 *	1101 0011 7654 3210
 *	XRL 	A,#n
 ***********************************/
static INLINE void xrl_i(void)
{
	UINT8 val = ROP_ARG(PC);
	PC++;
	A = A ^ val;
}


/***********************************************************************
 *	Cycle Timings
 ***********************************************************************/

static UINT8 i8x41_cycles[] = {
	1,1,1,2,2,1,1,1,2,2,2,2,2,2,2,2,
	1,1,2,2,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,1,2,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,2,1,2,1,2,1,2,2,2,2,2,2,2,2,
	1,1,1,2,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,2,2,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,
	1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,1,2,2,1,2,1,2,2,2,2,2,2,2,2,
	1,1,2,2,1,1,2,1,2,2,2,2,2,2,2,2,
	1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,1,2,1,2,2,2,2,2,2,2,2,
	1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,2,1,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,1,2,2,1,2,1,1,1,1,1,1,1,1,1,
	1,1,2,1,2,1,2,1,2,2,2,2,2,2,2,2
};


/****************************************************************************
 *	Inits CPU emulation
 ****************************************************************************/

void i8x41_init(void)
{
	int cpu = cpu_getactivecpu();
	state_save_register_UINT16("i8x41", cpu, "PPC",       &i8x41.ppc,      1);
	state_save_register_UINT16("i8x41", cpu, "PC",        &i8x41.pc,       1);
	state_save_register_UINT8 ("i8x41", cpu, "TIMER",     &i8x41.timer,    1);
	state_save_register_UINT8 ("i8x41", cpu, "PRESCALER", &i8x41.prescaler,1);
	state_save_register_UINT16("i8x41", cpu, "SUBTYPE",   &i8x41.subtype,  1);
	state_save_register_UINT8 ("i8x41", cpu, "A",         &i8x41.a,        1);
	state_save_register_UINT8 ("i8x41", cpu, "PSW",       &i8x41.psw,      1);
	state_save_register_UINT8 ("i8x41", cpu, "STATE",     &i8x41.state,    1);
	state_save_register_UINT8 ("i8x41", cpu, "ENABLE",    &i8x41.enable,   1);
	state_save_register_UINT8 ("i8x41", cpu, "CONTROL",   &i8x41.control,  1);
	state_save_register_UINT8 ("i8x41", cpu, "DBBI",      &i8x41.dbbi,     1);
	state_save_register_UINT8 ("i8x41", cpu, "DBBO",      &i8x41.dbbo,     1);
	state_save_register_UINT8 ("i8x41", cpu, "P1",        &i8x41.p1,       1);
	state_save_register_UINT8 ("i8x41", cpu, "P2",        &i8x41.p2,       1);
	state_save_register_UINT8 ("i8x41", cpu, "P2_HS",     &i8x41.p2_hs,    1);
}


/****************************************************************************
 *	Reset registers to their initial values
 ****************************************************************************/

void i8x41_reset(void *param)
{
	memset(&i8x41, 0, sizeof(I8X41));
	/* default to 8041 behaviour for DBBI/DBBO and extended commands */
	i8x41.subtype = 8041;
	/* ugly hack.. excuse my lazyness */
	i8x41.ram = memory_region(REGION_CPU1 + cpu_getactivecpu());
	ENABLE = IBFI | TCNTI;
	DBBI = 0xff;
	DBBO = 0xff;
	/* Set Ports 1 and 2 to input mode */
	P1   = 0xff;
	P2   = 0xff;
	P2_HS= 0xff;
}


/****************************************************************************
 *	Shut down CPU emulation
 ****************************************************************************/

void i8x41_exit(void)
{
	/* nothing to do */
}


/****************************************************************************
 *	Execute cycles - returns number of cycles actually run
 ****************************************************************************/

int i8x41_execute(int cycles)
{
	int inst_cycles, T1_level;

	i8x41_ICount = cycles;

	do
	{
		UINT8 op = cpu_readop(PC);

		PPC = PC;

		CALL_MAME_DEBUG;

		PC += 1;
		i8x41_ICount -= i8x41_cycles[op];

		switch( op )
		{
		/* opcode cycles bitmask */
		case 0x00: /* 1: 0000 0000 */
			nop();
			break;
		case 0x01: /* 1: 0000 0001 */
			illegal();
			break;
		case 0x02: /* 1: 0000 0010 */
			out_dbb_a();
			break;
		case 0x03: /* 2: 0000 0011 */
			add_i();
			break;
		case 0x04: /* 2: aaa0 0100 */
			jmp_i(0x000);
			break;
		case 0x05: /* 1: 0000 0101 */
			en_i();
			break;
		case 0x06: /* 1: 0000 0110 */
			illegal();
			break;
		case 0x07: /* 1: 0000 0111 */
			dec_a();
			break;
		case 0x08: /* 2: 0000 10pp */
		case 0x09: /* 2: 0000 10pp */
		case 0x0a: /* 2: 0000 10pp */
		case 0x0b: /* 2: 0000 10pp */
			in_a_p(op & 3);
			break;
		case 0x0c: /* 2: 0000 11pp */
		case 0x0d: /* 2: 0000 11pp */
		case 0x0e: /* 2: 0000 11pp */
		case 0x0f: /* 2: 0000 11pp */
			movd_a_p(op & 3);
			break;
		case 0x10: /* 1: 0001 000r */
			inc_rm(0);
			break;
		case 0x11: /* 1: 0001 000r */
			inc_rm(1);
			break;
		case 0x12: /* 2: bbb1 0010 */
			jbb_i(0);
			break;
		case 0x13: /* 2: 0001 0011 */
			addc_i();
			break;
		case 0x14: /* 2: aaa1 0100 */
			call_i(0x000);
			break;
		case 0x15: /* 1: 0001 0101 */
			dis_i();
			break;
		case 0x16: /* 2: 0001 0110 */
			jtf_i();
			break;
		case 0x17: /* 1: 0001 0111 */
			inc_a();
			break;
		case 0x18: /* 1: 0001 1rrr */
		case 0x19: /* 1: 0001 1rrr */
		case 0x1a: /* 1: 0001 1rrr */
		case 0x1b: /* 1: 0001 1rrr */
		case 0x1c: /* 1: 0001 1rrr */
		case 0x1d: /* 1: 0001 1rrr */
		case 0x1e: /* 1: 0001 1rrr */
		case 0x1f: /* 1: 0001 1rrr */
			inc_r(op & 7);
			break;
		case 0x20: /* 1: 0010 000r */
			xch_a_rm(0);
			break;
		case 0x21: /* 1: 0010 000r */
			xch_a_rm(1);
			break;
		case 0x22: /* 1: 0010 0010 */
			in_a_dbb();
			break;
		case 0x23: /* 2: 0010 0011 */
			mov_a_i();
			break;
		case 0x24: /* 2: aaa0 0100 */
			jmp_i(0x100);
			break;
		case 0x25: /* 1: 0010 0101 */
			en_tcnti();
			break;
		case 0x26: /* 2: 0010 0110 */
			jnt0_i();
			break;
		case 0x27: /* 1: 0010 0111 */
			clr_a();
			break;
		case 0x28: /* 1: 0010 1rrr */
		case 0x29: /* 1: 0010 1rrr */
		case 0x2a: /* 1: 0010 1rrr */
		case 0x2b: /* 1: 0010 1rrr */
		case 0x2c: /* 1: 0010 1rrr */
		case 0x2d: /* 1: 0010 1rrr */
		case 0x2e: /* 1: 0010 1rrr */
		case 0x2f: /* 1: 0010 1rrr */
			xch_a_r(op & 7);
			break;
		case 0x30: /* 1: 0011 000r */
			xchd_a_rm(0);
			break;
		case 0x31: /* 1: 0011 000r */
			xchd_a_rm(1);
			break;
		case 0x32: /* 2: bbb1 0010 */
			jbb_i(1);
			break;
		case 0x33: /* 1: 0011 0101 */
			illegal();
			break;
		case 0x34: /* 2: aaa1 0100 */
			call_i(0x100);
			break;
		case 0x35: /* 1: 0000 0101 */
			dis_tcnti();
			break;
		case 0x36: /* 2: 0011 0110 */
			jt0_i();
			break;
		case 0x37: /* 1: 0011 0111 */
			cpl_a();
			break;
		case 0x38: /* 2: 0011 10pp */
		case 0x39: /* 2: 0011 10pp */
		case 0x3a: /* 2: 0011 10pp */
		case 0x3b: /* 2: 0011 10pp */
			out_p_a(op & 3);
			break;
		case 0x3c: /* 2: 0011 11pp */
		case 0x3d: /* 2: 0011 11pp */
		case 0x3e: /* 2: 0011 11pp */
		case 0x3f: /* 2: 0011 11pp */
			movd_p_a(op & 3);
			break;
		case 0x40: /* 1: 0100 000r */
			orl_rm(0);
			break;
		case 0x41: /* 1: 0100 000r */
			orl_rm(1);
			break;
		case 0x42: /* 1: 0100 0010 */
			mov_a_t();
			break;
		case 0x43: /* 2: 0100 0011 */
			orl_i();
			break;
		case 0x44: /* 2: aaa0 0100 */
			jmp_i(0x200);
			break;
		case 0x45: /* 1: 0100 0101 */
			strt_cnt();
			break;
		case 0x46: /* 2: 0100 0110 */
			jnt1_i();
			break;
		case 0x47: /* 1: 0100 0111 */
			swap_a();
			break;
		case 0x48: /* 1: 0100 1rrr */
		case 0x49: /* 1: 0100 1rrr */
		case 0x4a: /* 1: 0100 1rrr */
		case 0x4b: /* 1: 0100 1rrr */
		case 0x4c: /* 1: 0100 1rrr */
		case 0x4d: /* 1: 0100 1rrr */
		case 0x4e: /* 1: 0100 1rrr */
		case 0x4f: /* 1: 0100 1rrr */
			orl_r(op & 7);
			break;
		case 0x50: /* 1: 0101 000r */
			anl_rm(0);
			break;
		case 0x51: /* 1: 0101 000r */
			anl_rm(1);
			break;
		case 0x52: /* 2: bbb1 0010 */
			jbb_i(2);
			break;
		case 0x53: /* 2: 0101 0011 */
			anl_i();
			break;
		case 0x54: /* 2: aaa1 0100 */
			call_i(0x200);
			break;
		case 0x55: /* 1: 0101 0101 */
			strt_t();
			break;
		case 0x56: /* 2: 0101 0110 */
			jt1_i();
			break;
		case 0x57: /* 1: 0101 0111 */
			da_a();
			break;
		case 0x58: /* 1: 0101 1rrr */
		case 0x59: /* 1: 0101 1rrr */
		case 0x5a: /* 1: 0101 1rrr */
		case 0x5b: /* 1: 0101 1rrr */
		case 0x5c: /* 1: 0101 1rrr */
		case 0x5d: /* 1: 0101 1rrr */
		case 0x5e: /* 1: 0101 1rrr */
		case 0x5f: /* 1: 0101 1rrr */
			anl_r(op & 7);
			break;
		case 0x60: /* 1: 0110 000r */
			add_rm(0);
			break;
		case 0x61: /* 1: 0110 000r */
			add_rm(1);
			break;
		case 0x62: /* 1: 0110 0010 */
			mov_t_a();
			break;
		case 0x63: /* 1: 0110 0011 */
			illegal();
			break;
		case 0x64: /* 2: aaa0 0100 */
			jmp_i(0x300);
			break;
		case 0x65: /* 1: 0110 0101 */
			stop_tcnt();
			break;
		case 0x66: /* 1: 0110 0110 */
			illegal();
			break;
		case 0x67: /* 1: 0110 0111 */
			rrc_a();
			break;
		case 0x68: /* 1: 0110 1rrr */
		case 0x69: /* 1: 0110 1rrr */
		case 0x6a: /* 1: 0110 1rrr */
		case 0x6b: /* 1: 0110 1rrr */
		case 0x6c: /* 1: 0110 1rrr */
		case 0x6d: /* 1: 0110 1rrr */
		case 0x6e: /* 1: 0110 1rrr */
		case 0x6f: /* 1: 0110 1rrr */
			add_r(op & 7);
			break;
		case 0x70: /* 1: 0111 000r */
			addc_rm(0);
			break;
		case 0x71: /* 1: 0111 000r */
			addc_rm(1);
			break;
		case 0x72: /* 2: bbb1 0010 */
			jbb_i(3);
			break;
		case 0x73: /* 1: 0111 0011 */
			illegal();
			break;
		case 0x74: /* 2: aaa1 0100 */
			call_i(0x300);
			break;
		case 0x75: /* 1: 0111 0101 */
			illegal();
			break;
		case 0x76: /* 2: 0111 0110 */
			jf1_i();
			break;
		case 0x77: /* 1: 0111 0111 */
			rr_a();
			break;
		case 0x78: /* 1: 0111 1rrr */
		case 0x79: /* 1: 0111 1rrr */
		case 0x7a: /* 1: 0111 1rrr */
		case 0x7b: /* 1: 0111 1rrr */
		case 0x7c: /* 1: 0111 1rrr */
		case 0x7d: /* 1: 0111 1rrr */
		case 0x7e: /* 1: 0111 1rrr */
		case 0x7f: /* 1: 0111 1rrr */
			addc_r(op & 7);
			break;
		case 0x80: /* 1: 1000 0000 */
			illegal();
			break;
		case 0x81: /* 1: 1000 0001 */
			illegal();
			break;
		case 0x82: /* 1: 1000 0010 */
			illegal();
			break;
		case 0x83: /* 2: 1000 0011 */
			ret();
			break;
		case 0x84: /* 2: aaa0 0100 */
			jmp_i(0x400);
			break;
		case 0x85: /* 1: 1000 0101 */
			clr_f0();
			break;
		case 0x86: /* 2: 1000 0110 */
			jobf_i();
			break;
		case 0x87: /* 1: 1000 0111 */
			illegal();
			break;
		case 0x88: /* 2: 1000 10pp */
		case 0x89: /* 2: 1000 10pp */
		case 0x8a: /* 2: 1000 10pp */
		case 0x8b: /* 2: 1000 10pp */
			orl_p_i(op & 3);
			break;
		case 0x8c: /* 2: 1000 11pp */
		case 0x8d: /* 2: 1000 11pp */
		case 0x8e: /* 2: 1000 11pp */
		case 0x8f: /* 2: 1000 11pp */
			orld_p_a(op & 7);
			break;
		case 0x90: /* 1: 1001 0000 */
			mov_sts_a();
			break;
		case 0x91: /* 1: 1001 0001 */
			illegal();
			break;
		case 0x92: /* 2: bbb1 0010 */
			jbb_i(4);
			break;
		case 0x93: /* 2: 1001 0011 */
			retr();
			break;
		case 0x94: /* 1: aaa1 0100 */
			call_i(0x400);
			break;
		case 0x95: /* 1: 1001 0101 */
			cpl_f0();
			break;
		case 0x96: /* 2: 1001 0110 */
			jnz_i();
			break;
		case 0x97: /* 1: 1001 0111 */
			clr_c();
			break;
		case 0x98: /* 2: 1001 10pp , illegal port */
		case 0x99: /* 2: 1001 10pp */
		case 0x9a: /* 2: 1001 10pp */
		case 0x9b: /* 2: 1001 10pp , illegal port */
			anl_p_i(op & 3);
			break;
		case 0x9c: /* 2: 1001 11pp */
		case 0x9d: /* 2: 1001 11pp */
		case 0x9e: /* 2: 1001 11pp */
		case 0x9f: /* 2: 1001 11pp */
			anld_p_a(op & 7);
			break;
		case 0xa0: /* 1: 1010 000r */
			mov_rm_a(0);
			break;
		case 0xa1: /* 1: 1010 000r */
			mov_rm_a(1);
			break;
		case 0xa2: /* 1: 1010 0010 */
			illegal();
			break;
		case 0xa3: /* 2: 1010 0011 */
			movp_a_am();
			break;
		case 0xa4: /* 2: aaa0 0100 */
			jmp_i(0x500);
			break;
		case 0xa5: /* 1: 1010 0101 */
			clr_f1();
			break;
		case 0xa6: /* 1: 1010 0110 */
			illegal();
			break;
		case 0xa7: /* 1: 1010 0111 */
			cpl_c();
			break;
		case 0xa8: /* 1: 1010 1rrr */
		case 0xa9: /* 1: 1010 1rrr */
		case 0xaa: /* 1: 1010 1rrr */
		case 0xab: /* 1: 1010 1rrr */
		case 0xac: /* 1: 1010 1rrr */
		case 0xad: /* 1: 1010 1rrr */
		case 0xae: /* 1: 1010 1rrr */
		case 0xaf: /* 1: 1010 1rrr */
			mov_r_a(op & 7);
			break;
		case 0xb0: /* 2: 1011 000r */
			mov_rm_i(0);
			break;
		case 0xb1: /* 2: 1011 000r */
			mov_rm_i(1);
			break;
		case 0xb2: /* 2: bbb1 0010 */
			jbb_i(5);
			break;
		case 0xb3: /* 2: 1011 0011 */
			jmpp_a();
			break;
		case 0xb4: /* 2: aaa1 0100 */
			call_i(0x500);
			break;
		case 0xb5: /* 1: 1011 0101 */
			cpl_f1();
			break;
		case 0xb6: /* 2: 1011 0110 */
			jf0_i();
			break;
		case 0xb7: /* 1: 1011 0111 */
			illegal();
			break;
		case 0xb8: /* 2: 1011 1rrr */
		case 0xb9: /* 2: 1011 1rrr */
		case 0xba: /* 2: 1011 1rrr */
		case 0xbb: /* 2: 1011 1rrr */
		case 0xbc: /* 2: 1011 1rrr */
		case 0xbd: /* 2: 1011 1rrr */
		case 0xbe: /* 2: 1011 1rrr */
		case 0xbf: /* 2: 1011 1rrr */
			mov_r_i(op & 7);
			break;
		case 0xc0: /* 1: 1100 0000 */
			illegal();
			break;
		case 0xc1: /* 1: 1100 0001 */
			illegal();
			break;
		case 0xc2: /* 1: 1100 0010 */
			illegal();
			break;
		case 0xc3: /* 1: 1100 0011 */
			illegal();
			break;
		case 0xc4: /* 2: aaa0 0100 */
			jmp_i(0x600);
			break;
		case 0xc5: /* 1: 1100 0101 */
			sel_rb0();
			break;
		case 0xc6: /* 2: 1100 0110 */
			jz_i();
			break;
		case 0xc7: /* 1: 1100 0111 */
			mov_a_psw();
			break;
		case 0xc8: /* 1: 1100 1rrr */
		case 0xc9: /* 1: 1100 1rrr */
		case 0xca: /* 1: 1100 1rrr */
		case 0xcb: /* 1: 1100 1rrr */
		case 0xcc: /* 1: 1100 1rrr */
		case 0xcd: /* 1: 1100 1rrr */
		case 0xcf: /* 1: 1100 1rrr */
			dec_r(op & 7);
			break;
		case 0xd0: /* 1: 1101 000r */
			xrl_rm(0);
			break;
		case 0xd1: /* 1: 1101 000r */
			xrl_rm(1);
			break;
		case 0xd2: /* 2: bbb1 0010 */
			jbb_i(6);
			break;
		case 0xd3: /* 1: 1101 0011 */
			xrl_i();
			break;
		case 0xd4: /* 2: aaa1 0100 */
			call_i(0x600);
			break;
		case 0xd5: /* 1: 1101 0101 */
			sel_rb1();
			break;
		case 0xd6: /* 2: 1101 0110 */
			jnibf_i();
			break;
		case 0xd7: /* 1: 1101 0111 */
			mov_psw_a();
			break;
		case 0xd8: /* 1: 1101 1rrr */
		case 0xd9: /* 1: 1101 1rrr */
		case 0xda: /* 1: 1101 1rrr */
		case 0xdb: /* 1: 1101 1rrr */
		case 0xdc: /* 1: 1101 1rrr */
		case 0xdd: /* 1: 1101 1rrr */
		case 0xde: /* 1: 1101 1rrr */
		case 0xdf: /* 1: 1101 1rrr */
			xrl_r(op & 7);
			break;
		case 0xe0: /* 1: 1110 0000 */
			illegal();
			break;
		case 0xe1: /* 1: 1110 0001 */
			illegal();
			break;
		case 0xe2: /* 1: 1110 0010 */
			illegal();
			break;
		case 0xe3: /* 2: 1110 0011 */
			movp3_a_am();
			break;
		case 0xe4: /* 2: aaa0 0100 */
			jmp_i(0x700);
			break;
		case 0xe5: /* 1: 1110 0101 */
			en_dma();
			break;
		case 0xe6: /* 2: 1110 0110 */
			jnc_i();
			break;
		case 0xe7: /* 1: 1110 0111 */
			rl_a();
			break;
		case 0xe8: /* 2: 1110 1rrr */
		case 0xe9: /* 2: 1110 1rrr */
		case 0xea: /* 2: 1110 1rrr */
		case 0xeb: /* 2: 1110 1rrr */
		case 0xec: /* 2: 1110 1rrr */
		case 0xed: /* 2: 1110 1rrr */
		case 0xee: /* 2: 1110 1rrr */
		case 0xef: /* 2: 1110 1rrr */
			djnz_r_i(op & 7);
			break;
		case 0xf0: /* 1: 1111 000r */
			mov_a_rm(0);
			break;
		case 0xf1: /* 1: 1111 000r */
			mov_a_rm(1);
			break;
		case 0xf2: /* 2: bbb1 0010 */
			jbb_i(7);
			break;
		case 0xf3: /* 1: 1111 0011 */
			illegal();
			break;
		case 0xf4: /* 2: aaa1 0100 */
			call_i(0x700);
			break;
		case 0xf5: /* 1: 1111 0101 */
			en_flags();
			break;
		case 0xf6: /* 2: 1111 0110 */
			jc_i();
			break;
		case 0xf7: /* 1: 1111 0111 */
			rlc_a();
			break;
		case 0xf8: /* 1: 1111 1rrr */
		case 0xf9: /* 1: 1111 1rrr */
		case 0xfa: /* 1: 1111 1rrr */
		case 0xfb: /* 1: 1111 1rrr */
		case 0xfc: /* 1: 1111 1rrr */
		case 0xfd: /* 1: 1111 1rrr */
		case 0xfe: /* 1: 1111 1rrr */
		case 0xff: /* 1: 1111 1rrr */
			mov_a_r(op & 7);
			break;
		}


		if( ENABLE & CNT )
		{
			inst_cycles = i8x41_cycles[op];
			for ( ; inst_cycles > 0; inst_cycles-- )
			{
				T1_level = RP(I8X41_t1);
				if( (CONTROL & TEST1) && (T1_level == 0) )	/* Negative Edge */
				{
					TIMER++;
					if (TIMER == 0)
					{
						CONTROL |= TOVF;
						if( ENABLE & TCNTI )
							CONTROL |= TIRQ_PEND;
					}
				}
				if( T1_level ) CONTROL |= TEST1;
				else CONTROL &= ~TEST1;
			}
		}

		if( ENABLE & T )
		{
			PRESCALER += i8x41_cycles[op];
			/**** timer is prescaled by 32 ****/
			if( PRESCALER >= 32 )
			{
				PRESCALER -= 32;
				TIMER++;
				if( TIMER == 0 )
				{
					CONTROL |= TOVF;
					if( ENABLE & TCNTI )
						CONTROL |= TIRQ_PEND;
				}
			}
		}

		if( CONTROL & IRQ_PEND )	/* Are any Interrupts Pending ? */
		{
			if( 0 == (CONTROL & IRQ_EXEC) )	/* Are any Interrupts being serviced ? */
			{
				if( (ENABLE & IBFI) && (CONTROL & IBFI_PEND) )
				{
					PUSH_PC_TO_STACK();
					PC = V_IBF;
					CONTROL &= ~IBFI_PEND;
					CONTROL |= IBFI_EXEC;
					i8x41_ICount -= 2;
				}
			}
			if( 0 == (CONTROL & IRQ_EXEC) )	/* Are any Interrupts being serviced ? */
			{
				if( (ENABLE & TCNTI) && (CONTROL & TIRQ_PEND) )
				{
					PUSH_PC_TO_STACK();
					PC = V_TIMER;
					CONTROL &= ~TIRQ_PEND;
					CONTROL |= TIRQ_EXEC;
					if( ENABLE & T ) PRESCALER += 2;	/* 2 states */
					i8x41_ICount -= 2;		/* 2 states to take interrupt */
				}
			}
		}


	} while( i8x41_ICount > 0 );

	return cycles - i8x41_ICount;
}


/****************************************************************************
 *	Get all registers in given buffer
 ****************************************************************************/

unsigned i8x41_get_context(void *dst)
{
	if( dst )
		memcpy(dst, &i8x41, sizeof(I8X41));
	return sizeof(I8X41);
}


/****************************************************************************
 *	Set all registers to given values
 ****************************************************************************/

void i8x41_set_context(void *src)
{
	if( src )
		memcpy(&i8x41, src, sizeof(I8X41));
}

/****************************************************************************
 *	Return a specific register
 ****************************************************************************/

unsigned i8x41_get_reg(int regnum)
{
	switch( regnum )
	{
	case REG_PC:
	case I8X41_PC:	return PC;
	case REG_SP:
	case I8X41_SP:	return PSW & SP;
	case I8X41_PSW: return PSW;
	case I8X41_A:	return A;
	case I8X41_T:	return TIMER;
	case I8X41_R0:	return R(0);
	case I8X41_R1:	return R(1);
	case I8X41_R2:	return R(2);
	case I8X41_R3:	return R(3);
	case I8X41_R4:	return R(4);
	case I8X41_R5:	return R(5);
	case I8X41_R6:	return R(6);
	case I8X41_R7:	return R(7);
	case I8X41_DATA:
  		log_cb(RETRO_LOG_DEBUG, LOGPRE "i8x41 #%d:%03x  Reading DATA DBBI %02x.  State was %02x,  ", cpu_getactivecpu(), PC, DBBO, STATE);
			STATE &= ~OBF;	/* reset the output buffer full flag */
			if( ENABLE & FLAGS)
			{
				P2_HS &= 0xef;
				if( STATE & IBF ) P2_HS |= 0x20;
				else P2_HS &= 0xdf;
				WP(0x02, (P2 & P2_HS) );	/* Clear the DBBO IRQ out on P24 */
			}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "STATE now %02x\n", STATE);
			return DBBO;
	case I8X41_DATA_DASM:	/* Same as I8X41_DATA, except this is used by the */
							/* debugger and does not upset the flag states */
			return DBBO;
	case I8X41_STAT:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "i8x41 #%d:%03x  Reading STAT %02x\n", cpu_getactivecpu(), PC, STATE);
			return STATE;
	case REG_PREVIOUSPC: return PPC;
	default:
		if( regnum <= REG_SP_CONTENTS )
		{
			unsigned offset = (PSW & SP) + (REG_SP_CONTENTS - regnum);
			if( offset < 8 )
				return RM( M_STACK + offset ) | ( RM( M_STACK + offset + 1 ) << 8 );
		}
	}
	return 0;
}

/****************************************************************************
 *	Set a specific register
 ****************************************************************************/

void i8x41_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case REG_PC:
	case I8X41_PC:	PC = val & 0x7ff; break;
	case REG_SP:
	case I8X41_SP:	PSW = (PSW & ~SP) | (val & SP); break;
	case I8X41_PSW: PSW = val; break;
	case I8X41_A:	A = val; break;
	case I8X41_T:	TIMER = val & 0x1fff; break;
	case I8X41_R0:	R(0) = val; break;
	case I8X41_R1:	R(1) = val; break;
	case I8X41_R2:	R(2) = val; break;
	case I8X41_R3:	R(3) = val; break;
	case I8X41_R4:	R(4) = val; break;
	case I8X41_R5:	R(5) = val; break;
	case I8X41_R6:	R(6) = val; break;
	case I8X41_R7:	R(7) = val; break;
	case I8X41_DATA:
  		log_cb(RETRO_LOG_DEBUG, LOGPRE "i8x41 #%d:%03x  Setting DATA DBBI to %02x. State was %02x  ", cpu_getactivecpu(), PC, val,STATE);
			DBBI = val;
			if( i8x41.subtype == 8041 ) /* plain 8041 had no split input/output DBB buffers */
				DBBO = val;
			STATE &= ~F1;
			STATE |= IBF;
			if( ENABLE & IBFI )
				CONTROL |= IBFI_PEND;
			if( ENABLE & FLAGS)
			{
				P2_HS |= 0x20;
				if( 0 == (STATE & OBF) ) P2_HS |= 0x10;
				else P2_HS &= 0xef;
				WP(0x02, (P2 & P2_HS) );	/* Assert the DBBI IRQ out on P25 */
			}
		  log_cb(RETRO_LOG_DEBUG, LOGPRE "State now %02x\n", STATE);
			break;
	case I8X41_DATA_DASM:	/* Same as I8X41_DATA, except this is used by the */
							/* debugger and does not upset the flag states */
			DBBI = val;
			if( i8x41.subtype == 8041 ) /* plain 8041 had no split input/output DBB buffers */
				DBBO = val;
			break;
	case I8X41_CMND:
      log_cb(RETRO_LOG_DEBUG, LOGPRE "i8x41 #%d:%03x  Setting CMND DBBI to %02x. State was %02x,  ", cpu_getactivecpu(), PC, val,STATE);
			DBBI = val;
			if( i8x41.subtype == 8041 ) /* plain 8041 had no split input/output DBB buffers */
				DBBO = val;
			STATE |= F1;
			STATE |= IBF;
			if( ENABLE & IBFI )
				CONTROL |= IBFI_PEND;
			if( ENABLE & FLAGS)
			{
				P2_HS |= 0x20;
				if( 0 == (STATE & OBF) ) P2_HS |= 0x10;
				else P2_HS &= 0xef;
				WP(0x02, (P2 & P2_HS) );	/* Assert the DBBI IRQ out on P25 */
			}
      log_cb(RETRO_LOG_DEBUG, LOGPRE "State now %02x\n", STATE);
			break;
	case I8X41_CMND_DASM:	/* Same as I8X41_CMND, except this is used by the */
							/* debugger and does not upset the flag states */
			DBBI = val;
			if( i8x41.subtype == 8041 ) /* plain 8041 had no split input/output DBB buffers */
				DBBO = val;
			break;
	case I8X41_STAT:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "i8x41 #%d:%03x  Setting STAT DBBI to %02x\n", cpu_getactivecpu(), PC, val);
			/* writing status.. hmm, should we issue interrupts here too? */
			STATE = val;
			break;
	default:
		if( regnum <= REG_SP_CONTENTS )
		{
			unsigned offset = (PSW & SP) + (REG_SP_CONTENTS - regnum);
			if( offset < 8 )
			{
				WM( M_STACK + offset, val & 0xff );
				WM( M_STACK + offset + 1, (val >> 8) & 0xff );
			}
		}
	}
}


/****************************************************************************
 *	Set IRQ line state
 ****************************************************************************/

void i8x41_set_irq_line(int irqline, int state)
{
	switch( irqline )
	{
	case I8X41_INT_IBF:
		if (state != CLEAR_LINE)
		{
			STATE |= IBF;
			if (ENABLE & IBFI)
			{
				CONTROL |= IBFI_PEND;
			}
		}
		else
		{
			STATE &= ~IBF;
		}
		break;

	case I8X41_INT_TEST1:
		if( state != CLEAR_LINE )
		{
			CONTROL |= TEST1;
		}
		else
		{
			/* high to low transition? */
			if( CONTROL & TEST1 )
			{
				/* counting enabled? */
				if( ENABLE & CNT )
				{
					TIMER++;
					if( TIMER == 0 )
					{
						CONTROL |= TOVF;
						CONTROL |= TIRQ_PEND;
					}
				}
			}
			CONTROL &= ~TEST1;
		}
		break;
	}
}

void i8x41_set_irq_callback(int (*callback)(int irqline))
{
	i8x41.irq_callback = callback;
}

/****************************************************************************
 *	Return a formatted string for a register
 ****************************************************************************/

const char *i8x41_info(void *context, int regnum)
{
	static char buffer[8][15+1];
	static int which = 0;
	I8X41 *r = context;

	which = (which+1) % 8;
	buffer[which][0] = '\0';
	if( !context )
		r = &i8x41;

	switch( regnum )
	{
		case CPU_INFO_REG+I8X41_PC: sprintf(buffer[which], "PC:%04X", r->pc); break;
		case CPU_INFO_REG+I8X41_SP: sprintf(buffer[which], "S:%X", r->psw & SP); break;
		case CPU_INFO_REG+I8X41_PSW:sprintf(buffer[which], "PSW:%02X", r->psw); break;
		case CPU_INFO_REG+I8X41_A:	sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+I8X41_T:	sprintf(buffer[which], "T:%02X.%02X", r->timer, (r->prescaler & 0x1f) ); break;
		case CPU_INFO_REG+I8X41_R0: sprintf(buffer[which], "R0:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 0]); break;
		case CPU_INFO_REG+I8X41_R1: sprintf(buffer[which], "R1:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 1]); break;
		case CPU_INFO_REG+I8X41_R2: sprintf(buffer[which], "R2:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 2]); break;
		case CPU_INFO_REG+I8X41_R3: sprintf(buffer[which], "R3:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 3]); break;
		case CPU_INFO_REG+I8X41_R4: sprintf(buffer[which], "R4:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 4]); break;
		case CPU_INFO_REG+I8X41_R5: sprintf(buffer[which], "R5:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 5]); break;
		case CPU_INFO_REG+I8X41_R6: sprintf(buffer[which], "R6:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 6]); break;
		case CPU_INFO_REG+I8X41_R7: sprintf(buffer[which], "R7:%02X", i8x41.ram[((r->psw & BS) ? M_BANK1 : M_BANK0) + 7]); break;
		case CPU_INFO_REG+I8X41_P1: sprintf(buffer[which], "P1:%02X", i8x41.p1); break;
		case CPU_INFO_REG+I8X41_P2: sprintf(buffer[which], "P2:%02X", i8x41.p2); break;
		case CPU_INFO_REG+I8X41_DATA_DASM:sprintf(buffer[which], "DBBI:%02X", i8x41.dbbi); break;
		case CPU_INFO_REG+I8X41_CMND_DASM:sprintf(buffer[which], "DBBO:%02X", i8x41.dbbo); break;
		case CPU_INFO_REG+I8X41_STAT:sprintf(buffer[which], "STAT:%02X", i8x41.state); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->psw & 0x80 ? 'C':'.',
				r->psw & 0x40 ? 'A':'.',
				r->psw & 0x20 ? '0':'.',
				r->psw & 0x10 ? 'B':'.',
				r->psw & 0x08 ? '?':'.',
				r->psw & 0x04 ? 's':'.',
				r->psw & 0x02 ? 's':'.',
				r->psw & 0x01 ? 's':'.');
			break;
		case CPU_INFO_NAME: return "I8X41";
		case CPU_INFO_FAMILY: return "Intel 8x41";
		case CPU_INFO_VERSION: return "0.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1999 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)i8x41_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)i8x41_win_layout;
	}
	return buffer[which];
}

unsigned i8x41_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm8x41( buffer, pc );
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
