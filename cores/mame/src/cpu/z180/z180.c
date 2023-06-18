/*****************************************************************************
 *
 *	 z180.c
 *	 Portable Z180 emulator V0.2
 *
 *	 Copyright (C) 2000 Juergen Buchmueller, all rights reserved.
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

#include "driver.h"
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "z180.h"
#include "log.h"

/* execute main opcodes inside a big switch statement */
/* execute main opcodes inside a big switch statement */
#ifndef BIG_SWITCH
#define BIG_SWITCH			1
#endif

/* use big flag arrays (4*64K) for ADD/ADC/SUB/SBC/CP results */
#define BIG_FLAGS_ARRAY 	1

/* Set to 1 for a more exact (but somewhat slower) Z180 emulation */
#define Z180_EXACT			1

/* on JP and JR opcodes check for tight loops */
#define BUSY_LOOP_HACKS 	1

/* check for delay loops counting down BC */
#define TIME_LOOP_HACKS 	1

static UINT8 z180_reg_layout[] = {
	Z180_PC, Z180_SP, Z180_AF,	Z180_BC,	Z180_DE,   Z180_HL, -1,
	Z180_IX, Z180_IY, Z180_AF2, Z180_BC2,	Z180_DE2,  Z180_HL2, -1,
	Z180_R,  Z180_I,  Z180_IL,	Z180_IM,	Z180_IFF1, Z180_IFF2, -1,
	Z180_INT0_STATE,Z180_INT1_STATE,Z180_INT2_STATE,Z180_DC0, Z180_DC1,Z180_DC2,Z180_DC3, -1,
	Z180_CCR,Z180_ITC,Z180_CBR, Z180_BBR,	Z180_CBAR, Z180_OMCR, 0
};

