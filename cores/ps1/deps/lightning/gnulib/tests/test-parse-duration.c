/* Test of parsing durations.
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

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "parse-duration.h"

int
main (int argc, char *argv[])
{
  if (--argc <= 0)
    {
      fprintf (stderr, "USAGE: %s <time-spec> [...]", argv[0]);
      return 1;
    }

  do
    {
      char const * arg = *++argv;
      time_t res = parse_duration (arg);
      if (errno != 0)
        {
          fprintf (stderr, "could not parse time:  %s\n\terr %d - %s\n", arg,
                   errno, strerror (errno));
          return 1;
        }
      printf ("%u\n", (unsigned int)res);
    } while (--argc > 0);

  return 0;
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "gnu"
 * indent-tabs-mode: nil
 * End:
 * end of parse-duration.c */
