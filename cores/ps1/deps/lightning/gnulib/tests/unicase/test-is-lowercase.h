/* Test of test whether a Unicode string is entirely lower case.
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
  /* Test cases from Unicode 5.0, chapter 3.  */
  {
    static const UNIT input[] = { 'a' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
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
  {
    static const UNIT input[] = { 'a', '2' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const UNIT input[] = { '3' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }

  /* Test cases from Unicode 5.1.0.  */
  {
    static const UNIT input[] = { 'H', 'H' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
  {
    static const UNIT input[] = { 'h', 'h' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == true);
  }
  {
    static const UNIT input[] = { 'H', 'h' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
  {
    static const UNIT input[] = { 'h', 'H' };
    bool result;

    ASSERT (my_is (input, SIZEOF (input), NULL, &result) == 0);
    ASSERT (result == false);
  }
}
