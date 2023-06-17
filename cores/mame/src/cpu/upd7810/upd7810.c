/*****************************************************************************
 *
 *	 upd7810.c
 *	 Portable uPD7810/11, 7810H/11H, 78C10/C11/C14 emulator V0.3
 *
 *	 Copyright (c) 2001 Juergen Buchmueller, all rights reserved.
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
 *	This work is based on the
 *	"NEC Electronics User's Manual, April 1987"
 *
 * NS20030115:
 * - fixed INRW_wa()
 * - TODO: add 7807, differences are listed below.
 *       I only added support for these opcodes needed by homedata.c (yes, I am
 *       lazy):
 *       4C CE (MOV A,PT)
 *       48 AC (EXA)
 *       48 AD (EXR)
 *       48 AE (EXH)
 *       48 AF (EXX)
 *       50 xx (SKN bit)
 *       58 xx (SETB)
 *       5B xx (CLR)
 *       5D xx (SK bit)
 *
 *****************************************************************************/
/* Hau around 23 May 2004
  gta, gti, dgt fixed
  working reg opcodes fixed
  sio input fixed
--
  PeT around 19 February 2002
  type selection/gamemaster support added
  gamemaster init hack? added
  ORAX added
  jre negativ fixed
  prefixed opcodes skipping fixed
  interrupts fixed and improved
  sub(and related)/add/daa flags fixed
  mvi ports,... fixed
  rll, rlr, drll, drlr fixed
  rets fixed
  l0, l1 skipping fixed
  calt fixed
*/

/*

7807 DESCRIPTION



   PA0  1     64 Vcc
   PA1  2     63 Vdd
   PA2  3     62 PD7/AD7
   PA3  4     61 PD6/AD6
   PA4  5     60 PD5/AD5
   PA5  6     59 PD4/AD4
   PA6  7     58 PD3/AD3
   PA7  8     57 PD2/AD2
   PB0  9     56 PD1/AD1
   PB1 10     55 PD0/AD0
   PB2 11     54 PF7/AB15
   PB3 12     53 PF6/AB14
   PB4 13     52 PF5/AB13
   PB5 14     51 PF4/AB12
   PB6 15     50 PF3/AB11
   PB7 16     49 PF2/AB10
   PC0 17     48 PF1/AB9
   PC1 18     47 PF0/AB8
   PC2 19     46 ALE
   PC3 20     45 WR*
   PC4 21     44 RD*
   PC5 22     43 HLDA
   PC6 23     42 HOLD
   PC7 24     41 PT7
  NMI* 25     40 PT6
  INT1 26     39 PT5
 MODE1 27     38 PT4
RESET* 28     37 PT3
 MODE0 29     36 PT2
    X2 30     35 PT1
    X1 31     34 PT0
   Vss 32     33 Vth

PA, PB, PC, PD, and PF is bidirectional I/O port
and PT is comparator input port in uPD7808.
uPD7807 uses PD port as data I/O and bottom address output,
and uses PF port as top address output.

NMI* is non maskable interrupt input signal (negative edge trigger).

INT1 is interrupt input (positive edge trigger). It can be used as
AC zero-cross input or trigger source of 16bit timer counter.

MODE0 and MODE1 is input terminal which decides total amount of
external memory of uPD7807 (4KByte, 16KBYte, and 64KByte).
It also decides number of PF ports used as top address output.
 4KByte mode: PF0~PF3=address output, PF4~PF7=data I/O port
16KByte mode: PF0~PF5=address output, PF6~PF7=data I/O port
64KByte mode: PF0~PF7=address output

RESET* is system rest terminal.

X1 and X2 does clock signal generation (connect OSC and condenser).

Vth is used to determine threshold voltage for PT port.
PT0~PT7 is connected to + input of each comparator,
and Vth deterimnes voltage connected to - input of PT0~PT7.
But the voltage of Vth is not directly connected to comapators.
It is connected via 16-level programmable voltage separate circuit.

HOLD and HLDA is terminal for DMA. RD*, WR*, and ALE is bus
interface signal (they are same type of Intel 8085).
Unlike 8085, I/O address space is not available, so IO /M* signal
does not exist. Read/write of external memory can be done
by RD*, WR*, and ALE only.

Vcc and Vss is main power source. Vdd is backup power source
for internal RWM (32 Byte).


PA and PB is I/O port. They have control register MA and MB.
If control register is set to 1, the port is input.
If control register is set to 0, the port is output.
They are set to 1 by reset.

PT is input-only port. It is consisted of input terminal PT0~PT7
and Vth (set threshold voltage). Each PT input has analog comparator
and latch, and + input of analog comparator is connected to
PT terminal. Every - input of analog comparator is connected
to devided voltage of Vth. Voltage dividing level can be set by
bottom 4bits of MT (mode T) register. The range is 1/16~16/16 of Vth.

Other internal I/Os are
8bit timer (x2): Upcounter. If the counter matches to specified value,
the timer is reset and counts again from 0.
You can also set it to generate interrupt, or invert output flip-flop
when the counter matches to specified value.
Furthermore, you can output that flip-flop output to PC4/TO output,
connect it to clock input of timer/event counter or watchdog timer.
Or you can use it as bitrate clock of serial interface.
Note: There is only 1 output flip-flop for 2 timers.
If you use it for timer output of 1 timer, another timer cannot be used
for other than interrupt generator.
Clock input for timer can be switched between internal clock (2 type)
or PC3/TI input. You can set 1 timer's match-output as another timer's
clock input, so that you can use them as 1 16bit timer.

16bit timer/event counter (x1): It can be used as
- Interval timer
- External event counter
- Frequency measurement
- Pulse width measurement
- Programmable rectangle wave output
- One pulse output
Related terminals are PC5/CI input, PC6/CO0 output, and PC7/CO1.
You can measure CI input's H duration, or you can output timing signal
(with phase difference) to CO0 and CO1.

serial I/F (x1): has 3 modes.
- Asynchronous mode
- Synchronous mode
- I/O interface mode
In all 3 modes, bitrate can be internal fixed clock, or timer output,
or external clock.
In asynchronous mode, you can
- switch 7bit/8bit data
- set parity ON/OFF and EVEN/ODD
- set 1/2 stop bit




DIFFERENCES BETWEEN 7810 and 7807

--------------------------
8bit transfer instructions
--------------------------

7810
inst.     1st byte 2nd byte state   action
EXX       00001001            4     Swap BC DE HL
EXA       00001000            4     Swap VA EA
EXH       01010000            4     Swap HL
BLOCK     00110001          13(C+1)  (DE)+ <- (HL)+, C <- C - 1, until CY

7807
inst.     1st byte  2nd byte state   action
EXR       01001000  10101101   8     Swap VA BC DE HL EA
EXX       01001000  10101111   8     Swap BC DE HL
EXA       01001000  10101100   8     Swap VA EA
EXH       01001000  10101110   8     Swap HL
BLOCK  D+ 00010000           13(C+1) (DE)+ <- (HL)+, C <- C - 1, until CY
BLOCK  D- 00010001           13(C+1) (DE)- <- (HL)-, C <- C - 1, until CY


---------------------------
16bit transfer instructions
---------------------------
All instructions are same except operand sr4 of DMOV instruction.
7810
V0-sr4 -function
 0-ECNT-timer/event counter upcounter
 1-ECPT-timer/event counter capture

7807
V1-V0- sr4 -function
 0- 0-ECNT -timer/event counter upcounter
 0- 1-ECPT0-timer/event counter capture 0
 1- 0-ECPT1-timer/event counter capture 1


-----------------------------------------
8bit operation instructions for registers
-----------------------------------------
All instructions are same.


--------------------------------------
8bit operation instructions for memory
--------------------------------------
All instructions are same.


-----------------------------------------
Operation instructions for immediate data
-----------------------------------------
uPD7807 has read-only PT port and special register group sr5 for it.
ins.               1st byte  2nd byte 3rd 4th state func
GTI    sr5, byte   01100100  s0101sss  dd      14   !CY  sr5 - byte - 1
LTI    sr5, byte   01100100  s0111sss  dd      14    CY  sr5 - byte
NEI    sr5, byte   01100100  s1101sss  dd      14   !Z   sr5 - byte
EQI    sr5, byte   01100100  s1111sss  dd      14    Z   sr5 - byte
ONI    sr5, byte   01100100  s1001sss  dd      14   !Z   sr5 & byte
OFFI   sr5, byte   01100100  s1011sss  dd      14    Z   sr5 & byte

S5-S4-S3-S2-S1-S0-sr -sr1-sr2-sr5-register function
 0  0  1  1  1  0 --- PT  --- PT  comparator input port T data
 1  0  0  1  0  0 WDM WDM --- --- watchdog timer mode register
 1  0  0  1  0  1 MT  --- --- --- port T mode

7807 doesn't have registers below
 0  0  1  0  0  0 ANM ANM ANM     A/D channel mode
 1  0  0  0  0  0 --- CR0 ---     A/D conversion result 0
 1  0  0  0  0  1 --- CR1 ---     A/D conversion result 1
 1  0  0  0  1  0 --- CR2 ---     A/D conversion result 2
 1  0  0  0  1  1 --- CR3 ---     A/D conversion result 3
 1  0  1  0  0  0 ZCM --- ---     zero cross mode

Special register operand (includes registers for I/O ports) has
6 groups - sr, sr1, sr2, sr3, sr4, and sr5. Among these groups,
sr, sr1, sr2, and sr5 includes registers described in the table
below, and expressed as bit pattern S5-S0.

S5S4S3S2S1S0 sr  sr1 sr2 sr5 register function
0 0 0 0 0 0  PA  PA  PA  PA  port A
0 0 0 0 0 1  PB  PB  PB  PB  port B
0 0 0 0 1 0  PC  PC  PC  PC  port C
0 0 0 0 1 1  PD  PD  PD  PD  port D
0 0 0 1 0 1  PF  PF  PF  PF  port F
0 0 0 1 1 0  MKH MKH MKH MKH mask high
0 0 0 1 1 1  MKL MKL MKL MKL mask low
0 0 1 0 0 1  SMH SMH SMH SMH serial mode high
0 0 1 0 1 0  SML --- --- --- serial mode low
0 0 1 0 1 1  EOM EOM EOM EOM timer/event counter output mode
0 0 1 1 0 0 ETMM --- --- --- timer/event counter mode
0 0 1 1 0 1  TMM TMM TMM TMM timer mode
0 0 1 1 1 0  --- PT  --- PT  port T
0 1 0 0 0 0  MM  --- --- --- memory mapping
0 1 0 0 0 1  MCC --- --- --- mode control C
0 1 0 0 1 0  MA  --- --- --- mode A
0 1 0 0 1 1  MB  --- --- --- mode B
0 1 0 1 0 0  MC  --- --- --- mode C
0 1 0 1 1 1  MF  --- --- --- mode F
0 1 1 0 0 0  TXB --- --- --- Tx buffer
0 1 1 0 0 1  --- RXB --- --- Rx buffer
0 1 1 0 1 0  TM0 --- --- --- timer register 0
0 1 1 0 1 1  TM1 --- --- --- timer register 1
1 0 0 1 0 0  WDM WDM --- --- watchdog timer mode
1 0 0 1 0 1  MT  --- --- --- mode T

For sr and sr1, all 6bits (S5, S4, S3, S2, S1, and S0) are used.
For sr2 and sr5, only 4bits (S3, S2, S1, AND S0) are used.
They are expressed as 'ssssss' and 's sss' in operation code.
Note that 's sss' (of sr2 and sr5) is located separately.
S0 is rightmost bit (LSB).


--------------------------------------------
Operation instructions for working registers
--------------------------------------------
All instructions are same.


--------------------------------------------------------------------------
16bit operation instructions and divider/multiplier operation instructions
--------------------------------------------------------------------------
All instructions are same.


------------------------------------------
Increment/decrement operation instructions
------------------------------------------
All instructions are same.


----------------------------
Other operation instructions
----------------------------
7807 has CMC instruction (inverts CY flag).
ins. 1st byte 2nd byte 3rd 4th state func
CMC  01001000 10101010           8   CY <- !CY


---------------------------
Rotation/shift instructions
---------------------------
All instructions are same.


-----------------------------
Jump/call/return instructions
-----------------------------
All instructions are same.


-----------------
Skip instructions
-----------------
7807 doesn't have this
ins.            1st byte 2nd byte 3rd 4th state func
BIT bit, wa     01011bbb  wwwwwwww          10*  bit skip if (V.wa).bit = 1

Instead, 7807 has these bit manipulation instructions.
ins.            1st byte 2nd byte 3rd 4th state func
MOV    CY, bit  01011111  bbbbbbbb          10* CY <- (bit)
MOV    bit, CY  01011010  bbbbbbbb          13* (bit) <- CY
AND    CY, bit  00110001  bbbbbbbb          10* CY <- CY & (bit)
OR     CY, bit  01011100  bbbbbbbb          10* CY <- CY | (bit)
XOR    CY, bit  01011110  bbbbbbbb          10* CY <- CY ^ (bit)
SETB   bit      01011000  bbbbbbbb          13* (bit) <- 1
CLR    bit      01011011  bbbbbbbb          13* (bit) <- 0
NOT    bit      01011001  bbbbbbbb          13* (bit) <- !(bit)
SK     bit      01011101  bbbbbbbb          10*  (b) skip if (bit) = 1
SKN    bit      01010000  bbbbbbbb          10* !(b) skip if (bit) = 0


------------------------
CPU control instructions
------------------------
ins.            1st byte 2nd byte 3rd 4th state func
HLT             01001000  00111011        11/12 halt
11 state in uPD7807 and uPD7810, 12 state in uPD78C10.

STOP            01001000  10111011          12  stop
7807 doesn't have STOP instruction.

*/


