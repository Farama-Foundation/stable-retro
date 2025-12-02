// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2019-2021 Paul Cercueil <paul@crapouillou.net>
 */

#include "blockcache.h"
#include "debug.h"
#include "interpreter.h"
#include "lightrec-private.h"
#include "memmanager.h"
#include "reaper.h"
#include "slist.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef __linux__
#include <unistd.h>
#endif

struct block_rec {
	struct block *block;
	struct slist_elm slist;
	unsigned int requests;
	bool compiling;
};

struct recompiler_thd {
	struct lightrec_cstate *cstate;
	unsigned int tid;
	pthread_t thd;
};

struct recompiler {
	struct lightrec_state *state;
	pthread_cond_t cond;
	pthread_cond_t cond2;
	pthread_mutex_t mutex;
	bool stop, pause, must_flush;
	struct slist_elm slist;

	pthread_mutex_t alloc_mutex;

	unsigned int nb_recs, nb_cpus;
	struct recompiler_thd thds[];
};

static unsigned int get_processors_count(void)
{
	int nb = 1;

#if defined(PTW32_VERSION)
        nb = pthread_num_processors_np();
#elif defined(__APPLE__) || defined(__FreeBSD__)
        int count;
        size_t size = sizeof(count);

        nb = sysctlbyname("hw.ncpu", &count, &size, NULL, 0) ? 1 : count;
#elif defined(_SC_NPROCESSORS_ONLN)
	nb = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif

	return nb < 1 ? 1 : nb;
}

static struct block_rec * lightrec_get_best_elm(struct slist_elm *head)
{
	struct block_rec *block_rec, *best = NULL;
	struct slist_elm *elm;

	for (elm = slist_first(head); elm; elm = elm->next) {
		block_rec = container_of(elm, struct block_rec, slist);

		if (!block_rec->compiling
		    && (!best || block_rec->requests > best->requests))
			best = block_rec;
	}

	return best;
}

static bool lightrec_cancel_block_rec(struct recompiler *rec,
				      struct block_rec *block_rec)
{
	if (block_rec->compiling) {
		/* Block is being recompiled - wait for
		 * completion */
		pthread_cond_wait(&rec->cond2, &rec->mutex);

		/* We can't guarantee the signal was for us.
		 * Since block_rec may have been removed while
		 * we were waiting on the condition, we cannot
		 * check block_rec->compiling again. The best
		 * thing is just to restart the function. */
		return false;
	}

	/* Block is not yet being processed - remove it from the list */
	slist_remove(&rec->slist, &block_rec->slist);
	lightrec_free(rec->state, MEM_FOR_LIGHTREC,
		      sizeof(*block_rec), block_rec);

	return true;
}

static void lightrec_cancel_list(struct recompiler *rec)
{
	struct block_rec *block_rec;
	struct slist_elm *elm, *head = &rec->slist;

	for (elm = slist_first(head); elm; elm = slist_first(head)) {
		block_rec = container_of(elm, struct block_rec, slist);
		lightrec_cancel_block_rec(rec, block_rec);
	}
}

static void lightrec_flush_code_buffer(struct lightrec_state *state, void *d)
{
	struct recompiler *rec = d;

	lightrec_remove_outdated_blocks(state->block_cache, NULL);
	rec->must_flush = false;
}

static void lightrec_compile_list(struct recompiler *rec,
				  struct recompiler_thd *thd)
{
	struct block_rec *block_rec;
	struct block *block;
	int ret;

