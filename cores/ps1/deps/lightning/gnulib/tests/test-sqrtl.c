/* Test of sqrtl() function.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (sqrtl, long double, (long double));

#include <float.h>

#include "fpucw.h"
#include "macros.h"

#define DOUBLE long double
#define L_(literal) literal##L
#define MANT_DIG DBL_MANT_DIG
#define SQRT sqrtl
#define RANDOM randoml
#include "test-sqrt.h"

static long double
my_ldexpl (long double x, int d)
{
  for (; d > 0; d--)
    x *= 2.0L;
  for (; d < 0; d++)
    x *= 0.5L;
  return x;
}

int
main ()
{
  DECL_LONG_DOUBLE_ROUNDING

  BEGIN_LONG_DOUBLE_ROUNDING ();

  /* A particular value.  */
  x = 0.6L;
  y = sqrtl (x);
  ASSERT (y >= 0.7745966692L && y <= 0.7745966693L);

  /* Another particular value.  */
  {
    long double z;
    long double err;

    x = 8.1974099812331540680810141969554806865L;
    y = sqrtl (x);
    z = y * y - x;
    err = my_ldexpl (z, LDBL_MANT_DIG);
    if (err < 0)
      err = - err;
    ASSERT (err <= 100.0L);
  }

  test_function ();

  return 0;
}
