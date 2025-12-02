/* Test of normalization insensitive comparison of UTF-16 strings.
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

#include "uninorm.h"

#include "macros.h"

#include "test-u16-normcmp.h"

static void
test_nonascii (int (*my_normcmp) (const uint16_t *, size_t, const uint16_t *, size_t, uninorm_t, int *))
{
  /* Normalization effects.  */
  {
    static const uint16_t input1[] = { 'H', 0x00F6, 'h', 'l', 'e' };
    static const uint16_t input2[] = { 'H', 'o', 0x0308, 'h', 'l', 'e' };
    static const uint16_t input3[] = { 'H', 0x00F6, 'h', 'l', 'e', 'n' };
    static const uint16_t input4[] = { 'H', 'o', 0x0308, 'h', 'l', 'e', 'n' };
    static const uint16_t input5[] = { 'H', 'u', 'r', 'z' };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_normcmp (input2, SIZEOF (input2), input1, SIZEOF (input1), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_normcmp (input3, SIZEOF (input3), input4, SIZEOF (input4), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_normcmp (input4, SIZEOF (input4), input3, SIZEOF (input3), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_normcmp (input2, SIZEOF (input2), input3, SIZEOF (input3), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_normcmp (input1, SIZEOF (input1), input4, SIZEOF (input4), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_normcmp (input1, SIZEOF (input1), input5, SIZEOF (input5), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_normcmp (input2, SIZEOF (input2), input5, SIZEOF (input5), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == -1);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    static const uint16_t input1[] = { 0x00C4 };
    static const uint16_t input2[] = { 0x0041, 0x0308 };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    static const uint16_t input1[] = { 0x01DE };
    static const uint16_t input2[] = { 0x0041, 0x0308, 0x0304 };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* GREEK DIALYTIKA AND PERISPOMENI */
    static const uint16_t input1[] = { 0x1FC1 };
    static const uint16_t input2[] = { 0x00A8, 0x0342 };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* HANGUL SYLLABLE GEUL */
    static const uint16_t input1[] = { 0xAE00 };
    static const uint16_t input2[] = { 0xADF8, 0x11AF };
    static const uint16_t input3[] = { 0x1100, 0x1173, 0x11AF };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);

    ASSERT (my_normcmp (input1, SIZEOF (input1), input3, SIZEOF (input3), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  { /* HANGUL SYLLABLE GEU */
    static const uint16_t input1[] = { 0xADF8 };
    static const uint16_t input2[] = { 0x1100, 0x1173 };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), UNINORM_NFD, &cmp) == 0);
    ASSERT (cmp == 0);
  }
}

int
main ()
{
  test_ascii (u16_normcmp, UNINORM_NFD);
  test_nonascii (u16_normcmp);

  return 0;
}
