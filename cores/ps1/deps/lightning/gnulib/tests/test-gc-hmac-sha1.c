/*
 * Copyright (C) 2005, 2010-2021 Free Software Foundation, Inc.
 * Written by Simon Josefsson
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

#include <config.h>

#include "gc.h"

#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  Gc_rc rc;

  rc = gc_init ();
  if (rc != GC_OK)
    {
      printf ("gc_init() failed\n");
      return 1;
    }

  {
    char *key =
      "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b";
    size_t key_len = 16;
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\x67\x5b\x0b\x3a\x1b\x4d\xdf\x4e\x12\x48"
      "\x72\xda\x6c\x2f\x63\x2b\xfe\xd9\x57\xe9";
    char out[GC_SHA1_DIGEST_SIZE];

    if (gc_hmac_sha1 (key, key_len, data, data_len, out) != 0)
      {
        printf ("call failure\n");
        return 1;
      }

    if (memcmp (digest, out, GC_SHA1_DIGEST_SIZE) != 0)
      {
        size_t i;
        printf ("hash 1 mismatch. expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", digest[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }
  }

  gc_done ();

  return 0;
}
