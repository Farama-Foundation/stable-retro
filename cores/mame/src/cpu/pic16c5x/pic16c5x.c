 /**************************************************************************\
 *						Microchip PIC16C5x Emulator							*
 *																			*
 *					  Copyright (C) 2003+ Tony La Porta						*
 *				   Originally written for the MAME project.					*
 *																			*
 *																			*
 *		Addressing architecture is based on the Harvard addressing scheme.	*
 *																			*
 *																			*
 *	**** Change Log ****													*
 *	TLP (06-Apr-2003)														*
 *	 - First Public release.												*
 *	BO  (07-Apr-2003) Ver 1.01  											*
 *	 - Renamed 'sleep' function to 'sleepic' to avoid C conflicts.			*
 *	TLP (09-Apr-2003) Ver 1.10  											*
 *	 - Fixed modification of file register $03 (Status).					*
 *	 - Corrected support for 7FFh (12-bit) size ROMs.						*
 *	 - The 'call' and 'goto' instructions weren't correctly handling the	*
 *	   STATUS page info correctly.											*
 *	 - The FSR register was incorrectly oring the data with 0xe0 when read.	*
 *	 - Prescaler masking information was set to 3 instead of 7.				*
 *	 - Prescaler assign bit was set to 4 instead of 8.						*
 *	 - Timer source and edge select flags/masks were wrong.					*
 *	 - Corrected the memory bank selection in GET/SET_REGFILE and also the	*
 *	   indirect register addressing.										*
 *	BMP (18-May-2003) Ver 1.11												*
 *	 - pic16c5x_get_reg functions were missing 'returns'.					*
 *	TLP (27-May-2003) Ver 1.12  											*
 *	 - Fixed the WatchDog timer count.										*
 *	 - The Prescaler rate was incorrectly being zeroed, instead of the		*
 *	   actual Prescaler counter in the CLRWDT and SLEEP instructions.		*
 *	 - Added masking to the FSR register. Upper unused bits are always 1.	*
 *																			*
 *																			*
 *	**** Notes: ****														*
 *	PIC WatchDog Timer has a seperate internal clock. For the moment, we're	*
 *	   basing the count on a 4MHz input clock, since 4MHz is the typical	*
 *	   input frequency (but by no means always).							*
 *	A single scaler is available for the Counter/Timer or WatchDog Timer.	*
 *	   When connected to the Counter/Timer, it functions as a Prescaler,	*
 *	   hence prescale overflows, tick the Counter/Timer.					*
 *	   When connected to the WatchDog Timer, it functions as a Postscaler	*
 *	   hence WatchDog Timer overflows, tick the Postscaler. This scenario	*
 *	   means that the WatchDog timeout occurs when the Postscaler has		*
 *	   reached the scaler rate value, not when the WatchDog reaches zero.	*
 *	CLRWDT should prevent the WatchDog Timer from timing out and generating *
 *	   a device reset, but how is not known. The manual also mentions that	*
 *	   the WatchDog Timer can only be disabled during ROM programming, and	*
 *	   no other means seem to exist???										*
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
#include "pic16c5x.h"


#define CLK 1	/* 1 cycle equals 4 Q-clock ticks */


#define M_RDRAM(A)		PIC16C5x_RAM_RDMEM(A)
#define M_WRTRAM(A,V)	PIC16C5x_RAM_WRMEM(A,V)
#define M_RDOP(A)		PIC16C5x_RDOP(A)
#define M_RDOP_ARG(A)	PIC16C5x_RDOP_ARG(A)
#define P_IN(A)			PIC16C5x_In(A)
#define P_OUT(A,V)		PIC16C5x_Out(A,V)
#define S_T0_IN			PIC16C5x_T0_In
#define ADDR_MASK		0x7ff




typedef struct
{
	/******************** CPU Internal Registers *******************/
	UINT16	PC;
	UINT16	PREVPC;		/* previous program counter */
	UINT8	W;
	UINT8	OPTION;
	UINT16	CONFIG;
	UINT8	ALU;
	UINT16	WDT;
	UINT8	TRISA;
	UINT8	TRISB;
	UINT8	TRISC;
	UINT16	STACK[2];
	UINT16	prescaler;	/* Note: this is really an 8-bit register */
	PAIR	opcode;
	UINT8	*picRAM;
} pic16C5x_Regs;

static pic16C5x_Regs R;
static UINT16 temp_config;
static UINT8  old_T0;
static INT8   old_data;
static UINT8 picRAMmask;
static int inst_cycles;
static int delay_timer;
static int picmodel;
static int pic16C5x_reset_vector;
int    pic16C5x_icount;
typedef void (*opcode_fn) (void);
static unsigned cycles_000_other[16];


#define TMR0	picRAM[1]
#define PCL		picRAM[2]
#define STATUS	picRAM[3]
#define FSR		picRAM[4]
#define PORTA	picRAM[5]
#define PORTB	picRAM[6]
#define PORTC	picRAM[7]
#define INDF	picRAM[(R.FSR)]

#define ADDR	(R.opcode.b.l & 0x1f)

#define POSITIVE_EDGE_T0  (( (int)(T0_in-old_T0) > 0) ? 1 : 0)
#define NEGATIVE_EDGE_T0  (( (int)(old_T0-T0_in) > 0) ? 1 : 0)


/********  The following is the Status Flag register definition.  *********/
			/* | 7 | 6 | 5 |  4 |  3 | 2 | 1  | 0 | */
			/* |    PA     | TO | PD | Z | DC | C | */
#define PA_REG		0xe0	/* PA	Program Page Preselect - bit 8 is unused here */
#define TO_FLAG		0x10	/* TO	Time Out flag (WatchDog) */
#define PD_FLAG		0x08	/* PD	Power Down flag */
#define Z_FLAG		0x04	/* Z	Zero Flag */
#define DC_FLAG		0x02	/* DC	Digit Carry/Borrow flag (Nibble) */
#define C_FLAG		0x01	/* C    Carry/Borrow Flag (Byte) */

