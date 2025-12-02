/* Test duplicating file descriptors.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Eric Blake <ebb9@byu.net>, 2009,
   and Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (dup3, int, (int, int, int));

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#if defined _WIN32 && ! defined __CYGWIN__
/* Get declarations of the native Windows API functions.  */
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
/* Get _get_osfhandle.  */
# if GNULIB_MSVC_NOTHROW
#  include "msvc-nothrow.h"
# else
#  include <io.h>
# endif
#endif

#include "binary-io.h"
#include "macros.h"

/* Return true if FD is open.  */
static bool
is_open (int fd)
{
#if defined _WIN32 && ! defined __CYGWIN__
  /* On native Windows, the initial state of unassigned standard file
     descriptors is that they are open but point to an
     INVALID_HANDLE_VALUE, and there is no fcntl.  */
  return (HANDLE) _get_osfhandle (fd) != INVALID_HANDLE_VALUE;
#else
# ifndef F_GETFL
#  error Please port fcntl to your platform
# endif
  return 0 <= fcntl (fd, F_GETFL);
#endif
}

/* Return true if FD is not inherited to child processes.  */
static bool
is_cloexec (int fd)
{
#if defined _WIN32 && ! defined __CYGWIN__
  HANDLE h = (HANDLE) _get_osfhandle (fd);
  DWORD flags;
  ASSERT (GetHandleInformation (h, &flags));
  return (flags & HANDLE_FLAG_INHERIT) == 0;
#else
  int flags;
  ASSERT ((flags = fcntl (fd, F_GETFD)) >= 0);
  return (flags & FD_CLOEXEC) != 0;
#endif
}

int
main ()
{
  int use_cloexec;
  int bad_fd = getdtablesize ();

#if O_CLOEXEC
  for (use_cloexec = 0; use_cloexec <= 1; use_cloexec++)
#else
  use_cloexec = 0;
#endif
    {
      const char *file = "test-dup3.tmp";
      int fd = open (file, O_CREAT | O_TRUNC | O_RDWR, 0600);
      int o_flags;
      char buffer[1];

      o_flags = 0;
#if O_CLOEXEC
      if (use_cloexec)
        o_flags |= O_CLOEXEC;
#endif

      /* Assume std descriptors were provided by invoker.  */
      ASSERT (STDERR_FILENO < fd);
      ASSERT (is_open (fd));
      /* Ignore any other fd's leaked into this process.  */
      close (fd + 1);
      close (fd + 2);
      ASSERT (!is_open (fd + 1));
      ASSERT (!is_open (fd + 2));

      /* Assigning to self is invalid.  */
      errno = 0;
      ASSERT (dup3 (fd, fd, o_flags) == -1);
      ASSERT (errno == EINVAL);
      ASSERT (is_open (fd));

      /* If the source is not open, then the destination is unaffected.  */
      errno = 0;
      ASSERT (dup3 (fd + 1, fd + 2, o_flags) == -1);
      ASSERT (errno == EBADF);
      ASSERT (!is_open (fd + 2));
      errno = 0;
      ASSERT (dup3 (fd + 1, fd, o_flags) == -1);
      ASSERT (errno == EBADF);
      ASSERT (is_open (fd));

      /* The destination must be valid.  */
      errno = 0;
      ASSERT (dup3 (fd, -2, o_flags) == -1);
      ASSERT (errno == EBADF);
      if (bad_fd > 256)
        {
          ASSERT (dup3 (fd, 255, 0) == 255);
          ASSERT (dup3 (fd, 256, 0) == 256);
          ASSERT (close (255) == 0);
          ASSERT (close (256) == 0);
        }
      ASSERT (dup3 (fd, bad_fd - 1, 0) == bad_fd - 1);
      ASSERT (close (bad_fd - 1) == 0);
      errno = 0;
      ASSERT (dup3 (fd, bad_fd, o_flags) == -1);
      ASSERT (errno == EBADF);

      /* Using dup3 can skip fds.  */
      ASSERT (dup3 (fd, fd + 2, o_flags) == fd + 2);
      ASSERT (is_open (fd));
      ASSERT (!is_open (fd + 1));
      ASSERT (is_open (fd + 2));
      if (use_cloexec)
        ASSERT (is_cloexec (fd + 2));
      else
        ASSERT (!is_cloexec (fd + 2));

      /* Verify that dup3 closes the previous occupant of a fd.  */
      ASSERT (open ("/dev/null", O_WRONLY, 0600) == fd + 1);
      ASSERT (dup3 (fd + 1, fd, o_flags) == fd);
      ASSERT (close (fd + 1) == 0);
      ASSERT (write (fd, "1", 1) == 1);
      ASSERT (dup3 (fd + 2, fd, o_flags) == fd);
      ASSERT (lseek (fd, 0, SEEK_END) == 0);
      ASSERT (write (fd + 2, "2", 1) == 1);
      ASSERT (lseek (fd, 0, SEEK_SET) == 0);
      ASSERT (read (fd, buffer, 1) == 1);
      ASSERT (*buffer == '2');

      /* Clean up.  */
      ASSERT (close (fd + 2) == 0);
      ASSERT (close (fd) == 0);
      ASSERT (unlink (file) == 0);
    }

  return 0;
}
