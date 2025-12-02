/* Test of u8_strncmp() function.
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

#include <config.h>

#include "unistr.h"

#include "macros.h"

#define UNIT uint8_t
#define U_STRNCMP u8_strncmp
#include "test-strncmp.h"

int
main ()
{
  test_strncmp ();

  /* Test comparison with non-BMP characters.  */
  {
    static const UNIT input1[] = { 0xF0, 0x9D, 0x94, 0x9E, 0 };
    static const UNIT input2[] = { 0xEF, 0xBB, 0xBF, 0 };
    ASSERT (U_STRNCMP (input1, input2, 1) > 0);
    ASSERT (U_STRNCMP (input2, input1, 1) < 0);
    ASSERT (U_STRNCMP (input1, input2, 2) > 0);
    ASSERT (U_STRNCMP (input2, input1, 2) < 0);
    ASSERT (U_STRNCMP (input1, input2, 3) > 0);
    ASSERT (U_STRNCMP (input2, input1, 3) < 0);
    ASSERT (U_STRNCMP (input1, input2, 4) > 0);
    ASSERT (U_STRNCMP (input2, input1, 4) < 0);
    ASSERT (U_STRNCMP (input1, input2, 5) > 0);
    ASSERT (U_STRNCMP (input2, input1, 5) < 0);
    ASSERT (U_STRNCMP (input1, input2, 1000000) > 0);
    ASSERT (U_STRNCMP (input2, input1, 1000000) < 0);
  }

  return 0;
}
