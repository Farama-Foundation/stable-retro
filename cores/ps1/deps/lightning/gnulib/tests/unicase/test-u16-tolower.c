/* Test of lowercase mapping for UTF-16 strings.
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
  result = u16_tolower (input, input_length, iso639_language, nf, NULL, &length);
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
      result = u16_tolower (input, input_length, iso639_language, nf, preallocated, &length);
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
    result = u16_tolower (input, input_length, iso639_language, nf, preallocated, &length);
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
    static const uint16_t casemapped[] =
      { 'g', 'r', 0x00FC, 0x00DF, ' ', 'g', 'o', 't', 't', '.', ' ',
        0x0437, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Turkish letters i İ ı I */
  { /* LATIN CAPITAL LETTER I */
    static const uint16_t input[]         = { 0x0049 };
    static const uint16_t casemapped[]    = { 0x0069 };
    static const uint16_t casemapped_tr[] = { 0x0131 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped_tr, SIZEOF (casemapped_tr)) == 0);
  }
  { /* LATIN SMALL LETTER I */
    static const uint16_t input[]      = { 0x0069 };
    static const uint16_t casemapped[] = { 0x0069 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    static const uint16_t input[]         = { 0x0130 };
    static const uint16_t casemapped[]    = { 0x0069, 0x0307 };
    static const uint16_t casemapped_tr[] = { 0x0069 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped_tr, SIZEOF (casemapped_tr)) == 0);
  }
  { /* LATIN SMALL LETTER DOTLESS I */
    static const uint16_t input[]      = { 0x0131 };
    static const uint16_t casemapped[] = { 0x0131 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "TOPKAPI" */
    static const uint16_t input[] =
      { 0x0054, 0x004F, 0x0050, 0x004B, 0x0041, 0x0050, 0x0049 };
    static const uint16_t casemapped[] =
      { 0x0074, 0x006F, 0x0070, 0x006B, 0x0061, 0x0070, 0x0131 };
    ASSERT (check (input, SIZEOF (input), "tr", NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "HEIß" */
    static const uint16_t input[]      = { 0x0048, 0x0045, 0x0049, 0x00DF };
    static const uint16_t casemapped[] = { 0x0068, 0x0065, 0x0069, 0x00DF };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  /* Case mappings for some characters can depend on the surrounding characters.  */
  { /* "ΠΕΡΙΣΣΌΤΕΡΕΣ ΠΛΗΡΟΦΟΡΊΕΣ" */
    static const uint16_t input[] =
      {
        0x03A0, 0x0395, 0x03A1, 0x0399, 0x03A3, 0x03A3, 0x038C, 0x03A4,
        0x0395, 0x03A1, 0x0395, 0x03A3, 0x0020, 0x03A0, 0x039B, 0x0397,
        0x03A1, 0x039F, 0x03A6, 0x039F, 0x03A1, 0x038A, 0x0395, 0x03A3
      };
    static const uint16_t casemapped[] =
      {
        0x03C0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C2, 0x0020, 0x03C0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C2
      };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "Σ" -> "σ" */
    static const uint16_t input[] =      { 0x03A3 };
    static const uint16_t casemapped[] = { 0x03C3 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "ΑΣ" -> "ας" */
    static const uint16_t input[] =      { 0x0391, 0x03A3 };
    static const uint16_t casemapped[] = { 0x03B1, 0x03C2 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  /* It's a final sigma only if not followed by a case-ignorable sequence and
     then a cased letter.  Note that U+0345 and U+037A are simultaneously
     case-ignorable and cased (which is a bit paradoxical).  */
  { /* "ΑΣΑ" -> "ασα" */
    static const uint16_t input[] =      { 0x0391, 0x03A3, 0x0391 };
    static const uint16_t casemapped[] = { 0x03B1, 0x03C3, 0x03B1 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "ΑΣ:" -> "ας:" */
    static const uint16_t input[] =      { 0x0391, 0x03A3, 0x003A };
    static const uint16_t casemapped[] = { 0x03B1, 0x03C2, 0x003A };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "ΑΣ:Α" -> "ασ:α" */
    static const uint16_t input[] =      { 0x0391, 0x03A3, 0x003A, 0x0391 };
    static const uint16_t casemapped[] = { 0x03B1, 0x03C3, 0x003A, 0x03B1 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "ΑΣ:ͺ" -> "ασ:ͺ" */
    static const uint16_t input[] =      { 0x0391, 0x03A3, 0x003A, 0x037A };
    static const uint16_t casemapped[] = { 0x03B1, 0x03C3, 0x003A, 0x037A };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "ΑΣ:ͺ " -> "ασ:ͺ " */
    static const uint16_t input[] =      { 0x0391, 0x03A3, 0x003A, 0x037A, 0x0020 };
    static const uint16_t casemapped[] = { 0x03B1, 0x03C3, 0x003A, 0x037A, 0x0020 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  /* It's a final sigma only if preceded by a case-ignorable sequence and
     a cased letter before it.  Note that U+0345 and U+037A are simultaneously
     case-ignorable and cased (which is a bit paradoxical).  */
  { /* ":Σ" -> ":σ" */
    static const uint16_t input[] =      { 0x003A, 0x03A3 };
    static const uint16_t casemapped[] = { 0x003A, 0x03C3 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "Α:Σ" -> "α:ς" */
    static const uint16_t input[] =      { 0x0391, 0x003A, 0x03A3 };
    static const uint16_t casemapped[] = { 0x03B1, 0x003A, 0x03C2 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* "ͺ:Σ" -> "ͺ:ς" */
    static const uint16_t input[] =      { 0x037A, 0x003A, 0x03A3 };
    static const uint16_t casemapped[] = { 0x037A, 0x003A, 0x03C2 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }
  { /* " ͺ:Σ" -> " ͺ:ς" */
    static const uint16_t input[] =      { 0x0020, 0x037A, 0x003A, 0x03A3 };
    static const uint16_t casemapped[] = { 0x0020, 0x037A, 0x003A, 0x03C2 };
    ASSERT (check (input, SIZEOF (input), NULL, NULL, casemapped, SIZEOF (casemapped)) == 0);
  }

  return 0;
}
