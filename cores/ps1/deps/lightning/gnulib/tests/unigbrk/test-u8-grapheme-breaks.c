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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

static void
test_u8_grapheme_breaks (const char *input, const char *expected)
{
  const uint8_t *s = (const uint8_t *) input;
  size_t n = strlen (expected);
  char *breaks;
  size_t i;

  breaks = malloc (n);
  if (!breaks)
    abort ();
  memset (breaks, 0xcc, n);

  u8_grapheme_breaks (s, n, breaks);
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

  free (breaks);
}

int
main (void)
{
  /* Standalone 1-unit graphemes.  */
  test_u8_grapheme_breaks ("a", "#");
  test_u8_grapheme_breaks ("ab", "##");
  test_u8_grapheme_breaks ("abc", "###");

  /* Multi-unit, single code point graphemes. */
#define HIRAGANA_A "\343\201\202" /* あ: Hiragana letter 'a'. */
  test_u8_grapheme_breaks (HIRAGANA_A, "#__");
  test_u8_grapheme_breaks (HIRAGANA_A"x", "#__#");
  test_u8_grapheme_breaks (HIRAGANA_A HIRAGANA_A, "#__#__");

  /* Combining accents. */
#define GRAVE "\314\200"        /* Combining grave accent. */
#define ACUTE "\314\201"        /* Combining acute accent. */
  test_u8_grapheme_breaks ("e"ACUTE, "#__");
  test_u8_grapheme_breaks ("e"ACUTE GRAVE, "#____");
  test_u8_grapheme_breaks ("e"ACUTE"x", "#__#");
  test_u8_grapheme_breaks ("e"ACUTE "e"ACUTE, "#__#__");

  return 0;
}
