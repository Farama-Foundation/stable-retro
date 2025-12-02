# va-args.m4 serial 1
dnl Copyright (C) 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Test for variadic macro support.
dnl Define HAVE_VA_ARGS if macros can use __VA_ARGS__.
AC_DEFUN([gl_VA_ARGS],
[
  AC_CACHE_CHECK([whether the preprocessor allows variadic macros],
    [gl_cv_c_va_args],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
       #include <stdio.h>
       #define a(func, arg, ...) func (arg, __VA_ARGS__)
       #define b(...) a (__VA_ARGS__)
      ]], [[b (printf, "%s", "test");]])],
      [gl_cv_c_va_args=yes], [gl_cv_c_va_args=no])
    ])
  if test $gl_cv_c_va_args = yes; then
    AC_DEFINE([HAVE_VA_ARGS], [1],
      [Define to 1 if the preprocessor supports variadic macros and
       __VA_ARGS__.])
  fi
])
