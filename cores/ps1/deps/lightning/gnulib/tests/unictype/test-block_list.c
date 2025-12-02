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
  const uc_block_t *blocks;
  size_t nblocks;

  uc_all_blocks (&blocks, &nblocks);

  {
    const uc_block_t *latin_block = NULL;
    const uc_block_t *hebrew_block = NULL;
    size_t i;

    for (i = 0; i < nblocks; i++)
      {
        if (strcmp (blocks[i].name, "Latin") == 0)
          latin_block = &blocks[i];
        if (strcmp (blocks[i].name, "Hebrew") == 0)
          hebrew_block = &blocks[i];
      }
    ASSERT (latin_block == NULL);
    ASSERT (hebrew_block != NULL);
  }

  return 0;
}
