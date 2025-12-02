/* Test of fabsl() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2010-2012.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (fabsl, long double, (long double));

#include <string.h>

#include "macros.h"
#include "minus-zero.h"

#define DOUBLE long double
#define L_(literal) literal##L
#define MINUS_ZERO minus_zerol
#define FABS fabsl
#define RANDOM randoml
#include "test-fabs.h"

int
main ()
{
  /* A particular positive value.  */
  x = 0.6L;
  y = fabsl (x);
  ASSERT (y == 0.6L);

  /* A particular negative value.  */
  x = -0.6L;
  y = fabsl (x);
  ASSERT (y == 0.6L);

  test_function ();

  return 0;
}
