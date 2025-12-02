# poll_h.m4 serial 3
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Bruno Haible.

AC_DEFUN([gl_POLL_H],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_POLL_H_DEFAULTS])

  AC_CHECK_HEADERS_ONCE([poll.h])
  if test $ac_cv_header_poll_h = yes; then
    HAVE_POLL_H=1
  else
    HAVE_POLL_H=0
  fi
  AC_SUBST([HAVE_POLL_H])

  dnl <poll.h> is always overridden, because of GNULIB_POSIXCHECK.
  gl_CHECK_NEXT_HEADERS([poll.h])

  gl_PREREQ_SYS_H_WINSOCK2 dnl for HAVE_WINSOCK2_H

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use.
  gl_WARN_ON_USE_PREPARE([[#include <poll.h>]],
    [poll])
])

AC_DEFUN([gl_POLL_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_POLL_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_POLL_H_DEFAULTS],
[
  GNULIB_POLL=0;        AC_SUBST([GNULIB_POLL])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_POLL=1;          AC_SUBST([HAVE_POLL])
  REPLACE_POLL=0;       AC_SUBST([REPLACE_POLL])
])
