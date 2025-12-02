/* Test of ordered map data type implementation.
   Copyright (C) 2006-2021 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2018.

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

#include "gl_avltree_omap.h"

#include <stdlib.h>
#include <string.h>

#include "gl_array_omap.h"
#include "macros.h"

extern void gl_avltree_omap_check_invariants (gl_omap_t set);

static const char *objects[30] =
  {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "<", ">", "[", "]"
  };

#define RANDOM(n) (rand () % (n))
#define RANDOM_OBJECT() objects[RANDOM (SIZEOF (objects))]

static void
check_equals (gl_omap_t map1, gl_omap_t map2)
{
  size_t n = gl_omap_size (map1);
  gl_omap_iterator_t iter1, iter2;
  const void *key1;
  const void *value1;
  const void *key2;
  const void *value2;
  size_t i;

  iter1 = gl_omap_iterator (map1);
  iter2 = gl_omap_iterator (map2);
  for (i = 0; i < n; i++)
    {
      ASSERT (gl_omap_iterator_next (&iter1, &key1, &value1));
      ASSERT (gl_omap_iterator_next (&iter2, &key2, &value2));
      ASSERT (key1 == key2);
      ASSERT (value1 == value2);
    }
  ASSERT (!gl_omap_iterator_next (&iter1, &key1, &value1));
  ASSERT (!gl_omap_iterator_next (&iter2, &key2, &value2));
  gl_omap_iterator_free (&iter1);
  gl_omap_iterator_free (&iter2);
}

static void
check_all (gl_omap_t map1, gl_omap_t map2)
{
  gl_avltree_omap_check_invariants (map2);
  check_equals (map1, map2);
}

int
main (int argc, char *argv[])
{
  gl_omap_t map1, map2;

  /* Allow the user to provide a non-default random seed on the command line.  */
  if (argc > 1)
    srand (atoi (argv[1]));

  {
    size_t initial_size = RANDOM (20);
    size_t i;
    unsigned int repeat;

    /* Create map1.  */
    map1 = gl_omap_nx_create_empty (GL_ARRAY_OMAP, (gl_mapkey_compar_fn) strcmp, NULL, NULL);
    ASSERT (map1 != NULL);

    /* Create map2.  */
    map2 = gl_omap_nx_create_empty (GL_AVLTREE_OMAP, (gl_mapkey_compar_fn) strcmp, NULL, NULL);
    ASSERT (map2 != NULL);

    check_all (map1, map2);

    /* Initialize them.  */
    for (i = 0; i < initial_size; i++)
      {
        const char *key = RANDOM_OBJECT ();
        const char *value = RANDOM_OBJECT ();
        ASSERT (gl_omap_nx_put (map1, key, value) == gl_omap_nx_put (map2, key, value));
        check_all (map1, map2);
      }

    for (repeat = 0; repeat < 100000; repeat++)
      {
        unsigned int operation = RANDOM (3);
        switch (operation)
          {
          case 0:
            {
              const char *key = RANDOM_OBJECT ();
              ASSERT (gl_omap_get (map1, key) == gl_omap_get (map2, key));
            }
            break;
          case 1:
            {
              const char *key = RANDOM_OBJECT ();
              const char *value = RANDOM_OBJECT ();
              ASSERT (gl_omap_nx_put (map1, key, value) == gl_omap_nx_put (map2, key, value));
            }
            break;
          case 2:
            {
              const char *key = RANDOM_OBJECT ();
              ASSERT (gl_omap_remove (map1, key) == gl_omap_remove (map2, key));
            }
            break;
          }
        check_all (map1, map2);
      }

    gl_omap_free (map1);
    gl_omap_free (map2);
  }

  return 0;
}
