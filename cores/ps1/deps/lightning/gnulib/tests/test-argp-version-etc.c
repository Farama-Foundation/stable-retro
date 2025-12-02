/* Test suite for argp-version-etc.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.
   This file is part of the GNUlib Library.

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
#include "argp-version-etc.h"
#include "argp.h"

static char doc[] = "test for the argp-version-etc module";

struct argp test_argp =
{
  NULL,
  NULL,
  NULL,
  doc,
  NULL,
  NULL,
  NULL
};

const char *authors[] =
{
  "Sergey Poznyakoff",
  NULL
};

int
main (int argc, char **argv)
{
  argp_version_setup ("test-argp-version-etc", authors);
  return argp_parse (&test_argp, argc, argv, 0, NULL, NULL);
}
