/* Test the Unicode character Arabic joining group functions.
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

int
main ()
{
  ASSERT (uc_combining_class_byname ("NR") == UC_CCC_NR);
  ASSERT (uc_combining_class_byname ("OV") == UC_CCC_OV);
  ASSERT (uc_combining_class_byname ("NK") == UC_CCC_NK);
  ASSERT (uc_combining_class_byname ("KV") == UC_CCC_KV);
  ASSERT (uc_combining_class_byname ("VR") == UC_CCC_VR);
  ASSERT (uc_combining_class_byname ("ATBL") == UC_CCC_ATBL);
  ASSERT (uc_combining_class_byname ("ATB") == UC_CCC_ATB);
  ASSERT (uc_combining_class_byname ("ATA") == UC_CCC_ATA);
  ASSERT (uc_combining_class_byname ("ATAR") == UC_CCC_ATAR);
  ASSERT (uc_combining_class_byname ("BL") == UC_CCC_BL);
  ASSERT (uc_combining_class_byname ("B") == UC_CCC_B);
  ASSERT (uc_combining_class_byname ("BR") == UC_CCC_BR);
  ASSERT (uc_combining_class_byname ("L") == UC_CCC_L);
  ASSERT (uc_combining_class_byname ("R") == UC_CCC_R);
  ASSERT (uc_combining_class_byname ("AL") == UC_CCC_AL);
  ASSERT (uc_combining_class_byname ("A") == UC_CCC_A);
  ASSERT (uc_combining_class_byname ("AR") == UC_CCC_AR);
  ASSERT (uc_combining_class_byname ("DB") == UC_CCC_DB);
  ASSERT (uc_combining_class_byname ("DA") == UC_CCC_DA);
  ASSERT (uc_combining_class_byname ("IS") == UC_CCC_IS);

  ASSERT (uc_combining_class_byname ("NOT REORDERED") == UC_CCC_NR);
  ASSERT (uc_combining_class_byname ("Not Reordered") == UC_CCC_NR);
  ASSERT (uc_combining_class_byname ("Not_Reordered") == UC_CCC_NR);
  ASSERT (uc_combining_class_byname ("NotReordered") == UC_CCC_NR);
  ASSERT (uc_combining_class_byname ("OVERLAY") == UC_CCC_OV);
  ASSERT (uc_combining_class_byname ("Overlay") == UC_CCC_OV);
  ASSERT (uc_combining_class_byname ("NUKTA") == UC_CCC_NK);
  ASSERT (uc_combining_class_byname ("Nukta") == UC_CCC_NK);
  ASSERT (uc_combining_class_byname ("KANA VOICING") == UC_CCC_KV);
  ASSERT (uc_combining_class_byname ("Kana Voicing") == UC_CCC_KV);
  ASSERT (uc_combining_class_byname ("Kana_Voicing") == UC_CCC_KV);
  ASSERT (uc_combining_class_byname ("KanaVoicing") == UC_CCC_KV);
  ASSERT (uc_combining_class_byname ("VIRAMA") == UC_CCC_VR);
  ASSERT (uc_combining_class_byname ("Virama") == UC_CCC_VR);
  ASSERT (uc_combining_class_byname ("ATTACHED BELOW LEFT") == UC_CCC_ATBL);
  ASSERT (uc_combining_class_byname ("Attached Below Left") == UC_CCC_ATBL);
  ASSERT (uc_combining_class_byname ("Attached_Below_Left") == UC_CCC_ATBL);
  ASSERT (uc_combining_class_byname ("AttachedBelowLeft") == UC_CCC_ATBL);
  ASSERT (uc_combining_class_byname ("ATTACHED BELOW") == UC_CCC_ATB);
  ASSERT (uc_combining_class_byname ("Attached Below") == UC_CCC_ATB);
  ASSERT (uc_combining_class_byname ("Attached_Below") == UC_CCC_ATB);
  ASSERT (uc_combining_class_byname ("AttachedBelow") == UC_CCC_ATB);
  ASSERT (uc_combining_class_byname ("ATTACHED ABOVE") == UC_CCC_ATA);
  ASSERT (uc_combining_class_byname ("Attached Above") == UC_CCC_ATA);
  ASSERT (uc_combining_class_byname ("Attached_Above") == UC_CCC_ATA);
  ASSERT (uc_combining_class_byname ("AttachedAbove") == UC_CCC_ATA);
  ASSERT (uc_combining_class_byname ("ATTACHED ABOVE RIGHT") == UC_CCC_ATAR);
  ASSERT (uc_combining_class_byname ("Attached Above Right") == UC_CCC_ATAR);
  ASSERT (uc_combining_class_byname ("Attached_Above_Right") == UC_CCC_ATAR);
  ASSERT (uc_combining_class_byname ("AttachedAboveRight") == UC_CCC_ATAR);
  ASSERT (uc_combining_class_byname ("BELOW LEFT") == UC_CCC_BL);
  ASSERT (uc_combining_class_byname ("Below Left") == UC_CCC_BL);
  ASSERT (uc_combining_class_byname ("Below_Left") == UC_CCC_BL);
  ASSERT (uc_combining_class_byname ("BelowLeft") == UC_CCC_BL);
  ASSERT (uc_combining_class_byname ("BELOW") == UC_CCC_B);
  ASSERT (uc_combining_class_byname ("Below") == UC_CCC_B);
  ASSERT (uc_combining_class_byname ("BELOW RIGHT") == UC_CCC_BR);
  ASSERT (uc_combining_class_byname ("Below Right") == UC_CCC_BR);
  ASSERT (uc_combining_class_byname ("Below_Right") == UC_CCC_BR);
  ASSERT (uc_combining_class_byname ("BelowRight") == UC_CCC_BR);
  ASSERT (uc_combining_class_byname ("LEFT") == UC_CCC_L);
  ASSERT (uc_combining_class_byname ("Left") == UC_CCC_L);
  ASSERT (uc_combining_class_byname ("RIGHT") == UC_CCC_R);
  ASSERT (uc_combining_class_byname ("Right") == UC_CCC_R);
  ASSERT (uc_combining_class_byname ("ABOVE LEFT") == UC_CCC_AL);
  ASSERT (uc_combining_class_byname ("Above Left") == UC_CCC_AL);
  ASSERT (uc_combining_class_byname ("Above_Left") == UC_CCC_AL);
  ASSERT (uc_combining_class_byname ("AboveLeft") == UC_CCC_AL);
  ASSERT (uc_combining_class_byname ("ABOVE") == UC_CCC_A);
  ASSERT (uc_combining_class_byname ("ABOVE RIGHT") == UC_CCC_AR);
  ASSERT (uc_combining_class_byname ("Above Right") == UC_CCC_AR);
  ASSERT (uc_combining_class_byname ("Above_Right") == UC_CCC_AR);
  ASSERT (uc_combining_class_byname ("AboveRight") == UC_CCC_AR);
  ASSERT (uc_combining_class_byname ("DOUBLE BELOW") == UC_CCC_DB);
  ASSERT (uc_combining_class_byname ("Double Below") == UC_CCC_DB);
  ASSERT (uc_combining_class_byname ("Double_Below") == UC_CCC_DB);
  ASSERT (uc_combining_class_byname ("DoubleBelow") == UC_CCC_DB);
  ASSERT (uc_combining_class_byname ("DOUBLE ABOVE") == UC_CCC_DA);
  ASSERT (uc_combining_class_byname ("Double Above") == UC_CCC_DA);
  ASSERT (uc_combining_class_byname ("Double_Above") == UC_CCC_DA);
  ASSERT (uc_combining_class_byname ("DoubleAbove") == UC_CCC_DA);
  ASSERT (uc_combining_class_byname ("IOTA SUBSCRIPT") == UC_CCC_IS);
  ASSERT (uc_combining_class_byname ("Iota Subscript") == UC_CCC_IS);
  ASSERT (uc_combining_class_byname ("Iota_Subscript") == UC_CCC_IS);
  ASSERT (uc_combining_class_byname ("IotaSubscript") == UC_CCC_IS);

  return 0;
}
