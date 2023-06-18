/*###################################################################################################
**
**
**		drccore.c
**		x86 Dynamic recompiler support routines.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#include "driver.h"
#include "x86drc.h"

#define LOG_DISPATCHES		0




const UINT8 scale_lookup[] = { 0,0,1,0,2,0,0,0,3 };

static UINT16 fp_control[4] = { 0x23f, 0x63f, 0xa3f, 0xe3f };


static void append_entry_point(struct drccore *drc);
static void append_recompile(struct drccore *drc);
static void append_out_of_cycles(struct drccore *drc);

#if LOG_DISPATCHES
static void log_dispatch(struct drccore *drc);
#endif


/*###################################################################################################
**	EXTERNAL INTERFACES
**#################################################################################################*/

/*------------------------------------------------------------------
	drc_init
------------------------------------------------------------------*/

struct drccore *drc_init(UINT8 cpunum, struct drcconfig *config)
{
	int address_bits = config->address_bits;
	int effective_address_bits = address_bits - config->lsbs_to_ignore;
	struct drccore *drc;

	/* allocate memory */
	drc = malloc(sizeof(*drc));
	if (!drc)
		return NULL;
	memset(drc, 0, sizeof(*drc));

	/* copy in relevant data from the config */
	drc->pcptr        = config->pcptr;
	drc->icountptr    = config->icountptr;
	drc->esiptr       = config->esiptr;
	drc->cb_reset     = config->cb_reset;
	drc->cb_recompile = config->cb_recompile;
	drc->cb_entrygen  = config->cb_entrygen;
	drc->uses_fp      = config->uses_fp;
	drc->uses_sse     = config->uses_sse;
	drc->fpcw_curr    = fp_control[0];

	/* allocate cache */
	drc->cache_base = malloc(config->cache_size);
	if (!drc->cache_base)
		return NULL;
	drc->cache_end = drc->cache_base + config->cache_size;
	drc->cache_danger = drc->cache_end - 65536;

	/* compute shifts and masks */
	drc->l1bits = effective_address_bits/2;
	drc->l2bits = effective_address_bits - drc->l1bits;
	drc->l1shift = config->lsbs_to_ignore + drc->l2bits;
	drc->l2mask = ((1 << drc->l2bits) - 1) << config->lsbs_to_ignore;
	drc->l2scale = 4 >> config->lsbs_to_ignore;

	/* allocate lookup tables */
	drc->lookup_l1 = malloc(sizeof(*drc->lookup_l1) * (1 << drc->l1bits));
	drc->lookup_l2_recompile = malloc(sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
	if (!drc->lookup_l1 || !drc->lookup_l2_recompile)
		return NULL;
	memset(drc->lookup_l1, 0, sizeof(*drc->lookup_l1) * (1 << drc->l1bits));
	memset(drc->lookup_l2_recompile, 0, sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));

	/* allocate the sequence and tentative lists */
	drc->sequence_count_max = config->max_instructions;
	drc->sequence_list = malloc(drc->sequence_count_max * sizeof(*drc->sequence_list));
	drc->tentative_count_max = config->max_instructions;
	drc->tentative_list = malloc(drc->tentative_count_max * sizeof(*drc->tentative_list));
	if (!drc->sequence_list || !drc->tentative_list)
		return NULL;

	/* seed the cache */
	drc_cache_reset(drc);
	return drc;
}


/*------------------------------------------------------------------
	drc_cache_reset
------------------------------------------------------------------*/

