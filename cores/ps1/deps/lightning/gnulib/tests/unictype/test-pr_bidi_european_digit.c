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

    { 0x0030, 0x0039 },
    { 0x00B2, 0x00B3 },
    { 0x00B9, 0x00B9 },
    { 0x06F0, 0x06F9 },
    { 0x2070, 0x2070 },
    { 0x2074, 0x2079 },
    { 0x2080, 0x2089 },
    { 0x2488, 0x249B },
    { 0xFF10, 0xFF19 },
    { 0x102E1, 0x102FB },
    { 0x1D7CE, 0x1D7FF },
    { 0x1F100, 0x1F10A }

#define PREDICATE(c) uc_is_property_bidi_european_digit (c)
#include "test-predicate-part2.h"
