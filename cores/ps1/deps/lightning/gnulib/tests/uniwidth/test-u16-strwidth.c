/* Test of u16_strwidth() function.
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
  {
    static const uint16_t input[] = /* "Danilo Šegan" */
      { 'D', 'a', 'n', 'i', 'l', 'o', ' ', 0x0160, 'e', 'g', 'a', 'n', 0 };
    ASSERT (u16_strwidth (input, "UTF-8") == 12);
  }

  {
    static const uint16_t input[] = /* "Danilo Šegan" decomposed */
      { 'D', 'a', 'n', 'i', 'l', 'o', ' ', 'S', 0x030C, 'e', 'g', 'a', 'n', 0 };
    ASSERT (u16_strwidth (input, "UTF-8") == 12);
  }

  {
    static const uint16_t input[] = /* "Данило Шеган" */
      {
        0x0414, 0x0430, 0x043D, 0x0438, 0x043B, 0x043E, ' ',
        0x0428, 0x0435, 0x0433, 0x0430, 0x043D, 0
      };
    ASSERT (u16_strwidth (input, "UTF-8") == 12);
    ASSERT (u16_strwidth (input, "EUC-JP") == 23);
  }

  {
    static const uint16_t input[] = /* 刘军民 */
      { 0x5218, 0x519B, 0x6C11, 0 };
    ASSERT (u16_strwidth (input, "UTF-8") == 6);
    ASSERT (u16_strwidth (input, "GB2312") == 6);
  }

  return 0;
}