void drc_cache_reset(struct drccore *drc)
{
	int i;

	/* reset the cache and add the basics */
	drc->cache_top = drc->cache_base;

	/* append the core entry points to the fresh cache */
	drc->entry_point = (void (*)(void))drc->cache_top;
	append_entry_point(drc);
	drc->out_of_cycles = drc->cache_top;
	append_out_of_cycles(drc);
	drc->recompile = drc->cache_top;
	append_recompile(drc);
	drc->dispatch = drc->cache_top;
	drc_append_dispatcher(drc);

	/* populate the recompile table */
	for (i = 0; i < (1 << drc->l2bits); i++)
		drc->lookup_l2_recompile[i] = drc->recompile;

	/* reset all the l1 tables */
	for (i = 0; i < (1 << drc->l1bits); i++)
	{
		/* point NULL entries to the generic recompile table */
		if (drc->lookup_l1[i] == NULL)
			drc->lookup_l1[i] = drc->lookup_l2_recompile;

		/* reset allocated tables to point all entries back to the recompiler */
		else if (drc->lookup_l1[i] != drc->lookup_l2_recompile)
			memcpy(drc->lookup_l1[i], drc->lookup_l2_recompile, sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
	}

	/* call back to the host */
	if (drc->cb_reset)
		(*drc->cb_reset)(drc);
}


/*------------------------------------------------------------------
	drc_execute
------------------------------------------------------------------*/

void drc_execute(struct drccore *drc)
{
	(*drc->entry_point)();
}


/*------------------------------------------------------------------
	drc_exit
------------------------------------------------------------------*/

void drc_exit(struct drccore *drc)
{
	int i;

	/* free the cache */
	if (drc->cache_base)
		free(drc->cache_base);

	/* free all the l2 tables allocated */
	for (i = 0; i < (1 << drc->l1bits); i++)
		if (drc->lookup_l1[i] != drc->lookup_l2_recompile)
			free(drc->lookup_l1[i]);

	/* free the l1 table */
	if (drc->lookup_l1)
		free(drc->lookup_l1);

	/* free the default l2 table */
	if (drc->lookup_l2_recompile)
		free(drc->lookup_l2_recompile);

	/* free the lists */
	if (drc->sequence_list)
		free(drc->sequence_list);
	if (drc->tentative_list)
		free(drc->tentative_list);

	/* and the drc itself */
	free(drc);
}


/*------------------------------------------------------------------
	drc_begin_sequence
------------------------------------------------------------------*/

void drc_begin_sequence(struct drccore *drc, UINT32 pc)
{
	UINT32 l1index = pc >> drc->l1shift;
	UINT32 l2index = ((pc & drc->l2mask) * drc->l2scale) / 4;

	/* reset the sequence and tentative counts */
	drc->sequence_count = 0;
	drc->tentative_count = 0;

	/* allocate memory if necessary */
	if (drc->lookup_l1[l1index] == drc->lookup_l2_recompile)
	{
		/* create a new copy of the recompile table */
		drc->lookup_l1[l1index] = malloc(sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
		if (!drc->lookup_l1[l1index])
			exit(1);
		memcpy(drc->lookup_l1[l1index], drc->lookup_l2_recompile, sizeof(*drc->lookup_l2_recompile) * (1 << drc->l2bits));
	}

	/* nuke any previous link to this instruction */
	if (drc->lookup_l1[l1index][l2index] != drc->recompile)
	{
		UINT8 *cache_save = drc->cache_top;
		drc->cache_top = drc->lookup_l1[l1index][l2index];
		_jmp(drc->dispatch);
		drc->cache_top = cache_save;
	}

	/* note the current location for this instruction */
	drc->lookup_l1[l1index][l2index] = drc->cache_top;
}


/*------------------------------------------------------------------
	drc_end_sequence
------------------------------------------------------------------*/

void drc_end_sequence(struct drccore *drc)
{
	int i, j;

	/* fix up any internal links */
	for (i = 0; i < drc->tentative_count; i++)
		for (j = 0; j < drc->sequence_count; j++)
			if (drc->tentative_list[i].pc == drc->sequence_list[j].pc)
			{
				UINT8 *cache_save = drc->cache_top;
				drc->cache_top = drc->tentative_list[i].target;
				_jmp(drc->sequence_list[j].target);
				drc->cache_top = cache_save;
				break;
			}
}


/*------------------------------------------------------------------
	drc_register_code_at_cache_top
------------------------------------------------------------------*/

void drc_register_code_at_cache_top(struct drccore *drc, UINT32 pc)
{
	struct pc_ptr_pair *pair = &drc->sequence_list[drc->sequence_count++];
	if (drc->sequence_count > drc->sequence_count_max)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "drc_register_code_at_cache_top: too many instructions!\n");
		exit(1);
	}
	pair->target = drc->cache_top;
	pair->pc = pc;
}


