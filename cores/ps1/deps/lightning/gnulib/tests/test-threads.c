/* Test of <threads.h> substitute.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2019.  */

#include <config.h>

#include <threads.h>

/* Don't check that thread_local is defined.
   We cannot define it properly on some platforms.  */

/* Check that ONCE_FLAG_INIT is defined.  */
#ifndef ONCE_FLAG_INIT
"oops1"
#endif
once_flag a = ONCE_FLAG_INIT;

/* Check that TSS_DTOR_ITERATIONS is defined.  */
#ifndef TSS_DTOR_ITERATIONS
"oops2"
#endif
int b = TSS_DTOR_ITERATIONS;

int
main (void)
{
  /* Ensure no overlap in thrd_*. */
  switch (0)
    {
    case thrd_success:
    case thrd_timedout:
    case thrd_busy:
    case thrd_nomem:
    case thrd_error:
      ;
    }

  /* Ensure no overlap among valid types for mtx_init. */
  switch (0)
    {
    case mtx_plain:
    case mtx_timed:
    case mtx_plain | mtx_recursive:
    case mtx_timed | mtx_recursive:
      ;
    }

  return 0;
}