#define PA		(R.STATUS & PA_REG)
#define TO		(R.STATUS & TO_FLAG)
#define PD		(R.STATUS & PD_FLAG)
#define ZERO	(R.STATUS & Z_FLAG)
#define DC		(R.STATUS & DC_FLAG)
#define CARRY	(R.STATUS & C_FLAG)


/********  The following is the Option Flag register definition.  *********/
			/* | 7 | 6 |   5  |   4  |  3  | 2 | 1 | 0 | */
			/* | 0 | 0 | TOCS | TOSE | PSA |    PS     | */
#define T0CS_FLAG	0x20	/* TOCS		Timer 0 clock source select */
#define T0SE_FLAG	0x10	/* TOSE		Timer 0 clock source edge select */
#define PSA_FLAG	0x08	/* PSA		Prescaler Assignment bit */
#define PS_REG		0x07	/* PS		Prescaler Rate select */

#define T0CS	(R.OPTION & T0CS_FLAG)
#define T0SE	(R.OPTION & T0SE_FLAG)
#define PSA		(R.OPTION & PSA_FLAG)
#define PS		(R.OPTION & PS_REG)


/********  The following is the Config Flag register definition.  *********/
	/* | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 |   2  | 1 | 0 | */
	/* |              CP                     | WDTE |  FOSC | */
							/* CP		Code Protect (ROM read protect) */
#define WDTE_FLAG	0x04	/* WDTE		WatchDog Timer enable */
#define FOSC_FLAG	0x03	/* FOSC		Oscillator source select */

#define WDTE	(R.CONFIG & WDTE_FLAG)
#define FOSC	(R.CONFIG & FOSC_FLAG)


/************************************************************************
 *	Shortcuts
 ************************************************************************/

/* Easy bit position selectors */
#define POS	 ((R.opcode.b.l >> 5) & 7)
static unsigned int bit_clr[8] = { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f };
static unsigned int bit_set[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };


static INLINE void CLR(UINT16 flag) { R.STATUS &= ~flag; }
static INLINE void SET(UINT16 flag) { R.STATUS |=  flag; }



static INLINE void CALCULATE_Z_FLAG(void)
{
	if (R.ALU == 0) SET(Z_FLAG);
	else CLR(Z_FLAG);
}

static INLINE void CALCULATE_ADD_CARRY(void)
{
	if ((UINT8)(old_data) > (UINT8)(R.ALU)) {
		SET(C_FLAG);
	}
	else {
		CLR(C_FLAG);
	}
}

static INLINE void CALCULATE_ADD_DIGITCARRY(void)
{
	if (((UINT8)(old_data) & 0x0f) > ((UINT8)(R.ALU) & 0x0f)) {
		SET(DC_FLAG);
	}
	else {
		CLR(DC_FLAG);
	}
}

static INLINE void CALCULATE_SUB_CARRY(void)
{
	if ((UINT8)(old_data) < (UINT8)(R.ALU)) {
		CLR(C_FLAG);
	}
	else {
		SET(C_FLAG);
	}
}

static INLINE void CALCULATE_SUB_DIGITCARRY(void)
{
	if (((UINT8)(old_data) & 0x0f) < ((UINT8)(R.ALU) & 0x0f)) {
		CLR(DC_FLAG);
	}
	else {
		SET(DC_FLAG);
	}
}



static INLINE UINT16 POP_STACK(void)
{
	UINT16 data = R.STACK[1];
	R.STACK[1] = R.STACK[0];
	return (data & ADDR_MASK);
}
static INLINE void PUSH_STACK(UINT16 data)
{
	R.STACK[0] = R.STACK[1];
	R.STACK[1] = (data & ADDR_MASK);
}


static INLINE UINT8 GET_REGFILE(offs_t addr)	/* Read from internal memory */
{
	UINT8 data;

	if ((picmodel == 0x16C57) || (picmodel == 0x16C58))
	{
		addr |= (R.FSR & 0x60);		/* FSR used for banking */
	}
	if ((addr & 0x10) == 0) addr &= 0x0f;

	switch(addr)
	{
		case 00:	addr = (R.FSR & picRAMmask);
					if (addr == 0) { data = 0; break; }
					if ((addr & 0x10) == 0) addr &= 0x0f;
					data = R.picRAM[addr];			/* Indirect address */
					break;
		case 04:	data = (R.FSR | (~picRAMmask)); break;
		case 05:	data = P_IN(0);
					data &= R.TRISA;
					data |= (~R.TRISA & R.PORTA);
					data &= 0xf;		/* 4-bit port (only lower 4 bits used) */
					break;
		case 06:	data = P_IN(1);
					data &= R.TRISB;
					data |= (~R.TRISB & R.PORTB);
					break;
		case 07:	if ((picmodel == 0x16C55) || (picmodel == 0x16C57)) {
						data = P_IN(2);
						data &= R.TRISC;
						data |= (~R.TRISC & R.PORTC);
					}
					else {		/* PIC16C54, PIC16C56, PIC16C58 */
						data = R.picRAM[addr];
					}
					break;
		default:	data = R.picRAM[addr]; break;
	}

	return data;
}

static INLINE void STORE_REGFILE(offs_t addr, UINT8 data)	/* Write to internal memory */
{
	if ((picmodel == 0x16C57) || (picmodel == 0x16C58))
	{
		addr |= (R.FSR & 0x60);
	}
	if ((addr & 0x10) == 0) addr &= 0x0f;

	switch(addr)
	{
		case 00:	addr = (R.FSR & picRAMmask);
					if (addr == 0) break;
					if ((addr & 0x10) == 0) addr &= 0x0f;
					R.picRAM[addr] = data; break;	/* Indirect address */
		case 01:	delay_timer = 2;		/* Timer starts after next two instructions */
					if (PSA == 0) R.prescaler = 0;	/* Must clear the Prescaler */
					R.TMR0 = data; break;
		case 02:	R.PCL = data;
					R.PC = ((R.STATUS & PA_REG) << 4) | data; break;
		case 03:	R.STATUS &= ~PA_REG; R.STATUS |= (data & PA_REG); break;
		case 04:	R.FSR = (data | (~picRAMmask)); break;
		case 05:	data &= 0xf;		/* 4-bit port (only lower 4 bits used) */
					P_OUT(0,data & (~R.TRISA)); R.PORTA = data; break;
		case 06:	P_OUT(1,data & (~R.TRISB)); R.PORTB = data; break;
		case 07:	if ((picmodel == 0x16C55) || (picmodel == 0x16C57)) {
						P_OUT(2,data & (~R.TRISC));
						R.PORTC = data;
					}
					else {		/* PIC16C54, PIC16C56, PIC16C58 */
						R.picRAM[addr] = data;
					}
					break;
		default:	R.picRAM[addr] = data; break;
	}
}


