/* Test of yn() function.
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
SIGNATURE_CHECK (yn, double, (int, double));

#include "macros.h"

volatile double x;
double y;

int
main ()
{
  /* A particular value.  */
  x = 3.8;
  y = yn (0, x);
  ASSERT (y >= 0.0645032466 && y <= 0.0645032467);

  /* A particular value.  */
  x = 3.8;
  y = yn (1, x);
  ASSERT (y >= 0.4141146893 && y <= 0.4141146894);

  /* A particular value.  */
  x = 3.8;
  y = yn (2, x);
  ASSERT (y >= 0.1534518529 && y <= 0.1534518530);

  /* A particular value.  */
  x = 3.8;
  y = yn (3, x);
  ASSERT (y >= -0.2525864231 && y <= -0.2525864230);

  /* A particular value.  */
  x = 3.8;
  y = yn (4, x);
  ASSERT (y >= -0.5522725209 && y <= -0.5522725208);

  /* A particular value.  */
  x = 3.8;
  y = yn (5, x);
  ASSERT (y >= -0.9100925684 && y <= -0.9100925683);

  /* A particular value.  */
  x = 3.8;
  y = yn (6, x);
  ASSERT (y >= -1.842707923 && y <= -1.842707922);

  /* A particular value.  */
  x = 3.8;
  y = yn (7, x);
  ASSERT (y >= -4.908985081 && y <= -4.908985080);

  /* A particular value.  */
  x = 3.8;
  y = yn (8, x);
  ASSERT (y >= -16.24302659 && y <= -16.24302658);

  /* A particular value.  */
  x = 3.8;
  y = yn (9, x);
  ASSERT (y >= -63.48270581 && y <= -63.48270580);

  /* A particular value.  */
  x = 3.8;
  y = yn (10, x);
  ASSERT (y >= -284.4645273 && y <= -284.4645272);

  return 0;
}
