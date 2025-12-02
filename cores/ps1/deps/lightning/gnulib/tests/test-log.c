/* Test of log() function.
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
SIGNATURE_CHECK (log, double, (double));

#include <float.h>

#include "minus-zero.h"
#include "macros.h"

#define DOUBLE double
#define HUGEVAL HUGE_VAL
#define L_(literal) literal
#define MANT_DIG DBL_MANT_DIG
#define MINUS_ZERO minus_zerod
#define LOG log
#define RANDOM randomd
#include "test-log.h"

int
main ()
{
  /* A particular value.  */
  x = 0.6;
  y = log (x);
  ASSERT (y >= -0.5108256238 && y <= -0.5108256237);

  test_function ();

  return 0;
}
