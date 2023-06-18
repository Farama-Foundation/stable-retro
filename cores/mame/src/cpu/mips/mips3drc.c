/*###################################################################################################
**
**
**		mips3drc.c
**		x86 Dynamic recompiler for MIPS III/IV emulator.
**		Written by Aaron Giles
**
**
**		Philosophy: this is intended to be a very basic implementation of a dynamic compiler in
**		order to keep things simple. There are certainly more optimizations that could be added
**		but for now, we keep it strictly to NOP stripping and LUI optimizations.
**
**
**		Still not implemented:
**			* MMU (it is logged, however)
**
**
**#################################################################################################*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "driver.h"
#include "mamedbg.h"
#include "mips3.h"
#include "x86drc.h"

/*

	Future optimizations:
	Don't save EDI/ESI across function calls?

*/



/*###################################################################################################
**	CONFIGURATION
**#################################################################################################*/

#ifdef MAME_DEBUG
#define LOG_CODE			0
#else
#define LOG_CODE			0
#endif

#define STRIP_NOPS			1

#define USE_SSE				0

#define CACHE_SIZE			(8 * 1024 * 1024)
#define MAX_INSTRUCTIONS	512



/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* recompiler flags */
#define RECOMPILE_UNIMPLEMENTED			0x0000
#define RECOMPILE_SUCCESSFUL			0x0001
#define RECOMPILE_SUCCESSFUL_CP(c,p)	(RECOMPILE_SUCCESSFUL | (((c) & 0xff) << 16) | (((p) & 0xff) << 24))
#define RECOMPILE_MAY_CAUSE_EXCEPTION	0x0002
#define RECOMPILE_END_OF_STRING			0x0004
#define RECOMPILE_CHECK_INTERRUPTS		0x0008
#define RECOMPILE_ADD_DISPATCH			0x0010

/* COP0 registers */
#define COP0_Index			0
#define COP0_Random			1
#define COP0_EntryLo		2
#define COP0_EntryLo0		2
#define COP0_EntryLo1		3
#define COP0_Context		4
#define COP0_PageMask		5
#define COP0_Wired			6
#define COP0_BadVAddr		8
#define COP0_Count			9
#define COP0_EntryHi		10
#define COP0_Compare		11
#define COP0_Status			12
#define COP0_Cause			13
#define COP0_EPC			14
#define COP0_PRId			15
#define COP0_XContext		20

/* Status register bits */
#define SR_IE				0x00000001
#define SR_EXL				0x00000002
#define SR_ERL				0x00000004
#define SR_KSU_MASK			0x00000018
#define SR_KSU_KERNEL		0x00000000
#define SR_KSU_SUPERVISOR	0x00000008
#define SR_KSU_USER			0x00000010
#define SR_IMSW0			0x00000100
#define SR_IMSW1			0x00000200
#define SR_IMEX0			0x00000400
#define SR_IMEX1			0x00000800
#define SR_IMEX2			0x00001000
#define SR_IMEX3			0x00002000
#define SR_IMEX4			0x00004000
#define SR_IMEX5			0x00008000
#define SR_DE				0x00010000
#define SR_CE				0x00020000
#define SR_CH				0x00040000
#define SR_SR				0x00100000
#define SR_TS				0x00200000
#define SR_BEV				0x00400000
#define SR_RE				0x02000000
#define SR_COP0				0x10000000
#define SR_COP1				0x20000000
#define SR_COP2				0x40000000
#define SR_COP3				0x80000000

/* exception types */
#define EXCEPTION_INTERRUPT	0
#define EXCEPTION_TLBMOD	1
#define EXCEPTION_TLBLOAD	2
#define EXCEPTION_TLBSTORE	3
#define EXCEPTION_ADDRLOAD	4
#define EXCEPTION_ADDRSTORE	5
#define EXCEPTION_BUSINST	6
#define EXCEPTION_BUSDATA	7
#define EXCEPTION_SYSCALL	8
#define EXCEPTION_BREAK		9
#define EXCEPTION_INVALIDOP	10
#define EXCEPTION_BADCOP	11
#define EXCEPTION_OVERFLOW	12
#define EXCEPTION_TRAP		13



/*###################################################################################################
**	HELPER MACROS
**#################################################################################################*/

#define RSREG		((op >> 21) & 31)
#define RTREG		((op >> 16) & 31)
#define RDREG		((op >> 11) & 31)
#define SHIFT		((op >> 6) & 31)

#define FRREG		((op >> 21) & 31)
#define FTREG		((op >> 16) & 31)
#define FSREG		((op >> 11) & 31)
#define FDREG		((op >> 6) & 31)

#define IS_SINGLE(o) (((o) & (1 << 21)) == 0)
#define IS_DOUBLE(o) (((o) & (1 << 21)) != 0)
#define IS_FLOAT(o) (((o) & (1 << 23)) == 0)
#define IS_INTEGRAL(o) (((o) & (1 << 23)) != 0)

#define SIMMVAL		((INT16)op)
#define UIMMVAL		((UINT16)op)
#define LIMMVAL		(op & 0x03ffffff)



/*###################################################################################################
**	STRUCTURES & TYPEDEFS
**#################################################################################################*/

/* memory access function table */
typedef struct
{
	data8_t		(*readbyte)(offs_t);
	data16_t	(*readword)(offs_t);
	data32_t	(*readlong)(offs_t);
	void		(*writebyte)(offs_t, data8_t);
	void		(*writeword)(offs_t, data16_t);
	void		(*writelong)(offs_t, data32_t);
} memory_handlers;


/* MIPS3 Registers */
typedef struct
{
	/* core registers */
	UINT32		pc;
	UINT64		hi;
	UINT64		lo;
	UINT64		r[32];

	/* COP registers */
	UINT64		cpr[4][32];
	UINT64		ccr[4][32];
	UINT8		cf[4][8];

	/* internal stuff */
	struct drccore *drc;
	UINT32		drcoptions;
	UINT32		nextpc;
	int 		(*irq_callback)(int irqline);
	UINT64		count_zero_time;
	void *		compare_int_timer;
	UINT8		is_mips4;

	/* memory accesses */
	UINT8		bigendian;
	memory_handlers memory;

	/* cache memory */
	data32_t *	icache;
	data32_t *	dcache;
	size_t		icache_size;
	size_t		dcache_size;
	
	/* callbacks */
	void *		generate_interrupt_exception;
	void *		generate_cop_exception;
	void *		generate_overflow_exception;
	void *		generate_invalidop_exception;
	void *		generate_syscall_exception;
	void *		generate_break_exception;
	void *		generate_trap_exception;
} mips3_regs;



/*###################################################################################################
**	PROTOTYPES
**#################################################################################################*/

static void mips3drc_reset(struct drccore *drc);
static void mips3drc_recompile(struct drccore *drc);
static void mips3drc_entrygen(struct drccore *drc);

static UINT32 compile_one(struct drccore *drc, UINT32 pc);

static void append_generate_exception(struct drccore *drc, UINT8 exception);
static void append_update_cycle_counting(struct drccore *drc);
static void append_check_interrupts(struct drccore *drc, int inline_generate);

static UINT32 recompile_instruction(struct drccore *drc, UINT32 pc);
static UINT32 recompile_special(struct drccore *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_regimm(struct drccore *drc, UINT32 pc, UINT32 op);

static UINT32 recompile_cop0(struct drccore *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_cop1(struct drccore *drc, UINT32 pc, UINT32 op);
static UINT32 recompile_cop1x(struct drccore *drc, UINT32 pc, UINT32 op);

static void update_cycle_counting(void);



/*###################################################################################################
**	PUBLIC GLOBAL VARIABLES
**#################################################################################################*/

int	mips3_icount=50000;



/*###################################################################################################
**	PRIVATE GLOBAL VARIABLES
**#################################################################################################*/

static mips3_regs mips3;

static UINT64 dmult_temp1;
static UINT64 dmult_temp2;

#ifdef MAME_DEBUG
static FILE *symfile;
#endif

static UINT8 in_delay_slot = 0;
static UINT32 drcoptions[MAX_CPU];

static UINT32 scratchspace[10];

/*
static void **ram_read_table;
static void **ram_write_table;
*/

static const memory_handlers be_memory =
{
	cpu_readmem32bedw,  cpu_readmem32bedw_word,  cpu_readmem32bedw_dword,
	cpu_writemem32bedw, cpu_writemem32bedw_word, cpu_writemem32bedw_dword
};

static const memory_handlers le_memory =
{
	cpu_readmem32ledw,  cpu_readmem32ledw_word,  cpu_readmem32ledw_dword,
	cpu_writemem32ledw, cpu_writemem32ledw_word, cpu_writemem32ledw_dword
};



/*###################################################################################################
**	IRQ HANDLING
**#################################################################################################*/

void mips3_set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE)
		mips3.cpr[0][COP0_Cause] |= 0x400 << irqline;
	else
		mips3.cpr[0][COP0_Cause] &= ~(0x400 << irqline);
}


void mips3_set_irq_callback(int (*callback)(int irqline))
{
	mips3.irq_callback = callback;
}



/*###################################################################################################
**	CONTEXT SWITCHING
**#################################################################################################*/

unsigned mips3_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(mips3_regs *)dst = mips3;

	/* return the context size */
	return sizeof(mips3_regs);
}


void mips3_set_context(void *src)
{
	/* copy the context */
	if (src)
		mips3 = *(mips3_regs *)src;
}



/*###################################################################################################
**	INITIALIZATION AND SHUTDOWN
**#################################################################################################*/

static void compare_int_callback(int cpu)
{
	cpu_set_irq_line(cpu, 5, ASSERT_LINE);
}


void mips3_init(void)
{
	struct drcconfig drconfig;
	
	/* fill in the config */
	memset(&drconfig, 0, sizeof(drconfig));
	drconfig.cache_size       = CACHE_SIZE;
	drconfig.max_instructions = MAX_INSTRUCTIONS;
	drconfig.address_bits     = 32;
	drconfig.lsbs_to_ignore   = 2;
	drconfig.uses_fp          = 1;
	drconfig.uses_sse         = USE_SSE;
	drconfig.pcptr            = (UINT32 *)&mips3.pc;
	drconfig.icountptr        = (UINT32 *)&mips3_icount;
	drconfig.esiptr           = NULL;
	drconfig.cb_reset         = mips3drc_reset;
	drconfig.cb_recompile     = mips3drc_recompile;
	drconfig.cb_entrygen      = mips3drc_entrygen;
	
	/* initialize the compiler */
	mips3.drc = drc_init(cpu_getactivecpu(), &drconfig);
	mips3.drcoptions = MIPS3DRC_FASTEST_OPTIONS;

	/* allocate a timer */
	mips3.compare_int_timer = timer_alloc(compare_int_callback);
}


static void mips3_reset(void *param, int bigendian)
{
	struct mips3_config *config = param;

	/* allocate memory */
	mips3.icache = malloc(config->icache);
	mips3.dcache = malloc(config->dcache);
	if (!mips3.icache || !mips3.dcache)
	{
		fprintf(stderr, "error: couldn't allocate cache for mips3!\n");
		exit(1);
	}

	/* set up the endianness */
	mips3.bigendian = bigendian;
	if (mips3.bigendian)
		mips3.memory = be_memory;
	else
		mips3.memory = le_memory;

	/* initialize the rest of the config */
	mips3.icache_size = config->icache;
	mips3.dcache_size = config->dcache;

	/* initialize the state */
	mips3.pc = 0xbfc00000;
	mips3.nextpc = ~0;
	mips3.cpr[0][COP0_Status] = SR_BEV | SR_ERL;
	mips3.cpr[0][COP0_Compare] = 0xffffffff;
	mips3.cpr[0][COP0_Count] = 0;
	mips3.count_zero_time = activecpu_gettotalcycles64();
	
	/* reset the DRC */
	mips3.drcoptions = drcoptions[cpu_getactivecpu()];
	drc_cache_reset(mips3.drc);
}


#if HAS_R4600
void r4600be_reset(void *param)
{
	mips3_reset(param, 1);
	mips3.cpr[0][COP0_PRId] = 0x2000;
	mips3.is_mips4 = 0;
}

void r4600le_reset(void *param)
{
	mips3_reset(param, 0);
	mips3.cpr[0][COP0_PRId] = 0x2000;
	mips3.is_mips4 = 0;
}
#endif


#if HAS_R5000
void r5000be_reset(void *param)
{
	mips3_reset(param, 1);
	mips3.cpr[0][COP0_PRId] = 0x2300;
	mips3.is_mips4 = 1;
}

void r5000le_reset(void *param)
{
	mips3_reset(param, 0);
	mips3.cpr[0][COP0_PRId] = 0x2300;
	mips3.is_mips4 = 1;
}
#endif


void mips3drc_set_options(UINT8 cpunum, UINT32 opts)
{
	drcoptions[cpunum] = opts;
}



int mips3_execute(int cycles)
{
	/* update the cycle timing */
	update_cycle_counting();

	/* count cycles and interrupt cycles */
	mips3_icount = cycles;
	drc_execute(mips3.drc);
	return cycles - mips3_icount;
}


void mips3_exit(void)
{
	/* free cache memory */
	if (mips3.icache)
		free(mips3.icache);
	mips3.icache = NULL;

	if (mips3.dcache)
		free(mips3.dcache);
	mips3.dcache = NULL;

#ifdef MAME_DEBUG
	if (symfile) fclose(symfile);
#endif
	drc_exit(mips3.drc);
}



/*###################################################################################################
**	RECOMPILER CALLBACKS
**#################################################################################################*/

/*------------------------------------------------------------------
	mips3drc_reset
------------------------------------------------------------------*/

static void mips3drc_reset(struct drccore *drc)
{
	mips3.generate_interrupt_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INTERRUPT);
	
	mips3.generate_cop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BADCOP);
		
	mips3.generate_overflow_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_OVERFLOW);
		
	mips3.generate_invalidop_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_INVALIDOP);
		
	mips3.generate_syscall_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_SYSCALL);
		
	mips3.generate_break_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_BREAK);
		
	mips3.generate_trap_exception = drc->cache_top;
	append_generate_exception(drc, EXCEPTION_TRAP);
}



/*------------------------------------------------------------------
	update_cycle_counting
------------------------------------------------------------------*/

static void update_cycle_counting(void)
{
	/* modify the timer to go off */
	if ((mips3.cpr[0][COP0_Status] & 0x8000) && mips3.cpr[0][COP0_Compare] != 0xffffffff)
	{
		UINT32 count = (activecpu_gettotalcycles64() - mips3.count_zero_time) / 2;
		UINT32 compare = mips3.cpr[0][COP0_Compare];
		UINT32 cyclesleft = compare - count;
		double newtime = TIME_IN_CYCLES(((UINT64)cyclesleft * 2), cpu_getactivecpu());
		
		/* due to accuracy issues, don't bother setting timers unless they're for less than 100msec */
		if (newtime < TIME_IN_MSEC(100))
			timer_adjust(mips3.compare_int_timer, newtime, cpu_getactivecpu(), 0);
	}
	else
		timer_adjust(mips3.compare_int_timer, TIME_NEVER, cpu_getactivecpu(), 0);
}



/*------------------------------------------------------------------
	logtlbentry
------------------------------------------------------------------*/

static INLINE void logonetlbentry(int which)
{
	UINT64 hi = mips3.cpr[0][COP0_EntryHi];
	UINT64 lo = mips3.cpr[0][COP0_EntryLo0 + which];
	UINT32 vpn = (((hi >> 13) & 0x07ffffff) << 1) + which;
	UINT32 asid = hi & 0xff;
	UINT32 r = (hi >> 62) & 3;
	UINT32 pfn = (lo >> 6) & 0x00ffffff;
	UINT32 c = (lo >> 3) & 7;
	UINT32 pagesize = ((mips3.cpr[0][COP0_PageMask] >> 1) | 0xfff) + 1;
	UINT64 vaddr = (UINT64)vpn * (UINT64)pagesize;
	UINT64 paddr = (UINT64)pfn * (UINT64)pagesize;

	logerror("pagesize = %08X  vaddr = %08X%08X  paddr = %08X%08X  asid = %02X  r = %X  c = %X  dvg=%c%c%c\n",
			pagesize, (UINT32)(vaddr >> 32), (UINT32)vaddr, (UINT32)(paddr >> 32), (UINT32)paddr,
			asid, r, c, (lo & 4) ? 'd' : '.', (lo & 2) ? 'v' : '.', (lo & 1) ? 'g' : '.');
}

static void logtlbentry(void)
{
	logonetlbentry(0);
	logonetlbentry(1);
}



/*------------------------------------------------------------------
	mips3drc_recompile
------------------------------------------------------------------*/

static void mips3drc_recompile(struct drccore *drc)
{
	int remaining = MAX_INSTRUCTIONS;
	UINT32 pc = mips3.pc;
	
	log_cb(RETRO_LOG_DEBUG, LOGPRE "recompile_callback @ PC=%08X\n", mips3.pc);
/*
	if (!ram_read_table)
	{
		ram_read_table = malloc(65536 * sizeof(void *));
		ram_write_table = malloc(65536 * sizeof(void *));
		if (ram_read_table && ram_write_table)
			for (i = 0; i < 65536; i++)
			{
				ram_read_table[i] = memory_get_read_ptr(cpu_getactivecpu(), i << 16);
				if (ram_read_table[i]) ram_read_table[i] = (UINT8 *)ram_read_table[i] - (i << 16);
				ram_write_table[i] = memory_get_write_ptr(cpu_getactivecpu(), i << 16);
				if (ram_write_table[i]) ram_write_table[i] = (UINT8 *)ram_write_table[i] - (i << 16);
			}
	}
*/
	/* begin the sequence */
	drc_begin_sequence(drc, pc);
	
	/* loose verification case: one verification here only */
	if (!(mips3.drcoptions & MIPS3DRC_STRICT_VERIFY))
	{
		if (mips3.bigendian)
			change_pc32bedw(pc);
		else
			change_pc32ledw(pc);
		drc_append_verify_code(drc, (UINT32 *)&OP_ROM[pc], 4);
	}

	/* loop until we hit an unconditional branch */
	while (--remaining != 0)
	{
		UINT32 result;

		/* compile one instruction */
		result = compile_one(drc, pc);
		pc += (INT8)(result >> 24);
		if (result & RECOMPILE_END_OF_STRING)
			break;
	}
	
	/* add dispatcher just in case */
	if (remaining == 0)
		drc_append_dispatcher(drc);
	
	/* end the sequence */
	drc_end_sequence(drc);

#if LOG_CODE
{
	FILE *temp;
	temp = fopen("code.bin", "wb");
	fwrite(drc->cache_base, 1, drc->cache_top - drc->cache_base, temp);
	fclose(temp);
}
#endif
}


/*------------------------------------------------------------------
	mips3drc_entrygen
------------------------------------------------------------------*/

static void mips3drc_entrygen(struct drccore *drc)
{
	append_check_interrupts(drc, 1);
}



/*###################################################################################################
**	RECOMPILER CORE
**#################################################################################################*/

/*------------------------------------------------------------------
	compile_one
------------------------------------------------------------------*/

static UINT32 compile_one(struct drccore *drc, UINT32 pc)
{
	int pcdelta, cycles;
	UINT32 *opptr;
	UINT32 result;
	
	/* register this instruction */
	drc_register_code_at_cache_top(drc, pc);

	/* get a pointer to the current instruction */
	if (mips3.bigendian)
		change_pc32bedw(pc);
	else
		change_pc32ledw(pc);
	opptr = (UINT32 *)&OP_ROM[pc];
	
#ifdef MAME_DEBUG
{
	char temp[256];
	if (!symfile) symfile = fopen("code.sym", "w");
	mips3_dasm(temp, pc);
	fprintf(symfile, "%08X   --------------------------------------------\n", drc->cache_top - drc->cache_base);
	fprintf(symfile, "%08X   %08X: %s\n", drc->cache_top - drc->cache_base, pc, temp);
}
#endif
	
	/* emit debugging and self-modifying code checks */
	drc_append_call_debugger(drc);
	if (mips3.drcoptions & MIPS3DRC_STRICT_VERIFY)
		drc_append_verify_code(drc, opptr, 4);
	
	/* compile the instruction */
	result = recompile_instruction(drc, pc);

	/* handle the results */		
	if (!(result & RECOMPILE_SUCCESSFUL))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unimplemented op %08X (%02X,%02X)\n", *opptr, *opptr >> 26, *opptr & 0x3f);
		mips3_exit();
		exit(1);
	}
	pcdelta = (INT8)(result >> 24);
	cycles = (INT8)(result >> 16);
	
	/* absorb any NOPs following */
	#if (STRIP_NOPS)
	{
		if (!(result & (RECOMPILE_END_OF_STRING | RECOMPILE_CHECK_INTERRUPTS)))
			while (pcdelta < 120 && opptr[pcdelta/4] == 0)
			{
				pcdelta += 4;
				cycles += 1;
			}
	}
	#endif

	/* epilogue */
	drc_append_standard_epilogue(drc, cycles, pcdelta, 1);

	/* check interrupts */
	if (result & RECOMPILE_CHECK_INTERRUPTS)
		append_check_interrupts(drc, 0);
	if (result & RECOMPILE_ADD_DISPATCH)
		drc_append_dispatcher(drc);
	
	return (result & 0xffff) | ((UINT8)cycles << 16) | ((UINT8)pcdelta << 24);
}



/*###################################################################################################
**	COMMON ROUTINES
**#################################################################################################*/

/*------------------------------------------------------------------
	append_generate_exception
------------------------------------------------------------------*/

