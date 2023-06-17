 /**************************************************************************\
 *				   Texas Instruments TMS32010 DSP Emulator					*
 *																			*
 *					Copyright (C) 1999-2002+ Tony La Porta					*
 *		You are not allowed to distribute this software commercially.		*
 *						Written for the MAME project.						*
 *																			*
 *																			*
 *		Notes : The term 'DMA' within this document, is in reference		*
 *					to Direct Memory Addressing, and NOT the usual term		*
 *					of Direct Memory Access.								*
 *				This is a word based microcontroller, with addressing		*
 *					architecture based on the Harvard addressing scheme.	*
 *																			*
 *																			*
 *																			*
 *	**** Change Log ****													*
 *																			*
 *	TLP (13-Jul-2002)														*
 *	 - Added Save-State support												*
 *	 - Converted the pending_irq flag to INTF (a real flag in this device)	*
 *	 - Fixed the ignore Interrupt Request for previous critical				*
 *	   instructions requiring an extra instruction to be processed. For		*
 *	   this reason, instant IRQ servicing cannot be supported here, so		*
 *	   INTF needs to be polled within the instruction execution loop		*
 *	 - Removed IRQ callback (IRQ ACK not supported on this device)			*
 *	 - A pending IRQ will remain pending until it's serviced. De-asserting	*
 *	   the IRQ Pin does not remove a pending IRQ state						*
 *	 - BIO is no longer treated as an IRQ line. It's polled when required.	*
 *	   This is the true behaviour of the device								*
 *	 - Removed the Clear OV flag from overflow instructions. Overflow		*
 *	   instructions can only set the flag. Flag test instructions clear it	*
 *	 - Fixed the ABST, SUBC and SUBH instructions							*
 *	 - Fixed the signedness in many equation based instructions				*
 *	 - Added the missing Previous PC to the get_register function			*
 *	 - Changed Cycle timings to include clock ticks							*
 *	 - Converted some registers from ints to pairs for much cleaner code	*
 *	TLP (20-Jul-2002) Ver 1.10												*
 *	 - Fixed the dissasembly from the debugger								*
 *	 - Changed all references from TMS320C10 to TMS32010					*
 *	ASG (24-Sep-2002) Ver 1.20												*
 *	 - Fixed overflow handling												*
 *	 - Simplified logic in a few locations									*
 *	 - Added macros for specifying address and port ranges					*
 *																			*
 \**************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <retro_inline.h>

#include "driver.h"
#include "cpuintrf.h"
#include "mamedbg.h"
#include "state.h"
#include "tms32010.h"


#define CLK 4	/* 1 cycle equals 4 clock ticks */


#define M_RDROM(A)		TMS32010_ROM_RDMEM(A)
#define M_WRTROM(A,V)	TMS32010_ROM_WRMEM(A,V)
#define M_RDRAM(A)		TMS32010_RAM_RDMEM(A)
#define M_WRTRAM(A,V)	TMS32010_RAM_WRMEM(A,V)
#define M_RDOP(A)		TMS32010_RDOP(A)
#define M_RDOP_ARG(A)	TMS32010_RDOP_ARG(A)
#define P_IN(A)			TMS32010_In(A)
#define P_OUT(A,V)		TMS32010_Out(A,V)
#define BIO_IN			TMS32010_BIO_In
#define ADDR_MASK		TMS32010_ADDR_MASK


static UINT8 tms32010_reg_layout[] = {
	TMS32010_PC,  TMS32010_SP,  TMS32010_STR, TMS32010_ACC,-1,
	TMS32010_PREG,TMS32010_TREG,TMS32010_AR0, TMS32010_AR1,-1,
	TMS32010_STK0,TMS32010_STK1,TMS32010_STK2,TMS32010_STK3,0
};

static UINT8 tms32010_win_layout[] = {
	28, 0,52, 4,	/* register window (top rows) */
	 0, 0,27,22,	/* disassembler window (left colums) */
	28, 5,52, 8,	/* memory #1 window (right, upper middle) */
	28,14,52, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};




