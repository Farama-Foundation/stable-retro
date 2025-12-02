# strfmon_l.m4 serial 3
dnl Copyright (C) 2017-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRFMON_L],
[
  AC_REQUIRE([gl_MONETARY_H_DEFAULTS])
  AC_REQUIRE([gt_LOCALE_FR_UTF8])

  dnl Persuade glibc <monetary.h> to declare strfmon_l().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  dnl On Mac OS X 10.12, <monetary.h> may declare strfmon_l() if
  dnl _USE_EXTENDED_LOCALES_ is defined. But this symbol is supposed
  dnl to be defined by <xlocale.h>, not by us.

  AC_CHECK_FUNCS_ONCE([strfmon_l])
  if test $ac_cv_func_strfmon_l = yes; then
    dnl Test for bug <https://sourceware.org/bugzilla/show_bug.cgi?id=19633>
    dnl which was fixed in glibc-2.24.
    AC_CACHE_CHECK([whether strfmon_l works],
      [gl_cv_strfmon_l_works],
      [
       dnl Initial guess, used when cross-compiling or when no suitable locale
       dnl is present.
       case "$host_os" in
         # Guess no on glibc versions < 2.24.
         *-gnu* | gnu*)
           AC_EGREP_CPP([Unlucky],
             [
#include <features.h>
#ifdef __GNU_LIBRARY__
 #if (__GLIBC__ == 2 && __GLIBC_MINOR__ < 24)
  Unlucky GNU user
 #endif
#endif
             ],
             [gl_cv_strfmon_l_works="guessing no"],
             [gl_cv_strfmon_l_works="guessing yes"])
           ;;
         # Guess no on FreeBSD and Cygwin.
         freebsd* | midnightbsd* | cygwin*) gl_cv_strfmon_l_works="guessing no" ;;
         # Guess yes otherwise.
         *) gl_cv_strfmon_l_works="guessing yes" ;;
       esac
       if test $LOCALE_FR_UTF8 != none; then
         AC_CHECK_HEADERS_ONCE([xlocale.h])
         AC_RUN_IFELSE(
           [AC_LANG_SOURCE([[
#include <monetary.h>
#include <locale.h>
#if HAVE_XLOCALE_H
# include <xlocale.h>
#endif
#include <string.h>
int main ()
{
  /* On older glibc systems:      expected_buf="$123.50" buf="$123,50"
     On FreeBSD 12.0, Cygwin 2.9: expected_buf="$123.50" buf="123,50 $"
   */
  if (setlocale (LC_ALL, "en_US.UTF-8") != NULL)
    {
      char expected_buf[80];
      if (strfmon (expected_buf, sizeof (expected_buf), "%.2n", 123.5) >= 0)
        if (setlocale (LC_ALL, "$LOCALE_FR_UTF8") != NULL)
          {
            locale_t loc = newlocale (LC_ALL_MASK, "en_US.UTF-8", NULL);
            if (loc != (locale_t) 0)
              {
                char buf[80];
                if (strfmon_l (buf, sizeof (buf), loc, "%.2n", 123.5) >= 0)
                  return strcmp (buf, expected_buf) != 0;
              }
          }
    }
  return 0;
}]])],
           [gl_cv_strfmon_l_works=yes],
           [gl_cv_strfmon_l_works=no],
           [:])
       fi
      ])
    if test "$gl_cv_strfmon_l_works" = no; then
      REPLACE_STRFMON_L=1
    fi
  else
    HAVE_STRFMON_L=0
  fi
])

# Prerequisites of lib/strfmon_l.c.
AC_DEFUN([gl_PREREQ_STRFMON_L], [
  :
])
