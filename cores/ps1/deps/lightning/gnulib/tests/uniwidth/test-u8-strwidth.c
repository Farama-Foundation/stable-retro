/* Test of u8_strwidth() function.
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
    static const uint8_t input[] = "Danilo \305\240egan"; /* "Danilo Šegan" */
    ASSERT (u8_strwidth (input, "UTF-8") == 12);
  }

  {
    static const uint8_t input[] = "Danilo S\314\214egan"; /* "Danilo Šegan" decomposed */
    ASSERT (u8_strwidth (input, "UTF-8") == 12);
  }

  {
    static const uint8_t input[] = /* "Данило Шеган" */
      "\320\224\320\260\320\275\320\270\320\273\320\276 \320\250\320\265\320\263\320\260\320\275";
    ASSERT (u8_strwidth (input, "UTF-8") == 12);
    ASSERT (u8_strwidth (input, "EUC-JP") == 23);
  }

  {
    static const uint8_t input[] = /* 刘军民 */
      "\345\210\230\345\206\233\346\260\221";
    ASSERT (u8_strwidth (input, "UTF-8") == 6);
    ASSERT (u8_strwidth (input, "GB2312") == 6);
  }

  return 0;
}