static void append_generate_exception(struct drccore *drc, UINT8 exception)
{
	UINT32 offset = (exception >= EXCEPTION_TLBMOD && exception <= EXCEPTION_TLBSTORE) ? 0x80 : 0x180;
	struct linkdata link1, link2;
	
	_mov_m32abs_r32(&mips3.cpr[0][COP0_EPC], REG_EDI);					/* mov	[mips3.cpr[0][COP0_EPC]],edi*/
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Cause]);				/* mov	eax,[mips3.cpr[0][COP0_Cause]]*/
	_and_r32_imm(REG_EAX, ~0x800000ff);									/* and	eax,~0x800000ff*/
	if (exception)
		_or_r32_imm(REG_EAX, exception << 2);							/* or	eax,exception << 2*/
	_cmp_m32abs_imm(&mips3.nextpc, ~0);									/* cmp	[mips3.nextpc],~0*/
	_jcc_short_link(COND_E, &link1);									/* je	skip*/
	_mov_m32abs_imm(&mips3.nextpc, ~0);									/* mov	[mips3.nextpc],~0*/
	_sub_m32abs_imm(&mips3.cpr[0][COP0_EPC], 4);						/* sub	[mips3.cpr[0][COP0_EPC]],4*/
	_or_r32_imm(REG_EAX, 0x80000000);									/* or	eax,0x80000000*/
	_resolve_link(&link1);												/* skip:*/
	_mov_m32abs_r32(&mips3.cpr[0][COP0_Cause], REG_EAX);				/* mov	[mips3.cpr[0][COP0_Cause]],eax*/
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);				/* mov	eax,[[mips3.cpr[0][COP0_Status]]*/
	_or_r32_imm(REG_EAX, SR_EXL);										/* or	eax,SR_EXL*/
	_test_r32_imm(REG_EAX, SR_BEV);										/* test	eax,SR_BEV*/
	_mov_m32abs_r32(&mips3.cpr[0][COP0_Status], REG_EAX);				/* mov	[[mips3.cpr[0][COP0_Status]],eax*/
	_mov_r32_imm(REG_EDI, 0xbfc00200 + offset);							/* mov	edi,0xbfc00200+offset*/
	_jcc_short_link(COND_NZ, &link2);									/* jnz	skip2*/
	_mov_r32_imm(REG_EDI, 0x80000000 + offset);							/* mov	edi,0x80000000+offset*/
	_resolve_link(&link2);												/* skip2:*/
	drc_append_dispatcher(drc);											/* dispatch*/
}


/*------------------------------------------------------------------
	append_update_cycle_counting
------------------------------------------------------------------*/

static void append_update_cycle_counting(struct drccore *drc)
{
	_mov_m32abs_r32(&mips3_icount, REG_EBP);							/* mov	[mips3_icount],ebp*/
	_call((void *)update_cycle_counting);								/* call	update_cycle_counting*/
	_mov_r32_m32abs(REG_EBP, &mips3_icount);							/* mov	ebp,[mips3_icount]*/
}


/*------------------------------------------------------------------
	append_check_interrupts
------------------------------------------------------------------*/

static void append_check_interrupts(struct drccore *drc, int inline_generate)
{
	struct linkdata link1, link2, link3;
	_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Cause]);				/* mov	eax,[mips3.cpr[0][COP0_Cause]]*/
	_and_r32_m32abs(REG_EAX, &mips3.cpr[0][COP0_Status]);				/* and	eax,[mips3.cpr[0][COP0_Status]]*/
	_and_r32_imm(REG_EAX, 0xff00);										/* and	eax,0xff00*/
	if (!inline_generate)
		_jcc_short_link(COND_Z, &link1);								/* jz	skip*/
	else
		_jcc_near_link(COND_Z, &link1);									/* jz	skip*/
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_IE);				/* test	[mips3.cpr[0][COP0_Status],SR_IE*/
	if (!inline_generate)
		_jcc_short_link(COND_Z, &link2);								/* jz	skip*/
	else
		_jcc_near_link(COND_Z, &link2);									/* jz	skip*/
	_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_EXL | SR_ERL);		/* test	[mips3.cpr[0][COP0_Status],SR_EXL | SR_ERL*/
	if (!inline_generate)
		_jcc(COND_Z, mips3.generate_interrupt_exception);				/* jz	generate_interrupt_exception*/
	else
	{
		_jcc_near_link(COND_NZ, &link3);								/* jnz	skip*/
		append_generate_exception(drc, EXCEPTION_INTERRUPT);			/* <generate exception>*/
		_resolve_link(&link3);											/* skip:*/
	}
	_resolve_link(&link1);												/* skip:*/
	_resolve_link(&link2);
}


/*------------------------------------------------------------------
	append_branch_or_dispatch
------------------------------------------------------------------*/

static void append_branch_or_dispatch(struct drccore *drc, UINT32 newpc, int cycles)
{
	void *code = drc_get_code_at_pc(drc, newpc);
	_mov_r32_imm(REG_EDI, newpc);
	drc_append_standard_epilogue(drc, cycles, 0, 1);

	if (code)
		_jmp(code);
	else
		drc_append_tentative_fixed_dispatcher(drc, newpc);
}



/*###################################################################################################
**	USEFUL PRIMITIVES
**#################################################################################################*/

#define _zero_m64abs(addr) 							\
do { 												\
	if (USE_SSE) 									\
	{												\
		_pxor_r128_r128(REG_XMM0, REG_XMM0);		\
		_movsd_m64abs_r128(addr, REG_XMM0);			\
	}												\
	else											\
		_mov_m64abs_imm32(addr, 0);					\
} while (0)											\

#define _mov_m64abs_m64abs(dst, src)				\
do { 												\
	if (USE_SSE) 									\
	{												\
		_movsd_r128_m64abs(REG_XMM0, src);			\
		_movsd_m64abs_r128(dst, REG_XMM0);			\
	}												\
	else											\
	{												\
		_mov_r64_m64abs(REG_EDX, REG_EAX, src);		\
		_mov_m64abs_r64(dst, REG_EDX, REG_EAX);		\
	}												\
} while (0)											\


/*###################################################################################################
**	CORE RECOMPILATION
**#################################################################################################*/

static void ddiv(INT64 *rs, INT64 *rt)
{
	if (*rt)
	{
		mips3.lo = *rs / *rt;
		mips3.hi = *rs % *rt;
	}
}

static void ddivu(UINT64 *rs, UINT64 *rt)
{
	if (*rt)
	{
		mips3.lo = *rs / *rt;
		mips3.hi = *rs % *rt;
	}
}


/*------------------------------------------------------------------
	recompile_delay_slot
------------------------------------------------------------------*/

static int recompile_delay_slot(struct drccore *drc, UINT32 pc)
{
	UINT8 *saved_top = drc->cache_top;
	UINT32 result;

	/* recompile the instruction as-is */
	in_delay_slot = 1;
	result = recompile_instruction(drc, pc);								/* <next instruction>*/
	in_delay_slot = 0;

	/* if the instruction can cause an exception, recompile setting nextpc */
	if (result & RECOMPILE_MAY_CAUSE_EXCEPTION)
	{
		drc->cache_top = saved_top;
		_mov_m32abs_imm(&mips3.nextpc, 0);									/* bogus nextpc for exceptions*/
		result = recompile_instruction(drc, pc);							/* <next instruction>*/
		_mov_m32abs_imm(&mips3.nextpc, ~0);									/* reset nextpc*/
	}

	return (INT8)(result >> 16);
}


/*------------------------------------------------------------------
	recompile_lui
------------------------------------------------------------------*/

static UINT32 recompile_lui(struct drccore *drc, UINT32 pc, UINT32 op)
{
	UINT32 address = UIMMVAL << 16;
	UINT32 targetreg = RTREG;
	UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
	UINT8 nextrsreg = (nextop >> 21) & 31;
	UINT8 nextrtreg = (nextop >> 16) & 31;
	INT32 nextsimm = (INT16)nextop;
	void *memory;
	
	/* if the next instruction is a load or store, see if we can consolidate */
	if (!in_delay_slot)
		switch (nextop >> 26)
		{
			case 0x08:	/* addi */
			case 0x09:	/* addiu */
				if (nextrsreg != targetreg || nextrtreg != targetreg)
					break;
				_mov_m64abs_imm32(&mips3.r[targetreg], address + nextsimm);	/* mov	[targetreg],const*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);
				
			case 0x0d:	/* ori */
				if (nextrsreg != targetreg || nextrtreg != targetreg)
					break;
				_mov_m64abs_imm32(&mips3.r[targetreg], address | (UINT16)nextsimm);	/* mov	[targetreg],const*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);
				
			case 0x20:	/* lb */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_movsx_r32_m8abs(REG_EAX, memory);							/* movsx eax,byte [memory]*/
				_cdq();														/* cdq*/
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		/* mov	[nextrtreg],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x21:	/* lh */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_movsx_r32_m16abs(REG_EAX, memory);							/* movsx eax,word [memory]*/
				_cdq();														/* cdq*/
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		/* mov	[nextrtreg],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x23:	/* lw */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_mov_r32_m32abs(REG_EAX, memory);							/* mov	eax,[memory]*/
				_cdq();														/* cdq*/
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		/* mov	[nextrtreg],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x24:	/* lbu */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_movzx_r32_m8abs(REG_EAX, memory);							/* movzx eax,byte [memory]*/
				_mov_m32abs_imm(HI(&mips3.r[nextrtreg]), 0);				/* mov	[nextrtreg].hi,0*/
				_mov_m32abs_r32(LO(&mips3.r[nextrtreg]), REG_EAX);			/* mov	[nextrtreg].lo,eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x25:	/* lhu */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_read_ptr(cpu_getactivecpu(), BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_movzx_r32_m16abs(REG_EAX, memory);							/* movzx eax,word [memory]*/
				_mov_m32abs_imm(HI(&mips3.r[nextrtreg]), 0);				/* mov	[nextrtreg].hi,0*/
				_mov_m32abs_r32(LO(&mips3.r[nextrtreg]), REG_EAX);			/* mov	[nextrtreg].lo,eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x27:	/* lwu */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_mov_r32_m32abs(REG_EAX, memory);							/* mov	eax,[memory]*/
				_mov_m32abs_imm(HI(&mips3.r[nextrtreg]), 0);				/* mov	[nextrtreg].hi,0*/
				_mov_m32abs_r32(LO(&mips3.r[nextrtreg]), REG_EAX);			/* mov	[nextrtreg].lo,eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x31:	/* lwc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r32_m32abs(REG_EAX, memory);							/* mov	eax,[memory]*/
				_mov_m32abs_r32(LO(&mips3.cpr[1][nextrtreg]), REG_EAX);		/* mov	cpr[1][nextrtreg].lo,eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x32:	/* lwc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r32_m32abs(REG_EAX, memory);							/* mov	eax,[memory]*/
				_mov_m32abs_r32(LO(&mips3.cpr[2][nextrtreg]), REG_EAX);		/* mov	cpr[2][nextrtreg].lo,eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x35:	/* ldc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r64_m64abs(REG_EDX, REG_EAX, memory);					/* mov	edx:eax,[memory]*/
				_mov_m64abs_r64(&mips3.cpr[1][nextrtreg], REG_EDX, REG_EAX);/* mov	cpr[1][nextrtreg],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x36:	/* ldc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r64_m64abs(REG_EDX, REG_EAX, memory);					/* mov	edx:eax,[memory]*/
				_mov_m64abs_r64(&mips3.cpr[2][nextrtreg], REG_EDX, REG_EAX);/* mov	cpr[2][nextrtreg],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x37:	/* ld */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_read_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway if we're not reading to the same register */
				if (nextrtreg != targetreg)
					_mov_m64abs_imm32(&mips3.r[targetreg], address);		/* mov	[targetreg],const << 16*/
				_mov_r64_m64abs(REG_EDX, REG_EAX, memory);					/* mov	eax,[memory]*/
				_mov_m64abs_r64(&mips3.r[nextrtreg], REG_EDX, REG_EAX);		/* mov	[nextrtreg],eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x28:	/* sb */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_write_ptr(cpu_getactivecpu(), BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				if (nextrtreg != 0)
				{
					_mov_r8_m8abs(REG_AL, &mips3.r[nextrtreg]);				/* mov	ax,[nextrtreg]*/
					_mov_m8abs_r8(memory, REG_AL);							/* mov	[memory],ax*/
				}
				else
					_mov_m8abs_imm(memory, 0);								/* mov	[memory],0*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x29:	/* sh */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				if (mips3.bigendian)
					memory = memory_get_write_ptr(cpu_getactivecpu(), BYTE4_XOR_BE(address + nextsimm));
				else
					memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				if (nextrtreg != 0)
				{
					_mov_r16_m16abs(REG_AX, &mips3.r[nextrtreg]);			/* mov	ax,[nextrtreg]*/
					_mov_m16abs_r16(memory, REG_AX);						/* mov	[memory],ax*/
				}
				else
					_mov_m16abs_imm(memory, 0);								/* mov	[memory],0*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x2b:	/* sw */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				if (nextrtreg != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[nextrtreg]);			/* mov	eax,[nextrtreg]*/
					_mov_m32abs_r32(memory, REG_EAX);						/* mov	[memory],eax*/
				}
				else
					_mov_m32abs_imm(memory, 0);								/* mov	[memory],0*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x39:	/* swc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][nextrtreg]);			/* mov	eax,cpr[1][nextrtreg]*/
				_mov_m32abs_r32(memory, REG_EAX);							/* mov	[memory],eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3a:	/* swc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r32_m32abs(REG_EAX, &mips3.cpr[2][nextrtreg]);			/* mov	eax,cpr[2][nextrtreg]*/
				_mov_m32abs_r32(memory, REG_EAX);							/* mov	[memory],eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3d:	/* sdc1 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.cpr[1][nextrtreg]);/* mov	edx:eax,cpr[1][nextrtreg]*/
				_mov_m64abs_r64(memory, REG_EDX, REG_EAX);					/* mov	[memory],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3e:	/* sdc2 */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.cpr[2][nextrtreg]);/* mov	edx:eax,cpr[2][nextrtreg]*/
				_mov_m64abs_r64(memory, REG_EDX, REG_EAX);					/* mov	[memory],edx:eax*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);

			case 0x3f:	/* sd */
				if (nextrsreg != targetreg || !(mips3.drcoptions & MIPS3DRC_DIRECT_RAM))
					break;

				/* see if this points to a RAM-like area */
				memory = memory_get_write_ptr(cpu_getactivecpu(), address + nextsimm);
				if (!memory)
					break;
				
				/* do the LUI anyway */
				_mov_m64abs_imm32(&mips3.r[targetreg], address);			/* mov	[targetreg],const << 16*/
				if (nextrtreg != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[nextrtreg]);	/* mov	edx:eax,[nextrtreg]*/
					_mov_m64abs_r64(memory, REG_EDX, REG_EAX);				/* mov	[memory],edx:eax*/
				}
				else
					_mov_m64abs_imm32(memory, 0);							/* mov	[memory],0*/
				return RECOMPILE_SUCCESSFUL_CP(2,8);
		}

	/* default case: standard LUI */	
	_mov_m64abs_imm32(&mips3.r[targetreg], address);					/* mov	[rtreg],const << 16*/
	return RECOMPILE_SUCCESSFUL_CP(1,4);
}


/*------------------------------------------------------------------
	recompile_ldlr_le
------------------------------------------------------------------*/

static UINT32 recompile_ldlr_le(struct drccore *drc, UINT8 rtreg, UINT8 rsreg, INT16 simmval)
{
	struct linkdata link1, link2;
	_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
	_mov_r32_m32abs(REG_EAX, &mips3.r[rsreg]);								/* mov	eax,[rsreg]*/
	if (simmval)
		_add_r32_imm(REG_EAX, simmval);										/* add	eax,simmval*/
	_mov_m32abs_r32(&scratchspace[0], REG_EAX);								/* mov	[scratchspace[0]],eax*/
	_and_r32_imm(REG_EAX, ~3);												/* and	eax,~3*/
	_push_r32(REG_EAX);														/* push	eax*/
	_call((void *)mips3.memory.readlong);									/* call	readlong*/
	_mov_m32abs_r32(&scratchspace[1], REG_EAX);								/* mov	[scratchspace[1]],eax*/
	_add_m32bd_imm(REG_ESP, 0, 4);											/* add	[esp],4*/
	_call((void *)mips3.memory.readlong);									/* call	readlong*/
	_test_m32abs_imm(&scratchspace[0], 3);									/* test	[scratchspace[0]],3*/
	_jcc_short_link(COND_Z, &link1);										/* jz	link1*/
	_mov_m32abs_r32(&scratchspace[2], REG_EAX);								/* mov	[scratchspace[2]],eax*/
	_add_m32bd_imm(REG_ESP, 0, 4);											/* add	[esp],4*/
	_call((void *)mips3.memory.readlong);									/* call	readlong*/
	_mov_r32_m32abs(REG_ECX, &scratchspace[0]);								/* mov	ecx,[scratchspace[0]]*/
	_mov_r32_m32abs(REG_EBX, &scratchspace[1]);								/* mov	ebx,[scratchspace[1]]*/
	_mov_r32_m32abs(REG_EDX, &scratchspace[2]);								/* mov	edx,[scratchspace[2]]*/
	_shl_r32_imm(REG_ECX, 3);												/* shl	ecx,3*/
	_shrd_r32_r32_cl(REG_EBX, REG_EDX);										/* shrd	ebx,edx,cl*/
	_shrd_r32_r32_cl(REG_EDX, REG_EAX);										/* shrd edx,eax,cl*/
	_mov_m64abs_r64(&mips3.r[rtreg], REG_EDX, REG_EBX);						/* mov	[rtreg],edx:ebx*/
	_jmp_short_link(&link2);												/* jmp	done*/
	_resolve_link(&link1);													/* link1:*/
	_mov_r32_m32abs(REG_EDX, &scratchspace[1]);								/* mov	edx,[scratchspace[1]]*/
	_mov_m64abs_r64(&mips3.r[rtreg], REG_EAX, REG_EDX);						/* mov	[rtreg],eax:edx*/
	_resolve_link(&link2);													/* link2:*/
	_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
	_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
	return RECOMPILE_SUCCESSFUL_CP(2,8);
}


/*------------------------------------------------------------------
	recompile_lwlr_le
------------------------------------------------------------------*/

static UINT32 recompile_lwlr_le(struct drccore *drc, UINT8 rtreg, UINT8 rsreg, INT16 simmval)
{
	struct linkdata link1;
	_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
	_mov_r32_m32abs(REG_EAX, &mips3.r[rsreg]);								/* mov	eax,[rsreg]*/
	if (simmval)
		_add_r32_imm(REG_EAX, simmval);										/* add	eax,simmval*/
	_mov_m32abs_r32(&scratchspace[0], REG_EAX);								/* mov	[scratchspace[0]],eax*/
	_and_r32_imm(REG_EAX, ~3);												/* and	eax,~3*/
	_push_r32(REG_EAX);														/* push	eax*/
	_call((void *)mips3.memory.readlong);									/* call	readlong*/
	_test_m32abs_imm(&scratchspace[0], 3);									/* test	[scratchspace[0]],3*/
	_jcc_short_link(COND_Z, &link1);										/* jz	link1*/
	_mov_m32abs_r32(&scratchspace[1], REG_EAX);								/* mov	[scratchspace[1]],eax*/
	_add_m32bd_imm(REG_ESP, 0, 4);											/* add	[esp],4*/
	_call((void *)mips3.memory.readlong);									/* call	readlong*/
	_mov_r32_m32abs(REG_ECX, &scratchspace[0]);								/* mov	ecx,[scratchspace[0]]*/
	_mov_r32_m32abs(REG_EDX, &scratchspace[1]);								/* mov	edx,[scratchspace[1]]*/
	_shl_r32_imm(REG_ECX, 3);												/* shl	ecx,3*/
	_shrd_r32_r32_cl(REG_EDX, REG_EAX);										/* shrd edx,eax,cl*/
	_mov_r32_r32(REG_EAX, REG_EDX);											/* mov	eax,edx*/
	_resolve_link(&link1);													/* link1:*/
	_cdq();																	/* cdq*/
	_mov_m64abs_r64(&mips3.r[rtreg], REG_EDX, REG_EAX);						/* mov	[rtreg],edx:eax*/
	_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
	_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
	return RECOMPILE_SUCCESSFUL_CP(2,8);
}


/*------------------------------------------------------------------
	recompile_instruction
------------------------------------------------------------------*/

