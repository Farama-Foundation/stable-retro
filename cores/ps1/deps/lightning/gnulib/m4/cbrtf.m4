# cbrtf.m4 serial 4
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_CBRTF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_CBRT])

  dnl Persuade glibc <math.h> to declare cbrtf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether cbrtf() exists. Assume that cbrtf(), if it exists, is
  dnl defined in the same library as cbrt().
  save_LIBS="$LIBS"
  LIBS="$LIBS $CBRT_LIBM"
  AC_CHECK_FUNCS([cbrtf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_cbrtf = yes; then
    CBRTF_LIBM="$CBRT_LIBM"
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has cbrtf() in libm but doesn't declare it in <math.h>.
    AC_CHECK_DECL([cbrtf], , [HAVE_DECL_CBRTF=0], [[#include <math.h>]])
    save_LIBS="$LIBS"
    LIBS="$LIBS $CBRTF_LIBM"
    gl_FUNC_CBRTF_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_cbrtf_works" in
      *yes) ;;
      *) REPLACE_CBRTF=1 ;;
    esac
  else
    HAVE_CBRTF=0
    HAVE_DECL_CBRTF=0
  fi
  if test $HAVE_CBRTF = 0 || test $REPLACE_CBRTF = 1; then
    dnl Find libraries needed to link lib/cbrtf.c.
    AC_REQUIRE([gl_FUNC_FABSF])
    AC_REQUIRE([gl_FUNC_FREXPF])
    AC_REQUIRE([gl_FUNC_LDEXPF])
    CBRTF_LIBM=
    dnl Append $FABSF_LIBM to CBRTF_LIBM, avoiding gratuitous duplicates.
    case " $CBRTF_LIBM " in
      *" $FABSF_LIBM "*) ;;
      *) CBRTF_LIBM="$CBRTF_LIBM $FABSF_LIBM" ;;
    esac
    dnl Append $FREXPF_LIBM to CBRTF_LIBM, avoiding gratuitous duplicates.
    case " $CBRTF_LIBM " in
      *" $FREXPF_LIBM "*) ;;
      *) CBRTF_LIBM="$CBRTF_LIBM $FREXPF_LIBM" ;;
    esac
    dnl Append $LDEXPF_LIBM to CBRTF_LIBM, avoiding gratuitous duplicates.
    case " $CBRTF_LIBM " in
      *" $LDEXPF_LIBM "*) ;;
      *) CBRTF_LIBM="$CBRTF_LIBM $LDEXPF_LIBM" ;;
    esac
  fi
  AC_SUBST([CBRTF_LIBM])
])

dnl Test whether cbrtf() works.
dnl It returns wrong values on IRIX 6.5.
AC_DEFUN([gl_FUNC_CBRTF_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether cbrtf works], [gl_cv_func_cbrtf_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <math.h>
volatile float x;
volatile float y;
int main ()
{
  extern
  #ifdef __cplusplus
  "C"
  #endif
  float cbrtf (float);
  /* This test fails on IRIX 6.5.  */
  x = - 0.0f;
  y = cbrtf (x);
  if (!(y == 0.0f))
    return 1;
  return 0;
}
]])],
        [gl_cv_func_cbrtf_works=yes],
        [gl_cv_func_cbrtf_works=no],
        [case "$host_os" in
           irix*)  gl_cv_func_cbrtf_works="guessing no";;
                   # Guess yes on native Windows.
           mingw*) gl_cv_func_cbrtf_works="guessing yes";;
           *)      gl_cv_func_cbrtf_works="guessing yes";;
         esac
        ])
    ])
])
