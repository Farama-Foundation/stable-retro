/* Test of u16_mblen() function.
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

int
main ()
{
  int ret;

  /* Test zero-length input.  */
  {
    static const uint16_t input[] = { 0 };
    ret = u16_mblen (input, 0);
    ASSERT (ret == -1);
  }

  /* Test NUL unit input.  */
  {
    static const uint16_t input[] = { 0 };
    ret = u16_mblen (input, 1);
    ASSERT (ret == 0);
  }

  /* Test ISO 646 unit input.  */
  {
    ucs4_t c;
    uint16_t buf[1];

    for (c = 1; c < 0x80; c++)
      {
        buf[0] = c;
        ret = u16_mblen (buf, 1);
        ASSERT (ret == 1);
      }
  }

  /* Test BMP unit input.  */
  {
    static const uint16_t input[] = { 0x20AC };
    ret = u16_mblen (input, 1);
    ASSERT (ret == 1);
  }

  /* Test 2-units character input.  */
  {
    static const uint16_t input[] = { 0xD835, 0xDD1F };
    ret = u16_mblen (input, 2);
    ASSERT (ret == 2);
  }

  /* Test incomplete/invalid 1-unit input.  */
  {
    static const uint16_t input[] = { 0xD835 };
    ret = u16_mblen (input, 1);
    ASSERT (ret == -1);
  }
  {
    static const uint16_t input[] = { 0xDD1F };
    ret = u16_mblen (input, 1);
    ASSERT (ret == -1);
  }

  return 0;
}
