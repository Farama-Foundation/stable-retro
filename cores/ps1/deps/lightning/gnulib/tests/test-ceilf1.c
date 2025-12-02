/* Test of rounding towards positive infinity.
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
SIGNATURE_CHECK (ceilf, float, (float));

#include "isnanf-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

/* If IEEE compliance was not requested, the ICC compiler inlines its
   own ceilf assembly that turns -0.0f to 0.0f; but that is a correct
   result when IEEE is not enforced.  To avoid spurious failure, we
   have to provide this dummy function in order to outsmart ICC's
   inlining, and call our ceilf through a function pointer.  */
static float
dummy (float f)
{
  return 0;
}

int
main (int argc, char **argv _GL_UNUSED)
{
  float (*my_ceilf) (float) = argc ? ceilf : dummy;

  /* Zero.  */
  ASSERT (my_ceilf (0.0f) == 0.0f);
  ASSERT (my_ceilf (minus_zerof) == 0.0f);
  /* Positive numbers.  */
  ASSERT (my_ceilf (0.3f) == 1.0f);
  ASSERT (my_ceilf (0.7f) == 1.0f);
  ASSERT (my_ceilf (1.0f) == 1.0f);
  ASSERT (my_ceilf (1.001f) == 2.0f);
  ASSERT (my_ceilf (1.5f) == 2.0f);
  ASSERT (my_ceilf (1.999f) == 2.0f);
  ASSERT (my_ceilf (2.0f) == 2.0f);
  ASSERT (my_ceilf (65535.99f) == 65536.0f);
  ASSERT (my_ceilf (65536.0f) == 65536.0f);
  ASSERT (my_ceilf (2.341e31f) == 2.341e31f);
  /* Negative numbers.  */
  ASSERT (my_ceilf (-0.3f) == 0.0f);
  ASSERT (my_ceilf (-0.7f) == 0.0f);
  ASSERT (my_ceilf (-1.0f) == -1.0f);
  ASSERT (my_ceilf (-1.5f) == -1.0f);
  ASSERT (my_ceilf (-1.999f) == -1.0f);
  ASSERT (my_ceilf (-2.0f) == -2.0f);
  ASSERT (my_ceilf (-65535.99f) == -65535.0f);
  ASSERT (my_ceilf (-65536.0f) == -65536.0f);
  ASSERT (my_ceilf (-2.341e31f) == -2.341e31f);
  /* Infinite numbers.  */
  ASSERT (my_ceilf (Infinityf ()) == Infinityf ());
  ASSERT (my_ceilf (- Infinityf ()) == - Infinityf ());
  /* NaNs.  */
  ASSERT (isnanf (my_ceilf (NaNf ())));

  return 0;
}
