/* Test of u8_v[a]s[n]printf() function.
   Copyright (C) 2007, 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

static void
test_xfunction (uint8_t * (*my_xasprintf) (const char *, ...))
{
  /* Test support of size specifiers as in C99.  */

  {
    uint8_t *result =
      my_xasprintf ("%ju %d", (uintmax_t) 12345671, 33, 44, 55);
    static const uint8_t expected[] = "12345671 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  {
    uint8_t *result =
      my_xasprintf ("%zu %d", (size_t) 12345672, 33, 44, 55);
    static const uint8_t expected[] = "12345672 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  {
    uint8_t *result =
      my_xasprintf ("%tu %d", (ptrdiff_t) 12345673, 33, 44, 55);
    static const uint8_t expected[] = "12345673 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  {
    uint8_t *result =
      my_xasprintf ("%Lg %d", (long double) 1.5, 33, 44, 55);
    static const uint8_t expected[] = "1.5 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the 'U' conversion specifier for Unicode strings.  */

  {
    static const uint8_t unicode_string[] = "Hello";
    {
      uint8_t *result =
        my_xasprintf ("%U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* Width.  */
      uint8_t *result =
        my_xasprintf ("%10U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "     Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      uint8_t *result =
        my_xasprintf ("%-10U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Hello      33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      uint8_t *result =
        my_xasprintf ("%010U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "     Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
  }

  {
    static const uint16_t unicode_string[] = { 'H', 'e', 'l', 'l', 'o', 0 };
    {
      uint8_t *result =
        my_xasprintf ("%lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* Width.  */
      uint8_t *result =
        my_xasprintf ("%10lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "     Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      uint8_t *result =
        my_xasprintf ("%-10lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Hello      33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      uint8_t *result =
        my_xasprintf ("%010lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "     Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
  }

  {
    static const uint32_t unicode_string[] = { 'H', 'e', 'l', 'l', 'o', 0 };
    {
      uint8_t *result =
        my_xasprintf ("%llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* Width.  */
      uint8_t *result =
        my_xasprintf ("%10llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "     Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      uint8_t *result =
        my_xasprintf ("%-10llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Hello      33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      uint8_t *result =
        my_xasprintf ("%010llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "     Hello 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
  }

  /* Test the support of the 's' conversion specifier for strings.  */

  {
    uint8_t *result =
      my_xasprintf ("Mr. %s %d", "Ronald Reagan", 33, 44, 55);
    static const uint8_t expected[] = "Mr. Ronald Reagan 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("Mr. %20s %d", "Ronald Reagan", 33, 44, 55);
    static const uint8_t expected[] = "Mr.        Ronald Reagan 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* FLAG_LEFT.  */
    uint8_t *result =
      my_xasprintf ("Mr. %-20s %d", "Ronald Reagan", 33, 44, 55);
    static const uint8_t expected[] = "Mr. Ronald Reagan        33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* FLAG_ZERO: no effect.  */
    uint8_t *result =
      my_xasprintf ("Mr. %020s %d", "Ronald Reagan", 33, 44, 55);
    static const uint8_t expected[] = "Mr.        Ronald Reagan 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the 'a' and 'A' conversion specifier for hexadecimal
     output of floating-point numbers.  */

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%a %d", 3.1416015625, 33, 44, 55);
    static const uint8_t expected1[] = "0x1.922p+1 33";
    static const uint8_t expected2[] = "0x3.244p+0 33";
    static const uint8_t expected3[] = "0x6.488p-1 33";
    static const uint8_t expected4[] = "0xc.91p-2 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%10a %d", 1.75, 33, 44, 55);
    static const uint8_t expected1[] = "  0x1.cp+0 33";
    static const uint8_t expected2[] = "  0x3.8p-1 33";
    static const uint8_t expected3[] = "    0x7p-2 33";
    static const uint8_t expected4[] = "    0xep-3 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* Small precision.  */
    uint8_t *result =
      my_xasprintf ("%.10a %d", 1.75, 33, 44, 55);
    static const uint8_t expected1[] = "0x1.c000000000p+0 33";
    static const uint8_t expected2[] = "0x3.8000000000p-1 33";
    static const uint8_t expected3[] = "0x7.0000000000p-2 33";
    static const uint8_t expected4[] = "0xe.0000000000p-3 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* Large precision.  */
    uint8_t *result =
      my_xasprintf ("%.50a %d", 1.75, 33, 44, 55);
    static const uint8_t expected1[] = "0x1.c0000000000000000000000000000000000000000000000000p+0 33";
    static const uint8_t expected2[] = "0x3.80000000000000000000000000000000000000000000000000p-1 33";
    static const uint8_t expected3[] = "0x7.00000000000000000000000000000000000000000000000000p-2 33";
    static const uint8_t expected4[] = "0xe.00000000000000000000000000000000000000000000000000p-3 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%La %d", 3.1416015625L, 33, 44, 55);
    static const uint8_t expected1[] = "0x1.922p+1 33";
    static const uint8_t expected2[] = "0x3.244p+0 33";
    static const uint8_t expected3[] = "0x6.488p-1 33";
    static const uint8_t expected4[] = "0xc.91p-2 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%10La %d", 1.75L, 33, 44, 55);
    static const uint8_t expected1[] = "  0x1.cp+0 33";
    static const uint8_t expected2[] = "  0x3.8p-1 33";
    static const uint8_t expected3[] = "    0x7p-2 33";
    static const uint8_t expected4[] = "    0xep-3 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* Small precision.  */
    uint8_t *result =
      my_xasprintf ("%.10La %d", 1.75L, 33, 44, 55);
    static const uint8_t expected1[] = "0x1.c000000000p+0 33";
    static const uint8_t expected2[] = "0x3.8000000000p-1 33";
    static const uint8_t expected3[] = "0x7.0000000000p-2 33";
    static const uint8_t expected4[] = "0xe.0000000000p-3 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  { /* Large precision.  */
    uint8_t *result =
      my_xasprintf ("%.50La %d", 1.75L, 33, 44, 55);
    static const uint8_t expected1[] = "0x1.c0000000000000000000000000000000000000000000000000p+0 33";
    static const uint8_t expected2[] = "0x3.80000000000000000000000000000000000000000000000000p-1 33";
    static const uint8_t expected3[] = "0x7.00000000000000000000000000000000000000000000000000p-2 33";
    static const uint8_t expected4[] = "0xe.00000000000000000000000000000000000000000000000000p-3 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0
            || u8_strcmp (result, expected3) == 0
            || u8_strcmp (result, expected4) == 0);
    free (result);
  }

  /* Test the support of the %f format directive.  */

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%f %d", 12.75, 33, 44, 55);
    static const uint8_t expected[] = "12.750000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%10f %d", 1.75, 33, 44, 55);
    static const uint8_t expected[] = "  1.750000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.f %d", 1234.0, 33, 44, 55);
    static const uint8_t expected[] = "1234 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%Lf %d", 12.75L, 33, 44, 55);
    static const uint8_t expected[] = "12.750000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%10Lf %d", 1.75L, 33, 44, 55);
    static const uint8_t expected[] = "  1.750000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.Lf %d", 1234.0L, 33, 44, 55);
    static const uint8_t expected[] = "1234 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the %F format directive.  */

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%F %d", 12.75, 33, 44, 55);
    static const uint8_t expected[] = "12.750000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.F %d", 1234.0, 33, 44, 55);
    static const uint8_t expected[] = "1234 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%LF %d", 12.75L, 33, 44, 55);
    static const uint8_t expected[] = "12.750000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.LF %d", 1234.0L, 33, 44, 55);
    static const uint8_t expected[] = "1234 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the %e format directive.  */

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%e %d", 12.75, 33, 44, 55);
    static const uint8_t expected1[] = "1.275000e+01 33";
    static const uint8_t expected2[] = "1.275000e+001 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%15e %d", 1.75, 33, 44, 55);
    static const uint8_t expected1[] = "   1.750000e+00 33";
    static const uint8_t expected2[] = "  1.750000e+000 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.e %d", 1234.0, 33, 44, 55);
    static const uint8_t expected1[] = "1e+03 33";
    static const uint8_t expected2[] = "1e+003 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0);
    free (result);
  }

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%Le %d", 12.75L, 33, 44, 55);
    static const uint8_t expected[] = "1.275000e+01 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%15Le %d", 1.75L, 33, 44, 55);
    static const uint8_t expected[] = "   1.750000e+00 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.Le %d", 1234.0L, 33, 44, 55);
    static const uint8_t expected[] = "1e+03 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the %g format directive.  */

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%g %d", 12.75, 33, 44, 55);
    static const uint8_t expected[] = "12.75 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%10g %d", 1.75, 33, 44, 55);
    static const uint8_t expected[] = "      1.75 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.g %d", 1234.0, 33, 44, 55);
    static const uint8_t expected1[] = "1e+03 33";
    static const uint8_t expected2[] = "1e+003 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected1) == 0
            || u8_strcmp (result, expected2) == 0);
    free (result);
  }

  { /* A positive number.  */
    uint8_t *result =
      my_xasprintf ("%Lg %d", 12.75L, 33, 44, 55);
    static const uint8_t expected[] = "12.75 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Width.  */
    uint8_t *result =
      my_xasprintf ("%10Lg %d", 1.75L, 33, 44, 55);
    static const uint8_t expected[] = "      1.75 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  { /* Precision.  */
    uint8_t *result =
      my_xasprintf ("%.Lg %d", 1234.0L, 33, 44, 55);
    static const uint8_t expected[] = "1e+03 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the %n format directive.  */

  {
    int count = -1;
    uint8_t *result =
      my_xasprintf ("%d %n", 123, &count, 33, 44, 55);
    static const uint8_t expected[] = "123 ";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    ASSERT (count == 4);
    free (result);
  }

  /* Test the support of the POSIX/XSI format strings with positions.  */

  {
    uint8_t *result =
      my_xasprintf ("%2$d %1$d", 33, 55);
    static const uint8_t expected[] = "55 33";
    ASSERT (result != NULL);
    ASSERT (u8_strcmp (result, expected) == 0);
    free (result);
  }

  /* Test the support of the grouping flag.  */

  {
    uint8_t *result =
      my_xasprintf ("%'d %d", 1234567, 99);
    ASSERT (result != NULL);
    ASSERT (result[u8_strlen (result) - 1] == '9');
    free (result);
  }

  /* Test the support of the 'U' conversion specifier for Unicode strings.  */

  {
    static const uint8_t unicode_string[] = "Rafa\305\202 Maszkowski"; /* Rafał Maszkowski */
    {
      uint8_t *result =
        my_xasprintf ("%U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* Width.  */
      uint8_t *result =
        my_xasprintf ("%20U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "    Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      uint8_t *result =
        my_xasprintf ("%-20U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Rafa\305\202 Maszkowski     33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      uint8_t *result =
        my_xasprintf ("%020U %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "    Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
  }

  {
    static const uint16_t unicode_string[] = /* Rafał Maszkowski */
      {
        'R', 'a', 'f', 'a', 0x0142, ' ', 'M', 'a', 's', 'z', 'k', 'o', 'w',
        's', 'k', 'i', 0
      };
    {
      uint8_t *result =
        my_xasprintf ("%lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* Width.  */
      uint8_t *result =
        my_xasprintf ("%20lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "    Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      uint8_t *result =
        my_xasprintf ("%-20lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Rafa\305\202 Maszkowski     33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      uint8_t *result =
        my_xasprintf ("%020lU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "    Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
  }

  {
    static const uint32_t unicode_string[] = /* Rafał Maszkowski */
      {
        'R', 'a', 'f', 'a', 0x0142, ' ', 'M', 'a', 's', 'z', 'k', 'o', 'w',
        's', 'k', 'i', 0
      };
    {
      uint8_t *result =
        my_xasprintf ("%llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* Width.  */
      uint8_t *result =
        my_xasprintf ("%20llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "    Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      uint8_t *result =
        my_xasprintf ("%-20llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "Rafa\305\202 Maszkowski     33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      uint8_t *result =
        my_xasprintf ("%020llU %d", unicode_string, 33, 44, 55);
      static const uint8_t expected[] = "    Rafa\305\202 Maszkowski 33";
      ASSERT (result != NULL);
      ASSERT (u8_strcmp (result, expected) == 0);
      free (result);
    }
  }

  /* Test non-ASCII characters in the format string.  */

  {
    uint8_t *result =
      my_xasprintf ("\304rger", 33, 44, 55);
    ASSERT (result == NULL && errno == EINVAL);
  }
}
