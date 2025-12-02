/* Test the Unicode character type functions.
   Copyright (C) 2011 Free Software Foundation, Inc.

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
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_NR), "Not Reordered") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_OV), "Overlay") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_NK), "Nukta") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_KV), "Kana Voicing") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_VR), "Virama") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_ATBL), "Attached Below Left") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_ATB), "Attached Below") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_ATA), "Attached Above") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_ATAR), "Attached Above Right") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_BL), "Below Left") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_B), "Below") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_BR), "Below Right") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_L), "Left") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_R), "Right") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_AL), "Above Left") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_A), "Above") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_AR), "Above Right") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_DB), "Double Below") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_DA), "Double Above") == 0);
  ASSERT (strcmp (uc_combining_class_long_name (UC_CCC_IS), "Iota Subscript") == 0);
  ASSERT (uc_combining_class_long_name (-5) == NULL);
  ASSERT (uc_combining_class_long_name (163) == NULL);
  ASSERT (uc_combining_class_long_name (255) == NULL);

  return 0;
}
