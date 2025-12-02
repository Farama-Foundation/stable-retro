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
    char buf[16];
    char key[] = "\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00";
    char pt[] = "\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00";
    char ct[] = "\xC3\x4C\x05\x2C\xC0\xDA\x8D\x73"
      "\x45\x1A\xFE\x5F\x03\xBE\x29\x7F";
    gc_cipher_handle ctx;
    size_t i;

    rc = gc_cipher_open (GC_AES128, GC_ECB, &ctx);
    if (rc != GC_OK)
      return 1;

    rc = gc_cipher_setkey (ctx, 16, key);
    if (rc != GC_OK)
      return 1;

    memcpy (buf, pt, 16);

    for (i = 0; i < 10000; i++)
      {
        rc = gc_cipher_encrypt_inline (ctx, 16, buf);
        if (rc != GC_OK)
          {
            printf ("encrypt failed %d\n", rc);
            return 1;
          }
      }

    if (memcmp (buf, ct, 16) != 0)
      {
        size_t i;
        printf ("expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", ct[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", buf[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    for (i = 0; i < 10000; i++)
      {
        rc = gc_cipher_decrypt_inline (ctx, 16, buf);
        if (rc != GC_OK)
          {
            printf ("decrypt failed %d\n", rc);
            return 1;
          }
      }

    if (memcmp (buf, pt, 16) != 0)
      {
        size_t i;
        printf ("expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", pt[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", buf[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_cipher_close (ctx);
  }


  {
    char buf[16];
    char iv[] = "\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00";
    char key[] = "\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00";
    char pt[] = "\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x00";
    char ct[] = "\x66\xe9\x4b\xd4\xef\x8a\x2c\x3b"
      "\x88\x4c\xfa\x59\xca\x34\x2b\x2e";
    gc_cipher_handle ctx;
    size_t i;

    rc = gc_cipher_open (GC_AES128, GC_CBC, &ctx);
    if (rc != GC_OK)
      return 1;

    rc = gc_cipher_setkey (ctx, 16, key);
    if (rc != GC_OK)
      return 1;

    rc = gc_cipher_setiv (ctx, 16, iv);
    if (rc != GC_OK)
      return 1;

    memcpy (buf, pt, 16);

    for (i = 0; i < 10000; i++)
      {
        rc = gc_cipher_encrypt_inline (ctx, 16, buf);
        if (rc != GC_OK)
          {
            printf ("encrypt failed %d\n", rc);
            return 1;
          }
      }

    if (memcmp (buf, ct, 16) != 0)
      {
        size_t i;
        printf ("expected:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", ct[i] & 0xFF);
        printf ("\ncomputed:\n");
        for (i = 0; i < 16; i++)
          printf ("%02x ", buf[i] & 0xFF);
        printf ("\n");
        return 1;
      }

    gc_cipher_close (ctx);
  }

  gc_done ();

  return 0;
}
