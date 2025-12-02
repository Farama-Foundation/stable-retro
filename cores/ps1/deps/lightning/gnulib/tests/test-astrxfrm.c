/* Test of astrxfrm() function.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include "astrxfrm.h"

#include <locale.h>
#include <string.h>

#include "macros.h"

int
main ()
{
  const char input[] = "B\303\266se B\303\274bchen"; /* "Böse Bübchen" */
  char *expected_transform;

  setlocale (LC_ALL, "de_DE.UTF-8");

  /* Test resultbuf = NULL.  */
  {
    size_t length = 0;
    char *transform = astrxfrm (input, NULL, &length);
    ASSERT (transform != NULL);
    ASSERT (length == strlen (transform) + 1);
    expected_transform = transform;
  }

  /* Test resultbuf = NULL.  Initial value of length is ignored here.  */
  {
    size_t length = 100;
    char *transform = astrxfrm (input, NULL, &length);
    ASSERT (transform != NULL);
    ASSERT (strcmp (transform, expected_transform) == 0);
    ASSERT (length == strlen (transform) + 1);
    free (transform);
  }

  /* Test resultbuf != NULL with an initial length = 0.  */
  {
    char buf[1];
    size_t length = 0;
    char *transform;
    buf[0] = '@';
    transform = astrxfrm (input, buf, &length);
    ASSERT (transform != NULL);
    ASSERT (transform != buf);
    ASSERT (strcmp (transform, expected_transform) == 0);
    ASSERT (length == strlen (transform) + 1);
    ASSERT (buf[0] == '@');
    free (transform);
  }

  /* Test resultbuf != NULL with an initial length that is too small.  */
  {
    char buf[3];
    size_t length = sizeof (buf) - 1;
    char *transform;
    buf[sizeof (buf) - 1] = '@';
    transform = astrxfrm (input, buf, &length);
    ASSERT (transform != NULL);
    ASSERT (transform != buf);
    ASSERT (strcmp (transform, expected_transform) == 0);
    ASSERT (length == strlen (transform) + 1);
    ASSERT (buf[sizeof (buf) - 1] == '@');
    free (transform);
  }

  /* Test resultbuf != NULL with an initial length that is large enough.  */
  {
    char buf[200];
    size_t length = sizeof (buf) - 1;
    char *transform;
    buf[sizeof (buf) - 1] = '@';
    transform = astrxfrm (input, buf, &length);
    ASSERT (transform != NULL);
    ASSERT (transform == buf);
    ASSERT (strcmp (transform, expected_transform) == 0);
    ASSERT (length == strlen (transform) + 1);
    ASSERT (buf[sizeof (buf) - 1] == '@');
  }

  free (expected_transform);

  return 0;
}
