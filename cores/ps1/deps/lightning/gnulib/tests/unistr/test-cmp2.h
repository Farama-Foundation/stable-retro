/* Test of uN_cmp2() functions.
   Copyright (C) 2008-2021 Free Software Foundation, Inc.

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

/* Written by Simon Josefsson and Bruno Haible <bruno@clisp.org>, 2010.  */

int
main ()
{
  {
    static const UNIT input1[] = { 'a' };
    static const UNIT input2[] = { 'b' };
    ASSERT (U_CMP2 (input1, 0, input2, 0) == 0);
    ASSERT (U_CMP2 (input1, 1, input2, 0) > 0);
    ASSERT (U_CMP2 (input1, 0, input2, 1) < 0);
  }
  {
    static const UNIT input1[] = { 'f', 'o', 'o', 0 };
    static const UNIT input2[] = { 'f', 'o', 'o', 'b', 'a', 'r', 0 };
    ASSERT (U_CMP2 (input1, 3, input2, 3) == 0);
    ASSERT (U_CMP2 (input1, 4, input2, 3) > 0);
    ASSERT (U_CMP2 (input2, 3, input1, 4) < 0);
    ASSERT (U_CMP2 (input1, 3, input2, 4) < 0);
    ASSERT (U_CMP2 (input2, 4, input1, 3) > 0);
    ASSERT (U_CMP2 (input1, 4, input2, 4) < 0);
    ASSERT (U_CMP2 (input2, 4, input1, 4) > 0);
    ASSERT (U_CMP2 (input1, 3, input2, 7) < 0);
    ASSERT (U_CMP2 (input2, 7, input1, 3) > 0);
  }
  {
    static const UNIT input1[] = { 'f', 'o', 'o' };
    static const UNIT input2[] = { 'm', 'o', 'o' };
    ASSERT (U_CMP2 (input1, 3, input2, 3) < 0);
    ASSERT (U_CMP2 (input2, 3, input1, 3) > 0);
  }
  {
    static const UNIT input1[] = { 'o', 'o', 'm', 'p', 'h' };
    static const UNIT input2[] = { 'o', 'o', 'p', 's' };
    ASSERT (U_CMP2 (input1, 5, input2, 4) < 0);
    ASSERT (U_CMP2 (input2, 4, input1, 5) > 0);
  }

  return 0;
}
