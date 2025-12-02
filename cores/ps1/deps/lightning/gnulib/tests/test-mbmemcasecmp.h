/* Test of case-insensitive memory area comparison function.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

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

static void
test_ascii (int (*my_casecmp) (const char *, size_t, const char *, size_t))
{
  /* Empty string.  */
  {
    ASSERT (my_casecmp (NULL, 0, NULL, 0) == 0);
  }
  {
    static const char input[] = { 'x', 'y' };

    ASSERT (my_casecmp (input, SIZEOF (input), NULL, 0) > 0);

    ASSERT (my_casecmp (NULL, 0, input, SIZEOF (input)) < 0);

    ASSERT (my_casecmp (input, SIZEOF (input), input, SIZEOF (input)) == 0);
  }

  /* Normal lexicographic order.  */
  {
    static const char input1[] = { 'A', 'm', 'e', 'r', 'i', 'c', 'a' };
    static const char input2[] = { 'A', 'm', 'i', 'g', 'o' };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) < 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1)) > 0);
  }

  /* Shorter and longer strings.  */
  {
    static const char input1[] = { 'R', 'e', 'a', 'g', 'a', 'n' };
    static const char input2[] = { 'R', 'e', 'a', 'g', 'a', 'n', 'o', 'm', 'i', 'c', 's' };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) < 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1)) > 0);
  }

  ASSERT (my_casecmp ("paragraph", 9, "Paragraph", 9) == 0);

  ASSERT (my_casecmp ("paragrapH", 9, "parAgRaph", 9) == 0);

  ASSERT (my_casecmp ("paragraph", 9, "paraLyzed", 9) < 0);
  ASSERT (my_casecmp ("paraLyzed", 9, "paragraph", 9) > 0);

  ASSERT (my_casecmp ("para", 4, "paragraph", 9) < 0);
  ASSERT (my_casecmp ("paragraph", 9, "para", 4) > 0);

  /* Embedded NULs.  */
  ASSERT (my_casecmp ("1\0", 2, "2\0", 2) < 0);
  ASSERT (my_casecmp ("2\0", 2, "1\0", 2) > 0);
  ASSERT (my_casecmp ("x\0""1", 3, "x\0""2", 3) < 0);
  ASSERT (my_casecmp ("x\0""2", 3, "x\0""1", 3) > 0);
}

static void
test_iso_8859_1 (int (*my_casecmp) (const char *, size_t, const char *, size_t), bool assume_byte_values)
{
  #if ! defined __osf__ /* This test fails on OSF/1 5.1.  */
  {
    static const char input1[] = { 'H', 0xF6, 'h', 'l', 'e' };
    static const char input2[] = { 'H', 0xD6, 'h', 'L', 'e' };
    static const char input3[] = { 'H', 0xF6, 'h', 'l', 'e', 'n' };
    static const char input4[] = { 'H', 0xD6, 'h', 'L', 'e', 'n' };
    static const char input5[] = { 'H', 'u', 'r', 'z' };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1)) == 0);

    ASSERT (my_casecmp (input3, SIZEOF (input3), input4, SIZEOF (input4)) == 0);

    ASSERT (my_casecmp (input4, SIZEOF (input4), input3, SIZEOF (input3)) == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3)) < 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input4, SIZEOF (input4)) < 0);

    if (assume_byte_values)
      {
        /* These results are the opposite of those in test-ulc-casecmp.c,
           because my_casecmp compares precomposed characters, whereas
           ulc_casecmp compares decomposed character sequences.  */

        ASSERT (my_casecmp (input1, SIZEOF (input1), input5, SIZEOF (input5)) > 0);

        ASSERT (my_casecmp (input2, SIZEOF (input2), input5, SIZEOF (input5)) > 0);
      }
  }
  #endif

  #if 0 /* This functionality requires ulc_casecmp.  */
  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "heiß" */
    static const char input1[] = { 0x68, 0x65, 0x69, 0xDF };
    static const char input2[] = { 0x68, 0x65, 0x69, 0x73, 0x73 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  #endif
}

