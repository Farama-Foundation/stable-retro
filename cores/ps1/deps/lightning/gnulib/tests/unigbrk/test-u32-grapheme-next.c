/* Next grapheme cluster length test.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "macros.h"

static void
test_u32_grapheme_next (size_t len, ...)
{
  const uint32_t *next;
  uint32_t s[32];
  va_list args;
  size_t n;

  va_start (args, len);
  n = 0;
  for (;;)
    {
      int unit = va_arg (args, int);
      if (unit == -1)
        break;
      else if (n >= sizeof s / sizeof *s)
        abort ();

      s[n++] = unit;
    }
  va_end (args);

  next = u32_grapheme_next (s, s + n);
  if (next != s + len)
    {
      size_t i;

      if (next == NULL)
        fputs ("u32_grapheme_next returned NULL", stderr);
      else
        fprintf (stderr, "u32_grapheme_next skipped %zu units", next - s);
      fprintf (stderr, ", expected %zu:\n", len);
      for (i = 0; i < n; i++)
        fprintf (stderr, " %04x", s[i]);
      putc ('\n', stderr);
      abort ();
    }
}

int
main (void)
{
  static const uint32_t s[] = { 'a', 'b', 'c' };

  /* Empty string. */
  ASSERT (u32_grapheme_next (NULL, NULL) == NULL);
  ASSERT (u32_grapheme_next (s, s) == NULL);

  /* Standalone 1-unit graphemes.  */
  test_u32_grapheme_next (1, 'a', -1);
  test_u32_grapheme_next (1, 'a', 'b', -1);
  test_u32_grapheme_next (1, 'a', 'b', 'c', -1);

  /* Multi-unit, single code point graphemes. */
#define HIRAGANA_A 0x3042       /* あ: Hiragana letter 'a'. */
  test_u32_grapheme_next (1, HIRAGANA_A, -1);
  test_u32_grapheme_next (1, HIRAGANA_A, 'x', -1);
  test_u32_grapheme_next (1, HIRAGANA_A, HIRAGANA_A, -1);

  /* Combining accents. */
#define GRAVE 0x0300            /* Combining grave accent. */
#define ACUTE 0x0301            /* Combining acute accent. */
  test_u32_grapheme_next (2, 'e', ACUTE, -1);
  test_u32_grapheme_next (3, 'e', ACUTE, GRAVE, -1);
  test_u32_grapheme_next (2, 'e', ACUTE, 'x', -1);
  test_u32_grapheme_next (2, 'e', ACUTE, 'e', ACUTE, -1);

  /* Outside BMP. */
#define NEUTRAL_FACE 0x1f610    /* 😐: neutral face. */
  test_u32_grapheme_next (1, NEUTRAL_FACE, -1);
  test_u32_grapheme_next (2, NEUTRAL_FACE, GRAVE, -1);

  return 0;
}
