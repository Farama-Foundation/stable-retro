/* Test of vdprintf() function.
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

#include <stdio.h>

#include "signature.h"
SIGNATURE_CHECK (vdprintf, int, (int, const char *, va_list));

#include <errno.h>
#include <unistd.h>

#include "macros.h"

static int
my_dprintf (int fd, const char *format, ...)
{
  va_list args;
  int ret;

  va_start (args, format);
  ret = vdprintf (fd, format, args);
  va_end (args);
  return ret;
}

int
main (int argc, char *argv[])
{
  /* Test behaviour for invalid file descriptors.  */
  {
    errno = 0;
    ASSERT (my_dprintf (-1, "test") == -1);
    ASSERT (errno == EBADF);
  }
  {
    close (99);
    errno = 0;
    ASSERT (my_dprintf (99, "test") == -1);
    ASSERT (errno == EBADF);
  }

  return 0;
}
