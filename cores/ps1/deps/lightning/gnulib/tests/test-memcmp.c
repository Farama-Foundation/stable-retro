/*
 * Copyright (C) 2008-2021 Free Software Foundation, Inc.
 * Written by Simon Josefsson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (memcmp, int, (void const *, void const *, size_t));

#include "zerosize-ptr.h"
#include "macros.h"

int
main (void)
{
  int (* volatile memcmp_ptr) (const void *, const void *, size_t) = memcmp;

  /* Test equal / not equal distinction.  */
  void *page_boundary1 = zerosize_ptr ();
  void *page_boundary2 = zerosize_ptr ();
  if (page_boundary1 && page_boundary2)
    ASSERT (memcmp (page_boundary1, page_boundary2, 0) == 0);
  ASSERT (memcmp ("foo", "foobar", 2) == 0);
  ASSERT (memcmp ("foo", "foobar", 3) == 0);
  ASSERT (memcmp ("foo", "foobar", 4) != 0);
  ASSERT (memcmp ("foo", "bar", 1) != 0);
  ASSERT (memcmp ("foo", "bar", 3) != 0);

  /* Test less / equal / greater distinction.  */
  ASSERT (memcmp ("foo", "moo", 4) < 0);
  ASSERT (memcmp ("moo", "foo", 4) > 0);
  ASSERT (memcmp ("oomph", "oops", 3) < 0);
  ASSERT (memcmp ("oops", "oomph", 3) > 0);
  ASSERT (memcmp ("foo", "foobar", 4) < 0);
  ASSERT (memcmp ("foobar", "foo", 4) > 0);

  /* Some old versions of memcmp were not 8-bit clean.  */
  /* Use the function pointer here, because otherwise this test is sometimes
     miscompiled by 'clang'.
     See <https://bugs.llvm.org/show_bug.cgi?id=40063>.  */
  ASSERT (memcmp_ptr ("\100", "\201", 1) < 0);
  ASSERT (memcmp_ptr ("\201", "\100", 1) > 0);
  ASSERT (memcmp_ptr ("\200", "\201", 1) < 0);
  ASSERT (memcmp_ptr ("\201", "\200", 1) > 0);

  /* The Next x86 OpenStep bug shows up only when comparing 16 bytes
     or more and with at least one buffer not starting on a 4-byte boundary.
     William Lewis provided this test program.   */
  {
    char foo[21];
    char bar[21];
    int i;
    for (i = 0; i < 4; i++)
      {
        char *a = foo + i;
        char *b = bar + i;
        strcpy (a, "--------01111111");
        strcpy (b, "--------10000000");
        ASSERT (memcmp (a, b, 16) < 0);
      }
  }

  return 0;
}