static UINT8 z180_win_layout[] = {
	27, 0,53, 6,	/* register window (top rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 7,53, 6,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/****************************************************************************/
/* The Z180 registers. HALT is set to 1 when the CPU is halted, the refresh */
/* register is calculated as follows: refresh=(Regs.R&127)|(Regs.R2&128)	*/
/****************************************************************************/
typedef struct {
/* 00 */	PAIR	PREPC,PC,SP,AF,BC,DE,HL,IX,IY;
/* 24 */	PAIR	AF2,BC2,DE2,HL2;
/* 34 */	UINT8	R,R2,IFF1,IFF2,HALT,IM,I;
/* 3B */	UINT8	tmdr_latch; 		/* flag latched TMDR0H, TMDR1H values */
/* 3C */	UINT32	iol;				/* I/O line status bits */
/* 40 */	UINT8	io[64]; 			/* 64 internal 8 bit registers */
/* 80 */	offs_t	mmu[16];			/* MMU address translation */
/* c0 */	UINT8	tmdr[2];			/* latched TMDR0H and TMDR1H values */
/* c2 */	UINT8	irq_max;			/* number of daisy chain devices		*/
/* c3 */	INT8	request_irq;		/* daisy chain next request device		*/
/* c4 */	INT8	service_irq;		/* daisy chain next reti handling device */
/* c5 */	UINT8	nmi_state;			/* nmi line state */
/* c6 */	UINT8	irq_state[10];		/* irq line states (INT0,INT1,INT2) */
/* d0 */	UINT8	int_state[Z80_MAXDAISY];
/* d4 */	Z80_DaisyChain irq[Z80_MAXDAISY];
/* e4 */	int 	(*irq_callback)(int irqline);
/* e8 */	int 	extra_cycles;		/* extra cycles for interrupts */
}	Z180_Regs;

#define CF	0x01
#define NF	0x02
#define PF	0x04
#define VF	PF
#define XF	0x08
#define HF	0x10
#define YF	0x20
#define ZF	0x40
#define SF	0x80

/* I/O line status flags */
#define Z180_CKA0	  0x00000001  /* I/O asynchronous clock 0 (active high) or DREQ0 (mux) */
#define Z180_CKA1	  0x00000002  /* I/O asynchronous clock 1 (active high) or TEND1 (mux) */
#define Z180_CKS	  0x00000004  /* I/O serial clock (active high) */
#define Z180_CTS0	  0x00000100  /* I	 clear to send 0 (active low) */
#define Z180_CTS1	  0x00000200  /* I	 clear to send 1 (active low) or RXS (mux) */
#define Z180_DCD0	  0x00000400  /* I	 data carrier detect (active low) */
#define Z180_DREQ0	  0x00000800  /* I	 data request DMA ch 0 (active low) or CKA0 (mux) */
#define Z180_DREQ1	  0x00001000  /* I	 data request DMA ch 1 (active low) */
#define Z180_RXA0	  0x00002000  /* I	 asynchronous receive data 0 (active high) */
#define Z180_RXA1	  0x00004000  /* I	 asynchronous receive data 1 (active high) */
#define Z180_RXS	  0x00008000  /* I	 clocked serial receive data (active high) or CTS1 (mux) */
#define Z180_RTS0	  0x00010000  /*   O request to send (active low) */
#define Z180_TEND0	  0x00020000  /*   O transfer end 0 (active low) or CKA1 (mux) */
#define Z180_TEND1	  0x00040000  /*   O transfer end 1 (active low) */
#define Z180_A18_TOUT 0x00080000  /*   O transfer out (PRT channel, active low) or A18 (mux) */
#define Z180_TXA0	  0x00100000  /*   O asynchronous transmit data 0 (active high) */
#define Z180_TXA1	  0x00200000  /*   O asynchronous transmit data 1 (active high) */
#define Z180_TXS	  0x00400000  /*   O clocked serial transmit data (active high) */

#define _PPC	Z180.PREPC.d	/* previous program counter */

#define _PCD	Z180.PC.d
#define _PC 	Z180.PC.w.l

#define _SPD	Z180.SP.d
#define _SP 	Z180.SP.w.l

#define _AFD	Z180.AF.d
#define _AF 	Z180.AF.w.l
#define _A		Z180.AF.b.h
#define _F		Z180.AF.b.l

#define _BCD	Z180.BC.d
#define _BC 	Z180.BC.w.l
#define _B		Z180.BC.b.h
#define _C		Z180.BC.b.l

#define _DED	Z180.DE.d
#define _DE 	Z180.DE.w.l
#define _D		Z180.DE.b.h
#define _E		Z180.DE.b.l

#define _HLD	Z180.HL.d
#define _HL 	Z180.HL.w.l
#define _H		Z180.HL.b.h
#define _L		Z180.HL.b.l

#define _IXD	Z180.IX.d
#define _IX 	Z180.IX.w.l
#define _HX 	Z180.IX.b.h
#define _LX 	Z180.IX.b.l

#define _IYD	Z180.IY.d
#define _IY 	Z180.IY.w.l
#define _HY 	Z180.IY.b.h
#define _LY 	Z180.IY.b.l

#define _I		Z180.I
#define _R		Z180.R
#define _R2 	Z180.R2
#define _IM 	Z180.IM
#define _IFF1	Z180.IFF1
#define _IFF2	Z180.IFF2
#define _HALT	Z180.HALT

#define IO(n)		Z180.io[(n)-Z180_CNTLA0]
#define IO_CNTLA0	IO(Z180_CNTLA0)
#define IO_CNTLA1	IO(Z180_CNTLA1)
#define IO_CNTLB0	IO(Z180_CNTLB0)
#define IO_CNTLB1	IO(Z180_CNTLB1)
#define IO_STAT0	IO(Z180_STAT0)
#define IO_STAT1	IO(Z180_STAT1)
#define IO_TDR0 	IO(Z180_TDR0)
#define IO_TDR1 	IO(Z180_TDR1)
#define IO_RDR0 	IO(Z180_RDR0)
#define IO_RDR1 	IO(Z180_RDR1)
#define IO_CNTR 	IO(Z180_CNTR)
#define IO_TRDR 	IO(Z180_TRDR)
#define IO_TMDR0L	IO(Z180_TMDR0L)
#define IO_TMDR0H	IO(Z180_TMDR0H)
#define IO_RLDR0L	IO(Z180_RLDR0L)
#define IO_RLDR0H	IO(Z180_RLDR0H)
#define IO_TCR		IO(Z180_TCR)
#define IO_IO11 	IO(Z180_IO11)
#define IO_ASEXT0	IO(Z180_ASEXT0)
#define IO_ASEXT1	IO(Z180_ASEXT1)
#define IO_TMDR1L	IO(Z180_TMDR1L)
#define IO_TMDR1H	IO(Z180_TMDR1H)
#define IO_RLDR1L	IO(Z180_RLDR1L)
#define IO_RLDR1H	IO(Z180_RLDR1H)
#define IO_FRC		IO(Z180_FRC)
#define IO_IO19 	IO(Z180_IO19)
#define IO_ASTC0L	IO(Z180_ASTC0L)
#define IO_ASTC0H	IO(Z180_ASTC0H)
#define IO_ASTC1L	IO(Z180_ASTC1L)
#define IO_ASTC1H	IO(Z180_ASTC1H)
#define IO_CMR		IO(Z180_CMR)
#define IO_CCR		IO(Z180_CCR)
#define IO_SAR0L	IO(Z180_SAR0L)
#define IO_SAR0H	IO(Z180_SAR0H)
#define IO_SAR0B	IO(Z180_SAR0B)
#define IO_DAR0L	IO(Z180_DAR0L)
#define IO_DAR0H	IO(Z180_DAR0H)
#define IO_DAR0B	IO(Z180_DAR0B)
#define IO_BCR0L	IO(Z180_BCR0L)
#define IO_BCR0H	IO(Z180_BCR0H)
#define IO_MAR1L	IO(Z180_MAR1L)
#define IO_MAR1H	IO(Z180_MAR1H)
#define IO_MAR1B	IO(Z180_MAR1B)
#define IO_IAR1L	IO(Z180_IAR1L)
#define IO_IAR1H	IO(Z180_IAR1H)
#define IO_IAR1B	IO(Z180_IAR1B)
#define IO_BCR1L	IO(Z180_BCR1L)
#define IO_BCR1H	IO(Z180_BCR1H)
#define IO_DSTAT	IO(Z180_DSTAT)
#define IO_DMODE	IO(Z180_DMODE)
#define IO_DCNTL	IO(Z180_DCNTL)
#define IO_IL		IO(Z180_IL)
#define IO_ITC		IO(Z180_ITC)
#define IO_IO35 	IO(Z180_IO35)
#define IO_RCR		IO(Z180_RCR)
#define IO_IO37 	IO(Z180_IO37)
#define IO_CBR		IO(Z180_CBR)
#define IO_BBR		IO(Z180_BBR)
#define IO_CBAR 	IO(Z180_CBAR)
#define IO_IO3B 	IO(Z180_IO3B)
#define IO_IO3C 	IO(Z180_IO3C)
#define IO_IO3D 	IO(Z180_IO3D)
#define IO_OMCR 	IO(Z180_OMCR)
#define IO_IOCR 	IO(Z180_IOCR)

/* 00 ASCI control register A ch 0 */
#define Z180_CNTLA0_MPE 		0x80
#define Z180_CNTLA0_RE			0x40
#define Z180_CNTLA0_TE			0x20
#define Z180_CNTLA0_RTS0		0x10
#define Z180_CNTLA0_MPBR_EFR	0x08
#define Z180_CNTLA0_MODE_DATA	0x04
#define Z180_CNTLA0_MODE_PARITY 0x02
#define Z180_CNTLA0_MODE_STOPB	0x01

#define Z180_CNTLA0_RESET		0x10
#define Z180_CNTLA0_RMASK		0xff
#define Z180_CNTLA0_WMASK		0xff

/* 01 ASCI control register A ch 1 */
#define Z180_CNTLA1_MPE 		0x80
#define Z180_CNTLA1_RE			0x40
#define Z180_CNTLA1_TE			0x20
#define Z180_CNTLA1_CKA1D		0x10
#define Z180_CNTLA1_MPBR_EFR	0x08
#define Z180_CNTLA1_MODE		0x07

#define Z180_CNTLA1_RESET		0x10
#define Z180_CNTLA1_RMASK		0xff
#define Z180_CNTLA1_WMASK		0xff

/* 02 ASCI control register B ch 0 */
#define Z180_CNTLB0_MPBT		0x80
#define Z180_CNTLB0_MP			0x40
#define Z180_CNTLB0_CTS_PS		0x20
#define Z180_CNTLB0_PEO 		0x10
#define Z180_CNTLB0_DR			0x08
#define Z180_CNTLB0_SS			0x07

#define Z180_CNTLB0_RESET		0x07
#define Z180_CNTLB0_RMASK		0xff
#define Z180_CNTLB0_WMASK		0xff

/* 03 ASCI control register B ch 1 */
#define Z180_CNTLB1_MPBT		0x80
#define Z180_CNTLB1_MP			0x40
#define Z180_CNTLB1_CTS_PS		0x20
#define Z180_CNTLB1_PEO 		0x10
#define Z180_CNTLB1_DR			0x08
#define Z180_CNTLB1_SS			0x07

#define Z180_CNTLB1_RESET		0x07
#define Z180_CNTLB1_RMASK		0xff
#define Z180_CNTLB1_WMASK		0xff

/* 04 ASCI status register 0 */
#define Z180_STAT0_RDRF 		0x80
#define Z180_STAT0_OVRN 		0x40
#define Z180_STAT0_PE			0x20
#define Z180_STAT0_FE			0x10
#define Z180_STAT0_RIE			0x08
#define Z180_STAT0_DCD0 		0x04
#define Z180_STAT0_TDRE 		0x02
#define Z180_STAT0_TIE			0x01

#define Z180_STAT0_RESET		0x00
#define Z180_STAT0_RMASK		0xff
#define Z180_STAT0_WMASK		0x09

/* 05 ASCI status register 1 */
#define Z180_STAT1_RDRF 		0x80
#define Z180_STAT1_OVRN 		0x40
#define Z180_STAT1_PE			0x20
#define Z180_STAT1_FE			0x10
#define Z180_STAT1_RIE			0x08
#define Z180_STAT1_CTS1E		0x04
#define Z180_STAT1_TDRE 		0x02
#define Z180_STAT1_TIE			0x01

#define Z180_STAT1_RESET		0x00
#define Z180_STAT1_RMASK		0xff
#define Z180_STAT1_WMASK		0x0d

/* 06 ASCI transmit data register 0 */
#define Z180_TDR0_TDR			0xff

#define Z180_TDR0_RESET 		0x00
#define Z180_TDR0_RMASK 		0xff
#define Z180_TDR0_WMASK 		0xff

/* 07 ASCI transmit data register 1 */
#define Z180_TDR1_TDR			0xff

#define Z180_TDR1_RESET 		0x00
#define Z180_TDR1_RMASK 		0xff
#define Z180_TDR1_WMASK 		0xff

/* 08 ASCI receive register 0 */
#define Z180_RDR0_RDR			0xff

#define Z180_RDR0_RESET 		0x00
#define Z180_RDR0_RMASK 		0xff
#define Z180_RDR0_WMASK 		0xff

/* 09 ASCI receive register 1 */
#define Z180_RDR1_RDR			0xff

#define Z180_RDR1_RESET 		0x00
#define Z180_RDR1_RMASK 		0xff
#define Z180_RDR1_WMASK 		0xff

/* 0a CSI/O control/status register */
#define Z180_CNTR_EF			0x80
#define Z180_CNTR_EIE			0x40
#define Z180_CNTR_RE			0x20
#define Z180_CNTR_TE			0x10
#define Z180_CNTR_SS			0x07

#define Z180_CNTR_RESET 		0x07
#define Z180_CNTR_RMASK 		0xff
#define Z180_CNTR_WMASK 		0x7f

/* 0b CSI/O transmit/receive register */
#define Z180_TRDR_RESET 		0x00
#define Z180_TRDR_RMASK 		0xff
#define Z180_TRDR_WMASK 		0xff

/* 0c TIMER data register ch 0 L */
#define Z180_TMDR0L_RESET		0x00
#define Z180_TMDR0L_RMASK		0xff
#define Z180_TMDR0L_WMASK		0xff

/* 0d TIMER data register ch 0 H */
#define Z180_TMDR0H_RESET		0x00
#define Z180_TMDR0H_RMASK		0xff
#define Z180_TMDR0H_WMASK		0xff

/* 0e TIMER reload register ch 0 L */
#define Z180_RLDR0L_RESET		0xff
#define Z180_RLDR0L_RMASK		0xff
#define Z180_RLDR0L_WMASK		0xff

/* 0f TIMER reload register ch 0 H */
#define Z180_RLDR0H_RESET		0xff
#define Z180_RLDR0H_RMASK		0xff
#define Z180_RLDR0H_WMASK		0xff

/* 10 TIMER control register */
#define Z180_TCR_TIF1			0x80
#define Z180_TCR_TIF0			0x40
#define Z180_TCR_TIE1			0x20
#define Z180_TCR_TIE0			0x10
#define Z180_TCR_TOC1			0x08
#define Z180_TCR_TOC0			0x04
#define Z180_TCR_TDE1			0x02
#define Z180_TCR_TDE0			0x01

#define Z180_TCR_RESET			0x00
#define Z180_TCR_RMASK			0xff
#define Z180_TCR_WMASK			0xff

/* 11 reserved */
#define Z180_IO11_RESET 		0x00
#define Z180_IO11_RMASK 		0xff
#define Z180_IO11_WMASK 		0xff

/* 12 (Z8S180/Z8L180) ASCI extension control register 0 */
#define Z180_ASEXT0_RDRF		0x80
#define Z180_ASEXT0_DCD0		0x40
#define Z180_ASEXT0_CTS0		0x20
#define Z180_ASEXT0_X1_BIT_CLK0 0x10
#define Z180_ASEXT0_BRG0_MODE	0x08
#define Z180_ASEXT0_BRK_EN		0x04
#define Z180_ASEXT0_BRK_DET 	0x02
#define Z180_ASEXT0_BRK_SEND	0x01

#define Z180_ASEXT0_RESET		0x00
#define Z180_ASEXT0_RMASK		0xff
#define Z180_ASEXT0_WMASK		0xfd

/* 13 (Z8S180/Z8L180) ASCI extension control register 0 */
#define Z180_ASEXT1_RDRF		0x80
#define Z180_ASEXT1_X1_BIT_CLK1 0x10
#define Z180_ASEXT1_BRG1_MODE	0x08
#define Z180_ASEXT1_BRK_EN		0x04
#define Z180_ASEXT1_BRK_DET 	0x02
#define Z180_ASEXT1_BRK_SEND	0x01

#define Z180_ASEXT1_RESET		0x00
#define Z180_ASEXT1_RMASK		0xff
#define Z180_ASEXT1_WMASK		0xfd


/* 14 TIMER data register ch 1 L */
#define Z180_TMDR1L_RESET		0x00
#define Z180_TMDR1L_RMASK		0xff
#define Z180_TMDR1L_WMASK		0xff

/* 15 TIMER data register ch 1 H */
#define Z180_TMDR1H_RESET		0x00
#define Z180_TMDR1H_RMASK		0xff
#define Z180_TMDR1H_WMASK		0xff

/* 16 TIMER reload register ch 1 L */
#define Z180_RLDR1L_RESET		0x00
#define Z180_RLDR1L_RMASK		0xff
#define Z180_RLDR1L_WMASK		0xff

/* 17 TIMER reload register ch 1 H */
#define Z180_RLDR1H_RESET		0x00
#define Z180_RLDR1H_RMASK		0xff
#define Z180_RLDR1H_WMASK		0xff

/* 18 free running counter */
#define Z180_FRC_RESET			0x00
#define Z180_FRC_RMASK			0xff
#define Z180_FRC_WMASK			0xff

/* 19 reserved */
#define Z180_IO19_RESET 		0x00
#define Z180_IO19_RMASK 		0xff
#define Z180_IO19_WMASK 		0xff

/* 1a ASCI time constant ch 0 L */
#define Z180_ASTC0L_RESET		0x00
#define Z180_ASTC0L_RMASK		0xff
#define Z180_ASTC0L_WMASK		0xff

/* 1b ASCI time constant ch 0 H */
#define Z180_ASTC0H_RESET		0x00
#define Z180_ASTC0H_RMASK		0xff
#define Z180_ASTC0H_WMASK		0xff

/* 1c ASCI time constant ch 1 L */
#define Z180_ASTC1L_RESET		0x00
#define Z180_ASTC1L_RMASK		0xff
#define Z180_ASTC1L_WMASK		0xff

/* 1d ASCI time constant ch 1 H */
#define Z180_ASTC1H_RESET		0x00
#define Z180_ASTC1H_RMASK		0xff
#define Z180_ASTC1H_WMASK		0xff

/* 1e clock multiplier */
#define Z180_CMR_X2 			0x80

#define Z180_CMR_RESET			0x7f
#define Z180_CMR_RMASK			0x80
#define Z180_CMR_WMASK			0x80

/* 1f chip control register */
#define Z180_CCR_CLOCK_DIVIDE	0x80
#define Z180_CCR_STDBY_IDLE1	0x40
#define Z180_CCR_BREXT			0x20
#define Z180_CCR_LNPHI			0x10
#define Z180_CCR_STDBY_IDLE0	0x08
#define Z180_CCR_LNIO			0x04
#define Z180_CCR_LNCPU_CTL		0x02
#define Z180_CCR_LNAD_DATA		0x01

#define Z180_CCR_RESET			0x00
#define Z180_CCR_RMASK			0xff
#define Z180_CCR_WMASK			0xff

/* 20 DMA source address register ch 0 L */
#define Z180_SAR0L_SAR			0xff

#define Z180_SAR0L_RESET		0x00
#define Z180_SAR0L_RMASK		0xff
#define Z180_SAR0L_WMASK		0xff

/* 21 DMA source address register ch 0 H */
#define Z180_SAR0H_SAR			0xff

#define Z180_SAR0H_RESET		0x00
#define Z180_SAR0H_RMASK		0xff
#define Z180_SAR0H_WMASK		0xff

/* 22 DMA source address register ch 0 B */
#define Z180_SAR0B_SAR			0x0f

#define Z180_SAR0B_RESET		0x00
#define Z180_SAR0B_RMASK		0x0f
#define Z180_SAR0B_WMASK		0x0f

/* 23 DMA destination address register ch 0 L */
#define Z180_DAR0L_DAR			0xff

#define Z180_DAR0L_RESET		0x00
#define Z180_DAR0L_RMASK		0xff
#define Z180_DAR0L_WMASK		0xff

/* 24 DMA destination address register ch 0 H */
#define Z180_DAR0H_DAR			0xff

#define Z180_DAR0H_RESET		0x00
#define Z180_DAR0H_RMASK		0xff
#define Z180_DAR0H_WMASK		0xff

/* 25 DMA destination address register ch 0 B */
#define Z180_DAR0B_DAR			0x00

#define Z180_DAR0B_RESET		0x00
#define Z180_DAR0B_RMASK		0x0f
#define Z180_DAR0B_WMASK		0x0f

/* 26 DMA byte count register ch 0 L */
#define Z180_BCR0L_BCR			0xff

#define Z180_BCR0L_RESET		0x00
#define Z180_BCR0L_RMASK		0xff
#define Z180_BCR0L_WMASK		0xff

/* 27 DMA byte count register ch 0 H */
#define Z180_BCR0H_BCR			0xff

#define Z180_BCR0H_RESET		0x00
#define Z180_BCR0H_RMASK		0xff
#define Z180_BCR0H_WMASK		0xff

/* 28 DMA memory address register ch 1 L */
#define Z180_MAR1L_MAR			0xff

#define Z180_MAR1L_RESET		0x00
#define Z180_MAR1L_RMASK		0xff
#define Z180_MAR1L_WMASK		0xff

/* 29 DMA memory address register ch 1 H */
#define Z180_MAR1H_MAR			0xff

#define Z180_MAR1H_RESET		0x00
#define Z180_MAR1H_RMASK		0xff
#define Z180_MAR1H_WMASK		0xff

/* 2a DMA memory address register ch 1 B */
#define Z180_MAR1B_MAR			0x0f

#define Z180_MAR1B_RESET		0x00
#define Z180_MAR1B_RMASK		0x0f
#define Z180_MAR1B_WMASK		0x0f

/* 2b DMA I/O address register ch 1 L */
#define Z180_IAR1L_IAR			0xff

#define Z180_IAR1L_RESET		0x00
#define Z180_IAR1L_RMASK		0xff
#define Z180_IAR1L_WMASK		0xff

/* 2c DMA I/O address register ch 1 H */
#define Z180_IAR1H_IAR			0xff

#define Z180_IAR1H_RESET		0x00
#define Z180_IAR1H_RMASK		0xff
#define Z180_IAR1H_WMASK		0xff

/* 2d (Z8S180/Z8L180) DMA I/O address register ch 1 B */
#define Z180_IAR1B_IAR			0x0f

#define Z180_IAR1B_RESET		0x00
#define Z180_IAR1B_RMASK		0x0f
#define Z180_IAR1B_WMASK		0x0f

/* 2e DMA byte count register ch 1 L */
#define Z180_BCR1L_BCR			0xff

#define Z180_BCR1L_RESET		0x00
#define Z180_BCR1L_RMASK		0xff
#define Z180_BCR1L_WMASK		0xff

/* 2f DMA byte count register ch 1 H */
#define Z180_BCR1H_BCR			0xff

#define Z180_BCR1H_RESET		0x00
#define Z180_BCR1H_RMASK		0xff
#define Z180_BCR1H_WMASK		0xff

/* 30 DMA status register */
#define Z180_DSTAT_DE1			0x80	/* DMA enable ch 1 */
#define Z180_DSTAT_DE0			0x40	/* DMA enable ch 0 */
#define Z180_DSTAT_DWE1 		0x20	/* DMA write enable ch 0 (active low) */
#define Z180_DSTAT_DWE0 		0x10	/* DMA write enable ch 1 (active low) */
#define Z180_DSTAT_DIE1 		0x08	/* DMA IRQ enable ch 1 */
#define Z180_DSTAT_DIE0 		0x04	/* DMA IRQ enable ch 0 */
#define Z180_DSTAT_DME			0x01	/* DMA enable (read only) */

#define Z180_DSTAT_RESET		0x30
#define Z180_DSTAT_RMASK		0xfd
#define Z180_DSTAT_WMASK		0xcc

/* 31 DMA mode register */
#define Z180_DMODE_DM			0x30
#define Z180_DMODE_SM			0x0c
#define Z180_DMODE_MMOD 		0x04

#define Z180_DMODE_RESET		0x00
#define Z180_DMODE_RMASK		0x3e
#define Z180_DMODE_WMASK		0x3e

/* 32 DMA/WAIT control register */
#define Z180_DCNTL_MWI1 		0x80
#define Z180_DCNTL_MWI0 		0x40
#define Z180_DCNTL_IWI1 		0x20
#define Z180_DCNTL_IWI0 		0x10
#define Z180_DCNTL_DMS1 		0x08
#define Z180_DCNTL_DMS0 		0x04
#define Z180_DCNTL_DIM1 		0x02
#define Z180_DCNTL_DIM0 		0x01

#define Z180_DCNTL_RESET		0x00
#define Z180_DCNTL_RMASK		0xff
#define Z180_DCNTL_WMASK		0xff

/* 33 INT vector low register */
#define Z180_IL_IL				0xe0

#define Z180_IL_RESET			0x00
#define Z180_IL_RMASK			0xe0
#define Z180_IL_WMASK			0xe0

/* 34 INT/TRAP control register */
#define Z180_ITC_TRAP			0x80
#define Z180_ITC_UFO			0x40
#define Z180_ITC_ITE2			0x04
#define Z180_ITC_ITE1			0x02
#define Z180_ITC_ITE0			0x01

#define Z180_ITC_RESET			0x01
#define Z180_ITC_RMASK			0xc7
#define Z180_ITC_WMASK			0x87

/* 35 reserved */
#define Z180_IO35_RESET 		0x00
#define Z180_IO35_RMASK 		0xff
#define Z180_IO35_WMASK 		0xff

/* 36 refresh control register */
#define Z180_RCR_REFE			0x80
#define Z180_RCR_REFW			0x80
#define Z180_RCR_CYC			0x03

#define Z180_RCR_RESET			0xc0
#define Z180_RCR_RMASK			0xc3
#define Z180_RCR_WMASK			0xc3

/* 37 reserved */
#define Z180_IO37_RESET 		0x00
#define Z180_IO37_RMASK 		0xff
#define Z180_IO37_WMASK 		0xff

/* 38 MMU common base register */
#define Z180_CBR_CB 			0xff

#define Z180_CBR_RESET			0x00
#define Z180_CBR_RMASK			0xff
#define Z180_CBR_WMASK			0xff

/* 39 MMU bank base register */
#define Z180_BBR_BB 			0xff

#define Z180_BBR_RESET			0x00
#define Z180_BBR_RMASK			0xff
#define Z180_BBR_WMASK			0xff

/* 3a MMU common/bank area register */
#define Z180_CBAR_CA			0xf0
#define Z180_CBAR_BA			0x0f

#define Z180_CBAR_RESET 		0xf0
#define Z180_CBAR_RMASK 		0xff
#define Z180_CBAR_WMASK 		0xff

/* 3b reserved */
#define Z180_IO3B_RESET 		0x00
#define Z180_IO3B_RMASK 		0xff
#define Z180_IO3B_WMASK 		0xff

/* 3c reserved */
#define Z180_IO3C_RESET 		0x00
#define Z180_IO3C_RMASK 		0xff
#define Z180_IO3C_WMASK 		0xff

/* 3d reserved */
#define Z180_IO3D_RESET 		0x00
#define Z180_IO3D_RMASK 		0xff
#define Z180_IO3D_WMASK 		0xff

/* 3e operation mode control register */
#define Z180_OMCR_RESET 		0x00
#define Z180_OMCR_RMASK 		0xff
#define Z180_OMCR_WMASK 		0xff

/* 3f I/O control register */
#define Z180_IOCR_RESET 		0x00
#define Z180_IOCR_RMASK 		0xff
#define Z180_IOCR_WMASK 		0xff

int z180_icount;
static Z180_Regs Z180;
static UINT32 EA;
static int after_EI = 0;

static UINT8 SZ[256];		/* zero and sign flags */
static UINT8 SZ_BIT[256];	/* zero, sign and parity/overflow (=zero) flags for BIT opcode */
static UINT8 SZP[256];		/* zero, sign and parity flags */
static UINT8 SZHV_inc[256]; /* zero, sign, half carry and overflow flags INC r8 */
static UINT8 SZHV_dec[256]; /* zero, sign, half carry and overflow flags DEC r8 */

#if BIG_FLAGS_ARRAY
static UINT8 *SZHVC_add = 0;
static UINT8 *SZHVC_sub = 0;
#endif

/****************************************************************************
 * Burn an odd amount of cycles, that is instructions taking something
 * different from 4 T-states per opcode (and R increment)
 ****************************************************************************/
static INLINE void BURNODD(int cycles, int opcodes, int cyclesum)
{
	if( cycles > 0 )
	{
		_R += (cycles / cyclesum) * opcodes;
		z180_icount -= (cycles / cyclesum) * cyclesum;
	}
}

static data8_t z180_readcontrol(offs_t port);
static void z180_writecontrol(offs_t port, data8_t data);
static void z180_dma0(void);
static void z180_dma1(void);

#include "z180daa.h"
#include "z180ops.h"
#include "z180tbl.h"

#include "z180cb.c"
#include "z180xy.c"
#include "z180dd.c"
#include "z180fd.c"
#include "z180ed.c"
#include "z180op.c"

static data8_t z180_readcontrol(offs_t port)
{
	/* normal external readport */
	data8_t data = cpu_readport16(port);

	/* but ignore the data and read the internal register */
	switch ((port & 0x3f) + Z180_CNTLA0)
	{
	case Z180_CNTLA0:
		data = IO_CNTLA0 & Z180_CNTLA0_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLA0 rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CNTLA1:
		data = IO_CNTLA1 & Z180_CNTLA1_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLA1 rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CNTLB0:
		data = IO_CNTLB0 & Z180_CNTLB0_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLB0 rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CNTLB1:
		data = IO_CNTLB1 & Z180_CNTLB1_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLB1 rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_STAT0:
		data = IO_STAT0 & Z180_STAT0_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d STAT0  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_STAT1:
		data = IO_STAT1 & Z180_STAT1_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d STAT1  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_TDR0:
		data = IO_TDR0 & Z180_TDR0_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TDR0   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_TDR1:
		data = IO_TDR1 & Z180_TDR1_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TDR1   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RDR0:
		data = IO_RDR0 & Z180_RDR0_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RDR0   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RDR1:
		data = IO_RDR1 & Z180_RDR1_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RDR1   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CNTR:
		data = IO_CNTR & Z180_CNTR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTR   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_TRDR:
		data = IO_TRDR & Z180_TRDR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TRDR   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_TMDR0L:
		data = IO_TMDR0L & Z180_TMDR0L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR0L rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		/* if timer is counting, latch the MSB and set the latch flag */
		if ((IO_TCR & Z180_TCR_TDE0) == 0)
		{
			Z180.tmdr_latch |= 1;
			Z180.tmdr[0] = IO_TMDR0H;
		}
		break;

	case Z180_TMDR0H:
		/* read latched value? */
		if (Z180.tmdr_latch & 1)
		{
			Z180.tmdr_latch &= ~1;
			data = Z180.tmdr[0] & Z180_TMDR1H_RMASK;
		}
		else
		{
			data = IO_TMDR0H & Z180_TMDR0H_RMASK;
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR0H rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RLDR0L:
		data = IO_RLDR0L & Z180_RLDR0L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR0L rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RLDR0H:
		data = IO_RLDR0H & Z180_RLDR0H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR0H rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_TCR:
		data = IO_TCR & Z180_TCR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TCR    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO11:
		data = IO_IO11 & Z180_IO11_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO11   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ASEXT0:
		data = IO_ASEXT0 & Z180_ASEXT0_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASEXT0 rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ASEXT1:
		data = IO_ASEXT1 & Z180_ASEXT1_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASEXT1 rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_TMDR1L:
		data = IO_TMDR1L & Z180_TMDR1L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR1L rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		/* if timer is counting, latch the MSB and set the latch flag */
		if ((IO_TCR & Z180_TCR_TDE1) == 0)
		{
			Z180.tmdr_latch |= 2;
			Z180.tmdr[1] = IO_TMDR1H;
		}
		break;

	case Z180_TMDR1H:
		/* read latched value? */
		if (Z180.tmdr_latch & 2)
		{
			Z180.tmdr_latch &= ~2;
			data = Z180.tmdr[0] & Z180_TMDR1H_RMASK;
		}
		else
		{
			data = IO_TMDR1H & Z180_TMDR1H_RMASK;
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR1H rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RLDR1L:
		data = IO_RLDR1L & Z180_RLDR1L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR1L rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RLDR1H:
		data = IO_RLDR1H & Z180_RLDR1H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR1H rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_FRC:
		data = IO_FRC & Z180_FRC_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d FRC    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO19:
		data = IO_IO19 & Z180_IO19_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO19   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ASTC0L:
		data = IO_ASTC0L & Z180_ASTC0L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC0L rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ASTC0H:
		data = IO_ASTC0H & Z180_ASTC0H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC0H rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ASTC1L:
		data = IO_ASTC1L & Z180_ASTC1L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC1L rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ASTC1H:
		data = IO_ASTC1H & Z180_ASTC1H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC1H rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CMR:
		data = IO_CMR & Z180_CMR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CMR    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CCR:
		data = IO_CCR & Z180_CCR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CCR    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_SAR0L:
		data = IO_SAR0L & Z180_SAR0L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d SAR0L  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_SAR0H:
		data = IO_SAR0H & Z180_SAR0H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d SAR0H  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_SAR0B:
		data = IO_SAR0B & Z180_SAR0B_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d SAR0B  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_DAR0L:
		data = IO_DAR0L & Z180_DAR0L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DAR0L  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_DAR0H:
		data = IO_DAR0H & Z180_DAR0H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DAR0H  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_DAR0B:
		data = IO_DAR0B & Z180_DAR0B_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DAR0B  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_BCR0L:
		data = IO_BCR0L & Z180_BCR0L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR0L  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_BCR0H:
		data = IO_BCR0H & Z180_BCR0H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR0H  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_MAR1L:
		data = IO_MAR1L & Z180_MAR1L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d MAR1L  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_MAR1H:
		data = IO_MAR1H & Z180_MAR1H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d MAR1H  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_MAR1B:
		data = IO_MAR1B & Z180_MAR1B_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d MAR1B  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IAR1L:
		data = IO_IAR1L & Z180_IAR1L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IAR1L  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IAR1H:
		data = IO_IAR1H & Z180_IAR1H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IAR1H  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IAR1B:
		data = IO_IAR1B & Z180_IAR1B_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IAR1B  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_BCR1L:
		data = IO_BCR1L & Z180_BCR1L_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR1L  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_BCR1H:
		data = IO_BCR1H & Z180_BCR1H_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR1H  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_DSTAT:
		data = IO_DSTAT & Z180_DSTAT_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DSTAT  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_DMODE:
		data = IO_DMODE & Z180_DMODE_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DMODE  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_DCNTL:
		data = IO_DCNTL & Z180_DCNTL_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DCNTL  rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IL:
		data = IO_IL & Z180_IL_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IL     rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_ITC:
		data = IO_ITC & Z180_ITC_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ITC    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO35:
		data = IO_IO35 & Z180_IO35_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO35   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_RCR:
		data = IO_RCR & Z180_RCR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RCR    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO37:
		data = IO_IO37 & Z180_IO37_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO37   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CBR:
		data = IO_CBR & Z180_CBR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CBR    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_BBR:
		data = IO_BBR & Z180_BBR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BBR    rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_CBAR:
		data = IO_CBAR & Z180_CBAR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CBAR   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO3B:
		data = IO_IO3B & Z180_IO3B_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO3B   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO3C:
		data = IO_IO3C & Z180_IO3C_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO3C   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IO3D:
		data = IO_IO3D & Z180_IO3D_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO3D   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_OMCR:
		data = IO_OMCR & Z180_OMCR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d OMCR   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;

	case Z180_IOCR:
		data = IO_IOCR & Z180_IOCR_RMASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IOCR   rd $%02x ($%02x)\n", cpu_getactivecpu(), data, Z180.io[port & 0x3f]);
		break;
	}
	return data;
}

static void z180_writecontrol(offs_t port, data8_t data)
{
	/* normal external write port */
	cpu_writeport16(port, data);
	/* store the data in the internal register */
	switch ((port & 0x3f) + Z180_CNTLA0)
	{
	case Z180_CNTLA0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLA0 wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CNTLA0_WMASK);
		IO_CNTLA0 = (IO_CNTLA0 & ~Z180_CNTLA0_WMASK) | (data & Z180_CNTLA0_WMASK);
		break;

	case Z180_CNTLA1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLA1 wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CNTLA1_WMASK);
		IO_CNTLA1 = (IO_CNTLA1 & ~Z180_CNTLA1_WMASK) | (data & Z180_CNTLA1_WMASK);
		break;

	case Z180_CNTLB0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLB0 wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CNTLB0_WMASK);
		IO_CNTLB0 = (IO_CNTLB0 & ~Z180_CNTLB0_WMASK) | (data & Z180_CNTLB0_WMASK);
		break;

	case Z180_CNTLB1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTLB1 wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CNTLB1_WMASK);
		IO_CNTLB1 = (IO_CNTLB1 & ~Z180_CNTLB1_WMASK) | (data & Z180_CNTLB1_WMASK);
		break;

	case Z180_STAT0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d STAT0  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_STAT0_WMASK);
		IO_STAT0 = (IO_STAT0 & ~Z180_STAT0_WMASK) | (data & Z180_STAT0_WMASK);
		break;

	case Z180_STAT1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d STAT1  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_STAT1_WMASK);
		IO_STAT1 = (IO_STAT1 & ~Z180_STAT1_WMASK) | (data & Z180_STAT1_WMASK);
		break;

	case Z180_TDR0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TDR0   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TDR0_WMASK);
		IO_TDR0 = (IO_TDR0 & ~Z180_TDR0_WMASK) | (data & Z180_TDR0_WMASK);
		break;

	case Z180_TDR1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TDR1   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TDR1_WMASK);
		IO_TDR1 = (IO_TDR1 & ~Z180_TDR1_WMASK) | (data & Z180_TDR1_WMASK);
		break;

	case Z180_RDR0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RDR0   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RDR0_WMASK);
		IO_RDR0 = (IO_RDR0 & ~Z180_RDR0_WMASK) | (data & Z180_RDR0_WMASK);
		break;

	case Z180_RDR1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RDR1   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RDR1_WMASK);
		IO_RDR1 = (IO_RDR1 & ~Z180_RDR1_WMASK) | (data & Z180_RDR1_WMASK);
		break;

	case Z180_CNTR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CNTR   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CNTR_WMASK);
		IO_CNTR = (IO_CNTR & ~Z180_CNTR_WMASK) | (data & Z180_CNTR_WMASK);
		break;

	case Z180_TRDR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TRDR   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TRDR_WMASK);
		IO_TRDR = (IO_TRDR & ~Z180_TRDR_WMASK) | (data & Z180_TRDR_WMASK);
		break;

	case Z180_TMDR0L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR0L wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TMDR0L_WMASK);
		IO_TMDR0L = (IO_TMDR0L & ~Z180_TMDR0L_WMASK) | (data & Z180_TMDR0L_WMASK);
		break;

	case Z180_TMDR0H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR0H wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TMDR0H_WMASK);
		IO_TMDR0H = (IO_TMDR0H & ~Z180_TMDR0H_WMASK) | (data & Z180_TMDR0H_WMASK);
		break;

	case Z180_RLDR0L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR0L wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RLDR0L_WMASK);
		IO_RLDR0L = (IO_RLDR0L & ~Z180_RLDR0L_WMASK) | (data & Z180_RLDR0L_WMASK);
		break;

	case Z180_RLDR0H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR0H wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RLDR0H_WMASK);
		IO_RLDR0H = (IO_RLDR0H & ~Z180_RLDR0H_WMASK) | (data & Z180_RLDR0H_WMASK);
		break;

	case Z180_TCR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TCR    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TCR_WMASK);
		IO_TCR = (IO_TCR & ~Z180_TCR_WMASK) | (data & Z180_TCR_WMASK);
		break;

	case Z180_IO11:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO11   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO11_WMASK);
		IO_IO11 = (IO_IO11 & ~Z180_IO11_WMASK) | (data & Z180_IO11_WMASK);
		break;

	case Z180_ASEXT0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASEXT0 wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ASEXT0_WMASK);
		IO_ASEXT0 = (IO_ASEXT0 & ~Z180_ASEXT0_WMASK) | (data & Z180_ASEXT0_WMASK);
		break;

	case Z180_ASEXT1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASEXT1 wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ASEXT1_WMASK);
		IO_ASEXT1 = (IO_ASEXT1 & ~Z180_ASEXT1_WMASK) | (data & Z180_ASEXT1_WMASK);
		break;

	case Z180_TMDR1L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR1L wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TMDR1L_WMASK);
		IO_TMDR1L = (IO_TMDR1L & ~Z180_TMDR1L_WMASK) | (data & Z180_TMDR1L_WMASK);
		break;

	case Z180_TMDR1H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TMDR1H wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_TMDR1H_WMASK);
		IO_TMDR1H = (IO_TMDR1H & ~Z180_TMDR1H_WMASK) | (data & Z180_TMDR1H_WMASK);
		break;

	case Z180_RLDR1L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR1L wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RLDR1L_WMASK);
		IO_RLDR1L = (IO_RLDR1L & ~Z180_RLDR1L_WMASK) | (data & Z180_RLDR1L_WMASK);
		break;

	case Z180_RLDR1H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RLDR1H wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RLDR1H_WMASK);
		IO_RLDR1H = (IO_RLDR1H & ~Z180_RLDR1H_WMASK) | (data & Z180_RLDR1H_WMASK);
		break;

	case Z180_FRC:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d FRC    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_FRC_WMASK);
		IO_FRC = (IO_FRC & ~Z180_FRC_WMASK) | (data & Z180_FRC_WMASK);
		break;

	case Z180_IO19:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO19   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO19_WMASK);
		IO_IO19 = (IO_IO19 & ~Z180_IO19_WMASK) | (data & Z180_IO19_WMASK);
		break;

	case Z180_ASTC0L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC0L wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ASTC0L_WMASK);
		IO_ASTC0L = (IO_ASTC0L & ~Z180_ASTC0L_WMASK) | (data & Z180_ASTC0L_WMASK);
		break;

	case Z180_ASTC0H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC0H wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ASTC0H_WMASK);
		IO_ASTC0H = (IO_ASTC0H & ~Z180_ASTC0H_WMASK) | (data & Z180_ASTC0H_WMASK);
		break;

	case Z180_ASTC1L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC1L wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ASTC1L_WMASK);
		IO_ASTC1L = (IO_ASTC1L & ~Z180_ASTC1L_WMASK) | (data & Z180_ASTC1L_WMASK);
		break;

	case Z180_ASTC1H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ASTC1H wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ASTC1H_WMASK);
		IO_ASTC1H = (IO_ASTC1H & ~Z180_ASTC1H_WMASK) | (data & Z180_ASTC1H_WMASK);
		break;

	case Z180_CMR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CMR    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CMR_WMASK);
		IO_CMR = (IO_CMR & ~Z180_CMR_WMASK) | (data & Z180_CMR_WMASK);
		break;

	case Z180_CCR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CCR    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CCR_WMASK);
		IO_CCR = (IO_CCR & ~Z180_CCR_WMASK) | (data & Z180_CCR_WMASK);
		break;

	case Z180_SAR0L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d SAR0L  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_SAR0L_WMASK);
		IO_SAR0L = (IO_SAR0L & ~Z180_SAR0L_WMASK) | (data & Z180_SAR0L_WMASK);
		break;

	case Z180_SAR0H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d SAR0H  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_SAR0H_WMASK);
		IO_SAR0H = (IO_SAR0H & ~Z180_SAR0H_WMASK) | (data & Z180_SAR0H_WMASK);
		break;

	case Z180_SAR0B:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d SAR0B  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_SAR0B_WMASK);
		IO_SAR0B = (IO_SAR0B & ~Z180_SAR0B_WMASK) | (data & Z180_SAR0B_WMASK);
		break;

	case Z180_DAR0L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DAR0L  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_DAR0L_WMASK);
		IO_DAR0L = (IO_DAR0L & ~Z180_DAR0L_WMASK) | (data & Z180_DAR0L_WMASK);
		break;

	case Z180_DAR0H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DAR0H  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_DAR0H_WMASK);
		IO_DAR0H = (IO_DAR0H & ~Z180_DAR0H_WMASK) | (data & Z180_DAR0H_WMASK);
		break;

	case Z180_DAR0B:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DAR0B  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_DAR0B_WMASK);
		IO_DAR0B = (IO_DAR0B & ~Z180_DAR0B_WMASK) | (data & Z180_DAR0B_WMASK);
		break;

	case Z180_BCR0L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR0L  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_BCR0L_WMASK);
		IO_BCR0L = (IO_BCR0L & ~Z180_BCR0L_WMASK) | (data & Z180_BCR0L_WMASK);
		break;

	case Z180_BCR0H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR0H  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_BCR0H_WMASK);
		IO_BCR0H = (IO_BCR0H & ~Z180_BCR0H_WMASK) | (data & Z180_BCR0H_WMASK);
		break;

	case Z180_MAR1L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d MAR1L  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_MAR1L_WMASK);
		IO_MAR1L = (IO_MAR1L & ~Z180_MAR1L_WMASK) | (data & Z180_MAR1L_WMASK);
		break;

	case Z180_MAR1H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d MAR1H  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_MAR1H_WMASK);
		IO_MAR1H = (IO_MAR1H & ~Z180_MAR1H_WMASK) | (data & Z180_MAR1H_WMASK);
		break;

	case Z180_MAR1B:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d MAR1B  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_MAR1B_WMASK);
		IO_MAR1B = (IO_MAR1B & ~Z180_MAR1B_WMASK) | (data & Z180_MAR1B_WMASK);
		break;

	case Z180_IAR1L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IAR1L  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IAR1L_WMASK);
		IO_IAR1L = (IO_IAR1L & ~Z180_IAR1L_WMASK) | (data & Z180_IAR1L_WMASK);
		break;

	case Z180_IAR1H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IAR1H  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IAR1H_WMASK);
		IO_IAR1H = (IO_IAR1H & ~Z180_IAR1H_WMASK) | (data & Z180_IAR1H_WMASK);
		break;

	case Z180_IAR1B:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IAR1B  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IAR1B_WMASK);
		IO_IAR1B = (IO_IAR1B & ~Z180_IAR1B_WMASK) | (data & Z180_IAR1B_WMASK);
		break;

	case Z180_BCR1L:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR1L  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_BCR1L_WMASK);
		IO_BCR1L = (IO_BCR1L & ~Z180_BCR1L_WMASK) | (data & Z180_BCR1L_WMASK);
		break;

	case Z180_BCR1H:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BCR1H  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_BCR1H_WMASK);
		IO_BCR1H = (IO_BCR1H & ~Z180_BCR1H_WMASK) | (data & Z180_BCR1H_WMASK);
		break;

	case Z180_DSTAT:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DSTAT  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_DSTAT_WMASK);
		IO_DSTAT = (IO_DSTAT & ~Z180_DSTAT_WMASK) | (data & Z180_DSTAT_WMASK);
		if ((data & (Z180_DSTAT_DE1 | Z180_DSTAT_DWE1)) == Z180_DSTAT_DE1)
			IO_DSTAT |= Z180_DSTAT_DME;  /* DMA enable */
		if ((data & (Z180_DSTAT_DE0 | Z180_DSTAT_DWE0)) == Z180_DSTAT_DE0)
			IO_DSTAT |= Z180_DSTAT_DME;  /* DMA enable */
		break;

	case Z180_DMODE:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DMODE  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_DMODE_WMASK);
		IO_DMODE = (IO_DMODE & ~Z180_DMODE_WMASK) | (data & Z180_DMODE_WMASK);
		break;

	case Z180_DCNTL:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DCNTL  wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_DCNTL_WMASK);
		IO_DCNTL = (IO_DCNTL & ~Z180_DCNTL_WMASK) | (data & Z180_DCNTL_WMASK);
		break;

	case Z180_IL:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IL     wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IL_WMASK);
		IO_IL = (IO_IL & ~Z180_IL_WMASK) | (data & Z180_IL_WMASK);
		break;

	case Z180_ITC:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d ITC    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_ITC_WMASK);
		IO_ITC = (IO_ITC & ~Z180_ITC_WMASK) | (data & Z180_ITC_WMASK);
		break;

	case Z180_IO35:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO35   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO35_WMASK);
		IO_IO35 = (IO_IO35 & ~Z180_IO35_WMASK) | (data & Z180_IO35_WMASK);
		break;

	case Z180_RCR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RCR    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_RCR_WMASK);
		IO_RCR = (IO_RCR & ~Z180_RCR_WMASK) | (data & Z180_RCR_WMASK);
		break;

	case Z180_IO37:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO37   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO37_WMASK);
		IO_IO37 = (IO_IO37 & ~Z180_IO37_WMASK) | (data & Z180_IO37_WMASK);
		break;

	case Z180_CBR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CBR    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CBR_WMASK);
		IO_CBR = (IO_CBR & ~Z180_CBR_WMASK) | (data & Z180_CBR_WMASK);
		z180_mmu();
		break;

	case Z180_BBR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d BBR    wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_BBR_WMASK);
		IO_BBR = (IO_BBR & ~Z180_BBR_WMASK) | (data & Z180_BBR_WMASK);
		z180_mmu();
		break;

	case Z180_CBAR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CBAR   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_CBAR_WMASK);
		IO_CBAR = (IO_CBAR & ~Z180_CBAR_WMASK) | (data & Z180_CBAR_WMASK);
		z180_mmu();
		break;

	case Z180_IO3B:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO3B   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO3B_WMASK);
		IO_IO3B = (IO_IO3B & ~Z180_IO3B_WMASK) | (data & Z180_IO3B_WMASK);
		break;

	case Z180_IO3C:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO3C   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO3C_WMASK);
		IO_IO3C = (IO_IO3C & ~Z180_IO3C_WMASK) | (data & Z180_IO3C_WMASK);
		break;

	case Z180_IO3D:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IO3D   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IO3D_WMASK);
		IO_IO3D = (IO_IO3D & ~Z180_IO3D_WMASK) | (data & Z180_IO3D_WMASK);
		break;

	case Z180_OMCR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d OMCR   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_OMCR_WMASK);
		IO_OMCR = (IO_OMCR & ~Z180_OMCR_WMASK) | (data & Z180_OMCR_WMASK);
		break;

	case Z180_IOCR:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d IOCR   wr $%02x ($%02x)\n", cpu_getactivecpu(), data,  data & Z180_IOCR_WMASK);
		IO_IOCR = (IO_IOCR & ~Z180_IOCR_WMASK) | (data & Z180_IOCR_WMASK);
		break;
	}
}

