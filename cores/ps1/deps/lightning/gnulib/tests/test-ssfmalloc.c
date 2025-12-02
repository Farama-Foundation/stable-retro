/* Test of simple and straight-forward malloc implementation.
   Copyright (C) 2020-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2020.  */

#include <config.h>

#include <stdint.h>
#include <stdlib.h>

#if defined _WIN32 && !defined __CYGWIN__

/* Declare VirtualAlloc(), GetSystemInfo.  */
# define WIN32_LEAN_AND_MEAN
# define WIN32_EXTRA_LEAN
# include <windows.h>

#else

/* Declare getpagesize(). */
# include <unistd.h>
/* On HP-UX, getpagesize exists, but it is not declared in <unistd.h> even if
   the compiler options -D_HPUX_SOURCE -D_XOPEN_SOURCE=600 are used.  */
# ifdef __hpux
extern
#  ifdef __cplusplus
       "C"
#  endif
       int getpagesize (void);
# endif

/* Declare mmap().  */
# include <sys/types.h>
# include <sys/mman.h>

/* Some old mmap() implementations require the flag MAP_VARIABLE whenever you
   pass an addr == NULL. */
# ifndef MAP_VARIABLE
#  define MAP_VARIABLE 0
# endif

#endif

/* ================= Back end of the malloc implementation ================= */

/* The memory page size.
   Once it is initialized, a power of 2.  Typically 4096 or 8192.  */
static uintptr_t pagesize;

/* Initializes pagesize.  */
static void
init_pagesize (void)
{
#if defined _WIN32 && !defined __CYGWIN__
  /* GetSystemInfo
     <https://msdn.microsoft.com/en-us/library/ms724381.aspx>
     <https://msdn.microsoft.com/en-us/library/ms724958.aspx>  */
  SYSTEM_INFO info;
  GetSystemInfo (&info);
  pagesize = info.dwPageSize;
#else
  pagesize = getpagesize ();
#endif
}

/* Allocates a contiguous set of pages of memory.
   size > 0, must be a multiple of pagesize.
   Returns a multiple of PAGESIZE, or 0 upon failure.  */
static uintptr_t
alloc_pages (size_t size)
{
#if defined _WIN32 && !defined __CYGWIN__
  /* VirtualAlloc
     <https://msdn.microsoft.com/en-us/library/aa366887.aspx>
     <https://msdn.microsoft.com/en-us/library/aa366786.aspx>  */
  void *mem = VirtualAlloc (NULL, size, MEM_COMMIT, PAGE_READWRITE);
  if (mem == NULL)
    return 0;
  return (uintptr_t) mem;
#else
  /* Use mmap with the MAP_ANONYMOUS or MAP_ANON flag.  */
  void *mem = mmap (NULL, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_VARIABLE, -1, 0);
  if (mem == (void *)(-1))
    return 0;
  return (uintptr_t) mem;
#endif
}

/* Frees a contiguous set of pages of memory, returned by alloc_pages.
   size > 0, must be a multiple of pagesize.  */
static void
free_pages (uintptr_t pages, size_t size)
{
#if defined _WIN32 && !defined __CYGWIN__
  /* VirtualFree
     <https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree>  */
  if (!VirtualFree ((void *) pages, 0, MEM_RELEASE))
    abort ();
#else
  if ((pages & (pagesize - 1)) != 0)
    abort ();
  if (munmap ((void *) pages, size) < 0)
    abort ();
#endif
}

/* Cygwin defines PAGESIZE in <limits.h>.  */
#undef PAGESIZE

/* ======================= Instantiate the front end ======================= */

#define PAGESIZE pagesize
/* On Cygwin and Linux/PowerPC, PAGESIZE is 65536.  On macOS 11, it is 16384.
   On all other platforms, it is either 4096 or 8192.  */
#if defined __CYGWIN__ || (defined __linux__ && defined __powerpc__)
# define PAGESIZE_MAX 65536
#else
# define PAGESIZE_MAX 16384
#endif

