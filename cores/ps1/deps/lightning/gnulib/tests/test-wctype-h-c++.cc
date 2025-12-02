/* Test of <wctype.h> substitute in C++ mode.
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

#include <wctype.h>

#include "signature.h"


SIGNATURE_CHECK (GNULIB_NAMESPACE::iswalnum, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswalpha, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswcntrl, int, (wint_t));
#if GNULIB_TEST_ISWDIGIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswdigit, int, (wint_t));
#endif
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswgraph, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswlower, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswprint, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswpunct, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswspace, int, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswupper, int, (wint_t));
#if GNULIB_TEST_ISWXDIGIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswxdigit, int, (wint_t));
#endif
#if GNULIB_TEST_ISWBLANK
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswblank, int, (wint_t));
#endif

#if GNULIB_TEST_WCTYPE
SIGNATURE_CHECK (GNULIB_NAMESPACE::wctype, wctype_t, (const char *));
#endif

#if GNULIB_TEST_ISWCTYPE
SIGNATURE_CHECK (GNULIB_NAMESPACE::iswctype, int, (wint_t, wctype_t));
#endif

SIGNATURE_CHECK (GNULIB_NAMESPACE::towlower, wint_t, (wint_t));
SIGNATURE_CHECK (GNULIB_NAMESPACE::towupper, wint_t, (wint_t));

#if GNULIB_TEST_WCTRANS
SIGNATURE_CHECK (GNULIB_NAMESPACE::wctrans, wctrans_t, (const char *));
#endif

#if GNULIB_TEST_TOWCTRANS
SIGNATURE_CHECK (GNULIB_NAMESPACE::towctrans, wint_t, (wint_t, wctrans_t));
#endif


int
main ()
{
}
