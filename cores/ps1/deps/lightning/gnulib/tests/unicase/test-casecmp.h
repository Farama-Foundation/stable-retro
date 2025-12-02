/* Test of case and normalization insensitive comparison of Unicode strings.
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
test_ascii (int (*my_casecmp) (const UNIT *, size_t, const UNIT *, size_t, const char *, uninorm_t, int *),
            uninorm_t nf)
{
  /* Empty string.  */
  {
    int cmp;

    ASSERT (my_casecmp (NULL, 0, NULL, 0, NULL, nf, &cmp) == 0);
    ASSERT (cmp == 0);
  }
  {
    static const UNIT input[] = { 'x', 'y' };
    int cmp;

    ASSERT (my_casecmp (input, SIZEOF (input), NULL, 0, NULL, nf, &cmp) == 0);
    ASSERT (cmp == 1);

    ASSERT (my_casecmp (NULL, 0, input, SIZEOF (input), NULL, nf, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input, SIZEOF (input), input, SIZEOF (input), NULL, nf, &cmp) == 0);
    ASSERT (cmp == 0);
  }

  /* Normal lexicographic order.  */
  {
    static const UNIT input1[] = { 'A', 'm', 'e', 'r', 'i', 'c', 'a' };
    static const UNIT input2[] = { 'A', 'm', 'i', 'g', 'o' };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, nf, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1), NULL, nf, &cmp) == 0);
    ASSERT (cmp == 1);
  }

  /* Shorter and longer strings.  */
  {
    static const UNIT input1[] = { 'R', 'e', 'a', 'g', 'a', 'n' };
    static const UNIT input2[] = { 'R', 'e', 'a', 'g', 'a', 'n', 'o', 'm', 'i', 'c', 's' };
    int cmp;

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2), NULL, nf, &cmp) == 0);
    ASSERT (cmp == -1);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1), NULL, nf, &cmp) == 0);
    ASSERT (cmp == 1);
  }
}
