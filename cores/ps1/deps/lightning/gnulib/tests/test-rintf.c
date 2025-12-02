/* Test of rintf() function.
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
SIGNATURE_CHECK (rintf, float, (float));

#include <float.h>
#include <stdio.h>

#include "isnanf-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

#undef INFINITY
#undef NAN

#define DOUBLE float
#define ISNAN isnanf
#define INFINITY Infinityf ()
#define NAN NaNf ()
#define L_(literal) literal##f
#define RINT rintf
#define RANDOM randomf
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
      ASSERT (rintf (0.0f) == 0.0f);
      ASSERT (rintf (minus_zerof) == 0.0f);
      /* Positive numbers.  */
      ASSERT (rintf (0.3f) == 0.0f);
      ASSERT (rintf (0.5f) == 0.0f); /* unlike roundf() */
      ASSERT (rintf (0.7f) == 1.0f);
      ASSERT (rintf (1.0f) == 1.0f);
      ASSERT (rintf (1.5f) == 2.0f);
      ASSERT (rintf (1.999f) == 2.0f);
      ASSERT (rintf (2.0f) == 2.0f);
      ASSERT (rintf (2.1f) == 2.0f);
      ASSERT (rintf (2.5f) == 2.0f); /* unlike roundf() */
      ASSERT (rintf (2.7f) == 3.0f);
      ASSERT (rintf (65535.99f) == 65536.0f);
      ASSERT (rintf (65536.0f) == 65536.0f);
      ASSERT (rintf (65536.01f) == 65536.0f);
      ASSERT (rintf (2.341e31f) == 2.341e31f);
      /* Negative numbers.  */
      ASSERT (rintf (-0.3f) == 0.0f);
      ASSERT (rintf (-0.5f) == 0.0f); /* unlike roundf() */
      ASSERT (rintf (-0.7f) == -1.0f);
      ASSERT (rintf (-1.0f) == -1.0f);
      ASSERT (rintf (-1.5f) == -2.0f);
      ASSERT (rintf (-1.999f) == -2.0f);
      ASSERT (rintf (-2.0f) == -2.0f);
      ASSERT (rintf (-2.1f) == -2.0f);
      ASSERT (rintf (-2.5f) == -2.0f); /* unlike roundf() */
      ASSERT (rintf (-2.7f) == -3.0f);
      ASSERT (rintf (-65535.99f) == -65536.0f);
      ASSERT (rintf (-65536.0f) == -65536.0f);
      ASSERT (rintf (-65536.01f) == -65536.0f);
      ASSERT (rintf (-2.341e31f) == -2.341e31f);

      test_function ();

      return 0;
    }
  else
    {
      fputs ("Skipping test: non-standard rounding mode\n", stderr);
      return 77;
    }
}
