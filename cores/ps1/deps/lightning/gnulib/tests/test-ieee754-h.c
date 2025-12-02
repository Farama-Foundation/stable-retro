/* Test <ieee754.h>.
   Copyright 2018-2021 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  */

#include <config.h>

#include <ieee754.h>
#include <stdio.h>

static struct {
  float x;
  unsigned sign; unsigned exponent; unsigned frac;
} const float_tests[] =
  {
   {0, 0, 0, 0},
   {-0.0, 1, 0, 0},
   {0.1, 0, 0x7b, 0x4ccccd}
  };

static struct {
  double x;
  unsigned sign; unsigned exponent; unsigned frac_hi; unsigned frac_lo;
} const double_tests[] =
  {
   {0, 0, 0, 0},
   {-0.0, 1, 0, 0 },
   {0.1, 0, 0x3fb, 0x99999, 0x9999999a}
  };

int
main (void)
{
  int i;

  for (i = 0; i < sizeof float_tests / sizeof *float_tests; i++)
    {
      union ieee754_float u;
      u.f = float_tests[i].x;
      if (float_tests[i].sign != u.ieee.negative)
        return 1;
      if (float_tests[i].exponent != u.ieee.exponent)
        return 2;
      if (float_tests[i].frac != u.ieee.mantissa)
        return 3;
    }

  for (i = 0; i < sizeof double_tests / sizeof *double_tests; i++)
    {
      union ieee754_double u;
      u.d = double_tests[i].x;
      if (double_tests[i].sign != u.ieee.negative)
        return 4;
      if (double_tests[i].exponent != u.ieee.exponent)
        return 5;
      if (double_tests[i].frac_hi != u.ieee.mantissa0)
        return 6;
      if (double_tests[i].frac_lo != u.ieee.mantissa1)
        return 7;
    }

  return 0;
}
