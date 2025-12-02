/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTNING_WRAPPER_H__
#define __LIGHTNING_WRAPPER_H__

#include <lightning.h>

#include "lightrec-config.h"

#if __WORDSIZE == 32

#define jit_ldxi_ui(u,v,w)	jit_ldxi_i(u,v,w)
#define jit_stxi_ui(u,v,w)	jit_stxi_i(u,v,w)
#define jit_extr_i(u,v)		jit_movr(u,v)
#define jit_extr_ui(u,v)	jit_movr(u,v)
#define jit_retval_ui(u)	jit_retval(u)
#define jit_getarg_ui(u,v)	jit_getarg_i(u,v)

#endif

#define jit_b()			jit_beqr(0, 0)

#if defined(__sh__) && OPT_SH4_USE_GBR
#define jit_add_state(u,v)						\
	do {								\
		jit_new_node_ww(jit_code_movr,_R0,LIGHTREC_REG_STATE);	\
		jit_new_node_www(jit_code_addr,u,v,_R0);		\
	} while (0)
#else
#define jit_add_state(u,v)	jit_addr(u,v,LIGHTREC_REG_STATE)
#endif

#endif /* __LIGHTNING_WRAPPER_H__ */
