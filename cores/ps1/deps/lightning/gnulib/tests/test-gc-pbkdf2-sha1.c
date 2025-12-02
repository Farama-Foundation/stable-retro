/*
 * Copyright (C) 2002-2005, 2007, 2010-2021 Free Software Foundation, Inc.
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

/* Written by Simon Josefsson.  */

#include <config.h>

#include "gc.h"

#include <stdio.h>
#include <string.h>

/* Test vectors from RFC 3962. */

#define G_CLEF "\xF0\x9D\x84\x9E"

struct pkcs5
{
  int iterations;
  const char *password;
  const char *salt;
  int dklen;
  const char *expected;
};
const struct pkcs5 pkcs5[] = {
  {1, "password", "ATHENA.MIT.EDUraeburn", 16,
   "\xCD\xED\xB5\x28\x1B\xB2\xF8\x01\x56\x5A\x11\x22\xB2\x56\x35\x15"},
  {2, "password", "ATHENA.MIT.EDUraeburn", 16,
   "\x01\xdb\xee\x7f\x4a\x9e\x24\x3e\x98\x8b\x62\xc7\x3c\xda\x93\x5d"},
  {2, "password", "ATHENA.MIT.EDUraeburn", 32,
   "\x01\xdb\xee\x7f\x4a\x9e\x24\x3e\x98\x8b\x62\xc7\x3c\xda\x93\x5d"
   "\xa0\x53\x78\xb9\x32\x44\xec\x8f\x48\xa9\x9e\x61\xad\x79\x9d\x86"},
  {1200, "password", "ATHENA.MIT.EDUraeburn", 16,
   "\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b"},
  {1200, "password", "ATHENA.MIT.EDUraeburn", 32,
   "\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b"
   "\xa7\xe5\x2d\xdb\xc5\xe5\x14\x2f\x70\x8a\x31\xe2\xe6\x2b\x1e\x13"},
  {5, "password", "\x12\x34\x56\x78\x78\x56\x34\x12\x00", 16,
   "\xd1\xda\xa7\x86\x15\xf2\x87\xe6\xa1\xc8\xb1\x20\xd7\x06\x2a\x49"},
  {5, "password", "\x12\x34\x56\x78\x78\x56\x34\x12\x00", 32,
   "\xd1\xda\xa7\x86\x15\xf2\x87\xe6\xa1\xc8\xb1\x20\xd7\x06\x2a\x49"
   "\x3f\x98\xd2\x03\xe6\xbe\x49\xa6\xad\xf4\xfa\x57\x4b\x6e\x64\xee"},
  {1200, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
   "pass phrase equals block size", 16,
   "\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9"},
  {1200, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
   "pass phrase equals block size", 32,
   "\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9"
   "\xc5\xec\x59\xf1\xa4\x52\xf5\xcc\x9a\xd9\x40\xfe\xa0\x59\x8e\xd1"},
  {1200, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
   "pass phrase exceeds block size", 16,
   "\x9c\xca\xd6\xd4\x68\x77\x0c\xd5\x1b\x10\xe6\xa6\x87\x21\xbe\x61"},
  {1200, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
   "pass phrase exceeds block size", 32,
   "\x9c\xca\xd6\xd4\x68\x77\x0c\xd5\x1b\x10\xe6\xa6\x87\x21\xbe\x61"
   "\x1a\x8b\x4d\x28\x26\x01\xdb\x3b\x36\xbe\x92\x46\x91\x5e\xc8\x2a"},
  {50, G_CLEF "\x00", "EXAMPLE.COMpianist", 16,
   "\x6b\x9c\xf2\x6d\x45\x45\x5a\x43\xa5\xb8\xbb\x27\x6a\x40\x3b\x39"},
  {50, G_CLEF "\x00", "EXAMPLE.COMpianist", 32,
   "\x6b\x9c\xf2\x6d\x45\x45\x5a\x43\xa5\xb8\xbb\x27\x6a\x40\x3b\x39"
   "\xe7\xfe\x37\xa0\xc4\x1e\x02\xc2\x81\xff\x30\x69\xe1\xe9\x4f\x52"},
  {500, "All n-entities must communicate with other n-entities via n-1 "
   "entiteeheehees", "\x12\x34\x56\x78\x78\x56\x34\x12\x00", 16,
   "\x6A\x89\x70\xBF\x68\xC9\x2C\xAE\xA8\x4A\x8D\xF2\x85\x10\x85\x86"}
};

int
main (int argc, char *argv[])
{
  size_t i;
  int rc;
  char out[BUFSIZ];

  for (i = 0; i < sizeof (pkcs5) / sizeof (pkcs5[0]); i++)
    {
      rc = gc_pbkdf2_sha1 (pkcs5[i].password, strlen (pkcs5[i].password),
                           pkcs5[i].salt, strlen (pkcs5[i].salt),
                           pkcs5[i].iterations, out, pkcs5[i].dklen);
      if (rc != GC_OK)
        {
          printf ("PKCS5 entry %ld failed fatally: %d\n",
                  (unsigned long) i, rc);
          return 1;
        }

      if (memcmp (pkcs5[i].expected, out, pkcs5[i].dklen) != 0)
        {
          printf ("PKCS5 entry %ld failed\n", (unsigned long) i);
          return 1;
        }
    }

  return 0;
}