	while (!rec->pause &&
	       !!(block_rec = lightrec_get_best_elm(&rec->slist))) {
		block_rec->compiling = true;
		block = block_rec->block;

		pthread_mutex_unlock(&rec->mutex);

		if (likely(!block_has_flag(block, BLOCK_IS_DEAD))) {
			ret = lightrec_compile_block(thd->cstate, block);
			if (ret == -ENOMEM) {
				/* Code buffer is full. Request the reaper to
				 * flush it. */

				pthread_mutex_lock(&rec->mutex);
				block_rec->compiling = false;
				pthread_cond_broadcast(&rec->cond2);

				if (!rec->must_flush) {
					rec->must_flush = true;
					lightrec_cancel_list(rec);

					lightrec_reaper_add(rec->state->reaper,
							    lightrec_flush_code_buffer,
							    rec);
				}
				return;
			}

			if (ret) {
				pr_err("Unable to compile block at "PC_FMT": %d\n",
				       block->pc, ret);
			}
		}

		pthread_mutex_lock(&rec->mutex);

		slist_remove(&rec->slist, &block_rec->slist);
		lightrec_free(rec->state, MEM_FOR_LIGHTREC,
			      sizeof(*block_rec), block_rec);
		pthread_cond_broadcast(&rec->cond2);
	}
}

static void * lightrec_recompiler_thd(void *d)
{
	struct recompiler_thd *thd = d;
	struct recompiler *rec = container_of(thd, struct recompiler, thds[thd->tid]);

	pthread_mutex_lock(&rec->mutex);

	while (!rec->stop) {
		do {
			pthread_cond_wait(&rec->cond, &rec->mutex);

			if (rec->stop)
				goto out_unlock;

		} while (rec->pause || slist_empty(&rec->slist));

		lightrec_compile_list(rec, thd);
	}

out_unlock:
	pthread_mutex_unlock(&rec->mutex);
	return NULL;
}

struct recompiler *lightrec_recompiler_init(struct lightrec_state *state)
{
	struct recompiler *rec;
	unsigned int i, nb_recs, nb_cpus;
	int ret;

	nb_cpus = get_processors_count();
	nb_recs = nb_cpus < 2 ? 1 : nb_cpus - 1;

	rec = lightrec_malloc(state, MEM_FOR_LIGHTREC, sizeof(*rec)
			      + nb_recs * sizeof(*rec->thds));
	if (!rec) {
		pr_err("Cannot create recompiler: Out of memory\n");
		return NULL;
	}

	for (i = 0; i < nb_recs; i++) {
		rec->thds[i].tid = i;
		rec->thds[i].cstate = NULL;
	}

	for (i = 0; i < nb_recs; i++) {
		rec->thds[i].cstate = lightrec_create_cstate(state);
		if (!rec->thds[i].cstate) {
			pr_err("Cannot create recompiler: Out of memory\n");
			goto err_free_cstates;
		}
	}

	rec->state = state;
	rec->stop = false;
	rec->pause = false;
	rec->must_flush = false;
	rec->nb_recs = nb_recs;
	rec->nb_cpus = nb_cpus;
	slist_init(&rec->slist);

	ret = pthread_cond_init(&rec->cond, NULL);
	if (ret) {
		pr_err("Cannot init cond variable: %d\n", ret);
		goto err_free_cstates;
	}

	ret = pthread_cond_init(&rec->cond2, NULL);
	if (ret) {
		pr_err("Cannot init cond variable: %d\n", ret);
		goto err_cnd_destroy;
	}

	ret = pthread_mutex_init(&rec->alloc_mutex, NULL);
	if (ret) {
		pr_err("Cannot init alloc mutex variable: %d\n", ret);
		goto err_cnd2_destroy;
	}

	ret = pthread_mutex_init(&rec->mutex, NULL);
	if (ret) {
		pr_err("Cannot init mutex variable: %d\n", ret);
		goto err_alloc_mtx_destroy;
	}

	for (i = 0; i < nb_recs; i++) {
		ret = pthread_create(&rec->thds[i].thd, NULL,
				     lightrec_recompiler_thd, &rec->thds[i]);
		if (ret) {
			pr_err("Cannot create recompiler thread: %d\n", ret);
			/* TODO: Handle cleanup properly */
			goto err_mtx_destroy;
		}
	}

	pr_info("Threaded recompiler started with %u workers.\n", nb_recs);

