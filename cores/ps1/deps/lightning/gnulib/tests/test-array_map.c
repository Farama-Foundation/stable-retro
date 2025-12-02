/* Test of map data type implementation.
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

#include "gl_array_map.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "gl_xlist.h"
#include "gl_array_list.h"
#include "macros.h"

static const char *objects[30] =
  {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "<", ">", "[", "]"
  };

#define SIZE_BITS (sizeof (size_t) * CHAR_BIT)

static bool
string_equals (const void *x1, const void *x2)
{
  const char *s1 = x1;
  const char *s2 = x2;
  return strcmp (s1, s2) == 0;
}

/* A hash function for NUL-terminated char* strings using
   the method described by Bruno Haible.
   See https://www.haible.de/bruno/hashfunc.html.  */
static size_t
string_hash (const void *x)
{
  const char *s = x;
  size_t h = 0;

  for (; *s; s++)
    h = *s + ((h << 9) | (h >> (SIZE_BITS - 9)));

  return h;
}

#define RANDOM(n) (rand () % (n))
#define RANDOM_OBJECT() objects[RANDOM (SIZEOF (objects))]

struct pair
{
  const void *key;
  const void *value;
};

static int
cmp_pairs_in_array (const void *pairptr1, const void *pairptr2)
{
  const void *key1 = ((struct pair const *)pairptr1)->key;
  const void *key2 = ((struct pair const *)pairptr2)->key;
  return strcmp ((const char *) key1, (const char *) key2);
}

static void
check_equals (gl_map_t map1, gl_list_t keys, gl_list_t values)
{
  size_t n = gl_map_size (map1);
  struct pair *pairs_of_map1 = XNMALLOC (n, struct pair);

  gl_map_iterator_t iter1;
  const void *key1;
  const void *value1;
  size_t i;

  ASSERT (gl_list_size (keys) == n);
  ASSERT (gl_list_size (values) == n);
  iter1 = gl_map_iterator (map1);
  for (i = 0; i < n; i++)
    {
      ASSERT (gl_map_iterator_next (&iter1, &key1, &value1));
      pairs_of_map1[i].key = key1;
      pairs_of_map1[i].value = value1;
    }
  ASSERT (!gl_map_iterator_next (&iter1, &key1, &value1));
  gl_map_iterator_free (&iter1);

  if (n > 0)
    qsort (pairs_of_map1, n, sizeof (struct pair), cmp_pairs_in_array);
  for (i = 0; i < n; i++)
    {
      ASSERT (pairs_of_map1[i].key == gl_list_get_at (keys, i));
      ASSERT (pairs_of_map1[i].value == gl_list_get_at (values, i));
    }
  free (pairs_of_map1);
}

static void
check_all (gl_map_t map1, gl_list_t keys, gl_list_t values)
{
  check_equals (map1, keys, values);
}

int
main (int argc, char *argv[])
{
  gl_map_t map1;
  gl_list_t keys;
  gl_list_t values;

  /* Allow the user to provide a non-default random seed on the command line.  */
  if (argc > 1)
    srand (atoi (argv[1]));

  {
    size_t initial_size = RANDOM (20);
    size_t i;
    unsigned int repeat;

    /* Create map1.  */
    map1 = gl_map_nx_create_empty (GL_ARRAY_MAP,
                                   string_equals, string_hash, NULL, NULL);
    ASSERT (map1 != NULL);

    /* Create keys and values.  */
    keys = gl_list_create_empty (GL_ARRAY_LIST, NULL, NULL, NULL, false);
    values = gl_list_create_empty (GL_ARRAY_LIST, NULL, NULL, NULL, false);

    check_all (map1, keys, values);

    /* Initialize them.  */
    for (i = 0; i < initial_size; i++)
      {
        const char *key = RANDOM_OBJECT ();
        const char *value = RANDOM_OBJECT ();
        bool added = gl_map_nx_put (map1, key, value);
        size_t index = gl_sortedlist_indexof (keys, (gl_listelement_compar_fn)strcmp, key);
        ASSERT (added == (index == (size_t)(-1)));
        if (added)
          {
            gl_sortedlist_add (keys, (gl_listelement_compar_fn)strcmp, key);
            index = gl_sortedlist_indexof (keys, (gl_listelement_compar_fn)strcmp, key);
            gl_list_add_at (values, index, value);
          }
        else
          gl_list_set_at (values, index, value);
        check_all (map1, keys, values);
      }

    for (repeat = 0; repeat < 100000; repeat++)
      {
        unsigned int operation = RANDOM (3);
        switch (operation)
          {
          case 0:
            {
              const char *key = RANDOM_OBJECT ();
              const void *ret = gl_map_get (map1, key);
              size_t index =
                gl_sortedlist_indexof (keys, (gl_listelement_compar_fn)strcmp, key);
              ASSERT (ret
                      == (index != (size_t)(-1) ? gl_list_get_at (values, index) : NULL));
            }
            break;
          case 1:
            {
              const char *key = RANDOM_OBJECT ();
              const char *value = RANDOM_OBJECT ();
              bool added = gl_map_nx_put (map1, key, value);
              size_t index =
                gl_sortedlist_indexof (keys, (gl_listelement_compar_fn)strcmp, key);
              ASSERT (added == (index == (size_t)(-1)));
              if (added)
                {
                  gl_sortedlist_add (keys, (gl_listelement_compar_fn)strcmp, key);
                  index = gl_sortedlist_indexof (keys, (gl_listelement_compar_fn)strcmp, key);
                  gl_list_add_at (values, index, value);
                }
              else
                gl_list_set_at (values, index, value);
            }
            break;
          case 2:
            {
              const char *key = RANDOM_OBJECT ();
              bool removed = gl_map_remove (map1, key);
              size_t index =
                gl_sortedlist_indexof (keys, (gl_listelement_compar_fn)strcmp, key);
              ASSERT (removed == (index != (size_t)(-1)));
              if (removed)
                {
                  gl_list_remove_at (keys, index);
                  gl_list_remove_at (values, index);
                }
            }
            break;
          }
        check_all (map1, keys, values);
      }

    gl_map_free (map1);
    gl_list_free (keys);
    gl_list_free (values);
  }

  return 0;
}
