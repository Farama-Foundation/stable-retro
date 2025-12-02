/* Test of list data type implementation as a C++ class.
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

#include "gl_list.hh"
#include "gl_array_list.h"

#include <string.h>

#include "macros.h"

static const char A[] = "A";
static const char C[] = "C";
static const char D[] = "D";

int
main (int argc, char *argv[])
{
  gl_List<const char *> list1;

  list1 = gl_List<const char *> (GL_ARRAY_LIST, NULL, NULL, NULL, true);
  list1.add_last (A);
  list1.add_last (C);
  list1.add_last (D);
  list1.add_last (C);
  ASSERT (list1.size () == 4);

  gl_List<const char *>::iterator iter1 = list1.begin ();
  {
    const char *elt;
    ASSERT (iter1.next (elt));
    ASSERT (strcmp (elt, "A") == 0);
    ASSERT (iter1.next (elt));
    ASSERT (strcmp (elt, "C") == 0);
    ASSERT (iter1.next (elt));
    ASSERT (strcmp (elt, "D") == 0);
    ASSERT (iter1.next (elt));
    ASSERT (strcmp (elt, "C") == 0);
    ASSERT (!iter1.next (elt));
  }

  gl_List<const char *> list2 = gl_List<const char *> (list1, 1, 3);

  gl_List<const char *>::iterator iter2 = list2.begin ();
  {
    const char *elt;
    ASSERT (iter2.next (elt));
    ASSERT (strcmp (elt, "C") == 0);
    ASSERT (iter2.next (elt));
    ASSERT (strcmp (elt, "D") == 0);
    ASSERT (!iter1.next (elt));
  }

  ASSERT (list2.indexof (A) == (size_t)(-1));
  ASSERT (list2.indexof (D) == 1);

  ASSERT (list2.sortedlist_indexof (strcmp, "A") == (size_t)(-1));
  ASSERT (list2.sortedlist_indexof (strcmp, "D") == 1);

  list2.free ();
  list1.free ();

  return 0;
}
