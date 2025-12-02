/*
 * Copyright (C) 2005, 2010-2021 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Simon Josefsson.  */

#include <config.h>

#include "hmac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test vectors from RFC 2104. */

static void
hmac_check (const void *key, size_t key_len,
            const void *data, size_t data_len, const char *digest)
{
  char out[16];

  if (hmac_md5 (key, key_len, data, data_len, out) != 0)
    {
      printf ("call failure\n");
      exit (1);
    }

  if (memcmp (digest, out, 16) != 0)
    {
      size_t i;
      printf ("hash 1 mismatch. expected:\n");
      for (i = 0; i < 16; i++)
        printf ("%02x ", digest[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 16; i++)
        printf ("%02x ", out[i] & 0xFF);
      printf ("\n");
      exit (1);
    }
}

int
main (int argc, char *argv[])
{
  {
    char key[16];
    size_t key_len = sizeof key;
    memset (key, '\x0b', sizeof key);
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\x92\x94\x72\x7a\x36\x38\xbb\x1c\x13\xf4\x8e\xf8\x15\x8b\xfc\x9d";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char *key = "Jefe";
    size_t key_len = 4;
    char *data = "what do ya want for nothing?";
    size_t data_len = 28;
    char *digest =
      "\x75\x0c\x78\x3e\x6a\xb0\xb5\x03\xea\xa8\x6e\x31\x0a\x5d\xb7\x38";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char key[16];
    size_t key_len = sizeof key;
    memset (key, '\xAA', sizeof key);
    char data[50];
    size_t data_len = sizeof data;
    memset (data, '\xDD', sizeof data);
    char *digest =
      "\x56\xbe\x34\x52\x1d\x14\x4c\x88\xdb\xb8\xc7\x33\xf0\xe8\xb3\xf6";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char key[65];
    size_t key_len = sizeof key;
    memset (key, '\x0b', sizeof key);
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\xd6\x07\x5b\xee\x4d\x91\x80\xd8\xd1\xa2\x99\x29\x5e\x7c\xc9\xcb";
    hmac_check (key, key_len, data, data_len, digest);
  }

  return 0;
}
