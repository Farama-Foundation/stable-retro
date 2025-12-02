# asinf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ASINF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_ASIN])

  dnl Persuade glibc <math.h> to declare asinf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether asinf() exists. Assume that asinf(), if it exists, is
  dnl defined in the same library as asin().
  save_LIBS="$LIBS"
  LIBS="$LIBS $ASIN_LIBM"
  AC_CHECK_FUNCS([asinf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_asinf = yes; then
    ASINF_LIBM="$ASIN_LIBM"
  else
    HAVE_ASINF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_asinf, not asinf.
    AC_CHECK_DECLS([asinf], [REPLACE_ASINF=1], , [[#include <math.h>]])
    ASINF_LIBM="$ASIN_LIBM"
  fi
  AC_SUBST([ASINF_LIBM])
])