static void z180_dma0(void)
{
	offs_t sar0 = 65536 * IO_SAR0B + 256 * IO_SAR0H + IO_SAR0L;
	offs_t dar0 = 65536 * IO_DAR0B + 256 * IO_DAR0H + IO_DAR0L;
	int bcr0 = 256 * IO_BCR0H + IO_BCR0L;
	int count = (IO_DMODE & Z180_DMODE_MMOD) ? bcr0 : 1;

	if (bcr0 == 0)
	{
		IO_DSTAT &= ~Z180_DSTAT_DE0;
		return;
	}

	while (count-- > 0)
	{
		/* last transfer happening now? */
		if (bcr0 == 1)
		{
			Z180.iol |= Z180_TEND0;
		}
		switch( IO_DMODE & (Z180_DMODE_SM | Z180_DMODE_DM) )
		{
		case 0x00:	/* memory SAR0+1 to memory DAR0+1 */
			cpu_writemem20(dar0++, cpu_readmem20(sar0++));
			break;
		case 0x04:	/* memory SAR0-1 to memory DAR0+1 */
			cpu_writemem20(dar0++, cpu_readmem20(sar0--));
			break;
		case 0x08:	/* memory SAR0 fixed to memory DAR0+1 */
			cpu_writemem20(dar0++, cpu_readmem20(sar0));
			break;
		case 0x0c:	/* I/O SAR0 fixed to memory DAR0+1 */
			if (Z180.iol & Z180_DREQ0)
			{
				cpu_writemem20(dar0++, IN(sar0));
				/* edge sensitive DREQ0 ? */
				if (IO_DCNTL & Z180_DCNTL_DIM0)
				{
					Z180.iol &= ~Z180_DREQ0;
					count = 0;
				}
			}
			break;
		case 0x10:	/* memory SAR0+1 to memory DAR0-1 */
			cpu_writemem20(dar0--, cpu_readmem20(sar0++));
			break;
		case 0x14:	/* memory SAR0-1 to memory DAR0-1 */
			cpu_writemem20(dar0--, cpu_readmem20(sar0--));
			break;
		case 0x18:	/* memory SAR0 fixed to memory DAR0-1 */
			cpu_writemem20(dar0--, cpu_readmem20(sar0));
			break;
		case 0x1c:	/* I/O SAR0 fixed to memory DAR0-1 */
			if (Z180.iol & Z180_DREQ0)
            {
				cpu_writemem20(dar0--, IN(sar0));
				/* edge sensitive DREQ0 ? */
				if (IO_DCNTL & Z180_DCNTL_DIM0)
				{
					Z180.iol &= ~Z180_DREQ0;
					count = 0;
				}
			}
			break;
		case 0x20:	/* memory SAR0+1 to memory DAR0 fixed */
			cpu_writemem20(dar0, cpu_readmem20(sar0++));
			break;
		case 0x24:	/* memory SAR0-1 to memory DAR0 fixed */
			cpu_writemem20(dar0, cpu_readmem20(sar0--));
			break;
		case 0x28:	/* reserved */
			break;
		case 0x2c:	/* reserved */
			break;
		case 0x30:	/* memory SAR0+1 to I/O DAR0 fixed */
			if (Z180.iol & Z180_DREQ0)
            {
				OUT(dar0, cpu_readmem20(sar0++));
				/* edge sensitive DREQ0 ? */
				if (IO_DCNTL & Z180_DCNTL_DIM0)
				{
					Z180.iol &= ~Z180_DREQ0;
					count = 0;
				}
			}
			break;
		case 0x34:	/* memory SAR0-1 to I/O DAR0 fixed */
			if (Z180.iol & Z180_DREQ0)
            {
				OUT(dar0, cpu_readmem20(sar0--));
				/* edge sensitive DREQ0 ? */
				if (IO_DCNTL & Z180_DCNTL_DIM0)
				{
					Z180.iol &= ~Z180_DREQ0;
					count = 0;
				}
			}
			break;
		case 0x38:	/* reserved */
			break;
		case 0x3c:	/* reserved */
			break;
		}
		bcr0--;
		count--;
		if ((z180_icount -= 6) < 0)
			break;
	}

	IO_SAR0L = sar0;
	IO_SAR0H = sar0 >> 8;
	IO_SAR0B = sar0 >> 16;
	IO_DAR0L = dar0;
	IO_DAR0H = dar0 >> 8;
	IO_DAR0B = dar0 >> 16;
	IO_BCR0L = bcr0;
	IO_BCR0H = bcr0 >> 8;

	/* DMA terminal count? */
	if (bcr0 == 0)
	{
		Z180.iol &= ~Z180_TEND0;
		IO_DSTAT &= ~Z180_DSTAT_DE0;
		/* terminal count interrupt enabled? */
		if (IO_DSTAT & Z180_DSTAT_DIE0 && _IFF1)
			take_interrupt(Z180_INT_DMA0);
	}
}

