 /**************************************************************************\
 *				  Texas Instruments TMS320x25 DSP Emulator					*
 *																			*
 *				   Copyright (C) 2001-2002+ Tony La Porta					*
 *						Written for the MAME project.						*
 *																			*
 *																			*
 *	Three versions of the chip are available, and they are: 				*
 *	TMS320C25   Internal ROM one time programmed at TI  					*
 *	TMS320E25   Internal ROM programmable as a normal EPROM 				*
 *	TMS320P25   Internal ROM programmable once as a normal EPROM only		*
 *	These devices can also be used as a MicroController with external ROM	*
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
 *	TLP (2x-May-2001)														*
 *	 - Work began on this emulator  										*
 *	TLP (12-Jul-2001)														*
 *	 - First private release												*
 *	TLP (xx-Dec-2001) Ver 0.11  											*
 *	 - Various undocumented fixes											*
 *	TLP (13-Jul-2002) Ver 0.12  											*
 *	 - Corrected IRQ2 vector pointer										*
 *	 - Fixed the signedness in many equation based instructions				*
 *	 - Adjusted the level sensing for the Signal inputs 					*
 *	 - Added the ability to view the CPU in the debugger when it's halted	*
 *	TLP (16-Nov-2002)														*
 *	 - First public release after nearly 1.5 years! 						*
 *	 - Adjusted more signedness instructions (ADDH, SUBC, SUBH, etc)		*
 *	TLP (21-Dec-2002)														*
 *	 - Added memory banking for the CNFD, CNFP and CONF instructions		*
 *	 - Corrected IRQ masking checks 										*
 *	TLP (25-Dec-2002) Ver 1.10  											*
 *	 - Added internal timer													*
 *																			*
 \**************************************************************************/

/*****************************************************************************
 To fix, or currently lacking from this emulator are:

 Fix the levels for S_IN and S_OUT - use assert/release line

 #	Support for the built in Timer/Counter Page 91
  	When idling, Counter must still be activly counting down. When counter
  	reaches 0 it should issue a TINT (if it's not masked), then come out of
  	IDLE mode.
  	If TINT is masked, the Timer still needs to count down.

 #	Support for the built in Serial Port
 #	Support for the Global memory register
 #	Support for the switch for RAM block 0 banking between RAM and ROM space
 #	Correct the mulit-cycle instruction cycle counts
 #	Add support to set ROM & RAM as Internal/External in order to correctly
  	compute cycle timings
 #	Possibly add internal memory into here (instead of having it in the driver)
 #	Check (read) Hold signal level during execution loop ?
 #	Fix bugs
 #	Fix more bugs :-)
 #	Add/fix other things I forgot
*****************************************************************************/

/*
     TMS32025 CONF Mode Decoding Table
|=======================================|
| Status bit |           Blocks         |
|     CNF    |   B0    |   B1    |  B2  |
|------------+---------+---------+------|
|     0  0   |  data   |  data   | data |
|     1  1   | program |  data   | data |
|=======================================|


     TMS32026 CONF Mode Decoding Table
|==================================================|
| Status bits |               Blocks               |
| CNF1 | CNF0 |   B0    |   B1    |  B2  |   B3    |
|------+------+---------+---------+------+---------|
|  0   |  0   |  data   |  data   | data |  data   |
|  0   |  1   | program |  data   | data |  data   |
|  1   |  0   | program | program | data |  data   |
|  1   |  1   | program | program | data | program |
|==================================================|



Table 3-2.  TMS32025/26 Memory Blocks
|=========================================================|
|             Configured As Data Memory                   |
|-------+-------TMS320C25--------+-------TMS320C26--------|
|       |         | Hexadecimal  |         | Hexadecimal  |
| Block |  Pages  |   Address    |  Pages  |   Address    |
|-------+---------+--------------+---------+--------------|
|   B2  |    0    | 0060h-007Fh  |    0    | 0060h-007Fh  |
|   B0  |   4-5   | 0200h-02FFh  |   4-7   | 0200h-03FFh  |
|   B1  |   6-7   | 0300h-03FFh  |   8-11  | 0400h-05FFh  |
|   B3  |   B3 does not exist    |  12-15  | 0600h-07FFh  |
|=========================================================|
|             Configured As Program Memory                |
|-------+-------TMS320C25--------+-------TMS320C26--------|
|       |         | Hexadecimal  |         | Hexadecimal  |
| Block |  Pages  |   Address    |  Pages  |   Address    |
|-------+---------+--------------+---------+--------------|
|   B2  | B2 is not configurable | B2 is not configurable |
|   B0  | 510-511 | FF00h-FFFFh  | 500-503 | FA00h-FBFFh  |
|   B1  | B1 is not configurable | 504-507 | FC00h-FDFFh  |
|   B3  | B3 does not exist      | 508-511 | FE00h-FFFFh  |
|=========================================================|
*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <retro_inline.h>

#include "driver.h"
#include "cpuintrf.h"
#include "mamedbg.h"
#include "state.h"
#include "tms32025.h"


#define CLK 4	/* 1 cycle equals 4 clock ticks */		/* PE/DI */


#define M_RDROM(A)		TMS32025_ROM_RDMEM(A)
#define M_WRTROM(A,V)	TMS32025_ROM_WRMEM(A,V)
#define M_RDRAM(A)		TMS32025_RAM_RDMEM(A)
#define M_WRTRAM(A,V)	TMS32025_RAM_WRMEM(A,V)
#define M_RDOP(A)		TMS32025_RDOP(A)
#define M_RDOP_ARG(A)	TMS32025_RDOP_ARG(A)
#define P_IN(A)			TMS32025_In(A)
#define P_OUT(A,V)		TMS32025_Out(A,V)
#define S_IN(A)			TMS32025_Signal_In(A)
#define S_OUT(A,V)		TMS32025_Signal_Out(A,V)



static UINT8 tms32025_reg_layout[] = {
	TMS32025_PC,  TMS32025_STR0,TMS32025_STR1,TMS32025_IFR, TMS32025_STK7,TMS32025_STK6,-1,
	TMS32025_STK5,TMS32025_STK4,TMS32025_STK3,TMS32025_STK2,TMS32025_STK1,TMS32025_STK0,-1,
	TMS32025_ACC, TMS32025_PREG,TMS32025_TREG,TMS32025_RPTC,TMS32025_AR0, TMS32025_AR1, -1,
	TMS32025_AR2, TMS32025_AR3, TMS32025_AR4, TMS32025_AR5, TMS32025_AR6, TMS32025_AR7, -1,
	TMS32025_DRR, TMS32025_DXR, TMS32025_TIM, TMS32025_PRD, TMS32025_IMR, TMS32025_GREG, 0
};

static UINT8 tms32025_win_layout[] = {
	 0, 0,80, 5,	/* register window (top rows) */
	 0, 6,29,16,	/* disassembler window (left colums) */
	30, 6,50, 7,	/* memory #1 window (right, upper middle) */
	30,14,50, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};




typedef struct			/* Page 3-6 (45) shows all registers */
{
	/******************** CPU Internal Registers *******************/
	UINT16	PREVPC;		/* previous program counter */
	UINT16	PC;
	UINT16	PFC;
	UINT16	STR0, STR1;
	UINT8	IFR;
	UINT8	RPTC;
	PAIR	ACC;		/* PAIR defined in os/osd_cpu.h */
	PAIR	Preg;
	UINT16	Treg;
	UINT16	AR[8];
	UINT16	STACK[8];
	PAIR	ALU;
	UINT16	*intRAM;

	/********************** Status data ****************************/
	PAIR	opcode;
	int		idle;
	int		hold;
	int		external_mem_access;	/** required for hold mode. Implement it ! */
	int		init_load_addr;			/* 0=No, 1=Yes, 2=Once for repeat mode */
	int		tms32025_irq_cycles;
	int		tms32025_dec_cycles;
	int		(*irq_callback)(int irqline);
} tms32025_Regs;

static tms32025_Regs R;
static PAIR  oldacc;
static UINT32 memaccess;
int    tms32025_icount;
offs_t TMS32025_DATA_BANK[0x10];
offs_t TMS32025_PRGM_BANK[0x10];
typedef void (*opcode_fn) (void);


/************************** Memory mapped registers ****************/
#define DRR 	R.intRAM[0]
#define DXR 	R.intRAM[1]
#define TIM 	R.intRAM[2]
#define PRD 	R.intRAM[3]
#define IMR 	R.intRAM[4]
#define GREG	R.intRAM[5]



/****************************************************************************
 *******  The following is the Status (Flag) register 0 definition.  ********
| 15 | 14 | 13 | 12 |  11 | 10 |   9  | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
| <----ARP---> | OV | OVM |  1 | INTM | <--------------DP---------------> | */

#define ARP_REG		0xe000	/* ARP	(Auxiliary Register Pointer) */
#define OV_FLAG		0x1000	/* OV	(Overflow flag) 1 indicates an overflow */
#define OVM_FLAG	0x0800	/* OVM	(Overflow Mode bit) 1 forces ACC overflow to greatest positive or negative saturation value */
#define INTM_FLAG	0x0200	/* INTM	(Interrupt Mask flag) 0 enables maskable interrupts */
#define DP_REG		0x01ff	/* DP	(Data bank memory Pointer) */


/***********************************************************************************
 *** The following is the Status (Flag) register 1 definition for TMS32025. ********
| 15 | 14 | 13 |  12  | 11 |  10 | 9 | 8 | 7 |  6 |  5  |  4 |  3 |  2  | 1 | 0  |
| <----ARB---> | CNF0 | TC | SXM | C | 1 | 1 | HM | FSM | XF | FO | TXM | <-PM-> | */

/*** The following is the Status (Flag) register 1 definition for TMS32026. ***********
| 15 | 14 | 13 |  12  | 11 |  10 | 9 | 8 |   7  |  6 |  5  |  4 |  3 |  2  | 1 | 0  |
| <----ARB---> | CNF0 | TC | SXM | C | 1 | CNF1 | HM | FSM | XF | FO | TXM | <-PM-> | */

#define ARB_REG		0xe000	/* ARB	(Auxiliary Register pointer Backup) */
#define CNF0_REG	0x1000	/* CNF0	(Onchip RAM CoNFiguration) 0 means B0=data memory, 1means B0=program memory */
#define CNF1_REG	0x0080	/* CNF1	(Onchip RAM CoNFiguration) 0 means B0=data memory, 1means B0=program memory */
#define TC_FLAG		0x0800	/* TC	(Test Control flag) */
#define SXM_FLAG	0x0400	/* SXM	(Sign eXtension Mode) */
#define C_FLAG		0x0200	/* C	(Carry flag) */
#define HM_FLAG		0x0040	/* HM	(Processor Hold Mode) */
#define FSM_FLAG	0x0020	/* FSM	(Frame Synchronization Mode - for serial port) */
#define XF_FLAG		0x0010	/* XF	(XF output pin status) */
#define FO_FLAG		0x0008	/* FO	(Serial port Format In/Out mode) */
#define TXM_FLAG	0x0004	/* TXM	(Transmit Mode - for serial port) */
#define PM_REG		0x0003	/* PM	(Product shift Mode) */


