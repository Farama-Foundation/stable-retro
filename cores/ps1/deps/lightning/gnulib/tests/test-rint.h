/* Test of rint*() function family.
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
  /* Infinite numbers.  */
  ASSERT (RINT (INFINITY) == INFINITY);
  ASSERT (RINT (- INFINITY) == - INFINITY);
  /* NaNs.  */
  ASSERT (ISNAN (RINT (NAN)));

  /* Randomized tests.  */
  {
    int i;

    for (i = 0; i < SIZEOF (RANDOM); i++)
      {
        DOUBLE x;

        x = L_(0.5) * RANDOM[i];
        ASSERT (RINT (x) == L_(0.0));
        x = - x;
        ASSERT (RINT (x) == L_(0.0));

        x = L_(1.0) - L_(0.5) * RANDOM[i];
        ASSERT (RINT (x) == L_(1.0));
        x = - x;
        ASSERT (RINT (x) == - L_(1.0));

        x = L_(1.0) + L_(0.5) * RANDOM[i];
        ASSERT (RINT (x) == L_(1.0));
        x = - x;
        ASSERT (RINT (x) == - L_(1.0));

        x = L_(2.0) - L_(0.5) * RANDOM[i];
        ASSERT (RINT (x) == L_(2.0));
        x = - x;
        ASSERT (RINT (x) == - L_(2.0));
      }
  }
}