	return rec;

err_mtx_destroy:
	pthread_mutex_destroy(&rec->mutex);
err_alloc_mtx_destroy:
	pthread_mutex_destroy(&rec->alloc_mutex);
err_cnd2_destroy:
	pthread_cond_destroy(&rec->cond2);
err_cnd_destroy:
	pthread_cond_destroy(&rec->cond);
err_free_cstates:
	for (i = 0; i < nb_recs; i++) {
		if (rec->thds[i].cstate)
			lightrec_free_cstate(rec->thds[i].cstate);
	}
	lightrec_free(state, MEM_FOR_LIGHTREC, sizeof(*rec), rec);
	return NULL;
}

void lightrec_free_recompiler(struct recompiler *rec)
{
	unsigned int i;

	rec->stop = true;

	/* Stop the thread */
	pthread_mutex_lock(&rec->mutex);
	pthread_cond_broadcast(&rec->cond);
	lightrec_cancel_list(rec);
	pthread_mutex_unlock(&rec->mutex);

	for (i = 0; i < rec->nb_recs; i++)
		pthread_join(rec->thds[i].thd, NULL);

	for (i = 0; i < rec->nb_recs; i++)
		lightrec_free_cstate(rec->thds[i].cstate);

	pthread_mutex_destroy(&rec->mutex);
	pthread_mutex_destroy(&rec->alloc_mutex);
	pthread_cond_destroy(&rec->cond);
	pthread_cond_destroy(&rec->cond2);
	lightrec_free(rec->state, MEM_FOR_LIGHTREC, sizeof(*rec), rec);
}

int lightrec_recompiler_add(struct recompiler *rec, struct block *block)
{
	struct slist_elm *elm;
	struct block_rec *block_rec;
	u32 pc1, pc2;
	int ret = 0;

	pthread_mutex_lock(&rec->mutex);

	/* If the recompiler must flush the code cache, we can't add the new
	 * job. It will be re-added next time the block's address is jumped to
	 * again. */
	if (rec->must_flush)
		goto out_unlock;

	/* If the block is marked as dead, don't compile it, it will be removed
	 * as soon as it's safe. */
	if (block_has_flag(block, BLOCK_IS_DEAD))
		goto out_unlock;

	for (elm = slist_first(&rec->slist); elm; elm = elm->next) {
		block_rec = container_of(elm, struct block_rec, slist);

		if (block_rec->block == block) {
			/* The block to compile is already in the queue -
			 * increment its counter to increase its priority */
			block_rec->requests++;

			if (rec->nb_cpus == 1) {
				/* On single-core CPUs, if we got a request for
				 * a block that's already in the queue, we'll
				 * probably get many more before the compiler
				 * thread can run, which means that the block
				 * will be interpreted until then, wasting a lot
				 * of performance. In that case, it is better to
				 * just let the compiler thread run now. */
				pthread_cond_wait(&rec->cond2, &rec->mutex);
			}
			goto out_unlock;
		}

		pc1 = kunseg(block_rec->block->pc);
		pc2 = kunseg(block->pc);
		if (pc2 >= pc1 && pc2 < pc1 + block_rec->block->nb_ops * 4) {
			/* The block we want to compile is already covered by
			 * another one in the queue - increment its counter to
			 * increase its priority */
			block_rec->requests++;
			goto out_unlock;
		}
	}

	/* By the time this function was called, the block has been recompiled
	 * and ins't in the wait list anymore. Just return here. */
	if (block->function && !block_has_flag(block, BLOCK_SHOULD_RECOMPILE))
		goto out_unlock;

	block_rec = lightrec_malloc(rec->state, MEM_FOR_LIGHTREC,
				    sizeof(*block_rec));
	if (!block_rec) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	pr_debug("Adding block "PC_FMT" to recompiler\n", block->pc);

	block_rec->block = block;
	block_rec->compiling = false;
	block_rec->requests = 1;

	elm = &rec->slist;

	/* Push the new entry to the front of the queue */
	slist_append(elm, &block_rec->slist);

	/* Signal the thread */
	pthread_cond_signal(&rec->cond);

out_unlock:
	pthread_mutex_unlock(&rec->mutex);

	return ret;
}

