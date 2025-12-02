/* Test of fused multiply-add.
   Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2011.  */

/* Returns 2^e as a DOUBLE.  */
#define POW2(e) \
  LDEXP (L_(1.0), e)

/* One could define XE_RANGE and YE_RANGE to 5 or 60, but this would slow down
   the test without the expectation of catching more bugs.  */
#define XE_RANGE 0
#define YE_RANGE 0

/* Define to 1 if you want to allow the behaviour of the 'double-double'
   implementation of 'long double' (seen on IRIX 6.5 and Linux/PowerPC).
   This floating-point type does not follow IEEE 754.  */
#if MANT_BIT == LDBL_MANT_BIT && LDBL_MANT_BIT == 2 * DBL_MANT_BIT
# define FORGIVE_DOUBLEDOUBLE_BUG 1
#else
# define FORGIVE_DOUBLEDOUBLE_BUG 0
#endif

/* Subnormal numbers appear to not work as expected on IRIX 6.5.  */
#ifdef __sgi
# define MIN_SUBNORMAL_EXP (MIN_EXP - 1)
#else
# define MIN_SUBNORMAL_EXP (MIN_EXP - MANT_BIT)
#endif

/* Check rounding behaviour.  */

static void
test_function (DOUBLE (*my_fma) (DOUBLE, DOUBLE, DOUBLE))
{
  /* Array mapping n to (-1)^n.  */
  static const DOUBLE pow_m1[] =
    {
      L_(1.0), - L_(1.0), L_(1.0), - L_(1.0),
      L_(1.0), - L_(1.0), L_(1.0), - L_(1.0)
    };

  /* A product x * y that consists of two bits.  */
  {
    volatile DOUBLE x;
    volatile DOUBLE y;
    volatile DOUBLE z;
    volatile DOUBLE result;
    volatile DOUBLE expected;
    int xs;
    int xe;
    int ys;
    int ye;
    int ze;
    DOUBLE sign;

    for (xs = 0; xs < 2; xs++)
      for (xe = -XE_RANGE; xe <= XE_RANGE; xe++)
        {
          x = pow_m1[xs] * POW2 (xe); /* (-1)^xs * 2^xe */

          for (ys = 0; ys < 2; ys++)
            for (ye = -YE_RANGE; ye <= YE_RANGE; ye++)
              {
                y = pow_m1[ys] * POW2 (ye); /* (-1)^ys * 2^ye */

                sign = pow_m1[xs + ys];

                /* Test addition (same signs).  */
                for (ze = MIN_EXP - MANT_BIT; ze <= MAX_EXP - 1;)
                  {
                    z = sign * POW2 (ze); /* (-1)^(xs+ys) * 2^ze */
                    result = my_fma (x, y, z);
                    if (xe + ye >= ze + MANT_BIT)
                      expected = sign * POW2 (xe + ye);
                    else if (xe + ye > ze - MANT_BIT)
                      expected = sign * (POW2 (xe + ye) + POW2 (ze));
                    else
                      expected = z;
                    ASSERT (result == expected);

                    ze++;
                    /* Shortcut some values of ze, to speed up the test.  */
                    if (ze == MIN_EXP + MANT_BIT)
                      ze = - 2 * MANT_BIT - 1;
                    else if (ze == 2 * MANT_BIT)
                      ze = MAX_EXP - MANT_BIT - 1;
                  }

                /* Test subtraction (opposite signs).  */
                for (ze = MIN_EXP - MANT_BIT; ze <= MAX_EXP - 1;)
                  {
                    z = - sign * POW2 (ze); /* (-1)^(xs+ys+1) * 2^ze */
                    result = my_fma (x, y, z);
                    if (xe + ye > ze + MANT_BIT)
                      expected = sign * POW2 (xe + ye);
                    else if (xe + ye >= ze)
                      expected = sign * (POW2 (xe + ye) - POW2 (ze));
                    else if (xe + ye > ze - 1 - MANT_BIT)
                      expected = - sign * (POW2 (ze) - POW2 (xe + ye));
                    else
                      expected = z;
                    ASSERT (result == expected);

                    ze++;
                    /* Shortcut some values of ze, to speed up the test.  */
                    if (ze == MIN_EXP + MANT_BIT)
                      ze = - 2 * MANT_BIT - 1;
                    else if (ze == 2 * MANT_BIT)
                      ze = MAX_EXP - MANT_BIT - 1;
                  }
              }
        }
  }
  /* A product x * y that consists of three bits.  */
  {
    volatile DOUBLE x;
    volatile DOUBLE y;
    volatile DOUBLE z;
    volatile DOUBLE result;
    volatile DOUBLE expected;
    int i;
    int xs;
    int xe;
    int ys;
    int ye;
    int ze;
    DOUBLE sign;

    for (i = 1; i <= MANT_BIT - 1; i++)
      for (xs = 0; xs < 2; xs++)
        for (xe = -XE_RANGE; xe <= XE_RANGE; xe++)
          {
            x = /* (-1)^xs * (2^xe + 2^(xe-i)) */
              pow_m1[xs] * (POW2 (xe) + POW2 (xe - i));

            for (ys = 0; ys < 2; ys++)
              for (ye = -YE_RANGE; ye <= YE_RANGE; ye++)
                {
                  y = /* (-1)^ys * (2^ye + 2^(ye-i)) */
                    pow_m1[ys] * (POW2 (ye) + POW2 (ye - i));

                  sign = pow_m1[xs + ys];

                  /* The exact value of x * y is
                     (-1)^(xs+ys) * (2^(xe+ye) + 2^(xe+ye-i+1) + 2^(xe+ye-2*i)) */

                  /* Test addition (same signs).  */
                  for (ze = MIN_SUBNORMAL_EXP; ze <= MAX_EXP - 1;)
                    {
                      z = sign * POW2 (ze); /* (-1)^(xs+ys) * 2^ze */
                      result = my_fma (x, y, z);
                      if (FORGIVE_DOUBLEDOUBLE_BUG)
                        if ((xe + ye > ze
                             && xe + ye < ze + MANT_BIT
                             && i == DBL_MANT_BIT)
                            || (xe + ye == ze + DBL_MANT_BIT && i == DBL_MANT_BIT + 1)
                            || (xe + ye == ze + MANT_BIT - 1 && i == 1))
                          goto skip1;
                      if (xe + ye > ze + MANT_BIT)
                        {
                          if (2 * i > MANT_BIT)
                            expected =
                              sign * (POW2 (xe + ye)
                                      + POW2 (xe + ye - i + 1));
                          else if (2 * i == MANT_BIT)
                            expected =
                              sign * (POW2 (xe + ye)
                                      + POW2 (xe + ye - i + 1)
                                      + POW2 (xe + ye - MANT_BIT + 1));
                          else
                            expected =
                              sign * (POW2 (xe + ye)
                                      + POW2 (xe + ye - i + 1)
                                      + POW2 (xe + ye - 2 * i));
                        }
                      else if (xe + ye == ze + MANT_BIT)
                        {
                          if (2 * i >= MANT_BIT)
                            expected =
                              sign * (POW2 (xe + ye)
                                      + POW2 (xe + ye - i + 1)
                                      + POW2 (xe + ye - MANT_BIT + 1));
                          else if (2 * i == MANT_BIT - 1)
                            /* round-to-even rounds up */
                            expected =
                              sign * (POW2 (xe + ye)
                                      + POW2 (xe + ye - i + 1)
                                      + POW2 (xe + ye - 2 * i + 1));
                          else
                            expected =
                              sign * (POW2 (xe + ye)
                                      + POW2 (xe + ye - i + 1)
                                      + POW2 (xe + ye - 2 * i));
                        }
                      else if (xe + ye > ze - MANT_BIT + 2 * i)
                        expected =
                          sign * (POW2 (ze)
                                  + POW2 (xe + ye)
                                  + POW2 (xe + ye - i + 1)
                                  + POW2 (xe + ye - 2 * i));
                      else if (xe + ye >= ze - MANT_BIT + i)
                        expected =
                          sign * (POW2 (ze)
                                  + POW2 (xe + ye)
                                  + POW2 (xe + ye - i + 1));
                      else if (xe + ye == ze - MANT_BIT + i - 1)
                        {
                          if (i == 1)
                            expected =
                              sign * (POW2 (ze) + POW2 (ze - MANT_BIT + 1));
                          else
                            expected =
                              sign * (POW2 (ze)
                                      + POW2 (xe + ye)
                                      + POW2 (ze - MANT_BIT + 1));
                        }
                      else if (xe + ye >= ze - MANT_BIT + 1)
                        expected = sign * (POW2 (ze) + POW2 (xe + ye));
                      else if (xe + ye == ze - MANT_BIT)
                        expected =
                          sign * (POW2 (ze) + POW2 (ze - MANT_BIT + 1));
                      else if (xe + ye == ze - MANT_BIT - 1)
                        {
                          if (i == 1)
                            expected =
                              sign * (POW2 (ze) + POW2 (ze - MANT_BIT + 1));
                          else
                            expected = z;
                        }
                      else
                        expected = z;
                      ASSERT (result == expected);

                     skip1:
                      ze++;
                      /* Shortcut some values of ze, to speed up the test.  */
                      if (ze == MIN_EXP + MANT_BIT)
                        ze = - 2 * MANT_BIT - 1;
                      else if (ze == 2 * MANT_BIT)
                        ze = MAX_EXP - MANT_BIT - 1;
                    }

                  /* Test subtraction (opposite signs).  */
                  if (i > 1)
                    for (ze = MIN_SUBNORMAL_EXP; ze <= MAX_EXP - 1;)
                      {
                        z = - sign * POW2 (ze); /* (-1)^(xs+ys+1) * 2^ze */
                        result = my_fma (x, y, z);
                        if (FORGIVE_DOUBLEDOUBLE_BUG)
                          if ((xe + ye == ze && i == MANT_BIT - 1)
                              || (xe + ye > ze
                                  && xe + ye <= ze + DBL_MANT_BIT - 1
                                  && i == DBL_MANT_BIT + 1)
                              || (xe + ye >= ze + DBL_MANT_BIT - 1
                                  && xe + ye < ze + MANT_BIT
                                  && xe + ye == ze + i - 1)
                              || (xe + ye > ze + DBL_MANT_BIT
                                  && xe + ye < ze + MANT_BIT
                                  && i == DBL_MANT_BIT))
                            goto skip2;
                        if (xe + ye == ze)
                          {
                            /* maximal extinction */
                            expected =
                              sign * (POW2 (xe + ye - i + 1)
                                      + POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye == ze - 1)
                          {
                            /* significant extinction */
                            if (2 * i > MANT_BIT)
                              expected =
                                sign * (- POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1));
                            else
                              expected =
                                sign * (- POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1)
                                        + POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye > ze + MANT_BIT)
                          {
                            if (2 * i >= MANT_BIT)
                              expected =
                                sign * (POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1));
                            else
                              expected =
                                sign * (POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1)
                                        + POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye == ze + MANT_BIT)
                          {
                            if (2 * i >= MANT_BIT)
                              expected =
                                sign * (POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1));
                            else if (2 * i == MANT_BIT - 1)
                              /* round-to-even rounds down */
                              expected =
                                sign * (POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1));
                            else
                              /* round-to-even rounds up */
                              expected =
                                sign * (POW2 (xe + ye)
                                        + POW2 (xe + ye - i + 1)
                                        + POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye >= ze - MANT_BIT + 2 * i)
                          expected =
                            sign * (- POW2 (ze)
                                    + POW2 (xe + ye)
                                    + POW2 (xe + ye - i + 1)
                                    + POW2 (xe + ye - 2 * i));
                        else if (xe + ye >= ze - MANT_BIT + i - 1)
                          expected =
                            sign * (- POW2 (ze)
                                    + POW2 (xe + ye)
                                    + POW2 (xe + ye - i + 1));
                        else if (xe + ye == ze - MANT_BIT + i - 2)
                          expected =
                            sign * (- POW2 (ze)
                                    + POW2 (xe + ye)
                                    + POW2 (ze - MANT_BIT));
                        else if (xe + ye >= ze - MANT_BIT)
                          expected =
                            sign * (- POW2 (ze)
                                    + POW2 (xe + ye));
                        else if (xe + ye == ze - MANT_BIT - 1)
                          expected =
                            sign * (- POW2 (ze)
                                    + POW2 (ze - MANT_BIT));
                        else
                          expected = z;
                        ASSERT (result == expected);

                       skip2:
                        ze++;
                        /* Shortcut some values of ze, to speed up the test.  */
                        if (ze == MIN_EXP + MANT_BIT)
                          ze = - 2 * MANT_BIT - 1;
                        else if (ze == 2 * MANT_BIT)
                          ze = MAX_EXP - MANT_BIT - 1;
                      }
                }
          }
  }
  /* TODO: Similar tests with
     x = (-1)^xs * (2^xe - 2^(xe-i)), y = (-1)^ys * (2^ye - 2^(ye-i))  */
  /* A product x * y that consists of one segment of bits (or, if you prefer,
     two bits, one with positive weight and one with negative weight).  */
  {
    volatile DOUBLE x;
    volatile DOUBLE y;
    volatile DOUBLE z;
    volatile DOUBLE result;
    volatile DOUBLE expected;
    int i;
    int xs;
    int xe;
    int ys;
    int ye;
    int ze;
    DOUBLE sign;

    for (i = 1; i <= MANT_BIT - 1; i++)
      for (xs = 0; xs < 2; xs++)
        for (xe = -XE_RANGE; xe <= XE_RANGE; xe++)
          {
            x = /* (-1)^xs * (2^xe + 2^(xe-i)) */
              pow_m1[xs] * (POW2 (xe) + POW2 (xe - i));

            for (ys = 0; ys < 2; ys++)
              for (ye = -YE_RANGE; ye <= YE_RANGE; ye++)
                {
                  y = /* (-1)^ys * (2^ye - 2^(ye-i)) */
                    pow_m1[ys] * (POW2 (ye) - POW2 (ye - i));

                  sign = pow_m1[xs + ys];

                  /* The exact value of x * y is
                     (-1)^(xs+ys) * (2^(xe+ye) - 2^(xe+ye-2*i)) */

                  /* Test addition (same signs).  */
                  for (ze = MIN_EXP - MANT_BIT; ze <= MAX_EXP - 1;)
                    {
                      z = sign * POW2 (ze); /* (-1)^(xs+ys) * 2^ze */
                      result = my_fma (x, y, z);
                      if (FORGIVE_DOUBLEDOUBLE_BUG)
                        if ((xe + ye == ze + MANT_BIT && i > DBL_MANT_BIT)
                            || (xe + ye < ze + MANT_BIT
                                && xe + ye >= ze
                                && i == DBL_MANT_BIT)
                            || (xe + ye < ze
                                && xe + ye == ze - MANT_BIT + 2 * i))
                          goto skip3;
                      if (xe + ye > ze + MANT_BIT + 1)
                        {
                          if (2 * i > MANT_BIT)
                            expected = sign * POW2 (xe + ye);
                          else
                            expected =
                              sign * (POW2 (xe + ye)
                                      - POW2 (xe + ye - 2 * i));
                        }
                      else if (xe + ye == ze + MANT_BIT + 1)
                        {
                          if (2 * i >= MANT_BIT)
                            expected = sign * POW2 (xe + ye);
                          else
                            expected =
                              sign * (POW2 (xe + ye)
                                      - POW2 (xe + ye - 2 * i));
                        }
                      else if (xe + ye >= ze - MANT_BIT + 2 * i)
                        {
                          if (2 * i > MANT_BIT)
                            expected =
                              sign * (POW2 (xe + ye) + POW2 (ze));
                          else
                            expected =
                              sign * (POW2 (xe + ye)
                                      - POW2 (xe + ye - 2 * i)
                                      + POW2 (ze));
                        }
                      else if (xe + ye >= ze - MANT_BIT + 1)
                        expected =
                          sign * (POW2 (ze) + POW2 (xe + ye));
                      else
                        expected = z;
                      ASSERT (result == expected);

                     skip3:
                      ze++;
                      /* Shortcut some values of ze, to speed up the test.  */
                      if (ze == MIN_EXP + MANT_BIT)
                        ze = - 2 * MANT_BIT - 1;
                      else if (ze == 2 * MANT_BIT)
                        ze = MAX_EXP - MANT_BIT - 1;
                    }

                  /* Test subtraction (opposite signs).  */
                  if (i > 1)
                    for (ze = MIN_SUBNORMAL_EXP; ze <= MAX_EXP - 1;)
                      {
                        z = - sign * POW2 (ze); /* (-1)^(xs+ys+1) * 2^ze */
                        result = my_fma (x, y, z);
                        if (FORGIVE_DOUBLEDOUBLE_BUG)
                          if (xe + ye > ze
                              && xe + ye < ze + DBL_MANT_BIT
                              && xe + ye == ze + 2 * i - LDBL_MANT_BIT)
                            goto skip4;
                        if (xe + ye == ze)
                          {
                            /* maximal extinction */
                            expected = sign * - POW2 (xe + ye - 2 * i);
                          }
                        else if (xe + ye > ze + MANT_BIT + 1)
                          {
                            if (2 * i > MANT_BIT + 1)
                              expected = sign * POW2 (xe + ye);
                            else if (2 * i == MANT_BIT + 1)
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - MANT_BIT));
                            else
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye == ze + MANT_BIT + 1)
                          {
                            if (2 * i > MANT_BIT)
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - MANT_BIT));
                            else if (2 * i == MANT_BIT)
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - MANT_BIT + 1));
                            else
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye == ze + MANT_BIT)
                          {
                            if (2 * i > MANT_BIT + 1)
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - MANT_BIT));
                            else if (2 * i == MANT_BIT + 1)
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (xe + ye - MANT_BIT + 1));
                            else
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (ze)
                                        - POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye > ze - MANT_BIT + 2 * i)
                          {
                            if (2 * i > MANT_BIT)
                              expected =
                                sign * (POW2 (xe + ye) - POW2 (ze));
                            else
                              expected =
                                sign * (POW2 (xe + ye)
                                        - POW2 (ze)
                                        - POW2 (xe + ye - 2 * i));
                          }
                        else if (xe + ye == ze - MANT_BIT + 2 * i)
                          expected =
                            sign * (- POW2 (ze)
                                    + POW2 (xe + ye)
                                    - POW2 (xe + ye - 2 * i));
                        else if (xe + ye >= ze - MANT_BIT)
                          expected = sign * (- POW2 (ze) + POW2 (xe + ye));
                        else
                          expected = z;
                        ASSERT (result == expected);

                       skip4:
                        ze++;
                        /* Shortcut some values of ze, to speed up the test.  */
                        if (ze == MIN_EXP + MANT_BIT)
                          ze = - 2 * MANT_BIT - 1;
                        else if (ze == 2 * MANT_BIT)
                          ze = MAX_EXP - MANT_BIT - 1;
                      }
                }
          }
  }
  /* TODO: Tests with denormalized results.  */
  /* Tests with temporary overflow.  */
  {
    volatile DOUBLE x = POW2 (MAX_EXP - 1);
    volatile DOUBLE y = POW2 (MAX_EXP - 1);
    volatile DOUBLE z = - INFINITY;
    volatile DOUBLE result = my_fma (x, y, z);
    ASSERT (result == - INFINITY);
  }
  {
    volatile DOUBLE x = POW2 (MAX_EXP - 1); /* 2^(MAX_EXP-1) */
    volatile DOUBLE y = L_(2.0);            /* 2^1 */
    volatile DOUBLE z =               /* -(2^MAX_EXP - 2^(MAX_EXP-MANT_BIT)) */
      - LDEXP (POW2 (MAX_EXP - 1) - POW2 (MAX_EXP - MANT_BIT - 1), 1);
    volatile DOUBLE result = my_fma (x, y, z);
    if (!FORGIVE_DOUBLEDOUBLE_BUG)
      ASSERT (result == POW2 (MAX_EXP - MANT_BIT));
  }
  {
    volatile DOUBLE x = POW2 (MAX_EXP - 1);             /* 2^(MAX_EXP-1) */
    volatile DOUBLE y = L_(3.0);                        /* 3 */
    volatile DOUBLE z = - LDEXP (L_(5.0), MAX_EXP - 3); /* -5*2^(MAX_EXP-3) */
    volatile DOUBLE result = my_fma (x, y, z);
    ASSERT (result == LDEXP (L_(7.0), MAX_EXP - 3));
  }
}
