/* Test of reallocarray function.
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

#include <config.h>

#include <stdlib.h>
#include <errno.h>

#include "signature.h"
SIGNATURE_CHECK (reallocarray, void *, (void *, size_t, size_t));

int
main ()
{
   size_t n;

   /* Check that reallocarray fails when requested to allocate a block
      of memory larger than SIZE_MAX bytes.  */
   for (n = 2; n != 0; n <<= 1)
     {
       if (reallocarray (NULL, (size_t) -1 / n + 1, n))
         return 1;

       /* Ensure that errno is correctly set.  */
       if (errno != ENOMEM)
         return 1;
     }

   return 0;
}
