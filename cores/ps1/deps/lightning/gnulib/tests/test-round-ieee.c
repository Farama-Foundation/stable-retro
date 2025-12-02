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

#include "isnand-nolibm.h"
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
  ASSERT (!signbit (round (0.0)));
  ASSERT (!!signbit (round (minus_zerod)) == !!signbit (minus_zerod));
  /* Positive numbers.  */
  ASSERT (!signbit (round (0.3)));
  ASSERT (!signbit (round (0.7)));
  /* Negative numbers.  */
  ASSERT (!!signbit (round (-0.3)) == !!signbit (minus_zerod));
  ASSERT (!!signbit (round (-0.7)) == !!signbit (minus_zerod));

  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (isnand (round (NaNd ())));
  /* Infinity.  */
  ASSERT (round (Infinityd ()) == Infinityd ());
  ASSERT (round (- Infinityd ()) == - Infinityd ());

  return 0;
}
