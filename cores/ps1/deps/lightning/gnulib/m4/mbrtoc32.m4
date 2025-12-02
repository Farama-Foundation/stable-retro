# mbrtoc32.m4 serial 7
dnl Copyright (C) 2014-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MBRTOC32],
[
  AC_REQUIRE([gl_UCHAR_H_DEFAULTS])

  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  gl_MBSTATE_T_BROKEN

  AC_REQUIRE([gl_TYPE_CHAR32_T])
  AC_REQUIRE([gl_MBRTOC32_SANITYCHECK])

  AC_REQUIRE([gl_CHECK_FUNC_MBRTOC32])
  if test $gl_cv_func_mbrtoc32 = no; then
    HAVE_MBRTOC32=0
  else
    if test $GNULIB_OVERRIDES_CHAR32_T = 1 || test $REPLACE_MBSTATE_T = 1; then
      REPLACE_MBRTOC32=1
    else
      gl_MBRTOC32_EMPTY_INPUT
      gl_MBRTOC32_C_LOCALE
      case "$gl_cv_func_mbrtoc32_empty_input" in
        *yes) ;;
        *) AC_DEFINE([MBRTOC32_EMPTY_INPUT_BUG], [1],
             [Define if the mbrtoc32 function does not return (size_t) -2 for empty input.])
           REPLACE_MBRTOC32=1
           ;;
      esac
      case "$gl_cv_func_mbrtoc32_C_locale_sans_EILSEQ" in
        *yes) ;;
        *) AC_DEFINE([MBRTOC32_IN_C_LOCALE_MAYBE_EILSEQ], [1],
             [Define if the mbrtoc32 function may signal encoding errors in the C locale.])
           REPLACE_MBRTOC32=1
           ;;
      esac
    fi
    if test $HAVE_WORKING_MBRTOC32 = 0; then
      REPLACE_MBRTOC32=1
    fi
  fi
])

dnl We can't use AC_CHECK_FUNC here, because mbrtoc32() is defined as a
dnl static inline function on Haiku 2020.
AC_DEFUN([gl_CHECK_FUNC_MBRTOC32],
[
  AC_CACHE_CHECK([for mbrtoc32], [gl_cv_func_mbrtoc32],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <stdlib.h>
            #include <uchar.h>
          ]],
          [[char32_t c;
            return mbrtoc32 (&c, "", 1, NULL) == 0;
          ]])
       ],
       [gl_cv_func_mbrtoc32=yes],
       [gl_cv_func_mbrtoc32=no])
    ])
])

AC_DEFUN([gl_MBRTOC32_EMPTY_INPUT],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether mbrtoc32 works on empty input],
    [gl_cv_func_mbrtoc32_empty_input],
    [
      dnl Initial guess, used when cross-compiling or when no suitable locale
      dnl is present.
changequote(,)dnl
      case "$host_os" in
                       # Guess no on glibc systems.
        *-gnu* | gnu*) gl_cv_func_mbrtoc32_empty_input="guessing no" ;;
        *)             gl_cv_func_mbrtoc32_empty_input="guessing yes" ;;
      esac
changequote([,])dnl
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
           #include <uchar.h>
           static char32_t wc;
           static mbstate_t mbs;
           int
           main (void)
           {
             return mbrtoc32 (&wc, "", 0, &mbs) != (size_t) -2;
           }]])],
        [gl_cv_func_mbrtoc32_empty_input=yes],
        [gl_cv_func_mbrtoc32_empty_input=no],
        [:])
    ])
])

AC_DEFUN([gl_MBRTOC32_C_LOCALE],
[
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether the C locale is free of encoding errors],
    [gl_cv_func_mbrtoc32_C_locale_sans_EILSEQ],
    [
     dnl Initial guess, used when cross-compiling or when no suitable locale
     dnl is present.
     gl_cv_func_mbrtoc32_C_locale_sans_EILSEQ="$gl_cross_guess_normal"

     AC_RUN_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <limits.h>
            #include <locale.h>
            #include <uchar.h>
          ]], [[
            int i;
            char *locale = setlocale (LC_ALL, "C");
            if (! locale)
              return 2;
            for (i = CHAR_MIN; i <= CHAR_MAX; i++)
              {
                char c = i;
                char32_t wc;
                mbstate_t mbs = { 0, };
                size_t ss = mbrtoc32 (&wc, &c, 1, &mbs);
                if (1 < ss)
                  return 3;
              }
            return 0;
          ]])],
      [gl_cv_func_mbrtoc32_C_locale_sans_EILSEQ=yes],
      [gl_cv_func_mbrtoc32_C_locale_sans_EILSEQ=no],
      [case "$host_os" in
                 # Guess yes on native Windows.
         mingw*) gl_cv_func_mbrtoc32_C_locale_sans_EILSEQ="guessing yes" ;;
       esac
      ])
    ])
])

