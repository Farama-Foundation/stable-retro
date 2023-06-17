/*###################################################################################################
**
**
**		dsp32.c
**		Core implementation for the portable DSP32 emulator.
**		Written by Aaron Giles
**
**
**##################################################################################################
**
**
**		Important note:
**
**		At this time, the emulator is rather incomplete. However, it is sufficiently
**		complete to run both Race Drivin' and Hard Drivin's Airborne, which is all I
**		was after.
**
**		Things that still need to be implemented:
**
**			* interrupts
**			* carry-reverse add operations
**			* do loops
**			* ieee/dsp conversions
**			* input/output conversion
**			* serial I/O
**
**		In addition, there are several optimizations enabled which make assumptions
**		about the code which may not be valid for other applications. Check dsp32ops.c
**		for details.
**
**
**#################################################################################################*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cpuintrf.h"
#include "mamedbg.h"
#include "dsp32.h"



/*###################################################################################################
**	DEBUGGING
**#################################################################################################*/

#define DETECT_MISALIGNED_MEMORY	0



/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* internal register numbering for PIO registers */
#define PIO_PAR			0
#define PIO_PDR			1
#define PIO_EMR			2
#define PIO_ESR			3
#define PIO_PCR			4
#define PIO_PIR			5
#define PIO_PARE		6
#define PIO_PDR2		7
#define PIO_RESERVED	8

#define UPPER			(0x00ff << 8)
#define LOWER			(0xff00 << 8)

/* bits in the PCR register */
#define PCR_RESET		0x001
#define PCR_REGMAP		0x002
#define PCR_ENI			0x004
#define PCR_DMA			0x008
#define PCR_AUTO		0x010
#define PCR_PDFs		0x020
#define PCR_PIFs		0x040
#define PCR_RES			0x080
#define PCR_DMA32		0x100
#define PCR_PIO16		0x200
#define PCR_FLG			0x400

/* internal flag bits */
#define UFLAGBIT		1
#define VFLAGBIT		2



/*###################################################################################################
**	MACROS
**#################################################################################################*/

/* register mapping */
#define R0				r[0]
#define R1				r[1]
#define R2				r[2]
#define R3				r[3]
#define R4				r[4]
#define R5				r[5]
#define R6				r[6]
#define R7				r[7]
#define R8				r[8]
#define R9				r[9]
#define R10				r[10]
#define R11				r[11]
#define R12				r[12]
#define R13				r[13]
#define R14				r[14]
#define PC				r[15]
#define R0_ALT			r[16]
#define R15				r[17]
#define R16				r[18]
#define R17				r[19]
#define R18				r[20]
#define R19				r[21]
#define RMM				r[22]
#define RPP				r[23]
#define R20				r[24]
#define R21				r[25]
#define DAUC			r[26]
#define IOC				r[27]
#define R22				r[29]
#define PCSH			r[30]

#define A0				a[0]
#define A1				a[1]
#define A2				a[2]
#define A3				a[3]
#define A_0				a[4]
#define A_1				a[5]

#define OP				dsp32.op

#define zFLAG			((dsp32.nzcflags & 0xffffff) == 0)
#define nFLAG			((dsp32.nzcflags & 0x800000) != 0)
#define cFLAG			((dsp32.nzcflags & 0x1000000) != 0)
#define vFLAG			((dsp32.vflags & 0x800000) != 0)
#define ZFLAG			(dsp32.NZflags == 0)
#define NFLAG			(dsp32.NZflags < 0)
#define UFLAG			(dsp32.VUflags & UFLAGBIT)
#define VFLAG			(dsp32.VUflags & VFLAGBIT)



/*###################################################################################################
**	STRUCTURES & TYPEDEFS
**#################################################################################################*/

/* DSP32 Registers */
typedef struct
{
	/* core registers */
	UINT32			r[32];
	UINT32			pin, pout;
	UINT32			ivtp;
	UINT32			nzcflags;
	UINT32			vflags;

	double			a[6];
	double			NZflags;
	UINT8			VUflags;

	double			abuf[4];
	UINT8			abufreg[4];
	UINT8			abufVUflags[4];
	UINT8			abufNZflags[4];
	int				abufcycle[4];
	int				abuf_index;

	INT32			mbufaddr[4];
	UINT32			mbufdata[4];
	int				mbuf_index;

	UINT16			par;
	UINT8			pare;
	UINT16			pdr;
	UINT16			pdr2;
	UINT16			pir;
	UINT16			pcr;
	UINT16			emr;
	UINT8			esr;
	UINT16			pcw;
	UINT8			piop;

	UINT32			ibuf;
	UINT32			isr;
	UINT32			obuf;
	UINT32			osr;

	/* internal stuff */
	UINT8			lastpins;
	UINT32			ppc;
	UINT32			op;
	int				interrupt_cycles;
	void			(*output_pins_changed)(UINT32 pins);
} dsp32_regs;



