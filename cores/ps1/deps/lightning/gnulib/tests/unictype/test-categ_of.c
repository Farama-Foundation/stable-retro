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
  {
    unsigned int c;
    for (c = 0; c < 0x110000; c++)
      ASSERT (uc_is_general_category (c, uc_general_category (c)));
  }

  {
    uc_general_category_t decimal_digits = uc_general_category ('7');
    unsigned int c;

    for (c = 0x30; c <= 0x39; c++)
      ASSERT (uc_is_general_category (c, decimal_digits));
    for (c = 0x40; c < 0x80; c++)
      ASSERT (!uc_is_general_category (c, decimal_digits));
  }

  {
    uc_general_category_t lowercase_letters = uc_general_category ('x');
    unsigned int c;

    for (c = 0x41; c <= 0x5A; c++)
      ASSERT (!uc_is_general_category (c, lowercase_letters));
    for (c = 0x61; c <= 0x7A; c++)
      ASSERT (uc_is_general_category (c, lowercase_letters));
  }

  return 0;
}
