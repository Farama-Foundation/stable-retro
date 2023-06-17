/*###################################################################################################
**
**
**		tms32031.c
**		Core implementation for the portable TMS32C031 emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define exp _exp
#include "cpuintrf.h"
#include "mamedbg.h"
#include "tms32031.h"


#define LOG_OPCODE_USAGE	(0)


/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

enum
{
	TMR_R0 = 0,
	TMR_R1,
	TMR_R2,
	TMR_R3,
	TMR_R4,
	TMR_R5,
	TMR_R6,
	TMR_R7,
	TMR_AR0,
	TMR_AR1,
	TMR_AR2,
	TMR_AR3,
	TMR_AR4,
	TMR_AR5,
	TMR_AR6,
	TMR_AR7,
	TMR_DP,
	TMR_IR0,
	TMR_IR1,
	TMR_BK,
	TMR_SP,
	TMR_ST,
	TMR_IE,
	TMR_IF,
	TMR_IOF,
	TMR_RS,
	TMR_RE,
	TMR_RC,
	TMR_R8,		/* 3204x only */
	TMR_R9,		/* 3204x only */
	TMR_R10,	/* 3204x only */
	TMR_R11,	/* 3204x only */
	TMR_TEMP1,	/* used by the interpreter */
	TMR_TEMP2,	/* used by the interpreter */
	TMR_TEMP3	/* used by the interpreter */
};

#define CFLAG		0x0001
#define VFLAG		0x0002
#define ZFLAG		0x0004
#define NFLAG		0x0008
#define UFFLAG		0x0010
#define LVFLAG		0x0020
#define LUFFLAG		0x0040
#define OVMFLAG		0x0080
#define RMFLAG		0x0100
#define CFFLAG		0x0400
#define CEFLAG		0x0800
#define CCFLAG		0x1000
#define GIEFLAG		0x2000

#define IREG(rnum)			(tms32031.r[rnum].i32[0])



/*###################################################################################################
**	STRUCTURES & TYPEDEFS
**#################################################################################################*/

union genreg
{
	UINT32		i32[2];
	UINT16		i16[4];
	UINT8		i8[8];
};

/* TMS34031 Registers */
typedef struct
{
	/* core registers */
	UINT32			pc;
	union genreg	r[36];
	UINT32			bkmask;

	/* internal stuff */
	UINT32			ppc;
	UINT32			op;
	UINT8			delayed;
	UINT8			irq_pending;
	UINT8			mcu_mode;
	int				interrupt_cycles;

	void			(*xf0_w)(UINT8 val);
	void			(*xf1_w)(UINT8 val);
} tms32031_regs;



/*###################################################################################################
**	PROTOTYPES
**#################################################################################################*/

static void trap(int trapnum);
static UINT32 boot_loader(UINT32 boot_rom_addr);



/*###################################################################################################
**	PUBLIC GLOBAL VARIABLES
**#################################################################################################*/

int	tms32031_icount=50000;



/*###################################################################################################
**	PRIVATE GLOBAL VARIABLES
**#################################################################################################*/

static tms32031_regs tms32031;



/*###################################################################################################
**	MEMORY ACCESSORS
**#################################################################################################*/

#define ROPCODE(pc)		cpu_readop32((pc) * 4)
#define OP				tms32031.op

#define RMEM(addr)		cpu_readmem26ledw_dword(((addr) & 0xffffff) * 4)
#define WMEM(addr,data)	cpu_writemem26ledw_dword(((addr) & 0xffffff) * 4, data)
#define UPDATEPC(addr)	cpu_setopbase26ledw(((addr) & 0xffffff) * 4)



/*###################################################################################################
**	HELPER MACROS
**#################################################################################################*/

#define MANTISSA(r)			((INT32)(r)->i32[0])
#define EXPONENT(r)			((INT8)(r)->i32[1])
#define SET_MANTISSA(r,v)	((r)->i32[0] = (v))
#define SET_EXPONENT(r,v)	((r)->i32[1] = (v))

typedef union int_double
{
	double d;
	float f[2];
	UINT32 i[2];
} int_double;


#if 0
static float dsp_to_float(union genreg *fp)
{
	int_double id;

	if (MANTISSA(fp) == 0 && EXPONENT(fp) == -128)
		return 0;
	else if (MANTISSA(fp) >= 0)
	{
		int exponent = (EXPONENT(fp) + 127) << 23;
		id.i[0] = exponent + (MANTISSA(fp) >> 8);
	}
	else
	{
		int exponent = (EXPONENT(fp) + 127) << 23;
		INT32 man = -MANTISSA(fp);
		id.i[0] = 0x80000000 + exponent + ((man >> 8) & 0x00ffffff);
	}
	return id.f[0];
}
#endif


