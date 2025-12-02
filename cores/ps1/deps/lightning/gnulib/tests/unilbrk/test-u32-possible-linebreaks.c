/* Test of line breaking of UTF-32 strings.
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

#include "unilbrk.h"

#include <stdlib.h>

#include "macros.h"

int
main ()
{
  /* Test case n = 0.  */
  u32_possible_linebreaks (NULL, 0, "GB18030", NULL);

  {
    static const uint32_t input[61] =
      /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
      { 'G', 'r', 0x00FC, 0x00DF, ' ', 'G', 'o', 't', 't', '.', ' ',
        0x0417, 0x0434, 0x0440, 0x0430, 0x0432, 0x0441, 0x0442, 0x0432, 0x0443,
        0x0439, 0x0442, 0x0435, '!', ' ',
        'x', '=', '(', '-', 'b', 0x00B1, 's', 'q', 'r', 't', '(', 'b', 0x00B2,
        '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')', ' ', ' ',
        0x65E5, 0x672C, 0x8A9E, ',', 0x4E2D, 0x6587, ',', 0xD55C, 0xAE00, '\n'
      };

    {
      char *p = (char *) malloc (SIZEOF (input));
      size_t i;

      u32_possible_linebreaks (input, SIZEOF (input), "GB18030", p);
      for (i = 0; i < 61; i++)
        {
          ASSERT (p[i] == (i == 60 ? UC_BREAK_MANDATORY :
                           i == 5
                           || i == 11 || i == 25
                           || i == 29 || i == 30
                           || i == 45 || i == 51
                           || i == 52 || i == 53 || i == 55 || i == 56
                           || i == 58 || i == 59 ? UC_BREAK_POSSIBLE :
                           UC_BREAK_PROHIBITED));
        }
      free (p);
    }

    {
      char *p = (char *) malloc (SIZEOF (input));
      size_t i;

      u32_possible_linebreaks (input, SIZEOF (input), "GB2312", p);
      for (i = 0; i < 61; i++)
        {
          ASSERT (p[i] == (i == 60 ? UC_BREAK_MANDATORY :
                           i == 5
                           || i == 11 || i == 25
                           || i == 29 || i == 30
                           || i == 37 || i == 45 || i == 51
                           || i == 52 || i == 53 || i == 55 || i == 56
                           || i == 58 || i == 59 ? UC_BREAK_POSSIBLE :
                           UC_BREAK_PROHIBITED));
        }
      free (p);
    }
  }

  /* Test that a break is possible after a zero-width space followed by some
     regular spaces (rule LB8 in Unicode TR#14 revision 26).  */
  {
    static const uint32_t input[4] = { 'x', 0x200B, ' ', 'y' };
    char *p = (char *) malloc (SIZEOF (input));
    size_t i;

    u32_possible_linebreaks (input, SIZEOF (input), "UTF-8", p);
    for (i = 0; i < 4; i++)
      {
        ASSERT (p[i] == (i == 3 ? UC_BREAK_POSSIBLE : UC_BREAK_PROHIBITED));
      }
    free (p);
  }

  return 0;
}
