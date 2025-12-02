/* Test of case-insensitive memory area comparison function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "mbmemcasecmp.h"

#include <locale.h>
#include <stdbool.h>
#include <string.h>

#include "macros.h"

#include "test-mbmemcasecmp.h"

int
main (int argc, char *argv[])
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  test_ascii (mbmemcasecmp);

  if (argc > 1)
    switch (argv[1][0])
      {
      case '1':
        /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
        test_iso_8859_1 (mbmemcasecmp, true);
        return 0;

      case '2':
        /* Locale encoding is UTF-8, locale is not Turkish.  */
        test_utf_8 (mbmemcasecmp, false);
        return 0;

      case '3':
        /* Locale encoding is UTF-8, locale is Turkish.  */
        test_utf_8 (mbmemcasecmp, true);
        return 0;
      }

  return 1;
}