/*###################################################################################################
**	PROTOTYPES
**#################################################################################################*/


/*###################################################################################################
**	PUBLIC GLOBAL VARIABLES
**#################################################################################################*/

int dsp32_icount;



/*###################################################################################################
**	PRIVATE GLOBAL VARIABLES
**#################################################################################################*/

static dsp32_regs dsp32;



/*###################################################################################################
**	MEMORY ACCESSORS
**#################################################################################################*/

#define ROPCODE(pc)			cpu_readop32(pc)

#define RBYTE(addr)			cpu_readmem24ledw(addr)
#define WBYTE(addr,data)	cpu_writemem24ledw((addr), data)

#if (!DETECT_MISALIGNED_MEMORY)

#define RWORD(addr)			cpu_readmem24ledw_word(addr)
#define WWORD(addr,data)	cpu_writemem24ledw_word((addr), data)
#define RLONG(addr)			cpu_readmem24ledw_dword(addr)
#define WLONG(addr,data)	cpu_writemem24ledw_dword((addr), data)

#else

static INLINE data16_t RWORD(offs_t addr)
{
	data16_t data;
	if (addr & 1) fprintf(stderr, "Unaligned word read @ %06X, PC=%06X\n", addr, dsp32.PC);
	data = cpu_readmem24ledw_word(addr);
	return data;
}

static INLINE data32_t RLONG(offs_t addr)
{
	data32_t data;
	if (addr & 3) fprintf(stderr, "Unaligned long read @ %06X, PC=%06X\n", addr, dsp32.PC);
	data = cpu_readmem24ledw_dword(addr);
	return data;
}

static INLINE void WWORD(offs_t addr, data16_t data)
{
	if (addr & 1) fprintf(stderr, "Unaligned word write @ %06X, PC=%06X\n", addr, dsp32.PC);
	cpu_writemem24ledw_word((addr), data);
}

static INLINE void WLONG(offs_t addr, data32_t data)
{
	if (addr & 3) fprintf(stderr, "Unaligned long write @ %06X, PC=%06X\n", addr, dsp32.PC);
	cpu_writemem24ledw_dword((addr), data);
}

#endif



/*###################################################################################################
**	EXECEPTION HANDLING
**#################################################################################################*/

static INLINE void generate_exception(int exception)
{
}


static INLINE void invalid_instruction(UINT32 op)
{
}



/*###################################################################################################
**	IRQ HANDLING
**#################################################################################################*/

static void check_irqs(void)
{
	/* finish me! */
}


void dsp32c_set_irq_line(int irqline, int state)
{
	/* finish me! */
}


void dsp32c_set_irq_callback(int (*callback)(int irqline))
{
	/* finish me! */
}



/*###################################################################################################
**	REGISTER HANDLING
**#################################################################################################*/

static void update_pcr(data16_t newval)
{
	data16_t oldval = dsp32.pcr;
	dsp32.pcr = newval;

	/* reset the chip if we get a reset */
	if ((oldval & PCR_RESET) == 0 && (newval & PCR_RESET) != 0)
		dsp32c_reset(0);

	/* track the state of the output pins */
	if (dsp32.output_pins_changed)
	{
		data16_t newoutput = ((newval & (PCR_PIFs | PCR_ENI)) == (PCR_PIFs | PCR_ENI)) ? DSP32_OUTPUT_PIF : 0;
		if (newoutput != dsp32.lastpins)
		{
			dsp32.lastpins = newoutput;
			(*dsp32.output_pins_changed)(newoutput);
		}
	}
}



/*###################################################################################################
**	CONTEXT SWITCHING
**#################################################################################################*/

unsigned dsp32c_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(dsp32_regs *)dst = dsp32;

	/* return the context size */
	return sizeof(dsp32_regs);
}


void dsp32c_set_context(void *src)
{
	/* copy the context */
	if (src)
		dsp32 = *(dsp32_regs *)src;
	cpu_setopbase24ledw(dsp32.PC);

	/* check for IRQs */
	check_irqs();
}



