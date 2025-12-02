/* Test of u32_strcmp() and u32_strcoll() functions.
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

#define UNIT uint32_t
#include "test-strcmp.h"

static void
test_u32_strcmp (void)
{
  test_strcmp ();

  /* Test comparison between ASCII and non-ASCII characters.  */
  {
    static const UNIT input1[] = { 'f', 'o', 'o', 0 };
    static const UNIT input2[] = { 0x2022, 0 };
    ASSERT (U_STRCMP (input1, input2) < 0);
    ASSERT (U_STRCMP (input2, input1) > 0);
  }

  /* Test comparison with non-BMP characters.  */
  {
    static const UNIT input1[] = { 0x1D51F, 0 };
    static const UNIT input2[] = { 0xFEFF, 0 };
    ASSERT (U_STRCMP (input1, input2) > 0);
    ASSERT (U_STRCMP (input2, input1) < 0);
  }
}