static double dsp_to_double(union genreg *fp)
{
	int_double id;

	if (MANTISSA(fp) == 0 && EXPONENT(fp) == -128)
		return 0;
	else if (MANTISSA(fp) >= 0)
	{
		int exponent = (EXPONENT(fp) + 1023) << 20;
		id.i[BYTE_XOR_BE(0)] = exponent + (MANTISSA(fp) >> 11);
		id.i[BYTE_XOR_BE(1)] = (MANTISSA(fp) << 21) & 0xffe00000;
	}
	else
	{
		int exponent = (EXPONENT(fp) + 1023) << 20;
		INT32 man = -MANTISSA(fp);
		id.i[BYTE_XOR_BE(0)] = 0x80000000 + exponent + ((man >> 11) & 0x001fffff);
		id.i[BYTE_XOR_BE(1)] = (man << 21) & 0xffe00000;
	}
	return id.d;
}


static void double_to_dsp(double val, union genreg *result)
{
	int mantissa, exponent;
	int_double id;
	id.d = val;

	mantissa = ((id.i[BYTE_XOR_BE(0)] & 0x000fffff) << 11) | ((id.i[BYTE_XOR_BE(1)] & 0xffe00000) >> 21);
	exponent = ((id.i[BYTE_XOR_BE(0)] & 0x7ff00000) >> 20) - 1023;
	if (exponent < -128)
	{
		SET_MANTISSA(result, 0);
		SET_EXPONENT(result, -128);
	}
	else if (exponent > 127)
	{
		if ((INT32)id.i[BYTE_XOR_BE(0)] >= 0)
			SET_MANTISSA(result, 0x7fffffff);
		else
			SET_MANTISSA(result, 0x80000001);
		SET_EXPONENT(result, 127);
	}
	else if ((INT32)id.i[BYTE_XOR_BE(0)] >= 0)
	{
		SET_MANTISSA(result, mantissa);
		SET_EXPONENT(result, exponent);
	}
	else if (mantissa != 0)
	{
		SET_MANTISSA(result, 0x80000000 | -mantissa);
		SET_EXPONENT(result, exponent);
	}
	else
	{
		SET_MANTISSA(result, 0x80000000);
		SET_EXPONENT(result, exponent - 1);
	}
}



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
	int validints = IREG(TMR_IF) & IREG(TMR_IE) & 0x07ff;
	if (validints && (IREG(TMR_ST) & GIEFLAG))
	{
		int whichtrap = 0;
		int i;

		for (i = 0; i < 11; i++)
			if (validints & (1 << i))
			{
				whichtrap = i + 1;
				break;
			}

		if (whichtrap)
		{
			if (!tms32031.delayed)
			{
				trap(whichtrap);

				/* for internal sources, clear the interrupt when taken */
				if (whichtrap > 4)
					IREG(TMR_IF) &= ~(1 << (whichtrap - 1));
			}
			else
				tms32031.irq_pending = 1;
		}
	}
}


void tms32031_set_irq_line(int irqline, int state)
{
	if (irqline < 11)
	{
	    /* update the state */
	    if (state == ASSERT_LINE)
			IREG(TMR_IF) |= 1 << irqline;
		else
			IREG(TMR_IF) &= ~(1 << irqline);

		/* check for IRQs */
	    if (state != CLEAR_LINE)
	    	check_irqs();
	}
}


void tms32031_set_irq_callback(int (*callback)(int irqline))
{
	/* finish me! */
}



/*###################################################################################################
**	CONTEXT SWITCHING
**#################################################################################################*/

unsigned tms32031_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(tms32031_regs *)dst = tms32031;

	/* return the context size */
	return sizeof(tms32031_regs);
}


void tms32031_set_context(void *src)
{
	/* copy the context */
	if (src)
		tms32031 = *(tms32031_regs *)src;
	UPDATEPC(tms32031.pc);

	/* check for IRQs */
	check_irqs();
}



/*###################################################################################################
**	INITIALIZATION AND SHUTDOWN
**#################################################################################################*/

void tms32031_init(void)
{
}

