/* Child program invoked by test-system-quote-main.
   Copyright (C) 2012-2021 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <stdio.h>
#include <string.h>

/* Do not use any gnulib replacements, since this program should
   link against as few libraries as possible.  */
#undef fclose
#undef fopen
#undef fprintf
#undef fread
/* Restore the original fopen definition on AIX.  */
#if defined _AIX && defined _LARGE_FILES
# define fopen fopen64
#endif

#define EXPECTED_DATA_FILE "t-sq-data.tmp"

int
main (int argc, char *argv[])
{
  const char *arg;
  char expected_data[1000];
  size_t expected_data_len;

  if (argc < 2)
    /* Expected one data argument, received none.  */
    return 2;
  if (argc > 2)
    /* Expected one data argument, received more than one.  */
    return 3;
  arg = argv[1];

  /* Read the contents of EXPECTED_DATA_FILE.  */
  {
    FILE *fp = fopen (EXPECTED_DATA_FILE, "rb");
    if (fp == NULL)
      return 4;
    expected_data_len = fread (expected_data, 1, sizeof (expected_data), fp);
    if (fclose (fp))
      return 5;
  }

  if (!(strlen (arg) == expected_data_len
        && memcmp (arg, expected_data, expected_data_len) == 0))
    {
      /* arg is not as expected.  */
      fprintf (stderr, "expected: %.*s\nreceived: %s\n",
               (int)expected_data_len, expected_data, arg);
      return 1;
    }
  else
    return 0;
}
