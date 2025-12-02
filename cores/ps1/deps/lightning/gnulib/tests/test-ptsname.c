/* Test of ptsname(3).
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

#include <stdlib.h>

#include "signature.h"
SIGNATURE_CHECK (ptsname, char *, (int));

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "same-inode.h"

#include "macros.h"

/* Compare two slave names.
   On some systems, there are hard links in the /dev/ directory.
   For example, on OSF/1 5.1,
     /dev/ttyp0 == /dev/pts/0
     /dev/ttyp9 == /dev/pts/9
     /dev/ttypa == /dev/pts/10
     /dev/ttype == /dev/pts/14
 */
static int
same_slave (const char *slave_name1, const char *slave_name2)
{
  struct stat statbuf1;
  struct stat statbuf2;

  return (strcmp (slave_name1, slave_name2) == 0
          || (stat (slave_name1, &statbuf1) >= 0
              && stat (slave_name2, &statbuf2) >= 0
              && SAME_INODE (statbuf1, statbuf2)));
}

int
main (void)
{
#if HAVE_DECL_ALARM
  /* Declare failure if test takes too long, by using default abort
     caused by SIGALRM.  */
# if defined _AIX
  int alarm_value = 20;
# else
  int alarm_value = 5;
# endif
  signal (SIGALRM, SIG_DFL);
  alarm (alarm_value);
#endif

  {
    char *result;

    errno = 0;
    result = ptsname (-1);
    ASSERT (result == NULL);
    ASSERT (errno == EBADF
            || errno == ENOTTY /* seen on glibc */
           );
  }

  {
    int fd;
    char *result;

    /* Open the controlling tty of the current process.  */
    fd = open ("/dev/tty", O_RDONLY);
    if (fd < 0)
      {
        fprintf (stderr, "Skipping test: cannot open controlling tty\n");
        return 77;
      }

    result = ptsname (fd);
    /* The result is usually NULL, because /dev/tty is a slave, not a
       master.  */
    if (result != NULL)
      {
        ASSERT (memcmp (result, "/dev/", 5) == 0);
      }

    close (fd);
  }

#if defined __sun
  /* Solaris has BSD-style /dev/pty[p-r][0-9a-f] files, but the function
     ptsname() does not work on them.  */
  {
    int fd;
    char *result;

    /* Open the controlling tty of the current process.  */
    fd = open ("/dev/ptmx", O_RDWR | O_NOCTTY);
    if (fd < 0)
      {
        fprintf (stderr, "Skipping test: cannot open pseudo-terminal\n");
        return 77;
      }

    result = ptsname (fd);
    ASSERT (result != NULL);
    ASSERT (memcmp (result, "/dev/pts/", 9) == 0);

    close (fd);
  }

#elif defined _AIX
  /* AIX has BSD-style /dev/ptyp[0-9a-f] files, and the function ptsname()
     produces the corresponding /dev/ttyp[0-9a-f] file for each.  But opening
     such a pty causes the process to hang in a state where it does not even
     react to the SIGALRM signal for N * 15 seconds, where N is the number of
     opened ptys, either in the close (fd) call or - when this close (fd) call
     is commented out - at the process exit.
     So, better don't use these BSD-style ptys.  The modern way to open a pty
     is to go through /dev/ptc.  */
  {
    int fd;
    char *result;

    /* Open a pty master.  */
    fd = open ("/dev/ptc", O_RDWR | O_NOCTTY);
    if (fd < 0)
      {
        fprintf (stderr, "Skipping test: cannot open pseudo-terminal\n");
        return 77;
      }

    result = ptsname (fd);
    ASSERT (result != NULL);
    ASSERT (memcmp (result, "/dev/pts/", 9) == 0);

    /* This close (fd) call takes 15 seconds.  It would be interruptible by the
       SIGALRM timer, but then this test would report failure.  */
    close (fd);
  }

#else

  /* Try various master names of Mac OS X: /dev/pty[p-w][0-9a-f]  */
  {
    int char1;
    int char2;

    for (char1 = 'p'; char1 <= 'w'; char1++)
      for (char2 = '0'; char2 <= 'f'; (char2 == '9' ? char2 = 'a' : char2++))
        {
          char master_name[32];
          int fd;

          sprintf (master_name, "/dev/pty%c%c", char1, char2);
          fd = open (master_name, O_RDONLY);
          if (fd >= 0)
            {
              char *result;
              char slave_name[32];

              result = ptsname (fd);
              ASSERT (result != NULL);
              sprintf (slave_name, "/dev/tty%c%c", char1, char2);
              ASSERT (same_slave (result, slave_name));

              close (fd);
            }
        }
  }

  /* Try various master names of *BSD: /dev/pty[p-sP-S][0-9a-v]  */
  {
    int upper;
    int char1;
    int char2;

    for (upper = 0; upper <= 1; upper++)
      for (char1 = (upper ? 'P' : 'p'); char1 <= (upper ? 'S' : 's'); char1++)
        for (char2 = '0'; char2 <= 'v'; (char2 == '9' ? char2 = 'a' : char2++))
          {
            char master_name[32];
            int fd;

            sprintf (master_name, "/dev/pty%c%c", char1, char2);
            fd = open (master_name, O_RDONLY);
            if (fd >= 0)
              {
                char *result;
                char slave_name[32];

                result = ptsname (fd);
                ASSERT (result != NULL);
                sprintf (slave_name, "/dev/tty%c%c", char1, char2);
                ASSERT (same_slave (result, slave_name));

                close (fd);
              }
          }
  }

#endif

  return 0;
}
