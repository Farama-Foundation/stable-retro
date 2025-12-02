/* Test the system defined function stpncpy().
   Copyright (C) 2003, 2008-2021 Free Software Foundation, Inc.

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

#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (stpncpy, char *, (char *, char const *, size_t));

#include <stdio.h>

int
main ()
{
  char from[10];
  char to[10];
  int i, j, k, h;
  char *ret;

  for (i = 0; i < 10; i++)
    for (j = 0; j < 10; j++)
      for (k = 0; k < 10; k++)
        {
          memset (from, 'X', sizeof from);
          memcpy (from, "SourceString", i);
          if (i < 10) from[i] = '\0';
          memset (to, 'Y', sizeof to);
          memcpy (to, "Destination", k);
          if (k < 10) to[k] = '\0';
          ret = stpncpy (to, from, j);
          printf ("i = %2d, j = %2d, k = %2d:  from = ", i, j, k);
          for (h = 0; h < 10; h++)
            if (from[h] >= 0x20 && from[h] < 0x7f)
              putchar (from[h]);
            else
              printf ("\\x%02x", from[h]);
          printf ("  to = ");
          for (h = 0; h < 10; h++)
            if (to[h] >= 0x20 && to[h] < 0x7f)
              putchar (to[h]);
            else
              printf ("\\x%02x", to[h]);
          printf ("  ret = to + %d\n", ret - to);
        }

  return 0;
}
