/* Test of u32_[v]asnprintf() function.
   Copyright (C) 2007, 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

static void
test_function (uint32_t * (*my_asnprintf) (uint32_t *, size_t *, const char *, ...))
{
  uint32_t buf[8];
  int size;

  /* Test return value convention.  */

  for (size = 0; size <= 8; size++)
    {
      size_t length = size;
      uint32_t *result = my_asnprintf (NULL, &length, "%d", 12345);
      static const uint32_t expected[] =
        { '1', '2', '3', '4', '5', 0 };
      ASSERT (result != NULL);
      ASSERT (u32_strcmp (result, expected) == 0);
      ASSERT (length == 5);
      free (result);
    }

  for (size = 0; size <= 8; size++)
    {
      static const uint32_t initializer[] =
        { 'D', 'E', 'A', 'D', 'B', 'E', 'E', 'F', 0 };
      static const uint32_t expected[] =
        { '1', '2', '3', '4', '5', 0 };
      size_t length;
      uint32_t *result;

      u32_cpy (buf, initializer, 8);
      length = size;
      result = my_asnprintf (buf, &length, "%d", 12345);
      ASSERT (result != NULL);
      ASSERT (u32_strcmp (result, expected) == 0);
      ASSERT (length == 5);
      if (size < 6)
        ASSERT (result != buf);
      ASSERT (u32_cmp (buf + size, initializer + size, 8 - size) == 0);
      if (result != buf)
        free (result);
    }
}
