/* Test for nonblocking read and write on sockets.

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
#include <sys/socket.h>

#if defined _WIN32 && ! defined __CYGWIN__
# include <process.h>
#else
# include <spawn.h>
#endif

#include "nonblocking.h"
#include "wait-process.h"

#include "macros.h"
#include "socket-server.h"
#include "test-nonblocking-socket.h"
#define PROG_ROLE "main"
#include "test-nonblocking-writer.h"

int
main (int argc, char *argv[])
{
  const char *child_path;
  int test;
  int server;
  int port;
  pid_t child;
  int server_socket;
  int exitcode;

  child_path = argv[1];
  test = atoi (argv[2]);

  /* Create a server socket.  */
  server = create_server (0, 1, &port);

  /* Spawn the child process.  */
  {
    char port_arg[10+1];
    const char *child_argv[4];

    sprintf (port_arg, "%u", port);
    child_argv[0] = child_path;
    child_argv[1] = argv[2];
    child_argv[2] = port_arg;
    child_argv[3] = NULL;

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

  /* Accept a connection from the child process.  */
  server_socket = create_server_socket (server);

  /* Prepare the file descriptor.  */
  if (test & 1)
    ASSERT (set_nonblocking_flag (server_socket, true) >= 0);

#if ENABLE_DEBUGGING
# ifdef SO_SNDBUF
  {
    int value;
    socklen_t value_len = sizeof (value);
    if (getsockopt (server_socket, SOL_SOCKET, SO_SNDBUF, &value, &value_len) >= 0)
      fprintf (stderr, "SO_SNDBUF = %d\n", value);
  }
# endif
# ifdef SO_RCVBUF
  {
    int value;
    socklen_t value_len = sizeof (value);
    if (getsockopt (server_socket, SOL_SOCKET, SO_RCVBUF, &value, &value_len) >= 0)
      fprintf (stderr, "SO_RCVBUF = %d\n", value);
  }
# endif
#endif

  exitcode =
    main_writer_loop (test, SOCKET_DATA_BLOCK_SIZE, server_socket,
                      SOCKET_HAS_LARGE_BUFFER);

  {
    int err =
      wait_subprocess (child, child_path, false, false, false, false, NULL);
    ASSERT (err == 0);
  }

  return exitcode;
}
