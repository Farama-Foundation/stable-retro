/* Test of u8_width() function.
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
  static const uint8_t input[] =
    #define INPUT1_OFFSET 0
    #define INPUT1_LENGTH 13
    "Danilo \305\240egan"
    #define INPUT2_OFFSET 13
    #define INPUT2_LENGTH 14
    "Danilo S\314\214egan"
    #define INPUT3_OFFSET 27
    #define INPUT3_LENGTH 23
    "\320\224\320\260\320\275\320\270\320\273\320\276 \320\250\320\265\320\263\320\260\320\275"
    #define INPUT4_OFFSET 50
    #define INPUT4_LENGTH 9
    "\345\210\230\345\206\233\346\260\221"
    ;

  { /* "Danilo Šegan" */
    ASSERT (u8_width (input + INPUT1_OFFSET, INPUT1_LENGTH, "UTF-8") == 12);
  }

  { /* "Danilo Šegan" decomposed */
    ASSERT (u8_width (input + INPUT2_OFFSET, INPUT2_LENGTH, "UTF-8") == 12);
  }

  { /* "Данило Шеган" */
    ASSERT (u8_width (input + INPUT3_OFFSET, INPUT3_LENGTH, "UTF-8") == 12);
    ASSERT (u8_width (input + INPUT3_OFFSET, INPUT3_LENGTH, "EUC-JP") == 23);
  }

  { /* 刘军民 */
    ASSERT (u8_width (input + INPUT4_OFFSET, INPUT4_LENGTH, "UTF-8") == 6);
    ASSERT (u8_width (input + INPUT4_OFFSET, INPUT4_LENGTH, "GB2312") == 6);
  }

  return 0;
}
