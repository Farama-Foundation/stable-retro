/* Test of macros for declaring functions as non-returning.
   Copyright (C) 2017-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2017.  */

#include <config.h>

#include <noreturn.h>

/* Test _GL_NORETURN_FUNC on function declarations.  */

extern "C" { _GL_NORETURN_FUNC void func1_c (void); }
_GL_NORETURN_FUNC extern void func1_cxx (void);

/* Test _GL_NORETURN_FUNC on function definitions.  */

_GL_NORETURN_FUNC void funcd_cxx (void)
{
  for (;;)
    ;
}

/* Test _GL_NORETURN_FUNCPTR.  */

_GL_NORETURN_FUNCPTR void (*func1_c_ptr) (void) = func1_c;
_GL_NORETURN_FUNCPTR void (*func1_cxx_ptr) (void) = func1_cxx;
_GL_NORETURN_FUNCPTR void (*funcd_cxx_ptr) (void) = funcd_cxx;

/* These could also be defined in a separate compilation unit.  */

void func1_c (void)
{
  for (;;)
    ;
}

void func1_cxx (void)
{
  for (;;)
    ;
}


int
main ()
{
}
