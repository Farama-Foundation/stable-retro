/* Test of splitting a double into fraction and mantissa.
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
  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  {
    int exp = -9999;
    DOUBLE mantissa;
    mantissa = FREXP (NAN, &exp);
    ASSERT (ISNAN (mantissa));
  }

  /* Signed zero.  */
  {
    int exp = -9999;
    DOUBLE mantissa;
    mantissa = FREXP (L_(0.0), &exp);
    ASSERT (mantissa == L_(0.0));
    ASSERT (!signbit (mantissa));
    ASSERT (exp == 0);
  }
  {
    int exp = -9999;
    DOUBLE mantissa;
    mantissa = FREXP (MINUS_ZERO, &exp);
    ASSERT (mantissa == L_(0.0));
    ASSERT (!!signbit (mantissa) == !!signbit (MINUS_ZERO));
    ASSERT (exp == 0);
  }

  /* Infinity.  */
  {
    int exp = -9999;
    DOUBLE mantissa;
    mantissa = FREXP (INFINITY, &exp);
    ASSERT (mantissa == INFINITY);
  }
  {
    int exp = -9999;
    DOUBLE mantissa;
    mantissa = FREXP (- INFINITY, &exp);
    ASSERT (mantissa == - INFINITY);
  }
}