void tms32031_reset(void *param)
{
	struct tms32031_config *config = param;

	/* if we have a config struct, get the boot ROM address */
	if (config && config->bootoffset)
	{
		tms32031.mcu_mode = 1;
		tms32031.pc = boot_loader(config->bootoffset);
	}
	else
	{
		tms32031.mcu_mode = 0;
		tms32031.pc = RMEM(0);
	}

	/* copy in the xf write routines */
	if (config)
	{
		tms32031.xf0_w = config->xf0_w;
		tms32031.xf1_w = config->xf1_w;
	}

	/* reset some registers */
	IREG(TMR_IE) = 0;
	IREG(TMR_IF) = 0;
	IREG(TMR_ST) = 0;
	IREG(TMR_IOF) = 0;

	/* reset internal stuff */
	tms32031.delayed = tms32031.irq_pending = 0;
}


#if (LOG_OPCODE_USAGE)
static UINT32 hits[0x200*4];
#endif

void tms32031_exit(void)
{
#if (LOG_OPCODE_USAGE)
	int i;
	for (i = 0; i < 0x200*4; i++)
		if (hits[i])
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%10d - %03X.%X\n", hits[i], i / 4, i % 4);
#endif
}



/*###################################################################################################
**	CORE OPCODES
**#################################################################################################*/

#include "32031ops.c"



/*###################################################################################################
**	CORE EXECUTION LOOP
**#################################################################################################*/

int tms32031_execute(int cycles)
{
	/* count cycles and interrupt cycles */
	tms32031_icount = cycles;
	tms32031_icount -= tms32031.interrupt_cycles;
	tms32031.interrupt_cycles = 0;

	/* check IRQs up front */
	check_irqs();

	while (tms32031_icount > 0)
	{
		if ((IREG(TMR_ST) & RMFLAG) && tms32031.pc == IREG(TMR_RE))
		{
			execute_one();

			if ((INT32)--IREG(TMR_RC) >= 0)
				tms32031.pc = IREG(TMR_RS);
			else
			{
				IREG(TMR_ST) &= ~RMFLAG;
				if (tms32031.delayed)
				{
					tms32031.delayed = 0;
					if (tms32031.irq_pending)
					{
						tms32031.irq_pending = 0;
						check_irqs();
					}
				}
			}
			continue;
		}

		execute_one();
	}

	tms32031_icount -= tms32031.interrupt_cycles;
	tms32031.interrupt_cycles = 0;

	return cycles - tms32031_icount;
}



/*###################################################################################################
**	REGISTER SNOOP
**#################################################################################################*/

