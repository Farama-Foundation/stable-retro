/* Test of conversion of 32-bit wide string to string.
   Copyright (C) 2008-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2008.  */

#include <config.h>

#include <uchar.h>

#include "signature.h"
SIGNATURE_CHECK (c32stombs, size_t,
                 (char *, const char32_t *, size_t));

#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  if (argc > 1)
    {
      char32_t input[10];
      size_t n;
      #define BUFSIZE 20
      char buf[BUFSIZE];
      size_t ret;

      {
        size_t i;
        for (i = 0; i < BUFSIZE; i++)
          buf[i] = '_';
      }

      switch (argv[1][0])
        {
        case '1':
          /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
          {
            const char original[] = "B\374\337er"; /* "Büßer" */

            ret = mbstoc32s (input, original, 10);
            ASSERT (ret == 5);

            for (n = 0; n < 10; n++)
              {
                ret = c32stombs (NULL, input, n);
                ASSERT (ret == 5);

                ret = c32stombs (buf, input, n);
                ASSERT (ret == (n <= 5 ? n : 5));
                ASSERT (memcmp (buf, original, ret) == 0);
                if (n > 5)
                  ASSERT (buf[ret] == '\0');
                ASSERT (buf[ret + (n > 5) + 0] == '_');
                ASSERT (buf[ret + (n > 5) + 1] == '_');
                ASSERT (buf[ret + (n > 5) + 2] == '_');
              }
          }
          break;

        case '2':
          /* Locale encoding is UTF-8.  */
          {
            const char original[] = "s\303\274\303\237\360\237\230\213!"; /* "süß😋!" */

            ret = mbstoc32s (input, original, 10);
            ASSERT (ret == 5);

            for (n = 0; n < 15; n++)
              {
                ret = c32stombs (NULL, input, n);
                ASSERT (ret == 10);

                ret = c32stombs (buf, input, n);
                ASSERT (ret == (n < 1 ? n :
                                n < 3 ? 1 :
                                n < 5 ? 3 :
                                n < 9 ? 5 :
                                n <= 10 ? n : 10));
                ASSERT (memcmp (buf, original, ret) == 0);
                if (n > 10)
                  ASSERT (buf[ret] == '\0');
                ASSERT (buf[ret + (n > 10) + 0] == '_');
                ASSERT (buf[ret + (n > 10) + 1] == '_');
                ASSERT (buf[ret + (n > 10) + 2] == '_');
              }
          }
          break;

        case '3':
          /* Locale encoding is EUC-JP.  */
          {
            const char original[] = "<\306\374\313\334\270\354>"; /* "<日本語>" */

            ret = mbstoc32s (input, original, 10);
            ASSERT (ret == 5);

            for (n = 0; n < 10; n++)
              {
                ret = c32stombs (NULL, input, n);
                ASSERT (ret == 8);

                ret = c32stombs (buf, input, n);
                ASSERT (ret == (n < 1 ? n :
                                n < 3 ? 1 :
                                n < 5 ? 3 :
                                n < 7 ? 5 :
                                n <= 8 ? n : 8));
                ASSERT (memcmp (buf, original, ret) == 0);
                if (n > 8)
                  ASSERT (buf[ret] == '\0');
                ASSERT (buf[ret + (n > 8) + 0] == '_');
                ASSERT (buf[ret + (n > 8) + 1] == '_');
                ASSERT (buf[ret + (n > 8) + 2] == '_');
              }
          }
          break;


        case '4':
          /* Locale encoding is GB18030.  */
          {
            const char original[] = "s\250\271\201\060\211\070\224\071\375\067!"; /* "süß😋!" */

            ret = mbstoc32s (input, original, 10);
            ASSERT (ret == 5);

            for (n = 0; n < 15; n++)
              {
                ret = c32stombs (NULL, input, n);
                ASSERT (ret == 12);

                ret = c32stombs (buf, input, n);
                ASSERT (ret == (n < 1 ? n :
                                n < 3 ? 1 :
                                n < 7 ? 3 :
                                n < 11 ? 7 :
                                n <= 12 ? n : 12));
                ASSERT (memcmp (buf, original, ret) == 0);
                if (n > 12)
                  ASSERT (buf[ret] == '\0');
                ASSERT (buf[ret + (n > 12) + 0] == '_');
                ASSERT (buf[ret + (n > 12) + 1] == '_');
                ASSERT (buf[ret + (n > 12) + 2] == '_');
              }
          }
          break;

        default:
          return 1;
        }

      return 0;
    }

  return 1;
}