#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "upd7810.h"

#ifdef RUNTIME_LOADER
#define upd7810_ICount upd7810_icount
struct cpu_interface upd7810_interface=
CPU0(UPD7810,  upd7810,  2,  0,1.00,UPD7810_INT_NONE,  UPD7810_INTF1,  UPD7810_INTNMI, 8, 16,	  0,16,LE,1, 4	);

extern void upd7810_runtime_loader_init(void)
{
	cpuintf[CPU_UPD7810]=upd7810_interface;
}
#endif


UPD7810 upd7810;
int upd7810_icount;

/* Layout of the registers in the debugger */
static UINT8 upd7810_reg_layout[] = {
	UPD7810_PC, UPD7810_PSW, UPD7810_A, UPD7810_V, UPD7810_EA, UPD7810_BC, UPD7810_DE, UPD7810_HL, -1,
	UPD7810_SP, UPD7810_MM, UPD7810_A2, UPD7810_V2, UPD7810_EA2, UPD7810_BC2, UPD7810_DE2, UPD7810_HL2, -1,
	UPD7810_TMM, UPD7810_CNT0, UPD7810_TM0, UPD7810_CNT1, UPD7810_TM1, -1,
	UPD7810_ETMM, UPD7810_ECNT, UPD7810_ECPT, UPD7810_ETM0, UPD7810_ETM1, -1,
	UPD7810_TXB, UPD7810_RXB, UPD7810_CR0, UPD7810_CR1, UPD7810_CR2, UPD7810_CR3, -1,
    UPD7810_TXD, UPD7810_RXD, UPD7810_SCK, UPD7810_TI, UPD7810_TO, UPD7810_CI, UPD7810_CO0, UPD7810_CO1, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 upd7810_win_layout[] = {
	 0, 0,80, 6,	/* register window (top rows) */
	 0, 7,24,15,	/* disassembler window (left colums) */
	25, 7,55, 7,	/* memory #1 window (right, upper middle) */
	25,15,55, 7,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

#define CY	0x01
#define F1	0x02
#define L0	0x04
#define L1	0x08
#define HC	0x10
#define SK	0x20
#define Z	0x40
#define F7	0x80

/* IRR flags */
#define INTNMI	0x0001
#define INTFT0	0x0002
#define INTFT1	0x0004
#define INTF1	0x0008
#define INTF2	0x0010
#define INTFE0	0x0020
#define INTFE1	0x0040
#define INTFEIN 0x0080
#define INTFAD	0x0100
#define INTFSR	0x0200
#define INTFST	0x0400
#define INTER	0x0800
#define INTOV	0x1000

/* ITF flags */
#define INTAN4	0x0001
#define INTAN5	0x0002
#define INTAN6	0x0004
#define INTAN7	0x0008
#define INTSB	0x0010

#define PPC 	upd7810.ppc.w.l
#define PC		upd7810.pc.w.l
#define PCL 	upd7810.pc.b.l
#define PCH 	upd7810.pc.b.h
#define PCD 	upd7810.pc.d
#define SP		upd7810.sp.w.l
#define SPL 	upd7810.sp.b.l
#define SPH 	upd7810.sp.b.h
#define SPD 	upd7810.sp.d
#define PSW 	upd7810.psw
#define OP		upd7810.op
#define OP2 	upd7810.op2
#define IFF 	upd7810.iff
#define EA		upd7810.ea.w.l
#define EAL 	upd7810.ea.b.l
#define EAH 	upd7810.ea.b.h
#define VA		upd7810.va.w.l
#define V		upd7810.va.b.h
#define A		upd7810.va.b.l
#define VAD 	upd7810.va.d
#define BC		upd7810.bc.w.l
#define B		upd7810.bc.b.h
#define C		upd7810.bc.b.l
#define DE		upd7810.de.w.l
#define D		upd7810.de.b.h
#define E		upd7810.de.b.l
#define HL		upd7810.hl.w.l
#define H		upd7810.hl.b.h
#define L		upd7810.hl.b.l
#define EA2 	upd7810.ea2.w.l
#define VA2 	upd7810.va2.w.l
#define BC2 	upd7810.bc2.w.l
#define DE2 	upd7810.de2.w.l
#define HL2 	upd7810.hl2.w.l

#define OVC0	upd7810.ovc0
#define OVC1	upd7810.ovc1
#define OVCE	upd7810.ovce
#define OVCF	upd7810.ovcf
#define OVCS	upd7810.ovcs
#define EDGES	upd7810.edges

#define CNT0	upd7810.cnt.b.l
#define CNT1	upd7810.cnt.b.h
#define TM0 	upd7810.tm.b.l
#define TM1 	upd7810.tm.b.h
#define ECNT	upd7810.ecnt.w.l
#define ECPT	upd7810.ecnt.w.h
#define ETM0	upd7810.etm.w.l
#define ETM1	upd7810.etm.w.h

#define MA		upd7810.ma
#define MB		upd7810.mb
#define MCC 	upd7810.mcc
#define MC		upd7810.mc
#define MM		upd7810.mm
#define MF		upd7810.mf
#define TMM 	upd7810.tmm
#define ETMM	upd7810.etmm
#define EOM 	upd7810.eom
#define SML 	upd7810.sml
#define SMH 	upd7810.smh
#define ANM 	upd7810.anm
#define MKL 	upd7810.mkl
#define MKH 	upd7810.mkh
#define ZCM 	upd7810.zcm

#define CR0 	upd7810.cr0
#define CR1 	upd7810.cr1
#define CR2 	upd7810.cr2
#define CR3 	upd7810.cr3
#define RXB 	upd7810.rxb
#define TXB 	upd7810.txb

#define RXD 	upd7810.rxd
#define TXD 	upd7810.txd
#define SCK 	upd7810.sck
#define TI		upd7810.ti
#define TO		upd7810.to
#define CI		upd7810.ci
#define CO0 	upd7810.co0
#define CO1 	upd7810.co1

#define IRR 	upd7810.irr
#define ITF 	upd7810.itf

struct opcode_s {
	void (*opfunc)(void);
	UINT8 oplen;
	UINT8 cycles;
	UINT8 cycles_skip;
	UINT8 mask_l0_l1;
};

#define RDOP(O) 	O = cpu_readop(PCD); PC++
#define RDOPARG(A)	A = cpu_readop_arg(PCD); PC++
#define RM(A)		cpu_readmem16(A)
#define WM(A,V) 	cpu_writemem16(A,V)

#define ZHC_ADD(after,before,carry) 	\
	if (after == 0) PSW |= Z; else PSW &= ~Z; \
	if (after == before) \
		PSW = (PSW&~CY) | (carry); \
	else if (after < before)			\
		PSW |= CY;			\
	else								\
		PSW &= ~CY;				\
	if ((after & 15) < (before & 15))	\
		PSW |= HC;						\
	else								\
		PSW &= ~HC; 					\

#define ZHC_SUB(after,before,carry) 	\
	if (after == 0) PSW |= Z; else PSW &= ~Z; \
	if (before == after) 					\
		PSW = (PSW & ~CY) | (carry);	\
	else if (after > before)			\
		PSW |= CY;			\
	else								\
		PSW &= ~CY;				\
	if ((after & 15) > (before & 15))	\
		PSW |= HC;						\
	else								\
		PSW &= ~HC; 					\

#define SKIP_CY 	if (CY == (PSW & CY)) PSW |= SK
#define SKIP_NC 	if (0 == (PSW & CY)) PSW |= SK
#define SKIP_Z		if (Z == (PSW & Z)) PSW |= SK
#define SKIP_NZ 	if (0 == (PSW & Z)) PSW |= SK
#define SET_Z(n)	if (n) PSW &= ~Z; else PSW |= Z

static data8_t RP(offs_t port)
{
	data8_t data = 0xff;
	switch (port)
	{
	case UPD7810_PORTA:
		if (upd7810.ma)	/* NS20031301 no need to read if the port is set as output*/
			upd7810.pa_in = cpu_readport16(port);
		data = (upd7810.pa_in & upd7810.ma) | (upd7810.pa_out & ~upd7810.ma);
		break;
	case UPD7810_PORTB:
		if (upd7810.mb)	/* NS20031301 no need to read if the port is set as output*/
			upd7810.pb_in = cpu_readport16(port);
		data = (upd7810.pb_in & upd7810.mb) | (upd7810.pb_out & ~upd7810.mb);
		break;
	case UPD7810_PORTC:
		if (upd7810.mc)	/* NS20031301 no need to read if the port is set as output*/
			upd7810.pc_in = cpu_readport16(port);
		data = (upd7810.pc_in & upd7810.mc) | (upd7810.pc_out & ~upd7810.mc);
		if (upd7810.mcc & 0x01) 	/* PC0 = TxD output */
			data = (data & ~0x01) | (upd7810.txd & 1 ? 0x01 : 0x00);
		if (upd7810.mcc & 0x02) 	/* PC1 = RxD input */
			data = (data & ~0x02) | (upd7810.rxd & 1 ? 0x02 : 0x00);
		if (upd7810.mcc & 0x04) 	/* PC2 = SCK input/output */
			data = (data & ~0x04) | (upd7810.sck & 1 ? 0x04 : 0x00);
		if (upd7810.mcc & 0x08) 	/* PC3 = TI input */
			data = (data & ~0x08) | (upd7810.ti & 1 ? 0x08 : 0x00);
		if (upd7810.mcc & 0x10) 	/* PC4 = TO output */
			data = (data & ~0x10) | (upd7810.to & 1 ? 0x10 : 0x00);
		if (upd7810.mcc & 0x20) 	/* PC5 = CI input */
			data = (data & ~0x20) | (upd7810.ci & 1 ? 0x20 : 0x00);
		if (upd7810.mcc & 0x40) 	/* PC6 = CO0 output */
			data = (data & ~0x40) | (upd7810.co0 & 1 ? 0x40 : 0x00);
		if (upd7810.mcc & 0x80) 	/* PC7 = CO1 output */
			data = (data & ~0x80) | (upd7810.co1 & 1 ? 0x80 : 0x00);
		break;
	case UPD7810_PORTD:
		upd7810.pd_in = cpu_readport16(port);
		switch (upd7810.mm & 0x07)
		{
		case 0x00:			/* PD input mode, PF port mode */
			data = upd7810.pd_in;
			break;
		case 0x01:			/* PD output mode, PF port mode */
			data = upd7810.pd_out;
			break;
		default:			/* PD extension mode, PF port/extension mode */
			data = 0xff;	/* what do we see on the port here? */
			break;
		}
		break;
	case UPD7810_PORTF:
		upd7810.pf_in = cpu_readport16(port);
		switch (upd7810.mm & 0x06)
		{
		case 0x00:			/* PD input/output mode, PF port mode */
			data = (upd7810.pf_in & upd7810.mf) | (upd7810.pf_out & ~upd7810.mf);
			break;
		case 0x02:			/* PD extension mode, PF0-3 extension mode, PF4-7 port mode */
			data = (upd7810.pf_in & upd7810.mf) | (upd7810.pf_out & ~upd7810.mf);
			data |= 0x0f;	/* what would we see on the lower bits here? */
			break;
		case 0x04:			/* PD extension mode, PF0-5 extension mode, PF6-7 port mode */
			data = (upd7810.pf_in & upd7810.mf) | (upd7810.pf_out & ~upd7810.mf);
			data |= 0x3f;	/* what would we see on the lower bits here? */
			break;
		case 0x06:
			data = 0xff;	/* what would we see on the lower bits here? */
			break;
		}
		break;
	case UPD7807_PORTT:	/* NS20031301 partial implementation*/
		data = cpu_readport16(port);
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "uPD7810 internal error: RP() called with invalid port number\n");
	}
	return data;
}

static void WP(offs_t port, data8_t data)
{
	switch (port)
	{
	case UPD7810_PORTA:
		upd7810.pa_out = data;
/*		data = (data & ~upd7810.ma) | (upd7810.pa_in & upd7810.ma);*/
		data = (data & ~upd7810.ma) | (upd7810.ma);	/* NS20031401*/
		cpu_writeport16(port, data);
		break;
	case UPD7810_PORTB:
		upd7810.pb_out = data;
/*		data = (data & ~upd7810.mb) | (upd7810.pb_in & upd7810.mb);*/
		data = (data & ~upd7810.mb) | (upd7810.mb);	/* NS20031401*/
		cpu_writeport16(port, data);
		break;
	case UPD7810_PORTC:
		upd7810.pc_out = data;
/*		data = (data & ~upd7810.mc) | (upd7810.pc_in & upd7810.mc);*/
		data = (data & ~upd7810.mc) | (upd7810.mc);	/* NS20031401*/
		if (upd7810.mcc & 0x01) 	/* PC0 = TxD output */
			data = (data & ~0x01) | (upd7810.txd & 1 ? 0x01 : 0x00);
		if (upd7810.mcc & 0x02) 	/* PC1 = RxD input */
			data = (data & ~0x02) | (upd7810.rxd & 1 ? 0x02 : 0x00);
		if (upd7810.mcc & 0x04) 	/* PC2 = SCK input/output */
			data = (data & ~0x04) | (upd7810.sck & 1 ? 0x04 : 0x00);
		if (upd7810.mcc & 0x08) 	/* PC3 = TI input */
			data = (data & ~0x08) | (upd7810.ti & 1 ? 0x08 : 0x00);
		if (upd7810.mcc & 0x10) 	/* PC4 = TO output */
			data = (data & ~0x10) | (upd7810.to & 1 ? 0x10 : 0x00);
		if (upd7810.mcc & 0x20) 	/* PC5 = CI input */
			data = (data & ~0x20) | (upd7810.ci & 1 ? 0x20 : 0x00);
		if (upd7810.mcc & 0x40) 	/* PC6 = CO0 output */
			data = (data & ~0x40) | (upd7810.co0 & 1 ? 0x40 : 0x00);
		if (upd7810.mcc & 0x80) 	/* PC7 = CO1 output */
			data = (data & ~0x80) | (upd7810.co1 & 1 ? 0x80 : 0x00);
		cpu_writeport16(port, data);
		break;
	case UPD7810_PORTD:
		upd7810.pd_out = data;
		switch (upd7810.mm & 0x07)
		{
		case 0x00:			/* PD input mode, PF port mode */
			data = upd7810.pd_in;
			break;
		case 0x01:			/* PD output mode, PF port mode */
			data = upd7810.pd_out;
			break;
		default:			/* PD extension mode, PF port/extension mode */
			return;
		}
		cpu_writeport16(port, data);
		break;
	case UPD7810_PORTF:
		upd7810.pf_out = data;
		data = (data & ~upd7810.mf) | (upd7810.pf_in & upd7810.mf);
		switch (upd7810.mm & 0x06)
		{
		case 0x00:			/* PD input/output mode, PF port mode */
			break;
		case 0x02:			/* PD extension mode, PF0-3 extension mode, PF4-7 port mode */
			data |= 0x0f;	/* what would come out for the lower bits here? */
			break;
		case 0x04:			/* PD extension mode, PF0-5 extension mode, PF6-7 port mode */
			data |= 0x3f;	/* what would come out for the lower bits here? */
			break;
		case 0x06:
			data |= 0xff;	/* what would come out for the lower bits here? */
			break;
		}
		cpu_writeport16(port, data);
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "uPD7810 internal error: RP() called with invalid port number\n");
	}
}

