#ifndef _GNU_SOURCE
#define _GNU_SOURCE // *_np
#endif
#ifdef _3DS
#include <3ds/svc.h>
#include <3ds/os.h>
#include <3ds/services/apt.h>
#include <sys/time.h>
#endif

#include "../deps/libretro-common/rthreads/rthreads.c"
#include "features/features_cpu.h"
#include "libretro-rthreads.h"

// pcsxr "extensions"
extern void SysPrintf(const char *fmt, ...);

#ifdef _3DS
static bool is_new_3ds;
#endif

void pcsxr_sthread_init(void)
{
	SysPrintf("%d cpu core(s) detected\n", cpu_features_get_core_amount());
#ifdef _3DS
	int64_t version = 0;
	int fpscr = -1;

	APT_CheckNew3DS(&is_new_3ds);
	svcGetSystemInfo(&version, 0x10000, 0);

	APT_SetAppCpuTimeLimit(35);
	u32 percent = -1;
	APT_GetAppCpuTimeLimit(&percent);

	__asm__ volatile("fmrx %0, fpscr" : "=r"(fpscr));
	SysPrintf("%s3ds detected, v%d.%d, AppCpuTimeLimit=%ld fpscr=%08x\n",
		is_new_3ds ? "new" : "old", (int)GET_VERSION_MAJOR(version),
		(int)GET_VERSION_MINOR(version), percent, fpscr);
#endif
}

sthread_t *pcsxr_sthread_create(void (*thread_func)(void *),
	enum pcsxr_thread_type type)
{
	sthread_t *h = NULL;
#ifdef _3DS
	Thread ctr_thread;
	int core_id = 0;
	s32 prio = 0x30;

	h = calloc(1, sizeof(*h));
	if (!h)
		return NULL;

	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);

	switch (type) {
	case PCSXRT_CDR:
	case PCSXRT_SPU:
		core_id = 1;
		break;
	case PCSXRT_DRC:
	case PCSXRT_GPU:
		core_id = is_new_3ds ? 2 : 1;
		break;
	case PCSXRT_COUNT:
		break;
	}

	ctr_thread = threadCreate(thread_func, NULL, STACKSIZE, prio, core_id, false);
	if (!ctr_thread) {
		if (core_id == 1) {
			SysPrintf("threadCreate pcsxt %d core %d failed\n",
				type, core_id);
			core_id = is_new_3ds ? 2 : -1;
			ctr_thread = threadCreate(thread_func, NULL, STACKSIZE,
				prio, core_id, false);
		}
	}
	if (!ctr_thread) {
		SysPrintf("threadCreate pcsxt %d core %d failed\n", type, core_id);
		free(h);
		return NULL;
	}
	h->id = (pthread_t)ctr_thread;
#else
	h = sthread_create(thread_func, NULL);
 #if defined(__GLIBC__) || \
    (defined(__ANDROID_API__) && __ANDROID_API__ >= 26)
	if (h && (unsigned int)type < (unsigned int)PCSXRT_COUNT)
	{
		const char * const pcsxr_tnames[PCSXRT_COUNT] = {
			"pcsxr-cdrom", "pcsxr-drc", "pcsxr-gpu", "pcsxr-spu"
		};
		pthread_setname_np(h->id, pcsxr_tnames[type]);
	}
 #endif
#endif
	return h;
}
