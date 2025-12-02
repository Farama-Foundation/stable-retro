/* Test of constructing a regular expression from a literal string.
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

#include "regex-quote.h"

#include <string.h>

#include "regex.h"
#include "xalloc.h"
#include "macros.h"

static void
check (const char *literal, int cflags, const char *expected)
{
  struct regex_quote_spec spec;
  char *result;
  size_t length;

  spec = regex_quote_spec_posix (cflags, false);
  result = regex_quote (literal, &spec);
  ASSERT (strcmp (result, expected) == 0);
  length = regex_quote_length (literal, &spec);
  ASSERT (length == strlen (result));
  free (result);

  result = (char *) xmalloc (1 + length + 1 + 1);
  result[0] = '^';
  strcpy (regex_quote_copy (result + 1, literal, &spec), "$");
  {
    regex_t regex;
    regmatch_t match[1];

    ASSERT (regcomp (&regex, result, cflags) == 0);

    ASSERT (regexec (&regex, literal, 1, match, 0) == 0);
    ASSERT (match[0].rm_so == 0);
    ASSERT (match[0].rm_eo == strlen (literal));
    regfree (&regex);
  }
  free (result);

  spec = regex_quote_spec_posix (cflags, true);
  result = regex_quote (literal, &spec);
  length = regex_quote_length (literal, &spec);
  ASSERT (length == strlen (result));
  {
    regex_t regex;
    regmatch_t match[1];

    ASSERT (regcomp (&regex, result, cflags) == 0);

    ASSERT (regexec (&regex, literal, 1, match, 0) == 0);
    ASSERT (match[0].rm_so == 0);
    ASSERT (match[0].rm_eo == strlen (literal));
    regfree (&regex);
  }
  free (result);
}

static void
test_bre (void)
{
  check ("aBc", 0, "aBc");
  check ("(foo[$HOME])", 0, "(foo\\[\\$HOME\\])");
}

static void
test_ere (void)
{
  check ("aBc", REG_EXTENDED, "aBc");
  check ("(foo[$HOME])", REG_EXTENDED, "\\(foo\\[\\$HOME\\]\\)");
}

int
main ()
{
  test_bre ();
  test_ere ();
  return 0;
}
