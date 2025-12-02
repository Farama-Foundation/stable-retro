/* Callee program for the tests of the exec*() functions.
   Copyright (C) 2020-2021 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

/* Do not use any gnulib replacements, since this program should
   link against as few libraries as possible.  */
#undef printf

int
main (int argc, char* argv[])
{
  /* Print the arguments.  */
  int i;
  printf ("argc = %d\n", argc);
  for (i = 1; i < argc; i++)
    printf ("argv[%d] = |%s|\n", i, argv[i]);

  /* Print a particular environment variable.  */
  const char *var = "Hommingberg";
  if (getenv (var) != NULL)
    printf ("%s = |%s|\n", var, getenv (var));

  return 49;
}
