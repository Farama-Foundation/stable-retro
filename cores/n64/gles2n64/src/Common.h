#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>

#include <retro_miscellaneous.h>

#define LOG_NONE	0
#define LOG_ERROR   1
#define LOG_MINIMAL	2
#define LOG_WARNING 3
#define LOG_VERBOSE 4

#define LOG_LEVEL LOG_WARNING

//#define DBG_LOGS

#ifdef DBG_LOGS
#define LOG(A, ...) fprintf(stdout, __VA_ARGS__)
#else
#define LOG(A, ...)
#endif

#endif
