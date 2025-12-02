# wcsnrtombs.m4 serial 7
dnl Copyright (C) 2008-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCSNRTOMBS],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  gl_MBSTATE_T_BROKEN

  AC_CHECK_FUNCS_ONCE([wcsnrtombs])
  if test $ac_cv_func_wcsnrtombs = no; then
    HAVE_WCSNRTOMBS=0
  else
    if test $REPLACE_MBSTATE_T = 1; then
      REPLACE_WCSNRTOMBS=1
    else
      gl_WCSNRTOMBS_WORKS_IN_TRADITIONAL_LOCALE
      case "$gl_cv_func_wcsnrtombs_works_in_traditional_locale" in
        *yes) ;;
        *) REPLACE_WCSNRTOMBS=1 ;;
      esac

    fi
  fi
])

# Prerequisites of lib/wcsnrtombs.c.
AC_DEFUN([gl_PREREQ_WCSNRTOMBS], [
  :
])

dnl Test whether wcsnrtombs works in an ISO-8859-1 locale.
dnl Result is gl_cv_func_wcsnrtombs_works_in_traditional_locale.

AC_DEFUN([gl_WCSNRTOMBS_WORKS_IN_TRADITIONAL_LOCALE],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gt_LOCALE_FR])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether wcsnrtombs works in a traditional locale],
    [gl_cv_func_wcsnrtombs_works_in_traditional_locale],
    [
      dnl Initial guess, used when cross-compiling or when no suitable locale
      dnl is present.
changequote(,)dnl
      case "$host_os" in
                  # Guess no on Solaris.
        solaris*) gl_cv_func_wcsnrtombs_works_in_traditional_locale="guessing no" ;;
                  # Guess yes otherwise.
        *)        gl_cv_func_wcsnrtombs_works_in_traditional_locale="guessing yes" ;;
      esac
changequote([,])dnl
      if test $LOCALE_FR != none; then
        AC_RUN_IFELSE(
          [AC_LANG_SOURCE([[
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
int main ()
{
  /* This fails on Solaris 11.4.  */
  if (setlocale (LC_ALL, "$LOCALE_FR") != NULL)
    {
      /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
      const char original[] = "B\374\337er"; /* "Büßer" */
      wchar_t input[10];
      size_t ret;
      const wchar_t *src;

      #define BUFSIZE 20
      char buf[BUFSIZE];
      memset (buf, '_', BUFSIZE);

      ret = mbstowcs (input, original, 10);
      if (!(ret == 5))
        return 1;

      src = input;
      ret = wcsnrtombs (buf, &src, 6, 2, NULL);
      if (!(ret == 2))
        return 2;
      if (!(src == input + 2))
        return 3;
      if (!(memcmp (buf, original, ret) == 0))
        return 4;
    }
  return 0;
}]])],
          [gl_cv_func_wcsnrtombs_works_in_traditional_locale=yes],
          [gl_cv_func_wcsnrtombs_works_in_traditional_locale=no],
          [:])
      fi
    ])
])
