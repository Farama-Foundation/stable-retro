/* Test of POSIX compatible printf() function.
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

#include <stdio.h>

#include "signature.h"
SIGNATURE_CHECK (printf, int, (char const *, ...));

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "macros.h"

#include "test-printf-posix.h"

int
main (int argc, char *argv[])
{
  test_function (printf);
  return 0;
}

/* Test whether __attribute__ (__format__ (...)) still works.  */
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || defined __clang__) && !defined __STRICT_ANSI__
extern int func1 (char *, size_t, const char *, ...)
     __attribute__ ((__format__ (__printf__, 3, 4)));
extern int func2 (char *, size_t, const char *, ...)
     __attribute__ ((__format__ (printf, 3, 4)));
extern int func3 (char *, size_t, const char *, ...)
     __attribute__ ((format (__printf__, 3, 4)));
extern int func4 (char *, size_t, const char *, ...)
     __attribute__ ((format (printf, 3, 4)));
#endif
