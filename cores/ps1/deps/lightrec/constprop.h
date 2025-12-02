/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_CONSTPROP_H__
#define __LIGHTREC_CONSTPROP_H__

#include "lightrec.h"

#define LIGHTREC_CONSTPROP_INITIALIZER { { 0, 0xffffffff, 0 }, }

struct block;

struct constprop_data {
	u32 value;
	u32 known;
	u32 sign;
};

static inline _Bool is_known(const struct constprop_data *v, u8 reg)
{
	return v[reg].known == 0xffffffff;
}

static inline _Bool bits_are_known_zero(const struct constprop_data *v,
					u8 reg, u32 mask)
{
	return !(~v[reg].known & mask) && !(v[reg].value & mask);
}

static inline _Bool is_known_zero(const struct constprop_data *v, u8 reg)
{
	return bits_are_known_zero(v, reg, 0xffffffff);
}

void lightrec_consts_propagate(const struct block *block,
			       unsigned int idx,
			       struct constprop_data *v);

enum psx_map
lightrec_get_constprop_map(const struct lightrec_state *state,
			   const struct constprop_data *v, u8 reg, s16 imm);

#endif /* __LIGHTREC_CONSTPROP_H__ */
