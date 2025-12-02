# log10l.m4 serial 10
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOG10L],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_LOG10])

  dnl Persuade glibc <math.h> to declare log10l().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether log10l() is declared. On AIX 5.1 it is not declared.
  AC_CHECK_DECL([log10l], , [HAVE_DECL_LOG10L=0], [[#include <math.h>]])

  dnl Test whether log10l() exists. Assume that log10l(), if it exists, is
  dnl defined in the same library as log10().
  save_LIBS="$LIBS"
  LIBS="$LIBS $LOG10_LIBM"
  AC_CHECK_FUNCS([log10l])
  LIBS="$save_LIBS"
  if test $ac_cv_func_log10l = yes; then
    LOG10L_LIBM="$LOG10_LIBM"

    save_LIBS="$LIBS"
    LIBS="$LIBS $LOG10L_LIBM"
    gl_FUNC_LOG10L_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_log10l_works" in
      *yes) ;;
      *) REPLACE_LOG10L=1 ;;
    esac
  else
    HAVE_LOG10L=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_log10l, not log10l.
    AC_CHECK_DECLS([log10l], [REPLACE_LOG10L=1], , [[#include <math.h>]])
  fi
  if test $HAVE_LOG10L = 0 || test $REPLACE_LOG10L = 1; then
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      LOG10L_LIBM="$LOG10_LIBM"
    else
      if test $HAVE_LOG10L = 0; then
        AC_REQUIRE([gl_FUNC_LOGL])
        LOG10L_LIBM="$LOGL_LIBM"
      fi
    fi
  fi
  AC_SUBST([LOG10L_LIBM])
])

dnl Test whether log10l() works.
dnl On OSF/1 5.1, log10l(-0.0L) is NaN.
dnl On IRIX 6.5, log10l(-0.0L) is an unnormalized negative infinity
dnl 0xFFF00000000000007FF0000000000000, should be
dnl 0xFFF00000000000000000000000000000.
dnl On AIX 5.1, log10l(-0.0L) is finite if it's not the first log10l call
dnl in the program.
dnl On NetBSD 9.0, the result is accurate to only 16 digits.
AC_DEFUN([gl_FUNC_LOG10L_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether log10l works], [gl_cv_func_log10l_works],
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
#undef log10l /* for AIX */
extern
#ifdef __cplusplus
"C"
#endif
long double log10l (long double);
static long double dummy (long double x) { return 0; }
volatile long double gx;
long double gy;
int main (int argc, char *argv[])
{
  long double (* volatile my_log10l) (long double) = argc ? log10l : dummy;
  int result = 0;
  /* Dummy call, to trigger the AIX 5.1 bug.  */
  gx = 0.6L;
  gy = log10l (gx);
  /* This test fails on AIX 5.1, IRIX 6.5, OSF/1 5.1.  */
  {
    gx = -0.0L;
    gy = log10l (gx);
    if (!(gy + gy == gy))
      result |= 1;
  }
  /* This test fails on musl 1.2.2/arm64, musl 1.2.2/s390x, NetBSD 9.0.  */
  {
    const long double TWO_LDBL_MANT_DIG = /* 2^LDBL_MANT_DIG */
      (long double) (1U << ((LDBL_MANT_DIG - 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 1) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 2) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 3) / 5))
      * (long double) (1U << ((LDBL_MANT_DIG - 1 + 4) / 5));
    long double x = 7.90097792256024576L;
    long double err = (my_log10l (x) + my_log10l (1.0L / x)) * TWO_LDBL_MANT_DIG;
    if (!(err >= -100.0L && err <= 100.0L))
      result |= 2;
  }
  return result;
}
]])],
        [gl_cv_func_log10l_works=yes],
        [gl_cv_func_log10l_works=no],
        [case "$host_os" in
                          # Guess yes on glibc systems.
           *-gnu* | gnu*) gl_cv_func_log10l_works="guessing yes" ;;
                          # Guess no on musl systems.
           *-musl*)       gl_cv_func_log10l_works="guessing no" ;;
                          # Guess yes on native Windows.
           mingw*)        gl_cv_func_log10l_works="guessing yes" ;;
                          # If we don't know, obey --enable-cross-guesses.
           *)             gl_cv_func_log10l_works="$gl_cross_guess_normal" ;;
         esac
        ])
    ])
])
