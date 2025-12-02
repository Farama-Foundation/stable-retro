/*
 * Copyright (C) 2011-2021 Free Software Foundation, Inc.
 * Written by Ben Walton.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <unistd.h>

/* for HOST_NAME_MAX */
#include <limits.h>
/* for strlen */
#include <string.h>

#include <errno.h>
#include <stdio.h>

#include "root-uid.h"

#include "macros.h"

#define TESTHOSTNAME "gnulib-hostname"

/* mingw and MSVC 9 lack geteuid, so setup a dummy value.
   On Cygwin, geteuid() may return non-zero even for user accounts with
   administrator privileges, so use a dummy value as well.  */
#if !HAVE_GETEUID || defined __CYGWIN__
# define geteuid() ROOT_UID
#endif

int
main (int argc, char *argv[] _GL_UNUSED)
{
  char origname[HOST_NAME_MAX];
  char newname[HOST_NAME_MAX];
  char longname[HOST_NAME_MAX + 2];
  int rcs, i;

  /* skip the tests if we don't have root privilege.  this does not
     consider things like CAP_SYS_ADMIN (linux) or PRIV_SYS_ADMIN
     (solaris), etc.  systems without a working geteuid (mingw, MSVC
     9) will always skip this test. */
  if (geteuid () != ROOT_UID)
    {
      fprintf (stderr, "Skipping test: insufficient permissions.\n");
      return 77;
    }

  /* we want to ensure we can do a get/set/get check to ensure the
     change is accepted. record the current name so it can be restored
     later */
  ASSERT (gethostname (origname, sizeof (origname)) == 0);

  /* try setting a valid hostname.  if it fails -1/ENOSYS, we will
     skip the test for long names as this is an indication we're using
     the stub function that doesn't do anything on this platform. */
  rcs = sethostname (TESTHOSTNAME, strlen (TESTHOSTNAME));

  if (rcs != 0)
    {
      if (rcs == -1 && errno == ENOSYS)
        {
          fprintf (stderr,
                   "Skipping test: sethostname is not really implemented.\n");
          return 77;
        }
      else if (rcs == -1
               && (errno == EPERM
                   || errno == EACCES)) /* Cygwin */
        {
          fprintf (stderr, "Skipping test: insufficient permissions.\n");
          return 77;
        }
      else
        {
          fprintf (stderr, "error setting valid hostname.\n");
          return 1;
        }
    }
  else
    {
      ASSERT (gethostname (newname, sizeof (newname)) == 0);

      /* On Windows, a hostname change becomes effective only after
         a reboot.  */
#if !(defined _WIN32 || defined __CYGWIN__)

      /* if we don't get back what we put in, there is no need to
         restore the original name as we will assume it was not
         properly changed. */
      if (strcmp (newname, TESTHOSTNAME) != 0)
        {
          fprintf (stderr, "set/get comparison failed.\n");
          return 1;
        }
#endif
    }

  /* glibc does allow setting a zero length name, so the lower bound
     needs no test. validate that we are constrained by
     HOST_NAME_MAX */
  for (i = 0; i < (HOST_NAME_MAX + 1); i++)
    longname[i] = 'a';

  longname[i] = '\0';

  rcs = sethostname (longname, (HOST_NAME_MAX + 1));

  if (rcs != -1)
    {
      /* attempt to restore the original name. */
      ASSERT (sethostname (origname, strlen (origname)) == 0);
      fprintf (stderr, "setting a too long hostname succeeded.\n");
      return 1;
    }

  /* restore the original name. */
  ASSERT (sethostname (origname, strlen (origname)) == 0);

  return 0;
}
