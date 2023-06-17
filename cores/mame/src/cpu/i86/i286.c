/****************************************************************************
*			  real mode i286 emulator v1.4 by Fabrice Frances				*
*				(initial work based on David Hedley's pcemu)                *
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "host.h"
#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"
#include "mame.h"
#include "state.h"


/* All post-i286 CPUs have a 16MB address space */
#define AMASK	I.amask


#define i86_ICount i286_ICount

#include "i286.h"
#include "i286intf.h"


static UINT8 i286_reg_layout[] = {
	I286_FLAGS,
	I286_MSW,
	I286_TR,
	I286_TR_2,
	I286_GDTR,
	I286_GDTR_2,
	-1,
	I286_AX,
	I286_BP,
	I286_LDTR,
	I286_LDTR_2,
	I286_IDTR,
	I286_IDTR_2,
	-1,
	I286_BX,
	I286_SP,
	I286_SS,
	I286_SS_2,
	I286_IRQ_STATE,
	I286_VECTOR,
	-1,
	I286_CX,
	I286_IP,
	I286_CS,
	I286_CS_2,
	I286_NMI_STATE,
	-1,
	I286_DX,
	I286_SI,
	I286_DS,
	I286_DS_2,
	-1,
	I286_EMPTY,
	I286_DI,
	I286_ES,
	I286_ES_2,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 i286_win_layout[] = {
     0, 0,80, 6,    /* register window (top rows) */
	 0, 7,40,15,	/* disassembler window (left colums) */
	41, 7,39, 7,	/* memory #1 window (right, upper middle) */
	41,15,39, 7,	/* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};


#include "i86time.c"

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/
/* I86 registers */
typedef union
{                   /* eight general registers */
    UINT16 w[8];    /* viewed as 16 bits registers */
    UINT8  b[16];   /* or as 8 bit registers */
} i286basicregs;

typedef struct
{
    i286basicregs regs;
	int 	amask;			/* address mask */
    UINT32  pc;
    UINT32  prevpc;
	UINT16	flags;
	UINT16	msw;
	UINT32	base[4];
	UINT16	sregs[4];
	UINT16	limit[4];
	UINT8 rights[4];
	struct {
		UINT32 base;
		UINT16 limit;
	} gdtr, idtr;
	struct {
		UINT16 sel;
		UINT32 base;
		UINT16 limit;
		UINT8 rights;
	} ldtr, tr;
    int     (*irq_callback)(int irqline);
    int     AuxVal, OverVal, SignVal, ZeroVal, CarryVal, DirVal; /* 0 or non-0 valued flags */
    UINT8	ParityVal;
	UINT8	TF, IF; 	/* 0 or 1 valued flags */
	UINT8	int_vector;
	INT8	nmi_state;
	INT8	irq_state;
	int 	extra_cycles;       /* extra cycles for interrupts */
} i286_Regs;

int i286_ICount;

static i286_Regs I;
static unsigned prefix_base;	/* base address of the latest prefix segment */
static char seg_prefix;         /* prefix segment indicator */

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

static UINT8 parity_table[256];

static struct i86_timing cycles;

/***************************************************************************/

#define I286
#define PREFIX(fname) i286##fname
#define PREFIX86(fname) i286##fname
#define PREFIX186(fname) i286##fname
#define PREFIX286(fname) i286##fname

#include "ea.h"
#include "modrm.h"
#include "instr86.h"
#include "instr186.h"
#include "instr286.h"
#include "table286.h"
#include "instr86.c"
#include "instr186.c"
#include "instr286.c"

static void i286_urinit(void)
{
	unsigned int i,j,c;
	BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

	for (i = 0;i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1) c++;

		parity_table[i] = !(c & 1);
	}

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS)( i & 7 );
		Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
	}
}

void i286_set_address_mask(offs_t mask)
{
	I.amask=mask;
}

