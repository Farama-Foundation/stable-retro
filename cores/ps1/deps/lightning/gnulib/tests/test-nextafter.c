/* Test of nextafter() function.
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
SIGNATURE_CHECK (nextafter, double, (double, double));

#include "macros.h"

volatile double x;
double y;

int
main ()
{
  /* A particular value.  */
  x = 1.628947572;
  /* Towards 0.  */
  y = nextafter (x, 0);
  ASSERT (y < x);
  ASSERT (y > 1.628947571);
  y = nextafter (y, x + x);
  ASSERT (y == x);
  /* Towards infinity.  */
  y = nextafter (x, x + x);
  ASSERT (y > x);
  ASSERT (y < 1.628947573);
  y = nextafter (y, 0);
  ASSERT (y == x);
  /* Towards itself.  */
  y = nextafter (x, x);
  ASSERT (y == x);

  return 0;
}
