/* Test of uN_strstr() functions.
   Copyright (C) 2004, 2007-2021 Free Software Foundation, Inc.

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
test_u_strstr (void)
{
  {
    const UNIT input[] = { 'f', 'o', 'o', 0 };
    const UNIT needle[] = { 0 };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == input);
  }

  {
    const UNIT input[] = { 'f', 'o', 'o', 0 };
    const UNIT needle[] = { 'o', 0 };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == input + 1);
  }

  {
    const UNIT input[] =
      { 'A', 'B', 'C', ' ', 'A', 'B', 'C', 'D', 'A', 'B', ' ', 'A', 'B', 'C',
        'D', 'A', 'B', 'C', 'D', 'A', 'B', 'D', 'E', 0
      };
    const UNIT needle[] = { 'A', 'B', 'C', 'D', 'A', 'B', 'D', 0 };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == input + 15);
  }

  {
    const UNIT input[] =
      { 'A', 'B', 'C', ' ', 'A', 'B', 'C', 'D', 'A', 'B', ' ', 'A', 'B', 'C',
        'D', 'A', 'B', 'C', 'D', 'A', 'B', 'D', 'E', 0
      };
    const UNIT needle[] = { 'A', 'B', 'C', 'D', 'A', 'B', 'E', 0 };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == NULL);
  }

  {
    const UNIT input[] =
      { 'A', 'B', 'C', ' ', 'A', 'B', 'C', 'D', 'A', 'B', ' ', 'A', 'B', 'C',
        'D', 'A', 'B', 'C', 'D', 'A', 'B', 'D', 'E', 0
      };
    const UNIT needle[] = { 'A', 'B', 'C', 'D', 'A', 'B', 'C', 'D', 0 };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == input + 11);
  }

  /* Check that a long periodic needle does not cause false positives.  */
  {
    const UNIT input[] =
      { 'F', '_', 'B', 'D', '_', 'C', 'E', '_', 'B', 'D', '_', 'E', 'F',
        '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'C', '3',
        '_', '8', '8', '_', '2', '0', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'C', '3',
        '_', 'A', '7', '_', '2', '0', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', 0
      };
    const UNIT needle[] =
      { '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F',
        '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', 0
      };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == NULL);
  }
  {
    const UNIT input[] =
      { 'F', '_', 'B', 'D', '_', 'C', 'E', '_', 'B', 'D', '_', 'E', 'F',
        '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'C', '3',
        '_', '8', '8', '_', '2', '0', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'C', '3',
        '_', 'A', '7', '_', '2', '0', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'D', 'A', '_', 'B', '5', '_', 'C', '2',
        '_', 'A', '6', '_', '2', '0', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F',
        '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', 0
      };
    const UNIT needle[] =
      { '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F',
        '_', 'B', 'F', '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F',
        '_', 'B', 'D', '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D',
        '_', 'E', 'F', '_', 'B', 'F', '_', 'B', 'D', 0
      };
    const UNIT *result = U_STRSTR (input, needle);
    ASSERT (result == input + 115);
  }

  /* Check that a very long haystack is handled quickly if the needle is
     short and occurs near the beginning.  */
  {
    size_t repeat = 10000;
    size_t m = 1000000;
    const UNIT needle[] =
      { 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 0
      };
    UNIT *haystack = (UNIT *) malloc ((m + 1) * sizeof (UNIT));
    if (haystack != NULL)
      {
        size_t i;

        haystack[0] = 'B';
        for (i = 1; i < m; i++)
          haystack[i] = 'A';
        haystack[m] = '\0';

        for (; repeat > 0; repeat--)
          {
            ASSERT (U_STRSTR (haystack, needle) == haystack + 1);
          }

        free (haystack);
      }
  }

  /* Check that a very long needle is discarded quickly if the haystack is
     short.  */
  {
    size_t repeat = 10000;
    size_t m = 1000000;
    const UNIT haystack[] =
      { 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'B',
        'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B',
        'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B',
        'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B',
        'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B',
        'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 'A', 'B', 0
      };
    UNIT *needle = (UNIT *) malloc ((m + 1) * sizeof (UNIT));
    if (needle != NULL)
      {
        size_t i;

        for (i = 0; i < m; i++)
          needle[i] = 'A';
        needle[m] = '\0';

        for (; repeat > 0; repeat--)
          {
            ASSERT (U_STRSTR (haystack, needle) == NULL);
          }

        free (needle);
      }
  }

  /* Check that the asymptotic worst-case complexity is not quadratic.  */
  {
    size_t m = 1000000;
    UNIT *haystack = (UNIT *) malloc ((2 * m + 2) * sizeof (UNIT));
    UNIT *needle = (UNIT *) malloc ((m + 2) * sizeof (UNIT));
    if (haystack != NULL && needle != NULL)
      {
        size_t i;
        const UNIT *result;

        for (i = 0; i < 2 * m; i++)
          haystack[i] = 'A';
        haystack[2 * m] = 'B';
        haystack[2 * m + 1] = 0;

        for (i = 0; i < m; i++)
          needle[i] = 'A';
        needle[m] = 'B';
        needle[m + 1] = 0;

        result = U_STRSTR (haystack, needle);
        ASSERT (result == haystack + m);
      }
    free (needle);
    free (haystack);
  }
}