#define ALLOC_PAGES alloc_pages
#define FREE_PAGES free_pages
#define ALIGNMENT (sizeof (void *)) /* or 8 or 16 or 32 */
#define PAGE_RESERVED_HEADER_SIZE (3 * UINTPTR_WIDTH / 8) /* = 3 * sizeof (void *) */

#include "ssfmalloc.h"

/* ================================= Tests ================================= */

#include <limits.h>
#include <string.h>

#include "macros.h"

/* Fills a block of a given size with some contents.  */
static void
fill_block (uintptr_t block, size_t size)
{
  unsigned char code = (size % (UCHAR_MAX - 1)) + 1;
  memset ((char *) block, code, size);
}

/* Verifies that the contents of a block is still present
   (i.e. has not accidentally been overwritten by other operations).  */
static void
verify_block (uintptr_t block, size_t size)
{
  unsigned char code = (size % (UCHAR_MAX - 1)) + 1;
  char *p = (char *) block;
  for (; size > 0; p++, size--)
    if ((unsigned char) *p != code)
      abort ();
}

static size_t block_sizes[] =
  {
    /* Small blocks.  */
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    12,
    15,
    16,
    17,
    24,
    31,
    32,
    37,
    42,
    49,
    57,
    63,
    64,
    65,
    71,
    77,
    83,
    96,
    99,
    110,
    119,
    127,
    128,
    130,
    144,
    150,
    157,
    161,
    169,
    180,
    192,
    199,
    204,
    210,
    224,
    225,
    236,
    241,
    249,
    255,
    256,
    /* Medium blocks.  */
    257,
    281,
    284,
    294,
    301,
    308,
    341,
    447,
    525,
    659,
    771,
    842,
    729,
    999,
    1000,
    1020,
    1023,
    1024,
    1025,
    1280,
    1414,
    2047,
    2048,
    2049,
    2096,
    2401,
    2613,
    2843,
    3010,
    3213,
    3512,
    3678,
    3801,
    3900,
    /* Large blocks.  */
    4000,
    4060,
    4080,
    4090,
    4095,
    4096,
    4097,
    4121,
    5381,
    7814,
    8191,
    8192,
    8193,
    11238,
    16383,
    16384,
    16385,
    20184,
    51202,
    135010
  };

#define RANDOM(n) (rand () % (n))
#define RANDOM_BLOCK_SIZE() block_sizes[RANDOM (SIZEOF (block_sizes))]

int
main (int argc, char *argv[])
{
  /* Allow the user to provide a non-default random seed on the command line.  */
  if (argc > 1)
    srand (atoi (argv[1]));

  init_pagesize ();

  /* Randomly allocate and deallocate blocks.
     Also verify that there are no unexpected modifications to the contents of
     these blocks.  */
  {
    unsigned int repeat;
    char *blocks[SIZEOF (block_sizes)];

    {
      size_t i;

      for (i = 0; i < SIZEOF (block_sizes); i++)
        blocks[i] = NULL;
    }

    for (repeat = 0; repeat < 100000; repeat++)
      {
        unsigned int operation = RANDOM (2);

        switch (operation)
          {
          case 0:
            { /* Allocate a block.  */
              size_t i = RANDOM (SIZEOF (block_sizes));
              size_t size = block_sizes[i];
              if (blocks[i] == NULL)
                {
                  uintptr_t block = allocate_block (size);
                  if (block == 0)
                    abort ();
                  fill_block (block, size);
                  blocks[i] = (char *) block;
                }
            }
            break;
          case 1:
            { /* Free a block.  */
              size_t i = RANDOM (SIZEOF (block_sizes));
              size_t size = block_sizes[i];
              if (blocks[i] != NULL)
                {
                  uintptr_t block = (uintptr_t) blocks[i];
                  verify_block (block, size);
                  free_block (block);
                  blocks[i] = NULL;
                }
            }
            break;
          }
      }

    /* Free the remaining blocks.  */
    {
      size_t i;

      for (i = 0; i < SIZEOF (block_sizes); i++)
        if (blocks[i] != NULL)
          {
            uintptr_t block = (uintptr_t) blocks[i];
            size_t size = block_sizes[i];
            verify_block (block, size);
            free_block (block);
          }
    }
  }

  return 0;
}
