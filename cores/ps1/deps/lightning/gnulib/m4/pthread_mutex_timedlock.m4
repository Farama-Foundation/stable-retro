# pthread_mutex_timedlock.m4 serial 2
dnl Copyright (C) 2019-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_PTHREAD_MUTEX_TIMEDLOCK],
[
  AC_REQUIRE([gl_PTHREAD_H_DEFAULTS])

  AC_CHECK_DECL([pthread_mutex_timedlock], , [HAVE_PTHREAD_MUTEX_TIMEDLOCK=0],
    [[#include <pthread.h>]])
])
