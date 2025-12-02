# pthread-tss.m4 serial 1
dnl Copyright (C) 2019-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PTHREAD_TSS],
[
  AC_REQUIRE([gl_PTHREAD_H])
  AC_REQUIRE([AC_CANONICAL_HOST])

  if { case "$host_os" in mingw*) true;; *) false;; esac; } \
     && test $gl_threads_api = windows; then
    dnl Choose function names that don't conflict with the mingw-w64 winpthreads
    dnl library.
    REPLACE_PTHREAD_KEY_CREATE=1
    REPLACE_PTHREAD_SETSPECIFIC=1
    REPLACE_PTHREAD_GETSPECIFIC=1
    REPLACE_PTHREAD_KEY_DELETE=1
  else
    if test $HAVE_PTHREAD_H = 0; then
      HAVE_PTHREAD_KEY_CREATE=0
      HAVE_PTHREAD_SETSPECIFIC=0
      HAVE_PTHREAD_GETSPECIFIC=0
      HAVE_PTHREAD_KEY_DELETE=0
    fi
  fi
])
