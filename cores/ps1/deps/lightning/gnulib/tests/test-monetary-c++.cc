/* Test of <monetary.h> substitute in C++ mode.
   Copyright (C) 2017-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2017.  */

#define GNULIB_NAMESPACE gnulib
#include <config.h>

#if HAVE_MONETARY_H
# include <monetary.h>
#endif

#include "signature.h"


#if GNULIB_TEST_STRFMON_L
SIGNATURE_CHECK (GNULIB_NAMESPACE::strfmon_l, ssize_t,
                 (char *s, size_t maxsize, locale_t locale,
                  const char *format, ...));
#endif


int
main ()
{
}
