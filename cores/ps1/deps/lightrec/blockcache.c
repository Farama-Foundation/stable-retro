// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2015-2021 Paul Cercueil <paul@crapouillou.net>
 */

#include "blockcache.h"
#include "debug.h"
#include "lightrec-private.h"
#include "memmanager.h"
#include "reaper.h"
#include "recompiler.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Must be power of two */
#define LUT_SIZE 0x4000

struct blockcache {
	struct lightrec_state *state;
	struct block * lut[LUT_SIZE];
};

u16 lightrec_get_lut_entry(const struct block *block)
{
	return (kunseg(block->pc) >> 2) & (LUT_SIZE - 1);
}

struct block * lightrec_find_block(struct blockcache *cache, u32 pc)
{
	struct block *block;

	pc = kunseg(pc);

	for (block = cache->lut[(pc >> 2) & (LUT_SIZE - 1)];
	     block; block = block->next)
		if (kunseg(block->pc) == pc)
			return block;

	return NULL;
}

struct block * lightrec_find_block_from_lut(struct blockcache *cache,
					    u16 lut_entry, u32 addr_in_block)
{
	struct block *block;
	u32 pc;

	addr_in_block = kunseg(addr_in_block);

	for (block = cache->lut[lut_entry]; block; block = block->next) {
		pc = kunseg(block->pc);
		if (addr_in_block >= pc &&
		    addr_in_block < pc + (block->nb_ops << 2))
			return block;
	}

	return NULL;
}

void remove_from_code_lut(struct blockcache *cache, struct block *block)
{
	struct lightrec_state *state = cache->state;
	u32 offset = lut_offset(block->pc);

	if (block->function) {
		memset(lut_address(state, offset), 0,
		       block->nb_ops * lut_elm_size(state));
	}
}

void lightrec_register_block(struct blockcache *cache, struct block *block)
{
	u32 pc = kunseg(block->pc);
	struct block *old;

	old = cache->lut[(pc >> 2) & (LUT_SIZE - 1)];
	if (old)
		block->next = old;

	cache->lut[(pc >> 2) & (LUT_SIZE - 1)] = block;

	remove_from_code_lut(cache, block);
}

void lightrec_unregister_block(struct blockcache *cache, struct block *block)
{
	u32 pc = kunseg(block->pc);
	struct block *old = cache->lut[(pc >> 2) & (LUT_SIZE - 1)];

	if (old == block) {
		cache->lut[(pc >> 2) & (LUT_SIZE - 1)] = old->next;
		return;
	}

	for (; old; old = old->next) {
		if (old->next == block) {
			old->next = block->next;
			return;
		}
	}

	pr_err("Block at "PC_FMT" is not in cache\n", block->pc);
}

static bool lightrec_block_is_old(const struct lightrec_state *state,
				  const struct block *block)
{
	u32 diff = state->current_cycle - block->precompile_date;

	return diff > (1 << 27); /* About 4 seconds */
}

static void lightrec_free_blocks(struct blockcache *cache,
				 const struct block *except, bool all)
{
	struct lightrec_state *state = cache->state;
	struct block *block, *next;
	bool outdated = all;
	unsigned int i;
	u8 old_flags;

	for (i = 0; i < LUT_SIZE; i++) {
		for (block = cache->lut[i]; block; block = next) {
			next = block->next;

			if (except && block == except)
				continue;

			if (!all) {
				outdated = lightrec_block_is_old(state, block) ||
					lightrec_block_is_outdated(state, block);
			}

			if (!outdated)
				continue;

			old_flags = block_set_flags(block, BLOCK_IS_DEAD);

			if (!(old_flags & BLOCK_IS_DEAD)) {
				if (ENABLE_THREADED_COMPILER)
					lightrec_recompiler_remove(state->rec, block);

				pr_debug("Freeing outdated block at "PC_FMT"\n", block->pc);
				remove_from_code_lut(cache, block);
				lightrec_unregister_block(cache, block);
				lightrec_free_block(state, block);
			}
		}
	}
}

void lightrec_remove_outdated_blocks(struct blockcache *cache,
				     const struct block *except)
{
	pr_info("Running out of code space. Cleaning block cache...\n");

	lightrec_free_blocks(cache, except, false);
}

void lightrec_free_all_blocks(struct blockcache *cache)
{
	lightrec_free_blocks(cache, NULL, true);
}

void lightrec_free_block_cache(struct blockcache *cache)
{
	lightrec_free_all_blocks(cache);
	lightrec_free(cache->state, MEM_FOR_LIGHTREC, sizeof(*cache), cache);
}

struct blockcache * lightrec_blockcache_init(struct lightrec_state *state)
{
	struct blockcache *cache;

	cache = lightrec_calloc(state, MEM_FOR_LIGHTREC, sizeof(*cache));
	if (!cache)
		return NULL;

	cache->state = state;

	return cache;
}

u32 lightrec_calculate_block_hash(const struct block *block)
{
	const u32 *code = block->code;
	u32 hash = 0xffffffff;
	unsigned int i;

	/* Jenkins one-at-a-time hash algorithm */
	for (i = 0; i < block->nb_ops; i++) {
		hash += *code++;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

static void lightrec_reset_lut_offset(struct lightrec_state *state, void *d)
{
	u32 pc = (u32)(uintptr_t) d;
	struct block *block;
	void *addr;

	block = lightrec_find_block(state->block_cache, pc);
	if (!block)
		return;

	if (block_has_flag(block, BLOCK_IS_DEAD))
		return;

	addr = block->function ?: state->get_next_block;
	lut_write(state, lut_offset(pc), addr);
}

bool lightrec_block_is_outdated(struct lightrec_state *state, struct block *block)
{
	u32 offset = lut_offset(block->pc);
	bool outdated;

	if (lut_read(state, offset))
		return false;

	outdated = block->hash != lightrec_calculate_block_hash(block);
	if (likely(!outdated)) {
		/* The block was marked as outdated, but the content is still
		 * the same */

		if (ENABLE_THREADED_COMPILER) {
			/*
			 * When compiling a block that covers ours, the threaded
			 * compiler will set the LUT entries of the various
			 * entry points. Therefore we cannot write the LUT here,
			 * as we would risk overwriting the new entry points.
			 * Leave it to the reaper to re-install the LUT entries.
			 */

			lightrec_reaper_add(state->reaper,
					    lightrec_reset_lut_offset,
					    (void *)(uintptr_t) block->pc);
		} else if (block->function) {
			lut_write(state, offset, block->function);
		} else {
			lut_write(state, offset, state->get_next_block);
		}
	}

	return outdated;
}