void i286_reset (void *param)
{
	static int urinit=1;

	/* in my docu not all registers are initialized! */
	/*memset( &I, 0, sizeof(I) );*/

	if (urinit) {
		i286_urinit();
		urinit=0;

		/* this function seams to be called as a result of
		   cpu_set_reset_line */
		/* If a reset parameter is given, take it as pointer to an address mask */
		if( param )
			I.amask = *(unsigned*)param;
		else
			I.amask = 0x00ffff;
	}

	I.sregs[CS] = 0xf000;
	I.base[CS] = 0xff0000;
	/* temporary, until I have the right reset vector working */
	I.base[CS] = I.sregs[CS] << 4;
	I.pc = 0xffff0;
	I.limit[CS]=I.limit[SS]=I.limit[DS]=I.limit[ES]=0xffff;
	I.sregs[DS]=I.sregs[SS]=I.sregs[ES]=0;
	I.base[DS]=I.base[SS]=I.base[ES]=0;
	I.msw=0xfff0;
	I.flags=2;
	ExpandFlags(I.flags);
	I.idtr.base=0;I.idtr.limit=0x3ff;

	CHANGE_PC(I.pc);

}

void i286_exit (void)
{
	/* nothing to do ? */
}

/****************************************************************************/

/* ASG 971222 -- added these interface functions */

unsigned i286_get_context(void *dst)
{
	if( dst )
		*(i286_Regs*)dst = I;
	 return sizeof(i286_Regs);
}

void i286_set_context(void *src)
{
	if( src )
	{
		I = *(i286_Regs*)src;
		if (PM) {

		} else {
			I.base[CS] = SegBase(CS);
			I.base[DS] = SegBase(DS);
			I.base[ES] = SegBase(ES);
			I.base[SS] = SegBase(SS);
		}
		CHANGE_PC(I.pc);
	}
}

unsigned i286_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return I.pc;
		case I286_IP: return I.pc - I.base[CS];
		case REG_SP: return I.base[SS] + I.regs.w[SP];
		case I286_SP: return I.regs.w[SP];
		case I286_FLAGS: CompressFlags(); return I.flags;
		case I286_AX: return I.regs.w[AX];
		case I286_CX: return I.regs.w[CX];
		case I286_DX: return I.regs.w[DX];
		case I286_BX: return I.regs.w[BX];
		case I286_BP: return I.regs.w[BP];
		case I286_SI: return I.regs.w[SI];
		case I286_DI: return I.regs.w[DI];
		case I286_ES: return I.sregs[ES];
		case I286_CS: return I.sregs[CS];
		case I286_SS: return I.sregs[SS];
		case I286_DS: return I.sregs[DS];
		case I286_VECTOR: return I.int_vector;
		case I286_PENDING: return I.irq_state;
		case I286_NMI_STATE: return I.nmi_state;
		case I286_IRQ_STATE: return I.irq_state;
		case REG_PREVIOUSPC: return I.prevpc;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP]) & I.amask) + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < I.amask )
					return cpu_readmem24( offset ) | ( cpu_readmem24( offset + 1) << 8 );
			}
	}
	return 0;
}

void i286_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:
			if (PM) {
			} else {
				if (val - I.base[CS] >= 0x10000)
				{
					I.base[CS] = val & 0xffff0;
					I.sregs[CS] = I.base[CS] >> 4;
				}
				I.pc = val;
			}
			break;
		case I286_IP: I.pc = I.base[CS] + val; break;
		case REG_SP:
			if (PM) {
			} else {
				if( val - I.base[SS] < 0x10000 )
				{
					I.regs.w[SP] = val - I.base[SS];
				}
				else
				{
					I.base[SS] = val & 0xffff0;
					I.sregs[SS] = I.base[SS] >> 4;
					I.regs.w[SP] = val & 0x0000f;
				}
			}
			break;
		case I286_SP: I.regs.w[SP] = val; break;
		case I286_FLAGS: I.flags = val; ExpandFlags(val); break;
		case I286_AX: I.regs.w[AX] = val; break;
		case I286_CX: I.regs.w[CX] = val; break;
		case I286_DX: I.regs.w[DX] = val; break;
		case I286_BX: I.regs.w[BX] = val; break;
		case I286_BP: I.regs.w[BP] = val; break;
		case I286_SI: I.regs.w[SI] = val; break;
		case I286_DI: I.regs.w[DI] = val; break;
		case I286_ES: I.sregs[ES] = val; break;
		case I286_CS: I.sregs[CS] = val; break;
		case I286_SS: I.sregs[SS] = val; break;
		case I286_DS: I.sregs[DS] = val; break;
		case I286_VECTOR: I.int_vector = val; break;
		case I286_PENDING: /* obsolete */ break;
		case I286_NMI_STATE: i286_set_irq_line(IRQ_LINE_NMI,val); break;
		case I286_IRQ_STATE: i286_set_irq_line(0,val); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP]) & I.amask) + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < I.amask - 1 )
				{
					cpu_writemem24( offset, val & 0xff );
					cpu_writemem24( offset+1, (val >> 8) & 0xff );
				}
			}
    }
}

