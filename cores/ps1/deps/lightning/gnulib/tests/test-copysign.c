/* Test of copysign() function.
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
SIGNATURE_CHECK (copysign, double, (double, double));

#include "macros.h"
#include "minus-zero.h"

#include <string.h>

volatile double x;
volatile double y;
double z;
double zero = 0.0;

int
main ()
{
  /* A particular value in the first quadrant.  */
  x = 0.6;
  y = 0.8;
  z = copysign (x, y);
  ASSERT (z == 0.6);

  /* A particular value in the second quadrant.  */
  x = -0.6;
  y = 0.8;
  z = copysign (x, y);
  ASSERT (z == 0.6);

  /* A particular value in the third quadrant.  */
  x = -0.6;
  y = -0.8;
  z = copysign (x, y);
  ASSERT (z == -0.6);

  /* A particular value in the fourth quadrant.  */
  x = 0.6;
  y = -0.8;
  z = copysign (x, y);
  ASSERT (z == -0.6);

  /* From signed zero.  */
  x = 1.0;
  y = 0.0;
  z = copysign (x, y);
  ASSERT (z == 1.0);

  x = 1.0;
  y = minus_zerod;
  z = copysign (x, y);
  /* Assume all gnulib targets support -0.0, until proven otherwise.  */
  ASSERT (z == -1.0);

  x = -1.0;
  y = 0.0;
  z = copysign (x, y);
  ASSERT (z == 1.0);

  x = -1.0;
  y = minus_zerod;
  z = copysign (x, y);
  ASSERT (z == -1.0);

  /* To signed zero.  */
  x = 0.0;
  y = 1.0;
  z = copysign (x, y);
  ASSERT (z == 0.0);
  ASSERT (memcmp (&z, &zero, sizeof z) == 0);

  x = 0.0;
  y = -1.0;
  z = copysign (x, y);
  ASSERT (z == 0.0);
  ASSERT (memcmp (&z, &zero, sizeof z) != 0);

  x = minus_zerod;
  y = 1.0;
  z = copysign (x, y);
  ASSERT (z == 0.0);
  ASSERT (memcmp (&z, &zero, sizeof z) == 0);

  x = minus_zerod;
  y = -1.0;
  z = copysign (x, y);
  ASSERT (z == 0.0);
  ASSERT (memcmp (&z, &zero, sizeof z) != 0);

  return 0;
}
