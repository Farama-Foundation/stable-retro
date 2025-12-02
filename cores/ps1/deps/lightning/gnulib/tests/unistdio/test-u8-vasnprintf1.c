/* Test of u8_vasnprintf() function.
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

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "unistr.h"
#include "macros.h"

#include "test-u8-asnprintf1.h"
#include "test-u8-printf1.h"

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

static uint8_t *
my_xasprintf (const char *format, ...)
{
  va_list args;
  size_t length;
  uint8_t *ret;

  va_start (args, format);
  ret = u8_vasnprintf (NULL, &length, format, args);
  va_end (args);
  if (ret != NULL)
    ASSERT (length == u8_strlen (ret));
  return ret;
}

static void
test_vasnprintf ()
{
  test_function (my_asnprintf);
  test_xfunction (my_xasprintf);
}

int
main (int argc, char *argv[])
{
  test_vasnprintf ();
  return 0;
}