static UINT32 recompile_instruction(struct drccore *drc, UINT32 pc)
{
	static UINT32 ldl_mask[] =
	{
		0x00000000,0x00000000,
		0x00000000,0x000000ff,
		0x00000000,0x0000ffff,
		0x00000000,0x00ffffff,
		0x00000000,0xffffffff,
		0x000000ff,0xffffffff,
		0x0000ffff,0xffffffff,
		0x00ffffff,0xffffffff
	};
	static UINT32 ldr_mask[] =
	{
		0x00000000,0x00000000,
		0xff000000,0x00000000,
		0xffff0000,0x00000000,
		0xffffff00,0x00000000,
		0xffffffff,0x00000000,
		0xffffffff,0xff000000,
		0xffffffff,0xffff0000,
		0xffffffff,0xffffff00
	};
	static UINT32 sdl_mask[] =
	{
		0x00000000,0x00000000,
		0xff000000,0x00000000,
		0xffff0000,0x00000000,
		0xffffff00,0x00000000,
		0xffffffff,0x00000000,
		0xffffffff,0xff000000,
		0xffffffff,0xffff0000,
		0xffffffff,0xffffff00
	};
	static UINT32 sdr_mask[] =
	{
		0x00000000,0x00000000,
		0x00000000,0x000000ff,
		0x00000000,0x0000ffff,
		0x00000000,0x00ffffff,
		0x00000000,0xffffffff,
		0x000000ff,0xffffffff,
		0x0000ffff,0xffffffff,
		0x00ffffff,0xffffffff
	};
	struct linkdata link1, link2, link3;
	UINT32 op = *(UINT32 *)&OP_ROM[pc];
	int cycles;

	switch (op >> 26)
	{
		case 0x00:	/* SPECIAL */
			return recompile_special(drc, pc, op);

		case 0x01:	/* REGIMM */
			return recompile_regimm(drc, pc, op);

		case 0x02:	/* J */
			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, (pc & 0xf0000000) | (LIMMVAL << 2), 1+cycles);/* <branch or dispatch>*/
			return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;

		case 0x03:	/* JAL */
			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								/* mov	[31],pc + 8*/
			append_branch_or_dispatch(drc, (pc & 0xf0000000) | (LIMMVAL << 2), 1+cycles);/* <branch or dispatch>*/
			return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;

		case 0x04:	/* BEQ */
			if (RSREG == RTREG)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* mov	eax,[rtreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* or	eax,[rtreg].hi*/
				_jcc_near_link(COND_NZ, &link1);									/* jnz	skip*/
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* or	eax,[rsreg].hi*/
				_jcc_near_link(COND_NZ, &link1);									/* jnz	skip*/
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].lo*/
				_jcc_near_link(COND_NE, &link1);									/* jne	skip*/
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* mov	eax,[rsreg].hi*/
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].hi*/
				_jcc_near_link(COND_NE, &link2);									/* jne	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			if (RSREG != 0 && RTREG != 0)
				_resolve_link(&link2);												/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x05:	/* BNE */
			if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* mov	eax,[rtreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* or	eax,[rtreg].hi*/
				_jcc_near_link(COND_Z, &link1);										/* jz	skip*/
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* or	eax,[rsreg].hi*/
				_jcc_near_link(COND_Z, &link1);										/* jz	skip*/
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].lo*/
				_jcc_short_link(COND_NE, &link2);									/* jne	takeit*/
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* mov	eax,[rsreg].hi*/
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].hi*/
				_jcc_near_link(COND_E, &link1);										/* je	skip*/
				_resolve_link(&link2);												/* takeit:*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* BLEZ */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_G, &link1);										/* jg	skip*/
				_jcc_short_link(COND_L, &link2);									/* jl	takeit*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].lo,0*/
				_jcc_near_link(COND_NE, &link3);									/* jne	skip*/
				_resolve_link(&link2);												/* takeit:*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			_resolve_link(&link3);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x07:	/* BGTZ */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_L, &link1);										/* jl	skip*/
				_jcc_short_link(COND_G, &link2);									/* jg	takeit*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].lo,0*/
				_jcc_near_link(COND_E, &link3);										/* je	skip*/
				_resolve_link(&link2);												/* takeit:*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			_resolve_link(&link3);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* ADDI */
			if (RSREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_jcc(COND_O, mips3.generate_overflow_exception);					/* jo	generate_overflow_exception*/
				if (RTREG != 0)
				{
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				/* mov	[rtreg],edx:eax*/
				}
			}
			else if (RTREG != 0)
				_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);						/* mov	[rtreg],const*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x09:	/* ADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_add_r32_imm(REG_EAX, SIMMVAL);									/* add	eax,SIMMVAL*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				/* mov	[rtreg],edx:eax*/
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);					/* mov	[rtreg],const*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0a:	/* SLTI */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
					_sub_r32_imm(REG_EAX, SIMMVAL);									/* sub	eax,[rtreg].lo*/
					_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					/* sbb	edx,[rtreg].lo*/
					_shr_r32_imm(REG_EDX, 31);										/* shr	edx,31*/
					_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EDX);					/* mov	[rdreg].lo,edx*/
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rdreg].hi,0*/
				}
				else
				{
					_mov_m32abs_imm(LO(&mips3.r[RTREG]), (0 < SIMMVAL));			/* mov	[rtreg].lo,const*/
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rtreg].hi,sign-extend(const)*/
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0b:	/* SLTIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_xor_r32_r32(REG_ECX, REG_ECX);									/* xor	ecx,ecx*/
					_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	/* cmp	[rsreg].hi,upper*/
					_jcc_short_link(COND_B, &link1);								/* jb	takeit*/
					_jcc_short_link(COND_A, &link2);								/* ja	skip*/
					_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					/* cmp	[rsreg].lo,lower*/
					_jcc_short_link(COND_AE, &link3);								/* jae	skip*/
					_resolve_link(&link1);											/* takeit:*/
					_add_r32_imm(REG_ECX, 1);										/* add	ecx,1*/
					_resolve_link(&link2);											/* skip:*/
					_resolve_link(&link3);											/* skip:*/
					_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_ECX);					/* mov	[rtreg].lo,ecx*/
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rtreg].hi,sign-extend(const)*/
				}
				else
				{
					_mov_m32abs_imm(LO(&mips3.r[RTREG]), (0 < SIMMVAL));			/* mov	[rtreg].lo,const*/
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rtreg].hi,sign-extend(const)*/
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0c:	/* ANDI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
				{
					_and_m32abs_imm(&mips3.r[RTREG], UIMMVAL);						/* and	[rtreg],UIMMVAL*/
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rtreg].hi,0*/
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg].lo*/
					_and_r32_imm(REG_EAX, UIMMVAL);									/* and	eax,UIMMVAL*/
					_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);					/* mov	[rtreg].lo,eax*/
					_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rtreg].hi,0*/
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], 0);							/* mov	[rtreg],0*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0d:	/* ORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					_or_m32abs_imm(&mips3.r[RTREG], UIMMVAL);						/* or	[rtreg],UIMMVAL*/
				else if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
					_or_r32_imm(REG_EAX, UIMMVAL);									/* or	eax,UIMMVAL*/
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				/* mov	[rtreg],edx:eax*/
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], UIMMVAL);					/* mov	[rtreg],const*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0e:	/* XORI */
			if (RTREG != 0)
			{
				if (RSREG == RTREG)
					_xor_m32abs_imm(&mips3.r[RTREG], UIMMVAL);						/* xor	[rtreg],UIMMVAL*/
				else if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
					_xor_r32_imm(REG_EAX, UIMMVAL);									/* xor	eax,UIMMVAL*/
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				/* mov	[rtreg],edx:eax*/
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], UIMMVAL);					/* mov	[rtreg],const*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0f:	/* LUI */
			if (RTREG != 0)
				return recompile_lui(drc, pc, op);
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x10:	/* COP0 */
			return recompile_cop0(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;
			
		case 0x11:	/* COP1 */
			return recompile_cop1(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x12:	/* COP2 */
			_jmp((void *)mips3.generate_invalidop_exception);						/* jmp	generate_invalidop_exception*/
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x13:	/* COP1X - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					/* jmp	generate_invalidop_exception*/
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			return recompile_cop1x(drc, pc, op) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x14:	/* BEQL */
			if (RSREG == RTREG)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* mov	eax,[rtreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* or	eax,[rtreg].hi*/
				_jcc_near_link(COND_NZ, &link1);									/* jnz	skip*/
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* or	eax,[rsreg].hi*/
				_jcc_near_link(COND_NZ, &link1);									/* jnz	skip*/
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].lo*/
				_jcc_near_link(COND_NE, &link1);									/* jne	skip*/
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* mov	eax,[rsreg].hi*/
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].hi*/
				_jcc_near_link(COND_NE, &link2);									/* jne	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			if (RSREG != 0 && RTREG != 0)
				_resolve_link(&link2);												/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x15:	/* BNEL */
			if (RSREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* mov	eax,[rtreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* or	eax,[rtreg].hi*/
				_jcc_near_link(COND_Z, &link1);										/* jz	skip*/
			}
			else if (RTREG == 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* or	eax,[rsreg].hi*/
				_jcc_near_link(COND_Z, &link1);										/* jz	skip*/
			}
			else
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].lo*/
				_jcc_short_link(COND_NE, &link2);									/* jne	takeit*/
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* mov	eax,[rsreg].hi*/
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].hi*/
				_jcc_near_link(COND_E, &link1);										/* je	skip*/
				_resolve_link(&link2);												/* takeit:*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x16:	/* BLEZL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_G, &link1);										/* jg	skip*/
				_jcc_short_link(COND_L, &link2);									/* jl	takeit*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].lo,0*/
				_jcc_near_link(COND_NE, &link3);									/* jne	skip*/
				_resolve_link(&link2);												/* takeit:*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			_resolve_link(&link3);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x17:	/* BGTZL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,8);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_L, &link1);										/* jl	skip*/
				_jcc_short_link(COND_G, &link2);									/* jg	takeit*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].lo,0*/
				_jcc_near_link(COND_E, &link3);										/* je	skip*/
				_resolve_link(&link2);												/* takeit:*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			_resolve_link(&link3);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x18:	/* DADDI */
			if (RSREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					/* mov	edx:eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_adc_r32_imm(REG_EDX, (SIMMVAL < 0) ? -1 : 0);						/* adc	edx,signext(SIMMVAL)*/
				_jcc(COND_O, mips3.generate_overflow_exception);					/* jo	generate_overflow_exception*/
				if (RTREG != 0)
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				/* mov	[rtreg],edx:eax*/
			}
			else if (RTREG != 0)
				_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);						/* mov	[rtreg],const*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x19:	/* DADDIU */
			if (RTREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
					_add_r32_imm(REG_EAX, SIMMVAL);									/* add	eax,SIMMVAL*/
					_adc_r32_imm(REG_EDX, (SIMMVAL < 0) ? -1 : 0);					/* adc	edx,signext(SIMMVAL)*/
					_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);				/* mov	[rtreg],edx:eax*/
				}
				else
					_mov_m64abs_imm32(&mips3.r[RTREG], SIMMVAL);					/* mov	[rtreg],const*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1a:	/* LDL */
			if (!mips3.bigendian)
			{
				UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
				if ((nextop >> 26) == 0x1b &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL - 7)
					return recompile_ldlr_le(drc, RTREG, RSREG, SIMMVAL - 7);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~7);												/* and	eax,~7*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_push_r32(REG_EAX);														/* push	eax*/
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									/* mov	eax,[esp+4]*/
			_add_r32_imm(REG_EAX, 4);												/* add	eax,4*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													/* pop	edx*/
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										/* mov	edx,eax*/
				_pop_r32(REG_EAX);													/* pop	eax*/
			}
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_pop_r32(REG_ECX);														/* pop	ecx*/
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 7);											/* and	ecx,7*/
				_shl_r32_imm(REG_ECX, 3);											/* shl	ecx,3*/
				if (!mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x38);									/* xor	ecx,0x38*/
				_test_r32_imm(REG_ECX, 0x20);										/* test	ecx,0x20*/
				_jcc_short_link(COND_Z, &link1);									/* jz	skip*/
				_mov_r32_r32(REG_EDX, REG_EAX);										/* mov	edx,eax*/
				_xor_r32_r32(REG_EAX, REG_EAX);										/* xor	eax,eax*/
				_resolve_link(&link1);												/* skip:*/
				_shld_r32_r32_cl(REG_EDX, REG_EAX);									/* shld	edx,eax,cl*/
				_shl_r32_cl(REG_EAX);												/* shl	eax,cl*/
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						/* mov	ebx,[rtreg].lo*/
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask + 1);						/* and	ebx,[ldl_mask + ecx + 4]*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
				_mov_r32_m32abs(REG_EBX, HI(&mips3.r[RTREG]));						/* mov	ebx,[rtreg].hi*/
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask);							/* and	ebx,[ldl_mask + ecx]*/
				_or_r32_r32(REG_EDX, REG_EBX);										/* or	edx,ebx*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x1b:	/* LDR */
			if (!mips3.bigendian)
			{
				UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
				if ((nextop >> 26) == 0x1a &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL + 7)
					return recompile_ldlr_le(drc, RTREG, RSREG, SIMMVAL);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~7);												/* and	eax,~7*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_push_r32(REG_EAX);														/* push	eax*/
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									/* mov	eax,[esp+4]*/
			_add_r32_imm(REG_EAX, 4);												/* add	eax,4*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													/* pop	edx*/
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										/* mov	edx,eax*/
				_pop_r32(REG_EAX);													/* pop	eax*/
			}
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_pop_r32(REG_ECX);														/* pop	ecx*/
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 7);											/* and	ecx,7*/
				_shl_r32_imm(REG_ECX, 3);											/* shl	ecx,3*/
				if (mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x38);									/* xor	ecx,0x38*/
				_test_r32_imm(REG_ECX, 0x20);										/* test	ecx,0x20*/
				_jcc_short_link(COND_Z, &link1);									/* jz	skip*/
				_mov_r32_r32(REG_EAX, REG_EDX);										/* mov	eax,edx*/
				_xor_r32_r32(REG_EDX, REG_EDX);										/* xor	edx,edx*/
				_resolve_link(&link1);												/* skip:*/
				_shrd_r32_r32_cl(REG_EAX, REG_EDX);									/* shrd	eax,edx,cl*/
				_shr_r32_cl(REG_EDX);												/* shr	edx,cl*/
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						/* mov	ebx,[rtreg].lo*/
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask + 1);						/* and	ebx,[ldr_mask + ecx + 4]*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
				_mov_r32_m32abs(REG_EBX, HI(&mips3.r[RTREG]));						/* mov	ebx,[rtreg].hi*/
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask);							/* and	ebx,[ldr_mask + ecx]*/
				_or_r32_r32(REG_EDX, REG_EBX);										/* or	edx,ebx*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x20:	/* LB */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readbyte);											/* call	readbyte*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			if (RTREG != 0)
			{
				_movsx_r32_r8(REG_EAX, REG_AL);										/* movsx eax,al*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x21:	/* LH */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readword);											/* call	readword*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			if (RTREG != 0)
			{
				_movsx_r32_r16(REG_EAX, REG_AX);									/* movsx eax,ax*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x22:	/* LWL */
			if (!mips3.bigendian)
			{
				UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
				if ((nextop >> 26) == 0x26 &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL - 3)
					return recompile_lwlr_le(drc, RTREG, RSREG, SIMMVAL - 3);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~3);												/* and	eax,~3*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_pop_r32(REG_ECX);														/* pop	ecx*/
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 3);											/* and	ecx,3*/
				_shl_r32_imm(REG_ECX, 3);											/* shl	ecx,3*/
				if (!mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x18);									/* xor	ecx,0x18*/
				_shl_r32_cl(REG_EAX);												/* shl	eax,cl*/
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						/* mov	ebx,[rtreg].lo*/
				_and_r32_m32bd(REG_EBX, REG_ECX, ldl_mask + 1);						/* and	ebx,[ldl_mask + ecx + 4]*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);									/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x23:	/* LW */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			if (RTREG != 0)
			{
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:	/* LBU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readbyte);											/* call	readbyte*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			if (RTREG != 0)
			{
				_and_r32_imm(REG_EAX, 0xff);										/* and	eax,0xff*/
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);							/* mov	[rtreg].hi,0*/
				_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);						/* mov	[rtreg].lo,eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x25:	/* LHU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readword);											/* call	readword*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			if (RTREG != 0)
			{
				_and_r32_imm(REG_EAX, 0xffff);										/* and	eax,0xffff*/
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);							/* mov	[rtreg].hi,0*/
				_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);						/* mov	[rtreg].lo,eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x26:	/* LWR */
			if (!mips3.bigendian)
			{
				UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
				if ((nextop >> 26) == 0x22 &&
					(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
					(INT16)nextop == SIMMVAL + 3)
					return recompile_lwlr_le(drc, RTREG, RSREG, SIMMVAL);
			}
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~3);												/* and	eax,~3*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_pop_r32(REG_ECX);														/* pop	ecx*/
			
			if (RTREG != 0)
			{
				_and_r32_imm(REG_ECX, 3);											/* and	ecx,3*/
				_shl_r32_imm(REG_ECX, 3);											/* shl	ecx,3*/
				if (mips3.bigendian)
					_xor_r32_imm(REG_ECX, 0x18);									/* xor	ecx,0x18*/
				_shr_r32_cl(REG_EAX);												/* shr	eax,cl*/
				_mov_r32_m32abs(REG_EBX, LO(&mips3.r[RTREG]));						/* mov	ebx,[rtreg].lo*/
				_and_r32_m32bd(REG_EBX, REG_ECX, ldr_mask);							/* and	ebx,[ldr_mask + ecx]*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x27:	/* LWU */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			if (RTREG != 0)
			{
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);							/* mov	[rtreg].hi,0*/
				_mov_m32abs_r32(LO(&mips3.r[RTREG]), REG_EAX);						/* mov	[rtreg].lo,eax*/
			}
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x28:	/* SB */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										/* push	dword [rtreg]*/
			else
				_push_imm(0);														/* push	0*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.writebyte);											/* call	writebyte*/
			_add_r32_imm(REG_ESP, 8);												/* add  esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x29:	/* SH */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										/* push	dword [rtreg]*/
			else
				_push_imm(0);														/* push	0*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.writeword);											/* call	writeword*/
			_add_r32_imm(REG_ESP, 8);												/* add  esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2a:	/* SWL */
/*{
UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
if ((nextop >> 26) == 0x2e &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL - 3)
	_add_m32abs_imm(&swlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~3);												/* and	eax,~3*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									/* mov	ecx,[esp]*/
			
			_and_r32_imm(REG_ECX, 3);												/* and	ecx,3*/
			_shl_r32_imm(REG_ECX, 3);												/* shl	ecx,3*/
			if (!mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x18);										/* xor	ecx,0x18*/
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdl_mask);								/* and	eax,[sdl_mask + ecx]*/

			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EBX, &mips3.r[RTREG]);							/* mov	ebx,[rtreg]*/
				_shr_r32_cl(REG_EBX);												/* shr	ebx,cl*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
			}
			
			_pop_r32(REG_EBX);														/* pop	ebx*/
			_and_r32_imm(REG_EBX, ~3);												/* and	ebx,~3*/
			_push_r32(REG_EAX);														/* push	eax*/
			_push_r32(REG_EBX);														/* push	ebx*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2b:	/* SW */
/*			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								// mov	eax,[rsreg]
			if (SIMMVAL != 0)
				_add_r32_imm(REG_EAX, SIMMVAL);										//add	eax,SIMMVAL
			_mov_r32_r32(REG_EBX, REG_EAX);											// mov	ebx,eax
			_shr_r32_imm(REG_EBX, 16);												// shr	ebx,16
			_mov_r32_m32isd(REG_EBX, REG_EBX, 4, ram_write_table);					// mov	ebx,[ebx*4 + ram_write_table]
			_cmp_r32_imm(REG_EBX, 0);												// cmp	ebx,0
			_jcc_short_link(COND_NE, &link1);										// jne	fast
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										//push	dword [rtreg]
			else
				_push_imm(0);														//push	0
			_push_r32(REG_EAX);														// push	eax
			drc_append_save_call_restore(drc, (void *)mips3.memory.writelong, 8);	// call	writelong
			_jmp_short_link(&link2);												// jmp	done
			_resolve_link(&link1);													// fast:
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);							// mov  ecx,[rtreg]
				_mov_m32bisd_r32(REG_EBX, REG_EAX, 1, 0, REG_ECX);					// mov	[ebx+eax],ecx
			}
			else
				_mov_m32bisd_imm(REG_EBX, REG_EAX, 1, 0, 0);						// mov	[ebx+eax],0
			_resolve_link(&link2);													// fast:
*/

			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RTREG != 0)
				_push_m32abs(&mips3.r[RTREG]);										/* push	dword [rtreg]*/
			else
				_push_imm(0);														/* push	0*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.writelong);											/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add  esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x2c:	/* SDL */
/*{
UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
if ((nextop >> 26) == 0x2d &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL - 7)
	_add_m32abs_imm(&sdlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~7);												/* and	eax,~7*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_push_r32(REG_EAX);														/* push	eax*/
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									/* mov	eax,[esp+4]*/
			_add_r32_imm(REG_EAX, 4);												/* add	eax,4*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													/* pop	edx*/
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										/* mov	edx,eax*/
				_pop_r32(REG_EAX);													/* pop	eax*/
			}
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									/* mov	ecx,[esp]*/
			
			_and_r32_imm(REG_ECX, 7);												/* and	ecx,7*/
			_shl_r32_imm(REG_ECX, 3);												/* shl	ecx,3*/
			if (!mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x38);										/* xor	ecx,0x38*/
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdl_mask + 1);							/* and	eax,[sdl_mask + ecx + 4]*/
			_and_r32_m32bd(REG_EDX, REG_ECX, sdl_mask);								/* and	eax,[sdl_mask + ecx]*/

			if (RTREG != 0)
			{
				_test_r32_imm(REG_ECX, 0x20);										/* test	ecx,0x20*/
				_mov_r64_m64abs(REG_ESI, REG_EBX, &mips3.r[RTREG]);					/* mov	esi:ebx,[rtreg]*/
				_jcc_short_link(COND_Z, &link1);									/* jz	skip*/
				_mov_r32_r32(REG_EBX, REG_ESI);										/* mov	ebx,esi*/
				_xor_r32_r32(REG_ESI, REG_ESI);										/* xor	esi,esi*/
				_resolve_link(&link1);												/* skip:*/
				_shrd_r32_r32_cl(REG_EBX, REG_ESI);									/* shrd	ebx,esi,cl*/
				_shr_r32_cl(REG_ESI);												/* shr	esi,cl*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
				_or_r32_r32(REG_EDX, REG_ESI);										/* or	edx,esi*/
			}
			
			_pop_r32(REG_EBX);														/* pop	ebx*/
			_and_r32_imm(REG_EBX, ~7);												/* and	ebx,~7*/
			_lea_r32_m32bd(REG_ECX, REG_EBX, 4);									/* lea	ecx,[ebx+4]*/
			_push_r32(mips3.bigendian ? REG_EAX : REG_EDX);							/* push	eax/edx*/
			_push_r32(REG_ECX);														/* push ecx*/
			_push_r32(mips3.bigendian ? REG_EDX : REG_EAX);							/* push	edx/eax*/
			_push_r32(REG_EBX);														/* push	ebx*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2d:	/* SDR */
