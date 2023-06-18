/*###################################################################################################
**
**
**		jaguar.c
**		Core implementation for the portable Jaguar DSP emulator.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "mamedbg.h"
#include "state.h"
#include "jaguar.h"


/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

#define ZFLAG				0x00001
#define CFLAG				0x00002
#define NFLAG				0x00004
#define IFLAG				0x00008
#define EINT0FLAG			0x00010
#define EINT1FLAG			0x00020
#define EINT2FLAG			0x00040
#define EINT3FLAG			0x00080
#define EINT4FLAG			0x00100
#define EINT04FLAGS			(EINT0FLAG | EINT1FLAG | EINT2FLAG | EINT3FLAG | EINT4FLAG)
#define CINT0FLAG			0x00200
#define CINT1FLAG			0x00400
#define CINT2FLAG			0x00800
#define CINT3FLAG			0x01000
#define CINT4FLAG			0x02000
#define CINT04FLAGS			(CINT0FLAG | CINT1FLAG | CINT2FLAG | CINT3FLAG | CINT4FLAG)
#define RPAGEFLAG			0x04000
#define DMAFLAG				0x08000
#define EINT5FLAG			0x10000		/* DSP only */
#define CINT5FLAG			0x20000		/* DSP only */

#define CLR_Z				(jaguar.FLAGS &= ~ZFLAG)
#define CLR_ZN				(jaguar.FLAGS &= ~(ZFLAG | NFLAG))
#define CLR_ZNC				(jaguar.FLAGS &= ~(CFLAG | ZFLAG | NFLAG))
#define SET_Z(r)			(jaguar.FLAGS |= ((r) == 0))
#define SET_C_ADD(a,b)		(jaguar.FLAGS |= ((UINT32)(b) > (UINT32)(~(a))) << 1)
#define SET_C_SUB(a,b)		(jaguar.FLAGS |= ((UINT32)(b) > (UINT32)(a)) << 1)
#define SET_N(r)			(jaguar.FLAGS |= (((UINT32)(r) >> 29) & 4))
#define SET_ZN(r)			SET_N(r); SET_Z(r)
#define SET_ZNC_ADD(a,b,r)	SET_N(r); SET_Z(r); SET_C_ADD(a,b)
#define SET_ZNC_SUB(a,b,r)	SET_N(r); SET_Z(r); SET_C_SUB(a,b)



/*###################################################################################################
**	MACROS
**#################################################################################################*/

#define PC				ctrl[G_PC]
#define FLAGS			ctrl[G_FLAGS]

#define CONDITION(x)	condition_table[(x) + ((jaguar.FLAGS & 7) << 5)]

#define READBYTE(a)		cpu_readmem24bedw(a)
#define READWORD(a)		cpu_readmem24bedw_word(a)
#define READLONG(a)		cpu_readmem24bedw_dword(a)

#define WRITEBYTE(a,v)	cpu_writemem24bedw(a,v)
#define WRITEWORD(a,v)	cpu_writemem24bedw_word(a,v)
#define WRITELONG(a,v)	cpu_writemem24bedw_dword(a,v)



/*###################################################################################################
**	STRUCTURES & TYPEDEFS
**#################################################################################################*/

/* Jaguar Registers */
typedef struct
{
	/* core registers */
	UINT32		r[32];
	UINT32		a[32];
	UINT32 *	b0;
	UINT32 *	b1;

	/* control registers */
	UINT32		ctrl[G_CTRLMAX];
	UINT32		ppc;
	UINT64		accum;

	/* internal stuff */
	int			isdsp;
	int			op;
	int			interrupt_cycles;
	void 		(**table)(void);
	int 		(*irq_callback)(int irqline);
	void		(*cpu_interrupt)(void);
} jaguar_regs;



/*###################################################################################################
**	PUBLIC GLOBAL VARIABLES
**#################################################################################################*/

int	jaguar_icount;
static int bankswitch_icount;



/*###################################################################################################
**	PRIVATE GLOBAL VARIABLES
**#################################################################################################*/

static jaguar_regs	jaguar;
static UINT16 *		mirror_table;
static UINT8 *		condition_table;
static int			executing_cpu = -1;

static const UINT32 convert_zero[32] =
{ 32,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };



/*###################################################################################################
**	FUNCTION TABLES
**#################################################################################################*/

static void abs_rn(void);
static void add_rn_rn(void);
static void addc_rn_rn(void);
static void addq_n_rn(void);
static void addqmod_n_rn(void);	/* DSP only */
static void addqt_n_rn(void);
static void and_rn_rn(void);
static void bclr_n_rn(void);
static void bset_n_rn(void);
static void btst_n_rn(void);
static void cmp_rn_rn(void);
static void cmpq_n_rn(void);
static void div_rn_rn(void);
static void illegal(void);
static void imacn_rn_rn(void);
static void imult_rn_rn(void);
static void imultn_rn_rn(void);
static void jr_cc_n(void);
static void jump_cc_rn(void);
static void load_rn_rn(void);
static void load_r14n_rn(void);
static void load_r15n_rn(void);
static void load_r14rn_rn(void);
static void load_r15rn_rn(void);
static void loadb_rn_rn(void);
static void loadw_rn_rn(void);
static void loadp_rn_rn(void);	/* GPU only */
static void mirror_rn(void);	/* DSP only */
static void mmult_rn_rn(void);
static void move_rn_rn(void);
static void move_pc_rn(void);
static void movefa_rn_rn(void);
static void movei_n_rn(void);
static void moveq_n_rn(void);
static void moveta_rn_rn(void);
static void mtoi_rn_rn(void);
static void mult_rn_rn(void);
static void neg_rn(void);
static void nop(void);
static void normi_rn_rn(void);
static void not_rn(void);
static void or_rn_rn(void);
static void pack_rn(void);		/* GPU only */
static void resmac_rn(void);
static void ror_rn_rn(void);
static void rorq_n_rn(void);
static void sat8_rn(void);		/* GPU only */
static void sat16_rn(void);		/* GPU only */
static void sat16s_rn(void);		/* DSP only */
static void sat24_rn(void);			/* GPU only */
static void sat32s_rn(void);		/* DSP only */
static void sh_rn_rn(void);
static void sha_rn_rn(void);
static void sharq_n_rn(void);
static void shlq_n_rn(void);
static void shrq_n_rn(void);
static void store_rn_rn(void);
static void store_rn_r14n(void);
static void store_rn_r15n(void);
static void store_rn_r14rn(void);
static void store_rn_r15rn(void);
static void storeb_rn_rn(void);
static void storew_rn_rn(void);
static void storep_rn_rn(void);	/* GPU only */
static void sub_rn_rn(void);
static void subc_rn_rn(void);
static void subq_n_rn(void);
static void subqmod_n_rn(void);	/* DSP only */
static void subqt_n_rn(void);
static void xor_rn_rn(void);

