/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* Test the Unicode character type functions.
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#include "test-predicate-part1.h"

    { 0x01C5, 0x01C5 },
    { 0x01C8, 0x01C8 },
    { 0x01CB, 0x01CB },
    { 0x01F2, 0x01F2 },
    { 0x1F88, 0x1F8F },
    { 0x1F98, 0x1F9F },
    { 0x1FA8, 0x1FAF },
    { 0x1FBC, 0x1FBC },
    { 0x1FCC, 0x1FCC },
    { 0x1FFC, 0x1FFC }

#define PREDICATE(c) uc_is_general_category (c, UC_CATEGORY_Lt)
#include "test-predicate-part2.h"