/*###################################################################################################
**	INITIALIZATION AND SHUTDOWN
**#################################################################################################*/

void dsp32c_init(void)
{
}


void dsp32c_reset(void *param)
{
	struct dsp32_config *config = param;

	/* copy in config data */
	if (config)
		dsp32.output_pins_changed = config->output_pins_changed;

	/* reset goes to 0 */
	dsp32.PC = 0;
	cpu_setopbase24ledw(dsp32.PC);

	/* clear some registers */
	dsp32.pcw &= 0x03ff;
	update_pcr(dsp32.pcr & PCR_RESET);
	dsp32.esr = 0;
	dsp32.emr = 0xffff;

	/* initialize fixed registers */
	dsp32.R0 = dsp32.R0_ALT = 0;
	dsp32.RMM = -1;
	dsp32.RPP = 1;
	dsp32.A_0 = 0.0;
	dsp32.A_1 = 1.0;

	/* init internal stuff */
	dsp32.abufcycle[0] = dsp32.abufcycle[1] = dsp32.abufcycle[2] = dsp32.abufcycle[3] = 12345678;
	dsp32.mbufaddr[0] = dsp32.mbufaddr[1] = dsp32.mbufaddr[2] = dsp32.mbufaddr[3] = 1;
}


void dsp32c_exit(void)
{
}



/*###################################################################################################
**	CORE INCLUDE
**#################################################################################################*/

#include "dsp32ops.c"



/*###################################################################################################
**	CORE EXECUTION LOOP
**#################################################################################################*/

int dsp32c_execute(int cycles)
{
	/* skip if halted */
	if ((dsp32.pcr & PCR_RESET) == 0)
		return cycles;

	/* count cycles and interrupt cycles */
	dsp32_icount = cycles;
	dsp32_icount -= dsp32.interrupt_cycles;
	dsp32.interrupt_cycles = 0;

	/* update buffered accumulator values */
	dsp32.abufcycle[0] += dsp32_icount;
	dsp32.abufcycle[1] += dsp32_icount;
	dsp32.abufcycle[2] += dsp32_icount;
	dsp32.abufcycle[3] += dsp32_icount;

	while (dsp32_icount > 0)
		execute_one();

	dsp32_icount -= dsp32.interrupt_cycles;
	dsp32.interrupt_cycles = 0;

	/* normalize buffered accumulator values */
	dsp32.abufcycle[0] -= dsp32_icount;
	dsp32.abufcycle[1] -= dsp32_icount;
	dsp32.abufcycle[2] -= dsp32_icount;
	dsp32.abufcycle[3] -= dsp32_icount;

	return cycles - dsp32_icount;
}



/*###################################################################################################
**	REGISTER SNOOP
**#################################################################################################*/

unsigned dsp32c_get_reg(int regnum)
{
	switch (regnum)
	{
		/* CAU */
		case REG_PC:
		case DSP32_PC:			return dsp32.PC;
		case DSP32_R0:			return dsp32.R0;
		case DSP32_R1:			return dsp32.R1;
		case DSP32_R2:			return dsp32.R2;
		case DSP32_R3:			return dsp32.R3;
		case DSP32_R4:			return dsp32.R4;
		case DSP32_R5:			return dsp32.R5;
		case DSP32_R6:			return dsp32.R6;
		case DSP32_R7:			return dsp32.R7;
		case DSP32_R8:			return dsp32.R8;
		case DSP32_R9:			return dsp32.R9;
		case DSP32_R10:			return dsp32.R10;
		case DSP32_R11:			return dsp32.R11;
		case DSP32_R12:			return dsp32.R12;
		case DSP32_R13:			return dsp32.R13;
		case DSP32_R14:			return dsp32.R14;
		case DSP32_R15:			return dsp32.R15;
		case DSP32_R16:			return dsp32.R16;
		case DSP32_R17:			return dsp32.R17;
		case DSP32_R18:			return dsp32.R18;
		case DSP32_R19:			return dsp32.R19;
		case DSP32_R20:			return dsp32.R20;
		case REG_SP:
		case DSP32_R21:			return dsp32.R21;
		case DSP32_R22:			return dsp32.R22;
		case DSP32_PIN:			return dsp32.pin;
		case DSP32_POUT:		return dsp32.pout;
		case DSP32_IVTP:		return dsp32.ivtp;

		/* DAU */
		case DSP32_A0:			return dsp32.A0;	/* fix me -- very wrong */
		case DSP32_A1:			return dsp32.A1;	/* fix me -- very wrong */
		case DSP32_A2:			return dsp32.A2;	/* fix me -- very wrong */
		case DSP32_A3:			return dsp32.A3;	/* fix me -- very wrong */
		case DSP32_DAUC:		return dsp32.DAUC;

		/* PIO */
		case DSP32_PAR:			return dsp32.par;
		case DSP32_PDR:			return dsp32.pdr;
		case DSP32_PIR:			return dsp32.pir;
		case DSP32_PCR:			return dsp32.pcr;
		case DSP32_EMR:			return dsp32.emr;
		case DSP32_ESR:			return dsp32.esr;
		case DSP32_PCW:			return dsp32.pcw;
		case DSP32_PIOP:		return dsp32.piop;

		/* SIO */
		case DSP32_IBUF:		return dsp32.ibuf;
		case DSP32_ISR:			return dsp32.isr;
		case DSP32_OBUF:		return dsp32.obuf;
		case DSP32_OSR:			return dsp32.osr;
		case DSP32_IOC:			return dsp32.IOC;

		case REG_PREVIOUSPC: 	return dsp32.ppc;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
/*				unsigned offset = REG_SP_CONTENTS - regnum;*/
/*				if (offset < PC_STACK_DEPTH)*/
/*					return tms32031.pc_stack[offset];*/
			}
	}
	return 0;
}