static void z180_dma1(void)
{
	offs_t mar1 = 65536 * IO_MAR1B + 256 * IO_MAR1H + IO_MAR1L;
	offs_t iar1 = 256 * IO_IAR1H + IO_IAR1L;
	int bcr1 = 256 * IO_BCR1H + IO_BCR1L;

	if ((Z180.iol & Z180_DREQ1) == 0)
		return;

	/* counter is zero? */
	if (bcr1 == 0)
	{
		IO_DSTAT &= ~Z180_DSTAT_DE1;
		return;
	}

	/* last transfer happening now? */
	if (bcr1 == 1)
	{
		Z180.iol |= Z180_TEND1;
	}

	switch (IO_DCNTL & (Z180_DCNTL_DIM1 | Z180_DCNTL_DIM0))
	{
	case 0x00:	/* memory MAR1+1 to I/O IAR1 fixed */
		cpu_writeport16(iar1, cpu_readmem20(mar1++));
		break;
	case 0x01:	/* memory MAR1-1 to I/O IAR1 fixed */
		cpu_writeport16(iar1, cpu_readmem20(mar1--));
		break;
	case 0x02:	/* I/O IAR1 fixed to memory MAR1+1 */
		cpu_writemem20(mar1++, cpu_readport16(iar1));
		break;
	case 0x03:	/* I/O IAR1 fixed to memory MAR1-1 */
		cpu_writemem20(mar1--, cpu_readport16(iar1));
		break;
	}

	/* edge sensitive DREQ1 ? */
	if (IO_DCNTL & Z180_DCNTL_DIM1)
		Z180.iol &= ~Z180_DREQ1;

	IO_MAR1L = mar1;
	IO_MAR1H = mar1 >> 8;
	IO_MAR1B = mar1 >> 16;
	IO_BCR1L = bcr1;
	IO_BCR1H = bcr1 >> 8;

	/* DMA terminal count? */
	if (bcr1 == 0)
	{
		Z180.iol &= ~Z180_TEND1;
		IO_DSTAT &= ~Z180_DSTAT_DE1;
		if (IO_DSTAT & Z180_DSTAT_DIE1 && _IFF1)
			take_interrupt(Z180_INT_DMA1);
	}

	/* six cycles per transfer (minimum) */
	z180_icount -= 6;
}

