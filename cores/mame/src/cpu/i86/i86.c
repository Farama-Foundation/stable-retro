/****************************************************************************
*			  real mode i286 emulator v1.4 by Fabrice Frances				*
*				(initial work based on David Hedley's pcemu)                *
****************************************************************************/
/* 26.March 2000 PeT changed set_irq_line */

#include <stdio.h>
#include <string.h>
#include "host.h"
#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"
#include "mame.h"
#include "state.h"

#include "i86.h"
#include "i86intf.h"


/* All pre-i286 CPUs have a 1MB address space */
#define AMASK	0xfffff


static UINT8 i86_reg_layout[] =
{
	I86_AX, I86_BX, I86_DS, I86_ES, I86_SS, I86_FLAGS, I86_CS, I86_VECTOR, -1,
	I86_CX, I86_DX, I86_SI, I86_DI, I86_SP, I86_BP, I86_IP,
	I86_IRQ_STATE, I86_NMI_STATE, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 i86_win_layout[] =
{
	0, 0, 80, 2,					   /* register window (top rows) */
	0, 3, 34, 19,					   /* disassembler window (left colums) */
	35, 3, 45, 9,					   /* memory #1 window (right, upper middle) */
	35, 13, 45, 9,					   /* memory #2 window (right, lower middle) */
	0, 23, 80, 1,					   /* command line window (bottom rows) */
};

/* I86 registers */
typedef union
{									   /* eight general registers */
	UINT16 w[8];					   /* viewed as 16 bits registers */
	UINT8 b[16];					   /* or as 8 bit registers */
}
i86basicregs;

typedef struct
{
	i86basicregs regs;
	UINT32 pc;
	UINT32 prevpc;
	UINT32 base[4];
	UINT16 sregs[4];
	UINT16 flags;
	int (*irq_callback) (int irqline);
	int AuxVal, OverVal, SignVal, ZeroVal, CarryVal, DirVal;		/* 0 or non-0 valued flags */
	UINT8 ParityVal;
	UINT8 TF, IF;				   /* 0 or 1 valued flags */
	UINT8 MF;						   /* V30 mode flag */
	UINT8 int_vector;
	INT8 nmi_state;
	INT8 irq_state;
	int extra_cycles;       /* extra cycles for interrupts */
}
i86_Regs;


#include "i86time.c"

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/

int i86_ICount;

static i86_Regs I;
static unsigned prefix_base;		   /* base address of the latest prefix segment */
static char seg_prefix;				   /* prefix segment indicator */

static UINT8 parity_table[256];

static struct i86_timing cycles;

/* The interrupt number of a pending external interrupt pending NMI is 2.	*/
/* For INTR interrupts, the level is caught on the bus during an INTA cycle */

#define PREFIX(name) i86##name
#define PREFIX86(name) i86##name

#define I86
#include "instr86.h"
#include "ea.h"
#include "modrm.h"
#include "table86.h"

#include "instr86.c"
#undef I86

/***************************************************************************/
static void i86_state_register(void)
{
	int cpu = cpu_getactivecpu();
	const char *type = "I86";
	state_save_register_UINT16(type, cpu, "REGS",			I.regs.w, 8);
	state_save_register_UINT32(type, cpu, "PC",				&I.pc, 1);
	state_save_register_UINT32(type, cpu, "PREVPC",			&I.prevpc, 1);
	state_save_register_UINT32(type, cpu, "BASE",			I.base, 4);
	state_save_register_UINT16(type, cpu, "SREGS",			I.sregs, 4);
	state_save_register_UINT16(type, cpu, "FLAGS",			&I.flags, 1);
	state_save_register_int(   type, cpu, "AUXVAL",			&I.AuxVal);
	state_save_register_int(   type, cpu, "OVERVAL",		&I.OverVal);
	state_save_register_int(   type, cpu, "SIGNVAL",		&I.SignVal);
	state_save_register_int(   type, cpu, "ZEROVAL",		&I.ZeroVal);
	state_save_register_int(   type, cpu, "CARRYVAL",		&I.CarryVal);
	state_save_register_int(   type, cpu, "DIRVAL",			&I.DirVal);
	state_save_register_UINT8( type, cpu, "PARITYVAL",		&I.ParityVal, 1);
	state_save_register_UINT8( type, cpu, "TF",				&I.TF, 1);
	state_save_register_UINT8( type, cpu, "IF",				&I.IF, 1);
	state_save_register_UINT8( type, cpu, "MF",				&I.MF, 1);
	state_save_register_UINT8( type, cpu, "INT_VECTOR",		&I.int_vector, 1);
	state_save_register_INT8(  type, cpu, "NMI_STATE",		&I.nmi_state, 1);
	state_save_register_INT8(  type, cpu, "IRQ_STATE",		&I.irq_state, 1);
	state_save_register_int(   type, cpu, "EXTRA_CYCLES",	&I.extra_cycles);
}

void i86_init(void)
{
	unsigned int i, j, c;
	BREGS reg_name[8] = {AL, CL, DL, BL, AH, CH, DH, BH};
	for (i = 0; i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1)
				c++;

		parity_table[i] = !(c & 1);
	}

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ((i & 0x38) >> 3);
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS) (i & 7);
		Mod_RM.RM.b[i] = (BREGS) reg_name[i & 7];
	}

	i86_state_register();
}