static INLINE void STORE_RESULT(offs_t addr, UINT8 data)
{
	if (R.opcode.b.l & 0x20)
	{
		STORE_REGFILE(addr, data);
	}
	else
	{
		R.W = data;
	}
}


/************************************************************************
 *	Emulate the Instructions
 ************************************************************************/

/* This following function is here to fill in the void for */
/* the opcode call function. This function is never called. */


static void illegal(void)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PIC16C5x:  PC=%03x,  Illegal opcode = %04x\n", (R.PC-1), R.opcode.w.l);
}


static void addwf(void)
{
	old_data = GET_REGFILE(ADDR);
	R.ALU = old_data + R.W;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
	CALCULATE_ADD_CARRY();
	CALCULATE_ADD_DIGITCARRY();
}

static void andwf(void)
{
	R.ALU = GET_REGFILE(ADDR) & R.W;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void andlw(void)
{
	R.ALU = R.opcode.b.l & R.W;
	R.W = R.ALU;
	CALCULATE_Z_FLAG();
}

static void bcf(void)
{
	R.ALU = GET_REGFILE(ADDR);
	R.ALU &= bit_clr[POS];
	STORE_REGFILE(ADDR, R.ALU);
}

static void bsf(void)
{
	R.ALU = GET_REGFILE(ADDR);
	R.ALU |= bit_set[POS];
	STORE_REGFILE(ADDR, R.ALU);
}

static void btfss(void)
{
	if ((GET_REGFILE(ADDR) & bit_set[POS]) == bit_set[POS])
	{
		R.PC++ ;
		R.PCL = R.PC & 0xff;
		inst_cycles += cycles_000_other[0];		/* Add NOP cycles */
	}
}

static void btfsc(void)
{
	if ((GET_REGFILE(ADDR) & bit_set[POS]) == 0)
	{
		R.PC++ ;
		R.PCL = R.PC & 0xff;
		inst_cycles += cycles_000_other[0];		/* Add NOP cycles */
	}
}

static void call(void)
{
	PUSH_STACK(R.PC);
	R.PC = ((R.STATUS & PA_REG) << 4) | R.opcode.b.l;
	R.PC &= 0x6ff;
	R.PCL = R.PC & 0xff;
}

static void clrw(void)
{
	R.W = 0;
	SET(Z_FLAG);
}

static void clrf(void)
{
	STORE_REGFILE(ADDR, 0);
	SET(Z_FLAG);
}

static void clrwdt(void)
{
	R.WDT = 0;
	if (PSA) R.prescaler = 0;
	SET(TO_FLAG);
	SET(PD_FLAG);
}

static void comf(void)
{
	R.ALU = ~(GET_REGFILE(ADDR));
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void decf(void)
{
	R.ALU = GET_REGFILE(ADDR) - 1;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void decfsz(void)
{
	R.ALU = GET_REGFILE(ADDR) - 1;
	STORE_RESULT(ADDR, R.ALU);
	if (R.ALU == 0)
	{
		R.PC++ ;
		R.PCL = R.PC & 0xff;
		inst_cycles += cycles_000_other[0];		/* Add NOP cycles */
	}
}

static void goto_op(void)
{
	R.PC = ((R.STATUS & PA_REG) << 4) | (R.opcode.w.l & 0x1ff);
	R.PC &= ADDR_MASK;
	R.PCL = R.PC & 0xff;
}

static void incf(void)
{
	R.ALU = GET_REGFILE(ADDR) + 1;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void incfsz(void)
{
	R.ALU = GET_REGFILE(ADDR) + 1;
	STORE_RESULT(ADDR, R.ALU);
	if (R.ALU == 0)
	{
		R.PC++ ;
		R.PCL = R.PC & 0xff;
		inst_cycles += cycles_000_other[0];		/* Add NOP cycles */
	}
}

static void iorlw(void)
{
	R.ALU = R.opcode.b.l | R.W;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void iorwf(void)
{
	R.ALU = GET_REGFILE(ADDR) | R.W;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void movf(void)
{
	R.ALU = GET_REGFILE(ADDR);
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}

static void movlw(void)
{
	R.W = R.opcode.b.l;
}

static void movwf(void)
{
	STORE_REGFILE(ADDR, R.W);
}

static void nop(void)
{
	/* Do nothing */
}

static void option(void)
{
	R.OPTION = R.W & 0x3f;
}

static void retlw(void)
{
	R.W = R.opcode.b.l;
	R.PC = POP_STACK();
	R.PCL = R.PC & 0xff;
}

static void rlf(void)
{
	R.ALU = GET_REGFILE(ADDR);
	R.ALU <<= 1;
	if (R.STATUS & C_FLAG) R.ALU |= 1;
	if (GET_REGFILE(ADDR) & 0x80) SET(C_FLAG);
	else CLR(C_FLAG);
	STORE_RESULT(ADDR, R.ALU);
}

static void rrf(void)
{
	R.ALU = GET_REGFILE(ADDR);
	R.ALU >>= 1;
	if (R.STATUS & C_FLAG) R.ALU |= 0x80;
	if (GET_REGFILE(ADDR) & 1) SET(C_FLAG);
	else CLR(C_FLAG);
	STORE_RESULT(ADDR, R.ALU);
}

static void sleepic(void)
{
	if (WDTE) R.WDT = 0;
	if (PSA) R.prescaler = 0;
	SET(TO_FLAG);
	CLR(PD_FLAG);
}

static void subwf(void)
{
	old_data = GET_REGFILE(ADDR);
	R.ALU = old_data - R.W;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
	CALCULATE_SUB_CARRY();
	CALCULATE_SUB_DIGITCARRY();
}

static void swapf(void)
{
	R.ALU  = ((GET_REGFILE(ADDR) << 4) & 0xf0);
	R.ALU |= ((GET_REGFILE(ADDR) >> 4) & 0x0f);
	STORE_RESULT(ADDR, R.ALU);
}

static void tris(void)
{
	switch(R.opcode.b.l & 0x7)
	{
		case 05:	if (R.TRISA == R.W) break;
					else R.TRISA = R.W; P_OUT(0,R.PORTA & (~R.TRISA) & 0xf); break;
		case 06:	if (R.TRISB == R.W) break;
					else R.TRISB = R.W; P_OUT(1,R.PORTB & (~R.TRISB)); break;
		case 07:	if (R.TRISC == R.W) break;
					else R.TRISC = R.W; P_OUT(2,R.PORTC & (~R.TRISC)); break;
		default:	illegal(); break;
	}
}

static void xorlw(void)
{
	R.ALU = R.W ^ R.opcode.b.l;
	R.W = R.ALU;
	CALCULATE_Z_FLAG();
}

static void xorwf(void)
{
	R.ALU = GET_REGFILE(ADDR) ^ R.W;
	STORE_RESULT(ADDR, R.ALU);
	CALCULATE_Z_FLAG();
}



/***********************************************************************
 *	Cycle Timings
 ***********************************************************************/

static unsigned cycles_main[256]=
{
/*00*/	1*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*10*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*20*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*30*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*40*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*50*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*60*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*70*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*80*/	2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*90*/	2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*A0*/	2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*B0*/	2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK, 2*CLK,
/*C0*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*D0*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*E0*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK,
/*F0*/	1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK
};

static opcode_fn opcode_main[256]=
{
/*00*/  nop,	illegal,movwf,	movwf,	clrw,	illegal,clrf,	clrf,
/*08*/  subwf,	subwf,	subwf,	subwf,	decf,	decf,	decf,	decf,
/*10*/  iorwf,	iorwf,	iorwf,	iorwf,	andwf,	andwf,	andwf,	andwf,
/*18*/  xorwf,	xorwf,	xorwf,	xorwf,	addwf,	addwf,	addwf,	addwf,
/*20*/  movf,	movf,	movf,	movf,	comf,	comf,	comf,	comf,
/*28*/  incf,	incf,	incf,	incf,	decfsz,	decfsz,	decfsz,	decfsz,
/*30*/  rrf,	rrf,	rrf,	rrf,	rlf,	rlf,	rlf,	rlf,
/*38*/  swapf,	swapf,	swapf,	swapf,	incfsz,	incfsz,	incfsz,	incfsz,
/*40*/  bcf,	bcf,	bcf,	bcf,	bcf,	bcf,	bcf,	bcf,
/*48*/  bcf,	bcf,	bcf,	bcf,	bcf,	bcf,	bcf,	bcf,
/*50*/  bsf,	bsf,	bsf,	bsf,	bsf,	bsf,	bsf,	bsf,
/*58*/  bsf,	bsf,	bsf,	bsf,	bsf,	bsf,	bsf,	bsf,
/*60*/  btfsc,	btfsc,	btfsc,	btfsc,	btfsc,	btfsc,	btfsc,	btfsc,
/*68*/  btfsc,	btfsc,	btfsc,	btfsc,	btfsc,	btfsc,	btfsc,	btfsc,
/*70*/  btfss,	btfss,	btfss,	btfss,	btfss,	btfss,	btfss,	btfss,
/*78*/  btfss,	btfss,	btfss,	btfss,	btfss,	btfss,	btfss,	btfss,
/*80*/  retlw,	retlw,	retlw,	retlw,	retlw,	retlw,	retlw,	retlw,
/*88*/  retlw,	retlw,	retlw,	retlw,	retlw,	retlw,	retlw,	retlw,
/*90*/  call,	call,	call,	call,	call,	call,	call,	call,
/*98*/  call,	call,	call,	call,	call,	call,	call,	call,
/*A0*/  goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,
/*A8*/  goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,
/*B0*/  goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,
/*B8*/  goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,goto_op,
/*C0*/  movlw,	movlw,	movlw,	movlw,	movlw,	movlw,	movlw,	movlw,
/*C8*/  movlw,	movlw,	movlw,	movlw,	movlw,	movlw,	movlw,	movlw,
/*D0*/  iorlw,	iorlw,	iorlw,	iorlw,	iorlw,	iorlw,	iorlw,	iorlw,
/*D8*/  iorlw,	iorlw,	iorlw,	iorlw,	iorlw,	iorlw,	iorlw,	iorlw,
/*E0*/  andlw,	andlw,	andlw,	andlw,	andlw,	andlw,	andlw,	andlw,
/*E8*/  andlw,	andlw,	andlw,	andlw,	andlw,	andlw,	andlw,	andlw,
/*F0*/  xorlw,	xorlw,	xorlw,	xorlw,	xorlw,	xorlw,	xorlw,	xorlw,
/*F8*/  xorlw,	xorlw,	xorlw,	xorlw,	xorlw,	xorlw,	xorlw,	xorlw
};


static unsigned cycles_000_other[16]=
{
/*00*/	1*CLK, 0*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 1*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK, 0*CLK
};

static opcode_fn opcode_000_other[16]=
{
/*00*/  nop,	illegal,option,	sleepic,clrwdt,	tris,	tris,	tris,
/*08*/  illegal,illegal,illegal,illegal,illegal,illegal,illegal,illegal
};



/****************************************************************************
 *	Inits CPU emulation
 ****************************************************************************/

void pic16C5x_init(void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_INT8("pic16C5x", cpu, "Old_Data", &old_data, 1);
	state_save_register_UINT8("pic16C5x", cpu, "W", &R.W, 1);
	state_save_register_UINT8("pic16C5x", cpu, "ALU", &R.ALU, 1);
	state_save_register_UINT8("pic16C5x", cpu, "Option", &R.OPTION, 1);
	state_save_register_UINT8("pic16C5x", cpu, "TRISA", &R.TRISA, 1);
	state_save_register_UINT8("pic16C5x", cpu, "TRISB", &R.TRISB, 1);
	state_save_register_UINT8("pic16C5x", cpu, "TRISC", &R.TRISC, 1);
	state_save_register_UINT8("pic16C5x", cpu, "PORTA", &R.PORTA, 1);
	state_save_register_UINT8("pic16C5x", cpu, "PORTB", &R.PORTB, 1);
	state_save_register_UINT8("pic16C5x", cpu, "PORTC", &R.PORTC, 1);
	state_save_register_UINT8("pic16C5x", cpu, "Old_T0", &old_T0, 1);
	state_save_register_UINT8("pic16C5x", cpu, "STR", &R.STATUS, 1);
	state_save_register_UINT8("pic16C5x", cpu, "RAM_mask", &picRAMmask, 1);
	state_save_register_UINT16("pic16C5x", cpu, "WDT", &R.WDT, 1);
	state_save_register_UINT16("pic16C5x", cpu, "Prescaler", &R.prescaler, 1);
	state_save_register_UINT16("pic16C5x", cpu, "Stack0", &R.STACK[0], 1);
	state_save_register_UINT16("pic16C5x", cpu, "Stack1", &R.STACK[1], 1);
	state_save_register_UINT16("pic16C5x", cpu, "PC", &R.PC, 1);
	state_save_register_UINT16("pic16C5x", cpu, "PrevPC", &R.PREVPC, 1);
	state_save_register_UINT16("pic16C5x", cpu, "Config", &R.CONFIG, 1);
	state_save_register_UINT32("pic16C5x", cpu, "Opcode", &R.opcode.d, 1);
	state_save_register_INT32("pic16C5x", cpu, "Delay_Timer", &delay_timer, 1);
	state_save_register_INT32("pic16C5x", cpu, "PIC_model", &picmodel, 1);
	state_save_register_INT32("pic16C5x", cpu, "Reset_Vector", &pic16C5x_reset_vector, 1);
}


/****************************************************************************
 *	Reset registers to their initial values
 ****************************************************************************/

void pic16C5x_reset_regs(void)
{
	/* ugly hack.. */
	R.picRAM = memory_region(REGION_CPU1 + cpu_getactivecpu());

	R.PC     = pic16C5x_reset_vector;
	R.CONFIG = temp_config;
	R.TRISA  = 0xff;
	R.TRISB  = 0xff;
	R.TRISC  = 0xff;
	R.OPTION = 0x3f;
	R.PCL    = 0xff;
	R.FSR   |= (~picRAMmask);
	R.PORTA &= 0x0f;
	R.prescaler = 0;
	delay_timer = 0;
	old_T0      = 0;
	inst_cycles = 0;
}

void pic16C5x_soft_reset(void)
{
	R.STATUS &= 0x1f;
	pic16C5x_reset_regs();
}

void pic16c5x_config(int data)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %04x to the PIC16C5x config register\n",data);
	temp_config = (data & 0xfff);
}


/****************************************************************************
 *	Shut down CPU emulation
 ****************************************************************************/

void pic16C5x_exit (void)
{
	/* nothing to do */
}


/****************************************************************************
 *	WatchDog
 ****************************************************************************/

static void pic16C5x_update_watchdog(int counts)
{
	/* WatchDog is set up to count 18,000 (0x464f hex) ticks to provide */
	/* the timeout period of 0.018ms based on a 4MHz input clock. */
	/* Note: the 4MHz clock should be divided by the PIC16C5x_CLOCK_DIVIDER */
	/* which effectively makes the PIC run at 1MHz internally. */

	/* If the current instruction is CLRWDT or SLEEP, don't update the WDT */

	if ((R.opcode.w.l != 3) && (R.opcode.w.l != 4))
	{
		UINT16 old_WDT = R.WDT;

		R.WDT -= counts;

		if (R.WDT > 0x464f) {
			R.WDT = 0x464f - (0xffff - R.WDT);
		}

		if (((old_WDT != 0) && (old_WDT < R.WDT)) || (R.WDT == 0))
		{
			if (PSA) {
				R.prescaler++;
				if (R.prescaler >= (1 << PS)) {	/* Prescale values from 1 to 128 */
					R.prescaler = 0;
					CLR(TO_FLAG);
					pic16C5x_soft_reset();
				}
			}
			else {
				CLR(TO_FLAG);
				pic16C5x_soft_reset();
			}
		}
	}
}


/****************************************************************************
 *	Update Timer
 ****************************************************************************/

static void pic16C5x_update_timer(int counts)
{
	if (PSA == 0) {
		R.prescaler += counts;
		if (R.prescaler >= (2 << PS)) {	/* Prescale values from 2 to 256 */
			R.TMR0 += (R.prescaler / (2 << PS));
			R.prescaler %= (2 << PS);	/* Overflow prescaler */
		}
	}
	else {
		R.TMR0 += counts;
	}
}


/****************************************************************************
 *	Execute IPeriod. Return 0 if emulation should be stopped
 ****************************************************************************/

int pic16C5x_execute(int cycles)
{
	int T0_in;
	pic16C5x_icount = cycles;

	do
	{
		if (PD == 0)						/* Sleep Mode */
		{
			inst_cycles = (1*CLK);
			CALL_MAME_DEBUG;
			if (WDTE) {
				pic16C5x_update_watchdog(1*CLK);
			}
		}
		else
		{
			R.PREVPC = R.PC;

			CALL_MAME_DEBUG;

			R.opcode.d = M_RDOP(R.PC);
			R.PC++;
			R.PCL++;

			if ((R.opcode.w.l & 0xff0) != 0x000)	{	/* Do all opcodes except the 00? ones */
				inst_cycles = cycles_main[((R.opcode.w.l >> 4) & 0xff)];
				(*(opcode_main[((R.opcode.w.l >> 4) & 0xff)]))();
			}
			else {	/* Opcode 0x00? has many opcodes in its minor nibble */
				inst_cycles = cycles_000_other[(R.opcode.b.l & 0x1f)];
				(*(opcode_000_other[(R.opcode.b.l & 0x1f)]))();
			}

			if (T0CS) {						/* Count mode */
				T0_in = S_T0_IN;
				if (T0SE) {					/* Count rising edge */
					if (POSITIVE_EDGE_T0) {
						pic16C5x_update_timer(1);
					}
				}
				else {						/* Count falling edge */
					if (NEGATIVE_EDGE_T0) {
						pic16C5x_update_timer(1);
					}
				}
				old_T0 = T0_in;
			}
			else {							/* Timer mode */
				if (delay_timer) {
					delay_timer--;
				}
				else {
					pic16C5x_update_timer((inst_cycles/CLK));
				}
			}
			if (WDTE) {
				pic16C5x_update_watchdog((inst_cycles/CLK));
			}
		}

		pic16C5x_icount -= inst_cycles;

	} while (pic16C5x_icount>0);

	return cycles - pic16C5x_icount;
}


/****************************************************************************
 *	Get all registers in given buffer
 ****************************************************************************/

unsigned pic16C5x_get_context (void *dst)
{
	if( dst )
		*(pic16C5x_Regs*)dst = R;
	return sizeof(pic16C5x_Regs);
}


/****************************************************************************
 *	Set all registers to given values
 ****************************************************************************/

void pic16C5x_set_context (void *src)
{
	if (src)
		R = *(pic16C5x_Regs*)src;
}


/****************************************************************************
 *	Return a specific register
 ****************************************************************************/

unsigned pic16C5x_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case PIC16C5x_PC: return R.PC;
		case REG_PREVIOUSPC: return R.PREVPC;
		/* This is actually not a stack pointer, but the stack contents */
		case REG_SP:
		case PIC16C5x_STK1: return R.STACK[1];
		case PIC16C5x_STK0: return R.STACK[0];
		case PIC16C5x_W:    return R.W;
		case PIC16C5x_ALU:  return R.ALU;
		case PIC16C5x_STR:  return R.STATUS;
		case PIC16C5x_OPT:  return R.OPTION;
		case PIC16C5x_TMR0: return R.TMR0;
		case PIC16C5x_WDT:  return R.WDT;
		case PIC16C5x_PSCL: return R.prescaler;
		case PIC16C5x_PRTA: return R.PORTA;
		case PIC16C5x_PRTB: return R.PORTB;
		case PIC16C5x_PRTC: return R.PORTC;
		case PIC16C5x_FSR:  return (R.FSR & picRAMmask);
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if (offset < 2)
					return R.STACK[offset];
			}
			break;
	}
	return 0;
}


/****************************************************************************
 *	Set a specific register
 ****************************************************************************/

void pic16C5x_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
		case PIC16C5x_PC: R.PC = val; R.PCL = val & 0xff ; break;
		/* This is actually not a stack pointer, but the stack contents */
		/* Stack is a 2 level First In Last Out stack */
		case REG_SP:
		case PIC16C5x_STK1: R.STACK[1] = val; break;
		case PIC16C5x_STK0: R.STACK[0] = val; break;
		case PIC16C5x_W:    R.W      = val; break;
		case PIC16C5x_ALU:  R.ALU    = val; break;
		case PIC16C5x_STR:  R.STATUS = val; break;
		case PIC16C5x_OPT:  R.OPTION = val; break;
		case PIC16C5x_TMR0: R.TMR0   = val; break;
		case PIC16C5x_WDT:  R.WDT    = val; break;
		case PIC16C5x_PSCL: R.prescaler = val; break;
		case PIC16C5x_PRTA: R.PORTA  = val; break;
		case PIC16C5x_PRTB: R.PORTB  = val; break;
		case PIC16C5x_PRTC: R.PORTC  = val; break;
		case PIC16C5x_FSR:  R.FSR    = (val & picRAMmask); break;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = (REG_SP_CONTENTS - regnum);
				if (offset < 4)
					R.STACK[offset] = val;
			}
			break;
	}
}


