// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#include "arch.h"
#include "blockcache.h"
#include "debug.h"
#include "disassembler.h"
#include "emitter.h"
#include "interpreter.h"
#include "lightrec-config.h"
#include "lightning-wrapper.h"
#include "lightrec.h"
#include "memmanager.h"
#include "reaper.h"
#include "recompiler.h"
#include "regcache.h"
#include "optimizer.h"
#include "tlsf/tlsf.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#if ENABLE_THREADED_COMPILER
#include <stdatomic.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static struct block * lightrec_precompile_block(struct lightrec_state *state,
						u32 pc);
static bool lightrec_block_is_fully_tagged(const struct block *block);

static void lightrec_mtc2(struct lightrec_state *state, u8 reg, u32 data);
static u32 lightrec_mfc2(struct lightrec_state *state, u8 reg);

static void lightrec_reap_block(struct lightrec_state *state, void *data);

static void lightrec_default_sb(struct lightrec_state *state, u32 opcode,
				void *host, u32 addr, u32 data)
{
	*(u8 *)host = (u8)data;

	if (!(state->opt_flags & LIGHTREC_OPT_INV_DMA_ONLY))
		lightrec_invalidate(state, addr, 1);
}

static void lightrec_default_sh(struct lightrec_state *state, u32 opcode,
				void *host, u32 addr, u32 data)
{
	*(u16 *)host = HTOLE16((u16)data);

	if (!(state->opt_flags & LIGHTREC_OPT_INV_DMA_ONLY))
		lightrec_invalidate(state, addr, 2);
}

static void lightrec_default_sw(struct lightrec_state *state, u32 opcode,
				void *host, u32 addr, u32 data)
{
	*(u32 *)host = HTOLE32(data);

	if (!(state->opt_flags & LIGHTREC_OPT_INV_DMA_ONLY))
		lightrec_invalidate(state, addr, 4);
}

static u8 lightrec_default_lb(struct lightrec_state *state,
			      u32 opcode, void *host, u32 addr)
{
	return *(u8 *)host;
}

static u16 lightrec_default_lh(struct lightrec_state *state,
			       u32 opcode, void *host, u32 addr)
{
	return LE16TOH(*(u16 *)host);
}

static u32 lightrec_default_lw(struct lightrec_state *state,
			       u32 opcode, void *host, u32 addr)
{
	return LE32TOH(*(u32 *)host);
}

static u32 lightrec_default_lwu(struct lightrec_state *state,
				u32 opcode, void *host, u32 addr)
{
	u32 val;

	memcpy(&val, host, 4);

	return LE32TOH(val);
}

static void lightrec_default_swu(struct lightrec_state *state, u32 opcode,
				 void *host, u32 addr, u32 data)
{
	data = HTOLE32(data);

	memcpy(host, &data, 4);

	if (!(state->opt_flags & LIGHTREC_OPT_INV_DMA_ONLY))
		lightrec_invalidate(state, addr & ~0x3, 8);
}

static const struct lightrec_mem_map_ops lightrec_default_ops = {
	.sb = lightrec_default_sb,
	.sh = lightrec_default_sh,
	.sw = lightrec_default_sw,
	.lb = lightrec_default_lb,
	.lh = lightrec_default_lh,
	.lw = lightrec_default_lw,
	.lwu = lightrec_default_lwu,
	.swu = lightrec_default_swu,
};

static void __segfault_cb(struct lightrec_state *state, u32 addr,
			  const struct block *block)
{
	lightrec_set_exit_flags(state, LIGHTREC_EXIT_SEGFAULT);
	pr_err("Segmentation fault in recompiled code: invalid "
	       "load/store at address "PC_FMT"\n", addr);
	if (block)
		pr_err("Was executing block "PC_FMT"\n", block->pc);
}

static void lightrec_swl(struct lightrec_state *state,
			 const struct lightrec_mem_map_ops *ops,
			 u32 opcode, void *host, u32 addr, u32 data)
{
	unsigned int shift = addr & 0x3;
	unsigned int mask = shift < 3 ? GENMASK(31, (shift + 1) * 8) : 0;
	u32 old_data;

	/* Align to 32 bits */
	addr &= ~3;
	host = (void *)((uintptr_t)host & ~3);

	old_data = ops->lw(state, opcode, host, addr);

	data = (data >> ((3 - shift) * 8)) | (old_data & mask);

	ops->sw(state, opcode, host, addr, data);
}

static void lightrec_swr(struct lightrec_state *state,
			 const struct lightrec_mem_map_ops *ops,
			 u32 opcode, void *host, u32 addr, u32 data)
{
	unsigned int shift = addr & 0x3;
	unsigned int mask = (1 << (shift * 8)) - 1;
	u32 old_data;

	/* Align to 32 bits */
	addr &= ~3;
	host = (void *)((uintptr_t)host & ~3);

	old_data = ops->lw(state, opcode, host, addr);

	data = (data << (shift * 8)) | (old_data & mask);

	ops->sw(state, opcode, host, addr, data);
}

static void lightrec_swc2(struct lightrec_state *state, union code op,
			  const struct lightrec_mem_map_ops *ops,
			  void *host, u32 addr)
{
	u32 data = lightrec_mfc2(state, op.i.rt);

	ops->sw(state, op.opcode, host, addr, data);
}

static u32 lightrec_lwl(struct lightrec_state *state,
			const struct lightrec_mem_map_ops *ops,
			u32 opcode, void *host, u32 addr, u32 data)
{
	unsigned int shift = addr & 0x3;
	unsigned int mask = (1 << (24 - shift * 8)) - 1;
	u32 old_data;

	/* Align to 32 bits */
	addr &= ~3;
	host = (void *)((uintptr_t)host & ~3);

	old_data = ops->lw(state, opcode, host, addr);

	return (data & mask) | (old_data << (24 - shift * 8));
}

static u32 lightrec_lwr(struct lightrec_state *state,
			const struct lightrec_mem_map_ops *ops,
			u32 opcode, void *host, u32 addr, u32 data)
{
	unsigned int shift = addr & 0x3;
	unsigned int mask = shift ? GENMASK(31, 32 - shift * 8) : 0;
	u32 old_data;

	/* Align to 32 bits */
	addr &= ~3;
	host = (void *)((uintptr_t)host & ~3);

	old_data = ops->lw(state, opcode, host, addr);

	return (data & mask) | (old_data >> (shift * 8));
}

static void lightrec_lwc2(struct lightrec_state *state, union code op,
			  const struct lightrec_mem_map_ops *ops,
			  void *host, u32 addr)
{
	u32 data = ops->lw(state, op.opcode, host, addr);

	lightrec_mtc2(state, op.i.rt, data);
}

static void lightrec_invalidate_map(struct lightrec_state *state,
		const struct lightrec_mem_map *map, u32 addr, u32 len)
{
	if (map == &state->maps[PSX_MAP_KERNEL_USER_RAM]) {
		memset(lut_address(state, lut_offset(addr)), 0,
		       ((len + 3) / 4) * lut_elm_size(state));
	}
}

static enum psx_map
lightrec_get_map_idx(struct lightrec_state *state, u32 kaddr)
{
	const struct lightrec_mem_map *map;
	unsigned int i;

	for (i = 0; i < state->nb_maps; i++) {
		map = &state->maps[i];

		if (kaddr >= map->pc && kaddr < map->pc + map->length)
			return (enum psx_map) i;
	}

	return PSX_MAP_UNKNOWN;
}

const struct lightrec_mem_map *
lightrec_get_map(struct lightrec_state *state, void **host, u32 kaddr)
{
	const struct lightrec_mem_map *map;
	enum psx_map idx;
	u32 addr;

	idx = lightrec_get_map_idx(state, kaddr);
	if (idx == PSX_MAP_UNKNOWN)
		return NULL;

	map = &state->maps[idx];
	addr = kaddr - map->pc;

	while (map->mirror_of)
		map = map->mirror_of;

	if (host)
		*host = map->address + addr;

	return map;
}

