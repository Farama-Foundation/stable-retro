/* Test of getting user name.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible and Paul Eggert.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if !(defined _WIN32 && !defined __CYGWIN__)
# include <pwd.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#include "macros.h"

static void
test_getlogin_result (const char *buf, int err)
{
  if (err != 0)
    {
      if (err == ENOENT)
        {
          /* This can happen on GNU/Linux.  */
          fprintf (stderr, "Skipping test: no entry in utmp file.\n");
          exit (77);
        }

      /* It fails when stdin is not connected to a tty.  */
      ASSERT (err == ENOTTY
              || err == EINVAL /* seen on Linux/SPARC */
              || err == ENXIO
             );
#if !defined __hpux /* On HP-UX 11.11 it fails anyway.  */
      ASSERT (! isatty (0));
#endif
      fprintf (stderr, "Skipping test: stdin is not a tty.\n");
      exit (77);
    }

  /* Compare against the value from the environment.  */
#if !(defined _WIN32 && !defined __CYGWIN__)
  /* Unix platform */
  {
    struct stat stat_buf;
    struct passwd *pwd;

    if (!isatty (STDIN_FILENO))
      {
         fprintf (stderr, "Skipping test: stdin is not a tty.\n");
         exit (77);
      }

    ASSERT (fstat (STDIN_FILENO, &stat_buf) == 0);

    pwd = getpwnam (buf);
    if (! pwd)
      {
        fprintf (stderr, "Skipping test: %s: no such user\n", buf);
        exit (77);
      }

    ASSERT (pwd->pw_uid == stat_buf.st_uid);
  }
#endif
#if defined _WIN32 && !defined __CYGWIN__
  /* Native Windows platform.
     Note: This test would fail on Cygwin in an ssh session, because sshd
     sets USERNAME=SYSTEM.  */
  {
    const char *name = getenv ("USERNAME");
    if (name != NULL && name[0] != '\0')
      ASSERT (strcmp (buf, name) == 0);
  }
#endif
}
