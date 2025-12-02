/* Test manipulation of non-blocking flag.
   Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

/* Written by Eric Blake <ebb9@byu.net>, 2011.  */

#include <config.h>

#include "nonblocking.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "macros.h"

int
main (void)
{
  const char *file = "test-nonblock.tmp";
  int fd_file;
  int fd_pipe[2];

  fd_file = creat (file, 0600);

  /* Assume std descriptors were provided by invoker.  */
  ASSERT (STDERR_FILENO < fd_file);

  /* Test regular files; setting nonblocking on file is unspecified.  */
  ASSERT (get_nonblocking_flag (fd_file) == 0);
  ASSERT (set_nonblocking_flag (fd_file, false) == 0);
  ASSERT (get_nonblocking_flag (fd_file) == 0);
  ASSERT (close (fd_file) == 0);
  ASSERT (unlink (file) == 0);

  /* Test directories; setting nonblocking is unspecified.  */
  fd_file = open (".", O_RDONLY);
  if (STDERR_FILENO < fd_file)
    {
      /* mingw can't open directories unless fchdir module is active.  */
      ASSERT (get_nonblocking_flag (fd_file) == 0);
      ASSERT (set_nonblocking_flag (fd_file, false) == 0);
      ASSERT (get_nonblocking_flag (fd_file) == 0);
      ASSERT (close (fd_file) == 0);
    }

  /* Test pipes.  */
  ASSERT (pipe (fd_pipe) == 0);
  ASSERT (get_nonblocking_flag (fd_pipe[0]) == 0);
  ASSERT (get_nonblocking_flag (fd_pipe[1]) == 0);
  ASSERT (set_nonblocking_flag (fd_pipe[0], true) == 0);
  ASSERT (get_nonblocking_flag (fd_pipe[0]) == 1);
  ASSERT (get_nonblocking_flag (fd_pipe[1]) == 0);
  ASSERT (set_nonblocking_flag (fd_pipe[1], true) == 0);
  ASSERT (set_nonblocking_flag (fd_pipe[0], false) == 0);
  ASSERT (get_nonblocking_flag (fd_pipe[0]) == 0);
  ASSERT (get_nonblocking_flag (fd_pipe[1]) == 1);
  ASSERT (close (fd_pipe[0]) == 0);
  ASSERT (close (fd_pipe[1]) == 0);

#if GNULIB_TEST_PIPE2
  ASSERT (pipe2 (fd_pipe, O_NONBLOCK) == 0);
  ASSERT (get_nonblocking_flag (fd_pipe[0]) == 1);
  ASSERT (get_nonblocking_flag (fd_pipe[1]) == 1);
  ASSERT (close (fd_pipe[0]) == 0);
  ASSERT (close (fd_pipe[1]) == 0);
#endif /* GNULIB_TEST_PIPE2 */

#if GNULIB_TEST_SOCKET
  {
    /* Test sockets.  */
    bool sock_works = true;
    int fd_sock;

# if defined _WIN32 && ! defined __CYGWIN__
    /* For now, we can't get nonblocking status of windows sockets.  */
    sock_works = false;
# endif

    fd_sock = socket (AF_INET, SOCK_STREAM, 0);
    ASSERT (get_nonblocking_flag (fd_sock) == (sock_works ? 0 : -1));
    ASSERT (set_nonblocking_flag (fd_sock, true) == 0);
    ASSERT (get_nonblocking_flag (fd_sock) == (sock_works ? 1 : -1));
    ASSERT (set_nonblocking_flag (fd_sock, false) == 0);
    ASSERT (get_nonblocking_flag (fd_sock) == (sock_works ? 0 : -1));
    ASSERT (close (fd_sock) == 0);

# if SOCK_NONBLOCK
    fd_sock = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    ASSERT (get_nonblocking_flag (fd_sock) == (sock_works ? 1 : -1));
    ASSERT (close (fd_sock) == 0);
# endif /* SOCK_NONBLOCK */
  }
#endif /* GNULIB_TEST_SOCKET */

  /* Test error handling.  */
  {
    errno = 0;
    ASSERT (get_nonblocking_flag (-1) == -1);
    ASSERT (errno == EBADF);
  }
  {
    errno = 0;
    ASSERT (set_nonblocking_flag (-1, false) == -1);
    ASSERT (errno == EBADF);
  }
  {
    errno = 0;
    ASSERT (set_nonblocking_flag (-1, true) == -1);
    ASSERT (errno == EBADF);
  }
  {
    errno = 0;
    ASSERT (set_nonblocking_flag (getdtablesize (), false) == -1);
    ASSERT (errno == EBADF);
  }

  return 0;
}
