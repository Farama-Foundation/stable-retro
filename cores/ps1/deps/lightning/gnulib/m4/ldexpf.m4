# ldexpf.m4 serial 2
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_LDEXPF],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_LDEXP])

  dnl Persuade glibc <math.h> to declare ldexpf().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  dnl Test whether ldexpf() exists. We cannot assume that ldexpf(), if it
  dnl exists, is defined in the same library as ldexp(). This is not the case
  dnl on FreeBSD, NetBSD, OpenBSD.
  gl_MATHFUNC([ldexpf], [float], [(float, int)])
  if test $gl_cv_func_ldexpf_no_libm = no \
     && test $gl_cv_func_ldexpf_in_libm = no; then
    HAVE_LDEXPF=0
    LDEXPF_LIBM="$LDEXP_LIBM"
  fi
  AC_SUBST([LDEXPF_LIBM])
])
