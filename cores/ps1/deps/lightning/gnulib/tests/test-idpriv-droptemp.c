/* Test of dropping uid/gid privileges of the current process temporarily.
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

#include "idpriv.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

static void
show_uids ()
{
#if HAVE_GETRESUID /* glibc, FreeBSD, OpenBSD, HP-UX */
  uid_t real;
  uid_t effective;
  uid_t saved;
  ASSERT (getresuid (&real, &effective, &saved) >= 0);
  printf ("uids: real=%d effective=%d saved=%d",
          (int) real, (int) effective, (int) saved);
#elif HAVE_GETEUID
  printf ("uids: real=%d effective=%d",
          (int) getuid (), (int) geteuid ());
#elif HAVE_GETUID
  printf ("uids: real=%d",
          (int) getuid ());
#endif
}

static void
show_gids ()
{
#if HAVE_GETRESGID /* glibc, FreeBSD, OpenBSD, HP-UX */
  gid_t real;
  gid_t effective;
  gid_t saved;
  ASSERT (getresgid (&real, &effective, &saved) >= 0);
  printf ("gids: real=%d effective=%d saved=%d",
          (int) real, (int) effective, (int) saved);
#elif HAVE_GETEGID
  printf ("gids: real=%d effective=%d",
          (int) getgid (), (int) getegid ());
#elif HAVE_GETGID
  printf ("gids: real=%d",
          (int) getgid ());
#endif
}

static void
show (const char *prefix)
{
  printf ("%s  ", prefix);
  show_uids ();
  printf ("  ");
  show_gids ();
  printf ("\n");
}

int
main (int argc, char *argv[])
{
  bool verbose = false;
  int i;

#if HAVE_GETUID
  int uid = getuid ();
#endif
#if HAVE_GETEUID
  int privileged_uid = geteuid ();
#endif
#if HAVE_GETGID
  int gid = getgid ();
#endif
#if HAVE_GETEGID
  int privileged_gid = getegid ();
#endif

  /* Parse arguments.
     -v  enables verbose output.
   */
  for (i = 1; i < argc; i++)
    {
      const char *arg = argv[i];
      if (strcmp (arg, "-v") == 0)
        verbose = true;
    }

  for (i = 0; i < 3; i++)
    {
      if (verbose)
        show ("before droptemp:");

      ASSERT (idpriv_temp_drop () == 0);

      if (verbose)
        show ("privileged:     ");

      /* Verify that the privileges have really been dropped.  */
#if HAVE_GETEUID
      if (geteuid () != uid)
        abort ();
#endif
#if HAVE_GETUID
      if (getuid () != uid)
        abort ();
#endif
#if HAVE_GETEGID
      if (getegid () != gid)
        abort ();
#endif
#if HAVE_GETGID
      if (getgid () != gid)
        abort ();
#endif

      ASSERT (idpriv_temp_restore () == 0);

      if (verbose)
        show ("unprivileged:   ");

      /* Verify that the privileges have really been acquired again.  */
#if HAVE_GETEUID
      if (geteuid () != privileged_uid)
        abort ();
#endif
#if HAVE_GETUID
      if (getuid () != uid)
        abort ();
#endif
#if HAVE_GETEGID
      if (getegid () != privileged_gid)
        abort ();
#endif
#if HAVE_GETGID
      if (getgid () != gid)
        abort ();
#endif
    }


  return 0;
}
