/* Test of conversion of wide string to string.
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

#include <wchar.h>

#include "signature.h"
SIGNATURE_CHECK (wcstok, wchar_t *,
                 (wchar_t *wcs, const wchar_t *delim, wchar_t **ptr));

#include <string.h>

#include "macros.h"

int
main ()
{
  wchar_t string[] = L"hello  world!";
  wchar_t delim[] = L" ";
  wchar_t *ptr;
  wchar_t *ret;

  ret = wcstok (string, delim, &ptr);
  ASSERT (ret == string);
  ASSERT (memcmp (string, L"hello\0 world!", 14 * sizeof (wchar_t)) == 0);
  ASSERT (ptr == string + 6);

  ret = wcstok (NULL, delim, &ptr);
  ASSERT (ret == string + 7);
  ASSERT (memcmp (string, L"hello\0 world!", 14 * sizeof (wchar_t)) == 0);
  ASSERT (ptr == NULL || *ptr == L'\0');

  ret = wcstok (NULL, delim, &ptr);
  ASSERT (ret == NULL);

  return 0;
}
