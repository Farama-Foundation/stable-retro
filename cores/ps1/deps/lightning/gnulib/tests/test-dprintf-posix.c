/* Test of POSIX compatible dprintf() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include <stdio.h>

#include "signature.h"
SIGNATURE_CHECK (dprintf, int, (int, const char *, ...));

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "infinity.h"
#include "macros.h"

static void
test_function (int (*my_dprintf) (int, const char *, ...))
{
  /* Here we don't test output that may be platform dependent.
     The bulk of the tests is done as part of the 'vasnprintf-posix' module.  */

  /* Test support of size specifiers as in C99.  */

  my_dprintf (fileno (stdout), "%ju %d\n", (uintmax_t) 12345671, 33, 44, 55);

  my_dprintf (fileno (stdout), "%zu %d\n", (size_t) 12345672, 33, 44, 55);

  my_dprintf (fileno (stdout), "%tu %d\n", (ptrdiff_t) 12345673, 33, 44, 55);

  /* Test the support of the 'a' and 'A' conversion specifier for hexadecimal
     output of floating-point numbers.  */

  /* Positive zero.  */
  my_dprintf (fileno (stdout), "%a %d\n", 0.0, 33, 44, 55);

  /* Positive infinity.  */
  my_dprintf (fileno (stdout), "%a %d\n", Infinityd (), 33, 44, 55);

  /* Negative infinity.  */
  my_dprintf (fileno (stdout), "%a %d\n", - Infinityd (), 33, 44, 55);

  /* FLAG_ZERO with infinite number.  */
  my_dprintf (fileno (stdout), "%010a %d\n", Infinityd (), 33, 44, 55);

  /* Test the support of the %f format directive.  */

  /* A positive number.  */
  my_dprintf (fileno (stdout), "%f %d\n", 12.75, 33, 44, 55);

  /* A larger positive number.  */
  my_dprintf (fileno (stdout), "%f %d\n", 1234567.0, 33, 44, 55);

  /* A negative number.  */
  my_dprintf (fileno (stdout), "%f %d\n", -0.03125, 33, 44, 55);

  /* Positive zero.  */
  my_dprintf (fileno (stdout), "%f %d\n", 0.0, 33, 44, 55);

  /* FLAG_ZERO.  */
  my_dprintf (fileno (stdout), "%015f %d\n", 1234.0, 33, 44, 55);

  /* Precision.  */
  my_dprintf (fileno (stdout), "%.f %d\n", 1234.0, 33, 44, 55);

  /* Precision with no rounding.  */
  my_dprintf (fileno (stdout), "%.2f %d\n", 999.95, 33, 44, 55);

  /* Precision with rounding.  */
  my_dprintf (fileno (stdout), "%.2f %d\n", 999.996, 33, 44, 55);

  /* A positive number.  */
  my_dprintf (fileno (stdout), "%Lf %d\n", 12.75L, 33, 44, 55);

  /* A larger positive number.  */
  my_dprintf (fileno (stdout), "%Lf %d\n", 1234567.0L, 33, 44, 55);

  /* A negative number.  */
  my_dprintf (fileno (stdout), "%Lf %d\n", -0.03125L, 33, 44, 55);

  /* Positive zero.  */
  my_dprintf (fileno (stdout), "%Lf %d\n", 0.0L, 33, 44, 55);

  /* FLAG_ZERO.  */
  my_dprintf (fileno (stdout), "%015Lf %d\n", 1234.0L, 33, 44, 55);

  /* Precision.  */
  my_dprintf (fileno (stdout), "%.Lf %d\n", 1234.0L, 33, 44, 55);

  /* Precision with no rounding.  */
  my_dprintf (fileno (stdout), "%.2Lf %d\n", 999.95L, 33, 44, 55);

  /* Precision with rounding.  */
  my_dprintf (fileno (stdout), "%.2Lf %d\n", 999.996L, 33, 44, 55);

  /* Test the support of the %F format directive.  */

  /* A positive number.  */
  my_dprintf (fileno (stdout), "%F %d\n", 12.75, 33, 44, 55);

  /* A larger positive number.  */
  my_dprintf (fileno (stdout), "%F %d\n", 1234567.0, 33, 44, 55);

  /* A negative number.  */
  my_dprintf (fileno (stdout), "%F %d\n", -0.03125, 33, 44, 55);

  /* Positive zero.  */
  my_dprintf (fileno (stdout), "%F %d\n", 0.0, 33, 44, 55);

  /* FLAG_ZERO.  */
  my_dprintf (fileno (stdout), "%015F %d\n", 1234.0, 33, 44, 55);

  /* Precision.  */
  my_dprintf (fileno (stdout), "%.F %d\n", 1234.0, 33, 44, 55);

  /* Precision with no rounding.  */
  my_dprintf (fileno (stdout), "%.2F %d\n", 999.95, 33, 44, 55);

  /* Precision with rounding.  */
  my_dprintf (fileno (stdout), "%.2F %d\n", 999.996, 33, 44, 55);

  /* A positive number.  */
  my_dprintf (fileno (stdout), "%LF %d\n", 12.75L, 33, 44, 55);

  /* A larger positive number.  */
  my_dprintf (fileno (stdout), "%LF %d\n", 1234567.0L, 33, 44, 55);

  /* A negative number.  */
  my_dprintf (fileno (stdout), "%LF %d\n", -0.03125L, 33, 44, 55);

  /* Positive zero.  */
  my_dprintf (fileno (stdout), "%LF %d\n", 0.0L, 33, 44, 55);

  /* FLAG_ZERO.  */
  my_dprintf (fileno (stdout), "%015LF %d\n", 1234.0L, 33, 44, 55);

  /* Precision.  */
  my_dprintf (fileno (stdout), "%.LF %d\n", 1234.0L, 33, 44, 55);

  /* Precision with no rounding.  */
  my_dprintf (fileno (stdout), "%.2LF %d\n", 999.95L, 33, 44, 55);

  /* Precision with rounding.  */
  my_dprintf (fileno (stdout), "%.2LF %d\n", 999.996L, 33, 44, 55);

  /* Test the support of the POSIX/XSI format strings with positions.  */

  my_dprintf (fileno (stdout), "%2$d %1$d\n", 33, 55);
}

int
main (int argc, char *argv[])
{
  test_function (dprintf);
  return 0;
}