/*------------------------------------------------------------------
	drc_get_code_at_pc
------------------------------------------------------------------*/

void *drc_get_code_at_pc(struct drccore *drc, UINT32 pc)
{
	UINT32 l1index = pc >> drc->l1shift;
	UINT32 l2index = ((pc & drc->l2mask) * drc->l2scale) / 4;
	return (drc->lookup_l1[l1index][l2index] != drc->recompile) ? drc->lookup_l1[l1index][l2index] : NULL;
}


/*------------------------------------------------------------------
	drc_append_verify_code
------------------------------------------------------------------*/

void drc_append_verify_code(struct drccore *drc, void *code, UINT8 length)
{
	if (length >= 4)
	{
		_cmp_m32abs_imm(code, *(UINT32 *)code);						/* cmp	[pc],opcode*/
		_jcc(COND_NE, drc->recompile);								/* jne	recompile*/
	}
	else if (length >= 2)
	{
		_cmp_m16abs_imm(code, *(UINT16 *)code);						/* cmp	[pc],opcode*/
		_jcc(COND_NE, drc->recompile);								/* jne	recompile*/
	}
	else
	{
		_cmp_m8abs_imm(code, *(UINT8 *)code);						/* cmp	[pc],opcode*/
		_jcc(COND_NE, drc->recompile);								/* jne	recompile*/
	}
}


/*------------------------------------------------------------------
	drc_append_call_debugger
------------------------------------------------------------------*/

void drc_append_call_debugger(struct drccore *drc)
{
#ifdef MAME_DEBUG
	struct linkdata link;
	_cmp_m32abs_imm(&mame_debug, 0);								/* cmp	[mame_debug],0*/
	_jcc_short_link(COND_E, &link);									/* je	skip*/
	drc_append_save_call_restore(drc, (void *)MAME_Debug, 0);		/* save volatiles*/
	_resolve_link(&link);
#endif
}


/*------------------------------------------------------------------
	drc_append_save_volatiles
------------------------------------------------------------------*/

void drc_append_save_volatiles(struct drccore *drc)
{
	if (drc->icountptr)
		_mov_m32abs_r32(drc->icountptr, REG_EBP);
	if (drc->pcptr)
		_mov_m32abs_r32(drc->pcptr, REG_EDI);
	if (drc->esiptr)
		_mov_m32abs_r32(drc->esiptr, REG_ESI);
}


/*------------------------------------------------------------------
	drc_append_restore_volatiles
------------------------------------------------------------------*/

void drc_append_restore_volatiles(struct drccore *drc)
{
	if (drc->icountptr)
		_mov_r32_m32abs(REG_EBP, drc->icountptr);
	if (drc->pcptr)
		_mov_r32_m32abs(REG_EDI, drc->pcptr);
	if (drc->esiptr)
		_mov_r32_m32abs(REG_ESI, drc->esiptr);
}


/*------------------------------------------------------------------
	drc_append_save_call_restore
------------------------------------------------------------------*/

void drc_append_save_call_restore(struct drccore *drc, void *target, UINT32 stackadj)
{
	drc_append_save_volatiles(drc);									/* save volatiles*/
	_call(target);													/* call	target*/
	drc_append_restore_volatiles(drc);								/* restore volatiles*/
	if (stackadj)
		_add_r32_imm(REG_ESP, stackadj);							/* adjust stack*/
}


/*------------------------------------------------------------------
	drc_append_standard_epilogue
------------------------------------------------------------------*/

void drc_append_standard_epilogue(struct drccore *drc, INT32 cycles, INT32 pcdelta, int allow_exit)
{
	if (cycles != 0)
		_sub_r32_imm(REG_EBP, cycles);								/* sub	ebp,cycles*/
	if (pcdelta != 0)
		_lea_r32_m32bd(REG_EDI, REG_EDI, pcdelta);					/* lea	edi,[edi+pcdelta]*/
	if (allow_exit && cycles != 0)
		_jcc(COND_S, drc->out_of_cycles);							/* js	out_of_cycles*/
}


