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

    { 0x002B, 0x002B },
    { 0x002D, 0x002D },
    { 0x207A, 0x207B },
    { 0x208A, 0x208B },
    { 0x2212, 0x2212 },
    { 0xFB29, 0xFB29 },
    { 0xFE62, 0xFE63 },
    { 0xFF0B, 0xFF0B },
    { 0xFF0D, 0xFF0D }

#define PREDICATE(c) uc_is_property_bidi_eur_num_separator (c)
#include "test-predicate-part2.h"
