#ifndef __LIBRETRO_PCSXR_RTHREADS_H__
#define __LIBRETRO_PCSXR_RTHREADS_H__

#include "rthreads/rthreads.h"

enum pcsxr_thread_type
{
	PCSXRT_CDR = 0,
	PCSXRT_DRC,
	PCSXRT_GPU,
	PCSXRT_SPU,
	PCSXRT_COUNT // must be last
};

void pcsxr_sthread_init(void);
sthread_t *pcsxr_sthread_create(void (*thread_func)(void*),
	enum pcsxr_thread_type type);

#endif // __LIBRETRO_PCSXR_RTHREADS_H__
