/* Test of u8_vasprintf() function.
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

#include "test-u8-printf1.h"

static uint8_t *
my_xasprintf (const char *format, ...)
{
  va_list args;
  uint8_t *result;
  int retval;

  va_start (args, format);
  retval = u8_vasprintf (&result, format, args);
  va_end (args);
  if (retval < 0)
    return NULL;
  ASSERT (result != NULL);
  return result;
}

static void
test_vasprintf ()
{
  test_xfunction (my_xasprintf);
}

int
main (int argc, char *argv[])
{
  test_vasprintf ();
  return 0;
}
