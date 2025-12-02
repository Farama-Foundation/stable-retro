/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __EMITTER_H__
#define __EMITTER_H__

#include "lightrec.h"

struct block;
struct lightrec_cstate;
struct opcode;

void lightrec_rec_opcode(struct lightrec_cstate *state, const struct block *block, u16 offset);
void lightrec_emit_jump_to_interpreter(struct lightrec_cstate *state,
				       const struct block *block, u16 offset);

#endif /* __EMITTER_H__ */
