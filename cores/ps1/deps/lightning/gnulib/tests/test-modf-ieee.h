/* Test of modf*() function family.
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
    DOUBLE integer_part = -9999;
    DOUBLE fractional_part = MODF (NAN, &integer_part);
    ASSERT (ISNAN (integer_part));
    ASSERT (ISNAN (fractional_part));
  }

  /* Infinity.  */
  {
    DOUBLE integer_part = -9999;
    DOUBLE fractional_part = MODF (INFINITY, &integer_part);
    ASSERT (integer_part == INFINITY);
    ASSERT (fractional_part == L_(0.0));
    ASSERT (!signbit (fractional_part));
  }
  {
    DOUBLE integer_part = -9999;
    DOUBLE fractional_part = MODF (- INFINITY, &integer_part);
    ASSERT (integer_part == - INFINITY);
    ASSERT (fractional_part == L_(0.0));
    ASSERT (!!signbit (fractional_part) == !!signbit (MINUS_ZERO));
  }
}
