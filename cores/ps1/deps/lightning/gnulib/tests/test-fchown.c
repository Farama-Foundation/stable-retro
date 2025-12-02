/* Tests of fchown.
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

#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (fchown, int, (int, uid_t, gid_t));

#include <errno.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  /* Test behaviour for invalid file descriptors.  */
  {
    errno = 0;
    ASSERT (fchown (-1, getuid (), getgid ()) == -1);
    ASSERT (errno == EBADF);
  }
  {
    close (99);
    errno = 0;
    ASSERT (fchown (99, getuid (), getgid ()) == -1);
    ASSERT (errno == EBADF);
  }

  return 0;
}
