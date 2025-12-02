# wcsdup.m4 serial 4
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCSDUP],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])
  AC_CACHE_CHECK([for wcsdup],
    [gl_cv_func_wcsdup],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <wchar.h>
          ]GL_MDA_DEFINES],
          [[return wcsdup (L"hello") != NULL;]])
       ],
       [gl_cv_func_wcsdup=yes],
       [gl_cv_func_wcsdup=no])
    ])
  if test $gl_cv_func_wcsdup = no; then
    HAVE_WCSDUP=0
  fi
])
