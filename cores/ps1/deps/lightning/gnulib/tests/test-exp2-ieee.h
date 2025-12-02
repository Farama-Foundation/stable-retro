/* Test of exp2*() function family.
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

  /* Underflow.  */
  {
    volatile DOUBLE z = EXP2 (-100000.0);
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }

  /* NaN.  */
  ASSERT (ISNAN (EXP2 (NAN)));

  /* Zero.  */
  ASSERT (EXP2 (L_(0.0)) == L_(1.0));
  ASSERT (EXP2 (MINUS_ZERO) == L_(1.0));

  /* Infinity.  */
  {
    DOUBLE z = EXP2 (- INFINITY);
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  ASSERT (EXP2 (INFINITY) == INFINITY);
}
