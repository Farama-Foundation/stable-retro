/* Test of logb*() function family.
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
  VOLATILE DOUBLE x;
  DOUBLE y;

  /* Some particular values.  */
  x = L_(0.6);
  y = LOGB (x);
  ASSERT (y == - L_(1.0));

  x = L_(1.2);
  y = LOGB (x);
  ASSERT (y == L_(0.0));

  x = L_(2.1);
  y = LOGB (x);
  ASSERT (y == L_(1.0));

  x = L_(3.9);
  y = LOGB (x);
  ASSERT (y == L_(1.0));

  x = L_(4.0);
  y = LOGB (x);
  ASSERT (y == (FLT_RADIX == 2 ? L_(2.0) : L_(1.0)));

  x = L_(0.25);
  y = LOGB (x);
  ASSERT (y == (FLT_RADIX == 2 ? - L_(2.0) : - L_(1.0)));

  /* Zero.  */
  ASSERT (LOGB (L_(0.0)) == - HUGEVAL);
  ASSERT (LOGB (MINUS_ZERO) == - HUGEVAL);

  /* From here on, this test assumes FLT_RADIX == 2.  */

  for (i = 1, x = L_(1.0); i <= MAX_EXP; i++, x *= L_(2.0))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (i = 1, x = L_(1.0); i >= MIN_NORMAL_EXP; i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (; i >= MIN_EXP - 100 && x > L_(0.0); i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }

  for (i = 1, x = - L_(1.0); i <= MAX_EXP; i++, x *= L_(2.0))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (i = 1, x = - L_(1.0); i >= MIN_NORMAL_EXP; i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (; i >= MIN_EXP - 100 && x < L_(0.0); i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }

  for (i = 1, x = L_(1.01); i <= MAX_EXP; i++, x *= L_(2.0))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (i = 1, x = L_(1.01); i >= MIN_NORMAL_EXP; i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (; i >= MIN_EXP - 100 && x > L_(0.0); i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }

  for (i = 1, x = L_(1.73205); i <= MAX_EXP; i++, x *= L_(2.0))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (i = 1, x = L_(1.73205); i >= MIN_NORMAL_EXP; i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1));
    }
  for (; i >= MIN_EXP - 100 && x > L_(0.0); i--, x *= L_(0.5))
    {
      y = LOGB (x);
      ASSERT (y == (DOUBLE)(i - 1) || y == (DOUBLE)i);
    }

  /* Randomized tests.  */
  for (i = 0; i < SIZEOF (RANDOM); i++)
    {
      x = L_(20.0) * RANDOM[i] - L_(10.0); /* -10.0 <= x <= 10.0 */
      if (x != L_(0.0))
        {
          DOUBLE abs_x = (x < L_(0.0) ? - x : x);
          y = LOGB (x);
          ASSERT (y == (DOUBLE) (int) y);
          ASSERT (abs_x >= my_ldexp (L_(1.0), (int) y));
          ASSERT (abs_x < my_ldexp (L_(1.0), (int) y + 1));
        }
    }
}

volatile DOUBLE x;
DOUBLE y;
