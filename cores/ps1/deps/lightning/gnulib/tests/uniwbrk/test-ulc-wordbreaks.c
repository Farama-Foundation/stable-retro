/* Test of word breaks in strings.
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

#include "uniwbrk.h"

#include <locale.h>
#include <stdlib.h>

#include "macros.h"

int
main ()
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  /* Test case n = 0.  */
  ulc_wordbreaks (NULL, 0, NULL);

#if HAVE_ICONV
  {
    static const char input[36] =
      /* "Grüß Gott. x=(-b±sqrt(b²-4ac))/(2a)" */
      "Gr\374\337 Gott. x=(-b\261sqrt(b\262-4ac))/(2a)\n";
    char *p = (char *) malloc (SIZEOF (input));
    size_t i;

    ulc_wordbreaks (input, SIZEOF (input), p);

    for (i = 0; i < 36; i++)
      {
        ASSERT (p[i] == ((i >= 4 && i <= 5)
                         || (i >= 9 && i <= 17)
                         || (i >= 21 && i <= 25)
                         || (i >= 28 && i <= 32)
                         || (i >= 34 && i <= 35)
                         ? 1 : 0));
      }
    free (p);
  }
#endif

  return 0;
}
