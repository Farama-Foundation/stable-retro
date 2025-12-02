/* Test of logbl() function.
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
SIGNATURE_CHECK (logbl, long double, (long double));

#include <float.h>

#include "minus-zero.h"
#include "macros.h"

#define DOUBLE long double
#define VOLATILE
#define HUGEVAL HUGE_VALL
#define L_(literal) literal##L
#define MINUS_ZERO minus_zerol
#define MAX_EXP LDBL_MAX_EXP
#define MIN_EXP LDBL_MIN_EXP
/* On MIPS IRIX machines, LDBL_MIN_EXP is -1021, but the smallest reliable
   exponent for 'long double' is -964.  Similarly, on PowerPC machines,
   LDBL_MIN_EXP is -1021, but the smallest reliable exponent for 'long double'
   is -968.  For exponents below that, the precision may be truncated to the
   precision used for 'double'.  */
#ifdef __sgi
# define MIN_NORMAL_EXP (LDBL_MIN_EXP + 57)
#elif defined __ppc || defined __ppc__ || defined __powerpc || defined __powerpc__
# define MIN_NORMAL_EXP (LDBL_MIN_EXP + 53)
#else
# define MIN_NORMAL_EXP LDBL_MIN_EXP
#endif
#define LOGB logbl
#define RANDOM randoml
#include "test-logb.h"

int
main ()
{
  test_function ();

  return 0;
}