static void (*gpu_op_table[64])(void) =
{
	/* 00-03 */	add_rn_rn,		addc_rn_rn,		addq_n_rn,		addqt_n_rn,
	/* 04-07 */	sub_rn_rn,		subc_rn_rn,		subq_n_rn,		subqt_n_rn,
	/* 08-11 */	neg_rn,			and_rn_rn,		or_rn_rn,		xor_rn_rn,
	/* 12-15 */	not_rn,			btst_n_rn,		bset_n_rn,		bclr_n_rn,
	/* 16-19 */	mult_rn_rn,		imult_rn_rn,	imultn_rn_rn,	resmac_rn,
	/* 20-23 */	imacn_rn_rn,	div_rn_rn,		abs_rn,			sh_rn_rn,
	/* 24-27 */	shlq_n_rn,		shrq_n_rn,		sha_rn_rn,		sharq_n_rn,
	/* 28-31 */	ror_rn_rn,		rorq_n_rn,		cmp_rn_rn,		cmpq_n_rn,
	/* 32-35 */	sat8_rn,		sat16_rn,		move_rn_rn,		moveq_n_rn,
	/* 36-39 */	moveta_rn_rn,	movefa_rn_rn,	movei_n_rn,		loadb_rn_rn,
	/* 40-43 */	loadw_rn_rn,	load_rn_rn,		loadp_rn_rn,	load_r14n_rn,
	/* 44-47 */	load_r15n_rn,	storeb_rn_rn,	storew_rn_rn,	store_rn_rn,
	/* 48-51 */	storep_rn_rn,	store_rn_r14n,	store_rn_r15n,	move_pc_rn,
	/* 52-55 */	jump_cc_rn,		jr_cc_n,		mmult_rn_rn,	mtoi_rn_rn,
	/* 56-59 */	normi_rn_rn,	nop,			load_r14rn_rn,	load_r15rn_rn,
	/* 60-63 */	store_rn_r14rn,	store_rn_r15rn,	sat24_rn,		pack_rn
};

static void (*dsp_op_table[64])(void) =
{
	/* 00-03 */	add_rn_rn,		addc_rn_rn,		addq_n_rn,		addqt_n_rn,
	/* 04-07 */	sub_rn_rn,		subc_rn_rn,		subq_n_rn,		subqt_n_rn,
	/* 08-11 */	neg_rn,			and_rn_rn,		or_rn_rn,		xor_rn_rn,
	/* 12-15 */	not_rn,			btst_n_rn,		bset_n_rn,		bclr_n_rn,
	/* 16-19 */	mult_rn_rn,		imult_rn_rn,	imultn_rn_rn,	resmac_rn,
	/* 20-23 */	imacn_rn_rn,	div_rn_rn,		abs_rn,			sh_rn_rn,
	/* 24-27 */	shlq_n_rn,		shrq_n_rn,		sha_rn_rn,		sharq_n_rn,
	/* 28-31 */	ror_rn_rn,		rorq_n_rn,		cmp_rn_rn,		cmpq_n_rn,
	/* 32-35 */	subqmod_n_rn,	sat16s_rn,		move_rn_rn,		moveq_n_rn,
	/* 36-39 */	moveta_rn_rn,	movefa_rn_rn,	movei_n_rn,		loadb_rn_rn,
	/* 40-43 */	loadw_rn_rn,	load_rn_rn,		sat32s_rn,		load_r14n_rn,
	/* 44-47 */	load_r15n_rn,	storeb_rn_rn,	storew_rn_rn,	store_rn_rn,
	/* 48-51 */	mirror_rn,		store_rn_r14n,	store_rn_r15n,	move_pc_rn,
	/* 52-55 */	jump_cc_rn,		jr_cc_n,		mmult_rn_rn,	mtoi_rn_rn,
	/* 56-59 */	normi_rn_rn,	nop,			load_r14rn_rn,	load_r15rn_rn,
	/* 60-63 */	store_rn_r14rn,	store_rn_r15rn,	illegal,		addqmod_n_rn
};



/*###################################################################################################
**	MEMORY ACCESSORS
**#################################################################################################*/

#ifdef MSB_FIRST
#define ROPCODE(pc)		(*(UINT16 *)&OP_ROM[(UINT32)(pc)])
#else
#define ROPCODE(pc)		(*(UINT16 *)&OP_ROM[(UINT32)(pc) ^ 2])
#endif



/*###################################################################################################
**	INLINES
**#################################################################################################*/

static INLINE void update_register_banks(void)
{
	UINT32 temp;
	int i, bank;

	/* pick the bank */
	bank = jaguar.FLAGS & RPAGEFLAG;
	if (jaguar.FLAGS & IFLAG) bank = 0;

	/* do we need to swap? */
	if ((!bank && jaguar.b0 != jaguar.r) || (bank && jaguar.b1 != jaguar.r))
	{
		/* remember the icount of the instruction after we swap */
		bankswitch_icount = jaguar_icount - 1;

		/* exchange the contents */
		for (i = 0; i < 32; i++)
			temp = jaguar.r[i], jaguar.r[i] = jaguar.a[i], jaguar.a[i] = temp;

		/* swap the bank pointers */
		if (!bank)
		{
			jaguar.b0 = jaguar.r;
			jaguar.b1 = jaguar.a;
		}
		else
		{
			jaguar.b0 = jaguar.a;
			jaguar.b1 = jaguar.r;
		}
	}
}



/*###################################################################################################
**	IRQ HANDLING
**#################################################################################################*/

static void check_irqs(void)
{
	int bits, mask, which = 0;

	/* if the IMASK is set, bail */
	if (jaguar.FLAGS & IFLAG)
		return;

	/* get the active interrupt bits */
	bits = (jaguar.ctrl[G_CTRL] >> 6) & 0x1f;
	bits |= (jaguar.ctrl[G_CTRL] >> 10) & 0x20;

	/* get the interrupt mask */
	mask = (jaguar.FLAGS >> 4) & 0x1f;
	mask |= (jaguar.FLAGS >> 11) & 0x20;

	/* bail if nothing is available */
	bits &= mask;
	if (!bits)
		return;

	/* determine which interrupt */
	if (bits & 0x01) which = 0;
	if (bits & 0x02) which = 1;
	if (bits & 0x04) which = 2;
	if (bits & 0x08) which = 3;
	if (bits & 0x10) which = 4;
	if (bits & 0x20) which = 5;

	/* set the interrupt flag */
	jaguar.FLAGS |= IFLAG;
	update_register_banks();

	/* push the PC-2 on the stack */
	jaguar.r[31] -= 4;
	WRITELONG(jaguar.r[31], jaguar.PC - 2);

	/* dispatch */
	jaguar.PC = (jaguar.isdsp) ? 0xf1b000 : 0xf03000;
	jaguar.PC += which * 0x10;
	change_pc24bedw(jaguar.PC);
}


void jaguargpu_set_irq_line(int irqline, int state)
{
	int mask = 0x40 << irqline;
	jaguar.ctrl[G_CTRL] &= ~mask;
	if (state != CLEAR_LINE)
	{
		jaguar.ctrl[G_CTRL] |= mask;
		check_irqs();
	}
}

