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
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_NONE),                  "No Joining Group") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_AIN),                   "Ain") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_ALAPH),                 "Alaph") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_ALEF),                  "Alef") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_BEH),                   "Beh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_BETH),                  "Beth") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_BURUSHASKI_YEH_BARREE), "Burushaski Yeh Barree") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_DAL),                   "Dal") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_DALATH_RISH),           "Dalath Rish") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_E),                     "E") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_FARSI_YEH),             "Farsi Yeh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_FE),                    "Fe") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_FEH),                   "Feh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_FINAL_SEMKATH),         "Final Semkath") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_GAF),                   "Gaf") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_GAMAL),                 "Gamal") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_HAH),                   "Hah") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_HE),                    "He") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_HEH),                   "Heh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_HEH_GOAL),              "Heh Goal") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_HETH),                  "Heth") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_KAF),                   "Kaf") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_KAPH),                  "Kaph") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_KHAPH),                 "Khaph") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_KNOTTED_HEH),           "Knotted Heh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_LAM),                   "Lam") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_LAMADH),                "Lamadh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_MEEM),                  "Meem") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_MIM),                   "Mim") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_NOON),                  "Noon") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_NUN),                   "Nun") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_NYA),                   "Nya") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_PE),                    "Pe") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_QAF),                   "Qaf") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_QAPH),                  "Qaph") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_REH),                   "Reh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_REVERSED_PE),           "Reversed Pe") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SAD),                   "Sad") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SADHE),                 "Sadhe") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SEEN),                  "Seen") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SEMKATH),               "Semkath") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SHIN),                  "Shin") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SWASH_KAF),             "Swash Kaf") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_SYRIAC_WAW),            "Syriac Waw") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_TAH),                   "Tah") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_TAW),                   "Taw") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_TEH_MARBUTA),           "Teh Marbuta") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_TEH_MARBUTA_GOAL),      "Teh Marbuta Goal") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_TETH),                  "Teth") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_WAW),                   "Waw") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_YEH),                   "Yeh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_YEH_BARREE),            "Yeh Barree") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_YEH_WITH_TAIL),         "Yeh With Tail") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_YUDH),                  "Yudh") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_YUDH_HE),               "Yudh He") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_ZAIN),                  "Zain") == 0);
  ASSERT (strcmp (uc_joining_group_name (UC_JOINING_GROUP_ZHAIN),                 "Zhain") == 0);
  ASSERT (uc_joining_group_name (-1) == NULL);

  return 0;
}