u32 lightrec_rw(struct lightrec_state *state, union code op, u32 base,
		u32 data, u32 *flags, struct block *block, u16 offset)
{
	const struct lightrec_mem_map *map;
	const struct lightrec_mem_map_ops *ops;
	u32 opcode = op.opcode;
	bool was_tagged = true;
	u16 old_flags;
	u32 addr;
	void *host;

	addr = kunseg(base + (s16) op.i.imm);

	map = lightrec_get_map(state, &host, addr);
	if (!map) {
		__segfault_cb(state, addr, block);
		return 0;
	}

	if (flags)
		was_tagged = LIGHTREC_FLAGS_GET_IO_MODE(*flags);

	if (likely(!map->ops)) {
		if (flags && !LIGHTREC_FLAGS_GET_IO_MODE(*flags)) {
			/* Force parallel port accesses as HW accesses, because
			 * the direct-I/O emitters can't differenciate it. */
			if (unlikely(map == &state->maps[PSX_MAP_PARALLEL_PORT]))
				*flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_HW);
			/* If the base register is 0x0, be extra suspicious.
			 * Some games (e.g. Sled Storm) actually do segmentation
			 * faults by using uninitialized pointers, which are
			 * later initialized to point to hardware registers. */
			else if (op.i.rs && base == 0x0)
				*flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_HW);
			else
				*flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_DIRECT);
		}

		ops = &lightrec_default_ops;
	} else if (flags &&
		   LIGHTREC_FLAGS_GET_IO_MODE(*flags) == LIGHTREC_IO_DIRECT_HW) {
		ops = &lightrec_default_ops;
	} else {
		if (flags && !LIGHTREC_FLAGS_GET_IO_MODE(*flags))
			*flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_HW);

		ops = map->ops;
	}

	if (!was_tagged) {
		old_flags = block_set_flags(block, BLOCK_SHOULD_RECOMPILE);

		if (!(old_flags & BLOCK_SHOULD_RECOMPILE)) {
			pr_debug("Opcode of block at "PC_FMT" has been tagged"
				 " - flag for recompilation\n", block->pc);

			lut_write(state, lut_offset(block->pc), NULL);
		}
	}

	switch (op.i.op) {
	case OP_SB:
		ops->sb(state, opcode, host, addr, data);
		return 0;
	case OP_SH:
		ops->sh(state, opcode, host, addr, data);
		return 0;
	case OP_SWL:
		lightrec_swl(state, ops, opcode, host, addr, data);
		return 0;
	case OP_SWR:
		lightrec_swr(state, ops, opcode, host, addr, data);
		return 0;
	case OP_SW:
		ops->sw(state, opcode, host, addr, data);
		return 0;
	case OP_SWC2:
		lightrec_swc2(state, op, ops, host, addr);
		return 0;
	case OP_LB:
		return (s32) (s8) ops->lb(state, opcode, host, addr);
	case OP_LBU:
		return ops->lb(state, opcode, host, addr);
	case OP_LH:
		return (s32) (s16) ops->lh(state, opcode, host, addr);
	case OP_LHU:
		return ops->lh(state, opcode, host, addr);
	case OP_LWC2:
		lightrec_lwc2(state, op, ops, host, addr);
		return 0;
	case OP_LWL:
		return lightrec_lwl(state, ops, opcode, host, addr, data);
	case OP_LWR:
		return lightrec_lwr(state, ops, opcode, host, addr, data);
	case OP_META_LWU:
		return ops->lwu(state, opcode, host, addr);
	case OP_META_SWU:
		ops->swu(state, opcode, host, addr, data);
		return 0;
	case OP_LW:
	default:
		return ops->lw(state, opcode, host, addr);
	}
}

static void lightrec_rw_helper(struct lightrec_state *state,
			       union code op, u32 *flags,
			       struct block *block, u16 offset)
{
	u32 ret = lightrec_rw(state, op, state->regs.gpr[op.i.rs],
			      state->regs.gpr[op.i.rt], flags, block, offset);

	switch (op.i.op) {
	case OP_LB:
	case OP_LBU:
	case OP_LH:
	case OP_LHU:
	case OP_LWL:
	case OP_LWR:
	case OP_LW:
	case OP_META_LWU:
		if (OPT_HANDLE_LOAD_DELAYS && unlikely(!state->in_delay_slot_n)) {
			state->temp_reg = ret;
			state->in_delay_slot_n = 0xff;
		} else if (op.i.rt) {
			state->regs.gpr[op.i.rt] = ret;
		}
		fallthrough;
	default:
		break;
	}
}

static void lightrec_rw_cb(struct lightrec_state *state, u32 arg)
{
	lightrec_rw_helper(state, (union code) arg, NULL, NULL, 0);
}

static void lightrec_rw_generic_cb(struct lightrec_state *state, u32 arg)
{
	struct block *block;
	struct opcode *op;
	u16 offset = (u16)arg;

	block = lightrec_find_block_from_lut(state->block_cache,
					     arg >> 16, state->curr_pc);
	if (unlikely(!block)) {
		pr_err("rw_generic: No block found in LUT for "PC_FMT" offset 0x%"PRIx16"\n",
			 state->curr_pc, offset);
		lightrec_set_exit_flags(state, LIGHTREC_EXIT_SEGFAULT);
		return;
	}

	op = &block->opcode_list[offset];
	lightrec_rw_helper(state, op->c, &op->flags, block, offset);
}

static u32 clamp_s32(s32 val, s32 min, s32 max)
{
	return val < min ? min : val > max ? max : val;
}

static u16 load_u16(u32 *ptr)
{
	return ((struct u16x2 *) ptr)->l;
}

static void store_u16(u32 *ptr, u16 value)
{
	((struct u16x2 *) ptr)->l = value;
}

static u32 lightrec_mfc2(struct lightrec_state *state, u8 reg)
{
	s16 gteir1, gteir2, gteir3;

	switch (reg) {
	case 1:
	case 3:
	case 5:
	case 8:
	case 9:
	case 10:
	case 11:
		return (s32)(s16) load_u16(&state->regs.cp2d[reg]);
	case 7:
	case 16:
	case 17:
	case 18:
	case 19:
		return load_u16(&state->regs.cp2d[reg]);
	case 28:
	case 29:
		gteir1 = (s16) load_u16(&state->regs.cp2d[9]);
		gteir2 = (s16) load_u16(&state->regs.cp2d[10]);
		gteir3 = (s16) load_u16(&state->regs.cp2d[11]);

		return clamp_s32(gteir1 >> 7, 0, 0x1f) << 0 |
			clamp_s32(gteir2 >> 7, 0, 0x1f) << 5 |
			clamp_s32(gteir3 >> 7, 0, 0x1f) << 10;
	case 15:
		reg = 14;
		fallthrough;
	default:
		return state->regs.cp2d[reg];
	}
}

u32 lightrec_mfc(struct lightrec_state *state, union code op)
{
	u32 val;

	if (op.i.op == OP_CP0)
		return state->regs.cp0[op.r.rd];

	if (op.i.op == OP_SWC2) {
		val = lightrec_mfc2(state, op.i.rt);
	} else if (op.r.rs == OP_CP2_BASIC_MFC2)
		val = lightrec_mfc2(state, op.r.rd);
	else {
		val = state->regs.cp2c[op.r.rd];

		switch (op.r.rd) {
		case 4:
		case 12:
		case 20:
		case 26:
		case 27:
		case 29:
		case 30:
			val = (u32)(s16)val;
			fallthrough;
		default:
			break;
		}
	}

	if (state->ops.cop2_notify)
		(*state->ops.cop2_notify)(state, op.opcode, val);

	return val;
}

static void lightrec_mfc_cb(struct lightrec_state *state, union code op)
{
	u32 rt = lightrec_mfc(state, op);

	if (op.i.op == OP_SWC2)
		state->temp_reg = rt;
	else if (op.r.rt)
		state->regs.gpr[op.r.rt] = rt;
}

static void lightrec_mtc0(struct lightrec_state *state, u8 reg, u32 data)
{
	u32 status, oldstatus, cause;

	switch (reg) {
	case 1:
	case 4:
	case 8:
	case 14:
	case 15:
		/* Those registers are read-only */
		return;
	default:
		break;
	}

	if (reg == 12) {
		status = state->regs.cp0[12];
		oldstatus = status;

		if (status & ~data & BIT(16)) {
			state->ops.enable_ram(state, true);
			lightrec_invalidate_all(state);
		} else if (~status & data & BIT(16)) {
			state->ops.enable_ram(state, false);
		}
	}

	if (reg == 13) {
		state->regs.cp0[13] &= ~0x300;
		state->regs.cp0[13] |= data & 0x300;
	} else {
		state->regs.cp0[reg] = data;
	}

	if (reg == 12 || reg == 13) {
		cause = state->regs.cp0[13];
		status = state->regs.cp0[12];

		/* Handle software interrupts */
		if ((!!(status & cause & 0x300)) & status)
			lightrec_set_exit_flags(state, LIGHTREC_EXIT_CHECK_INTERRUPT);

		/* Handle hardware interrupts */
		if (reg == 12 && !(~status & 0x401) && (~oldstatus & 0x401))
			lightrec_set_exit_flags(state, LIGHTREC_EXIT_CHECK_INTERRUPT);
	}
}

