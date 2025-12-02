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
  char out[64];

  if (hmac_sha512 (key, key_len, data, data_len, out) != 0)
    {
      printf ("call failure\n");
      exit (1);
    }

  if (memcmp (digest, out, 64) != 0)
    {
      size_t i;
      printf ("hash 1 mismatch. expected:\n");
      for (i = 0; i < 64; i++)
        printf ("%02x ", digest[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 64; i++)
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
      "\x87\xaa\x7c\xde\xa5\xef\x61\x9d\x4f\xf0\xb4\x24\x1a\x1d\x6c\xb0"
      "\x23\x79\xf4\xe2\xce\x4e\xc2\x78\x7a\xd0\xb3\x05\x45\xe1\x7c\xde"
      "\xda\xa8\x33\xb7\xd6\xb8\xa7\x02\x03\x8b\x27\x4e\xae\xa3\xf4\xe4"
      "\xbe\x9d\x91\x4e\xeb\x61\xf1\x70\x2e\x69\x6c\x20\x3a\x12\x68\x54";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char *key = "Jefe";
    size_t key_len = 4;
    char *data = "what do ya want for nothing?";
    size_t data_len = 28;
    char *digest =
      "\x16\x4b\x7a\x7b\xfc\xf8\x19\xe2\xe3\x95\xfb\xe7\x3b\x56\xe0\xa3"
      "\x87\xbd\x64\x22\x2e\x83\x1f\xd6\x10\x27\x0c\xd7\xea\x25\x05\x54"
      "\x97\x58\xbf\x75\xc0\x5a\x99\x4a\x6d\x03\x4f\x65\xf8\xf0\xe6\xfd"
      "\xca\xea\xb1\xa3\x4d\x4a\x6b\x4b\x63\x6e\x07\x0a\x38\xbc\xe7\x37";
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
      "\xfa\x73\xb0\x08\x9d\x56\xa2\x84\xef\xb0\xf0\x75\x6c\x89\x0b\xe9"
      "\xb1\xb5\xdb\xdd\x8e\xe8\x1a\x36\x55\xf8\x3e\x33\xb2\x27\x9d\x39"
      "\xbf\x3e\x84\x82\x79\xa7\x22\xc8\x06\xb4\x85\xa4\x7e\x67\xc8\x07"
      "\xb9\x46\xa3\x37\xbe\xe8\x94\x26\x74\x27\x88\x59\xe1\x32\x92\xfb";
    hmac_check (key, key_len, data, data_len, digest);
  }

  {
    char key[129];
    size_t key_len = sizeof key;
    memset (key, '\x0b', sizeof key);
    char *data = "Hi There";
    size_t data_len = 8;
    char *digest =
      "\xaa\x1c\x23\xfe\x04\x0c\x4f\x3e\x65\x45\xa9\x15\x4e\x33\x9d\x17"
      "\xff\xb5\x27\x2e\x0a\x54\x5b\x84\xd3\x8b\x9b\xf8\xe2\xc7\x46\x4d"
      "\xf2\xd6\x2b\xb5\x00\x05\x57\x68\x6f\x85\x10\xeb\x43\x02\xa0\xca"
      "\xe6\xb5\xdd\x1f\x37\x00\xbe\xae\xde\x75\x5f\x86\xfd\xbe\xb4\x8f";
    hmac_check (key, key_len, data, data_len, digest);
  }

  return 0;
}
