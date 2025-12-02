/* Test of POSIX compatible fprintf() function.
   Copyright (C) 2007, 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <stdio.h>

/* This test assumes getrlimit() and setrlimit().
   With "gcc -fcheck-pointer-bounds -mmpx -static", it produces an
   endless loop of "Saw a #BR!" messages.  */
#if HAVE_GETRLIMIT && HAVE_SETRLIMIT && !defined __CHKP__

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

int
main (int argc, char *argv[])
{
  struct rlimit limit;
  int arg;
  int ret;

  /* Some printf implementations allocate temporary space with malloc.  */
  /* On BSD systems, malloc() is limited by RLIMIT_DATA.  */
#ifdef RLIMIT_DATA
  if (getrlimit (RLIMIT_DATA, &limit) < 0)
    return 77;
  if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > 10000000)
    limit.rlim_max = 10000000;
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit (RLIMIT_DATA, &limit) < 0)
    return 77;
#endif
  /* On Linux systems, malloc() is limited by RLIMIT_AS.  */
#ifdef RLIMIT_AS
  if (getrlimit (RLIMIT_AS, &limit) < 0)
    return 77;
  if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > 10000000)
    limit.rlim_max = 10000000;
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit (RLIMIT_AS, &limit) < 0)
    return 77;
#endif
  /* Some printf implementations allocate temporary space on the stack.  */
#ifdef RLIMIT_STACK
  if (getrlimit (RLIMIT_STACK, &limit) < 0)
    return 77;
  if (limit.rlim_max == RLIM_INFINITY || limit.rlim_max > 10000000)
    limit.rlim_max = 10000000;
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit (RLIMIT_STACK, &limit) < 0)
    return 77;
#endif

  arg = atoi (argv[1]);
  switch (arg)
    {
    case 0:
      {
        void *memory = malloc (10000000);
        if (memory == NULL)
          return 1;
        memset (memory, 17, 10000000);
        return 78;
      }
    case 1:
      ret = fprintf (stdout, "%.10000000f", 1.0);
      return !(ret == 10000002 || (ret < 0 && errno == ENOMEM));
    case 2:
      ret = fprintf (stdout, "%.10000000f", -1.0);
      return !(ret == 10000003 || (ret < 0 && errno == ENOMEM));
    case 3:
      ret = fprintf (stdout, "%.10000000e", 1.0);
      return !(ret >= 10000006 || (ret < 0 && errno == ENOMEM));
    case 4:
      ret = fprintf (stdout, "%.10000000d", 1);
      return !(ret == 10000000 || (ret < 0 && errno == ENOMEM));
    case 5:
      ret = fprintf (stdout, "%.10000000d", -1);
      return !(ret == 10000001 || (ret < 0 && errno == ENOMEM));
    case 6:
      ret = fprintf (stdout, "%.10000000u", 1);
      return !(ret == 10000000 || (ret < 0 && errno == ENOMEM));
    }
  return 0;
}

#else

int
main (int argc, char *argv[])
{
  return 77;
}

#endif
