# mbsnrtowcs.m4 serial 6
dnl Copyright (C) 2008, 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MBSNRTOWCS],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  dnl Persuade glibc <wchar.h> to declare mbsnrtowcs().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  gl_MBSTATE_T_BROKEN

  AC_CHECK_FUNCS_ONCE([mbsnrtowcs])
  if test $ac_cv_func_mbsnrtowcs = no; then
    HAVE_MBSNRTOWCS=0
  else
    if test $REPLACE_MBSTATE_T = 1; then
      REPLACE_MBSNRTOWCS=1
    else
      gl_MBSNRTOWCS_WORKS_IN_TRADITIONAL_LOCALE
      case "$gl_cv_func_mbsnrtowcs_works_in_traditional_locale" in
        *yes) ;;
        *) REPLACE_MBSNRTOWCS=1 ;;
      esac
    fi
  fi
])

# Prerequisites of lib/mbsnrtowcs.c.
AC_DEFUN([gl_PREREQ_MBSNRTOWCS], [
  :
])

dnl Test whether mbsnrtowcs works in an ISO-8859-1 locale.
dnl Result is gl_cv_func_mbsnrtowcs_works_in_traditional_locale.

AC_DEFUN([gl_MBSNRTOWCS_WORKS_IN_TRADITIONAL_LOCALE],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gt_LOCALE_FR])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether mbsnrtowcs works in a traditional locale],
    [gl_cv_func_mbsnrtowcs_works_in_traditional_locale],
    [
      dnl Initial guess, used when cross-compiling or when no suitable locale
      dnl is present.
changequote(,)dnl
      case "$host_os" in
                  # Guess no on Solaris.
        solaris*) gl_cv_func_mbsnrtowcs_works_in_traditional_locale="guessing no" ;;
                  # Guess yes otherwise.
        *)        gl_cv_func_mbsnrtowcs_works_in_traditional_locale="guessing yes" ;;
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
  /* This fails on Solaris 11.4:
     mbsnrtowcs stores a wrong wide character: 0xDF instead of btowc (0xDF).  */
  if (setlocale (LC_ALL, "$LOCALE_FR") != NULL)
    {
      /* Locale encoding is ISO-8859-1 or ISO-8859-15.  */
      char input[] = "B\374\337er"; /* "Büßer" */
      mbstate_t state;
      mbstate_t temp_state;
      wchar_t wc;
      size_t ret;
      const char *src;

      #define BUFSIZE 10
      wchar_t buf[BUFSIZE];
      {
        size_t i;
        for (i = 0; i < BUFSIZE; i++)
          buf[i] = (wchar_t) 0xBADFACE;
      }

      memset (&state, '\0', sizeof (mbstate_t));

      wc = (wchar_t) 0xBADFACE;
      ret = mbrtowc (&wc, input, 1, &state);
      if (!(ret == 1 && wc == 'B' && mbsinit (&state)))
        return 1;
      input[0] = '\0';

      wc = (wchar_t) 0xBADFACE;
      ret = mbrtowc (&wc, input + 1, 1, &state);
      if (!(ret == 1 && wctob (wc) == (unsigned char) '\374' && mbsinit (&state)))
        return 2;
      input[1] = '\0';

      src = input + 2;
      temp_state = state;
      ret = mbsnrtowcs (NULL, &src, 4, 1, &temp_state);
      if (!(ret == 3 && src == input + 2 && mbsinit (&state)))
        return 3;

      src = input + 2;
      ret = mbsnrtowcs (buf, &src, 4, 1, &state);
      if (!(ret == 1))
        return 4;
      if (!(src == input + 3))
        return 5;
      if (!(buf[0] == btowc ((unsigned char) '\337')))
        return 6;
    }
  return 0;
}]])],
          [gl_cv_func_mbsnrtowcs_works_in_traditional_locale=yes],
          [gl_cv_func_mbsnrtowcs_works_in_traditional_locale=no],
          [:])
      fi
    ])
])
