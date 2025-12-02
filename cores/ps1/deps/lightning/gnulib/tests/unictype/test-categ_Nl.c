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

    { 0x16EE, 0x16F0 },
    { 0x2160, 0x2182 },
    { 0x2185, 0x2188 },
    { 0x3007, 0x3007 },
    { 0x3021, 0x3029 },
    { 0x3038, 0x303A },
    { 0xA6E6, 0xA6EF },
    { 0x10140, 0x10174 },
    { 0x10341, 0x10341 },
    { 0x1034A, 0x1034A },
    { 0x103D1, 0x103D5 },
    { 0x12400, 0x1246E }

#define PREDICATE(c) uc_is_general_category (c, UC_CATEGORY_Nl)
#include "test-predicate-part2.h"