/****************************************************************************
 *	Set IRQ line state
 ****************************************************************************/

void pic16C5x_set_irq_line(int irqline, int state)
{
	/* No Interrupt Lines */
}

void pic16C5x_set_irq_callback(int (*callback)(int irqline))
{
	/* No IRQ Acknowledge Pins on this device */
}


/****************************************************************************
 *	Debugger definitions
 ****************************************************************************/

#if (HAS_PIC16C55 || HAS_PIC16C57)
static UINT8 pic16C5x_3p_reg_layout[] = {
	PIC16C5x_PC,  PIC16C5x_STK0, PIC16C5x_STK1, PIC16C5x_STR,  PIC16C5x_OPT,  -1,
	PIC16C5x_W,   PIC16C5x_TMR0, PIC16C5x_PSCL, PIC16C5x_PRTA, PIC16C5x_PRTB, PIC16C5x_PRTC, -1,
	PIC16C5x_ALU, PIC16C5x_WDT,  PIC16C5x_FSR,  PIC16C5x_TRSA, PIC16C5x_TRSB, PIC16C5x_TRSC, 0
};
#endif
#if (HAS_PIC16C54 || HAS_PIC16C56 || HAS_PIC16C58)
static UINT8 pic16C5x_2p_reg_layout[] = {
	PIC16C5x_PC,  PIC16C5x_STK0, PIC16C5x_STK1, PIC16C5x_STR,  PIC16C5x_OPT, -1,
	PIC16C5x_W,   PIC16C5x_TMR0, PIC16C5x_PSCL, PIC16C5x_PRTA, PIC16C5x_PRTB, -1,
	PIC16C5x_ALU, PIC16C5x_WDT,  PIC16C5x_FSR,  PIC16C5x_TRSA, PIC16C5x_TRSB, 0
};
#endif