void jaguardsp_set_irq_line(int irqline, int state)
{
	int mask = (irqline < 5) ? (0x40 << irqline) : 0x10000;
	jaguar.ctrl[G_CTRL] &= ~mask;
	if (state != CLEAR_LINE)
	{
		jaguar.ctrl[G_CTRL] |= mask;
		check_irqs();
	}
}


void jaguargpu_set_irq_callback(int (*callback)(int irqline))
{
	jaguar.irq_callback = callback;
}

void jaguardsp_set_irq_callback(int (*callback)(int irqline))
{
	jaguar.irq_callback = callback;
}



/*###################################################################################################
**	CONTEXT SWITCHING
**#################################################################################################*/

unsigned jaguargpu_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(jaguar_regs *)dst = jaguar;

	/* return the context size */
	return sizeof(jaguar_regs);
}

unsigned jaguardsp_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(jaguar_regs *)dst = jaguar;

	/* return the context size */
	return sizeof(jaguar_regs);
}


void jaguargpu_set_context(void *src)
{
	/* copy the context */
	if (src)
		jaguar = *(jaguar_regs *)src;

	/* check for IRQs */
	check_irqs();
}

void jaguardsp_set_context(void *src)
{
	/* copy the context */
	if (src)
		jaguar = *(jaguar_regs *)src;

	/* check for IRQs */
	check_irqs();
}


/*###################################################################################################
**	INITIALIZATION AND SHUTDOWN
**#################################################################################################*/

static void init_tables(void)
{
	int i, j;

	/* allocate the mirror table */
	if (!mirror_table)
		mirror_table = malloc(65536 * sizeof(mirror_table[0]));

	/* fill in the mirror table */
	if (mirror_table)
		for (i = 0; i < 65536; i++)
			mirror_table[i] = ((i >> 15) & 0x0001) | ((i >> 13) & 0x0002) |
			                  ((i >> 11) & 0x0004) | ((i >> 9)  & 0x0008) |
			                  ((i >> 7)  & 0x0010) | ((i >> 5)  & 0x0020) |
			                  ((i >> 3)  & 0x0040) | ((i >> 1)  & 0x0080) |
			                  ((i << 1)  & 0x0100) | ((i << 3)  & 0x0200) |
			                  ((i << 5)  & 0x0400) | ((i << 7)  & 0x0800) |
			                  ((i << 9)  & 0x1000) | ((i << 11) & 0x2000) |
			                  ((i << 13) & 0x4000) | ((i << 15) & 0x8000);

	/* allocate the condition table */
	if (!condition_table)
		condition_table = malloc(32 * 8 * sizeof(condition_table[0]));

	/* fill in the condition table */
	if (condition_table)
		for (i = 0; i < 8; i++)
			for (j = 0; j < 32; j++)
			{
				int result = 1;
				if (j & 1)
					if (i & ZFLAG) result = 0;
				if (j & 2)
					if (!(i & ZFLAG)) result = 0;
				if (j & 4)
					if (i & (CFLAG << (j >> 4))) result = 0;
				if (j & 8)
					if (!(i & (CFLAG << (j >> 4)))) result = 0;
				condition_table[i * 32 + j] = result;
			}
}

static void jaguar_state_register(const char *type)
{
	int cpu = cpu_getactivecpu();
	state_save_register_UINT32(type, cpu, "R",    jaguar.r, 32);
	state_save_register_UINT32(type, cpu, "A",    jaguar.a, 32);
	state_save_register_UINT32(type, cpu, "CTRL", jaguar.ctrl, G_CTRLMAX);
	state_save_register_UINT32(type, cpu, "PPC",  &jaguar.ppc, 1);
	state_save_register_func_postload(update_register_banks);
	state_save_register_func_postload(check_irqs);
}

void jaguargpu_init(void)
{
	jaguar_state_register("jaguargpu");
}

void jaguardsp_init(void)
{
	jaguar_state_register("jaguardsp");
}

static INLINE void common_reset(struct jaguar_config *config)
{
	init_tables();

	if (config)
		jaguar.cpu_interrupt = config->cpu_int_callback;

	jaguar.b0 = jaguar.r;
	jaguar.b1 = jaguar.a;

	change_pc24bedw(jaguar.PC);
}

void jaguargpu_reset(void *param)
{
	common_reset(param);
	jaguar.table = gpu_op_table;
	jaguar.isdsp = 0;
}

void jaguardsp_reset(void *param)
{
	common_reset(param);
	jaguar.table = dsp_op_table;
	jaguar.isdsp = 1;
}

static INLINE void common_exit(void)
{
	if (mirror_table)
		free(mirror_table);
	mirror_table = NULL;

	if (condition_table)
		free(condition_table);
	condition_table = NULL;
}

void jaguargpu_exit(void)
{
	common_exit();
}

void jaguardsp_exit(void)
{
	common_exit();
}



/*###################################################################################################
**	CORE EXECUTION LOOP
**#################################################################################################*/

int jaguargpu_execute(int cycles)
{
	/* if we're halted, we shouldn't be here */
	if (!(jaguar.ctrl[G_CTRL] & 1))
	{
		cpu_set_halt_line(cpu_getactivecpu(), ASSERT_LINE);
		return cycles;
	}

	/* count cycles and interrupt cycles */
	bankswitch_icount = -1000;
	jaguar_icount = cycles;
	jaguar_icount -= jaguar.interrupt_cycles;
	jaguar.interrupt_cycles = 0;
	change_pc24bedw(jaguar.PC);

	/* remember that we're executing */
	executing_cpu = cpu_getactivecpu();

	/* core execution loop */
	do
	{
		/* debugging */
		/*if (jaguar.PC < 0xf03000 || jaguar.PC > 0xf04000) { fprintf(stderr, "GPU: jaguar.PC = %06X (ppc = %06X)\n", jaguar.PC, jaguar.ppc); exit(1); }*/
		jaguar.ppc = jaguar.PC;
		CALL_MAME_DEBUG;

		/* instruction fetch */
		jaguar.op = ROPCODE(jaguar.PC);
		jaguar.PC += 2;

		/* parse the instruction */
		(*gpu_op_table[jaguar.op >> 10])();
		jaguar_icount--;

	} while (jaguar_icount > 0 || jaguar_icount == bankswitch_icount);

	/* no longer executing */
	executing_cpu = -1;

	/* adjust cycles for interrupts */
	jaguar_icount -= jaguar.interrupt_cycles;
	jaguar.interrupt_cycles = 0;
	return cycles - jaguar_icount;
}

