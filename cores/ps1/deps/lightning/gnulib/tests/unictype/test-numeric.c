/* Test the Unicode character type functions.
   Copyright (C) 2007-2009 Free Software Foundation, Inc.

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

#include <config.h>

#include "unictype.h"

#include <string.h>

#include "macros.h"

static const struct { unsigned int ch; int numerator; int denominator; }
  mapping[] =
  {
#include "test-numeric.h"
  };

int
main ()
{
  unsigned int c;
  uc_fraction_t value;
  size_t i;

  c = 0;
  for (i = 0; i < SIZEOF (mapping); i++)
    {
      for (; c < mapping[i].ch; c++)
        {
          value = uc_numeric_value (c);
          ASSERT (value.numerator == 0 && value.denominator == 0);
        }
      /* Here c = mapping[i].ch.  */
      value = uc_numeric_value (c);
      ASSERT (value.numerator == mapping[i].numerator
              && value.denominator == mapping[i].denominator);
      c++;
    }
  for (; c < 0x110000; c++)
    {
      value = uc_numeric_value (c);
      ASSERT (value.numerator == 0 && value.denominator == 0);
    }

  return 0;
}
