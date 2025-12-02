# logbf.m4 serial 3
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOGBF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare logbf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine LOGBF_LIBM.
  gl_MATHFUNC([logbf], [float], [(float)])
  if test $gl_cv_func_logbf_no_libm = yes \
     || test $gl_cv_func_logbf_in_libm = yes; then
    save_LIBS="$LIBS"
    LIBS="$LIBS $LOGBF_LIBM"
    gl_FUNC_LOGBF_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_logbf_works" in
      *yes) ;;
      *) REPLACE_LOGBF=1 ;;
    esac
  else
    HAVE_LOGBF=0
  fi
  if test $HAVE_LOGBF = 0 || test $REPLACE_LOGBF = 1; then
    dnl Find libraries needed to link lib/logbf.c.
    AC_REQUIRE([gl_FUNC_FREXPF])
    AC_REQUIRE([gl_FUNC_ISNANF])
    LOGBF_LIBM=
    dnl Append $FREXPF_LIBM to LOGBF_LIBM, avoiding gratuitous duplicates.
    case " $LOGBF_LIBM " in
      *" $FREXPF_LIBM "*) ;;
      *) LOGBF_LIBM="$LOGBF_LIBM $FREXPF_LIBM" ;;
    esac
    dnl Append $ISNANF_LIBM to LOGBF_LIBM, avoiding gratuitous duplicates.
    case " $LOGBF_LIBM " in
      *" $ISNANF_LIBM "*) ;;
      *) LOGBF_LIBM="$LOGBF_LIBM $ISNANF_LIBM" ;;
    esac
  fi
  AC_SUBST([LOGBF_LIBM])
])

dnl Test whether logbf() works.
dnl On glibc 2.11/ppc, glibc 2.7/sparc, glibc 2.7/hppa, Solaris 10/SPARC,
dnl Solaris 11.4/x86_64, the return value for subnormal (denormalized) arguments
dnl is too large.
AC_DEFUN([gl_FUNC_LOGBF_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether logbf works], [gl_cv_func_logbf_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
volatile float x;
int main ()
{
  int i;
  for (i = 1, x = 1.0f; i >= FLT_MIN_EXP; i--, x *= 0.5f)
    ;
  /* Here i = FLT_MIN_EXP - 1. Either x = 2^(i-1) is subnormal or x = 0.0.  */
  if (x > 0.0f && !(logbf (x) == (float)(i - 1)))
    return 1;
  return 0;
}
]])],
        [gl_cv_func_logbf_works=yes],
        [gl_cv_func_logbf_works=no],
        [case "$host_os" in
           *gnu* | solaris*) gl_cv_func_logbf_works="guessing no" ;;
           mingw*) # Guess yes on MSVC, no on mingw.
             AC_EGREP_CPP([Known], [
#ifdef _MSC_VER
 Known
#endif
               ],
               [gl_cv_func_logbf_works="guessing yes"],
               [gl_cv_func_logbf_works="guessing no"])
             ;;
           *) gl_cv_func_logbf_works="guessing yes" ;;
         esac
        ])
    ])
])
