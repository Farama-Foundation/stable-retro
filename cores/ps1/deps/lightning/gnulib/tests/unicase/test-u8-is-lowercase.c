/* Test of test whether an UTF-8 string is entirely lower case.
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

#include "unicase.h"

#include "unistr.h"
#include "macros.h"

#define UNIT uint8_t
#include "test-is-lowercase.h"
#undef UNIT

static void
test_nonascii (int (*my_is) (const uint8_t *, size_t, const char *, bool *))
{
  /* Test cases from Unicode 5.1.0.  */
  {
    static const uint8_t input[] = { 0xE2, 0x93, 0x97 };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const uint8_t input[] = { 0xE2, 0x92, 0xBD };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
  {
    static const uint8_t input[] = { 0xCA, 0xB0 };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const uint8_t input[] = { 0xE1, 0x80, 0xB4 };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const uint8_t input[] = { 0xCA, 0xBD };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
}

int
main ()
{
  test_ascii (u8_is_lowercase);
  test_nonascii (u8_is_lowercase);

  return 0;
}
