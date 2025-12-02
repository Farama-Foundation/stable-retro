/* Test of passing file descriptors.
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

#include <config.h>

#include "passfd.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "macros.h"

int
main ()
{
#if HAVE_SOCKETPAIR
  int pair[2];
  int ret;
  pid_t pid;
  int status;
  int fdnull;
  int fd;
  struct stat st;

# if HAVE_DECL_ALARM
  /* Avoid hanging on failure.  */
  int alarm_value = 5;
  signal (SIGALRM, SIG_DFL);
  alarm (alarm_value);
# endif

  fdnull = open ("/dev/null", O_RDWR);
  if (fdnull < 0)
    {
      perror ("Could not open /dev/null");
      return 1;
    }

  ret = socketpair (AF_UNIX, SOCK_STREAM, 0, pair);
  if (ret < 0)
    {
      perror ("socket pair failed");
      return 2;
    }

  pid = fork ();
  if (pid == -1)
    {
      perror ("fork");
      return 3;
    }
  if (pid == 0)
    {
      ret = sendfd (pair[1], fdnull);
      if (ret == -1)
        {
          perror ("sendfd");
          return 64;
        }
      return 0;
    }
  /* father */
  else
    {
      ASSERT (close (pair[1]) == 0);
      fd = recvfd (pair[0], 0);
      if (fd == -1)
        {
          perror ("recvfd");
          return 16;
        }
      ret = waitpid (pid, &status, 0);
      if (ret == -1)
        {
          perror ("waitpid");
          return 17;
        }
      ASSERT (ret == pid);

      if (!WIFEXITED (status))
        {
          fprintf (stderr, "Child does not normally exit\n");
          return 65;
        }
      ret = WEXITSTATUS (status);
      if (ret != 0)
        {
          fprintf (stderr, "Send fd fail\n");
          return ret;
        }

      /* try to stat new fd */
      ret = fstat (fd, &st);
      if (ret < 0)
        {
          perror ("fstat");
          return 80;
        }

      /* Check behavior when sender no longer around */
      errno = 0;
      fd = recvfd (pair[0], 0);
      ASSERT (fd == -1);
      ASSERT (errno == ENOTCONN);

      return 0;
    }
#else
  errno = 0;
  ASSERT(sendfd (0, 0) == -1);
  ASSERT(errno == ENOSYS);

  errno = 0;
  ASSERT(recvfd (0, 0) == -1);
  ASSERT(errno == ENOSYS);

  fputs ("skipping test: socketpair not supported on this system\n",
         stderr);
  return 77;
#endif
}
