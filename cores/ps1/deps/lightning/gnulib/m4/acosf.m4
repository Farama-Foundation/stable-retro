# acosf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_ACOSF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_ACOS])

  dnl Persuade glibc <math.h> to declare acosf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether acosf() exists. Assume that acosf(), if it exists, is
  dnl defined in the same library as acos().
  save_LIBS="$LIBS"
  LIBS="$LIBS $ACOS_LIBM"
  AC_CHECK_FUNCS([acosf])
  LIBS="$save_LIBS"
  if test $ac_cv_func_acosf = yes; then
    ACOSF_LIBM="$ACOS_LIBM"
  else
    HAVE_ACOSF=0
    dnl If the function is declared but does not appear to exist, it may be
    dnl defined as an inline function. In order to avoid a conflict, we have
    dnl to define rpl_acosf, not acosf.
    AC_CHECK_DECLS([acosf], [REPLACE_ACOSF=1], , [[#include <math.h>]])
    ACOSF_LIBM="$ACOS_LIBM"
  fi
  AC_SUBST([ACOSF_LIBM])
])
