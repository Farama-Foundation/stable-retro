/* Grapheme cluster break function test.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff <blp@cs.stanford.edu>, 2010. */

#include <config.h>

/* Specification. */
#include <unigbrk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

static const char *
graphemebreakproperty_to_string (int gbp)
{
  printf ("%d\n", gbp);
  switch (gbp)
    {
#define CASE(VALUE) case GBP_##VALUE: return #VALUE;
      CASE(OTHER)
      CASE(CR)
      CASE(LF)
      CASE(CONTROL)
      CASE(EXTEND)
      CASE(PREPEND)
      CASE(SPACINGMARK)
      CASE(L)
      CASE(V)
      CASE(T)
      CASE(LV)
      CASE(LVT)
      CASE(RI)
      CASE(ZWJ)
      CASE(EB)
      CASE(EM)
      CASE(GAZ)
      CASE(EBG)
    }
  abort ();
}

static void
test_uc_grapheme_breaks (const char *expected, ucs4_t *s, size_t n,
                          const char *filename, int lineno)
{
  char breaks[16];
  size_t i;

  ASSERT (n <= 16);

  uc_grapheme_breaks (s, n, breaks);
  for (i = 0; i < n; i++)
    if (breaks[i] != (expected[i] == '#'))
      {
        size_t j;

        fprintf (stderr, "wrong grapheme breaks:\n");

        fprintf (stderr, "   input:");
        for (j = 0; j < n; j++)
          fprintf (stderr, " %02x", s[j]);
        putc ('\n', stderr);

        fprintf (stderr, "expected:");
        for (j = 0; j < n; j++)
          fprintf (stderr, "  %d", expected[j] == '#');
        putc ('\n', stderr);

        fprintf (stderr, "  actual:");
        for (j = 0; j < n; j++)
          fprintf (stderr, "  %d", breaks[j]);
        putc ('\n', stderr);

        abort ();
      }
}

int
main (int argc, char *argv[])
{
  const char *filename;
  char line[1024];
  int exit_code;
  FILE *stream;
  int lineno;

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s FILENAME\n"
               "where FILENAME is the location of the GraphemeBreakTest.txt\n"
               "test file.\n", argv[0]);
      exit (1);
    }

  filename = argv[1];
  stream = fopen (filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", filename);
      exit (1);
    }

  exit_code = 0;
  lineno = 0;
  while (fgets (line, sizeof line, stream))
    {
      char *comment;
      const char *p;
      ucs4_t s[16];
      char breaks[16];
      size_t i = 0;

      lineno++;

      comment = strchr (line, '#');
      if (comment != NULL)
        *comment = '\0';
      if (line[strspn (line, " \t\r\n")] == '\0')
        continue;

      s[0] = 0;
      p = line;
      do
        {
          p += strspn (p, " \t\r\n");
          if (!strncmp (p, "\303\267" /* ÷ */, 2))
            {
              breaks[i] = '#';
              p += 2;
            }
          else if (!strncmp (p, "\303\227" /* × */, 2))
            {
              breaks[i] = '_';
              p += 2;
            }
          else
            {
              fprintf (stderr, "%s:%d.%d: syntax error expecting '÷' or '×'\n",
                       filename, lineno, (int) (p - line + 1));
              exit (1);
            }

          p += strspn (p, " \t\r\n");
          if (*p == '\0')
            s[i] = 0;
          else
            {
              unsigned int next_int;
              int n;

              if (sscanf (p, "%x%n", &next_int, &n) != 1)
                {
                  fprintf (stderr, "%s:%d.%d: syntax error at '%s' "
                           "expecting hexadecimal Unicode code point number\n",
                           filename, lineno, (int) (p - line + 1), p);
                  exit (1);
                }
              p += n;

              s[i] = next_int;
            }
          p += strspn (p, " \t\r\n");
          i++;
        }
      while (*p != '\0');

      if (i > 0)
        test_uc_grapheme_breaks (breaks, s, i, filename, lineno);
    }

  return exit_code;
}
