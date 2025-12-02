/* Test of uN_strcmp() and uN_strcoll() functions.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

static void
test_strcmp (void)
{
  {
    static const UNIT input1[] = { 0 };
    static const UNIT input2[] = { 0 };
    ASSERT (U_STRCMP (input1, input2) == 0);
  }
  {
    static const UNIT input1[] = { 0 };
    static const UNIT input2[] = { 'f', 'o', 'o', 0 };
    ASSERT (U_STRCMP (input1, input2) < 0);
    ASSERT (U_STRCMP (input2, input1) > 0);
  }
  {
    static const UNIT input1[] = { 'f', 'o', 'o', 0 };
    static const UNIT input2[] = { 'f', 'o', 'o', 0 };
    ASSERT (U_STRCMP (input1, input2) == 0);
  }
  {
    static const UNIT input1[] = { 'f', 'o', 'o', 0 };
    static const UNIT input2[] = { 'b', 'a', 'r', 0 };
    ASSERT (U_STRCMP (input1, input2) > 0);
    ASSERT (U_STRCMP (input2, input1) < 0);
  }
  {
    static const UNIT input1[] = { 'f', 'o', 'o', 0 };
    static const UNIT input2[] = { 'f', 'o', 'o', 'b', 'a', 'r', 0 };
    ASSERT (U_STRCMP (input1, input2) < 0);
    ASSERT (U_STRCMP (input2, input1) > 0);
  }
  {
    static const UNIT input1[] = { 'o', 'o', 'm', 'p', 'h', 0 };
    static const UNIT input2[] = { 'o', 'o', 'p', 's', 0 };
    ASSERT (U_STRCMP (input1, input2) < 0);
    ASSERT (U_STRCMP (input2, input1) > 0);
  }
}
