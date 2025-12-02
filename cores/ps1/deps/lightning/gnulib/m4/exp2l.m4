# exp2l.m4 serial 10
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXP2L],
[
  m4_divert_text([DEFAULTS], [gl_exp2l_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_EXP2])

  dnl Persuade glibc <math.h> to declare exp2l().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether exp2l() exists. Assume that exp2l(), if it exists, is
  dnl defined in the same library as exp2().
  save_LIBS="$LIBS"
  LIBS="$LIBS $EXP2_LIBM"
  AC_CHECK_FUNCS([exp2l])
  LIBS="$save_LIBS"
  if test $ac_cv_func_exp2l = yes; then
    HAVE_EXP2L=1
    EXP2L_LIBM="$EXP2_LIBM"
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has exp2l() in libm but doesn't declare it in <math.h>.
    AC_CHECK_DECL([exp2l], , [HAVE_DECL_EXP2L=0], [[#include <math.h>]])
    if test $REPLACE_EXP2L = 0; then
      AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
      AC_CACHE_CHECK([whether exp2l works],
        [gl_cv_func_exp2l_works],
        [
          save_LIBS="$LIBS"
          LIBS="$LIBS $EXP2L_LIBM"
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
#undef exp2l
extern
#ifdef __cplusplus
"C"
#endif
long double exp2l (long double);
static long double dummy (long double x) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_exp2l) (long double) = argc ? exp2l : dummy;
  int result = 0;
  /* This test fails on NetBSD 9.0.  */
  {
    const long double TWO_LDBL_MANT_DIG = /* 2^LDBL_MANT_DIG */
      (long double) (1U << ((LDBL_MANT_DIG - 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 2) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 3) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 4) / 5));
    long double x = 11.358L;
    long double err = (my_exp2l (x) * my_exp2l (- x) - 1.0L) * TWO_LDBL_MANT_DIG;
    if (!(err >= -100.0L && err <= 100.0L))
      result |= 1;
  }
  return result;
}
            ]])],
            [gl_cv_func_exp2l_works=yes],
            [gl_cv_func_exp2l_works=no],
            [case "$host_os" in
                              # Guess yes on glibc systems.
               *-gnu* | gnu*) gl_cv_func_exp2l_works="guessing yes" ;;
                              # Guess yes on musl systems.
               *-musl*)       gl_cv_func_exp2l_works="guessing yes" ;;
                              # Guess yes on native Windows.
               mingw*)        gl_cv_func_exp2l_works="guessing yes" ;;
                              # If we don't know, obey --enable-cross-guesses.
               *)             gl_cv_func_exp2l_works="$gl_cross_guess_normal" ;;
             esac
            ])
          LIBS="$save_LIBS"
        ])
      case "$gl_cv_func_exp2l_works" in
        *yes) ;;
        *) REPLACE_EXP2L=1 ;;
      esac
    fi
    m4_ifdef([gl_FUNC_EXP2L_IEEE], [
      if test $gl_exp2l_required = ieee && test $REPLACE_EXP2L = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether exp2l works according to ISO C 99 with IEC 60559],
          [gl_cv_func_exp2l_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $EXP2L_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
#undef exp2l
extern
#ifdef __cplusplus
"C"
#endif
long double exp2l (long double);
static long double dummy (long double x) { return 0; }
static long double zero;
int main (int argc, char *argv[])
{
  long double (* volatile my_exp2l) (long double) = argc ? exp2l : dummy;
  int result = 0;
  /* This test fails on OpenBSD 4.9, where exp2l(NaN) = 0.0.  */
  if (exp2l (zero / zero) == 0.0L)
    result |= 1;
  /* This test fails on IRIX 6.5, where exp2l(-Inf) = 1.0.  */
  if (!(exp2l (-1.0L / zero) == 0.0L))
    result |= 2;
  return result;
}
              ]])],
              [gl_cv_func_exp2l_ieee=yes],
              [gl_cv_func_exp2l_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_exp2l_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_exp2l_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_exp2l_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_exp2l_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_exp2l_ieee" in
          *yes) ;;
          *) REPLACE_EXP2L=1 ;;
        esac
      fi
    ])
  else
    HAVE_EXP2L=0
    HAVE_DECL_EXP2L=0
  fi
  if test $HAVE_EXP2L = 0 || test $REPLACE_EXP2L = 1; then
    dnl Find libraries needed to link lib/exp2l.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      EXP2L_LIBM="$EXP2_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ISNANL])
      AC_REQUIRE([gl_FUNC_ROUNDL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      EXP2L_LIBM=
      dnl Append $ISNANL_LIBM to EXP2L_LIBM, avoiding gratuitous duplicates.
      case " $EXP2L_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) EXP2L_LIBM="$EXP2L_LIBM $ISNANL_LIBM" ;;
      esac
      dnl Append $ROUNDL_LIBM to EXP2L_LIBM, avoiding gratuitous duplicates.
      case " $EXP2L_LIBM " in
        *" $ROUNDL_LIBM "*) ;;
        *) EXP2L_LIBM="$EXP2L_LIBM $ROUNDL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to EXP2L_LIBM, avoiding gratuitous duplicates.
      case " $EXP2L_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) EXP2L_LIBM="$EXP2L_LIBM $LDEXPL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([EXP2L_LIBM])
])
