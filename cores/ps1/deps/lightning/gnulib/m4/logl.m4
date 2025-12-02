# logl.m4 serial 15
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LOGL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare logl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  LOGL_LIBM=
  AC_CACHE_CHECK([whether logl() can be used without linking with libm],
    [gl_cv_func_logl_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             long double (*funcptr) (long double) = logl;
             long double x;]],
           [[return funcptr (x) > 1
                    || logl (x) > 1;]])],
        [gl_cv_func_logl_no_libm=yes],
        [gl_cv_func_logl_no_libm=no])
    ])
  if test $gl_cv_func_logl_no_libm = no; then
    AC_CACHE_CHECK([whether logl() can be used with libm],
      [gl_cv_func_logl_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               long double (*funcptr) (long double) = logl;
               long double x;]],
             [[return funcptr (x) > 1
                      || logl (x) > 1;]])],
          [gl_cv_func_logl_in_libm=yes],
          [gl_cv_func_logl_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_logl_in_libm = yes; then
      LOGL_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_logl_no_libm = yes \
     || test $gl_cv_func_logl_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl Mac OS X 10.3 has logl() in libc but doesn't declare it in <math.h>.
    AC_CHECK_DECL([logl], , [HAVE_DECL_LOGL=0], [[#include <math.h>]])
    save_LIBS="$LIBS"
    LIBS="$LIBS $LOGL_LIBM"
    gl_FUNC_LOGL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_logl_works" in
      *yes) ;;
      *) REPLACE_LOGL=1 ;;
    esac
  else
    HAVE_LOGL=0
    HAVE_DECL_LOGL=0
  fi
  if test $HAVE_LOGL = 0 || test $REPLACE_LOGL = 1; then
    dnl Find libraries needed to link lib/logl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_LOG])
      LOGL_LIBM="$LOG_LIBM"
    else
      if test $HAVE_LOGL = 0; then
        AC_REQUIRE([gl_FUNC_FREXPL])
        AC_REQUIRE([gl_FUNC_ISNANL])
        AC_REQUIRE([gl_FUNC_FLOORL])
        dnl Append $FREXPL_LIBM to LOGL_LIBM, avoiding gratuitous duplicates.
        case " $LOGL_LIBM " in
          *" $FREXPL_LIBM "*) ;;
          *) LOGL_LIBM="$LOGL_LIBM $FREXPL_LIBM" ;;
        esac
        dnl Append $ISNANL_LIBM to LOGL_LIBM, avoiding gratuitous duplicates.
        case " $LOGL_LIBM " in
          *" $ISNANL_LIBM "*) ;;
          *) LOGL_LIBM="$LOGL_LIBM $ISNANL_LIBM" ;;
        esac
        dnl Append $FLOORL_LIBM to LOGL_LIBM, avoiding gratuitous duplicates.
        case " $LOGL_LIBM " in
          *" $FLOORL_LIBM "*) ;;
          *) LOGL_LIBM="$LOGL_LIBM $FLOORL_LIBM" ;;
        esac
      fi
    fi
  fi
  AC_SUBST([LOGL_LIBM])
])

dnl Test whether logl() works.
dnl On OSF/1 5.1, logl(-0.0L) is NaN.
dnl On NetBSD 9.0, the result is accurate to only 16 digits.
AC_DEFUN([gl_FUNC_LOGL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether logl works], [gl_cv_func_logl_works],
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
#undef logl
extern
#ifdef __cplusplus
"C"
#endif
long double logl (long double);
static long double dummy (long double x) { return 0; }
volatile long double gx;
long double gy;
int main (int argc, char *argv[])
{
  long double (* volatile my_logl) (long double) = argc ? logl : dummy;
  int result = 0;
  /* This test fails on OSF/1 5.1.  */
  {
    gx = -0.0L;
    gy = logl (gx);
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
    long double x = 16.981137113807045L;
    long double err = (my_logl (x) + my_logl (1.0L / x)) * TWO_LDBL_MANT_DIG;
    if (!(err >= -100.0L && err <= 100.0L))
      result |= 2;
  }

  return result;
}
]])],
        [gl_cv_func_logl_works=yes],
        [gl_cv_func_logl_works=no],
        [case "$host_os" in
                          # Guess yes on glibc systems.
           *-gnu* | gnu*) gl_cv_func_logl_works="guessing yes" ;;
                          # Guess no on musl systems.
           *-musl*)       gl_cv_func_logl_works="guessing no" ;;
                          # Guess yes on native Windows.
           mingw*)        gl_cv_func_logl_works="guessing yes" ;;
                          # If we don't know, obey --enable-cross-guesses.
           *)             gl_cv_func_logl_works="$gl_cross_guess_normal" ;;
         esac
        ])
    ])
])
