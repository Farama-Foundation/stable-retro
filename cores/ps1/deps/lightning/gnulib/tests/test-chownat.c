/* Tests of chownat and lchownat
   Copyright 2020-2021 Free Software Foundation, Inc.

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

#include <config.h>

#include "openat.h"

#include "signature.h"
SIGNATURE_CHECK (chownat, int, (int, char const *, uid_t, gid_t));
SIGNATURE_CHECK (lchownat, int, (int, char const *, uid_t, gid_t));

#define BASE "test-chownat.t"
#define TEST_CHOWNAT
#include "test-fchownat.c"