static void z180_write_iolines(UINT32 data)
{
	UINT32 changes = Z180.iol ^ data;

    /* I/O asynchronous clock 0 (active high) or DREQ0 (mux) */
	if (changes & Z180_CKA0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CKA0   %d\n", cpu_getactivecpu(), data & Z180_CKA0 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_CKA0) | (data & Z180_CKA0);
    }

    /* I/O asynchronous clock 1 (active high) or TEND1 (mux) */
	if (changes & Z180_CKA1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CKA1   %d\n", cpu_getactivecpu(), data & Z180_CKA1 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_CKA1) | (data & Z180_CKA1);
    }

    /* I/O serial clock (active high) */
	if (changes & Z180_CKS)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CKS    %d\n", cpu_getactivecpu(), data & Z180_CKS ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_CKS) | (data & Z180_CKS);
    }

    /* I   clear to send 0 (active low) */
	if (changes & Z180_CTS0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CTS0   %d\n", cpu_getactivecpu(), data & Z180_CTS0 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_CTS0) | (data & Z180_CTS0);
    }

    /* I   clear to send 1 (active low) or RXS (mux) */
	if (changes & Z180_CTS1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d CTS1   %d\n", cpu_getactivecpu(), data & Z180_CTS1 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_CTS1) | (data & Z180_CTS1);
    }

    /* I   data carrier detect (active low) */
	if (changes & Z180_DCD0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DCD0   %d\n", cpu_getactivecpu(), data & Z180_DCD0 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_DCD0) | (data & Z180_DCD0);
    }

    /* I   data request DMA ch 0 (active low) or CKA0 (mux) */
	if (changes & Z180_DREQ0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DREQ0  %d\n", cpu_getactivecpu(), data & Z180_DREQ0 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_DREQ0) | (data & Z180_DREQ0);
    }

    /* I   data request DMA ch 1 (active low) */
	if (changes & Z180_DREQ1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d DREQ1  %d\n", cpu_getactivecpu(), data & Z180_DREQ1 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_DREQ1) | (data & Z180_DREQ1);
    }

    /* I   asynchronous receive data 0 (active high) */
	if (changes & Z180_RXA0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RXA0   %d\n", cpu_getactivecpu(), data & Z180_RXA0 ? 1 : 0);
        Z180.iol = (Z180.iol & ~Z180_RXA0) | (data & Z180_RXA0);
    }

    /* I   asynchronous receive data 1 (active high) */
	if (changes & Z180_RXA1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RXA1   %d\n", cpu_getactivecpu(), data & Z180_RXA1 ? 1 : 0);
		Z180.iol = (Z180.iol & ~Z180_RXA1) | (data & Z180_RXA1);
    }

    /* I   clocked serial receive data (active high) or CTS1 (mux) */
	if (changes & Z180_RXS)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RXS    %d\n", cpu_getactivecpu(), data & Z180_RXS ? 1 : 0);
        Z180.iol = (Z180.iol & ~Z180_RXS) | (data & Z180_RXS);
    }

    /*   O request to send (active low) */
	if (changes & Z180_RTS0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d RTS0   won't change output\n", cpu_getactivecpu());
    }

    /*   O transfer end 0 (active low) or CKA1 (mux) */
	if (changes & Z180_TEND0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TEND0  won't change output\n", cpu_getactivecpu());
    }

    /*   O transfer end 1 (active low) */
	if (changes & Z180_TEND1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TEND1  won't change output\n", cpu_getactivecpu());
    }

    /*   O transfer out (PRT channel, active low) or A18 (mux) */
	if (changes & Z180_A18_TOUT)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TOUT   won't change output\n", cpu_getactivecpu());
    }

    /*   O asynchronous transmit data 0 (active high) */
	if (changes & Z180_TXA0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TXA0   won't change output\n", cpu_getactivecpu());
    }

    /*   O asynchronous transmit data 1 (active high) */
	if (changes & Z180_TXA1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TXA1   won't change output\n", cpu_getactivecpu());
    }

    /*   O clocked serial transmit data (active high) */
	if (changes & Z180_TXS)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d TXS    won't change output\n", cpu_getactivecpu());
    }
}


