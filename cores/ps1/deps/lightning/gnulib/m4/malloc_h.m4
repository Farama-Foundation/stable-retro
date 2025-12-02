# malloc_h.m4 serial 1
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Bruno Haible.

AC_DEFUN([gl_MALLOC_H],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_MALLOC_H_DEFAULTS])

  gl_CHECK_NEXT_HEADERS([malloc.h])
  if test $ac_cv_header_malloc_h = yes; then
    HAVE_MALLOC_H=1
  else
    HAVE_MALLOC_H=0
  fi
  AC_SUBST([HAVE_MALLOC_H])

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use.
  gl_WARN_ON_USE_PREPARE([[
#if HAVE_MALLOC_H
# include <malloc.h>
#endif
    ]], [memalign])
])

AC_DEFUN([gl_MALLOC_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_MALLOC_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_MALLOC_H_DEFAULTS],
[
  GNULIB_MEMALIGN=0;      AC_SUBST([GNULIB_MEMALIGN])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_MEMALIGN=1;        AC_SUBST([HAVE_MEMALIGN])
  REPLACE_MEMALIGN=0;     AC_SUBST([REPLACE_MEMALIGN])
])
