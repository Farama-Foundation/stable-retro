/* Test of uN_move() functions.
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
  /* Test copying operations with disjoint source and destination.  */
  {
    static const UNIT src[] = { 'c', 'l', 'i', 'm', 'a', 't', 'e' };
    size_t n;

    for (n = 0; n <= SIZEOF (src); n++)
      {
        UNIT dest[1 + SIZEOF (src) + 1] =
          { MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC, MAGIC };
        UNIT *ret;
        size_t i;

        ret = U_MOVE (dest + 1, src, n);
        ASSERT (ret == dest + 1);
        ASSERT (dest[0] == MAGIC);
        for (i = 0; i < n; i++)
          ASSERT (dest[1 + i] == src[i]);
        ASSERT (dest[1 + n] == MAGIC);
      }
  }

  /* Test copying operations with overlap, in-place.  */
  {
    static const UNIT src[] = { 'c', 'l', 'i', 'm', 'a', 't', 'e' };
    size_t n;

    for (n = 0; n <= SIZEOF (src); n++)
      {
        UNIT dest[1 + SIZEOF (src) + 1];
        UNIT *ret;
        size_t i;

        dest[0] = MAGIC;
        for (i = 0; i < n; i++)
          dest[1 + i] = src[i];
        dest[1 + n] = MAGIC;

        ret = U_MOVE (dest + 1, dest + 1, n);
        ASSERT (ret == dest + 1);
        ASSERT (dest[0] == MAGIC);
        for (i = 0; i < n; i++)
          ASSERT (dest[1 + i] == src[i]);
        ASSERT (dest[1 + n] == MAGIC);
      }
  }

  /* Test copying operations with overlap, moving downward.  */
  {
    static const UNIT src[] = { 'c', 'l', 'i', 'm', 'a', 't', 'e' };
    static const UNIT src2[] = { 'C', 'L', 'I', 'M', 'A', 'T', 'E' };
    size_t d;

    ASSERT (SIZEOF (src) == SIZEOF (src2));
    for (d = 0; d <= SIZEOF (src); d++)
      {
        size_t n;

        for (n = 0; n <= SIZEOF (src); n++)
          {
            UNIT dest[1 + 2 * SIZEOF (src) + 1];
            UNIT *ret;
            size_t i;

            dest[0] = MAGIC;
            for (i = 0; i < SIZEOF (src2); i++)
              dest[1 + i] = src2[i];
            for (i = 0; i < SIZEOF (src); i++)
              dest[1 + SIZEOF (src) + i] = src[i];
            dest[1 + 2 * SIZEOF (src)] = MAGIC;

            ret =
              U_MOVE (dest + 1 + SIZEOF (src) - d, dest + 1 + SIZEOF (src), n);
            ASSERT (ret == dest + 1 + SIZEOF (src) - d);
            ASSERT (dest[0] == MAGIC);
            for (i = 0; i < SIZEOF (src) - d; i++)
              ASSERT (dest[1 + i] == src2[i]);
            for (i = 0; i < n; i++)
              ASSERT (dest[1 + SIZEOF (src) - d + i] == src[i]);
            for (i = SIZEOF (src) - d + n; i < SIZEOF (src2); i++)
              ASSERT (dest[1 + i] == src2[i]);
            for (i = (n >= d ? n - d : 0); i < SIZEOF (src); i++)
              ASSERT (dest[1 + SIZEOF (src) + i] == src[i]);
            ASSERT (dest[1 + 2 * SIZEOF (src)] == MAGIC);
          }
      }
  }

  /* Test copying operations with overlap, moving upward.  */
  {
    static const UNIT src[] = { 'c', 'l', 'i', 'm', 'a', 't', 'e' };
    static const UNIT src2[] = { 'C', 'L', 'I', 'M', 'A', 'T', 'E' };
    size_t d;

    ASSERT (SIZEOF (src) == SIZEOF (src2));
    for (d = 0; d <= SIZEOF (src); d++)
      {
        size_t n;

        for (n = 0; n <= SIZEOF (src); n++)
          {
            UNIT dest[1 + 2 * SIZEOF (src) + 1];
            UNIT *ret;
            size_t i;

            dest[0] = MAGIC;
            for (i = 0; i < SIZEOF (src); i++)
              dest[1 + i] = src[i];
            for (i = 0; i < SIZEOF (src2); i++)
              dest[1 + SIZEOF (src) + i] = src2[i];
            dest[1 + 2 * SIZEOF (src)] = MAGIC;

            ret = U_MOVE (dest + 1 + d, dest + 1, n);
            ASSERT (ret == dest + 1 + d);
            ASSERT (dest[0] == MAGIC);
            for (i = 0; i < d; i++)
              ASSERT (dest[1 + i] == src[i]);
            for (i = 0; i < n; i++)
              ASSERT (dest[1 + d + i] == src[i]);
            for (i = d + n; i < SIZEOF (src); i++)
              ASSERT (dest[1 + i] == src[i]);
            for (i = (d + n >= SIZEOF (src) ? d + n - SIZEOF (src) : 0);
                 i < SIZEOF (src2);
                 i++)
              ASSERT (dest[1 + SIZEOF (src) + i] == src2[i]);
            ASSERT (dest[1 + 2 * SIZEOF (src)] == MAGIC);
          }
      }
  }

  return 0;
}
