/* Test of u8_strchr() function.
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

/* Written by Paolo Bonzini <bonzini@gnu.org>, 2010.  */

#include <config.h>

#include "unistr.h"

#include <stdlib.h>
#include <string.h>

#include "zerosize-ptr.h"
#include "macros.h"

#define UNIT uint8_t
#define U_UCTOMB u8_uctomb
#define U32_TO_U u32_to_u8
#define U_STRCHR u8_strchr
#define U_SET u8_set
#include "test-strchr.h"

int
main (void)
{
  test_strchr ();

  /* Check that u8_strchr() does not read past the end of the string.  */
  {
    char *page_boundary = (char *) zerosize_ptr ();

    if (page_boundary != NULL)
      {
        UNIT *mem;

        mem = (UNIT *) (page_boundary - 1 * sizeof (UNIT));
        mem[0] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 2 * sizeof (UNIT));
        mem[0] = 0x50;
        mem[1] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 3 * sizeof (UNIT));
        mem[0] = 0x50;
        mem[1] = 0x50;
        mem[2] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 3 * sizeof (UNIT));
        mem[0] = 0xC4; mem[1] = 0xA0; /* U+0120 */
        mem[2] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 3 * sizeof (UNIT));
        mem[0] = 0xC5; mem[1] = 0xA3; /* U+0163 */
        mem[2] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 4 * sizeof (UNIT));
        mem[0] = 0x50;
        mem[1] = 0x50;
        mem[2] = 0x50;
        mem[3] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 4 * sizeof (UNIT));
        mem[0] = 0x50;
        mem[1] = 0xC5; mem[2] = 0xA3; /* U+0163 */
        mem[3] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);
        ASSERT (u8_strchr (mem, 0x163) == mem + 1);

        mem = (UNIT *) (page_boundary - 4 * sizeof (UNIT));
        mem[0] = 0xE3; mem[1] = 0x91; mem[2] = 0x00; /* U+3450 */
        mem[3] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 4 * sizeof (UNIT));
        mem[0] = 0xE3; mem[1] = 0x92; mem[2] = 0x96; /* U+3496 */
        mem[3] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 5 * sizeof (UNIT));
        mem[0] = 0x50;
        mem[1] = 0x50;
        mem[2] = 0x50;
        mem[3] = 0x50;
        mem[4] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);

        mem = (UNIT *) (page_boundary - 5 * sizeof (UNIT));
        mem[0] = 0x50;
        mem[1] = 0xE3; mem[2] = 0x92; mem[3] = 0x96; /* U+3496 */
        mem[4] = 0;
        ASSERT (u8_strchr (mem, 0x55) == NULL);
        ASSERT (u8_strchr (mem, 0x123) == NULL);
        ASSERT (u8_strchr (mem, 0x3456) == NULL);
        ASSERT (u8_strchr (mem, 0x23456) == NULL);
        ASSERT (u8_strchr (mem, 0x3496) == mem + 1);
      }
  }

  return 0;
}