unsigned tms32031_get_reg(int regnum)
{
	float temp;

	switch (regnum)
	{
		case REG_PC:		return tms32031.pc;

		case TMS32031_R0:	return IREG(TMR_R0);
		case TMS32031_R1:	return IREG(TMR_R1);
		case TMS32031_R2:	return IREG(TMR_R2);
		case TMS32031_R3:	return IREG(TMR_R3);
		case TMS32031_R4:	return IREG(TMR_R4);
		case TMS32031_R5:	return IREG(TMR_R5);
		case TMS32031_R6:	return IREG(TMR_R6);
		case TMS32031_R7:	return IREG(TMR_R7);
		case TMS32031_R0F:	temp = dsp_to_double(&tms32031.r[TMR_R0]); return *(UINT32 *)&temp;
		case TMS32031_R1F:	temp = dsp_to_double(&tms32031.r[TMR_R1]); return *(UINT32 *)&temp;
		case TMS32031_R2F:	temp = dsp_to_double(&tms32031.r[TMR_R2]); return *(UINT32 *)&temp;
		case TMS32031_R3F:	temp = dsp_to_double(&tms32031.r[TMR_R3]); return *(UINT32 *)&temp;
		case TMS32031_R4F:	temp = dsp_to_double(&tms32031.r[TMR_R4]); return *(UINT32 *)&temp;
		case TMS32031_R5F:	temp = dsp_to_double(&tms32031.r[TMR_R5]); return *(UINT32 *)&temp;
		case TMS32031_R6F:	temp = dsp_to_double(&tms32031.r[TMR_R6]); return *(UINT32 *)&temp;
		case TMS32031_R7F:	temp = dsp_to_double(&tms32031.r[TMR_R7]); return *(UINT32 *)&temp;
		case TMS32031_AR0:	return IREG(TMR_AR0);
		case TMS32031_AR1:	return IREG(TMR_AR1);
		case TMS32031_AR2:	return IREG(TMR_AR2);
		case TMS32031_AR3:	return IREG(TMR_AR3);
		case TMS32031_AR4:	return IREG(TMR_AR4);
		case TMS32031_AR5:	return IREG(TMR_AR5);
		case TMS32031_AR6:	return IREG(TMR_AR6);
		case TMS32031_AR7:	return IREG(TMR_AR7);
		case TMS32031_DP:	return IREG(TMR_DP);
		case TMS32031_IR0:	return IREG(TMR_IR0);
		case TMS32031_IR1:	return IREG(TMR_IR1);
		case TMS32031_BK:	return IREG(TMR_BK);
		case REG_SP:
		case TMS32031_SP:	return IREG(TMR_SP);
		case TMS32031_ST:	return IREG(TMR_ST);
		case TMS32031_IE:	return IREG(TMR_IE);
		case TMS32031_IF:	return IREG(TMR_IF);
		case TMS32031_IOF:	return IREG(TMR_IOF);
		case TMS32031_RS:	return IREG(TMR_RS);
		case TMS32031_RE:	return IREG(TMR_RE);
		case TMS32031_RC:	return IREG(TMR_RC);

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

void tms32031_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:		tms32031.pc = val; 				break;

		case TMS32031_R0:	IREG(TMR_R0) = val; 		break;
		case TMS32031_R1:	IREG(TMR_R1) = val; 		break;
		case TMS32031_R2:	IREG(TMR_R2) = val; 		break;
		case TMS32031_R3:	IREG(TMR_R3) = val; 		break;
		case TMS32031_R4:	IREG(TMR_R4) = val; 		break;
		case TMS32031_R5:	IREG(TMR_R5) = val; 		break;
		case TMS32031_R6:	IREG(TMR_R6) = val; 		break;
		case TMS32031_R7:	IREG(TMR_R7) = val; 		break;
		case TMS32031_R0F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R0]);	break;
		case TMS32031_R1F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R1]);	break;
		case TMS32031_R2F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R2]);	break;
		case TMS32031_R3F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R3]);	break;
		case TMS32031_R4F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R4]);	break;
		case TMS32031_R5F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R5]);	break;
		case TMS32031_R6F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R6]);	break;
		case TMS32031_R7F:	double_to_dsp(*(float *)&val, &tms32031.r[TMR_R7]);	break;
		case TMS32031_AR0:	IREG(TMR_AR0) = val; 		break;
		case TMS32031_AR1:	IREG(TMR_AR1) = val; 		break;
		case TMS32031_AR2:	IREG(TMR_AR2) = val; 		break;
		case TMS32031_AR3:	IREG(TMR_AR3) = val; 		break;
		case TMS32031_AR4:	IREG(TMR_AR4) = val; 		break;
		case TMS32031_AR5:	IREG(TMR_AR5) = val; 		break;
		case TMS32031_AR6:	IREG(TMR_AR6) = val; 		break;
		case TMS32031_AR7:	IREG(TMR_AR7) = val; 		break;
		case TMS32031_DP:	IREG(TMR_DP) = val; 		break;
		case TMS32031_IR0:	IREG(TMR_IR0) = val; 		break;
		case TMS32031_IR1:	IREG(TMR_IR1) = val; 		break;
		case TMS32031_BK:	IREG(TMR_BK) = val; 		break;
		case REG_SP:
		case TMS32031_SP:	IREG(TMR_SP) = val; 		break;
		case TMS32031_ST:	IREG(TMR_ST) = val; 		break;
		case TMS32031_IE:	IREG(TMR_IE) = val; 		break;
		case TMS32031_IF:	IREG(TMR_IF) = val; 		break;
		case TMS32031_IOF:	IREG(TMR_IOF) = val; 		break;
		case TMS32031_RS:	IREG(TMR_RS) = val; 		break;
		case TMS32031_RE:	IREG(TMR_RE) = val; 		break;
		case TMS32031_RC:	IREG(TMR_RC) = val; 		break;

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

