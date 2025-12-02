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

#include "arctwo.h"

#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  arctwo_context ctx;
  char scratch[16];

  /* Test vectors from Peter Gutmann's paper. */
  static char key_1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  static char plaintext_1[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static const char ciphertext_1[] =
    { 0x1C, 0x19, 0x8A, 0x83, 0x8D, 0xF0, 0x28, 0xB7 };

  static char key_2[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
  };
  static char plaintext_2[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static char ciphertext_2[] =
    { 0x50, 0xDC, 0x01, 0x62, 0xBD, 0x75, 0x7F, 0x31 };

  /* This one was checked against libmcrypt's RFC2268. */
  static char key_3[] = { 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  static char plaintext_3[] =
    { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static char ciphertext_3[] =
    { 0x8f, 0xd1, 0x03, 0x89, 0x33, 0x6b, 0xf9, 0x5e };

  /* From RFC2268. */
  static char key_4[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static char plaintext_4[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static char ciphertext_4[] =
    { 0xeb, 0xb7, 0x73, 0xf9, 0x93, 0x27, 0x8e, 0xff };

  static char key_5[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  static char plaintext_5[] =
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  static char ciphertext_5[] =
    { 0x27, 0x8b, 0x27, 0xe4, 0x2e, 0x2f, 0x0d, 0x49 };

  static char key_6[] = { 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  static char plaintext_6[] =
    { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
  static char ciphertext_6[] =
    { 0x30, 0x64, 0x9e, 0xdf, 0x9b, 0xe7, 0xd2, 0xc2 };

  /* First test. */
  arctwo_setkey_ekb (&ctx, sizeof (key_1), key_1, 0);
  arctwo_encrypt (&ctx, plaintext_1, scratch, ARCTWO_BLOCK_SIZE);

  if (memcmp (scratch, ciphertext_1, sizeof (ciphertext_1)))
    return 1;

  arctwo_setkey_ekb (&ctx, sizeof (key_1), key_1, 0);
  arctwo_decrypt (&ctx, scratch, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, plaintext_1, sizeof (plaintext_1)))
    return 1;

  /* Second test. */
  arctwo_setkey_ekb (&ctx, sizeof (key_2), key_2, 0);
  arctwo_encrypt (&ctx, plaintext_2, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, ciphertext_2, sizeof (ciphertext_2)))
    return 1;

  arctwo_setkey_ekb (&ctx, sizeof (key_2), key_2, 0);
  arctwo_decrypt (&ctx, scratch, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, plaintext_2, sizeof (plaintext_2)))
    return 1;

  /* Third test. */
  arctwo_setkey_ekb (&ctx, sizeof (key_3), key_3, 0);
  arctwo_encrypt (&ctx, plaintext_3, scratch, ARCTWO_BLOCK_SIZE);

  if (memcmp (scratch, ciphertext_3, sizeof (ciphertext_3)))
    return 1;

  arctwo_setkey_ekb (&ctx, sizeof (key_3), key_3, 0);
  arctwo_decrypt (&ctx, scratch, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, plaintext_3, sizeof (plaintext_3)))
    return 1;

  /* Fourth test. */
  arctwo_setkey_ekb (&ctx, sizeof (key_4), key_4, 63);
  arctwo_encrypt (&ctx, plaintext_4, scratch, ARCTWO_BLOCK_SIZE);

  if (memcmp (scratch, ciphertext_4, sizeof (ciphertext_4)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < sizeof (ciphertext_4); i++)
        printf ("%02x ", ciphertext_4[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < sizeof (ciphertext_4); i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  arctwo_setkey_ekb (&ctx, sizeof (key_4), key_4, 63);
  arctwo_decrypt (&ctx, scratch, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, plaintext_4, sizeof (plaintext_4)))
    return 1;

  /* Fifth test. */
  arctwo_setkey_ekb (&ctx, sizeof (key_5), key_5, 64);
  arctwo_encrypt (&ctx, plaintext_5, scratch, ARCTWO_BLOCK_SIZE);

  if (memcmp (scratch, ciphertext_5, sizeof (ciphertext_5)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < sizeof (ciphertext_5); i++)
        printf ("%02x ", ciphertext_5[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < sizeof (ciphertext_5); i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  arctwo_setkey_ekb (&ctx, sizeof (key_5), key_5, 64);
  arctwo_decrypt (&ctx, scratch, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, plaintext_5, sizeof (plaintext_5)))
    return 1;

  /* Sixth test. */
  arctwo_setkey_ekb (&ctx, 8, key_6, 64);
  arctwo_encrypt (&ctx, plaintext_6, scratch, ARCTWO_BLOCK_SIZE);

  if (memcmp (scratch, ciphertext_6, sizeof (ciphertext_6)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < sizeof (ciphertext_6); i++)
        printf ("%02x ", ciphertext_6[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < sizeof (ciphertext_6); i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  arctwo_setkey_ekb (&ctx, sizeof (key_6), key_6, 64);
  arctwo_decrypt (&ctx, scratch, scratch, ARCTWO_BLOCK_SIZE);
  if (memcmp (scratch, plaintext_6, sizeof (plaintext_6)))
    return 1;

  return 0;
}
