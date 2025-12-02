/* Test of hypot*() function family.
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

  /* Infinity.  */
  ASSERT (HYPOT (INFINITY, L_(2.0)) == INFINITY);
  ASSERT (HYPOT (- INFINITY, L_(2.0)) == INFINITY);
  ASSERT (HYPOT (INFINITY, - L_(2.0)) == INFINITY);
  ASSERT (HYPOT (- INFINITY, - L_(2.0)) == INFINITY);
  ASSERT (HYPOT (L_(2.0), INFINITY) == INFINITY);
  ASSERT (HYPOT (L_(2.0), - INFINITY) == INFINITY);
  ASSERT (HYPOT (- L_(2.0), INFINITY) == INFINITY);
  ASSERT (HYPOT (- L_(2.0), - INFINITY) == INFINITY);

  /* Mixed Infinity and NaN.  */
  ASSERT (HYPOT (INFINITY, NAN) == INFINITY);
  ASSERT (HYPOT (- INFINITY, NAN) == INFINITY);
  ASSERT (HYPOT (NAN, INFINITY) == INFINITY);
  ASSERT (HYPOT (NAN, - INFINITY) == INFINITY);

  /* NaN.  */
  ASSERT (ISNAN (HYPOT (NAN, L_(2.0))));
  ASSERT (ISNAN (HYPOT (NAN, - L_(2.0))));
  ASSERT (ISNAN (HYPOT (L_(2.0), NAN)));
  ASSERT (ISNAN (HYPOT (- L_(2.0), NAN)));
}
