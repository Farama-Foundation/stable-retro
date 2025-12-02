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

    { 0x005E, 0x005E },
    { 0x0060, 0x0060 },
    { 0x00A8, 0x00A8 },
    { 0x00AF, 0x00AF },
    { 0x00B4, 0x00B4 },
    { 0x00B8, 0x00B8 },
    { 0x02C2, 0x02C5 },
    { 0x02D2, 0x02DF },
    { 0x02E5, 0x02EB },
    { 0x02ED, 0x02ED },
    { 0x02EF, 0x02FF },
    { 0x0375, 0x0375 },
    { 0x0384, 0x0385 },
    { 0x1FBD, 0x1FBD },
    { 0x1FBF, 0x1FC1 },
    { 0x1FCD, 0x1FCF },
    { 0x1FDD, 0x1FDF },
    { 0x1FED, 0x1FEF },
    { 0x1FFD, 0x1FFE },
    { 0x309B, 0x309C },
    { 0xA700, 0xA716 },
    { 0xA720, 0xA721 },
    { 0xA789, 0xA78A },
    { 0xAB5B, 0xAB5B },
    { 0xFBB2, 0xFBC1 },
    { 0xFF3E, 0xFF3E },
    { 0xFF40, 0xFF40 },
    { 0xFFE3, 0xFFE3 },
    { 0x1F3FB, 0x1F3FF }

#define PREDICATE(c) uc_is_general_category (c, UC_CATEGORY_Sk)
#include "test-predicate-part2.h"