#define OV		( R.STR0 & OV_FLAG)			/* OV	(Overflow flag) */
#define OVM		( R.STR0 & OVM_FLAG)		/* OVM	(Overflow Mode bit) 1 indicates an overflow */
#define INTM	( R.STR0 & INTM_FLAG)		/* INTM	(Interrupt enable flag) 0 enables maskable interrupts */
#define ARP		((R.STR0 & ARP_REG) >> 13)	/* ARP	(Auxiliary Register Pointer) */
#define DP		((R.STR0 & DP_REG) << 7)	/* DP	(Data memory Pointer bit) */
#define ARB		( R.STR1 & ARB_REG)			/* ARB	(Backup Auxiliary Register pointer) */
#define CNF0	( R.STR1 & CNF0_REG)		/* CNF0	(Onchip Ram Config register) */
#define TC		( R.STR1 & TC_FLAG)			/* TC	(Test Control Flag) */
#define SXM		( R.STR1 & SXM_FLAG)		/* SXM	(Sign Extension Mode) */
#define CARRY	( R.STR1 & C_FLAG)			/* C	(Carry Flag for accumulator) */
#define HM		( R.STR1 & HM_FLAG)			/* HM	(Processor Hold Mode) */
#define FSM		( R.STR1 & FSM_FLAG)		/* FSM	(Frame Synchronization Mode - for serial port) */
#define XF		( R.STR1 & FSM_FLAG)		/* XF	(XF output pin status) */
#define FO		( R.STR1 & FO_FLAG)			/* FO	(Serial port Format In/Out mode) */
#define TXM		( R.STR1 & TXM_FLAG)		/* TXM	(Transmit Mode - for serial port) */
#define PM		( R.STR1 & PM_REG)			/* PM	(P register shift Mode. See SHIFT_Preg_TO_ALU below )*/

#define DMA		(DP | (R.opcode.b.l & 0x7f))	/* address used in direct memory access operations */
#define DMApg0	(R.opcode.b.l & 0x7f)			/* address used in direct memory access operations for sst instruction */
#define IND		R.AR[ARP]						/* address used in indirect memory access operations */



static INLINE void CLR0(UINT16 flag) { R.STR0 &= ~flag; R.STR0 |= 0x0400; }
static INLINE void SET0(UINT16 flag) { R.STR0 |=  flag; R.STR0 |= 0x0400; }
static INLINE void CLR1(UINT16 flag) { R.STR1 &= ~flag; R.STR1 |= 0x0180; }
static INLINE void SET1(UINT16 flag) { R.STR1 |=  flag; R.STR1 |= 0x0180; }
static INLINE void MODIFY_DP (int data) { R.STR0 &= ~DP_REG;  R.STR0 |= (data & DP_REG); R.STR0 |= 0x0400; }
static INLINE void MODIFY_PM (int data) { R.STR1 &= ~PM_REG;  R.STR1 |= (data & PM_REG); R.STR1 |= 0x0180; }
static INLINE void MODIFY_ARP(int data) { R.STR1 &= ~ARB_REG; R.STR1 |= (R.STR0 & ARP_REG); R.STR1 |= 0x0180; \
								   R.STR0 &= ~ARP_REG; R.STR0 |= ((data << 13) & ARP_REG); R.STR0 |= 0x0400; }
static INLINE void MODIFY_ARB(int data) { R.STR1 &= ~ARB_REG; R.STR1 |= ((data << 13) & ARB_REG); R.STR1 |= 0x0180; }

static INLINE void MODIFY_AR_ARP(void)
{
	switch (R.opcode.b.l & 0x70)		/* Cases ordered by predicted useage */
	{
		case 0x00:		break;
		case 0x10:		R.AR[ARP] -- ; break;
		case 0x20:		R.AR[ARP] ++ ; break;
		case 0x50:		R.AR[ARP] -= R.AR[0]; break;
		case 0x60:		R.AR[ARP] += R.AR[0]; break;
		case 0x40:		R.AR[ARP] -= (R.AR[0]>>1); break;	/* reverse carry propogation */
		case 0x70:		R.AR[ARP] += (R.AR[0]>>1); break;	/* reverse carry propogation */
		case 0x30:		break;	/* Reserved. Lets use it for LAR instruction */
		default:		break;
	}
	if (R.opcode.b.l & 8)
	{
/*		MODIFY_ARB(ARP);*/
		MODIFY_ARP(R.opcode.b.l & 7);
	}
}

static INLINE void CALCULATE_ADD_CARRY(void)
{
	if ( ((INT32)(oldacc.d) < 0) && ((INT32)(R.ACC.d) >= 0) ) {
		SET1(C_FLAG);
	}
	else {
		CLR1(C_FLAG);
	}
}

static INLINE void CALCULATE_SUB_CARRY(void)
{
	if ( ((INT32)(oldacc.d) >= 0) && ((INT32)(R.ACC.d) < 0) ) {
		CLR1(C_FLAG);
	}
	else {
		SET1(C_FLAG);
	}
}

static INLINE void CALCULATE_ADD_OVERFLOW(INT32 addval)
{
	if ((INT32)(~(oldacc.d ^ addval) & (oldacc.d ^ R.ACC.d)) < 0) {
		SET0(OV_FLAG);
		if (OVM)
			R.ACC.d = ((INT32)oldacc.d < 0) ? 0x80000000 : 0x7fffffff;
	}
}
static INLINE void CALCULATE_SUB_OVERFLOW(INT32 subval)
{
	if ((INT32)((oldacc.d ^ subval) & (oldacc.d ^ R.ACC.d)) < 0) {
		SET0(OV_FLAG);
		if (OVM)
			R.ACC.d = ((INT32)oldacc.d < 0) ? 0x80000000 : 0x7fffffff;
	}
}

static INLINE UINT16 POP_STACK(void)
{
	UINT16 data = R.STACK[7];
	R.STACK[7] = R.STACK[6];
	R.STACK[6] = R.STACK[5];
	R.STACK[5] = R.STACK[4];
	R.STACK[4] = R.STACK[3];
	R.STACK[3] = R.STACK[2];
	R.STACK[2] = R.STACK[1];
	R.STACK[1] = R.STACK[0];
	return data;
}
static INLINE void PUSH_STACK(UINT16 data)
{
	R.STACK[0] = R.STACK[1];
	R.STACK[1] = R.STACK[2];
	R.STACK[2] = R.STACK[3];
	R.STACK[3] = R.STACK[4];
	R.STACK[4] = R.STACK[5];
	R.STACK[5] = R.STACK[6];
	R.STACK[6] = R.STACK[7];
	R.STACK[7] = data;
}

static INLINE void SHIFT_Preg_TO_ALU(void)
{
	switch(PM)		/* PM (in STR1) is the shift mode for Preg */
	{
		case 0:		R.ALU.d = R.Preg.d; break;
		case 1:		R.ALU.d = (R.Preg.d << 1); break;
		case 2:		R.ALU.d = (R.Preg.d << 4); break;
		case 3:		R.ALU.d = (R.Preg.d >> 6); if (R.Preg.d & 0x80000000) R.ALU.d |= 0xfc000000; break;
		default:	break;
	}
}




static INLINE void GETDATA(int shift,int signext)
{
	if (R.opcode.b.l & 0x80) memaccess = IND;
	else memaccess = DMA;

	if (memaccess >= 0x800) R.external_mem_access = 1;	/* Pause if hold pin is active */
	else R.external_mem_access = 0;

	R.ALU.d = (UINT16)M_RDRAM(memaccess);
	if (signext) R.ALU.d = (INT16)R.ALU.d;
	R.ALU.d <<= shift;

	if (R.opcode.b.l & 0x80) MODIFY_AR_ARP();
}
static INLINE void PUTDATA(UINT16 data)
{
	if (R.opcode.b.l & 0x80) {
		if (memaccess >= 0x800) R.external_mem_access = 1;	/* Pause if hold pin is active */
		else R.external_mem_access = 0;

		M_WRTRAM(IND,data);
		MODIFY_AR_ARP();
	}
	else {
		if (memaccess >= 0x800) R.external_mem_access = 1;	/* Pause if hold pin is active */
		else R.external_mem_access = 0;

		M_WRTRAM(DMA,data);
	}
}
static INLINE void PUTDATA_SST(UINT16 data)
{
	if (R.opcode.b.l & 0x80) memaccess = IND;
	else memaccess = DMApg0;

	if (memaccess >= 0x800) R.external_mem_access = 1;		/* Pause if hold pin is active */
	else R.external_mem_access = 0;

	if (R.opcode.b.l & 0x80) {
		R.opcode.b.l &= 0xf7;					/* Stop ARP changes */
		MODIFY_AR_ARP();
	}
	M_WRTRAM(memaccess,data);
}




/* The following functions are here to fill the void for the */
/* opcode call functions. These functions are never actually called. */
static void opcodes_CE(void) { }
static void opcodes_DX(void) { }

static void illegal(void)
{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  PC = %04x,  Illegal opcode = %04x\n", (R.PC-1), R.opcode.w.l);
}

