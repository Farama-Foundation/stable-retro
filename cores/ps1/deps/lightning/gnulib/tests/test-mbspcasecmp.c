/* Test of case-insensitive string comparison function.
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

  {
    const char string[] = "paragraph";
    ASSERT (mbspcasecmp (string, "Paragraph") == string + 9);
  }

  {
    const char string[] = "paragrapH";
    ASSERT (mbspcasecmp (string, "parAgRaph") == string + 9);
  }

  {
    const char string[] = "paragraph";
    ASSERT (mbspcasecmp (string, "paraLyzed") == NULL);
  }

  {
    const char string[] = "paraLyzed";
    ASSERT (mbspcasecmp (string, "paragraph") == NULL);
  }

  {
    const char string[] = "para";
    ASSERT (mbspcasecmp (string, "paragraph") == NULL);
  }

  {
    const char string[] = "paragraph";
    ASSERT (mbspcasecmp (string, "para") == string + 4);
  }

  /* The following tests shows how mbspcasecmp() is different from
     strncasecmp().  */

  {
    const char string[] = "\303\266zg\303\274rt\303\274k"; /* özgürtük */
    ASSERT (mbspcasecmp (string, "\303\226ZG\303\234R") == string + 7); /* özgür */
  }

  {
    const char string[] = "\303\226ZG\303\234Rt\303\274k"; /* özgürtük */
    ASSERT (mbspcasecmp (string, "\303\266zg\303\274r") == string + 7); /* özgür */
  }

  /* This test shows how strings of different size can compare equal.  */

  {
    const char string[] = "turkishtime";
    ASSERT (mbspcasecmp (string, "TURK\304\260SH") == string + 7);
  }

  {
    const char string[] = "TURK\304\260SHK\303\234LT\303\234R";
    ASSERT (mbspcasecmp (string, "turkish") == string + 8);
  }

  return 0;
}
