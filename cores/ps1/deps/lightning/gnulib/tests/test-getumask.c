/* Test of retrieving the umask of the process.
   Copyright (C) 2020-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible.  */

#include <config.h>

#include <sys/stat.h>

#include "signature.h"
SIGNATURE_CHECK (getumask, mode_t, (void));

#include "macros.h"

int
main (void)
{
  int mask;

  mask = getumask ();
  ASSERT (mask >= 0);

#if !ASSUME_UMASK_CONSTANT
  /* These tests fail if the umask is required to not change.  */

  umask (002);

  mask = getumask ();
  ASSERT (mask == 002);

  umask (077);

  mask = getumask ();
  ASSERT (mask == 077);

#endif

  return 0;
}
