/* Test of decomposition of Unicode characters.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "uninorm.h"

#include "macros.h"

int
main ()
{
  ucs4_t decomposed[UC_DECOMPOSITION_MAX_LENGTH];
  int tag;
  int ret;

  /* SPACE */
  ret = uc_decomposition (0x0020, &tag, decomposed);
  ASSERT (ret == -1);

  /* LATIN CAPITAL LETTER A WITH DIAERESIS */
  ret = uc_decomposition (0x00C4, &tag, decomposed);
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_CANONICAL);
  ASSERT (decomposed[0] == 0x0041);
  ASSERT (decomposed[1] == 0x0308);

  /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
  ret = uc_decomposition (0x01DE, &tag, decomposed);
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_CANONICAL);
  ASSERT (decomposed[0] == 0x00C4);
  ASSERT (decomposed[1] == 0x0304);

  /* GREEK DIALYTIKA AND PERISPOMENI */
  ret = uc_decomposition (0x1FC1, &tag, decomposed);
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_CANONICAL);
  ASSERT (decomposed[0] == 0x00A8);
  ASSERT (decomposed[1] == 0x0342);

  /* SCRIPT SMALL L */
  ret = uc_decomposition (0x2113, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_FONT);
  ASSERT (decomposed[0] == 0x006C);

  /* NO-BREAK SPACE */
  ret = uc_decomposition (0x00A0, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_NOBREAK);
  ASSERT (decomposed[0] == 0x0020);

  /* ARABIC LETTER VEH INITIAL FORM */
  ret = uc_decomposition (0xFB6C, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_INITIAL);
  ASSERT (decomposed[0] == 0x06A4);

  /* ARABIC LETTER VEH MEDIAL FORM */
  ret = uc_decomposition (0xFB6D, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_MEDIAL);
  ASSERT (decomposed[0] == 0x06A4);

  /* ARABIC LETTER VEH FINAL FORM */
  ret = uc_decomposition (0xFB6B, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_FINAL);
  ASSERT (decomposed[0] == 0x06A4);

  /* ARABIC LETTER VEH ISOLATED FORM */
  ret = uc_decomposition (0xFB6A, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_ISOLATED);
  ASSERT (decomposed[0] == 0x06A4);

  /* CIRCLED NUMBER FIFTEEN */
  ret = uc_decomposition (0x246E, &tag, decomposed);
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_CIRCLE);
  ASSERT (decomposed[0] == 0x0031);
  ASSERT (decomposed[1] == 0x0035);

  /* TRADE MARK SIGN */
  ret = uc_decomposition (0x2122, &tag, decomposed);
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_SUPER);
  ASSERT (decomposed[0] == 0x0054);
  ASSERT (decomposed[1] == 0x004D);

  /* LATIN SUBSCRIPT SMALL LETTER I */
  ret = uc_decomposition (0x1D62, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_SUB);
  ASSERT (decomposed[0] == 0x0069);

  /* PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS */
  ret = uc_decomposition (0xFE35, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_VERTICAL);
  ASSERT (decomposed[0] == 0x0028);

  /* FULLWIDTH LATIN CAPITAL LETTER A */
  ret = uc_decomposition (0xFF21, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_WIDE);
  ASSERT (decomposed[0] == 0x0041);

  /* HALFWIDTH IDEOGRAPHIC COMMA */
  ret = uc_decomposition (0xFF64, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_NARROW);
  ASSERT (decomposed[0] == 0x3001);

  /* SMALL IDEOGRAPHIC COMMA */
  ret = uc_decomposition (0xFE51, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_SMALL);
  ASSERT (decomposed[0] == 0x3001);

  /* SQUARE MHZ */
  ret = uc_decomposition (0x3392, &tag, decomposed);
  ASSERT (ret == 3);
  ASSERT (tag == UC_DECOMP_SQUARE);
  ASSERT (decomposed[0] == 0x004D);
  ASSERT (decomposed[1] == 0x0048);
  ASSERT (decomposed[2] == 0x007A);

  /* VULGAR FRACTION THREE EIGHTHS */
  ret = uc_decomposition (0x215C, &tag, decomposed);
  ASSERT (ret == 3);
  ASSERT (tag == UC_DECOMP_FRACTION);
  ASSERT (decomposed[0] == 0x0033);
  ASSERT (decomposed[1] == 0x2044);
  ASSERT (decomposed[2] == 0x0038);

  /* MICRO SIGN */
  ret = uc_decomposition (0x00B5, &tag, decomposed);
  ASSERT (ret == 1);
  ASSERT (tag == UC_DECOMP_COMPAT);
  ASSERT (decomposed[0] == 0x03BC);

  /* ARABIC LIGATURE SALLALLAHOU ALAYHE WASALLAM */
  ret = uc_decomposition (0xFDFA, &tag, decomposed);
  ASSERT (ret == 18);
  ASSERT (tag == UC_DECOMP_ISOLATED);
  ASSERT (decomposed[0] == 0x0635);
  ASSERT (decomposed[1] == 0x0644);
  ASSERT (decomposed[2] == 0x0649);
  ASSERT (decomposed[3] == 0x0020);
  ASSERT (decomposed[4] == 0x0627);
  ASSERT (decomposed[5] == 0x0644);
  ASSERT (decomposed[6] == 0x0644);
  ASSERT (decomposed[7] == 0x0647);
  ASSERT (decomposed[8] == 0x0020);
  ASSERT (decomposed[9] == 0x0639);
  ASSERT (decomposed[10] == 0x0644);
  ASSERT (decomposed[11] == 0x064A);
  ASSERT (decomposed[12] == 0x0647);
  ASSERT (decomposed[13] == 0x0020);
  ASSERT (decomposed[14] == 0x0648);
  ASSERT (decomposed[15] == 0x0633);
  ASSERT (decomposed[16] == 0x0644);
  ASSERT (decomposed[17] == 0x0645);

  /* HANGUL SYLLABLE GEUL */
  ret = uc_decomposition (0xAE00, &tag, decomposed);
  /* See the clarification at <https://www.unicode.org/versions/Unicode5.1.0/>,
     section "Clarification of Hangul Jamo Handling".  */
#if 1
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_CANONICAL);
  ASSERT (decomposed[0] == 0xADF8);
  ASSERT (decomposed[1] == 0x11AF);
#else
  ASSERT (ret == 3);
  ASSERT (tag == UC_DECOMP_CANONICAL);
  ASSERT (decomposed[0] == 0x1100);
  ASSERT (decomposed[1] == 0x1173);
  ASSERT (decomposed[2] == 0x11AF);
#endif

  /* HANGUL SYLLABLE GEU */
  ret = uc_decomposition (0xADF8, &tag, decomposed);
  ASSERT (ret == 2);
  ASSERT (tag == UC_DECOMP_CANONICAL);
  ASSERT (decomposed[0] == 0x1100);
  ASSERT (decomposed[1] == 0x1173);

  return 0;
}
