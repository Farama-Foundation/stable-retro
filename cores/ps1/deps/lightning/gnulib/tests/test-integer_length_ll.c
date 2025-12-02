/* Test of integer_length_ll().
   Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

#include <config.h>

#include "integer_length.h"

#include <limits.h>

#include "macros.h"

#define NBITS (sizeof (unsigned long long) * CHAR_BIT)

static int
naive (unsigned long long x)
{
  int j;
  for (j = NBITS - 1; j >= 0; j--)
    if (x & (1ULL << j))
      return j + 1;
  return 0;
}

int
main (int argc, char *argv[])
{
  unsigned long x;
  int i;

  for (x = 0; x <= 256; x++)
    ASSERT (integer_length_ll (x) == naive (x));
  for (i = 0; i < NBITS; i++)
    {
      ASSERT (integer_length_ll (1ULL << i) == naive (1ULL << i));
      ASSERT (integer_length_ll (1ULL << i) == i + 1);
      ASSERT (integer_length_ll (-1ULL << i) == NBITS);
    }
  for (i = 0; i < NBITS - 1; i++)
    ASSERT (integer_length_ll (3ULL << i) == i + 2);
  for (i = 0; i < NBITS - 2; i++)
    ASSERT (integer_length_ll (-3ULL << i) == NBITS);
  for (i = 0; i < NBITS - 2; i++)
    {
      ASSERT (integer_length_ll (5ULL << i) == i + 3);
      ASSERT (integer_length_ll (7ULL << i) == i + 3);
    }
  for (i = 0; i < NBITS - 3; i++)
    {
      ASSERT (integer_length_ll (-5ULL << i) == NBITS);
      ASSERT (integer_length_ll (-7ULL << i) == NBITS);
    }
  return 0;
}
