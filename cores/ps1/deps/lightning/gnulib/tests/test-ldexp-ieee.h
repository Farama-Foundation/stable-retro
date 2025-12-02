/* Test of ldexp*() function family.
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
  int exp;

  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  for (exp = -100000; exp <= 100000; exp += 100000)
    ASSERT (ISNAN (LDEXP (NAN, exp)));

  /* Signed zero.  */
  for (exp = -100000; exp <= 100000; exp += 100000)
    {
      DOUBLE z = LDEXP (L_(0.0), exp);
      ASSERT (z == L_(0.0));
      ASSERT (!signbit (z));
    }
  for (exp = -100000; exp <= 100000; exp += 100000)
    {
      DOUBLE z = LDEXP (MINUS_ZERO, exp);
      ASSERT (z == L_(0.0));
      ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
    }

  /* Infinity.  */
  for (exp = -100000; exp <= 100000; exp += 100000)
    ASSERT (LDEXP (INFINITY, exp) == INFINITY);
  for (exp = -100000; exp <= 100000; exp += 100000)
    ASSERT (LDEXP (- INFINITY, exp) == - INFINITY);
}