void z180_init(void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("z180", cpu, "AF", &Z180.AF.w.l, 1);
	state_save_register_UINT16("z180", cpu, "BC", &Z180.BC.w.l, 1);
	state_save_register_UINT16("z180", cpu, "DE", &Z180.DE.w.l, 1);
	state_save_register_UINT16("z180", cpu, "HL", &Z180.HL.w.l, 1);
	state_save_register_UINT16("z180", cpu, "IX", &Z180.IX.w.l, 1);
	state_save_register_UINT16("z180", cpu, "IY", &Z180.IY.w.l, 1);
	state_save_register_UINT16("z180", cpu, "PC", &Z180.PC.w.l, 1);
	state_save_register_UINT16("z180", cpu, "SP", &Z180.SP.w.l, 1);
	state_save_register_UINT16("z180", cpu, "AF2", &Z180.AF2.w.l, 1);
	state_save_register_UINT16("z180", cpu, "BC2", &Z180.BC2.w.l, 1);
	state_save_register_UINT16("z180", cpu, "DE2", &Z180.DE2.w.l, 1);
	state_save_register_UINT16("z180", cpu, "HL2", &Z180.HL2.w.l, 1);
	state_save_register_UINT8("z180", cpu, "R", &Z180.R, 1);
	state_save_register_UINT8("z180", cpu, "R2", &Z180.R2, 1);
	state_save_register_UINT8("z180", cpu, "IFF1", &Z180.IFF1, 1);
	state_save_register_UINT8("z180", cpu, "IFF2", &Z180.IFF2, 1);
	state_save_register_UINT8("z180", cpu, "HALT", &Z180.HALT, 1);
	state_save_register_UINT8("z180", cpu, "IM", &Z180.IM, 1);
	state_save_register_UINT8("z180", cpu, "I", &Z180.I, 1);
	state_save_register_UINT8("z180", cpu, "irq_max", &Z180.irq_max, 1);
	state_save_register_INT8("z180", cpu, "request_irq", &Z180.request_irq, 1);
	state_save_register_INT8("z180", cpu, "service_irq", &Z180.service_irq, 1);
	state_save_register_UINT8("z180", cpu, "int_state", Z180.int_state, 4);
	state_save_register_UINT8("z180", cpu, "nmi_state", &Z180.nmi_state, 1);
	state_save_register_UINT8("z180", cpu, "int0_state", &Z180.irq_state[0], 1);
	state_save_register_UINT8("z180", cpu, "int1_state", &Z180.irq_state[1], 1);
	state_save_register_UINT8("z180", cpu, "int2_state", &Z180.irq_state[2], 1);
	/* daisy chain needs to be saved by z80ctc.c somehow */
}

/****************************************************************************
 * Reset registers to their initial values
 ****************************************************************************/
void z180_reset(void *param)
{
	Z80_DaisyChain *daisy_chain = (Z80_DaisyChain *)param;
	int i, p;
#if BIG_FLAGS_ARRAY
	if( !SZHVC_add || !SZHVC_sub )
	{
		int oldval, newval, val;
		UINT8 *padd, *padc, *psub, *psbc;
		/* allocate big flag arrays once */
		SZHVC_add = (UINT8 *)malloc(2*256*256);
		SZHVC_sub = (UINT8 *)malloc(2*256*256);
		if( !SZHVC_add || !SZHVC_sub )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180: failed to allocate 2 * 128K flags arrays!!!\n");
			/*raise(SIGABRT);*/
		}
		padd = &SZHVC_add[	0*256];
		padc = &SZHVC_add[256*256];
		psub = &SZHVC_sub[	0*256];
		psbc = &SZHVC_sub[256*256];
		for (oldval = 0; oldval < 256; oldval++)
		{
			for (newval = 0; newval < 256; newval++)
			{
				/* add or adc w/o carry set */
				val = newval - oldval;
				*padd = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
#if Z180_EXACT
				*padd |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) < (oldval & 0x0f) ) *padd |= HF;
				if( newval < oldval ) *padd |= CF;
				if( (val^oldval^0x80) & (val^newval) & 0x80 ) *padd |= VF;
				padd++;

				/* adc with carry set */
				val = newval - oldval - 1;
				*padc = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
#if Z180_EXACT
				*padc |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) <= (oldval & 0x0f) ) *padc |= HF;
				if( newval <= oldval ) *padc |= CF;
				if( (val^oldval^0x80) & (val^newval) & 0x80 ) *padc |= VF;
				padc++;

				/* cp, sub or sbc w/o carry set */
				val = oldval - newval;
				*psub = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
#if Z180_EXACT
				*psub |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) > (oldval & 0x0f) ) *psub |= HF;
				if( newval > oldval ) *psub |= CF;
				if( (val^oldval) & (oldval^newval) & 0x80 ) *psub |= VF;
				psub++;

				/* sbc with carry set */
				val = oldval - newval - 1;
				*psbc = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
#if Z180_EXACT
				*psbc |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
				if( (newval & 0x0f) >= (oldval & 0x0f) ) *psbc |= HF;
				if( newval >= oldval ) *psbc |= CF;
				if( (val^oldval) & (oldval^newval) & 0x80 ) *psbc |= VF;
				psbc++;
			}
		}
	}
#endif
	for (i = 0; i < 256; i++)
	{
		p = 0;
		if( i&0x01 ) ++p;
		if( i&0x02 ) ++p;
		if( i&0x04 ) ++p;
		if( i&0x08 ) ++p;
		if( i&0x10 ) ++p;
		if( i&0x20 ) ++p;
		if( i&0x40 ) ++p;
		if( i&0x80 ) ++p;
		SZ[i] = i ? i & SF : ZF;
#if Z180_EXACT
		SZ[i] |= (i & (YF | XF));		/* undocumented flag bits 5+3 */
#endif
		SZ_BIT[i] = i ? i & SF : ZF | PF;
#if Z180_EXACT
		SZ_BIT[i] |= (i & (YF | XF));	/* undocumented flag bits 5+3 */
#endif
		SZP[i] = SZ[i] | ((p & 1) ? 0 : PF);
		SZHV_inc[i] = SZ[i];
		if( i == 0x80 ) SZHV_inc[i] |= VF;
		if( (i & 0x0f) == 0x00 ) SZHV_inc[i] |= HF;
		SZHV_dec[i] = SZ[i] | NF;
		if( i == 0x7f ) SZHV_dec[i] |= VF;
		if( (i & 0x0f) == 0x0f ) SZHV_dec[i] |= HF;
	}

	memset(&Z180, 0, sizeof(Z180));
	_IX = _IY = 0xffff; /* IX and IY are FFFF after a reset! */
	_F = ZF;			/* Zero flag is set */
	Z180.request_irq = -1;
	Z180.service_irq = -1;
	Z180.nmi_state = CLEAR_LINE;
	Z180.irq_state[0] = CLEAR_LINE;
	Z180.irq_state[1] = CLEAR_LINE;
	Z180.irq_state[2] = CLEAR_LINE;

	/* reset io registers */
	IO_CNTLA0  = Z180_CNTLA0_RESET;
	IO_CNTLA1  = Z180_CNTLA1_RESET;
	IO_CNTLB0  = Z180_CNTLB0_RESET;
	IO_CNTLB1  = Z180_CNTLB1_RESET;
	IO_STAT0   = Z180_STAT0_RESET;
	IO_STAT1   = Z180_STAT1_RESET;
	IO_TDR0    = Z180_TDR0_RESET;
	IO_TDR1    = Z180_TDR1_RESET;
	IO_RDR0    = Z180_RDR0_RESET;
	IO_RDR1    = Z180_RDR1_RESET;
	IO_CNTR    = Z180_CNTR_RESET;
	IO_TRDR    = Z180_TRDR_RESET;
	IO_TMDR0L  = Z180_TMDR0L_RESET;
	IO_TMDR0H  = Z180_TMDR0H_RESET;
	IO_RLDR0L  = Z180_RLDR0L_RESET;
	IO_RLDR0H  = Z180_RLDR0H_RESET;
	IO_TCR	   = Z180_TCR_RESET;
	IO_IO11    = Z180_IO11_RESET;
	IO_ASEXT0  = Z180_ASEXT0_RESET;
	IO_ASEXT1  = Z180_ASEXT1_RESET;
	IO_TMDR1L  = Z180_TMDR1L_RESET;
	IO_TMDR1H  = Z180_TMDR1H_RESET;
	IO_RLDR1L  = Z180_RLDR1L_RESET;
	IO_RLDR1H  = Z180_RLDR1H_RESET;
	IO_FRC	   = Z180_FRC_RESET;
	IO_IO19    = Z180_IO19_RESET;
	IO_ASTC0L  = Z180_ASTC0L_RESET;
	IO_ASTC0H  = Z180_ASTC0H_RESET;
	IO_ASTC1L  = Z180_ASTC1L_RESET;
	IO_ASTC1H  = Z180_ASTC1H_RESET;
	IO_CMR	   = Z180_CMR_RESET;
	IO_CCR	   = Z180_CCR_RESET;
	IO_SAR0L   = Z180_SAR0L_RESET;
	IO_SAR0H   = Z180_SAR0H_RESET;
	IO_SAR0B   = Z180_SAR0B_RESET;
	IO_DAR0L   = Z180_DAR0L_RESET;
	IO_DAR0H   = Z180_DAR0H_RESET;
	IO_DAR0B   = Z180_DAR0B_RESET;
	IO_BCR0L   = Z180_BCR0L_RESET;
	IO_BCR0H   = Z180_BCR0H_RESET;
	IO_MAR1L   = Z180_MAR1L_RESET;
	IO_MAR1H   = Z180_MAR1H_RESET;
	IO_MAR1B   = Z180_MAR1B_RESET;
	IO_IAR1L   = Z180_IAR1L_RESET;
	IO_IAR1H   = Z180_IAR1H_RESET;
	IO_IAR1B   = Z180_IAR1B_RESET;
	IO_BCR1L   = Z180_BCR1L_RESET;
	IO_BCR1H   = Z180_BCR1H_RESET;
	IO_DSTAT   = Z180_DSTAT_RESET;
	IO_DMODE   = Z180_DMODE_RESET;
	IO_DCNTL   = Z180_DCNTL_RESET;
	IO_IL	   = Z180_IL_RESET;
	IO_ITC	   = Z180_ITC_RESET;
	IO_IO35    = Z180_IO35_RESET;
	IO_RCR	   = Z180_RCR_RESET;
	IO_IO37    = Z180_IO37_RESET;
	IO_CBR	   = Z180_CBR_RESET;
	IO_BBR	   = Z180_BBR_RESET;
	IO_CBAR    = Z180_CBAR_RESET;
	IO_IO3B    = Z180_IO3B_RESET;
	IO_IO3C    = Z180_IO3C_RESET;
	IO_IO3D    = Z180_IO3D_RESET;
	IO_OMCR    = Z180_OMCR_RESET;
	IO_IOCR    = Z180_IOCR_RESET;

	if( daisy_chain )
	{
		while( daisy_chain->irq_param != -1 && Z180.irq_max < Z80_MAXDAISY )
		{
			/* set callbackhandler after reti */
			Z180.irq[Z180.irq_max] = *daisy_chain;
			/* device reset */
			if( Z180.irq[Z180.irq_max].reset )
				Z180.irq[Z180.irq_max].reset(Z180.irq[Z180.irq_max].irq_param);
			Z180.irq_max++;
			daisy_chain++;
		}
	}
	z180_mmu();
	z180_change_pc(_PCD);
}

