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

#include <stdbool.h>
#include <string.h>

#include "macros.h"

static bool
category_equals (uc_general_category_t category1,
                 uc_general_category_t category2)
{
  return (category1.bitmask == category2.bitmask
          && category1.generic == category2.generic
          && (category1.generic
              ? category1.lookup.lookup_fn == category2.lookup.lookup_fn
              : category1.lookup.table == category2.lookup.table));
}

int
main ()
{
  ASSERT (category_equals (uc_general_category_byname ("L"), UC_CATEGORY_L));
  ASSERT (category_equals (uc_general_category_byname ("LC"), UC_CATEGORY_LC));
  ASSERT (category_equals (uc_general_category_byname ("Lu"), UC_CATEGORY_Lu));
  ASSERT (category_equals (uc_general_category_byname ("Ll"), UC_CATEGORY_Ll));
  ASSERT (category_equals (uc_general_category_byname ("Lt"), UC_CATEGORY_Lt));
  ASSERT (category_equals (uc_general_category_byname ("Lm"), UC_CATEGORY_Lm));
  ASSERT (category_equals (uc_general_category_byname ("Lo"), UC_CATEGORY_Lo));
  ASSERT (category_equals (uc_general_category_byname ("M"), UC_CATEGORY_M));
  ASSERT (category_equals (uc_general_category_byname ("Mn"), UC_CATEGORY_Mn));
  ASSERT (category_equals (uc_general_category_byname ("Mc"), UC_CATEGORY_Mc));
  ASSERT (category_equals (uc_general_category_byname ("Me"), UC_CATEGORY_Me));
  ASSERT (category_equals (uc_general_category_byname ("N"), UC_CATEGORY_N));
  ASSERT (category_equals (uc_general_category_byname ("Nd"), UC_CATEGORY_Nd));
  ASSERT (category_equals (uc_general_category_byname ("Nl"), UC_CATEGORY_Nl));
  ASSERT (category_equals (uc_general_category_byname ("No"), UC_CATEGORY_No));
  ASSERT (category_equals (uc_general_category_byname ("P"), UC_CATEGORY_P));
  ASSERT (category_equals (uc_general_category_byname ("Pc"), UC_CATEGORY_Pc));
  ASSERT (category_equals (uc_general_category_byname ("Pd"), UC_CATEGORY_Pd));
  ASSERT (category_equals (uc_general_category_byname ("Ps"), UC_CATEGORY_Ps));
  ASSERT (category_equals (uc_general_category_byname ("Pe"), UC_CATEGORY_Pe));
  ASSERT (category_equals (uc_general_category_byname ("Pi"), UC_CATEGORY_Pi));
  ASSERT (category_equals (uc_general_category_byname ("Pf"), UC_CATEGORY_Pf));
  ASSERT (category_equals (uc_general_category_byname ("Po"), UC_CATEGORY_Po));
  ASSERT (category_equals (uc_general_category_byname ("S"), UC_CATEGORY_S));
  ASSERT (category_equals (uc_general_category_byname ("Sm"), UC_CATEGORY_Sm));
  ASSERT (category_equals (uc_general_category_byname ("Sc"), UC_CATEGORY_Sc));
  ASSERT (category_equals (uc_general_category_byname ("Sk"), UC_CATEGORY_Sk));
  ASSERT (category_equals (uc_general_category_byname ("So"), UC_CATEGORY_So));
  ASSERT (category_equals (uc_general_category_byname ("Z"), UC_CATEGORY_Z));
  ASSERT (category_equals (uc_general_category_byname ("Zs"), UC_CATEGORY_Zs));
  ASSERT (category_equals (uc_general_category_byname ("Zl"), UC_CATEGORY_Zl));
  ASSERT (category_equals (uc_general_category_byname ("Zp"), UC_CATEGORY_Zp));
  ASSERT (category_equals (uc_general_category_byname ("C"), UC_CATEGORY_C));
  ASSERT (category_equals (uc_general_category_byname ("Cc"), UC_CATEGORY_Cc));
  ASSERT (category_equals (uc_general_category_byname ("Cf"), UC_CATEGORY_Cf));
  ASSERT (category_equals (uc_general_category_byname ("Cs"), UC_CATEGORY_Cs));
  ASSERT (category_equals (uc_general_category_byname ("Co"), UC_CATEGORY_Co));
  ASSERT (category_equals (uc_general_category_byname ("Cn"), UC_CATEGORY_Cn));

  ASSERT (category_equals (uc_general_category_byname ("LETTER"), UC_CATEGORY_L));
  ASSERT (category_equals (uc_general_category_byname ("Letter"), UC_CATEGORY_L));
  ASSERT (category_equals (uc_general_category_byname ("CASED LETTER"), UC_CATEGORY_LC));
  ASSERT (category_equals (uc_general_category_byname ("Cased Letter"), UC_CATEGORY_LC));
  ASSERT (category_equals (uc_general_category_byname ("Cased_Letter"), UC_CATEGORY_LC));
  ASSERT (category_equals (uc_general_category_byname ("CasedLetter"), UC_CATEGORY_LC));
  ASSERT (category_equals (uc_general_category_byname ("UPPERCASE LETTER"), UC_CATEGORY_Lu));
  ASSERT (category_equals (uc_general_category_byname ("Uppercase Letter"), UC_CATEGORY_Lu));
  ASSERT (category_equals (uc_general_category_byname ("Uppercase_Letter"), UC_CATEGORY_Lu));
  ASSERT (category_equals (uc_general_category_byname ("UppercaseLetter"), UC_CATEGORY_Lu));
  ASSERT (category_equals (uc_general_category_byname ("LOWERCASE LETTER"), UC_CATEGORY_Ll));
  ASSERT (category_equals (uc_general_category_byname ("Lowercase Letter"), UC_CATEGORY_Ll));
  ASSERT (category_equals (uc_general_category_byname ("Lowercase_Letter"), UC_CATEGORY_Ll));
  ASSERT (category_equals (uc_general_category_byname ("LowercaseLetter"), UC_CATEGORY_Ll));
  ASSERT (category_equals (uc_general_category_byname ("TITLECASE LETTER"), UC_CATEGORY_Lt));
  ASSERT (category_equals (uc_general_category_byname ("Titlecase Letter"), UC_CATEGORY_Lt));
  ASSERT (category_equals (uc_general_category_byname ("Titlecase_Letter"), UC_CATEGORY_Lt));
  ASSERT (category_equals (uc_general_category_byname ("TitlecaseLetter"), UC_CATEGORY_Lt));
  ASSERT (category_equals (uc_general_category_byname ("MODIFIER LETTER"), UC_CATEGORY_Lm));
  ASSERT (category_equals (uc_general_category_byname ("Modifier Letter"), UC_CATEGORY_Lm));
  ASSERT (category_equals (uc_general_category_byname ("Modifier_Letter"), UC_CATEGORY_Lm));
  ASSERT (category_equals (uc_general_category_byname ("ModifierLetter"), UC_CATEGORY_Lm));
  ASSERT (category_equals (uc_general_category_byname ("OTHER LETTER"), UC_CATEGORY_Lo));
  ASSERT (category_equals (uc_general_category_byname ("Other Letter"), UC_CATEGORY_Lo));
  ASSERT (category_equals (uc_general_category_byname ("Other_Letter"), UC_CATEGORY_Lo));
  ASSERT (category_equals (uc_general_category_byname ("OtherLetter"), UC_CATEGORY_Lo));
  ASSERT (category_equals (uc_general_category_byname ("MARK"), UC_CATEGORY_M));
  ASSERT (category_equals (uc_general_category_byname ("Mark"), UC_CATEGORY_M));
  ASSERT (category_equals (uc_general_category_byname ("NONSPACING MARK"), UC_CATEGORY_Mn));
  ASSERT (category_equals (uc_general_category_byname ("Nonspacing Mark"), UC_CATEGORY_Mn));
  ASSERT (category_equals (uc_general_category_byname ("Nonspacing_Mark"), UC_CATEGORY_Mn));
  ASSERT (category_equals (uc_general_category_byname ("NonspacingMark"), UC_CATEGORY_Mn));
  ASSERT (category_equals (uc_general_category_byname ("SPACING MARK"), UC_CATEGORY_Mc));
  ASSERT (category_equals (uc_general_category_byname ("Spacing Mark"), UC_CATEGORY_Mc));
  ASSERT (category_equals (uc_general_category_byname ("Spacing_Mark"), UC_CATEGORY_Mc));
  ASSERT (category_equals (uc_general_category_byname ("SpacingMark"), UC_CATEGORY_Mc));
  ASSERT (category_equals (uc_general_category_byname ("ENCLOSING MARK"), UC_CATEGORY_Me));
  ASSERT (category_equals (uc_general_category_byname ("Enclosing Mark"), UC_CATEGORY_Me));
  ASSERT (category_equals (uc_general_category_byname ("Enclosing_Mark"), UC_CATEGORY_Me));
  ASSERT (category_equals (uc_general_category_byname ("EnclosingMark"), UC_CATEGORY_Me));
  ASSERT (category_equals (uc_general_category_byname ("NUMBER"), UC_CATEGORY_N));
  ASSERT (category_equals (uc_general_category_byname ("Number"), UC_CATEGORY_N));
  ASSERT (category_equals (uc_general_category_byname ("DECIMAL NUMBER"), UC_CATEGORY_Nd));
  ASSERT (category_equals (uc_general_category_byname ("Decimal Number"), UC_CATEGORY_Nd));
  ASSERT (category_equals (uc_general_category_byname ("Decimal_Number"), UC_CATEGORY_Nd));
  ASSERT (category_equals (uc_general_category_byname ("DecimalNumber"), UC_CATEGORY_Nd));
  ASSERT (category_equals (uc_general_category_byname ("LETTER NUMBER"), UC_CATEGORY_Nl));
  ASSERT (category_equals (uc_general_category_byname ("Letter Number"), UC_CATEGORY_Nl));
  ASSERT (category_equals (uc_general_category_byname ("Letter_Number"), UC_CATEGORY_Nl));
  ASSERT (category_equals (uc_general_category_byname ("LetterNumber"), UC_CATEGORY_Nl));
  ASSERT (category_equals (uc_general_category_byname ("OTHER NUMBER"), UC_CATEGORY_No));
  ASSERT (category_equals (uc_general_category_byname ("Other Number"), UC_CATEGORY_No));
  ASSERT (category_equals (uc_general_category_byname ("Other_Number"), UC_CATEGORY_No));
  ASSERT (category_equals (uc_general_category_byname ("OtherNumber"), UC_CATEGORY_No));
  ASSERT (category_equals (uc_general_category_byname ("PUNCTUATION"), UC_CATEGORY_P));
  ASSERT (category_equals (uc_general_category_byname ("Punctuation"), UC_CATEGORY_P));
  ASSERT (category_equals (uc_general_category_byname ("CONNECTOR PUNCTUATION"), UC_CATEGORY_Pc));
  ASSERT (category_equals (uc_general_category_byname ("Connector Punctuation"), UC_CATEGORY_Pc));
  ASSERT (category_equals (uc_general_category_byname ("Connector_Punctuation"), UC_CATEGORY_Pc));
  ASSERT (category_equals (uc_general_category_byname ("ConnectorPunctuation"), UC_CATEGORY_Pc));
  ASSERT (category_equals (uc_general_category_byname ("DASH PUNCTUATION"), UC_CATEGORY_Pd));
  ASSERT (category_equals (uc_general_category_byname ("Dash Punctuation"), UC_CATEGORY_Pd));
  ASSERT (category_equals (uc_general_category_byname ("Dash_Punctuation"), UC_CATEGORY_Pd));
  ASSERT (category_equals (uc_general_category_byname ("DashPunctuation"), UC_CATEGORY_Pd));
  ASSERT (category_equals (uc_general_category_byname ("OPEN PUNCTUATION"), UC_CATEGORY_Ps));
  ASSERT (category_equals (uc_general_category_byname ("Open Punctuation"), UC_CATEGORY_Ps));
  ASSERT (category_equals (uc_general_category_byname ("Open_Punctuation"), UC_CATEGORY_Ps));
  ASSERT (category_equals (uc_general_category_byname ("OpenPunctuation"), UC_CATEGORY_Ps));
  ASSERT (category_equals (uc_general_category_byname ("CLOSE PUNCTUATION"), UC_CATEGORY_Pe));
  ASSERT (category_equals (uc_general_category_byname ("Close Punctuation"), UC_CATEGORY_Pe));
  ASSERT (category_equals (uc_general_category_byname ("Close_Punctuation"), UC_CATEGORY_Pe));
  ASSERT (category_equals (uc_general_category_byname ("ClosePunctuation"), UC_CATEGORY_Pe));
  ASSERT (category_equals (uc_general_category_byname ("INITIAL PUNCTUATION"), UC_CATEGORY_Pi));
  ASSERT (category_equals (uc_general_category_byname ("Initial Punctuation"), UC_CATEGORY_Pi));
  ASSERT (category_equals (uc_general_category_byname ("Initial_Punctuation"), UC_CATEGORY_Pi));
  ASSERT (category_equals (uc_general_category_byname ("InitialPunctuation"), UC_CATEGORY_Pi));
  ASSERT (category_equals (uc_general_category_byname ("FINAL PUNCTUATION"), UC_CATEGORY_Pf));
  ASSERT (category_equals (uc_general_category_byname ("Final Punctuation"), UC_CATEGORY_Pf));
  ASSERT (category_equals (uc_general_category_byname ("Final_Punctuation"), UC_CATEGORY_Pf));
  ASSERT (category_equals (uc_general_category_byname ("FinalPunctuation"), UC_CATEGORY_Pf));
  ASSERT (category_equals (uc_general_category_byname ("OTHER PUNCTUATION"), UC_CATEGORY_Po));
  ASSERT (category_equals (uc_general_category_byname ("Other Punctuation"), UC_CATEGORY_Po));
  ASSERT (category_equals (uc_general_category_byname ("Other_Punctuation"), UC_CATEGORY_Po));
  ASSERT (category_equals (uc_general_category_byname ("OtherPunctuation"), UC_CATEGORY_Po));
  ASSERT (category_equals (uc_general_category_byname ("SYMBOL"), UC_CATEGORY_S));
  ASSERT (category_equals (uc_general_category_byname ("Symbol"), UC_CATEGORY_S));
  ASSERT (category_equals (uc_general_category_byname ("MATH SYMBOL"), UC_CATEGORY_Sm));
  ASSERT (category_equals (uc_general_category_byname ("Math Symbol"), UC_CATEGORY_Sm));
  ASSERT (category_equals (uc_general_category_byname ("Math_Symbol"), UC_CATEGORY_Sm));
  ASSERT (category_equals (uc_general_category_byname ("MathSymbol"), UC_CATEGORY_Sm));
  ASSERT (category_equals (uc_general_category_byname ("CURRENCY SYMBOL"), UC_CATEGORY_Sc));
  ASSERT (category_equals (uc_general_category_byname ("Currency Symbol"), UC_CATEGORY_Sc));
  ASSERT (category_equals (uc_general_category_byname ("Currency_Symbol"), UC_CATEGORY_Sc));
  ASSERT (category_equals (uc_general_category_byname ("CurrencySymbol"), UC_CATEGORY_Sc));
  ASSERT (category_equals (uc_general_category_byname ("MODIFIER SYMBOL"), UC_CATEGORY_Sk));
  ASSERT (category_equals (uc_general_category_byname ("Modifier Symbol"), UC_CATEGORY_Sk));
  ASSERT (category_equals (uc_general_category_byname ("Modifier_Symbol"), UC_CATEGORY_Sk));
  ASSERT (category_equals (uc_general_category_byname ("ModifierSymbol"), UC_CATEGORY_Sk));
  ASSERT (category_equals (uc_general_category_byname ("OTHER SYMBOL"), UC_CATEGORY_So));
  ASSERT (category_equals (uc_general_category_byname ("Other Symbol"), UC_CATEGORY_So));
  ASSERT (category_equals (uc_general_category_byname ("Other_Symbol"), UC_CATEGORY_So));
  ASSERT (category_equals (uc_general_category_byname ("OtherSymbol"), UC_CATEGORY_So));
  ASSERT (category_equals (uc_general_category_byname ("SEPARATOR"), UC_CATEGORY_Z));
  ASSERT (category_equals (uc_general_category_byname ("Separator"), UC_CATEGORY_Z));
  ASSERT (category_equals (uc_general_category_byname ("SPACE SEPARATOR"), UC_CATEGORY_Zs));
  ASSERT (category_equals (uc_general_category_byname ("Space Separator"), UC_CATEGORY_Zs));
  ASSERT (category_equals (uc_general_category_byname ("Space_Separator"), UC_CATEGORY_Zs));
  ASSERT (category_equals (uc_general_category_byname ("SpaceSeparator"), UC_CATEGORY_Zs));
  ASSERT (category_equals (uc_general_category_byname ("LINE SEPARATOR"), UC_CATEGORY_Zl));
  ASSERT (category_equals (uc_general_category_byname ("Line Separator"), UC_CATEGORY_Zl));
  ASSERT (category_equals (uc_general_category_byname ("Line_Separator"), UC_CATEGORY_Zl));
  ASSERT (category_equals (uc_general_category_byname ("LineSeparator"), UC_CATEGORY_Zl));
  ASSERT (category_equals (uc_general_category_byname ("PARAGRAPH SEPARATOR"), UC_CATEGORY_Zp));
  ASSERT (category_equals (uc_general_category_byname ("Paragraph Separator"), UC_CATEGORY_Zp));
  ASSERT (category_equals (uc_general_category_byname ("Paragraph_Separator"), UC_CATEGORY_Zp));
  ASSERT (category_equals (uc_general_category_byname ("ParagraphSeparator"), UC_CATEGORY_Zp));
  ASSERT (category_equals (uc_general_category_byname ("OTHER"), UC_CATEGORY_C));
  ASSERT (category_equals (uc_general_category_byname ("Other"), UC_CATEGORY_C));
  ASSERT (category_equals (uc_general_category_byname ("CONTROL"), UC_CATEGORY_Cc));
  ASSERT (category_equals (uc_general_category_byname ("Control"), UC_CATEGORY_Cc));
  ASSERT (category_equals (uc_general_category_byname ("FORMAT"), UC_CATEGORY_Cf));
  ASSERT (category_equals (uc_general_category_byname ("Format"), UC_CATEGORY_Cf));
  ASSERT (category_equals (uc_general_category_byname ("SURROGATE"), UC_CATEGORY_Cs));
  ASSERT (category_equals (uc_general_category_byname ("Surrogate"), UC_CATEGORY_Cs));
  ASSERT (category_equals (uc_general_category_byname ("PRIVATE USE"), UC_CATEGORY_Co));
  ASSERT (category_equals (uc_general_category_byname ("Private Use"), UC_CATEGORY_Co));
  ASSERT (category_equals (uc_general_category_byname ("Private_Use"), UC_CATEGORY_Co));
  ASSERT (category_equals (uc_general_category_byname ("PrivateUse"), UC_CATEGORY_Co));
  ASSERT (category_equals (uc_general_category_byname ("UNASSIGNED"), UC_CATEGORY_Cn));
  ASSERT (category_equals (uc_general_category_byname ("Unassigned"), UC_CATEGORY_Cn));

  uc_general_category_byname ("Nl");

  {
    uc_general_category_t ct = uc_general_category_byname ("Nd");
    unsigned int c;

    for (c = 0x30; c <= 0x39; c++)
      ASSERT (uc_is_general_category (c, ct));
    for (c = 0x40; c < 0x80; c++)
      ASSERT (!uc_is_general_category (c, ct));
  }

  {
    uc_general_category_t ct = uc_general_category_byname ("Foo");
    unsigned int c;

    for (c = 0; c < 0x110000; c++)
      ASSERT (!uc_is_general_category (c, ct));
  }

  return 0;
}
