/* Test the getentropy function.
   Copyright 2020-2021 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#include <config.h>

#include <unistd.h>

#include <string.h>

#include "signature.h"
SIGNATURE_CHECK (getentropy, int, (void *, size_t));

#include "macros.h"

char empty_buf[256];
char buf[256];

int
main (int argc, char *argv[])
{
  /* If this test fails, there's something wrong with the setup anyway.  */
  ASSERT (getentropy (buf, sizeof buf) == 0);

  /* This test fails with probability 2**-2048.  (Run it again if so. :-) */
  ASSERT (memcmp (buf, empty_buf, sizeof buf) != 0);

  /* It is very unlikely that two calls to getentropy produce the same
     results.  */
  {
    char buf1[8];
    char buf2[8];

    ASSERT (getentropy (buf1, sizeof (buf1)) == 0);
    ASSERT (getentropy (buf2, sizeof (buf2)) == 0);
    ASSERT (memcmp (buf1, buf2, sizeof (buf1)) != 0);
  }

  return 0;
}