/*###################################################################################################
**	REGISTER MODIFY
**#################################################################################################*/

void dsp32c_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		/* CAU */
		case REG_PC:
		case DSP32_PC:			dsp32.PC = val & 0xffffff; break;
		case DSP32_R0:			dsp32.R0 = val & 0xffffff; break;
		case DSP32_R1:			dsp32.R1 = val & 0xffffff; break;
		case DSP32_R2:			dsp32.R2 = val & 0xffffff; break;
		case DSP32_R3:			dsp32.R3 = val & 0xffffff; break;
		case DSP32_R4:			dsp32.R4 = val & 0xffffff; break;
		case DSP32_R5:			dsp32.R5 = val & 0xffffff; break;
		case DSP32_R6:			dsp32.R6 = val & 0xffffff; break;
		case DSP32_R7:			dsp32.R7 = val & 0xffffff; break;
		case DSP32_R8:			dsp32.R8 = val & 0xffffff; break;
		case DSP32_R9:			dsp32.R9 = val & 0xffffff; break;
		case DSP32_R10:			dsp32.R10 = val & 0xffffff; break;
		case DSP32_R11:			dsp32.R11 = val & 0xffffff; break;
		case DSP32_R12:			dsp32.R12 = val & 0xffffff; break;
		case DSP32_R13:			dsp32.R13 = val & 0xffffff; break;
		case DSP32_R14:			dsp32.R14 = val & 0xffffff; break;
		case DSP32_R15:			dsp32.R15 = val & 0xffffff; break;
		case DSP32_R16:			dsp32.R16 = val & 0xffffff; break;
		case DSP32_R17:			dsp32.R17 = val & 0xffffff; break;
		case DSP32_R18:			dsp32.R18 = val & 0xffffff; break;
		case DSP32_R19:			dsp32.R19 = val & 0xffffff; break;
		case DSP32_R20:			dsp32.R20 = val & 0xffffff; break;
		case REG_SP:
		case DSP32_R21:			dsp32.R21 = val & 0xffffff; break;
		case DSP32_R22:			dsp32.R22 = val & 0xffffff; break;
		case DSP32_PIN:			dsp32.pin = val & 0xffffff; break;
		case DSP32_POUT:		dsp32.pout = val & 0xffffff; break;
		case DSP32_IVTP:		dsp32.ivtp = val & 0xffffff; break;

		/* DAU */
		case DSP32_A0:			dsp32.A0 = val; break;	/* fix me -- very wrong */
		case DSP32_A1:			dsp32.A1 = val; break;	/* fix me -- very wrong */
		case DSP32_A2:			dsp32.A2 = val; break;	/* fix me -- very wrong */
		case DSP32_A3:			dsp32.A3 = val; break;	/* fix me -- very wrong */
		case DSP32_DAUC:		dsp32.DAUC = val; break;

		/* PIO */
		case DSP32_PAR:			dsp32.par = val; break;
		case DSP32_PDR:			dsp32.pdr = val; break;
		case DSP32_PIR:			dsp32.pir = val; break;
		case DSP32_PCR:			update_pcr(val & 0x3ff); break;
		case DSP32_EMR:			dsp32.emr = val; break;
		case DSP32_ESR:			dsp32.esr = val; break;
		case DSP32_PCW:			dsp32.pcw = val; break;
		case DSP32_PIOP:		dsp32.piop = val; break;

		/* SIO */
		case DSP32_IBUF:		dsp32.ibuf = val; break;
		case DSP32_ISR:			dsp32.isr = val; break;
		case DSP32_OBUF:		dsp32.obuf = val; break;
		case DSP32_OSR:			dsp32.osr = val; break;
		case DSP32_IOC:			dsp32.IOC = val & 0xfffff; break;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
