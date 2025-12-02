
#ifndef _3DS_SEMAPHORE_WRAP__
#define _3DS_SEMAPHORE_WRAP__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "3ds_utils.h"

typedef uint32_t sem_t;

static inline int sem_init(sem_t *sem, int pshared, unsigned int value)
{
   return svcCreateSemaphore(sem, value, INT32_MAX);
}

static inline int sem_post(sem_t *sem)
{
   int32_t count;
   return svcReleaseSemaphore(&count, *sem, 1);
}

static inline int sem_wait(sem_t *sem)
{
   return svcWaitSynchronization(*sem, INT64_MAX);
}

static inline int sem_destroy(sem_t *sem)
{
   return svcCloseHandle(*sem);
}

#endif //_3DS_SEMAPHORE_WRAP__