/*{
UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
if ((nextop >> 26) == 0x2c &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL + 7)
	_add_m32abs_imm(&sdlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~7);												/* and	eax,~7*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_push_r32(REG_EAX);														/* push	eax*/
			_mov_r32_m32bd(REG_EAX, REG_ESP, 4);									/* mov	eax,[esp+4]*/
			_add_r32_imm(REG_EAX, 4);												/* add	eax,4*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			if (mips3.bigendian)
				_pop_r32(REG_EDX);													/* pop	edx*/
			else
			{
				_mov_r32_r32(REG_EDX, REG_EAX);										/* mov	edx,eax*/
				_pop_r32(REG_EAX);													/* pop	eax*/
			}
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									/* mov	ecx,[esp]*/
			
			_and_r32_imm(REG_ECX, 7);												/* and	ecx,7*/
			_shl_r32_imm(REG_ECX, 3);												/* shl	ecx,3*/
			if (mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x38);										/* xor	ecx,0x38*/
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdr_mask + 1);							/* and	eax,[sdr_mask + ecx + 4]*/
			_and_r32_m32bd(REG_EDX, REG_ECX, sdr_mask);								/* and	eax,[sdr_mask + ecx]*/

			if (RTREG != 0)
			{
				_test_r32_imm(REG_ECX, 0x20);										/* test	ecx,0x20*/
				_mov_r64_m64abs(REG_ESI, REG_EBX, &mips3.r[RTREG]);					/* mov	esi:ebx,[rtreg]*/
				_jcc_short_link(COND_Z, &link1);									/* jz	skip*/
				_mov_r32_r32(REG_ESI, REG_EBX);										/* mov	esi,ebx*/
				_xor_r32_r32(REG_EBX, REG_EBX);										/* xor	ebx,ebx*/
				_resolve_link(&link1);												/* skip:*/
				_shld_r32_r32_cl(REG_ESI, REG_EBX);									/* shld	esi,ebx,cl*/
				_shl_r32_cl(REG_EBX);												/* shl	ebx,cl*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
				_or_r32_r32(REG_EDX, REG_ESI);										/* or	edx,esi*/
			}
			
			_pop_r32(REG_EBX);														/* pop	ebx*/
			_and_r32_imm(REG_EBX, ~7);												/* and	ebx,~7*/
			_lea_r32_m32bd(REG_ECX, REG_EBX, 4);									/* lea	ecx,[ebx+4]*/
			_push_r32(mips3.bigendian ? REG_EAX : REG_EDX);							/* push	eax/edx*/
			_push_r32(REG_ECX);														/* push ecx*/
			_push_r32(mips3.bigendian ? REG_EDX : REG_EAX);							/* push	edx/eax*/
			_push_r32(REG_EBX);														/* push	ebx*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2e:	/* SWR */
/*{
UINT32 nextop = *(UINT32 *)&OP_ROM[pc + 4];
if ((nextop >> 26) == 0x2a &&
	(nextop & 0x03ff0000) == (op & 0x03ff0000) &&
	(INT16)nextop == SIMMVAL + 3)
	_add_m32abs_imm(&swlr_hits, 1);
}*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			if (SIMMVAL)
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
			_push_r32(REG_EAX);														/* push	eax*/
			_and_r32_imm(REG_EAX, ~3);												/* and	eax,~3*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_mov_r32_m32bd(REG_ECX, REG_ESP, 0);									/* mov	ecx,[esp]*/
			
			_and_r32_imm(REG_ECX, 3);												/* and	ecx,3*/
			_shl_r32_imm(REG_ECX, 3);												/* shl	ecx,3*/
			if (mips3.bigendian)
				_xor_r32_imm(REG_ECX, 0x18);										/* xor	ecx,0x18*/
			
			_and_r32_m32bd(REG_EAX, REG_ECX, sdr_mask + 1);							/* and	eax,[sdr_mask + ecx + 4]*/

			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EBX, &mips3.r[RTREG]);							/* mov	ebx,[rtreg]*/
				_shl_r32_cl(REG_EBX);												/* shl	ebx,cl*/
				_or_r32_r32(REG_EAX, REG_EBX);										/* or	eax,ebx*/
			}
			
			_pop_r32(REG_EBX);														/* pop	ebx*/
			_and_r32_imm(REG_EBX, ~3);												/* and	ebx,~3*/
			_push_r32(REG_EAX);														/* push	eax*/
			_push_r32(REG_EBX);														/* push	ebx*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/

			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2f:	/* CACHE */
			return RECOMPILE_SUCCESSFUL_CP(1,4);
#if 0

			case 0x30:	/* LL */		log_cb(RETRO_LOG_DEBUG, LOGPRE "mips3 Unhandled op: LL\n");									break;

#endif

		case 0x31:	/* LWC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			_mov_m32abs_r32(&mips3.cpr[1][RTREG], REG_EAX);							/* mov	[rtreg],eax*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x32:	/* LWC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add  esp,4*/
			_mov_m32abs_r32(&mips3.cpr[2][RTREG], REG_EAX);							/* mov	[rtreg],eax*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x33:	/* PREF */
			if (mips3.is_mips4)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_jmp((void *)mips3.generate_invalidop_exception);					/* jmp	generate_invalidop_exception*/
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}

/*		case 0x34:	// LLD		log_cb(RETRO_LOG_DEBUG, LOGPRE "mips3 Unhandled op: LLD\n");									break; */

		case 0x35:	/* LDC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.cpr[1][RTREG]) : LO(&mips3.cpr[1][RTREG]), REG_EAX);/* mov	[rtreg].hi/lo,eax*/

			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL+4);									/* add	eax,SIMMVAL+4*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												/* push	SIMMVAL+4*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.cpr[1][RTREG]) : HI(&mips3.cpr[1][RTREG]), REG_EAX);/* mov	[rtreg].lo/hi,eax*/

			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x36:	/* LDC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.cpr[2][RTREG]) : LO(&mips3.cpr[2][RTREG]), REG_EAX);/* mov	[rtreg].hi/lo,eax*/

			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL+4);									/* add	eax,SIMMVAL+4*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												/* push	SIMMVAL+4*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.cpr[2][RTREG]) : HI(&mips3.cpr[2][RTREG]), REG_EAX);/* mov	[rtreg].lo/hi,eax*/

			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x37:	/* LD */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			if (RTREG != 0)
				_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.r[RTREG]) : LO(&mips3.r[RTREG]), REG_EAX);	/* mov	[rtreg].hi/lo,eax*/

			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL+4);									/* add	eax,SIMMVAL+4*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												/* push	SIMMVAL+4*/
			_call((void *)mips3.memory.readlong);									/* call	readlong*/
			if (RTREG != 0)
				_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.r[RTREG]) : HI(&mips3.r[RTREG]), REG_EAX);	/* mov	[rtreg].lo/hi,eax*/

			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

/*		case 0x38:	// SC 	log_cb(RETRO_LOG_DEBUG, LOGPRE "mips3 Unhandled op: SC\n");									break;*/

		case 0x39:	/* SWC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_m32abs(&mips3.cpr[1][RTREG]);										/* push	dword [rtreg]*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.writelong);											/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3a:	/* SWC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_m32abs(&mips3.cpr[2][RTREG]);										/* push	dword [rtreg]*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call(mips3.memory.writelong);											/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

/*		case 0x3b:	// SWC3		invalid_instruction(op);												break;*/
/*		case 0x3c:	// SCD		log_cb(RETRO_LOG_DEBUG, LOGPRE "mips3 Unhandled op: SCD\n");									break;*/

		case 0x3d:	/* SDC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_m32abs(mips3.bigendian ? HI(&mips3.cpr[1][RTREG]) : LO(&mips3.cpr[1][RTREG]));/* push	dword [rtreg].lo/hi*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			
			_push_m32abs(mips3.bigendian ? LO(&mips3.cpr[1][RTREG]) : HI(&mips3.cpr[1][RTREG]));/* push	dword [rtreg].hi/lo*/
			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL+4);									/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												/* push	SIMMVAL*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			
			_add_r32_imm(REG_ESP, 16);												/* add	esp,16*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3e:	/* SDC2 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_m32abs(mips3.bigendian ? HI(&mips3.cpr[2][RTREG]) : LO(&mips3.cpr[2][RTREG]));/* push	dword [rtreg].lo/hi*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			
			_push_m32abs(mips3.bigendian ? LO(&mips3.cpr[2][RTREG]) : HI(&mips3.cpr[2][RTREG]));/* push	dword [rtreg].hi/lo*/
			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL+4);									/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												/* push	SIMMVAL*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			
			_add_r32_imm(REG_ESP, 16);												/* add	esp,16*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3f:	/* SD */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			if (RTREG != 0)
				_push_m32abs(mips3.bigendian ? HI(&mips3.r[RTREG]) : LO(&mips3.r[RTREG]));/* push	dword [rtreg].lo/hi*/
			else
				_push_imm(0);														/* push	0*/
			if (RSREG != 0 && SIMMVAL != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL);										/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if (SIMMVAL != 0)
				_push_imm(SIMMVAL);													/* push	SIMMVAL*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			
			if (RTREG != 0)
				_push_m32abs(mips3.bigendian ? LO(&mips3.r[RTREG]) : HI(&mips3.r[RTREG]));/* push	dword [rtreg].hi/lo*/
			else
				_push_imm(0);														/* push	0*/
			if (RSREG != 0 && (SIMMVAL+4) != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_imm(REG_EAX, SIMMVAL+4);									/* add	eax,SIMMVAL*/
				_push_r32(REG_EAX);													/* push	eax*/
			}
			else if (RSREG != 0)
				_push_m32abs(&mips3.r[RSREG]);										/* push	[rsreg]*/
			else if ((SIMMVAL+4) != 0)
				_push_imm(SIMMVAL+4);												/* push	SIMMVAL*/
			_call((void *)mips3.memory.writelong);									/* call	writelong*/
			
			_add_r32_imm(REG_ESP, 16);												/* add	esp,16*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
				
/*		default:	*/ /*invalid_instruction(op);												break;*/
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
	recompile_special
------------------------------------------------------------------*/