static u32 count_leading_bits(s32 data)
{
#ifdef __has_builtin
#if __has_builtin(__builtin_clrsb)
	return 1 + __builtin_clrsb(data);
#endif
#endif
	data ^= data >> 31;
	return data ? clz32(data) : 32;
}

static void lightrec_mtc2(struct lightrec_state *state, u8 reg, u32 data)
{
	switch (reg) {
	case 15:
		state->regs.cp2d[12] = state->regs.cp2d[13];
		state->regs.cp2d[13] = state->regs.cp2d[14];
		state->regs.cp2d[14] = data;
		break;
	case 28:
		state->regs.cp2d[9] = (data << 7) & 0xf80;
		state->regs.cp2d[10] = (data << 2) & 0xf80;
		state->regs.cp2d[11] = (data >> 3) & 0xf80;
		break;
	case 31:
		return;
	case 30:
		state->regs.cp2d[31] = count_leading_bits((s32) data);
		fallthrough;
	default:
		state->regs.cp2d[reg] = data;
		break;
	}
}

static void lightrec_ctc2(struct lightrec_state *state, u8 reg, u32 data)
{
	switch (reg) {
	case 4:
	case 12:
	case 20:
	case 26:
	case 27:
	case 29:
	case 30:
		store_u16(&state->regs.cp2c[reg], data);
		break;
	case 31:
		data = (data & 0x7ffff000) | !!(data & 0x7f87e000) << 31;
		fallthrough;
	default:
		state->regs.cp2c[reg] = data;
		break;
	}
}

void lightrec_mtc(struct lightrec_state *state, union code op, u8 reg, u32 data)
{
	if (op.i.op == OP_CP0) {
		lightrec_mtc0(state, reg, data);
	} else {
		if (op.i.op == OP_LWC2 || op.r.rs != OP_CP2_BASIC_CTC2)
			lightrec_mtc2(state, reg, data);
		else
			lightrec_ctc2(state, reg, data);

		if (state->ops.cop2_notify)
			(*state->ops.cop2_notify)(state, op.opcode, data);
	}
}

static void lightrec_mtc_cb(struct lightrec_state *state, u32 arg)
{
	union code op = (union code) arg;
	u32 data;
	u8 reg;

	if (op.i.op == OP_LWC2) {
		data = state->temp_reg;
		reg = op.i.rt;
	} else {
		data = state->regs.gpr[op.r.rt];
		reg = op.r.rd;
	}

	lightrec_mtc(state, op, reg, data);
}

void lightrec_rfe(struct lightrec_state *state)
{
	u32 status;

	/* Read CP0 Status register (r12) */
	status = state->regs.cp0[12];

	/* Switch the bits */
	status = ((status & 0x3c) >> 2) | (status & ~0xf);

	/* Write it back */
	lightrec_mtc0(state, 12, status);
}

void lightrec_cp(struct lightrec_state *state, union code op)
{
	if (op.i.op == OP_CP0) {
		pr_err("Invalid CP opcode to coprocessor #0\n");
		return;
	}

	(*state->ops.cop2_op)(state, op.opcode);
}

static void lightrec_cp_cb(struct lightrec_state *state, u32 arg)
{
	lightrec_cp(state, (union code) arg);
}

static struct block * lightrec_get_block(struct lightrec_state *state, u32 pc)
{
	struct block *block = lightrec_find_block(state->block_cache, pc);
	u8 old_flags;

	if (block && lightrec_block_is_outdated(state, block)) {
		pr_debug("Block at "PC_FMT" is outdated!\n", block->pc);

		old_flags = block_set_flags(block, BLOCK_IS_DEAD);
		if (!(old_flags & BLOCK_IS_DEAD)) {
			/* Make sure the recompiler isn't processing the block
			 * we'll destroy */
			if (ENABLE_THREADED_COMPILER)
				lightrec_recompiler_remove(state->rec, block);

			remove_from_code_lut(state->block_cache, block);

			if (ENABLE_THREADED_COMPILER) {
				lightrec_reaper_add(state->reaper,
						    lightrec_reap_block, block);
			} else {
				lightrec_unregister_block(state->block_cache, block);
				lightrec_free_block(state, block);
			}
		}

		block = NULL;
	}

	if (!block) {
		block = lightrec_precompile_block(state, pc);
		if (!block) {
			pr_err("Unable to recompile block at "PC_FMT"\n", pc);
			lightrec_set_exit_flags(state, LIGHTREC_EXIT_SEGFAULT);
			return NULL;
		}

		lightrec_register_block(state->block_cache, block);
	}

	return block;
}

static void * get_next_block_func(struct lightrec_state *state, u32 pc)
{
	struct block *block;
	bool should_recompile;
	void *func;
	int err;

	do {
		func = lut_read(state, lut_offset(pc));
		if (func && func != state->get_next_block)
			break;

		block = lightrec_get_block(state, pc);

		if (unlikely(!block))
			break;

		if (OPT_REPLACE_MEMSET &&
		    block_has_flag(block, BLOCK_IS_MEMSET)) {
			func = state->memset_func;
			break;
		}

		should_recompile = block_has_flag(block, BLOCK_SHOULD_RECOMPILE) &&
			!block_has_flag(block, BLOCK_NEVER_COMPILE) &&
			!block_has_flag(block, BLOCK_IS_DEAD);

		if (unlikely(should_recompile)) {
			pr_debug("Block at "PC_FMT" should recompile\n", pc);

			if (ENABLE_THREADED_COMPILER) {
				lightrec_recompiler_add(state->rec, block);
			} else {
				err = lightrec_compile_block(state->cstate, block);
				if (err) {
					state->exit_flags = LIGHTREC_EXIT_NOMEM;
					return NULL;
				}
			}
		}

		if (ENABLE_THREADED_COMPILER && likely(!should_recompile))
			func = lightrec_recompiler_run_first_pass(state, block, &pc);
		else
			func = block->function;

		if (likely(func))
			break;

		if (unlikely(block_has_flag(block, BLOCK_NEVER_COMPILE))) {
			pc = lightrec_emulate_block(state, block, pc);

		} else if (!ENABLE_THREADED_COMPILER) {
			/* Block wasn't compiled yet - run the interpreter */
			if (block_has_flag(block, BLOCK_FULLY_TAGGED))
				pr_debug("Block fully tagged, skipping first pass\n");
			else if (ENABLE_FIRST_PASS && likely(!should_recompile))
				pc = lightrec_emulate_block(state, block, pc);

			/* Then compile it using the profiled data */
			err = lightrec_compile_block(state->cstate, block);
			if (err) {
				state->exit_flags = LIGHTREC_EXIT_NOMEM;
				return NULL;
			}
		} else if (unlikely(block_has_flag(block, BLOCK_IS_DEAD))) {
			/*
			 * If the block is dead but has never been compiled,
			 * then its function pointer is NULL and we cannot
			 * execute the block. In that case, reap all the dead
			 * blocks now, and in the next loop we will create a
			 * new block.
			 */
			lightrec_reaper_reap(state->reaper);
		} else {
			lightrec_recompiler_add(state->rec, block);
		}
	} while (state->exit_flags == LIGHTREC_EXIT_NORMAL
		 && state->current_cycle < state->target_cycle);

	state->curr_pc = pc;
	return func;
}

static void * lightrec_alloc_code(struct lightrec_state *state, size_t size)
{
	void *code;

	if (ENABLE_THREADED_COMPILER)
		lightrec_code_alloc_lock(state);

	code = tlsf_malloc(state->tlsf, size);

	if (ENABLE_THREADED_COMPILER)
		lightrec_code_alloc_unlock(state);

	return code;
}

static void lightrec_realloc_code(struct lightrec_state *state,
				  void *ptr, size_t size)
{
	/* NOTE: 'size' MUST be smaller than the size specified during
	 * the allocation. */

	if (ENABLE_THREADED_COMPILER)
		lightrec_code_alloc_lock(state);

	tlsf_realloc(state->tlsf, ptr, size);

	if (ENABLE_THREADED_COMPILER)
		lightrec_code_alloc_unlock(state);
}

static void lightrec_free_code(struct lightrec_state *state, void *ptr)
{
	if (ENABLE_THREADED_COMPILER)
		lightrec_code_alloc_lock(state);

	tlsf_free(state->tlsf, ptr);

	if (ENABLE_THREADED_COMPILER)
		lightrec_code_alloc_unlock(state);
}

static char lightning_code_data[0x80000];

