/* Test of u8_mbtouc() function.
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

#include "test-u8-mbtouc.h"

static void
test_safe_function (int (*my_u8_mbtouc) (ucs4_t *, const uint8_t *, size_t))
{
  ucs4_t uc;
  int ret;

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
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 6);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 5);
    ASSERT (ret == 1);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 2, 4);
    ASSERT (ret == 1);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 3, 3);
    ASSERT (ret == 1);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 4, 2);
    ASSERT (ret == 1);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 5, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
  /* 3.2. Lonely start characters.  */
  {
    ucs4_t c;
    uint8_t input[2];

    for (c = 0xC0; c <= 0xFF; c++)
      {
        input[0] = c;
        input[1] = ' ';

        uc = 0xBADFACE;
        ret = my_u8_mbtouc (&uc, input, 2);
        ASSERT (ret == 1);
        ASSERT (uc == 0xFFFD);
      }
  }
  /* 3.3. Sequences with last continuation byte missing.  */
  /* 3.3.1. 2-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xC0, '"' };
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 3);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 2);
    ASSERT (ret == 1);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 2, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
  /* 3.3.6. 2-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xDF, '"' };
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 3);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 2);
    ASSERT (ret == 1);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 2, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
  /* 3.3.2. 3-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xE0, 0x80, '"' };
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 4);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 3);
    ASSERT (ret == 2);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 3, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
  /* 3.3.7. 3-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xEF, 0xBF, '"' };
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 4);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 3);
    ASSERT (ret == 2);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 3, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
  /* 3.3.3. 4-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xF0, 0x80, 0x80, '"' };
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 5);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 4);
    ASSERT (ret == 3);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 4, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
  /* 3.3.8. 4-byte sequence with last byte missing.  */
  {
    static const uint8_t input[] = { '"', 0xF7, 0xBF, 0xBF, '"' };
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input, 5);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 1, 4);
    ASSERT (ret == 3);
    ASSERT (uc == 0xFFFD);
    uc = 0xBADFACE;
    ret = my_u8_mbtouc (&uc, input + 4, 1);
    ASSERT (ret == 1);
    ASSERT (uc == 0x0022);
  }
}

int
main ()
{
  test_function (u8_mbtouc);
  test_safe_function (u8_mbtouc);

  return 0;
}