void i286_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if (I.nmi_state == state)
			return;
		I.nmi_state = state;

		/* on a rising edge, signal the NMI */
		if (state != CLEAR_LINE)
			PREFIX(_interrupt)(I86_NMI_INT_VECTOR);
	}
	else
	{
		I.irq_state = state;

		/* if the IF is set, signal an interrupt */
		if (state != CLEAR_LINE && I.IF)
			PREFIX(_interrupt)(-1);
	}
}

void i286_set_irq_callback(int (*callback)(int))
{
	I.irq_callback = callback;
}

int i286_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 80286)
		cycles = i286_cycles;

	/* adjust for any interrupts that came in */
	i286_ICount = num_cycles;
	i286_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while(i286_ICount>0)
	{

		log_cb(RETRO_LOG_DEBUG, LOGPRE "[%04x:%04x]=%02x\tF:%04x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x %d%d%d%d%d%d%d%d%d\n",I.sregs[CS],I.pc - I.base[CS],ReadByte(I.pc),I.flags,I.regs.w[AX],I.regs.w[BX],I.regs.w[CX],I.regs.w[DX], I.AuxVal?1:0, I.OverVal?1:0, I.SignVal?1:0, I.ZeroVal?1:0, I.CarryVal?1:0, I.ParityVal?1:0,I.TF, I.IF, I.DirVal<0?1:0);

		CALL_MAME_DEBUG;

		seg_prefix=FALSE;
		I.prevpc = I.pc;

		TABLE286 /* call instruction*/
    }

	/* adjust for any interrupts that came in */
	i286_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i286_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i286_info(void *context, int regnum)
{
	static char buffer[32][63+1];
	static int which = 0;
	i286_Regs *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &I;

	switch( regnum )
	{
	case CPU_INFO_REG+I286_IP: sprintf(buffer[which], "IP:%04X", r->pc - r->base[CS]); break;
	case CPU_INFO_REG+I286_SP: sprintf(buffer[which], "SP:%04X", r->regs.w[SP]); break;
	case CPU_INFO_REG+I286_FLAGS: sprintf(buffer[which], "F:%04X", r->flags); break;
	case CPU_INFO_REG+I286_AX: sprintf(buffer[which], "AX:%04X", r->regs.w[AX]); break;
	case CPU_INFO_REG+I286_CX: sprintf(buffer[which], "CX:%04X", r->regs.w[CX]); break;
	case CPU_INFO_REG+I286_DX: sprintf(buffer[which], "DX:%04X", r->regs.w[DX]); break;
	case CPU_INFO_REG+I286_BX: sprintf(buffer[which], "BX:%04X", r->regs.w[BX]); break;
	case CPU_INFO_REG+I286_BP: sprintf(buffer[which], "BP:%04X", r->regs.w[BP]); break;
	case CPU_INFO_REG+I286_SI: sprintf(buffer[which], "SI:%04X", r->regs.w[SI]); break;
	case CPU_INFO_REG+I286_DI: sprintf(buffer[which], "DI:%04X", r->regs.w[DI]); break;
	case CPU_INFO_REG+I286_ES:
		sprintf(buffer[which], "ES:  %04X %02X", r->sregs[ES], r->rights[ES]);
		break;
	case CPU_INFO_REG+I286_ES_2:
		sprintf(buffer[which],"%06X %04X", r->base[ES], r->limit[ES]);
		break;
	case CPU_INFO_REG+I286_CS:
		sprintf(buffer[which], "CS:  %04X %02X", r->sregs[CS], r->rights[CS]);
		break;
	case CPU_INFO_REG+I286_CS_2:
		sprintf(buffer[which],"%06X %04X", r->base[CS], r->limit[CS]);
		break;
	case CPU_INFO_REG+I286_SS:
		sprintf(buffer[which], "SS:  %04X %02X", r->sregs[SS], r->rights[SS]);
		break;
	case CPU_INFO_REG+I286_SS_2:
		sprintf(buffer[which],"%06X %04X", r->base[SS], r->limit[SS]);
		break;
	case CPU_INFO_REG+I286_DS:
		sprintf(buffer[which], "DS:  %04X %02X", r->sregs[DS], r->rights[DS]);
		break;
	case CPU_INFO_REG+I286_DS_2:
		sprintf(buffer[which],"%06X %04X", r->base[DS], r->limit[DS]);
		break;
	case CPU_INFO_REG+I286_MSW: sprintf(buffer[which],"MSW:%04X", r->msw); break;
	case CPU_INFO_REG+I286_GDTR: sprintf(buffer[which],"GDTR: %06X", r->gdtr.base); break;
	case CPU_INFO_REG+I286_GDTR_2: sprintf(buffer[which],"%04X", r->gdtr.limit); break;
	case CPU_INFO_REG+I286_IDTR: sprintf(buffer[which],"IDTR: %06X", r->idtr.base); break;
	case CPU_INFO_REG+I286_IDTR_2: sprintf(buffer[which],"%04X", r->idtr.limit); break;
	case CPU_INFO_REG+I286_LDTR:
		sprintf(buffer[which],"LDTR:%04X %02X", r->ldtr.sel, r->ldtr.rights);
		break;
	case CPU_INFO_REG+I286_LDTR_2:
		sprintf(buffer[which],"%06X %04X", r->ldtr.base, r->ldtr.limit);
		break;
	case CPU_INFO_REG+I286_TR:
		sprintf(buffer[which],"TR:  %04X %02X", r->tr.sel, r->tr.rights);
		break;
	case CPU_INFO_REG+I286_TR_2:
		sprintf(buffer[which],"%06X %04X", r->tr.base, r->tr.limit);
		break;
	case CPU_INFO_REG+I286_VECTOR: sprintf(buffer[which], "V:%02X", r->int_vector); break;
	case CPU_INFO_REG+I286_PENDING: sprintf(buffer[which], "P:%X", r->irq_state); break;
	case CPU_INFO_REG+I286_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
	case CPU_INFO_REG+I286_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
	case CPU_INFO_FLAGS:
		r->flags = CompressFlags();
		sprintf(buffer[which], "%c%c %c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->flags & 0x8000 ? '?':'.',
				r->flags & 0x4000 ? '?':'.',
				((r->flags & 0x3000)>>12)+'0',
				r->flags & 0x0800 ? 'O':'.',
				r->flags & 0x0400 ? 'D':'.',
				r->flags & 0x0200 ? 'I':'.',
				r->flags & 0x0100 ? 'T':'.',
				r->flags & 0x0080 ? 'S':'.',
				r->flags & 0x0040 ? 'Z':'.',
				r->flags & 0x0020 ? '?':'.',
				r->flags & 0x0010 ? 'A':'.',
				r->flags & 0x0008 ? '?':'.',
				r->flags & 0x0004 ? 'P':'.',
				r->flags & 0x0002 ? 'N':'.',
				r->flags & 0x0001 ? 'C':'.');
		break;
	case CPU_INFO_REG+I286_EMPTY: sprintf(buffer[which]," ");break;
	case CPU_INFO_NAME: return "I80286";
	case CPU_INFO_FAMILY: return "Intel 80286";
	case CPU_INFO_VERSION: return "1.4";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Real mode i286 emulator v1.4 by Fabrice Frances\n(initial work I.based on David Hedley's pcemu)";
	case CPU_INFO_REG_LAYOUT: return (const char*)i286_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char*)i286_win_layout;
	}
	return buffer[which];
}

