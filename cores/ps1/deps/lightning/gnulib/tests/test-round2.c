/* Test of rounding to nearest, breaking ties away from zero.
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

/* Written by Ben Pfaff <blp@gnu.org>, 2007.
   Heavily based on code by Bruno Haible. */

/* When this test fails on some platform, build it together with the gnulib
   module 'fprintf-posix' for optimal debugging output.  */

/* Get the two reference implementations of round under the names
   round_reference1 and round_reference2.

   round.c will #include <config.h> for us. */
#define FLOOR_BASED_ROUND round_reference1
#define FLOOR_FREE_ROUND round_reference2
#include "round.c"

#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "verify.h"

#ifdef USE_LONG_DOUBLE
# error Long double not supported.
#elif ! defined USE_FLOAT
# include "isnand-nolibm.h"
# define ISNAN isnand
# define FUNCTION "round"
# define DOUBLE_UINT uint64_t
# define DOUBLE_BITS 64
# define NUM_HIGHBITS 13
# define NUM_LOWBITS 4
#else /* defined USE_FLOAT */
# include "isnanf-nolibm.h"
# define ISNAN isnanf
# define FUNCTION "roundf"
# define DOUBLE_UINT uint32_t
# define DOUBLE_BITS 32
# define NUM_HIGHBITS 12
# define NUM_LOWBITS 4
#endif

/* Test for equality.  */
static bool
equal (const char *message, DOUBLE x, DOUBLE y0, DOUBLE y1)
{
  if (ISNAN (y0) ? ISNAN (y1) : y0 == y1)
    return true;
  else
    {
#if GNULIB_TEST_FPRINTF_POSIX
      fprintf (stderr, "%s: "FUNCTION"(%g(%a)) = %g(%a) or %g(%a)?\n",
               message, x, x, y0, y0, y1, y1);
#endif
      return false;
    }
}

/* Test the function for a given argument.  */
static bool
check (DOUBLE x)
{
  DOUBLE ref1 = round_reference1 (x);
  DOUBLE ref2 = round_reference2 (x);
  DOUBLE result = ROUND (x);

  /* If the reference implementations disagree, bail out immediately.  */
  if (!equal ("reference implementations disagree", x, ref1, ref2))
    exit (EXIT_FAILURE);

  /* If the actual implementation is wrong, return an error code.  */
  return equal ("bad round implementation", x, ref1, result);
}

int
main (void)
{
  DOUBLE_UINT highbits, lowbits;
  int error = 0;
  for (highbits = 0; highbits < (1 << NUM_HIGHBITS); highbits++)
    for (lowbits = 0; lowbits < (1 << NUM_LOWBITS); lowbits++)
      {
        /* Combine highbits and lowbits into a floating-point number,
           sign-extending the lowbits to DOUBLE_BITS-NUM_HIGHBITS bits.  */
        union { DOUBLE f; DOUBLE_UINT i; } janus;
        verify (sizeof janus.f == sizeof janus.i);
        janus.i = lowbits | (highbits << (DOUBLE_BITS - NUM_HIGHBITS));
        if (lowbits >> (NUM_LOWBITS - 1))
          janus.i |= ((DOUBLE_UINT) -1
                      >> (NUM_LOWBITS + NUM_HIGHBITS)
                      << NUM_LOWBITS);
        if (!check (janus.f))
          error = true;
      }
  return (error ? 1 : 0);
}
