/*
 * Copyright (C) 2008-2021 Free Software Foundation, Inc.
 * Written by Simon Josefsson.
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

/* Get getdomainname() declaration.  */
#include <unistd.h>

#include "signature.h"
SIGNATURE_CHECK (getdomainname, int, (char *, size_t));

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define YPMAXDOMAIN 64

#define NODOMAINNAME "magic-gnulib-test-string"

int
main (int argc, char *argv[] _GL_UNUSED)
{
  char buf[YPMAXDOMAIN];
  int rc;

  if (strlen (NODOMAINNAME) >= YPMAXDOMAIN)
    {
      printf ("YPMAXDOMAIN impossibly small?! %d\n", YPMAXDOMAIN);
      return 2;
    }

  strcpy (buf, NODOMAINNAME);

  rc = getdomainname (buf, sizeof (buf));

  if (rc != 0)
    {
      printf ("getdomainname failed, rc %d errno %d\n", rc, errno);
      return 1;
    }

  if (strcmp (buf, NODOMAINNAME) == 0)
    {
      printf ("getdomainname left buffer untouched.\n");
      return 1;
    }

  if (argc > 1)
    printf ("domainname: %s\n", buf);

  return 0;
}