int jaguardsp_execute(int cycles)
{
	/* if we're halted, we shouldn't be here */
	if (!(jaguar.ctrl[G_CTRL] & 1))
	{
		cpu_set_halt_line(cpu_getactivecpu(), ASSERT_LINE);
		return cycles;
	}

	/* count cycles and interrupt cycles */
	bankswitch_icount = -1000;
	jaguar_icount = cycles;
	jaguar_icount -= jaguar.interrupt_cycles;
	jaguar.interrupt_cycles = 0;
	change_pc24bedw(jaguar.PC);

	/* remember that we're executing */
	executing_cpu = cpu_getactivecpu();

	/* core execution loop */
	do
	{
		/* debugging */
		/*if (jaguar.PC < 0xf1b000 || jaguar.PC > 0xf1d000) { fprintf(stderr, "DSP: jaguar.PC = %06X\n", jaguar.PC); exit(1); }*/
		jaguar.ppc = jaguar.PC;
		CALL_MAME_DEBUG;

		/* instruction fetch */
		jaguar.op = ROPCODE(jaguar.PC);
		jaguar.PC += 2;

		/* parse the instruction */
		(*dsp_op_table[jaguar.op >> 10])();
		jaguar_icount--;

	} while (jaguar_icount > 0 || jaguar_icount == bankswitch_icount);

	/* no longer executing */
	executing_cpu = -1;

	/* adjust cycles for interrupts */
	jaguar_icount -= jaguar.interrupt_cycles;
	jaguar.interrupt_cycles = 0;
	return cycles - jaguar_icount;
}



/*###################################################################################################
**	REGISTER SNOOP
**#################################################################################################*/

static INLINE unsigned common_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case JAGUAR_PC:		return jaguar.PC;
		case JAGUAR_FLAGS:	return jaguar.FLAGS;

		case JAGUAR_R0:		return jaguar.r[0];
		case JAGUAR_R1:		return jaguar.r[1];
		case JAGUAR_R2:		return jaguar.r[2];
		case JAGUAR_R3:		return jaguar.r[3];
		case JAGUAR_R4:		return jaguar.r[4];
		case JAGUAR_R5:		return jaguar.r[5];
		case JAGUAR_R6:		return jaguar.r[6];
		case JAGUAR_R7:		return jaguar.r[7];
		case JAGUAR_R8:		return jaguar.r[8];
		case JAGUAR_R9:		return jaguar.r[9];
		case JAGUAR_R10:	return jaguar.r[10];
		case JAGUAR_R11:	return jaguar.r[11];
		case JAGUAR_R12:	return jaguar.r[12];
		case JAGUAR_R13:	return jaguar.r[13];
		case JAGUAR_R14:	return jaguar.r[14];
		case JAGUAR_R15:	return jaguar.r[15];
		case JAGUAR_R16:	return jaguar.r[16];
		case JAGUAR_R17:	return jaguar.r[17];
		case JAGUAR_R18:	return jaguar.r[18];
		case JAGUAR_R19:	return jaguar.r[19];
		case JAGUAR_R20:	return jaguar.r[20];
		case JAGUAR_R21:	return jaguar.r[21];
		case JAGUAR_R22:	return jaguar.r[22];
		case JAGUAR_R23:	return jaguar.r[23];
		case JAGUAR_R24:	return jaguar.r[24];
		case JAGUAR_R25:	return jaguar.r[25];
		case JAGUAR_R26:	return jaguar.r[26];
		case JAGUAR_R27:	return jaguar.r[27];
		case JAGUAR_R28:	return jaguar.r[28];
		case JAGUAR_R29:	return jaguar.r[29];
		case JAGUAR_R30:	return jaguar.r[30];
		case JAGUAR_R31:	return jaguar.r[31];
		case REG_SP:		return jaguar.b0[31];

		case REG_PREVIOUSPC: return jaguar.ppc;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
/*				unsigned offset = REG_SP_CONTENTS - regnum;*/
/*				if (offset < PC_STACK_DEPTH)*/
/*					return jaguar.pc_stack[offset];*/
			}
	}
	return 0;
}

unsigned jaguargpu_get_reg(int regnum)
{
	return common_get_reg(regnum);
}

unsigned jaguardsp_get_reg(int regnum)
{
	return common_get_reg(regnum);
}



/*###################################################################################################
**	REGISTER MODIFY
**#################################################################################################*/

static INLINE void common_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
		case JAGUAR_PC:		jaguar.PC = val;	break;
		case JAGUAR_FLAGS:	jaguar.FLAGS = val;	break;

		case JAGUAR_R0:		jaguar.r[0] = val;	break;
		case JAGUAR_R1:		jaguar.r[1] = val;	break;
		case JAGUAR_R2:		jaguar.r[2] = val;	break;
		case JAGUAR_R3:		jaguar.r[3] = val;	break;
		case JAGUAR_R4:		jaguar.r[4] = val;	break;
		case JAGUAR_R5:		jaguar.r[5] = val;	break;
		case JAGUAR_R6:		jaguar.r[6] = val;	break;
		case JAGUAR_R7:		jaguar.r[7] = val;	break;
		case JAGUAR_R8:		jaguar.r[8] = val;	break;
		case JAGUAR_R9:		jaguar.r[9] = val;	break;
		case JAGUAR_R10:	jaguar.r[10] = val;	break;
		case JAGUAR_R11:	jaguar.r[11] = val;	break;
		case JAGUAR_R12:	jaguar.r[12] = val;	break;
		case JAGUAR_R13:	jaguar.r[13] = val;	break;
		case JAGUAR_R14:	jaguar.r[14] = val;	break;
		case JAGUAR_R15:	jaguar.r[15] = val;	break;
		case JAGUAR_R16:	jaguar.r[16] = val;	break;
		case JAGUAR_R17:	jaguar.r[17] = val;	break;
		case JAGUAR_R18:	jaguar.r[18] = val;	break;
		case JAGUAR_R19:	jaguar.r[19] = val;	break;
		case JAGUAR_R20:	jaguar.r[20] = val;	break;
		case JAGUAR_R21:	jaguar.r[21] = val;	break;
		case JAGUAR_R22:	jaguar.r[22] = val;	break;
		case JAGUAR_R23:	jaguar.r[23] = val;	break;
		case JAGUAR_R24:	jaguar.r[24] = val;	break;
		case JAGUAR_R25:	jaguar.r[25] = val;	break;
		case JAGUAR_R26:	jaguar.r[26] = val;	break;
		case JAGUAR_R27:	jaguar.r[27] = val;	break;
		case JAGUAR_R28:	jaguar.r[28] = val;	break;
		case JAGUAR_R29:	jaguar.r[29] = val;	break;
		case JAGUAR_R30:	jaguar.r[30] = val;	break;
		case JAGUAR_R31:	jaguar.r[31] = val;	break;
		case REG_SP:		jaguar.b0[31] = val; break;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
/*				unsigned offset = REG_SP_CONTENTS - regnum;*/
/*				if (offset < PC_STACK_DEPTH)*/
/*					jaguar.pc_stack[offset] = val;*/
			}
    }
}

void jaguargpu_set_reg(int regnum, unsigned val)
{
	common_set_reg(regnum, val);
}

void jaguardsp_set_reg(int regnum, unsigned val)
{
	common_set_reg(regnum, val);
}



/*###################################################################################################
**	DEBUGGER DEFINITIONS
**#################################################################################################*/

