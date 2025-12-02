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
    uc_property_t pr = uc_property_byname ("composite");
    unsigned int c;

    for (c = 0; c < 0x110000; c++)
      ASSERT (uc_is_property (c, pr) == uc_is_property_composite (c));
  }

  {
    uc_property_t pr = uc_property_byname ("foobar");
    ASSERT (! uc_property_is_valid (pr));
  }

  return 0;
}
