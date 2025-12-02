/* Test of rintl() function.
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
SIGNATURE_CHECK (rintl, long double, (long double));

#include <float.h>
#include <stdio.h>

#include "fpucw.h"
#include "isnanl-nolibm.h"
#include "minus-zero.h"
#include "infinity.h"
#include "nan.h"
#include "macros.h"

#undef INFINITY
#undef NAN

#define DOUBLE long double
#define ISNAN isnanl
#define INFINITY Infinityl ()
#define NAN NaNl ()
#define L_(literal) literal##L
#define RINT rintl
#define RANDOM randoml
#include "test-rint.h"

int
main ()
{
  DECL_LONG_DOUBLE_ROUNDING

  BEGIN_LONG_DOUBLE_ROUNDING ();

  /* Consider the current rounding mode, cf.
     <https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/float.h.html>  */
  if (FLT_ROUNDS == 1)
    {
      /* The current rounding mode is round-to-nearest
         (the default in IEEE 754).  */

      /* Zero.  */
      ASSERT (rintl (0.0L) == 0.0L);
      ASSERT (rintl (minus_zerol) == 0.0L);
      /* Positive numbers.  */
      ASSERT (rintl (0.3L) == 0.0L);
      ASSERT (rintl (0.5L) == 0.0L); /* unlike roundl() */
      ASSERT (rintl (0.7L) == 1.0L);
      ASSERT (rintl (1.0L) == 1.0L);
      ASSERT (rintl (1.5L) == 2.0L);
      ASSERT (rintl (1.999L) == 2.0L);
      ASSERT (rintl (2.0L) == 2.0L);
      ASSERT (rintl (2.1L) == 2.0L);
      ASSERT (rintl (2.5L) == 2.0L); /* unlike roundl() */
      ASSERT (rintl (2.7L) == 3.0L);
      ASSERT (rintl (65535.999L) == 65536.0L);
      ASSERT (rintl (65536.0L) == 65536.0L);
      ASSERT (rintl (65536.001L) == 65536.0L);
      ASSERT (rintl (2.341e31L) == 2.341e31L);
      /* Negative numbers.  */
      ASSERT (rintl (-0.3L) == 0.0L);
      ASSERT (rintl (-0.5L) == 0.0L); /* unlike roundl() */
      ASSERT (rintl (-0.7L) == -1.0L);
      ASSERT (rintl (-1.0L) == -1.0L);
      ASSERT (rintl (-1.5L) == -2.0L);
      ASSERT (rintl (-1.999L) == -2.0L);
      ASSERT (rintl (-2.0L) == -2.0L);
      ASSERT (rintl (-2.1L) == -2.0L);
      ASSERT (rintl (-2.5L) == -2.0L); /* unlike roundl() */
      ASSERT (rintl (-2.7L) == -3.0L);
      ASSERT (rintl (-65535.999L) == -65536.0L);
      ASSERT (rintl (-65536.0L) == -65536.0L);
      ASSERT (rintl (-65536.001L) == -65536.0L);
      ASSERT (rintl (-2.341e31L) == -2.341e31L);

      test_function ();

      return 0;
    }
  else
    {
      fputs ("Skipping test: non-standard rounding mode\n", stderr);
      return 77;
    }
}
