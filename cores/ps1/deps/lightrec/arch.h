/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_ARCH_H__
#define __LIGHTREC_ARCH_H__

#include <lightning.h>
#include <stdbool.h>

static bool arch_has_fast_mask(void)
{
#if __mips_isa_rev >= 2
	/* On MIPS32 >= r2, we can use extr / ins instructions */
	return true;
#endif
#ifdef __powerpc__
	/* On PowerPC, we can use the RLWINM instruction */
	return true;
#endif
#ifdef __aarch64__
	/* Aarch64 can use the UBFX instruction */
	return true;
#endif
#if defined(__x86__) || defined(__x86_64__)
	/* x86 doesn't have enough registers, using cached values make
	 * little sense. Using jit_andi() will give a better result as it will
	 * use bit-shifts for low/high masks. */
	return true;
#endif

	return false;
}

#endif /* __LIGHTREC_ARCH_H__ */
