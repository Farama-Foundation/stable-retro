/* Test of hypotf() function.
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
SIGNATURE_CHECK (hypotf, float, (float, float));

#include <float.h>

#include "macros.h"

#undef MIN
#undef MAX

#define DOUBLE float
#define HUGEVAL HUGE_VALF
#define L_(literal) literal##f
#define MANT_DIG FLT_MANT_DIG
#define MIN FLT_MIN
#define MAX FLT_MAX
#define HYPOT hypotf
#define RANDOM randomf
#include "test-hypot.h"

int
main ()
{
  /* A particular value.  */
  x = 0.4f;
  y = 0.6f;
  z = hypot (x, y);
  ASSERT (z >= 0.7211102f && z <= 0.7211103f);

  test_function ();

  return 0;
}
