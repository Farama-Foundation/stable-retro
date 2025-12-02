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

static void
test_function (DOUBLE (*my_fma) (DOUBLE, DOUBLE, DOUBLE))
{
  volatile DOUBLE x;
  volatile DOUBLE y;
  volatile DOUBLE z;
  volatile DOUBLE result;
  volatile DOUBLE expected;

  /* Combinations with NaN.  */
  /* "If x or y are NaN, a NaN shall be returned."  */
  {
    x = NAN;
    y = NAN;
    z = NAN;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = NAN;
    y = NAN;
    z = L_(1.0);
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = NAN;
    y = L_(0.0);
    z = NAN;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = NAN;
    y = L_(0.0);
    z = L_(1.0);
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = L_(0.0);
    y = NAN;
    z = NAN;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = L_(0.0);
    y = NAN;
    z = L_(1.0);
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  /* "If x*y is not 0*Inf nor Inf*0 and z is a NaN, a NaN shall be returned." */
  {
    x = L_(3.0);
    y = - L_(2.0);
    z = NAN;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  /* "If one of x and y is infinite, the other is zero, and z is a NaN, a NaN
     shall be returned and a domain error may occur."  */
  {
    x = INFINITY;
    y = L_(0.0);
    z = NAN;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = L_(0.0);
    y = INFINITY;
    z = NAN;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }

  /* Combinations with Infinity.  */
  /* "If x multiplied by y is an exact infinity and z is also an infinity but
     with the opposite sign, a domain error shall occur, and either a NaN
     (if supported), or an implementation-defined value shall be returned."  */
  {
    x = INFINITY;
    y = L_(3.0);
    z = - INFINITY;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = INFINITY;
    y = - L_(3.0);
    z = INFINITY;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = L_(3.0);
    y = INFINITY;
    z = - INFINITY;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = - L_(3.0);
    y = INFINITY;
    z = INFINITY;
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  /* "If one of x and y is infinite, the other is zero, and z is not a NaN, a
     domain error shall occur, and either a NaN (if supported), or an
     implementation-defined value shall be returned."  */
  {
    x = INFINITY;
    y = L_(0.0);
    z = L_(5.0);
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  {
    x = L_(0.0);
    y = INFINITY;
    z = L_(5.0);
    result = my_fma (x, y, z);
    ASSERT (ISNAN (result));
  }
  /* Infinite results.  */
  {
    x = - L_(2.0);
    y = L_(3.0);
    z = INFINITY;
    result = my_fma (x, y, z);
    expected = INFINITY;
    ASSERT (result == expected);
  }
  {
    x = INFINITY;
    y = L_(3.0);
    z = INFINITY;
    result = my_fma (x, y, z);
    expected = INFINITY;
    ASSERT (result == expected);
  }
  {
    x = INFINITY;
    y = - L_(3.0);
    z = - INFINITY;
    result = my_fma (x, y, z);
    expected = - INFINITY;
    ASSERT (result == expected);
  }
  {
    x = L_(3.0);
    y = INFINITY;
    z = INFINITY;
    result = my_fma (x, y, z);
    expected = INFINITY;
    ASSERT (result == expected);
  }
  {
    x = - L_(3.0);
    y = INFINITY;
    z = - INFINITY;
    result = my_fma (x, y, z);
    expected = - INFINITY;
    ASSERT (result == expected);
  }

  /* Combinations with zero.  */
  {
    x = L_(0.0);
    y = L_(3.0);
    z = L_(11.0);
    result = my_fma (x, y, z);
    expected = L_(11.0);
    ASSERT (result == expected);
  }
  {
    x = L_(3.0);
    y = L_(0.0);
    z = L_(11.0);
    result = my_fma (x, y, z);
    expected = L_(11.0);
    ASSERT (result == expected);
  }
  {
    x = L_(3.0);
    y = L_(4.0);
    z = L_(0.0);
    result = my_fma (x, y, z);
    expected = L_(12.0);
    ASSERT (result == expected);
  }
}
