/* Test of <glob.h> substitute.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Simon Josefsson <simon@josefsson.org>, 2009.  */

#include <config.h>

#include <glob.h>

#include "macros.h"

/* Check that size_t is defined.  */
size_t a;

/* Check that the various GLOB_* macros are defined.  */
int flags[] =
  {
    GLOB_APPEND,
    GLOB_DOOFFS,
    GLOB_ERR,
    GLOB_MARK,
    GLOB_NOCHECK,
    GLOB_NOESCAPE,
    GLOB_NOSORT
  };
int ret[] =
  {
    GLOB_ABORTED,
    GLOB_NOMATCH,
    GLOB_NOSPACE
  };

int
main ()
{
  glob_t g;

  /* Make sure glob_t struct members exists. */
  ASSERT (sizeof (g.gl_pathc));
  ASSERT (sizeof (g.gl_pathv));
  ASSERT (sizeof (g.gl_offs));

  return 0;
}
