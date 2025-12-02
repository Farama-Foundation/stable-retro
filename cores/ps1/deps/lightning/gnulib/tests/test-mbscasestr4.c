/* Test of case-insensitive searching in a string.
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
    const char input[] = "GOLD NEEDLE BEATS TIN NEEDLE";
    ASSERT (mbscasestr (input, "Needle") == input + 5);
  }

  /* The following tests show how mbscasestr() is different from
     strcasestr().  */

  {
    const char input[] = "s\303\266zc\303\274k"; /* sözcük */
    ASSERT (mbscasestr (input, "\303\266z") == input + 1);
    ASSERT (mbscasestr (input, "\303\266c") == NULL);
  }

  /* This test shows how a string of larger size can be found in a string of
     smaller size.  */
  {
    const char input[] = "*Tbilisi imini*";
    ASSERT (mbscasestr (input, "TB\304\260L\304\260S\304\260 \304\260m\304\260n\304\260") == input + 1); /* TBİLİSİ İmİnİ */
  }

  return 0;
}
