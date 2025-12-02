/* Test of <sysexits.h> substitute.
   Copyright (C) 2007, 2009-2021 Free Software Foundation, Inc.

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

#include <sysexits.h>

int exitcode;

int
main ()
{
  /* Check that all EX_* symbols are defined to integer constant expressions
     with mutually different values.  */
  switch (exitcode)
    {
    case EX_OK:
    case EX_USAGE:
    case EX_DATAERR:
    case EX_NOINPUT:
    case EX_NOUSER:
    case EX_NOHOST:
    case EX_UNAVAILABLE:
    case EX_SOFTWARE:
    case EX_OSERR:
    case EX_OSFILE:
    case EX_CANTCREAT:
    case EX_IOERR:
    case EX_TEMPFAIL:
    case EX_PROTOCOL:
    case EX_NOPERM:
    case EX_CONFIG:
      break;
    }

  return 0;
}