static UINT32 recompile_special(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata link1, link2, link3;
	int cycles;
	
	switch (op & 63)
	{
		case 0x00:	/* SLL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					if (SHIFT != 0)
						_shl_r32_imm(REG_EAX, SHIFT);								/* shl	eax,SHIFT*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x01:	/* MOVF - R5000*/
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					/* jmp	generate_invalidop_exception*/
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			_cmp_m8abs_imm(&mips3.cf[1][(op >> 18) & 7], 0);						/* cmp	[cf[x]],0*/
			_jcc_short_link(((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);			/* jz/nz skip*/
			_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);						/* mov	edx:eax,[rsreg]*/
			_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);						/* mov	[rdreg],edx:eax*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* SRL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					if (SHIFT != 0)
						_shr_r32_imm(REG_EAX, SHIFT);								/* shr	eax,SHIFT*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x03:	/* SRA */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					if (SHIFT != 0)
						_sar_r32_imm(REG_EAX, SHIFT);								/* sar	eax,SHIFT*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* SLLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					/* mov	ecx,[rsreg]*/
						_shl_r32_cl(REG_EAX);										/* shl	eax,cl*/
					}
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* SRLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					/* mov	ecx,[rsreg]*/
						_shr_r32_cl(REG_EAX);										/* shr	eax,cl*/
					}
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x07:	/* SRAV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					/* mov	ecx,[rsreg]*/
						_sar_r32_cl(REG_EAX);										/* sar	eax,cl*/
					}
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* JR */
			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			_mov_r32_m32abs(REG_EDI, &mips3.r[RSREG]);								/* mov	edi,[rsreg]*/
			return RECOMPILE_SUCCESSFUL_CP(1+cycles,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case 0x09:	/* JALR */
			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			if (RDREG != 0)
				_mov_m64abs_imm32(&mips3.r[RDREG], pc + 8);							/* mov	[rdreg],pc + 8*/
			_mov_r32_m32abs(REG_EDI, &mips3.r[RSREG]);								/* mov	edi,[rsreg]*/
			return RECOMPILE_SUCCESSFUL_CP(1+cycles,0) | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

		case 0x0a:	/* MOVZ - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					/* jmp	generate_invalidop_exception*/
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			if (RDREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* mov	eax,[rtreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* or	eax,[rtreg].hi*/
				_jcc_short_link(COND_NZ, &link1);									/* jnz	skip*/
				_mov_m64abs_m64abs(&mips3.r[RDREG],  &mips3.r[RSREG]);				/* mov	[rdreg],[rsreg]*/
				_resolve_link(&link1);												/* skip:*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0b:	/* MOVN - R5000 */
			if (!mips3.is_mips4)
			{
				_jmp((void *)mips3.generate_invalidop_exception);					/* jmp	generate_invalidop_exception*/
				return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;
			}
			if (RDREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* mov	eax,[rtreg].lo*/
				_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* or	eax,[rtreg].hi*/
				_jcc_short_link(COND_Z, &link1);									/* jz	skip*/
				_mov_m64abs_m64abs(&mips3.r[RDREG],  &mips3.r[RSREG]);				/* mov	[rdreg],[rsreg]*/
				_resolve_link(&link1);												/* skip:*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x0c:	/* SYSCALL */
			_jmp((void *)mips3.generate_syscall_exception);							/* jmp	generate_syscall_exception*/
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x0d:	/* BREAK */
			_jmp((void *)mips3.generate_break_exception);							/* jmp	generate_break_exception*/
			return RECOMPILE_SUCCESSFUL | RECOMPILE_MAY_CAUSE_EXCEPTION | RECOMPILE_END_OF_STRING;

		case 0x0f:	/* SYNC */
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x10:	/* MFHI */
			if (RDREG != 0)
				_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.hi);						/* mov	[rdreg],[hi]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x11:	/* MTHI */
			_mov_m64abs_m64abs(&mips3.hi, &mips3.r[RSREG]);							/* mov	[hi],[rsreg]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x12:	/* MFLO */
			if (RDREG != 0)
				_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.lo);						/* mov	[rdreg],[lo]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x13:	/* MTLO */
			_mov_m64abs_m64abs(&mips3.lo,  &mips3.r[RSREG]);						/* mov	[lo],[rsreg]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x14:	/* DSLLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				/* mov	edx:eax,[rtreg]*/
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					/* mov	ecx,[rsreg]*/
						_test_r32_imm(REG_ECX, 0x20);								/* test	ecx,0x20*/
						_jcc_short_link(COND_Z, &link1);							/* jz	skip*/
						_mov_r32_r32(REG_EDX, REG_EAX);								/* mov	edx,eax*/
						_xor_r32_r32(REG_EAX, REG_EAX);								/* xor	eax,eax*/
						_resolve_link(&link1);										/* skip:*/
						_shld_r32_r32_cl(REG_EDX, REG_EAX);							/* shld	edx,eax,cl*/
						_shl_r32_cl(REG_EAX);										/* shl	eax,cl*/
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x16:	/* DSRLV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				/* mov	edx:eax,[rtreg]*/
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					/* mov	ecx,[rsreg]*/
						_test_r32_imm(REG_ECX, 0x20);								/* test	ecx,0x20*/
						_jcc_short_link(COND_Z, &link1);							/* jz	skip*/
						_mov_r32_r32(REG_EAX, REG_EDX);								/* mov	eax,edx*/
						_xor_r32_r32(REG_EDX, REG_EDX);								/* xor	edx,edx*/
						_resolve_link(&link1);										/* skip:*/
						_shrd_r32_r32_cl(REG_EAX, REG_EDX);							/* shrd	eax,edx,cl*/
						_shr_r32_cl(REG_EDX);										/* shr	edx,cl*/
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x17:	/* DSRAV */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				/* mov	edx:eax,[rtreg]*/
					if (RSREG != 0)
					{
						_mov_r32_m32abs(REG_ECX, &mips3.r[RSREG]);					/* mov	ecx,[rsreg]*/
						_test_r32_imm(REG_ECX, 0x20);								/* test	ecx,0x20*/
						_jcc_short_link(COND_Z, &link1);							/* jz	skip*/
						_mov_r32_r32(REG_EAX, REG_EDX);								/* mov	eax,edx*/
						_cdq();														/* cdq*/
						_resolve_link(&link1);										/* skip:*/
						_shrd_r32_r32_cl(REG_EAX, REG_EDX);							/* shrd	eax,edx,cl*/
						_sar_r32_cl(REG_EDX);										/* sar	edx,cl*/
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x18:	/* MULT */
			_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);								/* mov	ecx,[rtreg].lo*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg].lo*/
			_imul_r32(REG_ECX);														/* imul	ecx*/
			_push_r32(REG_EDX);														/* push	edx*/
			_cdq();																	/* cdq*/
			_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);							/* mov	[lo],edx:eax*/
			_pop_r32(REG_EAX);														/* pop	eax*/
			_cdq();																	/* cdq*/
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);							/* mov	[hi],edx:eax*/
			return RECOMPILE_SUCCESSFUL_CP(4,4);

		case 0x19:	/* MULTU */
			_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);								/* mov	ecx,[rtreg].lo*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg].lo*/
			_mul_r32(REG_ECX);														/* mul	ecx*/
			_push_r32(REG_EDX);														/* push	edx*/
			_cdq();																	/* cdq*/
			_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);							/* mov	[lo],edx:eax*/
			_pop_r32(REG_EAX);														/* pop	eax*/
			_cdq();																	/* cdq*/
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);							/* mov	[hi],edx:eax*/
			return RECOMPILE_SUCCESSFUL_CP(4,4);

		case 0x1a:	/* DIV */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);							/* mov	ecx,[rtreg].lo*/
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg].lo*/
				_cdq();																/* cdq*/
				_cmp_r32_imm(REG_ECX, 0);											/* cmp	ecx,0*/
				_jcc_short_link(COND_E, &link1);									/* je	skip*/
				_idiv_r32(REG_ECX);													/* idiv	ecx*/
				_push_r32(REG_EDX);													/* push	edx*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);						/* mov	[lo],edx:eax*/
				_pop_r32(REG_EAX);													/* pop	eax*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);						/* mov	[hi],edx:eax*/
				_resolve_link(&link1);												/* skip:*/
			}
			return RECOMPILE_SUCCESSFUL_CP(36,4);
			
		case 0x1b:	/* DIVU */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_ECX, &mips3.r[RTREG]);							/* mov	ecx,[rtreg].lo*/
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg].lo*/
				_xor_r32_r32(REG_EDX, REG_EDX);										/* xor	edx,edx*/
				_cmp_r32_imm(REG_ECX, 0);											/* cmp	ecx,0*/
				_jcc_short_link(COND_E, &link1);									/* je	skip*/
				_div_r32(REG_ECX);													/* div	ecx*/
				_push_r32(REG_EDX);													/* push	edx*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.lo, REG_EDX, REG_EAX);						/* mov	[lo],edx:eax*/
				_pop_r32(REG_EAX);													/* pop	eax*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_EAX);						/* mov	[hi],edx:eax*/
				_resolve_link(&link1);												/* skip:*/
			}
			return RECOMPILE_SUCCESSFUL_CP(36,4);

		case 0x1c:	/* DMULT */
			_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);						/* mov	edx:eax,[rsreg]*/
			_cmp_r32_imm(REG_EDX, 0);												/* cmp	edx,0*/
			_jcc_short_link(COND_GE, &link1);										/* jge	skip1*/
			_mov_r32_r32(REG_ECX, REG_EDX);											/* mov	ecx,edx*/
			_xor_r32_r32(REG_EDX, REG_EDX);											/* xor	edx,edx*/
			_neg_r32(REG_EAX);														/* neg	eax*/
			_sbb_r32_r32(REG_EDX, REG_ECX);											/* sbb	edx,ecx*/
			_resolve_link(&link1);													/* skip1:*/
			_mov_m64abs_r64(&dmult_temp1, REG_EDX, REG_EAX);						/* mov	[dmult_temp1],edx:eax*/
		
			_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);						/* mov	edx:eax,[rtreg]*/
			_cmp_r32_imm(REG_EDX, 0);												/* cmp	edx,0*/
			_jcc_short_link(COND_GE, &link2);										/* jge	skip2*/
			_mov_r32_r32(REG_ECX, REG_EDX);											/* mov	ecx,edx*/
			_xor_r32_r32(REG_EDX, REG_EDX);											/* xor	edx,edx*/
			_neg_r32(REG_EAX);														/* neg	eax*/
			_sbb_r32_r32(REG_EDX, REG_ECX);											/* sbb	edx,ecx*/
			_resolve_link(&link2);													/* skip2:*/
			_mov_m64abs_r64(&dmult_temp2, REG_EDX, REG_EAX);						/* mov	[dmult_temp2],edx:eax*/
		
			_mov_r32_m32abs(REG_EAX, LO(&dmult_temp1));								/* mov	eax,[dmult_temp1].lo*/
			_mul_m32abs(LO(&dmult_temp2));											/* mul	[dmult_temp2].lo*/
			_mov_r32_r32(REG_ECX, REG_EDX);											/* mov	ecx,edx*/
			_xor_r32_r32(REG_EBX, REG_EBX);											/* xor	ebx,ebx*/
			_mov_m32abs_r32(LO(&mips3.lo), REG_EAX);								/* mov	[lo].lo,eax*/

			_mov_r32_m32abs(REG_EAX, HI(&dmult_temp1));								/* mov	eax,[dmult_temp1].hi*/
			_mul_m32abs(LO(&dmult_temp2));											/* mul	[dmult_temp2].lo*/
			_add_r32_r32(REG_ECX, REG_EAX);											/* add	ecx,eax*/
			_adc_r32_r32(REG_EBX, REG_EDX);											/* adc	ebx,edx*/
			
			_mov_r32_m32abs(REG_EAX, LO(&dmult_temp1));								/* mov	eax,[dmult_temp1].lo*/
			_mul_m32abs(HI(&dmult_temp2));											/* mul	[dmult_temp2].hi*/
			_add_r32_r32(REG_ECX, REG_EAX);											/* add	ecx,eax*/
			_adc_r32_r32(REG_EBX, REG_EDX);											/* adc	ebx,edx*/
			_mov_m32abs_r32(HI(&mips3.lo), REG_ECX);								/* mov	[lo].hi,ecx*/
			
			_mov_r32_m32abs(REG_EAX, HI(&dmult_temp1));								/* mov	eax,[dmult_temp1].hi*/
			_mul_m32abs(HI(&dmult_temp2));											/* mul	[dmult_temp2].hi*/
			_add_r32_r32(REG_EBX, REG_EAX);											/* add	ebx,eax*/
			_adc_r32_imm(REG_EDX, 0);												/* adc	edx,0*/
			_mov_m32abs_r32(LO(&mips3.hi), REG_EBX);								/* mov	[hi].lo,ebx*/
			_mov_m32abs_r32(HI(&mips3.hi), REG_EDX);								/* mov	[hi].hi,edx*/
			
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_xor_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							/* xor	eax,[rtreg].hi*/
			_jcc_short_link(COND_NS, &link3);										/* jns	noflip*/
			_xor_r32_r32(REG_EAX, REG_EAX);											/* xor	eax,eax*/
			_xor_r32_r32(REG_EBX, REG_EBX);											/* xor	ebx,ebx*/
			_xor_r32_r32(REG_ECX, REG_ECX);											/* xor	ecx,ecx*/
			_xor_r32_r32(REG_EDX, REG_EDX);											/* xor	edx,edx*/
			_sub_r32_m32abs(REG_EAX, LO(&mips3.lo));								/* sub	eax,[lo].lo*/
			_sbb_r32_m32abs(REG_EBX, HI(&mips3.lo));								/* sbb	ebx,[lo].hi*/
			_sbb_r32_m32abs(REG_ECX, LO(&mips3.hi));								/* sbb	ecx,[hi].lo*/
			_sbb_r32_m32abs(REG_EDX, HI(&mips3.hi));								/* sbb	edx,[hi].hi*/
			_mov_m64abs_r64(&mips3.lo, REG_EBX, REG_EAX);							/* mov	[lo],ebx:eax*/
			_mov_m64abs_r64(&mips3.hi, REG_EDX, REG_ECX);							/* mov	[lo],edx:ecx*/
			_resolve_link(&link3);													/* noflip:*/
			return RECOMPILE_SUCCESSFUL_CP(8,4);

		case 0x1d:	/* DMULTU */
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							/* mov	eax,[rsreg].lo*/
			_mul_m32abs(LO(&mips3.r[RTREG]));										/* mul	[rtreg].lo*/
			_mov_r32_r32(REG_ECX, REG_EDX);											/* mov	ecx,edx*/
			_xor_r32_r32(REG_EBX, REG_EBX);											/* xor	ebx,ebx*/
			_mov_m32abs_r32(LO(&mips3.lo), REG_EAX);								/* mov	[lo].lo,eax*/

			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_mul_m32abs(LO(&mips3.r[RTREG]));										/* mul	[rtreg].lo*/
			_add_r32_r32(REG_ECX, REG_EAX);											/* add	ecx,eax*/
			_adc_r32_r32(REG_EBX, REG_EDX);											/* adc	ebx,edx*/
			
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							/* mov	eax,[rsreg].lo*/
			_mul_m32abs(HI(&mips3.r[RTREG]));										/* mul	[rtreg].hi*/
			_add_r32_r32(REG_ECX, REG_EAX);											/* add	ecx,eax*/
			_adc_r32_r32(REG_EBX, REG_EDX);											/* adc	ebx,edx*/
			_mov_m32abs_r32(HI(&mips3.lo), REG_ECX);								/* mov	[lo].hi,ecx*/
			
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_mul_m32abs(HI(&mips3.r[RTREG]));										/* mul	[rtreg].hi*/
			_add_r32_r32(REG_EBX, REG_EAX);											/* add	ebx,eax*/
			_adc_r32_imm(REG_EDX, 0);												/* adc	edx,0*/
			_mov_m32abs_r32(LO(&mips3.hi), REG_EBX);								/* mov	[hi].lo,ebx*/
			_mov_m32abs_r32(HI(&mips3.hi), REG_EDX);								/* mov	[hi].hi,edx*/
			return RECOMPILE_SUCCESSFUL_CP(8,4);
					
		case 0x1e:	/* DDIV */
			_push_imm(&mips3.r[RTREG]);												/* push	[rtreg]*/
			_push_imm(&mips3.r[RSREG]);												/* push	[rsreg]*/
			_call((void *)ddiv);													/* call ddiv*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			return RECOMPILE_SUCCESSFUL_CP(68,4);

		case 0x1f:	/* DDIVU */
			_push_imm(&mips3.r[RTREG]);												/* push	[rtreg]*/
			_push_imm(&mips3.r[RSREG]);												/* push	[rsreg]*/
			_call((void *)ddivu);													/* call ddivu*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			return RECOMPILE_SUCCESSFUL_CP(68,4);

		case 0x20:	/* ADD */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* add	eax,[rtreg]*/
				_jcc(COND_O, mips3.generate_overflow_exception);					/* jo	generate_overflow_exception*/
				if (RDREG != 0)
				{
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x21:	/* ADDU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* add	eax,[rtreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x22:	/* SUB */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);							/* mov	eax,[rsreg]*/
				_sub_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* sub	eax,[rtreg]*/
				_jcc(COND_O, mips3.generate_overflow_exception);					/* jo	generate_overflow_exception*/
				if (RDREG != 0)
				{
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					_neg_r32(REG_EAX);												/* neg	eax*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x23:	/* SUBU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_sub_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* sub	eax,[rtreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RSREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);						/* mov	eax,[rsreg]*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);						/* mov	eax,[rtreg]*/
					_neg_r32(REG_EAX);												/* neg	eax*/
					_cdq();															/* cdq*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:	/* AND */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				/* movsd xmm0,[rsreg]*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_pand_r128_r128(REG_XMM0, REG_XMM1);						/* pand	xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			/* mov	edx:eax,[rsreg]*/
						_and_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* and	edx,[rtreg].hi*/
						_and_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* and	eax,[rtreg].lo*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x25:	/* OR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				/* movsd xmm0,[rsreg]*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_por_r128_r128(REG_XMM0, REG_XMM1);							/* por	xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			/* mov	edx:eax,[rsreg]*/
						_or_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* or	edx,[rtreg].hi*/
						_or_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* or	eax,[rtreg].lo*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			/* mov	[rdreg],[rsreg]*/
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			/* mov	[rdreg],[rtreg]*/
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x26:	/* XOR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				/* movsd xmm0,[rsreg]*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_pxor_r128_r128(REG_XMM0, REG_XMM1);						/* pxor	xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			/* mov	edx:eax,[rsreg]*/
						_xor_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* xor	edx,[rtreg].hi*/
						_xor_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* xor	eax,[rtreg].lo*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			/* mov	[rdreg],[rsreg]*/
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			/* mov	[rdreg],[rtreg]*/
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x27:	/* NOR */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
					_or_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));					/* or	edx,[rtreg].hi*/
					_or_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					/* or	eax,[rtreg].lo*/
					_not_r32(REG_EDX);												/* not	edx*/
					_not_r32(REG_EAX);												/* not	eax*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RSREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
					_not_r32(REG_EDX);												/* not	edx*/
					_not_r32(REG_EAX);												/* not	eax*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				/* mov	edx:eax,[rtreg]*/
					_not_r32(REG_EDX);												/* not	edx*/
					_not_r32(REG_EAX);												/* not	eax*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
				{
					_mov_m32abs_imm(LO(&mips3.r[RDREG]), ~0);						/* mov	[rtreg].lo,~0*/
					_mov_m32abs_imm(HI(&mips3.r[RDREG]), ~0);						/* mov	[rtreg].hi,~0*/
				}
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2a:	/* SLT */
			if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
				else
				{
					_xor_r32_r32(REG_EDX, REG_EDX);									/* xor	edx,edx*/
					_xor_r32_r32(REG_EAX, REG_EAX);									/* xor	eax,eax*/
				}
				if (RTREG != 0)
				{
					_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					/* sub	eax,[rtreg].lo*/
					_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));					/* sbb	edx,[rtreg].lo*/
				}
				_shr_r32_imm(REG_EDX, 31);											/* shr	edx,31*/
				_mov_m32abs_r32(LO(&mips3.r[RDREG]), REG_EDX);						/* mov	[rdreg].lo,edx*/
				_mov_m32abs_imm(HI(&mips3.r[RDREG]), 0);							/* mov	[rdreg].hi,0*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x2b:	/* SLTU */
			if (RDREG != 0)
			{
				_xor_r32_r32(REG_ECX, REG_ECX);										/* xor	ecx,ecx*/
				_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));						/* mov	eax,[rsreg].hi*/
				_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].hi*/
				_jcc_short_link(COND_B, &link1);									/* jb	setit*/
				_jcc_short_link(COND_A, &link2);									/* ja	skipit*/
				_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));						/* mov	eax,[rsreg].lo*/
				_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* cmp	eax,[rtreg].lo*/
				_jcc_short_link(COND_AE, &link3);									/* jae	skipit*/
				_resolve_link(&link1);												/* setit:*/
				_add_r32_imm(REG_ECX, 1);											/* add	ecx,1*/
				_resolve_link(&link2);												/* skipit:*/
				_resolve_link(&link3);												/* skipit:*/
				_mov_m32abs_r32(LO(&mips3.r[RDREG]), REG_ECX);						/* mov	[rdreg].lo,ecx*/
				_mov_m32abs_imm(HI(&mips3.r[RDREG]), 0);							/* mov	[rdreg].hi,0*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2c:	/* DADD */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					/* mov	edx:eax,[rsreg]*/
				_add_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* add	eax,[rtreg].lo*/
				_adc_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						/* adc	edx,[rtreg].hi*/
				_jcc(COND_O, mips3.generate_overflow_exception);					/* jo	generate_overflow_exception*/
				if (RDREG != 0)
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			/* mov	[rdreg],[rsreg]*/
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			/* mov	[rdreg],[rtreg]*/
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;
			
		case 0x2d:	/* DADDU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				/* movsd xmm0,[rsreg]*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_paddq_r128_r128(REG_XMM0, REG_XMM1);						/* paddq xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			/* mov	edx:eax,[rsreg]*/
						_add_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* add	eax,[rtreg].lo*/
						_adc_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* adc	edx,[rtreg].hi*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			/* mov	[rdreg],[rsreg]*/
				else if (RTREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RTREG]);			/* mov	[rdreg],[rtreg]*/
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x2e:	/* DSUB */
			if (RSREG != 0 && RTREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					/* mov	edx:eax,[rsreg]*/
				_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* sub	eax,[rtreg].lo*/
				_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						/* sbb	edx,[rtreg].hi*/
				_jcc(COND_O, mips3.generate_overflow_exception);					/* jo	generate_overflow_exception*/
				if (RDREG != 0)
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
			}
			else if (RDREG != 0)
			{
				if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			/* mov	[rdreg],[rsreg]*/
				else if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_pxor_r128_r128(REG_XMM0, REG_XMM0);						/* pxor	xmm0,xmm0*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						/* psubq xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_xor_r32_r32(REG_EAX, REG_EAX);								/* xor	eax,eax*/
						_xor_r32_r32(REG_EDX, REG_EDX);								/* xor	edx,edx*/
						_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* sub	eax,[rtreg].lo*/
						_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* sbb	edx,[rtreg].hi*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x2f:	/* DSUBU */
			if (RDREG != 0)
			{
				if (RSREG != 0 && RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RSREG]);				/* movsd xmm0,[rsreg]*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						/* psubq xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);			/* mov	edx:eax,[rsreg]*/
						_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* sub	eax,[rtreg].lo*/
						_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* sbb	edx,[rtreg].hi*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else if (RSREG != 0)
					_mov_m64abs_m64abs(&mips3.r[RDREG], &mips3.r[RSREG]);			/* mov	[rdreg],[rsreg]*/
				else if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_pxor_r128_r128(REG_XMM0, REG_XMM0);						/* pxor	xmm0,xmm0*/
						_movsd_r128_m64abs(REG_XMM1, &mips3.r[RTREG]);				/* movsd xmm1,[rtreg]*/
						_psubq_r128_r128(REG_XMM0, REG_XMM1);						/* psubq xmm0,xmm1*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* mov	[rdreg],xmm0*/
					}
					else
					{
						_xor_r32_r32(REG_EAX, REG_EAX);								/* xor	eax,eax*/
						_xor_r32_r32(REG_EDX, REG_EDX);								/* xor	edx,edx*/
						_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* sub	eax,[rtreg].lo*/
						_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));				/* sbb	edx,[rtreg].hi*/
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x30:	/* TGE */
			if (RSREG != 0)
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					/* mov	edx:eax,[rsreg]*/
			else
			{
				_xor_r32_r32(REG_EDX, REG_EDX);										/* xor	edx,edx*/
				_xor_r32_r32(REG_EAX, REG_EAX);										/* xor	eax,eax*/
			}
			if (RTREG != 0)
			{
				_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* sub	eax,[rtreg].lo*/
				_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						/* sbb	edx,[rtreg].hi*/
			}
			else
				_cmp_r32_imm(REG_EDX, 0);											/* cmp	edx,0*/
			_jcc(COND_GE, mips3.generate_trap_exception);							/* jge	generate_trap_exception*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x31:	/* TGEU */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].hi*/
			_jcc(COND_A, mips3.generate_trap_exception);							/* ja	generate_trap_exception*/
			_jcc_short_link(COND_B, &link1);										/* jb	skipit*/
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							/* mov	eax,[rsreg].lo*/
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].lo*/
			_jcc(COND_AE, mips3.generate_trap_exception);							/* jae	generate_trap_exception*/
			_resolve_link(&link1);													/* skipit:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x32:	/* TLT */
			if (RSREG != 0)
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);					/* mov	edx:eax,[rsreg]*/
			else
			{
				_xor_r32_r32(REG_EDX, REG_EDX);										/* xor	edx,edx*/
				_xor_r32_r32(REG_EAX, REG_EAX);										/* xor	eax,eax*/
			}
			if (RTREG != 0)
			{
				_sub_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));						/* sub	eax,[rtreg].lo*/
				_sbb_r32_m32abs(REG_EDX, HI(&mips3.r[RTREG]));						/* sbb	edx,[rtreg].hi*/
			}
			else
				_cmp_r32_imm(REG_EDX, 0);											/* cmp	edx,0*/
			_jcc(COND_L, mips3.generate_trap_exception);							/* jl	generate_trap_exception*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x33:	/* TLTU */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].hi*/
			_jcc(COND_B, mips3.generate_trap_exception);							/* jb	generate_trap_exception*/
			_jcc_short_link(COND_A, &link1);										/* ja	skipit*/
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							/* mov	eax,[rsreg].lo*/
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].lo*/
			_jcc(COND_B, mips3.generate_trap_exception);							/* jb	generate_trap_exception*/
			_resolve_link(&link1);													/* skipit:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x34:	/* TEQ */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].hi*/
			_jcc_short_link(COND_NE, &link1);										/* jne	skipit*/
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							/* mov	eax,[rsreg].lo*/
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].lo*/
			_jcc(COND_E, mips3.generate_trap_exception);							/* je	generate_trap_exception*/
			_resolve_link(&link1);													/* skipit:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x36:	/* TNE */
			_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RSREG]));							/* mov	eax,[rsreg].hi*/
			_cmp_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].hi*/
			_jcc_short_link(COND_E, &link1);										/* je	skipit*/
			_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RSREG]));							/* mov	eax,[rsreg].lo*/
			_cmp_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));							/* cmp	eax,[rtreg].lo*/
			_jcc(COND_NE, mips3.generate_trap_exception);							/* jne	generate_trap_exception*/
			_resolve_link(&link1);													/* skipit:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x38:	/* DSLL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				/* movsd xmm0,[rtreg]*/
						if (SHIFT)
							_psllq_r128_imm(REG_XMM0, SHIFT);						/* psllq xmm0,SHIFT*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* movsd [rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);			/* mov	edx:eax,[rtreg]*/
						if (SHIFT != 0)
						{
							_shld_r32_r32_imm(REG_EDX, REG_EAX, SHIFT);				/* shld	edx,eax,SHIFT*/
							_shl_r32_imm(REG_EAX, SHIFT);							/* shl	eax,SHIFT*/
						}
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3a:	/* DSRL */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				/* movsd xmm0,[rtreg]*/
						if (SHIFT)
							_psrlq_r128_imm(REG_XMM0, SHIFT);						/* psrlq xmm0,SHIFT*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* movsd [rdreg],xmm0*/
					}
					else
					{
						_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);			/* mov	edx:eax,[rtreg]*/
						if (SHIFT != 0)
						{
							_shrd_r32_r32_imm(REG_EAX, REG_EDX, SHIFT);				/* shrd	eax,edx,SHIFT*/
							_shr_r32_imm(REG_EDX, SHIFT);							/* shr	edx,SHIFT*/
						}
						_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);			/* mov	[rdreg],edx:eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x3b:	/* DSRA */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RTREG]);				/* mov	edx:eax,[rtreg]*/
					if (SHIFT != 0)
					{
						_shrd_r32_r32_imm(REG_EAX, REG_EDX, SHIFT);					/* shrd	eax,edx,SHIFT*/
						_sar_r32_imm(REG_EDX, SHIFT);								/* sar	edx,SHIFT*/
					}
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3c:	/* DSLL32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				/* movsd xmm0,[rtreg]*/
						_psllq_r128_imm(REG_XMM0, SHIFT+32);						/* psllq xmm0,SHIFT+32*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* movsd [rdreg],xmm0*/
					}
					else
					{
						_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));				/* mov	eax,[rtreg].lo*/
						if (SHIFT != 0)
							_shl_r32_imm(REG_EAX, SHIFT);							/* shl	eax,SHIFT*/
						_mov_m32abs_imm(LO(&mips3.r[RDREG]), 0);					/* mov	[rdreg].lo,0*/
						_mov_m32abs_r32(HI(&mips3.r[RDREG]), REG_EAX);				/* mov	[rdreg].hi,eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3e:	/* DSRL32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					if (USE_SSE)
					{
						_movsd_r128_m64abs(REG_XMM0, &mips3.r[RTREG]);				/* movsd xmm0,[rtreg]*/
						_psrlq_r128_imm(REG_XMM0, SHIFT+32);						/* psrlq xmm0,SHIFT+32*/
						_movsd_m64abs_r128(&mips3.r[RDREG], REG_XMM0);				/* movsd [rdreg],xmm0*/
					}
					else
					{
						_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));				/* mov	eax,[rtreg].hi*/
						if (SHIFT != 0)
							_shr_r32_imm(REG_EAX, SHIFT);							/* shr	eax,SHIFT*/
						_mov_m32abs_imm(HI(&mips3.r[RDREG]), 0);					/* mov	[rdreg].hi,0*/
						_mov_m32abs_r32(LO(&mips3.r[RDREG]), REG_EAX);				/* mov	[rdreg].lo,eax*/
					}
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x3f:	/* DSRA32 */
			if (RDREG != 0)
			{
				if (RTREG != 0)
				{
					_mov_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));					/* mov	eax,[rtreg].hi*/
					_cdq();															/* cdq*/
					if (SHIFT != 0)
						_sar_r32_imm(REG_EAX, SHIFT);								/* sar	eax,SHIFT*/
					_mov_m64abs_r64(&mips3.r[RDREG], REG_EDX, REG_EAX);				/* mov	[rdreg],edx:eax*/
				}
				else
					_zero_m64abs(&mips3.r[RDREG]);
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								/* jmp	generate_invalidop_exception*/
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}


/*------------------------------------------------------------------
	recompile_regimm
------------------------------------------------------------------*/