static void abst(void)
{
		if ( (INT32)(R.ACC.d) < 0 ) {
			R.ACC.d = -R.ACC.d;
			if (OVM) {
				SET0(OV_FLAG);
				if (R.ACC.d == 0x80000000) R.ACC.d-- ;
			}
		}
		CLR1(C_FLAG);
}
static void add(void)		/* #### add carry support - see page 3-31 (70) #### */
{							/* page 10-13 (348) spru031d */
		oldacc.d = R.ACC.d;
		GETDATA((R.opcode.b.h & 0xf),SXM);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void addc(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		if (CARRY) R.ALU.d++;
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void addh(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.ACC.w.h += R.ALU.w.l;
		if ((INT16)(~(oldacc.w.h ^ R.ALU.w.l) & (oldacc.w.h ^ R.ACC.w.h)) < 0) {
			SET0(OV_FLAG);
			if (OVM)
				R.ACC.w.h = ((INT16)oldacc.w.h < 0) ? 0x8000 : 0x7fff;
		}
		if ( ((INT16)(oldacc.w.h) < 0) && ((INT16)(R.ACC.w.h) >= 0) ) {
			SET1(C_FLAG);
		}
		/* Carry flag is not cleared, if no carry occured */
}
static void addk(void)
{
		oldacc.d = R.ACC.d;
		R.ALU.d = (UINT8)R.opcode.b.l;
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void adds(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void addt(void)
{
		oldacc.d = R.ACC.d;
		GETDATA((R.Treg & 0xf),SXM);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void adlk(void)
{
		oldacc.d = R.ACC.d;
		if (SXM) R.ALU.d =  (INT16)M_RDOP_ARG(R.PC);
		else     R.ALU.d = (UINT16)M_RDOP_ARG(R.PC);
		R.PC++;
		R.ALU.d <<= (R.opcode.b.h & 0xf);
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void adrk(void)
{
		R.AR[ARP] += R.opcode.b.l;
}
static void and(void)
{
		GETDATA(0,0);
		R.ACC.d &= R.ALU.d;
}
static void andk(void)
{
		oldacc.d = R.ACC.d;
		R.ALU.d = (UINT16)M_RDOP_ARG(R.PC);
		R.PC++;
		R.ALU.d <<= (R.opcode.b.h & 0xf);
		R.ACC.d &= R.ALU.d;
		R.ACC.d &= 0x7fffffff;
}
static void apac(void)
{
		oldacc.d = R.ACC.d;
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void br(void)
{
		R.PC = M_RDOP_ARG(R.PC);
		MODIFY_AR_ARP();
}
static void bacc(void)
{
		R.PC = R.ACC.w.l;
}
static void banz(void)
{
		if (R.AR[ARP]) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bbnz(void)
{
		if (TC) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bbz(void)
{
		if (TC == 0) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bc(void)
{
		if (CARRY) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bgez(void)
{
		if ( (INT32)(R.ACC.d) >= 0 ) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bgz(void)
{
		if ( (INT32)(R.ACC.d) > 0 ) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bioz(void)
{
		if (S_IN(TMS32025_BIO) != CLEAR_LINE) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bit(void)
{
		GETDATA(0,0);
		if (R.ALU.d & (0x8000 >> (R.opcode.b.h & 0xf))) SET1(TC_FLAG);
		else CLR1(TC_FLAG);
}
static void bitt(void)
{
		GETDATA(0,0);
		if (R.ALU.d & (0x8000 >> (R.Treg & 0xf))) SET1(TC_FLAG);
		else CLR1(TC_FLAG);
}
static void blez(void)
{
		if ( (INT32)(R.ACC.d) <= 0 ) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void blkd(void)
{										/** Fix cycle timing **/
		if (R.init_load_addr) {
			R.PFC = M_RDOP_ARG(R.PC);
			R.PC++;
		}
		R.ALU.d = M_RDRAM(R.PFC);
		PUTDATA(R.ALU.d);
		R.PFC++;
		tms32025_icount -= (1*CLK);
}
static void blkp(void)
{										/** Fix cycle timing **/
		if (R.init_load_addr) {
			R.PFC = M_RDOP_ARG(R.PC);
			R.PC++;
		}
		R.ALU.d = M_RDROM(R.PFC);
		PUTDATA(R.ALU.d);
		R.PFC++;
		tms32025_icount -= (2*CLK);
}
static void blz(void)
{
		if ( (INT32)(R.ACC.d) <  0 ) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bnc(void)
{
		if (CARRY == 0) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bnv(void)
{
		if (OV == 0) R.PC = M_RDOP_ARG(R.PC);
		else {
			R.PC++ ;
			CLR0(OV_FLAG);
		}
		MODIFY_AR_ARP();
}
static void bnz(void)
{
		if (R.ACC.d != 0) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bv(void)
{
		if (OV) {
			R.PC = M_RDOP_ARG(R.PC);
			CLR0(OV_FLAG);
		}
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void bz(void)
{
		if (R.ACC.d == 0) R.PC = M_RDOP_ARG(R.PC);
		else R.PC++ ;
		MODIFY_AR_ARP();
}
static void cala(void)
{
		PUSH_STACK(R.PC);
		R.PC = R.ACC.w.l;
}
static void call(void)
{
		R.PC++ ;
		PUSH_STACK(R.PC);
		R.PC = M_RDOP_ARG((R.PC - 1));
		MODIFY_AR_ARP();
}
static void cmpl(void)
{
		R.ACC.d = (~R.ACC.d);
}
static void cmpr(void)
{
		switch (R.opcode.b.l & 3)
		{
			case 00:	if ( (UINT16)(R.AR[ARP]) == (UINT16)(R.AR[0]) ) SET1(TC_FLAG); else CLR1(TC_FLAG); break;
			case 01:	if ( (UINT16)(R.AR[ARP]) <  (UINT16)(R.AR[0]) ) SET1(TC_FLAG); else CLR1(TC_FLAG); break;
			case 02:	if ( (UINT16)(R.AR[ARP])  > (UINT16)(R.AR[0]) ) SET1(TC_FLAG); else CLR1(TC_FLAG); break;
			case 03:	if ( (UINT16)(R.AR[ARP]) != (UINT16)(R.AR[0]) ) SET1(TC_FLAG); else CLR1(TC_FLAG); break;
			default:	break;
		}
}
static void cnfd(void)	/** next two fetches need to use previous CNF value ! **/
{
		CLR1(CNF0_REG);
		TMS32025_DATA_BANK[0x2] = TMS32025_DATA_OFFSET + 0x0200;
		TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0f00;
}
static void cnfp(void)	/** next two fetches need to use previous CNF value ! **/
{
		SET1(CNF0_REG);
		TMS32025_DATA_BANK[0x2] = TMS32025_DATA_OFFSET + 0xff00;
		TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0200;
}
static void conf(void)	/** Need to reconfigure the memory blocks */
{
		switch (R.opcode.b.l & 3)
		{
			case 00:	CLR1(CNF1_REG); CLR1(CNF0_REG);
						TMS32025_DATA_BANK[0x2] = TMS32025_DATA_OFFSET + 0x0200;
						TMS32025_DATA_BANK[0x3] = TMS32025_DATA_OFFSET + 0x0300;
						TMS32025_DATA_BANK[0x4] = TMS32025_DATA_OFFSET + 0x0400;
						TMS32025_DATA_BANK[0x5] = TMS32025_DATA_OFFSET + 0x0500;
						TMS32025_DATA_BANK[0x6] = TMS32025_DATA_OFFSET + 0x0600;
						TMS32025_DATA_BANK[0x7] = TMS32025_DATA_OFFSET + 0x0700;
						TMS32025_PRGM_BANK[0xa] = TMS32025_PGM_OFFSET + 0x0a00;
						TMS32025_PRGM_BANK[0xb] = TMS32025_PGM_OFFSET + 0x0b00;
						TMS32025_PRGM_BANK[0xc] = TMS32025_PGM_OFFSET + 0x0c00;
						TMS32025_PRGM_BANK[0xd] = TMS32025_PGM_OFFSET + 0x0d00;
						TMS32025_PRGM_BANK[0xe] = TMS32025_PGM_OFFSET + 0x0e00;
						TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0f00;
						break;
			case 01:	CLR1(CNF1_REG); SET1(CNF0_REG);
						TMS32025_DATA_BANK[0x2] = TMS32025_PGM_OFFSET + 0xfa00;
						TMS32025_DATA_BANK[0x3] = TMS32025_PGM_OFFSET + 0xfb00;
						TMS32025_DATA_BANK[0x4] = TMS32025_PGM_OFFSET + 0x0400;
						TMS32025_DATA_BANK[0x5] = TMS32025_PGM_OFFSET + 0x0500;
						TMS32025_DATA_BANK[0x6] = TMS32025_DATA_OFFSET + 0x0600;
						TMS32025_DATA_BANK[0x7] = TMS32025_DATA_OFFSET + 0x0700;
						TMS32025_PRGM_BANK[0xa] = TMS32025_DATA_OFFSET + 0x0200;
						TMS32025_PRGM_BANK[0xb] = TMS32025_DATA_OFFSET + 0x0300;
						TMS32025_PRGM_BANK[0xc] = TMS32025_DATA_OFFSET + 0x0c00;
						TMS32025_PRGM_BANK[0xd] = TMS32025_DATA_OFFSET + 0x0d00;
						TMS32025_PRGM_BANK[0xe] = TMS32025_PGM_OFFSET + 0x0e00;
						TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0f00;
						break;
			case 02:	SET1(CNF1_REG); CLR1(CNF0_REG);
						TMS32025_DATA_BANK[0x2] = TMS32025_PGM_OFFSET + 0xfa00;
						TMS32025_DATA_BANK[0x3] = TMS32025_PGM_OFFSET + 0xfb00;
						TMS32025_DATA_BANK[0x4] = TMS32025_PGM_OFFSET + 0xfc00;
						TMS32025_DATA_BANK[0x5] = TMS32025_PGM_OFFSET + 0xfd00;
						TMS32025_DATA_BANK[0x6] = TMS32025_DATA_OFFSET + 0x0600;
						TMS32025_DATA_BANK[0x7] = TMS32025_DATA_OFFSET + 0x0700;
						TMS32025_PRGM_BANK[0xa] = TMS32025_DATA_OFFSET + 0x0200;
						TMS32025_PRGM_BANK[0xb] = TMS32025_DATA_OFFSET + 0x0300;
						TMS32025_PRGM_BANK[0xc] = TMS32025_DATA_OFFSET + 0x0400;
						TMS32025_PRGM_BANK[0xd] = TMS32025_DATA_OFFSET + 0x0500;
						TMS32025_PRGM_BANK[0xe] = TMS32025_PGM_OFFSET + 0x0e00;
						TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0f00;
						break;
			case 03:	SET1(CNF1_REG); SET1(CNF0_REG);
						TMS32025_DATA_BANK[0x2] = TMS32025_PGM_OFFSET + 0xfa00;
						TMS32025_DATA_BANK[0x3] = TMS32025_PGM_OFFSET + 0xfb00;
						TMS32025_DATA_BANK[0x4] = TMS32025_PGM_OFFSET + 0xfc00;
						TMS32025_DATA_BANK[0x5] = TMS32025_PGM_OFFSET + 0xfd00;
						TMS32025_DATA_BANK[0x6] = TMS32025_DATA_OFFSET + 0xfe00;
						TMS32025_DATA_BANK[0x7] = TMS32025_DATA_OFFSET + 0xff00;
						TMS32025_PRGM_BANK[0xa] = TMS32025_DATA_OFFSET + 0x0200;
						TMS32025_PRGM_BANK[0xb] = TMS32025_DATA_OFFSET + 0x0300;
						TMS32025_PRGM_BANK[0xc] = TMS32025_DATA_OFFSET + 0x0400;
						TMS32025_PRGM_BANK[0xd] = TMS32025_DATA_OFFSET + 0x0500;
						TMS32025_PRGM_BANK[0xe] = TMS32025_PGM_OFFSET + 0x0600;
						TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0700;
						break;
			default:	break;
		}
}
static void dint(void)
{
		SET0(INTM_FLAG);
}
static void dmov(void)	/** Careful with how memory is configured !! */
{
		GETDATA(0,0);
		M_WRTRAM((memaccess + 1),R.ALU.w.l);
}
static void eint(void)
{
		CLR0(INTM_FLAG);
}
static void fort(void)
{
		if (R.opcode.b.l & 1) SET1(FO_FLAG);
		else CLR1(FO_FLAG);
}
static void idle(void)
{
		CLR0(INTM_FLAG);
		R.idle = 1;
}
static void in(void)
{
		R.ALU.w.l = P_IN( (R.opcode.b.h & 0xf) );
		PUTDATA(R.ALU.w.l);
}
static void lac(void)
{
		GETDATA( (R.opcode.b.h & 0xf),SXM );
		R.ACC.d = R.ALU.d;
}
static void lack(void)		/* ZAC is a subset of this instruction */
{
		R.ACC.d = (UINT8)R.opcode.b.l;
}
static void lact(void)
{
		GETDATA( (R.Treg & 0xf),SXM );
		R.ACC.d = R.ALU.d;
}
static void lalk(void)
{
		if (SXM) {
			R.ALU.d = (INT16)M_RDOP_ARG(R.PC);
			R.ACC.d = R.ALU.d << (R.opcode.b.h & 0xf);
		}
		else {
			R.ALU.d = (UINT16)M_RDOP_ARG(R.PC);
			R.ACC.d = R.ALU.d << (R.opcode.b.h & 0xf);
			R.ACC.d &= 0x7fffffff;
		}
		R.PC++;
}
static void lar_ar0(void)	{ GETDATA(0,0); R.AR[0] = R.ALU.w.l; }
static void lar_ar1(void)	{ GETDATA(0,0); R.AR[1] = R.ALU.w.l; }
static void lar_ar2(void)	{ GETDATA(0,0); R.AR[2] = R.ALU.w.l; }
static void lar_ar3(void)	{ GETDATA(0,0); R.AR[3] = R.ALU.w.l; }
static void lar_ar4(void)	{ GETDATA(0,0); R.AR[4] = R.ALU.w.l; }
static void lar_ar5(void)	{ GETDATA(0,0); R.AR[5] = R.ALU.w.l; }
static void lar_ar6(void)	{ GETDATA(0,0); R.AR[6] = R.ALU.w.l; }
static void lar_ar7(void)	{ GETDATA(0,0); R.AR[7] = R.ALU.w.l; }
static void lark_ar0(void)	{ R.AR[0] = R.opcode.b.l; }
static void lark_ar1(void)	{ R.AR[1] = R.opcode.b.l; }
static void lark_ar2(void)	{ R.AR[2] = R.opcode.b.l; }
static void lark_ar3(void)	{ R.AR[3] = R.opcode.b.l; }
static void lark_ar4(void)	{ R.AR[4] = R.opcode.b.l; }
static void lark_ar5(void)	{ R.AR[5] = R.opcode.b.l; }
static void lark_ar6(void)	{ R.AR[6] = R.opcode.b.l; }
static void lark_ar7(void)	{ R.AR[7] = R.opcode.b.l; }
static void ldp(void)
{
		GETDATA(0,0);
		MODIFY_DP(R.ALU.d & 0x1ff);
}
static void ldpk(void)
{
		MODIFY_DP(R.opcode.b.l & 0x1ff);
}
static void lph(void)
{
		GETDATA(0,0);
		R.Preg.w.h = R.ALU.w.l;
}
static void lrlk(void)
{
		R.ALU.d = (UINT16)M_RDOP_ARG(R.PC);
		R.PC++;
		R.AR[R.opcode.b.h & 7] = R.ALU.w.l;
}
static void lst(void)
{
		R.opcode.b.l &= 0xf7;		/* Must ignore next ARP */
		GETDATA(0,0);
		R.ALU.w.l &= (~INTM_FLAG);
		R.STR0 &= INTM_FLAG;
		R.STR0 |= R.ALU.w.l;		/* Must not affect INTM */
		R.STR0 |= 0x0400;
}
static void lst1(void)
{
		R.opcode.b.l &= 0xf7;		/* Must ignore next ARP */
		GETDATA(0,0);
		R.STR1 = R.ALU.w.l;
		R.STR1 |= 0x0180;
		R.STR0 &= (~ARP_REG);		/* ARB also gets copied to ARP */
		R.STR0 |= (R.STR1 & ARB_REG);
}
static void lt(void)
{
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
}
static void lta(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void ltd(void)	/** Careful with how memory is configured !! */
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		M_WRTRAM((memaccess+1),R.ALU.w.l);
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
}
static void ltp(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		SHIFT_Preg_TO_ALU();
		R.ACC.d = R.ALU.d;
}
static void lts(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		SHIFT_Preg_TO_ALU();
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void mac(void)			/** RAM blocks B0,B1,B2 may be important ! */
{								/** Fix cycle timing **/
		oldacc.d = R.ACC.d;
		if (R.init_load_addr) {
			R.PFC = M_RDOP_ARG(R.PC);
			R.PC++;
		}
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		R.Preg.d = ( R.ALU.w.l * M_RDROM(R.PFC) );
		R.PFC++;
		tms32025_icount -= (2*CLK);
}
static void macd(void)			/** RAM blocks B0,B1,B2 may be important ! */
{								/** Fix cycle timing **/
		oldacc.d = R.ACC.d;
		if (R.init_load_addr) {
			R.PFC = M_RDOP_ARG(R.PC);
			R.PC++;
		}
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
		GETDATA(0,0);
		if ( (R.opcode.b.l & 0x80) || R.init_load_addr ) {	/* No writing during repitition, or DMA mode */
			M_WRTRAM((memaccess+1),R.ALU.w.l);
		}
		R.Treg = R.ALU.w.l;
		R.Preg.d = ( R.ALU.w.l * M_RDROM(R.PFC) );
		R.PFC++;
		tms32025_icount -= (2*CLK);
}
static void mar(void)		/* LARP and NOP are a subset of this instruction */
{
		if (R.opcode.b.l & 0x80) MODIFY_AR_ARP();
}
static void mpy(void)
{
		GETDATA(0,0);
		R.Preg.d = (INT16)(R.ALU.w.l) * (INT16)(R.Treg);
}
static void mpya(void)
{
		oldacc.d = R.ACC.d;
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
		GETDATA(0,0);
		R.Preg.d = (INT16)(R.ALU.w.l) * (INT16)(R.Treg);
}
static void mpyk(void)
{
		R.Preg.d = (INT16)R.Treg * ((INT16)(R.opcode.w.l << 3) >> 3);

}
static void mpys(void)
{
		oldacc.d = R.ACC.d;
		SHIFT_Preg_TO_ALU();
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
		GETDATA(0,0);
		R.Preg.d = (INT16)(R.ALU.w.l) * (INT16)(R.Treg);
}
static void mpyu(void)
{
		GETDATA(0,0); R.Preg.d = (UINT16)(R.ALU.w.l) * (UINT16)(R.Treg);
}
static void neg(void)
{
		if (R.ACC.d == 0x80000000) {
			SET0(OV_FLAG);
			if (OVM) R.ACC.d = 0x7fffffff;
		}
		else R.ACC.d = -R.ACC.d;
		if (R.ACC.d) CLR0(C_FLAG);
		else SET0(C_FLAG);
}
/*
static void nop(void) { }	// NOP is a subset of the MAR instruction
*/
static void norm(void)
{
		if (R.ACC.d == 0) {
			SET1(TC_FLAG);
		}
		else {
			if ( ((R.ACC.d & 0x80000000) ^ (R.ACC.d & 0x40000000)) ) {
				SET1(TC_FLAG);
			}
			else {
				CLR1(TC_FLAG);
				R.ACC.d <<= 1;
				MODIFY_AR_ARP();	/* ARP not changed in this instruction */
			}
		}
}
static void or(void)
{
		GETDATA(0,0);
		R.ACC.w.l |= R.ALU.w.l;
}
static void ork(void)
{
		R.ALU.d = (UINT16)M_RDOP_ARG(R.PC);
		R.PC++;
		R.ALU.d <<= (R.opcode.b.h & 0xf);
		R.ACC.d |=  (R.ALU.d & 0x7fffffff);
}
static void out(void)
{
		GETDATA(0,0);
		P_OUT( (R.opcode.b.h & 0xf), R.ALU.w.l );
}
static void pac(void)
{
		SHIFT_Preg_TO_ALU();
		R.ACC.d = R.ALU.d;
}
static void pop(void)
{
		R.ACC.d = (UINT16)POP_STACK();
}
static void popd(void)
{
		R.ALU.d = (UINT16)POP_STACK();
		PUTDATA(R.ALU.w.l);
}
static void pshd(void)
{
		GETDATA(0,0);
		PUSH_STACK(R.ALU.w.l);
}
static void push(void)
{
		PUSH_STACK(R.ACC.w.l);
}
static void rc(void)
{
		CLR1(C_FLAG);
}
static void ret(void)
{
		R.PC = POP_STACK();
}
static void rfsm(void)				/** serial port mode */
{
		CLR1(FSM_FLAG);
}
static void rhm(void)
{
		CLR1(HM_FLAG);
}
static void rol(void)
{
		R.ALU.d = R.ACC.d;
		R.ACC.d <<= 1;
		if (CARRY) R.ACC.d |= 1;
		if (R.ALU.d & 0x80000000) SET1(C_FLAG);
		else CLR1(C_FLAG);
}
static void ror(void)
{
		R.ALU.d = R.ACC.d;
		R.ACC.d >>= 1;
		if (CARRY) R.ACC.d |= 0x80000000;
		if (R.ALU.d & 1) SET1(C_FLAG);
		else CLR1(C_FLAG);
}
static void rovm(void)
{
		CLR0(OVM_FLAG);
}
static void rpt(void)
{
		GETDATA(0,0);
		R.RPTC = R.ALU.b.l;
		R.init_load_addr = 2;		/* Initiate repeat mode */
}
static void rptk(void)
{
		R.RPTC = R.opcode.b.l;
		R.init_load_addr = 2;		/* Initiate repeat mode */
}
static void rsxm(void)
{
		CLR1(SXM_FLAG);
}
static void rtc(void)
{
		CLR1(TC_FLAG);
}
static void rtxm(void)				/** serial port stuff */
{
		CLR1(TXM_FLAG);
}
static void rxf(void)
{
		CLR1(XF_FLAG);
		S_OUT(TMS32025_XF,CLEAR_LINE);
}
static void sach(void)
{
		R.ALU.d = (R.ACC.d << (R.opcode.b.h & 7));
		PUTDATA(R.ALU.w.h);
}
static void sacl(void)
{
		R.ALU.d = (R.ACC.d << (R.opcode.b.h & 7));
		PUTDATA(R.ALU.w.l);
}
static void sar_ar0(void)	{ PUTDATA(R.AR[0]); }
static void sar_ar1(void)	{ PUTDATA(R.AR[1]); }
static void sar_ar2(void)	{ PUTDATA(R.AR[2]); }
static void sar_ar3(void)	{ PUTDATA(R.AR[3]); }
static void sar_ar4(void)	{ PUTDATA(R.AR[4]); }
static void sar_ar5(void)	{ PUTDATA(R.AR[5]); }
static void sar_ar6(void)	{ PUTDATA(R.AR[6]); }
static void sar_ar7(void)	{ PUTDATA(R.AR[7]); }
static void sblk(void)
{
		oldacc.d = R.ACC.d;
		R.ALU.d = (M_RDOP_ARG(R.PC) << (R.opcode.b.h & 0xf));
		R.PC++;
		if (SXM && (R.ALU.d & 0x8000)) R.ALU.d = -R.ALU.d;
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void sbrk(void)
{
		R.AR[ARP] -= R.opcode.b.l;
}
static void sc(void)
{
		SET1(C_FLAG);
}
static void sfl(void)
{
		R.ALU.d = R.ACC.d;
		R.ACC.d <<= 1;
		if (R.ALU.d & 0x80000000) SET1(C_FLAG);
		else CLR1(C_FLAG);
}
static void sfr(void)
{
		R.ALU.d = R.ACC.d;
		R.ACC.d >>= 1;
		if (SXM) {
			if (R.ALU.d & 0x80000000) R.ACC.d |= 0x80000000;
		}
		if (R.ALU.d & 1) SET1(C_FLAG);
		else CLR1(C_FLAG);
}
static void sfsm(void)				/** serial port mode */
{
		SET1(FSM_FLAG);
}
static void shm(void)
{
		SET1(HM_FLAG);
}
static void sovm(void)
{
		SET0(OVM_FLAG);
}
static void spac(void)
{
		oldacc.d = R.ACC.d;
		SHIFT_Preg_TO_ALU();
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void sph(void)
{
		SHIFT_Preg_TO_ALU();
		PUTDATA(R.ALU.w.h);
}
static void spl(void)
{
		SHIFT_Preg_TO_ALU();
		PUTDATA(R.ALU.w.l);
}
static void spm(void)
{
		MODIFY_PM( (R.opcode.b.l & 3) );
}
static void sqra(void)
{
		oldacc.d = R.ACC.d;
		SHIFT_Preg_TO_ALU();
		R.ACC.d += R.ALU.d;
		CALCULATE_ADD_OVERFLOW(R.ALU.d);
		CALCULATE_ADD_CARRY();
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		R.Preg.d = (R.ALU.w.l * R.ALU.w.l);
}
static void sqrs(void)
{
		oldacc.d = R.ACC.d;
		SHIFT_Preg_TO_ALU();
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
		GETDATA(0,0);
		R.Treg = R.ALU.w.l;
		R.Preg.d = (R.ALU.w.l * R.ALU.w.l);
}
static void sst(void)
{
		PUTDATA_SST(R.STR0);
}
static void sst1(void)
{
		PUTDATA_SST(R.STR1);
}
static void ssxm(void)
{		/** Check instruction description, and make sure right instructions use SXM */
		SET1(SXM_FLAG);
}
static void stc(void)
{
		SET1(TC_FLAG);
}
static void stxm(void)				/** serial port stuff */
{
		SET1(TXM_FLAG);
}
static void sub(void)
{
		oldacc.d = R.ACC.d;
		GETDATA((R.opcode.b.h & 0xf),SXM);
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void subb(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		if (CARRY == 0) R.ALU.d--;
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void subc(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(15,0);
		R.ALU.d = R.ACC.d - R.ALU.d;
		if ((INT32)((oldacc.d ^ R.ALU.d) & (oldacc.d ^ R.ACC.d)) < 0) {
			SET0(OV_FLAG);			/* Not affected by OVM */
		}
		CALCULATE_SUB_CARRY();
		if ( (INT32)(R.ALU.d) >= 0 ) {
			R.ACC.d = ((R.ALU.d << 1) + 1);
		}
		else {
			R.ACC.d = (R.ACC.d << 1);
		}
}
static void subh(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.ACC.w.h -= R.ALU.w.l;
		if ((INT16)((oldacc.w.h ^ R.ALU.w.l) & (oldacc.w.h ^ R.ACC.w.h)) < 0) {
			SET0(OV_FLAG);
			if (OVM)
				R.ACC.w.h = ((INT16)oldacc.w.h < 0) ? 0x8000 : 0x7fff;
		}
		if ( ((INT16)(oldacc.w.h) >= 0) && ((INT16)(R.ACC.w.h) < 0) ) {
			CLR1(C_FLAG);
		}
		/* Carry flag is not affected, if no borrow occured */
}
static void subk(void)
{
		oldacc.d = R.ACC.d;
		R.ALU.d = (UINT8)R.opcode.b.l;
		R.ACC.d -= R.ALU.b.l;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void subs(void)
{
		oldacc.d = R.ACC.d;
		GETDATA(0,0);
		R.ACC.d -= R.ALU.w.l;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void subt(void)
{
		oldacc.d = R.ACC.d;
		GETDATA((R.Treg & 0xf),SXM);
		R.ACC.d -= R.ALU.d;
		CALCULATE_SUB_OVERFLOW(R.ALU.d);
		CALCULATE_SUB_CARRY();
}
static void sxf(void)
{
		SET1(XF_FLAG);
		S_OUT(TMS32025_XF,ASSERT_LINE);
}
static void tblr(void)
{
		if (R.init_load_addr) R.PFC = R.ACC.w.l;
		R.ALU.w.l = M_RDROM(R.PFC);
		if ( (CNF0) && ( (UINT16)(R.PFC) >= 0xff00 ) ) {}	/** TMS32025 only */
		else tms32025_icount -= (1*CLK);
		PUTDATA(R.ALU.w.l);
		R.PFC++;
}
static void tblw(void)
{
		if (R.init_load_addr) R.PFC = R.ACC.w.l;
		tms32025_icount -= (1*CLK);
		GETDATA(0,0);
		if (R.external_mem_access) tms32025_icount -= (1*CLK);
		M_WRTROM(R.PFC, R.ALU.w.l);
		R.PFC++;
}
static void trap(void)
{
		PUSH_STACK(R.PC);
		R.PC = 0x001E;		/* Trap vector */
}
static void xor(void)
{
		GETDATA(0,0);
		R.ACC.w.l ^= R.ALU.w.l;
}
static void xork(void)
{
		oldacc.d = R.ACC.d;
		R.ALU.d = M_RDOP_ARG(R.PC);
		R.PC++;
		R.ALU.d <<= (R.opcode.b.h & 0xf);
		R.ACC.d ^= R.ALU.d;
		R.ACC.d |= (oldacc.d & 0x80000000);
}
static void zalh(void)
{
		GETDATA(0,0);
		R.ACC.w.h = R.ALU.w.l;
		R.ACC.w.l = 0x0000;
}
static void zalr(void)
{
		GETDATA(0,0);
		R.ACC.w.h = R.ALU.w.l;
		R.ACC.w.l = 0x8000;
}
static void zals(void)
{
		GETDATA(0,0);
		R.ACC.w.l = R.ALU.w.l;
		R.ACC.w.h = 0x0000;
}


/***********************************************************************
 *	Cycle Timings
 ***********************************************************************/

static unsigned cycles_main[256]=
{
/*00*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*08*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*10*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*18*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*20*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*28*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*30*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*38*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*40*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*48*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*50*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*58*/		3*CLK, 2*CLK, 1*CLK, 1*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*60*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*68*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*70*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*78*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*80*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*88*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*90*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*98*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*A0*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*A8*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*B0*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*B8*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*C0*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*C8*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*D0*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 0*CLK,
/*D8*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*E0*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*E8*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*F0*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*F8*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK
};

static unsigned cycles_DX_subset[8]=
{
/*00*/		2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 0
};

static unsigned cycles_CE_subset[256]=
{
/*00*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*08*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*10*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 0*CLK,
/*18*/		1*CLK, 1*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 2*CLK, 3*CLK,
/*20*/		1*CLK, 1*CLK, 0*CLK, 1*CLK, 2*CLK, 2*CLK, 2*CLK, 1*CLK,
/*28*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*30*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*38*/		1*CLK, 1*CLK, 0*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*40*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*48*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*50*/		1*CLK, 1*CLK, 1*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*58*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*60*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*68*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*70*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*78*/		0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*80*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*88*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*90*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*98*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*A0*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*A8*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*B0*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*B8*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*C0*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*C8*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*D0*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*D8*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*E0*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*E8*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*F0*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK,
/*F8*/		0*CLK, 0*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK
};


/***********************************************************************
 *	Opcode Table
 ***********************************************************************/

static opcode_fn opcode_main[256]=
{
/*00*/ add,			add,		add,		add,		add,		add,		add,		add,
/*08*/ add,			add,		add,		add,		add,		add,		add,		add,
/*10*/ sub,			sub,		sub,		sub,		sub,		sub,		sub,		sub,
/*18*/ sub,			sub,		sub,		sub,		sub,		sub,		sub,		sub,
/*20*/ lac,			lac,		lac,		lac,		lac,		lac,		lac,		lac,
/*28*/ lac,			lac,		lac,		lac,		lac,		lac,		lac,		lac,
/*30*/ lar_ar0,		lar_ar1,	lar_ar2,	lar_ar3,	lar_ar4,	lar_ar5,	lar_ar6,	lar_ar7,
/*38*/ mpy,			sqra,		mpya,		mpys,		lt,			lta,		ltp,		ltd,
/*40*/ zalh,		zals,		lact,		addc,		subh,		subs,		subt,		subc,
/*48*/ addh,		adds,		addt,		rpt,		xor,		or,			and,		subb,
/*50*/ lst,			lst1,		ldp,		lph,		pshd,		mar,		dmov,		bitt,
/*58*/ tblr,		tblw,		sqrs,		lts,		macd,		mac,		bc,			bnc,
/*60*/ sacl,		sacl,		sacl,		sacl,		sacl,		sacl,		sacl,		sacl,
/*68*/ sach,		sach,		sach,		sach,		sach,		sach,		sach,		sach,
/*70*/ sar_ar0,		sar_ar1,	sar_ar2,	sar_ar3,	sar_ar4,	sar_ar5,	sar_ar6,	sar_ar7,
/*78*/ sst,			sst1,		popd,		zalr,		spl,		sph,		adrk,		sbrk,
/*80*/ in,			in,			in,			in,			in,			in,			in,			in,
/*88*/ in,			in,			in,			in,			in,			in,			in,			in,
/*90*/ bit,			bit,		bit,		bit,		bit,		bit,		bit,		bit,
/*98*/ bit,			bit,		bit,		bit,		bit,		bit,		bit,		bit,
/*A0*/ mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,
/*A8*/ mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,
/*B0*/ mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,
/*B8*/ mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,		mpyk,
/*C0*/ lark_ar0,	lark_ar1,	lark_ar2,	lark_ar3,	lark_ar4,	lark_ar5,	lark_ar6,	lark_ar7,
/*C8*/ ldpk,		ldpk,		lack,		rptk,		addk,		subk,		opcodes_CE,	mpyu,
/*D0*/ opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,
/*D8*/ opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,	opcodes_DX,
/*E0*/ out,			out,		out,		out,		out,		out,		out,		out,
/*E8*/ out,			out,		out,		out,		out,		out,		out,		out,
/*F0*/ bv,			bgz,		blez,		blz,		bgez,		bnz,		bz,			bnv,
/*F8*/ bbz,			bbnz,		bioz,		banz,		blkp,		blkd,		call,		br
};

static opcode_fn opcode_DX_subset[8]=	/* Instructions living under the Dxxx opcode */
{
/*00*/ lrlk,		lalk,		adlk,		sblk,		andk,		ork,		xork,		illegal
};

static opcode_fn opcode_CE_subset[256]=
{
/*00*/ eint,		dint,		rovm,		sovm,		cnfd,		cnfp,		rsxm,		ssxm,
/*08*/ spm,			spm,		spm,		spm,		rxf,		sxf,		fort,		fort,
/*10*/ illegal,		illegal,	illegal,	illegal,	pac,		apac,		spac,		illegal,
/*18*/ sfl,			sfr,		illegal,	abst,		push,		pop,		trap,		idle,
/*20*/ rtxm,		stxm,		illegal,	neg,		cala,		bacc,		ret,		cmpl,
/*28*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*30*/ rc,			sc,			rtc,		stc,		rol,		ror,		rfsm,		sfsm,
/*38*/ rhm,			shm,		illegal,	illegal,	conf,		conf,		conf,		conf,
/*40*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*48*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*50*/ cmpr,		cmpr,		cmpr,		cmpr,		illegal,	illegal,	illegal,	illegal,
/*58*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*60*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*68*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*70*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*78*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*80*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*88*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*90*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*98*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*A0*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*A8*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*B0*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*B8*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*C0*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*C8*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*D0*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*D8*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*E0*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*E8*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal,
/*F0*/ illegal,		illegal,	norm,		illegal,	illegal,	illegal,	illegal,	illegal,
/*F8*/ illegal,		illegal,	illegal,	illegal,	illegal,	illegal,	illegal,	illegal
};



/****************************************************************************
 *	Inits CPU emulation
 ****************************************************************************/
void tms32025_init (void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("tms32025", cpu, "PC", &R.PC, 1);
	state_save_register_UINT16("tms32025", cpu, "STR0", &R.STR0, 1);
	state_save_register_UINT16("tms32025", cpu, "STR1", &R.STR1, 1);
	state_save_register_UINT16("tms32025", cpu, "PFC", &R.PFC, 1);
	state_save_register_UINT8("tms32025", cpu, "IFR", &R.IFR, 1);
	state_save_register_UINT8("tms32025", cpu, "RPTC", &R.RPTC, 1);
	state_save_register_UINT32("tms32025", cpu, "ACC", &R.ACC.d, 1);
	state_save_register_UINT32("tms32025", cpu, "ALU", &R.ALU.d, 1);
	state_save_register_UINT32("tms32025", cpu, "Preg", &R.Preg.d, 1);
	state_save_register_UINT16("tms32025", cpu, "Treg", &R.Treg, 1);
	state_save_register_UINT16("tms32025", cpu, "AR0", &R.AR[0], 1);
	state_save_register_UINT16("tms32025", cpu, "AR1", &R.AR[1], 1);
	state_save_register_UINT16("tms32025", cpu, "AR2", &R.AR[2], 1);
	state_save_register_UINT16("tms32025", cpu, "AR3", &R.AR[3], 1);
	state_save_register_UINT16("tms32025", cpu, "AR4", &R.AR[4], 1);
	state_save_register_UINT16("tms32025", cpu, "AR5", &R.AR[5], 1);
	state_save_register_UINT16("tms32025", cpu, "AR6", &R.AR[6], 1);
	state_save_register_UINT16("tms32025", cpu, "AR7", &R.AR[7], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack0", &R.STACK[0], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack1", &R.STACK[1], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack2", &R.STACK[2], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack3", &R.STACK[3], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack4", &R.STACK[4], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack5", &R.STACK[5], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack6", &R.STACK[6], 1);
	state_save_register_UINT16("tms32025", cpu, "Stack7", &R.STACK[7], 1);

	state_save_register_INT32("tms32025", cpu, "idle", &R.idle, 1);
	state_save_register_INT32("tms32025", cpu, "hold", &R.hold, 1);
	state_save_register_INT32("tms32025", cpu, "external_mem_access", &R.external_mem_access, 1);
	state_save_register_INT32("tms32025", cpu, "init_load_addr", &R.init_load_addr, 1);
	state_save_register_UINT16("tms32025", cpu, "prevpc", &R.PREVPC, 1);

	state_save_register_UINT32("tms32025", cpu, "D_bank0", &TMS32025_DATA_BANK[0x0], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank1", &TMS32025_DATA_BANK[0x1], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank2", &TMS32025_DATA_BANK[0x2], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank3", &TMS32025_DATA_BANK[0x3], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank4", &TMS32025_DATA_BANK[0x4], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank5", &TMS32025_DATA_BANK[0x5], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank6", &TMS32025_DATA_BANK[0x6], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank7", &TMS32025_DATA_BANK[0x7], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank8", &TMS32025_DATA_BANK[0x8], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bank9", &TMS32025_DATA_BANK[0x9], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bankA", &TMS32025_DATA_BANK[0xa], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bankB", &TMS32025_DATA_BANK[0xb], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bankC", &TMS32025_DATA_BANK[0xc], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bankD", &TMS32025_DATA_BANK[0xd], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bankE", &TMS32025_DATA_BANK[0xe], 1);
	state_save_register_UINT32("tms32025", cpu, "D_bankF", &TMS32025_DATA_BANK[0xf], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank0", &TMS32025_PRGM_BANK[0x0], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank1", &TMS32025_PRGM_BANK[0x1], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank2", &TMS32025_PRGM_BANK[0x2], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank3", &TMS32025_PRGM_BANK[0x3], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank4", &TMS32025_PRGM_BANK[0x4], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank5", &TMS32025_PRGM_BANK[0x5], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank6", &TMS32025_PRGM_BANK[0x6], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank7", &TMS32025_PRGM_BANK[0x7], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank8", &TMS32025_PRGM_BANK[0x8], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bank9", &TMS32025_PRGM_BANK[0x9], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bankA", &TMS32025_PRGM_BANK[0xa], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bankB", &TMS32025_PRGM_BANK[0xb], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bankC", &TMS32025_PRGM_BANK[0xc], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bankD", &TMS32025_PRGM_BANK[0xd], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bankE", &TMS32025_PRGM_BANK[0xe], 1);
	state_save_register_UINT32("tms32025", cpu, "P_bankF", &TMS32025_PRGM_BANK[0xf], 1);
}

/****************************************************************************
 *	Reset registers to their initial values
 ****************************************************************************/
void tms32025_reset (void *param)
{
	R.PC = 0;			/* Starting address on a reset */
	R.STR0 |= 0x0600;	/* INTM and unused bit set to 1 */
	R.STR0 &= 0xefff;	/* OV cleared to 0. Remaining bits undefined */
	R.STR1 |= 0x07f0;	/* SXM, C, HM, FSM, XF and unused bits set to 1 */
	R.STR1 &= 0xeff0;	/* CNF, FO, TXM, PM bits cleared to 0. Remaining bits undefined */
	R.RPTC = 0;			/* Reset repeat counter to 0 */
	R.IFR = 0;			/* IRQ pending flags */

	S_OUT(TMS32025_XF,ASSERT_LINE);	/* XF flag is high. Must set the pin */

	/* ugly hack.. */
	R.intRAM = (UINT16 *)memory_region(REGION_CPU1 + cpu_getactivecpu());
	/* Set the internal memory mapped registers */
	GREG = 0;
	TIM  = 0xffff;
	PRD  = 0xffff;
	IMR  = 0xffc0;

	R.idle = 0;
	R.hold = 0;
	R.init_load_addr = 1;

	/* Reset the Data/Program address banks */
	TMS32025_DATA_BANK[0x0] = TMS32025_DATA_OFFSET + 0x0000;
	TMS32025_DATA_BANK[0x1] = TMS32025_DATA_OFFSET + 0x0100;
	TMS32025_DATA_BANK[0x2] = TMS32025_DATA_OFFSET + 0x0200;
	TMS32025_DATA_BANK[0x3] = TMS32025_DATA_OFFSET + 0x0300;
	TMS32025_DATA_BANK[0x4] = TMS32025_DATA_OFFSET + 0x0400;
	TMS32025_DATA_BANK[0x5] = TMS32025_DATA_OFFSET + 0x0500;
	TMS32025_DATA_BANK[0x6] = TMS32025_DATA_OFFSET + 0x0600;
	TMS32025_DATA_BANK[0x7] = TMS32025_DATA_OFFSET + 0x0700;
	TMS32025_DATA_BANK[0x8] = TMS32025_DATA_OFFSET + 0x0800;
	TMS32025_DATA_BANK[0x9] = TMS32025_DATA_OFFSET + 0x0900;
	TMS32025_DATA_BANK[0xa] = TMS32025_DATA_OFFSET + 0x0a00;
	TMS32025_DATA_BANK[0xb] = TMS32025_DATA_OFFSET + 0x0b00;
	TMS32025_DATA_BANK[0xc] = TMS32025_DATA_OFFSET + 0x0c00;
	TMS32025_DATA_BANK[0xd] = TMS32025_DATA_OFFSET + 0x0d00;
	TMS32025_DATA_BANK[0xe] = TMS32025_DATA_OFFSET + 0x0e00;
	TMS32025_DATA_BANK[0xf] = TMS32025_DATA_OFFSET + 0x0f00;

	TMS32025_PRGM_BANK[0x0] = TMS32025_PGM_OFFSET + 0x0000;
	TMS32025_PRGM_BANK[0x1] = TMS32025_PGM_OFFSET + 0x0100;
	TMS32025_PRGM_BANK[0x2] = TMS32025_PGM_OFFSET + 0x0200;
	TMS32025_PRGM_BANK[0x3] = TMS32025_PGM_OFFSET + 0x0300;
	TMS32025_PRGM_BANK[0x4] = TMS32025_PGM_OFFSET + 0x0400;
	TMS32025_PRGM_BANK[0x5] = TMS32025_PGM_OFFSET + 0x0500;
	TMS32025_PRGM_BANK[0x6] = TMS32025_PGM_OFFSET + 0x0600;
	TMS32025_PRGM_BANK[0x7] = TMS32025_PGM_OFFSET + 0x0700;
	TMS32025_PRGM_BANK[0x8] = TMS32025_PGM_OFFSET + 0x0800;
	TMS32025_PRGM_BANK[0x9] = TMS32025_PGM_OFFSET + 0x0900;
	TMS32025_PRGM_BANK[0xa] = TMS32025_PGM_OFFSET + 0x0a00;
	TMS32025_PRGM_BANK[0xb] = TMS32025_PGM_OFFSET + 0x0b00;
	TMS32025_PRGM_BANK[0xc] = TMS32025_PGM_OFFSET + 0x0c00;
	TMS32025_PRGM_BANK[0xd] = TMS32025_PGM_OFFSET + 0x0d00;
	TMS32025_PRGM_BANK[0xe] = TMS32025_PGM_OFFSET + 0x0e00;
	TMS32025_PRGM_BANK[0xf] = TMS32025_PGM_OFFSET + 0x0f00;
}


/****************************************************************************
 *	Shut down CPU emulation
 ****************************************************************************/
void tms32025_exit (void)
{
	/* nothing to do ? */
}


/****************************************************************************
 *	Issue an interrupt if necessary
 ****************************************************************************/
static int process_IRQs(void)
{
	/********** Interrupt Flag Register (IFR) **********
		|  5  |  4  |  3  |  2  |  1  |  0  |
		| XINT| RINT| TINT| INT2| INT1| INT0|
	*/

	R.tms32025_irq_cycles = 0;

	/* Dont service Interrupts if masked, or prev instruction was EINT ! */

	if ( (INTM == 0) && (R.opcode.w.l != 0xce00) && (R.IFR & IMR) )
	{
		R.tms32025_irq_cycles = (3*CLK);	/* 3 clock cycles used due to PUSH and DINT operation ? */
		PUSH_STACK(R.PC);

		if ((R.IFR & 0x01) && (IMR & 0x01)) {		/* IRQ line 0 */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  Active INT0\n");
			R.PC = 0x0002;
			(*R.irq_callback)(0);
			R.idle = 0;
			R.IFR &= (~0x01);
			SET0(INTM_FLAG);
			return R.tms32025_irq_cycles;
		}
		if ((R.IFR & 0x02) && (IMR & 0x02)) {		/* IRQ line 1 */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  Active INT1\n");
			R.PC = 0x0004;
			(*R.irq_callback)(1);
			R.idle = 0;
			R.IFR &= (~0x02);
			SET0(INTM_FLAG);
			return R.tms32025_irq_cycles;
		}
		if ((R.IFR & 0x04) && (IMR & 0x04)) {		/* IRQ line 2 */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  Active INT2\n");
			R.PC = 0x0006;
			(*R.irq_callback)(2);
			R.idle = 0;
			R.IFR &= (~0x04);
			SET0(INTM_FLAG);
			return R.tms32025_irq_cycles;
		}
		if ((R.IFR & 0x08) && (IMR & 0x08)) {		/* Timer IRQ (internal) */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  Active TINT (Timer)\n");
			R.PC = 0x0018;
			R.idle = 0;
			R.IFR &= (~0x08);
			SET0(INTM_FLAG);
			return R.tms32025_irq_cycles;
		}
		if ((R.IFR & 0x10) && (IMR & 0x10)) {		/* Serial port receive IRQ (internal) */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  Active RINT (Serial recieve)\n");
			R.PC = 0x001A;
			R.idle = 0;
			R.IFR &= (~0x10);
			SET0(INTM_FLAG);
			return R.tms32025_irq_cycles;
		}
		if ((R.IFR & 0x20) && (IMR & 0x20)) {		/* Serial port transmit IRQ (internal) */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS32025:  Active XINT (Serial transmit)\n");
			R.PC = 0x001C;
			R.idle = 0;
			R.IFR &= (~0x20);
			SET0(INTM_FLAG);
			return R.tms32025_irq_cycles;
		}
	}
	return R.tms32025_irq_cycles;
}

static INLINE void process_timer(int counts)
{
	if (counts > TIM) {				/* Overflow timer counts ? */
		if (counts > PRD) {
			counts %= (PRD + 1);
		}
		if (counts > TIM) {
			TIM = (PRD + 1) - (counts - TIM);
		}
		else {
			TIM -= counts;
		}

		R.IFR |= 0x08;
		tms32025_icount -= process_IRQs();		/* Handle Timer IRQ */
	}
	else {
		TIM -= counts;
	}
}

/****************************************************************************
 *	Execute ICount cycles. Exit when 0 or less
 ****************************************************************************/
int tms32025_execute(int cycles)
{
	tms32025_icount = cycles;


	/**** Respond to external hold signal */
	if (S_IN(TMS32025_HOLD) == ASSERT_LINE) {
		if (R.hold == 0) {
			S_OUT(TMS32025_HOLDA,ASSERT_LINE);	/* Hold-Ack (active low) */
		}
		R.hold = 1;
		if (HM) {
			tms32025_icount = 0;		/* Exit */
		}
		else {
			if (R.external_mem_access) {
				tms32025_icount = 0;	/* Exit */
			}
		}
	}
	else {
		if (R.hold == 1) {
			S_OUT(TMS32025_HOLDA,CLEAR_LINE);	/* Hold-Ack (active low) */
			tms32025_icount -= 3;
		}
		R.hold = 0;
	}

	/**** If idling, update timer and/or exit execution */
	if (R.idle) {
		if ((R.hold == 0) && (IMR & 0x08) && (INTM == 0)){
			process_timer(TIM + 1);
			tms32025_icount -= ((TIM + 1) * CLK);
		}
		else {
			process_timer(((tms32025_icount + CLK) / CLK));
			tms32025_icount = (tms32025_icount % CLK) - CLK;	/* Exit */
		}
	}

	if (tms32025_icount <= 0) CALL_MAME_DEBUG;


	while (tms32025_icount > 0)
	{
		R.tms32025_dec_cycles = (1*CLK);

		if (R.IFR) {	/* Check IRQ Flag Register for pending IRQs */
			R.tms32025_dec_cycles += process_IRQs();
		}

		R.PREVPC = R.PC;

		CALL_MAME_DEBUG;

		R.opcode.d = M_RDOP(R.PC);
		R.PC++;

		if (R.opcode.b.h == 0xCE)	/* Opcode 0xCExx has many opcodes in its minor byte */
		{
			R.tms32025_dec_cycles = cycles_CE_subset[R.opcode.b.l];
			(*(opcode_CE_subset[R.opcode.b.l]))();
		}
		else if ((R.opcode.w.l & 0xf0f8) == 0xd000)	/* Opcode 0xDxxx has many opcodes in its minor byte */
		{
			R.tms32025_dec_cycles = cycles_DX_subset[R.opcode.b.l];
			(*(opcode_DX_subset[R.opcode.b.l]))();
		}
		else			/* Do all opcodes except the CExx and Dxxx ones */
		{
			R.tms32025_dec_cycles = cycles_main[R.opcode.b.h];
			(*(opcode_main[R.opcode.b.h]))();
		}


		if (R.init_load_addr == 2) {		/* Repeat next instruction */
			R.PREVPC = R.PC;

			CALL_MAME_DEBUG;

			R.opcode.d = M_RDOP(R.PC);
			R.PC++;
			R.tms32025_dec_cycles += (1*CLK);

			do {
				if (R.opcode.b.h == 0xCE)
				{							/* Do all 0xCExx Opcodes */
					if (R.init_load_addr) {
						R.tms32025_dec_cycles += (1*CLK);
					}
					else {
						R.tms32025_dec_cycles += (1*CLK);
					}
					(*(opcode_CE_subset[R.opcode.b.l]))();
				}
				if ((R.opcode.w.l & 0xf0f8) == 0xd000)
				{							/* Do all valid 0xDxxx Opcodes */
					if (R.init_load_addr) {
						R.tms32025_dec_cycles += (1*CLK);
					}
					else {
						R.tms32025_dec_cycles += (1*CLK);
					}
					(*(opcode_DX_subset[R.opcode.b.l]))();
				}
				else
				{							/* Do all other opcodes */
					if (R.init_load_addr) {
						R.tms32025_dec_cycles += (1*CLK);
					}
					else {
						R.tms32025_dec_cycles += (1*CLK);
					}
					(*(opcode_main[R.opcode.b.h]))();
				}
				R.init_load_addr = 0;
				R.RPTC-- ;
			} while ((INT8)(R.RPTC) != -1);
			R.RPTC = 0;
			R.PFC = R.PC;
			R.init_load_addr = 1;
		}

		tms32025_icount -= R.tms32025_dec_cycles;

		/**** If device is put into idle mode, exit and wait for an interrupt */
		if (R.idle) {
			if ((R.hold == 0) && (IMR & 0x08) && (INTM == 0)) {
				if (R.tms32025_dec_cycles < (TIM * CLK)) {
					int burn_cycles;
					burn_cycles = ((TIM * CLK) <= tms32025_icount) ? (TIM * CLK) : tms32025_icount;
					R.tms32025_dec_cycles += burn_cycles;
					tms32025_icount -= burn_cycles;
				}
			}
			else {
				R.tms32025_dec_cycles += (tms32025_icount + CLK);
				tms32025_icount = (tms32025_icount % CLK) - CLK;	/* Exit */
			}
		}

		process_timer((R.tms32025_dec_cycles / CLK));

		/**** If hold pin is active, exit if accessing external memory or if HM is set */
		if (R.hold) {
			if (R.external_mem_access || (HM)) {
				if (tms32025_icount > 0) {
					tms32025_icount = 0;
				}
			}
		}
	}

	return (cycles - tms32025_icount);
}

/****************************************************************************
 *	Get all registers in given buffer
 ****************************************************************************/
unsigned tms32025_get_context (void *dst)
{
	if (dst)
		*(tms32025_Regs*)dst = R;
	return sizeof(tms32025_Regs);
}

/****************************************************************************
 *	Set all registers to given values
 ****************************************************************************/
void tms32025_set_context (void *src)
{
	if (src)
		R = *(tms32025_Regs*)src;
}


/****************************************************************************
 *	Return a specific register
 ****************************************************************************/
unsigned tms32025_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case TMS32025_PC: return R.PC;
		/* This is actually not a stack pointer, but the stack contents */
		case REG_SP:
		case TMS32025_STK7:  return R.STACK[7];
		case TMS32025_STK6:  return R.STACK[6];
		case TMS32025_STK5:  return R.STACK[5];
		case TMS32025_STK4:  return R.STACK[4];
		case TMS32025_STK3:  return R.STACK[3];
		case TMS32025_STK2:  return R.STACK[2];
		case TMS32025_STK1:  return R.STACK[1];
		case TMS32025_STK0:  return R.STACK[0];
		case TMS32025_STR0:  return R.STR0;
		case TMS32025_STR1:  return R.STR1;
		case TMS32025_IFR:   return R.IFR;
		case TMS32025_RPTC:  return R.RPTC;
		case TMS32025_ACC:   return R.ACC.d;
		case TMS32025_PREG:  return R.Preg.d;
		case TMS32025_TREG:  return R.Treg;
		case TMS32025_AR0:   return R.AR[0];
		case TMS32025_AR1:   return R.AR[1];
		case TMS32025_AR2:   return R.AR[2];
		case TMS32025_AR3:   return R.AR[3];
		case TMS32025_AR4:   return R.AR[4];
		case TMS32025_AR5:   return R.AR[5];
		case TMS32025_AR6:   return R.AR[6];
		case TMS32025_AR7:   return R.AR[7];
		case TMS32025_DRR:   return M_RDRAM(0);
		case TMS32025_DXR:   return M_RDRAM(1);
		case TMS32025_TIM:   return M_RDRAM(2);
		case TMS32025_PRD:   return M_RDRAM(3);
		case TMS32025_IMR:   return M_RDRAM(4);
		case TMS32025_GREG:  return M_RDRAM(5);
		case REG_PREVIOUSPC: return R.PREVPC;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if (offset < 8)
					return R.STACK[offset];
			}
	}
	return 0;
}


/****************************************************************************
 *	Set a specific register
 ****************************************************************************/
void tms32025_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
		case TMS32025_PC: R.PC = val; break;
		/* This is actually not a stack pointer, but the stack contents */
		case REG_SP:
		case TMS32025_STK7: R.STACK[7] = val; break;
		case TMS32025_STK6: R.STACK[6] = val; break;
		case TMS32025_STK5: R.STACK[5] = val; break;
		case TMS32025_STK4: R.STACK[4] = val; break;
		case TMS32025_STK3: R.STACK[3] = val; break;
		case TMS32025_STK2: R.STACK[2] = val; break;
		case TMS32025_STK1: R.STACK[1] = val; break;
		case TMS32025_STK0: R.STACK[0] = val; break;
		case TMS32025_STR0: R.STR0 = val; break;
		case TMS32025_STR1: R.STR1 = val; break;
		case TMS32025_IFR:  R.IFR = val; break;
		case TMS32025_RPTC: R.RPTC = val; break;
		case TMS32025_ACC:  R.ACC.d = val; break;
		case TMS32025_PREG: R.Preg.d = val; break;
		case TMS32025_TREG: R.Treg = val; break;
		case TMS32025_AR0:  R.AR[0] = val; break;
		case TMS32025_AR1:  R.AR[1] = val; break;
		case TMS32025_AR2:  R.AR[2] = val; break;
		case TMS32025_AR3:  R.AR[3] = val; break;
		case TMS32025_AR4:  R.AR[4] = val; break;
		case TMS32025_AR5:  R.AR[5] = val; break;
		case TMS32025_AR6:  R.AR[6] = val; break;
		case TMS32025_AR7:  R.AR[7] = val; break;
		case TMS32025_DRR:  M_WRTRAM(0,val); break;
		case TMS32025_DXR:  M_WRTRAM(1,val); break;
		case TMS32025_TIM:  M_WRTRAM(2,val); break;
		case TMS32025_PRD:  M_WRTRAM(3,val); break;
		case TMS32025_IMR:  M_WRTRAM(4,val); break;
		case TMS32025_GREG: M_WRTRAM(5,val); break;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if (offset < 8)
					R.STACK[offset] = val;
			}
	}
}


/****************************************************************************
 *	Set IRQ line state
 ****************************************************************************/
void tms32025_set_irq_line(int irqline, int state)
{
	/* Pending IRQs cannot be cleared */

	if (state != CLEAR_LINE)
	{
		R.IFR |= (1 << irqline);
		R.IFR &= 0x07;
	}
}

void tms32025_set_irq_callback(int (*callback)(int irqline))
{
	/* IACK is only a general IRQ Ack - no vector related stuff */

	R.irq_callback = callback;
}

/****************************************************************************
 *	Return a formatted string for a register
 ****************************************************************************/
const char *tms32025_info(void *context, int regnum)
{
	static char buffer[32][63+1];
	static int which = 0;
	tms32025_Regs *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';
	if (!context)
		r = &R;

	switch (regnum)
	{
		case CPU_INFO_REG+TMS32025_PC: sprintf(buffer[which], "PC:%04X",  r->PC); break;
		case CPU_INFO_REG+TMS32025_STR0: sprintf(buffer[which], "STR0:%04X", r->STR0); break;
		case CPU_INFO_REG+TMS32025_STR1: sprintf(buffer[which], "STR1:%04X", r->STR1); break;
		case CPU_INFO_REG+TMS32025_IFR: sprintf(buffer[which], "IFR:%04X", r->IFR); break;
		case CPU_INFO_REG+TMS32025_RPTC: sprintf(buffer[which], "RPTC:%02X", r->RPTC); break;
		case CPU_INFO_REG+TMS32025_STK7: sprintf(buffer[which], "STK7:%04X", r->STACK[7]); break;
		case CPU_INFO_REG+TMS32025_STK6: sprintf(buffer[which], "STK6:%04X", r->STACK[6]); break;
		case CPU_INFO_REG+TMS32025_STK5: sprintf(buffer[which], "STK5:%04X", r->STACK[5]); break;
		case CPU_INFO_REG+TMS32025_STK4: sprintf(buffer[which], "STK4:%04X", r->STACK[4]); break;
		case CPU_INFO_REG+TMS32025_STK3: sprintf(buffer[which], "STK3:%04X", r->STACK[3]); break;
		case CPU_INFO_REG+TMS32025_STK2: sprintf(buffer[which], "STK2:%04X", r->STACK[2]); break;
		case CPU_INFO_REG+TMS32025_STK1: sprintf(buffer[which], "STK1:%04X", r->STACK[1]); break;
		case CPU_INFO_REG+TMS32025_STK0: sprintf(buffer[which], "STK0:%04X", r->STACK[0]); break;
		case CPU_INFO_REG+TMS32025_ACC: sprintf(buffer[which], "ACC:%08X", r->ACC.d); break;
		case CPU_INFO_REG+TMS32025_PREG: sprintf(buffer[which], "P:%08X", r->Preg.d); break;
		case CPU_INFO_REG+TMS32025_TREG: sprintf(buffer[which], "T:%04X", r->Treg); break;
		case CPU_INFO_REG+TMS32025_AR0: sprintf(buffer[which], "AR0:%04X", r->AR[0]); break;
		case CPU_INFO_REG+TMS32025_AR1: sprintf(buffer[which], "AR1:%04X", r->AR[1]); break;
		case CPU_INFO_REG+TMS32025_AR2: sprintf(buffer[which], "AR2:%04X", r->AR[2]); break;
		case CPU_INFO_REG+TMS32025_AR3: sprintf(buffer[which], "AR3:%04X", r->AR[3]); break;
		case CPU_INFO_REG+TMS32025_AR4: sprintf(buffer[which], "AR4:%04X", r->AR[4]); break;
		case CPU_INFO_REG+TMS32025_AR5: sprintf(buffer[which], "AR5:%04X", r->AR[5]); break;
		case CPU_INFO_REG+TMS32025_AR6: sprintf(buffer[which], "AR6:%04X", r->AR[6]); break;
		case CPU_INFO_REG+TMS32025_AR7: sprintf(buffer[which], "AR7:%04X", r->AR[7]); break;
		case CPU_INFO_REG+TMS32025_DRR: sprintf(buffer[which], "DRR:%04X", M_RDRAM(0)); break;
		case CPU_INFO_REG+TMS32025_DXR: sprintf(buffer[which], "DXR:%04X", M_RDRAM(1)); break;
		case CPU_INFO_REG+TMS32025_TIM: sprintf(buffer[which], "TIM:%04X", M_RDRAM(2)); break;
		case CPU_INFO_REG+TMS32025_PRD: sprintf(buffer[which], "PRD:%04X", M_RDRAM(3)); break;
		case CPU_INFO_REG+TMS32025_IMR: sprintf(buffer[which], "IMR:%04X", M_RDRAM(4)); break;
		case CPU_INFO_REG+TMS32025_GREG: sprintf(buffer[which], "GREG:%04X", M_RDRAM(5)); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "arp%d%c%c%c%cdp%03x  arb%d%c%c%c%c%c%c%c%c%c%c%cpm%d",
				(r->STR0 & 0xe000) >> 13,
				r->STR0 & 0x1000 ? 'O':'.',
				r->STR0 & 0x0800 ? 'M':'.',
				r->STR0 & 0x0400 ? '.':'?',
				r->STR0 & 0x0200 ? 'I':'.',
				(r->STR0 & 0x01ff),

				(r->STR1 & 0xe000) >> 13,
				r->STR1 & 0x1000 ? 'P':'D',
				r->STR1 & 0x0800 ? 'T':'.',
				r->STR1 & 0x0400 ? 'S':'.',
				r->STR1 & 0x0200 ? 'C':'?',
				r->STR0 & 0x0100 ? '.':'?',
				r->STR1 & 0x0080 ? '.':'?',
				r->STR1 & 0x0040 ? 'H':'.',
				r->STR1 & 0x0020 ? 'F':'.',
				r->STR1 & 0x0010 ? 'X':'.',
				r->STR1 & 0x0008 ? 'f':'.',
				r->STR1 & 0x0004 ? 'o':'i',
				(r->STR1 & 0x0003) );
			break;
		case CPU_INFO_NAME: return "TMS32025";
		case CPU_INFO_FAMILY: return "Texas Instruments TMS320x25";
		case CPU_INFO_VERSION: return "1.10";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) 2001 by Tony La Porta";
		case CPU_INFO_REG_LAYOUT: return (const char*)tms32025_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)tms32025_win_layout;
		default: return "";
	}
	return buffer[which];
}

unsigned tms32025_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm32025( buffer, pc );
#else
	sprintf( buffer, "$%04X", TMS32025_RDOP(pc) );
	return 2;
#endif
}