static void upd7810_take_irq(void)
{
	UINT16 vector = 0;
	int irqline = 0;

	/* global interrupt disable? */
	if (0 == IFF)
		return;

	/* check the interrupts in priority sequence */
	if ((IRR & INTFT0)	&& 0 == (MKL & 0x02))
	{
	    switch (upd7810.config.type)
		{
			case TYPE_7810_GAMEMASTER:
				vector = 0xff2a;
				break;
			default:
				vector = 0x0008;
		}
	    if (!((IRR & INTFT1)	&& 0 == (MKL & 0x04)))
		IRR&=~INTFT0;
	}
	else
	if ((IRR & INTFT1)	&& 0 == (MKL & 0x04))
	{
	    switch (upd7810.config.type)
		{
			case TYPE_7810_GAMEMASTER:
				vector = 0xff2a;
				break;
			default:
				vector = 0x0008;
		}
	    IRR&=~INTFT1;
	}
	else
	if ((IRR & INTF1)	&& 0 == (MKL & 0x08))
	{
		irqline = UPD7810_INTF1;
		vector = 0x0010;
		if (!((IRR & INTF2)	&& 0 == (MKL & 0x10)))
		    IRR&=~INTF1;
	}
	else
	if ((IRR & INTF2)	&& 0 == (MKL & 0x10))
	{
		irqline = UPD7810_INTF2;
		vector = 0x0010;
		IRR&=~INTF2;
	}
	else
	if ((IRR & INTFE0)	&& 0 == (MKL & 0x20))
	{
	    switch (upd7810.config.type)
		{
			case TYPE_7810_GAMEMASTER:
				vector = 0xff2d;
				break;
			default:
				vector = 0x0018;
		}
	    if (!((IRR & INTFE1)	&& 0 == (MKL & 0x40)))
		IRR&=~INTFE0;
	}
	else
	if ((IRR & INTFE1)	&& 0 == (MKL & 0x40))
	{
	    switch (upd7810.config.type)
		{
		    case TYPE_7810_GAMEMASTER:
				vector = 0xff2d;
				break;
		    default:
				vector = 0x0018;
		}
	    IRR&=~INTFE1;
	}
	else
	if ((IRR & INTFEIN) && 0 == (MKL & 0x80))
	{
		vector = 0x0020;
	}
	else
	if ((IRR & INTFAD)	&& 0 == (MKH & 0x01))
	{
		vector = 0x0020;
	}
	else
	if ((IRR & INTFSR)	&& 0 == (MKH & 0x02))
	{
		vector = 0x0028;
	    IRR&=~INTFSR;
	}
	else
	if ((IRR & INTFST)	&& 0 == (MKH & 0x04))
	{
		vector = 0x0028;
	    IRR&=~INTFST;
	}
	if (vector)
	{
		/* acknowledge external IRQ */
		if (irqline)
			(*upd7810.irq_callback)(irqline);
		SP--;
		WM( SP, PSW );
		SP--;
		WM( SP, PCH );
		SP--;
		WM( SP, PCL );
		IFF = 0;
		PSW &= ~(SK|L0|L1);
		PC = vector;
		change_pc16( PCD );
	}
}