static UINT8 tms32031_reg_layout[] =
{
	TMS32031_PC,		TMS32031_SP,		-1,
	TMS32031_R0,		TMS32031_R0F,		-1,
	TMS32031_R1,		TMS32031_R1F,		-1,
	TMS32031_R2,		TMS32031_R2F,		-1,
	TMS32031_R3,		TMS32031_R3F,		-1,
	TMS32031_R4,		TMS32031_R4F,		-1,
	TMS32031_R5,		TMS32031_R5F,		-1,
	TMS32031_R6,		TMS32031_R6F,		-1,
	TMS32031_R7,		TMS32031_R7F,		-1,
	TMS32031_AR0,		TMS32031_IR0,		-1,
	TMS32031_AR1,		TMS32031_IR1,		-1,
	TMS32031_AR2,		TMS32031_RC,		-1,
	TMS32031_AR3,		TMS32031_RS,		-1,
	TMS32031_AR4,		TMS32031_RE,		-1,
	TMS32031_AR5,		TMS32031_BK,		-1,
	TMS32031_AR6,		TMS32031_ST,		-1,
	TMS32031_AR7,		TMS32031_IE,		-1,
	TMS32031_DP,		TMS32031_IF,		-1,
	TMS32031_ST,		TMS32031_IOF, 		0
};

static UINT8 tms32031_win_layout[] =
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