/*------------------------------------------------------------------
	drc_append_dispatcher
------------------------------------------------------------------*/

void drc_append_dispatcher(struct drccore *drc)
{
#if LOG_DISPATCHES
	_push_imm(drc);													/* push	drc*/
	drc_append_save_call_restore(drc, (void *)log_dispatch, 4);		/* call	log_dispatch*/
#endif
	_mov_r32_r32(REG_EAX, REG_EDI);									/* mov	eax,edi*/
	_shr_r32_imm(REG_EAX, drc->l1shift);							/* shr	eax,l1shift*/
	_mov_r32_r32(REG_EDX, REG_EDI);									/* mov	edx,edi*/
	_mov_r32_m32isd(REG_EAX, REG_EAX, 4, drc->lookup_l1);			/* mov	eax,[eax*4 + l1lookup]*/
	_and_r32_imm(REG_EDX, drc->l2mask);								/* and	edx,l2mask*/
	_jmp_m32bisd(REG_EAX, REG_EDX, drc->l2scale, 0);				/* jmp	[eax+edx*l2scale]*/
}


/*------------------------------------------------------------------
	drc_append_fixed_dispatcher
------------------------------------------------------------------*/

void drc_append_fixed_dispatcher(struct drccore *drc, UINT32 newpc)
{
	void **base = drc->lookup_l1[newpc >> drc->l1shift];
	if (base == drc->lookup_l2_recompile)
	{
		_mov_r32_m32abs(REG_EAX, &drc->lookup_l1[newpc >> drc->l1shift]);/* mov	eax,[(newpc >> l1shift)*4 + l1lookup]*/
		_jmp_m32bd(REG_EAX, (newpc & drc->l2mask) * drc->l2scale);		/* jmp	[eax+(newpc & l2mask)*l2scale]*/
	}
	else
		_jmp_m32abs((UINT8 *)base + (newpc & drc->l2mask) * drc->l2scale);	/* jmp	[eax+(newpc & l2mask)*l2scale]*/
}


/*------------------------------------------------------------------
	drc_append_tentative_fixed_dispatcher
------------------------------------------------------------------*/

void drc_append_tentative_fixed_dispatcher(struct drccore *drc, UINT32 newpc)
{
	struct pc_ptr_pair *pair = &drc->tentative_list[drc->tentative_count++];
	if (drc->tentative_count > drc->tentative_count_max)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "drc_append_tentative_fixed_dispatcher: too many tentative branches!\n");
		exit(1);
	}
	pair->target = drc->cache_top;
	pair->pc = newpc;
	drc_append_fixed_dispatcher(drc, newpc);
}


/*------------------------------------------------------------------
	drc_append_set_fp_rounding
------------------------------------------------------------------*/

void drc_append_set_fp_rounding(struct drccore *drc, UINT8 regindex)
{
	_fldcw_m16isd(regindex, 2, &fp_control[0]);						/* fldcw [fp_control + reg*2]*/
	_fnstcw_m16abs(&drc->fpcw_curr);								/* fnstcw [fpcw_curr]*/
}



/*------------------------------------------------------------------
	drc_append_set_temp_fp_rounding
------------------------------------------------------------------*/

void drc_append_set_temp_fp_rounding(struct drccore *drc, UINT8 rounding)
{
	_fldcw_m16abs(&fp_control[rounding]);							/* fldcw [fp_control]*/
}



/*------------------------------------------------------------------
	drc_append_restore_fp_rounding
------------------------------------------------------------------*/

void drc_append_restore_fp_rounding(struct drccore *drc)
{
	_fldcw_m16abs(&drc->fpcw_curr);									/* fldcw [fpcw_curr]*/
}



/*------------------------------------------------------------------
	drc_dasm

	An attempt to make a disassembler for DRC code; currently limited
	by the functionality of DasmI386
------------------------------------------------------------------*/

