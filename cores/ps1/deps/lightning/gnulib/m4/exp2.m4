# exp2.m4 serial 3
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXP2],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare exp2().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  EXP2_LIBM=
  AC_CACHE_CHECK([whether exp2() can be used without linking with libm],
    [gl_cv_func_exp2_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             extern
             #ifdef __cplusplus
             "C"
             #endif
             double exp2 (double);
             double (* volatile funcptr) (double) = exp2;
             double x;]],
           [[return funcptr (x) > 1.5
                    || exp2 (x) > 1.5;]])],
        [gl_cv_func_exp2_no_libm=yes],
        [gl_cv_func_exp2_no_libm=no])
    ])
  if test $gl_cv_func_exp2_no_libm = no; then
    AC_CACHE_CHECK([whether exp2() can be used with libm],
      [gl_cv_func_exp2_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               extern
               #ifdef __cplusplus
               "C"
               #endif
               double exp2 (double);
               double (* volatile funcptr) (double) = exp2;
               double x;]],
             [[return funcptr (x) > 1.5
                      || exp2 (x) > 1.5;]])],
          [gl_cv_func_exp2_in_libm=yes],
          [gl_cv_func_exp2_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_exp2_in_libm = yes; then
      EXP2_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_exp2_no_libm = yes \
     || test $gl_cv_func_exp2_in_libm = yes; then
    HAVE_EXP2=1
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has exp2() in libm but doesn't declare it in <math.h>.
    AC_CHECK_DECL([exp2], , [HAVE_DECL_EXP2=0], [[#include <math.h>]])
    save_LIBS="$LIBS"
    LIBS="$LIBS $EXP2_LIBM"
    gl_FUNC_EXP2_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_exp2_works" in
      *yes) ;;
      *) REPLACE_EXP2=1 ;;
    esac
  else
    HAVE_EXP2=0
    HAVE_DECL_EXP2=0
  fi
  if test $HAVE_EXP2 = 0 || test $REPLACE_EXP2 = 1; then
    dnl Find libraries needed to link lib/exp2.c.
    AC_REQUIRE([gl_FUNC_ISNAND])
    AC_REQUIRE([gl_FUNC_ROUND])
    AC_REQUIRE([gl_FUNC_LDEXP])
    EXP2_LIBM=
    dnl Append $ISNAND_LIBM to EXP2_LIBM, avoiding gratuitous duplicates.
    case " $EXP2_LIBM " in
      *" $ISNAND_LIBM "*) ;;
      *) EXP2_LIBM="$EXP2_LIBM $ISNAND_LIBM" ;;
    esac
    dnl Append $ROUND_LIBM to EXP2_LIBM, avoiding gratuitous duplicates.
    case " $EXP2_LIBM " in
      *" $ROUND_LIBM "*) ;;
      *) EXP2_LIBM="$EXP2_LIBM $ROUND_LIBM" ;;
    esac
    dnl Append $LDEXP_LIBM to EXP2_LIBM, avoiding gratuitous duplicates.
    case " $EXP2_LIBM " in
      *" $LDEXP_LIBM "*) ;;
      *) EXP2_LIBM="$EXP2_LIBM $LDEXP_LIBM" ;;
    esac
  fi
  AC_SUBST([EXP2_LIBM])
])

dnl Test whether exp2() works.
dnl On OpenBSD 4.9, for the argument 0.6, it returns 1.517358639986284397,
dnl which has a relative error of 0.1%.
AC_DEFUN([gl_FUNC_EXP2_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether exp2 works], [gl_cv_func_exp2_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <math.h>
extern
#ifdef __cplusplus
"C"
#endif
double exp2 (double);
volatile double x;
double y;
int main ()
{
  x = 0.6;
  y = exp2 (x);
  if (y > 1.516)
    return 1;
  return 0;
}
]])],
        [gl_cv_func_exp2_works=yes],
        [gl_cv_func_exp2_works=no],
        [case "$host_os" in
           openbsd*) gl_cv_func_exp2_works="guessing no" ;;
                     # Guess yes on native Windows.
           mingw*)   gl_cv_func_exp2_works="guessing yes" ;;
           *)        gl_cv_func_exp2_works="guessing yes" ;;
         esac
        ])
    ])
])
