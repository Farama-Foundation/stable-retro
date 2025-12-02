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

static void
hmac_check (const void *key, size_t key_len,
            const void *data, size_t data_len, const char *digest)
{
  char out[20];

  if (hmac_sha1 (key, key_len, data, data_len, out) != 0)
    {
      printf ("call failure\n");
      exit (1);
    }

  if (memcmp (digest, out, 20) != 0)
    {
      size_t i;
      printf ("hash 1 mismatch. expected:\n");
      for (i = 0; i < 20; i++)
        printf ("%02x ", digest[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 20; i++)
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
      "\x67\x5b\x0b\x3a\x1b\x4d\xdf\x4e\x12\x48\x72\xda\x6c\x2f\x63\x2b"
      "\xfe\xd9\x57\xe9";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char *key = "Jefe";
    size_t key_len = 4;
    char *data = "what do ya want for nothing?";
    size_t data_len = 28;
    char *digest =
      "\xef\xfc\xdf\x6a\xe5\xeb\x2f\xa2\xd2\x74\x16\xd5\xf1\x84\xdf\x9c"
      "\x25\x9a\x7c\x79";
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
      "\x12\x5d\x73\x42\xb9\xac\x11\xcd\x91\xa3\x9a\xf4\x8a\xa1\x7b\x4f"
      "\x63\xf1\x75\xd3";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char key[65];
    size_t key_len = sizeof key;
    memset (key, '\x0b', sizeof key);
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\x29\xda\xa9\xe9\xcc\x4b\x9f\x09\x48\x29\xdc\xd4\x03\xc0\x69\x27"
      "\xd8\xa9\x53\x93";
    hmac_check (key, key_len, data, data_len, digest);
  }
  return 0;
}