typedef struct			/* Page 3-6 shows all registers */
{
	/******************** CPU Internal Registers *******************/
	UINT16	PC;
	UINT16	PREVPC;		/* previous program counter */
	UINT16	STR;
	PAIR	ACC;
	PAIR	ALU;
	PAIR	Preg;
	UINT16	Treg;
	UINT16	AR[2];
	UINT16	STACK[4];

	/********************** Status data ****************************/
	PAIR	opcode;
	int		INTF;		/* Pending Interrupt flag */
} tms32010_Regs;

static tms32010_Regs R;
static PAIR oldacc;
static UINT16 memaccess;
int    tms32010_icount;
typedef void (*opcode_fn) (void);


/********  The following is the Status (Flag) register definition.  *********/
/* 15 | 14  |  13  | 12 | 11 | 10 | 9 |  8  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0  */
/* OV | OVM | INTM |  1 |  1 |  1 | 1 | ARP | 1 | 1 | 1 | 1 | 1 | 1 | 1 | DP */
#define OV_FLAG		0x8000	/* OV	(Overflow flag) 1 indicates an overflow */
#define OVM_FLAG	0x4000	/* OVM	(Overflow Mode bit) 1 forces ACC overflow to greatest positive or negative saturation value */
#define INTM_FLAG	0x2000	/* INTM	(Interrupt Mask flag) 0 enables maskable interrupts */
#define ARP_REG		0x0100	/* ARP	(Auxiliary Register Pointer) */
#define DP_REG		0x0001	/* DP	(Data memory Pointer (bank) bit) */

#define OV		( R.STR & OV_FLAG)			/* OV	(Overflow flag) */
#define OVM		( R.STR & OVM_FLAG)			/* OVM	(Overflow Mode bit) 1 indicates an overflow */
#define INTM	( R.STR & INTM_FLAG)		/* INTM	(Interrupt enable flag) 0 enables maskable interrupts */
#define ARP		((R.STR & ARP_REG) >> 8)	/* ARP	(Auxiliary Register Pointer) */
#define DP		((R.STR & DP_REG) << 7)		/* DP	(Data memory Pointer bit) */

#define DMA_DP	(DP | (R.opcode.b.l & 0x7f))	/* address used in direct memory access operations */
#define DMA_DP1	(0x80 | R.opcode.b.l)			/* address used in direct memory access operations for sst instruction */
#define IND		(R.AR[ARP] & 0xff)				/* address used in indirect memory access operations */




/************************************************************************
 *	Shortcuts
 ************************************************************************/

static INLINE void CLR(UINT16 flag) { R.STR &= ~flag; R.STR |= 0x1efe; }
static INLINE void SET(UINT16 flag) { R.STR |=  flag; R.STR |= 0x1efe; }


static INLINE void CALCULATE_ADD_OVERFLOW(INT32 addval)
{
	if ((INT32)(~(oldacc.d ^ addval) & (oldacc.d ^ R.ACC.d)) < 0) {
		SET(OV_FLAG);
		if (OVM)
			R.ACC.d = ((INT32)oldacc.d < 0) ? 0x80000000 : 0x7fffffff;
	}
}
static INLINE void CALCULATE_SUB_OVERFLOW(INT32 subval)
{
	if ((INT32)((oldacc.d ^ subval) & (oldacc.d ^ R.ACC.d)) < 0) {
		SET(OV_FLAG);
		if (OVM)
			R.ACC.d = ((INT32)oldacc.d < 0) ? 0x80000000 : 0x7fffffff;
	}
}

static INLINE UINT16 POP_STACK(void)
{
	UINT16 data = R.STACK[3];
	R.STACK[3] = R.STACK[2];
	R.STACK[2] = R.STACK[1];
	R.STACK[1] = R.STACK[0];
	return (data & ADDR_MASK);
}
static INLINE void PUSH_STACK(UINT16 data)
{
	R.STACK[0] = R.STACK[1];
	R.STACK[1] = R.STACK[2];
	R.STACK[2] = R.STACK[3];
	R.STACK[3] = (data & ADDR_MASK);
}

