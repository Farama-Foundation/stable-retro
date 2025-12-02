/**************************************************************************
*   Copyright (C) 2020 The RetroArch Team                                 *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../gpulib/gpu.h"
#include "../../frontend/plugin_lib.h"
#include "gpu.h"
#include "gpu_timing.h"
#include "gpulib_thread_if.h"

extern void SysPrintf(const char *fmt, ...);

#define FALSE 0
#define TRUE 1
#define BOOL unsigned short

typedef struct {
	uint32_t *cmd_list;
	int count;
	int last_cmd;
} video_thread_cmd;

#define QUEUE_SIZE 0x2000

typedef struct {
	size_t start;
	size_t end;
	size_t used;
	video_thread_cmd queue[QUEUE_SIZE];
} video_thread_queue;

typedef struct {
	pthread_t thread;
	pthread_mutex_t queue_lock;
	pthread_cond_t cond_msg_avail;
	pthread_cond_t cond_msg_done;
	pthread_cond_t cond_queue_empty;
	video_thread_queue *queue;
	video_thread_queue *bg_queue;
	BOOL running;
} video_thread_state;

static video_thread_state thread;
static video_thread_queue queues[2];
static int thread_rendering;
static BOOL hold_cmds;
static BOOL needs_display;
static BOOL flushed;

extern const unsigned char cmd_lengths[];

static void *video_thread_main(void *arg) {
	video_thread_cmd *cmd;
	int i;

#ifdef _3DS
	static int processed = 0;
#endif /* _3DS */

#if defined(__arm__) && defined(__ARM_FP)
	// RunFast mode
	uint32_t fpscr = ~0;
	__asm__ volatile("vmrs %0, fpscr" : "=r"(fpscr));
	fpscr &= ~0x00009f9f;
	fpscr |=  0x03000000; // DN | FZ
	__asm__ volatile("vmsr fpscr, %0" :: "r"(fpscr));
#endif

	while(1) {
		int result, cycles_dummy = 0, last_cmd, start, end;
		video_thread_queue *queue;
		pthread_mutex_lock(&thread.queue_lock);

		while (!thread.queue->used && thread.running) {
			pthread_cond_wait(&thread.cond_msg_avail, &thread.queue_lock);
		}

		if (!thread.running) {
			pthread_mutex_unlock(&thread.queue_lock);
			break;
		}

		queue = thread.queue;
		start = queue->start;
		end = queue->end > queue->start ? queue->end : QUEUE_SIZE;
		queue->start = end % QUEUE_SIZE;
		pthread_mutex_unlock(&thread.queue_lock);

		for (i = start; i < end; i++) {
			cmd = &queue->queue[i];
			result = real_do_cmd_list(cmd->cmd_list, cmd->count,
					&cycles_dummy, &cycles_dummy, &last_cmd);
			if (result != cmd->count) {
				fprintf(stderr, "Processed wrong cmd count: expected %d, got %d\n", cmd->count, result);
			}

#ifdef _3DS
			/* Periodically yield so as not to starve other threads */
			processed += cmd->count;
			if (processed >= 512) {
				svcSleepThread(1);
				processed %= 512;
			}
#endif /* _3DS */
		}

		pthread_mutex_lock(&thread.queue_lock);
		queue->used -= (end - start);

		if (!queue->used)
			pthread_cond_signal(&thread.cond_queue_empty);

		pthread_cond_signal(&thread.cond_msg_done);
		pthread_mutex_unlock(&thread.queue_lock);
	}

	return 0;
}

static void cmd_queue_swap() {
	video_thread_queue *tmp;
	if (!thread.bg_queue->used) return;

	pthread_mutex_lock(&thread.queue_lock);
	if (!thread.queue->used) {
		tmp = thread.queue;
		thread.queue = thread.bg_queue;
		thread.bg_queue = tmp;
		pthread_cond_signal(&thread.cond_msg_avail);
	}
	pthread_mutex_unlock(&thread.queue_lock);
}

/* Waits for the main queue to completely finish. */
void renderer_wait() {
	if (!thread.running) return;

	/* Not completely safe, but should be fine since the render thread
	 * only decreases used, and we check again inside the lock. */
	if (!thread.queue->used) {
		return;
	}

	pthread_mutex_lock(&thread.queue_lock);

	while (thread.queue->used) {
		pthread_cond_wait(&thread.cond_queue_empty, &thread.queue_lock);
	}

	pthread_mutex_unlock(&thread.queue_lock);
}

