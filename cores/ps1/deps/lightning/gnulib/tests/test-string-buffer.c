/* Test of buffer that accumulates a string by piecewise concatenation.
   Copyright (C) 2021 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2021.  */

#include <config.h>

#include "string-buffer.h"

#include <string.h>

#include "macros.h"

static int
my_appendf (struct string_buffer *buffer, const char *formatstring, ...)
{
  va_list args;

  va_start (args, formatstring);
  int ret = sb_appendvf (buffer, formatstring, args);
  va_end (args);

  return ret;
}

char invalid_format_string_1[] = "%&";
char invalid_format_string_2[] = "%^";

int
main ()
{
  /* Test simple string concatenation.  */
  {
    struct string_buffer buffer;

    sb_init (&buffer);
    char *s = sb_dupfree (&buffer);
    ASSERT (s != NULL && strcmp (s, "") == 0);
    free (s);
  }

  {
    struct string_buffer buffer;

    sb_init (&buffer);
    sb_append (&buffer, "abc");
    sb_append (&buffer, "");
    sb_append (&buffer, "defg");
    char *s = sb_dupfree (&buffer);
    ASSERT (s != NULL && strcmp (s, "abcdefg") == 0);
    free (s);
  }

  /* Test printf-like formatting.  */
  {
    struct string_buffer buffer;

    sb_init (&buffer);
    sb_append (&buffer, "<");
    sb_appendf (&buffer, "%x", 3735928559U);
    sb_append (&buffer, ">");
    char *s = sb_dupfree (&buffer);
    ASSERT (s != NULL && strcmp (s, "<deadbeef>") == 0);
    free (s);
  }

  /* Test vprintf-like formatting.  */
  {
    struct string_buffer buffer;

    sb_init (&buffer);
    sb_append (&buffer, "<");
    my_appendf (&buffer, "%x", 3735928559U);
    sb_append (&buffer, ">");
    char *s = sb_dupfree (&buffer);
    ASSERT (s != NULL && strcmp (s, "<deadbeef>") == 0);
    free (s);
  }

  /* Test printf-like formatting failure.
     On all systems except AIX, trying to convert the wide-character 0x76543210
     to a multibyte string (in the "C" locale) fails.
     On all systems where REPLACE_VSNPRINTF=1 (this includes AIX), i.e. where
     the Gnulib implementation of vsnprintf() is used), invalid format
     directives make the *printf call fail.  */
  {
    struct string_buffer buffer;

    sb_init (&buffer);
    sb_append (&buffer, "<");
    sb_appendf (&buffer, "%lc", 0x76543210);
    sb_append (&buffer, "|");
    sb_appendf (&buffer, invalid_format_string_1, 1);
    sb_append (&buffer, "|");
    sb_appendf (&buffer, invalid_format_string_2, 2);
    sb_append (&buffer, ">");
    char *s = sb_dupfree (&buffer);
    ASSERT (s == NULL);
  }

  return 0;
}
