/* Test of conversion from UTF-32 to legacy encodings.
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

#include "uniconv.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"

int
main ()
{
#if HAVE_ICONV
  static enum iconv_ilseq_handler handlers[] =
    { iconveh_error, iconveh_question_mark, iconveh_escape_sequence };
  size_t h;

  /* Assume that iconv() supports at least the encodings ASCII, ISO-8859-1,
     ISO-8859-2, and UTF-8.  */

  /* Test conversion from UTF-32 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const uint32_t input[] = /* Ärger mit bösen Bübchen ohne Augenmaß */
        {
          0xC4, 'r', 'g', 'e', 'r', ' ', 'm', 'i', 't', ' ', 'b', 0xF6, 's',
          'e', 'n', ' ', 'B', 0xFC, 'b', 'c', 'h', 'e', 'n', ' ', 'o', 'h',
          'n', 'e', ' ', 'A', 'u', 'g', 'e', 'n', 'm', 'a', 0xDF, 0
        };
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      char *result = u32_strconv_to_encoding (input, "ISO-8859-1", handler);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from UTF-32 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const uint32_t input[] = /* Rafał Maszkowski */
        {
          'R', 'a', 'f', 'a', 0x0142, ' ', 'M', 'a', 's', 'z', 'k', 'o', 'w',
          's', 'k', 'i', 0
        };
      char *result = u32_strconv_to_encoding (input, "ISO-8859-1", handler);
      switch (handler)
        {
        case iconveh_error:
          ASSERT (result == NULL && errno == EILSEQ);
          break;
        case iconveh_question_mark:
          {
            static const char expected[] = "Rafa? Maszkowski";
            static const char expected_translit[] = "Rafal Maszkowski";
            ASSERT (result != NULL);
            ASSERT (strcmp (result, expected) == 0
                    || strcmp (result, expected_translit) == 0);
            free (result);
          }
          break;
        case iconveh_escape_sequence:
          {
            static const char expected[] = "Rafa\\u0142 Maszkowski";
            ASSERT (result != NULL);
            ASSERT (strcmp (result, expected) == 0);
            free (result);
          }
          break;
        }
    }

#endif

  return 0;
}
