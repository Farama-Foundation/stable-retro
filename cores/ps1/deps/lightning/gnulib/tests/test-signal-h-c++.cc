/* Test of <signal.h> substitute in C++ mode.
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

#include <signal.h>

#include "signature.h"


#if GNULIB_TEST_PTHREAD_SIGMASK
SIGNATURE_CHECK (GNULIB_NAMESPACE::pthread_sigmask, int,
                 (int, const sigset_t *, sigset_t *));
#endif

#if GNULIB_TEST_RAISE
SIGNATURE_CHECK (GNULIB_NAMESPACE::raise, int, (int));
#endif

#if GNULIB_TEST_SIGPROCMASK
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigismember, int, (const sigset_t *, int));
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigemptyset, int, (sigset_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigaddset, int, (sigset_t *, int));
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigdelset, int, (sigset_t *, int));
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigfillset, int, (sigset_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigpending, int, (sigset_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigprocmask, int,
                 (int, const sigset_t *, sigset_t *));
SIGNATURE_CHECK (GNULIB_NAMESPACE::signal,
                 _gl_function_taking_int_returning_void_t,
                 (int, _gl_function_taking_int_returning_void_t));
#endif

#if GNULIB_TEST_SIGACTION
SIGNATURE_CHECK (GNULIB_NAMESPACE::sigaction, int,
                 (int, const struct sigaction *, struct sigaction *));
#endif


int
main ()
{
}
