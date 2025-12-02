#ifndef MMAN_H
#define MMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#include <3ds/svc.h>
#include "3ds_utils.h"

#define PROT_READ       0b001
#define PROT_WRITE      0b010
#define PROT_EXEC       0b100
#define MAP_PRIVATE     2
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20

#define MAP_FAILED      ((void *)-1)

void SysPrintf(const char *fmt, ...);

#if 0 // not used
static void* dynarec_cache = NULL;
static void* dynarec_cache_mapping = NULL;

static inline void* mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
   (void)fd;
   (void)offset;

   void* addr_out;

   if((prot == (PROT_READ | PROT_WRITE | PROT_EXEC)) &&
      (flags == (MAP_PRIVATE | MAP_ANONYMOUS)))
   {
      if(__ctr_svchax)
      {
         /* this hack works only for pcsx_rearmed */
         uint32_t currentHandle;

         if (!dynarec_cache) {
            dynarec_cache = memalign(0x1000, len);
            if (!dynarec_cache)
               return MAP_FAILED;
         }

         svcDuplicateHandle(&currentHandle, 0xFFFF8001);
         svcControlProcessMemory(currentHandle, (uintptr_t)addr, (uintptr_t)dynarec_cache,
                                 len, MEMOP_MAP, prot);
         svcCloseHandle(currentHandle);
         dynarec_cache_mapping = addr;
         memset(addr, 0, len);
         return addr;
      }
      else
      {
         printf("tried to mmap RWX pages without svcControlProcessMemory access !\n");
         return MAP_FAILED;
      }

   }

   addr_out = memalign(0x1000, len);
   if (!addr_out)
      return MAP_FAILED;

   memset(addr_out, 0, len);
   return addr_out;
}

static inline int munmap(void *addr, size_t len)
{
   if((addr == dynarec_cache_mapping) && __ctr_svchax)
   {
      uint32_t currentHandle;
      svcDuplicateHandle(&currentHandle, 0xFFFF8001);
      svcControlProcessMemory(currentHandle,
                              (uintptr_t)dynarec_cache, (uintptr_t)dynarec_cache_mapping,
                              len, MEMOP_UNMAP, 0b111);
      svcCloseHandle(currentHandle);
      dynarec_cache_mapping = NULL;

   }
   else
      free(addr);

   return 0;
}
#endif

static inline int mprotect(void *addr, size_t len, int prot)
{
   if (__ctr_svchax)
   {
      uint32_t currentHandle = 0;
      int r;
      svcDuplicateHandle(&currentHandle, 0xFFFF8001);
      r = svcControlProcessMemory(currentHandle, (uintptr_t)addr, 0,
                                  len, MEMOP_PROT, prot);
      svcCloseHandle(currentHandle);
      if (r < 0) {
         SysPrintf("svcControlProcessMemory failed for %p %u %x: %d\n",
                   addr, len, prot, r);
         return -1;
      }
      return 0;
   }

   SysPrintf("mprotect called without svcControlProcessMemory access!\n");
   return -1;
}

#ifdef __cplusplus
};
#endif

#endif // MMAN_H

