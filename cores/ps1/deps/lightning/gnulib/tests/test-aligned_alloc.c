/* Test of allocating memory with given alignment.

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
#include <stdlib.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

#define ROUNDUP(x,y) (((x) + (y) - 1) & - (y))

int
main (int argc, char *argv[])
{
#if HAVE_ALIGNED_ALLOC
  static size_t sizes[] =
    { 13, 8, 17, 450, 320, 1, 99, 4, 15, 16, 2, 76, 37, 127, 2406, 641, 5781 };
  void *aligned2_blocks[SIZEOF (sizes)];
  void *aligned4_blocks[SIZEOF (sizes)];
  void *aligned8_blocks[SIZEOF (sizes)];
  void *aligned16_blocks[SIZEOF (sizes)];
  void *aligned32_blocks[SIZEOF (sizes)];
  void *aligned64_blocks[SIZEOF (sizes)];
  size_t i;

  for (i = 0; i < SIZEOF (sizes); i++)
    {
      size_t size = sizes[i];

      aligned2_blocks[i] = aligned_alloc (2, ROUNDUP (size, 2));
      ASSERT (aligned2_blocks[i] != NULL);
      ASSERT (((uintptr_t) aligned2_blocks[i] % 2) == 0);
      memset (aligned2_blocks[i], 'u', size);

      aligned4_blocks[i] = aligned_alloc (4, ROUNDUP (size, 4));
      ASSERT (aligned4_blocks[i] != NULL);
      ASSERT (((uintptr_t) aligned4_blocks[i] % 4) == 0);
      memset (aligned4_blocks[i], 'v', size);

      aligned8_blocks[i] = aligned_alloc (8, ROUNDUP (size, 8));
      ASSERT (aligned8_blocks[i] != NULL);
      ASSERT (((uintptr_t) aligned8_blocks[i] % 8) == 0);
      memset (aligned8_blocks[i], 'w', size);

      aligned16_blocks[i] = aligned_alloc (16, ROUNDUP (size, 16));
      ASSERT (aligned16_blocks[i] != NULL);
      ASSERT (((uintptr_t) aligned16_blocks[i] % 16) == 0);
      memset (aligned16_blocks[i], 'x', size);

      aligned32_blocks[i] = aligned_alloc (32, ROUNDUP (size, 32));
      ASSERT (aligned32_blocks[i] != NULL);
      ASSERT (((uintptr_t) aligned32_blocks[i] % 32) == 0);
      memset (aligned32_blocks[i], 'y', size);

      aligned64_blocks[i] = aligned_alloc (64, ROUNDUP (size, 64));
      ASSERT (aligned64_blocks[i] != NULL);
      ASSERT (((uintptr_t) aligned64_blocks[i] % 64) == 0);
      memset (aligned64_blocks[i], 'z', size);
    }

  for (i = 0; i < SIZEOF (sizes); i++)
    {
      free (aligned2_blocks[i]);
      free (aligned4_blocks[i]);
      free (aligned8_blocks[i]);
      free (aligned16_blocks[i]);
      free (aligned32_blocks[i]);
      free (aligned64_blocks[i]);
    }

  return 0;
#else
  fputs ("Skipping test: function 'aligned_alloc' does not exist\n", stderr);
  return 77;
#endif
}
