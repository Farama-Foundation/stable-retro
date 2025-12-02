/* Test of u8_strstr() function.
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

/* Written by Pádraig Brady <P@draigBrady.com>, 2011.  */

#include <config.h>

#include "unistr.h"

#include <stdint.h>
#include <stdlib.h>
#include <signal.h> /* For signal.  */
#include <unistd.h> /* For alarm.  */

#include "macros.h"

#define UNIT uint8_t
#define U_STRSTR u8_strstr
#include "test-u-strstr.h"

int
main (void)
{
#if HAVE_DECL_ALARM
  /* Declare failure if test takes too long, by using default abort
     caused by SIGALRM.  Note since we defer to strstr() in this
     case, we're assuming that we're running this test on the
     same system that we did the check to ensure it has linear
     performance characteristics.  */
  signal (SIGALRM, SIG_DFL);
  alarm (5);
#endif

  test_u_strstr ();

  return 0;
}