void i86_reset(void *param)
{
	memset(&I, 0, sizeof (I));

	I.sregs[CS] = 0xf000;
	I.base[CS] = SegBase(CS);
	I.pc = 0xffff0 & AMASK;
	ExpandFlags(I.flags);

	change_pc20(I.pc);
}

void i86_exit(void)
{
	/* nothing to do ? */
}

/* ASG 971222 -- added these interface functions */

unsigned i86_get_context(void *dst)
{
	if (dst)
		*(i86_Regs *) dst = I;
	return sizeof (i86_Regs);
}

void i86_set_context(void *src)
{
	if (src)
	{
		I = *(i86_Regs *)src;
		I.base[CS] = SegBase(CS);
		I.base[DS] = SegBase(DS);
		I.base[ES] = SegBase(ES);
		I.base[SS] = SegBase(SS);
		change_pc20(I.pc);
	}
}

unsigned i86_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:		return I.pc;
		case I86_IP:		return I.pc - I.base[CS];
		case REG_SP:		return I.base[SS] + I.regs.w[SP];
		case I86_SP:		return I.regs.w[SP];
		case I86_FLAGS: 	CompressFlags(); return I.flags;
		case I86_AX:		return I.regs.w[AX];
		case I86_CX:		return I.regs.w[CX];
		case I86_DX:		return I.regs.w[DX];
		case I86_BX:		return I.regs.w[BX];
		case I86_BP:		return I.regs.w[BP];
		case I86_SI:		return I.regs.w[SI];
		case I86_DI:		return I.regs.w[DI];
		case I86_ES:		return I.sregs[ES];
		case I86_CS:		return I.sregs[CS];
		case I86_SS:		return I.sregs[SS];
		case I86_DS:		return I.sregs[DS];
		case I86_VECTOR:	return I.int_vector;
		case I86_PENDING:	return I.irq_state;
		case I86_NMI_STATE: return I.nmi_state;
		case I86_IRQ_STATE: return I.irq_state;
		case REG_PREVIOUSPC:return I.prevpc;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP]) & AMASK) + 2 * (REG_SP_CONTENTS - regnum);

				if (offset < AMASK)
					return cpu_readmem20(offset) | (cpu_readmem20(offset + 1) << 8);
			}
	}
	return 0;
}

void i86_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
			if (val - I.base[CS] >= 0x10000)
			{
				I.base[CS] = val & 0xffff0;
				I.sregs[CS] = I.base[CS] >> 4;
			}
			I.pc = val;
			break;
		case I86_IP:		I.pc = I.base[CS] + val;	break;
		case REG_SP:
			if (val - I.base[SS] < 0x10000)
			{
				I.regs.w[SP] = val - I.base[SS];
			}
			else
			{
				I.base[SS] = val & 0xffff0;
				I.sregs[SS] = I.base[SS] >> 4;
				I.regs.w[SP] = val & 0x0000f;
			}
			break;
		case I86_SP:		I.regs.w[SP] = val; 		break;
		case I86_FLAGS: 	I.flags = val;	ExpandFlags(val); break;
		case I86_AX:		I.regs.w[AX] = val; 		break;
		case I86_CX:		I.regs.w[CX] = val; 		break;
		case I86_DX:		I.regs.w[DX] = val; 		break;
		case I86_BX:		I.regs.w[BX] = val; 		break;
		case I86_BP:		I.regs.w[BP] = val; 		break;
		case I86_SI:		I.regs.w[SI] = val; 		break;
		case I86_DI:		I.regs.w[DI] = val; 		break;
		case I86_ES:		I.sregs[ES] = val;	I.base[ES] = SegBase(ES);	break;
		case I86_CS:		I.sregs[CS] = val;	I.base[CS] = SegBase(CS);	break;
		case I86_SS:		I.sregs[SS] = val;	I.base[SS] = SegBase(SS);	break;
		case I86_DS:		I.sregs[DS] = val;	I.base[DS] = SegBase(DS);	break;
		case I86_VECTOR:	I.int_vector = val; 		break;
		case I86_PENDING:								break;
		case I86_NMI_STATE: i86_set_irq_line(IRQ_LINE_NMI,val);		break;
		case I86_IRQ_STATE: i86_set_irq_line(0, val);	break;
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP]) & AMASK) + 2 * (REG_SP_CONTENTS - regnum);

				if (offset < AMASK - 1)
				{
					cpu_writemem20(offset, val & 0xff);
					cpu_writemem20(offset + 1, (val >> 8) & 0xff);
				}
			}
	}
}

