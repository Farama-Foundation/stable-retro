# nonblocking.m4 serial 2
dnl Copyright (C) 2011-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Tests whether non-blocking I/O is natively supported by read(), write().
dnl Sets gl_cv_have_nonblocking.
dnl Also tests whether open() supports O_NONBLOCK.
dnl Sets gl_cv_have_open_O_NONBLOCK.
AC_DEFUN([gl_NONBLOCKING_IO],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_NONBLOCKING_IO_BODY])
])

AC_DEFUN([gl_NONBLOCKING_IO_BODY],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  case "$host_os" in
    mingw*) gl_cv_have_nonblocking=no ;;
    *)      gl_cv_have_nonblocking=yes ;;
  esac
  case "$host_os" in
    mingw*) gl_cv_have_open_O_NONBLOCK=no ;;
    *)      gl_cv_have_open_O_NONBLOCK=yes ;;
  esac
])