dnl Test whether mbrtoc32 works not worse than mbrtowc.
dnl Result is HAVE_WORKING_MBRTOC32.

AC_DEFUN([gl_MBRTOC32_SANITYCHECK],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gl_TYPE_CHAR32_T])
  AC_REQUIRE([gl_CHECK_FUNC_MBRTOC32])
  AC_REQUIRE([gt_LOCALE_FR])
  AC_REQUIRE([gt_LOCALE_ZH_CN])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  if test $GNULIB_OVERRIDES_CHAR32_T = 1 || test $gl_cv_func_mbrtoc32 = no; then
    HAVE_WORKING_MBRTOC32=0
  else
    AC_CACHE_CHECK([whether mbrtoc32 works as well as mbrtowc],
      [gl_cv_func_mbrtoc32_sanitycheck],
      [
        dnl Initial guess, used when cross-compiling or when no suitable locale
        dnl is present.
changequote(,)dnl
        case "$host_os" in
          # Guess no on FreeBSD, Solaris, native Windows.
          freebsd* | midnightbsd* | solaris* | mingw*)
            gl_cv_func_mbrtoc32_sanitycheck="guessing no"
            ;;
          # Guess yes otherwise.
          *)
            gl_cv_func_mbrtoc32_sanitycheck="guessing yes"
            ;;
        esac
changequote([,])dnl
        if test $LOCALE_FR != none || test $LOCALE_ZH_CN != none; then
          AC_RUN_IFELSE(
            [AC_LANG_SOURCE([[
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <uchar.h>
int main ()
{
  int result = 0;
  /* This fails on native Windows:
     mbrtoc32 returns (size_t)-1.
     mbrtowc returns 1 (correct).  */
  if (setlocale (LC_ALL, "$LOCALE_FR") != NULL)
    {
      mbstate_t state;
      wchar_t wc = (wchar_t) 0xBADFACE;
      memset (&state, '\0', sizeof (mbstate_t));
      if (mbrtowc (&wc, "\374", 1, &state) == 1)
        {
          char32_t c32 = (wchar_t) 0xBADFACE;
          memset (&state, '\0', sizeof (mbstate_t));
          if (mbrtoc32 (&c32, "\374", 1, &state) != 1)
            result |= 1;
        }
    }
  /* This fails on FreeBSD 12 and Solaris 11.4:
     mbrtoc32 returns (size_t)-2 or (size_t)-1.
     mbrtowc returns 4 (correct).  */
  if (setlocale (LC_ALL, "$LOCALE_ZH_CN") != NULL)
    {
      mbstate_t state;
      wchar_t wc = (wchar_t) 0xBADFACE;
      memset (&state, '\0', sizeof (mbstate_t));
      if (mbrtowc (&wc, "\224\071\375\067", 4, &state) == 4)
        {
          char32_t c32 = (wchar_t) 0xBADFACE;
          memset (&state, '\0', sizeof (mbstate_t));
          if (mbrtoc32 (&c32, "\224\071\375\067", 4, &state) != 4)
            result |= 2;
        }
    }
  return result;
}]])],
            [gl_cv_func_mbrtoc32_sanitycheck=yes],
            [gl_cv_func_mbrtoc32_sanitycheck=no],
            [:])
        fi
      ])
    case "$gl_cv_func_mbrtoc32_sanitycheck" in
      *yes)
        HAVE_WORKING_MBRTOC32=1
        AC_DEFINE([HAVE_WORKING_MBRTOC32], [1],
          [Define if the mbrtoc32 function basically works.])
        ;;
      *) HAVE_WORKING_MBRTOC32=0 ;;
    esac
  fi
  AC_SUBST([HAVE_WORKING_MBRTOC32])
])

# Prerequisites of lib/mbrtoc32.c and lib/lc-charset-dispatch.c.
AC_DEFUN([gl_PREREQ_MBRTOC32], [
  :
])
