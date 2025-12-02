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

    { 0x0E40, 0x0E44 },
    { 0x0EC0, 0x0EC4 },
    { 0x19B5, 0x19B7 },
    { 0x19BA, 0x19BA },
    { 0xAAB5, 0xAAB6 },
    { 0xAAB9, 0xAAB9 },
    { 0xAABB, 0xAABC }

#define PREDICATE(c) uc_is_property_logical_order_exception (c)
#include "test-predicate-part2.h"