static UINT8 pic16C5x_win_layout[] = {
	28, 0,53, 3,	/* register window (top rows) */
	 0, 0,27,22,	/* disassembler window (left colums) */
	28, 4,53, 8,	/* memory #1 window (right, upper middle) */
	28,13,53, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};




/****************************************************************************
 *	Return a formatted string for a register
 ****************************************************************************/

const char *pic16C5x_info(void *context, int regnum)
{
	static char buffer[18][47+1];
	static int which = 0;
	pic16C5x_Regs *r = context;

	which = (which+1) % 18;
	buffer[which][0] = '\0';
	if (!context)
		r = &R;

	switch (regnum)
	{
		case CPU_INFO_REG+PIC16C5x_PC:   sprintf(buffer[which], "PC:%03X",   r->PC); break;
		case CPU_INFO_REG+PIC16C5x_W:    sprintf(buffer[which], "W:%02X",    r->W); break;
		case CPU_INFO_REG+PIC16C5x_ALU:  sprintf(buffer[which], "ALU:%02X",  r->ALU); break;
		case CPU_INFO_REG+PIC16C5x_STR:  sprintf(buffer[which], "STR:%02X",  r->STATUS); break;
		case CPU_INFO_REG+PIC16C5x_TMR0: sprintf(buffer[which], "TMR:%02X",  r->TMR0); break;
		case CPU_INFO_REG+PIC16C5x_WDT:  sprintf(buffer[which], "WDT:%04X",  r->WDT); break;
		case CPU_INFO_REG+PIC16C5x_OPT:  sprintf(buffer[which], "OPT:%02X",  r->OPTION); break;
		case CPU_INFO_REG+PIC16C5x_STK0: sprintf(buffer[which], "STK0:%03X", r->STACK[0]); break;
		case CPU_INFO_REG+PIC16C5x_STK1: sprintf(buffer[which], "STK1:%03X", r->STACK[1]); break;
		case CPU_INFO_REG+PIC16C5x_PRTA: sprintf(buffer[which], "PRTA:%01X", ((r->PORTA) & 0xf)); break;
		case CPU_INFO_REG+PIC16C5x_PRTB: sprintf(buffer[which], "PRTB:%02X", r->PORTB); break;
		case CPU_INFO_REG+PIC16C5x_PRTC: sprintf(buffer[which], "PRTC:%02X", r->PORTC); break;
		case CPU_INFO_REG+PIC16C5x_TRSA: sprintf(buffer[which], "TRSA:%01X", ((r->TRISA) & 0xf)); break;
		case CPU_INFO_REG+PIC16C5x_TRSB: sprintf(buffer[which], "TRSB:%02X", r->TRISB); break;
		case CPU_INFO_REG+PIC16C5x_TRSC: sprintf(buffer[which], "TRSC:%02X", r->TRISC); break;
		case CPU_INFO_REG+PIC16C5x_FSR:  sprintf(buffer[which], "FSR:%02X",  ((r->FSR) & picRAMmask)); break;
		case CPU_INFO_REG+PIC16C5x_PSCL: sprintf(buffer[which], "PSCL:%c%02X", ((r->OPTION & 0x08) ? 'W':'T'), r->prescaler); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%01x%c%c%c%c%c %c%c%c%03x",
				(r->STATUS & 0xe0) >> 5,
				r->STATUS & 0x10 ? '.':'O',		/* WDT Overflow */
				r->STATUS & 0x08 ? 'P':'D',		/* Power/Down */
				r->STATUS & 0x04 ? 'Z':'.',		/* Zero */
				r->STATUS & 0x02 ? 'c':'b',		/* Nibble Carry/Borrow */
				r->STATUS & 0x01 ? 'C':'B',		/* Carry/Borrow */

				r->OPTION & 0x20 ? 'C':'T',		/* Counter/Timer */
				r->OPTION & 0x10 ? 'N':'P',		/* Negative/Positive */
				r->OPTION & 0x08 ? 'W':'T',		/* WatchDog/Timer */
				r->OPTION & 0x08 ? (1<<(r->OPTION&7)) : (2<<(r->OPTION&7)) );
			break;
		case CPU_INFO_NAME: return "PIC16C5x";
		case CPU_INFO_FAMILY: return "Microchip";
		case CPU_INFO_VERSION: return "1.12";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C)2003+ by Tony La Porta";
		case CPU_INFO_REG_LAYOUT: return (const char*)pic16C5x_3p_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)pic16C5x_win_layout;
	}
	return buffer[which];
}


