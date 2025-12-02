/* Test of opening a subcommand stream.
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

/* Written by Eric Blake <ebb9@byu.net>, 2009.  */

#include <config.h>

/* Specification.  */
#include "stdio--.h"

/* Helpers.  */
#include <sys/wait.h>
#include <unistd.h>

/* This test intentionally closes stderr.  So, we arrange to have fd 10
   (outside the range of interesting fd's during the test) set up to
   duplicate the original stderr.  */

#define BACKUP_STDERR_FILENO 10
#define ASSERT_STREAM myerr
#include "macros.h"

static FILE *myerr;

int
main (int argc, char **argv)
{
  FILE *fp;
  int status;

  /* We close fd 2 later, so save it in fd 10.  */
  if (dup2 (STDERR_FILENO, BACKUP_STDERR_FILENO) != BACKUP_STDERR_FILENO
      || (myerr = fdopen (BACKUP_STDERR_FILENO, "w")) == NULL)
    return 2;

  ASSERT (fp = popen ("exit 0", "r"));
  ASSERT (STDERR_FILENO < fileno (fp));
  status = pclose (fp);
  ASSERT (WIFEXITED (status));
  ASSERT (!WEXITSTATUS (status));

  ASSERT (fclose (stdin) == 0);
  ASSERT (fp = popen ("exit 0", "r"));
  ASSERT (STDERR_FILENO < fileno (fp));
  status = pclose (fp);
  ASSERT (WIFEXITED (status));
  ASSERT (!WEXITSTATUS (status));

  ASSERT (fclose (stdout) == 0);
  ASSERT (fp = popen ("exit 0", "r"));
  ASSERT (STDERR_FILENO < fileno (fp));
  status = pclose (fp);
  ASSERT (WIFEXITED (status));
  ASSERT (!WEXITSTATUS (status));

  ASSERT (fclose (stderr) == 0);
  ASSERT (fp = popen ("exit 0", "r"));
  ASSERT (STDERR_FILENO < fileno (fp));
  status = pclose (fp);
  ASSERT (WIFEXITED (status));
  ASSERT (!WEXITSTATUS (status));
  return 0;
}
