/* Test of rounding towards negative infinity.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <math.h>

#include "signature.h"
SIGNATURE_CHECK (floorl, long double, (long double));

#include <float.h>

#include "fpucw.h"
#include "isnanl-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

int
main ()
{
  DECL_LONG_DOUBLE_ROUNDING

  BEGIN_LONG_DOUBLE_ROUNDING ();

  /* Zero.  */
  ASSERT (floorl (0.0L) == 0.0L);
  ASSERT (floorl (minus_zerol) == 0.0L);
  /* Positive numbers.  */
  ASSERT (floorl (0.3L) == 0.0L);
  ASSERT (floorl (0.7L) == 0.0L);
  ASSERT (floorl (1.0L) == 1.0L);
  ASSERT (floorl (1.5L) == 1.0L);
  ASSERT (floorl (1.999L) == 1.0L);
  ASSERT (floorl (2.0L) == 2.0L);
  ASSERT (floorl (65535.999L) == 65535.0L);
  ASSERT (floorl (65536.0L) == 65536.0L);
  ASSERT (floorl (2.341e31L) == 2.341e31L);
  /* Negative numbers.  */
  ASSERT (floorl (-0.3L) == -1.0L);
  ASSERT (floorl (-0.7L) == -1.0L);
  ASSERT (floorl (-1.0L) == -1.0L);
  ASSERT (floorl (-1.001L) == -2.0L);
  ASSERT (floorl (-1.5L) == -2.0L);
  ASSERT (floorl (-1.999L) == -2.0L);
  ASSERT (floorl (-2.0L) == -2.0L);
  ASSERT (floorl (-65535.999L) == -65536.0L);
  ASSERT (floorl (-65536.0L) == -65536.0L);
  ASSERT (floorl (-2.341e31L) == -2.341e31L);
  /* Infinite numbers.  */
  ASSERT (floorl (Infinityl ()) == Infinityl ());
  ASSERT (floorl (- Infinityl ()) == - Infinityl ());
  /* NaNs.  */
  ASSERT (isnanl (floorl (NaNl ())));

  return 0;
}
