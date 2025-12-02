# roundl.m4 serial 20
dnl Copyright (C) 2007, 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ROUNDL],
[
  m4_divert_text([DEFAULTS], [gl_roundl_required=plain])
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare roundl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  gl_CHECK_MATH_LIB([ROUNDL_LIBM], [x = roundl (x);],
    [extern
     #ifdef __cplusplus
     "C"
     #endif
     long double roundl (long double);
    ])
  if test "$ROUNDL_LIBM" != missing; then
    HAVE_ROUNDL=1
    dnl Also check whether it's declared.
    dnl IRIX 6.5 has roundl() in libm but doesn't declare it in <math.h>.
    AC_CHECK_DECLS([roundl], , [HAVE_DECL_ROUNDL=0], [[#include <math.h>]])

    m4_ifdef([gl_FUNC_ROUNDL_IEEE], [
      if test $gl_roundl_required = ieee && test $REPLACE_ROUNDL = 0; then
        AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
        AC_CACHE_CHECK([whether roundl works according to ISO C 99 with IEC 60559],
          [gl_cv_func_roundl_ieee],
          [
            save_LIBS="$LIBS"
            LIBS="$LIBS $ROUNDL_LIBM"
            AC_RUN_IFELSE(
              [AC_LANG_SOURCE([[
#ifndef __NO_MATH_INLINES
# define __NO_MATH_INLINES 1 /* for glibc */
#endif
#include <math.h>
extern
#ifdef __cplusplus
"C"
#endif
long double roundl (long double);
]gl_LONG_DOUBLE_MINUS_ZERO_CODE[
]gl_LONG_DOUBLE_SIGNBIT_CODE[
static long double dummy (long double f) { return 0; }
int main (int argc, char *argv[])
{
  long double (* volatile my_roundl) (long double) = argc ? roundl : dummy;
  int result = 0;
  /* Test whether roundl (-0.0L) is -0.0L.  */
  if (signbitl (minus_zerol) && !signbitl (my_roundl (minus_zerol)))
    result |= 1;
  /* Test whether roundl (-0.3L) is -0.0L.  */
  if (signbitl (-0.3L) && !signbitl (my_roundl (-0.3L)))
    result |= 2;
  return result;
}
              ]])],
              [gl_cv_func_roundl_ieee=yes],
              [gl_cv_func_roundl_ieee=no],
              [case "$host_os" in
                                # Guess yes on glibc systems.
                 *-gnu* | gnu*) gl_cv_func_roundl_ieee="guessing yes" ;;
                                # Guess yes on musl systems.
                 *-musl*)       gl_cv_func_roundl_ieee="guessing yes" ;;
                                # Guess yes on native Windows.
                 mingw*)        gl_cv_func_roundl_ieee="guessing yes" ;;
                                # If we don't know, obey --enable-cross-guesses.
                 *)             gl_cv_func_roundl_ieee="$gl_cross_guess_normal" ;;
               esac
              ])
            LIBS="$save_LIBS"
          ])
        case "$gl_cv_func_roundl_ieee" in
          *yes) ;;
          *) REPLACE_ROUNDL=1 ;;
        esac
      fi
    ])
  else
    HAVE_ROUNDL=0
    HAVE_DECL_ROUNDL=0
  fi
  if test $HAVE_ROUNDL = 0 || test $REPLACE_ROUNDL = 1; then
    dnl Find libraries needed to link lib/roundl.c.
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_ROUND])
      ROUNDL_LIBM="$ROUND_LIBM"
    else
      AC_CHECK_DECLS([ceill, floorl], , , [[#include <math.h>]])
      if test "$ac_cv_have_decl_floorl" = yes \
         && test "$ac_cv_have_decl_ceill" = yes; then
        gl_FUNC_FLOORL_LIBS
        gl_FUNC_CEILL_LIBS
        if test "$FLOORL_LIBM" != '?' && test "$CEILL_LIBM" != '?'; then
          AC_DEFINE([HAVE_FLOORL_AND_CEILL], [1],
            [Define if the both the floorl() and ceill() functions exist.])
          ROUNDL_LIBM=
          dnl Append $FLOORL_LIBM to ROUNDL_LIBM, avoiding gratuitous duplicates.
          case " $ROUNDL_LIBM " in
            *" $FLOORL_LIBM "*) ;;
            *) ROUNDL_LIBM="$ROUNDL_LIBM $FLOORL_LIBM" ;;
          esac
          dnl Append $CEILL_LIBM to ROUNDL_LIBM, avoiding gratuitous duplicates.
          case " $ROUNDL_LIBM " in
            *" $CEILL_LIBM "*) ;;
            *) ROUNDL_LIBM="$ROUNDL_LIBM $CEILL_LIBM" ;;
          esac
        else
          ROUNDL_LIBM=
        fi
      else
        ROUNDL_LIBM=
      fi
    fi
  fi
  AC_SUBST([ROUNDL_LIBM])
])
