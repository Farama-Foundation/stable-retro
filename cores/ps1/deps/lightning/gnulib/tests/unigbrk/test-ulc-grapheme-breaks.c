/* Grapheme cluster breaks test.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Ben Pfaff <blp@cs.stanford.edu>, 2010,
   based on code by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "unigbrk.h"

#include <locale.h>
#include <stdlib.h>

#include "macros.h"

static bool
is_8859_6_break (unsigned char c)
{
  /* ISO-8859-6 has combining characters in positions 0xeb through 0xf2. */
  return !(c >= 0xeb && c <= 0xf2);
}

int
main ()
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  /* Test case n = 0.  */
  ulc_grapheme_breaks (NULL, 0, NULL);

#if HAVE_ICONV
  {
    /* This is just a random collection of bytes from ISO-8859-6.

       (We use ISO-8859-6 because it is one of very few non-UTF-8 locale
       encodings supported by glibc that have combining characters.) */
    static const char s[] = "ZYX\352\353W\360\361V\362";
    enum { LENGTH = sizeof s - 1 };
    char p[LENGTH];
    size_t i;

    ulc_grapheme_breaks (s, LENGTH, p);
    for (i = 0; i < LENGTH; i++)
      if (p[i] != is_8859_6_break (s[i]))
        {
          size_t j;

          fprintf (stderr, "wrong grapheme breaks:\n");

          fprintf (stderr, "   input:");
          for (j = 0; j < LENGTH; j++)
            fprintf (stderr, " %02x", (unsigned char) s[j]);
          putc ('\n', stderr);

          fprintf (stderr, "expected:");
          for (j = 0; j < LENGTH; j++)
            fprintf (stderr, "  %d", is_8859_6_break (s[j]));
          putc ('\n', stderr);

          fprintf (stderr, "  actual:");
          for (j = 0; j < LENGTH; j++)
            fprintf (stderr, "  %d", p[j]);
          putc ('\n', stderr);

          abort ();
        }
  }
#endif

  return 0;
}
