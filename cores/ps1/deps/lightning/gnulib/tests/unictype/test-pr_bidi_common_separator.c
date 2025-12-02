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

    { 0x002C, 0x002C },
    { 0x002E, 0x002F },
    { 0x003A, 0x003A },
    { 0x00A0, 0x00A0 },
    { 0x060C, 0x060C },
    { 0x202F, 0x202F },
    { 0x2044, 0x2044 },
    { 0xFE50, 0xFE50 },
    { 0xFE52, 0xFE52 },
    { 0xFE55, 0xFE55 },
    { 0xFF0C, 0xFF0C },
    { 0xFF0E, 0xFF0F },
    { 0xFF1A, 0xFF1A }

#define PREDICATE(c) uc_is_property_bidi_common_separator (c)
#include "test-predicate-part2.h"
