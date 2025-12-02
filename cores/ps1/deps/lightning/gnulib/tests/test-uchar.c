/* Test of <uchar.h> substitute.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2019.  */

#include <config.h>

#include <uchar.h>

#include "verify.h"

/* Check that the types are defined.  */
mbstate_t a = { 0 };
size_t b = 5;
char16_t c = 'x';
char32_t d = 'y';

/* Check that char16_t and char32_t are unsigned types.  */
verify ((char16_t)(-1) >= 0);
#if !defined __HP_cc
verify ((char32_t)(-1) >= 0);
#endif

/* Check that char32_t is at least 31 bits wide.  */
verify ((char32_t)0x7FFFFFFF != (char32_t)0x3FFFFFFF);

/* Check that _GL_LARGE_CHAR32_T is correctly defined.  */
#if _GL_LARGE_CHAR32_T
verify (sizeof (char32_t) > sizeof (wchar_t));
#else
verify (sizeof (char32_t) == sizeof (wchar_t));
#endif

int
main (void)
{
  return 0;
}
