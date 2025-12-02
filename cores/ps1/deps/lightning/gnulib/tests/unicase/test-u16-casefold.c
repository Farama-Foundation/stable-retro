/* Test of casefolding mapping for UTF-16 strings.
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
check (const uint16_t *input, size_t input_length,
       const char *iso639_language, uninorm_t nf,
       const uint16_t *expected, size_t expected_length)
{
  size_t length;
  uint16_t *result;

  /* Test return conventions with resultbuf == NULL.  */
  result = u16_casefold (input, input_length, iso639_language, nf, NULL, &length);
  if (!(result != NULL))
    return 1;
  if (!(length == expected_length))
    return 2;
  if (!(u16_cmp (result, expected, expected_length) == 0))
    return 3;
  free (result);

  /* Test return conventions with resultbuf too small.  */
  if (expected_length > 0)
    {
      uint16_t *preallocated;

      length = expected_length - 1;
      preallocated = (uint16_t *) malloc (length * sizeof (uint16_t));
      result = u16_casefold (input, input_length, iso639_language, nf, preallocated, &length);
      if (!(result != NULL))
        return 4;
      if (!(result != preallocated))
        return 5;
      if (!(length == expected_length))
        return 6;
      if (!(u16_cmp (result, expected, expected_length) == 0))
        return 7;
      free (result);
      free (preallocated);
    }

  /* Test return conventions with resultbuf large enough.  */
  {
    uint16_t *preallocated;

    length = expected_length;
    preallocated = (uint16_t *) malloc (length * sizeof (uint16_t));
    result = u16_casefold (input, input_length, iso639_language, nf, preallocated, &length);
    if (!(result != NULL))
      return 8;
    if (!(preallocated == NULL || result == preallocated))
      return 9;
    if (!(length == expected_length))
      return 10;
    if (!(u16_cmp (result, expected, expected_length) == 0))
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
  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint16_t input[] =
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    static const uint16_t casefolded[] =
      { 'g', 'r', 0x00FC, 0x0073, 0x0073, ' ', 'g', 'o', 't', 't', '.', ' ',
        0x0437, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
  }

  /* Case mapping can increase the number of Unicode characters.  */
  { /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    static const uint16_t input[]      = { 0x0149 };
    static const uint16_t casefolded[] = { 0x02BC, 0x006E };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
  }
  { /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    static const uint16_t input[]      = { 0x0390 };
    static const uint16_t casefolded[] = { 0x03B9, 0x0308, 0x0301 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
  }

  /* Turkish letters i İ ı I */
  { /* LATIN CAPITAL LETTER I */
    static const uint16_t input[]         = { 0x0049 };
    static const uint16_t casefolded[]    = { 0x0069 };
    static const uint16_t casefolded_tr[] = { 0x0131 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casefolded_tr, SIZEOF (casefolded_tr)) == 0);
  }
  { /* LATIN SMALL LETTER I */
    static const uint16_t input[]      = { 0x0069 };
    static const uint16_t casefolded[] = { 0x0069 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casefolded, SIZEOF (casefolded)) == 0);
  }
  { /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    static const uint16_t input[]         = { 0x0130 };
    static const uint16_t casefolded[]    = { 0x0069, 0x0307 };
    static const uint16_t casefolded_tr[] = { 0x0069 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casefolded_tr, SIZEOF (casefolded_tr)) == 0);
  }
  { /* LATIN SMALL LETTER DOTLESS I */
    static const uint16_t input[]      = { 0x0131 };
    static const uint16_t casefolded[] = { 0x0131 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casefolded, SIZEOF (casefolded)) == 0);
  }
  { /* "topkapı" */
    static const uint16_t input[] =
      { 0x0074, 0x006F, 0x0070, 0x006B, 0x0061, 0x0070, 0x0131 };
    static const uint16_t casefolded[] =
      { 0x0074, 0x006F, 0x0070, 0x006B, 0x0061, 0x0070, 0x0131 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casefolded, SIZEOF (casefolded)) == 0);
  }

  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "heiß" */
    static const uint16_t input[]      = { 0x0068, 0x0065, 0x0069, 0x00DF };
    static const uint16_t casefolded[] = { 0x0068, 0x0065, 0x0069, 0x0073, 0x0073 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
  }

  /* Case mappings for some characters can depend on the surrounding characters.  */
  { /* "περισσότερες πληροφορίες" */
    static const uint16_t input[] =
      {
        0x03C0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C2, 0x0020, 0x03C0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C2
      };
    static const uint16_t casefolded[] =
      {
        0x03C0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C3, 0x0020, 0x03C0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C3
      };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casefolded, SIZEOF (casefolded)) == 0);
  }

  /* Case mapping can require subsequent normalization.  */
  { /* LATIN SMALL LETTER J WITH CARON, COMBINING DOT BELOW */
    static const uint16_t input[]                 = { 0x01F0, 0x0323 };
    static const uint16_t casefolded[]            = { 0x006A, 0x030C, 0x0323 };
    static const uint16_t casefolded_decomposed[] = { 0x006A, 0x0323, 0x030C };
    static const uint16_t casefolded_normalized[] = { 0x01F0, 0x0323 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL,        casefolded, SIZEOF (casefolded)) == 0);
    ASSERT (check (input, SIZEOF (input), NULL, UNINORM_NFD, casefolded_decomposed, SIZEOF (casefolded_decomposed)) == 0);
    ASSERT (check (input, SIZEOF (input), NULL, UNINORM_NFC, casefolded_normalized, SIZEOF (casefolded_normalized)) == 0);
  }

  return 0;
}
