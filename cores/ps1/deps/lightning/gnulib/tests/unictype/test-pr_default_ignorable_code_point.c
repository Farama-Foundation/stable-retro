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

    { 0x00AD, 0x00AD },
    { 0x034F, 0x034F },
    { 0x061C, 0x061C },
    { 0x115F, 0x1160 },
    { 0x17B4, 0x17B5 },
    { 0x180B, 0x180E },
    { 0x200B, 0x200F },
    { 0x202A, 0x202E },
    { 0x2060, 0x206F },
    { 0x3164, 0x3164 },
    { 0xFE00, 0xFE0F },
    { 0xFEFF, 0xFEFF },
    { 0xFFA0, 0xFFA0 },
    { 0xFFF0, 0xFFF8 },
    { 0x1BCA0, 0x1BCA3 },
    { 0x1D173, 0x1D17A },
    { 0xE0000, 0xE0FFF }

#define PREDICATE(c) uc_is_property_default_ignorable_code_point (c)
#include "test-predicate-part2.h"
