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
SIGNATURE_CHECK (imaxabs, intmax_t, (intmax_t));

#include "macros.h"

int
main (void)
{
  {
    intmax_t result = imaxabs (0);
    ASSERT (result == 0);
  }
  {
    intmax_t result = imaxabs (23);
    ASSERT (result == 23);
  }
  {
    intmax_t result = imaxabs (-23);
    ASSERT (result == 23);
  }

  return 0;
}
