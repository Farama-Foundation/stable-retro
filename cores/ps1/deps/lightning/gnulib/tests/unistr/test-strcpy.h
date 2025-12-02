/* Test of uN_strcpy() functions.
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
    static const UNIT src[] = { 'c', 'l', 'i', 'm', 'a', 't', 'e', 0 };
    size_t n;

    for (n = 1; n <= SIZEOF (src); n++)
      {
        UNIT dest[1 + SIZEOF (src) + 1] =
          { MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC,
            MAGIC
          };
        UNIT *result;
        size_t i;

        result = U_STRCPY (dest + 1, src + SIZEOF (src) - n);
        ASSERT (result == dest + 1);

        ASSERT (dest[0] == MAGIC);
        for (i = 0; i < n; i++)
          ASSERT (dest[1 + i] == src[SIZEOF (src) - n + i]);
        ASSERT (dest[1 + n] == MAGIC);
      }
  }

  return 0;
}