/*				unsigned offset = REG_SP_CONTENTS - regnum;*/
/*				if (offset < PC_STACK_DEPTH)*/
/*					tms32031.pc_stack[offset] = val;*/
			}
    }
}



/*###################################################################################################
**	DEBUGGER DEFINITIONS
**#################################################################################################*/

static UINT8 dsp32c_reg_layout[] =
{
	DSP32_PC,		DSP32_R11,		-1,
	DSP32_R0,	 	DSP32_R12,		-1,
	DSP32_R1, 		DSP32_R13,		-1,
	DSP32_R2, 		DSP32_R14,		-1,
	DSP32_R3, 		DSP32_R15,		-1,
	DSP32_R4, 		DSP32_R16,		-1,
	DSP32_R5, 		DSP32_R17,		-1,
	DSP32_R6, 		DSP32_R18,		-1,
	DSP32_R7, 		DSP32_R19,		-1,
	DSP32_R8,		DSP32_R20,		-1,
	DSP32_R9,		DSP32_R21,		-1,
	DSP32_R10,		DSP32_R22,		-1,
	DSP32_PCW,		DSP32_A0,		-1,
	DSP32_PCR,		DSP32_A1,		-1,
	DSP32_PIR,		DSP32_A2,		-1,
	DSP32_EMR,		DSP32_A3,		-1,
	DSP32_ESR,		DSP32_DAUC,		0
};

static UINT8 dsp32c_win_layout[] =
{
	 0, 0,30,20,	/* register window (top rows) */
	31, 0,48,14,	/* disassembler window (left colums) */
	 0,21,30, 1,	/* memory #1 window (right, upper middle) */
	31,15,48, 7,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};



/*###################################################################################################
**	DEBUGGER STRINGS
**#################################################################################################*/

