/* Test of test whether a Unicode string is already case-folded.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

static void
test_ascii (int (*my_is) (const UNIT *, size_t, const char *, bool *))
{
  {
    static const UNIT input[] = { 'J', 'O', 'H', 'N', ' ', 'S', 'M', 'I', 'T', 'H' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
  {
    static const UNIT input[] = { 'J', 'o', 'h', 'n', ' ', 'S', 'm', 'i', 't', 'h' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
  {
    static const UNIT input[] = { 'j', 'o', 'h', 'n', ' ', 's', 'm', 'i', 't', 'h' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
}
