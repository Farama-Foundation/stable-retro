/* Test of explicit_bzero() function.
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

/* Specification.  */
#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (explicit_bzero, void, (void *, size_t));

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "vma-iter.h"
#include "macros.h"

#define SECRET "xyzzy1729"
#define SECRET_SIZE 9

static char zero[SECRET_SIZE] = { 0 };

/* Enable this to verify that the test is effective.  */
#if 0
# define explicit_bzero(a, n)  memset (a, '\0', n)
#endif

/* =================== Verify operation on static memory =================== */

static char stbuf[SECRET_SIZE];

static void
test_static (void)
{
  memcpy (stbuf, SECRET, SECRET_SIZE);
  explicit_bzero (stbuf, SECRET_SIZE);
  ASSERT (memcmp (zero, stbuf, SECRET_SIZE) == 0);
}

/* =============== Verify operation on heap-allocated memory =============== */

/* Test whether an address range is mapped in memory.  */
#if VMA_ITERATE_SUPPORTED

struct locals
{
  uintptr_t range_start;
  uintptr_t range_end;
};

static int
vma_iterate_callback (void *data, uintptr_t start, uintptr_t end,
                      unsigned int flags)
{
  struct locals *lp = (struct locals *) data;

  /* Remove from [range_start, range_end) the part at the beginning or at the
     end that is covered by [start, end).  */
  if (start <= lp->range_start && end > lp->range_start)
    lp->range_start = (end < lp->range_end ? end : lp->range_end);
  if (start < lp->range_end && end >= lp->range_end)
    lp->range_end = (start > lp->range_start ? start : lp->range_start);

  return 0;
}

static bool
is_range_mapped (uintptr_t range_start, uintptr_t range_end)
{
  struct locals l;

  l.range_start = range_start;
  l.range_end = range_end;
  vma_iterate (vma_iterate_callback, &l);
  return l.range_start == l.range_end;
}

#else

static bool
is_range_mapped (uintptr_t range_start, uintptr_t range_end)
{
  return true;
}

#endif

static void
test_heap (void)
{
  char *heapbuf = (char *) malloc (SECRET_SIZE);
  uintptr_t addr = (uintptr_t) heapbuf;
  memcpy (heapbuf, SECRET, SECRET_SIZE);
  explicit_bzero (heapbuf, SECRET_SIZE);
  free (heapbuf);
  if (is_range_mapped (addr, addr + SECRET_SIZE))
    {
      /* some implementation could override freed memory by canaries so
         compare against secret */
      ASSERT (memcmp (heapbuf, SECRET, SECRET_SIZE) != 0);
      printf ("test_heap: address range is still mapped after free().\n");
    }
  else
    printf ("test_heap: address range is unmapped after free().\n");
}

/* =============== Verify operation on stack-allocated memory =============== */

/* There are two passes:
     1. Put a secret in memory and invoke explicit_bzero on it.
     2. Verify that the memory has been erased.
   Implement them in the same function, so that they access the same memory
   range on the stack.  */
static int _GL_ATTRIBUTE_NOINLINE
do_secret_stuff (volatile int pass)
{
  char stackbuf[SECRET_SIZE];
  if (pass == 1)
    {
      memcpy (stackbuf, SECRET, SECRET_SIZE);
      explicit_bzero (stackbuf, SECRET_SIZE);
      return 0;
    }
  else /* pass == 2 */
    {
      return memcmp (zero, stackbuf, SECRET_SIZE) != 0;
    }
}

static void
test_stack (void)
{
  int count = 0;
  int repeat;

  for (repeat = 1000; repeat > 0; repeat--)
    {
      do_secret_stuff (1);
      count += do_secret_stuff (2);
    }
  /* If explicit_bzero works, count is near 0.  (It may be > 0 if there were
     some asynchronous signal invocations between the two calls of
     do_secret_stuff.)
     If explicit_bzero is optimized away by the compiler, count comes out as
     approximately 1000.  */
  printf ("test_stack: count = %d\n", count);
  ASSERT (count < 50);
}

/* ========================================================================== */

int
main ()
{
  test_static ();
  test_heap ();
  test_stack ();

  return 0;
}
