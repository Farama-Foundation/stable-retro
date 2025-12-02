/*
 * Copyright (C) 2005, 2010-2021 Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Simon Josefsson. */

#include <config.h>

#include "md2.h"

#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  const char *in1 = "abc";
  const char *out1 =
    "\xda\x85\x3b\x0d\x3f\x88\xd9\x9b\x30\x28\x3a\x69\xe6\xde\xd6\xbb";
  const char *in2 = "abcdefghijklmnopqrstuvwxyz";
  const char *out2 =
    "\x4e\x8d\xdf\xf3\x65\x02\x92\xab\x5a\x41\x08\xc3\xaa\x47\x94\x0b";
  char buf[MD2_DIGEST_SIZE];

  if (memcmp (md2_buffer (in1, strlen (in1), buf), out1, MD2_DIGEST_SIZE) !=
      0)
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < MD2_DIGEST_SIZE; i++)
        printf ("%02x ", out1[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < MD2_DIGEST_SIZE; i++)
        printf ("%02x ", buf[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  if (memcmp (md2_buffer (in2, strlen (in2), buf), out2, MD2_DIGEST_SIZE) !=
      0)
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < MD2_DIGEST_SIZE; i++)
        printf ("%02x ", out2[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < MD2_DIGEST_SIZE; i++)
        printf ("%02x ", buf[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  return 0;
}
