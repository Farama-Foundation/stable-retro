/* Grapheme cluster breaks test.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff <blp@cs.stanford.edu>, 2010. */

#include <config.h>

/* Specification. */
#include <unigbrk.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

static void
test_u32_grapheme_breaks (const char *expected, ...)
{
  size_t n = strlen (expected);
  uint32_t s[16];
  va_list args;
  char breaks[16];
  size_t i;

  ASSERT (n <= 16);

  memset (breaks, 0xcc, n);

  va_start (args, expected);
  for (i = 0; i < n; i++)
    {
      int unit = va_arg (args, int);
      ASSERT (unit >= 0);
      s[i] = unit;
    }
  ASSERT (va_arg (args, int) == -1);
  va_end (args);

  u32_grapheme_breaks (s, n, breaks);
  for (i = 0; i < n; i++)
    if (breaks[i] != (expected[i] == '#'))
      {
        size_t j;

        fprintf (stderr, "wrong grapheme breaks:\n");

        fprintf (stderr, "   input:");
        for (j = 0; j < n; j++)
          fprintf (stderr, " %02x", s[j]);
        putc ('\n', stderr);

        fprintf (stderr, "expected:");
        for (j = 0; j < n; j++)
          fprintf (stderr, "  %d", expected[j] == '#');
        putc ('\n', stderr);

        fprintf (stderr, "  actual:");
        for (j = 0; j < n; j++)
          fprintf (stderr, "  %d", breaks[j]);
        putc ('\n', stderr);

        abort ();
      }
}

int
main (void)
{
  /* Standalone 1-unit graphemes.  */
  test_u32_grapheme_breaks ("#", 'a', -1);
  test_u32_grapheme_breaks ("##", 'a', 'b', -1);
  test_u32_grapheme_breaks ("###", 'a', 'b', 'c', -1);

#define HIRAGANA_A 0x3042       /* あ: Hiragana letter 'a'. */
  test_u32_grapheme_breaks ("#", HIRAGANA_A, -1);
  test_u32_grapheme_breaks ("##", HIRAGANA_A, 'x', -1);
  test_u32_grapheme_breaks ("##", HIRAGANA_A, HIRAGANA_A, -1);

  /* Combining accents. */
#define GRAVE 0x0300            /* Combining grave accent. */
#define ACUTE 0x0301            /* Combining acute accent. */
  test_u32_grapheme_breaks ("#_", 'e', ACUTE, -1);
  test_u32_grapheme_breaks ("#__", 'e', ACUTE, GRAVE, -1);
  test_u32_grapheme_breaks ("#_#", 'e', ACUTE, 'x', -1);
  test_u32_grapheme_breaks ("#_#_", 'e', ACUTE, 'e', GRAVE, -1);

  return 0;
}
