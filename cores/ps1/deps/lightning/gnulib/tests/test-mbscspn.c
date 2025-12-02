/* Test of searching a string for a character among a given set of characters.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

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

#include <string.h>

#include <locale.h>

#include "macros.h"

int
main ()
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  ASSERT (mbscspn ("Some text", "") == 9);

  {
    const char input[] = "A long sentence";
    ASSERT (mbscspn (input, "aeiou") == 3);
    ASSERT (mbscspn (input, "iI") == 15);
  }

  /* The following tests shows how mbscspn() is different from strcspn().  */

  {
    const char input[] = "B\303\266se B\303\274bchen"; /* "Böse Bübchen" */
    ASSERT (mbscspn (input, "\303\244\303\274") == 7); /* "äü" */
  }
#if 0 /* This would be for a GB18030 locale.  */
  {
    const char input[] = "B\201\060\213\062se B\250\271bchen"; /* "Böse Bübchen" */
    ASSERT (mbscspn (input, "\201\060\212\061\250\271") == 9); /* "äü" */
  }
#endif

  {
    const char input[] = "B\303\266se B\303\274bchen"; /* "Böse Bübchen" */
    ASSERT (mbscspn (input, "\303") == 14); /* invalid multibyte sequence */
  }

  return 0;
}
