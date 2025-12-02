/* Test of POSIX compatible vasnprintf() and asnprintf() functions.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include "vasnprintf.h"

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

/* glibc >= 2.2 supports the 'I' flag, and in glibc >= 2.2.3 the fa_IR
   locale defines the 'outdigits' to be U+06F0..U+06F9.
   So we test for glibc >= 2.3.  */
#if (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3)) && !defined __UCLIBC__

static void
test_function (char * (*my_asnprintf) (char *, size_t *, const char *, ...))
{
  /* Test that the 'I' flag is supported.  */
  {
    size_t length;
    char *result =
      my_asnprintf (NULL, &length, "%Id %d", 1234567, 99);
    static const char expected[] = /* "۱۲۳۴۵۶۷ 99" */
      "\xDB\xB1\xDB\xB2\xDB\xB3\xDB\xB4\xDB\xB5\xDB\xB6\xDB\xB7 99";
    ASSERT (result != NULL);
    ASSERT (strcmp (result, expected) == 0);
    ASSERT (length == strlen (result));
    free (result);
  }
}

static char *
my_asnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
{
  va_list args;
  char *ret;

  va_start (args, format);
  ret = vasnprintf (resultbuf, lengthp, format, args);
  va_end (args);
  return ret;
}

static void
test_vasnprintf ()
{
  test_function (my_asnprintf);
}

static void
test_asnprintf ()
{
  test_function (asnprintf);
}

#endif

int
main (int argc, char *argv[])
{
#if (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3)) && !defined __UCLIBC__
  /* Select a locale with Arabic 'outdigits'.  */
  if (setlocale (LC_ALL, "fa_IR.UTF-8") == NULL)
    {
      fprintf (stderr, "Skipping test: no Iranian locale is installed\n");
      return 77;
    }

  test_vasnprintf ();
  test_asnprintf ();

  return 0;
#else
  fprintf (stderr, "Skipping test: not a glibc >= 2.3 system\n");
  return 77;
#endif
}
