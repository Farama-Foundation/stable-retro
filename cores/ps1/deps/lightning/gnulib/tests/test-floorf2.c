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

/* When this test fails on some platform, build it together with the gnulib
   module 'fprintf-posix' for optimal debugging output.  */

#include <config.h>

#include <math.h>

#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "isnanf-nolibm.h"
#include "macros.h"

/* MSVC with option -fp:strict refuses to compile constant initializers that
   contain floating-point operations.  Pacify this compiler.  */
#if defined _MSC_VER && !defined __clang__
# pragma fenv_access (off)
#endif


/* The reference implementation, taken from lib/floor.c.  */

#define DOUBLE float
#define MANT_DIG FLT_MANT_DIG
#define L_(literal) literal##f

/* 2^(MANT_DIG-1).  */
static const DOUBLE TWO_MANT_DIG =
  /* Assume MANT_DIG <= 5 * 31.
     Use the identity
       n = floor(n/5) + floor((n+1)/5) + ... + floor((n+4)/5).  */
  (DOUBLE) (1U << ((MANT_DIG - 1) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 1) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 2) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 3) / 5))
  * (DOUBLE) (1U << ((MANT_DIG - 1 + 4) / 5));

DOUBLE
floorf_reference (DOUBLE x)
{
  /* The use of 'volatile' guarantees that excess precision bits are dropped
     at each addition step and before the following comparison at the caller's
     site.  It is necessary on x86 systems where double-floats are not IEEE
     compliant by default, to avoid that the results become platform and compiler
     option dependent.  'volatile' is a portable alternative to gcc's
     -ffloat-store option.  */
  volatile DOUBLE y = x;
  volatile DOUBLE z = y;

  if (z > L_(0.0))
    {
      /* For 0 < x < 1, return +0.0 even if the current rounding mode is
         FE_DOWNWARD.  */
      if (z < L_(1.0))
        z = L_(0.0);
      /* Avoid rounding errors for values near 2^k, where k >= MANT_DIG-1.  */
      else if (z < TWO_MANT_DIG)
        {
          /* Round to the next integer (nearest or up or down, doesn't matter).  */
          z += TWO_MANT_DIG;
          z -= TWO_MANT_DIG;
          /* Enforce rounding down.  */
          if (z > y)
            z -= L_(1.0);
        }
    }
  else if (z < L_(0.0))
    {
      /* Work around ICC's desire to optimize denormal floats to 0.  */
      if (z > -FLT_MIN)
        return L_(-1.0);
      /* Avoid rounding errors for values near -2^k, where k >= MANT_DIG-1.  */
      if (z > - TWO_MANT_DIG)
        {
          /* Round to the next integer (nearest or up or down, doesn't matter).  */
          z -= TWO_MANT_DIG;
          z += TWO_MANT_DIG;
          /* Enforce rounding down.  */
          if (z > y)
            z -= L_(1.0);
        }
    }
  return z;
}


/* Test for equality.  */
static int
equal (DOUBLE x, DOUBLE y)
{
  return (isnanf (x) ? isnanf (y) : x == y);
}

/* Test whether the result for a given argument is correct.  */
static bool
correct_result_p (DOUBLE x, DOUBLE result)
{
  return
    (x < 0 && x >= -1 ? result == - L_(1.0) :
     x - 1 < x ? result <= x && result >= x - 1 && x - result < 1 :
     equal (result, x));
}

/* Test the function for a given argument.  */
static int
check (float x)
{
  /* If the reference implementation is incorrect, bail out immediately.  */
  float reference = floorf_reference (x);
  ASSERT (correct_result_p (x, reference));
  /* If the actual implementation is wrong, return an error code.  */
  {
    float result = floorf (x);
    if (correct_result_p (x, result))
      return 0;
    else
      {
#if GNULIB_TEST_FPRINTF_POSIX
        fprintf (stderr, "floorf %g(%a) = %g(%a) or %g(%a)?\n",
                 x, x, reference, reference, result, result);
#endif
        return 1;
      }
  }
}

#define NUM_HIGHBITS 12
#define NUM_LOWBITS 4

int
main ()
{
  unsigned int highbits;
  unsigned int lowbits;
  int error = 0;
  for (highbits = 0; highbits < (1 << NUM_HIGHBITS); highbits++)
    for (lowbits = 0; lowbits < (1 << NUM_LOWBITS); lowbits++)
      {
        /* Combine highbits and lowbits into a floating-point number,
           sign-extending the lowbits to 32-NUM_HIGHBITS bits.  */
        union { float f; uint32_t i; } janus;
        janus.i = ((uint32_t) highbits << (32 - NUM_HIGHBITS))
                  | ((uint32_t) ((int32_t) ((uint32_t) lowbits << (32 - NUM_LOWBITS))
                                 >> (32 - NUM_LOWBITS - NUM_HIGHBITS))
                     >> NUM_HIGHBITS);
        error |= check (janus.f);
      }
  return (error ? 1 : 0);
}
