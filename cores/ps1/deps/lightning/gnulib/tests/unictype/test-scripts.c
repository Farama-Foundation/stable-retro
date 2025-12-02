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

int
main ()
{
  /* Test the uc_script function.  */
  ASSERT (strcmp (uc_script ('A')->name, "Latin") == 0);
  ASSERT (strcmp (uc_script ('%')->name, "Common") == 0);
  ASSERT (uc_script (0xE0000) == NULL);

  /* Test the uc_script_byname function.  */
  ASSERT (strcmp (uc_script_byname ("Armenian")->name, "Armenian") == 0);
  ASSERT (uc_script_byname ("Pekinese") == NULL);

  /* Test the uc_is_script function.  */
  ASSERT (uc_is_script ('A', uc_script_byname ("Latin")));
  ASSERT (!uc_is_script ('A', uc_script_byname ("Common")));
  ASSERT (!uc_is_script (0x05DA, uc_script_byname ("Latin")));
  ASSERT (uc_is_script (0x05DA, uc_script_byname ("Hebrew")));

  {
    unsigned int c;

    for (c = 0; c < 0x110000; c++)
      {
        const uc_script_t *script = uc_script (c);
        if (script != NULL)
          ASSERT (uc_is_script (c, script));
      }
  }

  /* Test the uc_all_scripts function.  */
  {
    const uc_script_t *scripts;
    size_t nscripts;
    size_t i;
    bool found;

    uc_all_scripts (&scripts, &nscripts);

    found = false;
    for (i = 0; i < nscripts; i++)
      {
        ASSERT (scripts[i].name != NULL);
        if (strcmp (scripts[i].name, "Hebrew") == 0)
          found = true;
      }
    ASSERT (found);
  }

  return 0;
}
