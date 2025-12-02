/* Test of <search.h> substitute in C++ mode.
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

#include <search.h>

#include "signature.h"


#if GNULIB_TEST_TSEARCH
SIGNATURE_CHECK (GNULIB_NAMESPACE::tsearch, void *,
                 (const void *, void **, _gl_search_compar_fn));
SIGNATURE_CHECK (GNULIB_NAMESPACE::tfind, void *,
                 (const void *, void *const *, _gl_search_compar_fn));
SIGNATURE_CHECK (GNULIB_NAMESPACE::tdelete, void *,
                 (const void *, void **, _gl_search_compar_fn));
SIGNATURE_CHECK (GNULIB_NAMESPACE::twalk, void,
                 (const void *, _gl_search_action_fn));
#endif


int
main ()
{
}