const char *dsp32c_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	dsp32_regs *r = context;

	which = ( which + 1 ) % 16;
    buffer[which][0] = '\0';

	if (!context)
		r = &dsp32;

    switch (regnum)
	{
		/* CAU */
		case CPU_INFO_REG+REG_PC:
		case CPU_INFO_REG+DSP32_PC:		sprintf(buffer[which], "PC: %06X", dsp32.PC); break;
		case CPU_INFO_REG+DSP32_R0:		sprintf(buffer[which], "R0: %06X", dsp32.R0); break;
		case CPU_INFO_REG+DSP32_R1:		sprintf(buffer[which], "R1: %06X", dsp32.R1); break;
		case CPU_INFO_REG+DSP32_R2:		sprintf(buffer[which], "R2: %06X", dsp32.R2); break;
		case CPU_INFO_REG+DSP32_R3:		sprintf(buffer[which], "R3: %06X", dsp32.R3); break;
		case CPU_INFO_REG+DSP32_R4:		sprintf(buffer[which], "R4: %06X", dsp32.R4); break;
		case CPU_INFO_REG+DSP32_R5:		sprintf(buffer[which], "R5: %06X", dsp32.R5); break;
		case CPU_INFO_REG+DSP32_R6:		sprintf(buffer[which], "R6: %06X", dsp32.R6); break;
		case CPU_INFO_REG+DSP32_R7:		sprintf(buffer[which], "R7: %06X", dsp32.R7); break;
		case CPU_INFO_REG+DSP32_R8:		sprintf(buffer[which], "R8: %06X", dsp32.R8); break;
		case CPU_INFO_REG+DSP32_R9:		sprintf(buffer[which], "R9: %06X", dsp32.R9); break;
		case CPU_INFO_REG+DSP32_R10:	sprintf(buffer[which], "R10:%06X", dsp32.R10); break;
		case CPU_INFO_REG+DSP32_R11:	sprintf(buffer[which], "R11:%06X", dsp32.R11); break;
		case CPU_INFO_REG+DSP32_R12:	sprintf(buffer[which], "R12:%06X", dsp32.R12); break;
		case CPU_INFO_REG+DSP32_R13:	sprintf(buffer[which], "R13:%06X", dsp32.R13); break;
		case CPU_INFO_REG+DSP32_R14:	sprintf(buffer[which], "R14:%06X", dsp32.R14); break;
		case CPU_INFO_REG+DSP32_R15:	sprintf(buffer[which], "R15:%06X", dsp32.R15); break;
		case CPU_INFO_REG+DSP32_R16:	sprintf(buffer[which], "R16:%06X", dsp32.R16); break;
		case CPU_INFO_REG+DSP32_R17:	sprintf(buffer[which], "R17:%06X", dsp32.R17); break;
		case CPU_INFO_REG+DSP32_R18:	sprintf(buffer[which], "R18:%06X", dsp32.R18); break;
		case CPU_INFO_REG+DSP32_R19:	sprintf(buffer[which], "R19:%06X", dsp32.R19); break;
		case CPU_INFO_REG+DSP32_R20:	sprintf(buffer[which], "R20:%06X", dsp32.R20); break;
		case CPU_INFO_REG+REG_SP:
		case CPU_INFO_REG+DSP32_R21:	sprintf(buffer[which], "R21:%06X", dsp32.R21); break;
		case CPU_INFO_REG+DSP32_R22:	sprintf(buffer[which], "R22:%06X", dsp32.R22); break;
		case CPU_INFO_REG+DSP32_PIN:	sprintf(buffer[which], "PIN:%06X", dsp32.pin); break;
		case CPU_INFO_REG+DSP32_POUT:	sprintf(buffer[which], "POUT:%06X", dsp32.pout); break;
		case CPU_INFO_REG+DSP32_IVTP:	sprintf(buffer[which], "IVTP:%06X", dsp32.ivtp); break;

		/* DAU */
		case CPU_INFO_REG+DSP32_A0:		sprintf(buffer[which], "A0:%8g", dsp32.A0); break;
		case CPU_INFO_REG+DSP32_A1:		sprintf(buffer[which], "A1:%8g", dsp32.A1); break;
		case CPU_INFO_REG+DSP32_A2:		sprintf(buffer[which], "A2:%8g", dsp32.A2); break;
		case CPU_INFO_REG+DSP32_A3:		sprintf(buffer[which], "A3:%8g", dsp32.A3); break;
		case CPU_INFO_REG+DSP32_DAUC:	sprintf(buffer[which], "DAUC:%02X", dsp32.DAUC); break;

		/* PIO */
		case CPU_INFO_REG+DSP32_PAR:	sprintf(buffer[which], "PAR:%08X", dsp32.par); break;
		case CPU_INFO_REG+DSP32_PDR:	sprintf(buffer[which], "PDR:%08X", dsp32.pdr); break;
		case CPU_INFO_REG+DSP32_PIR:	sprintf(buffer[which], "PIR:%04X", dsp32.pir); break;
		case CPU_INFO_REG+DSP32_PCR:	sprintf(buffer[which], "PCR:%03X", dsp32.pcr); break;
		case CPU_INFO_REG+DSP32_EMR:	sprintf(buffer[which], "EMR:%04X", dsp32.emr); break;
		case CPU_INFO_REG+DSP32_ESR:	sprintf(buffer[which], "ESR:%02X", dsp32.esr); break;
		case CPU_INFO_REG+DSP32_PCW:	sprintf(buffer[which], "PCW:%04X", dsp32.pcw); break;
		case CPU_INFO_REG+DSP32_PIOP:	sprintf(buffer[which], "PIOP:%02X", dsp32.piop); break;

		/* SIO */
		case CPU_INFO_REG+DSP32_IBUF:	sprintf(buffer[which], "IBUF:%08X", dsp32.ibuf); break;
		case CPU_INFO_REG+DSP32_ISR:	sprintf(buffer[which], "ISR:%08X", dsp32.isr); break;
		case CPU_INFO_REG+DSP32_OBUF:	sprintf(buffer[which], "OBUF:%08X", dsp32.obuf); break;
		case CPU_INFO_REG+DSP32_OSR:	sprintf(buffer[which], "OSR:%08X", dsp32.osr); break;
		case CPU_INFO_REG+DSP32_IOC:	sprintf(buffer[which], "IOC:%05X", dsp32.IOC); break;

		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				NFLAG ? 'N':'.',
				ZFLAG ? 'Z':'.',
                UFLAG ? 'U':'.',
                VFLAG ? 'V':'.',
                nFLAG ? 'n':'.',
                zFLAG ? 'z':'.',
                cFLAG ? 'c':'.',
                vFLAG ? 'v':'.');
            break;

		case CPU_INFO_NAME: return "DSP32C";
		case CPU_INFO_FAMILY: return "Lucent DSP32";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Aaron Giles";
		case CPU_INFO_REG_LAYOUT: return (const char *)dsp32c_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)dsp32c_win_layout;
		case CPU_INFO_REG+10000: return "         ";
    }
	return buffer[which];
}



