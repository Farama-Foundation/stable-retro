# sqrtl.m4 serial 11
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SQRTL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare sqrtl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  SQRTL_LIBM=
  AC_CACHE_CHECK([whether sqrtl() can be used without linking with libm],
    [gl_cv_func_sqrtl_no_libm],
    [
      AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
           [[#ifndef __NO_MATH_INLINES
             # define __NO_MATH_INLINES 1 /* for glibc */
             #endif
             #include <math.h>
             long double (* volatile funcptr) (long double) = sqrtl;
             long double x;]],
           [[return funcptr (x) > 0.4
                    || sqrtl (x) > 0.4;]])],
        [gl_cv_func_sqrtl_no_libm=yes],
        [gl_cv_func_sqrtl_no_libm=no])
    ])
  if test $gl_cv_func_sqrtl_no_libm = no; then
    AC_CACHE_CHECK([whether sqrtl() can be used with libm],
      [gl_cv_func_sqrtl_in_libm],
      [
        save_LIBS="$LIBS"
        LIBS="$LIBS -lm"
        AC_LINK_IFELSE(
          [AC_LANG_PROGRAM(
             [[#ifndef __NO_MATH_INLINES
               # define __NO_MATH_INLINES 1 /* for glibc */
               #endif
               #include <math.h>
               long double (* volatile funcptr) (long double) = sqrtl;
               long double x;]],
             [[return funcptr (x) > 0.4
                      || sqrtl (x) > 0.4;]])],
          [gl_cv_func_sqrtl_in_libm=yes],
          [gl_cv_func_sqrtl_in_libm=no])
        LIBS="$save_LIBS"
      ])
    if test $gl_cv_func_sqrtl_in_libm = yes; then
      SQRTL_LIBM=-lm
    fi
  fi
  if test $gl_cv_func_sqrtl_no_libm = yes \
     || test $gl_cv_func_sqrtl_in_libm = yes; then
    dnl Also check whether it's declared.
    dnl Mac OS X 10.3 has sqrtl() in libc but doesn't declare it in <math.h>.
    AC_CHECK_DECL([sqrtl], , [HAVE_DECL_SQRTL=0], [[#include <math.h>]])

    save_LIBS="$LIBS"
    LIBS="$LIBS $SQRTL_LIBM"
    gl_FUNC_SQRTL_WORKS
    LIBS="$save_LIBS"
    case "$gl_cv_func_sqrtl_works" in
      *yes) ;;
      *) REPLACE_SQRTL=1 ;;
    esac
  else
    HAVE_DECL_SQRTL=0
    HAVE_SQRTL=0
  fi
  if test $HAVE_SQRTL = 0 || test $REPLACE_SQRTL = 1; then
    dnl Find libraries needed to link lib/sqrtl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_SQRT])
      SQRTL_LIBM="$SQRT_LIBM"
    else
      AC_REQUIRE([gl_FUNC_ISNANL])
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      AC_REQUIRE([gl_FUNC_SQRT])
      dnl Append $ISNANL_LIBM to SQRTL_LIBM, avoiding gratuitous duplicates.
      case " $SQRTL_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) SQRTL_LIBM="$SQRTL_LIBM $ISNANL_LIBM" ;;
      esac
      dnl Append $FREXPL_LIBM to SQRTL_LIBM, avoiding gratuitous duplicates.
      case " $SQRTL_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) SQRTL_LIBM="$SQRTL_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to SQRTL_LIBM, avoiding gratuitous duplicates.
      case " $SQRTL_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) SQRTL_LIBM="$SQRTL_LIBM $LDEXPL_LIBM" ;;
      esac
      dnl Append $SQRT_LIBM to SQRTL_LIBM, avoiding gratuitous duplicates.
      case " $SQRTL_LIBM " in
        *" $SQRT_LIBM "*) ;;
        *) SQRTL_LIBM="$SQRTL_LIBM $SQRT_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([SQRTL_LIBM])
])

dnl Test whether sqrtl() works.
dnl On OpenBSD 5.1/SPARC, sqrtl(8.1974099812331540680810141969554806865L) has
dnl rounding errors that eat up the last 8 to 9 decimal digits.
AC_DEFUN([gl_FUNC_SQRTL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether sqrtl works], [gl_cv_func_sqrtl_works],
    [
      AC_RUN_IFELSE(
        [AC_LANG_SOURCE([[
#include <float.h>
#include <math.h>
#ifndef sqrtl /* for AIX */
extern
#ifdef __cplusplus
"C"
#endif
long double sqrtl (long double);
#endif
static long double
my_ldexpl (long double x, int d)
{
  for (; d > 0; d--)
    x *= 2.0L;
  for (; d < 0; d++)
    x *= 0.5L;
  return x;
}
volatile long double x;
volatile long double y;
long double z;
int main ()
{
  x = 8.1974099812331540680810141969554806865L;
  y = sqrtl (x);
  z = y * y - x;
  z = my_ldexpl (z, LDBL_MANT_DIG);
  if (z < 0)
    z = - z;
  if (z > 100.0L)
    return 1;
  return 0;
}
]])],
        [gl_cv_func_sqrtl_works=yes],
        [gl_cv_func_sqrtl_works=no],
        [case "$host_os" in
           openbsd*) gl_cv_func_sqrtl_works="guessing no" ;;
                     # Guess yes on native Windows.
           mingw*)   gl_cv_func_sqrtl_works="guessing yes" ;;
           *)        gl_cv_func_sqrtl_works="guessing yes" ;;
         esac
        ])
    ])
])
