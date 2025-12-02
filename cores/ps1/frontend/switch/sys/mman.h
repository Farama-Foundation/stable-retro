#ifndef MMAN_H
#define MMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <switch.h>

#define PROT_READ       0b001
#define PROT_WRITE      0b010
#define PROT_EXEC       0b100
#define MAP_PRIVATE     2
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20

#define MAP_FAILED      ((void *)-1)

#define ALIGNMENT       0x1000

#if 0 // not used
static inline void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    (void)fd;
    (void)offset;

    // match Linux behavior
    len = (len + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    Result rc = svcMapPhysicalMemory(addr, len);
    if (R_FAILED(rc))
    {
        //printf("mmap failed\n");
        addr = aligned_alloc(ALIGNMENT, len);
    }
    if (!addr)
        return MAP_FAILED;
    memset(addr, 0, len);
    return addr;
}

static inline int munmap(void *addr, size_t len)
{
    len = (len + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
    Result rc = svcUnmapPhysicalMemory(addr, len);
    if (R_FAILED(rc))
    {
        //printf("munmap failed\n");
        free(addr);
    }
    return 0;
}
#endif

#ifdef __cplusplus
};
#endif

#endif // MMAN_H

