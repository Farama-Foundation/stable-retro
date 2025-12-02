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
    char *in = "abcdefghijklmnopqrstuvwxyz";
    size_t inlen = strlen (in);
    char *expect =
      "\x4d\xbf\xf8\x6c\xc2\xca\x1b\xae\x1e\x16\x46\x8a\x05\xcb\x98\x81"
      "\xc9\x7f\x17\x53\xbc\xe3\x61\x90\x34\x89\x8f\xaa\x1a\xab\xe4\x29"
      "\x95\x5a\x1b\xf8\xec\x48\x3d\x74\x21\xfe\x3c\x16\x46\x61\x3a\x59"
      "\xed\x54\x41\xfb\x0f\x32\x13\x89\xf7\x7f\x48\xa8\x79\xc7\xb1\xf1";
    char out[64];
    const char *p;

    if (gc_sha512 (in, inlen, out) != 0)
      {
        printf ("gc_sha512 call failed\n");
        return 1;
      }

    if (memcmp (out, expect, 64) != 0)
      {
        size_t i;
        printf ("sha512 test1 mismatch. expected:\n");
        for (i = 0; i < 64; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 64; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    rc = gc_hash_buffer (GC_SHA512, "abcdefghijklmnopqrstuvwxyz", 26, out);
    if (rc != GC_OK)
      {
        printf ("gc_hash_buffer(sha512) call failed: %d\n", rc);
        return 1;
      }

    if (memcmp (out, expect, 64) != 0)
      {
        size_t i;
        printf ("sha512 test2 mismatch. expected:\n");
        for (i = 0; i < 64; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 64; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    if (gc_hash_digest_length (GC_SHA512) != 64)
      {
        printf ("gc_hash_digest_length (GC_SHA512) failed\n");
        return 1;
      }

    if ((rc = gc_hash_open (GC_SHA512, 0, &h)) != GC_OK)
      {
        printf ("gc_hash_open(GC_SHA512) failed (%d)\n", rc);
        return 1;
      }

    gc_hash_write (h, inlen, in);

    p = gc_hash_read (h);

    if (!p)
      {
        printf ("gc_hash_read failed\n");
        return 1;
      }

    if (memcmp (p, expect, 64) != 0)
      {
        size_t i;
        printf ("sha512 test3 mismatch. expected:\n");
        for (i = 0; i < 64; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 64; i++)
          printf ("%02x ", p[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_hash_close (h);
  }

  gc_done ();

  return 0;
}