/* Waits for all GPU commands in both queues to finish, bringing VRAM
 * completely up-to-date. */
void renderer_sync(void) {
	if (!thread.running) return;

	/* Not completely safe, but should be fine since the render thread
	 * only decreases used, and we check again inside the lock. */
	if (!thread.queue->used && !thread.bg_queue->used) {
		return;
	}

	if (thread.bg_queue->used) {
		/* When we flush the background queue, the vblank handler can't
		 * know that we had a frame pending, and we delay rendering too
		 * long. Force it. */
		flushed = TRUE;
	}

	/* Flush both queues. This is necessary because gpulib could be
	 * trying to process a DMA write that a command in the queue should
	 * run beforehand. For example, Xenogears sprites write a black
	 * rectangle over the to-be-DMA'd spot in VRAM -- if this write
	 * happens after the DMA, it will clear the DMA, resulting in
	 * flickering sprites. We need to be totally up-to-date. This may
	 * drop a frame. */
	renderer_wait();
	cmd_queue_swap();
	hold_cmds = FALSE;
	renderer_wait();
}

static void video_thread_stop() {
	int i;
	renderer_sync();

	if (thread.running) {
		thread.running = FALSE;
		pthread_cond_signal(&thread.cond_msg_avail);
		pthread_join(thread.thread, NULL);
	}

	pthread_mutex_destroy(&thread.queue_lock);
	pthread_cond_destroy(&thread.cond_msg_avail);
	pthread_cond_destroy(&thread.cond_msg_done);
	pthread_cond_destroy(&thread.cond_queue_empty);

	for (i = 0; i < QUEUE_SIZE; i++) {
		video_thread_cmd *cmd = &thread.queue->queue[i];
		free(cmd->cmd_list);
		cmd->cmd_list = NULL;
	}

	for (i = 0; i < QUEUE_SIZE; i++) {
		video_thread_cmd *cmd = &thread.bg_queue->queue[i];
		free(cmd->cmd_list);
		cmd->cmd_list = NULL;
	}
}

static void video_thread_start() {
	SysPrintf("Starting render thread\n");

	thread.queue = &queues[0];
	thread.bg_queue = &queues[1];
	thread.running = TRUE;

	if (pthread_cond_init(&thread.cond_msg_avail, NULL) ||
			pthread_cond_init(&thread.cond_msg_done, NULL) ||
			pthread_cond_init(&thread.cond_queue_empty, NULL) ||
			pthread_mutex_init(&thread.queue_lock, NULL) ||
			pthread_create(&thread.thread, NULL, video_thread_main, &thread)) {
		goto error;
	}

	return;

 error:
	SysPrintf("Failed to start rendering thread\n");
	thread.running = FALSE;
	video_thread_stop();
}

static void video_thread_queue_cmd(uint32_t *list, int count, int last_cmd) {
	video_thread_cmd *cmd;
	uint32_t *cmd_list;
	video_thread_queue *queue;
	BOOL lock;

	cmd_list = (uint32_t *)calloc(count, sizeof(uint32_t));

	if (!cmd_list) {
		/* Out of memory, disable the thread and run sync from now on */
		SysPrintf("Failed to allocate render thread command list, stopping thread\n");
		video_thread_stop();
	}

	memcpy(cmd_list, list, count * sizeof(uint32_t));

	if (hold_cmds && thread.bg_queue->used >= QUEUE_SIZE) {
		/* If the bg queue is full, do a full sync to empty both queues
		 * and clear space. This should be very rare, I've only seen it in
		 * Tekken 3 post-battle-replay. */
		renderer_sync();
	}

	if (hold_cmds) {
		queue = thread.bg_queue;
		lock = FALSE;
	} else {
		queue = thread.queue;
		lock = TRUE;
	}

	if (lock) {
		pthread_mutex_lock(&thread.queue_lock);

		while (queue->used >= QUEUE_SIZE) {
			pthread_cond_wait(&thread.cond_msg_done, &thread.queue_lock);
		}
	}

	cmd = &queue->queue[queue->end];
	free(cmd->cmd_list);
	cmd->cmd_list = cmd_list;
	cmd->count = count;
	cmd->last_cmd = last_cmd;
	queue->end = (queue->end + 1) % QUEUE_SIZE;
	queue->used++;

	if (lock) {
		pthread_cond_signal(&thread.cond_msg_avail);
		pthread_mutex_unlock(&thread.queue_lock);
	}
}

