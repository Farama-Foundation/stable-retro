/* Test of getter for RLIMIT_DATA.
   Copyright (C) 2011-2021 Free Software Foundation, Inc.

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

#include "resource-ext.h"

#include <stdio.h>
#include <stdlib.h>

#include "macros.h"

void *memchunk1;
void *memchunk2;

int
main ()
{
  uintptr_t value1, value2, value3;

  value1 = get_rusage_data ();

  memchunk1 = malloc (0x88);

  value2 = get_rusage_data ();

  memchunk2 = malloc (0x281237);

  value3 = get_rusage_data ();

  if (value1 == 0 && value2 == 0 && value3 == 0)
    {
      fprintf (stderr, "Skipping test: no way to determine data segment size\n");
      return 77;
    }
  else
    {
      /* The data segment size is positive, except possibly at the beginning.  */
      ASSERT (value2 > 0);

      /* Allocating memory should not decrease the data segment size.  */
      ASSERT (value2 >= value1);
      ASSERT (value3 >= value2);

#if !(__GLIBC__ == 2 || MUSL_LIBC || (defined __APPLE__ && defined __MACH__) || defined __FreeBSD__ || defined __DragonFly__ || defined __OpenBSD__ || defined _WIN32 || defined __CYGWIN__)
      /* Allocating 2.5 MB of memory should increase the data segment size.  */
      ASSERT (value3 > value1);
#endif

      return 0;
    }
}
