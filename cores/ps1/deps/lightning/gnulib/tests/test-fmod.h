/* Test of fmod*() function family.
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

static DOUBLE
my_ldexp (DOUBLE x, int d)
{
  for (; d > 0; d--)
    x *= L_(2.0);
  for (; d < 0; d++)
    x *= L_(0.5);
  return x;
}

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
  for (i = 0; i < SIZEOF (RANDOM) / 5; i++)
    for (j = 0; j < SIZEOF (RANDOM) / 5; j++)
      {
        DOUBLE x = L_(16.0) * RANDOM[i]; /* 0.0 <= x <= 16.0 */
        DOUBLE y = RANDOM[j]; /* 0.0 <= y < 1.0 */
        if (y > L_(0.0))
          {
            DOUBLE z = FMOD (x, y);
            ASSERT (z >= L_(0.0));
            ASSERT (z < y);
            z -= x - (int) (x / y) * y;
            ASSERT (/* The common case.  */
                    (z > - L_(16.0) / TWO_MANT_DIG
                     && z < L_(16.0) / TWO_MANT_DIG)
                    || /* rounding error: x / y computed too large */
                       (z > y - L_(16.0) / TWO_MANT_DIG
                        && z < y + L_(16.0) / TWO_MANT_DIG)
                    || /* rounding error: x / y computed too small */
                       (z > - y - L_(16.0) / TWO_MANT_DIG
                        && z < - y + L_(16.0) / TWO_MANT_DIG));
          }
      }

  for (i = 0; i < SIZEOF (RANDOM) / 5; i++)
    for (j = 0; j < SIZEOF (RANDOM) / 5; j++)
      {
        DOUBLE x = L_(1.0e9) * RANDOM[i]; /* 0.0 <= x <= 10^9 */
        DOUBLE y = RANDOM[j]; /* 0.0 <= y < 1.0 */
        if (y > L_(0.0))
          {
            DOUBLE z = FMOD (x, y);
            DOUBLE r;
            ASSERT (z >= L_(0.0));
            ASSERT (z < y);
            {
              /* Determine the quotient x / y in two steps, because it
                 may be > 2^31.  */
              int q1 = (int) (x / y / L_(65536.0));
              int q2 = (int) ((x - q1 * L_(65536.0) * y) / y);
              DOUBLE q = (DOUBLE) q1 * L_(65536.0) + (DOUBLE) q2;
              r = x - q * y;
            }
            /* The absolute error of z can be up to 1e9/2^MANT_DIG.
               The absolute error of r can also be up to 1e9/2^MANT_DIG.
               Therefore the error of z - r can be twice as large.  */
            z -= r;
            ASSERT (/* The common case.  */
                    (z > - L_(2.0) * L_(1.0e9) / TWO_MANT_DIG
                     && z < L_(2.0) * L_(1.0e9) / TWO_MANT_DIG)
                    || /* rounding error: x / y computed too large */
                       (z > y - L_(2.0) * L_(1.0e9) / TWO_MANT_DIG
                        && z < y + L_(2.0) * L_(1.0e9) / TWO_MANT_DIG)
                    || /* rounding error: x / y computed too small */
                       (z > - y - L_(2.0) * L_(1.0e9) / TWO_MANT_DIG
                        && z < - y + L_(2.0) * L_(1.0e9) / TWO_MANT_DIG));
          }
      }

  {
    int large_exp = (MAX_EXP - 1 < 1000 ? MAX_EXP - 1 : 1000);
    DOUBLE large = my_ldexp (L_(1.0), large_exp); /* = 2^large_exp */
    for (i = 0; i < SIZEOF (RANDOM) / 10; i++)
      for (j = 0; j < SIZEOF (RANDOM) / 10; j++)
        {
          DOUBLE x = large * RANDOM[i]; /* 0.0 <= x <= 2^large_exp */
          DOUBLE y = RANDOM[j]; /* 0.0 <= y < 1.0 */
          if (y > L_(0.0))
            {
              DOUBLE z = FMOD (x, y);
              /* Regardless how large the rounding errors are, the result
                 must be >= 0, < y.  */
              ASSERT (z >= L_(0.0));
              ASSERT (z < y);
            }
        }
  }
}

volatile DOUBLE x;
volatile DOUBLE y;
DOUBLE z;
