/* Test of u32_strmblen() function.
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

  /* Test NUL unit input.  */
  {
    static const uint32_t input[] = { 0 };
    ret = u32_strmblen (input);
    ASSERT (ret == 0);
  }

  /* Test ISO 646 unit input.  */
  {
    ucs4_t c;
    uint32_t buf[2];

    for (c = 1; c < 0x80; c++)
      {
        buf[0] = c;
        buf[1] = 0;
        ret = u32_strmblen (buf);
        ASSERT (ret == 1);
      }
  }

  /* Test BMP unit input.  */
  {
    static const uint32_t input[] = { 0x20AC, 0 };
    ret = u32_strmblen (input);
    ASSERT (ret == 1);
  }

  /* Test non-BMP unit input.  */
  {
    static const uint32_t input[] = { 0x1D51F, 0 };
    ret = u32_strmblen (input);
    ASSERT (ret == 1);
  }

  /* Test incomplete/invalid 1-unit input.  */
  {
    static const uint32_t input[] = { 0x340000, 0 };
    ret = u32_strmblen (input);
    ASSERT (ret == -1);
  }

  return 0;
}