void drc_dasm(FILE *f, unsigned pc, void *begin, void *end)
{
#if 0
	unsigned DasmI386(char* buffer, unsigned _pc);

	char buffer[256];
	char buffer2[256];
	unsigned addr = (unsigned) begin;
	unsigned addr_end = (unsigned) end;
	unsigned offset;
	UINT8 *saved_op_rom;
	UINT8 *saved_op_ram;
	size_t saved_op_mem_min;
	size_t saved_op_mem_max;

	activecpu_dasm(buffer, pc);
	if (addr == addr_end)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "%08x: %s\t(NOP)\n", pc, buffer);
	}
	else
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "%08x: %s\t(%08x-%08x)\n", pc, buffer, addr, addr_end - 1);

		saved_op_rom		= OP_ROM;
		saved_op_ram		= OP_RAM;
		saved_op_mem_min	= OP_MEM_MIN;
		saved_op_mem_max	= OP_MEM_MAX;
		OP_ROM = OP_RAM = (UINT8 *) 0;
		OP_MEM_MIN = (size_t) 0;
		OP_MEM_MAX = (size_t) -1;

		while(addr < addr_end)
		{
			offset = DasmI386(buffer, addr);
			sprintf(buffer2, "\t%08x: %s\n", addr, buffer);
			log_cb(RETRO_LOG_ERROR, LOGPRE "%s", buffer2);
			if (f)
				fputs(buffer2, f);
			addr += offset;
		}

		OP_ROM = saved_op_rom;
		OP_RAM = saved_op_ram;
		OP_MEM_MIN = saved_op_mem_min;
		OP_MEM_MAX = saved_op_mem_max;
	}
#endif
}




/*###################################################################################################
**	INTERNAL CODEGEN
**#################################################################################################*/

/*------------------------------------------------------------------
	append_entry_point
------------------------------------------------------------------*/

static void append_entry_point(struct drccore *drc)
{
	_pushad();														/* pushad*/
	if (drc->uses_fp)
	{
		_fnstcw_m16abs(&drc->fpcw_save);							/* fstcw [fpcw_save]*/
		_fldcw_m16abs(&drc->fpcw_curr);								/* fldcw [fpcw_curr]*/
	}
	drc_append_restore_volatiles(drc);								/* load volatiles*/
	if (drc->cb_entrygen)
		(*drc->cb_entrygen)(drc);									/* additional entry point duties*/
	drc_append_dispatcher(drc);										/* dispatch*/
}


/*------------------------------------------------------------------
	recompile_code
------------------------------------------------------------------*/

static void recompile_code(struct drccore *drc)
{
	if (drc->cache_top >= drc->cache_danger)
		drc_cache_reset(drc);
	(*drc->cb_recompile)(drc);
}


/*------------------------------------------------------------------
	append_recompile
------------------------------------------------------------------*/

static void append_recompile(struct drccore *drc)
{
	_push_imm(drc);													/* push	drc*/
	drc_append_save_call_restore(drc, (void *)recompile_code, 4);	/* call	recompile_code*/
	drc_append_dispatcher(drc);										/* dispatch*/
}


/*------------------------------------------------------------------
	append_out_of_cycles
------------------------------------------------------------------*/

static void append_out_of_cycles(struct drccore *drc)
{
	drc_append_save_volatiles(drc);									/* save volatiles*/
	if (drc->uses_fp)
	{
		_fnclex();													/* fnclex*/
		_fldcw_m16abs(&drc->fpcw_save);								/* fldcw [fpcw_save]*/
	}
	_popad();														/* popad*/
	_ret();															/* ret*/
}


/*------------------------------------------------------------------
	log_dispatch
------------------------------------------------------------------*/

#if LOG_DISPATCHES
static void log_dispatch(struct drccore *drc)
{
	if (keyboard_pressed(KEYCODE_D))
		log_cb(RETRO_LOG_ERROR, LOGPRE "Disp:%08X\n", *drc->pcptr);
}
#endif