void i86_set_irq_line(int irqline, int state)
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

void i86_set_irq_callback(int (*callback) (int))
{
	I.irq_callback = callback;
}

int i86_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 8086)
		cycles = i86_cycles;

	/* adjust for any interrupts that came in */
	i86_ICount = num_cycles;
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while (i86_ICount > 0)
	{
/*#define VERBOSE_DEBUG*/
#ifdef VERBOSE_DEBUG
		logerror("[%04x:%04x]=%02x\tF:%04x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x %d%d%d%d%d%d%d%d%d\n",
				I.sregs[CS], I.pc - I.base[CS], ReadByte(I.pc), I.flags, I.regs.w[AX], I.regs.w[BX], I.regs.w[CX], I.regs.w[DX], I.AuxVal ? 1 : 0, I.OverVal ? 1 : 0,
				I.SignVal ? 1 : 0, I.ZeroVal ? 1 : 0, I.CarryVal ? 1 : 0, I.ParityVal ? 1 : 0, I.TF, I.IF, I.DirVal < 0 ? 1 : 0);
#endif
		CALL_MAME_DEBUG;

		seg_prefix = FALSE;
		I.prevpc = I.pc;
		TABLE86;
	}

	/* adjust for any interrupts that came in */
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i86_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i86_info(void *context, int regnum)
{
	static char buffer[32][63 + 1];
	static int which = 0;
	i86_Regs *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';
	if (!context)
		r = &I;

	switch (regnum)
	{
	case CPU_INFO_REG + I86_IP: 		sprintf(buffer[which], "IP: %04X", r->pc - r->base[CS]); break;
	case CPU_INFO_REG + I86_SP: 		sprintf(buffer[which], "SP: %04X", r->regs.w[SP]);  break;
	case CPU_INFO_REG + I86_FLAGS:		sprintf(buffer[which], "F:%04X", r->flags);         break;
	case CPU_INFO_REG + I86_AX: 		sprintf(buffer[which], "AX:%04X", r->regs.w[AX]);   break;
	case CPU_INFO_REG + I86_CX: 		sprintf(buffer[which], "CX:%04X", r->regs.w[CX]);   break;
	case CPU_INFO_REG + I86_DX: 		sprintf(buffer[which], "DX:%04X", r->regs.w[DX]);   break;
	case CPU_INFO_REG + I86_BX: 		sprintf(buffer[which], "BX:%04X", r->regs.w[BX]);   break;
	case CPU_INFO_REG + I86_BP: 		sprintf(buffer[which], "BP:%04X", r->regs.w[BP]);   break;
	case CPU_INFO_REG + I86_SI: 		sprintf(buffer[which], "SI: %04X", r->regs.w[SI]);  break;
	case CPU_INFO_REG + I86_DI: 		sprintf(buffer[which], "DI: %04X", r->regs.w[DI]);  break;
	case CPU_INFO_REG + I86_ES: 		sprintf(buffer[which], "ES:%04X", r->sregs[ES]);    break;
	case CPU_INFO_REG + I86_CS: 		sprintf(buffer[which], "CS:%04X", r->sregs[CS]);    break;
	case CPU_INFO_REG + I86_SS: 		sprintf(buffer[which], "SS:%04X", r->sregs[SS]);    break;
	case CPU_INFO_REG + I86_DS: 		sprintf(buffer[which], "DS:%04X", r->sregs[DS]);    break;
	case CPU_INFO_REG + I86_VECTOR: 	sprintf(buffer[which], "V:%02X", r->int_vector);    break;
	case CPU_INFO_REG + I86_PENDING:	sprintf(buffer[which], "P:%X", r->irq_state);       break;
	case CPU_INFO_REG + I86_NMI_STATE:	sprintf(buffer[which], "NMI:%X", r->nmi_state);     break;
	case CPU_INFO_REG + I86_IRQ_STATE:	sprintf(buffer[which], "IRQ:%X", r->irq_state);     break;
	case CPU_INFO_FLAGS:
		r->flags = CompressFlags();
		sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->flags & 0x8000 ? '?' : '.',
				r->flags & 0x4000 ? '?' : '.',
				r->flags & 0x2000 ? '?' : '.',
				r->flags & 0x1000 ? '?' : '.',
				r->flags & 0x0800 ? 'O' : '.',
				r->flags & 0x0400 ? 'D' : '.',
				r->flags & 0x0200 ? 'I' : '.',
				r->flags & 0x0100 ? 'T' : '.',
				r->flags & 0x0080 ? 'S' : '.',
				r->flags & 0x0040 ? 'Z' : '.',
				r->flags & 0x0020 ? '?' : '.',
				r->flags & 0x0010 ? 'A' : '.',
				r->flags & 0x0008 ? '?' : '.',
				r->flags & 0x0004 ? 'P' : '.',
				r->flags & 0x0002 ? 'N' : '.',
				r->flags & 0x0001 ? 'C' : '.');
		break;
	case CPU_INFO_NAME: 		return "I8086";
	case CPU_INFO_FAMILY:		return "Intel 80x86";
	case CPU_INFO_VERSION:		return "1.4";
	case CPU_INFO_FILE: 		return __FILE__;
	case CPU_INFO_CREDITS:		return "Real mode i286 emulator v1.4 by Fabrice Frances\n(initial work I.based on David Hedley's pcemu)";
	case CPU_INFO_REG_LAYOUT:	return (const char *) i86_reg_layout;
	case CPU_INFO_WIN_LAYOUT:	return (const char *) i86_win_layout;
	}
	return buffer[which];
}

