/* Test of <sys/stat.h> substitute in C++ mode.
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

#include <sys/stat.h>

#include "signature.h"


#if GNULIB_TEST_FCHMODAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::fchmodat, int,
                 (int, char const *, mode_t, int));
#endif

#if GNULIB_TEST_FSTAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::fstat, int, (int, struct stat *));
#endif

#if GNULIB_TEST_FSTATAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::fstatat, int,
                 (int, char const *, struct stat *, int));
#endif

#if GNULIB_TEST_FUTIMENS
SIGNATURE_CHECK (GNULIB_NAMESPACE::futimens, int,
                 (int, struct timespec const[2]));
#endif

#if GNULIB_TEST_GETUMASK
SIGNATURE_CHECK (GNULIB_NAMESPACE::getumask, mode_t, (void));
#endif

#if GNULIB_TEST_LCHMOD
SIGNATURE_CHECK (GNULIB_NAMESPACE::lchmod, int, (const char *, mode_t));
#endif

#if GNULIB_TEST_LSTAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::lstat, int, (const char *, struct stat *));
#endif

SIGNATURE_CHECK (GNULIB_NAMESPACE::mkdir, int, (char const *, mode_t));

#if GNULIB_TEST_MKDIRAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::mkdirat, int, (int, char const *, mode_t));
#endif

#if GNULIB_TEST_MKFIFO
SIGNATURE_CHECK (GNULIB_NAMESPACE::mkfifo, int, (char const *, mode_t));
#endif

#if GNULIB_TEST_MKFIFOAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::mkfifoat, int, (int, char const *, mode_t));
#endif

#if GNULIB_TEST_MKNOD
SIGNATURE_CHECK (GNULIB_NAMESPACE::mknod, int, (char const *, mode_t, dev_t));
#endif

#if GNULIB_TEST_MKNODAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::mknodat, int,
                 (int, char const *, mode_t, dev_t));
#endif

#if GNULIB_TEST_STAT
//SIGNATURE_CHECK (GNULIB_NAMESPACE::stat, int, (const char *, struct stat *));
#endif

#if GNULIB_TEST_UTIMENSAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::utimensat, int,
                 (int, char const *, struct timespec const[2], int));
#endif


int
main ()
{
}
