/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __REGCACHE_H__
#define __REGCACHE_H__

#include "lightning-wrapper.h"
#include "lightrec-config.h"

#if defined(__sh__) && OPT_SH4_USE_GBR
#  define NUM_REGS JIT_V_NUM
#  define LIGHTREC_REG_STATE _GBR
#else
#  define NUM_REGS (JIT_V_NUM - 1)
#  define LIGHTREC_REG_STATE (JIT_V(JIT_V_NUM - 1))
#endif

#if defined(__powerpc__)
#  define NUM_TEMPS JIT_R_NUM
/* JIT_R0 is callee-saved on PowerPC, we have to use something else */
#  define LIGHTREC_REG_CYCLE _R10
#  define FIRST_TEMP 0
#else
#  define NUM_TEMPS (JIT_R_NUM - 1)
#  define LIGHTREC_REG_CYCLE JIT_R0
#  define FIRST_TEMP 1
#endif

#include "lightrec-private.h"

#define FIRST_REG 0

/* Flags for lightrec_alloc_reg_in / lightrec_alloc_reg_out. */
#define REG_EXT		BIT(0) /* register is sign-extended */
#define REG_ZEXT	BIT(1) /* register is zero-extended */

struct register_value {
	_Bool known;
	u32 value;
};

struct native_register;
struct regcache;

u8 lightrec_alloc_reg(struct regcache *cache, jit_state_t *_jit, u8 jit_reg);
u8 lightrec_alloc_reg_temp(struct regcache *cache, jit_state_t *_jit);
u8 lightrec_alloc_reg_out(struct regcache *cache, jit_state_t *_jit,
			  u16 reg, u8 flags);
u8 lightrec_alloc_reg_in(struct regcache *cache, jit_state_t *_jit,
			 u16 reg, u8 flags);

void lightrec_remap_reg(struct regcache *cache, jit_state_t *_jit,
			u8 jit_reg, u16 reg_out, _Bool discard);

void lightrec_load_imm(struct regcache *cache,
		       jit_state_t *_jit, u8 jit_reg, u32 pc, u32 imm);
void lightrec_load_next_pc(struct regcache *cache, jit_state_t *_jit, u8 reg);
void lightrec_load_next_pc_imm(struct regcache *cache,
			       jit_state_t *_jit, u32 pc, u32 imm);

s8 lightrec_get_reg_with_value(struct regcache *cache, intptr_t value);
void lightrec_temp_set_value(struct regcache *cache, u8 jit_reg, intptr_t value);
u8 lightrec_alloc_reg_temp_with_value(struct regcache *cache,
				      jit_state_t *_jit, intptr_t value);

u8 lightrec_get_reg_in_flags(struct regcache *cache, u8 jit_reg);
void lightrec_set_reg_out_flags(struct regcache *cache, u8 jit_reg, u8 flags);

void lightrec_regcache_reset(struct regcache *cache);
void lightrec_preload_pc(struct regcache *cache, jit_state_t *_jit);
void lightrec_preload_imm(struct regcache *cache, jit_state_t *_jit,
			  u8 jit_reg, u32 imm);

void lightrec_free_reg(struct regcache *cache, u8 jit_reg);
void lightrec_free_regs(struct regcache *cache);
void lightrec_clean_reg(struct regcache *cache, jit_state_t *_jit, u8 jit_reg);
void lightrec_clean_regs(struct regcache *cache, jit_state_t *_jit);
void lightrec_unload_reg(struct regcache *cache, jit_state_t *_jit, u8 jit_reg);
void lightrec_storeback_regs(struct regcache *cache, jit_state_t *_jit);
_Bool lightrec_has_dirty_regs(struct regcache *cache);

_Bool lightrec_reg_is_loaded(struct regcache *cache, u16 reg);
void lightrec_clean_reg_if_loaded(struct regcache *cache, jit_state_t *_jit,
				  u16 reg, _Bool unload);
void lightrec_discard_reg_if_loaded(struct regcache *cache, u16 reg);

u8 lightrec_alloc_reg_in_address(struct regcache *cache,
		jit_state_t *_jit, u16 reg, s16 offset);

struct native_register * lightrec_regcache_enter_branch(struct regcache *cache);
void lightrec_regcache_leave_branch(struct regcache *cache,
			struct native_register *regs);

struct regcache * lightrec_regcache_init(struct lightrec_state *state);
void lightrec_free_regcache(struct regcache *cache);

__cnst const char * lightrec_reg_name(u8 reg);

void lightrec_regcache_mark_live(struct regcache *cache, jit_state_t *_jit);

#endif /* __REGCACHE_H__ */
