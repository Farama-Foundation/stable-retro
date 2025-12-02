/* Test of conversion to UTF-32 from legacy encodings.
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

#include <stdlib.h>

#include "unistr.h"
#include "macros.h"
extern int iconv_supports_encoding (const char *encoding);

int
main ()
{
#if HAVE_ICONV
  static enum iconv_ilseq_handler handlers[] =
    { iconveh_error, iconveh_question_mark, iconveh_escape_sequence };
  size_t h;

  /* Assume that iconv() supports at least the encodings ASCII, ISO-8859-1,
     ISO-8859-2, and UTF-8.  */

  /* Test conversion from ISO-8859-1 to UTF-16 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "\304rger mit b\366sen B\374bchen ohne Augenma\337";
      static const uint32_t expected[] = /* Ärger mit bösen Bübchen ohne Augenmaß */
        {
          0xC4, 'r', 'g', 'e', 'r', ' ', 'm', 'i', 't', ' ', 'b', 0xF6, 's',
          'e', 'n', ' ', 'B', 0xFC, 'b', 'c', 'h', 'e', 'n', ' ', 'o', 'h',
          'n', 'e', ' ', 'A', 'u', 'g', 'e', 'n', 'm', 'a', 0xDF, 0
        };
      uint32_t *result = u32_strconv_from_encoding (input, "ISO-8859-1", handler);
      ASSERT (result != NULL);
      ASSERT (u32_strcmp (result, expected) == 0);
      free (result);
    }

  /* Test conversion from ISO-8859-2 to UTF-16 with no errors.  */
  for (h = 0; h < SIZEOF (handlers); h++)
    {
      enum iconv_ilseq_handler handler = handlers[h];
      static const char input[] = "Rafa\263 Maszkowski"; /* Rafał Maszkowski */
      static const uint32_t expected[] =
        {
          'R', 'a', 'f', 'a', 0x0142, ' ', 'M', 'a', 's', 'z', 'k', 'o', 'w',
          's', 'k', 'i', 0
        };
      uint32_t *result = u32_strconv_from_encoding (input, "ISO-8859-2", handler);
      ASSERT (result != NULL);
      ASSERT (u32_strcmp (result, expected) == 0);
      free (result);
    }

  /* autodetect_jp is only supported when iconv() support ISO-2022-JP-2.  */
# if defined _LIBICONV_VERSION || !(defined _AIX || defined __sgi || defined __hpux || defined __osf__ || defined __sun)
  if (iconv_supports_encoding ("ISO-2022-JP-2"))
    {
      /* Test conversions from autodetect_jp to UTF-16.  */
      for (h = 0; h < SIZEOF (handlers); h++)
        {
          enum iconv_ilseq_handler handler = handlers[h];
          static const char input[] = "\244\263\244\363\244\313\244\301\244\317"; /* こんにちは in EUC-JP */
          static const uint32_t expected[] = /* こんにちは */
            {
              0x3053, 0x3093, 0x306B, 0x3061, 0x306F, 0
            };
          uint32_t *result = u32_strconv_from_encoding (input, "autodetect_jp", handler);
          ASSERT (result != NULL);
          ASSERT (u32_strcmp (result, expected) == 0);
          free (result);
        }
      for (h = 0; h < SIZEOF (handlers); h++)
        {
          enum iconv_ilseq_handler handler = handlers[h];
          static const char input[] = "\202\261\202\361\202\311\202\277\202\315"; /* こんにちは in Shift_JIS */
          static const uint32_t expected[] = /* こんにちは */
            {
              0x3053, 0x3093, 0x306B, 0x3061, 0x306F, 0
            };
          uint32_t *result = u32_strconv_from_encoding (input, "autodetect_jp", handler);
          ASSERT (result != NULL);
          ASSERT (u32_strcmp (result, expected) == 0);
          free (result);
        }
      for (h = 0; h < SIZEOF (handlers); h++)
        {
          enum iconv_ilseq_handler handler = handlers[h];
          static const char input[] = "\033$B$3$s$K$A$O\033(B"; /* こんにちは in ISO-2022-JP-2 */
          static const uint32_t expected[] = /* こんにちは */
            {
              0x3053, 0x3093, 0x306B, 0x3061, 0x306F, 0
            };
          uint32_t *result = u32_strconv_from_encoding (input, "autodetect_jp", handler);
          ASSERT (result != NULL);
          ASSERT (u32_strcmp (result, expected) == 0);
          free (result);
        }
    }
# endif

#endif

  return 0;
}