static void * lightrec_emit_code(struct lightrec_state *state,
				 const struct block *block,
				 jit_state_t *_jit, unsigned int *size)
{
	bool has_code_buffer = ENABLE_CODE_BUFFER && state->tlsf;
	jit_word_t code_size, new_code_size;
	void *code;

	jit_realize();

	if (ENABLE_DISASSEMBLER)
		jit_set_data(lightning_code_data, sizeof(lightning_code_data), 0);
	else
		jit_set_data(NULL, 0, JIT_DISABLE_DATA | JIT_DISABLE_NOTE);

	if (has_code_buffer) {
		jit_get_code(&code_size);

#ifdef __i386__
		/* Lightning's code size estimation routine is buggy on x86 and
		 * will return a value that's too small. */
		code_size *= 2;
#endif

		code = lightrec_alloc_code(state, (size_t) code_size);

		if (!code) {
			if (ENABLE_THREADED_COMPILER) {
				/* If we're using the threaded compiler, return
				 * an allocation error here. The threaded
				 * compiler will then empty its job queue and
				 * request a code flush using the reaper. */
				return NULL;
			}

			/* Remove outdated blocks, and try again */
			lightrec_remove_outdated_blocks(state->block_cache, block);

			pr_debug("Re-try to alloc %zu bytes...\n", code_size);

			code = lightrec_alloc_code(state, code_size);
			if (!code) {
				pr_err("Could not alloc even after removing old blocks!\n");
				return NULL;
			}
		}

		jit_set_code(code, code_size);
	}

	code = jit_emit();
	if (!code) {
		if (has_code_buffer)
			lightrec_free_code(state, code);

		return NULL;
	}

	jit_get_code(&new_code_size);
	lightrec_register(MEM_FOR_CODE, new_code_size);

	if (has_code_buffer) {
		lightrec_realloc_code(state, code, (size_t) new_code_size);

		pr_debug("Creating code block at address 0x%" PRIxPTR ", "
			 "code size: %" PRIuPTR " new: %" PRIuPTR "\n",
			 (uintptr_t) code, code_size, new_code_size);
	}

	*size = (unsigned int) new_code_size;

	if (state->ops.code_inv)
		state->ops.code_inv(code, new_code_size);

	return code;
}

static struct block * generate_wrapper(struct lightrec_state *state)
{
	struct block *block;
	jit_state_t *_jit;
	unsigned int i;

	block = lightrec_malloc(state, MEM_FOR_IR, sizeof(*block));
	if (!block)
		goto err_no_mem;

	_jit = jit_new_state();
	if (!_jit)
		goto err_free_block;

	jit_name("RW wrapper");
	jit_note(__FILE__, __LINE__);

	/* Wrapper entry point */
	jit_prolog();
	jit_tramp(256);

	/* Load pointer to C wrapper */
	jit_add_state(JIT_R1, JIT_R1);
	jit_ldxi(JIT_R1, JIT_R1, lightrec_offset(c_wrappers));

	jit_epilog();
	jit_prolog();

	/* Save all temporaries on stack */
	for (i = 0; i < NUM_TEMPS; i++) {
		if (i + FIRST_TEMP != 1) {
			jit_stxi(lightrec_offset(wrapper_regs[i]),
				 LIGHTREC_REG_STATE, JIT_R(i + FIRST_TEMP));
		}
	}

	jit_getarg(JIT_R2, jit_arg());

	jit_prepare();
	jit_pushargr(LIGHTREC_REG_STATE);
	jit_pushargr(JIT_R2);

	jit_ldxi_ui(JIT_R2, LIGHTREC_REG_STATE, lightrec_offset(target_cycle));

	/* state->current_cycle = state->target_cycle - delta; */
	jit_subr(LIGHTREC_REG_CYCLE, JIT_R2, LIGHTREC_REG_CYCLE);
	jit_stxi_i(lightrec_offset(current_cycle), LIGHTREC_REG_STATE, LIGHTREC_REG_CYCLE);

	/* Call the wrapper function */
	jit_finishr(JIT_R1);

	/* delta = state->target_cycle - state->current_cycle */;
	jit_ldxi_ui(LIGHTREC_REG_CYCLE, LIGHTREC_REG_STATE, lightrec_offset(current_cycle));
	jit_ldxi_ui(JIT_R1, LIGHTREC_REG_STATE, lightrec_offset(target_cycle));
	jit_subr(LIGHTREC_REG_CYCLE, JIT_R1, LIGHTREC_REG_CYCLE);

	/* Restore temporaries from stack */
	for (i = 0; i < NUM_TEMPS; i++) {
		if (i + FIRST_TEMP != 1) {
			jit_ldxi(JIT_R(i + FIRST_TEMP), LIGHTREC_REG_STATE,
				 lightrec_offset(wrapper_regs[i]));
		}
	}

	jit_ret();
	jit_epilog();

	block->_jit = _jit;
	block->opcode_list = NULL;
	block->flags = BLOCK_NO_OPCODE_LIST;
	block->nb_ops = 0;

	block->function = lightrec_emit_code(state, block, _jit,
					     &block->code_size);
	if (!block->function)
		goto err_free_jit;

	state->c_wrapper = block->function;

	if (ENABLE_DISASSEMBLER) {
		pr_debug("Wrapper block:\n");
		jit_disassemble();
	}

	jit_clear_state();
	return block;

err_free_jit:
	jit_destroy_state();
err_free_block:
	lightrec_free(state, MEM_FOR_IR, sizeof(*block), block);
err_no_mem:
	pr_err("Unable to compile wrapper: Out of memory\n");
	return NULL;
}

static u32 lightrec_memset(struct lightrec_state *state)
{
	u32 kunseg_pc = kunseg(state->regs.gpr[4]);
	void *host;
	const struct lightrec_mem_map *map = lightrec_get_map(state, &host, kunseg_pc);
	u32 length = state->regs.gpr[5] * 4;

	if (!map) {
		pr_err("Unable to find memory map for memset target address "PC_FMT"\n",
		       kunseg_pc);
		return 0;
	}

	pr_debug("Calling host memset, "PC_FMT" (host address 0x%"PRIxPTR") for %"PRIu32" bytes\n",
		 kunseg_pc, (uintptr_t)host, length);
	memset(host, 0, length);

	if (!(state->opt_flags & LIGHTREC_OPT_INV_DMA_ONLY))
		lightrec_invalidate_map(state, map, kunseg_pc, length);

	/* Rough estimation of the number of cycles consumed */
	return 8 + 5 * (length  + 3 / 4);
}

static u32 lightrec_check_load_delay(struct lightrec_state *state, u32 pc, u8 reg)
{
	struct block *block;
	union code first_op;

	first_op = lightrec_read_opcode(state, pc);

	if (likely(!opcode_reads_register(first_op, reg))) {
		state->regs.gpr[reg] = state->temp_reg;
	} else {
		block = lightrec_get_block(state, pc);
		if (unlikely(!block)) {
			pr_err("Unable to get block at "PC_FMT"\n", pc);
			lightrec_set_exit_flags(state, LIGHTREC_EXIT_SEGFAULT);
			pc = 0;
		} else {
			pc = lightrec_handle_load_delay(state, block, pc, reg);
		}
	}

	return pc;
}

static void update_cycle_counter_before_c(jit_state_t *_jit)
{
	/* update state->current_cycle */
	jit_ldxi_i(JIT_R2, LIGHTREC_REG_STATE, lightrec_offset(target_cycle));
	jit_subr(JIT_R1, JIT_R2, LIGHTREC_REG_CYCLE);
	jit_stxi_i(lightrec_offset(current_cycle), LIGHTREC_REG_STATE, JIT_R1);
}

static void update_cycle_counter_after_c(jit_state_t *_jit)
{
	/* Recalc the delta */
	jit_ldxi_i(JIT_R1, LIGHTREC_REG_STATE, lightrec_offset(current_cycle));
	jit_ldxi_i(JIT_R2, LIGHTREC_REG_STATE, lightrec_offset(target_cycle));
	jit_subr(LIGHTREC_REG_CYCLE, JIT_R2, JIT_R1);
}

static void sync_next_pc(jit_state_t *_jit)
{
	if (lightrec_store_next_pc()) {
		jit_ldxi_ui(JIT_V0, LIGHTREC_REG_STATE,
			    lightrec_offset(next_pc));
	}
}

static struct block * generate_dispatcher(struct lightrec_state *state)
{
	struct block *block;
	jit_state_t *_jit;
	jit_node_t *to_end, *loop, *loop2,
		   *addr, *addr2, *addr3, *addr4, *addr5;
	unsigned int i;
	u32 offset;

	block = lightrec_malloc(state, MEM_FOR_IR, sizeof(*block));
	if (!block)
		goto err_no_mem;

	_jit = jit_new_state();
	if (!_jit)
		goto err_free_block;

