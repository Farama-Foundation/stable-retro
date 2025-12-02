/* Child program invoked by test-nonblocking-socket-main.

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

#include "nonblocking.h"

#include "macros.h"
#include "socket-client.h"
#include "test-nonblocking-socket.h"
#define PROG_ROLE "child"
#include "test-nonblocking-reader.h"

int
main (int argc, char *argv[])
{
  int test = atoi (argv[1]);
  int port = atoi (argv[2]);
  int client_socket;

  /* Create a client socket.  */
  client_socket = create_client_socket (port);

  /* Prepare the file descriptor.  */
  if (test & 2)
    ASSERT (set_nonblocking_flag (client_socket, true) >= 0);

  main_reader_loop (test, SOCKET_DATA_BLOCK_SIZE, client_socket);

  return 0;
}
