/* Test of u16_uctomb() function.
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

#define MAGIC 0xBADE

int
main ()
{
  /* Test ISO 646 character, in particular the NUL character.  */
  {
    ucs4_t uc;

    for (uc = 0; uc < 0x80; uc++)
      {
        uint16_t buf[3] = { MAGIC, MAGIC, MAGIC };
        int ret;

        ret = u16_uctomb (buf, uc, 0);
        ASSERT (ret == -2);
        ASSERT (buf[0] == MAGIC);

        ret = u16_uctomb (buf, uc, 1);
        ASSERT (ret == 1);
        ASSERT (buf[0] == uc);
        ASSERT (buf[1] == MAGIC);
      }
  }

  /* Test BMP character.  */
  {
    ucs4_t uc = 0x20AC;
    uint16_t buf[3] = { MAGIC, MAGIC, MAGIC };
    int ret;

    ret = u16_uctomb (buf, uc, 0);
    ASSERT (ret == -2);
    ASSERT (buf[0] == MAGIC);

    ret = u16_uctomb (buf, uc, 1);
    ASSERT (ret == 1);
    ASSERT (buf[0] == uc);
    ASSERT (buf[1] == MAGIC);
  }

  /* Test non-BMP character.  */
  {
    ucs4_t uc = 0x10FFFD;
    uint16_t buf[3] = { MAGIC, MAGIC, MAGIC };
    int ret;

    ret = u16_uctomb (buf, uc, 0);
    ASSERT (ret == -2);
    ASSERT (buf[0] == MAGIC);

    ret = u16_uctomb (buf, uc, 1);
    ASSERT (ret == -2);
    ASSERT (buf[0] == MAGIC);

    ret = u16_uctomb (buf, uc, 2);
    ASSERT (ret == 2);
    ASSERT (buf[0] == 0xDBFF);
    ASSERT (buf[1] == 0xDFFD);
    ASSERT (buf[2] == MAGIC);
  }

  /* Test invalid characters.  */
  {
    ucs4_t invalid[] = { 0x110000, 0xD800, 0xDBFF, 0xDC00, 0xDFFF };
    uint16_t buf[3] = { MAGIC, MAGIC, MAGIC };
    size_t i;

    for (i = 0; i < SIZEOF (invalid); i++)
      {
        ucs4_t uc = invalid[i];
        int n;

        for (n = 0; n <= 2; n++)
          {
            int ret = u16_uctomb (buf, uc, n);
            ASSERT (ret == -1);
            ASSERT (buf[0] == MAGIC);
            ASSERT (buf[1] == MAGIC);
            ASSERT (buf[2] == MAGIC);
          }
      }
  }

  return 0;
}
