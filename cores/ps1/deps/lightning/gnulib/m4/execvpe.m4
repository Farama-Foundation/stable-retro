# execvpe.m4 serial 1
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_EXECVPE],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])

  dnl Persuade glibc <unistd.h> to declare execvpe().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  case "$host_os" in
    mingw*) REPLACE_EXECVPE=1 ;;
    *)
      AC_CHECK_FUNCS([execvpe])
      if test $ac_cv_func_execvpe != yes; then
        HAVE_EXECVPE=0
      fi
      ;;
  esac
])