static UINT32 recompile_regimm(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata link1;
	int cycles;
	
	switch (RTREG)
	{
		case 0x00:	/* BLTZ */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_GE, &link1);									/* jge	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x01:	/* BGEZ */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_L, &link1);										/* jl	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* BLTZL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_GE, &link1);									/* jge	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x03:	/* BGEZL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_L, &link1);										/* jl	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x08:	/* TGEI */
			if (RSREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
				_sub_r32_imm(REG_EAX, SIMMVAL);									/* sub	eax,[rtreg].lo*/
				_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					/* sbb	edx,[rtreg].lo*/
				_jcc(COND_GE, mips3.generate_trap_exception);					/* jge	generate_trap_exception*/
			}
			else
			{
				if (0 >= SIMMVAL)
					_jmp(mips3.generate_trap_exception);						/* jmp	generate_trap_exception*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x09:	/* TGEIU */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	/* cmp	[rsreg].hi,upper*/
				_jcc(COND_A, mips3.generate_trap_exception);					/* ja	generate_trap_exception*/
				_jcc_short_link(COND_B, &link1);								/* jb	skip*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					/* cmp	[rsreg].lo,lower*/
				_jcc(COND_AE, mips3.generate_trap_exception);					/* jae	generate_trap_exception*/
				_resolve_link(&link1);											/* skip:*/
			}
			else
			{
				if (0 >= SIMMVAL)
					_jmp(mips3.generate_trap_exception);						/* jmp	generate_trap_exception*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0a:	/* TLTI */
			if (RSREG != 0)
			{
				_mov_r64_m64abs(REG_EDX, REG_EAX, &mips3.r[RSREG]);				/* mov	edx:eax,[rsreg]*/
				_sub_r32_imm(REG_EAX, SIMMVAL);									/* sub	eax,[rtreg].lo*/
				_sbb_r32_imm(REG_EDX, ((INT32)SIMMVAL >> 31));					/* sbb	edx,[rtreg].lo*/
				_jcc(COND_L, mips3.generate_trap_exception);					/* jl	generate_trap_exception*/
			}
			else
			{
				if (0 < SIMMVAL)
					_jmp(mips3.generate_trap_exception);						/* jmp	generate_trap_exception*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0b:	/* TLTIU */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	/* cmp	[rsreg].hi,upper*/
				_jcc(COND_B, mips3.generate_trap_exception);					/* jb	generate_trap_exception*/
				_jcc_short_link(COND_A, &link1);								/* ja	skip*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					/* cmp	[rsreg].lo,lower*/
				_jcc(COND_B, mips3.generate_trap_exception);					/* jb	generate_trap_exception*/
				_resolve_link(&link1);											/* skip:*/
			}
			else
			{
				_mov_m32abs_imm(LO(&mips3.r[RTREG]), (0 < SIMMVAL));			/* mov	[rtreg].lo,const*/
				_mov_m32abs_imm(HI(&mips3.r[RTREG]), 0);						/* mov	[rtreg].hi,sign-extend(const)*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0c:	/* TEQI */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	/* cmp	[rsreg].hi,upper*/
				_jcc_short_link(COND_NE, &link1);								/* jne	skip*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					/* cmp	[rsreg].lo,lower*/
				_jcc(COND_E, mips3.generate_trap_exception);					/* je	generate_trap_exception*/
				_resolve_link(&link1);											/* skip:*/
			}
			else
			{
				if (0 == SIMMVAL)
					_jmp(mips3.generate_trap_exception);						/* jmp	generate_trap_exception*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x0e:	/* TNEI */
			if (RSREG != 0)
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), ((INT32)SIMMVAL >> 31));	/* cmp	[rsreg].hi,upper*/
				_jcc_short_link(COND_E, &link1);								/* je	skip*/
				_cmp_m32abs_imm(LO(&mips3.r[RSREG]), SIMMVAL);					/* cmp	[rsreg].lo,lower*/
				_jcc(COND_NE, mips3.generate_trap_exception);					/* jne	generate_trap_exception*/
				_resolve_link(&link1);											/* skip:*/
			}
			else
			{
				if (0 != SIMMVAL)
					_jmp(mips3.generate_trap_exception);						/* jmp	generate_trap_exception*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_MAY_CAUSE_EXCEPTION;

		case 0x10:	/* BLTZAL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_GE, &link1);									/* jge	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								/* mov	[31],pc + 8*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x11:	/* BGEZAL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				_mov_m64abs_imm32(&mips3.r[31], pc + 8);							/* mov	[31],pc + 8*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_L, &link1);										/* jl	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								/* mov	[31],pc + 8*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x12:	/* BLTZALL */
			if (RSREG == 0)
				return RECOMPILE_SUCCESSFUL_CP(1,4);
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_GE, &link1);									/* jge	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								/* mov	[31],pc + 8*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);

		case 0x13:	/* BGEZALL */
			if (RSREG == 0)
			{
				cycles = recompile_delay_slot(drc, pc + 4);							/* <next instruction>*/
				_mov_m64abs_imm32(&mips3.r[31], pc + 8);							/* mov	[31],pc + 8*/
				append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);	/* <branch or dispatch>*/
				return RECOMPILE_SUCCESSFUL_CP(0,0) | RECOMPILE_END_OF_STRING;
			}
			else
			{
				_cmp_m32abs_imm(HI(&mips3.r[RSREG]), 0);							/* cmp	[rsreg].hi,0*/
				_jcc_near_link(COND_L, &link1);										/* jl	skip*/
			}

			cycles = recompile_delay_slot(drc, pc + 4);								/* <next instruction>*/
			_mov_m64abs_imm32(&mips3.r[31], pc + 8);								/* mov	[31],pc + 8*/
			append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);		/* <branch or dispatch>*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,8);
	}

	_jmp((void *)mips3.generate_invalidop_exception);								/* jmp	generate_invalidop_exception*/
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}



/*###################################################################################################
**	COP0 RECOMPILATION
**#################################################################################################*/

/*------------------------------------------------------------------
	recompile_set_cop0_reg
------------------------------------------------------------------*/

static UINT32 recompile_set_cop0_reg(struct drccore *drc, UINT8 reg)
{
	struct linkdata link1;
	
	switch (reg)
	{
		case COP0_Cause:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Cause]);					/* mov	ebx,[mips3.cpr[0][COP0_Cause]]*/
			_and_r32_imm(REG_EAX, ~0xfc00);											/* and	eax,~0xfc00*/
			_and_r32_imm(REG_EBX, 0xfc00);											/* and	ebx,0xfc00*/
			_or_r32_r32(REG_EAX, REG_EBX);											/* or	eax,ebx*/
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Cause], REG_EAX);					/* mov	[mips3.cpr[0][COP0_Cause]],eax*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_CHECK_INTERRUPTS;
		
		case COP0_Status:
			_mov_r32_m32abs(REG_EBX, &mips3.cpr[0][COP0_Status]);					/* mov	ebx,[mips3.cpr[0][COP0_Status]]*/
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Status], REG_EAX);					/* mov	[mips3.cpr[0][COP0_Status]],eax*/
			_xor_r32_r32(REG_EAX, REG_EBX);											/* xor	eax,ebx*/
			_test_r32_imm(REG_EAX, 0x8000);											/* test	eax,0x8000*/
			_jcc_short_link(COND_Z, &link1);										/* jz	skip*/
			append_update_cycle_counting(drc);										/* update cycle counting*/
			_resolve_link(&link1);													/* skip:*/
			return RECOMPILE_SUCCESSFUL_CP(1,4) | RECOMPILE_CHECK_INTERRUPTS;
					
		case COP0_Count:
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Count], REG_EAX);					/* mov	[mips3.cpr[0][COP0_Count]],eax*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_r32(REG_EAX);														/* push eax*/
			_call((void *)activecpu_gettotalcycles64);								/* call	activecpu_gettotalcycles64*/
			_pop_r32(REG_EBX);														/* pop	ebx*/
			_sub_r32_r32(REG_EAX, REG_EBX);											/* sub	eax,ebx*/
			_sbb_r32_imm(REG_EDX, 0);												/* sbb	edx,0*/
			_sub_r32_r32(REG_EAX, REG_EBX);											/* sub	eax,ebx*/
			_sbb_r32_imm(REG_EDX, 0);												/* sbb	edx,0*/
			_mov_m64abs_r64(&mips3.count_zero_time, REG_EDX, REG_EAX);				/* mov	[mips3.count_zero_time],edx:eax*/
			append_update_cycle_counting(drc);										/* update cycle counting*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Compare:
			_mov_m32abs_r32(&mips3.cpr[0][COP0_Compare], REG_EAX);					/* mov	[mips3.cpr[0][COP0_Compare]],eax*/
			_and_m32abs_imm(&mips3.cpr[0][COP0_Cause], ~0x8000);					/* and	[mips3.cpr[0][COP0_Cause]],~0x8000*/
			append_update_cycle_counting(drc);										/* update cycle counting*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case COP0_PRId:
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		default:
			_mov_m32abs_r32(&mips3.cpr[0][reg], REG_EAX);							/* mov	[mips3.cpr[0][reg]],eax*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
	recompile_get_cop0_reg
------------------------------------------------------------------*/

static UINT32 recompile_get_cop0_reg(struct drccore *drc, UINT8 reg)
{
	struct linkdata link1;

	switch (reg)
	{
		case COP0_Count:
			_sub_r32_imm(REG_EBP, 24);												/* sub  ebp,24*/
			_jcc_short_link(COND_NS, &link1);										/* jns	notneg*/
			_xor_r32_r32(REG_EBP, REG_EBP);											/* xor	ebp,ebp*/
			_resolve_link(&link1);													/* notneg:*/
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_call((void *)activecpu_gettotalcycles64);								/* call	activecpu_gettotalcycles64*/
			_sub_r32_m32abs(REG_EAX, LO(&mips3.count_zero_time));					/* sub	eax,[mips3.count_zero_time+0]*/
			_sbb_r32_m32abs(REG_EDX, HI(&mips3.count_zero_time));					/* sbb	edx,[mips3.count_zero_time+4]*/
			_shrd_r32_r32_imm(REG_EAX, REG_EDX, 1);									/* shrd	eax,edx,1*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case COP0_Cause:
			_sub_r32_imm(REG_EBP, 25);												/* sub  ebp,24*/
			_jcc_short_link(COND_NS, &link1);										/* jns	notneg*/
			_xor_r32_r32(REG_EBP, REG_EBP);											/* xor	ebp,ebp*/
			_resolve_link(&link1);													/* notneg:*/
			_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][reg]);							/* mov	eax,[mips3.cpr[0][reg]]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		default:
			_mov_r32_m32abs(REG_EAX, &mips3.cpr[0][reg]);							/* mov	eax,[mips3.cpr[0][reg]]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
	}
	return RECOMPILE_UNIMPLEMENTED;
}


/*------------------------------------------------------------------
	recompile_cop0
------------------------------------------------------------------*/

static UINT32 recompile_cop0(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata checklink;
	UINT32 result;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP0)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_KSU_MASK);					/* test	[mips3.cpr[0][COP0_Status]],SR_KSU_MASK*/
		_jcc_short_link(COND_Z, &checklink);										/* jz	okay*/
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP0);						/* test	[mips3.cpr[0][COP0_Status]],SR_COP0*/
		_jcc(COND_Z, mips3.generate_cop_exception);									/* jz	generate_cop_exception*/
		_resolve_link(&checklink);													/* okay:*/
	}
	
	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			result = RECOMPILE_SUCCESSFUL_CP(1,4);
			if (RTREG != 0)
			{
				result = recompile_get_cop0_reg(drc, RDREG);									/* read cop0 reg*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			return result;
			
		case 0x01:	/* DMFCz */
			result = RECOMPILE_SUCCESSFUL_CP(1,4);
			if (RTREG != 0)
			{
				result = recompile_get_cop0_reg(drc, RDREG);									/* read cop0 reg*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			return result;

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.ccr[0][RDREG]);						/* mov	eax,[mips3.ccr[0][rdreg]]*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[rtreg],edx:eax*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* MTCz */
			if (RTREG != 0)
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* mov	eax,[mips3.r[RTREG]]*/
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										/* xor	eax,eax*/
			result = recompile_set_cop0_reg(drc, RDREG);										/* write cop0 reg*/
			return result;

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* mov	eax,[mips3.r[RTREG]]*/
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										/* xor	eax,eax*/
			result = recompile_set_cop0_reg(drc, RDREG);										/* write cop0 reg*/
			return result;

		case 0x06:	/* CTCz */
			if (RTREG != 0)
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* mov	eax,[mips3.r[RTREG]]*/
			else
				_xor_r32_r32(REG_EAX, REG_EAX);										/* xor	eax,eax*/
			_mov_m32abs_r32(&mips3.ccr[0][RDREG], REG_EAX);							/* mov	[mips3.ccr[0][RDREG]],eax*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

/*
		case 0x08:	// BC
//			switch (RTREG)
//			{
//				case 0x00:	// BCzF     if (!mips3.cf[0][0]) ADDPC(SIMMVAL);				break;
//				case 0x01:	// BCzF     if (mips3.cf[0][0]) ADDPC(SIMMVAL);					break;
//				case 0x02:	// BCzFL    invalid_instruction(op);							break;
//				case 0x03:	// BCzTL    invalid_instruction(op);							break;
//				default:	invalid_instruction(op);										break;
//			}
//			break;
*/
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:	/* COP */
			switch (op & 0x01ffffff)
			{
				case 0x01:	/* TLBR */
					return RECOMPILE_SUCCESSFUL_CP(1,4);
					
				case 0x02:	/* TLBWI */
					drc_append_save_call_restore(drc, (void *)logtlbentry, 0);		/* call	logtlbentry*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x06:	/* TLBWR */
					drc_append_save_call_restore(drc, (void *)logtlbentry, 0);		/* call	logtlbentry*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x08:	/* TLBP */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x10:	/* RFE */
					_jmp(mips3.generate_invalidop_exception);						/* jmp	generate_invalidop_exception*/
					return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;

				case 0x18:	/* ERET */
					_mov_r32_m32abs(REG_EDI, &mips3.cpr[0][COP0_EPC]);				/* mov	edi,[mips3.cpr[0][COP0_EPC]]*/
					_and_m32abs_imm(&mips3.cpr[0][COP0_Status], ~SR_EXL);			/* and	[mips3.cpr[0][COP0_Status]],~SR_EXL*/
					return RECOMPILE_SUCCESSFUL_CP(1,0) | RECOMPILE_CHECK_INTERRUPTS | RECOMPILE_END_OF_STRING | RECOMPILE_ADD_DISPATCH;

				default:
					_jmp(mips3.generate_invalidop_exception);						/* jmp	generate_invalidop_exception*/
					return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
			}
			break;

/*		default:*/
/*			_jmp(mips3.generate_invalidop_exception);								*/ /* jmp	generate_invalidop_exception*/
/*			return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;*/
	}
	return RECOMPILE_UNIMPLEMENTED;
}



/*###################################################################################################
**	COP1 RECOMPILATION
**#################################################################################################*/

/*------------------------------------------------------------------
	recompile_cop1
------------------------------------------------------------------*/

static UINT32 recompile_cop1(struct drccore *drc, UINT32 pc, UINT32 op)
{
	struct linkdata link1;
	int cycles, i;

	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP1);						/* test	[mips3.cpr[0][COP0_Status]],SR_COP1*/
		_jcc(COND_Z, mips3.generate_cop_exception);									/* jz	generate_cop_exception*/
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][RDREG]);						/* mov	eax,[mips3.cpr[1][RDREG]]*/
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[mips3.r[RTREG]],edx:eax*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:	/* DMFCz */
			if (RTREG != 0)
				_mov_m64abs_m64abs(&mips3.r[RTREG], &mips3.cpr[1][RDREG]);
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x02:	/* CFCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.ccr[1][RDREG]);						/* mov	eax,[mips3.ccr[1][RDREG]]*/
				if (RDREG == 31)
				{
					_and_r32_imm(REG_EAX, ~0xfe800000);								/* and	eax,~0xfe800000*/
					_xor_r32_r32(REG_EBX, REG_EBX);									/* xor	ebx,ebx*/
					_cmp_m8abs_imm(&mips3.cf[1][0], 0);								/* cmp	[cf[0]],0*/
					_setcc_r8(COND_NZ, REG_BL);										/* setnz bl*/
					_shl_r32_imm(REG_EBX, 23);										/* shl	ebx,23*/
					_or_r32_r32(REG_EAX, REG_EBX);									/* or	eax,ebx*/
					if (mips3.is_mips4)
						for (i = 1; i <= 7; i++)
						{
							_cmp_m8abs_imm(&mips3.cf[1][i], 0);						/* cmp	[cf[i]],0*/
							_setcc_r8(COND_NZ, REG_BL);								/* setnz bl*/
							_shl_r32_imm(REG_EBX, 24+i);							/* shl	ebx,24+i*/
							_or_r32_r32(REG_EAX, REG_EBX);							/* or	eax,ebx*/
						}
				}
				_cdq();																/* cdq*/
				_mov_m64abs_r64(&mips3.r[RTREG], REG_EDX, REG_EAX);					/* mov	[mips3.r[RTREG]],edx:eax*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x04:	/* MTCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* mov	eax,[mips3.r[RTREG]]*/
				_mov_m32abs_r32(LO(&mips3.cpr[1][RDREG]), REG_EAX);					/* mov	[mips3.cpr[1][RDREG]],eax*/
			}
			else
				_mov_m32abs_imm(&mips3.cpr[1][RDREG], 0);							/* mov	[mips3.cpr[1][RDREG]],0*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x05:	/* DMTCz */
			if (RTREG != 0)
				_mov_m64abs_m64abs(&mips3.cpr[1][RDREG], &mips3.r[RTREG]);
			else
				_zero_m64abs(&mips3.cpr[1][RDREG]);									/* mov	[mips3.cpr[1][RDREG]],0*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x06:	/* CTCz */
			if (RTREG != 0)
			{
				_mov_r32_m32abs(REG_EAX, &mips3.r[RTREG]);							/* mov	eax,[mips3.r[RTREG]]*/
				_mov_m32abs_r32(&mips3.ccr[1][RDREG], REG_EAX);						/* mov	[mips3.ccr[1][RDREG]],eax*/
			}
			else
				_mov_m32abs_imm(&mips3.ccr[1][RDREG], 0);							/* mov	[mips3.ccr[1][RDREG]],0*/
			if (RDREG == 31)
			{
				_mov_r32_m32abs(REG_EAX, LO(&mips3.ccr[1][RDREG]));					/* mov	eax,[mips3.ccr[1][RDREG]]*/
				_test_r32_imm(REG_EAX, 1 << 23);									/* test	eax,1<<23*/
				_setcc_m8abs(COND_NZ, &mips3.cf[1][0]);								/* setnz [cf[0]]*/
				if (mips3.is_mips4)
					for (i = 1; i <= 7; i++)
					{
						_test_r32_imm(REG_EAX, 1 << (24+i));						/* test	eax,1<<(24+i)*/
						_setcc_m8abs(COND_NZ, &mips3.cf[1][i]);						/* setnz [cf[i]]*/
					}
				_and_r32_imm(REG_EAX, 3);											/* and	eax,3*/
				_test_r32_imm(REG_EAX, 1);											/* test eax,1*/
				_jcc_near_link(COND_Z, &link1);										/* jz	skip*/
				_xor_r32_imm(REG_EAX, 2);											/* xor  eax,2*/
				_resolve_link(&link1);												/* skip:*/
				drc_append_set_fp_rounding(drc, REG_EAX);							/* set_rounding(EAX)*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	/* cmp	[cf[x]],0*/
					_jcc_near_link(COND_NZ, &link1);								/* jnz	link1*/
					cycles = recompile_delay_slot(drc, pc + 4);						/* <next instruction>*/
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);/* <branch or dispatch>*/
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);
				
				case 0x01:	/* BCzT */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	/* cmp	[cf[x]],0*/
					_jcc_near_link(COND_Z, &link1);									/* jz	link1*/
					cycles = recompile_delay_slot(drc, pc + 4);						/* <next instruction>*/
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);/* <branch or dispatch>*/
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:	/* BCzFL */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	/* cmp	[cf[x]],0*/
					_jcc_near_link(COND_NZ, &link1);								/* jnz	link1*/
					cycles = recompile_delay_slot(drc, pc + 4);						/* <next instruction>*/
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);/* <branch or dispatch>*/
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,8);

				case 0x03:	/* BCzTL */
					_cmp_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 18) & 7) : 0], 0);	/* cmp	[cf[x]],0*/
					_jcc_near_link(COND_Z, &link1);									/* jz	link1*/
					cycles = recompile_delay_slot(drc, pc + 4);						/* <next instruction>*/
					append_branch_or_dispatch(drc, pc + 4 + (SIMMVAL << 2), 1+cycles);/* <branch or dispatch>*/
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,8);
			}
			break;

		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_addss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* addss xmm0,[ftreg]*/
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movss [fdreg],xmm0*/
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_faddp();													/* faddp*/
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					else				/* ADD.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_addsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* addsd xmm0,[ftreg]*/
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movsd [fdreg],xmm0*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_faddp();													/* faddp*/
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_subss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* subss xmm0,[ftreg]*/
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movss [fdreg],xmm0*/
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fsubp();													/* fsubp*/
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					else				/* SUB.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_subsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* subsd xmm0,[ftreg]*/
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movsd [fdreg],xmm0*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fsubp();													/* fsubp*/
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* mulss xmm0,[ftreg]*/
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movss [fdreg],xmm0*/
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fmulp();													/* fmulp*/
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					else				/* MUL.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* mulsd xmm0,[ftreg]*/
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movsd [fdreg],xmm0*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fmulp();													/* fmulp*/
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S */
					{
						if (USE_SSE)
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_divss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* divss xmm0,[ftreg]*/
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movss [fdreg],xmm0*/
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fdivp();													/* fdivp*/
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					else				/* DIV.D */
					{
						if (USE_SSE)
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_divsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);			/* divsd xmm0,[ftreg]*/
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movsd [fdreg],xmm0*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fdivp();													/* fdivp*/
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S */
					{
						if (USE_SSE)
						{
							_sqrtss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);		/* sqrtss xmm0,[fsreg]*/
							_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movss [fdreg],xmm0*/
						}
						else
						{
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fsqrt();													/* fsqrt*/
							_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					else				/* SQRT.D */
					{
						if (USE_SSE)
						{
							_sqrtsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);		/* sqrtsd xmm0,[fsreg]*/
							_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);			/* movsd [fdreg],xmm0*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
							_fsqrt();													/* fsqrt*/
							_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
						}
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);								/* fld	[fsreg]*/
						_fabs();														/* fabs*/
						_fstp_m32abs(&mips3.cpr[1][FDREG]);								/* fstp	[fdreg]*/
					}
					else				/* ABS.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);								/* fld	[fsreg]*/
						_fabs();														/* fabs*/
						_fstp_m64abs(&mips3.cpr[1][FDREG]);								/* fstp	[fdreg]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);					/* mov	eax,[fsreg]*/
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);					/* mov	[fdreg],eax*/
					}
					else				/* MOV.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);								/* fld	[fsreg]*/
						_fchs();														/* fchs*/
						_fstp_m32abs(&mips3.cpr[1][FDREG]);								/* fstp	[fdreg]*/
					}
					else				/* NEG.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);								/* fld	[fsreg]*/
						_fchs();														/* fchs*/
						_fstp_m64abs(&mips3.cpr[1][FDREG]);								/* fstp	[fdreg]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x08:
					drc_append_set_temp_fp_rounding(drc, FPRND_NEAR);
					if (IS_SINGLE(op))	/* ROUND.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* ROUND.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x09:
					drc_append_set_temp_fp_rounding(drc, FPRND_CHOP);
					if (IS_SINGLE(op))	/* TRUNC.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* TRUNC.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0a:
					drc_append_set_temp_fp_rounding(drc, FPRND_UP);
					if (IS_SINGLE(op))	/* CEIL.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* CEIL.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0b:
					drc_append_set_temp_fp_rounding(drc, FPRND_DOWN);
					if (IS_SINGLE(op))	/* FLOOR.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* FLOOR.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0c:
					drc_append_set_temp_fp_rounding(drc, FPRND_NEAR);
					if (IS_SINGLE(op))	/* ROUND.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* ROUND.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0d:
					drc_append_set_temp_fp_rounding(drc, FPRND_CHOP);
					if (IS_SINGLE(op))	/* TRUNC.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* TRUNC.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0e:
					drc_append_set_temp_fp_rounding(drc, FPRND_UP);
					if (IS_SINGLE(op))	/* CEIL.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* CEIL.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x0f:
					drc_append_set_temp_fp_rounding(drc, FPRND_DOWN);
					if (IS_SINGLE(op))	/* FLOOR.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* FLOOR.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					drc_append_restore_fp_rounding(drc);
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x11:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			/* jmp	generate_invalidop_exception*/
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_cmp_m8abs_imm(&mips3.cf[1][(op >> 18) & 7], 0);				/* cmp	[cf[x]],0*/
					_jcc_short_link(((op >> 16) & 1) ? COND_Z : COND_NZ, &link1);	/* jz/nz skip*/
					if (IS_SINGLE(op))	/* MOVT/F.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);				/* mov	eax,[fsreg]*/
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);				/* mov	[fdreg],eax*/
					}
					else				/* MOVT/F.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x12:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			/* jmp	generate_invalidop_exception*/
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					/* mov	eax,[rtreg].lo*/
					_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));					/* or	eax,[rtreg].hi*/
					_jcc_short_link(COND_NZ, &link1);								/* jnz	skip*/
					if (IS_SINGLE(op))	/* MOVZ.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);				/* mov	eax,[fsreg]*/
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);				/* mov	[fdreg],eax*/
					}
					else				/* MOVZ.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x13:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			/* jmp	generate_invalidop_exception*/
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_mov_r32_m32abs(REG_EAX, LO(&mips3.r[RTREG]));					/* mov	eax,[rtreg].lo*/
					_or_r32_m32abs(REG_EAX, HI(&mips3.r[RTREG]));					/* or	eax,[rtreg].hi*/
					_jcc_short_link(COND_Z, &link1);								/* jz	skip*/
					if (IS_SINGLE(op))	/* MOVN.S */
					{
						_mov_r32_m32abs(REG_EAX, &mips3.cpr[1][FSREG]);				/* mov	eax,[fsreg]*/
						_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);				/* mov	[fdreg],eax*/
					}
					else				/* MOVN.D */
						_mov_m64abs_m64abs(&mips3.cpr[1][FDREG], &mips3.cpr[1][FSREG]);
					_resolve_link(&link1);											/* skip:*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x15:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			/* jmp	generate_invalidop_exception*/
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_fld1();														/* fld1*/
					if (IS_SINGLE(op))	/* RECIP.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						_fdivp();													/* fdivp*/
						_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
					}
					else				/* RECIP.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						_fdivp();													/* fdivp*/
						_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x16:	/* R5000 */
					if (!mips3.is_mips4)
					{
						_jmp((void *)mips3.generate_invalidop_exception);			/* jmp	generate_invalidop_exception*/
						return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
					}
					_fld1();														/* fld1*/
					if (IS_SINGLE(op))	/* RSQRT.S */
					{
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						_fsqrt();													/* fsqrt*/
						_fdivp();													/* fdivp*/
						_fstp_m32abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
					}
					else				/* RSQRT.D */
					{
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						_fsqrt();													/* fsqrt*/
						_fdivp();													/* fdivp*/
						_fstp_m64abs(&mips3.cpr[1][FDREG]);							/* fstp	[fdreg]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W */
							_fild_m32abs(&mips3.cpr[1][FSREG]);						/* fild	[fsreg]*/
						else				/* CVT.S.L */
							_fild_m64abs(&mips3.cpr[1][FSREG]);						/* fild	[fsreg]*/
					}
					else					/* CVT.S.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fstp_m32abs(&mips3.cpr[1][FDREG]);								/* fstp	[fdreg]*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W */
							_fild_m32abs(&mips3.cpr[1][FSREG]);						/* fild	[fsreg]*/
						else				/* CVT.D.L */
							_fild_m64abs(&mips3.cpr[1][FSREG]);						/* fild	[fsreg]*/
					}
					else					/* CVT.D.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fstp_m64abs(&mips3.cpr[1][FDREG]);								/* fstp	[fdreg]*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* CVT.W.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m32abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S */
						_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					else				/* CVT.L.D */
						_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
					_fistp_m64abs(&mips3.cpr[1][FDREG]);							/* fistp [fdreg]*/
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x30:
				case 0x38:
					_mov_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0], 0);	/* C.F.S/D */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x31:
				case 0x39:
					_mov_m8abs_imm(&mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0], 0);	/* C.UN.S/D */
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x32:
				case 0x3a:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.EQ.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_comiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* comiss xmm0,[ftreg]*/
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_comisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* comisd xmm0,[ftreg]*/
						}
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* sete [cf[x]]*/
					}
					else
					{
						if (IS_SINGLE(op))	/* C.EQ.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						_fcompp();														/* fcompp*/
						_fnstsw_ax();													/* fnstsw ax*/
						_sahf();														/* sahf*/
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* sete [cf[x]]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x33:
				case 0x3b:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.UEQ.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_ucomiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* ucomiss xmm0,[ftreg]*/
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_ucomisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* ucomisd xmm0,[ftreg]*/
						}
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* sete [cf[x]]*/
					}
					else
					{
						if (IS_SINGLE(op))	/* C.UEQ.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						_fucompp();														/* fucompp*/
						_fnstsw_ax();													/* fnstsw ax*/
						_sahf();														/* sahf*/
						_setcc_m8abs(COND_E, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* sete [cf[x]]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x34:
				case 0x3c:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.OLT.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_comiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* comiss xmm0,[ftreg]*/
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_comisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* comisd xmm0,[ftreg]*/
						}
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setb [cf[x]]*/
					}
					else
					{
						if (IS_SINGLE(op))	/* C.OLT.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						_fcompp();														/* fcompp*/
						_fnstsw_ax();													/* fnstsw ax*/
						_sahf();														/* sahf*/
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setb [cf[x]]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x35:
				case 0x3d:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.ULT.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_ucomiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* ucomiss xmm0,[ftreg]*/
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_ucomisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* ucomisd xmm0,[ftreg]*/
						}
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setb [cf[x]]*/
					}
					else
					{
						if (IS_SINGLE(op))	/* C.ULT.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						_fucompp();														/* fucompp*/
						_fnstsw_ax();													/* fnstsw ax*/
						_sahf();														/* sahf*/
						_setcc_m8abs(COND_B, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setb [cf[x]]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x36:
				case 0x3e:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.OLE.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_comiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* comiss xmm0,[ftreg]*/
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_comisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* comisd xmm0,[ftreg]*/
						}
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setle [cf[x]]*/
					}
					else
					{
						if (IS_SINGLE(op))	/* C.OLE.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						_fcompp();														/* fcompp*/
						_fnstsw_ax();													/* fnstsw ax*/
						_sahf();														/* sahf*/
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setbe [cf[x]]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);

				case 0x37:
				case 0x3f:
					if (USE_SSE)
					{
						if (IS_SINGLE(op))	/* C.ULE.S */
						{
							_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movss xmm0,[fsreg]*/
							_ucomiss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* ucomiss xmm0,[ftreg]*/
						}
						else
						{
							_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);			/* movsd xmm0,[fsreg]*/
							_ucomisd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);		/* ucomisd xmm0,[ftreg]*/
						}
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setl [cf[x]]*/
					}
					else
					{
						if (IS_SINGLE(op))	/* C.ULE.S */
						{
							_fld_m32abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m32abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						else
						{
							_fld_m64abs(&mips3.cpr[1][FTREG]);							/* fld	[ftreg]*/
							_fld_m64abs(&mips3.cpr[1][FSREG]);							/* fld	[fsreg]*/
						}
						_fucompp();														/* fucompp*/
						_fnstsw_ax();													/* fnstsw ax*/
						_sahf();														/* sahf*/
						_setcc_m8abs(COND_BE, &mips3.cf[1][mips3.is_mips4 ? ((op >> 8) & 7) : 0]); /* setbe [cf[x]]*/
					}
					return RECOMPILE_SUCCESSFUL_CP(1,4);
			}
			break;
	}
	_jmp((void *)mips3.generate_invalidop_exception);								/* jmp	generate_invalidop_exception*/
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}



