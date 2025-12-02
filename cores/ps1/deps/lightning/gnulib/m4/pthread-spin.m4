# pthread-spin.m4 serial 2
dnl Copyright (C) 2019-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PTHREAD_SPIN],
[
  AC_REQUIRE([gl_PTHREAD_H])
  AC_REQUIRE([AC_CANONICAL_HOST])

  if { case "$host_os" in mingw*) true;; *) false;; esac; } \
     && test $gl_threads_api = windows; then
    dnl Choose function names that don't conflict with the mingw-w64 winpthreads
    dnl library.
    REPLACE_PTHREAD_SPIN_INIT=1
    REPLACE_PTHREAD_SPIN_LOCK=1
    REPLACE_PTHREAD_SPIN_TRYLOCK=1
    REPLACE_PTHREAD_SPIN_UNLOCK=1
    REPLACE_PTHREAD_SPIN_DESTROY=1
  else
    if test $HAVE_PTHREAD_H = 0 || test $HAVE_PTHREAD_SPINLOCK_T = 0; then
      HAVE_PTHREAD_SPIN_INIT=0
      HAVE_PTHREAD_SPIN_LOCK=0
      HAVE_PTHREAD_SPIN_TRYLOCK=0
      HAVE_PTHREAD_SPIN_UNLOCK=0
      HAVE_PTHREAD_SPIN_DESTROY=0
    fi
  fi
])
