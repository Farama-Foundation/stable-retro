/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __LIGHTREC_REAPER_H__
#define __LIGHTREC_REAPER_H__

struct lightrec_state;
struct reaper;

typedef void (*reap_func_t)(struct lightrec_state *state, void *);

struct reaper *lightrec_reaper_init(struct lightrec_state *state);
void lightrec_reaper_destroy(struct reaper *reaper);

int lightrec_reaper_add(struct reaper *reaper, reap_func_t f, void *data);
void lightrec_reaper_reap(struct reaper *reaper);

void lightrec_reaper_pause(struct reaper *reaper);
void lightrec_reaper_continue(struct reaper *reaper);

#endif /* __LIGHTREC_REAPER_H__ */
