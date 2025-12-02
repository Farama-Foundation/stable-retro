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

    { 0x0022, 0x0022 },
    { 0x0027, 0x0027 },
    { 0x00AB, 0x00AB },
    { 0x00BB, 0x00BB },
    { 0x2018, 0x201F },
    { 0x2039, 0x203A },
    { 0x2E42, 0x2E42 },
    { 0x300C, 0x300F },
    { 0x301D, 0x301F },
    { 0xFE41, 0xFE44 },
    { 0xFF02, 0xFF02 },
    { 0xFF07, 0xFF07 },
    { 0xFF62, 0xFF63 }

#define PREDICATE(c) uc_is_property_quotation_mark (c)
#include "test-predicate-part2.h"