static INLINE void GET_MEM_ADDR(UINT16 DMA)
{
	if (R.opcode.b.l & 0x80)
		memaccess = IND;
	else
		memaccess = DMA;
}
static INLINE void UPDATE_AR(void)
{
	if (R.opcode.b.l & 0x30) {
		UINT16 tmpAR = R.AR[ARP];
		if (R.opcode.b.l & 0x20) tmpAR++ ;
		if (R.opcode.b.l & 0x10) tmpAR-- ;
		R.AR[ARP] = (R.AR[ARP] & 0xfe00) | (tmpAR & 0x01ff);
	}
}
static INLINE void UPDATE_ARP(void)
{
	if (~R.opcode.b.l & 0x08) {
		if (R.opcode.b.l & 0x01) SET(ARP_REG);
		else CLR(ARP_REG);
	}
}


static INLINE void getdata(UINT8 shift,UINT8 signext)
{
	GET_MEM_ADDR(DMA_DP);
	R.ALU.d = (UINT16)M_RDRAM(memaccess);
	if (signext) R.ALU.d = (INT16)R.ALU.d;
	R.ALU.d <<= shift;
	if (R.opcode.b.l & 0x80) {
		UPDATE_AR();
		UPDATE_ARP();
	}
}

static INLINE void putdata(UINT16 data)
{
	GET_MEM_ADDR(DMA_DP);
	if (R.opcode.b.l & 0x80) {
		UPDATE_AR();
		UPDATE_ARP();
	}
	M_WRTRAM(memaccess,data);
}
static INLINE void putdata_sar(UINT8 data)
{
	GET_MEM_ADDR(DMA_DP);
	if (R.opcode.b.l & 0x80) {
		UPDATE_AR();
		UPDATE_ARP();
	}
	M_WRTRAM(memaccess,R.AR[data]);
}
static INLINE void putdata_sst(UINT16 data)
{
	GET_MEM_ADDR(DMA_DP1);		/* Page 1 only */
	if (R.opcode.b.l & 0x80) {
		UPDATE_AR();
	}
	M_WRTRAM(memaccess,data);
}



/************************************************************************
 *	Emulate the Instructions
 ************************************************************************/

/* This following function is here to fill in the void for */
/* the opcode call function. This function is never called. */

static void other_7F_opcodes(void)  { }


static void illegal(void)
{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32010:  PC=%04x,  Illegal opcode = %04x\n", (R.PC-1), R.opcode.w.l);
}

static void abst(void)
{
		if ( (INT32)(R.ACC.d) < 0 ) {
			R.ACC.d = -R.ACC.d;
			if (OVM && (R.ACC.d == 0x80000000)) R.ACC.d-- ;
		}
}

/*** The manual does not mention overflow with the ADD? instructions *****
 *** however I implemented overflow, coz it doesnt make sense otherwise **
 *** and newer generations of this type of chip supported it. I think ****
 *** the manual is wrong (apart from other errors the manual has). *******

static void add_sh(void)	{ getdata(R.opcode.b.h,1); R.ACC.d += R.ALU.d; }
static void addh(void)		{ getdata(0,0); R.ACC.d += (R.ALU.d << 16); }
 ***/

