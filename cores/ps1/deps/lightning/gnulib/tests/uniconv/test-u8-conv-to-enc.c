/* Test of conversion from UTF-8 to legacy encodings.
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

#include "unistr.h"
#include "macros.h"

/* Magic number for detecting bounds violations.  */
#define MAGIC 0x1983EFF1

static size_t *
new_offsets (size_t n)
{
  size_t *offsets = (size_t *) malloc ((n + 1) * sizeof (size_t));
  offsets[n] = MAGIC;
  return offsets;
}

int
main ()
{
#if HAVE_ICONV
  static enum iconv_ilseq_handler handlers[] =
    { iconveh_error, iconveh_question_mark, iconveh_escape_sequence };
  size_t h;
  size_t o;
  size_t i;

  /* Assume that iconv() supports at least the encodings ASCII, ISO-8859-1,
     ISO-8859-2, and UTF-8.  */

  /* Test conversion from UTF-8 to ISO-8859-1 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const uint8_t input[] = "\303\204rger mit b\303\266sen B\303\274bchen ohne Augenma\303\237";
      static const char expected[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      for (o = 0; o < 2; o++)
        {
          size_t *offsets = (o ? new_offsets (u8_strlen (input)) : NULL);
          size_t length;
          char *result = u8_conv_to_encoding ("ISO-8859-1", handler,
                                              input, u8_strlen (input),
                                              offsets,
                                              NULL, &length);
          ASSERT (result != NULL);
          ASSERT (length == strlen (expected));
          ASSERT (memcmp (result, expected, length) == 0);
          if (o)
            {
              for (i = 0; i < 41; i++)
                ASSERT (offsets[i] == (i < 1 ? i :
                                       i == 1 ? (size_t)(-1) :
                                       i < 13 ? i - 1 :
                                       i == 13 ? (size_t)(-1) :
                                       i < 20 ? i - 2 :
                                       i == 20 ? (size_t)(-1) :
                                       i < 40 ? i - 3 :
                                       i == 40 ? (size_t)(-1) :
                                       i - 4));
              ASSERT (offsets[41] == MAGIC);
              free (offsets);
            }
          free (result);
        }
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EILSEQ.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const uint8_t input[] = "Rafa\305\202 Maszkowski"; /* Rafał Maszkowski */
      for (o = 0; o < 2; o++)
        {
          size_t *offsets = (o ? new_offsets (u8_strlen (input)) : NULL);
          size_t length = 0xdead;
          char *result = u8_conv_to_encoding ("ISO-8859-1", handler,
                                              input, u8_strlen (input),
                                              offsets,
                                              NULL, &length);
          switch (handler)
            {
            case iconveh_error:
              ASSERT (result == NULL);
              ASSERT (errno == EILSEQ);
              ASSERT (length == 0xdead);
              if (o)
                free (offsets);
              break;
            case iconveh_question_mark:
              {
                static const char expected[] = "Rafa? Maszkowski";
                static const char expected_translit[] = "Rafal Maszkowski";
                ASSERT (result != NULL);
                ASSERT (length == strlen (expected));
                ASSERT (memcmp (result, expected, length) == 0
                        || memcmp (result, expected_translit, length) == 0);
                if (o)
                  {
                    for (i = 0; i < 17; i++)
                      ASSERT (offsets[i] == (i < 5 ? i :
                                             i == 5 ? (size_t)(-1) :
                                             i - 1));
                    ASSERT (offsets[17] == MAGIC);
                    free (offsets);
                  }
                free (result);
              }
              break;
            case iconveh_escape_sequence:
              {
                static const char expected[] = "Rafa\\u0142 Maszkowski";
                ASSERT (result != NULL);
                ASSERT (length == strlen (expected));
                ASSERT (memcmp (result, expected, length) == 0);
                if (o)
                  {
                    for (i = 0; i < 17; i++)
                      ASSERT (offsets[i] == (i < 5 ? i :
                                             i == 5 ? (size_t)(-1) :
                                             i + 4));
                    ASSERT (offsets[17] == MAGIC);
                    free (offsets);
                  }
                free (result);
              }
              break;
            }
        }
    }

  /* Test conversion from UTF-8 to ISO-8859-1 with EINVAL.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const uint8_t input[] = "\342";
      for (o = 0; o < 2; o++)
        {
          size_t *offsets = (o ? new_offsets (u8_strlen (input)) : NULL);
          size_t length;
          char *result = u8_conv_to_encoding ("ISO-8859-1", handler,
                                              input, u8_strlen (input),
                                              offsets,
                                              NULL, &length);
          ASSERT (result != NULL);
          ASSERT (length == strlen (""));
          if (o)
            {
              ASSERT (offsets[0] == 0);
              ASSERT (offsets[1] == MAGIC);
              free (offsets);
            }
          free (result);
        }
    }

#endif

  return 0;
}