/*###################################################################################################
**	DISASSEMBLY HOOK
**#################################################################################################*/

unsigned dsp32c_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasm_dsp32(char *, unsigned);
	return dasm_dsp32(buffer, pc);
#else
	strcpy(buffer, "???");
	return 4;
#endif
}



/*###################################################################################################
**	PARALLEL INTERFACE WRITES
**#################################################################################################*/

/* context finder */
static INLINE dsp32_regs *FINDCONTEXT(int cpu)
{
	dsp32_regs *context = cpunum_get_context_ptr(cpu);
	if (!context)
		context = &dsp32;
	return context;
}

static UINT32 regmap[4][16] =
{
	{	/* DSP32 compatible mode */
		PIO_PAR|LOWER, PIO_PAR|UPPER, PIO_PDR|LOWER, PIO_PDR|UPPER,
		PIO_EMR|LOWER, PIO_EMR|UPPER, PIO_ESR|LOWER, PIO_PCR|LOWER,
		PIO_PIR|UPPER, PIO_PIR|UPPER, PIO_PIR|UPPER, PIO_PIR|UPPER,
		PIO_PIR|UPPER, PIO_PIR|UPPER, PIO_PIR|UPPER, PIO_PIR|UPPER
	},
	{	/* DSP32C 8-bit mode */
		PIO_PAR|LOWER, PIO_PAR|UPPER, PIO_PDR|LOWER, PIO_PDR|UPPER,
		PIO_EMR|LOWER, PIO_EMR|UPPER, PIO_ESR|LOWER, PIO_PCR|LOWER,
		PIO_PIR|LOWER, PIO_PIR|UPPER, PIO_PCR|UPPER, PIO_PARE|LOWER,
		PIO_PDR2|LOWER,PIO_PDR2|UPPER,PIO_RESERVED,  PIO_RESERVED
	},
	{	/* DSP32C illegal mode */
		PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,
		PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,
		PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,
		PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED
	},
	{	/* DSP32C 16-bit mode */
		PIO_PAR,       PIO_RESERVED,  PIO_PDR,       PIO_RESERVED,
		PIO_EMR,       PIO_RESERVED,  PIO_ESR|LOWER, PIO_PCR,
		PIO_PIR,       PIO_RESERVED,  PIO_RESERVED,  PIO_PARE|LOWER,
		PIO_PDR2,      PIO_RESERVED,  PIO_RESERVED,  PIO_RESERVED
	}
};



/*###################################################################################################
**	PARALLEL INTERFACE WRITES
**#################################################################################################*/

static INLINE void dma_increment(void)
{
	if (dsp32.pcr & PCR_AUTO)
	{
		int amount = (dsp32.pcr & PCR_DMA32) ? 4 : 2;
		dsp32.par += amount;
		if (dsp32.par < amount)
			dsp32.pare++;
	}
}


static INLINE void dma_load(void)
{
	/* only process if DMA is enabled */
	if (dsp32.pcr & PCR_DMA)
	{
		UINT32 addr = dsp32.par | (dsp32.pare << 16);

		/* 16-bit case */
		if (!(dsp32.pcr & PCR_DMA32))
			dsp32.pdr = RWORD(addr & 0xfffffe);

		/* 32-bit case */
		else
		{
			UINT32 temp = RLONG(addr & 0xfffffc);
			dsp32.pdr = temp >> 16;
			dsp32.pdr2 = temp & 0xffff;
		}

		/* set the PDF flag to indicate we have data ready */
		update_pcr(dsp32.pcr | PCR_PDFs);
	}
}


