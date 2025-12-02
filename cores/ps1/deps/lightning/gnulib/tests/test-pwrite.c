/* Test the pwrite function.
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

#include <config.h>

#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (pwrite, ssize_t, (int, const void *, size_t, off_t));

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "macros.h"

#define N (sizeof buf - 1)

int
main (void)
{
  char const *file = "out";
  int fd;
  char buf[] = "0123456789";
  off_t pos = 2;
  size_t i;
  off_t init_pos;

  ASSERT (file);

  fd = open (file, O_CREAT | O_WRONLY, 0600);
  ASSERT (0 <= fd);
  ASSERT (write (fd, buf, N) == N);
  ASSERT (close (fd) == 0);

  fd = open (file, O_WRONLY, 0600);
  ASSERT (0 <= fd);

  init_pos = lseek (fd, pos, SEEK_SET);
  ASSERT (init_pos == pos);

  for (i = 0; i < N; i+=2)
    {
      const char byte = 'W';
      ASSERT (pwrite (fd, &byte, 1, i) == 1);
      ASSERT (lseek (fd, 0, SEEK_CUR) == init_pos);
    }

  {
    /* Invalid offset must evoke failure with EINVAL.  */
    const char byte = 'b';
    ASSERT (pwrite (fd, &byte, 1, (off_t) -1) == -1);
    ASSERT (errno == EINVAL);
  }

  ASSERT (close (fd) == 0);

  {
    fd = open (file, O_RDONLY);
    ASSERT (0 <= fd);
    ASSERT (read (fd, buf, N) == N);
    ASSERT (close (fd) == 0);
    ASSERT (strcmp ("W1W3W5W7W9",buf) == 0);
  }

  /* Test behaviour for invalid file descriptors.  */
  {
    char byte = 'x';
    errno = 0;
    ASSERT (pwrite (-1, &byte, 1, 0) == -1);
    ASSERT (errno == EBADF);
  }
  {
    char byte = 'x';
    close (99);
    errno = 0;
    ASSERT (pwrite (99, &byte, 1, 0) == -1);
    ASSERT (errno == EBADF);
  }

  return 0;
}
