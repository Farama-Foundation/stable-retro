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
  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (ISNAN (FMA (NAN, L_(3.0), L_(5.0))));
  ASSERT (ISNAN (FMA (NAN, L_(0.0), L_(5.0))));
  ASSERT (ISNAN (FMA (L_(2.0), NAN, L_(5.0))));
  ASSERT (ISNAN (FMA (L_(0.0), NAN, L_(5.0))));
  ASSERT (ISNAN (FMA (NAN, NAN, L_(5.0))));
  ASSERT (ISNAN (FMA (NAN, L_(3.0), NAN)));
  ASSERT (ISNAN (FMA (NAN, L_(0.0), NAN)));
  ASSERT (ISNAN (FMA (L_(2.0), NAN, NAN)));
  ASSERT (ISNAN (FMA (L_(0.0), NAN, NAN)));
  ASSERT (ISNAN (FMA (NAN, NAN, NAN)));

  /* Infinity.  */
  ASSERT (ISNAN (FMA (INFINITY, L_(2.0), - INFINITY)));
  ASSERT (ISNAN (FMA (INFINITY, - L_(2.0), INFINITY)));
  ASSERT (ISNAN (FMA (- INFINITY, L_(2.0), INFINITY)));
  ASSERT (ISNAN (FMA (- INFINITY, - L_(2.0), - INFINITY)));
  ASSERT (ISNAN (FMA (L_(2.0), INFINITY, - INFINITY)));
  ASSERT (ISNAN (FMA (L_(2.0), - INFINITY, INFINITY)));
  ASSERT (ISNAN (FMA (- L_(2.0), INFINITY, INFINITY)));
  ASSERT (ISNAN (FMA (- L_(2.0), - INFINITY, - INFINITY)));

  ASSERT (FMA (INFINITY, L_(2.0), INFINITY) == INFINITY);
  ASSERT (FMA (INFINITY, - L_(2.0), - INFINITY) == - INFINITY);
  ASSERT (FMA (- INFINITY, L_(2.0), - INFINITY) == - INFINITY);
  ASSERT (FMA (- INFINITY, - L_(2.0), INFINITY) == INFINITY);
  ASSERT (FMA (L_(2.0), INFINITY, INFINITY) == INFINITY);
  ASSERT (FMA (L_(2.0), - INFINITY, - INFINITY) == - INFINITY);
  ASSERT (FMA (- L_(2.0), INFINITY, - INFINITY) == - INFINITY);
  ASSERT (FMA (- L_(2.0), - INFINITY, INFINITY) == INFINITY);

  /* Infinity times zero is NaN.  */
  ASSERT (ISNAN (FMA (INFINITY, L_(0.0), L_(7.0))));
  ASSERT (ISNAN (FMA (INFINITY, L_(0.0), INFINITY)));
  ASSERT (ISNAN (FMA (INFINITY, L_(0.0), NAN)));
  ASSERT (ISNAN (FMA (- INFINITY, L_(0.0), L_(7.0))));
  ASSERT (ISNAN (FMA (- INFINITY, L_(0.0), INFINITY)));
  ASSERT (ISNAN (FMA (- INFINITY, L_(0.0), NAN)));
  ASSERT (ISNAN (FMA (L_(0.0), INFINITY, L_(7.0))));
  ASSERT (ISNAN (FMA (L_(0.0), INFINITY, INFINITY)));
  ASSERT (ISNAN (FMA (L_(0.0), INFINITY, NAN)));
  ASSERT (ISNAN (FMA (L_(0.0), - INFINITY, L_(7.0))));
  ASSERT (ISNAN (FMA (L_(0.0), - INFINITY, INFINITY)));
  ASSERT (ISNAN (FMA (L_(0.0), - INFINITY, NAN)));

  /* NaN in the z argument.  */
  ASSERT (ISNAN (FMA (L_(2.0), L_(3.0), NAN)));
  ASSERT (ISNAN (FMA (L_(2.0), INFINITY, NAN)));
  ASSERT (ISNAN (FMA (INFINITY, L_(3.0), NAN)));
  ASSERT (ISNAN (FMA (INFINITY, INFINITY, NAN)));
}
