# wcsftime.m4 serial 1
dnl Copyright (C) 2017-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCSFTIME],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CHECK_FUNCS_ONCE([wcsftime])
  if test $ac_cv_func_wcsftime = no; then
    HAVE_WCSFTIME=0
  else
    case "$host_os" in
      mingw*) REPLACE_WCSFTIME=1 ;;
    esac
  fi
])
