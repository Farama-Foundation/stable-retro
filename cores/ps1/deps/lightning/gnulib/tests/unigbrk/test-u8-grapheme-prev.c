/* Previous grapheme cluster test.
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
#include <stdlib.h>

#include "macros.h"

static void
test_u8_grapheme_prev (const char *input, size_t n, size_t len)
{
  const uint8_t *s = (const uint8_t *) input;
  const uint8_t *end = s + n;
  const uint8_t *prev = u8_grapheme_prev (end, s);
  if (prev != end - len)
    {
      size_t i;

      if (prev == NULL)
        fputs ("u8_grapheme_prev returned NULL", stderr);
      else
        fprintf (stderr, "u8_grapheme_prev skipped %zu bytes", end - prev);
      fprintf (stderr, ", expected %zu:\n", len);
      for (i = 0; i < n; i++)
        fprintf (stderr, " %02x", s[i]);
      putc ('\n', stderr);
      abort ();
    }
}

int
main (void)
{
  static const uint8_t s[] = "abc";

  /* Empty string. */
  ASSERT (u8_grapheme_prev (NULL, NULL) == NULL);
  ASSERT (u8_grapheme_prev (s, s) == NULL);

  /* Standalone 1-unit graphemes.  */
  test_u8_grapheme_prev ("a", 1, 1);
  test_u8_grapheme_prev ("ab", 2, 1);
  test_u8_grapheme_prev ("abc", 3, 1);

  /* Multi-unit, single code point graphemes. */
#define HIRAGANA_A "\343\201\202" /* あ: Hiragana letter 'a'. */
  test_u8_grapheme_prev (HIRAGANA_A, 3, 3);
  test_u8_grapheme_prev (HIRAGANA_A"x", 4, 1);
  test_u8_grapheme_prev (HIRAGANA_A HIRAGANA_A, 6, 3);

  /* Combining accents. */
#define GRAVE "\314\200"        /* Combining grave accent. */
#define ACUTE "\314\201"        /* Combining acute accent. */
  test_u8_grapheme_prev ("e"ACUTE, 3, 3);
  test_u8_grapheme_prev ("e"ACUTE GRAVE, 5, 5);
  test_u8_grapheme_prev ("e"ACUTE"x", 4, 1);
  test_u8_grapheme_prev ("e"ACUTE "e"ACUTE, 6, 3);

  return 0;
}
