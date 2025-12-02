/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_INTERPRETER_H__
#define __LIGHTREC_INTERPRETER_H__

#include "lightrec.h"

struct block;

u32 lightrec_emulate_block(struct lightrec_state *state, struct block *block, u32 pc);
u32 lightrec_handle_load_delay(struct lightrec_state *state,
			       struct block *block, u32 pc, u32 reg);

#endif /* __LIGHTREC_INTERPRETER_H__ */
