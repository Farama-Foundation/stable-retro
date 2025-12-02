/* Test of ordered set data type implementation.
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

static void
action (const void *str, void *data)
{
  ((char *) str)[0] += *(int *)data;
}

static void
test_update (gl_oset_implementation_t implementation)
{
  char A[2] = "A";
  char B[2] = "B";
  char C[2] = "C";
  char D[2] = "D";

  gl_oset_t set1 =
    gl_oset_nx_create_empty (implementation, (gl_setelement_compar_fn) strcmp, NULL);
  ASSERT (set1 != NULL);

  /* Fill the set.  */
  ASSERT (gl_oset_nx_add (set1, C) == 1);
  ASSERT (gl_oset_nx_add (set1, A) == 1);
  ASSERT (gl_oset_nx_add (set1, B) == 1);
  ASSERT (gl_oset_nx_add (set1, D) == 1);

  /* Verify that set1 = ["A", "B", "C", "D"].  */
  {
    gl_oset_iterator_t iter = gl_oset_iterator (set1);
    const void *elt;

    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == A);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == B);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == C);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == D);
    ASSERT (!gl_oset_iterator_next (&iter, &elt));
  }

  /* Make a side effect on an element in the set, that moves the element.  */
  {
    int data = 'G' - 'B';
    ASSERT (gl_oset_update (set1, B, action, &data) == 1);
  }
  /* Verify that set1 = ["A", "C", "D", "G"].  */
  {
    gl_oset_iterator_t iter = gl_oset_iterator (set1);
    const void *elt;

    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == A);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == C);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == D);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == B);
    ASSERT (!gl_oset_iterator_next (&iter, &elt));
  }

  /* Make a side effect on an element in the set, that does not move the
     element.  */
  {
    int data = 'E' - 'D';
    ASSERT (gl_oset_update (set1, D, action, &data) == 0);
  }
  /* Verify that set1 = ["A", "C", "E", "G"].  */
  {
    gl_oset_iterator_t iter = gl_oset_iterator (set1);
    const void *elt;

    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == A);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == C);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == D);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == B);
    ASSERT (!gl_oset_iterator_next (&iter, &elt));
  }

  /* Make a side effect on an element in the set, that provokes a
     collision.  */
  {
    int data = 'G' - 'A';
    ASSERT (gl_oset_update (set1, A, action, &data) == -1);
  }
  /* Verify that set1 = ["C", "E", "G"].  */
  {
    gl_oset_iterator_t iter = gl_oset_iterator (set1);
    const void *elt;

    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == C);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == D);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == B);
    ASSERT (!gl_oset_iterator_next (&iter, &elt));
  }

  /* Make a side effect on an element that is not in the set.  */
  {
    int data = 'R' - 'G';
    ASSERT (gl_oset_update (set1, A, action, &data) == 0);
  }
  /* Verify that set1 = ["C", "E", "G"].  */
  {
    gl_oset_iterator_t iter = gl_oset_iterator (set1);
    const void *elt;

    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == C);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == D);
    ASSERT (gl_oset_iterator_next (&iter, &elt));
    ASSERT (elt == B);
    ASSERT (!gl_oset_iterator_next (&iter, &elt));
  }

  gl_oset_free (set1);
}
