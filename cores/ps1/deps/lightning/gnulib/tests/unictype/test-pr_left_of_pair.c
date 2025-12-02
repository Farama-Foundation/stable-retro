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

    { 0x0028, 0x0028 },
    { 0x005B, 0x005B },
    { 0x007B, 0x007B },
    { 0x00AB, 0x00AB },
    { 0x0F3A, 0x0F3A },
    { 0x0F3C, 0x0F3C },
    { 0x169B, 0x169B },
    { 0x2018, 0x2018 },
    { 0x201A, 0x201C },
    { 0x201E, 0x201F },
    { 0x2039, 0x2039 },
    { 0x2045, 0x2045 },
    { 0x207D, 0x207D },
    { 0x208D, 0x208D },
    { 0x2329, 0x2329 },
    { 0x3008, 0x3008 },
    { 0x300A, 0x300A },
    { 0x300C, 0x300C },
    { 0x300E, 0x300E },
    { 0x3010, 0x3010 },
    { 0x3014, 0x3014 },
    { 0x3016, 0x3016 },
    { 0x3018, 0x3018 },
    { 0x301A, 0x301A },
    { 0x301D, 0x301D },
    { 0xFD3E, 0xFD3E },
    { 0xFE35, 0xFE35 },
    { 0xFE37, 0xFE37 },
    { 0xFE39, 0xFE39 },
    { 0xFE3B, 0xFE3B },
    { 0xFE3D, 0xFE3D },
    { 0xFE3F, 0xFE3F },
    { 0xFE41, 0xFE41 },
    { 0xFE43, 0xFE43 },
    { 0xFE59, 0xFE59 },
    { 0xFE5B, 0xFE5B },
    { 0xFE5D, 0xFE5D },
    { 0xFF08, 0xFF08 },
    { 0xFF3B, 0xFF3B },
    { 0xFF5B, 0xFF5B },
    { 0xFF62, 0xFF62 }

#define PREDICATE(c) uc_is_property_left_of_pair (c)
#include "test-predicate-part2.h"
