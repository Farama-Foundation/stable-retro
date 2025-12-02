/* Create sockets for use in tests (client side).
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

/* Written by Bruno Haible <bruno@clisp.org>, 2011.  */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Creates a client socket, by connecting to a server on the given port.  */
static int
create_client_socket (int port)
{
  int client_socket;

  /* Create a client socket.  */
  client_socket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  ASSERT (client_socket >= 0);
  /* Connect to the server process at the specified port.  */
  {
    struct sockaddr_in addr;

    memset (&addr, 0, sizeof (addr)); /* needed on AIX and OSF/1 */
    addr.sin_family = AF_INET;
    #if 0 /* Unoptimized */
    inet_pton (AF_INET, "127.0.0.1", &addr.sin_addr);
    #elif 0 /* Nearly optimized */
    addr.sin_addr.s_addr = htonl (0x7F000001); /* 127.0.0.1 */
    #else /* Fully optimized */
    {
      unsigned char dotted[4] = { 127, 0, 0, 1 }; /* 127.0.0.1 */
      memcpy (&addr.sin_addr.s_addr, dotted, 4);
    }
    #endif
    addr.sin_port = htons (port);

    ASSERT (connect (client_socket,
                     (const struct sockaddr *) &addr, sizeof (addr))
            == 0);
  }

  return client_socket;
}