/*###################################################################################################
**	COP1X RECOMPILATION
**#################################################################################################*/

/*------------------------------------------------------------------
	recompile_cop1x
------------------------------------------------------------------*/

static UINT32 recompile_cop1x(struct drccore *drc, UINT32 pc, UINT32 op)
{
	if (mips3.drcoptions & MIPS3DRC_STRICT_COP1)
	{
		_test_m32abs_imm(&mips3.cpr[0][COP0_Status], SR_COP1);						/* test	[mips3.cpr[0][COP0_Status]],SR_COP1*/
		_jcc(COND_Z, mips3.generate_cop_exception);									/* jz	generate_cop_exception*/
	}

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								/* add	eax,[rtreg]*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_mov_m32abs_r32(&mips3.cpr[1][FDREG], REG_EAX);							/* mov	[fdreg],eax*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x01:		/* LDXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								/* add	eax,[rtreg]*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_mov_m32abs_r32(mips3.bigendian ? HI(&mips3.cpr[1][FDREG]) : LO(&mips3.cpr[1][FDREG]), REG_EAX);/* mov	[fdreg].hi/lo,eax*/
			_pop_r32(REG_EAX);														/* pop	eax*/
			_add_r32_imm(REG_EAX, 4);												/* add	eax,4*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call(mips3.memory.readlong);											/* call	readlong*/
			_mov_m32abs_r32(mips3.bigendian ? LO(&mips3.cpr[1][FDREG]) : HI(&mips3.cpr[1][FDREG]), REG_EAX);/* mov	[fdreg].lo/hi,eax*/
			_add_r32_imm(REG_ESP, 4);												/* add	esp,4*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case 0x08:		/* SWXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_m32abs(&mips3.cpr[1][FSREG]);										/* push	[fsreg]*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								/* add	eax,[rtreg]*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call(mips3.memory.writelong);											/* call	writelong*/
			_add_r32_imm(REG_ESP, 8);												/* add	esp,8*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case 0x09:		/* SDXC1 */
			_mov_m32abs_r32(&mips3_icount, REG_EBP);								/* mov	[mips3_icount],ebp*/
			_push_m32abs(mips3.bigendian ? HI(&mips3.cpr[1][FSREG]) : LO(&mips3.cpr[1][FSREG]));/* push	[fsreg].hi/lo*/
			_mov_r32_m32abs(REG_EAX, &mips3.r[RSREG]);								/* mov	eax,[rsreg]*/
			_add_r32_m32abs(REG_EAX, &mips3.r[RTREG]);								/* add	eax,[rtreg]*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call(mips3.memory.writelong);											/* call	writelong*/
			_pop_r32(REG_EAX);														/* pop	eax*/
			_add_r32_imm(REG_EAX, 4);												/* add	eax,4*/
			_push_m32abs(mips3.bigendian ? LO(&mips3.cpr[1][FSREG]) : HI(&mips3.cpr[1][FSREG]));/* push	[fsreg].lo/hi*/
			_push_r32(REG_EAX);														/* push	eax*/
			_call(mips3.memory.writelong);											/* call	writelong*/
			_add_r32_imm(REG_ESP, 12);												/* add	esp,12*/
			_mov_r32_m32abs(REG_EBP, &mips3_icount);								/* mov	ebp,[mips3_icount]*/
			return RECOMPILE_SUCCESSFUL_CP(1,4);
		
		case 0x0f:		/* PREFX */
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x20:		/* MADD.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movss xmm0,[fsreg]*/
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulss xmm0,[ftreg]*/
				_addss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FRREG]);						/* addss xmm0,[frreg]*/
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						/* movss [fdreg],xmm0*/
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m32abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m32abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_faddp();																/* faddp*/
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x21:		/* MADD.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movsd xmm0,[fsreg]*/
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulsd xmm0,[ftreg]*/
				_addsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FRREG]);						/* addsd xmm0,[frreg]*/
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						/* movsd [fdreg],xmm0*/
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m64abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m64abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_faddp();																/* faddp*/
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x28:		/* MSUB.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movss xmm0,[fsreg]*/
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulss xmm0,[ftreg]*/
				_subss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FRREG]);						/* subss xmm0,[frreg]*/
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						/* movss [fdreg],xmm0*/
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m32abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m32abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_fsubrp();																/* fsubrp*/
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x29:		/* MSUB.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movsd xmm0,[fsreg]*/
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulsd xmm0,[ftreg]*/
				_subsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FRREG]);						/* subsd xmm0,[frreg]*/
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM0);						/* movsd [fdreg],xmm0*/
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m64abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m64abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_fsubrp();																/* fsubrp*/
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x30:		/* NMADD.S */
			if (USE_SSE)
			{
				_pxor_r128_r128(REG_XMM1, REG_XMM1);									/* pxor	xmm1,xmm1*/
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movss xmm0,[fsreg]*/
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulss xmm0,[ftreg]*/
				_addss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FRREG]);						/* addss xmm0,[frreg]*/
				_subss_r128_r128(REG_XMM1, REG_XMM0);									/* subss xmm1,xmm0*/
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						/* movss [fdreg],xmm1*/
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m32abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m32abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_faddp();																/* faddp*/
				_fchs();																/* fchs*/
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x31:		/* NMADD.D */
			if (USE_SSE)
			{
				_pxor_r128_r128(REG_XMM1, REG_XMM1);									/* pxor	xmm1,xmm1*/
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movsd xmm0,[fsreg]*/
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulsd xmm0,[ftreg]*/
				_addsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FRREG]);						/* addsd xmm0,[frreg]*/
				_subss_r128_r128(REG_XMM1, REG_XMM0);									/* subss xmm1,xmm0*/
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						/* movsd [fdreg],xmm1*/
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m64abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m64abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_faddp();																/* faddp*/
				_fchs();																/* fchs*/
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);
			
		case 0x38:		/* NMSUB.S */
			if (USE_SSE)
			{
				_movss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movss xmm0,[fsreg]*/
				_mulss_r128_m32abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulss xmm0,[ftreg]*/
				_movss_r128_m32abs(REG_XMM1, &mips3.cpr[1][FRREG]);						/* movss xmm1,[frreg]*/
				_subss_r128_r128(REG_XMM1, REG_XMM0);									/* subss xmm1,xmm0*/
				_movss_m32abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						/* movss [fdreg],xmm1*/
			}
			else
			{
				_fld_m32abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m32abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m32abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_fsubp();																/* fsubp*/
				_fstp_m32abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x39:		/* NMSUB.D */
			if (USE_SSE)
			{
				_movsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FSREG]);						/* movsd xmm0,[fsreg]*/
				_mulsd_r128_m64abs(REG_XMM0, &mips3.cpr[1][FTREG]);						/* mulsd xmm0,[ftreg]*/
				_movsd_r128_m64abs(REG_XMM1, &mips3.cpr[1][FRREG]);						/* movsd xmm1,[frreg]*/
				_subss_r128_r128(REG_XMM1, REG_XMM0);									/* subss xmm1,xmm0*/
				_movsd_m64abs_r128(&mips3.cpr[1][FDREG], REG_XMM1);						/* movsd [fdreg],xmm1*/
			}
			else
			{
				_fld_m64abs(&mips3.cpr[1][FRREG]);										/* fld	[frreg]*/
				_fld_m64abs(&mips3.cpr[1][FSREG]);										/* fld	[fsreg]*/
				_fld_m64abs(&mips3.cpr[1][FTREG]);										/* fld	[ftreg]*/
				_fmulp();																/* fmulp*/
				_fsubp();																/* fsubrp*/
				_fstp_m64abs(&mips3.cpr[1][FDREG]);										/* fstp	[fdreg]*/
			}
			return RECOMPILE_SUCCESSFUL_CP(1,4);

		case 0x24:		/* MADD.W */
		case 0x25:		/* MADD.L */
		case 0x2c:		/* MSUB.W */
		case 0x2d:		/* MSUB.L */
		case 0x34:		/* NMADD.W */
		case 0x35:		/* NMADD.L */
		case 0x3c:		/* NMSUB.W */
		case 0x3d:		/* NMSUB.L */
		default:
			fprintf(stderr, "cop1x %X\n", op);
			break;
	}
	_jmp((void *)mips3.generate_invalidop_exception);								/* jmp	generate_invalidop_exception*/
	return RECOMPILE_SUCCESSFUL | RECOMPILE_END_OF_STRING;
}

/*###################################################################################################
**	REGISTER SNOOP
**#################################################################################################*/

unsigned mips3_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case MIPS3_PC:		return mips3.pc;
		case MIPS3_SR:		return mips3.cpr[0][COP0_Status];
		case MIPS3_EPC:		return mips3.cpr[0][COP0_EPC];
		case MIPS3_CAUSE:	return mips3.cpr[0][COP0_Cause];
		case MIPS3_COUNT:	return ((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2);
		case MIPS3_COMPARE:	return mips3.cpr[0][COP0_Compare];

		case MIPS3_R0:		return (UINT32)mips3.r[0];
		case MIPS3_R1:		return (UINT32)mips3.r[1];
		case MIPS3_R2:		return (UINT32)mips3.r[2];
		case MIPS3_R3:		return (UINT32)mips3.r[3];
		case MIPS3_R4:		return (UINT32)mips3.r[4];
		case MIPS3_R5:		return (UINT32)mips3.r[5];
		case MIPS3_R6:		return (UINT32)mips3.r[6];
		case MIPS3_R7:		return (UINT32)mips3.r[7];
		case MIPS3_R8:		return (UINT32)mips3.r[8];
		case MIPS3_R9:		return (UINT32)mips3.r[9];
		case MIPS3_R10:		return (UINT32)mips3.r[10];
		case MIPS3_R11:		return (UINT32)mips3.r[11];
		case MIPS3_R12:		return (UINT32)mips3.r[12];
		case MIPS3_R13:		return (UINT32)mips3.r[13];
		case MIPS3_R14:		return (UINT32)mips3.r[14];
		case MIPS3_R15:		return (UINT32)mips3.r[15];
		case MIPS3_R16:		return (UINT32)mips3.r[16];
		case MIPS3_R17:		return (UINT32)mips3.r[17];
		case MIPS3_R18:		return (UINT32)mips3.r[18];
		case MIPS3_R19:		return (UINT32)mips3.r[19];
		case MIPS3_R20:		return (UINT32)mips3.r[20];
		case MIPS3_R21:		return (UINT32)mips3.r[21];
		case MIPS3_R22:		return (UINT32)mips3.r[22];
		case MIPS3_R23:		return (UINT32)mips3.r[23];
		case MIPS3_R24:		return (UINT32)mips3.r[24];
		case MIPS3_R25:		return (UINT32)mips3.r[25];
		case MIPS3_R26:		return (UINT32)mips3.r[26];
		case MIPS3_R27:		return (UINT32)mips3.r[27];
		case MIPS3_R28:		return (UINT32)mips3.r[28];
		case MIPS3_R29:		return (UINT32)mips3.r[29];
		case MIPS3_R30:		return (UINT32)mips3.r[30];
/*		case REG_SP:*/
		case MIPS3_R31:		return (UINT32)mips3.r[31];
		case MIPS3_HI:		return (UINT32)mips3.hi;
		case MIPS3_LO:		return (UINT32)mips3.lo;

		case MIPS3_R0LO:	return (UINT32)mips3.r[0];
		case MIPS3_R1LO:	return (UINT32)mips3.r[1];
		case MIPS3_R2LO:	return (UINT32)mips3.r[2];
		case MIPS3_R3LO:	return (UINT32)mips3.r[3];
		case MIPS3_R4LO:	return (UINT32)mips3.r[4];
		case MIPS3_R5LO:	return (UINT32)mips3.r[5];
		case MIPS3_R6LO:	return (UINT32)mips3.r[6];
		case MIPS3_R7LO:	return (UINT32)mips3.r[7];
		case MIPS3_R8LO:	return (UINT32)mips3.r[8];
		case MIPS3_R9LO:	return (UINT32)mips3.r[9];
		case MIPS3_R10LO:	return (UINT32)mips3.r[10];
		case MIPS3_R11LO:	return (UINT32)mips3.r[11];
		case MIPS3_R12LO:	return (UINT32)mips3.r[12];
		case MIPS3_R13LO:	return (UINT32)mips3.r[13];
		case MIPS3_R14LO:	return (UINT32)mips3.r[14];
		case MIPS3_R15LO:	return (UINT32)mips3.r[15];
		case MIPS3_R16LO:	return (UINT32)mips3.r[16];
		case MIPS3_R17LO:	return (UINT32)mips3.r[17];
		case MIPS3_R18LO:	return (UINT32)mips3.r[18];
		case MIPS3_R19LO:	return (UINT32)mips3.r[19];
		case MIPS3_R20LO:	return (UINT32)mips3.r[20];
		case MIPS3_R21LO:	return (UINT32)mips3.r[21];
		case MIPS3_R22LO:	return (UINT32)mips3.r[22];
		case MIPS3_R23LO:	return (UINT32)mips3.r[23];
		case MIPS3_R24LO:	return (UINT32)mips3.r[24];
		case MIPS3_R25LO:	return (UINT32)mips3.r[25];
		case MIPS3_R26LO:	return (UINT32)mips3.r[26];
		case MIPS3_R27LO:	return (UINT32)mips3.r[27];
		case MIPS3_R28LO:	return (UINT32)mips3.r[28];
		case MIPS3_R29LO:	return (UINT32)mips3.r[29];
		case MIPS3_R30LO:	return (UINT32)mips3.r[30];
		case MIPS3_R31LO:	return (UINT32)mips3.r[31];
		case MIPS3_HILO:	return (UINT32)mips3.hi;
		case MIPS3_LOLO:	return (UINT32)mips3.lo;

		case MIPS3_R0HI:	return (UINT32)(mips3.r[0] >> 32);
		case MIPS3_R1HI:	return (UINT32)(mips3.r[1] >> 32);
		case MIPS3_R2HI:	return (UINT32)(mips3.r[2] >> 32);
		case MIPS3_R3HI:	return (UINT32)(mips3.r[3] >> 32);
		case MIPS3_R4HI:	return (UINT32)(mips3.r[4] >> 32);
		case MIPS3_R5HI:	return (UINT32)(mips3.r[5] >> 32);
		case MIPS3_R6HI:	return (UINT32)(mips3.r[6] >> 32);
		case MIPS3_R7HI:	return (UINT32)(mips3.r[7] >> 32);
		case MIPS3_R8HI:	return (UINT32)(mips3.r[8] >> 32);
		case MIPS3_R9HI:	return (UINT32)(mips3.r[9] >> 32);
		case MIPS3_R10HI:	return (UINT32)(mips3.r[10] >> 32);
		case MIPS3_R11HI:	return (UINT32)(mips3.r[11] >> 32);
		case MIPS3_R12HI:	return (UINT32)(mips3.r[12] >> 32);
		case MIPS3_R13HI:	return (UINT32)(mips3.r[13] >> 32);
		case MIPS3_R14HI:	return (UINT32)(mips3.r[14] >> 32);
		case MIPS3_R15HI:	return (UINT32)(mips3.r[15] >> 32);
		case MIPS3_R16HI:	return (UINT32)(mips3.r[16] >> 32);
		case MIPS3_R17HI:	return (UINT32)(mips3.r[17] >> 32);
		case MIPS3_R18HI:	return (UINT32)(mips3.r[18] >> 32);
		case MIPS3_R19HI:	return (UINT32)(mips3.r[19] >> 32);
		case MIPS3_R20HI:	return (UINT32)(mips3.r[20] >> 32);
		case MIPS3_R21HI:	return (UINT32)(mips3.r[21] >> 32);
		case MIPS3_R22HI:	return (UINT32)(mips3.r[22] >> 32);
		case MIPS3_R23HI:	return (UINT32)(mips3.r[23] >> 32);
		case MIPS3_R24HI:	return (UINT32)(mips3.r[24] >> 32);
		case MIPS3_R25HI:	return (UINT32)(mips3.r[25] >> 32);
		case MIPS3_R26HI:	return (UINT32)(mips3.r[26] >> 32);
		case MIPS3_R27HI:	return (UINT32)(mips3.r[27] >> 32);
		case MIPS3_R28HI:	return (UINT32)(mips3.r[28] >> 32);
		case MIPS3_R29HI:	return (UINT32)(mips3.r[29] >> 32);
		case MIPS3_R30HI:	return (UINT32)(mips3.r[30] >> 32);
		case MIPS3_R31HI:	return (UINT32)(mips3.r[31] >> 32);
		case MIPS3_HIHI:	return (UINT32)(mips3.hi >> 32);
		case MIPS3_LOHI:	return (UINT32)(mips3.lo >> 32);

		case REG_PREVIOUSPC: return mips3.pc;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
/*				unsigned offset = REG_SP_CONTENTS - regnum;*/
/*				if (offset < PC_STACK_DEPTH)*/
/*					return mips3.pc_stack[offset];*/
			}
	}
	return 0;
}



