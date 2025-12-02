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
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_NR), "NR") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_OV), "OV") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_NK), "NK") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_KV), "KV") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_VR), "VR") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_ATBL), "ATBL") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_ATB), "ATB") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_ATA), "ATA") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_ATAR), "ATAR") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_BL), "BL") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_B), "B") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_BR), "BR") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_L), "L") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_R), "R") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_AL), "AL") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_A), "A") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_AR), "AR") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_DB), "DB") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_DA), "DA") == 0);
  ASSERT (strcmp (uc_combining_class_name (UC_CCC_IS), "IS") == 0);
  ASSERT (uc_combining_class_name (-5) == NULL);
  ASSERT (uc_combining_class_name (163) == NULL);
  ASSERT (uc_combining_class_name (255) == NULL);

  return 0;
}
