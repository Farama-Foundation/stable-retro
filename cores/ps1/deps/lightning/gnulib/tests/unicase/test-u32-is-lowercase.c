/* Test of test whether an UTF-32 string is entirely lower case.
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

#define UNIT uint32_t
#include "test-is-lowercase.h"
#undef UNIT

static void
test_nonascii (int (*my_is) (const uint32_t *, size_t, const char *, bool *))
{
  /* Test cases from Unicode 5.1.0.  */
  {
    static const uint32_t input[] = { 0x24D7 };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const uint32_t input[] = { 0x24BD };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
  {
    static const uint32_t input[] = { 0x02B0 };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const uint32_t input[] = { 0x1D34 };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const uint32_t input[] = { 0x02BD };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
}

int
main ()
{
  test_ascii (u32_is_lowercase);
  test_nonascii (u32_is_lowercase);

  return 0;
}
