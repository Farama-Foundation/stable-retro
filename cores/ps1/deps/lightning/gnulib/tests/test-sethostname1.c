/*
 * Copyright (C) 2011-2021 Free Software Foundation, Inc.
 * Written by Ben Walton.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (sethostname, int, (const char *, size_t));

int do_dangerous_things;

int
main ()
{
  /* Some code that has a link-time dependency to the sethostname() function
     and that is likely not optimized away by compilers.  */
  if (do_dangerous_things)
    {
      /* Never executed.  */
      int ret = sethostname ("oprah", 5);
      (void) ret;
    }

  return 0;
}
