#ifndef _LIBRETRO_PERF_H
#define _LIBRETRO_PERF_H

#include "libretro.h"

#ifdef LOG_PERFORMANCE
#define RETRO_PERFORMANCE_INIT(perf_cb, name) static struct retro_perf_counter name = {#name}; if (!name.registered) perf_cb.perf_register(&(name))
#define RETRO_PERFORMANCE_START(perf_cb, name) perf_cb.perf_start(&(name))
#define RETRO_PERFORMANCE_STOP(perf_cb, name) perf_cb.perf_stop(&(name))
#else
#define RETRO_PERFORMANCE_INIT(perf_cb, name)
#define RETRO_PERFORMANCE_START(perf_cb, name)
#define RETRO_PERFORMANCE_STOP(perf_cb, name)
#endif

#endif
