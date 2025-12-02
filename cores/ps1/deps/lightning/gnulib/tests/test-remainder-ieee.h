/* Test of remainder*() function family.
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
  ASSERT (ISNAN (REMAINDER (NAN, L_(2.0))));
  ASSERT (ISNAN (REMAINDER (NAN, L_(0.0))));
  ASSERT (ISNAN (REMAINDER (L_(2.0), NAN)));
  ASSERT (ISNAN (REMAINDER (L_(0.0), NAN)));
  ASSERT (ISNAN (REMAINDER (NAN, NAN)));

  /* y is zero.  */
  ASSERT (ISNAN (REMAINDER (L_(2.0), L_(0.0))));
  ASSERT (ISNAN (REMAINDER (L_(2.0), MINUS_ZERO)));
  ASSERT (ISNAN (REMAINDER (INFINITY, L_(0.0))));
  ASSERT (ISNAN (REMAINDER (INFINITY, MINUS_ZERO)));

  /* x is infinite.  */
  ASSERT (ISNAN (REMAINDER (INFINITY, L_(2.0))));
  ASSERT (ISNAN (REMAINDER (INFINITY, - L_(2.0))));

  /* The following tests originally applied to the FMOD function.
     They are not specified for the REMAINDER function by POSIX.  */

  /* x is zero.  */
  {
    DOUBLE z = REMAINDER (L_(0.0), L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  {
    DOUBLE z = REMAINDER (MINUS_ZERO, L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
  }
  {
    DOUBLE z = REMAINDER (L_(0.0), - L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  {
    DOUBLE z = REMAINDER (MINUS_ZERO, - L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
  }
}
