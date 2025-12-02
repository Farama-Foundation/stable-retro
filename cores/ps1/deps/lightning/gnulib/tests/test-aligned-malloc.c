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

#include <stdint.h>
#include <stdlib.h>

#define ALIGNMENT 4
#define aligned_malloc aligned4_malloc
#define aligned_free aligned4_free
#include "aligned-malloc.h"
#undef aligned_free
#undef aligned_malloc
#undef ALIGNMENT

#define ALIGNMENT 8
#define aligned_malloc aligned8_malloc
#define aligned_free aligned8_free
#include "aligned-malloc.h"
#undef aligned_free
#undef aligned_malloc
#undef ALIGNMENT

#define ALIGNMENT 16
#define aligned_malloc aligned16_malloc
#define aligned_free aligned16_free
#include "aligned-malloc.h"
#undef aligned_free
#undef aligned_malloc
#undef ALIGNMENT

#define ALIGNMENT 32
#define aligned_malloc aligned32_malloc
#define aligned_free aligned32_free
#include "aligned-malloc.h"
#undef aligned_free
#undef aligned_malloc
#undef ALIGNMENT

#include <string.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  static size_t sizes[] =
    { 13, 8, 17, 450, 320, 1, 99, 4, 15, 16, 2, 76, 37, 127, 2406, 641 };
  void *aligned4_blocks[SIZEOF (sizes)];
  void *aligned8_blocks[SIZEOF (sizes)];
  void *aligned16_blocks[SIZEOF (sizes)];
  void *aligned32_blocks[SIZEOF (sizes)];
  size_t i;

  for (i = 0; i < SIZEOF (sizes); i++)
    {
      size_t size = sizes[i];

      aligned4_blocks[i] = aligned4_malloc (size);
      ASSERT (((uintptr_t) aligned4_blocks[i] % 4) == 0);
      memset (aligned4_blocks[i], 'w', size);

      aligned8_blocks[i] = aligned8_malloc (size);
      ASSERT (((uintptr_t) aligned8_blocks[i] % 8) == 0);
      memset (aligned8_blocks[i], 'x', size);

      aligned16_blocks[i] = aligned16_malloc (size);
      ASSERT (((uintptr_t) aligned16_blocks[i] % 16) == 0);
      memset (aligned16_blocks[i], 'y', size);

      aligned32_blocks[i] = aligned32_malloc (size);
      ASSERT (((uintptr_t) aligned32_blocks[i] % 32) == 0);
      memset (aligned32_blocks[i], 'z', size);
    }

  for (i = 0; i < SIZEOF (sizes); i++)
    {
      aligned4_free (aligned4_blocks[i]);
      aligned8_free (aligned8_blocks[i]);
      aligned16_free (aligned16_blocks[i]);
      aligned32_free (aligned32_blocks[i]);
    }

  return 0;
}
