/*
 * Copyright (C) 2005, 2010-2021 Free Software Foundation, Inc.
 * Written by Simon Josefsson
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

#include <config.h>

#include "rijndael-api-fst.h"

#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  int rc;
  rijndaelKeyInstance key;
  rijndaelCipherInstance cipher;
  char in[RIJNDAEL_BITSPERBLOCK / 8];
  char out[RIJNDAEL_BITSPERBLOCK / 8];
  char pt[] = "\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00";
  char ct[] = "\xC3\x4C\x05\x2C\xC0\xDA\x8D\x73"
    "\x45\x1A\xFE\x5F\x03\xBE\x29\x7F";
  size_t i;

  rc = rijndaelMakeKey (&key, RIJNDAEL_DIR_ENCRYPT,
                        128, "00000000000000000000000000000000");
  if (rc != 0)
    printf ("makeKey failed %d\n", rc);

  rc = rijndaelCipherInit (&cipher, RIJNDAEL_MODE_ECB, NULL);
  if (rc != 0)
    printf ("cipherInit failed %d\n", rc);

  memset (in, 0, RIJNDAEL_BITSPERBLOCK / 8);

  for (i = 0; i < 10000; i++)
    {
      rc = rijndaelBlockEncrypt (&cipher, &key, in, 128, out);
      if (rc < 0)
        printf ("blockEncrypt failed %d\n", rc);

      memcpy (in, out, RIJNDAEL_BITSPERBLOCK / 8);
    }

  if (memcmp (out, ct, RIJNDAEL_BITSPERBLOCK / 8) != 0)
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < RIJNDAEL_BITSPERBLOCK / 8; i++)
        printf ("%02x ", ct[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < RIJNDAEL_BITSPERBLOCK / 8; i++)
        printf ("%02x ", out[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  rc = rijndaelMakeKey (&key, RIJNDAEL_DIR_DECRYPT,
                        128, "00000000000000000000000000000000");
  if (rc != 0)
    printf ("makeKey failed %d\n", rc);

  rc = rijndaelCipherInit (&cipher, RIJNDAEL_MODE_ECB, NULL);
  if (rc != 0)
    printf ("cipherInit failed %d\n", rc);

  for (i = 0; i < 10000; i++)
    {
      memcpy (in, out, RIJNDAEL_BITSPERBLOCK / 8);

      rc = rijndaelBlockDecrypt (&cipher, &key, in, 128, out);
      if (rc < 0)
        printf ("blockEncrypt failed %d\n", rc);
    }

  if (memcmp (out, pt, RIJNDAEL_BITSPERBLOCK / 8) != 0)
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < RIJNDAEL_BITSPERBLOCK / 8; i++)
        printf ("%02x ", pt[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < RIJNDAEL_BITSPERBLOCK / 8; i++)
        printf ("%02x ", out[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  return 0;
}
