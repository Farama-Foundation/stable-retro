/* Test of the type-safe stack data type.
   Copyright (C) 2020-2021 Free Software Foundation, Inc.

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

/* Written by Marc Nieper-Wißkirchen <marc@nieper-wisskirchen.de>, 2020.  */

#include <config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "assure.h"
#include "xalloc.h"

#include "macros.h"

#define GL_STACK_ELEMENT int
#define GL_STACK_STORAGECLASS static
#include "stack.h"

#define GL_STACK_ELEMENT const char *
#define GL_STACK_STORAGECLASS static
#define GL_STACK_NAME string_stack
#include "stack.h"

int
main (void)
{
  stack_type int_stack;
  stack_init (&int_stack);
  ASSERT (stack_size (&int_stack) == 0);
  ASSERT (stack_empty (&int_stack));
  stack_push (&int_stack, 0);
  stack_push (&int_stack, 1);
  stack_push (&int_stack, 2);
  stack_push (&int_stack, 3);
  stack_push (&int_stack, 4);
  stack_push (&int_stack, 5);
  stack_push (&int_stack, 6);
  stack_push (&int_stack, 7);
  stack_push (&int_stack, 8);
  stack_push (&int_stack, 9);
  ASSERT (stack_size (&int_stack) == 10);
  ASSERT (!stack_empty (&int_stack));
  ASSERT (stack_top (&int_stack) == 9);
  ASSERT (stack_size (&int_stack) == 10);
  ASSERT (stack_pop (&int_stack) == 9);
  ASSERT (stack_size (&int_stack) == 9);
  stack_discard (&int_stack);
  ASSERT (stack_size (&int_stack) == 8);
  ASSERT (stack_top (&int_stack) == 7);
  stack_destroy (&int_stack);

  string_stack_type string_stack [1];
  string_stack_init (string_stack);
  string_stack_push (string_stack, "foo");
  ASSERT (STREQ (string_stack_pop (string_stack), "foo"));
  ASSERT (string_stack_empty (string_stack));
  string_stack_destroy (string_stack);

  return EXIT_SUCCESS;
}
