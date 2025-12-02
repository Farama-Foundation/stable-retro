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

  /* Test vectors from RFC 1319. */

  {
    char *in = "abcdefghijklmnopqrstuvwxyz";
    size_t inlen = strlen (in);
    char *expect =
      "\x4e\x8d\xdf\xf3\x65\x02\x92\xab\x5a\x41\x08\xc3\xaa\x47\x94\x0b";
    char out[16];
    const char *p;

    if (gc_md2 (in, inlen, out) != 0)
      {
        printf ("gc_md2 call failed\n");
        return 1;
      }

    if (memcmp (out, expect, 16) != 0)
      {
        size_t i;
        printf ("md2 1 mismatch. expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    if (gc_hash_buffer (GC_MD2, in, inlen, out) != 0)
      {
        printf ("gc_hash_buffer(MD2) call failed\n");
        return 1;
      }

    if (memcmp (out, expect, 16) != 0)
      {
        size_t i;
        printf ("md2 2 mismatch. expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    if (gc_hash_digest_length (GC_MD2) != 16)
      {
        printf ("gc_hash_digest_length (GC_MD2) failed\n");
        return 1;
      }

    if ((rc = gc_hash_open (GC_MD2, 0, &h)) != GC_OK)
      {
        printf ("gc_hash_open(GC_MD2) failed (%d)\n", rc);
        return 1;
      }

    gc_hash_write (h, inlen, in);

    p = gc_hash_read (h);

    if (!p)
      {
        printf ("gc_hash_read failed\n");
        return 1;
      }

    if (memcmp (p, expect, 16) != 0)
        {
        size_t i;
        printf ("md2 3 mismatch. expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", p[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_hash_close (h);
  }

  gc_done ();

  return 0;
}
