/* Test of timespec_get() function.
   Copyright (C) 2021 Free Software Foundation, Inc.

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

#include <time.h>

#include "signature.h"
SIGNATURE_CHECK (timespec_get, int, (struct timespec *ts, int base));

#include "macros.h"

int
main (void)
{
  struct timespec ts1;
  time_t tt2;
  struct timespec ts3;
  time_t tt4;

  ASSERT (timespec_get (&ts1, TIME_UTC));
  tt2 = time (NULL);
  ASSERT (timespec_get (&ts3, TIME_UTC));
  tt4 = time (NULL);

  /* Verify monotonicity of timespec_get().  */
  ASSERT (ts1.tv_sec < ts3.tv_sec
          || (ts1.tv_sec == ts3.tv_sec && ts1.tv_nsec <= ts3.tv_nsec));

  /* Verify monotonicity of time().  */
  ASSERT (tt2 <= tt4);

  /* Verify that the tv_sec field of the result is the same as time(NULL).  */
  ASSERT (ts1.tv_sec <= tt2);
  ASSERT (tt2 <= ts3.tv_sec);
  ASSERT (ts3.tv_sec <= tt4);

  return 0;
}