unsigned i286_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return DasmI286(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

void i286_init(void)
{
	int cpu = cpu_getactivecpu();
	const char *type = "I286";
	state_save_register_UINT16(type, cpu, "REGS",			I.regs.w, 8);
	state_save_register_int(   type, cpu, "AMASK",			&I.amask);
	state_save_register_UINT32(type, cpu, "PC",				&I.pc, 1);
	state_save_register_UINT32(type, cpu, "PREVPC",			&I.prevpc, 1);
	state_save_register_UINT16(type, cpu, "MSW",			&I.msw, 1);
	state_save_register_UINT32(type, cpu, "BASE",			I.base, 4);
	state_save_register_UINT16(type, cpu, "SREGS",			I.sregs, 4);
	state_save_register_UINT16(type, cpu, "LIMIT",			I.limit, 4);
	state_save_register_UINT8 (type, cpu, "RIGHTS",			I.rights, 4);
	state_save_register_UINT32(type, cpu, "GDTR_BASE",		&I.gdtr.base, 1);
	state_save_register_UINT16(type, cpu, "GDTR_LIMIT",		&I.gdtr.limit, 1);
	state_save_register_UINT32(type, cpu, "IDTR_BASE",		&I.idtr.base, 1);
	state_save_register_UINT16(type, cpu, "IDTR_LIMIT",		&I.idtr.limit, 1);
	state_save_register_UINT16(type, cpu, "LDTR_SEL",		&I.ldtr.sel, 1);
	state_save_register_UINT32(type, cpu, "LDTR_BASE",		&I.ldtr.base, 1);
	state_save_register_UINT16(type, cpu, "LDTR_LIMIT",		&I.ldtr.limit, 1);
	state_save_register_UINT8 (type, cpu, "LDTR_RIGHTS",	&I.ldtr.rights, 1);
	state_save_register_UINT16(type, cpu, "TR_SEL",			&I.tr.sel, 1);
	state_save_register_UINT32(type, cpu, "TR_BASE",		&I.tr.base, 1);
	state_save_register_UINT16(type, cpu, "TR_LIMIT",		&I.tr.limit, 1);
	state_save_register_UINT8 (type, cpu, "TR_RIGHTS",		&I.tr.rights, 1);
	state_save_register_int(   type, cpu, "AUXVAL",			&I.AuxVal);
	state_save_register_int(   type, cpu, "OVERVAL",		&I.OverVal);
	state_save_register_int(   type, cpu, "SIGNVAL",		&I.SignVal);
	state_save_register_int(   type, cpu, "ZEROVAL",		&I.ZeroVal);
	state_save_register_int(   type, cpu, "CARRYVAL",		&I.CarryVal);
	state_save_register_int(   type, cpu, "DIRVAL",			&I.DirVal);
	state_save_register_UINT8( type, cpu, "PARITYVAL",		&I.ParityVal, 1);
	state_save_register_UINT8( type, cpu, "TF",				&I.TF, 1);
	state_save_register_UINT8( type, cpu, "IF",				&I.IF, 1);
	state_save_register_UINT8( type, cpu, "INT_VECTOR",		&I.int_vector, 1);
	state_save_register_INT8(  type, cpu, "NMI_STATE",		&I.nmi_state, 1);
	state_save_register_INT8(  type, cpu, "IRQ_STATE",		&I.irq_state, 1);
	state_save_register_int(   type, cpu, "EXTRA_CYCLES",	&I.extra_cycles);
}
