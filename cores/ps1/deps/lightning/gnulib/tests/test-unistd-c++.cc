/* Test of <unistd.h> substitute in C++ mode.
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

#include <unistd.h>

#include "signature.h"


#if GNULIB_TEST_ACCESS
SIGNATURE_CHECK (GNULIB_NAMESPACE::access, int, (const char *, int));
#endif

#if GNULIB_TEST_CHDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::chdir, int, (const char *));
#endif

#if GNULIB_TEST_CHOWN
SIGNATURE_CHECK (GNULIB_NAMESPACE::chown, int, (const char *, uid_t, gid_t));
#endif

#if GNULIB_TEST_CLOSE
SIGNATURE_CHECK (GNULIB_NAMESPACE::close, int, (int));
#endif

#if GNULIB_TEST_DUP
SIGNATURE_CHECK (GNULIB_NAMESPACE::dup, int, (int));
#endif

#if GNULIB_TEST_DUP2
SIGNATURE_CHECK (GNULIB_NAMESPACE::dup2, int, (int, int));
#endif

#if GNULIB_TEST_DUP3
SIGNATURE_CHECK (GNULIB_NAMESPACE::dup3, int, (int, int, int));
#endif

#if GNULIB_TEST_EXECL
SIGNATURE_CHECK (GNULIB_NAMESPACE::execl, int,
                 (const char *, const char *, ...));
#endif

#if GNULIB_TEST_EXECLE
SIGNATURE_CHECK (GNULIB_NAMESPACE::execle, int,
                 (const char *, const char *, ...));
#endif

#if GNULIB_TEST_EXECLP
SIGNATURE_CHECK (GNULIB_NAMESPACE::execlp, int,
                 (const char *, const char *, ...));
#endif

#if GNULIB_TEST_EXECV
SIGNATURE_CHECK (GNULIB_NAMESPACE::execv, int,
                 (const char *, char * const *));
#endif

#if GNULIB_TEST_EXECVE
SIGNATURE_CHECK (GNULIB_NAMESPACE::execve, int,
                 (const char *, char * const *, char * const *));
#endif

#if GNULIB_TEST_EXECVP
SIGNATURE_CHECK (GNULIB_NAMESPACE::execvp, int,
                 (const char *, char * const *));
#endif

#if GNULIB_TEST_EXECVPE
SIGNATURE_CHECK (GNULIB_NAMESPACE::execvpe, int,
                 (const char *, char * const *, char * const *));
#endif

#if GNULIB_TEST_EUIDACCESS
SIGNATURE_CHECK (GNULIB_NAMESPACE::euidaccess, int, (const char *, int));
#endif

#if GNULIB_TEST_FACCESSAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::faccessat, int,
                 (int, char const *, int, int));
#endif

#if GNULIB_TEST_FCHDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::fchdir, int, (int));
#endif

#if GNULIB_TEST_FCHOWNAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::fchownat, int,
                 (int, char const *, uid_t, gid_t, int));
#endif

#if GNULIB_TEST_FDATASYNC
SIGNATURE_CHECK (GNULIB_NAMESPACE::fdatasync, int, (int));
#endif

#if GNULIB_TEST_FSYNC
SIGNATURE_CHECK (GNULIB_NAMESPACE::fsync, int, (int));
#endif

#if GNULIB_TEST_FTRUNCATE
SIGNATURE_CHECK (GNULIB_NAMESPACE::ftruncate, int, (int, off_t));
#endif

#if GNULIB_TEST_GETCWD
SIGNATURE_CHECK (GNULIB_NAMESPACE::getcwd, char *, (char *, size_t));
#endif

#if GNULIB_TEST_GETDOMAINNAME
SIGNATURE_CHECK (GNULIB_NAMESPACE::getdomainname, int, (char *, size_t));
#endif

#if GNULIB_TEST_GETDTABLESIZE
SIGNATURE_CHECK (GNULIB_NAMESPACE::getdtablesize, int, (void));
#endif

#if GNULIB_TEST_GETENTROPY
SIGNATURE_CHECK (GNULIB_NAMESPACE::getentropy, int, (void *, size_t));
#endif

#if GNULIB_TEST_GETGROUPS
SIGNATURE_CHECK (GNULIB_NAMESPACE::getgroups, int, (int, gid_t *));
#endif

#if GNULIB_TEST_GETHOSTNAME
SIGNATURE_CHECK (GNULIB_NAMESPACE::gethostname, int, (char *, size_t));
#endif

#if GNULIB_TEST_GETLOGIN
SIGNATURE_CHECK (GNULIB_NAMESPACE::getlogin, char *, (void));
#endif

#if GNULIB_TEST_GETLOGIN_R
SIGNATURE_CHECK (GNULIB_NAMESPACE::getlogin_r, int, (char *, size_t));
#endif

#if GNULIB_TEST_GETPAGESIZE
SIGNATURE_CHECK (GNULIB_NAMESPACE::getpagesize, int, (void));
#endif

#if GNULIB_TEST_GETPASS
SIGNATURE_CHECK (GNULIB_NAMESPACE::getpass, char *, (const char *));
#endif

#if GNULIB_TEST_GETUSERSHELL
SIGNATURE_CHECK (GNULIB_NAMESPACE::getusershell, char *, (void));
#endif

#if GNULIB_TEST_GETUSERSHELL
SIGNATURE_CHECK (GNULIB_NAMESPACE::setusershell, void, (void));
#endif

#if GNULIB_TEST_GETUSERSHELL
SIGNATURE_CHECK (GNULIB_NAMESPACE::endusershell, void, (void));
#endif

#if GNULIB_TEST_GROUP_MEMBER
SIGNATURE_CHECK (GNULIB_NAMESPACE::group_member, int, (gid_t));
#endif

#if GNULIB_TEST_ISATTY
SIGNATURE_CHECK (GNULIB_NAMESPACE::isatty, int, (int));
#endif

#if GNULIB_TEST_LCHOWN
SIGNATURE_CHECK (GNULIB_NAMESPACE::lchown, int, (char const *, uid_t, gid_t));
#endif

#if GNULIB_TEST_LINK
SIGNATURE_CHECK (GNULIB_NAMESPACE::link, int, (const char *, const char *));
#endif

#if GNULIB_TEST_LINKAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::linkat, int,
                 (int, const char *, int, const char *, int));
#endif

#if GNULIB_TEST_LSEEK
SIGNATURE_CHECK (GNULIB_NAMESPACE::lseek, off_t, (int, off_t, int));
#endif

#if GNULIB_TEST_PIPE
SIGNATURE_CHECK (GNULIB_NAMESPACE::pipe, int, (int[2]));
#endif

#if GNULIB_TEST_PIPE2
SIGNATURE_CHECK (GNULIB_NAMESPACE::pipe2, int, (int[2], int));
#endif

#if GNULIB_TEST_PREAD
SIGNATURE_CHECK (GNULIB_NAMESPACE::pread, ssize_t,
                 (int, void *, size_t, off_t));
#endif

#if GNULIB_TEST_PWRITE
SIGNATURE_CHECK (GNULIB_NAMESPACE::pwrite, ssize_t,
                 (int, const void *, size_t, off_t));
#endif

#if GNULIB_TEST_READ
SIGNATURE_CHECK (GNULIB_NAMESPACE::read, ssize_t, (int, void *, size_t));
#endif

#if GNULIB_TEST_READLINK
SIGNATURE_CHECK (GNULIB_NAMESPACE::readlink, ssize_t,
                 (const char *, char *, size_t));
#endif

#if GNULIB_TEST_READLINKAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::readlinkat, ssize_t,
                 (int, char const *, char *, size_t));
#endif

#if GNULIB_TEST_RMDIR
SIGNATURE_CHECK (GNULIB_NAMESPACE::rmdir, int, (char const *));
#endif

#if GNULIB_TEST_SETHOSTNAME
SIGNATURE_CHECK (GNULIB_NAMESPACE::sethostname, int, (const char *, size_t));
#endif

#if GNULIB_TEST_SLEEP
SIGNATURE_CHECK (GNULIB_NAMESPACE::sleep, unsigned int, (unsigned int));
#endif

#if GNULIB_TEST_SYMLINK
SIGNATURE_CHECK (GNULIB_NAMESPACE::symlink, int, (char const *, char const *));
#endif

#if GNULIB_TEST_SYMLINKAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::symlinkat, int,
                 (char const *, int, char const *));
#endif

#if GNULIB_TEST_TRUNCATE
SIGNATURE_CHECK (GNULIB_NAMESPACE::truncate, int, (const char *, off_t));
#endif

#if GNULIB_TEST_TTYNAME_R
SIGNATURE_CHECK (GNULIB_NAMESPACE::ttyname_r, int,
                 (int fd, char *buf, size_t buflen));
#endif

#if GNULIB_TEST_UNLINK
SIGNATURE_CHECK (GNULIB_NAMESPACE::unlink, int, (char const *));
#endif

#if GNULIB_TEST_UNLINKAT
SIGNATURE_CHECK (GNULIB_NAMESPACE::unlinkat, int, (int, char const *, int));
#endif

#if GNULIB_TEST_USLEEP
SIGNATURE_CHECK (GNULIB_NAMESPACE::usleep, int, (useconds_t));
#endif

#if GNULIB_TEST_WRITE
SIGNATURE_CHECK (GNULIB_NAMESPACE::write, ssize_t,
                 (int, const void *, size_t));
#endif


int
main ()
{
}