static void upd7810_write_EOM(void)
{
	if (EOM & 0x01) /* output LV0 content ? */
	{
		switch (EOM & 0x0e)
		{
		case 0x02:	/* toggle CO0 */
			CO0 = (CO0 >> 1) | ((CO0 ^ 2) & 2);
			break;
		case 0x04:	/* reset CO0 */
			CO0 = 0;
			break;
		case 0x08:	/* set CO0 */
			CO0 = 1;
			break;
		}
	}
	if (EOM & 0x10) /* output LV0 content ? */
	{
		switch (EOM & 0xe0)
		{
		case 0x20:	/* toggle CO1 */
			CO1 = (CO1 >> 1) | ((CO1 ^ 2) & 2);
			break;
		case 0x40:	/* reset CO1 */
			CO1 = 0;
			break;
		case 0x80:	/* set CO1 */
			CO1 = 1;
			break;
		}
	}
}

static void upd7810_write_TXB(void)
{
	upd7810.txbuf = 1;
}

#define PAR7(n) ((((n)>>6)^((n)>>5)^((n)>>4)^((n)>>3)^((n)>>2)^((n)>>1)^((n)))&1)
#define PAR8(n) ((((n)>>7)^((n)>>6)^((n)>>5)^((n)>>4)^((n)>>3)^((n)>>2)^((n)>>1)^((n)))&1)

static void upd7810_sio_output(void)
{
	/* shift out more bits? */
	if (upd7810.txcnt > 0)
	{
		TXD = upd7810.txs & 1;
		if (upd7810.config.io_callback)
			(*upd7810.config.io_callback)(UPD7810_TXD,TXD);
		upd7810.txs >>= 1;
		upd7810.txcnt--;
		if (0 == upd7810.txcnt)
			IRR |= INTFST;		/* serial transfer completed */
	}
	else
	if (SMH & 0x04) /* send enable ? */
	{
		/* nothing written into the transmitter buffer ? */
        if (0 == upd7810.txbuf)
			return;
        upd7810.txbuf = 0;

        if (SML & 0x03)         /* asynchronous mode ? */
		{
			switch (SML & 0xfc)
			{
			case 0x48:	/* 7bits, no parity, 1 stop bit */
			case 0x68:	/* 7bits, no parity, 1 stop bit (parity select = 1 but parity is off) */
				/* insert start bit in bit0, stop bit int bit8 */
				upd7810.txs = (TXB << 1) | (1 << 8);
				upd7810.txcnt = 9;
				break;
			case 0x4c:	/* 8bits, no parity, 1 stop bit */
			case 0x6c:	/* 8bits, no parity, 1 stop bit (parity select = 1 but parity is off) */
				/* insert start bit in bit0, stop bit int bit9 */
				upd7810.txs = (TXB << 1) | (1 << 9);
				upd7810.txcnt = 10;
				break;
			case 0x58:	/* 7bits, odd parity, 1 stop bit */
				/* insert start bit in bit0, parity in bit 8, stop bit in bit9 */
				upd7810.txs = (TXB << 1) | (PAR7(TXB) << 8) | (1 << 9);
				upd7810.txcnt = 10;
				break;
			case 0x5c:	/* 8bits, odd parity, 1 stop bit */
				/* insert start bit in bit0, parity in bit 9, stop bit int bit10 */
				upd7810.txs = (TXB << 1) | (PAR8(TXB) << 9) | (1 << 10);
				upd7810.txcnt = 11;
				break;
			case 0x78:	/* 7bits, even parity, 1 stop bit */
				/* insert start bit in bit0, parity in bit 8, stop bit in bit9 */
				upd7810.txs = (TXB << 1) | ((PAR7(TXB) ^ 1) << 8) | (1 << 9);
				upd7810.txcnt = 10;
				break;
			case 0x7c:	/* 8bits, even parity, 1 stop bit */
				/* insert start bit in bit0, parity in bit 9, stop bit int bit10 */
				upd7810.txs = (TXB << 1) | ((PAR8(TXB) ^ 1) << 9) | (1 << 10);
				upd7810.txcnt = 11;
				break;
			case 0xc8:	/* 7bits, no parity, 2 stop bits */
			case 0xe8:	/* 7bits, no parity, 2 stop bits (parity select = 1 but parity is off) */
				/* insert start bit in bit0, stop bits int bit8+9 */
				upd7810.txs = (TXB << 1) | (3 << 8);
				upd7810.txcnt = 10;
				break;
			case 0xcc:	/* 8bits, no parity, 2 stop bits */
			case 0xec:	/* 8bits, no parity, 2 stop bits (parity select = 1 but parity is off) */
				/* insert start bit in bit0, stop bits in bits9+10 */
				upd7810.txs = (TXB << 1) | (3 << 9);
				upd7810.txcnt = 11;
				break;
			case 0xd8:	/* 7bits, odd parity, 2 stop bits */
				/* insert start bit in bit0, parity in bit 8, stop bits in bits9+10 */
				upd7810.txs = (TXB << 1) | (PAR7(TXB) << 8) | (3 << 9);
				upd7810.txcnt = 11;
				break;
			case 0xdc:	/* 8bits, odd parity, 2 stop bits */
				/* insert start bit in bit0, parity in bit 9, stop bits int bit10+11 */
				upd7810.txs = (TXB << 1) | (PAR8(TXB) << 9) | (3 << 10);
				upd7810.txcnt = 12;
				break;
			case 0xf8:	/* 7bits, even parity, 2 stop bits */
				/* insert start bit in bit0, parity in bit 8, stop bits in bit9+10 */
				upd7810.txs = (TXB << 1) | ((PAR7(TXB) ^ 1) << 8) | (3 << 9);
				upd7810.txcnt = 11;
				break;
			case 0xfc:	/* 8bits, even parity, 2 stop bits */
				/* insert start bit in bit0, parity in bit 9, stop bits int bits10+10 */
				upd7810.txs = (TXB << 1) | ((PAR8(TXB) ^ 1) << 9) | (1 << 10);
				upd7810.txcnt = 12;
				break;
			}
		}
		else
		{
			/* synchronous mode */
			upd7810.txs = TXB;
			upd7810.txcnt = 8;
		}
	}
}

