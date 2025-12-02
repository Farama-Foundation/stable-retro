/* Test of duplicating a locale object.
   Copyright (C) 2009-2021 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <locale.h>

#if HAVE_WORKING_DUPLOCALE

#include "signature.h"
SIGNATURE_CHECK (duplocale, locale_t, (locale_t));

#include <langinfo.h>
#if HAVE_MONETARY_H
# include <monetary.h>
#endif
#include <stdio.h>
#include <string.h>

#include "macros.h"

struct locale_dependent_values
{
#if HAVE_MONETARY_H
  char monetary[100];
#endif
  char numeric[100];
  char time[100];
};

static void
get_locale_dependent_values (struct locale_dependent_values *result)
{
#if HAVE_MONETARY_H
  strfmon (result->monetary, sizeof (result->monetary),
           "%n", 123.75);
  /* result->monetary is usually "$123.75" */
#endif
  snprintf (result->numeric, sizeof (result->numeric),
            "%g", 3.5);
  /* result->numeric is usually "3,5" */
  strncpy (result->time, nl_langinfo (MON_1), sizeof result->time - 1);
  result->time[sizeof result->time - 1] = '\0';
  /* result->time is usually "janvier" */
}

#if HAVE_WORKING_USELOCALE

static int
test_with_uselocale (void)
{
  struct locale_dependent_values expected_results;
  locale_t mixed1;
  locale_t mixed2;
  locale_t perthread;

  /* Set up a locale which is a mix between different system locales.  */
  setlocale (LC_ALL, "en_US.UTF-8");
  setlocale (LC_NUMERIC, "de_DE.UTF-8");
  setlocale (LC_TIME, "fr_FR.UTF-8");
  get_locale_dependent_values (&expected_results);

  /* Save the locale in a locale_t object.  */
  mixed1 = duplocale (LC_GLOBAL_LOCALE);
  ASSERT (mixed1 != NULL);

  /* Use a per-thread locale.  */
  perthread = newlocale (LC_ALL_MASK, "es_ES.UTF-8", NULL);
  if (perthread == NULL)
    return 1;
  uselocale (perthread);

  /* Save the locale in a locale_t object again.  */
  mixed2 = duplocale (LC_GLOBAL_LOCALE);
  ASSERT (mixed2 != NULL);

  /* Set up a default locale.  */
  setlocale (LC_ALL, "C");
  uselocale (LC_GLOBAL_LOCALE);
  {
    struct locale_dependent_values c_results;
    get_locale_dependent_values (&c_results);
  }

  /* Now use the saved locale mixed1 again.  */
  setlocale (LC_ALL, "C");
  uselocale (LC_GLOBAL_LOCALE);
  uselocale (mixed1);
  {
    struct locale_dependent_values results;
    get_locale_dependent_values (&results);
# if HAVE_MONETARY_H
    ASSERT (strcmp (results.monetary, expected_results.monetary) == 0);
# endif
    ASSERT (strcmp (results.numeric, expected_results.numeric) == 0);
    ASSERT (strcmp (results.time, expected_results.time) == 0);
  }

  /* Now use the saved locale mixed2 again.  */
  setlocale (LC_ALL, "C");
  uselocale (LC_GLOBAL_LOCALE);
  uselocale (mixed2);
  {
    struct locale_dependent_values results;
    get_locale_dependent_values (&results);
# if HAVE_MONETARY_H
    ASSERT (strcmp (results.monetary, expected_results.monetary) == 0);
# endif
    ASSERT (strcmp (results.numeric, expected_results.numeric) == 0);
    ASSERT (strcmp (results.time, expected_results.time) == 0);
  }

  setlocale (LC_ALL, "C");
  uselocale (LC_GLOBAL_LOCALE);
  freelocale (mixed1);
  freelocale (mixed2);
  freelocale (perthread);
  return 0;
}

#endif

#if HAVE_STRFMON_L || HAVE_SNPRINTF_L || (HAVE_NL_LANGINFO_L && HAVE_WORKING_USELOCALE)

static void
get_locale_dependent_values_from (struct locale_dependent_values *result, locale_t locale)
{
#if HAVE_STRFMON_L
  strfmon_l (result->monetary, sizeof (result->monetary), locale,
             "%n", 123.75);
  /* result->monetary is usually "$123.75" */
#endif
#if HAVE_SNPRINTF_L
  snprintf_l (result->numeric, sizeof (result->numeric), locale,
              "%g", 3.5);
  /* result->numeric is usually "3,5" */
#endif
#if HAVE_NL_LANGINFO_L && HAVE_WORKING_USELOCALE
  strcpy (result->time, nl_langinfo_l (MON_1, locale));
  /* result->time is usually "janvier" */
#endif
}

static int
test_with_locale_parameter (void)
{
  struct locale_dependent_values expected_results;
  locale_t mixed1;
  locale_t mixed2;

  /* Set up a locale which is a mix between different system locales.  */
  setlocale (LC_ALL, "en_US.UTF-8");
  setlocale (LC_NUMERIC, "de_DE.UTF-8");
  setlocale (LC_TIME, "fr_FR.UTF-8");
  get_locale_dependent_values (&expected_results);

  /* Save the locale in a locale_t object.  */
  mixed1 = duplocale (LC_GLOBAL_LOCALE);
  ASSERT (mixed1 != NULL);

  /* Create another locale_t object.  */
  mixed2 = newlocale (LC_ALL_MASK, "es_ES.UTF-8", NULL);
  if (mixed2 == NULL)
    return 1;

  /* Set up a default locale.  */
  setlocale (LC_ALL, "C");
  {
    struct locale_dependent_values c_results;
    get_locale_dependent_values (&c_results);
  }

  /* Now use the saved locale mixed2.  */
  {
    struct locale_dependent_values results;
    get_locale_dependent_values_from (&results, mixed2);
  }

  /* Now use the saved locale mixed1 again.  */
  {
    struct locale_dependent_values results;
    get_locale_dependent_values_from (&results, mixed1);
#if HAVE_STRFMON_L
    ASSERT (strcmp (results.monetary, expected_results.monetary) == 0);
#endif
#if HAVE_SNPRINTF_L
    ASSERT (strcmp (results.numeric, expected_results.numeric) == 0);
#endif
#if HAVE_NL_LANGINFO_L && HAVE_WORKING_USELOCALE
    ASSERT (strcmp (results.time, expected_results.time) == 0);
#endif
  }

  freelocale (mixed1);
  freelocale (mixed2);
  return 0;
}

#endif

int
main ()
{
  int skipped = 0;
#if HAVE_WORKING_USELOCALE
  skipped |= test_with_uselocale ();
#endif
#if HAVE_STRFMON_L || HAVE_SNPRINTF_L || (HAVE_NL_LANGINFO_L && HAVE_WORKING_USELOCALE)
  skipped |= test_with_locale_parameter ();
#endif

  if (skipped)
    {
      fprintf (stderr, "Skipping test: Spanish Unicode locale is not installed\n");
      return 77;
    }

  return 0;
}

#else

#include <stdio.h>

int
main ()
{
  fprintf (stderr, "Skipping test: function duplocale not available\n");
  return 77;
}

#endif
