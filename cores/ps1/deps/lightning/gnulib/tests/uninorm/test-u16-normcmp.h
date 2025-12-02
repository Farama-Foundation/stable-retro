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

static void
test_ascii (int (*my_normcmp) (const uint16_t *, size_t, const uint16_t *, size_t, uninorm_t, int *),
            uninorm_t nf)
{
  /* Empty string.  */
  {
    int cmp;

    ASSERT (my_normcmp (NULL, 0, NULL, 0, nf, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  {
    static const uint16_t input[] = { 'x', 'y' };
    int cmp;

    ASSERT (my_normcmp (input, SIZEOF (input), NULL, 0, nf, &cmp) == 0);
    ASSERT (cmp == 1);

    ASSERT (my_normcmp (NULL, 0, input, SIZEOF (input), nf, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_normcmp (input, SIZEOF (input), input, SIZEOF (input), nf, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Normal lexicographic order.  */
  {
    static const uint16_t input1[] = { 'A', 'm', 'e', 'r', 'i', 'c', 'a' };
    static const uint16_t input2[] = { 'A', 'm', 'i', 'g', 'o' };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), nf, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_normcmp (input2, SIZEOF (input2), input1, SIZEOF (input1), nf, &cmp) == 0);
    ASSERT (cmp == 1);
  }

  /* Shorter and longer strings.  */
  {
    static const uint16_t input1[] = { 'R', 'e', 'a', 'g', 'a', 'n' };
    static const uint16_t input2[] = { 'R', 'e', 'a', 'g', 'a', 'n', 'o', 'm', 'i', 'c', 's' };
    int cmp;

    ASSERT (my_normcmp (input1, SIZEOF (input1), input2, SIZEOF (input2), nf, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_normcmp (input2, SIZEOF (input2), input1, SIZEOF (input1), nf, &cmp) == 0);
    ASSERT (cmp == 1);
  }
}