unsigned i86_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmI86(buffer, pc);
#else
	sprintf(buffer, "$%02X", cpu_readop(pc));
	return 1;
#endif
}

#if (HAS_I88)
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i88_info(void *context, int regnum)
{
	switch (regnum)
	{
	case CPU_INFO_NAME:
		return "I8088";
	}
	return i86_info(context, regnum);
}
#endif

#if (HAS_I186 || HAS_I188)

#include "i186intf.h"

#undef PREFIX
#define PREFIX(name) i186##name
#define PREFIX186(name) i186##name

#define I186
#include "instr186.h"
#include "table186.h"

#include "instr86.c"
#include "instr186.c"
#undef I186

int i186_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 80186)
		cycles = i186_cycles;

	/* adjust for any interrupts that came in */
	i86_ICount = num_cycles;
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while (i86_ICount > 0)
	{
#ifdef VERBOSE_DEBUG
		printf("[%04x:%04x]=%02x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x\n", I.sregs[CS], I.pc, ReadByte(I.pc), I.regs.w[AX],
			   I.regs.w[BX], I.regs.w[CX], I.regs.w[DX]);
#endif
		CALL_MAME_DEBUG;

		seg_prefix = FALSE;
		I.prevpc = I.pc;
		TABLE186;
	}

	/* adjust for any interrupts that came in */
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i86_ICount;
}

#if (HAS_I188)
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i188_info(void *context, int regnum)
{
	switch (regnum)
	{
	case CPU_INFO_NAME:
		return "I80188";
	}
	return i186_info(context, regnum);
}
#endif

#if (HAS_I186)
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i186_info(void *context, int regnum)
{
	switch (regnum)
	{
	case CPU_INFO_NAME:
		return "I80186";
	}
	return i86_info(context, regnum);
}
#endif

unsigned i186_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmI186(buffer, pc);
#else
	sprintf(buffer, "$%02X", cpu_readop(pc));
	return 1;
#endif
}

#endif

#if defined(INCLUDE_V20) && (HAS_V20 || HAS_V30 || HAS_V33)

#include "v30.h"
#include "v30intf.h"

/* compile the opcode emulating instruction new with v20 timing value */
/* MF flag -> opcode must be compiled new */
#undef PREFIX
#define PREFIX(name) v30##name
#undef PREFIX86
#define PREFIX86(name) v30##name
#undef PREFIX186
#define PREFIX186(name) v30##name
#define PREFIXV30(name) v30##name

#define nec_ICount i86_ICount

