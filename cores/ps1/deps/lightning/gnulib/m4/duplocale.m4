# duplocale.m4 serial 12
dnl Copyright (C) 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_DUPLOCALE],
[
  AC_REQUIRE([gl_LOCALE_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([gl_FUNC_SETLOCALE_NULL])
  AC_CHECK_FUNCS_ONCE([duplocale])
  if test $ac_cv_func_duplocale = yes; then
    dnl Check against glibc bug where duplocale crashes.
    dnl See <https://sourceware.org/bugzilla/show_bug.cgi?id=10969>.
    dnl Also, on AIX 7.1, duplocale(LC_GLOBAL_LOCALE) returns (locale_t)0 with
    dnl errno set to EINVAL.
    dnl Also, on NetBSD 7.0, duplocale(LC_GLOBAL_LOCALE) returns a locale that
    dnl corresponds to the C locale.
    AC_REQUIRE([gl_LOCALE_H])
    if test $HAVE_LOCALE_T = 1; then
      AC_CHECK_FUNCS_ONCE([snprintf_l nl_langinfo_l])
      AC_CACHE_CHECK([whether duplocale(LC_GLOBAL_LOCALE) works],
        [gl_cv_func_duplocale_works],
        [AC_RUN_IFELSE(
           [AC_LANG_SOURCE([[
#include <locale.h>
#if HAVE_XLOCALE_H
# include <xlocale.h>
#endif
#if HAVE_SNPRINTF_L
# include <stdio.h>
#endif
#if HAVE_NL_LANGINFO_L
# include <langinfo.h>
#endif
#include <string.h>
struct locale_dependent_values
{
  char numeric[100];
  char time[100];
};
int main ()
{
  struct locale_dependent_values expected_result;
  struct locale_dependent_values result;
  locale_t loc;
  setlocale (LC_ALL, "en_US.UTF-8");
  setlocale (LC_NUMERIC, "de_DE.UTF-8");
  setlocale (LC_TIME, "fr_FR.UTF-8");
#if HAVE_SNPRINTF_L
  snprintf (expected_result.numeric, sizeof (expected_result.numeric), "%g", 3.5);
#endif
#if HAVE_NL_LANGINFO_L
  strcpy (expected_result.time, nl_langinfo (MON_1));
#endif
  loc = duplocale (LC_GLOBAL_LOCALE);
  if (!loc)
    return 1;
#if HAVE_SNPRINTF_L
  snprintf_l (result.numeric, sizeof (result.numeric), loc, "%g", 3.5);
#endif
#if HAVE_NL_LANGINFO_L
  strcpy (result.time, nl_langinfo_l (MON_1, loc));
#endif
#if HAVE_SNPRINTF_L
  if (strcmp (result.numeric, expected_result.numeric) != 0)
    return 2;
#endif
#if HAVE_NL_LANGINFO_L
  if (strcmp (result.time, expected_result.time) != 0)
    return 3;
#endif
  freelocale (loc);
  return 0;
}]])],
           [gl_cv_func_duplocale_works=yes],
           [gl_cv_func_duplocale_works=no],
           [dnl Guess it works except on glibc < 2.12, uClibc, AIX, and NetBSD.
            case "$host_os" in
              aix* | netbsd*) gl_cv_func_duplocale_works="guessing no";;
              *-gnu* | gnu*)
                AC_EGREP_CPP([Unlucky], [
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ < 12)
  Unlucky GNU user
 #endif
#endif
#ifdef __UCLIBC__
 Unlucky user
#endif
                  ],
                  [gl_cv_func_duplocale_works="guessing no"],
                  [gl_cv_func_duplocale_works="guessing yes"])
                ;;
              *) gl_cv_func_duplocale_works="guessing yes";;
            esac
           ])
        ])
      case "$gl_cv_func_duplocale_works" in
        *no) REPLACE_DUPLOCALE=1 ;;
      esac
    else
      dnl In 2019, some versions of z/OS lack the locale_t type and have broken
      dnl newlocale, duplocale, freelocale functions. In this situation, we
      dnl cannot use nor override duplocale.
      HAVE_DUPLOCALE=0
    fi
  else
    HAVE_DUPLOCALE=0
  fi
  if test $REPLACE_DUPLOCALE = 1; then
    LIB_DUPLOCALE="$LIB_SETLOCALE_NULL"
  else
    LIB_DUPLOCALE=
  fi
  dnl LIB_DUPLOCALE is expected to be '-pthread' or '-lpthread' on AIX
  dnl with gcc or xlc, and empty otherwise.
  AC_SUBST([LIB_DUPLOCALE])
])

# Prerequisites of lib/duplocale.c.
AC_DEFUN([gl_PREREQ_DUPLOCALE],
[
  :
])
