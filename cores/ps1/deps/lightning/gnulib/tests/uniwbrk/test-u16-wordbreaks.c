/* Test of word breaks in UTF-16 strings.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "uniwbrk.h"

#include <stdlib.h>

#include "macros.h"

int
main ()
{
  /* Test case n = 0.  */
  u16_wordbreaks (NULL, 0, NULL);

  {
    static const uint16_t input[61] =
      /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };
    char *p = (char *) malloc (SIZEOF (input));
    size_t i;

    u16_wordbreaks (input, SIZEOF (input), p);

    for (i = 0; i < 61; i++)
      {
        ASSERT (p[i] == ((i >= 4 && i <= 5)
                         || (i >= 9 && i <= 11)
                         || (i >= 23 && i <= 31)
                         || (i >= 35 && i <= 39)
                         || (i >= 42 && i <= 46)
                         || (i >= 48 && i <= 58)
                         || i == 60
                         ? 1 : 0));
      }
    free (p);
  }

  {
    /* Same input string, decomposed.  */
    static const uint16_t input[67] =
      /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
      { 'G', 'r', 0x0075, 0x0308, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0438, 0x0306, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',',
        0x1112, 0x1161, 0x11AB, 0x1100, 0x1173, 0x11AF, '\n'
      };
    char *p = (char *) malloc (SIZEOF (input));
    size_t i;

    u16_wordbreaks (input, SIZEOF (input), p);

    for (i = 0; i < 67; i++)
      {
        ASSERT (p[i] == ((i >= 5 && i <= 6)
                         || (i >= 10 && i <= 12)
                         || (i >= 25 && i <= 33)
                         || (i >= 37 && i <= 41)
                         || (i >= 44 && i <= 48)
                         || (i >= 50 && i <= 60)
                         || i == 66
                         ? 1 : 0));
      }
    free (p);
  }

  return 0;
}
