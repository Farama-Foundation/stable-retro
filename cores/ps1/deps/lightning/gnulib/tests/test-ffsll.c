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

#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (ffsll, int, (long long int));

#include <limits.h>

#include "macros.h"

#define NBITS (sizeof (long long int) * CHAR_BIT)

static int
naive (long long int i)
{
  unsigned long long int j;
  for (j = 0; j < NBITS; j++)
    if (i & (1ULL << j))
      return j + 1;
  return 0;
}

int
main (int argc, char *argv[])
{
  long long int x;
  int i;

  for (x = -128; x <= 128; x++)
    ASSERT (ffsll (x) == naive (x));
  for (i = 0; i < NBITS; i++)
    {
      ASSERT (ffsll (1ULL << i) == naive (1ULL << i));
      ASSERT (ffsll (1ULL << i) == i + 1);
      ASSERT (ffsll (-1ULL << i) == i + 1);
    }
  for (i = 0; i < NBITS - 1; i++)
    {
      ASSERT (ffsll (3ULL << i) == i + 1);
      ASSERT (ffsll (-3ULL << i) == i + 1);
    }
  for (i = 0; i < NBITS - 2; i++)
    {
      ASSERT (ffsll (5ULL << i) == i + 1);
      ASSERT (ffsll (-5ULL << i) == i + 1);
      ASSERT (ffsll (7ULL << i) == i + 1);
      ASSERT (ffsll (-7ULL << i) == i + 1);
    }
  return 0;
}
