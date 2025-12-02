/* Test of u8_chr() function.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include "unistr.h"

#include <stdlib.h>
#include <string.h>

#include "zerosize-ptr.h"
#include "macros.h"

#define UNIT uint8_t
#define U_UCTOMB u8_uctomb
#define U32_TO_U u32_to_u8
#define U_CHR u8_chr
#define U_SET u8_set
#include "test-chr.h"
