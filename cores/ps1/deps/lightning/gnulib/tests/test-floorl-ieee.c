/* Test of rounding towards negative infinity.
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

#include <config.h>

#include <math.h>

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

  /* See IEEE 754, section 6.3:
       "the sign of the result of the round floating-point number to
        integral value operation is the sign of the operand. These rules
        shall apply even when operands or results are zero or infinite."  */

  /* Zero.  */
  ASSERT (!signbit (floorl (0.0L)));
  ASSERT (!!signbit (floorl (minus_zerol)) == !!signbit (minus_zerol));
  /* Positive numbers.  */
  ASSERT (!signbit (floorl (0.3L)));
  ASSERT (!signbit (floorl (0.7L)));
  /* Negative numbers.  */
  ASSERT (!!signbit (floorl (-0.3L)) == !!signbit (minus_zerol));
  ASSERT (!!signbit (floorl (-0.7L)) == !!signbit (minus_zerol));

  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (isnanl (floorl (NaNl ())));
  /* Infinity.  */
  ASSERT (floorl (Infinityl ()) == Infinityl ());
  ASSERT (floorl (- Infinityl ()) == - Infinityl ());

  return 0;
}
