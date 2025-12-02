/*
 * Copyright (C) 2017-2021 Free Software Foundation, Inc.
 * Written by Jia Zhang <qianyue.zj@alibaba-inc.com>
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

  /* Test vector from GM/T 004-2012. */

  {
    const char *in = "abc";
    size_t inlen = strlen (in);
    const char *expect = "\x66\xc7\xf0\xf4\x62\xee\xed\xd9\xd1"
      "\xf2\xd4\x6b\xdc\x10\xe4\xe2\x41\x67\xc4\x87\x5c"
      "\xf2\xf7\xa2\x29\x7d\xa0\x2b\x8f\x4b\xa8\xe0";
    char out[32];
    const char *p;

    if (gc_sm3 (in, inlen, out) != 0)
      {
        printf ("gc_sm3 call failed\n");
        return 1;
      }

    if (memcmp (out, expect, 32) != 0)
      {
        size_t i;
        printf ("sm3 mismatch. expected:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", (unsigned int) expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", (unsigned int) out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    rc = gc_hash_buffer (GC_SM3, "abc", 3, out);
    if (rc != GC_OK)
      {
        printf ("gc_hash_buffer(sm3) call failed: %u\n", rc);
        return 1;
      }

    if (memcmp (out, expect, 32) != 0)
      {
        size_t i;
        printf ("sm3' mismatch. expected:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", (unsigned int) expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", (unsigned int) out[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    if (gc_hash_digest_length (GC_SM3) != 32)
      {
        printf ("gc_hash_digest_length (GC_SM3) failed\n");
        return 1;
      }

    if ((rc = gc_hash_open (GC_SM3, 0, &h)) != GC_OK)
      {
        printf ("gc_hash_open(GC_SM3) failed (%u)\n", rc);
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
        printf ("sm3 mismatch. expected:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", (unsigned int) expect[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 32; i++)
          printf ("%02x ", (unsigned int) p[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_hash_close (h);
  }

  gc_done ();

  return 0;
}
