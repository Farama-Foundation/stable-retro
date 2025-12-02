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
  gc_hash_handle h;

  rc = gc_init ();
  if (rc != GC_OK)
    {
      printf ("gc_init() failed\n");
      return 1;
    }

  {
    char *in = "abcdefgh";
    size_t inlen = strlen (in);
    char *expect = "\x42\x5a\xf1\x2a\x07\x43\x50\x2b"
      "\x32\x2e\x93\xa0\x15\xbc\xf8\x68\xe3\x24\xd5\x6a";
    char out[20];
    const char *p;

    if (gc_sha1 (in, inlen, out) != 0)
      {
        printf ("gc_sha1 call failed\n");
        return 1;
      }

    if (memcmp (out, expect, 20) != 0)
      {
        size_t i;
        printf ("sha1 test1 mismatch. expected:\n");
        for (i = 0; i < 20; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 20; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    rc = gc_hash_buffer (GC_SHA1, "abcdefgh", 8, out);
    if (rc != GC_OK)
      {
        printf ("gc_hash_buffer(sha1) call failed: %d\n", rc);
        return 1;
      }

    if (memcmp (out, expect, 20) != 0)
      {
        size_t i;
        printf ("sha1 test2 mismatch. expected:\n");
        for (i = 0; i < 20; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 20; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    if (gc_hash_digest_length (GC_SHA1) != 20)
      {
        printf ("gc_hash_digest_length (GC_SHA1) failed\n");
        return 1;
      }

    if ((rc = gc_hash_open (GC_SHA1, 0, &h)) != GC_OK)
      {
        printf ("gc_hash_open(GC_SHA1) failed (%d)\n", rc);
        return 1;
      }

    gc_hash_write (h, inlen, in);

    p = gc_hash_read (h);

    if (!p)
      {
        printf ("gc_hash_read failed\n");
        return 1;
      }

    if (memcmp (p, expect, 20) != 0)
      {
        size_t i;
        printf ("sha1 test3 mismatch. expected:\n");
        for (i = 0; i < 20; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 20; i++)
          printf ("%02x ", p[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_hash_close (h);
  }

  gc_done ();

  return 0;
}
