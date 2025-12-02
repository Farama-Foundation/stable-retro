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
  gc_cipher_handle ctx;
  /* Test vectors from RFC 2268. */
  static char key[8] = "\xff\xff\xff\xff\xff\xff\xff\xff";
  static char plaintext[8] = "\xff\xff\xff\xff\xff\xff\xff\xff";
  static const char ciphertext[8] = "\x27\x8b\x27\xe4\x2e\x2f\x0d\x49";
  char scratch[16];
  Gc_rc rc;

  rc = gc_init ();
  if (rc != GC_OK)
    {
      printf ("gc_init() failed\n");
      return 1;
    }

  rc = gc_cipher_open (GC_ARCTWO40, GC_ECB, &ctx);
  if (rc != GC_OK)
    return 1;

  rc = gc_cipher_setkey (ctx, sizeof (key), key);
  if (rc != GC_OK)
    return 1;

  memcpy (scratch, plaintext, sizeof (plaintext));
  rc = gc_cipher_encrypt_inline (ctx, sizeof (plaintext), scratch);
  if (rc != GC_OK)
    return 1;

  if (memcmp (scratch, ciphertext, sizeof (ciphertext)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", ciphertext[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  /* decrypt */

  rc = gc_cipher_setkey (ctx, sizeof (key), key);
  if (rc != GC_OK)
    return 1;

  rc = gc_cipher_decrypt_inline (ctx, sizeof (plaintext), scratch);
  if (rc != GC_OK)
    return 1;

  if (memcmp (scratch, plaintext, sizeof (plaintext)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", plaintext[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  gc_cipher_close (ctx);

  gc_done ();

  return 0;
}