/*###################################################################################################
**	REGISTER MODIFY
**#################################################################################################*/

void mips3_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:
		case MIPS3_PC:		mips3.pc = val;	break;
		case MIPS3_SR:		mips3.cpr[0][COP0_Status] = val; break;
		case MIPS3_EPC:		mips3.cpr[0][COP0_EPC] = val; break;
		case MIPS3_CAUSE:	mips3.cpr[0][COP0_Cause] = val; break;
		case MIPS3_COUNT:	mips3.cpr[0][COP0_Count] = val; break;
		case MIPS3_COMPARE:	mips3.cpr[0][COP0_Compare] = val; break;

		case MIPS3_R0:		mips3.r[0] = (INT32)val;	break;
		case MIPS3_R1:		mips3.r[1] = (INT32)val;	break;
		case MIPS3_R2:		mips3.r[2] = (INT32)val;	break;
		case MIPS3_R3:		mips3.r[3] = (INT32)val;	break;
		case MIPS3_R4:		mips3.r[4] = (INT32)val;	break;
		case MIPS3_R5:		mips3.r[5] = (INT32)val;	break;
		case MIPS3_R6:		mips3.r[6] = (INT32)val;	break;
		case MIPS3_R7:		mips3.r[7] = (INT32)val;	break;
		case MIPS3_R8:		mips3.r[8] = (INT32)val;	break;
		case MIPS3_R9:		mips3.r[9] = (INT32)val;	break;
		case MIPS3_R10:		mips3.r[10] = (INT32)val;	break;
		case MIPS3_R11:		mips3.r[11] = (INT32)val;	break;
		case MIPS3_R12:		mips3.r[12] = (INT32)val;	break;
		case MIPS3_R13:		mips3.r[13] = (INT32)val;	break;
		case MIPS3_R14:		mips3.r[14] = (INT32)val;	break;
		case MIPS3_R15:		mips3.r[15] = (INT32)val;	break;
		case MIPS3_R16:		mips3.r[16] = (INT32)val;	break;
		case MIPS3_R17:		mips3.r[17] = (INT32)val;	break;
		case MIPS3_R18:		mips3.r[18] = (INT32)val;	break;
		case MIPS3_R19:		mips3.r[19] = (INT32)val;	break;
		case MIPS3_R20:		mips3.r[20] = (INT32)val;	break;
		case MIPS3_R21:		mips3.r[21] = (INT32)val;	break;
		case MIPS3_R22:		mips3.r[22] = (INT32)val;	break;
		case MIPS3_R23:		mips3.r[23] = (INT32)val;	break;
		case MIPS3_R24:		mips3.r[24] = (INT32)val;	break;
		case MIPS3_R25:		mips3.r[25] = (INT32)val;	break;
		case MIPS3_R26:		mips3.r[26] = (INT32)val;	break;
		case MIPS3_R27:		mips3.r[27] = (INT32)val;	break;
		case MIPS3_R28:		mips3.r[28] = (INT32)val;	break;
		case MIPS3_R29:		mips3.r[29] = (INT32)val;	break;
		case MIPS3_R30:		mips3.r[30] = (INT32)val;	break;
/*		case REG_SP:*/
		case MIPS3_R31:		mips3.r[31] = (INT32)val;	break;
		case MIPS3_HI:		mips3.hi = (INT32)val;		break;
		case MIPS3_LO:		mips3.lo = (INT32)val;		break;

		case MIPS3_R0LO:	mips3.r[0] = (mips3.r[0] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R1LO:	mips3.r[1] = (mips3.r[1] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R2LO:	mips3.r[2] = (mips3.r[2] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R3LO:	mips3.r[3] = (mips3.r[3] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R4LO:	mips3.r[4] = (mips3.r[4] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R5LO:	mips3.r[5] = (mips3.r[5] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R6LO:	mips3.r[6] = (mips3.r[6] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R7LO:	mips3.r[7] = (mips3.r[7] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R8LO:	mips3.r[8] = (mips3.r[8] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R9LO:	mips3.r[9] = (mips3.r[9] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R10LO:	mips3.r[10] = (mips3.r[10] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R11LO:	mips3.r[11] = (mips3.r[11] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R12LO:	mips3.r[12] = (mips3.r[12] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R13LO:	mips3.r[13] = (mips3.r[13] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R14LO:	mips3.r[14] = (mips3.r[14] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R15LO:	mips3.r[15] = (mips3.r[15] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R16LO:	mips3.r[16] = (mips3.r[16] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R17LO:	mips3.r[17] = (mips3.r[17] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R18LO:	mips3.r[18] = (mips3.r[18] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R19LO:	mips3.r[19] = (mips3.r[19] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R20LO:	mips3.r[20] = (mips3.r[20] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R21LO:	mips3.r[21] = (mips3.r[21] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R22LO:	mips3.r[22] = (mips3.r[22] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R23LO:	mips3.r[23] = (mips3.r[23] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R24LO:	mips3.r[24] = (mips3.r[24] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R25LO:	mips3.r[25] = (mips3.r[25] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R26LO:	mips3.r[26] = (mips3.r[26] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R27LO:	mips3.r[27] = (mips3.r[27] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R28LO:	mips3.r[28] = (mips3.r[28] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R29LO:	mips3.r[29] = (mips3.r[29] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R30LO:	mips3.r[30] = (mips3.r[30] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_R31LO:	mips3.r[31] = (mips3.r[31] & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_HILO:	mips3.hi = (mips3.hi & ~((UINT64)0xffffffff)) | val;	break;
		case MIPS3_LOLO:	mips3.lo = (mips3.lo & ~((UINT64)0xffffffff)) | val;	break;

		case MIPS3_R0HI:	mips3.r[0] = (mips3.r[0] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R1HI:	mips3.r[1] = (mips3.r[1] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R2HI:	mips3.r[2] = (mips3.r[2] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R3HI:	mips3.r[3] = (mips3.r[3] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R4HI:	mips3.r[4] = (mips3.r[4] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R5HI:	mips3.r[5] = (mips3.r[5] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R6HI:	mips3.r[6] = (mips3.r[6] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R7HI:	mips3.r[7] = (mips3.r[7] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R8HI:	mips3.r[8] = (mips3.r[8] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R9HI:	mips3.r[9] = (mips3.r[9] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R10HI:	mips3.r[10] = (mips3.r[10] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R11HI:	mips3.r[11] = (mips3.r[11] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R12HI:	mips3.r[12] = (mips3.r[12] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R13HI:	mips3.r[13] = (mips3.r[13] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R14HI:	mips3.r[14] = (mips3.r[14] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R15HI:	mips3.r[15] = (mips3.r[15] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R16HI:	mips3.r[16] = (mips3.r[16] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R17HI:	mips3.r[17] = (mips3.r[17] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R18HI:	mips3.r[18] = (mips3.r[18] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R19HI:	mips3.r[19] = (mips3.r[19] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R20HI:	mips3.r[20] = (mips3.r[20] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R21HI:	mips3.r[21] = (mips3.r[21] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R22HI:	mips3.r[22] = (mips3.r[22] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R23HI:	mips3.r[23] = (mips3.r[23] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R24HI:	mips3.r[24] = (mips3.r[24] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R25HI:	mips3.r[25] = (mips3.r[25] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R26HI:	mips3.r[26] = (mips3.r[26] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R27HI:	mips3.r[27] = (mips3.r[27] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R28HI:	mips3.r[28] = (mips3.r[28] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R29HI:	mips3.r[29] = (mips3.r[29] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R30HI:	mips3.r[30] = (mips3.r[30] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_R31HI:	mips3.r[31] = (mips3.r[31] & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_HIHI:	mips3.hi = (mips3.hi & 0xffffffff) | ((UINT64)val << 32);	break;
		case MIPS3_LOHI:	mips3.lo = (mips3.lo & 0xffffffff) | ((UINT64)val << 32);	break;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
/*				unsigned offset = REG_SP_CONTENTS - regnum;*/
/*				if (offset < PC_STACK_DEPTH)*/
/*					mips3.pc_stack[offset] = val;*/
			}
    }
}



/*###################################################################################################
**	DEBUGGER DEFINITIONS
**#################################################################################################*/

static UINT8 mips3_reg_layout[] =
{
	MIPS3_PC,		MIPS3_SR,		-1,
	MIPS3_EPC,		MIPS3_CAUSE,	-1,
	MIPS3_COUNT,	MIPS3_COMPARE,	-1,
	MIPS3_HI,		MIPS3_LO,		-1,
	MIPS3_R0,	 	MIPS3_R16,		-1,
	MIPS3_R1, 		MIPS3_R17,		-1,
	MIPS3_R2, 		MIPS3_R18,		-1,
	MIPS3_R3, 		MIPS3_R19,		-1,
	MIPS3_R4, 		MIPS3_R20,		-1,
	MIPS3_R5, 		MIPS3_R21,		-1,
	MIPS3_R6, 		MIPS3_R22,		-1,
	MIPS3_R7, 		MIPS3_R23,		-1,
	MIPS3_R8,		MIPS3_R24,		-1,
	MIPS3_R9,		MIPS3_R25,		-1,
	MIPS3_R10,		MIPS3_R26,		-1,
	MIPS3_R11,		MIPS3_R27,		-1,
	MIPS3_R12,		MIPS3_R28,		-1,
	MIPS3_R13,		MIPS3_R29,		-1,
	MIPS3_R14,		MIPS3_R30,		-1,
	MIPS3_R15,		MIPS3_R31,		0
};

static UINT8 mips3_win_layout[] =
{
	 0, 0,45,20,	/* register window (top rows) */
	46, 0,33,14,	/* disassembler window (left colums) */
	 0,21,45, 1,	/* memory #1 window (right, upper middle) */
	46,15,33, 7,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};



/*###################################################################################################
**	DEBUGGER STRINGS
**#################################################################################################*/

const char *mips3_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	mips3_regs *r = context;

	which = (which + 1) % 16;
    buffer[which][0] = '\0';

	if (!context)
		r = &mips3;

    switch( regnum )
	{
		case CPU_INFO_REG+MIPS3_PC:  	sprintf(buffer[which], "PC: %08X", r->pc); break;
		case CPU_INFO_REG+MIPS3_SR:  	sprintf(buffer[which], "SR: %08X", (UINT32)r->cpr[0][COP0_Status]); break;
		case CPU_INFO_REG+MIPS3_EPC:  	sprintf(buffer[which], "EPC:%08X", (UINT32)r->cpr[0][COP0_EPC]); break;
		case CPU_INFO_REG+MIPS3_CAUSE: 	sprintf(buffer[which], "Cause:%08X", (UINT32)r->cpr[0][COP0_Cause]); break;
		case CPU_INFO_REG+MIPS3_COUNT: 	sprintf(buffer[which], "Count:%08X", (UINT32)((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2)); break;
		case CPU_INFO_REG+MIPS3_COMPARE:sprintf(buffer[which], "Compare:%08X", (UINT32)r->cpr[0][COP0_Compare]); break;

		case CPU_INFO_REG+MIPS3_R0:		sprintf(buffer[which], "R0: %08X%08X", (UINT32)(r->r[0] >> 32), (UINT32)r->r[0]); break;
		case CPU_INFO_REG+MIPS3_R1:		sprintf(buffer[which], "R1: %08X%08X", (UINT32)(r->r[1] >> 32), (UINT32)r->r[1]); break;
		case CPU_INFO_REG+MIPS3_R2:		sprintf(buffer[which], "R2: %08X%08X", (UINT32)(r->r[2] >> 32), (UINT32)r->r[2]); break;
		case CPU_INFO_REG+MIPS3_R3:		sprintf(buffer[which], "R3: %08X%08X", (UINT32)(r->r[3] >> 32), (UINT32)r->r[3]); break;
		case CPU_INFO_REG+MIPS3_R4:		sprintf(buffer[which], "R4: %08X%08X", (UINT32)(r->r[4] >> 32), (UINT32)r->r[4]); break;
		case CPU_INFO_REG+MIPS3_R5:		sprintf(buffer[which], "R5: %08X%08X", (UINT32)(r->r[5] >> 32), (UINT32)r->r[5]); break;
		case CPU_INFO_REG+MIPS3_R6:		sprintf(buffer[which], "R6: %08X%08X", (UINT32)(r->r[6] >> 32), (UINT32)r->r[6]); break;
		case CPU_INFO_REG+MIPS3_R7:		sprintf(buffer[which], "R7: %08X%08X", (UINT32)(r->r[7] >> 32), (UINT32)r->r[7]); break;
		case CPU_INFO_REG+MIPS3_R8:		sprintf(buffer[which], "R8: %08X%08X", (UINT32)(r->r[8] >> 32), (UINT32)r->r[8]); break;
		case CPU_INFO_REG+MIPS3_R9:		sprintf(buffer[which], "R9: %08X%08X", (UINT32)(r->r[9] >> 32), (UINT32)r->r[9]); break;
		case CPU_INFO_REG+MIPS3_R10:	sprintf(buffer[which], "R10:%08X%08X", (UINT32)(r->r[10] >> 32), (UINT32)r->r[10]); break;
		case CPU_INFO_REG+MIPS3_R11:	sprintf(buffer[which], "R11:%08X%08X", (UINT32)(r->r[11] >> 32), (UINT32)r->r[11]); break;
		case CPU_INFO_REG+MIPS3_R12:	sprintf(buffer[which], "R12:%08X%08X", (UINT32)(r->r[12] >> 32), (UINT32)r->r[12]); break;
		case CPU_INFO_REG+MIPS3_R13:	sprintf(buffer[which], "R13:%08X%08X", (UINT32)(r->r[13] >> 32), (UINT32)r->r[13]); break;
		case CPU_INFO_REG+MIPS3_R14:	sprintf(buffer[which], "R14:%08X%08X", (UINT32)(r->r[14] >> 32), (UINT32)r->r[14]); break;
		case CPU_INFO_REG+MIPS3_R15:	sprintf(buffer[which], "R15:%08X%08X", (UINT32)(r->r[15] >> 32), (UINT32)r->r[15]); break;
		case CPU_INFO_REG+MIPS3_R16:	sprintf(buffer[which], "R16:%08X%08X", (UINT32)(r->r[16] >> 32), (UINT32)r->r[16]); break;
		case CPU_INFO_REG+MIPS3_R17:	sprintf(buffer[which], "R17:%08X%08X", (UINT32)(r->r[17] >> 32), (UINT32)r->r[17]); break;
		case CPU_INFO_REG+MIPS3_R18:	sprintf(buffer[which], "R18:%08X%08X", (UINT32)(r->r[18] >> 32), (UINT32)r->r[18]); break;
		case CPU_INFO_REG+MIPS3_R19:	sprintf(buffer[which], "R19:%08X%08X", (UINT32)(r->r[19] >> 32), (UINT32)r->r[19]); break;
		case CPU_INFO_REG+MIPS3_R20:	sprintf(buffer[which], "R20:%08X%08X", (UINT32)(r->r[20] >> 32), (UINT32)r->r[20]); break;
		case CPU_INFO_REG+MIPS3_R21:	sprintf(buffer[which], "R21:%08X%08X", (UINT32)(r->r[21] >> 32), (UINT32)r->r[21]); break;
		case CPU_INFO_REG+MIPS3_R22:	sprintf(buffer[which], "R22:%08X%08X", (UINT32)(r->r[22] >> 32), (UINT32)r->r[22]); break;
		case CPU_INFO_REG+MIPS3_R23:	sprintf(buffer[which], "R23:%08X%08X", (UINT32)(r->r[23] >> 32), (UINT32)r->r[23]); break;
		case CPU_INFO_REG+MIPS3_R24:	sprintf(buffer[which], "R24:%08X%08X", (UINT32)(r->r[24] >> 32), (UINT32)r->r[24]); break;
		case CPU_INFO_REG+MIPS3_R25:	sprintf(buffer[which], "R25:%08X%08X", (UINT32)(r->r[25] >> 32), (UINT32)r->r[25]); break;
		case CPU_INFO_REG+MIPS3_R26:	sprintf(buffer[which], "R26:%08X%08X", (UINT32)(r->r[26] >> 32), (UINT32)r->r[26]); break;
		case CPU_INFO_REG+MIPS3_R27:	sprintf(buffer[which], "R27:%08X%08X", (UINT32)(r->r[27] >> 32), (UINT32)r->r[27]); break;
		case CPU_INFO_REG+MIPS3_R28:	sprintf(buffer[which], "R28:%08X%08X", (UINT32)(r->r[28] >> 32), (UINT32)r->r[28]); break;
		case CPU_INFO_REG+MIPS3_R29:	sprintf(buffer[which], "R29:%08X%08X", (UINT32)(r->r[29] >> 32), (UINT32)r->r[29]); break;
		case CPU_INFO_REG+MIPS3_R30:	sprintf(buffer[which], "R30:%08X%08X", (UINT32)(r->r[30] >> 32), (UINT32)r->r[30]); break;
		case CPU_INFO_REG+MIPS3_R31:	sprintf(buffer[which], "R31:%08X%08X", (UINT32)(r->r[31] >> 32), (UINT32)r->r[31]); break;
		case CPU_INFO_REG+MIPS3_HI:		sprintf(buffer[which], "HI: %08X%08X", (UINT32)(r->hi >> 32), (UINT32)r->hi); break;
		case CPU_INFO_REG+MIPS3_LO:		sprintf(buffer[which], "LO: %08X%08X", (UINT32)(r->lo >> 32), (UINT32)r->lo); break;

		case CPU_INFO_NAME: return "MIPS III";
		case CPU_INFO_FAMILY: return r->bigendian ? "MIPS III (big-endian)" : "MIPS III (little-endian)";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Aaron Giles 2000-2002";
		case CPU_INFO_REG_LAYOUT: return (const char *)mips3_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)mips3_win_layout;
		case CPU_INFO_REG+10000: return "         ";
    }
	return buffer[which];
}


const char *r4600_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	mips3_regs *r = context;

	which = (which + 1) % 16;
    buffer[which][0] = '\0';

	if (!context)
		r = &mips3;

    switch( regnum )
	{
		case CPU_INFO_NAME: return "R4600";
		case CPU_INFO_FAMILY: return r->bigendian ? "MIPS R4600 (big-endian)" : "MIPS R4600 (little-endian)";
		default: return mips3_info(context, regnum);
    }
	return buffer[which];
}


const char *r5000_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	mips3_regs *r = context;

	which = (which + 1) % 16;
    buffer[which][0] = '\0';

	if (!context)
		r = &mips3;

    switch( regnum )
	{
		case CPU_INFO_NAME: return "R5000";
		case CPU_INFO_FAMILY: return r->bigendian ? "MIPS R5000 (big-endian)" : "MIPS R5000 (little-endian)";
		default: return mips3_info(context, regnum);
    }
	return buffer[which];
}



/*###################################################################################################
**	DISASSEMBLY HOOK
**#################################################################################################*/

unsigned mips3_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	extern unsigned dasmmips3(char *, unsigned);
	unsigned result;
	if (mips3.bigendian)
		change_pc32bedw(pc);
	else
		change_pc32ledw(pc);
    result = dasmmips3(buffer, pc);
	if (mips3.bigendian)
		change_pc32bedw(mips3.pc);
	else
		change_pc32ledw(mips3.pc);
    return result;
#else
	sprintf(buffer, "$%04X", cpu_readop32(pc));
	return 4;
#endif
}
