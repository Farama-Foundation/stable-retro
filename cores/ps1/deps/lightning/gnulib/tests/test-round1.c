/* Test of rounding to nearest, breaking ties away from zero.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff <blp@gnu.org>, 2007.
   Based heavily on Bruno Haible's test-trunc.c. */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (round, double, (double));

#include "isnand-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

int
main ()
{
  /* Zero.  */
  ASSERT (round (0.0) == 0.0);
  ASSERT (round (minus_zerod) == 0.0);
  /* Positive numbers.  */
  ASSERT (round (0.3) == 0.0);
  ASSERT (round (0.5) == 1.0);
  ASSERT (round (0.7) == 1.0);
  ASSERT (round (1.0) == 1.0);
  ASSERT (round (1.5) == 2.0);
  ASSERT (round (2.5) == 3.0);
  ASSERT (round (1.999) == 2.0);
  ASSERT (round (2.0) == 2.0);
  ASSERT (round (65535.999) == 65536.0);
  ASSERT (round (65536.0) == 65536.0);
  ASSERT (round (65536.001) == 65536.0);
  ASSERT (round (2.341e31) == 2.341e31);
  /* Negative numbers.  */
  ASSERT (round (-0.3) == 0.0);
  ASSERT (round (-0.5) == -1.0);
  ASSERT (round (-0.7) == -1.0);
  ASSERT (round (-1.0) == -1.0);
  ASSERT (round (-1.5) == -2.0);
  ASSERT (round (-2.5) == -3.0);
  ASSERT (round (-1.999) == -2.0);
  ASSERT (round (-2.0) == -2.0);
  ASSERT (round (-65535.999) == -65536.0);
  ASSERT (round (-65536.0) == -65536.0);
  ASSERT (round (-65536.001) == -65536.0);
  ASSERT (round (-2.341e31) == -2.341e31);
  /* Infinite numbers.  */
  ASSERT (round (Infinityd ()) == Infinityd ());
  ASSERT (round (- Infinityd ()) == - Infinityd ());
  /* NaNs.  */
  ASSERT (isnand (round (NaNd ())));

  return 0;
}
