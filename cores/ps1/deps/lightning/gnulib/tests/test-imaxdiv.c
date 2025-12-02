/*
 * Copyright (C) 2011-2021 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <inttypes.h>

#include "signature.h"
SIGNATURE_CHECK (imaxdiv, imaxdiv_t, (intmax_t, intmax_t));

#include "macros.h"

int
main (void)
{
  {
    imaxdiv_t result = imaxdiv (101, 7);
    ASSERT (result.quot == 14);
    ASSERT (result.rem == 3);
  }
  {
    imaxdiv_t result = imaxdiv (-101, 7);
    ASSERT (result.quot == -14);
    ASSERT (result.rem == -3);
  }
  {
    imaxdiv_t result = imaxdiv (101, -7);
    ASSERT (result.quot == -14);
    ASSERT (result.rem == 3);
  }
  {
    imaxdiv_t result = imaxdiv (-101, -7);
    ASSERT (result.quot == 14);
    ASSERT (result.rem == -3);
  }

  return 0;
}
