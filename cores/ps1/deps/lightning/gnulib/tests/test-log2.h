/* Test of log2*() function family.
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

  /* Pole.  */
  ASSERT (LOG2 (L_(0.0)) == - HUGEVAL);
  ASSERT (LOG2 (MINUS_ZERO) == - HUGEVAL);

  /* Integral values.  */
  {
    DOUBLE x = L_(1.0);
    DOUBLE y = LOG2 (x);
    ASSERT (y == L_(0.0));
  }
  {
    int e;
    DOUBLE x;
    DOUBLE y;
    for (e = 0, x = L_(0.0), y = L_(1.0);
         e <= MAX_EXP - 1;
         e++, x = x + L_(1.0), y = y * L_(2.0))
      {
        /* Invariant: x = e, y = 2^e.  */
        DOUBLE z = LOG2 (y);
        ASSERT (z == x);
      }
  }
  {
    int e;
    DOUBLE x;
    DOUBLE y;
    for (e = 0, x = L_(0.0), y = L_(1.0);
         e >= MIN_EXP - 1;
         e--, x = x - L_(1.0), y = y * L_(0.5))
      {
        /* Invariant: x = e, y = 2^e.  */
        DOUBLE z = LOG2 (y);
        ASSERT (z == x);
      }
  }

  /* Randomized tests.  */
  {
    /* Error bound, in ulps.  */
    const DOUBLE err_bound =
      (sizeof (DOUBLE) > sizeof (double) ?
#if defined __i386__ && defined __FreeBSD__
       /* On FreeBSD/x86 6.4, the 'long double' type really has only 53 bits of
          precision in the compiler but 64 bits of precision at runtime.  See
          <https://lists.gnu.org/r/bug-gnulib/2008-07/msg00063.html>.
          The compiler has truncated all 'long double' literals in log2l.c to
          53 bits of precision.  */
       L_(8193.0)
#else
       L_(5.0)
#endif
       : L_(5.0));

    for (i = 0; i < SIZEOF (RANDOM); i++)
      {
        DOUBLE x = L_(16.0) * RANDOM[i] + L_(1.0); /* 1.0 <= x <= 17.0 */
        DOUBLE y = LOG2 (x);
        DOUBLE z = LOG2 (L_(1.0) / x);
        DOUBLE err = y + z;
        ASSERT (y >= L_(0.0));
        ASSERT (z <= L_(0.0));
        ASSERT (err > - err_bound / TWO_MANT_DIG
                && err < err_bound / TWO_MANT_DIG);
      }
  }

  {
    /* Error bound, in ulps.  */
    const DOUBLE err_bound =
      (sizeof (DOUBLE) > sizeof (double) ?
#if defined __i386__ && defined __FreeBSD__
       /* On FreeBSD/x86 6.4, the 'long double' type really has only 53 bits of
          precision in the compiler but 64 bits of precision at runtime.  See
          <https://lists.gnu.org/r/bug-gnulib/2008-07/msg00063.html>.
          The compiler has truncated all 'long double' literals in log2l.c to
          53 bits of precision.  */
       L_(8193.0)
#else
       L_(9.0)
#endif
       : L_(9.0));

    for (i = 0; i < SIZEOF (RANDOM) / 5; i++)
      for (j = 0; j < SIZEOF (RANDOM) / 5; j++)
        {
          DOUBLE x = L_(17.0) / (L_(16.0) - L_(15.0) * RANDOM[i]) - L_(1.0);
          DOUBLE y = L_(17.0) / (L_(16.0) - L_(15.0) * RANDOM[j]) - L_(1.0);
          /* 1/16 <= x,y <= 16 */
          DOUBLE z = L_(1.0) / (x * y);
          /* Approximately  x * y * z = 1.  */
          DOUBLE err = LOG2 (x) + LOG2 (y) + LOG2 (z);
          ASSERT (err > - err_bound / TWO_MANT_DIG
                  && err < err_bound / TWO_MANT_DIG);
        }
  }
}

volatile DOUBLE x;
DOUBLE y;
