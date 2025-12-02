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

    { 0x0028, 0x0029 },
    { 0x005B, 0x005B },
    { 0x005D, 0x005D },
    { 0x007B, 0x007B },
    { 0x007D, 0x007D },
    { 0x00AB, 0x00AB },
    { 0x00BB, 0x00BB },
    { 0x0F3A, 0x0F3D },
    { 0x169B, 0x169C },
    { 0x2018, 0x201F },
    { 0x2039, 0x203A },
    { 0x2045, 0x2046 },
    { 0x207D, 0x207E },
    { 0x208D, 0x208E },
    { 0x2329, 0x232A },
    { 0x3008, 0x3011 },
    { 0x3014, 0x301B },
    { 0x301D, 0x301F },
    { 0xFD3E, 0xFD3F },
    { 0xFE35, 0xFE44 },
    { 0xFE59, 0xFE5E },
    { 0xFF08, 0xFF09 },
    { 0xFF3B, 0xFF3B },
    { 0xFF3D, 0xFF3D },
    { 0xFF5B, 0xFF5B },
    { 0xFF5D, 0xFF5D },
    { 0xFF62, 0xFF63 }

#define PREDICATE(c) uc_is_property_paired_punctuation (c)
#include "test-predicate-part2.h"
