/*
 * Copyright (C) 2017-2021 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#if HAVE_MONETARY_H
# include <monetary.h>
#endif

#include "signature.h"
#if HAVE_STRFMON_L
SIGNATURE_CHECK (strfmon_l, ssize_t, (char *s, size_t maxsize, locale_t locale,
                                      const char *format, ...));
#endif

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

int
main (void)
{
#if HAVE_STRFMON_L
  /* Simple test in the C locale.  */
  {
    char buf[80];
    locale_t loc;
    ssize_t ret;

    loc = newlocale (LC_ALL_MASK, "C", NULL);
    ASSERT (loc != NULL);
    ret = strfmon_l (buf, sizeof (buf), loc, "%^#5.0n", 123.4);
    ASSERT (   (ret == 5 && strcmp (buf,  "  123") == 0) /* AIX, Solaris */
            || (ret == 6 && strcmp (buf, "   123") == 0) /* glibc */
            || (ret == 7 && strcmp (buf, "   123 ") == 0) /* Mac OS X */
           );
  }

  /* Test whether the decimal point comes from the right locale:
     glibc bug <https://sourceware.org/bugzilla/show_bug.cgi?id=19633>.  */
  if (setlocale (LC_ALL, "en_US.UTF-8") == NULL)
    {
      fprintf (stderr, "Skipping test: English Unicode locale is not installed\n");
      return 77;
    }
  if (setlocale (LC_ALL, "de_DE.UTF-8") == NULL)
    {
      fprintf (stderr, "Skipping test: English Unicode locale is not installed\n");
      return 77;
    }
  {
    char expected_buf[80];
    locale_t loc;
    char buf[80];

    setlocale (LC_ALL, "en_US.UTF-8");
    ASSERT (strfmon (expected_buf, sizeof (expected_buf), "%.2n", 123.5) >= 0);
    setlocale (LC_ALL, "de_DE.UTF-8");
    loc = newlocale (LC_ALL_MASK, "en_US.UTF-8", NULL);
    ASSERT (strfmon_l (buf, sizeof (buf), loc, "%.2n", 123.5) >= 0);
    ASSERT (strcmp (buf, expected_buf) == 0);
    freelocale (loc);
  }
  {
    char expected_buf[80];
    locale_t loc;
    char buf[80];

    setlocale (LC_ALL, "de_DE.UTF-8");
    ASSERT (strfmon (expected_buf, sizeof (expected_buf), "%.2n", 123.5) >= 0);
    setlocale (LC_ALL, "en_US.UTF-8");
    loc = newlocale (LC_ALL_MASK, "de_DE.UTF-8", NULL);
    ASSERT (strfmon_l (buf, sizeof (buf), loc, "%.2n", 123.5) >= 0);
    ASSERT (strcmp (buf, expected_buf) == 0);
    freelocale (loc);
  }
#endif

  return 0;
}