/* Slice off just the part of the list that can be handled async, and
 * update ex_regs. */
static int scan_cmd_list(uint32_t *data, int count,
	int *cycles_sum_out, int *cycles_last, int *last_cmd)
{
	int cpu_cycles_sum = 0, cpu_cycles = *cycles_last;
	int cmd = 0, pos = 0, len, v;

	while (pos < count) {
		uint32_t *list = data + pos;
		short *slist = (void *)list;
		cmd = LE32TOH(list[0]) >> 24;
		len = 1 + cmd_lengths[cmd];

		switch (cmd) {
			case 0x02:
				gput_sum(cpu_cycles_sum, cpu_cycles,
					gput_fill(LE16TOH(slist[4]) & 0x3ff,
						LE16TOH(slist[5]) & 0x1ff));
				break;
			case 0x20 ... 0x23:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base());
				break;
			case 0x24 ... 0x27:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_t());
				gpu.ex_regs[1] &= ~0x1ff;
				gpu.ex_regs[1] |= LE32TOH(list[4]) & 0x1ff;
				break;
			case 0x28 ... 0x2b:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base());
				break;
			case 0x2c ... 0x2f:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_t());
				gpu.ex_regs[1] &= ~0x1ff;
				gpu.ex_regs[1] |= LE32TOH(list[4]) & 0x1ff;
				break;
			case 0x30 ... 0x33:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_g());
				break;
			case 0x34 ... 0x37:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_gt());
				gpu.ex_regs[1] &= ~0x1ff;
				gpu.ex_regs[1] |= LE32TOH(list[5]) & 0x1ff;
				break;
			case 0x38 ... 0x3b:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_g());
				break;
			case 0x3c ... 0x3f:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_gt());
				gpu.ex_regs[1] &= ~0x1ff;
				gpu.ex_regs[1] |= LE32TOH(list[5]) & 0x1ff;
				break;
			case 0x40 ... 0x47:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));
				break;
			case 0x48 ... 0x4F:
				for (v = 3; pos + v < count; v++)
				{
					gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));
					if ((list[v] & 0xf000f000) == 0x50005000)
						break;
				}
				len += v - 3;
				break;
			case 0x50 ... 0x57:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));
				break;
			case 0x58 ... 0x5F:
				for (v = 4; pos + v < count; v += 2)
				{
					gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));
					if ((list[v] & 0xf000f000) == 0x50005000)
						break;
				}
				len += v - 4;
				break;
			case 0x60 ... 0x63:
				gput_sum(cpu_cycles_sum, cpu_cycles,
					gput_sprite(LE16TOH(slist[4]) & 0x3ff,
						LE16TOH(slist[5]) & 0x1ff));
				break;
			case 0x64 ... 0x67:
				gput_sum(cpu_cycles_sum, cpu_cycles,
					gput_sprite(LE16TOH(slist[6]) & 0x3ff,
						LE16TOH(slist[7]) & 0x1ff));
				break;
			case 0x68 ... 0x6b:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(1, 1));
				break;
			case 0x70 ... 0x77:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(8, 8));
				break;
			case 0x78 ... 0x7f:
				gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(16, 16));
				break;
			default:
				if ((cmd & 0xf8) == 0xe0)
					gpu.ex_regs[cmd & 7] = list[0];
				break;
		}

		if (pos + len > count) {
			cmd = -1;
			break; /* incomplete cmd */
		}
		if (0x80 <= cmd && cmd <= 0xdf)
			break; /* image i/o */

		pos += len;
	}

	*cycles_sum_out += cpu_cycles_sum;
	*cycles_last = cpu_cycles;
	*last_cmd = cmd;
	return pos;
}

int do_cmd_list(uint32_t *list, int count,
 int *cycles_sum, int *cycles_last, int *last_cmd)
{
	int pos = 0;

	if (thread.running) {
		pos = scan_cmd_list(list, count, cycles_sum, cycles_last, last_cmd);
		video_thread_queue_cmd(list, pos, *last_cmd);
	} else {
		pos = real_do_cmd_list(list, count, cycles_sum, cycles_last, last_cmd);
		memcpy(gpu.ex_regs, gpu.scratch_ex_regs, sizeof(gpu.ex_regs));
	}
	return pos;
}

int renderer_init(void) {
	if (thread_rendering) {
		video_thread_start();
	}
	return real_renderer_init();
}