static UINT8 jaguar_reg_layout[] =
{
	JAGUAR_PC,		JAGUAR_FLAGS,	-1,
	JAGUAR_R0,	 	JAGUAR_R16,		-1,
	JAGUAR_R1, 		JAGUAR_R17,		-1,
	JAGUAR_R2, 		JAGUAR_R18,		-1,
	JAGUAR_R3, 		JAGUAR_R19,		-1,
	JAGUAR_R4, 		JAGUAR_R20,		-1,
	JAGUAR_R5, 		JAGUAR_R21,		-1,
	JAGUAR_R6, 		JAGUAR_R22,		-1,
	JAGUAR_R7, 		JAGUAR_R23,		-1,
	JAGUAR_R8,		JAGUAR_R24,		-1,
	JAGUAR_R9,		JAGUAR_R25,		-1,
	JAGUAR_R10,		JAGUAR_R26,		-1,
	JAGUAR_R11,		JAGUAR_R27,		-1,
	JAGUAR_R12,		JAGUAR_R28,		-1,
	JAGUAR_R13,		JAGUAR_R29,		-1,
	JAGUAR_R14,		JAGUAR_R30,		-1,
	JAGUAR_R15,		JAGUAR_R31,		0
};

static UINT8 jaguar_win_layout[] =
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

static const char *common_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	jaguar_regs *r = context;

	which = (which+1) % 16;
    buffer[which][0] = '\0';

	if (!context)
		r = &jaguar;

    switch( regnum )
	{
		case CPU_INFO_REG+JAGUAR_PC:  	sprintf(buffer[which], "PC: %08X", r->PC); break;

		case CPU_INFO_REG+JAGUAR_R0:	sprintf(buffer[which], "R0: %08X", r->r[0]); break;
		case CPU_INFO_REG+JAGUAR_R1:	sprintf(buffer[which], "R1: %08X", r->r[1]); break;
		case CPU_INFO_REG+JAGUAR_R2:	sprintf(buffer[which], "R2: %08X", r->r[2]); break;
		case CPU_INFO_REG+JAGUAR_R3:	sprintf(buffer[which], "R3: %08X", r->r[3]); break;
		case CPU_INFO_REG+JAGUAR_R4:	sprintf(buffer[which], "R4: %08X", r->r[4]); break;
		case CPU_INFO_REG+JAGUAR_R5:	sprintf(buffer[which], "R5: %08X", r->r[5]); break;
		case CPU_INFO_REG+JAGUAR_R6:	sprintf(buffer[which], "R6: %08X", r->r[6]); break;
		case CPU_INFO_REG+JAGUAR_R7:	sprintf(buffer[which], "R7: %08X", r->r[7]); break;
		case CPU_INFO_REG+JAGUAR_R8:	sprintf(buffer[which], "R8: %08X", r->r[8]); break;
		case CPU_INFO_REG+JAGUAR_R9:	sprintf(buffer[which], "R9: %08X", r->r[9]); break;
		case CPU_INFO_REG+JAGUAR_R10:	sprintf(buffer[which], "R10:%08X", r->r[10]); break;
		case CPU_INFO_REG+JAGUAR_R11:	sprintf(buffer[which], "R11:%08X", r->r[11]); break;
		case CPU_INFO_REG+JAGUAR_R12:	sprintf(buffer[which], "R12:%08X", r->r[12]); break;
		case CPU_INFO_REG+JAGUAR_R13:	sprintf(buffer[which], "R13:%08X", r->r[13]); break;
		case CPU_INFO_REG+JAGUAR_R14:	sprintf(buffer[which], "R14:%08X", r->r[14]); break;
		case CPU_INFO_REG+JAGUAR_R15:	sprintf(buffer[which], "R15:%08X", r->r[15]); break;
		case CPU_INFO_REG+JAGUAR_R16:	sprintf(buffer[which], "R16:%08X", r->r[16]); break;
		case CPU_INFO_REG+JAGUAR_R17:	sprintf(buffer[which], "R17:%08X", r->r[17]); break;
		case CPU_INFO_REG+JAGUAR_R18:	sprintf(buffer[which], "R18:%08X", r->r[18]); break;
		case CPU_INFO_REG+JAGUAR_R19:	sprintf(buffer[which], "R19:%08X", r->r[19]); break;
		case CPU_INFO_REG+JAGUAR_R20:	sprintf(buffer[which], "R20:%08X", r->r[20]); break;
		case CPU_INFO_REG+JAGUAR_R21:	sprintf(buffer[which], "R21:%08X", r->r[21]); break;
		case CPU_INFO_REG+JAGUAR_R22:	sprintf(buffer[which], "R22:%08X", r->r[22]); break;
		case CPU_INFO_REG+JAGUAR_R23:	sprintf(buffer[which], "R23:%08X", r->r[23]); break;
		case CPU_INFO_REG+JAGUAR_R24:	sprintf(buffer[which], "R24:%08X", r->r[24]); break;
		case CPU_INFO_REG+JAGUAR_R25:	sprintf(buffer[which], "R25:%08X", r->r[25]); break;
		case CPU_INFO_REG+JAGUAR_R26:	sprintf(buffer[which], "R26:%08X", r->r[26]); break;
		case CPU_INFO_REG+JAGUAR_R27:	sprintf(buffer[which], "R27:%08X", r->r[27]); break;
		case CPU_INFO_REG+JAGUAR_R28:	sprintf(buffer[which], "R28:%08X", r->r[28]); break;
		case CPU_INFO_REG+JAGUAR_R29:	sprintf(buffer[which], "R29:%08X", r->r[29]); break;
		case CPU_INFO_REG+JAGUAR_R30:	sprintf(buffer[which], "R30:%08X", r->r[30]); break;
		case CPU_INFO_REG+JAGUAR_R31:	sprintf(buffer[which], "R31:%08X", r->r[31]); break;

		case CPU_INFO_REG+JAGUAR_FLAGS:	sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c",
												r->FLAGS & 0x8000 ? 'D':'.',
												r->FLAGS & 0x4000 ? 'A':'.',
												r->FLAGS & 0x0100 ? '4':'.',
												r->FLAGS & 0x0080 ? '3':'.',
												r->FLAGS & 0x0040 ? '2':'.',
												r->FLAGS & 0x0020 ? '1':'.',
												r->FLAGS & 0x0010 ? '0':'.',
												r->FLAGS & 0x0008 ? 'I':'.',
												r->FLAGS & 0x0004 ? 'N':'.',
												r->FLAGS & 0x0002 ? 'C':'.',
												r->FLAGS & 0x0001 ? 'Z':'.'); break;

		case CPU_INFO_FAMILY: return "Jaguar";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Aaron Giles 2000-2002";
		case CPU_INFO_REG_LAYOUT: return (const char *)jaguar_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)jaguar_win_layout;
		case CPU_INFO_REG+10000: return "         ";
    }
	return buffer[which];
}

const char *jaguargpu_info(void *context, int regnum)
{
	switch (regnum)
	{
		case CPU_INFO_NAME: return "Jaguar GPU";
		default:			return common_info(context, regnum);
	}
}

