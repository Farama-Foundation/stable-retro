/* Test of glob/globfree functions.
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

#include "signature.h"
SIGNATURE_CHECK (glob, int, (char const *, int, int (*) (char const *, int),
                             glob_t *));
SIGNATURE_CHECK (globfree, void, (glob_t *));

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

#define BASE "test-glob.t"
#define GL_NO_SUCH_FILE "/gnulib-magic-does-not-exist"

int
main ()
{
  int res;
  glob_t g;

  res = glob (".", 0, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc == 1);
  globfree (&g);

  res = glob (".", GLOB_NOSORT, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc == 1);
  globfree (&g);

  res = glob (".", GLOB_MARK, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc == 1);

  res = glob (".", GLOB_APPEND, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc == 2);
  globfree (&g);

  res = glob (".", GLOB_NOESCAPE, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc == 1);
  globfree (&g);

  res = glob ("./*", 0, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc >= 1);
  globfree (&g);

  res = glob (GL_NO_SUCH_FILE, 0, NULL, &g);
  ASSERT (res == GLOB_NOMATCH);

  res = glob (GL_NO_SUCH_FILE, GLOB_NOCHECK, NULL, &g);
  ASSERT (res == 0 && g.gl_pathc == 1);
  ASSERT (strcmp (g.gl_pathv[0], GL_NO_SUCH_FILE) == 0);
  globfree (&g);

  if ((symlink (GL_NO_SUCH_FILE, BASE "globlink1") == 0 || errno == EEXIST)
      && (symlink (".", BASE "globlink2") == 0 || errno == EEXIST))
    {
      res = glob (BASE "globlink[12]", 0, NULL, &g);
      ASSERT (res == 0 && g.gl_pathc == 2);
      ASSERT (strcmp (g.gl_pathv[0], BASE "globlink1") == 0);
      ASSERT (strcmp (g.gl_pathv[1], BASE "globlink2") == 0);
      globfree (&g);

      res = glob (BASE "globlink[12]", GLOB_MARK, NULL, &g);
      ASSERT (res == 0 && g.gl_pathc == 2);
      ASSERT (strcmp (g.gl_pathv[0], BASE "globlink1") == 0);
      ASSERT (strcmp (g.gl_pathv[1], BASE "globlink2/") == 0);
      globfree (&g);
    }

  return 0;
}
