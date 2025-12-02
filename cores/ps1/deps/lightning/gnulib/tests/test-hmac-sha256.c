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

/* Written by Simon Josefsson.  Test vectors from RFC 4231.  */

#include <config.h>

#include "hmac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
hmac_check (const void *key, size_t key_len,
            const void *data, size_t data_len, const char *digest)
{
  char out[32];

  if (hmac_sha256 (key, key_len, data, data_len, out) != 0)
    {
      printf ("call failure\n");
      exit (1);
    }

  if (memcmp (digest, out, 32) != 0)
    {
      size_t i;
      printf ("hash 1 mismatch. expected:\n");
      for (i = 0; i < 32; i++)
        printf ("%02x ", digest[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 32; i++)
        printf ("%02x ", out[i] & 0xFF);
      printf ("\n");
      exit (1);
    }
}

int
main (int argc, char *argv[])
{
  {
    char key[20];
    size_t key_len = sizeof key;
    memset (key, '\x0b', sizeof key);
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\xb0\x34\x4c\x61\xd8\xdb\x38\x53\x5c\xa8\xaf\xce\xaf\x0b\xf1\x2b"
      "\x88\x1d\xc2\x00\xc9\x83\x3d\xa7\x26\xe9\x37\x6c\x2e\x32\xcf\xf7";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char *key = "Jefe";
    size_t key_len = 4;
    char *data = "what do ya want for nothing?";
    size_t data_len = 28;
    char *digest =
      "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75\xc7"
      "\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec\x38\x43";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char key[20];
    size_t key_len = sizeof key;
    memset (key, '\xAA', sizeof key);
    char data[50];
    size_t data_len = sizeof data;
    memset (data, '\xDD', sizeof data);
    char *digest =
      "\x77\x3e\xa9\x1e\x36\x80\x0e\x46\x85\x4d\xb8\xeb\xd0\x91\x81\xa7"
      "\x29\x59\x09\x8b\x3e\xf8\xc1\x22\xd9\x63\x55\x14\xce\xd5\x65\xfe";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char key[65];
    size_t key_len = sizeof key;
    memset (key, '\x0b', sizeof key);
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\x72\x7b\x82\xfb\xa2\x64\x39\x3c\x5d\x67\xfd\x6d\x6a\xd7\x83\xe9"
      "\x01\x9a\x1f\xa6\xa8\x57\xfc\xcb\x70\xf5\x85\x2f\x04\xbe\x5d\x5d";
    hmac_check (key, key_len, data, data_len, digest);
  }

  return 0;
}