#if (HAS_PIC16C54)
/****************************************************************************
 *	PIC16C54
 ****************************************************************************/

void pic16C54_reset(void *param)
{
	picmodel = 0x16C54;
	picRAMmask = 0x1f;
	pic16C5x_reset_vector = PIC16C54_RESET_VECTOR;
	pic16C5x_reset_regs();
	R.STATUS = 0x00;
	SET(TO_FLAG);
	SET(PD_FLAG);
}
void pic16C54_init(void)	{ pic16C5x_init(); }
void pic16C54_exit(void)	{ pic16C5x_exit(); }
int pic16C54_execute(int cycles) { return pic16C5x_execute(cycles); }
unsigned pic16C54_get_context(void *dst) { return pic16C5x_get_context(dst); }
void pic16C54_set_context(void *src) { pic16C5x_set_context(src); }
unsigned pic16C54_get_reg(int regnum) { return pic16C5x_get_reg(regnum); }
void pic16C54_set_reg(int regnum, unsigned val) { pic16C5x_set_reg(regnum, val); }
void pic16C54_set_irq_line(int irqline, int state) { pic16C5x_set_irq_line(irqline, state); }
void pic16C54_set_irq_callback(int (*callback)(int irqline)) { pic16C5x_set_irq_callback(callback); }

