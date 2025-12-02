// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2020-2021 Paul Cercueil <paul@crapouillou.net>
 */

#include "blockcache.h"
#include "debug.h"
#include "lightrec-private.h"
#include "memmanager.h"
#include "slist.h"
#include "reaper.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

struct reaper_elm {
	reap_func_t func;
	void *data;
	struct slist_elm slist;
};

struct reaper {
	struct lightrec_state *state;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct slist_elm reap_list;

	bool running;
	atomic_uint sem;
};

struct reaper *lightrec_reaper_init(struct lightrec_state *state)
{
	struct reaper *reaper;
	int ret;

	reaper = lightrec_malloc(state, MEM_FOR_LIGHTREC, sizeof(*reaper));
	if (!reaper) {
		pr_err("Cannot create reaper: Out of memory\n");
		return NULL;
	}

	reaper->state = state;
	reaper->running = false;
	reaper->sem = 0;
	slist_init(&reaper->reap_list);

	ret = pthread_mutex_init(&reaper->mutex, NULL);
	if (ret) {
		pr_err("Cannot init mutex variable: %d\n", ret);
		goto err_free_reaper;
	}

	ret = pthread_cond_init(&reaper->cond, NULL);
	if (ret) {
		pr_err("Cannot init cond variable: %d\n", ret);
		goto err_destroy_mutex;
	}

	return reaper;

err_destroy_mutex:
	pthread_mutex_destroy(&reaper->mutex);
err_free_reaper:
	lightrec_free(reaper->state, MEM_FOR_LIGHTREC, sizeof(*reaper), reaper);
	return NULL;
}

void lightrec_reaper_destroy(struct reaper *reaper)
{
	lightrec_reaper_reap(reaper);

	pthread_cond_destroy(&reaper->cond);
	pthread_mutex_destroy(&reaper->mutex);
	lightrec_free(reaper->state, MEM_FOR_LIGHTREC, sizeof(*reaper), reaper);
}

int lightrec_reaper_add(struct reaper *reaper, reap_func_t f, void *data)
{
	struct reaper_elm *reaper_elm;
	struct slist_elm *elm;
	int ret = 0;

	pthread_mutex_lock(&reaper->mutex);

	for (elm = reaper->reap_list.next; elm; elm = elm->next) {
		reaper_elm = container_of(elm, struct reaper_elm, slist);

		if (reaper_elm->data == data)
			goto out_unlock;
	}

	reaper_elm = lightrec_malloc(reaper->state, MEM_FOR_LIGHTREC,
				     sizeof(*reaper_elm));
	if (!reaper_elm) {
		pr_err("Cannot add reaper entry: Out of memory\n");
		ret = -ENOMEM;
		goto out_unlock;
	}

	reaper_elm->func = f;
	reaper_elm->data = data;
	slist_append(&reaper->reap_list, &reaper_elm->slist);

out_unlock:
	pthread_mutex_unlock(&reaper->mutex);
	return ret;
}

static bool lightrec_reaper_can_reap(struct reaper *reaper)
{
	return !atomic_load_explicit(&reaper->sem, memory_order_relaxed);
}

void lightrec_reaper_reap(struct reaper *reaper)
{
	struct reaper_elm *reaper_elm;
	struct slist_elm *elm;

	pthread_mutex_lock(&reaper->mutex);

	while (lightrec_reaper_can_reap(reaper) &&
	       !!(elm = slist_first(&reaper->reap_list))) {
		slist_remove(&reaper->reap_list, elm);
		reaper->running = true;
		pthread_mutex_unlock(&reaper->mutex);

		reaper_elm = container_of(elm, struct reaper_elm, slist);

		(*reaper_elm->func)(reaper->state, reaper_elm->data);

		lightrec_free(reaper->state, MEM_FOR_LIGHTREC,
			      sizeof(*reaper_elm), reaper_elm);

		pthread_mutex_lock(&reaper->mutex);
		reaper->running = false;
		pthread_cond_broadcast(&reaper->cond);
	}

	pthread_mutex_unlock(&reaper->mutex);
}

void lightrec_reaper_pause(struct reaper *reaper)
{
	atomic_fetch_add_explicit(&reaper->sem, 1, memory_order_relaxed);

	pthread_mutex_lock(&reaper->mutex);
	while (reaper->running)
		pthread_cond_wait(&reaper->cond, &reaper->mutex);
	pthread_mutex_unlock(&reaper->mutex);
}

void lightrec_reaper_continue(struct reaper *reaper)
{
	atomic_fetch_sub_explicit(&reaper->sem, 1, memory_order_relaxed);
}
