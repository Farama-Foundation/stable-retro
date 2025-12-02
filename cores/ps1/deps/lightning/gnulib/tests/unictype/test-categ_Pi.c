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

    { 0x00AB, 0x00AB },
    { 0x2018, 0x2018 },
    { 0x201B, 0x201C },
    { 0x201F, 0x201F },
    { 0x2039, 0x2039 },
    { 0x2E02, 0x2E02 },
    { 0x2E04, 0x2E04 },
    { 0x2E09, 0x2E09 },
    { 0x2E0C, 0x2E0C },
    { 0x2E1C, 0x2E1C },
    { 0x2E20, 0x2E20 }

#define PREDICATE(c) uc_is_general_category (c, UC_CATEGORY_Pi)
#include "test-predicate-part2.h"