const char *pic16C54_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "PIC16C54";
		case CPU_INFO_REG_LAYOUT: return (const char*)pic16C5x_2p_reg_layout;
	}
	return pic16C5x_info(context,regnum);
}

unsigned pic16C54_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm16C5x( buffer, pc );
#else
	sprintf( buffer, "$%03X", M_RDOP(pc) );
	return 2;
#endif
}

#endif


#if (HAS_PIC16C55)
/****************************************************************************
 *	PIC16C55
 ****************************************************************************/

void pic16C55_reset(void *param)
{
	picmodel = 0x16C55;
	picRAMmask = 0x1f;
	pic16C5x_reset_vector = PIC16C55_RESET_VECTOR;
	pic16C5x_reset_regs();
	R.STATUS = 0x00;
	SET(TO_FLAG);
	SET(PD_FLAG);
}
void pic16C55_init(void)	{ pic16C5x_init(); }
void pic16C55_exit(void)	{ pic16C5x_exit(); }
int pic16C55_execute(int cycles) { return pic16C5x_execute(cycles); }
unsigned pic16C55_get_context(void *dst) { return pic16C5x_get_context(dst); }
void pic16C55_set_context(void *src) { pic16C5x_set_context(src); }
unsigned pic16C55_get_reg(int regnum) { return pic16C5x_get_reg(regnum); }
void pic16C55_set_reg(int regnum, unsigned val) { pic16C5x_set_reg(regnum, val); }
void pic16C55_set_irq_line(int irqline, int state) { pic16C5x_set_irq_line(irqline, state); }
void pic16C55_set_irq_callback(int (*callback)(int irqline)) { pic16C5x_set_irq_callback(callback); }

