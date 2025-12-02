/* Test of u8_vasnprintf() function in an ISO-8859-1 locale.
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

#include "unistdio.h"

#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "unistr.h"
#include "macros.h"

static void
test_function (uint8_t * (*my_asnprintf) (uint8_t *, size_t *, const char *, ...))
{
  /* Test the support of the 's' conversion specifier for strings.  */

  {
    const char *locale_string = "\304rger"; /* Ärger */
    {
      size_t length;
      uint8_t *result =
        my_asnprintf (NULL, &length, "%s %d", locale_string, 33, 44, 55);
      static const uint8_t expected[] = "\303\204rger 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      ASSERT (length == u8_strlen (result));
      free (result);
    }
    { /* Width.  */
      size_t length;
      uint8_t *result =
        my_asnprintf (NULL, &length, "%10s %d", locale_string, 33, 44, 55);
      static const uint8_t expected[] = "     \303\204rger 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      ASSERT (length == u8_strlen (result));
      free (result);
    }
    { /* FLAG_LEFT.  */
      size_t length;
      uint8_t *result =
        my_asnprintf (NULL, &length, "%-10s %d", locale_string, 33, 44, 55);
      static const uint8_t expected[] = "\303\204rger      33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      ASSERT (length == u8_strlen (result));
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      size_t length;
      uint8_t *result =
        my_asnprintf (NULL, &length, "%010s %d", locale_string, 33, 44, 55);
      static const uint8_t expected[] = "     \303\204rger 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      ASSERT (length == u8_strlen (result));
      free (result);
    }
  }
}

static uint8_t *
my_asnprintf (uint8_t *resultbuf, size_t *lengthp, const char *format, ...)
{
  va_list args;
  uint8_t *ret;

  va_start (args, format);
  ret = u8_vasnprintf (resultbuf, lengthp, format, args);
  va_end (args);
  return ret;
}

static void
test_vasnprintf ()
{
  test_function (my_asnprintf);
}

int
main (int argc, char *argv[])
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  test_vasnprintf ();
  return 0;
}
