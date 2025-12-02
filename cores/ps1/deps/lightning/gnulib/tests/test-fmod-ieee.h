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

static void
test_function (void)
{
  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (ISNAN (FMOD (NAN, L_(2.0))));
  ASSERT (ISNAN (FMOD (NAN, L_(0.0))));
  ASSERT (ISNAN (FMOD (L_(2.0), NAN)));
  ASSERT (ISNAN (FMOD (L_(0.0), NAN)));
  ASSERT (ISNAN (FMOD (NAN, NAN)));

  /* y is zero.  */
  ASSERT (ISNAN (FMOD (L_(2.0), L_(0.0))));
  ASSERT (ISNAN (FMOD (L_(2.0), MINUS_ZERO)));
  ASSERT (ISNAN (FMOD (INFINITY, L_(0.0))));
  ASSERT (ISNAN (FMOD (INFINITY, MINUS_ZERO)));

  /* x is infinite.  */
  ASSERT (ISNAN (FMOD (INFINITY, L_(2.0))));
  ASSERT (ISNAN (FMOD (INFINITY, - L_(2.0))));

  /* x is zero.  */
  {
    DOUBLE z = FMOD (L_(0.0), L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  {
    DOUBLE z = FMOD (MINUS_ZERO, L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
  }
  {
    DOUBLE z = FMOD (L_(0.0), - L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  {
    DOUBLE z = FMOD (MINUS_ZERO, - L_(2.0));
    ASSERT (z == L_(0.0));
    ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
  }
  {
    DOUBLE z = FMOD (L_(0.0), INFINITY);
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  {
    DOUBLE z = FMOD (MINUS_ZERO, INFINITY);
    ASSERT (z == L_(0.0));
    ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
  }
  {
    DOUBLE z = FMOD (L_(0.0), - INFINITY);
    ASSERT (z == L_(0.0));
    ASSERT (!signbit (z));
  }
  {
    DOUBLE z = FMOD (MINUS_ZERO, - INFINITY);
    ASSERT (z == L_(0.0));
    ASSERT (!!signbit (z) == !!signbit (MINUS_ZERO));
  }

  /* y is infinite.  */
  {
    DOUBLE z = FMOD (L_(2.5), INFINITY);
    ASSERT (z == L_(2.5));
  }
  {
    DOUBLE z = FMOD (L_(2.5), - INFINITY);
    ASSERT (z == L_(2.5));
  }
  {
    DOUBLE z = FMOD (- L_(2.5), INFINITY);
    ASSERT (z == - L_(2.5));
  }
  {
    DOUBLE z = FMOD (- L_(2.5), - INFINITY);
    ASSERT (z == - L_(2.5));
  }
}
