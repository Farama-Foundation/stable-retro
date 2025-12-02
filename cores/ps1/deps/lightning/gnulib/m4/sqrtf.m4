# sqrtf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SQRTF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_SQRT])

  dnl Persuade glibc <math.h> to declare sqrtf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether sqrtf() exists. Assume that sqrtf(), if it exists, is
  dnl defined in the same library as sqrt().
  save_LIBS="$LIBS"
  LIBS="$LIBS $SQRT_LIBM"
  AC_CHECK_FUNCS([sqrtf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_sqrtf = yes; then
    SQRTF_LIBM="$SQRT_LIBM"
  else
    HAVE_SQRTF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_sqrtf, not sqrtf.
    AC_CHECK_DECLS([sqrtf], [REPLACE_SQRTF=1], , [[#include <math.h>]])
    SQRTF_LIBM="$SQRT_LIBM"
  fi
  AC_SUBST([SQRTF_LIBM])
])
