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

    { 0x002D, 0x002D },
    { 0x058A, 0x058A },
    { 0x05BE, 0x05BE },
    { 0x1400, 0x1400 },
    { 0x1806, 0x1806 },
    { 0x2010, 0x2015 },
    { 0x2E17, 0x2E17 },
    { 0x2E1A, 0x2E1A },
    { 0x2E3A, 0x2E3B },
    { 0x2E40, 0x2E40 },
    { 0x301C, 0x301C },
    { 0x3030, 0x3030 },
    { 0x30A0, 0x30A0 },
    { 0xFE31, 0xFE32 },
    { 0xFE58, 0xFE58 },
    { 0xFE63, 0xFE63 },
    { 0xFF0D, 0xFF0D }

#define PREDICATE(c) uc_is_general_category (c, UC_CATEGORY_Pd)
#include "test-predicate-part2.h"
