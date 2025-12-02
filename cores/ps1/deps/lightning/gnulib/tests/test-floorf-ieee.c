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

#include "isnanf-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

/* If IEEE compliance was not requested, the ICC compiler inlines its
   own floorf assembly that turns -0.0f to 0.0f; but that is a correct
   result when IEEE is not enforced.  To avoid spurious failure, we
   have to provide this dummy function in order to outsmart ICC's
   inlining, and call our floorf through a function pointer.  */
static float
dummy (float f)
{
  return 0;
}

int
main (int argc, char **argv _GL_UNUSED)
{
  float (*my_floorf) (float) = argc ? floorf : dummy;

  /* See IEEE 754, section 6.3:
       "the sign of the result of the round floating-point number to
        integral value operation is the sign of the operand. These rules
        shall apply even when operands or results are zero or infinite."  */

  /* Zero.  */
  ASSERT (!signbit (my_floorf (0.0f)));
  ASSERT (!!signbit (my_floorf (minus_zerof)) == !!signbit (minus_zerof));
  /* Positive numbers.  */
  ASSERT (!signbit (my_floorf (0.3f)));
  ASSERT (!signbit (my_floorf (0.7f)));
  /* Negative numbers.  */
  ASSERT (!!signbit (my_floorf (-0.3f)) == !!signbit (minus_zerof));
  ASSERT (!!signbit (my_floorf (-0.7f)) == !!signbit (minus_zerof));

  /* [MX] shaded specification in POSIX.  */

  /* NaN.  */
  ASSERT (isnanf (floorf (NaNf ())));
  /* Infinity.  */
  ASSERT (floorf (Infinityf ()) == Infinityf ());
  ASSERT (floorf (- Infinityf ()) == - Infinityf ());

  return 0;
}
