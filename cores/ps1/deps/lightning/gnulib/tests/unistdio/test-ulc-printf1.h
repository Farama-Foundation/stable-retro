/* Test of ulc_v[a]s[n]printf() functions.
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
test_xfunction (char * (*my_xasprintf) (const char *, ...))
{
  /* Test support of size specifiers as in C99.  */

  {
    char *result =
      my_xasprintf ("%ju %d", (uintmax_t) 12345671, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12345671 33") == 0);
    free (result);
  }

  {
    char *result =
      my_xasprintf ("%zu %d", (size_t) 12345672, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12345672 33") == 0);
    free (result);
  }

  {
    char *result =
      my_xasprintf ("%tu %d", (ptrdiff_t) 12345673, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12345673 33") == 0);
    free (result);
  }

  {
    char *result =
      my_xasprintf ("%Lg %d", (long double) 1.5, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1.5 33") == 0);
    free (result);
  }

  /* Test the support of the 'U' conversion specifier for Unicode strings.  */

  {
    static const uint8_t unicode_string[] = "Hello";
    {
      char *result =
        my_xasprintf ("%U %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "Hello 33") == 0);
      free (result);
    }
    { /* Width.  */
      char *result =
        my_xasprintf ("%10U %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "     Hello 33") == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      char *result =
        my_xasprintf ("%-10U %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "Hello      33") == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      char *result =
        my_xasprintf ("%010U %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "     Hello 33") == 0);
      free (result);
    }
  }

  {
    static const uint16_t unicode_string[] = { 'H', 'e', 'l', 'l', 'o', 0 };
    {
      char *result =
        my_xasprintf ("%lU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "Hello 33") == 0);
      free (result);
    }
    { /* Width.  */
      char *result =
        my_xasprintf ("%10lU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "     Hello 33") == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      char *result =
        my_xasprintf ("%-10lU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "Hello      33") == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      char *result =
        my_xasprintf ("%010lU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "     Hello 33") == 0);
      free (result);
    }
  }

  {
    static const uint32_t unicode_string[] = { 'H', 'e', 'l', 'l', 'o', 0 };
    {
      char *result =
        my_xasprintf ("%llU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "Hello 33") == 0);
      free (result);
    }
    { /* Width.  */
      char *result =
        my_xasprintf ("%10llU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "     Hello 33") == 0);
      free (result);
    }
    { /* FLAG_LEFT.  */
      char *result =
        my_xasprintf ("%-10llU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "Hello      33") == 0);
      free (result);
    }
    { /* FLAG_ZERO: no effect.  */
      char *result =
        my_xasprintf ("%010llU %d", unicode_string, 33, 44, 55);
      ASSERT (result != NULL);
      ASSERT (strcmp (result, "     Hello 33") == 0);
      free (result);
    }
  }

  /* Test the support of the 's' conversion specifier for strings.  */

  {
    char *result =
      my_xasprintf ("Mr. %s %d", "Ronald Reagan", 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "Mr. Ronald Reagan 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("Mr. %20s %d", "Ronald Reagan", 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "Mr.        Ronald Reagan 33") == 0);
    free (result);
  }

  { /* FLAG_LEFT.  */
    char *result =
      my_xasprintf ("Mr. %-20s %d", "Ronald Reagan", 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "Mr. Ronald Reagan        33") == 0);
    free (result);
  }

  { /* FLAG_ZERO: no effect.  */
    char *result =
      my_xasprintf ("Mr. %020s %d", "Ronald Reagan", 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "Mr.        Ronald Reagan 33") == 0);
    free (result);
  }

  /* Test the support of the 'a' and 'A' conversion specifier for hexadecimal
     output of floating-point numbers.  */

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%a %d", 3.1416015625, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "0x1.922p+1 33") == 0
            || strcmp (result, "0x3.244p+0 33") == 0
            || strcmp (result, "0x6.488p-1 33") == 0
            || strcmp (result, "0xc.91p-2 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%10a %d", 1.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "  0x1.cp+0 33") == 0
            || strcmp (result, "  0x3.8p-1 33") == 0
            || strcmp (result, "    0x7p-2 33") == 0
            || strcmp (result, "    0xep-3 33") == 0);
    free (result);
  }

  { /* Small precision.  */
    char *result =
      my_xasprintf ("%.10a %d", 1.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "0x1.c000000000p+0 33") == 0
            || strcmp (result, "0x3.8000000000p-1 33") == 0
            || strcmp (result, "0x7.0000000000p-2 33") == 0
            || strcmp (result, "0xe.0000000000p-3 33") == 0);
    free (result);
  }

  { /* Large precision.  */
    char *result =
      my_xasprintf ("%.50a %d", 1.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "0x1.c0000000000000000000000000000000000000000000000000p+0 33") == 0
            || strcmp (result, "0x3.80000000000000000000000000000000000000000000000000p-1 33") == 0
            || strcmp (result, "0x7.00000000000000000000000000000000000000000000000000p-2 33") == 0
            || strcmp (result, "0xe.00000000000000000000000000000000000000000000000000p-3 33") == 0);
    free (result);
  }

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%La %d", 3.1416015625L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "0x1.922p+1 33") == 0
            || strcmp (result, "0x3.244p+0 33") == 0
            || strcmp (result, "0x6.488p-1 33") == 0
            || strcmp (result, "0xc.91p-2 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%10La %d", 1.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "  0x1.cp+0 33") == 0
            || strcmp (result, "  0x3.8p-1 33") == 0
            || strcmp (result, "    0x7p-2 33") == 0
            || strcmp (result, "    0xep-3 33") == 0);
    free (result);
  }

  { /* Small precision.  */
    char *result =
      my_xasprintf ("%.10La %d", 1.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "0x1.c000000000p+0 33") == 0
            || strcmp (result, "0x3.8000000000p-1 33") == 0
            || strcmp (result, "0x7.0000000000p-2 33") == 0
            || strcmp (result, "0xe.0000000000p-3 33") == 0);
    free (result);
  }

  { /* Large precision.  */
    char *result =
      my_xasprintf ("%.50La %d", 1.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "0x1.c0000000000000000000000000000000000000000000000000p+0 33") == 0
            || strcmp (result, "0x3.80000000000000000000000000000000000000000000000000p-1 33") == 0
            || strcmp (result, "0x7.00000000000000000000000000000000000000000000000000p-2 33") == 0
            || strcmp (result, "0xe.00000000000000000000000000000000000000000000000000p-3 33") == 0);
    free (result);
  }

  /* Test the support of the %f format directive.  */

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%f %d", 12.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12.750000 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%10f %d", 1.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "  1.750000 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.f %d", 1234.0, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1234 33") == 0);
    free (result);
  }

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%Lf %d", 12.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12.750000 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%10Lf %d", 1.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "  1.750000 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.Lf %d", 1234.0L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1234 33") == 0);
    free (result);
  }

  /* Test the support of the %F format directive.  */

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%F %d", 12.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12.750000 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.F %d", 1234.0, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1234 33") == 0);
    free (result);
  }

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%LF %d", 12.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12.750000 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.LF %d", 1234.0L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1234 33") == 0);
    free (result);
  }

  /* Test the support of the %e format directive.  */

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%e %d", 12.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1.275000e+01 33") == 0
            || strcmp (result, "1.275000e+001 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%15e %d", 1.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "   1.750000e+00 33") == 0
            || strcmp (result, "  1.750000e+000 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.e %d", 1234.0, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1e+03 33") == 0
            || strcmp (result, "1e+003 33") == 0);
    free (result);
  }

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%Le %d", 12.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1.275000e+01 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%15Le %d", 1.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "   1.750000e+00 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.Le %d", 1234.0L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1e+03 33") == 0);
    free (result);
  }

  /* Test the support of the %g format directive.  */

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%g %d", 12.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12.75 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%10g %d", 1.75, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "      1.75 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.g %d", 1234.0, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1e+03 33") == 0
            || strcmp (result, "1e+003 33") == 0);
    free (result);
  }

  { /* A positive number.  */
    char *result =
      my_xasprintf ("%Lg %d", 12.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "12.75 33") == 0);
    free (result);
  }

  { /* Width.  */
    char *result =
      my_xasprintf ("%10Lg %d", 1.75L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "      1.75 33") == 0);
    free (result);
  }

  { /* Precision.  */
    char *result =
      my_xasprintf ("%.Lg %d", 1234.0L, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "1e+03 33") == 0);
    free (result);
  }

  /* Test the support of the %n format directive.  */

  {
    int count = -1;
    char *result =
      my_xasprintf ("%d %n", 123, &count, 33, 44, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "123 ") == 0);
    ASSERT (count == 4);
    free (result);
  }

  /* Test the support of the POSIX/XSI format strings with positions.  */

  {
    char *result =
      my_xasprintf ("%2$d %1$d", 33, 55);
    ASSERT (result != NULL);
    ASSERT (strcmp (result, "55 33") == 0);
    free (result);
  }

  /* Test the support of the grouping flag.  */

  {
    char *result =
      my_xasprintf ("%'d %d", 1234567, 99);
    ASSERT (result != NULL);
    ASSERT (result[strlen (result) - 1] == '9');
    free (result);
  }
}
