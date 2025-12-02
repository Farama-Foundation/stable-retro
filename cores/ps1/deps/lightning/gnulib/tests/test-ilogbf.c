/* Test of ilogbf() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2012.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (ilogbf, int, (float));

#include <float.h>
#include <limits.h>

#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

#undef INFINITY
#undef NAN

#define DOUBLE float
#define VOLATILE volatile
#define INFINITY Infinityf ()
#define NAN NaNf ()
#define L_(literal) literal##f
#define MINUS_ZERO minus_zerof
#define MAX_EXP FLT_MAX_EXP
#define MIN_EXP FLT_MIN_EXP
#define MIN_NORMAL_EXP FLT_MIN_EXP
#define ILOGB ilogbf
#define RANDOM randomf
#include "test-ilogb.h"

int
main ()
{
  test_function ();

  return 0;
}
