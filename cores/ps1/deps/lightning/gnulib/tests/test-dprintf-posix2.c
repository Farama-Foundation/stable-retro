/* Test of POSIX compatible dprintf() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if HAVE_GETRLIMIT && HAVE_SETRLIMIT
# include <sys/types.h>
# include <sys/time.h>
# include <sys/resource.h>
#endif

#include "resource-ext.h"

/* Test against a memory leak in the fprintf replacement.  */

/* Number of iterations across the loop.  */
#define NUM_ROUNDS 1000

/* Number of bytes that are allowed to escape per round.  */
#define MAX_ALLOC_ROUND 10000

/* Number of bytes that are allowed to escape in total.
   This should be at least 10 MB, since it includes the normal memory
   or address space of the test program.  */
#define MAX_ALLOC_TOTAL (NUM_ROUNDS * MAX_ALLOC_ROUND)

int
main (int argc, char *argv[])
{
  uintptr_t initial_rusage_as;
  int arg;
  int result;

  /* Limit the amount of malloc()ed memory to MAX_ALLOC_TOTAL or less.  */

  /* On AIX systems, malloc() is limited by RLIMIT_DATA.  */
#if HAVE_GETRLIMIT && HAVE_SETRLIMIT && defined RLIMIT_DATA
  {
    struct rlimit limit;

    if (getrlimit (RLIMIT_DATA, &limit) >= 0)
      {
        if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > MAX_ALLOC_TOTAL)
          limit.rlim_max = MAX_ALLOC_TOTAL;
        limit.rlim_cur = limit.rlim_max;
        (void) setrlimit (RLIMIT_DATA, &limit);
      }
  }
#endif
  /* On all systems except AIX and OpenBSD, malloc() is limited by RLIMIT_AS.
     On some systems, setrlimit of RLIMIT_AS doesn't work but get_rusage_as ()
     does.  Allow the address space size to grow by at most MAX_ALLOC_TOTAL.  */
  initial_rusage_as = get_rusage_as ();
#if !defined _AIX
  if (initial_rusage_as == 0)
    return 77;
#endif

  arg = atoi (argv[1]);
  if (arg == 0)
    {
      void *memory = malloc (MAX_ALLOC_TOTAL);
      if (memory == NULL)
        return 1;
      memset (memory, 17, MAX_ALLOC_TOTAL);
      result = 78;
    }
  else
    {
      /* Perform the test and test whether it triggers a permanent memory
         allocation of more than MAX_ALLOC_TOTAL bytes.  */
      int repeat;

      for (repeat = 0; repeat < NUM_ROUNDS; repeat++)
        {
          /* This may produce a temporary memory allocation of 11000 bytes.
             but should not result in a permanent memory allocation.  */
          if (dprintf (STDOUT_FILENO, "%011000d\n", 17) == -1
              && errno == ENOMEM)
            return 1;
        }

      result = 0;
    }

  if (get_rusage_as () > initial_rusage_as + MAX_ALLOC_TOTAL)
    return 1;

  return result;
}
