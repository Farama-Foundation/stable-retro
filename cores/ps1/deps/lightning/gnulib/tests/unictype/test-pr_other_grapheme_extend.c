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

    { 0x09BE, 0x09BE },
    { 0x09D7, 0x09D7 },
    { 0x0B3E, 0x0B3E },
    { 0x0B57, 0x0B57 },
    { 0x0BBE, 0x0BBE },
    { 0x0BD7, 0x0BD7 },
    { 0x0CC2, 0x0CC2 },
    { 0x0CD5, 0x0CD6 },
    { 0x0D3E, 0x0D3E },
    { 0x0D57, 0x0D57 },
    { 0x0DCF, 0x0DCF },
    { 0x0DDF, 0x0DDF },
    { 0x200C, 0x200C },
    { 0x302E, 0x302F },
    { 0xFF9E, 0xFF9F },
    { 0x1133E, 0x1133E },
    { 0x11357, 0x11357 },
    { 0x114B0, 0x114B0 },
    { 0x114BD, 0x114BD },
    { 0x115AF, 0x115AF },
    { 0x1D165, 0x1D165 },
    { 0x1D16E, 0x1D172 },
    { 0xE0020, 0xE007F }

#define PREDICATE(c) uc_is_property_other_grapheme_extend (c)
#include "test-predicate-part2.h"
