/* Tests of access.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.

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
SIGNATURE_CHECK (access, int, (const char *, int));

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "root-uid.h"
#include "macros.h"

/* mingw and MSVC 9 lack geteuid, so setup a dummy value.  */
#if !HAVE_GETEUID
# define geteuid() ROOT_UID
#endif

#define BASE "test-access.t"

int
main ()
{
  /* Remove anything from prior partial run.  */
  unlink (BASE "f");
  unlink (BASE "f1");
  chmod (BASE "f2", 0600);
  unlink (BASE "f2");

  {
    errno = 0;
    ASSERT (access (BASE "f", R_OK) == -1);
    ASSERT (errno == ENOENT);

    errno = 0;
    ASSERT (access (BASE "f", W_OK) == -1);
    ASSERT (errno == ENOENT);

    errno = 0;
    ASSERT (access (BASE "f", X_OK) == -1);
    ASSERT (errno == ENOENT);
  }
  {
    ASSERT (close (creat (BASE "f1", 0700)) == 0);

    ASSERT (access (BASE "f1", R_OK) == 0);
    ASSERT (access (BASE "f1", W_OK) == 0);
    ASSERT (access (BASE "f1", X_OK) == 0);
  }
  {
    ASSERT (close (creat (BASE "f2", 0600)) == 0);
    ASSERT (chmod (BASE "f2", 0400) == 0);

    ASSERT (access (BASE "f2", R_OK) == 0);

    if (geteuid () != ROOT_UID)
      {
        errno = 0;
        ASSERT (access (BASE "f2", W_OK) == -1);
        ASSERT (errno == EACCES);
      }

#if defined _WIN32 && !defined __CYGWIN__
    /* X_OK works like R_OK.  */
    ASSERT (access (BASE "f2", X_OK) == 0);
#else
    errno = 0;
    ASSERT (access (BASE "f2", X_OK) == -1);
    ASSERT (errno == EACCES);
#endif
  }

  /* Cleanup.  */
  ASSERT (unlink (BASE "f1") == 0);
  ASSERT (chmod (BASE "f2", 0600) == 0);
  ASSERT (unlink (BASE "f2") == 0);

  return 0;
}
