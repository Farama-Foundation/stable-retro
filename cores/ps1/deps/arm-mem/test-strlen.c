#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h>

//extern size_t mystrlen(const char *s);
//#define strlen mystrlen

#define PAGESIZE 4096

int main(void)
{
    /* To check we don't accidentally read off the end of the string
     * across a page boundary, do our tests up to a mapped-out page.
     * To check we handle boundaries between valid pages, we require
     * two mapped-in pages beforehand.
     */
    uint8_t *buffer = mmap(NULL, 3*PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED)
    {
        fprintf(stderr, "mmap() failed\n");
        exit(EXIT_FAILURE);
    }
    if (mprotect(buffer + 2*PAGESIZE, PAGESIZE, PROT_NONE) != 0)
    {
        perror("mprotect");
        munmap(buffer, 3*PAGESIZE);
        exit(EXIT_FAILURE);
    }

    for (uint32_t postamble = 0; postamble <= 32; postamble++)
    {
        memset(buffer, 'x', 2*PAGESIZE);
        buffer[2*PAGESIZE - 1 - postamble] = '\0';
        for (uint32_t start = 0; start <= 2*PAGESIZE - 1 - postamble; start++)
            assert(strlen(buffer + start) == 2*PAGESIZE - 1 - postamble - start);
    }

    printf("strlen passes OK\n");
    munmap(buffer, 3*PAGESIZE);
    exit(EXIT_SUCCESS);
}
