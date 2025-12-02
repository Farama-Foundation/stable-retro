/* Test of jn() function.
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
SIGNATURE_CHECK (jn, double, (int, double));

#include "macros.h"

volatile double x;
double y;

int
main ()
{
  /* A particular value.  */
  x = 3.8;
  y = jn (0, x);
  ASSERT (y >= -0.4025564102 && y <= -0.4025564101);

  /* A particular value.  */
  x = 3.8;
  y = jn (1, x);
  ASSERT (y >= 0.0128210029 && y <= 0.0128210030);

  /* A particular value.  */
  x = 3.8;
  y = jn (2, x);
  ASSERT (y >= 0.4093043064 && y <= 0.4093043065);

  /* A particular value.  */
  x = 3.8;
  y = jn (3, x);
  ASSERT (y >= 0.4180256354 && y <= 0.4180256355);

  /* A particular value.  */
  x = 3.8;
  y = jn (4, x);
  ASSERT (y >= 0.2507361705 && y <= 0.2507361706);

  /* A particular value.  */
  x = 3.8;
  y = jn (5, x);
  ASSERT (y >= 0.1098399867 && y <= 0.1098399868);

  /* A particular value.  */
  x = 3.8;
  y = jn (6, x);
  ASSERT (y >= 0.0383164262 && y <= 0.0383164263);

  /* A particular value.  */
  x = 3.8;
  y = jn (7, x);
  ASSERT (y >= 0.0111592540 && y <= 0.0111592541);

  /* A particular value.  */
  x = 3.8;
  y = jn (8, x);
  ASSERT (y >= 0.0027966149 && y <= 0.0027966150);

  /* A particular value.  */
  x = 3.8;
  y = jn (9, x);
  ASSERT (y >= 0.0006159669 && y <= 0.0006159670);

  /* A particular value.  */
  x = 3.8;
  y = jn (10, x);
  ASSERT (y >= 0.0001211233 && y <= 0.0001211234);

  return 0;
}
