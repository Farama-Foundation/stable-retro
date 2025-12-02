# fabsf.m4 serial 3
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FABSF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_FABS])

  dnl Persuade glibc <math.h> to declare fabsf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether fabsf() exists. We cannot assume that fabsf(), if it
  dnl exists, is defined in the same library as fabs(). This is not the case
  dnl on FreeBSD, NetBSD, OpenBSD.
  gl_MATHFUNC([fabsf], [float], [(float)])
  if test $gl_cv_func_fabsf_no_libm = no \
     && test $gl_cv_func_fabsf_in_libm = no; then
    HAVE_FABSF=0
    FABSF_LIBM="$FABS_LIBM"
  fi
  AC_SUBST([FABSF_LIBM])
])
