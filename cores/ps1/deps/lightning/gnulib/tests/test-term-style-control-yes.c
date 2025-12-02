/* Interactive test program for the term-style-control module.
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

/* This program outputs an endless amount of lines, each consisting of a
   single 'y', in red color and underlined.
   It can be used to exercise race conditions caused by
     - simultaneous keyboard input on the terminal,
     - pressing Ctrl-C,
     - pressing Ctrl-Z and then "fg".  */

/* ECMA-48 / ISO 6429 escape sequences.  See
   https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
 */
static const char set_underline_on[] = "\033[4m";
static const char set_underline_off[] = "\033[24m";
static const char set_foreground_color_red[] = "\033[31m";
static const char set_foreground_color_default[] = "\033[39m";

struct term_style_user_data
{
  /* This field is marked volatile, because it is accessed from the
     async-safe function async_set_attributes_from_default.  */
  bool volatile red_and_underline;

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
  fputs (set_foreground_color_default, stdout);
  fflush (stdout);
}

static _GL_ASYNC_SAFE void
async_restore (struct term_style_user_data *user_data)
{
  /* No <stdio.h> calls here!  */
  full_write (STDOUT_FILENO, set_underline_off,
              strlen (set_underline_off));
  full_write (STDOUT_FILENO, set_foreground_color_default,
              strlen (set_foreground_color_default));
}

static _GL_ASYNC_SAFE void
async_set_attributes_from_default (struct term_style_user_data *user_data)
{
  /* No <stdio.h> calls here!  */
  if (user_data->red_and_underline)
    {
      full_write (STDOUT_FILENO, set_underline_on,
                  strlen (set_underline_on));
      full_write (STDOUT_FILENO, set_foreground_color_red,
                  strlen (set_foreground_color_red));
    }
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
  user_data.red_and_underline = false;

  activate_term_style_controller (&controller, &user_data, STDOUT_FILENO,
                                  TTYCTL_AUTO);

  for (;;)
    {
      /* Before any styling, enable the non-default mode.  */
      activate_term_non_default_mode (&controller, &user_data);

      /* Set user_data.red_and_underline *before* emitting the appropriate
         escape sequences, otherwise async_set_attributes_from_default will not
         do its job correctly.  */
      user_data.red_and_underline = true;
      fputs (set_underline_on, stdout);
      fputs (set_foreground_color_red, stdout);
      fflush (stdout);

      fputs ("y", stdout);
      fflush (stdout);

      /* Revert to the default style before emitting a newline.  */
      user_data.red_and_underline = false;
      fputs (set_underline_off, stdout);
      fputs (set_foreground_color_default, stdout);
      fflush (stdout);

      /* Optional.  */
      deactivate_term_non_default_mode (&controller, &user_data);

      fputs ("\n", stdout);
      fflush (stdout);
    }
}