	jit_name("dispatcher");
	jit_note(__FILE__, __LINE__);

	jit_prolog();
	jit_frame(256);

	jit_getarg(LIGHTREC_REG_STATE, jit_arg());
	jit_getarg(JIT_V0, jit_arg());
	jit_getarg(JIT_V1, jit_arg());
	jit_getarg_i(LIGHTREC_REG_CYCLE, jit_arg());

	/* Force all callee-saved registers to be pushed on the stack */
	for (i = 0; i < NUM_REGS; i++)
		jit_movr(JIT_V(i + FIRST_REG), JIT_V(i + FIRST_REG));

	loop = jit_label();

	if (!arch_has_fast_mask())
		jit_movi(JIT_R1, 0x1fffffff);

	/* Call the block's code */
	jit_jmpr(JIT_V1);

	/* The block will jump here, with the number of cycles remaining in
	 * LIGHTREC_REG_CYCLE */
	addr2 = jit_indirect();

	sync_next_pc(_jit);

	loop2 = jit_label();

	/* Jump to end if state->target_cycle < state->current_cycle */
	to_end = jit_blei(LIGHTREC_REG_CYCLE, 0);

	/* Convert next PC to KUNSEG and avoid mirrors */
	jit_andi(JIT_V1, JIT_V0, RAM_SIZE - 1);
	jit_andi(JIT_R2, JIT_V0, BIOS_SIZE - 1);
	jit_andi(JIT_R1, JIT_V0, BIT(28));
	jit_addi(JIT_R2, JIT_R2, RAM_SIZE);
	jit_movnr(JIT_V1, JIT_R2, JIT_R1);

	/* If possible, use the code LUT */
	if (!lut_is_32bit(state))
		jit_lshi(JIT_V1, JIT_V1, 1);
	jit_add_state(JIT_V1, JIT_V1);

	offset = lightrec_offset(code_lut);
	if (lut_is_32bit(state))
		jit_ldxi_ui(JIT_V1, JIT_V1, offset);
	else
		jit_ldxi(JIT_V1, JIT_V1, offset);

	/* Store back the current PC to the lightrec_state structure */
	jit_stxi_i(lightrec_offset(curr_pc), LIGHTREC_REG_STATE, JIT_V0);

	/* If we get non-NULL, loop */
	jit_patch_at(jit_bnei(JIT_V1, 0), loop);

	/* The code LUT will be set to this address when the block at the target
	 * PC has been preprocessed but not yet compiled by the threaded
	 * recompiler */
	addr = jit_indirect();

	/* Slow path: call C function get_next_block_func() */

	if (ENABLE_FIRST_PASS || OPT_DETECT_IMPOSSIBLE_BRANCHES) {
		/* We may call the interpreter - update state->current_cycle */
		update_cycle_counter_before_c(_jit);
	}

	jit_prepare();
	jit_pushargr(LIGHTREC_REG_STATE);
	jit_pushargr(JIT_V0);

	/* Save the cycles register if needed */
	if (!(ENABLE_FIRST_PASS || OPT_DETECT_IMPOSSIBLE_BRANCHES))
		jit_movr(JIT_V0, LIGHTREC_REG_CYCLE);

	/* Get the next block */
	jit_finishi(&get_next_block_func);
	jit_retval(JIT_V1);

	if (ENABLE_FIRST_PASS || OPT_DETECT_IMPOSSIBLE_BRANCHES) {
		/* The interpreter may have updated state->current_cycle and
		 * state->target_cycle - recalc the delta */
		update_cycle_counter_after_c(_jit);
	} else {
		jit_movr(LIGHTREC_REG_CYCLE, JIT_V0);
	}

	/* Reset JIT_V0 to the next PC */
	jit_ldxi_ui(JIT_V0, LIGHTREC_REG_STATE, lightrec_offset(curr_pc));

	/* If we get non-NULL, loop */
	jit_patch_at(jit_bnei(JIT_V1, 0), loop);

	/* When exiting, the recompiled code will jump to that address */
	jit_note(__FILE__, __LINE__);
	jit_patch(to_end);

	/* Store back the current PC to the lightrec_state structure */
	jit_stxi_i(lightrec_offset(curr_pc), LIGHTREC_REG_STATE, JIT_V0);

	jit_retr(LIGHTREC_REG_CYCLE);

	if (OPT_REPLACE_MEMSET) {
		/* Blocks will jump here when they need to call
		 * lightrec_memset() */
		addr3 = jit_indirect();

		jit_movr(JIT_V1, LIGHTREC_REG_CYCLE);

		jit_prepare();
		jit_pushargr(LIGHTREC_REG_STATE);

		jit_finishi(lightrec_memset);
		jit_retval(LIGHTREC_REG_CYCLE);

		jit_ldxi_ui(JIT_V0, LIGHTREC_REG_STATE, lightrec_offset(regs.gpr[31]));

		jit_subr(LIGHTREC_REG_CYCLE, JIT_V1, LIGHTREC_REG_CYCLE);

		jit_patch_at(jit_b(), loop2);
	}

	if (OPT_DETECT_IMPOSSIBLE_BRANCHES) {
		/* Blocks will jump here when they reach a branch that should
		 * be executed with the interpreter, passing the branch's PC
		 * in JIT_V0 and the address of the block in JIT_V1. */
		addr4 = jit_indirect();

		sync_next_pc(_jit);
		update_cycle_counter_before_c(_jit);

		jit_prepare();
		jit_pushargr(LIGHTREC_REG_STATE);
		jit_pushargr(JIT_V1);
		jit_pushargr(JIT_V0);
		jit_finishi(lightrec_emulate_block);

		jit_retval(JIT_V0);

		update_cycle_counter_after_c(_jit);

		jit_patch_at(jit_b(), loop2);

	}

	if (OPT_HANDLE_LOAD_DELAYS) {
		/* Blocks will jump here when they reach a branch with a load
		 * opcode in its delay slot. The delay slot has already been
		 * executed; the load value is in (state->temp_reg), and the
		 * register number is in JIT_V1.
		 * Jump to a C function which will evaluate the branch target's
		 * first opcode, to make sure that it does not read the register
		 * in question; and if it does, handle it accordingly. */
		addr5 = jit_indirect();

		sync_next_pc(_jit);
		update_cycle_counter_before_c(_jit);

		jit_prepare();
		jit_pushargr(LIGHTREC_REG_STATE);
		jit_pushargr(JIT_V0);
		jit_pushargr(JIT_V1);
		jit_finishi(lightrec_check_load_delay);

		jit_retval(JIT_V0);

		update_cycle_counter_after_c(_jit);

		jit_patch_at(jit_b(), loop2);
	}

	jit_epilog();

	block->_jit = _jit;
	block->opcode_list = NULL;
	block->flags = BLOCK_NO_OPCODE_LIST;
	block->nb_ops = 0;

	block->function = lightrec_emit_code(state, block, _jit,
					     &block->code_size);
	if (!block->function)
		goto err_free_jit;

	state->eob_wrapper_func = jit_address(addr2);
	if (OPT_DETECT_IMPOSSIBLE_BRANCHES)
		state->interpreter_func = jit_address(addr4);
	if (OPT_HANDLE_LOAD_DELAYS)
		state->ds_check_func = jit_address(addr5);
	if (OPT_REPLACE_MEMSET)
		state->memset_func = jit_address(addr3);
	state->get_next_block = jit_address(addr);

	if (ENABLE_DISASSEMBLER) {
		pr_debug("Dispatcher block:\n");
		jit_disassemble();
	}

	/* We're done! */
	jit_clear_state();
	return block;

err_free_jit:
	jit_destroy_state();
err_free_block:
	lightrec_free(state, MEM_FOR_IR, sizeof(*block), block);
err_no_mem:
	pr_err("Unable to compile dispatcher: Out of memory\n");
	return NULL;
}

union code lightrec_read_opcode(struct lightrec_state *state, u32 pc)
{
	void *host = NULL;

	lightrec_get_map(state, &host, kunseg(pc));

	const u32 *code = (u32 *)host;
	return (union code) LE32TOH(*code);
}

unsigned int lightrec_cycles_of_opcode(const struct lightrec_state *state,
				       union code code)
{
	return state->cycles_per_op;
}

void lightrec_free_opcode_list(struct lightrec_state *state, struct opcode *ops)
{
	struct opcode_list *list = container_of(ops, struct opcode_list, ops);

	lightrec_free(state, MEM_FOR_IR,
		      sizeof(*list) + list->nb_ops * sizeof(struct opcode),
		      list);
}

