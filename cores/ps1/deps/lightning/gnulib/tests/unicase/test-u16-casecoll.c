/* Test of locale dependent, case and normalization insensitive comparison of
   UTF-16 strings.
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

#include "uninorm.h"
#include "macros.h"

#define UNIT uint16_t
#include "test-casecmp.h"
#undef UNIT

int
main ()
{
  /* In the "C" locale, strcoll is equivalent to strcmp, therefore u8_casecoll
     on ASCII strings should behave like strcasecmp.  */
  test_ascii (u16_casecoll, UNINORM_NFC);

  return 0;
}