void z180_exit(void)
{
#if BIG_FLAGS_ARRAY
	if (SZHVC_add) free(SZHVC_add);
	SZHVC_add = NULL;
	if (SZHVC_sub) free(SZHVC_sub);
	SZHVC_sub = NULL;
#endif
}

/****************************************************************************
 * Execute 'cycles' T-states. Return number of T-states really executed
 ****************************************************************************/
int z180_execute(int cycles)
{
	z180_icount = cycles - Z180.extra_cycles;
	Z180.extra_cycles = 0;

again:
    /* check if any DMA transfer is running */
	if ((IO_DSTAT & Z180_DSTAT_DME) == Z180_DSTAT_DME)
	{
		/* check if DMA channel 0 is running and also is in burst mode */
		if ((IO_DSTAT & Z180_DSTAT_DE0) == Z180_DSTAT_DE0 &&
			(IO_DMODE & Z180_DMODE_MMOD) == Z180_DMODE_MMOD)
		{
			CALL_MAME_DEBUG;
			z180_dma0();
		}
		else
		{
			do
			{
				_PPC = _PCD;
				CALL_MAME_DEBUG;
				_R++;
				EXEC_INLINE(op,ROP());
				z180_dma0();
				z180_dma1();
				/* If DMA is done break out to the faster loop */
				if ((IO_DSTAT & Z180_DSTAT_DME) != Z180_DSTAT_DME)
					break;
			} while( z180_icount > 0 );
		}
    }

    if (z180_icount > 0)
    {
        do
		{
			_PPC = _PCD;
			CALL_MAME_DEBUG;
			_R++;
			EXEC_INLINE(op,ROP());
			/* If DMA is started go to check the mode */
			if ((IO_DSTAT & Z180_DSTAT_DME) == Z180_DSTAT_DME)
				goto again;
        } while( z180_icount > 0 );
	}

	z180_icount -= Z180.extra_cycles;
	Z180.extra_cycles = 0;

	return cycles - z180_icount;
}

/****************************************************************************
 * Burn 'cycles' T-states. Adjust R register for the lost time
 ****************************************************************************/
void z180_burn(int cycles)
{
	if( cycles > 0 )
	{
		/* NOP takes 3 cycles per instruction */
		int n = (cycles + 2) / 3;
		_R += n;
		z180_icount -= 3 * n;
	}
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned z180_get_context (void *dst)
{
	if( dst )
		*(Z180_Regs*)dst = Z180;
	return sizeof(Z180_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void z180_set_context (void *src)
{
	if( src )
		Z180 = *(Z180_Regs*)src;
	z180_change_pc(_PCD);
}

/****************************************************************************
 * Get a pointer to a cycle count table
 ****************************************************************************/
const void *z180_get_cycle_table (int which)
{
	if (which >= 0 && which <= Z180_TABLE_xycb)
		return cc[which];
	return NULL;
}

/****************************************************************************
 * Set a new cycle count table
 ****************************************************************************/
void z180_set_cycle_table (int which, void *new_table)
{
	if (which >= 0 && which <= Z180_TABLE_ex)
		cc[which] = new_table;
}

/****************************************************************************
 * Return a specific register
 ****************************************************************************/
unsigned z180_get_reg (int regnum)
{
	switch( regnum )
	{
		case REG_PC: return _PCD;
		case Z180_PC: return Z180.PC.w.l;
		case REG_SP: return _SPD;
		case Z180_SP: return Z180.SP.w.l;
		case Z180_AF: return Z180.AF.w.l;
		case Z180_BC: return Z180.BC.w.l;
		case Z180_DE: return Z180.DE.w.l;
		case Z180_HL: return Z180.HL.w.l;
		case Z180_IX: return Z180.IX.w.l;
		case Z180_IY: return Z180.IY.w.l;
		case Z180_R: return (Z180.R & 0x7f) | (Z180.R2 & 0x80);
		case Z180_I: return Z180.I;
		case Z180_AF2: return Z180.AF2.w.l;
		case Z180_BC2: return Z180.BC2.w.l;
		case Z180_DE2: return Z180.DE2.w.l;
		case Z180_HL2: return Z180.HL2.w.l;
		case Z180_IM: return Z180.IM;
		case Z180_IFF1: return Z180.IFF1;
		case Z180_IFF2: return Z180.IFF2;
		case Z180_HALT: return Z180.HALT;
        case Z180_NMI_STATE: return Z180.nmi_state;
		case Z180_INT0_STATE: return Z180.irq_state[0];
		case Z180_INT1_STATE: return Z180.irq_state[1];
		case Z180_INT2_STATE: return Z180.irq_state[2];
		case Z180_DC0: return Z180.int_state[0];
		case Z180_DC1: return Z180.int_state[1];
		case Z180_DC2: return Z180.int_state[2];
		case Z180_DC3: return Z180.int_state[3];
		case Z180_CNTLA0: return Z180.io[0x00];
		case Z180_CNTLA1: return Z180.io[0x01];
		case Z180_CNTLB0: return Z180.io[0x02];
		case Z180_CNTLB1: return Z180.io[0x03];
		case Z180_STAT0: return Z180.io[0x04];
		case Z180_STAT1: return Z180.io[0x05];
		case Z180_TDR0: return Z180.io[0x06];
		case Z180_TDR1: return Z180.io[0x07];
		case Z180_RDR0: return Z180.io[0x08];
		case Z180_RDR1: return Z180.io[0x09];
		case Z180_CNTR: return Z180.io[0x0a];
		case Z180_TRDR: return Z180.io[0x0b];
		case Z180_TMDR0L: return Z180.io[0x0c];
		case Z180_TMDR0H: return Z180.io[0x0d];
		case Z180_RLDR0L: return Z180.io[0x0e];
		case Z180_RLDR0H: return Z180.io[0x0f];
		case Z180_TCR: return Z180.io[0x10];
		case Z180_IO11: return Z180.io[0x11];
		case Z180_ASEXT0: return Z180.io[0x12];
		case Z180_ASEXT1: return Z180.io[0x13];
		case Z180_TMDR1L: return Z180.io[0x14];
		case Z180_TMDR1H: return Z180.io[0x15];
		case Z180_RLDR1L: return Z180.io[0x16];
		case Z180_RLDR1H: return Z180.io[0x17];
		case Z180_FRC: return Z180.io[0x18];
		case Z180_IO19: return Z180.io[0x19];
		case Z180_ASTC0L: return Z180.io[0x1a];
		case Z180_ASTC0H: return Z180.io[0x1b];
		case Z180_ASTC1L: return Z180.io[0x1c];
		case Z180_ASTC1H: return Z180.io[0x1d];
		case Z180_CMR: return Z180.io[0x1e];
		case Z180_CCR: return Z180.io[0x1f];
		case Z180_SAR0L: return Z180.io[0x20];
		case Z180_SAR0H: return Z180.io[0x21];
		case Z180_SAR0B: return Z180.io[0x22];
		case Z180_DAR0L: return Z180.io[0x23];
		case Z180_DAR0H: return Z180.io[0x24];
		case Z180_DAR0B: return Z180.io[0x25];
		case Z180_BCR0L: return Z180.io[0x26];
		case Z180_BCR0H: return Z180.io[0x27];
		case Z180_MAR1L: return Z180.io[0x28];
		case Z180_MAR1H: return Z180.io[0x29];
		case Z180_MAR1B: return Z180.io[0x2a];
		case Z180_IAR1L: return Z180.io[0x2b];
		case Z180_IAR1H: return Z180.io[0x2c];
		case Z180_IAR1B: return Z180.io[0x2d];
		case Z180_BCR1L: return Z180.io[0x2e];
		case Z180_BCR1H: return Z180.io[0x2f];
		case Z180_DSTAT: return Z180.io[0x30];
		case Z180_DMODE: return Z180.io[0x31];
		case Z180_DCNTL: return Z180.io[0x32];
		case Z180_IL: return Z180.io[0x33];
		case Z180_ITC: return Z180.io[0x34];
		case Z180_IO35: return Z180.io[0x35];
		case Z180_RCR: return Z180.io[0x36];
		case Z180_IO37: return Z180.io[0x37];
		case Z180_CBR: return Z180.io[0x38];
		case Z180_BBR: return Z180.io[0x39];
		case Z180_CBAR: return Z180.io[0x3a];
		case Z180_IO3B: return Z180.io[0x3b];
		case Z180_IO3C: return Z180.io[0x3c];
		case Z180_IO3D: return Z180.io[0x3d];
		case Z180_OMCR: return Z180.io[0x3e];
		case Z180_IOCR: return Z180.io[0x3f];
		case Z180_IOLINES: return Z180.iol; break;
        case REG_PREVIOUSPC: return Z180.PREPC.w.l;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = _SPD + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return RM( offset ) | ( RM( offset + 1) << 8 );
			}
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void z180_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: _PC = val; z180_change_pc(_PCD); break;
		case Z180_PC: Z180.PC.w.l = val; break;
		case REG_SP: _SP = val; break;
		case Z180_SP: Z180.SP.w.l = val; break;
		case Z180_AF: Z180.AF.w.l = val; break;
		case Z180_BC: Z180.BC.w.l = val; break;
		case Z180_DE: Z180.DE.w.l = val; break;
		case Z180_HL: Z180.HL.w.l = val; break;
		case Z180_IX: Z180.IX.w.l = val; break;
		case Z180_IY: Z180.IY.w.l = val; break;
		case Z180_R: Z180.R = val; Z180.R2 = val & 0x80; break;
		case Z180_I: Z180.I = val; break;
		case Z180_AF2: Z180.AF2.w.l = val; break;
		case Z180_BC2: Z180.BC2.w.l = val; break;
		case Z180_DE2: Z180.DE2.w.l = val; break;
		case Z180_HL2: Z180.HL2.w.l = val; break;
		case Z180_IM: Z180.IM = val; break;
		case Z180_IFF1: Z180.IFF1 = val; break;
		case Z180_IFF2: Z180.IFF2 = val; break;
		case Z180_HALT: Z180.HALT = val; break;
        case Z180_NMI_STATE: z180_set_irq_line(IRQ_LINE_NMI,val); break;
		case Z180_INT0_STATE: z180_set_irq_line(0,val); break;
		case Z180_INT1_STATE: z180_set_irq_line(1,val); break;
		case Z180_INT2_STATE: z180_set_irq_line(2,val); break;
		case Z180_DC0: Z180.int_state[0] = val; break;
		case Z180_DC1: Z180.int_state[1] = val; break;
		case Z180_DC2: Z180.int_state[2] = val; break;
		case Z180_DC3: Z180.int_state[3] = val; break;
		case Z180_CNTLA0: Z180.io[0x00] = val; break;
		case Z180_CNTLA1: Z180.io[0x01] = val; break;
		case Z180_CNTLB0: Z180.io[0x02] = val; break;
		case Z180_CNTLB1: Z180.io[0x03] = val; break;
		case Z180_STAT0: Z180.io[0x04] = val; break;
		case Z180_STAT1: Z180.io[0x05] = val; break;
		case Z180_TDR0: Z180.io[0x06] = val; break;
		case Z180_TDR1: Z180.io[0x07] = val; break;
		case Z180_RDR0: Z180.io[0x08] = val; break;
		case Z180_RDR1: Z180.io[0x09] = val; break;
		case Z180_CNTR: Z180.io[0x0a] = val; break;
		case Z180_TRDR: Z180.io[0x0b] = val; break;
		case Z180_TMDR0L: Z180.io[0x0c] = val; break;
		case Z180_TMDR0H: Z180.io[0x0d] = val; break;
		case Z180_RLDR0L: Z180.io[0x0e] = val; break;
		case Z180_RLDR0H: Z180.io[0x0f] = val; break;
		case Z180_TCR: Z180.io[0x10] = val; break;
		case Z180_IO11: Z180.io[0x11] = val; break;
		case Z180_ASEXT0: Z180.io[0x12] = val; break;
		case Z180_ASEXT1: Z180.io[0x13] = val; break;
		case Z180_TMDR1L: Z180.io[0x14] = val; break;
		case Z180_TMDR1H: Z180.io[0x15] = val; break;
		case Z180_RLDR1L: Z180.io[0x16] = val; break;
		case Z180_RLDR1H: Z180.io[0x17] = val; break;
		case Z180_FRC: Z180.io[0x18] = val; break;
		case Z180_IO19: Z180.io[0x19] = val; break;
		case Z180_ASTC0L: Z180.io[0x1a] = val; break;
		case Z180_ASTC0H: Z180.io[0x1b] = val; break;
		case Z180_ASTC1L: Z180.io[0x1c] = val; break;
		case Z180_ASTC1H: Z180.io[0x1d] = val; break;
		case Z180_CMR: Z180.io[0x1e] = val; break;
		case Z180_CCR: Z180.io[0x1f] = val; break;
		case Z180_SAR0L: Z180.io[0x20] = val; break;
		case Z180_SAR0H: Z180.io[0x21] = val; break;
		case Z180_SAR0B: Z180.io[0x22] = val; break;
		case Z180_DAR0L: Z180.io[0x23] = val; break;
		case Z180_DAR0H: Z180.io[0x24] = val; break;
		case Z180_DAR0B: Z180.io[0x25] = val; break;
		case Z180_BCR0L: Z180.io[0x26] = val; break;
		case Z180_BCR0H: Z180.io[0x27] = val; break;
		case Z180_MAR1L: Z180.io[0x28] = val; break;
		case Z180_MAR1H: Z180.io[0x29] = val; break;
		case Z180_MAR1B: Z180.io[0x2a] = val; break;
		case Z180_IAR1L: Z180.io[0x2b] = val; break;
		case Z180_IAR1H: Z180.io[0x2c] = val; break;
		case Z180_IAR1B: Z180.io[0x2d] = val; break;
		case Z180_BCR1L: Z180.io[0x2e] = val; break;
		case Z180_BCR1H: Z180.io[0x2f] = val; break;
		case Z180_DSTAT: Z180.io[0x30] = val; break;
		case Z180_DMODE: Z180.io[0x31] = val; break;
		case Z180_DCNTL: Z180.io[0x32] = val; break;
		case Z180_IL: Z180.io[0x33] = val; break;
		case Z180_ITC: Z180.io[0x34] = val; break;
		case Z180_IO35: Z180.io[0x35] = val; break;
		case Z180_RCR: Z180.io[0x36] = val; break;
		case Z180_IO37: Z180.io[0x37] = val; break;
		case Z180_CBR: Z180.io[0x38] = val; z180_mmu(); break;
		case Z180_BBR: Z180.io[0x39] = val; z180_mmu(); break;
		case Z180_CBAR: Z180.io[0x3a] = val; z180_mmu(); break;
		case Z180_IO3B: Z180.io[0x3b] = val; break;
		case Z180_IO3C: Z180.io[0x3c] = val; break;
		case Z180_IO3D: Z180.io[0x3d] = val; break;
		case Z180_OMCR: Z180.io[0x3e] = val; break;
		case Z180_IOCR: Z180.io[0x3f] = val; break;
		case Z180_IOLINES: z180_write_iolines(val); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = _SPD + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, val & 0xff );
					WM( offset+1, (val >> 8) & 0xff );
				}
			}
	}
}