void renderer_finish(void) {
	real_renderer_finish();

	if (thread_rendering && thread.running) {
		video_thread_stop();
	}
}

void renderer_sync_ecmds(uint32_t * ecmds) {
	if (thread.running) {
		int dummy = 0;
		do_cmd_list(&ecmds[1], 6, &dummy, &dummy, &dummy);
	} else {
		real_renderer_sync_ecmds(ecmds);
	}
}

void renderer_update_caches(int x, int y, int w, int h, int state_changed) {
	renderer_sync();
	real_renderer_update_caches(x, y, w, h, state_changed);
}

void renderer_flush_queues(void) {
	/* Called during DMA and updateLace. We want to sync if it's DMA,
	 * but not if it's updateLace. Instead of syncing here, there's a
	 * renderer_sync call during DMA. */
	real_renderer_flush_queues();
}

/*
 * Normally all GPU commands are processed before rendering the
 * frame. For games that naturally run < 50/60fps, this is unnecessary
 * -- it forces the game to render as if it was 60fps and leaves the
 * GPU idle half the time on a 30fps game, for example.
 *
 * Allowing the renderer to wait until a frame is done before
 * rendering it would give it double, triple, or quadruple the amount
 * of time to finish before we have to wait for it.
 *
 * We can use a heuristic to figure out when to force a render.
 *
 * - If a frame isn't done when we're asked to render, wait for it and
 *   put future GPU commands in a separate buffer (for the next frame)
 *
 * - If the frame is done, and had no future GPU commands, render it.
 *
 * - If we do have future GPU commands, it meant the frame took too
 *   long to render and there's another frame waiting. Stop until the
 *   first frame finishes, render it, and start processing the next
 *   one.
 *
 * This may possibly add a frame or two of latency that shouldn't be
 * different than the real device. It may skip rendering a frame
 * entirely if a VRAM transfer happens while a frame is waiting, or in
 * games that natively run at 60fps if frames are coming in too
 * quickly to process. Depending on how the game treats "60fps," this
 * may not be noticeable.
 */
void renderer_notify_update_lace(int updated) {
	if (!thread.running) return;

	if (thread_rendering == THREAD_RENDERING_SYNC) {
		renderer_sync();
		return;
	}

	if (updated) {
		cmd_queue_swap();
		return;
	}

	pthread_mutex_lock(&thread.queue_lock);
	if (thread.bg_queue->used || flushed) {
		/* We have commands for a future frame to run. Force a wait until
		 * the current frame is finished, and start processing the next
		 * frame after it's drawn (see the `updated` clause above). */
		pthread_mutex_unlock(&thread.queue_lock);
		renderer_wait();
		pthread_mutex_lock(&thread.queue_lock);

		/* We are no longer holding commands back, so the next frame may
		 * get mixed into the following frame. This is usually fine, but can
		 * result in frameskip-like effects for 60fps games. */
		flushed = FALSE;
		hold_cmds = FALSE;
		needs_display = TRUE;
		gpu.state.fb_dirty = TRUE;
	} else if (thread.queue->used) {
		/* We are still drawing during a vblank. Cut off the current frame
		 * by sending new commands to the background queue and skip
		 * drawing our partly rendered frame to the display. */
		hold_cmds = TRUE;
		needs_display = TRUE;
		gpu.state.fb_dirty = FALSE;
	} else if (needs_display && !thread.queue->used) {
		/* We have processed all commands in the queue, render the
		 * buffer. We know we have something to render, because
		 * needs_display is TRUE. */
		hold_cmds = FALSE;
		needs_display = FALSE;
		gpu.state.fb_dirty = TRUE;
	} else {
		/* Everything went normally, so do the normal thing. */
	}

	pthread_mutex_unlock(&thread.queue_lock);
}

void renderer_set_interlace(int enable, int is_odd) {
	real_renderer_set_interlace(enable, is_odd);
}

void renderer_set_config(const struct rearmed_cbs *cbs) {
	renderer_sync();
	thread_rendering = cbs->thread_rendering;
	if (!thread.running && thread_rendering != THREAD_RENDERING_OFF) {
		video_thread_start();
	} else if (thread.running && thread_rendering == THREAD_RENDERING_OFF) {
		video_thread_stop();
	}
	real_renderer_set_config(cbs);
}

void renderer_notify_res_change(void) {
	renderer_sync();
	real_renderer_notify_res_change();
}