static unsigned int lightrec_get_mips_block_len(const u32 *src)
{
	unsigned int i;
	union code c;

	for (i = 1; ; i++) {
		c.opcode = LE32TOH(*src++);

		if (is_syscall(c))
			return i;

		if (c.i.op == OP_META_BIOS)
			return i;

		if (is_unconditional_jump(c))
			return i + 1;
	}
}

static struct opcode * lightrec_disassemble(struct lightrec_state *state,
					    const u32 *src, unsigned int *len)
{
	struct opcode_list *list;
	unsigned int i, length;

	length = lightrec_get_mips_block_len(src);

	list = lightrec_malloc(state, MEM_FOR_IR,
			       sizeof(*list) + sizeof(struct opcode) * length);
	if (!list) {
		pr_err("Unable to allocate memory\n");
		return NULL;
	}

	list->nb_ops = (u16) length;

	for (i = 0; i < length; i++) {
		list->ops[i].opcode = LE32TOH(src[i]);
		list->ops[i].flags = 0;
	}

	*len = length * sizeof(u32);

	return list->ops;
}

static struct block * lightrec_precompile_block(struct lightrec_state *state,
						u32 pc)
{
	struct opcode *list;
	struct block *block;
	void *host, *addr;
	const struct lightrec_mem_map *map = lightrec_get_map(state, &host, kunseg(pc));
	const u32 *code = (u32 *) host;
	unsigned int length;
	bool fully_tagged;
	u8 block_flags = 0;

	if (!map)
		return NULL;

	block = lightrec_malloc(state, MEM_FOR_IR, sizeof(*block));
	if (!block) {
		pr_err("Unable to recompile block: Out of memory\n");
		return NULL;
	}

	list = lightrec_disassemble(state, code, &length);
	if (!list) {
		lightrec_free(state, MEM_FOR_IR, sizeof(*block), block);
		return NULL;
	}

	block->pc = pc;
	block->_jit = NULL;
	block->function = NULL;
	block->opcode_list = list;
	block->code = code;
	block->next = NULL;
	block->flags = 0;
	block->code_size = 0;
	block->precompile_date = state->current_cycle;
	block->nb_ops = length / sizeof(u32);

	lightrec_optimize(state, block);

	length = block->nb_ops * sizeof(u32);

	lightrec_register(MEM_FOR_MIPS_CODE, length);

	if (ENABLE_DISASSEMBLER) {
		pr_debug("Disassembled block at "PC_FMT"\n", block->pc);
		lightrec_print_disassembly(block, code);
	}

	pr_debug("Block size: %hu opcodes\n", block->nb_ops);

	fully_tagged = lightrec_block_is_fully_tagged(block);
	if (fully_tagged)
		block_flags |= BLOCK_FULLY_TAGGED;

	if (block_flags)
		block_set_flags(block, block_flags);

	block->hash = lightrec_calculate_block_hash(block);

	if (OPT_REPLACE_MEMSET && block_has_flag(block, BLOCK_IS_MEMSET))
		addr = state->memset_func;
	else
		addr = state->get_next_block;
	lut_write(state, lut_offset(pc), addr);

	pr_debug("Blocks created: %u\n", ++state->nb_precompile);

	return block;
}

static bool lightrec_block_is_fully_tagged(const struct block *block)
{
	const struct opcode *op;
	unsigned int i;

	for (i = 0; i < block->nb_ops; i++) {
		op = &block->opcode_list[i];

		/* If we have one branch that must be emulated, we cannot trash
		 * the opcode list. */
		if (should_emulate(op))
			return false;

		/* Check all loads/stores of the opcode list and mark the
		 * block as fully compiled if they all have been tagged. */
		switch (op->c.i.op) {
		case OP_LB:
		case OP_LH:
		case OP_LWL:
		case OP_LW:
		case OP_LBU:
		case OP_LHU:
		case OP_LWR:
		case OP_SB:
		case OP_SH:
		case OP_SWL:
		case OP_SW:
		case OP_SWR:
		case OP_LWC2:
		case OP_SWC2:
		case OP_META_LWU:
		case OP_META_SWU:
			if (!LIGHTREC_FLAGS_GET_IO_MODE(op->flags))
				return false;
			fallthrough;
		default:
			continue;
		}
	}

	return true;
}

static void lightrec_reap_block(struct lightrec_state *state, void *data)
{
	struct block *block = data;

	pr_debug("Reap dead block at "PC_FMT"\n", block->pc);
	lightrec_unregister_block(state->block_cache, block);
	lightrec_free_block(state, block);
}

static void lightrec_reap_jit(struct lightrec_state *state, void *data)
{
	_jit_destroy_state(data);
}

static void lightrec_free_function(struct lightrec_state *state, void *fn)
{
	if (ENABLE_CODE_BUFFER && state->tlsf) {
		pr_debug("Freeing code block at 0x%" PRIxPTR "\n", (uintptr_t) fn);
		lightrec_free_code(state, fn);
	}
}

static void lightrec_reap_function(struct lightrec_state *state, void *data)
{
	lightrec_free_function(state, data);
}

static void lightrec_reap_opcode_list(struct lightrec_state *state, void *data)
{
	lightrec_free_opcode_list(state, data);
}

int lightrec_compile_block(struct lightrec_cstate *cstate,
			   struct block *block)
{
	struct block *dead_blocks[ARRAY_SIZE(cstate->targets)];
	u32 was_dead[ARRAY_SIZE(cstate->targets) / 8];
	struct lightrec_state *state = cstate->state;
	struct lightrec_branch_target *target;
	bool fully_tagged = false;
	struct block *block2;
	struct opcode *elm;
	jit_state_t *_jit, *oldjit;
	jit_node_t *start_of_block;
	bool skip_next = false;
	void *old_fn, *new_fn;
	size_t old_code_size;
	unsigned int i, j;
	u8 old_flags;
	u32 offset;

	fully_tagged = lightrec_block_is_fully_tagged(block);
	if (fully_tagged)
		block_set_flags(block, BLOCK_FULLY_TAGGED);

	_jit = jit_new_state();
	if (!_jit)
		return -ENOMEM;

	oldjit = block->_jit;
	old_fn = block->function;
	old_code_size = block->code_size;
	block->_jit = _jit;

	lightrec_regcache_reset(cstate->reg_cache);

	if (OPT_PRELOAD_PC && (block->flags & BLOCK_PRELOAD_PC))
		lightrec_preload_pc(cstate->reg_cache, _jit);

	if (!arch_has_fast_mask())
		lightrec_preload_imm(cstate->reg_cache, _jit, JIT_R1, 0x1fffffff);

	cstate->cycles = 0;
	cstate->nb_local_branches = 0;
	cstate->nb_targets = 0;
	cstate->no_load_delay = false;

	jit_prolog();
	jit_tramp(256);

	start_of_block = jit_label();

	for (i = 0; i < block->nb_ops; i++) {
		elm = &block->opcode_list[i];

		if (skip_next) {
			skip_next = false;
			continue;
		}

		if (should_emulate(elm)) {
			pr_debug("Branch at offset 0x%x will be emulated\n",
				 i << 2);

			lightrec_emit_jump_to_interpreter(cstate, block, i);
			skip_next = !op_flag_no_ds(elm->flags);
		} else {
			lightrec_rec_opcode(cstate, block, i);
			skip_next = !op_flag_no_ds(elm->flags) && has_delay_slot(elm->c);
#if _WIN32
			/* FIXME: GNU Lightning on Windows seems to use our
			 * mapped registers as temporaries. Until the actual bug
			 * is found and fixed, unconditionally mark our
			 * registers as live here. */
			lightrec_regcache_mark_live(cstate->reg_cache, _jit);
#endif
		}

		cstate->cycles += lightrec_cycles_of_opcode(state, elm->c);
	}

	for (i = 0; i < cstate->nb_local_branches; i++) {
		struct lightrec_branch *branch = &cstate->local_branches[i];

		pr_debug("Patch local branch to offset 0x%"PRIx32"\n",
			 branch->target << 2);

		if (branch->target == 0) {
			jit_patch_at(branch->branch, start_of_block);
			continue;
		}

		for (j = 0; j < cstate->nb_targets; j++) {
			if (cstate->targets[j].offset == branch->target) {
				jit_patch_at(branch->branch,
					     cstate->targets[j].label);
				break;
			}
		}

		if (j == cstate->nb_targets)
			pr_err("Unable to find branch target\n");
	}

	jit_ret();
	jit_epilog();

	new_fn = lightrec_emit_code(state, block, _jit, &block->code_size);
	if (!new_fn) {
		if (!ENABLE_THREADED_COMPILER)
			pr_err("Unable to compile block!\n");
		block->_jit = oldjit;
		jit_clear_state();
		_jit_destroy_state(_jit);
		return -ENOMEM;
	}

