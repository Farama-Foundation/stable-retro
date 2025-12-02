/* Test of <dirent.h> substitute in C++ mode.
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

#include <dirent.h>

#include "signature.h"


#if GNULIB_TEST_OPENDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::opendir, DIR *, (const char *));
#endif

#if GNULIB_TEST_READDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::readdir, struct dirent *, (DIR *));
#endif

#if GNULIB_TEST_REWINDDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::rewinddir, void, (DIR *));
#endif

#if GNULIB_TEST_CLOSEDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::closedir, int, (DIR *));
#endif

#if GNULIB_TEST_DIRFD
SIGNATURE_CHECK (GNULIB_NAMESPACE::dirfd, int, (DIR *));
#endif

#if GNULIB_TEST_FDOPENDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::fdopendir, DIR *, (int));
#endif

#if GNULIB_TEST_SCANDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::scandir, int,
                 (const char *, struct dirent ***,
                  int (*) (const struct dirent *),
                  int (*) (const struct dirent **, const struct dirent **)));
#endif

#if GNULIB_TEST_ALPHASORT
SIGNATURE_CHECK (GNULIB_NAMESPACE::alphasort, int,
                 (const struct dirent **, const struct dirent **));
#endif


int
main ()
{
}
