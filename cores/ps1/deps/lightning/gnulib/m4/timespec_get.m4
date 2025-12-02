# timespec_get.m4 serial 1
dnl Copyright (C) 2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_TIMESPEC_GET],
[
  AC_REQUIRE([gl_HEADER_TIME_H_DEFAULTS])

  dnl Persuade OpenBSD <time.h> to declare timespec_get().
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([timespec_get])
  if test $ac_cv_func_timespec_get != yes; then
    HAVE_TIMESPEC_GET=0
  fi
])
