/* Test of uN_strnlen() functions.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

static void
check_single (const UNIT *input, size_t length, size_t n)
{
  size_t result = U_STRNLEN (input, n);
  ASSERT (result == (n <= length ? n : length));
}

static void
check (const UNIT *input, size_t input_length)
{
  size_t length;
  size_t n;

  ASSERT (input_length > 0);
  ASSERT (input[input_length - 1] == 0);
  length = input_length - 1; /* = U_STRLEN (input) */

  for (n = 0; n <= 2 * length + 2; n++)
    check_single (input, length, n);

  /* Check that U_STRNLEN (S, N) does not look at more than
     MIN (U_STRLEN (S) + 1, N) units.  */
  {
    char *page_boundary = (char *) zerosize_ptr ();

    if (page_boundary != NULL)
      {
        for (n = 0; n <= 2 * length + 2; n++)
          {
            size_t n_to_copy = (n <= length ? n : length + 1);
            UNIT *copy;
            size_t i;

            copy = (UNIT *) page_boundary - n_to_copy;
            for (i = 0; i < n_to_copy; i++)
              copy[i] = input[i];

            check_single (copy, length, n);
          }
      }
  }
}
