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

    { 0x00B7, 0x00B7 },
    { 0x02D0, 0x02D1 },
    { 0x0640, 0x0640 },
    { 0x07FA, 0x07FA },
    { 0x0E46, 0x0E46 },
    { 0x0EC6, 0x0EC6 },
    { 0x180A, 0x180A },
    { 0x1843, 0x1843 },
    { 0x1AA7, 0x1AA7 },
    { 0x1C36, 0x1C36 },
    { 0x1C7B, 0x1C7B },
    { 0x3005, 0x3005 },
    { 0x3031, 0x3035 },
    { 0x309D, 0x309E },
    { 0x30FC, 0x30FE },
    { 0xA015, 0xA015 },
    { 0xA60C, 0xA60C },
    { 0xA9CF, 0xA9CF },
    { 0xA9E6, 0xA9E6 },
    { 0xAA70, 0xAA70 },
    { 0xAADD, 0xAADD },
    { 0xAAF3, 0xAAF4 },
    { 0xFF70, 0xFF70 },
    { 0x1135D, 0x1135D },
    { 0x115C6, 0x115C8 },
    { 0x16B42, 0x16B43 },
    { 0x16FE0, 0x16FE0 },
    { 0x1E944, 0x1E946 }

#define PREDICATE(c) uc_is_property_extender (c)
#include "test-predicate-part2.h"
