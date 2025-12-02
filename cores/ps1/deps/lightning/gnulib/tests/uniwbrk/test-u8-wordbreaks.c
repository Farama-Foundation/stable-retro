/* Test of word breaks in UTF-8 strings.
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
  u8_wordbreaks (NULL, 0, NULL);

  {
    static const uint8_t input[91] =
      /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
      "Gr\303\274\303\237 Gott. \320\227\320\264\321\200\320\260\320\262\321\201\321\202\320\262\321\203\320\271\321\202\320\265! x=(-b\302\261sqrt(b\302\262-4ac))/(2a)  \346\227\245\346\234\254\350\252\236,\344\270\255\346\226\207,\355\225\234\352\270\200\n";
    char *p = (char *) malloc (SIZEOF (input));
    size_t i;

    u8_wordbreaks (input, SIZEOF (input), p);

    for (i = 0; i < 91; i++)
      {
        ASSERT (p[i] == ((i >= 6 && i <= 7)
                         || (i >= 11 && i <= 13)
                         || (i >= 37 && i <= 44)
                         || i == 46 || (i >= 50 && i <= 52)
                         || (i >= 54 && i <= 55)
                         || (i >= 58 && i <= 62) || (i >= 64 && i <= 67)
                         || i == 70 || i == 73 || i == 76
                         || i == 77 || i == 80 || i == 83
                         || i == 84 || i == 90
                         ? 1 : 0));
      }
    free (p);
  }

  {
    /* Same input string, decomposed.  */
    static const uint8_t input[106] =
      /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
      "Gru\314\210\303\237 Gott. \320\227\320\264\321\200\320\260\320\262\321\201\321\202\320\262\321\203\320\270\314\206\321\202\320\265! x=(-b\302\261sqrt(b\302\262-4ac))/(2a)  \346\227\245\346\234\254\350\252\236,\344\270\255\346\226\207,\341\204\222\341\205\241\341\206\253\341\204\200\341\205\263\341\206\257\n";
    char *p = (char *) malloc (SIZEOF (input));
    size_t i;

    u8_wordbreaks (input, SIZEOF (input), p);

    for (i = 0; i < 106; i++)
      {
        ASSERT (p[i] == ((i >= 7 && i <= 8)
                         || (i >= 12 && i <= 14)
                         || (i >= 40 && i <= 47)
                         || i == 49 || (i >= 53 && i <= 55)
                         || (i >= 57 && i <= 58)
                         || (i >= 61 && i <= 65) || (i >= 67 && i <= 70)
                         || i == 73 || i == 76 || i == 79
                         || i == 80 || i == 83 || i == 86
                         || i == 87 || i == 105
                         ? 1 : 0));
      }
    free (p);
  }

  return 0;
}
