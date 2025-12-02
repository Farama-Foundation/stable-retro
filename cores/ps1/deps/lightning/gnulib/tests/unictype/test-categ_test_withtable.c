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
  uint32_t ctmask =
    (UC_CATEGORY_MASK_L | UC_CATEGORY_MASK_N)
    & ~(UC_CATEGORY_MASK_Lu | UC_CATEGORY_MASK_Nd);

  ASSERT (!uc_is_general_category_withtable ('A', ctmask));
  ASSERT (uc_is_general_category_withtable ('a', ctmask));
  ASSERT (!uc_is_general_category_withtable ('7', ctmask));
  ASSERT (uc_is_general_category_withtable (0x00B2, ctmask));

  return 0;
}
