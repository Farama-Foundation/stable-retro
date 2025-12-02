/* Test the Unicode character Arabic joining type functions.
   Copyright (C) 2011 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2011.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "unictype.h"

#include <string.h>

#include "macros.h"

typedef struct { ucs4_t ch; int value; } pair_t;

static const pair_t mapping[] =
  {
#include "test-joiningtype_of.h"
  };

int
main ()
{
  unsigned int c;
  size_t i;

  c = 0;
  for (i = 0; i < SIZEOF (mapping); i++)
    {
      for (; c < mapping[i].ch; c++)
        ASSERT (uc_joining_type (c) == UC_JOINING_TYPE_U
                || uc_joining_type (c) == UC_JOINING_TYPE_T);
      ASSERT (uc_joining_type (c) == mapping[i].value);
      c++;
    }
  for (; c < 0x110000; c++)
    ASSERT (uc_joining_type (c) == UC_JOINING_TYPE_U
            || uc_joining_type (c) == UC_JOINING_TYPE_T);

  /* Test distinction between U and T.  */
  ASSERT (uc_joining_type (0x060C) == UC_JOINING_TYPE_U);
  ASSERT (uc_joining_type (0x060D) == UC_JOINING_TYPE_U);
  ASSERT (uc_joining_type (0x060E) == UC_JOINING_TYPE_U);
  ASSERT (uc_joining_type (0x060F) == UC_JOINING_TYPE_U);
  ASSERT (uc_joining_type (0x0610) == UC_JOINING_TYPE_T);
  ASSERT (uc_joining_type (0x0611) == UC_JOINING_TYPE_T);
  ASSERT (uc_joining_type (0x0612) == UC_JOINING_TYPE_T);
  ASSERT (uc_joining_type (0x0613) == UC_JOINING_TYPE_T);
  ASSERT (uc_joining_type (0x0614) == UC_JOINING_TYPE_T);

  return 0;
}
