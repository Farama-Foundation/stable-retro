/* Test truncating a file.
   Copyright (C) 2017-2021 Free Software Foundation, Inc.

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
SIGNATURE_CHECK (truncate, int, (const char *, off_t));

#include <errno.h>
#include <fcntl.h>

#include "ignore-value.h"
#include "macros.h"

#define BASE "test-truncate.t"

int
main (int argc, char *argv[])
{
  /* Clean up any trash from prior testsuite runs.  */
  ignore_value (system ("rm -rf " BASE "*"));

  {
    int fd = open (BASE "file", O_RDWR | O_TRUNC | O_CREAT, 0600);
    ASSERT (fd >= 0);
    ASSERT (write (fd, "Hello", 5) == 5);
    close (fd);
  }

  {
    int fd = open (BASE "file", O_RDONLY);
    ASSERT (fd >= 0);
    ASSERT (lseek (fd, 0, SEEK_END) == 5);
    close (fd);
  }

  /* Test increasing the size.  */
  ASSERT (truncate (BASE "file", 314159) == 0);
  {
    int fd = open (BASE "file", O_RDONLY);
    ASSERT (fd >= 0);
    ASSERT (lseek (fd, 0, SEEK_END) == 314159);
    close (fd);
  }

  /* Test reducing the size.  */
  ASSERT (truncate (BASE "file", 3) == 0);
  {
    int fd = open (BASE "file", O_RDONLY);
    ASSERT (fd >= 0);
    ASSERT (lseek (fd, 0, SEEK_END) == 3);
    close (fd);
  }

  /* Test reducing the size to 0.  */
  ASSERT (truncate (BASE "file", 0) == 0);
  {
    int fd = open (BASE "file", O_RDONLY);
    ASSERT (fd >= 0);
    ASSERT (lseek (fd, 0, SEEK_END) == 0);
    close (fd);
  }

  /* Test behaviour for nonexistent files.  */
  {
    errno = 0;
    ASSERT (truncate ("/nonexistent", 0) == -1);
    ASSERT (errno == ENOENT);
  }
  /* Test behaviour for directories.  */
  {
    errno = 0;
    ASSERT (truncate (".", 0) == -1);
    ASSERT (errno == EISDIR
            || /* on native Windows */ errno == EACCES);
  }
  /* Test behaviour for trailing slashes.  */
  {
    errno = 0;
    ASSERT (truncate (BASE "file/", 0) == -1);
    ASSERT (errno == ENOTDIR
            || /* on native Windows */ errno == EINVAL);
  }
  /* Test behaviour for invalid lengths.  */
  {
    errno = 0;
    ASSERT (truncate (BASE "file", -3) == -1);
    ASSERT (errno == EINVAL);
  }

  /* Cleanup.  */
  ASSERT (unlink (BASE "file") == 0);

  return 0;
}
