/* Test of log2l() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2012.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (log2l, long double, (long double));

#include <float.h>

#include "fpucw.h"
#include "minus-zero.h"
#include "macros.h"

#define DOUBLE long double
#define HUGEVAL HUGE_VALL
#define L_(literal) literal##L
#define MANT_DIG LDBL_MANT_DIG
#define MAX_EXP LDBL_MAX_EXP
#define MIN_EXP LDBL_MIN_EXP
#define MINUS_ZERO minus_zerol
#define LOG2 log2l
#define RANDOM randoml
#include "test-log2.h"

int
main ()
{
  DECL_LONG_DOUBLE_ROUNDING

  BEGIN_LONG_DOUBLE_ROUNDING ();

  /* A particular value.  */
  x = 0.6L;
  y = log2l (x);
  ASSERT (y >= -0.7369655942L && y <= -0.7369655941L);

  test_function ();

  return 0;
}
