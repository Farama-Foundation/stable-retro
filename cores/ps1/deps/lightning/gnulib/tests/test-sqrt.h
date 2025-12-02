/* Test of sqrt*() function family.
   Copyright (C) 2012-2021 Free Software Foundation, Inc.

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

static void
test_function (void)
{
  int i;
  int j;
  const DOUBLE TWO_MANT_DIG =
    /* Assume MANT_DIG <= 5 * 31.
       Use the identity
         n = floor(n/5) + floor((n+1)/5) + ... + floor((n+4)/5).  */
    (DOUBLE) (1U << ((MANT_DIG - 1) / 5))
    * (DOUBLE) (1U << ((MANT_DIG - 1 + 1) / 5))
    * (DOUBLE) (1U << ((MANT_DIG - 1 + 2) / 5))
    * (DOUBLE) (1U << ((MANT_DIG - 1 + 3) / 5))
    * (DOUBLE) (1U << ((MANT_DIG - 1 + 4) / 5));

  /* Randomized tests.  */
  for (i = 0; i < SIZEOF (RANDOM); i++)
    {
      DOUBLE x = L_(16.0) * RANDOM[i]; /* 0.0 <= x <= 16.0 */
      DOUBLE y = SQRT (x);
      DOUBLE err = y * y - x;
      ASSERT (y >= L_(0.0));
      ASSERT (err > - L_(16.0) / TWO_MANT_DIG
              && err < L_(16.0) / TWO_MANT_DIG);
    }

  for (i = 0; i < SIZEOF (RANDOM) / 4; i++)
    for (j = 0; j < SIZEOF (RANDOM) / 4; j++)
      {
        DOUBLE x = L_(16.0) * RANDOM[i]; /* 0.0 <= x <= 16.0 */
        DOUBLE y = L_(16.0) * RANDOM[j]; /* 0.0 <= y <= 16.0 */
        if (x > L_(0.0) && y > L_(0.0))
          {
            DOUBLE z = L_(1.0) / (x * y);
            /* Approximately  x * y * z = 1.  */
            DOUBLE p = SQRT (x) * SQRT (y) * SQRT (z);
            ASSERT (p > L_(1.0) - L_(4.0) / TWO_MANT_DIG
                    && p < L_(1.0) + L_(4.0) / TWO_MANT_DIG);
          }
      }
}

volatile DOUBLE x;
DOUBLE y;
