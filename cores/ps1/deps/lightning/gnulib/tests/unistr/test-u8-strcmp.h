/* Test of u8_strcmp() and u8_strcoll() functions.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#define UNIT uint8_t
#include "test-strcmp.h"

static void
test_u8_strcmp (void)
{
  test_strcmp ();

  /* Test comparison between ASCII and non-ASCII characters.  */
  {
    static const UNIT input1[] = { 'f', 'o', 'o', 0 };
    static const UNIT input2[] = { 0xE2, 0x80, 0xA2, 0 };
    ASSERT (U_STRCMP (input1, input2) < 0);
    ASSERT (U_STRCMP (input2, input1) > 0);
  }

  /* Test comparison with non-BMP characters.  */
  {
    static const UNIT input1[] = { 0xF0, 0x9D, 0x94, 0x9E, 0 };
    static const UNIT input2[] = { 0xEF, 0xBB, 0xBF, 0 };
    ASSERT (U_STRCMP (input1, input2) > 0);
    ASSERT (U_STRCMP (input2, input1) < 0);
  }
}