READ_HANDLER( z180_internal_r )
{
	return Z180.io[offset & 0x3f];
}

WRITE_HANDLER( z180_internal_w )
{
	z180_set_reg( Z180_CNTLA0 + (offset & 0x3f), data );
}

/****************************************************************************
 * Set IRQ line state
 ****************************************************************************/
void z180_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if( Z180.nmi_state == state ) return;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d set_irq_line (NMI) %d\n", cpu_getactivecpu(), state);
		Z180.nmi_state = state;
		if( state == CLEAR_LINE ) return;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d take NMI\n", cpu_getactivecpu());
		_PPC = -1;			/* there isn't a valid previous program counter */
		LEAVE_HALT; 		/* Check if processor was halted */

		/* disable DMA transfers!! */
		IO_DSTAT &= ~Z180_DSTAT_DME;

		_IFF1 = 0;
		PUSH( PC );
		_PCD = 0x0066;
		Z180.extra_cycles += 11;
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d set_irq_line %d\n",cpu_getactivecpu() , state);
		Z180.irq_state[irqline] = state;
		if( state == CLEAR_LINE ) return;

		if( irqline == 0 && Z180.irq_max )
		{
			int daisychain, device, int_state;
			daisychain = (*Z180.irq_callback)(irqline);
			device = daisychain >> 8;
			int_state = daisychain & 0xff;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d daisy chain $%04x -> device %d, state $%02x",cpu_getactivecpu(), daisychain, device, int_state);

			if( Z180.int_state[device] != int_state )
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE " change\n");
				/* set new interrupt status */
				Z180.int_state[device] = int_state;
				/* check interrupt status */
				Z180.request_irq = Z180.service_irq = -1;

				/* search higher IRQ or IEO */
				for( device = 0 ; device < Z180.irq_max ; device ++ )
				{
					/* IEO = disable ? */
					if( Z180.int_state[device] & Z80_INT_IEO )
					{
						Z180.request_irq = -1;		 /* if IEO is disable , masking lower IRQ */
						Z180.service_irq = device;	 /* set highest interrupt service device */
					}
					/* IRQ = request ? */
					if( Z180.int_state[device] & Z80_INT_REQ )
						Z180.request_irq = device;
				}
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d daisy chain service_irq $%02x, request_irq $%02x\n", cpu_getactivecpu(), Z180.service_irq, Z180.request_irq);
				if( Z180.request_irq < 0 ) return;
			}
			else
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE " no change\n");
				return;
			}
		}
		take_interrupt(irqline);
	}
}

/****************************************************************************
 * Set IRQ vector callback
 ****************************************************************************/
void z180_set_irq_callback(int (*callback)(int))
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Z180 #%d set_irq_callback $%08x\n",cpu_getactivecpu() , (int)callback);
	Z180.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *z180_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	Z180_Regs *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &Z180;

	switch( regnum )
	{
	case CPU_INFO_REG+Z180_PC: sprintf(buffer[which], "PC:%04X", r->PC.w.l); break;
	case CPU_INFO_REG+Z180_SP: sprintf(buffer[which], "SP:%04X", r->SP.w.l); break;
	case CPU_INFO_REG+Z180_AF: sprintf(buffer[which], "AF:%04X", r->AF.w.l); break;
	case CPU_INFO_REG+Z180_BC: sprintf(buffer[which], "BC:%04X", r->BC.w.l); break;
	case CPU_INFO_REG+Z180_DE: sprintf(buffer[which], "DE:%04X", r->DE.w.l); break;
	case CPU_INFO_REG+Z180_HL: sprintf(buffer[which], "HL:%04X", r->HL.w.l); break;
	case CPU_INFO_REG+Z180_IX: sprintf(buffer[which], "IX:%04X", r->IX.w.l); break;
	case CPU_INFO_REG+Z180_IY: sprintf(buffer[which], "IY:%04X", r->IY.w.l); break;
	case CPU_INFO_REG+Z180_R:  sprintf(buffer[which], "R   :%02X", (r->R & 0x7f) | (r->R2 & 0x80)); break;
	case CPU_INFO_REG+Z180_I:  sprintf(buffer[which], "I   :%02X", r->I); break;
	case CPU_INFO_REG+Z180_IL: sprintf(buffer[which], "IL  :%02X", r->io[Z180_IL-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_AF2: sprintf(buffer[which], "AF'%04X", r->AF2.w.l); break;
	case CPU_INFO_REG+Z180_BC2: sprintf(buffer[which], "BC'%04X", r->BC2.w.l); break;
	case CPU_INFO_REG+Z180_DE2: sprintf(buffer[which], "DE'%04X", r->DE2.w.l); break;
	case CPU_INFO_REG+Z180_HL2: sprintf(buffer[which], "HL'%04X", r->HL2.w.l); break;
	case CPU_INFO_REG+Z180_IM: sprintf(buffer[which], "IM  :%X", r->IM); break;
	case CPU_INFO_REG+Z180_IFF1: sprintf(buffer[which], "IFF1:%X", r->IFF1); break;
	case CPU_INFO_REG+Z180_IFF2: sprintf(buffer[which], "IFF2:%X", r->IFF2); break;
	case CPU_INFO_REG+Z180_HALT: sprintf(buffer[which], "HALT:%X", r->HALT); break;
	case CPU_INFO_REG+Z180_INT0_STATE: sprintf(buffer[which], "INT0:%X", r->irq_state[0]); break;
	case CPU_INFO_REG+Z180_INT1_STATE: sprintf(buffer[which], "INT1:%X", r->irq_state[1]); break;
	case CPU_INFO_REG+Z180_INT2_STATE: sprintf(buffer[which], "INT2:%X", r->irq_state[2]); break;
	case CPU_INFO_REG+Z180_DC0: if(Z180.irq_max >= 1) sprintf(buffer[which], "DC0:%X", r->int_state[0]); break;
	case CPU_INFO_REG+Z180_DC1: if(Z180.irq_max >= 2) sprintf(buffer[which], "DC1:%X", r->int_state[1]); break;
	case CPU_INFO_REG+Z180_DC2: if(Z180.irq_max >= 3) sprintf(buffer[which], "DC2:%X", r->int_state[2]); break;
	case CPU_INFO_REG+Z180_DC3: if(Z180.irq_max >= 4) sprintf(buffer[which], "DC3:%X", r->int_state[3]); break;
	case CPU_INFO_REG+Z180_CCR:  sprintf(buffer[which], "CCR :%02X", r->io[Z180_CCR-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_ITC:  sprintf(buffer[which], "ITC :%02X", r->io[Z180_ITC-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_CBR:  sprintf(buffer[which], "CBR :%02X", r->io[Z180_CBR-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_BBR:  sprintf(buffer[which], "BBR :%02X", r->io[Z180_BBR-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_CBAR: sprintf(buffer[which], "CBAR:%02X", r->io[Z180_CBAR-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_OMCR: sprintf(buffer[which], "OMCR:%02X", r->io[Z180_OMCR-Z180_CNTLA0]); break;
	case CPU_INFO_REG+Z180_IOCR: sprintf(buffer[which], "IOCR:%02X", r->io[Z180_IOCR-Z180_CNTLA0]); break;
    case CPU_INFO_FLAGS:
		sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
			r->AF.b.l & 0x80 ? 'S':'.',
			r->AF.b.l & 0x40 ? 'Z':'.',
			r->AF.b.l & 0x20 ? '5':'.',
			r->AF.b.l & 0x10 ? 'H':'.',
			r->AF.b.l & 0x08 ? '3':'.',
			r->AF.b.l & 0x04 ? 'P':'.',
			r->AF.b.l & 0x02 ? 'N':'.',
			r->AF.b.l & 0x01 ? 'C':'.');
		break;
	case CPU_INFO_NAME: return "Z180";
	case CPU_INFO_FAMILY: return "Zilog Z8x180";
	case CPU_INFO_VERSION: return "0.2";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright (C) 2000 Juergen Buchmueller, all rights reserved.";
	case CPU_INFO_REG_LAYOUT: return (const char *)z180_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char *)z180_win_layout;
	}
	return buffer[which];
}

unsigned z180_dasm( char *buffer, unsigned pc )
{
#ifdef MAME_DEBUG
	return DasmZ180( buffer, pc );
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
