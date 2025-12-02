/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_RECOMPILER_H__
#define __LIGHTREC_RECOMPILER_H__

struct block;
struct lightrec_state;
struct recompiler;

struct recompiler *lightrec_recompiler_init(struct lightrec_state *state);
void lightrec_free_recompiler(struct recompiler *rec);
int lightrec_recompiler_add(struct recompiler *rec, struct block *block);
void lightrec_recompiler_remove(struct recompiler *rec, struct block *block);

void * lightrec_recompiler_run_first_pass(struct lightrec_state *state,
					  struct block *block, u32 *pc);

void lightrec_recompiler_pause(struct recompiler *rec);
void lightrec_recompiler_unpause(struct recompiler *rec);

void lightrec_code_alloc_lock(struct lightrec_state *state);
void lightrec_code_alloc_unlock(struct lightrec_state *state);

#endif /* __LIGHTREC_RECOMPILER_H__ */
