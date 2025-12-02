/* Test of rounding to nearest, breaking ties away from zero.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

#include <config.h>

#include <math.h>

#include "isnanf-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

int
main ()
{
  /* See IEEE 754, section 6.3:
       "the sign of the result of the round floating-point number to
        integral value operation is the sign of the operand. These rules
        shall apply even when operands or results are zero or infinite."  */

  /* Zero.  */
  ASSERT (!signbit (roundf (0.0f)));
  ASSERT (!!signbit (roundf (minus_zerof)) == !!signbit (minus_zerof));
  /* Positive numbers.  */
  ASSERT (!signbit (roundf (0.3f)));
  ASSERT (!signbit (roundf (0.7f)));
  /* Negative numbers.  */
  ASSERT (!!signbit (roundf (-0.3f)) == !!signbit (minus_zerof));
  ASSERT (!!signbit (roundf (-0.7f)) == !!signbit (minus_zerof));

  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (isnanf (roundf (NaNf ())));
  /* Infinity.  */
  ASSERT (roundf (Infinityf ()) == Infinityf ());
  ASSERT (roundf (- Infinityf ()) == - Infinityf ());

  return 0;
}
