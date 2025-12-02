/* Test getsockname() function.
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

#include <sys/socket.h>

#include "signature.h"
SIGNATURE_CHECK (getsockname, int, (int, struct sockaddr *, socklen_t *));

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "sockets.h"
#include "macros.h"

static int
open_server_socket (void)
{
  int s, x;
  struct sockaddr_in ia;

  s = socket (AF_INET, SOCK_STREAM, 0);

  x = 1;
  setsockopt (s, SOL_SOCKET, SO_REUSEPORT, &x, sizeof (x));

  memset (&ia, 0, sizeof (ia));
  ia.sin_family = AF_INET;
  inet_pton (AF_INET, "0.0.0.0", &ia.sin_addr);
  /* Port 0 means that the system should assign a port.  */
  ia.sin_port = htons (0);
  if (bind (s, (struct sockaddr *) &ia, sizeof (ia)) < 0)
    {
      perror ("bind");
      exit (77);
    }

  if (listen (s, 1) < 0)
    {
      perror ("listen");
      exit (77);
    }

  return s;
}

int
main (void)
{
  (void) gl_sockets_startup (SOCKETS_1_1);

  /* Test behaviour for invalid file descriptors.  */
  {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof (addr);

    errno = 0;
    ASSERT (getsockname (-1, (struct sockaddr *) &addr, &addrlen) == -1);
    ASSERT (errno == EBADF);
  }
  {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof (addr);

    close (99);
    errno = 0;
    ASSERT (getsockname (99, (struct sockaddr *) &addr, &addrlen) == -1);
    ASSERT (errno == EBADF);
  }

  /* Test behaviour for a server socket.  */
  {
    int s = open_server_socket ();
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof (addr);

    memset (&addr, 0, sizeof (addr));
    ASSERT (getsockname (s, (struct sockaddr *) &addr, &addrlen) == 0);
    ASSERT (addr.sin_family == AF_INET);
    ASSERT (ntohs (addr.sin_port) != 0);
  }

  return 0;
}
