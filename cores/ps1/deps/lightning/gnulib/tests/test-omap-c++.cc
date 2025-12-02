/* Test of ordered map data type implementation as a C++ class.
   Copyright (C) 2020-2021 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2020.

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

#include <config.h>

#include "gl_omap.hh"
#include "gl_array_omap.h"

#include <string.h>

#include "macros.h"

static const int integers[6] = { 0, 1, 2, 3, 4, 5 };

int
main (int argc, char *argv[])
{
  gl_OMap<const char *, const int *> map1;

  map1 = gl_OMap<const char *, const int *> (GL_ARRAY_OMAP, strcmp, NULL, NULL);
  map1.put ("five",  integers);
  map1.put ("one",   integers + 1);
  map1.put ("two",   integers + 2);
  map1.put ("three", integers + 3);
  map1.put ("four",  integers + 4);
  map1.put ("five",  integers + 5);
  ASSERT (map1.size () == 5);

  ASSERT (map1.get ("two")[0] == 2);

  gl_OMap<const char *, const int *>::iterator iter1 = map1.begin ();
  const char *key;
  const int *val;
  ASSERT (iter1.next (key, val));
  ASSERT (strcmp (key, "five") == 0);
  ASSERT (*val == 5);
  ASSERT (iter1.next (key, val));
  ASSERT (strcmp (key, "four") == 0);
  ASSERT (*val == 4);
  ASSERT (iter1.next (key, val));
  ASSERT (strcmp (key, "one") == 0);
  ASSERT (*val == 1);
  ASSERT (iter1.next (key, val));
  ASSERT (strcmp (key, "three") == 0);
  ASSERT (*val == 3);
  ASSERT (iter1.next (key, val));
  ASSERT (strcmp (key, "two") == 0);
  ASSERT (*val == 2);
  ASSERT (!iter1.next (key, val));

  map1.free ();

  return 0;
}
