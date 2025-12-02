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

/* Include <config.h> and a form of <stdio.h> first.  */

/* Helpers.  */
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "macros.h"

int
main (int argc, char **argv)
{
  size_t len;
  char *cmd;
  int i;

  /* Children - use the pipe.  */
  if (argc > 1)
    {
      if (*argv[1] == 'r') /* Parent is reading, so we write.  */
        ASSERT (putchar ('c') == 'c');
      else /* Parent is writing, so we read.  */
        ASSERT (getchar () == 'p');
      /* Test that parent can read non-zero status.  */
      return 42;
    }

  /* Parent - create read and write child, once under normal
     circumstances and once with stdin and stdout closed.  */
  len = strlen (argv[0]);
  cmd = malloc (len + 3); /* Adding " r" and NUL.  */
  ASSERT (cmd);
  /* We count on argv[0] not containing any shell metacharacters.  */
  strcpy (cmd, argv[0]);
  cmd[len] = ' ';
  cmd[len + 2] = '\0';
  for (i = 0; i < 2; i++)
    {
      FILE *child;
      int status;

      if (i)
        {
          ASSERT (fclose (stdin) == 0);
          ASSERT (fclose (stdout) == 0);
        }

      cmd[len + 1] = 'r';
      ASSERT (child = popen (cmd, "r"));
      ASSERT (fgetc (child) == 'c');
      status = pclose (child);
      ASSERT (WIFEXITED (status));
      ASSERT (WEXITSTATUS (status) == 42);
      if (i)
        {
          ASSERT (dup2 (STDIN_FILENO, STDIN_FILENO) == -1);
          ASSERT (dup2 (STDOUT_FILENO, STDOUT_FILENO) == -1);
        }

      cmd[len + 1] = 'w';
      ASSERT (child = popen (cmd, "w"));
      ASSERT (fputc ('p', child) == 'p');
      status = pclose (child);
      ASSERT (WIFEXITED (status));
      ASSERT (WEXITSTATUS (status) == 42);
      if (i)
        {
          ASSERT (dup2 (STDIN_FILENO, STDIN_FILENO) == -1);
          ASSERT (dup2 (STDOUT_FILENO, STDOUT_FILENO) == -1);
        }
    }
  free (cmd);
  return 0;
}
