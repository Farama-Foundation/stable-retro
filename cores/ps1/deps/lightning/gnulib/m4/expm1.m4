# expm1.m4 serial 9
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXPM1],
[
  m4_divert_text([DEFAULTS], [gl_expm1_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])

  dnl Persuade glibc <math.h> to declare expm1().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  EXPM1_LIBM=
  AC_CACHE_CHECK([whether expm1() can be used without linking with libm],
    [gl_cv_func_expm1_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             double (* volatile funcptr) (double) = expm1;
             double x;]],
           [[return funcptr (x) > 0.5
                    || expm1 (x) > 0.5;]])],
        [gl_cv_func_expm1_no_libm=yes],
        [gl_cv_func_expm1_no_libm=no])
    ])
  if test $gl_cv_func_expm1_no_libm = no; then
    AC_CACHE_CHECK([whether expm1() can be used with libm],
      [gl_cv_func_expm1_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               double (* volatile funcptr) (double) = expm1;
               double x;]],
             [[return funcptr (x) > 0.5
                      || expm1 (x) > 0.5;]])],
          [gl_cv_func_expm1_in_libm=yes],
          [gl_cv_func_expm1_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_expm1_in_libm = yes; then
      EXPM1_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_expm1_no_libm = yes \
     || test $gl_cv_func_expm1_in_libm = yes; then
    :
    m4_ifdef([gl_FUNC_EXPM1_IEEE], [
      if test $gl_expm1_required = ieee && test $REPLACE_EXPM1 = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether expm1 works according to ISO C 99 with IEC 60559],
          [gl_cv_func_expm1_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $EXPM1_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_DOUBLE_MINUS_ZERO_CODE[
]gl_DOUBLE_SIGNBIT_CODE[
static double dummy (double x) { return 0; }
int main (int argc, char *argv[])
{
  double (* volatile my_expm1) (double) = argc ? expm1 : dummy;
  double y = my_expm1 (minus_zerod);
  if (!(y == 0.0) || (signbitd (minus_zerod) && !signbitd (y)))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_expm1_ieee=yes],
              [gl_cv_func_expm1_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_expm1_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_expm1_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_expm1_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_expm1_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_expm1_ieee" in
          *yes) ;;
          *) REPLACE_EXPM1=1 ;;
        esac
      fi
    ])
  else
    HAVE_EXPM1=0
  fi
  if test $HAVE_EXPM1 = 0 || test $REPLACE_EXPM1 = 1; then
    dnl Find libraries needed to link lib/expm1.c.
    AC_REQUIRE([gl_FUNC_ISNAND])
    AC_REQUIRE([gl_FUNC_EXP])
    AC_REQUIRE([gl_FUNC_ROUND])
    AC_REQUIRE([gl_FUNC_LDEXP])
    EXPM1_LIBM=
    dnl Append $ISNAND_LIBM to EXPM1_LIBM, avoiding gratuitous duplicates.
    case " $EXPM1_LIBM " in
      *" $ISNAND_LIBM "*) ;;
      *) EXPM1_LIBM="$EXPM1_LIBM $ISNAND_LIBM" ;;
    esac
    dnl Append $EXP_LIBM to EXPM1_LIBM, avoiding gratuitous duplicates.
    case " $EXPM1_LIBM " in
      *" $EXP_LIBM "*) ;;
      *) EXPM1_LIBM="$EXPM1_LIBM $EXP_LIBM" ;;
    esac
    dnl Append $ROUND_LIBM to EXPM1_LIBM, avoiding gratuitous duplicates.
    case " $EXPM1_LIBM " in
      *" $ROUND_LIBM "*) ;;
      *) EXPM1_LIBM="$EXPM1_LIBM $ROUND_LIBM" ;;
    esac
    dnl Append $LDEXP_LIBM to EXPM1_LIBM, avoiding gratuitous duplicates.
    case " $EXPM1_LIBM " in
      *" $LDEXP_LIBM "*) ;;
      *) EXPM1_LIBM="$EXPM1_LIBM $LDEXP_LIBM" ;;
    esac
  fi
  AC_SUBST([EXPM1_LIBM])
])
