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

    { 0x005F, 0x005F },
    { 0x203F, 0x2040 },
    { 0x2054, 0x2054 },
    { 0xFE33, 0xFE34 },
    { 0xFE4D, 0xFE4F },
    { 0xFF3F, 0xFF3F }

#define PREDICATE(c) uc_is_general_category (c, UC_CATEGORY_Pc)
#include "test-predicate-part2.h"
