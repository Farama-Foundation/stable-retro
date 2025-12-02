/* Test of case and normalization insensitive comparison of UTF-16 strings.
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

#include "uninorm.h"
#include "macros.h"

#define UNIT uint16_t
#include "test-casecmp.h"
#undef UNIT

static void
test_nonascii (int (*my_casecmp) (const uint16_t *, size_t, const uint16_t *, size_t, const char *, uninorm_t, int *))
{
  /* Normalization effects.  */
  {
    static const uint16_t input1[] = { 'H', 0x00F6, 'h', 'l', 'e' };
    static const uint16_t input2[] = { 'H', 'O', 0x0308, 'h', 'L', 'e' };
    static const uint16_t input3[] = { 'H', 0x00F6, 'h', 'l', 'e', 'n' };
    static const uint16_t input4[] = { 'H', 'O', 0x0308, 'h', 'L', 'e', 'n' };
    static const uint16_t input5[] = { 'H', 'u', 'r', 'z' };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input3, SIZEOF (input3), input4, SIZEOF (input4), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input4, SIZEOF (input4), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input4, SIZEOF (input4), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input5, SIZEOF (input5), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input5, SIZEOF (input5), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    static const uint16_t input1[] = { 0x00C4 };
    static const uint16_t input2[] = { 0x0041, 0x0308 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    static const uint16_t input1[] = { 0x01DE };
    static const uint16_t input2[] = { 0x0041, 0x0308, 0x0304 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* GREEK DIALYTIKA AND PERISPOMENI */
    static const uint16_t input1[] = { 0x1FC1 };
    static const uint16_t input2[] = { 0x00A8, 0x0342 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* HANGUL SYLLABLE GEUL */
    static const uint16_t input1[] = { 0xAE00 };
    static const uint16_t input2[] = { 0xADF8, 0x11AF };
    static const uint16_t input3[] = { 0x1100, 0x1173, 0x11AF };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* HANGUL SYLLABLE GEU */
    static const uint16_t input1[] = { 0xADF8 };
    static const uint16_t input2[] = { 0x1100, 0x1173 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Simple string.  */
  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint16_t input1[] =
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    static const uint16_t input2[] =
      { 'g', 'r', 0x00FC, 0x0073, 0x0073, ' ', 'g', 'o', 't', 't', '.', ' ',
        0x0437, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    static const uint16_t input3[] =
      { 'G', 'R', 0x00DC, 0x0053, 0x0053, ' ', 'G', 'O', 'T', 'T', '.', ' ',
        0x0417, 0x0414, 0x0420, 0x0410, 0x0412, 0x0421, 0x0422, 0x0412, 0x0423,
        0x0419, 0x0422, 0x0415, '!', ' ',
        'X', '=', '(', '-', 'B', 0x00B1, 'S', 'Q', 'R', 'T', '(', 'B', 0x00B2,
        '-', '4', 'A', 'C', ')', ')', '/', '(', '2', 'A', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Case mapping can increase the number of Unicode characters.  */
  { /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    static const uint16_t input1[] = { 0x0149 };
    static const uint16_t input2[] = { 0x02BC, 0x006E };
    static const uint16_t input3[] = { 0x02BC, 0x004E };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    static const uint16_t input1[] = { 0x0390 };
    static const uint16_t input2[] = { 0x03B9, 0x0308, 0x0301 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Turkish letters i İ ı I */
  { /* LATIN CAPITAL LETTER I */
    static const uint16_t input[]         = { 0x0049 };
    static const uint16_t casefolded[]    = { 0x0069 };
    static const uint16_t casefolded_tr[] = { 0x0131 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN SMALL LETTER I */
    static const uint16_t input[]         = { 0x0069 };
    static const uint16_t casefolded[]    = { 0x0049 };
    static const uint16_t casefolded_tr[] = { 0x0130 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    static const uint16_t input[]         = { 0x0130 };
    static const uint16_t casefolded[]    = { 0x0069, 0x0307 };
    static const uint16_t casefolded_tr[] = { 0x0069 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN SMALL LETTER DOTLESS I */
    static const uint16_t input[]      = { 0x0131 };
    static const uint16_t casefolded[] = { 0x0049 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 1);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* "topkapı" */
    static const uint16_t input[] =
      { 0x0054, 0x004F, 0x0050, 0x004B, 0x0041, 0x0050, 0x0049 };
    static const uint16_t casefolded[] =
      { 0x0074, 0x006F, 0x0070, 0x006B, 0x0061, 0x0070, 0x0131 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "heiß" */
    static const uint16_t input1[] = { 0x0068, 0x0065, 0x0069, 0x00DF };
    static const uint16_t input2[] = { 0x0068, 0x0065, 0x0069, 0x0073, 0x0073 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Case mappings for some characters can depend on the surrounding characters.  */
  { /* "περισσότερες πληροφορίες" */
    static const uint16_t input1[] =
      {
        0x03C0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C2, 0x0020, 0x03C0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C2
      };
    static const uint16_t input2[] =
      {
        0x03C0, 0x03B5, 0x03C1, 0x03B9, 0x03C3, 0x03C3, 0x03CC, 0x03C4,
        0x03B5, 0x03C1, 0x03B5, 0x03C3, 0x0020, 0x03C0, 0x03BB, 0x03B7,
        0x03C1, 0x03BF, 0x03C6, 0x03BF, 0x03C1, 0x03AF, 0x03B5, 0x03C3
      };
    static const uint16_t input3[] =
      {
        0x03A0, 0x0395, 0x03A1, 0x0399, 0x03A3, 0x03A3, 0x038C, 0x03A4,
        0x0395, 0x03A1, 0x0395, 0x03A3, 0x0020, 0x03A0, 0x039B, 0x0397,
        0x03A1, 0x039F, 0x03A6, 0x039F, 0x03A1, 0x038A, 0x0395, 0x03A3
      };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Case mapping can require subsequent normalization.  */
  { /* LATIN SMALL LETTER J WITH CARON, COMBINING DOT BELOW */
    static const uint16_t input[]                 = { 0x01F0, 0x0323 };
    static const uint16_t casefolded[]            = { 0x006A, 0x030C, 0x0323 };
    static const uint16_t casefolded_decomposed[] = { 0x006A, 0x0323, 0x030C };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_decomposed, SIZEOF (casefolded_decomposed), NULL, NULL, &cmp) == 0);
    ASSERT (cmp != 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_decomposed, SIZEOF (casefolded_decomposed), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
}

int
main ()
{
  test_ascii (u16_casecmp, UNINORM_NFD);
  test_nonascii (u16_casecmp);

  return 0;
}
