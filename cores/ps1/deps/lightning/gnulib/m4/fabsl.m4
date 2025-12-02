# fabsl.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FABSL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare fabsl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine FABSL_LIBM.
  gl_MATHFUNC([fabsl], [long double], [(long double)])
  if test $gl_cv_func_fabsl_no_libm = yes \
     || test $gl_cv_func_fabsl_in_libm = yes; then
    save_LIBS="$LIBS"
    LIBS="$LIBS $FABSL_LIBM"
    gl_FUNC_FABSL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_fabsl_works" in
      *yes) ;;
      *) REPLACE_FABSL=1 ;;
    esac
  else
    HAVE_FABSL=0
  fi
  if test $HAVE_FABSL = 0 || test $REPLACE_FABSL = 1; then
    dnl Find libraries needed to link lib/fabsl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_FABS])
      FABSL_LIBM="$FABS_LIBM"
    else
      FABSL_LIBM=
    fi
  fi
  AC_SUBST([FABSL_LIBM])
])

dnl Test whether fabsl() works on a minus zero.
dnl On IRIX 6.5, with gcc 4.2.4 and without -fno-builtin-fabsl, the value is a
dnl minus zero rather than a positive zero.
AC_DEFUN([gl_FUNC_FABSL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether fabsl works], [gl_cv_func_fabsl_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
#include <string.h>
volatile long double x;
long double y;
long double zero = 0.0L;
int main ()
{
  x = - (LDBL_MIN * LDBL_MIN * LDBL_MIN);
  y = fabsl (x);
  return (memcmp (&y, &zero, sizeof y) != 0);
}
]])],
        [gl_cv_func_fabsl_works=yes],
        [gl_cv_func_fabsl_works=no],
        [case "$host_os" in
           irix*)  gl_cv_func_fabsl_works="guessing no" ;;
                   # Guess yes on native Windows.
           mingw*) gl_cv_func_fabsl_works="guessing yes" ;;
           *)      gl_cv_func_fabsl_works="guessing yes" ;;
         esac
        ])
    ])
])
