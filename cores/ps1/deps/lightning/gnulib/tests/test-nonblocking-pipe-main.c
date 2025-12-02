/* Test for nonblocking read and write on pipes.

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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#if defined _WIN32 && ! defined __CYGWIN__
# include <process.h>
#else
# include <spawn.h>
#endif

#include "nonblocking.h"
#include "wait-process.h"

#include "macros.h"
#include "test-nonblocking-pipe.h"
#define PROG_ROLE "main"
#include "test-nonblocking-writer.h"

int
main (int argc, char *argv[])
{
  const char *child_path;
  int test;
  int fd[2];
  pid_t child;
  int exitcode;

  child_path = argv[1];
  test = atoi (argv[2]);

  /* Create a pipe.  */
  ASSERT (pipe (fd) >= 0);

  /* Map fd[0] to STDIN_FILENO and fd[1] to STDOUT_FILENO, because on Windows,
     the only three file descriptors that are inherited by child processes are
     STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO.  */
  if (fd[0] != STDIN_FILENO)
    {
      ASSERT (dup2 (fd[0], STDIN_FILENO) >= 0);
      close (fd[0]);
    }
  if (fd[1] != STDOUT_FILENO)
    {
      ASSERT (dup2 (fd[1], STDOUT_FILENO) >= 0);
      close (fd[1]);
    }

  /* Prepare the file descriptors.  */
  if (test & 1)
    ASSERT (set_nonblocking_flag (STDOUT_FILENO, true) >= 0);
  if (test & 2)
    ASSERT (set_nonblocking_flag (STDIN_FILENO, true) >= 0);

  /* Spawn the child process.  */
  {
    const char *child_argv[3];

    child_argv[0] = child_path;
    child_argv[1] = argv[2];
    child_argv[2] = NULL;

#if defined _WIN32 && ! defined __CYGWIN__
    child = _spawnvpe (P_NOWAIT, child_path, child_argv,
                       (const char **) environ);
    ASSERT (child >= 0);
#else
    {
      pid_t child_pid;
      int err =
        posix_spawnp (&child_pid, child_path, NULL, NULL, (char **) child_argv,
                      environ);
      ASSERT (err == 0);
      child = child_pid;
    }
#endif
  }

  /* Close unused file descriptors.  */
  close (STDIN_FILENO);

  exitcode =
    main_writer_loop (test, PIPE_DATA_BLOCK_SIZE, STDOUT_FILENO, false);

  {
    int err =
      wait_subprocess (child, child_path, false, false, false, false, NULL);
    ASSERT (err == 0);
  }

  return exitcode;
}
