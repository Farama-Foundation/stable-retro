# copysignl.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_COPYSIGNL],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])

  dnl Persuade glibc <math.h> to declare copysignl().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Determine COPYSIGNL_LIBM.
  gl_MATHFUNC([copysignl], [long double], [(long double, long double)])
  if test $gl_cv_func_copysignl_no_libm = no \
     && test $gl_cv_func_copysignl_in_libm = no; then
    HAVE_COPYSIGNL=0
    if test $HAVE_SAME_LONG_DOUBLE_AS_DOUBLE = 1; then
      AC_REQUIRE([gl_FUNC_COPYSIGN])
      COPYSIGNL_LIBM="$COPYSIGN_LIBM"
    else
      COPYSIGNL_LIBM=
    fi
  fi
  AC_SUBST([COPYSIGNL_LIBM])
])
