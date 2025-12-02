/* Test of conversion of string to 32-bit wide string.
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
SIGNATURE_CHECK (mbstoc32s, size_t, (char32_t *, const char *, size_t));

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

int
main (int argc, char *argv[])
{
  char32_t wc;
  size_t ret;

  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  /* Test NUL byte input.  */
  {
    const char *src;

    src = "";
    ret = mbstoc32s (NULL, src, 0);
    ASSERT (ret == 0);

    src = "";
    ret = mbstoc32s (NULL, src, 1);
    ASSERT (ret == 0);

    wc = (char32_t) 0xBADFACE;
    src = "";
    ret = mbstoc32s (&wc, src, 0);
    ASSERT (ret == 0);
    ASSERT (wc == (char32_t) 0xBADFACE);

    wc = (char32_t) 0xBADFACE;
    src = "";
    ret = mbstoc32s (&wc, src, 1);
    ASSERT (ret == 0);
    ASSERT (wc == 0);
  }

  if (argc > 1)
    {
      int unlimited;

      for (unlimited = 0; unlimited < 2; unlimited++)
        {
          #define BUFSIZE 10
          char32_t buf[BUFSIZE];
          const char *src;

          {
            size_t i;
            for (i = 0; i < BUFSIZE; i++)
              buf[i] = (char32_t) 0xBADFACE;
          }

          switch (argv[1][0])
            {
            case '1':
              /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
              {
                char input[] = "B\374\337er"; /* "Büßer" */

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input, 1);
                ASSERT (ret == 1);
                ASSERT (wc == 'B');
                input[0] = '\0';

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input + 1, 1);
                ASSERT (ret == 1);
                ASSERT (c32tob (wc) == (unsigned char) '\374');
                input[1] = '\0';

                src = input + 2;
                ret = mbstoc32s (NULL, src, unlimited ? BUFSIZE : 1);
                ASSERT (ret == 3);

                src = input + 2;
                ret = mbstoc32s (buf, src, unlimited ? BUFSIZE : 1);
                ASSERT (ret == (unlimited ? 3 : 1));
                ASSERT (c32tob (buf[0]) == (unsigned char) '\337');
                if (unlimited)
                  {
                    ASSERT (buf[1] == 'e');
                    ASSERT (buf[2] == 'r');
                    ASSERT (buf[3] == 0);
                    ASSERT (buf[4] == (char32_t) 0xBADFACE);
                  }
                else
                  ASSERT (buf[1] == (char32_t) 0xBADFACE);
              }
              break;

            case '2':
              /* Locale encoding is UTF-8.  */
              {
                char input[] = "s\303\274\303\237\360\237\230\213!"; /* "süß😋!" */

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input, 1);
                ASSERT (ret == 1);
                ASSERT (wc == 's');
                input[0] = '\0';

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input + 1, 1);
                ASSERT (ret == 1);
                ASSERT (wc == 0x00FC); /* expect Unicode encoding */
                input[1] = '\0';
                input[2] = '\0';

                src = input + 3;
                ret = mbstoc32s (NULL, src, unlimited ? BUFSIZE : 2);
                ASSERT (ret == 3);

                src = input + 3;
                ret = mbstoc32s (buf, src, unlimited ? BUFSIZE : 2);
                ASSERT (ret == (unlimited ? 3 : 2));
                ASSERT (buf[0] == 0x00DF); /* expect Unicode encoding */
                ASSERT (buf[1] == 0x1F60B); /* expect Unicode encoding */
                if (unlimited)
                  {
                    ASSERT (buf[2] == '!');
                    ASSERT (buf[3] == 0);
                    ASSERT (buf[4] == (char32_t) 0xBADFACE);
                  }
                else
                  ASSERT (buf[2] == (char32_t) 0xBADFACE);
              }
              break;

            case '3':
              /* Locale encoding is EUC-JP.  */
              {
                char input[] = "<\306\374\313\334\270\354>"; /* "<日本語>" */

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input, 1);
                ASSERT (ret == 1);
                ASSERT (wc == '<');
                input[0] = '\0';

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input + 1, 1);
                ASSERT (ret == 1);
                ASSERT (c32tob (wc) == EOF);
                input[1] = '\0';
                input[2] = '\0';

                src = input + 3;
                ret = mbstoc32s (NULL, src, unlimited ? BUFSIZE : 2);
                ASSERT (ret == 3);

                src = input + 3;
                ret = mbstoc32s (buf, src, unlimited ? BUFSIZE : 2);
                ASSERT (ret == (unlimited ? 3 : 2));
                ASSERT (c32tob (buf[0]) == EOF);
                ASSERT (c32tob (buf[1]) == EOF);
                if (unlimited)
                  {
                    ASSERT (buf[2] == '>');
                    ASSERT (buf[3] == 0);
                    ASSERT (buf[4] == (char32_t) 0xBADFACE);
                  }
                else
                  ASSERT (buf[2] == (char32_t) 0xBADFACE);
              }
              break;

            case '4':
              /* Locale encoding is GB18030.  */
              {
                char input[] = "s\250\271\201\060\211\070\224\071\375\067!"; /* "süß😋!" */

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input, 1);
                ASSERT (ret == 1);
                ASSERT (wc == 's');
                input[0] = '\0';

                wc = (char32_t) 0xBADFACE;
                ret = mbstoc32s (&wc, input + 1, 1);
                ASSERT (ret == 1);
                ASSERT (c32tob (wc) == EOF);
                input[1] = '\0';

                src = input + 3;
                ret = mbstoc32s (NULL, src, unlimited ? BUFSIZE : 2);
                ASSERT (ret == 3);

                src = input + 3;
                ret = mbstoc32s (buf, src, unlimited ? BUFSIZE : 2);
                ASSERT (ret == (unlimited ? 3 : 2));
                ASSERT (c32tob (buf[0]) == EOF);
                ASSERT (c32tob (buf[1]) == EOF);
                if (unlimited)
                  {
                    ASSERT (buf[2] == '!');
                    ASSERT (buf[3] == 0);
                    ASSERT (buf[4] == (char32_t) 0xBADFACE);
                  }
                else
                  ASSERT (buf[2] == (char32_t) 0xBADFACE);
              }
              break;

            default:
              return 1;
            }
        }

      return 0;
    }

  return 1;
}
