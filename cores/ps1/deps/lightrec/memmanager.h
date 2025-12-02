/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __MEMMANAGER_H__
#define __MEMMANAGER_H__

#include "lightrec.h"

enum mem_type {
	MEM_FOR_CODE,
	MEM_FOR_MIPS_CODE,
	MEM_FOR_IR,
	MEM_FOR_LIGHTREC,
	MEM_TYPE_END,
};

void * lightrec_malloc(struct lightrec_state *state,
		       enum mem_type type, unsigned int len);
void * lightrec_calloc(struct lightrec_state *state,
		       enum mem_type type, unsigned int len);
void lightrec_free(struct lightrec_state *state,
		   enum mem_type type, unsigned int len, void *ptr);

void lightrec_register(enum mem_type type, unsigned int len);
void lightrec_unregister(enum mem_type type, unsigned int len);

unsigned int lightrec_get_mem_usage(enum mem_type type);
unsigned int lightrec_get_total_mem_usage(void);
float lightrec_get_average_ipi(void);

#endif /* __MEMMANAGER_H__ */
