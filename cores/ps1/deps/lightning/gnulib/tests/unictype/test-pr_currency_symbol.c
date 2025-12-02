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

    { 0x0024, 0x0024 },
    { 0x00A2, 0x00A5 },
    { 0x058F, 0x058F },
    { 0x060B, 0x060B },
    { 0x09F2, 0x09F3 },
    { 0x09FB, 0x09FB },
    { 0x0AF1, 0x0AF1 },
    { 0x0BF9, 0x0BF9 },
    { 0x0E3F, 0x0E3F },
    { 0x17DB, 0x17DB },
    { 0x20A0, 0x20BE },
    { 0xA838, 0xA838 },
    { 0xFDFC, 0xFDFC },
    { 0xFE69, 0xFE69 },
    { 0xFF04, 0xFF04 },
    { 0xFFE0, 0xFFE1 },
    { 0xFFE5, 0xFFE6 }

#define PREDICATE(c) uc_is_property_currency_symbol (c)
#include "test-predicate-part2.h"
