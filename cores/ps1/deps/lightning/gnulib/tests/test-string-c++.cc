/* Test of <string.h> substitute in C++ mode.
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

#include <string.h>

#include "signature.h"


#if GNULIB_TEST_FFSL
SIGNATURE_CHECK (GNULIB_NAMESPACE::ffsl, int, (long int));
#endif

#if GNULIB_TEST_FFSLL
SIGNATURE_CHECK (GNULIB_NAMESPACE::ffsll, int, (long long int));
#endif

#if GNULIB_TEST_MEMCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::memchr, void *,
                 (void const *, int, size_t));
#endif

//SIGNATURE_CHECK (GNULIB_NAMESPACE::memcmp, int,
//                 (void const *, void const *, size_t));

#if GNULIB_TEST_MEMMEM
SIGNATURE_CHECK (GNULIB_NAMESPACE::memmem, void *,
                 (void const *, size_t, void const *, size_t));
#endif

#if GNULIB_TEST_MEMPCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::mempcpy, void *,
                 (void *, void const *, size_t));
#endif

#if GNULIB_TEST_MEMRCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::memrchr, void *,
                 (void const *, int, size_t));
#endif

#if GNULIB_TEST_RAWMEMCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::rawmemchr, void *, (void const *, int));
#endif

#if GNULIB_TEST_STPCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::stpcpy, char *, (char *, char const *));
#endif

#if GNULIB_TEST_STPNCPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::stpncpy, char *,
                 (char *, char const *, size_t));
#endif

#if GNULIB_TEST_STRCHRNUL
SIGNATURE_CHECK (GNULIB_NAMESPACE::strchrnul, char *, (char const *, int));
#endif

#if GNULIB_TEST_STRDUP
SIGNATURE_CHECK (GNULIB_NAMESPACE::strdup, char *, (char const *));
#endif

#if GNULIB_TEST_STRNCAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::strncat, char *,
                 (char *, const char *, size_t));
#endif

#if GNULIB_TEST_STRNDUP
SIGNATURE_CHECK (GNULIB_NAMESPACE::strndup, char *, (char const *, size_t));
#endif

#if GNULIB_TEST_STRNLEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::strnlen, size_t, (char const *, size_t));
#endif

#if GNULIB_TEST_STRPBRK
SIGNATURE_CHECK (GNULIB_NAMESPACE::strpbrk, char *,
                 (char const *, char const *));
#endif

#if GNULIB_TEST_STRSEP
SIGNATURE_CHECK (GNULIB_NAMESPACE::strsep, char *, (char **, char const *));
#endif

#if GNULIB_TEST_STRSTR
SIGNATURE_CHECK (GNULIB_NAMESPACE::strstr, char *,
                 (const char *, const char *));
#endif

#if GNULIB_TEST_STRCASESTR
SIGNATURE_CHECK (GNULIB_NAMESPACE::strcasestr, char *,
                 (const char *, const char *));
#endif

#if GNULIB_TEST_STRTOK_R
SIGNATURE_CHECK (GNULIB_NAMESPACE::strtok_r, char *,
                 (char *, char const *, char **));
#endif

#if GNULIB_TEST_MBSLEN
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbslen, size_t, (const char *));
#endif

#if GNULIB_TEST_MBSCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbschr, char *, (const char *, int));
#endif

#if GNULIB_TEST_MBSRCHR
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbsrchr, char *, (const char *, int));
#endif

#if GNULIB_TEST_MBSPBRK
SIGNATURE_CHECK (GNULIB_NAMESPACE::mbspbrk, char *,
                 (const char *, const char *));
#endif

#if GNULIB_TEST_STRERROR
SIGNATURE_CHECK (GNULIB_NAMESPACE::strerror, char *, (int));
#endif

#if GNULIB_TEST_STRERROR_R
SIGNATURE_CHECK (GNULIB_NAMESPACE::strerror_r, int, (int, char *, size_t));
#endif

#if GNULIB_TEST_STRERRORNAME_NP
SIGNATURE_CHECK (GNULIB_NAMESPACE::strerrorname_np, const char *, (int));
#endif

#if GNULIB_TEST_SIGABBREV_NP
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigabbrev_np, const char *, (int));
#endif

#if GNULIB_TEST_SIGDESCR_NP
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigdescr_np, const char *, (int));
#endif

#if GNULIB_TEST_STRSIGNAL
SIGNATURE_CHECK (GNULIB_NAMESPACE::strsignal, char *, (int));
#endif

#if GNULIB_TEST_STRVERSCMP
SIGNATURE_CHECK (GNULIB_NAMESPACE::strverscmp, int,
                 (const char *, const char *));
#endif


int
main ()
{
}
