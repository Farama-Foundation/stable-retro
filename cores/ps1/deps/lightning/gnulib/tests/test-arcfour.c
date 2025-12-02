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

#include "arcfour.h"

#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  arcfour_context ctx;
  /* Test vector from Cryptlib via Libgcrypt labeled there: "from the
     State/Commerce Department". */
  static char key_1[] = { 0x61, 0x8A, 0x63, 0xD2, 0xFB };
  static char plaintext_1[] = { 0xDC, 0xEE, 0x4C, 0xF9, 0x2C };
  static const char ciphertext_1[] = { 0xF1, 0x38, 0x29, 0xC9, 0xDE };
  char scratch[16];

  arcfour_setkey (&ctx, key_1, sizeof (key_1));
  arcfour_stream (&ctx, plaintext_1, scratch, sizeof (plaintext_1));
  if (memcmp (scratch, ciphertext_1, sizeof (ciphertext_1)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", ciphertext_1[i] & 0xFF);
      printf ("\n");
      return 1;
    }

  /* decrypt */

  arcfour_setkey (&ctx, key_1, sizeof (key_1));
  arcfour_stream (&ctx, scratch, scratch, sizeof (plaintext_1));
  if (memcmp (scratch, plaintext_1, sizeof (plaintext_1)))
    {
      size_t i;
      printf ("expected:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", plaintext_1[i] & 0xFF);
      printf ("\ncomputed:\n");
      for (i = 0; i < 5; i++)
        printf ("%02x ", scratch[i] & 0xFF);
      printf ("\n");
      return 1;
    }


  return 0;
}
