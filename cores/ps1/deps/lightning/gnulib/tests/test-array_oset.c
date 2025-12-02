/* Test of ordered set data type implementation.
   Copyright (C) 2006-2021 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2007.

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

#include "gl_array_oset.h"

#include <stdlib.h>
#include <string.h>

#include "gl_xlist.h"
#include "gl_array_list.h"
#include "macros.h"

#include "test-oset-update.h"

static const char *objects[30] =
  {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "<", ">", "[", "]"
  };

#define RANDOM(n) (rand () % (n))
#define RANDOM_OBJECT() objects[RANDOM (SIZEOF (objects))]

static void
check_equals (gl_oset_t set1, gl_list_t set2)
{
  size_t n = gl_oset_size (set1);
  gl_oset_iterator_t iter1;
  gl_list_iterator_t iter2;
  const void *elt1;
  const void *elt2;
  gl_list_node_t node2;
  size_t i;

  iter1 = gl_oset_iterator (set1);
  iter2 = gl_list_iterator (set2);
  for (i = 0; i < n; i++)
    {
      ASSERT (gl_oset_iterator_next (&iter1, &elt1));
      ASSERT (gl_list_iterator_next (&iter2, &elt2, &node2));
      ASSERT (elt1 == elt2);
    }
  ASSERT (!gl_oset_iterator_next (&iter1, &elt1));
  ASSERT (!gl_list_iterator_next (&iter2, &elt2, &node2));
  gl_oset_iterator_free (&iter1);
  gl_list_iterator_free (&iter2);
}

static void
check_all (gl_oset_t set1, gl_list_t set2)
{
  check_equals (set1, set2);
}

static bool
is_at_least (const void *elt, const void *threshold)
{
  return strcmp ((const char *) elt, (const char *) threshold) >= 0;
}

static size_t
gl_sortedlist_indexof_atleast (gl_list_t set,
                               gl_setelement_threshold_fn threshold_fn,
                               const void *threshold)
{
  /* This implementation is slow, but easy to verify.  */
  size_t count = gl_list_size (set);
  size_t index;

  for (index = 0; index < count; index++)
    if (threshold_fn (gl_list_get_at (set, index), threshold))
      return index;
  return (size_t)(-1);
}

int
main (int argc, char *argv[])
{
  gl_oset_t set1;
  gl_list_t set2;

  /* Allow the user to provide a non-default random seed on the command line.  */
  if (argc > 1)
    srand (atoi (argv[1]));

  {
    size_t initial_size = RANDOM (20);
    size_t i;
    unsigned int repeat;

    /* Create set1.  */
    set1 = gl_oset_nx_create_empty (GL_ARRAY_OSET, (gl_setelement_compar_fn) strcmp, NULL);
    ASSERT (set1 != NULL);

    /* Create set2.  */
    set2 = gl_list_create_empty (GL_ARRAY_LIST, NULL, NULL, NULL, false);

    check_all (set1, set2);

    /* Initialize them.  */
    for (i = 0; i < initial_size; i++)
      {
        const char *obj = RANDOM_OBJECT ();
        ASSERT (gl_oset_nx_add (set1, obj)
                == (gl_sortedlist_search (set2, (gl_listelement_compar_fn)strcmp, obj) != NULL
                    ? false
                    : (gl_sortedlist_add (set2, (gl_listelement_compar_fn)strcmp, obj), true)));
        check_all (set1, set2);
      }

    for (repeat = 0; repeat < 100000; repeat++)
      {
        unsigned int operation = RANDOM (4);
        switch (operation)
          {
          case 0:
            {
              const char *obj = RANDOM_OBJECT ();
              ASSERT (gl_oset_search (set1, obj)
                      == (gl_sortedlist_search (set2, (gl_listelement_compar_fn)strcmp, obj) != NULL));
            }
            break;
          case 1:
            {
              const char *obj = RANDOM_OBJECT ();
              ASSERT (gl_oset_nx_add (set1, obj)
                      == (gl_sortedlist_search (set2, (gl_listelement_compar_fn)strcmp, obj) != NULL
                          ? false
                          : (gl_sortedlist_add (set2, (gl_listelement_compar_fn)strcmp, obj), true)));
            }
            break;
          case 2:
            {
              const char *obj = RANDOM_OBJECT ();
              ASSERT (gl_oset_remove (set1, obj)
                      == gl_sortedlist_remove (set2, (gl_listelement_compar_fn)strcmp, obj));
            }
            break;
          case 3:
            {
              const char *obj = RANDOM_OBJECT ();
              gl_oset_iterator_t iter = gl_oset_iterator_atleast (set1, is_at_least, obj);
              size_t index = gl_sortedlist_indexof_atleast (set2, is_at_least, obj);
              const void *elt;
              /* Check the first two values that the iterator produces.
                 Checking them all would make this part of the test dominate the
                 run time of the test.  */
              if (index == (size_t)(-1))
                ASSERT (!gl_oset_iterator_next (&iter, &elt));
              else
                {
                  ASSERT (gl_oset_iterator_next (&iter, &elt));
                  ASSERT (elt == gl_list_get_at (set2, index));
                  if (index + 1 == gl_list_size (set2))
                    ASSERT (!gl_oset_iterator_next (&iter, &elt));
                  else
                    {
                      ASSERT (gl_oset_iterator_next (&iter, &elt));
                      ASSERT (elt == gl_list_get_at (set2, index + 1));
                    }
                }
              gl_oset_iterator_free (&iter);
            }
            break;
          }
        check_all (set1, set2);
      }

    gl_oset_free (set1);
    gl_list_free (set2);
  }

  test_update (GL_ARRAY_OSET);

  return 0;
}
