# sigabbrev_np.m4 serial 1
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SIGABBREV_NP],
[
  dnl Persuade glibc <string.h> to declare sigabbrev_np().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_CHECK_FUNCS([sigabbrev_np])
  if test $ac_cv_func_sigabbrev_np = no; then
    HAVE_SIGABBREV_NP=0
  fi
])
