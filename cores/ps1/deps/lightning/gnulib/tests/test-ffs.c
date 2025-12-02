/*
 * Copyright (C) 2011-2021 Free Software Foundation, Inc.
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

/* Written by Eric Blake.  */
#include <config.h>

#include <strings.h>

#include "signature.h"
SIGNATURE_CHECK (ffs, int, (int));

#include <limits.h>

#include "macros.h"

#define NBITS (sizeof (int) * CHAR_BIT)

static int
naive (int i)
{
  unsigned int j;
  for (j = 0; j < NBITS; j++)
    if (i & (1U << j))
      return j + 1;
  return 0;
}

int
main (int argc, char *argv[])
{
  int x;
  int i;

  for (x = -128; x <= 128; x++)
    ASSERT (ffs (x) == naive (x));
  for (i = 0; i < NBITS; i++)
    {
      ASSERT (ffs (1U << i) == naive (1U << i));
      ASSERT (ffs (1U << i) == i + 1);
      ASSERT (ffs (-1U << i) == i + 1);
    }
  for (i = 0; i < NBITS - 1; i++)
    {
      ASSERT (ffs (3U << i) == i + 1);
      ASSERT (ffs (-3U << i) == i + 1);
    }
  for (i = 0; i < NBITS - 2; i++)
    {
      ASSERT (ffs (5U << i) == i + 1);
      ASSERT (ffs (-5U << i) == i + 1);
      ASSERT (ffs (7U << i) == i + 1);
      ASSERT (ffs (-7U << i) == i + 1);
    }
  return 0;
}