const char *jaguardsp_info(void *context, int regnum)
{
	switch (regnum)
	{
		case CPU_INFO_NAME: return "Jaguar DSP";
		default:			return common_info(context, regnum);
	}
}



/*###################################################################################################
**	DISASSEMBLY HOOK
**#################################################################################################*/

unsigned jaguargpu_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasmjag(int, char *, unsigned);
    return dasmjag(JAGUAR_VARIANT_GPU, buffer, pc);
#else
	sprintf(buffer, "$%04X", ROPCODE(pc));
	return 2;
#endif
}

unsigned jaguardsp_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasmjag(int, char *, unsigned);
    return dasmjag(JAGUAR_VARIANT_DSP, buffer, pc);
#else
	sprintf(buffer, "$%04X", ROPCODE(pc));
	return 2;
#endif
}



/*###################################################################################################
**	OPCODES
**#################################################################################################*/

void abs_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 res = jaguar.r[dreg];
	CLR_ZNC;
	if (res & 0x80000000)
	{
		jaguar.r[dreg] = res = -res;
		jaguar.FLAGS |= CFLAG;
	}
	SET_Z(res);
}

void add_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 + r1;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_ADD(r2,r1,res);
}

void addc_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 + r1 + ((jaguar.FLAGS >> 1) & 1);
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_ADD(r2,r1,res);
}

void addq_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 + r1;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_ADD(r2,r1,res);
}

void addqmod_n_rn(void)	/* DSP only */
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 + r1;
	res = (res & ~jaguar.ctrl[D_MOD]) | (r2 & ~jaguar.ctrl[D_MOD]);
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_ADD(r2,r1,res);
}

void addqt_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 + r1;
	jaguar.r[dreg] = res;
}

void and_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 & r1;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void bclr_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = (jaguar.op >> 5) & 31;
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 & ~(1 << r1);
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void bset_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = (jaguar.op >> 5) & 31;
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 | (1 << r1);
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void btst_n_rn(void)
{
	UINT32 r1 = (jaguar.op >> 5) & 31;
	UINT32 r2 = jaguar.r[jaguar.op & 31];
	CLR_Z; jaguar.FLAGS |= (~r2 >> r1) & 1;
}

void cmp_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[jaguar.op & 31];
	UINT32 res = r2 - r1;
	CLR_ZNC; SET_ZNC_SUB(r2,r1,res);
}

void cmpq_n_rn(void)
{
	UINT32 r1 = (INT8)(jaguar.op >> 2) >> 3;
	UINT32 r2 = jaguar.r[jaguar.op & 31];
	UINT32 res = r2 - r1;
	CLR_ZNC; SET_ZNC_SUB(r2,r1,res);
}

void div_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	if (r1)
	{
		if (jaguar.ctrl[D_DIVCTRL] & 1)
		{
			jaguar.r[dreg] = ((UINT64)r2 << 16) / r1;
			jaguar.ctrl[D_REMAINDER] = ((UINT64)r2 << 16) % r1;
		}
		else
		{
			jaguar.r[dreg] = r2 / r1;
			jaguar.ctrl[D_REMAINDER] = r2 % r1;
		}
	}
	else
		jaguar.r[dreg] = 0xffffffff;
}

void illegal(void)
{
}

void imacn_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[jaguar.op & 31];
	jaguar.accum += (INT64)((INT16)r1 * (INT16)r2);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected IMACN instruction!\n");
}

void imult_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = (INT16)r1 * (INT16)r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void imultn_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = (INT16)r1 * (INT16)r2;
	jaguar.accum = (INT32)res;
	CLR_ZN; SET_ZN(res);

	jaguar.op = ROPCODE(jaguar.PC);
	while ((jaguar.op >> 10) == 20)
	{
		r1 = jaguar.r[(jaguar.op >> 5) & 31];
		r2 = jaguar.r[jaguar.op & 31];
		jaguar.accum += (INT64)((INT16)r1 * (INT16)r2);
		jaguar.PC += 2;
		jaguar.op = ROPCODE(jaguar.PC);
	}
	if ((jaguar.op >> 10) == 19)
	{
		jaguar.PC += 2;
		jaguar.r[jaguar.op & 31] = (UINT32)jaguar.accum;
	}
}

void jr_cc_n(void)
{
	if (CONDITION(jaguar.op & 31))
	{
		INT32 r1 = (INT8)((jaguar.op >> 2) & 0xf8) >> 2;
		UINT32 newpc = jaguar.PC + r1;
		CALL_MAME_DEBUG;
		jaguar.op = ROPCODE(jaguar.PC);
		jaguar.PC = newpc;
		(*jaguar.table[jaguar.op >> 10])();

		jaguar_icount -= 3;	/* 3 wait states guaranteed */
	}
}

void jump_cc_rn(void)
{
	if (CONDITION(jaguar.op & 31))
	{
		UINT8 reg = (jaguar.op >> 5) & 31;

		/* special kludge for risky code in the cojag DSP interrupt handlers */
		UINT32 newpc = (jaguar_icount == bankswitch_icount) ? jaguar.a[reg] : jaguar.r[reg];
		CALL_MAME_DEBUG;
		jaguar.op = ROPCODE(jaguar.PC);
		jaguar.PC = newpc;
		(*jaguar.table[jaguar.op >> 10])();

		jaguar_icount -= 3;	/* 3 wait states guaranteed */
	}
}

void load_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READLONG(r1);
}

void load_r14n_rn(void)
{
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READLONG(jaguar.r[14] + 4 * r1);
}

void load_r15n_rn(void)
{
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READLONG(jaguar.r[15] + 4 * r1);
}

void load_r14rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READLONG(jaguar.r[14] + r1);
}

void load_r15rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READLONG(jaguar.r[15] + r1);
}

void loadb_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READBYTE(r1);
}

void loadw_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = READWORD(r1);
}

void loadp_rn_rn(void)	/* GPU only */
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.ctrl[G_HIDATA] = READWORD(r1);
	jaguar.r[jaguar.op & 31] = READWORD(r1+4);
}

void mirror_rn(void)	/* DSP only */
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[dreg];
	UINT32 res = (mirror_table[r1 & 0xffff] << 16) | mirror_table[r1 >> 16];
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void mmult_rn_rn(void)
{
	int count = jaguar.ctrl[G_MTXC] & 15, i;
	int sreg = (jaguar.op >> 5) & 31;
	int dreg = jaguar.op & 31;
	UINT32 addr = jaguar.ctrl[G_MTXA];
	INT64 accum = 0;
	UINT32 res;

	if (!(jaguar.ctrl[G_MTXC] & 0x10))
	{
		for (i = 0; i < count; i++)
		{
			accum += (INT16)(jaguar.b1[sreg + i/2] >> (16 * ((i & 1) ^ 1))) * (INT16)READWORD(addr);
			addr += 2;
		}
	}
	else
	{
		for (i = 0; i < count; i++)
		{
			accum += (INT16)(jaguar.b1[sreg + i/2] >> (16 * ((i & 1) ^ 1))) * (INT16)READWORD(addr);
			addr += 2 * count;
		}
	}
	jaguar.r[dreg] = res = (UINT32)accum;
	CLR_ZN; SET_ZN(res);
}

