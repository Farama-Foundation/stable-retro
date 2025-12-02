/* Test of ldexp*() function family.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007, 2010.  */

static void
test_function (void)
{
  int i;
  volatile DOUBLE x;
  volatile DOUBLE y;

  /* A particular value.  */
  {
    x = L_(0.6);
    y = LDEXP (x, 0);
    ASSERT (y >= L_(0.5999999999) && y <= L_(0.6000000001));
  }
  {
    x = L_(0.6);
    y = LDEXP (x, 1);
    ASSERT (y >= L_(1.199999999) && y <= L_(1.200000001));
  }
  {
    x = L_(0.6);
    y = LDEXP (x, -1);
    ASSERT (y >= L_(0.2999999999) && y <= L_(0.3000000001));
  }

  { /* NaN.  */
    x = NAN;
    y = LDEXP (x, 0); ASSERT (ISNAN (y));
    y = LDEXP (x, 5); ASSERT (ISNAN (y));
    y = LDEXP (x, -5); ASSERT (ISNAN (y));
  }

  { /* Positive infinity.  */
    x = INFINITY;
    y = LDEXP (x, 0); ASSERT (y == x);
    y = LDEXP (x, 5); ASSERT (y == x);
    y = LDEXP (x, -5); ASSERT (y == x);
  }

  { /* Negative infinity.  */
    x = - INFINITY;
    y = LDEXP (x, 0); ASSERT (y == x);
    y = LDEXP (x, 5); ASSERT (y == x);
    y = LDEXP (x, -5); ASSERT (y == x);
  }

  { /* Positive zero.  */
    x = L_(0.0);
    y = LDEXP (x, 0); ASSERT (y == x); ASSERT (!signbit (x));
    y = LDEXP (x, 5); ASSERT (y == x); ASSERT (!signbit (x));
    y = LDEXP (x, -5); ASSERT (y == x); ASSERT (!signbit (x));
  }

  { /* Negative zero.  */
    x = MINUS_ZERO;
    y = LDEXP (x, 0); ASSERT (y == x); ASSERT (signbit (x));
    y = LDEXP (x, 5); ASSERT (y == x); ASSERT (signbit (x));
    y = LDEXP (x, -5); ASSERT (y == x); ASSERT (signbit (x));
  }

  { /* Positive finite number.  */
    x = L_(1.73205);
    y = LDEXP (x, 0); ASSERT (y == x);
    y = LDEXP (x, 5); ASSERT (y == x * L_(32.0));
    y = LDEXP (x, -5); ASSERT (y == x * L_(0.03125));
  }

  { /* Negative finite number.  */
    x = - L_(20.085536923187667742);
    y = LDEXP (x, 0); ASSERT (y == x);
    y = LDEXP (x, 5); ASSERT (y == x * L_(32.0));
    y = LDEXP (x, -5); ASSERT (y == x * L_(0.03125));
  }

  for (i = 1, x = L_(1.73205); i <= MAX_EXP; i++, x *= L_(2.0))
    {
      y = LDEXP (x, 0); ASSERT (y == x);
      {
        volatile DOUBLE expected;
        y = LDEXP (x, 5);
        expected = x * L_(32.0);
        ASSERT (y == expected);
      }
      y = LDEXP (x, -5); ASSERT (y == x * 0.03125L);
    }
  for (i = 1, x = L_(1.73205); i >= MIN_EXP; i--, x *= L_(0.5))
    {
      y = LDEXP (x, 0); ASSERT (y == x);
      y = LDEXP (x, 5); ASSERT (y == x * L_(32.0));
      if (i - 5 >= MIN_EXP)
        {
          y = LDEXP (x, -5); ASSERT (y == x * L_(0.03125));
        }
    }
  for (; i >= LDBL_MIN_EXP - 100 && x > L_(0.0); i--, x *= L_(0.5))
    {
      y = LDEXP (x, 0); ASSERT (y == x);
      y = LDEXP (x, 5); ASSERT (y == x * L_(32.0));
    }

  /* Randomized tests.  */
  for (i = 0; i < SIZEOF (RANDOM); i++)
    {
      int u, v;

      x = L_(20.0) * RANDOM[i] - L_(10.0); /* -10.0 <= x <= 10.0 */
      /* LDEXP only does rounding when it returns a denormalized number
         or there is underflow.  It doesn't happen here.  */
      for (u = -10; u <= 10; u++)
        for (v = -10; v <= 10; v++)
          ASSERT (LDEXP (x, u + v) == LDEXP (LDEXP (x, u), v));
    }
}
