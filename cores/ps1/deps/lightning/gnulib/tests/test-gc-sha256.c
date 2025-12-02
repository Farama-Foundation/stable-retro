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
      "\x71\xc4\x80\xdf\x93\xd6\xae\x2f\x1e\xfa\xd1\x44\x7c\x66\xc9\x52"
      "\x5e\x31\x62\x18\xcf\x51\xfc\x8d\x9e\xd8\x32\xf2\xda\xf1\x8b\x73";
    char out[32];
    const char *p;

    if (gc_sha256 (in, inlen, out) != 0)
      {
        printf ("gc_sha256 call failed\n");
        return 1;
      }

    if (memcmp (out, expect, 32) != 0)
      {
        size_t i;
        printf ("sha256 test1 mismatch. expected:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    rc = gc_hash_buffer (GC_SHA256, "abcdefghijklmnopqrstuvwxyz", 26, out);
    if (rc != GC_OK)
      {
        printf ("gc_hash_buffer(sha256) call failed: %d\n", rc);
        return 1;
      }

    if (memcmp (out, expect, 32) != 0)
      {
        size_t i;
        printf ("sha256 test2 mismatch. expected:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    if (gc_hash_digest_length (GC_SHA256) != 32)
      {
        printf ("gc_hash_digest_length (GC_SHA256) failed\n");
        return 1;
      }

    if ((rc = gc_hash_open (GC_SHA256, 0, &h)) != GC_OK)
      {
        printf ("gc_hash_open(GC_SHA256) failed (%d)\n", rc);
        return 1;
      }

    gc_hash_write (h, inlen, in);

    p = gc_hash_read (h);

    if (!p)
      {
        printf ("gc_hash_read failed\n");
        return 1;
      }

    if (memcmp (p, expect, 32) != 0)
      {
        size_t i;
        printf ("sha256 test3 mismatch. expected:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", p[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_hash_close (h);
  }

  gc_done ();

  return 0;
}