void move_rn_rn(void)
{
	jaguar.r[jaguar.op & 31] = jaguar.r[(jaguar.op >> 5) & 31];
}

void move_pc_rn(void)
{
	jaguar.r[jaguar.op & 31] = jaguar.ppc;
}

void movefa_rn_rn(void)
{
	jaguar.r[jaguar.op & 31] = jaguar.a[(jaguar.op >> 5) & 31];
}

void movei_n_rn(void)
{
	UINT32 res = ROPCODE(jaguar.PC) | (ROPCODE(jaguar.PC + 2) << 16);
	jaguar.PC += 4;
	jaguar.r[jaguar.op & 31] = res;
}

void moveq_n_rn(void)
{
	jaguar.r[jaguar.op & 31] = (jaguar.op >> 5) & 31;
}

void moveta_rn_rn(void)
{
	jaguar.a[jaguar.op & 31] = jaguar.r[(jaguar.op >> 5) & 31];
}

void mtoi_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	jaguar.r[jaguar.op & 31] = (((INT32)r1 >> 8) & 0xff800000) | (r1 & 0x007fffff);
}

void mult_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = (UINT16)r1 * (UINT16)r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void neg_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = -r2;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_SUB(0,r2,res);
}

void nop(void)
{
}

void normi_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 res = 0;
	if (r1 != 0)
	{
		while ((r1 & 0xffc00000) == 0)
		{
			r1 <<= 1;
			res--;
		}
		while ((r1 & 0xff800000) != 0)
		{
			r1 >>= 1;
			res++;
		}
	}
	jaguar.r[jaguar.op & 31] = res;
	CLR_ZN; SET_ZN(res);
}

void not_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 res = ~jaguar.r[dreg];
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void or_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r1 | r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void pack_rn(void)		/* GPU only */
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res;
	if (r1 == 0)	/* PACK */
		res = ((r2 >> 10) & 0xf000) | ((r2 >> 5) & 0x0f00) | (r2 & 0xff);
	else			/* UNPACK */
		res = ((r2 & 0xf000) << 10) | ((r2 & 0x0f00) << 5) | (r2 & 0xff);
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void resmac_rn(void)
{
	jaguar.r[jaguar.op & 31] = (UINT32)jaguar.accum;
}

void ror_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31] & 31;
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = (r2 >> r1) | (r2 << (32 - r1));
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZN(res); jaguar.FLAGS |= (r2 >> 30) & 2;
}

void rorq_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = (r2 >> r1) | (r2 << (32 - r1));
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZN(res); jaguar.FLAGS |= (r2 >> 30) & 2;
}

void sat8_rn(void)		/* GPU only */
{
	int dreg = jaguar.op & 31;
	INT32 r2 = jaguar.r[dreg];
	UINT32 res = (r2 < 0) ? 0 : (r2 > 255) ? 255 : r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void sat16_rn(void)		/* GPU only */
{
	int dreg = jaguar.op & 31;
	INT32 r2 = jaguar.r[dreg];
	UINT32 res = (r2 < 0) ? 0 : (r2 > 65535) ? 65535 : r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void sat16s_rn(void)		/* DSP only */
{
    UINT32 res;
	int dreg = jaguar.op & 31;
	INT32 r2 = jaguar.r[dreg];
   MAME_CLAMP_SAMPLE(r2);
   res = r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void sat24_rn(void)			/* GPU only */
{
	int dreg = jaguar.op & 31;
	INT32 r2 = jaguar.r[dreg];
	UINT32 res = (r2 < 0) ? 0 : (r2 > 16777215) ? 16777215 : r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void sat32s_rn(void)		/* DSP only */
{
	int dreg = jaguar.op & 31;
	INT32 r2 = (UINT32)jaguar.r[dreg];
	INT32 temp = jaguar.accum >> 32;
	UINT32 res = (temp < -1) ? (INT32)0x80000000 : (temp > 0) ? (INT32)0x7fffffff : r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}

void sh_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	INT32 r1 = (INT32)jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res;

	CLR_ZNC;
	if (r1 < 0)
	{
		res = (r1 <= -32) ? 0 : (r2 << -r1);
		jaguar.FLAGS |= (r2 >> 30) & 2;
	}
	else
	{
		res = (r1 >= 32) ? 0 : (r2 >> r1);
		jaguar.FLAGS |= (r2 << 1) & 2;
	}
	jaguar.r[dreg] = res;
	SET_ZN(res);
}

void sha_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	INT32 r1 = (INT32)jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res;

	CLR_ZNC;
	if (r1 < 0)
	{
		res = (r1 <= -32) ? 0 : (r2 << -r1);
		jaguar.FLAGS |= (r2 >> 30) & 2;
	}
	else
	{
		res = (r1 >= 32) ? ((INT32)r2 >> 31) : ((INT32)r2 >> r1);
		jaguar.FLAGS |= (r2 << 1) & 2;
	}
	jaguar.r[dreg] = res;
	SET_ZN(res);
}

void sharq_n_rn(void)
{
	int dreg = jaguar.op & 31;
	INT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = (INT32)r2 >> r1;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZN(res); jaguar.FLAGS |= (r2 << 1) & 2;
}

void shlq_n_rn(void)
{
	int dreg = jaguar.op & 31;
	INT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 << (32 - r1);
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZN(res); jaguar.FLAGS |= (r2 >> 30) & 2;
}

void shrq_n_rn(void)
{
	int dreg = jaguar.op & 31;
	INT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 >> r1;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZN(res); jaguar.FLAGS |= (r2 << 1) & 2;
}

void store_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	WRITELONG(r1, jaguar.r[jaguar.op & 31]);
}

void store_rn_r14n(void)
{
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	WRITELONG(jaguar.r[14] + r1 * 4, jaguar.r[jaguar.op & 31]);
}

void store_rn_r15n(void)
{
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	WRITELONG(jaguar.r[15] + r1 * 4, jaguar.r[jaguar.op & 31]);
}

void store_rn_r14rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	WRITELONG(jaguar.r[14] + r1, jaguar.r[jaguar.op & 31]);
}

void store_rn_r15rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	WRITELONG(jaguar.r[15] + r1, jaguar.r[jaguar.op & 31]);
}

void storeb_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	WRITEBYTE(r1, jaguar.r[jaguar.op & 31]);
}

void storew_rn_rn(void)
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	WRITEWORD(r1, jaguar.r[jaguar.op & 31]);
}

void storep_rn_rn(void)	/* GPU only */
{
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	WRITELONG(r1, jaguar.ctrl[G_HIDATA]);
	WRITELONG(r1+4, jaguar.r[jaguar.op & 31]);
}

void sub_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 - r1;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_SUB(r2,r1,res);
}

