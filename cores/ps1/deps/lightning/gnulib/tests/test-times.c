/* Test of times function.
   Copyright (C) 2008-2021 Free Software Foundation, Inc.

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

/* Written by Simon Josefsson <simon@josefsson.org>, 2008.  */

#include <config.h>

#include <sys/times.h>

#include "signature.h"
SIGNATURE_CHECK (times, clock_t, (struct tms *));

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

static int
doublecmp (const void *p, const void *q)
{
  double a = *(double *) p;
  double b = *(double *) q;

  return a < b ? -1 : a > b ? 1 : 0;
}

int
main (int argc, char *argv[])
{
  struct tms tms;
  clock_t t;
#ifndef _SC_CLK_TCK
  clock_t clk_tck = CLK_TCK;
#else
  clock_t clk_tck = sysconf (_SC_CLK_TCK);
#endif

  t = times (&tms);
  if (t == (clock_t) -1)
    {
      perror ("times");
      return EXIT_FAILURE;
    }

  if (argc > 1)
    {
      printf ("clk_tck %ld\n", (long int) clk_tck);

      printf ("t %ld\n", (long int) t);
      printf ("tms.tms_utime %ldms\n", ((long int) tms.tms_utime * 1000) / (long int) clk_tck);
      printf ("tms.tms_stime %ldms\n", ((long int) tms.tms_stime * 1000) / (long int) clk_tck);
      printf ("tms.tms_cutime %ldms\n", ((long int) tms.tms_cutime * 1000) / (long int) clk_tck);
      printf ("tms.tms_cstime %ldms\n", ((long int) tms.tms_cstime * 1000) / (long int) clk_tck);
    }

  if (argc > 1)
    {
      size_t size = atoi (argv[1]);
      double *base;
      size_t i;

      base = malloc (size * sizeof (double));

      for (i = 0; i < size; i++)
        base[i] = i * i;

      qsort (base, size, sizeof (double), doublecmp);

      free (base);
    }

  t = times (&tms);
  if (t == (clock_t) -1)
    {
      perror ("times");
      return EXIT_FAILURE;
    }

  if (argc > 1)
    {
      printf ("clk_tck %ld\n", (long int) clk_tck);

      printf ("t %ld\n", (long int) t);
      printf ("tms.tms_utime %ldms\n", ((long int) tms.tms_utime * 1000) / (long int) clk_tck);
      printf ("tms.tms_stime %ldms\n", ((long int) tms.tms_stime * 1000) / (long int) clk_tck);
      printf ("tms.tms_cutime %ldms\n", ((long int) tms.tms_cutime * 1000) / (long int) clk_tck);
      printf ("tms.tms_cstime %ldms\n", ((long int) tms.tms_cstime * 1000) / (long int) clk_tck);
    }

  return 0;
}
