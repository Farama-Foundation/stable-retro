/* Test of c_xasprintf() and c_xvasprintf() functions.
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

#include <config.h>

#include "c-xvasprintf.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

static char *
my_c_xasprintf (const char *format, ...)
{
  va_list args;
  char *s;

  va_start (args, format);
  s = c_xvasprintf (format, args);
  va_end (args);

  return s;
}

int
main (int argc, char *argv[])
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  /* Test behaviour of snprintf() as a "control group".
     (We should be running in a locale where ',' is the decimal point.) */
  {
    char s[16];

    snprintf (s, sizeof s, "%#.0f", 1.0);
    if (!strcmp (s, "1."))
      {
        /* Skip the test, since we're not in a useful locale for testing. */
        return 77;
      }
    ASSERT (!strcmp (s, "1,"));
  }

  /* Test behaviour of c_xasprintf() and c_xvasprintf().
     They should always use '.' as the decimal point. */
  {
    char *s;

    s = c_xasprintf ("%#.0f", 1.0);
    ASSERT (s != NULL);
    ASSERT (!strcmp (s, "1."));
    free (s);

    s = my_c_xasprintf ("%#.0f", 1.0);
    ASSERT (s != NULL);
    ASSERT (!strcmp (s, "1."));
    free (s);
  }

  return 0;
}
