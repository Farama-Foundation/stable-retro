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

    { 0x3006, 0x3007 },
    { 0x3021, 0x3029 },
    { 0x3038, 0x303A },
    { 0x3400, 0x4DB5 },
    { 0x4E00, 0x9FD5 },
    { 0xF900, 0xFA6D },
    { 0xFA70, 0xFAD9 },
    { 0x17000, 0x187EC },
    { 0x18800, 0x18AF2 },
    { 0x20000, 0x2A6D6 },
    { 0x2A700, 0x2B734 },
    { 0x2B740, 0x2B81D },
    { 0x2B820, 0x2CEA1 },
    { 0x2F800, 0x2FA1D }

#define PREDICATE(c) uc_is_property_ideographic (c)
#include "test-predicate-part2.h"
