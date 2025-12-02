/* Test of titlecase mapping for UTF-32 strings.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "unicase.h"

#include <stdlib.h>

#include "unistr.h"
#include "uninorm.h"
#include "macros.h"

static int
check (const uint32_t *input, size_t input_length,
       const char *iso639_language, uninorm_t nf,
       const uint32_t *expected, size_t expected_length)
{
  size_t length;
  uint32_t *result;

  /* Test return conventions with resultbuf == NULL.  */
  result = u32_totitle (input, input_length, iso639_language, nf, NULL, &length);
  if (!(result != NULL))
    return 1;
  if (!(length == expected_length))
    return 2;
  if (!(u32_cmp (result, expected, expected_length) == 0))
    return 3;
  free (result);

  /* Test return conventions with resultbuf too small.  */
  if (expected_length > 0)
    {
      uint32_t *preallocated;

      length = expected_length - 1;
      preallocated = (uint32_t *) malloc (length * sizeof (uint32_t));
      result = u32_totitle (input, input_length, iso639_language, nf, preallocated, &length);
      if (!(result != NULL))
        return 4;
      if (!(result != preallocated))
        return 5;
      if (!(length == expected_length))
        return 6;
      if (!(u32_cmp (result, expected, expected_length) == 0))
        return 7;
      free (result);
      free (preallocated);
    }

  /* Test return conventions with resultbuf large enough.  */
  {
    uint32_t *preallocated;

    length = expected_length;
    preallocated = (uint32_t *) malloc (length * sizeof (uint32_t));
    result = u32_totitle (input, input_length, iso639_language, nf, preallocated, &length);
    if (!(result != NULL))
      return 8;
    if (!(preallocated == NULL || result == preallocated))
      return 9;
    if (!(length == expected_length))
      return 10;
    if (!(u32_cmp (result, expected, expected_length) == 0))
      return 11;
    free (preallocated);
  }

  return 0;
}

int
main ()
{
  { /* Empty string.  */
    ASSERT (check (NULL, 0, NULL, NULL, NULL, 0) == 0);
    ASSERT (check (NULL, 0, NULL, UNINORM_NFC, NULL, 0) == 0);
  }

  /* Simple string.  */
  { /* "GRÜß GOTT. ЗДРАВСТВУЙТЕ! X=(-B±SQRT(B²-4AC))/(2A)  日本語,中文,한글" */
    static const uint32_t input[] =
      { 'G', 'R', 0x00DC, 0x00DF, ' ', 'G', 'O', 'T', 'T', '.', ' ',
        0x0417, 0x0414, 0x0420, 0x0410, 0x0412, 0x0421, 0x0422, 0x0412, 0x0423,
        0x0419, 0x0422, 0x0415, '!', ' ',
        'X', '=', '(', '-', 'B', 0x00B1, 'S', 'Q', 'R', 'T', '(', 'B', 0x00B2,
        '-', '4', 'A', 'C', ')', ')', '/', '(', '2', 'A', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    static const uint32_t casemapped[] =
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'X', '=', '(', '-', 'B', 0x00B1, 'S', 'q', 'r', 't', '(', 'B', 0x00B2,
        '-', '4', 'A', 'c', ')', ')', '/', '(', '2', 'A', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Case mapping can increase the number of Unicode characters.  */
  { /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    static const uint32_t input[]      = { 0x0149 };
    static const uint32_t casemapped[] = { 0x02BC, 0x004E };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    static const uint32_t input[]      = { 0x0390 };
    static const uint32_t casemapped[] = { 0x0399, 0x0308, 0x0301 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Turkish letters i İ ı I */
  { /* LATIN CAPITAL LETTER I */
    static const uint32_t input[]      = { 0x0049 };
    static const uint32_t casemapped[] = { 0x0049 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* LATIN SMALL LETTER I */
    static const uint32_t input[]         = { 0x0069 };
    static const uint32_t casemapped[]    = { 0x0049 };
    static const uint32_t casemapped_tr[] = { 0x0130 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped_tr, SIZEOF (casemapped_tr)) == 0);
  }
  { /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    static const uint32_t input[]      = { 0x0130 };
    static const uint32_t casemapped[] = { 0x0130 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* LATIN SMALL LETTER DOTLESS I */
    static const uint32_t input[]      = { 0x0131 };
    static const uint32_t casemapped[] = { 0x0049 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "topkapı" */
    static const uint32_t input[] =
      { 0x0074, 0x006F, 0x0070, 0x006B, 0x0061, 0x0070, 0x0131 };
    static const uint32_t casemapped[] =
      { 0x0054, 0x006F, 0x0070, 0x006B, 0x0061, 0x0070, 0x0131 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "heiß" */
    static const uint32_t input[]      = { 0x0068, 0x0065, 0x0069, 0x00DF };
    static const uint32_t casemapped[] = { 0x0048, 0x0065, 0x0069, 0x00DF };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Case mappings for some characters can depend on the surrounding characters.  */
  { /* "περισσότερες πληροφορίες" */
    static const uint32_t input[] =
      {
        0x03C0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C2, 0x0020, 0x03C0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C2
      };
    static const uint32_t casemapped[] =
      {
        0x03A0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C2, 0x0020, 0x03A0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C2
      };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Case mapping can require subsequent normalization.  */
  { /* LATIN SMALL LETTER J WITH CARON, COMBINING DOT BELOW */
    static const uint32_t input[]                 = { 0x01F0, 0x0323 };
    static const uint32_t casemapped[]            = { 0x004A, 0x030C, 0x0323 };
    static const uint32_t casemapped_normalized[] = { 0x004A, 0x0323, 0x030C };
    ASSERT (check (input, SIZEOF (input), NULL, NULL,        casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), NULL, UNINORM_NFC, casemapped_normalized, SIZEOF (casemapped_normalized)) == 0);
  }

  return 0;
}
