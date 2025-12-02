/* Test of login_tty() function.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Specification.  */
extern int login_tty (int);

#include <errno.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int
main ()
{
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

  /* Create a new session and make it the controlling tty of this session.  */
  {
    int res = login_tty (slave);
    if (res < 0)
      {
        fprintf (stderr, "login_tty failed\n");
        return 1;
      }
  }

  /* From here on, we cannot use stderr for error messages any more.
     If a test fails, just abort.  */

  /* Check that fd = 0, 1, 2 are now open to the controlling terminal for the
     current process and that it is a session of its own.  */
  {
    int fd;
    for (fd = 0; fd < 3; fd++)
      if (!(tcgetpgrp (fd) == getpid ()))
        abort ();
    for (fd = 0; fd < 3; fd++)
      {
        int sid = tcgetsid (fd);
        if (!(sid == -1 ? errno == ENOSYS : sid == getpid ()))
          abort ();
      }
  }

  return 0;
}