	/* Pause the reaper, because lightrec_reset_lut_offset() may try to set
	 * the old block->function pointer to the code LUT. */
	if (ENABLE_THREADED_COMPILER)
		lightrec_reaper_pause(state->reaper);

	block->function = new_fn;
	block_clear_flags(block, BLOCK_SHOULD_RECOMPILE);

	/* Add compiled function to the LUT */
	lut_write(state, lut_offset(block->pc), block->function);

	/* Detect old blocks that have been covered by the new one */
	for (i = 0; ENABLE_THREADED_COMPILER && i < cstate->nb_targets; i++) {
		target = &cstate->targets[i];

		if (!target->offset)
			continue;

		offset = block->pc + target->offset * sizeof(u32);

		block2 = lightrec_find_block(state->block_cache, offset);
		if (block2) {
			/* No need to check if block2 is compilable - it must
			 * be, otherwise block wouldn't be compilable either */

			/* Set the "block dead" flag to prevent the dynarec from
			 * recompiling this block */
			old_flags = block_set_flags(block2, BLOCK_IS_DEAD);

			if (old_flags & BLOCK_IS_DEAD)
				was_dead[i / 32] |= BIT(i % 32);
			else
				was_dead[i / 32] &= ~BIT(i % 32);
		}

		dead_blocks[i] = block2;

		/* If block2 was pending for compilation, cancel it.
		 * If it's being compiled right now, wait until it finishes. */
		if (block2)
			lightrec_recompiler_remove(state->rec, block2);
	}

	for (i = 0; i < cstate->nb_targets; i++) {
		target = &cstate->targets[i];

		if (!target->offset)
			continue;

		/* We know from now on that block2 (if present) isn't going to
		 * be compiled. We can override the LUT entry with our new
		 * block's entry point. */
		offset = lut_offset(block->pc) + target->offset;
		lut_write(state, offset, jit_address(target->label));

		if (ENABLE_THREADED_COMPILER) {
			block2 = dead_blocks[i];
		} else {
			offset = block->pc + target->offset * sizeof(u32);
			block2 = lightrec_find_block(state->block_cache, offset);
		}
		if (block2) {
			pr_debug("Reap block "X32_FMT" as it's covered by block "
				 X32_FMT"\n", block2->pc, block->pc);

			/* Finally, reap the block. */
			if (!ENABLE_THREADED_COMPILER) {
				lightrec_unregister_block(state->block_cache, block2);
				lightrec_free_block(state, block2);
			} else if (!(was_dead[i / 32] & BIT(i % 32))) {
				lightrec_reaper_add(state->reaper,
						    lightrec_reap_block,
						    block2);
			}
		}
	}

	if (ENABLE_THREADED_COMPILER)
		lightrec_reaper_continue(state->reaper);

	if (ENABLE_DISASSEMBLER) {
		pr_debug("Compiling block at "PC_FMT"\n", block->pc);
		jit_disassemble();
	}

	jit_clear_state();

	if (fully_tagged)
		old_flags = block_set_flags(block, BLOCK_NO_OPCODE_LIST);

	if (fully_tagged && !(old_flags & BLOCK_NO_OPCODE_LIST)) {
		pr_debug("Block "PC_FMT" is fully tagged"
			 " - free opcode list\n", block->pc);

		if (ENABLE_THREADED_COMPILER) {
			lightrec_reaper_add(state->reaper,
					    lightrec_reap_opcode_list,
					    block->opcode_list);
		} else {
			lightrec_free_opcode_list(state, block->opcode_list);
		}
	}

	if (oldjit) {
		pr_debug("Block "X32_FMT" recompiled, reaping old jit context.\n",
			 block->pc);

		if (ENABLE_THREADED_COMPILER) {
			lightrec_reaper_add(state->reaper,
					    lightrec_reap_jit, oldjit);
			lightrec_reaper_add(state->reaper,
					    lightrec_reap_function, old_fn);
		} else {
			_jit_destroy_state(oldjit);
			lightrec_free_function(state, old_fn);
		}

		lightrec_unregister(MEM_FOR_CODE, old_code_size);
	}

	pr_debug("Blocks compiled: %u\n", ++state->nb_compile);

	return 0;
}

static void lightrec_print_info(struct lightrec_state *state)
{
	if ((state->current_cycle & ~0xfffffff) != state->old_cycle_counter) {
		pr_info("Lightrec RAM usage: IR %u KiB, CODE %u KiB, "
			"MIPS %u KiB, TOTAL %u KiB, avg. IPI %f\n",
			lightrec_get_mem_usage(MEM_FOR_IR) / 1024,
			lightrec_get_mem_usage(MEM_FOR_CODE) / 1024,
			lightrec_get_mem_usage(MEM_FOR_MIPS_CODE) / 1024,
			lightrec_get_total_mem_usage() / 1024,
		       lightrec_get_average_ipi());
		state->old_cycle_counter = state->current_cycle & ~0xfffffff;
	}
}

u32 lightrec_execute(struct lightrec_state *state, u32 pc, u32 target_cycle)
{
	s32 (*func)(struct lightrec_state *, u32, void *, s32) = (void *)state->dispatcher->function;
	void *block_trace;
	s32 cycles_delta;

	state->exit_flags = LIGHTREC_EXIT_NORMAL;

	/* Handle the cycle counter overflowing */
	if (unlikely(target_cycle < state->current_cycle))
		target_cycle = UINT_MAX;

	state->target_cycle = target_cycle;
	state->curr_pc = pc;

	block_trace = get_next_block_func(state, pc);
	if (block_trace) {
		cycles_delta = state->target_cycle - state->current_cycle;

		cycles_delta = (*func)(state, state->curr_pc,
				       block_trace, cycles_delta);

		state->current_cycle = state->target_cycle - cycles_delta;
	}

	if (ENABLE_THREADED_COMPILER)
		lightrec_reaper_reap(state->reaper);

	if (LOG_LEVEL >= INFO_L)
		lightrec_print_info(state);

	return state->curr_pc;
}

u32 lightrec_run_interpreter(struct lightrec_state *state, u32 pc,
			     u32 target_cycle)
{
	struct block *block;

	state->exit_flags = LIGHTREC_EXIT_NORMAL;
	state->target_cycle = target_cycle;

	do {
		block = lightrec_get_block(state, pc);
		if (!block)
			break;

		pc = lightrec_emulate_block(state, block, pc);

		if (ENABLE_THREADED_COMPILER)
			lightrec_reaper_reap(state->reaper);
	} while (state->current_cycle < state->target_cycle);

	if (LOG_LEVEL >= INFO_L)
		lightrec_print_info(state);

	return pc;
}

void lightrec_free_block(struct lightrec_state *state, struct block *block)
{
	u8 old_flags;

	lightrec_unregister(MEM_FOR_MIPS_CODE, block->nb_ops * sizeof(u32));
	old_flags = block_set_flags(block, BLOCK_NO_OPCODE_LIST);

	if (!(old_flags & BLOCK_NO_OPCODE_LIST))
		lightrec_free_opcode_list(state, block->opcode_list);
	if (block->_jit)
		_jit_destroy_state(block->_jit);
	if (block->function) {
		lightrec_free_function(state, block->function);
		lightrec_unregister(MEM_FOR_CODE, block->code_size);
	}
	lightrec_free(state, MEM_FOR_IR, sizeof(*block), block);
}

struct lightrec_cstate * lightrec_create_cstate(struct lightrec_state *state)
{
	struct lightrec_cstate *cstate;

	cstate = lightrec_malloc(state, MEM_FOR_LIGHTREC, sizeof(*cstate));
	if (!cstate)
		return NULL;

	cstate->reg_cache = lightrec_regcache_init(state);
	if (!cstate->reg_cache) {
		lightrec_free(state, MEM_FOR_LIGHTREC, sizeof(*cstate), cstate);
		return NULL;
	}

	cstate->state = state;

	return cstate;
}

void lightrec_free_cstate(struct lightrec_cstate *cstate)
{
	lightrec_free_regcache(cstate->reg_cache);
	lightrec_free(cstate->state, MEM_FOR_LIGHTREC, sizeof(*cstate), cstate);
}

