/* Test the Unicode grapheme break property function.
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

#include <config.h>

/* Specification. */
#include <unigbrk.h>

struct uc_gbrk_prop_range
{
  ucs4_t end;
  int gbp;
};

static const struct uc_gbrk_prop_range set[] =
  {
#include "test-uc-gbrk-prop.h"
  };

#include "macros.h"

const char *
graphemebreakproperty_to_string (int gbp)
{
  switch (gbp)
    {
#define CASE(VALUE) case GBP_##VALUE: return #VALUE;
      CASE(OTHER)
      CASE(CR)
      CASE(LF)
      CASE(CONTROL)
      CASE(EXTEND)
      CASE(PREPEND)
      CASE(SPACINGMARK)
      CASE(L)
      CASE(V)
      CASE(T)
      CASE(LV)
      CASE(LVT)
      CASE(RI)
      CASE(ZWJ)
      CASE(EB)
      CASE(EM)
      CASE(GAZ)
      CASE(EBG)
    }
  abort ();
}

int
main (void)
{
  const struct uc_gbrk_prop_range *r;
  ucs4_t uc;

  uc = 0;
  for (r = set; r < set + SIZEOF (set); r++)
    {
      for (; uc < r->end; uc++)
        {
          int retval = uc_graphemeclusterbreak_property (uc);
          if (retval != r->gbp)
            {
              fprintf (stderr, "uc_graphemeclusterbreak_property(%#x) "
                       "yielded %s but should have been %s\n",
                       uc, graphemebreakproperty_to_string (retval),
                       graphemebreakproperty_to_string (r->gbp));
              fflush (stderr);
              abort ();
            }
        }
    }
  ASSERT (uc == 0x110000);

  return 0;
}
