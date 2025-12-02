/* Test changing the protections of a file.
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

#include <sys/stat.h>

#include "signature.h"
SIGNATURE_CHECK (fchmod, int, (int, mode_t));

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "macros.h"

#define BASE "test-fchmod."

int
main (void)
{
  /* Test behaviour for invalid file descriptors.  */
  {
    errno = 0;
    ASSERT (fchmod (-1, 0600) == -1);
    ASSERT (errno == EBADF);
  }
  {
    close (99);
    errno = 0;
    ASSERT (fchmod (99, 0600) == -1);
    ASSERT (errno == EBADF);
  }

  /* Test that fchmod works on regular files.  */
  {
    struct stat statbuf;
    int fd;

    unlink (BASE "file");
    ASSERT (close (creat (BASE "file", 0600)) == 0);
    fd = open (BASE "file", O_RDWR);
    ASSERT (fd >= 0);
    ASSERT (fchmod (fd, 0400) == 0);
    ASSERT (stat (BASE "file", &statbuf) >= 0);
    ASSERT ((statbuf.st_mode & 0700) == 0400);
    ASSERT (close (fd) == 0);

    /* Clean up.  */
    ASSERT (chmod (BASE "file", 0600) == 0);
    ASSERT (unlink (BASE "file") == 0);
  }

  return 0;
}
