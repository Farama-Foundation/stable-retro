/* Test of <wchar.h> substitute in C++ mode.
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

#include <wchar.h>

#include "signature.h"


#if GNULIB_TEST_BTOWC
SIGNATURE_CHECK (GNULIB_NAMESPACE::btowc, wint_t, (int));
#endif

#if GNULIB_TEST_WCTOB
SIGNATURE_CHECK (GNULIB_NAMESPACE::wctob, int, (wint_t));
#endif

#if GNULIB_TEST_MBSINIT
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbsinit, int, (const mbstate_t *));
#endif

#if GNULIB_TEST_MBRTOWC
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbrtowc, size_t,
                 (wchar_t *, const char *, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_MBRLEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbrlen, size_t,
                 (const char *, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_MBSRTOWCS
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbsrtowcs, size_t,
                 (wchar_t *, const char **, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_MBSNRTOWCS
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbsnrtowcs, size_t,
                 (wchar_t *, const char **, size_t, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_WCRTOMB
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcrtomb, size_t,
                 (char *, wchar_t, mbstate_t *));
#endif

#if GNULIB_TEST_WCSRTOMBS
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsrtombs, size_t,
                 (char *, const wchar_t **, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_WCSNRTOMBS
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsnrtombs, size_t,
                 (char *, const wchar_t **, size_t, size_t, mbstate_t *));
#endif

#if GNULIB_TEST_WCWIDTH
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcwidth, int, (wchar_t));
#endif

#if GNULIB_TEST_WMEMCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::wmemchr, wchar_t *,
                 (const wchar_t *, wchar_t, size_t));
#endif

#if GNULIB_TEST_WMEMCMP
SIGNATURE_CHECK (GNULIB_NAMESPACE::wmemcmp, int,
                 (const wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WMEMCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::wmemcpy, wchar_t *,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WMEMMOVE
SIGNATURE_CHECK (GNULIB_NAMESPACE::wmemmove, wchar_t *,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WMEMPCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::wmempcpy, wchar_t *,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WMEMSET
SIGNATURE_CHECK (GNULIB_NAMESPACE::wmemset, wchar_t *,
                 (wchar_t *, wchar_t, size_t));
#endif

#if GNULIB_TEST_WCSLEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcslen, size_t, (const wchar_t *));
#endif

#if GNULIB_TEST_WCSNLEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsnlen, size_t, (const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcscpy, wchar_t *,
                 (wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCPCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcpcpy, wchar_t *,
                 (wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSNCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsncpy, wchar_t *,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCPNCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcpncpy, wchar_t *,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSCAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcscat, wchar_t *,
                 (wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSNCAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsncat, wchar_t *,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSCMP
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcscmp, int,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSNCMP
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsncmp, int,
                 (const wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSCASECMP
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcscasecmp, int,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSNCASECMP
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsncasecmp, int,
                 (const wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSCOLL
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcscoll, int,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSXFRM
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsxfrm, size_t,
                 (wchar_t *, const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSDUP
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsdup, wchar_t *, (const wchar_t *));
#endif

#if GNULIB_TEST_WCSCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcschr, wchar_t *,
                 (const wchar_t *, wchar_t));
#endif

#if GNULIB_TEST_WCSRCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsrchr, wchar_t *,
                 (const wchar_t *, wchar_t));
#endif

#if GNULIB_TEST_WCSCSPN
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcscspn, size_t,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSSPN
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsspn, size_t,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSPBRK
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcspbrk, wchar_t *,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSSTR
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsstr, wchar_t *,
                 (const wchar_t *, const wchar_t *));
#endif

#if GNULIB_TEST_WCSTOK
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcstok, wchar_t *,
                 (wchar_t *, const wchar_t *, wchar_t **));
#endif

#if GNULIB_TEST_WCSWIDTH
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcswidth, int, (const wchar_t *, size_t));
#endif

#if GNULIB_TEST_WCSFTIME
SIGNATURE_CHECK (GNULIB_NAMESPACE::wcsftime, size_t,
                 (wchar_t *, size_t, const wchar_t *, const struct tm *));
#endif


int
main ()
{
}
