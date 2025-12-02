/* Test the Unicode character type functions.
   Copyright (C) 2007-2011 Free Software Foundation, Inc.

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
  ASSERT (uc_bidi_class_byname ("L") == UC_BIDI_L);
  ASSERT (uc_bidi_class_byname ("LRE") == UC_BIDI_LRE);
  ASSERT (uc_bidi_class_byname ("LRO") == UC_BIDI_LRO);
  ASSERT (uc_bidi_class_byname ("R") == UC_BIDI_R);
  ASSERT (uc_bidi_class_byname ("AL") == UC_BIDI_AL);
  ASSERT (uc_bidi_class_byname ("RLE") == UC_BIDI_RLE);
  ASSERT (uc_bidi_class_byname ("RLO") == UC_BIDI_RLO);
  ASSERT (uc_bidi_class_byname ("PDF") == UC_BIDI_PDF);
  ASSERT (uc_bidi_class_byname ("EN") == UC_BIDI_EN);
  ASSERT (uc_bidi_class_byname ("ES") == UC_BIDI_ES);
  ASSERT (uc_bidi_class_byname ("ET") == UC_BIDI_ET);
  ASSERT (uc_bidi_class_byname ("AN") == UC_BIDI_AN);
  ASSERT (uc_bidi_class_byname ("CS") == UC_BIDI_CS);
  ASSERT (uc_bidi_class_byname ("NSM") == UC_BIDI_NSM);
  ASSERT (uc_bidi_class_byname ("BN") == UC_BIDI_BN);
  ASSERT (uc_bidi_class_byname ("B") == UC_BIDI_B);
  ASSERT (uc_bidi_class_byname ("S") == UC_BIDI_S);
  ASSERT (uc_bidi_class_byname ("WS") == UC_BIDI_WS);
  ASSERT (uc_bidi_class_byname ("ON") == UC_BIDI_ON);

  ASSERT (uc_bidi_class_byname ("ARABIC LETTER") == UC_BIDI_AL);
  ASSERT (uc_bidi_class_byname ("Arabic Letter") == UC_BIDI_AL);
  ASSERT (uc_bidi_class_byname ("Arabic_Letter") == UC_BIDI_AL);
  ASSERT (uc_bidi_class_byname ("ArabicLetter") == UC_BIDI_AL);
  ASSERT (uc_bidi_class_byname ("ARABIC NUMBER") == UC_BIDI_AN);
  ASSERT (uc_bidi_class_byname ("Arabic Number") == UC_BIDI_AN);
  ASSERT (uc_bidi_class_byname ("Arabic_Number") == UC_BIDI_AN);
  ASSERT (uc_bidi_class_byname ("ArabicNumber") == UC_BIDI_AN);
  ASSERT (uc_bidi_class_byname ("PARAGRAPH SEPARATOR") == UC_BIDI_B);
  ASSERT (uc_bidi_class_byname ("Paragraph Separator") == UC_BIDI_B);
  ASSERT (uc_bidi_class_byname ("Paragraph_Separator") == UC_BIDI_B);
  ASSERT (uc_bidi_class_byname ("ParagraphSeparator") == UC_BIDI_B);
  ASSERT (uc_bidi_class_byname ("BOUNDARY NEUTRAL") == UC_BIDI_BN);
  ASSERT (uc_bidi_class_byname ("Boundary Neutral") == UC_BIDI_BN);
  ASSERT (uc_bidi_class_byname ("Boundary_Neutral") == UC_BIDI_BN);
  ASSERT (uc_bidi_class_byname ("BoundaryNeutral") == UC_BIDI_BN);
  ASSERT (uc_bidi_class_byname ("COMMON SEPARATOR") == UC_BIDI_CS);
  ASSERT (uc_bidi_class_byname ("Common Separator") == UC_BIDI_CS);
  ASSERT (uc_bidi_class_byname ("Common_Separator") == UC_BIDI_CS);
  ASSERT (uc_bidi_class_byname ("CommonSeparator") == UC_BIDI_CS);
  ASSERT (uc_bidi_class_byname ("EUROPEAN NUMBER") == UC_BIDI_EN);
  ASSERT (uc_bidi_class_byname ("European Number") == UC_BIDI_EN);
  ASSERT (uc_bidi_class_byname ("European_Number") == UC_BIDI_EN);
  ASSERT (uc_bidi_class_byname ("EuropeanNumber") == UC_BIDI_EN);
  ASSERT (uc_bidi_class_byname ("EUROPEAN SEPARATOR") == UC_BIDI_ES);
  ASSERT (uc_bidi_class_byname ("European Separator") == UC_BIDI_ES);
  ASSERT (uc_bidi_class_byname ("European_Separator") == UC_BIDI_ES);
  ASSERT (uc_bidi_class_byname ("EuropeanSeparator") == UC_BIDI_ES);
  ASSERT (uc_bidi_class_byname ("EUROPEAN TERMINATOR") == UC_BIDI_ET);
  ASSERT (uc_bidi_class_byname ("European Terminator") == UC_BIDI_ET);
  ASSERT (uc_bidi_class_byname ("European_Terminator") == UC_BIDI_ET);
  ASSERT (uc_bidi_class_byname ("EuropeanTerminator") == UC_BIDI_ET);
  ASSERT (uc_bidi_class_byname ("LEFT TO RIGHT") == UC_BIDI_L);
  ASSERT (uc_bidi_class_byname ("Left To Right") == UC_BIDI_L);
  ASSERT (uc_bidi_class_byname ("Left_To_Right") == UC_BIDI_L);
  ASSERT (uc_bidi_class_byname ("LeftToRight") == UC_BIDI_L);
  ASSERT (uc_bidi_class_byname ("LEFT TO RIGHT EMBEDDING") == UC_BIDI_LRE);
  ASSERT (uc_bidi_class_byname ("Left To Right Embedding") == UC_BIDI_LRE);
  ASSERT (uc_bidi_class_byname ("Left_To_Right_Embedding") == UC_BIDI_LRE);
  ASSERT (uc_bidi_class_byname ("LeftToRightEmbedding") == UC_BIDI_LRE);
  ASSERT (uc_bidi_class_byname ("LEFT TO RIGHT OVERRIDE") == UC_BIDI_LRO);
  ASSERT (uc_bidi_class_byname ("Left To Right Override") == UC_BIDI_LRO);
  ASSERT (uc_bidi_class_byname ("Left_To_Right_Override") == UC_BIDI_LRO);
  ASSERT (uc_bidi_class_byname ("LeftToRightOverride") == UC_BIDI_LRO);
  ASSERT (uc_bidi_class_byname ("NONSPACING MARK") == UC_BIDI_NSM);
  ASSERT (uc_bidi_class_byname ("Nonspacing Mark") == UC_BIDI_NSM);
  ASSERT (uc_bidi_class_byname ("Nonspacing_Mark") == UC_BIDI_NSM);
  ASSERT (uc_bidi_class_byname ("NonspacingMark") == UC_BIDI_NSM);
  ASSERT (uc_bidi_class_byname ("OTHER NEUTRAL") == UC_BIDI_ON);
  ASSERT (uc_bidi_class_byname ("Other Neutral") == UC_BIDI_ON);
  ASSERT (uc_bidi_class_byname ("Other_Neutral") == UC_BIDI_ON);
  ASSERT (uc_bidi_class_byname ("OtherNeutral") == UC_BIDI_ON);
  ASSERT (uc_bidi_class_byname ("POP DIRECTIONAL FORMAT") == UC_BIDI_PDF);
  ASSERT (uc_bidi_class_byname ("Pop Directional Format") == UC_BIDI_PDF);
  ASSERT (uc_bidi_class_byname ("Pop_Directional_Format") == UC_BIDI_PDF);
  ASSERT (uc_bidi_class_byname ("PopDirectionalFormat") == UC_BIDI_PDF);
  ASSERT (uc_bidi_class_byname ("RIGHT TO LEFT") == UC_BIDI_R);
  ASSERT (uc_bidi_class_byname ("Right To Left") == UC_BIDI_R);
  ASSERT (uc_bidi_class_byname ("Right_To_Left") == UC_BIDI_R);
  ASSERT (uc_bidi_class_byname ("RightToLeft") == UC_BIDI_R);
  ASSERT (uc_bidi_class_byname ("RIGHT TO LEFT EMBEDDING") == UC_BIDI_RLE);
  ASSERT (uc_bidi_class_byname ("Right To Left Embedding") == UC_BIDI_RLE);
  ASSERT (uc_bidi_class_byname ("Right_To_Left_Embedding") == UC_BIDI_RLE);
  ASSERT (uc_bidi_class_byname ("RightToLeftEmbedding") == UC_BIDI_RLE);
  ASSERT (uc_bidi_class_byname ("RIGHT TO LEFT OVERRIDE") == UC_BIDI_RLO);
  ASSERT (uc_bidi_class_byname ("Right To Left Override") == UC_BIDI_RLO);
  ASSERT (uc_bidi_class_byname ("Right_To_Left_Override") == UC_BIDI_RLO);
  ASSERT (uc_bidi_class_byname ("RightToLeftOverride") == UC_BIDI_RLO);
  ASSERT (uc_bidi_class_byname ("SEGMENT SEPARATOR") == UC_BIDI_S);
  ASSERT (uc_bidi_class_byname ("Segment Separator") == UC_BIDI_S);
  ASSERT (uc_bidi_class_byname ("Segment_Separator") == UC_BIDI_S);
  ASSERT (uc_bidi_class_byname ("SegmentSeparator") == UC_BIDI_S);
  ASSERT (uc_bidi_class_byname ("WHITE SPACE") == UC_BIDI_WS);
  ASSERT (uc_bidi_class_byname ("White Space") == UC_BIDI_WS);
  ASSERT (uc_bidi_class_byname ("White_Space") == UC_BIDI_WS);
  ASSERT (uc_bidi_class_byname ("WhiteSpace") == UC_BIDI_WS);

  ASSERT (uc_bidi_class_byname ("X") < 0);
  ASSERT (uc_bidi_class_byname ("") < 0);

  return 0;
}
