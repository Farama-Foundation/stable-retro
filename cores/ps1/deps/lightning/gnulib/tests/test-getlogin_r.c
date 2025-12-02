/* Test of getting user name.
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

/* Written by Bruno Haible and Paul Eggert.  */

#include <config.h>

#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (getlogin_r, int, (char *, size_t));

#include "test-getlogin.h"

int
main (void)
{
  /* Test value.  */
  /* Test with a large enough buffer.  */
  char buf[1024];
  int err = getlogin_r (buf, sizeof buf);
#if defined __sun
  if (err == EINVAL)
    {
      /* This can happen on Solaris 11 OpenIndiana in the MATE desktop.  */
      fprintf (stderr, "Skipping test: no entry in /var/adm/utmpx.\n");
      exit (77);
    }
#endif
  test_getlogin_result (buf, err);

  /* Test with a small buffer.  */
  {
    char smallbuf[1024];
    size_t n = strlen (buf);
    size_t i;

    for (i = 0; i <= n; i++)
      {
        err = getlogin_r (smallbuf, i);
        if (i == 0)
          ASSERT (err == ERANGE || err == EINVAL);
        else
          ASSERT (err == ERANGE);
      }
  }

  /* Test with a huge buffer.  */
  {
    static char hugebuf[70000];

    ASSERT (getlogin_r (hugebuf, sizeof (hugebuf)) == 0);
    ASSERT (strcmp (hugebuf, buf) == 0);
  }

  return 0;
}
