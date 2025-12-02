/* Word break function test, using test data from UCD.
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

/* Written by Daiki Ueno <ueno@gnu.org>, 2014.

   Largely based on unigbrk/test-uc-is-grapheme-break.c,
   written by Ben Pfaff <blp@cs.stanford.edu>, 2010.  */

#include <config.h>

/* Specification. */
#include <uniwbrk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *
wordbreakproperty_to_string (int wbp)
{
  switch (wbp)
    {
#define CASE(VALUE) case WBP_##VALUE: return #VALUE;
      CASE(OTHER)
      CASE(CR)
      CASE(LF)
      CASE(NEWLINE)
      CASE(EXTEND)
      CASE(FORMAT)
      CASE(KATAKANA)
      CASE(ALETTER)
      CASE(MIDNUMLET)
      CASE(MIDLETTER)
      CASE(MIDNUM)
      CASE(NUMERIC)
      CASE(EXTENDNUMLET)
      CASE(RI)
      CASE(DQ)
      CASE(SQ)
      CASE(HL)
      CASE(ZWJ)
      CASE(EB)
      CASE(EM)
      CASE(GAZ)
      CASE(EBG)
    }
  abort ();
}

int
main (int argc, char *argv[])
{
  const char *filename;
  char line[4096];
  int exit_code;
  FILE *stream;
  int lineno;

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s FILENAME\n"
               "where FILENAME is the location of the WordBreakTest.txt\n"
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
      uint32_t input[100];
      char breaks[101];
      char breaks_expected[101];
      int i;

      lineno++;

      memset (breaks, 0, sizeof (breaks));
      memset (breaks_expected, 0, sizeof (breaks_expected));

      comment = strchr (line, '#');
      if (comment != NULL)
        *comment = '\0';
      if (line[strspn (line, " \t\r\n")] == '\0')
        continue;

      i = 0;
      p = line;
      do
        {
          p += strspn (p, " \t\r\n");
          if (!strncmp (p, "\303\267" /* ÷ */, 2))
            {
              breaks_expected[i] = 1;
              p += 2;
            }
          else if (!strncmp (p, "\303\227" /* × */, 2))
            {
              breaks_expected[i] = 0;
              p += 2;
            }
          else
            {
              fprintf (stderr, "%s:%d.%d: syntax error expecting '÷' or '×'\n",
                       filename, lineno, (int) (p - line + 1));
              exit (1);
            }

          p += strspn (p, " \t\r\n");
          if (*p != '\0')
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

              input[i] = next_int;
            }

          p += strspn (p, " \t\r\n");
          i++;
        }
      while (*p != '\0');

      u32_wordbreaks (input, i - 1, breaks);

      /* u32_wordbreaks always set BREAKS[0] to 0.  */
      breaks[0] = breaks_expected[0] = 1;
      if (memcmp (breaks, breaks_expected, i - 1) != 0)
        {
          int j;

          fprintf (stderr, "%s:%d: expected: ", filename, lineno);
          for (j = 0; j < i - 1; j++)
            {
              int input_wbp = uc_wordbreak_property (input[j]);
              fprintf (stderr, "%s U+%04X (%s) ",
                       breaks_expected[j] == 1 ? "\303\267" : "\303\227",
                       input[j], wordbreakproperty_to_string (input_wbp));
            }
          fprintf (stderr, "\n");
          fprintf (stderr, "%s:%d: actual: ", filename, lineno);
          for (j = 0; j < i - 1; j++)
            {
              int input_wbp = uc_wordbreak_property (input[j]);
              fprintf (stderr, "%s U+%04X (%s) ",
                       breaks[j] == 1 ? "\303\267" : "\303\227",
                       input[j], wordbreakproperty_to_string (input_wbp));
            }
          fprintf (stderr, "\n");
          exit_code = 1;
        }
    }

  return exit_code;
}
