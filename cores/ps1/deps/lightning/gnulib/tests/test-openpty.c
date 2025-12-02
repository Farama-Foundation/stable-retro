/* Test of pty.h and openpty function.
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

/* Written by Simon Josefsson <simon@josefsson.org>, 2009
   and Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include <pty.h>

#include "signature.h"
SIGNATURE_CHECK (openpty, int, (int *, int *, char *, struct termios const *,
                                struct winsize const *));

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int
main ()
{
  {
#ifndef _WIN32
    int master;
    int slave;

    /* Open a pseudo-terminal, as a master-slave pair.  */
    {
      int res = openpty (&master, &slave, NULL, NULL, NULL);
      if (res != 0)
        {
          fprintf (stderr, "openpty returned %d\n", res);
          return 1;
        }
    }

    /* Set the terminal characteristics.
       On Linux or Mac OS X, they can be set on either the master or the slave;
       the effect is the same.  But on Solaris, they have to be set on the
       master; tcgetattr on the slave fails.  */
    {
      int tcfd = slave; /* You can try  tcfd = master;  here.  */
      struct termios attributes;

      if (tcgetattr (tcfd, &attributes) < 0)
        {
          fprintf (stderr, "tcgetattr failed\n");
          return 1;
        }
      /* Enable canonical processing, including erase.  */
      attributes.c_lflag |= ECHO | ICANON | ECHOE;
      attributes.c_cc[VERASE] = '\177';
      if (tcsetattr (tcfd, TCSANOW, &attributes) < 0)
        {
          fprintf (stderr, "tcsetattr failed\n");
          return 1;
        }
    }

    /* Write into the master side.  */
    {
      static const char input[] = "Hello worst\177\177ld!\n";

      if (write (master, input, strlen (input)) < (int) strlen (input))
        {
          fprintf (stderr, "write failed\n");
          return 1;
        }
    }

    /* Read from the slave side.  */
    {
      char buf[100];
      int res = read (slave, buf, sizeof (buf));
      static const char expected[] = "Hello world!\n";

      if (res < 0)
        {
          fprintf (stderr, "read failed\n");
          return 1;
        }
      if (!(res == strlen (expected)
            && memcmp (buf, expected, strlen (expected)) == 0))
        {
          fprintf (stderr, "read result unexpected\n");
          return 1;
        }
    }

    /* Close the master side before the slave side gets closed.
       This is necessary on Mac OS X 10.4.11.  */
    close (master);
#endif
  }

  return 0;
}