const char *tms32031_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	tms32031_regs *r = context;

	which = ( which + 1 ) % 16;
    buffer[which][0] = '\0';

	if (!context)
		r = &tms32031;

    switch( regnum )
	{
		case CPU_INFO_REG+TMS32031_PC:  	sprintf(buffer[which], "PC: %08X", r->pc); break;

		case CPU_INFO_REG+TMS32031_R0:		sprintf(buffer[which], " R0:%08X", r->r[TMR_R0].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R1:		sprintf(buffer[which], " R1:%08X", r->r[TMR_R1].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R2:		sprintf(buffer[which], " R2:%08X", r->r[TMR_R2].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R3:		sprintf(buffer[which], " R3:%08X", r->r[TMR_R3].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R4:		sprintf(buffer[which], " R4:%08X", r->r[TMR_R4].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R5:		sprintf(buffer[which], " R5:%08X", r->r[TMR_R5].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R6:		sprintf(buffer[which], " R6:%08X", r->r[TMR_R6].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R7:		sprintf(buffer[which], " R7:%08X", r->r[TMR_R7].i32[0]); break;
		case CPU_INFO_REG+TMS32031_R0F:		sprintf(buffer[which], "R0F:%8g", dsp_to_double(&r->r[TMR_R0])); break;
		case CPU_INFO_REG+TMS32031_R1F:		sprintf(buffer[which], "R1F:%8g", dsp_to_double(&r->r[TMR_R1])); break;
		case CPU_INFO_REG+TMS32031_R2F:		sprintf(buffer[which], "R2F:%8g", dsp_to_double(&r->r[TMR_R2])); break;
		case CPU_INFO_REG+TMS32031_R3F:		sprintf(buffer[which], "R3F:%8g", dsp_to_double(&r->r[TMR_R3])); break;
		case CPU_INFO_REG+TMS32031_R4F:		sprintf(buffer[which], "R4F:%8g", dsp_to_double(&r->r[TMR_R4])); break;
		case CPU_INFO_REG+TMS32031_R5F:		sprintf(buffer[which], "R5F:%8g", dsp_to_double(&r->r[TMR_R5])); break;
		case CPU_INFO_REG+TMS32031_R6F:		sprintf(buffer[which], "R6F:%8g", dsp_to_double(&r->r[TMR_R6])); break;
		case CPU_INFO_REG+TMS32031_R7F:		sprintf(buffer[which], "R7F:%8g", dsp_to_double(&r->r[TMR_R7])); break;
		case CPU_INFO_REG+TMS32031_AR0:		sprintf(buffer[which], "AR0:%08X", r->r[TMR_AR0].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR1:		sprintf(buffer[which], "AR1:%08X", r->r[TMR_AR1].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR2:		sprintf(buffer[which], "AR2:%08X", r->r[TMR_AR2].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR3:		sprintf(buffer[which], "AR3:%08X", r->r[TMR_AR3].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR4:		sprintf(buffer[which], "AR4:%08X", r->r[TMR_AR4].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR5:		sprintf(buffer[which], "AR5:%08X", r->r[TMR_AR5].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR6:		sprintf(buffer[which], "AR6:%08X", r->r[TMR_AR6].i32[0]); break;
		case CPU_INFO_REG+TMS32031_AR7:		sprintf(buffer[which], "AR7:%08X", r->r[TMR_AR7].i32[0]); break;
		case CPU_INFO_REG+TMS32031_DP:		sprintf(buffer[which], " DP:%02X", r->r[TMR_DP].i8[0]); break;
		case CPU_INFO_REG+TMS32031_IR0:		sprintf(buffer[which], "IR0:%08X", r->r[TMR_IR0].i32[0]); break;
		case CPU_INFO_REG+TMS32031_IR1:		sprintf(buffer[which], "IR1:%08X", r->r[TMR_IR1].i32[0]); break;
		case CPU_INFO_REG+TMS32031_BK:		sprintf(buffer[which], " BK:%08X", r->r[TMR_BK].i32[0]); break;
		case CPU_INFO_REG+TMS32031_SP:		sprintf(buffer[which], " SP:%08X", r->r[TMR_SP].i32[0]); break;
		case CPU_INFO_REG+TMS32031_ST:		sprintf(buffer[which], " ST:%08X", r->r[TMR_ST].i32[0]); break;
		case CPU_INFO_REG+TMS32031_IE:		sprintf(buffer[which], " IE:%08X", r->r[TMR_IE].i32[0]); break;
		case CPU_INFO_REG+TMS32031_IF:		sprintf(buffer[which], " IF:%08X", r->r[TMR_IF].i32[0]); break;
		case CPU_INFO_REG+TMS32031_IOF:		sprintf(buffer[which], "IOF:%08X", r->r[TMR_IOF].i32[0]); break;
		case CPU_INFO_REG+TMS32031_RS:		sprintf(buffer[which], " RS:%08X", r->r[TMR_RS].i32[0]); break;
		case CPU_INFO_REG+TMS32031_RE:		sprintf(buffer[which], " RE:%08X", r->r[TMR_RE].i32[0]); break;
		case CPU_INFO_REG+TMS32031_RC:		sprintf(buffer[which], " RC:%08X", r->r[TMR_RC].i32[0]); break;

		case CPU_INFO_FLAGS:
		{
			UINT32 temp = r->r[TMR_ST].i32[0];
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				(temp & 0x80) ? 'O':'.',
				(temp & 0x40) ? 'U':'.',
                (temp & 0x20) ? 'V':'.',
                (temp & 0x10) ? 'u':'.',
                (temp & 0x08) ? 'n':'.',
                (temp & 0x04) ? 'z':'.',
                (temp & 0x02) ? 'v':'.',
                (temp & 0x01) ? 'c':'.');
            break;
        }

		case CPU_INFO_NAME: return "TMS32031";
		case CPU_INFO_FAMILY: return "TMS32031";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Aaron Giles 2002";
		case CPU_INFO_REG_LAYOUT: return (const char *)tms32031_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)tms32031_win_layout;
		case CPU_INFO_REG+10000: return "         ";
    }
	return buffer[which];
}



/*###################################################################################################
**	DISASSEMBLY HOOK
**#################################################################################################*/

unsigned tms32031_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasm_tms32031(char *, unsigned);
    return dasm_tms32031(buffer, pc);
#else
	sprintf(buffer, "$%04X", ROPCODE(pc));
	return 4;
#endif
}



/*###################################################################################################
**	BOOT LOADER
**#################################################################################################*/

static UINT32 boot_loader(UINT32 boot_rom_addr)
{
	UINT32 bits, control, advance;
	UINT32 start_offset = 0;
	int first = 1, i;

	/* read the size of the data */
	bits = RMEM(boot_rom_addr);
	if (bits != 8 && bits != 16 && bits != 32)
		return 0;
	advance = 32 / bits;
	boot_rom_addr += advance;

	/* read the control register */
	control = RMEM(boot_rom_addr++);
	for (i = 1; i < advance; i++)
		control |= RMEM(boot_rom_addr++) << (bits * i);

	/* now parse the data */
	while (1)
	{
		UINT32 offs, len;

		/* read the length of this section */
		len = RMEM(boot_rom_addr++);
		for (i = 1; i < advance; i++)
			len |= RMEM(boot_rom_addr++) << (bits * i);

		/* stop at 0 */
		if (len == 0)
			return start_offset;

		/* read the destination offset of this section */
		offs = RMEM(boot_rom_addr++);
		for (i = 1; i < advance; i++)
			offs |= RMEM(boot_rom_addr++) << (bits * i);

		/* if this is the first block, that's where we boot to */
		if (first)
		{
			start_offset = offs;
			first = 0;
		}

		/* now copy the data */
		while (len--)
		{
			UINT32 data;

			/* extract the 32-bit word */
			data = RMEM(boot_rom_addr++);
			for (i = 1; i < advance; i++)
				data |= RMEM(boot_rom_addr++) << (bits * i);

			/* write it out */
			WMEM(offs++, data);
		}
	}

	/* keep the compiler happy */
	return 0;
}