const char *pic16C55_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "PIC16C55";
		case CPU_INFO_REG_LAYOUT: return (const char*)pic16C5x_3p_reg_layout;
	}
	return pic16C5x_info(context,regnum);
}

unsigned pic16C55_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm16C5x( buffer, pc );
#else
	sprintf( buffer, "$%03X", M_RDOP(pc) );
	return 2;
#endif
}

#endif


#if (HAS_PIC16C56)
/****************************************************************************
 *	PIC16C56
 ****************************************************************************/

void pic16C56_reset(void *param)
{
	picmodel = 0x16C56;
	picRAMmask = 0x1f;
	pic16C5x_reset_vector = PIC16C56_RESET_VECTOR;
	pic16C5x_reset_regs();
	R.STATUS = 0x00;
	SET(TO_FLAG);
	SET(PD_FLAG);
}
void pic16C56_init(void)	{ pic16C5x_init(); }
void pic16C56_exit(void)	{ pic16C5x_exit(); }
int pic16C56_execute(int cycles) { return pic16C5x_execute(cycles); }
unsigned pic16C56_get_context(void *dst) { return pic16C5x_get_context(dst); }
void pic16C56_set_context(void *src) { pic16C5x_set_context(src); }
unsigned pic16C56_get_reg(int regnum) { return pic16C5x_get_reg(regnum); }
void pic16C56_set_reg(int regnum, unsigned val) { pic16C5x_set_reg(regnum, val); }
void pic16C56_set_irq_line(int irqline, int state) { pic16C5x_set_irq_line(irqline, state); }
void pic16C56_set_irq_callback(int (*callback)(int irqline)) { pic16C5x_set_irq_callback(callback); }

const char *pic16C56_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "PIC16C56";
		case CPU_INFO_REG_LAYOUT: return (const char*)pic16C5x_2p_reg_layout;
	}
	return pic16C5x_info(context,regnum);
}

unsigned pic16C56_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm16C5x( buffer, pc );
#else
	sprintf( buffer, "$%03X", M_RDOP(pc) );
	return 2;
#endif
}

#endif


#if (HAS_PIC16C57)
/****************************************************************************
 *	PIC16C57
 ****************************************************************************/

void pic16C57_reset(void *param)
{
	picmodel = 0x16C57;
	picRAMmask = 0x7f;
	pic16C5x_reset_vector = PIC16C57_RESET_VECTOR;
	pic16C5x_reset_regs();
	R.STATUS = 0x00;
	SET(TO_FLAG);
	SET(PD_FLAG);
}
void pic16C57_init(void)	{ pic16C5x_init(); }
void pic16C57_exit(void)	{ pic16C5x_exit(); }
int pic16C57_execute(int cycles) { return pic16C5x_execute(cycles); }
unsigned pic16C57_get_context(void *dst) { return pic16C5x_get_context(dst); }
void pic16C57_set_context(void *src) { pic16C5x_set_context(src); }
unsigned pic16C57_get_reg(int regnum) { return pic16C5x_get_reg(regnum); }
void pic16C57_set_reg(int regnum, unsigned val) { pic16C5x_set_reg(regnum, val); }
void pic16C57_set_irq_line(int irqline, int state) { pic16C5x_set_irq_line(irqline, state); }
void pic16C57_set_irq_callback(int (*callback)(int irqline)) { pic16C5x_set_irq_callback(callback); }

const char *pic16C57_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "PIC16C57";
		case CPU_INFO_REG_LAYOUT: return (const char*)pic16C5x_3p_reg_layout;
	}
	return pic16C5x_info(context,regnum);
}

unsigned pic16C57_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm16C5x( buffer, pc );
#else
	sprintf( buffer, "$%03X", M_RDOP(pc) );
	return 2;
#endif
}

#endif


#if (HAS_PIC16C58)
/****************************************************************************
 *	PIC16C58
 ****************************************************************************/

void pic16C58_reset(void *param)
{
	picmodel = 0x16C58;
	picRAMmask = 0x7f;
	pic16C5x_reset_vector = PIC16C57_RESET_VECTOR;
	pic16C5x_reset_regs();
	R.STATUS = 0x00;
	SET(TO_FLAG);
	SET(PD_FLAG);
}
void pic16C58_init(void)	{ pic16C5x_init(); }
void pic16C58_exit(void)	{ pic16C5x_exit(); }
int pic16C58_execute(int cycles) { return pic16C5x_execute(cycles); }
unsigned pic16C58_get_context(void *dst) { return pic16C5x_get_context(dst); }
void pic16C58_set_context(void *src) { pic16C5x_set_context(src); }
unsigned pic16C58_get_reg(int regnum) { return pic16C5x_get_reg(regnum); }
void pic16C58_set_reg(int regnum, unsigned val) { pic16C5x_set_reg(regnum, val); }
void pic16C58_set_irq_line(int irqline, int state) { pic16C5x_set_irq_line(irqline, state); }
void pic16C58_set_irq_callback(int (*callback)(int irqline)) { pic16C5x_set_irq_callback(callback); }

const char *pic16C58_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "PIC16C58";
		case CPU_INFO_REG_LAYOUT: return (const char*)pic16C5x_2p_reg_layout;
	}
	return pic16C5x_info(context,regnum);
}

unsigned pic16C58_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm16C5x( buffer, pc );
#else
	sprintf( buffer, "$%03X", M_RDOP(pc) );
	return 2;
#endif
}

#endif
