/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __OPTIMIZER_H__
#define __OPTIMIZER_H__

#include "disassembler.h"

struct block;
struct opcode;

__cnst _Bool opcode_reads_register(union code op, u8 reg);
__cnst _Bool opcode_writes_register(union code op, u8 reg);
__cnst u64 opcode_write_mask(union code op);
__cnst _Bool has_delay_slot(union code op);
_Bool is_delay_slot(const struct opcode *list, unsigned int offset);
__cnst _Bool opcode_has_load_delay(union code op);
__cnst _Bool opcode_is_io(union code op);
__cnst _Bool is_unconditional_jump(union code c);
__cnst _Bool is_syscall(union code c);

_Bool should_emulate(const struct opcode *op);

int lightrec_optimize(struct lightrec_state *state, struct block *block);

#endif /* __OPTIMIZER_H__ */
