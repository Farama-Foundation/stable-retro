/* Test of case and normalization insensitive comparison of UTF-8 strings.
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

#define UNIT uint8_t
#include "test-casecmp.h"
#undef UNIT

static void
test_nonascii (int (*my_casecmp) (const uint8_t *, size_t, const uint8_t *, size_t, const char *, uninorm_t, int *))
{
  /* Normalization effects.  */
  {
    static const uint8_t input1[] = { 'H', 0xC3, 0xB6, 'h', 'l', 'e' };
    static const uint8_t input2[] = { 'H', 'O', 0xCC, 0x88, 'h', 'L', 'e' };
    static const uint8_t input3[] = { 'H', 0xC3, 0xB6, 'h', 'l', 'e', 'n' };
    static const uint8_t input4[] = { 'H', 'O', 0xCC, 0x88, 'h', 'L', 'e', 'n' };
    static const uint8_t input5[] = { 'H', 'u', 'r', 'z' };
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
    static const uint8_t input1[] = { 0xC3, 0x84 };
    static const uint8_t input2[] = { 0x41, 0xCC, 0x88 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    static const uint8_t input1[] = { 0xC7, 0x9E };
    static const uint8_t input2[] = { 0x41, 0xCC, 0x88, 0xCC, 0x84 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* GREEK DIALYTIKA AND PERISPOMENI */
    static const uint8_t input1[] = { 0xE1, 0xBF, 0x81 };
    static const uint8_t input2[] = { 0xC2, 0xA8, 0xCD, 0x82 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* HANGUL SYLLABLE GEUL */
    static const uint8_t input1[] = { 0xEA, 0xB8, 0x80 };
    static const uint8_t input2[] = { 0xEA, 0xB7, 0xB8, 0xE1, 0x86, 0xAF };
    static const uint8_t input3[] = { 0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3, 0xE1, 0x86, 0xAF };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* HANGUL SYLLABLE GEU */
    static const uint8_t input1[] = { 0xEA, 0xB7, 0xB8 };
    static const uint8_t input2[] = { 0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Simple string.  */
  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint8_t input1[] =
      { 'G', 'r', 0xC3, 0xBC, 0xC3, 0x9F, ' ', 'G', 'o', 't', 't', '.', ' ',
        0xD0, 0x97, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1, 0x81,
        0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB9, 0xD1, 0x82, 0xD0, 0xB5,
        '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1, 's', 'q', 'r', 't', '(',
        'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
      };
    static const uint8_t input2[] =
      { 'g', 'r', 0xC3, 0xBC, 0x73, 0x73, ' ', 'g', 'o', 't', 't', '.', ' ',
        0xD0, 0xB7, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1, 0x81,
        0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB9, 0xD1, 0x82, 0xD0, 0xB5,
        '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1, 's', 'q', 'r', 't', '(',
        'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
      };
    static const uint8_t input3[] =
      { 'G', 'R', 0xC3, 0x9C, 0x53, 0x53, ' ', 'G', 'O', 'T', 'T', '.', ' ',
        0xD0, 0x97, 0xD0, 0x94, 0xD0, 0xA0, 0xD0, 0x90, 0xD0, 0x92, 0xD0, 0xA1,
        0xD0, 0xA2, 0xD0, 0x92, 0xD0, 0xA3, 0xD0, 0x99, 0xD0, 0xA2, 0xD0, 0x95,
        '!', ' ', 'X', '=', '(', '-', 'B', 0xC2, 0xB1, 'S', 'Q', 'R', 'T', '(',
        'B', 0xC2, 0xB2, '-', '4', 'A', 'C', ')', ')', '/', '(', '2', 'A', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
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
    static const uint8_t input1[] = { 0xC5, 0x89 };
    static const uint8_t input2[] = { 0xCA, 0xBC, 0x6E };
    static const uint8_t input3[] = { 0xCA, 0xBC, 0x4E };
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
    static const uint8_t input1[] = { 0xCE, 0x90 };
    static const uint8_t input2[] = { 0xCE, 0xB9, 0xCC, 0x88, 0xCC, 0x81 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Turkish letters i İ ı I */
  { /* LATIN CAPITAL LETTER I */
    static const uint8_t input[]         = { 0x49 };
    static const uint8_t casefolded[]    = { 0x69 };
    static const uint8_t casefolded_tr[] = { 0xC4, 0xB1 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN SMALL LETTER I */
    static const uint8_t input[]         = { 0x69 };
    static const uint8_t casefolded[]    = { 0x49 };
    static const uint8_t casefolded_tr[] = { 0xC4, 0xB0 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    static const uint8_t input[]         = { 0xC4, 0xB0 };
    static const uint8_t casefolded[]    = { 0x69, 0xCC, 0x87 };
    static const uint8_t casefolded_tr[] = { 0x69 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN SMALL LETTER DOTLESS I */
    static const uint8_t input[]      = { 0xC4, 0xB1 };
    static const uint8_t casefolded[] = { 0x49 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 1);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* "topkapı" */
    static const uint8_t input[] =
      { 0x54, 0x4F, 0x50, 0x4B, 0x41, 0x50, 0x49 };
    static const uint8_t casefolded[] =
      { 0x74, 0x6F, 0x70, 0x6B, 0x61, 0x70, 0xC4, 0xB1 };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded), "tr", NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "heiß" */
    static const uint8_t input1[] = { 0x68, 0x65, 0x69, 0xC3, 0x9F };
    static const uint8_t input2[] = { 0x68, 0x65, 0x69, 0x73, 0x73 };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, NULL, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Case mappings for some characters can depend on the surrounding characters.  */
  { /* "περισσότερες πληροφορίες" */
    static const uint8_t input1[] =
      {
        0xCF, 0x80, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB9, 0xCF, 0x83, 0xCF, 0x83,
        0xCF, 0x8C, 0xCF, 0x84, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB5, 0xCF, 0x82,
        ' ', 0xCF, 0x80, 0xCE, 0xBB, 0xCE, 0xB7, 0xCF, 0x81, 0xCE, 0xBF,
        0xCF, 0x86, 0xCE, 0xBF, 0xCF, 0x81, 0xCE, 0xAF, 0xCE, 0xB5, 0xCF, 0x82
      };
    static const uint8_t input2[] =
      {
        0xCF, 0x80, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB9, 0xCF, 0x83, 0xCF, 0x83,
        0xCF, 0x8C, 0xCF, 0x84, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB5, 0xCF, 0x83,
        ' ', 0xCF, 0x80, 0xCE, 0xBB, 0xCE, 0xB7, 0xCF, 0x81, 0xCE, 0xBF,
        0xCF, 0x86, 0xCE, 0xBF, 0xCF, 0x81, 0xCE, 0xAF, 0xCE, 0xB5, 0xCF, 0x83
      };
    static const uint8_t input3[] =
      {
        0xCE, 0xA0, 0xCE, 0x95, 0xCE, 0xA1, 0xCE, 0x99, 0xCE, 0xA3, 0xCE, 0xA3,
        0xCE, 0x8C, 0xCE, 0xA4, 0xCE, 0x95, 0xCE, 0xA1, 0xCE, 0x95, 0xCE, 0xA3,
        ' ', 0xCE, 0xA0, 0xCE, 0x9B, 0xCE, 0x97, 0xCE, 0xA1, 0xCE, 0x9F,
        0xCE, 0xA6, 0xCE, 0x9F, 0xCE, 0xA1, 0xCE, 0x8A, 0xCE, 0x95, 0xCE, 0xA3
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
    static const uint8_t input[]                 = { 0xC7, 0xB0, 0xCC, 0xA3 };
    static const uint8_t casefolded[]            = { 0x6A, 0xCC, 0x8C, 0xCC, 0xA3 };
    static const uint8_t casefolded_decomposed[] = { 0x6A, 0xCC, 0xA3, 0xCC, 0x8C };
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
  test_ascii (u8_casecmp, UNINORM_NFD);
  test_nonascii (u8_casecmp);

  return 0;
}