static INLINE void dma_store(void)
{
	/* only process if DMA is enabled */
	if (dsp32.pcr & PCR_DMA)
	{
		UINT32 addr = dsp32.par | (dsp32.pare << 16);

		/* 16-bit case */
		if (!(dsp32.pcr & PCR_DMA32))
			WWORD(addr & 0xfffffe, dsp32.pdr);

		/* 32-bit case */
		else
			WLONG(addr & 0xfffffc, (dsp32.pdr << 16) | dsp32.pdr2);

		/* clear the PDF flag to indicate we have taken the data */
		update_pcr(dsp32.pcr & ~PCR_PDFs);
	}
}


void dsp32c_pio_w(int cpunum, int reg, int data)
{
	data16_t mask;
	UINT8 mode;

	cpuintrf_push_context(cpunum);

	/* look up register and mask */
	mode = ((dsp32.pcr >> 8) & 2) | ((dsp32.pcr >> 1) & 1);
	reg = regmap[mode][reg];
	mask = reg >> 8;
	if (mask == 0x00ff) data <<= 8;
	data &= ~mask;
	reg &= 0xff;

	/* switch off the register */
	switch (reg)
	{
		case PIO_PAR:
			dsp32.par = (dsp32.par & mask) | data;

			/* trigger a load on the upper half */
			if (!(mask & 0xff00))
				dma_load();
			break;

		case PIO_PARE:
			dsp32.pare = (dsp32.pare & mask) | data;
			break;

		case PIO_PDR:
			dsp32.pdr = (dsp32.pdr & mask) | data;

			/* trigger a write and PDF setting on the upper half */
			if (!(mask & 0xff00))
			{
				dma_store();
				dma_increment();
			}
			break;

		case PIO_PDR2:
			dsp32.pdr2 = (dsp32.pdr2 & mask) | data;
			break;

		case PIO_EMR:
			dsp32.emr = (dsp32.emr & mask) | data;
			break;

		case PIO_ESR:
			dsp32.esr = (dsp32.esr & mask) | data;
			break;

		case PIO_PCR:
			mask |= 0x0060;
			data &= ~mask;
			update_pcr((dsp32.pcr & mask) | data);
			break;

		case PIO_PIR:
			dsp32.pir = (dsp32.pir & mask) | data;

			/* set PIF on upper half */
			if (!(mask & 0xff00))
				update_pcr(dsp32.pcr | PCR_PIFs);
			break;

		/* error case */
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "dsp32_pio_w called on invalid register %d\n", reg);
			break;
	}

	cpuintrf_pop_context();
}



/*###################################################################################################
**	PARALLEL INTERFACE READS
**#################################################################################################*/

int dsp32c_pio_r(int cpunum, int reg)
{
	data16_t mask, result = 0xffff;
	UINT8 mode, shift = 0;

	cpuintrf_push_context(cpunum);

	/* look up register and mask */
	mode = ((dsp32.pcr >> 8) & 2) | ((dsp32.pcr >> 1) & 1);
	reg = regmap[mode][reg];
	mask = reg >> 8;
	if (mask == 0x00ff) mask = 0xff00, shift = 8;
	reg &= 0xff;

	/* switch off the register */
	switch (reg)
	{
		case PIO_PAR:
			result = dsp32.par | 1;
			break;

		case PIO_PARE:
			result = dsp32.pare;
			break;

		case PIO_PDR:
			result = dsp32.pdr;

			/* trigger an increment on the lower half */
			if (shift != 8)
				dma_increment();

			/* trigger a fetch on the upper half */
			if (!(mask & 0xff00))
				dma_load();
			break;

		case PIO_PDR2:
			result = dsp32.pdr2;
			break;

		case PIO_EMR:
			result = dsp32.emr;
			break;

		case PIO_ESR:
			result = dsp32.esr;
			break;

		case PIO_PCR:
			result = dsp32.pcr;
			break;

		case PIO_PIR:
			if (!(mask & 0xff00))
				update_pcr(dsp32.pcr & ~PCR_PIFs);	/* clear PIFs */
			result = dsp32.pir;
			break;

		/* error case */
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "dsp32_pio_w called on invalid register %d\n", reg);
			break;
	}

	cpuintrf_pop_context();
	return (result >> shift) & ~mask;
}