static void
test_utf_8 (int (*my_casecmp) (const char *, size_t, const char *, size_t), bool turkish)
{
  /* The following tests shows how my_casecmp() is different from
     strcasecmp().  */

  ASSERT (my_casecmp ("\303\266zg\303\274r", 7, "\303\226ZG\303\234R", 7) == 0); /* özgür */
  ASSERT (my_casecmp ("\303\226ZG\303\234R", 7, "\303\266zg\303\274r", 7) == 0); /* özgür */

  /* This test shows how strings of different size can compare equal.  */
  if (turkish)
    {
      ASSERT (my_casecmp ("turkish", 7, "TURK\304\260SH", 8) == 0);
      ASSERT (my_casecmp ("TURK\304\260SH", 8, "turkish", 7) == 0);
    }

  #if 0 /* This functionality requires ulc_casecmp.  */
  /* Normalization effects.  */
  {
    static const char input1[] = { 'H', 0xC3, 0xB6, 'h', 'l', 'e' };
    static const char input2[] = { 'H', 'O', 0xCC, 0x88, 'h', 'L', 'e' };
    static const char input3[] = { 'H', 0xC3, 0xB6, 'h', 'l', 'e', 'n' };
    static const char input4[] = { 'H', 'O', 0xCC, 0x88, 'h', 'L', 'e', 'n' };
    static const char input5[] = { 'H', 'u', 'r', 'z' };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input1, SIZEOF (input1)) == 0);

    ASSERT (my_casecmp (input3, SIZEOF (input3), input4, SIZEOF (input4)) == 0);

    ASSERT (my_casecmp (input4, SIZEOF (input4), input3, SIZEOF (input3)) == 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3)) < 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input4, SIZEOF (input4)) < 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input5, SIZEOF (input5)) < 0);

    ASSERT (my_casecmp (input2, SIZEOF (input2), input5, SIZEOF (input5)) < 0);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    static const char input1[] = { 0xC3, 0x84 };
    static const char input2[] = { 0x41, 0xCC, 0x88 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  { /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    static const char input1[] = { 0xC7, 0x9E };
    static const char input2[] = { 0x41, 0xCC, 0x88, 0xCC, 0x84 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  { /* GREEK DIALYTIKA AND PERISPOMENI */
    static const char input1[] = { 0xE1, 0xBF, 0x81 };
    static const char input2[] = { 0xC2, 0xA8, 0xCD, 0x82 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  { /* HANGUL SYLLABLE GEUL */
    static const char input1[] = { 0xEA, 0xB8, 0x80 };
    static const char input2[] = { 0xEA, 0xB7, 0xB8, 0xE1, 0x86, 0xAF };
    static const char input3[] = { 0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3, 0xE1, 0x86, 0xAF };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3)) == 0);
  }
  { /* HANGUL SYLLABLE GEU */
    static const char input1[] = { 0xEA, 0xB7, 0xB8 };
    static const char input2[] = { 0xE1, 0x84, 0x80, 0xE1, 0x85, 0xB3 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  #endif

  /* Simple string.  */
  { /* "Grüß Gott. Здравствуйте! x=(-b±sqrt(b²-4ac))/(2a)  日本語,中文,한글" */
    static const char input1[] =
      { 'G', 'r', 0xC3, 0xBC, 0xC3, 0x9F, ' ', 'G', 'o', 't', 't', '.', ' ',
        0xD0, 0x97, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1, 0x81,
        0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB9, 0xD1, 0x82, 0xD0, 0xB5,
        '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1, 's', 'q', 'r', 't', '(',
        'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
      };
    static const char input2[] =
      { 'g', 'r', 0xC3, 0xBC, 0x73, 0x73, ' ', 'g', 'o', 't', 't', '.', ' ',
        0xD0, 0xB7, 0xD0, 0xB4, 0xD1, 0x80, 0xD0, 0xB0, 0xD0, 0xB2, 0xD1, 0x81,
        0xD1, 0x82, 0xD0, 0xB2, 0xD1, 0x83, 0xD0, 0xB9, 0xD1, 0x82, 0xD0, 0xB5,
        '!', ' ', 'x', '=', '(', '-', 'b', 0xC2, 0xB1, 's', 'q', 'r', 't', '(',
        'b', 0xC2, 0xB2, '-', '4', 'a', 'c', ')', ')', '/', '(', '2', 'a', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
      };
    static const char input3[] =
      { 'G', 'R', 0xC3, 0x9C, 0x53, 0x53, ' ', 'G', 'O', 'T', 'T', '.', ' ',
        0xD0, 0x97, 0xD0, 0x94, 0xD0, 0xA0, 0xD0, 0x90, 0xD0, 0x92, 0xD0, 0xA1,
        0xD0, 0xA2, 0xD0, 0x92, 0xD0, 0xA3, 0xD0, 0x99, 0xD0, 0xA2, 0xD0, 0x95,
        '!', ' ', 'X', '=', '(', '-', 'B', 0xC2, 0xB1, 'S', 'Q', 'R', 'T', '(',
        'B', 0xC2, 0xB2, '-', '4', 'A', 'C', ')', ')', '/', '(', '2', 'A', ')',
        ' ', ' ', 0xE6, 0x97, 0xA5, 0xE6, 0x9C, 0xAC, 0xE8, 0xAA, 0x9E, ',',
        0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87, ',',
        0xED, 0x95, 0x9C, 0xEA, 0xB8, 0x80, '\n'
      };

    (void) input1;

    #if 0 /* This functionality requires ulc_casecmp.  */
    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3)) == 0);
    #endif

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3)) == 0);
  }

  #if 0 /* This functionality requires ulc_casecmp.  */
  /* Case mapping can increase the number of Unicode characters.  */
  { /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    static const char input1[] = { 0xC5, 0x89 };
    static const char input2[] = { 0xCA, 0xBC, 0x6E };
    static const char input3[] = { 0xCA, 0xBC, 0x4E };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3)) == 0);
  }
  { /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    static const char input1[] = { 0xCE, 0x90 };
    static const char input2[] = { 0xCE, 0xB9, 0xCC, 0x88, 0xCC, 0x81 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  #endif

  /* Turkish letters i İ ı I */
  { /* LATIN CAPITAL LETTER I */
    static const char input[]         = { 0x49 };
    static const char casefolded[]    = { 0x69 };
    static const char casefolded_tr[] = { 0xC4, 0xB1 };

    if (!turkish)
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);
    else
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr)) == 0);
  }
  { /* LATIN SMALL LETTER I */
    static const char input[]         = { 0x69 };
    static const char casefolded[]    = { 0x49 };
    static const char casefolded_tr[] = { 0xC4, 0xB0 };

    if (!turkish)
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);
    else
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr)) == 0);
  }
  { /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    static const char input[]         = { 0xC4, 0xB0 };
    static const char casefolded[]    = { 0x69, 0xCC, 0x87 };
    static const char casefolded_tr[] = { 0x69 };

    (void) casefolded;

    if (!turkish)
      {
        #if 0 /* This functionality requires ulc_casecmp.  */
        ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);
        #endif
      }
    else
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded_tr, SIZEOF (casefolded_tr)) == 0);
  }
  { /* LATIN SMALL LETTER DOTLESS I */
    static const char input[]      = { 0xC4, 0xB1 };
    static const char casefolded[] = { 0x49 };

    if (!turkish)
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) > 0);
    else
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);
  }
  { /* "topkapı" */
    static const char input[] =
      { 0x54, 0x4F, 0x50, 0x4B, 0x41, 0x50, 0x49 };
    static const char casefolded[] =
      { 0x74, 0x6F, 0x70, 0x6B, 0x61, 0x70, 0xC4, 0xB1 };

    if (!turkish)
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) < 0);
    else
      ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);
  }

  #if 0 /* This functionality requires ulc_casecmp.  */
  /* Uppercasing can increase the number of Unicode characters.  */
  { /* "heiß" */
    static const char input1[] = { 0x68, 0x65, 0x69, 0xC3, 0x9F };
    static const char input2[] = { 0x68, 0x65, 0x69, 0x73, 0x73 };

    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);
  }
  #endif

  /* Case mappings for some characters can depend on the surrounding characters.  */
  { /* "περισσότερες πληροφορίες" */
    static const char input1[] =
      {
        0xCF, 0x80, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB9, 0xCF, 0x83, 0xCF, 0x83,
        0xCF, 0x8C, 0xCF, 0x84, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB5, 0xCF, 0x82,
        ' ', 0xCF, 0x80, 0xCE, 0xBB, 0xCE, 0xB7, 0xCF, 0x81, 0xCE, 0xBF,
        0xCF, 0x86, 0xCE, 0xBF, 0xCF, 0x81, 0xCE, 0xAF, 0xCE, 0xB5, 0xCF, 0x82
      };
    static const char input2[] =
      {
        0xCF, 0x80, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB9, 0xCF, 0x83, 0xCF, 0x83,
        0xCF, 0x8C, 0xCF, 0x84, 0xCE, 0xB5, 0xCF, 0x81, 0xCE, 0xB5, 0xCF, 0x83,
        ' ', 0xCF, 0x80, 0xCE, 0xBB, 0xCE, 0xB7, 0xCF, 0x81, 0xCE, 0xBF,
        0xCF, 0x86, 0xCE, 0xBF, 0xCF, 0x81, 0xCE, 0xAF, 0xCE, 0xB5, 0xCF, 0x83
      };
    static const char input3[] =
      {
        0xCE, 0xA0, 0xCE, 0x95, 0xCE, 0xA1, 0xCE, 0x99, 0xCE, 0xA3, 0xCE, 0xA3,
        0xCE, 0x8C, 0xCE, 0xA4, 0xCE, 0x95, 0xCE, 0xA1, 0xCE, 0x95, 0xCE, 0xA3,
        ' ', 0xCE, 0xA0, 0xCE, 0x9B, 0xCE, 0x97, 0xCE, 0xA1, 0xCE, 0x9F,
        0xCE, 0xA6, 0xCE, 0x9F, 0xCE, 0xA1, 0xCE, 0x8A, 0xCE, 0x95, 0xCE, 0xA3
      };

    (void) input1;

    #if 0 /* This functionality requires ulc_casecmp.  */
    ASSERT (my_casecmp (input1, SIZEOF (input1), input2, SIZEOF (input2)) == 0);

    ASSERT (my_casecmp (input1, SIZEOF (input1), input3, SIZEOF (input3)) == 0);
    #endif

    ASSERT (my_casecmp (input2, SIZEOF (input2), input3, SIZEOF (input3)) == 0);
  }

  #if 0 /* This functionality requires ulc_casecmp.  */
  /* Case mapping can require subsequent normalization.  */
  { /* LATIN SMALL LETTER J WITH CARON, COMBINING DOT BELOW */
    static const char input[]                 = { 0xC7, 0xB0, 0xCC, 0xA3 };
    static const char casefolded[]            = { 0x6A, 0xCC, 0x8C, 0xCC, 0xA3 };
    static const char casefolded_decomposed[] = { 0x6A, 0xCC, 0xA3, 0xCC, 0x8C };

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_decomposed, SIZEOF (casefolded_decomposed)) != 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded, SIZEOF (casefolded)) == 0);

    ASSERT (my_casecmp (input, SIZEOF (input), casefolded_decomposed, SIZEOF (casefolded_decomposed)) == 0);
  }
  #endif
}
