/* Test of <uchar.h> substitute in C++ mode.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2019.  */

#define GNULIB_NAMESPACE gnulib
#include <config.h>

#include <uchar.h>

#include "signature.h"


#if GNULIB_TEST_BTOC32
SIGNATURE_CHECK (GNULIB_NAMESPACE::btoc32, wint_t, (int));
#endif

#if GNULIB_TEST_C32ISALNUM
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isalnum, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISALPHA
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isalpha, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISBLANK
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isblank, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISCNTRL
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32iscntrl, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISDIGIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isdigit, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISGRAPH
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isgraph, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISLOWER
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32islower, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISPRINT
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isprint, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISPUNCT
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32ispunct, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISSPACE
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isspace, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISUPPER
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isupper, int, (wint_t));
#endif

#if GNULIB_TEST_C32ISXDIGIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32isxdigit, int, (wint_t));
#endif

#if GNULIB_TEST_C32RTOMB
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32rtomb, size_t,
                 (char *, char32_t , mbstate_t *));
#endif

#if GNULIB_TEST_C32SNRTOMBS
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32snrtombs, size_t,
                 (char *, const char32_t **, size_t, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_C32SRTOMBS
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32srtombs, size_t,
                 (char *, const char32_t **, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_C32STOMBS
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32stombs, size_t,
                 (char *, const char32_t *, size_t));
#endif

#if GNULIB_TEST_C32TOB
SIGNATURE_CHECK (GNULIB_NAMESPACE::c32tob, int, (wint_t));
#endif

#if GNULIB_TEST_MBRTOC32
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbrtoc32, size_t,
                 (char32_t *, const char *, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_MBSNRTOC32S
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbsnrtoc32s, size_t,
                 (char32_t *, const char **, size_t, size_t, mbstate_t *));

#endif

#if GNULIB_TEST_MBSRTOC32S
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbsrtoc32s, size_t,
                 (char32_t *, const char **, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_MBSTOC32S
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbstoc32s, size_t,
                 (char32_t *, const char *, size_t));
#endif


int
main ()
{
}
