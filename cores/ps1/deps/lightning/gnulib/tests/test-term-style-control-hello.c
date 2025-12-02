/* Simple test program for the term-style-control module.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2019.

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

/* Specification.  */
#include "term-style-control.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "full-write.h"

/* This program outputs the line:     Hello Dr. Linus Pauling!
   with underlining here:                   _________________
   and a cyan background color here:            _____
 */

/* ECMA-48 / ISO 6429 escape sequences.  See
   https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
 */
static const char set_underline_on[] = "\033[4m";
static const char set_underline_off[] = "\033[24m";
static const char set_background_color_cyan[] = "\033[46m";
static const char set_background_color_default[] = "\033[49m";

struct term_style_user_data
{
  /* These fields are marked volatile, because they are accessed from the
     async-safe function async_set_attributes_from_default.  */
  bool volatile underline;
  bool volatile background_color_cyan;

  struct term_style_control_data ctrl_data;
};

static struct term_style_control_data *
get_control_data (struct term_style_user_data *user_data)
{
  return &user_data->ctrl_data;
}

static void
restore (struct term_style_user_data *user_data)
{
  fputs (set_underline_off, stdout);
  fputs (set_background_color_default, stdout);
  fflush (stdout);
}

static _GL_ASYNC_SAFE void
async_restore (struct term_style_user_data *user_data)
{
  /* No <stdio.h> calls here!  */
  full_write (STDOUT_FILENO, set_underline_off,
              strlen (set_underline_off));
  full_write (STDOUT_FILENO, set_background_color_default,
              strlen (set_background_color_default));
}

static _GL_ASYNC_SAFE void
async_set_attributes_from_default (struct term_style_user_data *user_data)
{
  /* No <stdio.h> calls here!  */
  if (user_data->underline)
    full_write (STDOUT_FILENO, set_underline_on,
                strlen (set_underline_on));
  if (user_data->background_color_cyan)
    full_write (STDOUT_FILENO, set_background_color_cyan,
                strlen (set_background_color_cyan));
}

static const struct term_style_controller controller =
{
  get_control_data,
  restore,
  async_restore,
  async_set_attributes_from_default
};

int
main (int argc, char *argv[])
{
  struct term_style_user_data user_data;

  /* Initialization.  */
  user_data.underline = false;
  user_data.background_color_cyan = false;

  activate_term_style_controller (&controller, &user_data, STDOUT_FILENO,
                                  TTYCTL_AUTO);

  /* As long as no styling is needed, we can stay in the default mode.  */
  fputs ("Hello ", stdout);
  fflush (stdout);

  /* Before any styling, enable the non-default mode.  */
  activate_term_non_default_mode (&controller, &user_data);

  /* Set user_data.underline *before* emitting the appropriate
     escape sequences, otherwise async_set_attributes_from_default will not
     do its job correctly.  */
  user_data.underline = true;
  fputs (set_underline_on, stdout);
  fflush (stdout);

  fputs ("Dr. ", stdout);
  fflush (stdout);

  /* Set user_data.background_color_cyan *before* emitting the appropriate
     escape sequences, otherwise async_set_attributes_from_default will not
     do its job correctly.  */
  user_data.background_color_cyan = true;
  fputs (set_background_color_cyan, stdout);
  fflush (stdout);

  fputs ("Linus", stdout);
  fflush (stdout);

  user_data.background_color_cyan = false;
  fputs (set_background_color_default, stdout);
  fflush (stdout);

  fputs (" Pauling", stdout);
  fflush (stdout);

  user_data.underline = false;
  fputs (set_underline_off, stdout);
  fflush (stdout);

  /* Needed as a prerequisite of the deactivate_term_style_controller call
     below.  */
  deactivate_term_non_default_mode (&controller, &user_data);

  fputs ("!\n", stdout);

  /* If the user_data was allocated in heap memory, with indefinite extent,
     this call would be optional.  But since we have allocated it on the
     stack, we must deactivate it before it goes out of scope.  Otherwise
     we get undefined behaviour in an atexit() handler.  */
  deactivate_term_style_controller (&controller, &user_data);

  return 0;
}