struct lightrec_state * lightrec_init(char *argv0,
				      const struct lightrec_mem_map *maps,
				      size_t nb,
				      const struct lightrec_ops *ops)
{
	const struct lightrec_mem_map *codebuf_map = &maps[PSX_MAP_CODE_BUFFER];
	const struct lightrec_mem_map *map;
	struct lightrec_state *state;
	uintptr_t addr;
	void *tlsf = NULL;
	bool with_32bit_lut = false;
	size_t lut_size;

	/* Sanity-check ops */
	if (!ops || !ops->cop2_op || !ops->enable_ram) {
		pr_err("Missing callbacks in lightrec_ops structure\n");
		return NULL;
	}

	if (ops->cop2_notify)
		pr_debug("Optional cop2_notify callback in lightrec_ops\n");
	else
		pr_debug("No optional cop2_notify callback in lightrec_ops\n");

	if (ENABLE_CODE_BUFFER && nb > PSX_MAP_CODE_BUFFER
	    && codebuf_map->address) {
		tlsf = tlsf_create_with_pool(codebuf_map->address,
					     codebuf_map->length);
		if (!tlsf) {
			pr_err("Unable to initialize code buffer\n");
			return NULL;
		}

		if (__WORDSIZE == 64) {
			addr = (uintptr_t) codebuf_map->address + codebuf_map->length - 1;
			with_32bit_lut = addr == (u32) addr;
		}
	}

	if (with_32bit_lut)
		lut_size = CODE_LUT_SIZE * 4;
	else
		lut_size = CODE_LUT_SIZE * sizeof(void *);

	init_jit_with_debug(argv0, stdout);

	state = calloc(1, sizeof(*state) + lut_size);
	if (!state)
		goto err_finish_jit;

	lightrec_register(MEM_FOR_LIGHTREC, sizeof(*state) + lut_size);

	state->tlsf = tlsf;
	state->with_32bit_lut = with_32bit_lut;
	state->in_delay_slot_n = 0xff;
	state->cycles_per_op = 2;

	state->block_cache = lightrec_blockcache_init(state);
	if (!state->block_cache)
		goto err_free_state;

	if (ENABLE_THREADED_COMPILER) {
		state->rec = lightrec_recompiler_init(state);
		if (!state->rec)
			goto err_free_block_cache;

		state->reaper = lightrec_reaper_init(state);
		if (!state->reaper)
			goto err_free_recompiler;
	} else {
		state->cstate = lightrec_create_cstate(state);
		if (!state->cstate)
			goto err_free_block_cache;
	}

	state->nb_maps = nb;
	state->maps = maps;

	memcpy(&state->ops, ops, sizeof(*ops));

	state->dispatcher = generate_dispatcher(state);
	if (!state->dispatcher)
		goto err_free_reaper;

	state->c_wrapper_block = generate_wrapper(state);
	if (!state->c_wrapper_block)
		goto err_free_dispatcher;

	state->c_wrappers[C_WRAPPER_RW] = lightrec_rw_cb;
	state->c_wrappers[C_WRAPPER_RW_GENERIC] = lightrec_rw_generic_cb;
	state->c_wrappers[C_WRAPPER_MFC] = lightrec_mfc_cb;
	state->c_wrappers[C_WRAPPER_MTC] = lightrec_mtc_cb;
	state->c_wrappers[C_WRAPPER_CP] = lightrec_cp_cb;

	map = &maps[PSX_MAP_BIOS];
	state->offset_bios = (uintptr_t)map->address - map->pc;

	map = &maps[PSX_MAP_SCRATCH_PAD];
	state->offset_scratch = (uintptr_t)map->address - map->pc;

	map = &maps[PSX_MAP_HW_REGISTERS];
	state->offset_io = (uintptr_t)map->address - map->pc;

	map = &maps[PSX_MAP_KERNEL_USER_RAM];
	state->offset_ram = (uintptr_t)map->address - map->pc;

	if (maps[PSX_MAP_MIRROR1].address == map->address + 0x200000 &&
	    maps[PSX_MAP_MIRROR2].address == map->address + 0x400000 &&
	    maps[PSX_MAP_MIRROR3].address == map->address + 0x600000)
		state->mirrors_mapped = true;

	if (state->offset_bios == 0 &&
	    state->offset_scratch == 0 &&
	    state->offset_ram == 0 &&
	    state->offset_io == 0 &&
	    state->mirrors_mapped) {
		pr_info("Memory map is perfect. Emitted code will be best.\n");
	} else {
		pr_info("Memory map is sub-par. Emitted code will be slow.\n");
	}

	if (state->with_32bit_lut)
		pr_info("Using 32-bit LUT\n");

	return state;

err_free_dispatcher:
	lightrec_free_block(state, state->dispatcher);
err_free_reaper:
	if (ENABLE_THREADED_COMPILER)
		lightrec_reaper_destroy(state->reaper);
err_free_recompiler:
	if (ENABLE_THREADED_COMPILER)
		lightrec_free_recompiler(state->rec);
	else
		lightrec_free_cstate(state->cstate);
err_free_block_cache:
	lightrec_free_block_cache(state->block_cache);
err_free_state:
	lightrec_unregister(MEM_FOR_LIGHTREC, sizeof(*state) +
			    lut_elm_size(state) * CODE_LUT_SIZE);
	free(state);
err_finish_jit:
	finish_jit();
	if (ENABLE_CODE_BUFFER && tlsf)
		tlsf_destroy(tlsf);
	return NULL;
}

void lightrec_destroy(struct lightrec_state *state)
{
	/* Force a print info on destroy*/
	state->current_cycle = ~state->current_cycle;
	lightrec_print_info(state);

	lightrec_free_block_cache(state->block_cache);
	lightrec_free_block(state, state->dispatcher);
	lightrec_free_block(state, state->c_wrapper_block);

	if (ENABLE_THREADED_COMPILER) {
		lightrec_free_recompiler(state->rec);
		lightrec_reaper_destroy(state->reaper);
	} else {
		lightrec_free_cstate(state->cstate);
	}

	finish_jit();
	if (ENABLE_CODE_BUFFER && state->tlsf)
		tlsf_destroy(state->tlsf);

	lightrec_unregister(MEM_FOR_LIGHTREC, sizeof(*state) +
			    lut_elm_size(state) * CODE_LUT_SIZE);
	free(state);
}

void lightrec_invalidate(struct lightrec_state *state, u32 addr, u32 len)
{
	u32 kaddr = kunseg(addr & ~0x3);
	enum psx_map idx = lightrec_get_map_idx(state, kaddr);

	switch (idx) {
	case PSX_MAP_MIRROR1:
	case PSX_MAP_MIRROR2:
	case PSX_MAP_MIRROR3:
		/* Handle mirrors */
		kaddr &= RAM_SIZE - 1;
		fallthrough;
	case PSX_MAP_KERNEL_USER_RAM:
		break;
	default:
		return;
	}

	memset(lut_address(state, lut_offset(kaddr)), 0,
	       ((len + 3) / 4) * lut_elm_size(state));
}

void lightrec_invalidate_all(struct lightrec_state *state)
{
	memset(state->code_lut, 0, lut_elm_size(state) * CODE_LUT_SIZE);
}

void lightrec_set_unsafe_opt_flags(struct lightrec_state *state, u32 flags)
{
	if ((flags ^ state->opt_flags) & LIGHTREC_OPT_INV_DMA_ONLY)
		lightrec_invalidate_all(state);

	state->opt_flags = flags;
}

void lightrec_set_exit_flags(struct lightrec_state *state, u32 flags)
{
	if (flags != LIGHTREC_EXIT_NORMAL) {
		state->exit_flags |= flags;
		state->target_cycle = state->current_cycle;
	}
}

u32 lightrec_exit_flags(struct lightrec_state *state)
{
	return state->exit_flags;
}

u32 lightrec_current_cycle_count(const struct lightrec_state *state)
{
	return state->current_cycle;
}

void lightrec_reset_cycle_count(struct lightrec_state *state, u32 cycles)
{
	state->current_cycle = cycles;

	if (state->target_cycle < cycles)
		state->target_cycle = cycles;
}

void lightrec_set_target_cycle_count(struct lightrec_state *state, u32 cycles)
{
	if (state->exit_flags == LIGHTREC_EXIT_NORMAL) {
		if (cycles < state->current_cycle)
			cycles = state->current_cycle;

		state->target_cycle = cycles;
	}
}

struct lightrec_registers * lightrec_get_registers(struct lightrec_state *state)
{
	return &state->regs;
}

void lightrec_set_cycles_per_opcode(struct lightrec_state *state, u32 cycles)
{
	if (state->cycles_per_op == cycles)
		return;

	state->cycles_per_op = cycles;

	if (ENABLE_THREADED_COMPILER) {
		lightrec_recompiler_pause(state->rec);
		lightrec_reaper_reap(state->reaper);
	}

	lightrec_invalidate_all(state);
	lightrec_free_all_blocks(state->block_cache);

	if (ENABLE_THREADED_COMPILER)
		lightrec_recompiler_unpause(state->rec);
}
