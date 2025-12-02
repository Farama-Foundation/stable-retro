/* Create sockets for use in tests (server side).
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

/* Creates a server that can be used to listen on incoming
   connections.  It uses the IPv4 protocol.
   If PORT is 0, a port is assigned by the kernel.
   Returns the server.  Returns the chosen port in *PPORT.  */
static int
create_server (int port, unsigned int max_backlog, int *pport)
{
  int server;

  /* Create a server socket.  */
  server = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server < 0)
    {
      fputs ("Skipping test: cannot create server socket: socket() failed\n",
             stderr);
      exit (77);
    }
  /* Bind it to a local IPv4 address.  */
  if (port != 0)
    {
      /* Set an option for the next bind() call: Avoid an EADDRINUSE error
         in case there are TIME_WAIT or CLOSE_WAIT sockets hanging around on
         the port. (Sockets in LISTEN or ESTABLISHED state on the same port
         will still yield an error.)  */
      unsigned int flag = 1;
      if (setsockopt (server, SOL_SOCKET, SO_REUSEADDR, &flag,
                      sizeof (flag))
          < 0)
        {
          fputs ("Skipping test: cannot create server socket: setsockopt() failed\n",
                 stderr);
          exit (77);
        }
    }
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

    if (bind (server, (const struct sockaddr *) &addr, sizeof (addr)) < 0)
      {
        fputs ("Skipping test: cannot create server socket: bind() failed\n",
               stderr);
        exit (77);
      }
  }
  if (port == 0)
    {
      /* Get the port that was assigned by bind().  */
      struct sockaddr_in addr;
      socklen_t addrlen = sizeof (addr);

      if (getsockname (server, (struct sockaddr *) &addr, &addrlen) < 0)
        {
          fputs ("Skipping test: cannot create server socket: getsockname() failed\n",
                 stderr);
          exit (77);
        }
      port = ntohs (addr.sin_port);
    }
  /* Start listening for a connection from the child process.  */
  if (listen (server, max_backlog) < 0)
    {
      fputs ("Skipping test: cannot create server socket: listen() failed\n",
             stderr);
      exit (77);
    }

  *pport = port;
  return server;
}

/* Creates a server socket, by accepting a connection to a server.  */
static int
create_server_socket (int server)
{
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof (addr);
  int connected_socket = accept (server, (struct sockaddr *) &addr, &addrlen);
  ASSERT (connected_socket >= 0);
  return connected_socket;
}
