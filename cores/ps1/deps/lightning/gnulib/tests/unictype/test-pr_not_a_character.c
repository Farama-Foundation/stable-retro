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

    { 0xFDD0, 0xFDEF },
    { 0xFFFE, 0xFFFF },
    { 0x1FFFE, 0x1FFFF },
    { 0x2FFFE, 0x2FFFF },
    { 0x3FFFE, 0x3FFFF },
    { 0x4FFFE, 0x4FFFF },
    { 0x5FFFE, 0x5FFFF },
    { 0x6FFFE, 0x6FFFF },
    { 0x7FFFE, 0x7FFFF },
    { 0x8FFFE, 0x8FFFF },
    { 0x9FFFE, 0x9FFFF },
    { 0xAFFFE, 0xAFFFF },
    { 0xBFFFE, 0xBFFFF },
    { 0xCFFFE, 0xCFFFF },
    { 0xDFFFE, 0xDFFFF },
    { 0xEFFFE, 0xEFFFF },
    { 0xFFFFE, 0xFFFFF },
    { 0x10FFFE, 0x10FFFF }

#define PREDICATE(c) uc_is_property_not_a_character (c)
#include "test-predicate-part2.h"
