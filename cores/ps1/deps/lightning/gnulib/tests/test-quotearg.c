/* Test of quotearg family of functions.
   Copyright (C) 2008-2021 Free Software Foundation, Inc.

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

/* Written by Eric Blake <ebb9@byu.net>, 2008.  */

#include <config.h>

#include "quotearg.h"

#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gettext.h"
#include "macros.h"

#if ENABLE_NLS

# include "test-quotearg.h"

static struct result_groups locale_results[] = {
  /* locale_quoting_style */
  { { LQ RQ, LQ "\\0001\\0" RQ, 11, LQ "simple" RQ,
      LQ " \\t\\n'\"\\033?""?/\\\\" RQ, LQ "a:b" RQ, LQ "a\\\\b" RQ,
      LQ "a' b" RQ, LQ LQ RQ_ESC RQ, LQ LQ RQ_ESC RQ },
    { LQ RQ, LQ "\\0001\\0" RQ, 11, LQ "simple" RQ,
      LQ " \\t\\n'\"\\033?""?/\\\\" RQ, LQ "a:b" RQ, LQ "a\\\\b" RQ,
      LQ "a' b" RQ, LQ LQ RQ_ESC RQ, LQ LQ RQ_ESC RQ},
    { LQ RQ, LQ "\\0001\\0" RQ, 11, LQ "simple" RQ,
      LQ " \\t\\n'\"\\033?""?/\\\\" RQ, LQ "a\\:b" RQ, LQ "a\\\\b" RQ,
      LQ "a' b" RQ, LQ LQ RQ_ESC RQ, LQ LQ RQ_ESC RQ } },

  /* clocale_quoting_style */
  { { LQ RQ, LQ "\\0001\\0" RQ, 11, LQ "simple" RQ,
      LQ " \\t\\n'\"\\033?""?/\\\\" RQ, LQ "a:b" RQ, LQ "a\\\\b" RQ,
      LQ "a' b" RQ, LQ LQ RQ_ESC RQ, LQ LQ RQ_ESC RQ },
    { LQ RQ, LQ "\\0001\\0" RQ, 11, LQ "simple" RQ,
      LQ " \\t\\n'\"\\033?""?/\\\\" RQ, LQ "a:b" RQ, LQ "a\\\\b" RQ,
      LQ "a' b" RQ, LQ LQ RQ_ESC RQ, LQ LQ RQ_ESC RQ },
    { LQ RQ, LQ "\\0001\\0" RQ, 11, LQ "simple" RQ,
      LQ " \\t\\n'\"\\033?""?/\\\\" RQ, LQ "a\\:b" RQ, LQ "a\\\\b" RQ,
      LQ "a' b" RQ, LQ LQ RQ_ESC RQ, LQ LQ RQ_ESC RQ } }
};

#endif /* ENABLE_NLS */

int
main (int argc _GL_UNUSED, char *argv[])
{
#if ENABLE_NLS
  /* Clean up environment.  */
  unsetenv ("LANGUAGE");
  unsetenv ("LC_ALL");
  unsetenv ("LC_MESSAGES");
  unsetenv ("LC_CTYPE");
  unsetenv ("LANG");
  unsetenv ("OUTPUT_CHARSET");

  /* This program part runs in a French UTF-8 locale.  It uses
     the test-quotearg.mo message catalog.  */
  {
    const char *locale_name = getenv ("LOCALE");

    if (locale_name != NULL && strcmp (locale_name, "none") != 0
        && setenv ("LC_ALL", locale_name, 1) == 0
        && setlocale (LC_ALL, "") != NULL)
      {
        textdomain ("test-quotearg");
        bindtextdomain ("test-quotearg", getenv ("LOCALEDIR"));

        set_quoting_style (NULL, locale_quoting_style);
        compare_strings (use_quotearg_buffer, &locale_results[0].group1, false);
        compare_strings (use_quotearg, &locale_results[0].group2, false);
        compare_strings (use_quotearg_colon, &locale_results[0].group3, false);

        set_quoting_style (NULL, clocale_quoting_style);
        compare_strings (use_quotearg_buffer, &locale_results[1].group1, false);
        compare_strings (use_quotearg, &locale_results[1].group2, false);
        compare_strings (use_quotearg_colon, &locale_results[1].group3, false);

        quotearg_free ();
        return 0;
      }
  }

  fputs ("Skipping test: no french Unicode locale is installed\n", stderr);
  return 77;
#else
  fputs ("Skipping test: internationalization is disabled\n", stderr);
  return 77;
#endif /* ENABLE_NLS */
}