static void upd7810_sio_input(void)
{
	/* sample next bit? */
	if (upd7810.rxcnt > 0)
	{
		if (upd7810.config.io_callback)
			RXD = (*upd7810.config.io_callback)(UPD7810_RXD,RXD);
		upd7810.rxs = (upd7810.rxs >> 1) | ((UINT16)RXD << 15);
		upd7810.rxcnt--;
		if (0 == upd7810.rxcnt)
		{
			/* reset the TSK bit */
			SMH &= ~0x40;
			/* serial receive completed interrupt */
			IRR |= INTFSR;
			/* now extract the data from the shift register */
			if (SML & 0x03) 	/* asynchronous mode ? */
			{
				switch (SML & 0xfc)
				{
				case 0x48:	/* 7bits, no parity, 1 stop bit */
				case 0x68:	/* 7bits, no parity, 1 stop bit (parity select = 1 but parity is off) */
					upd7810.rxs >>= 16 - 9;
					RXB = (upd7810.rxs >> 1) & 0x7f;
					if ((1 << 8) != (upd7810.rxs & (1 | (1 << 8))))
						IRR |= INTER;	/* framing error */
					break;
				case 0x4c:	/* 8bits, no parity, 1 stop bit */
				case 0x6c:	/* 8bits, no parity, 1 stop bit (parity select = 1 but parity is off) */
					upd7810.rxs >>= 16 - 10;
					RXB = (upd7810.rxs >> 1) & 0xff;
					if ((1 << 9) != (upd7810.rxs & (1 | (1 << 9))))
						IRR |= INTER;	/* framing error */
					break;
				case 0x58:	/* 7bits, odd parity, 1 stop bit */
					upd7810.rxs >>= 16 - 10;
					RXB = (upd7810.rxs >> 1) & 0x7f;
					if ((1 << 9) != (upd7810.rxs & (1 | (1 << 9))))
						IRR |= INTER;	/* framing error */
					if (PAR7(RXB) != ((upd7810.rxs >> 8) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0x5c:	/* 8bits, odd parity, 1 stop bit */
					upd7810.rxs >>= 16 - 11;
					RXB = (upd7810.rxs >> 1) & 0xff;
					if ((1 << 10) != (upd7810.rxs & (1 | (1 << 10))))
						IRR |= INTER;	/* framing error */
					if (PAR8(RXB) != ((upd7810.rxs >> 9) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0x78:	/* 7bits, even parity, 1 stop bit */
					upd7810.rxs >>= 16 - 10;
					RXB = (upd7810.rxs >> 1) & 0x7f;
					if ((1 << 9) != (upd7810.rxs & (1 | (1 << 9))))
						IRR |= INTER;	/* framing error */
					if (PAR7(RXB) != ((upd7810.rxs >> 8) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0x7c:	/* 8bits, even parity, 1 stop bit */
					upd7810.rxs >>= 16 - 11;
					RXB = (upd7810.rxs >> 1) & 0xff;
					if ((1 << 10) != (upd7810.rxs & (1 | (1 << 10))))
						IRR |= INTER;	/* framing error */
					if (PAR8(RXB) != ((upd7810.rxs >> 9) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0xc8:	/* 7bits, no parity, 2 stop bits */
				case 0xe8:	/* 7bits, no parity, 2 stop bits (parity select = 1 but parity is off) */
					upd7810.rxs >>= 16 - 10;
					RXB = (upd7810.rxs >> 1) & 0x7f;
					if ((3 << 9) != (upd7810.rxs & (1 | (3 << 9))))
						IRR |= INTER;	/* framing error */
					if (PAR7(RXB) != ((upd7810.rxs >> 8) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0xcc:	/* 8bits, no parity, 2 stop bits */
				case 0xec:	/* 8bits, no parity, 2 stop bits (parity select = 1 but parity is off) */
					upd7810.rxs >>= 16 - 11;
					RXB = (upd7810.rxs >> 1) & 0xff;
					if ((3 << 10) != (upd7810.rxs & (1 | (3 << 10))))
						IRR |= INTER;	/* framing error */
					if (PAR8(RXB) != ((upd7810.rxs >> 9) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0xd8:	/* 7bits, odd parity, 2 stop bits */
					upd7810.rxs >>= 16 - 11;
					RXB = (upd7810.rxs >> 1) & 0x7f;
					if ((3 << 10) != (upd7810.rxs & (1 | (3 << 10))))
						IRR |= INTER;	/* framing error */
					if (PAR7(RXB) != ((upd7810.rxs >> 8) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0xdc:	/* 8bits, odd parity, 2 stop bits */
					upd7810.rxs >>= 16 - 12;
					RXB = (upd7810.rxs >> 1) & 0xff;
					if ((3 << 11) != (upd7810.rxs & (1 | (3 << 11))))
						IRR |= INTER;	/* framing error */
					if (PAR8(RXB) != ((upd7810.rxs >> 9) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0xf8:	/* 7bits, even parity, 2 stop bits */
					upd7810.rxs >>= 16 - 11;
					RXB = (upd7810.rxs >> 1) & 0x7f;
					if ((3 << 10) != (upd7810.rxs & (1 | (3 << 10))))
						IRR |= INTER;	/* framing error */
					if (PAR7(RXB) != ((upd7810.rxs >> 8) & 1))
						IRR |= INTER;	/* parity error */
					break;
				case 0xfc:	/* 8bits, even parity, 2 stop bits */
					upd7810.rxs >>= 16 - 12;
					RXB = (upd7810.rxs >> 1) & 0xff;
					if ((3 << 11) != (upd7810.rxs & (1 | (3 << 11))))
						IRR |= INTER;	/* framing error */
					if (PAR8(RXB) != ((upd7810.rxs >> 9) & 1))
						IRR |= INTER;	/* parity error */
					break;
				}
			}
			else
			{
				upd7810.rxs >>= 16 - 8;
				RXB = upd7810.rxs;
#if 0
				upd7810.rxcnt = 8;
#endif
			}
		}
	}
	else
	if (SMH & 0x08) /* receive enable ? */
	{
		if (SML & 0x03) 	/* asynchronous mode ? */
		{
			switch (SML & 0xfc)
			{
			case 0x48:	/* 7bits, no parity, 1 stop bit */
			case 0x68:	/* 7bits, no parity, 1 stop bit (parity select = 1 but parity is off) */
				upd7810.rxcnt = 9;
				break;
			case 0x4c:	/* 8bits, no parity, 1 stop bit */
			case 0x6c:	/* 8bits, no parity, 1 stop bit (parity select = 1 but parity is off) */
				upd7810.rxcnt = 10;
				break;
			case 0x58:	/* 7bits, odd parity, 1 stop bit */
				upd7810.rxcnt = 10;
				break;
			case 0x5c:	/* 8bits, odd parity, 1 stop bit */
				upd7810.rxcnt = 11;
				break;
			case 0x78:	/* 7bits, even parity, 1 stop bit */
				upd7810.rxcnt = 10;
				break;
			case 0x7c:	/* 8bits, even parity, 1 stop bit */
				upd7810.rxcnt = 11;
				break;
			case 0xc8:	/* 7bits, no parity, 2 stop bits */
			case 0xe8:	/* 7bits, no parity, 2 stop bits (parity select = 1 but parity is off) */
				upd7810.rxcnt = 10;
				break;
			case 0xcc:	/* 8bits, no parity, 2 stop bits */
			case 0xec:	/* 8bits, no parity, 2 stop bits (parity select = 1 but parity is off) */
				upd7810.rxcnt = 11;
				break;
			case 0xd8:	/* 7bits, odd parity, 2 stop bits */
				upd7810.rxcnt = 11;
				break;
			case 0xdc:	/* 8bits, odd parity, 2 stop bits */
				upd7810.rxcnt = 12;
				break;
			case 0xf8:	/* 7bits, even parity, 2 stop bits */
				upd7810.rxcnt = 11;
				break;
			case 0xfc:	/* 8bits, even parity, 2 stop bits */
				upd7810.rxcnt = 12;
				break;
			}
		}
		else
		/* TSK bit set ? */
		if (SMH & 0x40)
		{
			upd7810.rxcnt = 8;
		}
	}
}

static void upd7810_timers(int cycles)
{
	/**** TIMER 0 ****/
	if (TMM & 0x10) 		/* timer 0 upcounter reset ? */
		CNT0 = 0;
	else
	{
		switch (TMM & 0x0c) /* timer 0 clock source */
		{
		case 0x00:	/* clock divided by 12 */
			OVC0 += cycles;
			while (OVC0 >= 12)
			{
				OVC0 -= 12;
				CNT0++;
				if (CNT0 == TM0)
				{
					CNT0 = 0;
					IRR |= INTFT0;
					/* timer F/F source is timer 0 ? */
					if (0x00 == (TMM & 0x03))
					{
						TO ^= 1;
						if (upd7810.config.io_callback)
							(*upd7810.config.io_callback)(UPD7810_TO,TO);
					}
					/* timer 1 chained with timer 0 ? */
					if ((TMM & 0xe0) == 0x60)
					{
						CNT1++;
						if (CNT1 == TM1)
						{
							IRR |= INTFT1;
							CNT1 = 0;
							/* timer F/F source is timer 1 ? */
							if (0x01 == (TMM & 0x03))
							{
								TO ^= 1;
								if (upd7810.config.io_callback)
									(*upd7810.config.io_callback)(UPD7810_TO,TO);
							}
						}
					}
				}
			}
			break;
		case 0x04:	/* clock divided by 384 */
			OVC0 += cycles;
			while (OVC0 >= 384)
			{
				OVC0 -= 384;
				CNT0++;
				if (CNT0 == TM0)
				{
					CNT0 = 0;
					IRR |= INTFT0;
					/* timer F/F source is timer 0 ? */
					if (0x00 == (TMM & 0x03))
					{
						TO ^= 1;
						if (upd7810.config.io_callback)
							(*upd7810.config.io_callback)(UPD7810_TO,TO);
					}
					/* timer 1 chained with timer 0 ? */
					if ((TMM & 0xe0) == 0x60)
					{
						CNT1++;
						if (CNT1 == TM1)
						{
							CNT1 = 0;
							IRR |= INTFT1;
							/* timer F/F source is timer 1 ? */
							if (0x01 == (TMM & 0x03))
							{
								TO ^= 1;
								if (upd7810.config.io_callback)
									(*upd7810.config.io_callback)(UPD7810_TO,TO);
							}
						}
					}
				}
			}
			break;
		case 0x08:	/* external signal at TI */
			break;
		case 0x0c:	/* disabled */
			break;
		}
	}

	/**** TIMER 1 ****/
	if (TMM & 0x80) 		/* timer 1 upcounter reset ? */
		CNT1 = 0;
	else
	{
		switch (TMM & 0x60) /* timer 1 clock source */
		{
		case 0x00:	/* clock divided by 12 */
			OVC1 += cycles;
			while (OVC1 >= 12)
			{
				OVC1 -= 12;
				CNT1++;
				if (CNT1 == TM1)
				{
					CNT1 = 0;
					IRR |= INTFT1;
					/* timer F/F source is timer 1 ? */
					if (0x01 == (TMM & 0x03))
					{
						TO ^= 1;
						if (upd7810.config.io_callback)
							(*upd7810.config.io_callback)(UPD7810_TO,TO);
					}
				}
			}
			break;
		case 0x20:	/* clock divided by 384 */
			OVC1 += cycles;
			while (OVC1 >= 384)
			{
				OVC1 -= 384;
				CNT1++;
				if (CNT1 == TM1)
				{
					CNT1 = 0;
					IRR |= INTFT1;
					/* timer F/F source is timer 1 ? */
					if (0x01 == (TMM & 0x03))
					{
						TO ^= 1;
						if (upd7810.config.io_callback)
							(*upd7810.config.io_callback)(UPD7810_TO,TO);
					}
				}
			}
			break;
		case 0x40:	/* external signal at TI */
			break;
		case 0x60:	/* clocked with timer 0 */
			break;
		}
	}

	/**** TIMER F/F ****/
	/* timer F/F source is clock divided by 3 ? */
	if (0x02 == (TMM & 0x03))
	{
		OVCF += cycles;
		while (OVCF >= 3)
		{
			TO ^= 1;
			if (upd7810.config.io_callback)
				(*upd7810.config.io_callback)(UPD7810_TO,TO);
			OVCF -= 3;
		}
	}

	/**** ETIMER ****/
	/* ECNT clear */
	if (0x00 == (ETMM & 0x0c))
		ECNT = 0;
	else
	if (0x00 == (ETMM & 0x03) || (0x01 == (ETMM & 0x03) && CI))
	{
		OVCE += cycles;
		/* clock divided by 12 */
		while (OVCE >= 12)
		{
			OVCE -= 12;
			ECNT++;
			switch (ETMM & 0x0c)
			{
			case 0x00:				/* clear ECNT */
				break;
			case 0x04:				/* free running */
				if (0 == ECNT)
					ITF |= INTOV;	/* set overflow flag if counter wrapped */
				break;
			case 0x08:				/* reset at falling edge of CI or TO */
				break;
			case 0x0c:				/* reset if ECNT == ETM1 */
				if (ETM1 == ECNT)
					ECNT = 0;
				break;
			}
			switch (ETMM & 0x30)
			{
			case 0x00:	/* set CO0 if ECNT == ETM0 */
				if (ETM0 == ECNT)
				{
					switch (EOM & 0x0e)
					{
					case 0x02:	/* toggle CO0 */
						CO0 = (CO0 >> 1) | ((CO0 ^ 2) & 2);
						break;
					case 0x04:	/* reset CO0 */
						CO0 = 0;
						break;
					case 0x08:	/* set CO0 */
						CO0 = 1;
						break;
					}
				}
				break;
			case 0x10:	/* prohibited */
				break;
			case 0x20:	/* set CO0 if ECNT == ETM0 or at falling CI input */
				if (ETM0 == ECNT)
				{
					switch (EOM & 0x0e)
					{
					case 0x02:	/* toggle CO0 */
						CO0 = (CO0 >> 1) | ((CO0 ^ 2) & 2);
						break;
					case 0x04:	/* reset CO0 */
						CO0 = 0;
						break;
					case 0x08:	/* set CO0 */
						CO0 = 1;
						break;
					}
				}
				break;
			case 0x30:	/* latch CO0 if ECNT == ETM0 or ECNT == ETM1 */
				if (ETM0 == ECNT || ETM1 == ECNT)
				{
					switch (EOM & 0x0e)
					{
					case 0x02:	/* toggle CO0 */
						CO0 = (CO0 >> 1) | ((CO0 ^ 2) & 2);
						break;
					case 0x04:	/* reset CO0 */
						CO0 = 0;
						break;
					case 0x08:	/* set CO0 */
						CO0 = 1;
						break;
					}
				}
				break;
			}
			switch (ETMM & 0xc0)
			{
			case 0x00:	/* lacth CO1 if ECNT == ETM1 */
				if (ETM1 == ECNT)
				{
					switch (EOM & 0xe0)
					{
					case 0x20:	/* toggle CO1 */
						CO1 = (CO1 >> 1) | ((CO1 ^ 2) & 2);
						break;
					case 0x40:	/* reset CO1 */
						CO1 = 0;
						break;
					case 0x80:	/* set CO1 */
						CO1 = 1;
						break;
					}
				}
				break;
			case 0x40:	/* prohibited */
				break;
			case 0x80:	/* latch CO1 if ECNT == ETM1 or falling edge of CI input */
				if (ETM1 == ECNT)
				{
					switch (EOM & 0xe0)
					{
					case 0x20:	/* toggle CO1 */
						CO1 = (CO1 >> 1) | ((CO1 ^ 2) & 2);
						break;
					case 0x40:	/* reset CO1 */
						CO1 = 0;
						break;
					case 0x80:	/* set CO1 */
						CO1 = 1;
						break;
					}
				}
				break;
			case 0xc0:	/* latch CO1 if ECNT == ETM0 or ECNT == ETM1 */
				if (ETM0 == ECNT || ETM1 == ECNT)
				{
					switch (EOM & 0xe0)
					{
					case 0x20:	/* toggle CO1 */
						CO1 = (CO1 >> 1) | ((CO1 ^ 2) & 2);
						break;
					case 0x40:	/* reset CO1 */
						CO1 = 0;
						break;
					case 0x80:	/* set CO1 */
						CO1 = 1;
						break;
					}
				}
				break;
			}
		}
	}

	/**** SIO ****/
	switch (SMH & 0x03)
	{
	case 0x00:		/* interval timer F/F */
		break;
	case 0x01:		/* internal clock divided by 384 */
		OVCS += cycles;
		while (OVCS >= 384)
		{
			OVCS -= 384;
			if (0 == (EDGES ^= 1))
				upd7810_sio_input();
			else
				upd7810_sio_output();
		}
		break;
	case 0x02:		/* internal clock divided by 24 */
		OVCS += cycles;
		while (OVCS >= 24)
		{
			OVCS -= 24;
			if (0 == (EDGES ^= 1))
				upd7810_sio_input();
			else
				upd7810_sio_output();
		}
		break;
	}
}

void upd7810_init(void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("upd7810", cpu, "ppc", &upd7810.ppc.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "pc", &upd7810.pc.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "sp", &upd7810.sp.w.l, 1);
	state_save_register_UINT8 ("upd7810", cpu, "psw", &upd7810.psw, 1);
	state_save_register_UINT8 ("upd7810", cpu, "op", &upd7810.op, 1);
	state_save_register_UINT8 ("upd7810", cpu, "op2", &upd7810.op2, 1);
	state_save_register_UINT8 ("upd7810", cpu, "iff", &upd7810.iff, 1);
	state_save_register_UINT16("upd7810", cpu, "ea", &upd7810.ea.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "va", &upd7810.va.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "bc", &upd7810.bc.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "de", &upd7810.de.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "hl", &upd7810.hl.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "ea2", &upd7810.ea2.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "va2", &upd7810.va2.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "bc2", &upd7810.bc2.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "de2", &upd7810.de2.w.l, 1);
	state_save_register_UINT16("upd7810", cpu, "hl2", &upd7810.hl2.w.l, 1);
	state_save_register_UINT32("upd7810", cpu, "cnt", &upd7810.cnt.d, 1);
	state_save_register_UINT32("upd7810", cpu, "tm", &upd7810.tm.d, 1);
	state_save_register_UINT32("upd7810", cpu, "ecnt", &upd7810.ecnt.d, 1);
	state_save_register_UINT32("upd7810", cpu, "etm", &upd7810.etm.d, 1);
	state_save_register_UINT8 ("upd7810", cpu, "ma", &upd7810.ma, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mb", &upd7810.mb, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mcc", &upd7810.mcc, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mc", &upd7810.mc, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mm", &upd7810.mm, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mf", &upd7810.mf, 1);
	state_save_register_UINT8 ("upd7810", cpu, "tmm", &upd7810.tmm, 1);
	state_save_register_UINT8 ("upd7810", cpu, "etmm", &upd7810.etmm, 1);
	state_save_register_UINT8 ("upd7810", cpu, "eom", &upd7810.eom, 1);
	state_save_register_UINT8 ("upd7810", cpu, "sml", &upd7810.sml, 1);
	state_save_register_UINT8 ("upd7810", cpu, "smh", &upd7810.smh, 1);
	state_save_register_UINT8 ("upd7810", cpu, "anm", &upd7810.anm, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mkl", &upd7810.mkl, 1);
	state_save_register_UINT8 ("upd7810", cpu, "mkh", &upd7810.mkh, 1);
	state_save_register_UINT8 ("upd7810", cpu, "zcm", &upd7810.zcm, 1);
	state_save_register_UINT8 ("upd7810", cpu, "pa_out", &upd7810.pa_out, 1);
	state_save_register_UINT8 ("upd7810", cpu, "pb_out", &upd7810.pb_out, 1);
	state_save_register_UINT8 ("upd7810", cpu, "pc_out", &upd7810.pc_out, 1);
	state_save_register_UINT8 ("upd7810", cpu, "pd_out", &upd7810.pd_out, 1);
	state_save_register_UINT8 ("upd7810", cpu, "pf_out", &upd7810.pf_out, 1);
	state_save_register_UINT8 ("upd7810", cpu, "cr0", &upd7810.cr0, 1);
	state_save_register_UINT8 ("upd7810", cpu, "cr1", &upd7810.cr1, 1);
	state_save_register_UINT8 ("upd7810", cpu, "cr2", &upd7810.cr2, 1);
	state_save_register_UINT8 ("upd7810", cpu, "cr3", &upd7810.cr3, 1);
	state_save_register_UINT8 ("upd7810", cpu, "txb", &upd7810.txb, 1);
	state_save_register_UINT8 ("upd7810", cpu, "rxb", &upd7810.rxb, 1);
	state_save_register_UINT8 ("upd7810", cpu, "txd", &upd7810.txd, 1);
	state_save_register_UINT8 ("upd7810", cpu, "rxd", &upd7810.rxd, 1);
	state_save_register_UINT8 ("upd7810", cpu, "sck", &upd7810.sck, 1);
	state_save_register_UINT8 ("upd7810", cpu, "ti", &upd7810.ti, 1);
	state_save_register_UINT8 ("upd7810", cpu, "to", &upd7810.to, 1);
	state_save_register_UINT8 ("upd7810", cpu, "ci", &upd7810.ci, 1);
	state_save_register_UINT8 ("upd7810", cpu, "co0", &upd7810.co0, 1);
	state_save_register_UINT8 ("upd7810", cpu, "co1", &upd7810.co1, 1);
	state_save_register_UINT16("upd7810", cpu, "irr", &upd7810.irr, 1);
	state_save_register_UINT16("upd7810", cpu, "itf", &upd7810.itf, 1);
	state_save_register_INT32 ("upd7810", cpu, "ovc0", &upd7810.ovc0, 1);
	state_save_register_INT32 ("upd7810", cpu, "ovc1", &upd7810.ovc1, 1);
	state_save_register_INT32 ("upd7810", cpu, "ovcf", &upd7810.ovcf, 1);
	state_save_register_INT32 ("upd7810", cpu, "ovcs", &upd7810.ovcs, 1);
	state_save_register_UINT8 ("upd7810", cpu, "edges",&upd7810.edges,1);
}

#include "7810tbl.c"
#include "7810ops.c"

void upd7810_reset (void *param)
{
	memset(&upd7810, 0, sizeof(upd7810));
	upd7810.opXX = opXX_7810;
	upd7810.config = *(UPD7810_CONFIG*) param;
	ETMM = 0xff;
	TMM = 0xff;
	MA = 0xff;
	MB = 0xff;
	switch (upd7810.config.type)
	{
		case TYPE_7810_GAMEMASTER:
		    /* needed for lcd screen/ram selection; might be internal in cpu and therefor not needed; 0x10 written in some games*/
			MC = 0xff&~0x7;
			WP( UPD7810_PORTC, 1 ); /*hyper space*/
			PCD=0x8000;
			break;
		default:
			MC = 0xff;
	}
	MF = 0xff;
	/* gamemaster falling block "and"s to enable interrupts*/
	MKL = 0xff;
	MKH = 0xff; /*?*/
}

void upd7807_reset (void *param)
{
	upd7810_reset(param);
	upd7810.opXX = opXX_7807;
}

void upd7810_exit (void)
{
}

int upd7810_execute (int cycles)
{
	const struct opcode_s *opXX = upd7810.opXX;

	upd7810_icount = cycles;

	do
	{
		int cc = 0;

		CALL_MAME_DEBUG;

		PPC = PC;
		RDOP(OP);

		/*
		 * clear L0 and/or L1 flags for all opcodes except
		 * L0	for "MVI L,xx" or "LXI H,xxxx"
		 * L1	for "MVI A,xx"
		 */
		PSW &= ~opXX[OP].mask_l0_l1;

		/* skip flag set and not SOFTI opcode? */
		if ((PSW & SK) && (OP != 0x72))
		{
			if (opXX[OP].cycles)
			{
				cc = opXX[OP].cycles_skip;
				PC += opXX[OP].oplen - 1;
			}
			else
			{
				RDOP(OP2);
				switch (OP)
				{
				case 0x48:
					cc = op48[OP2].cycles_skip;
					PC += op48[OP2].oplen - 2;
					break;
				case 0x4c:
					cc = op4C[OP2].cycles_skip;
					PC += op4C[OP2].oplen - 2;
					break;
				case 0x4d:
					cc = op4D[OP2].cycles_skip;
					PC += op4D[OP2].oplen - 2;
					break;
				case 0x60:
					cc = op60[OP2].cycles_skip;
					PC += op60[OP2].oplen - 2;
					break;
				case 0x64:
					cc = op64[OP2].cycles_skip;
					PC += op64[OP2].oplen - 2;
					break;
				case 0x70:
					cc = op70[OP2].cycles_skip;
					PC += op70[OP2].oplen - 2;
					break;
				case 0x74:
					cc = op74[OP2].cycles_skip;
					PC += op74[OP2].oplen - 2;
					break;
				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "uPD7810 internal error: check cycle counts for main\n"); exit(1); break;
				}
			}
			PSW &= ~SK;
			upd7810_timers( cc );
			change_pc16( PCD );
		}
		else
		{
			cc = opXX[OP].cycles;
			upd7810_timers( cc );
			(*opXX[OP].opfunc)();
		}
		upd7810_icount -= cc;
		upd7810_take_irq();

	} while (upd7810_icount > 0);

	return cycles - upd7810_icount;
}

unsigned upd7810_get_context (void *dst)
{
	if (dst)
		memcpy(dst, &upd7810, sizeof(upd7810));
	return sizeof(upd7810);
}

void upd7810_set_context (void *src)
{
	if (src)
		memcpy(&upd7810, src, sizeof(upd7810));
}

unsigned upd7810_get_reg (int regnum)
{
	switch (regnum)
	{
	case REG_PC:
	case UPD7810_PC:	return PC;
	case REG_SP:
	case UPD7810_SP:	return SP;
	case UPD7810_PSW:	return PSW;
	case UPD7810_EA:	return EA;
	case UPD7810_VA:	return VA;
	case UPD7810_BC:	return BC;
	case UPD7810_DE:	return DE;
	case UPD7810_HL:	return HL;
	case UPD7810_EA2:	return EA2;
	case UPD7810_VA2:	return VA2;
	case UPD7810_BC2:	return BC2;
	case UPD7810_DE2:	return DE2;
	case UPD7810_HL2:	return HL2;
	case UPD7810_CNT0:	return CNT0;
	case UPD7810_CNT1:	return CNT1;
	case UPD7810_TM0:	return TM0;
	case UPD7810_TM1:	return TM1;
	case UPD7810_ECNT:	return ECNT;
	case UPD7810_ECPT:	return ECPT;
	case UPD7810_ETM0:	return ETM0;
	case UPD7810_ETM1:	return ETM1;
	case UPD7810_MA:	return MA;
	case UPD7810_MB:	return MB;
	case UPD7810_MCC:	return MCC;
	case UPD7810_MC:	return MC;
	case UPD7810_MM:	return MM;
	case UPD7810_MF:	return MF;
	case UPD7810_TMM:	return TMM;
	case UPD7810_ETMM:	return ETMM;
	case UPD7810_EOM:	return EOM;
	case UPD7810_SML:	return SML;
	case UPD7810_SMH:	return SMH;
	case UPD7810_ANM:	return ANM;
	case UPD7810_MKL:	return MKL;
	case UPD7810_MKH:	return MKH;
	case UPD7810_ZCM:	return ZCM;
	case UPD7810_TXB:	return TXB;
	case UPD7810_RXB:	return RXB;
	case UPD7810_CR0:	return CR0;
	case UPD7810_CR1:	return CR1;
	case UPD7810_CR2:	return CR2;
	case UPD7810_CR3:	return CR3;
	case UPD7810_TXD:	return TXD;
	case UPD7810_RXD:	return RXD;
	case UPD7810_SCK:	return SCK;
	case UPD7810_TI:	return TI;
	case UPD7810_TO:	return TO;
	case UPD7810_CI:	return CI;
	case UPD7810_CO0:	return CO0;
	case UPD7810_CO1:	return CO1;

	case REG_PREVIOUSPC:return PPC;
	default:
		if( regnum <= REG_SP_CONTENTS )
		{
			unsigned offset = SP + (REG_SP_CONTENTS - regnum);
			return RM( offset ) | ( RM( offset + 1 ) << 8 );
		}
	}
	return 0;

}

void upd7810_set_reg (int regnum, unsigned val)
{
	switch (regnum)
	{
	case REG_PC:		PC = val; change_pc16(PCD); break;
	case UPD7810_PC:	PC = val;	break;
	case REG_SP:
	case UPD7810_SP:	SP = val;	break;
	case UPD7810_PSW:	PSW = val;	break;
	case UPD7810_A:		A = val; break;
	case UPD7810_V:		V = val; break;
	case UPD7810_EA:	EA = val;	break;
	case UPD7810_VA:	VA = val;	break;
	case UPD7810_BC:	BC = val;	break;
	case UPD7810_DE:	DE = val;	break;
	case UPD7810_HL:	HL = val;	break;
	case UPD7810_EA2:	EA2 = val;	break;
	case UPD7810_VA2:	VA2 = val;	break;
	case UPD7810_BC2:	BC2 = val;	break;
	case UPD7810_DE2:	DE2 = val;	break;
	case UPD7810_HL2:	HL2 = val;	break;
	case UPD7810_CNT0:	CNT0 = val; break;
	case UPD7810_CNT1:	CNT1 = val; break;
	case UPD7810_TM0:	TM0 = val;	break;
	case UPD7810_TM1:	TM1 = val;	break;
	case UPD7810_ECNT:	ECNT = val; break;
	case UPD7810_ECPT:	ECPT = val; break;
	case UPD7810_ETM0:	ETM0 = val; break;
	case UPD7810_ETM1:	ETM1 = val; break;
	case UPD7810_MA:	MA = val;	break;
	case UPD7810_MB:	MB = val;	break;
	case UPD7810_MCC:	MCC = val;	break;
	case UPD7810_MC:	MC = val;	break;
	case UPD7810_MM:	MM = val;	break;
	case UPD7810_MF:	MF = val;	break;
	case UPD7810_TMM:	TMM = val;	break;
	case UPD7810_ETMM:	ETMM = val; break;
	case UPD7810_EOM:	EOM = val;	break;
	case UPD7810_SML:	SML = val;	break;
	case UPD7810_SMH:	SMH = val;	break;
	case UPD7810_ANM:	ANM = val;	break;
	case UPD7810_MKL:	MKL = val;	break;
	case UPD7810_MKH:	MKH = val;	break;
	case UPD7810_ZCM:	ZCM = val;	break;
	case UPD7810_TXB:	TXB = val;	break;
	case UPD7810_RXB:	RXB = val;	break;
	case UPD7810_CR0:	CR0 = val;	break;
	case UPD7810_CR1:	CR1 = val;	break;
	case UPD7810_CR2:	CR2 = val;	break;
	case UPD7810_CR3:	CR3 = val;	break;
	case UPD7810_TXD:	TXD = val;	break;
	case UPD7810_RXD:	RXD = val;	break;
	case UPD7810_SCK:	SCK = val;	break;
	case UPD7810_TI:	TI	= val;	break;
	case UPD7810_TO:	TO	= val;	break;
	case UPD7810_CI:	CI	= val;	break;
	case UPD7810_CO0:	CO0 = val;	break;
	case UPD7810_CO1:	CO1 = val;	break;
	default:
		if( regnum <= REG_SP_CONTENTS )
		{
			unsigned offset = SP + (REG_SP_CONTENTS - regnum);
			WM( offset, val & 0xff );
			WM( offset + 1, (val >> 8) & 0xff );
		}
	}
}

void upd7810_set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE)
	{
		if (irqline == IRQ_LINE_NMI)
		{
			/* no nested NMIs ? */
#if 0			
	        if (0 == (IRR & INTNMI))
#endif			
			{
	            IRR |= INTNMI;
				SP--;
				WM( SP, PSW );
				SP--;
				WM( SP, PCH );
				SP--;
				WM( SP, PCL );
				IFF = 0;
				PSW &= ~(SK|L0|L1);
				PC = 0x0004;
				change_pc16( PCD );
			}
		}
		else
		if (irqline == UPD7810_INTF1)
			IRR |= INTF1;
		else
		if (irqline == UPD7810_INTF2)
			IRR |= INTF2;
		/* gamemaster hack*/
		else
		if (irqline == UPD7810_INTFE1)
			IRR |= INTFE1;
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "upd7810_set_irq_line invalid irq line #%d\n", irqline);
	}
	/* resetting interrupt requests is done with the SKIT/SKNIT opcodes only! */
}

void upd7810_set_irq_callback(int (*callback)(int irqline))
{
	upd7810.irq_callback = callback;
}

const char *upd7810_info(void *context, int regnum)
{
	static char buffer[8][31+1];
	static int which = 0;
	UPD7810 *r = context;

	which = (which+1) % 8;
	buffer[which][0] = '\0';
	if( !context )
		r = &upd7810;

	switch( regnum )
	{
		case CPU_INFO_REG+UPD7810_PC:	sprintf(buffer[which], "PC  :%04X", r->pc.w.l); break;
		case CPU_INFO_REG+UPD7810_SP:	sprintf(buffer[which], "SP  :%04X", r->sp.w.l); break;
		case CPU_INFO_REG+UPD7810_PSW:	sprintf(buffer[which], "PSW :%02X", r->psw); break;
		case CPU_INFO_REG+UPD7810_A:	sprintf(buffer[which], "A   :%02X", r->va.b.l); break;
		case CPU_INFO_REG+UPD7810_V:	sprintf(buffer[which], "V   :%02X", r->va.b.h); break;
		case CPU_INFO_REG+UPD7810_EA:	sprintf(buffer[which], "EA  :%04X", r->ea.w.l); break;
		case CPU_INFO_REG+UPD7810_BC:	sprintf(buffer[which], "BC  :%04X", r->bc.w.l); break;
		case CPU_INFO_REG+UPD7810_DE:	sprintf(buffer[which], "DE  :%04X", r->de.w.l); break;
		case CPU_INFO_REG+UPD7810_HL:	sprintf(buffer[which], "HL  :%04X", r->hl.w.l); break;
		case CPU_INFO_REG+UPD7810_A2:	sprintf(buffer[which], "A'  :%02X", r->va2.b.l); break;
		case CPU_INFO_REG+UPD7810_V2:	sprintf(buffer[which], "V'  :%02X", r->va2.b.h); break;
		case CPU_INFO_REG+UPD7810_EA2:	sprintf(buffer[which], "EA' :%04X", r->ea2.w.l); break;
		case CPU_INFO_REG+UPD7810_BC2:	sprintf(buffer[which], "BC' :%04X", r->bc2.w.l); break;
		case CPU_INFO_REG+UPD7810_DE2:	sprintf(buffer[which], "DE' :%04X", r->de2.w.l); break;
		case CPU_INFO_REG+UPD7810_HL2:	sprintf(buffer[which], "HL' :%04X", r->hl2.w.l); break;
		case CPU_INFO_REG+UPD7810_CNT0: sprintf(buffer[which], "CNT0:%02X", r->cnt.b.l); break;
		case CPU_INFO_REG+UPD7810_CNT1: sprintf(buffer[which], "CNT1:%02X", r->cnt.b.h); break;
		case CPU_INFO_REG+UPD7810_TM0:	sprintf(buffer[which], "TM0 :%02X", r->tm.b.l); break;
		case CPU_INFO_REG+UPD7810_TM1:	sprintf(buffer[which], "TM1 :%02X", r->tm.b.h); break;
		case CPU_INFO_REG+UPD7810_ECNT: sprintf(buffer[which], "ECNT:%04X", r->ecnt.w.l); break;
		case CPU_INFO_REG+UPD7810_ECPT: sprintf(buffer[which], "ECPT:%04X", r->ecnt.w.h); break;
		case CPU_INFO_REG+UPD7810_ETM0: sprintf(buffer[which], "ETM0:%04X", r->etm.w.l); break;
		case CPU_INFO_REG+UPD7810_ETM1: sprintf(buffer[which], "ETM1:%04X", r->etm.w.h); break;
		case CPU_INFO_REG+UPD7810_MA:	sprintf(buffer[which], "MA  :%02X", r->ma); break;
		case CPU_INFO_REG+UPD7810_MB:	sprintf(buffer[which], "MB  :%02X", r->mb); break;
		case CPU_INFO_REG+UPD7810_MCC:	sprintf(buffer[which], "MCC :%02X", r->mcc); break;
		case CPU_INFO_REG+UPD7810_MC:	sprintf(buffer[which], "MC  :%02X", r->mc); break;
		case CPU_INFO_REG+UPD7810_MM:	sprintf(buffer[which], "MM  :%02X", r->mm); break;
		case CPU_INFO_REG+UPD7810_MF:	sprintf(buffer[which], "MF  :%02X", r->mf); break;
		case CPU_INFO_REG+UPD7810_TMM:	sprintf(buffer[which], "TMM :%02X", r->tmm); break;
		case CPU_INFO_REG+UPD7810_ETMM: sprintf(buffer[which], "ETMM:%02X", r->etmm); break;
		case CPU_INFO_REG+UPD7810_EOM:	sprintf(buffer[which], "EOM :%02X", r->eom); break;
		case CPU_INFO_REG+UPD7810_SML:	sprintf(buffer[which], "SML :%02X", r->sml); break;
		case CPU_INFO_REG+UPD7810_SMH:	sprintf(buffer[which], "SMH :%02X", r->smh); break;
		case CPU_INFO_REG+UPD7810_ANM:	sprintf(buffer[which], "ANM :%02X", r->anm); break;
		case CPU_INFO_REG+UPD7810_MKL:	sprintf(buffer[which], "MKL :%02X", r->mkl); break;
		case CPU_INFO_REG+UPD7810_MKH:	sprintf(buffer[which], "MKH :%02X", r->mkh); break;
		case CPU_INFO_REG+UPD7810_ZCM:	sprintf(buffer[which], "ZCM :%02X", r->zcm); break;
		case CPU_INFO_REG+UPD7810_CR0:	sprintf(buffer[which], "CR0 :%02X", r->cr0); break;
		case CPU_INFO_REG+UPD7810_CR1:	sprintf(buffer[which], "CR1 :%02X", r->cr1); break;
		case CPU_INFO_REG+UPD7810_CR2:	sprintf(buffer[which], "CR2 :%02X", r->cr2); break;
		case CPU_INFO_REG+UPD7810_CR3:	sprintf(buffer[which], "CR3 :%02X", r->cr3); break;
		case CPU_INFO_REG+UPD7810_RXB:	sprintf(buffer[which], "RXB :%02X", r->rxb); break;
		case CPU_INFO_REG+UPD7810_TXB:	sprintf(buffer[which], "TXB :%02X", r->txb); break;
		case CPU_INFO_REG+UPD7810_TXD:	sprintf(buffer[which], "TXD :%d", r->txd); break;
		case CPU_INFO_REG+UPD7810_RXD:	sprintf(buffer[which], "RXD :%d", r->rxd); break;
		case CPU_INFO_REG+UPD7810_SCK:	sprintf(buffer[which], "SCK :%d", r->sck); break;
		case CPU_INFO_REG+UPD7810_TI:	sprintf(buffer[which], "TI  :%d", r->ti); break;
		case CPU_INFO_REG+UPD7810_TO:	sprintf(buffer[which], "TO  :%d", r->to); break;
		case CPU_INFO_REG+UPD7810_CI:	sprintf(buffer[which], "CI  :%d", r->ci); break;
		case CPU_INFO_REG+UPD7810_CO0:	sprintf(buffer[which], "CO0 :%d", r->co0 & 1); break;
		case CPU_INFO_REG+UPD7810_CO1:	sprintf(buffer[which], "CO1 :%d", r->co1 & 1); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%s:%s:%s:%s:%s:%s",
				r->psw & 0x40 ? "ZF":"--",
				r->psw & 0x20 ? "SK":"--",
				r->psw & 0x10 ? "HC":"--",
				r->psw & 0x08 ? "L1":"--",
				r->psw & 0x04 ? "L0":"--",
				r->psw & 0x01 ? "CY":"--");
			break;
		case CPU_INFO_NAME: return "uPD7810";
		case CPU_INFO_FAMILY: return "NEC uPD7810";
		case CPU_INFO_VERSION: return "0.3";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 2001 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)upd7810_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)upd7810_win_layout;
	}
	return buffer[which];
}

const char *upd7807_info(void *context, int regnum)
{
	if (regnum == CPU_INFO_NAME) return "uPD7807";
	else return upd7810_info(context, regnum);
}

unsigned upd7810_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm7810( buffer, pc );
#else
	UINT8 op = cpu_readop(pc);
	sprintf( buffer, "$%02X", op );
	return 1;
#endif
}


unsigned upd7807_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm7807( buffer, pc );
#else
	UINT8 op = cpu_readop(pc);
	sprintf( buffer, "$%02X", op );
	return 1;
#endif
}
