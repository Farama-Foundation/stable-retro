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

    { 0x3400, 0x4DB5 },
    { 0x4E00, 0x9FD5 },
    { 0xFA0E, 0xFA0F },
    { 0xFA11, 0xFA11 },
    { 0xFA13, 0xFA14 },
    { 0xFA1F, 0xFA1F },
    { 0xFA21, 0xFA21 },
    { 0xFA23, 0xFA24 },
    { 0xFA27, 0xFA29 },
    { 0x20000, 0x2A6D6 },
    { 0x2A700, 0x2B734 },
    { 0x2B740, 0x2B81D },
    { 0x2B820, 0x2CEA1 }

#define PREDICATE(c) uc_is_property_unified_ideograph (c)
#include "test-predicate-part2.h"