void lightrec_recompiler_remove(struct recompiler *rec, struct block *block)
{
	struct block_rec *block_rec;
	struct slist_elm *elm;

	pthread_mutex_lock(&rec->mutex);

	while (true) {
		for (elm = slist_first(&rec->slist); elm; elm = elm->next) {
			block_rec = container_of(elm, struct block_rec, slist);

			if (block_rec->block == block) {
				if (lightrec_cancel_block_rec(rec, block_rec))
					goto out_unlock;

				break;
			}
		}

		if (!elm)
			break;
	}

out_unlock:
	pthread_mutex_unlock(&rec->mutex);
}

void * lightrec_recompiler_run_first_pass(struct lightrec_state *state,
					  struct block *block, u32 *pc)
{
	u8 old_flags;

	/* There's no point in running the first pass if the block will never
	 * be compiled. Let the main loop run the interpreter instead. */
	if (block_has_flag(block, BLOCK_NEVER_COMPILE))
		return NULL;

	/* The block is marked as dead, and will be removed the next time the
	 * reaper is run. In the meantime, the old function can still be
	 * executed. */
	if (block_has_flag(block, BLOCK_IS_DEAD))
		return block->function;

	/* If the block is already fully tagged, there is no point in running
	 * the first pass. Request a recompilation of the block, and maybe the
	 * interpreter will run the block in the meantime. */
	if (block_has_flag(block, BLOCK_FULLY_TAGGED))
		lightrec_recompiler_add(state->rec, block);

	if (likely(block->function)) {
		if (block_has_flag(block, BLOCK_FULLY_TAGGED)) {
			old_flags = block_set_flags(block, BLOCK_NO_OPCODE_LIST);

			if (!(old_flags & BLOCK_NO_OPCODE_LIST)) {
				pr_debug("Block "PC_FMT" is fully tagged"
					 " - free opcode list\n", block->pc);

				/* The block was already compiled but the opcode list
				 * didn't get freed yet - do it now */
				lightrec_free_opcode_list(state, block->opcode_list);
			}
		}

		return block->function;
	}

	/* Mark the opcode list as freed, so that the threaded compiler won't
	 * free it while we're using it in the interpreter. */
	old_flags = block_set_flags(block, BLOCK_NO_OPCODE_LIST);

	/* Block wasn't compiled yet - run the interpreter */
	*pc = lightrec_emulate_block(state, block, *pc);

	if (!(old_flags & BLOCK_NO_OPCODE_LIST))
		block_clear_flags(block, BLOCK_NO_OPCODE_LIST);

	/* The block got compiled while the interpreter was running.
	 * We can free the opcode list now. */
	if (block->function && block_has_flag(block, BLOCK_FULLY_TAGGED)) {
		old_flags = block_set_flags(block, BLOCK_NO_OPCODE_LIST);

		if (!(old_flags & BLOCK_NO_OPCODE_LIST)) {
			pr_debug("Block "PC_FMT" is fully tagged"
				 " - free opcode list\n", block->pc);

			lightrec_free_opcode_list(state, block->opcode_list);
		}
	}

	return NULL;
}

void lightrec_code_alloc_lock(struct lightrec_state *state)
{
	pthread_mutex_lock(&state->rec->alloc_mutex);
}

void lightrec_code_alloc_unlock(struct lightrec_state *state)
{
	pthread_mutex_unlock(&state->rec->alloc_mutex);
}

void lightrec_recompiler_pause(struct recompiler *rec)
{
	rec->pause = true;

	pthread_mutex_lock(&rec->mutex);
	pthread_cond_broadcast(&rec->cond);
	lightrec_cancel_list(rec);
	pthread_mutex_unlock(&rec->mutex);
}

void lightrec_recompiler_unpause(struct recompiler *rec)
{
	rec->pause = false;
}
