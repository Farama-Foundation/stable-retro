#ifndef MMAN_H
#define MMAN_H

#include <stdlib.h>
//#include <psp2/kernel/sysmem.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROT_READ       0b001
#define PROT_WRITE      0b010
#define PROT_EXEC       0b100
#define MAP_PRIVATE     2
#define MAP_ANONYMOUS   0x20

#define MAP_FAILED      ((void *)-1)

#if 0 // not used
static inline void* mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
   (void)prot;
   (void)flags;
   (void)fd;
   (void)offset;

   int block, ret;

   block = sceKernelAllocMemBlockForVM("code", len);
   if(block<=0){
     sceClibPrintf("could not alloc mem block @0x%08X 0x%08X \n", block, len);
     exit(1);
   }

   // get base address
   ret = sceKernelGetMemBlockBase(block, &addr);
   if (ret < 0)
   {
     sceClibPrintf("could get address @0x%08X 0x%08X  \n", block, addr);
     exit(1);
   }


   if(!addr)
      return MAP_FAILED;

   return addr;
}

static inline int mprotect(void *addr, size_t len, int prot)
{
   (void)addr;
   (void)len;
   (void)prot;
   return 0;
}

static inline int munmap(void *addr, size_t len)
{
  int uid = sceKernelFindMemBlockByAddr(addr, len);

  return sceKernelFreeMemBlock(uid);

}
#endif

#ifdef __cplusplus
};
#endif

#endif // MMAN_H
