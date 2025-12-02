# fmodl.m4 serial 11
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FMODL],
[
  m4_divert_text([DEFAULTS], [gl_fmodl_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([gl_FUNC_FMOD])

  dnl Persuade glibc <math.h> to declare fmodl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether fmodl() is declared.
  dnl AIX 5.1 has fmodl() in libc but doesn't declare it in <math.h>, and the
  dnl function produces NaN results for finite values.
  AC_CHECK_DECL([fmodl],
    [ac_cv_have_decl_fmodl=yes], [ac_cv_have_decl_fmodl=no],
    [[#include <math.h>]])

  dnl Test whether fmodl() exists. Assume that fmodl(), if it exists, is
  dnl defined in the same library as fmod().
  save_LIBS="$LIBS"
  LIBS="$LIBS $FMOD_LIBM"
  AC_CHECK_FUNCS([fmodl])
  LIBS="$save_LIBS"
  if test $ac_cv_func_fmodl = yes; then
    FMODL_LIBM="$FMOD_LIBM"
    if test $ac_cv_have_decl_fmodl = no; then
      REPLACE_FMODL=1
    fi
    m4_ifdef([gl_FUNC_FMODL_IEEE], [
      if test $gl_fmodl_required = ieee && test $REPLACE_FMODL = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether fmodl works according to ISO C 99 with IEC 60559],
          [gl_cv_func_fmodl_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $FMODL_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
/* Compare two numbers with ==.
   This is a separate function because IRIX 6.5 "cc -O" miscompiles an
   'x == x' test.  */
static int
numeric_equal (long double x, long double y)
{
  return x == y;
}
static long double dummy (long double x, long double y) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_fmodl) (long double, long double) = argc ? fmodl : dummy;
  long double f;
  /* Test fmodl(...,0.0L).
     This test fails on OSF/1 5.1.  */
  f = my_fmodl (2.0L, 0.0L);
  if (numeric_equal (f, f))
    return 1;
  return 0;
}
              ]])],
              [gl_cv_func_fmodl_ieee=yes],
              [gl_cv_func_fmodl_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_fmodl_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_fmodl_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_fmodl_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_fmodl_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_fmodl_ieee" in
          *yes) ;;
          *) REPLACE_FMODL=1 ;;
        esac
      fi
    ])
  else
    HAVE_FMODL=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_fmodl, not fmodl.
    AC_CHECK_DECLS([fmodl], [REPLACE_FMODL=1], , [[#include <math.h>]])
  fi
  if test $HAVE_FMODL = 0 || test $REPLACE_FMODL = 1; then
    dnl Find libraries needed to link lib/fmodl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      FMODL_LIBM="$FMOD_LIBM"
    else
      AC_REQUIRE([gl_FUNC_FABSL])
      AC_REQUIRE([gl_FUNC_FREXPL])
      AC_REQUIRE([gl_FUNC_TRUNCL])
      AC_REQUIRE([gl_FUNC_LDEXPL])
      AC_REQUIRE([gl_FUNC_ISNANL])
      FMODL_LIBM=
      dnl Append $FABSL_LIBM to FMODL_LIBM, avoiding gratuitous duplicates.
      case " $FMODL_LIBM " in
        *" $FABSL_LIBM "*) ;;
        *) FMODL_LIBM="$FMODL_LIBM $FABSL_LIBM" ;;
      esac
      dnl Append $FREXPL_LIBM to FMODL_LIBM, avoiding gratuitous duplicates.
      case " $FMODL_LIBM " in
        *" $FREXPL_LIBM "*) ;;
        *) FMODL_LIBM="$FMODL_LIBM $FREXPL_LIBM" ;;
      esac
      dnl Append $TRUNCL_LIBM to FMODL_LIBM, avoiding gratuitous duplicates.
      case " $FMODL_LIBM " in
        *" $TRUNCL_LIBM "*) ;;
        *) FMODL_LIBM="$FMODL_LIBM $TRUNCL_LIBM" ;;
      esac
      dnl Append $LDEXPL_LIBM to FMODL_LIBM, avoiding gratuitous duplicates.
      case " $FMODL_LIBM " in
        *" $LDEXPL_LIBM "*) ;;
        *) FMODL_LIBM="$FMODL_LIBM $LDEXPL_LIBM" ;;
      esac
      dnl Append $ISNANL_LIBM to FMODL_LIBM, avoiding gratuitous duplicates.
      case " $FMODL_LIBM " in
        *" $ISNANL_LIBM "*) ;;
        *) FMODL_LIBM="$FMODL_LIBM $ISNANL_LIBM" ;;
      esac
    fi
  fi
  AC_SUBST([FMODL_LIBM])
])
