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

    { 0x0590, 0x0590 },
    { 0x05BE, 0x05BE },
    { 0x05C0, 0x05C0 },
    { 0x05C3, 0x05C3 },
    { 0x05C6, 0x05C6 },
    { 0x05C8, 0x05FF },
    { 0x07C0, 0x07EA },
    { 0x07F4, 0x07F5 },
    { 0x07FA, 0x0815 },
    { 0x081A, 0x081A },
    { 0x0824, 0x0824 },
    { 0x0828, 0x0828 },
    { 0x082E, 0x0858 },
    { 0x085C, 0x089F },
    { 0x08B5, 0x08B5 },
    { 0x08BE, 0x08D3 },
    { 0x200F, 0x200F },
    { 0xFB1D, 0xFB1D },
    { 0xFB1F, 0xFB28 },
    { 0xFB2A, 0xFB4F },
    { 0x10800, 0x1091E },
    { 0x10920, 0x10A00 },
    { 0x10A04, 0x10A04 },
    { 0x10A07, 0x10A0B },
    { 0x10A10, 0x10A37 },
    { 0x10A3B, 0x10A3E },
    { 0x10A40, 0x10AE4 },
    { 0x10AE7, 0x10B38 },
    { 0x10B40, 0x10E5F },
    { 0x10E7F, 0x10FFF },
    { 0x1E800, 0x1E8C4 },
    { 0x1E8C7, 0x1E8CF },
    { 0x1E900, 0x1E943 },
    { 0x1E950, 0x1E959 },
    { 0x1E95E, 0x1E95F }

#define PREDICATE(c) uc_is_property_bidi_hebrew_right_to_left (c)
#include "test-predicate-part2.h"
