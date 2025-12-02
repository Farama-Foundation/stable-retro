/* Test of exp2f() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2010-2011.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (exp2f, float, (float));

#include <float.h>

#include "macros.h"

#define DOUBLE float
#define L_(literal) literal##f
#define MANT_DIG FLT_MANT_DIG
#define MAX_EXP FLT_MAX_EXP
#define MIN_EXP FLT_MIN_EXP
#define EXP2 exp2f
#define RANDOM randomf
#include "test-exp2.h"

int
main ()
{
  /* A particular value.  */
  x = 0.6f;
  y = exp2f (x);
  ASSERT (y >= 1.5157164f && y <= 1.5157167f);

  test_function ();

  return 0;
}
