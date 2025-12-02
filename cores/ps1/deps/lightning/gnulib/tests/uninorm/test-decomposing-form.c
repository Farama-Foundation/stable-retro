/* Test of decomposing variant of a normalization form.
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

#include <config.h>

#include "uninorm.h"

#include "macros.h"

int
main ()
{
  ASSERT (uninorm_decomposing_form (UNINORM_NFC) == UNINORM_NFD);
  ASSERT (uninorm_decomposing_form (UNINORM_NFD) == UNINORM_NFD);
  ASSERT (uninorm_decomposing_form (UNINORM_NFKC) == UNINORM_NFKD);
  ASSERT (uninorm_decomposing_form (UNINORM_NFKD) == UNINORM_NFKD);

  return 0;
}
