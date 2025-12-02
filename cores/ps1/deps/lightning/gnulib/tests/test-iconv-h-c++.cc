/* Test of <iconv.h> substitute in C++ mode.
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

#define GNULIB_NAMESPACE gnulib
#include <config.h>

#if HAVE_ICONV
# include <iconv.h>

# include "signature.h"


#if GNULIB_TEST_ICONV
SIGNATURE_CHECK (GNULIB_NAMESPACE::iconv_open, iconv_t,
                 (const char *, const char *));
#endif

#if GNULIB_TEST_ICONV
SIGNATURE_CHECK (GNULIB_NAMESPACE::iconv, size_t,
                 (iconv_t, ICONV_CONST char **, size_t *, char **, size_t *));
#endif

#if GNULIB_TEST_ICONV
SIGNATURE_CHECK (GNULIB_NAMESPACE::iconv_close, int, (iconv_t));
#endif

#endif


int
main ()
{
}
