/* Test of uN_cpy_alloc() functions.
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

int
main ()
{
  /* Test small copying operations.  */
  {
    static const UNIT src[] = { 'c', 'l', 'i', 'm', 'a', 't', 'e' };
    size_t n;

    for (n = 0; n <= SIZEOF (src); n++)
      {
        UNIT *result = U_CPY_ALLOC (src, n);
        size_t i;

        ASSERT (result != NULL);
        for (i = 0; i < n; i++)
          ASSERT (result[i] == src[i]);

        free (result);
      }
  }

  return 0;
}
