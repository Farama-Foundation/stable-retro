/* Test of u32_width() function.
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

#include "uniwidth.h"

#include "macros.h"

int
main ()
{
  static const uint32_t input[] =
    {
      #define INPUT1_OFFSET 0
      #define INPUT1_LENGTH 12
      'D', 'a', 'n', 'i', 'l', 'o', ' ', 0x0160, 'e', 'g', 'a', 'n',
      #define INPUT2_OFFSET 12
      #define INPUT2_LENGTH 13
      'D', 'a', 'n', 'i', 'l', 'o', ' ', 'S', 0x030C, 'e', 'g', 'a', 'n',
      #define INPUT3_OFFSET 25
      #define INPUT3_LENGTH 12
      0x0414, 0x0430, 0x043D, 0x0438, 0x043B, 0x043E, ' ',
      0x0428, 0x0435, 0x0433, 0x0430, 0x043D,
      #define INPUT4_OFFSET 37
      #define INPUT4_LENGTH 3
      0x5218, 0x519B, 0x6C11
    };

  { /* "Danilo Šegan" */
    ASSERT (u32_width (input + INPUT1_OFFSET, INPUT1_LENGTH, "UTF-8") == 12);
  }

  { /* "Danilo Šegan" decomposed */
    ASSERT (u32_width (input + INPUT2_OFFSET, INPUT2_LENGTH, "UTF-8") == 12);
  }

  { /* "Данило Шеган" */
    ASSERT (u32_width (input + INPUT3_OFFSET, INPUT3_LENGTH, "UTF-8") == 12);
    ASSERT (u32_width (input + INPUT3_OFFSET, INPUT3_LENGTH, "EUC-JP") == 23);
  }

  { /* 刘军民 */
    ASSERT (u32_width (input + INPUT4_OFFSET, INPUT4_LENGTH, "UTF-8") == 6);
    ASSERT (u32_width (input + INPUT4_OFFSET, INPUT4_LENGTH, "GB2312") == 6);
  }

  return 0;
}
