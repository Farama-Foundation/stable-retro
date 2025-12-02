/* Test the Unicode character type functions.
   Copyright (C) 2007-2009 Free Software Foundation, Inc.

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

#include <config.h>

#include "unictype.h"

#include <string.h>

#include "macros.h"

int
main ()
{
  ASSERT (strcmp (uc_block ('x')->name, "Basic Latin") == 0);
  ASSERT (strcmp (uc_block (0x20AC)->name, "Currency Symbols") == 0);

  {
    unsigned int c;

    for (c = 0; c < 0x110000; c++)
      {
        const uc_block_t *block = uc_block (c);
        ASSERT (block == NULL || uc_is_block (c, block));
      }
  }

  {
    const uc_block_t *blocks;
    size_t nblocks;
    size_t i;

    uc_all_blocks (&blocks, &nblocks);

    for (i = 0; i < nblocks; i++)
      {
        if (strcmp (blocks[i].name, "Hebrew") == 0)
          {
            ASSERT (uc_block (0x05DE) == &blocks[i]);
            ASSERT (uc_is_block (0x05DE, &blocks[i]));
          }
      }

    for (i = 0; i < nblocks; i++)
      {
        unsigned int c;

        for (c = blocks[i].start; c <= blocks[i].end; c++)
          ASSERT (uc_block (c) == &blocks[i]);
      }
  }

  return 0;
}