static void add_sh(void)
{
		oldacc.d = R.ACC.d;
		getdata((R.opcode.b.h & 0xf),1);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
}
static void addh(void)
{
		oldacc.d = R.ACC.d;
		getdata(16,0);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
}
static void adds(void)
{
		oldacc.d = R.ACC.d;
		getdata(0,0);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
}
static void and(void)
{
		getdata(0,0);
		R.ACC.d &= R.ALU.d;
}
static void apac(void)
{
		oldacc.d = R.ACC.d;
		R.ACC.d += R.Preg.d;
		CALCULATE_ADD_OVERFLOW(R.Preg.d);
}
static void br(void)
{
		R.PC = M_RDOP_ARG(R.PC);
}
static void banz(void)
{
		if (R.AR[ARP] & 0x01ff)
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
		R.ALU.w.l = R.AR[ARP];
		R.ALU.w.l-- ;
		R.AR[ARP] = (R.AR[ARP] & 0xfe00) | (R.ALU.w.l & 0x01ff);
}
static void bgez(void)
{
		if ( (INT32)(R.ACC.d) >= 0 )
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void bgz(void)
{
		if ( (INT32)(R.ACC.d) > 0 )
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void bioz(void)
{
		if (BIO_IN != CLEAR_LINE)
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void blez(void)
{
		if ( (INT32)(R.ACC.d) <= 0 )
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void blz(void)
{
		if ( (INT32)(R.ACC.d) <  0 )
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void bnz(void)
{
		if (R.ACC.d != 0)
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void bv(void)
{
		if (OV) {
			R.PC = M_RDOP_ARG(R.PC);
			CLR(OV_FLAG);
		}
		else
			R.PC++ ;
}
static void bz(void)
{
		if (R.ACC.d == 0)
			R.PC = M_RDOP_ARG(R.PC);
		else
			R.PC++ ;
}
static void cala(void)
{
		PUSH_STACK(R.PC);
		R.PC = R.ACC.w.l & ADDR_MASK;
}
static void call(void)
{
		R.PC++ ;
		PUSH_STACK(R.PC);
		R.PC = M_RDOP_ARG((R.PC - 1)) & ADDR_MASK;
}
static void dint(void)
{
		SET(INTM_FLAG);
}
static void dmov(void)
{
		getdata(0,0);
		M_WRTRAM((memaccess + 1),R.ALU.w.l);
}
static void eint(void)
{
		CLR(INTM_FLAG);
}
static void in_p(void)
{
		R.ALU.w.l = P_IN( (R.opcode.b.h & 7) );
		putdata(R.ALU.w.l);
}
static void lac_sh(void)
{
		getdata((R.opcode.b.h & 0x0f),1);
		R.ACC.d = R.ALU.d;
}
static void lack(void)
{
		R.ACC.d = R.opcode.b.l;
}
static void lar_ar0(void)
{
		getdata(0,0);
		R.AR[0] = R.ALU.w.l;
}
static void lar_ar1(void)
{
		getdata(0,0);
		R.AR[1] = R.ALU.w.l;
}
static void lark_ar0(void)
{
		R.AR[0] = R.opcode.b.l;
}
static void lark_ar1(void)
{
		R.AR[1] = R.opcode.b.l;
}
static void larp_mar(void)
{
		if (R.opcode.b.l & 0x80) {
			UPDATE_AR();
			UPDATE_ARP();
		}
}
static void ldp(void)
{
		getdata(0,0);
		if (R.ALU.d & 1)
			SET(DP_REG);
		else
			CLR(DP_REG);
}
static void ldpk(void)
{
		if (R.opcode.b.l & 1)
			SET(DP_REG);
		else
			CLR(DP_REG);
}
static void lst(void)
{
		R.opcode.b.l |= 0x08; /* Next arp not supported here, so mask it */
		getdata(0,0);
		R.ALU.w.l &= (~INTM_FLAG);	/* Must not affect INTM */
		R.STR &= INTM_FLAG;
		R.STR |= R.ALU.w.l;
		R.STR |= 0x1efe;
}
static void lt(void)
{
		getdata(0,0);
		R.Treg = R.ALU.w.l;
}
static void lta(void)
{
		oldacc.d = R.ACC.d;
		getdata(0,0);
		R.Treg = R.ALU.w.l;
		R.ACC.d += R.Preg.d;
		CALCULATE_ADD_OVERFLOW(R.Preg.d);
}
static void ltd(void)
{
		oldacc.d = R.ACC.d;
		getdata(0,0);
		R.Treg = R.ALU.w.l;
		M_WRTRAM((memaccess + 1),R.ALU.w.l);
		R.ACC.d += R.Preg.d;
		CALCULATE_ADD_OVERFLOW(R.Preg.d);
}
static void mpy(void)
{
		getdata(0,0);
		R.Preg.d = (INT16)R.ALU.w.l * (INT16)R.Treg;
		if (R.Preg.d == 0x40000000) R.Preg.d = 0xc0000000;
}
static void mpyk(void)
{
		R.Preg.d = (INT16)R.Treg * ((INT16)(R.opcode.w.l << 3) >> 3);
}
static void nop(void)
{
		/* Nothing to do */
}
static void or(void)
{
		getdata(0,0);
		R.ACC.w.l |= R.ALU.w.l;
}
static void out_p(void)
{
		getdata(0,0);
		P_OUT( (R.opcode.b.h & 7), R.ALU.w.l );
}
static void pac(void)
{
		R.ACC.d = R.Preg.d;
}
static void pop(void)
{
		R.ACC.w.l = POP_STACK();
		R.ACC.w.h = 0x0000;
}
static void push(void)
{
		PUSH_STACK(R.ACC.w.l);
}
static void ret(void)
{
		R.PC = POP_STACK();
}
static void rovm(void)
{
		CLR(OVM_FLAG);
}
static void sach_sh(void)
{
		R.ALU.d = (R.ACC.d << (R.opcode.b.h & 7));
		putdata(R.ALU.w.h);
}
static void sacl(void)
{
		putdata(R.ACC.w.l);
}
static void sar_ar0(void)
{
		putdata_sar(0);
}
static void sar_ar1(void)
{
		putdata_sar(1);
}
static void sovm(void)
{
		SET(OVM_FLAG);
}
static void spac(void)
{
		oldacc.d = R.ACC.d;
		R.ACC.d -= R.Preg.d;
		CALCULATE_SUB_OVERFLOW(R.Preg.d);
}
static void sst(void)
{
		putdata_sst(R.STR);
}
static void sub_sh(void)
{
		oldacc.d = R.ACC.d;
		getdata((R.opcode.b.h & 0x0f),1);
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
}
static void subc(void)
{
		oldacc.d = R.ACC.d;
		getdata(15,0);
		R.ALU.d -= R.ALU.d;
		if ((INT32)((oldacc.d ^ R.ALU.d) & (oldacc.d ^ R.ACC.d)) < 0)
			SET(OV_FLAG);
		if ( (INT32)(R.ALU.d) >= 0 )
			R.ACC.d = ((R.ALU.d << 1) + 1);
		else
			R.ACC.d = (R.ACC.d << 1);
}
static void subh(void)
{
		oldacc.d = R.ACC.d;
		getdata(16,0);
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
}
static void subs(void)
{
		oldacc.d = R.ACC.d;
		getdata(0,0);
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
}
static void tblr(void)
{
		R.ALU.d = M_RDROM((R.ACC.w.l & ADDR_MASK));
		putdata(R.ALU.w.l);
		R.STACK[0] = R.STACK[1];
}
static void tblw(void)
{
		getdata(0,0);
		M_WRTROM(((R.ACC.w.l & ADDR_MASK)),R.ALU.w.l);
		R.STACK[0] = R.STACK[1];
}
static void xor(void)
{
		getdata(0,0);
		R.ACC.w.l ^= R.ALU.w.l;
}
static void zac(void)
{
		R.ACC.d = 0;
}
static void zalh(void)
{
		getdata(0,0);
		R.ACC.w.h = R.ALU.w.l;
		R.ACC.w.l = 0x0000;
}
static void zals(void)
{
		getdata(0,0);
		R.ACC.w.l = R.ALU.w.l;
		R.ACC.w.h = 0x0000;
}


/***********************************************************************
 *	Cycle Timings
 ***********************************************************************/

static unsigned cycles_main[256]=
{
/*00*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*10*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*20*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*30*/	1*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 1*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*40*/	2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*50*/	1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*60*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 3*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*70*/	1*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 3*CLK, 1*CLK, 0*CLK,
/*80*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*90*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*A0*/	0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*B0*/	0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*C0*/	0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*D0*/	0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*E0*/	0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*F0*/	0*CLK, 0*CLK, 0*CLK, 0*CLK, 2*CLK, 2*CLK, 2*CLK, 0*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK
};

static unsigned cycles_7F_other[32]=
{
/*80*/	1*CLK, 1*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 2*CLK, 2*CLK, 1*CLK, 1*CLK,
/*90*/	1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 2*CLK, 2*CLK, 1*CLK, 1*CLK
};


/***********************************************************************
 *	Opcode Table
 ***********************************************************************/

static opcode_fn opcode_main[256]=
{
/*00*/  add_sh		,add_sh		,add_sh		,add_sh		,add_sh		,add_sh		,add_sh		,add_sh
/*08*/ ,add_sh		,add_sh		,add_sh		,add_sh		,add_sh		,add_sh		,add_sh		,add_sh
/*10*/ ,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh
/*18*/ ,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh		,sub_sh
/*20*/ ,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh
/*28*/ ,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh		,lac_sh
/*30*/ ,sar_ar0		,sar_ar1	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*38*/ ,lar_ar0		,lar_ar1	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*40*/ ,in_p		,in_p		,in_p		,in_p		,in_p		,in_p		,in_p		,in_p
/*48*/ ,out_p		,out_p		,out_p		,out_p		,out_p		,out_p		,out_p		,out_p
/*50*/ ,sacl		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*58*/ ,sach_sh		,sach_sh	,sach_sh	,sach_sh	,sach_sh	,sach_sh	,sach_sh	,sach_sh
/*60*/ ,addh		,adds		,subh		,subs		,subc		,zalh		,zals		,tblr
/*68*/ ,larp_mar	,dmov		,lt			,ltd		,lta		,mpy		,ldpk		,ldp
/*70*/ ,lark_ar0	,lark_ar1	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*78*/ ,xor			,and		,or			,lst		,sst		,tblw		,lack		,other_7F_opcodes
/*80*/ ,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk
/*88*/ ,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk
/*90*/ ,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk
/*98*/ ,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk		,mpyk
/*A0*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*A8*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*B0*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*B8*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*C0*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*C8*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*D0*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*D8*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*E0*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*E8*/ ,illegal		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*F0*/ ,illegal		,illegal	,illegal	,illegal	,banz		,bv			,bioz		,illegal
/*F8*/ ,call		,br			,blz		,blez		,bgz		,bgez		,bnz		,bz
};

static opcode_fn opcode_7F_other[32]=
{
/*80*/  nop			,dint		,eint		,illegal	,illegal	,illegal	,illegal	,illegal
/*88*/ ,abst		,zac		,rovm		,sovm		,cala		,ret		,pac		,apac
/*90*/ ,spac		,illegal	,illegal	,illegal	,illegal	,illegal	,illegal	,illegal
/*98*/ ,illegal		,illegal	,illegal	,illegal	,push		,pop		,illegal	,illegal
};



/****************************************************************************
 *	Inits CPU emulation
 ****************************************************************************/
void tms32010_init (void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("tms32010", cpu, "PC", &R.PC, 1);
	state_save_register_UINT16("tms32010", cpu, "PrevPC", &R.PREVPC, 1);
	state_save_register_UINT16("tms32010", cpu, "STR", &R.STR, 1);
	state_save_register_UINT32("tms32010", cpu, "ACC", &R.ACC.d, 1);
	state_save_register_UINT32("tms32010", cpu, "ALU", &R.ALU.d, 1);
	state_save_register_UINT32("tms32010", cpu, "Preg", &R.Preg.d, 1);
	state_save_register_UINT16("tms32010", cpu, "Treg", &R.Treg, 1);
	state_save_register_UINT16("tms32010", cpu, "AR0", &R.AR[0], 1);
	state_save_register_UINT16("tms32010", cpu, "AR1", &R.AR[1], 1);
	state_save_register_UINT16("tms32010", cpu, "Stack0", &R.STACK[0], 1);
	state_save_register_UINT16("tms32010", cpu, "Stack1", &R.STACK[1], 1);
	state_save_register_UINT16("tms32010", cpu, "Stack2", &R.STACK[2], 1);
	state_save_register_UINT16("tms32010", cpu, "Stack3", &R.STACK[3], 1);
	state_save_register_INT32("tms32010",  cpu, "IRQ_Flag", &R.INTF, 1);
	state_save_register_UINT32("tms32010", cpu, "Opcode", &R.opcode.d, 1);
}

/****************************************************************************
 *	Reset registers to their initial values
 ****************************************************************************/
void tms32010_reset (void *param)
{
	R.PC    = 0;
	R.STR   = 0xfefe;
	R.ACC.d = 0;
	R.INTF  = TMS32010_INT_NONE;
}


/****************************************************************************
 *	Shut down CPU emulation
 ****************************************************************************/
void tms32010_exit (void)
{
	/* nothing to do ? */
}


/****************************************************************************
 *	Issue an interrupt if necessary
 ****************************************************************************/
static int Ext_IRQ(void)
{
	if (INTM == 0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32010:  EXT INTERRUPT\n");
		R.INTF = TMS32010_INT_NONE;
		SET(INTM_FLAG);
		PUSH_STACK(R.PC);
		R.PC = 0x0002;
		return (3*CLK);  /* 3 cycles used due to PUSH and DINT operation ? */
	}
	return (0*CLK);
}



/****************************************************************************
 *	Execute IPeriod. Return 0 if emulation should be stopped
 ****************************************************************************/
int tms32010_execute(int cycles)
{
	tms32010_icount = cycles;

	do
	{
		if (R.INTF) {
			/* Dont service INT if prev instruction was MPY, MPYK or EINT */
			if ((R.opcode.b.h != 0x6d) && ((R.opcode.b.h & 0xe0) != 0x80) && (R.opcode.w.l != 0x7f82))
				tms32010_icount -= Ext_IRQ();
		}

		R.PREVPC = R.PC;

		CALL_MAME_DEBUG;

		R.opcode.d = M_RDOP(R.PC);
		R.PC++;

		if (R.opcode.b.h != 0x7f)	{ /* Do all opcodes except the 7Fxx ones */
			tms32010_icount -= cycles_main[R.opcode.b.h];
			(*(opcode_main[R.opcode.b.h]))();
		}
		else { /* Opcode major byte 7Fxx has many opcodes in its minor byte */
			tms32010_icount -= cycles_7F_other[(R.opcode.b.l & 0x1f)];
			(*(opcode_7F_other[(R.opcode.b.l & 0x1f)]))();
		}
	} while (tms32010_icount>0);

	return cycles - tms32010_icount;
}

/****************************************************************************
 *	Get all registers in given buffer
 ****************************************************************************/
unsigned tms32010_get_context (void *dst)
{
	if( dst )
		*(tms32010_Regs*)dst = R;
	return sizeof(tms32010_Regs);
}

/****************************************************************************
 *	Set all registers to given values
 ****************************************************************************/
void tms32010_set_context (void *src)
{
	if (src)
		R = *(tms32010_Regs*)src;
}


/****************************************************************************
 *	Return a specific register
 ****************************************************************************/
unsigned tms32010_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case TMS32010_PC: return R.PC;
		/* This is actually not a stack pointer, but the stack contents */
		case REG_SP:
		case TMS32010_STK3: return R.STACK[3];
		case TMS32010_ACC:  return R.ACC.d;
		case TMS32010_STR:  return R.STR;
		case TMS32010_PREG: return R.Preg.d;
		case TMS32010_TREG: return R.Treg;
		case TMS32010_AR0:  return R.AR[0];
		case TMS32010_AR1:  return R.AR[1];
		case REG_PREVIOUSPC: return R.PREVPC;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if (offset < 4)
					return R.STACK[offset];
			}
	}
	return 0;
}


/****************************************************************************
 *	Set a specific register
 ****************************************************************************/
void tms32010_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
		case TMS32010_PC: R.PC = val; break;
		/* This is actually not a stack pointer, but the stack contents */
		/* Stack is a 4 level First In Last Out stack */
		case REG_SP:
		case TMS32010_STK3: R.STACK[3] = val; break;
		case TMS32010_STR:  R.STR    = val; break;
		case TMS32010_ACC:  R.ACC.d  = val; break;
		case TMS32010_PREG: R.Preg.d = val; break;
		case TMS32010_TREG: R.Treg   = val; break;
		case TMS32010_AR0:  R.AR[0]  = val; break;
		case TMS32010_AR1:  R.AR[1]  = val; break;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if (offset < 4)
					R.STACK[offset] = val;
			}
	}
}