void subc_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 - r1 - ((jaguar.FLAGS >> 1) & 1);
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_SUB(r2,r1,res);
}

void subq_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 - r1;
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_SUB(r2,r1,res);
}

void subqmod_n_rn(void)	/* DSP only */
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 - r1;
	res = (res & ~jaguar.ctrl[D_MOD]) | (r2 & ~jaguar.ctrl[D_MOD]);
	jaguar.r[dreg] = res;
	CLR_ZNC; SET_ZNC_SUB(r2,r1,res);
}

void subqt_n_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = convert_zero[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r2 - r1;
	jaguar.r[dreg] = res;
}

void xor_rn_rn(void)
{
	int dreg = jaguar.op & 31;
	UINT32 r1 = jaguar.r[(jaguar.op >> 5) & 31];
	UINT32 r2 = jaguar.r[dreg];
	UINT32 res = r1 ^ r2;
	jaguar.r[dreg] = res;
	CLR_ZN; SET_ZN(res);
}



/*###################################################################################################
**	I/O HANDLING
**#################################################################################################*/

data32_t jaguargpu_ctrl_r(int cpunum, offs_t offset)
{
	data32_t result;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X/%d:GPU read register @ F021%02X\n", activecpu_get_previouspc(), cpu_getactivecpu(), offset * 4);

	/* switch to the target context */
	cpuintrf_push_context(cpunum);
	result = jaguar.ctrl[offset];
	cpuintrf_pop_context();

	return result;
}


void jaguargpu_ctrl_w(int cpunum, offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT32 			oldval, newval;

	if (offset != G_HIDATA)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X/%d:GPU write register @ F021%02X = %08X\n", activecpu_get_previouspc(), cpu_getactivecpu(), offset * 4, data);

	/* switch to the target context */
	cpuintrf_push_context(cpunum);

	/* remember the old and set the new */
	oldval = jaguar.ctrl[offset];
	newval = oldval;
	COMBINE_DATA(&newval);

	/* handle the various registers */
	switch (offset)
	{
		case G_FLAGS:

			/* combine the data properly */
			jaguar.ctrl[offset] = newval & (ZFLAG | CFLAG | NFLAG | EINT04FLAGS | RPAGEFLAG);
			if (newval & IFLAG)
				jaguar.ctrl[offset] |= oldval & IFLAG;

			/* clear interrupts */
			jaguar.ctrl[G_CTRL] &= ~((newval & CINT04FLAGS) >> 3);

			/* determine which register bank should be active */
			update_register_banks();

			/* update IRQs */
			check_irqs();
			break;

		case G_MTXC:
		case G_MTXA:
			jaguar.ctrl[offset] = newval;
			break;

		case G_END:
			jaguar.ctrl[offset] = newval;
			if ((newval & 7) != 7)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "GPU to set to little-endian!\n");
			break;

		case G_PC:
			jaguar.PC = newval & 0xffffff;
			if (executing_cpu == cpunum)
				change_pc24bedw(jaguar.PC);
			break;

		case G_CTRL:
			jaguar.ctrl[offset] = newval;
			if ((oldval ^ newval) & 0x01)
			{
				cpu_set_halt_line(cpunum, (newval & 1) ? CLEAR_LINE : ASSERT_LINE);
				cpu_yield();
			}
			if (newval & 0x02)
			{
				if (jaguar.cpu_interrupt)
					(*jaguar.cpu_interrupt)();
				jaguar.ctrl[offset] &= ~0x02;
			}
			if (newval & 0x04)
			{
				jaguar.ctrl[G_CTRL] |= 1 << 6;
				jaguar.ctrl[offset] &= ~0x04;
				check_irqs();
			}
			if (newval & 0x18)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "GPU single stepping was enabled!\n");
			}
			break;

		case G_HIDATA:
		case G_DIVCTRL:
			jaguar.ctrl[offset] = newval;
			break;
	}

	/* restore old context */
	cpuintrf_pop_context();
}



/*###################################################################################################
**	I/O HANDLING
**#################################################################################################*/

data32_t jaguardsp_ctrl_r(int cpunum, offs_t offset)
{
	data32_t result;

	if (offset != D_FLAGS)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X/%d:DSP read register @ F1A1%02X\n", activecpu_get_previouspc(), cpu_getactivecpu(), offset * 4);

	/* switch to the target context */
	cpuintrf_push_context(cpunum);
	result = jaguar.ctrl[offset];
	cpuintrf_pop_context();

	return result;
}


void jaguardsp_ctrl_w(int cpunum, offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT32 			oldval, newval;

	if (offset != D_FLAGS)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X/%d:DSP write register @ F1A1%02X = %08X\n", activecpu_get_previouspc(), cpu_getactivecpu(), offset * 4, data);

	/* switch to the target context */
	cpuintrf_push_context(cpunum);

	/* remember the old and set the new */
	oldval = jaguar.ctrl[offset];
	newval = oldval;
	COMBINE_DATA(&newval);

	/* handle the various registers */
	switch (offset)
	{
		case D_FLAGS:

			/* combine the data properly */
			jaguar.ctrl[offset] = newval & (ZFLAG | CFLAG | NFLAG | EINT04FLAGS | EINT5FLAG | RPAGEFLAG);
			if (newval & IFLAG)
				jaguar.ctrl[offset] |= oldval & IFLAG;

			/* clear interrupts */
			jaguar.ctrl[D_CTRL] &= ~((newval & CINT04FLAGS) >> 3);
			jaguar.ctrl[D_CTRL] &= ~((newval & CINT5FLAG) >> 1);

			/* determine which register bank should be active */
			update_register_banks();

			/* update IRQs */
			check_irqs();
			break;

		case D_MTXC:
		case D_MTXA:
			jaguar.ctrl[offset] = newval;
			break;

		case D_END:
			jaguar.ctrl[offset] = newval;
			if ((newval & 7) != 7)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP to set to little-endian!\n");
			break;

		case D_PC:
			jaguar.PC = newval & 0xffffff;
			if (executing_cpu == cpunum)
				change_pc24bedw(jaguar.PC);
			break;

		case D_CTRL:
			jaguar.ctrl[offset] = newval;
			if ((oldval ^ newval) & 0x01)
			{
				cpu_set_halt_line(cpunum, (newval & 1) ? CLEAR_LINE : ASSERT_LINE);
				cpu_yield();
			}
			if (newval & 0x02)
			{
				if (jaguar.cpu_interrupt)
					(*jaguar.cpu_interrupt)();
				jaguar.ctrl[offset] &= ~0x02;
			}
			if (newval & 0x04)
			{
				jaguar.ctrl[D_CTRL] |= 1 << 6;
				jaguar.ctrl[offset] &= ~0x04;
				check_irqs();
			}
			if (newval & 0x18)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP single stepping was enabled!\n");
			}
			break;

		case D_MOD:
		case D_DIVCTRL:
			jaguar.ctrl[offset] = newval;
			break;
	}

	/* restore old context */
	cpuintrf_pop_context();
}
