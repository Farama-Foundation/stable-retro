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

    { 0x0021, 0x002F },
    { 0x003A, 0x0040 },
    { 0x005B, 0x005E },
    { 0x0060, 0x0060 },
    { 0x007B, 0x007E },
    { 0x00A1, 0x00A7 },
    { 0x00A9, 0x00A9 },
    { 0x00AB, 0x00AC },
    { 0x00AE, 0x00AE },
    { 0x00B0, 0x00B1 },
    { 0x00B6, 0x00B6 },
    { 0x00BB, 0x00BB },
    { 0x00BF, 0x00BF },
    { 0x00D7, 0x00D7 },
    { 0x00F7, 0x00F7 },
    { 0x2010, 0x2027 },
    { 0x2030, 0x203E },
    { 0x2041, 0x2053 },
    { 0x2055, 0x205E },
    { 0x2190, 0x245F },
    { 0x2500, 0x2775 },
    { 0x2794, 0x2BFF },
    { 0x2E00, 0x2E7F },
    { 0x3001, 0x3003 },
    { 0x3008, 0x3020 },
    { 0x3030, 0x3030 },
    { 0xFD3E, 0xFD3F },
    { 0xFE45, 0xFE46 }

#define PREDICATE(c) uc_is_property_pattern_syntax (c)
#include "test-predicate-part2.h"
