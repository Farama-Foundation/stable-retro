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
  ASSERT (uc_joining_group_byname ("NO JOINING GROUP") == UC_JOINING_GROUP_NONE);
  ASSERT (uc_joining_group_byname ("No Joining Group") == UC_JOINING_GROUP_NONE);
  ASSERT (uc_joining_group_byname ("No_Joining_Group") == UC_JOINING_GROUP_NONE);
  ASSERT (uc_joining_group_byname ("NoJoiningGroup") == UC_JOINING_GROUP_NONE);
  ASSERT (uc_joining_group_byname ("AIN") == UC_JOINING_GROUP_AIN);
  ASSERT (uc_joining_group_byname ("Ain") == UC_JOINING_GROUP_AIN);
  ASSERT (uc_joining_group_byname ("ALAPH") == UC_JOINING_GROUP_ALAPH);
  ASSERT (uc_joining_group_byname ("Alaph") == UC_JOINING_GROUP_ALAPH);
  ASSERT (uc_joining_group_byname ("ALEF") == UC_JOINING_GROUP_ALEF);
  ASSERT (uc_joining_group_byname ("Alef") == UC_JOINING_GROUP_ALEF);
  ASSERT (uc_joining_group_byname ("BEH") == UC_JOINING_GROUP_BEH);
  ASSERT (uc_joining_group_byname ("Beh") == UC_JOINING_GROUP_BEH);
  ASSERT (uc_joining_group_byname ("BETH") == UC_JOINING_GROUP_BETH);
  ASSERT (uc_joining_group_byname ("Beth") == UC_JOINING_GROUP_BETH);
  ASSERT (uc_joining_group_byname ("BURUSHASKI YEH BARREE") == UC_JOINING_GROUP_BURUSHASKI_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("Burushaski Yeh Barree") == UC_JOINING_GROUP_BURUSHASKI_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("Burushaski_Yeh_Barree") == UC_JOINING_GROUP_BURUSHASKI_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("BurushaskiYehBarree") == UC_JOINING_GROUP_BURUSHASKI_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("DAL") == UC_JOINING_GROUP_DAL);
  ASSERT (uc_joining_group_byname ("Dal") == UC_JOINING_GROUP_DAL);
  ASSERT (uc_joining_group_byname ("DALATH RISH") == UC_JOINING_GROUP_DALATH_RISH);
  ASSERT (uc_joining_group_byname ("Dalath Rish") == UC_JOINING_GROUP_DALATH_RISH);
  ASSERT (uc_joining_group_byname ("Dalath_Rish") == UC_JOINING_GROUP_DALATH_RISH);
  ASSERT (uc_joining_group_byname ("DalathRish") == UC_JOINING_GROUP_DALATH_RISH);
  ASSERT (uc_joining_group_byname ("E") == UC_JOINING_GROUP_E);
  ASSERT (uc_joining_group_byname ("FARSI YEH") == UC_JOINING_GROUP_FARSI_YEH);
  ASSERT (uc_joining_group_byname ("Farsi Yeh") == UC_JOINING_GROUP_FARSI_YEH);
  ASSERT (uc_joining_group_byname ("Farsi_Yeh") == UC_JOINING_GROUP_FARSI_YEH);
  ASSERT (uc_joining_group_byname ("FarsiYeh") == UC_JOINING_GROUP_FARSI_YEH);
  ASSERT (uc_joining_group_byname ("FE") == UC_JOINING_GROUP_FE);
  ASSERT (uc_joining_group_byname ("Fe") == UC_JOINING_GROUP_FE);
  ASSERT (uc_joining_group_byname ("FEH") == UC_JOINING_GROUP_FEH);
  ASSERT (uc_joining_group_byname ("Feh") == UC_JOINING_GROUP_FEH);
  ASSERT (uc_joining_group_byname ("FINAL SEMKATH") == UC_JOINING_GROUP_FINAL_SEMKATH);
  ASSERT (uc_joining_group_byname ("Final Semkath") == UC_JOINING_GROUP_FINAL_SEMKATH);
  ASSERT (uc_joining_group_byname ("Final_Semkath") == UC_JOINING_GROUP_FINAL_SEMKATH);
  ASSERT (uc_joining_group_byname ("FinalSemkath") == UC_JOINING_GROUP_FINAL_SEMKATH);
  ASSERT (uc_joining_group_byname ("GAF") == UC_JOINING_GROUP_GAF);
  ASSERT (uc_joining_group_byname ("Gaf") == UC_JOINING_GROUP_GAF);
  ASSERT (uc_joining_group_byname ("GAMAL") == UC_JOINING_GROUP_GAMAL);
  ASSERT (uc_joining_group_byname ("Gamal") == UC_JOINING_GROUP_GAMAL);
  ASSERT (uc_joining_group_byname ("HAH") == UC_JOINING_GROUP_HAH);
  ASSERT (uc_joining_group_byname ("Hah") == UC_JOINING_GROUP_HAH);
  ASSERT (uc_joining_group_byname ("HE") == UC_JOINING_GROUP_HE);
  ASSERT (uc_joining_group_byname ("He") == UC_JOINING_GROUP_HE);
  ASSERT (uc_joining_group_byname ("HEH") == UC_JOINING_GROUP_HEH);
  ASSERT (uc_joining_group_byname ("Heh") == UC_JOINING_GROUP_HEH);
  ASSERT (uc_joining_group_byname ("HEH GOAL") == UC_JOINING_GROUP_HEH_GOAL);
  ASSERT (uc_joining_group_byname ("Heh Goal") == UC_JOINING_GROUP_HEH_GOAL);
  ASSERT (uc_joining_group_byname ("Heh_Goal") == UC_JOINING_GROUP_HEH_GOAL);
  ASSERT (uc_joining_group_byname ("HehGoal") == UC_JOINING_GROUP_HEH_GOAL);
  ASSERT (uc_joining_group_byname ("HETH") == UC_JOINING_GROUP_HETH);
  ASSERT (uc_joining_group_byname ("Heth") == UC_JOINING_GROUP_HETH);
  ASSERT (uc_joining_group_byname ("KAF") == UC_JOINING_GROUP_KAF);
  ASSERT (uc_joining_group_byname ("Kaf") == UC_JOINING_GROUP_KAF);
  ASSERT (uc_joining_group_byname ("KAPH") == UC_JOINING_GROUP_KAPH);
  ASSERT (uc_joining_group_byname ("Kaph") == UC_JOINING_GROUP_KAPH);
  ASSERT (uc_joining_group_byname ("KHAPH") == UC_JOINING_GROUP_KHAPH);
  ASSERT (uc_joining_group_byname ("Khaph") == UC_JOINING_GROUP_KHAPH);
  ASSERT (uc_joining_group_byname ("KNOTTED HEH") == UC_JOINING_GROUP_KNOTTED_HEH);
  ASSERT (uc_joining_group_byname ("Knotted Heh") == UC_JOINING_GROUP_KNOTTED_HEH);
  ASSERT (uc_joining_group_byname ("Knotted_Heh") == UC_JOINING_GROUP_KNOTTED_HEH);
  ASSERT (uc_joining_group_byname ("KnottedHeh") == UC_JOINING_GROUP_KNOTTED_HEH);
  ASSERT (uc_joining_group_byname ("LAM") == UC_JOINING_GROUP_LAM);
  ASSERT (uc_joining_group_byname ("Lam") == UC_JOINING_GROUP_LAM);
  ASSERT (uc_joining_group_byname ("LAMADH") == UC_JOINING_GROUP_LAMADH);
  ASSERT (uc_joining_group_byname ("Lamadh") == UC_JOINING_GROUP_LAMADH);
  ASSERT (uc_joining_group_byname ("MEEM") == UC_JOINING_GROUP_MEEM);
  ASSERT (uc_joining_group_byname ("Meem") == UC_JOINING_GROUP_MEEM);
  ASSERT (uc_joining_group_byname ("MIM") == UC_JOINING_GROUP_MIM);
  ASSERT (uc_joining_group_byname ("Mim") == UC_JOINING_GROUP_MIM);
  ASSERT (uc_joining_group_byname ("NOON") == UC_JOINING_GROUP_NOON);
  ASSERT (uc_joining_group_byname ("Noon") == UC_JOINING_GROUP_NOON);
  ASSERT (uc_joining_group_byname ("NUN") == UC_JOINING_GROUP_NUN);
  ASSERT (uc_joining_group_byname ("Nun") == UC_JOINING_GROUP_NUN);
  ASSERT (uc_joining_group_byname ("NYA") == UC_JOINING_GROUP_NYA);
  ASSERT (uc_joining_group_byname ("Nya") == UC_JOINING_GROUP_NYA);
  ASSERT (uc_joining_group_byname ("PE") == UC_JOINING_GROUP_PE);
  ASSERT (uc_joining_group_byname ("Pe") == UC_JOINING_GROUP_PE);
  ASSERT (uc_joining_group_byname ("QAF") == UC_JOINING_GROUP_QAF);
  ASSERT (uc_joining_group_byname ("Qaf") == UC_JOINING_GROUP_QAF);
  ASSERT (uc_joining_group_byname ("QAPH") == UC_JOINING_GROUP_QAPH);
  ASSERT (uc_joining_group_byname ("Qaph") == UC_JOINING_GROUP_QAPH);
  ASSERT (uc_joining_group_byname ("REH") == UC_JOINING_GROUP_REH);
  ASSERT (uc_joining_group_byname ("Reh") == UC_JOINING_GROUP_REH);
  ASSERT (uc_joining_group_byname ("REVERSED PE") == UC_JOINING_GROUP_REVERSED_PE);
  ASSERT (uc_joining_group_byname ("Reversed Pe") == UC_JOINING_GROUP_REVERSED_PE);
  ASSERT (uc_joining_group_byname ("Reversed_Pe") == UC_JOINING_GROUP_REVERSED_PE);
  ASSERT (uc_joining_group_byname ("ReversedPe") == UC_JOINING_GROUP_REVERSED_PE);
  ASSERT (uc_joining_group_byname ("SAD") == UC_JOINING_GROUP_SAD);
  ASSERT (uc_joining_group_byname ("Sad") == UC_JOINING_GROUP_SAD);
  ASSERT (uc_joining_group_byname ("SADHE") == UC_JOINING_GROUP_SADHE);
  ASSERT (uc_joining_group_byname ("Sadhe") == UC_JOINING_GROUP_SADHE);
  ASSERT (uc_joining_group_byname ("SEEN") == UC_JOINING_GROUP_SEEN);
  ASSERT (uc_joining_group_byname ("Seen") == UC_JOINING_GROUP_SEEN);
  ASSERT (uc_joining_group_byname ("SEMKATH") == UC_JOINING_GROUP_SEMKATH);
  ASSERT (uc_joining_group_byname ("Semkath") == UC_JOINING_GROUP_SEMKATH);
  ASSERT (uc_joining_group_byname ("SHIN") == UC_JOINING_GROUP_SHIN);
  ASSERT (uc_joining_group_byname ("Shin") == UC_JOINING_GROUP_SHIN);
  ASSERT (uc_joining_group_byname ("SWASH KAF") == UC_JOINING_GROUP_SWASH_KAF);
  ASSERT (uc_joining_group_byname ("Swash Kaf") == UC_JOINING_GROUP_SWASH_KAF);
  ASSERT (uc_joining_group_byname ("Swash_Kaf") == UC_JOINING_GROUP_SWASH_KAF);
  ASSERT (uc_joining_group_byname ("SwashKaf") == UC_JOINING_GROUP_SWASH_KAF);
  ASSERT (uc_joining_group_byname ("SYRIAC WAW") == UC_JOINING_GROUP_SYRIAC_WAW);
  ASSERT (uc_joining_group_byname ("Syriac Waw") == UC_JOINING_GROUP_SYRIAC_WAW);
  ASSERT (uc_joining_group_byname ("Syriac_Waw") == UC_JOINING_GROUP_SYRIAC_WAW);
  ASSERT (uc_joining_group_byname ("SyriacWaw") == UC_JOINING_GROUP_SYRIAC_WAW);
  ASSERT (uc_joining_group_byname ("TAH") == UC_JOINING_GROUP_TAH);
  ASSERT (uc_joining_group_byname ("Tah") == UC_JOINING_GROUP_TAH);
  ASSERT (uc_joining_group_byname ("TAW") == UC_JOINING_GROUP_TAW);
  ASSERT (uc_joining_group_byname ("Taw") == UC_JOINING_GROUP_TAW);
  ASSERT (uc_joining_group_byname ("TEH MARBUTA") == UC_JOINING_GROUP_TEH_MARBUTA);
  ASSERT (uc_joining_group_byname ("Teh Marbuta") == UC_JOINING_GROUP_TEH_MARBUTA);
  ASSERT (uc_joining_group_byname ("Teh_Marbuta") == UC_JOINING_GROUP_TEH_MARBUTA);
  ASSERT (uc_joining_group_byname ("TehMarbuta") == UC_JOINING_GROUP_TEH_MARBUTA);
  ASSERT (uc_joining_group_byname ("TEH MARBUTA GOAL") == UC_JOINING_GROUP_TEH_MARBUTA_GOAL);
  ASSERT (uc_joining_group_byname ("Teh Marbuta Goal") == UC_JOINING_GROUP_TEH_MARBUTA_GOAL);
  ASSERT (uc_joining_group_byname ("Teh_Marbuta_Goal") == UC_JOINING_GROUP_TEH_MARBUTA_GOAL);
  ASSERT (uc_joining_group_byname ("TehMarbutaGoal") == UC_JOINING_GROUP_TEH_MARBUTA_GOAL);
  ASSERT (uc_joining_group_byname ("TETH") == UC_JOINING_GROUP_TETH);
  ASSERT (uc_joining_group_byname ("Teth") == UC_JOINING_GROUP_TETH);
  ASSERT (uc_joining_group_byname ("WAW") == UC_JOINING_GROUP_WAW);
  ASSERT (uc_joining_group_byname ("Waw") == UC_JOINING_GROUP_WAW);
  ASSERT (uc_joining_group_byname ("YEH") == UC_JOINING_GROUP_YEH);
  ASSERT (uc_joining_group_byname ("Yeh") == UC_JOINING_GROUP_YEH);
  ASSERT (uc_joining_group_byname ("YEH BARREE") == UC_JOINING_GROUP_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("Yeh Barree") == UC_JOINING_GROUP_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("Yeh_Barree") == UC_JOINING_GROUP_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("YehBarree") == UC_JOINING_GROUP_YEH_BARREE);
  ASSERT (uc_joining_group_byname ("YEH WITH TAIL") == UC_JOINING_GROUP_YEH_WITH_TAIL);
  ASSERT (uc_joining_group_byname ("Yeh with tail") == UC_JOINING_GROUP_YEH_WITH_TAIL);
  ASSERT (uc_joining_group_byname ("Yeh_with_tail") == UC_JOINING_GROUP_YEH_WITH_TAIL);
  ASSERT (uc_joining_group_byname ("YehWithTail") == UC_JOINING_GROUP_YEH_WITH_TAIL);
  ASSERT (uc_joining_group_byname ("YUDH") == UC_JOINING_GROUP_YUDH);
  ASSERT (uc_joining_group_byname ("Yudh") == UC_JOINING_GROUP_YUDH);
  ASSERT (uc_joining_group_byname ("YUDH HE") == UC_JOINING_GROUP_YUDH_HE);
  ASSERT (uc_joining_group_byname ("Yudh He") == UC_JOINING_GROUP_YUDH_HE);
  ASSERT (uc_joining_group_byname ("Yudh_He") == UC_JOINING_GROUP_YUDH_HE);
  ASSERT (uc_joining_group_byname ("YudhHe") == UC_JOINING_GROUP_YUDH_HE);
  ASSERT (uc_joining_group_byname ("ZAIN") == UC_JOINING_GROUP_ZAIN);
  ASSERT (uc_joining_group_byname ("Zain") == UC_JOINING_GROUP_ZAIN);
  ASSERT (uc_joining_group_byname ("ZHAIN") == UC_JOINING_GROUP_ZHAIN);
  ASSERT (uc_joining_group_byname ("Zhain") == UC_JOINING_GROUP_ZHAIN);
  ASSERT (uc_joining_group_byname ("SEEN WITH 3 DOTS BELOW AND 3 DOTS ABOVE") < 0);
  ASSERT (uc_joining_group_byname ("X") < 0);
  ASSERT (uc_joining_group_byname ("") < 0);

  return 0;
}
