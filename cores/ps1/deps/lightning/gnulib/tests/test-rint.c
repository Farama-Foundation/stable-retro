/* Test of rint() function.
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
SIGNATURE_CHECK (rint, double, (double));

#include <float.h>
#include <stdio.h>

#include "isnand-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

#undef INFINITY
#undef NAN

#define DOUBLE double
#define ISNAN isnand
#define INFINITY Infinityd ()
#define NAN NaNd ()
#define L_(literal) literal
#define RINT rint
#define RANDOM randomd
#include "test-rint.h"

int
main ()
{
  /* Consider the current rounding mode, cf.
     <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/float.h.html>  */
  if (FLT_ROUNDS == 1)
    {
      /* The current rounding mode is round-to-nearest
         (the default in IEEE 754).  */

      /* Zero.  */
      ASSERT (rint (0.0) == 0.0);
      ASSERT (rint (minus_zerod) == 0.0);
      /* Positive numbers.  */
      ASSERT (rint (0.3) == 0.0);
      ASSERT (rint (0.5) == 0.0); /* unlike round() */
      ASSERT (rint (0.7) == 1.0);
      ASSERT (rint (1.0) == 1.0);
      ASSERT (rint (1.5) == 2.0);
      ASSERT (rint (1.999) == 2.0);
      ASSERT (rint (2.0) == 2.0);
      ASSERT (rint (2.1) == 2.0);
      ASSERT (rint (2.5) == 2.0); /* unlike round() */
      ASSERT (rint (2.7) == 3.0);
      ASSERT (rint (65535.999) == 65536.0);
      ASSERT (rint (65536.0) == 65536.0);
      ASSERT (rint (65536.001) == 65536.0);
      ASSERT (rint (2.341e31) == 2.341e31);
      /* Negative numbers.  */
      ASSERT (rint (-0.3) == 0.0);
      ASSERT (rint (-0.5) == 0.0); /* unlike round() */
      ASSERT (rint (-0.7) == -1.0);
      ASSERT (rint (-1.0) == -1.0);
      ASSERT (rint (-1.5) == -2.0);
      ASSERT (rint (-1.999) == -2.0);
      ASSERT (rint (-2.0) == -2.0);
      ASSERT (rint (-2.1) == -2.0);
      ASSERT (rint (-2.5) == -2.0); /* unlike round() */
      ASSERT (rint (-2.7) == -3.0);
      ASSERT (rint (-65535.999) == -65536.0);
      ASSERT (rint (-65536.0) == -65536.0);
      ASSERT (rint (-65536.001) == -65536.0);
      ASSERT (rint (-2.341e31) == -2.341e31);

      test_function ();

      return 0;
    }
  else
    {
      fputs ("Skipping test: non-standard rounding mode\n", stderr);
      return 77;
    }
}
