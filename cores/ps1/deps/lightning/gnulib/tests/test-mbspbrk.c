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

  ASSERT (mbspbrk ("Some text", "") == NULL);

  {
    const char input[] = "A long sentence";
    ASSERT (mbspbrk (input, "aeiou") == input + 3);
    ASSERT (mbspbrk (input, "iI") == NULL);
  }

  /* The following tests shows how mbspbrk() is different from strpbrk().  */

  {
    const char input[] = "B\303\266se B\303\274bchen"; /* "Böse Bübchen" */
    ASSERT (mbspbrk (input, "\303\244\303\274") == input + 7); /* "äü" */
  }

  {
    const char input[] = "B\303\266se B\303\274bchen"; /* "Böse Bübchen" */
    ASSERT (mbspbrk (input, "\303") == NULL); /* invalid multibyte sequence */
  }

  return 0;
}