/****************************************************************************
 *	Set IRQ line state
 ****************************************************************************/
void tms32010_set_irq_line(int irqline, int state)
{
	/* Pending Interrupts cannot be cleared! */
	if (state == ASSERT_LINE) R.INTF |=  TMS32010_INT_PENDING;
}

void tms32010_set_irq_callback(int (*callback)(int irqline))
{
	/* There are no IRQ Acknowledge Pins on this device */
}

/****************************************************************************
 *	Return a formatted string for a register
 ****************************************************************************/
const char *tms32010_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	tms32010_Regs *r = context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if (!context)
		r = &R;

	switch (regnum)
	{
		case CPU_INFO_REG+TMS32010_PC:   sprintf(buffer[which], "PC:%04X",   r->PC); break;
		case CPU_INFO_REG+TMS32010_SP:   sprintf(buffer[which], "SP:%X", 0); /* fake stack pointer */ break;
		case CPU_INFO_REG+TMS32010_STR:  sprintf(buffer[which], "STR:%04X",  r->STR); break;
		case CPU_INFO_REG+TMS32010_ACC:  sprintf(buffer[which], "ACC:%08X",  r->ACC.d); break;
		case CPU_INFO_REG+TMS32010_PREG: sprintf(buffer[which], "P:%08X",    r->Preg.d); break;
		case CPU_INFO_REG+TMS32010_TREG: sprintf(buffer[which], "T:%04X",    r->Treg); break;
		case CPU_INFO_REG+TMS32010_AR0:  sprintf(buffer[which], "AR0:%04X",  r->AR[0]); break;
		case CPU_INFO_REG+TMS32010_AR1:  sprintf(buffer[which], "AR1:%04X",  r->AR[1]); break;
		case CPU_INFO_REG+TMS32010_STK0: sprintf(buffer[which], "STK0:%04X", r->STACK[0]); break;
		case CPU_INFO_REG+TMS32010_STK1: sprintf(buffer[which], "STK1:%04X", r->STACK[1]); break;
		case CPU_INFO_REG+TMS32010_STK2: sprintf(buffer[which], "STK2:%04X", r->STACK[2]); break;
		case CPU_INFO_REG+TMS32010_STK3: sprintf(buffer[which], "STK3:%04X", r->STACK[3]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->STR & 0x8000 ? 'O':'.',
				r->STR & 0x4000 ? 'M':'.',
				r->STR & 0x2000 ? 'I':'.',
				r->STR & 0x1000 ? '.':'?',
				r->STR & 0x0800 ? 'a':'?',
				r->STR & 0x0400 ? 'r':'?',
				r->STR & 0x0200 ? 'p':'?',
				r->STR & 0x0100 ? '1':'0',
				r->STR & 0x0080 ? '.':'?',
				r->STR & 0x0040 ? '.':'?',
				r->STR & 0x0020 ? '.':'?',
				r->STR & 0x0010 ? '.':'?',
				r->STR & 0x0008 ? '.':'?',
				r->STR & 0x0004 ? 'd':'?',
				r->STR & 0x0002 ? 'p':'?',
				r->STR & 0x0001 ? '1':'0');
			break;
		case CPU_INFO_NAME: return "TMS32010";
		case CPU_INFO_FAMILY: return "Texas Instruments TMS32010";
		case CPU_INFO_VERSION: return "1.20";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C)1999-2002+ by Tony La Porta";
		case CPU_INFO_REG_LAYOUT: return (const char*)tms32010_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)tms32010_win_layout;
	}
	return buffer[which];
}

unsigned tms32010_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm32010( buffer, pc );
#else
	sprintf( buffer, "$%04X", TMS32010_RDOP(pc) );
	return 2;
#endif
}
