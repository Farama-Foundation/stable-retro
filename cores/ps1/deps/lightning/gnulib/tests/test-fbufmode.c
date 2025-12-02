/* Test of fbufmode() function.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include "fbufmode.h"

#include <stdio.h>

#include "macros.h"

#define TESTFILE "t-fbufmode.tmp"

/* ISO C99 disallows more than one setvbuf call on a given stream,
   and HP-UX 11 and musl libc indeed don't support such use of setvbuf.
   Therefore allocate a new stream for each possible mode value.  */
static int
test_mode (int mode)
{
  FILE *fp;
  char buf[5];

  /* Open it for reading.  */
  fp = fopen (TESTFILE, "r");

  switch (mode)
    {
    case _IONBF:
      ASSERT (setvbuf (fp, NULL, _IONBF, 0) == 0);
      ASSERT (fbufmode (fp) == _IONBF);
      break;

    case _IOLBF:
      ASSERT (setvbuf (fp, buf, _IOLBF, 5) == 0);
      /* mingw's setvbuf implements _IOLBF the same way as _IOFBF.  */
      ASSERT (fbufmode (fp) == _IOLBF
              || fbufmode (fp) == _IOFBF);
      break;

    case _IOFBF:
      ASSERT (setvbuf (fp, buf, _IOFBF, 5) == 0);
      ASSERT (fbufmode (fp) == _IOFBF);
      break;

    default:
      break;
    }

  fclose (fp);

  return 0;
}

int
main ()
{
  int ret;

  /* Create a file with some contents.  */
  {
    FILE *fp;

    fp = fopen (TESTFILE, "w");
    if (fp == NULL)
      goto skip;
    if (fwrite ("foobarsh", 1, 8, fp) < 8)
      goto skip;
    if (fclose (fp))
      goto skip;
  }

  ret = test_mode (_IONBF);
  if (ret != 0)
    goto fail;

  ret = test_mode (_IOLBF);
  if (ret != 0)
    goto fail;

  ret = test_mode (_IOFBF);
  if (ret != 0)
    goto fail;

  return 0;

 fail:
  return ret;

 skip:
  fprintf (stderr, "Skipping test: file operations failed.\n");
  return 77;
}
