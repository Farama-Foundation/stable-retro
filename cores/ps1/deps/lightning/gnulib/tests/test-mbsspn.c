/* Test of searching a string for a character outside a given set of characters.
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

  ASSERT (mbsspn ("Some text", "") == 0);

  ASSERT (mbsspn ("A long sentence", " ") == 0);
  ASSERT (mbsspn ("  xy", "aei ou") == 2);
  ASSERT (mbsspn ("eau", "aeiou") == 3);

  /* The following tests shows how mbsspn() is different from strspn().  */

  {
    const char input[] = "\303\266\303\274"; /* "öü" */
    ASSERT (mbsspn (input, "\303\266") == 2); /* "ö" */
    ASSERT (mbsspn (input, "\303\244") == 0); /* "ä" */
    ASSERT (mbsspn (input, "\303\274\303\266") == 4); /* "üö" */
    ASSERT (mbsspn (input, "\303\244\303\274") == 0); /* "äü" */
    ASSERT (mbsspn (input, "\303\244\303\266") == 2); /* "äö" */
  }

  {
    const char input[] = "\303\266\303\274"; /* "öü" */
    ASSERT (mbsspn (input, "\303") == 0); /* invalid multibyte sequence */
  }

  return 0;
}
