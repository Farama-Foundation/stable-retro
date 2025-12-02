# log1pl.m4 serial 9
dnl Copyright (C) 2012-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOG1PL],
[
  m4_divert_text([DEFAULTS], [gl_log1pl_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_LOG1P])

  dnl Persuade glibc <math.h> to declare log1pl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether log1pl() exists. Assume that log1pl(), if it exists, is
  dnl defined in the same library as log1p().
  save_LIBS="$LIBS"
  LIBS="$LIBS $LOG1P_LIBM"
  AC_CHECK_FUNCS([log1pl])
  LIBS="$save_LIBS"
  if test $ac_cv_func_log1pl = yes; then
    LOG1PL_LIBM="$LOG1P_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $LOG1PL_LIBM"
    gl_FUNC_LOG1PL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_log1pl_works" in
      *yes) ;;
      *) REPLACE_LOG1PL=1 ;;
    esac

    m4_ifdef([gl_FUNC_LOG1PL_IEEE], [
      if test $gl_log1pl_required = ieee && test $REPLACE_LOG1PL = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether log1pl works according to ISO C 99 with IEC 60559],
          [gl_cv_func_log1pl_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $LOG1PL_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
]gl_LONG_DOUBLE_MINUS_ZERO_CODE[
]gl_LONG_DOUBLE_SIGNBIT_CODE[
static long double dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_log1pl) (long double) = argc ? log1pl : dummy;
  /* This test fails on AIX 7.1, IRIX 6.5.  */
  long double y = my_log1pl (minus_zerol);
  if (!(y == 0.0L) || (signbitl (minus_zerol) && !signbitl (y)))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_log1pl_ieee=yes],
              [gl_cv_func_log1pl_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_log1pl_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_log1pl_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_log1pl_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_log1pl_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_log1pl_ieee" in
          *yes) ;;
          *) REPLACE_LOG1PL=1 ;;
        esac
      fi
    ])
  else
    HAVE_LOG1PL=0
    dnl Find libraries needed to link lib/log1pl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      LOG1PL_LIBM="$LOG1P_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ISNANL])
      AC_REQUIRE([gl_FUNC_LOGL])
      AC_REQUIRE([gl_FUNC_ROUNDL])
      LOG1PL_LIBM=
      dnl Append $ISNANL_LIBM to LOG1PL_LIBM, avoiding gratuitous duplicates.
      case " $LOG1PL_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) LOG1PL_LIBM="$LOG1PL_LIBM $ISNANL_LIBM" ;;
      esac
      dnl Append $LOGL_LIBM to LOG1PL_LIBM, avoiding gratuitous duplicates.
      case " $LOG1PL_LIBM " in
        *" $LOGL_LIBM "*) ;;
        *) LOG1PL_LIBM="$LOG1PL_LIBM $LOGL_LIBM" ;;
      esac
      dnl Append $ROUNDL_LIBM to LOG1PL_LIBM, avoiding gratuitous duplicates.
      case " $LOG1PL_LIBM " in
        *" $ROUNDL_LIBM "*) ;;
        *) LOG1PL_LIBM="$LOG1PL_LIBM $ROUNDL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([LOG1PL_LIBM])
])

dnl Test whether log1pl() works.
dnl On musl 1.2.2/{arm64,s390x}, the result is accurate to only 16 digits.
AC_DEFUN([gl_FUNC_LOG1PL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether log1pl works], [gl_cv_func_log1pl_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <float.h>
#include <math.h>
/* Override the values of <float.h>, like done in float.in.h.  */
#if defined __i386__ && (defined __BEOS__ || defined __OpenBSD__)
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG   64
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
# undef LDBL_MAX_EXP
# define LDBL_MAX_EXP    16384
#endif
#if defined __i386__ && (defined __FreeBSD__ || defined __DragonFly__)
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG   64
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP    (-16381)
# undef LDBL_MAX_EXP
# define LDBL_MAX_EXP    16384
#endif
#if (defined _ARCH_PPC || defined _POWER) && defined _AIX && (LDBL_MANT_DIG == 106) && defined __GNUC__
# undef LDBL_MIN_EXP
# define LDBL_MIN_EXP DBL_MIN_EXP
#endif
#if defined __sgi && (LDBL_MANT_DIG >= 106)
# undef LDBL_MANT_DIG
# define LDBL_MANT_DIG 106
# if defined __GNUC__
#  undef LDBL_MIN_EXP
#  define LDBL_MIN_EXP DBL_MIN_EXP
# endif
#endif
#undef log1pl
extern
#ifdef __cplusplus
"C"
#endif
long double log1pl (long double);
static long double dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_log1pl) (long double) = argc ? log1pl : dummy;
  int result = 0;
  /* This test fails on musl 1.2.2/arm64, musl 1.2.2/s390x.  */
  {
    const long double TWO_LDBL_MANT_DIG = /* 2^LDBL_MANT_DIG */
      (long double) (1U << ((LDBL_MANT_DIG - 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 2) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 3) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 4) / 5));
    long double x = 11.358L;
    long double y = my_log1pl (x);
    long double z = my_log1pl (- x / (1.0L + x));
    long double err = (y + z) * TWO_LDBL_MANT_DIG;
    if (!(err >= -900.0L && err <= 900.0L))
      result |= 1;
  }

  return result;
}
]])],
        [gl_cv_func_log1pl_works=yes],
        [gl_cv_func_log1pl_works=no],
        [case "$host_os" in
                          # Guess yes on glibc systems.
           *-gnu* | gnu*) gl_cv_func_log1pl_works="guessing yes" ;;
                          # Guess no on musl systems.
           *-musl*)       gl_cv_func_log1pl_works="guessing no" ;;
                          # Guess yes on native Windows.
           mingw*)        gl_cv_func_log1pl_works="guessing yes" ;;
                          # If we don't know, obey --enable-cross-guesses.
           *)             gl_cv_func_log1pl_works="$gl_cross_guess_normal" ;;
         esac
        ])
    ])
])
