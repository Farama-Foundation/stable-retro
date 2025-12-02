/* Test of <sys/select.h> substitute in C++ mode.
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

#include <sys/select.h>

#include "signature.h"


#if GNULIB_TEST_PSELECT
SIGNATURE_CHECK (GNULIB_NAMESPACE::pselect, int,
                 (int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
                  struct timespec const *restrict, sigset_t const *restrict));
#endif

#if GNULIB_TEST_SELECT
SIGNATURE_CHECK (GNULIB_NAMESPACE::select, int,
                 (int, fd_set *, fd_set *, fd_set *, struct timeval *));
#endif


int
main ()
{
}