static UINT16 bytes[] =
{
	   1,	 2,    4,	 8,
	  16,	32,   64,  128,
	 256,  512, 1024, 2048,
	4096, 8192,16384,32768	/*,65536 */
};

#undef IncWordReg
#undef DecWordReg
#define V20
#include "instr86.h"
#include "instr186.h"
#include "instrv30.h"
#include "tablev30.h"

static void v30_interrupt(unsigned int_num, BOOLEAN md_flag)
{
	unsigned dest_seg, dest_off;

#if 0
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC=%05x : NEC Interrupt %02d", activecpu_get_pc(), int_num);
#endif

	v30_pushf();
	I.TF = I.IF = 0;
	if (md_flag)
		SetMD(0);					   /* clear Mode-flag = start 8080 emulation mode */

	if (int_num == -1)
	{
		int_num = (*I.irq_callback) (0);
		log_cb(RETRO_LOG_DEBUG, LOGPRE " (indirect ->%02d) ",int_num);
	}

	dest_off = ReadWord(int_num * 4);
	dest_seg = ReadWord(int_num * 4 + 2);

	PUSH(I.sregs[CS]);
	PUSH(I.pc - I.base[CS]);
	I.sregs[CS] = (WORD) dest_seg;
	I.base[CS] = SegBase(CS);
	I.pc = (I.base[CS] + dest_off) & AMASK;
	change_pc20(I.pc);
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "=%06x\n",activecpu_get_pc()); */
}

void v30_trap(void)
{
	(*v30_instruction[FETCHOP])();
	v30_interrupt(1, 0);
}


#include "instr86.c"
#include "instr186.c"
#include "instrv30.c"
#undef V20

void v30_reset(void *param)
{
	i86_reset(param);
	SetMD(1);
}

int v30_execute(int num_cycles)
{
	/* copy over the cycle counts if they're not correct */
	if (cycles.id != 30)
		cycles = v30_cycles;

	/* adjust for any interrupts that came in */
	i86_ICount = num_cycles;
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	/* run until we're out */
	while (i86_ICount > 0)
	{
#ifdef VERBOSE_DEBUG
		printf("[%04x:%04x]=%02x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x\n", I.sregs[CS], I.pc, ReadByte(I.pc), I.regs.w[AX],
			   I.regs.w[BX], I.regs.w[CX], I.regs.w[DX]);
#endif
		CALL_MAME_DEBUG;

		seg_prefix = FALSE;
		I.prevpc = I.pc;
		TABLEV30;
	}

	/* adjust for any interrupts that came in */
	i86_ICount -= I.extra_cycles;
	I.extra_cycles = 0;

	return num_cycles - i86_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *v30_info(void *context, int regnum)
{
	static char buffer[17];

	switch (regnum)
	{
	case CPU_INFO_NAME:
		return "V30";
	case CPU_INFO_FLAGS:
		I.flags = CompressFlags();
		sprintf(buffer, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				I.flags & 0x8000 ? 'M' : '.',
				I.flags & 0x4000 ? '?' : '.',
				I.flags & 0x2000 ? '?' : '.',
				I.flags & 0x1000 ? '?' : '.',
				I.flags & 0x0800 ? 'O' : '.',
				I.flags & 0x0400 ? 'D' : '.',
				I.flags & 0x0200 ? 'I' : '.',
				I.flags & 0x0100 ? 'T' : '.',
				I.flags & 0x0080 ? 'S' : '.',
				I.flags & 0x0040 ? 'Z' : '.',
				I.flags & 0x0020 ? '?' : '.',
				I.flags & 0x0010 ? 'A' : '.',
				I.flags & 0x0008 ? '?' : '.',
				I.flags & 0x0004 ? 'P' : '.',
				I.flags & 0x0002 ? 'N' : '.',
				I.flags & 0x0001 ? 'C' : '.');
		break;
	}
	return i86_info(context, regnum);
}

#if (HAS_V20)
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *v20_info(void *context, int regnum)
{
	switch (regnum)
	{
	case CPU_INFO_NAME:
		return "V20";
	}
	return v30_info(context, regnum);
}
#endif

#if (HAS_V33)
/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *v33_info(void *context, int regnum)
{
	switch (regnum)
	{
	case CPU_INFO_NAME:
		return "V33";
	}
	return v30_info(context, regnum);
}
#endif

unsigned v30_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmV30(buffer, pc);
#else
	sprintf(buffer, "$%02X", cpu_readop(pc));
	return 1;
#endif
}

#endif
