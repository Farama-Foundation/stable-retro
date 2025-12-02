/* Recode strings between character sets, using iconv.
   Copyright (C) 2004-2005, 2009-2021 Free Software Foundation, Inc.
   Written by Simon Josefsson.

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
#include <stdio.h>

#include "iconvme.h"

int main (int ac, char *av[])
{
  char *in = NULL, *out = NULL;
  char *to = NULL, *from = NULL;

  if (ac > 1)
    from = av[1];

  if (ac > 2)
    to = av[2];

  if (ac > 3)
    in = av[3];

  if (!in)
    {
      size_t len = 0;
      printf ("Enter string to convert:\n\t> ");
      if (getline (&in, &len, stdin) < 0)
        perror ("getline");
      if (in[strlen (in) - 1] == '\n')
        in[strlen (in) - 1] = '\0';
    }

  if (!to)
    {
      size_t len = 0;
      printf ("Enter destination code set:\n\t> ");
      if (getline (&to, &len, stdin) < 0)
        perror ("getline");
      if (to[strlen (to) - 1] == '\n')
        to[strlen (to) - 1] = '\0';
    }

  if (!from)
    {
      size_t len = 0;
      printf ("Enter source code set:\n\t> ");
      if (getline (&from, &len, stdin) < 0)
        perror ("getline");
      if (from[strlen (from) - 1] == '\n')
        from[strlen (from) - 1] = '\0';
    }

  printf (" Input string: '%s'\n"
          "From code set: '%s'\n"
          "  To code set: '%s'\n",
          in, from, to);

  out = iconv_string (in, from, to);

  if (out == NULL)
    perror ("iconv");
  else
    {
      printf ("\nOutput: '%s'\n", out);
      free (out);
    }

  return EXIT_SUCCESS;
}
