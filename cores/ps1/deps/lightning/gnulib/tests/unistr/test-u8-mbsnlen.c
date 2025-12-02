/* Test of u8_mbsnlen() function.
   Copyright (C) 2010-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include "unistr.h"

#include "macros.h"

int
main ()
{
  /* Simple string.  */
  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const uint8_t input[] =
      { 'G', 'r', 0xC3, 0xBC, 0xC3, 0x9F, ' ', 'G', 'o', 't', 't', '.', ' ',
        0xD0, 0x97, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1, 0x81,
        0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB9, 0xD1, 0x82, 0xD0, 0xB5,
        '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1, 's', 'q', 'r', 't', '(',
        'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
      };
    static const size_t expected[SIZEOF (input) + 1] =
      { 0,
        1, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11,
        12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
        18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23,
        24, 25, 26, 27, 28, 29, 30, 31, 31, 32, 33, 34, 35, 36,
        37, 38, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 52, 52, 53, 53, 53, 54, 54, 54, 55,
        56, 56, 56, 57, 57, 57, 58,
        59, 59, 59, 60, 60, 60, 61
      };
    size_t n;

    for (n = 0; n <= SIZEOF (input); n++)
      {
        size_t len = u8_mbsnlen (input, n);
        ASSERT (len == expected[n]);
      }
  }

  /* Test behaviour required by ISO 10646-1, sections R.7 and 2.3c, namely,
     that a "malformed sequence" is interpreted in the same way as
     "a character that is outside the adopted subset".
     Reference:
       Markus Kuhn: UTF-8 decoder capability and stress test
       <https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt>
       <https://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html>
   */
  /* 3.1. Test that each unexpected continuation byte is signalled as a
     malformed sequence of its own.  */
  {
    static const uint8_t input[] = { '"', 0x80, 0xBF, 0x80, 0xBF, '"' };
    ASSERT (u8_mbsnlen (input, 6) == 6);
  }
  /* 3.2. Lonely start characters.  */
  {
    ucs4_t c;
    uint8_t input[2];

    for (c = 0xC0; c <= 0xFF; c++)
      {
        input[0] = c;
        input[1] = ' ';

        ASSERT (u8_mbsnlen (input, 2) == 2);
      }
  }
  /* 3.3. Sequences with last continuation byte missing.  */
  /* 3.3.1. 2-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xC0, '"' };
    ASSERT (u8_mbsnlen (input, 3) == 3);
  }
  /* 3.3.6. 2-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xDF, '"' };
    ASSERT (u8_mbsnlen (input, 3) == 3);
  }
  /* 3.3.2. 3-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xE0, 0x80, '"' };
    ASSERT (u8_mbsnlen (input, 4) == 3);
  }
  /* 3.3.7. 3-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xEF, 0xBF, '"' };
    ASSERT (u8_mbsnlen (input, 4) == 3);
  }
  /* 3.3.3. 4-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xF0, 0x80, 0x80, '"' };
    ASSERT (u8_mbsnlen (input, 5) == 3);
  }
  /* 3.3.8. 4-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xF7, 0xBF, 0xBF, '"' };
    ASSERT (u8_mbsnlen (input, 5) == 3);
  }

  return 0;
}
