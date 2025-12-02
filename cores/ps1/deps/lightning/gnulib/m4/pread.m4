# pread.m4 serial 6
dnl Copyright (C) 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_PREAD],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade glibc <unistd.h> to declare pread().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([pread])
  if test $ac_cv_func_pread = yes; then
    dnl On HP-UX 11.11 with _FILE_OFFSET_BITS=64, pread() on a file returns 0
    dnl instead of a positive value.
    dnl On HP-UX 11.31, pread() on a pipe does not fail.
    AC_CACHE_CHECK([whether pread works],
      [gl_cv_func_pread_works],
      [
        dnl Initial guess, used when cross-compiling.
changequote(,)dnl
        case "$host_os" in
                 # Guess no on HP-UX.
          hpux*) gl_cv_func_pread_works="guessing no" ;;
                 # Guess yes otherwise.
          *)     gl_cv_func_pread_works="guessing yes" ;;
        esac
changequote([,])dnl
        gl_save_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
        AC_RUN_IFELSE(
          [AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
             ]GL_MDA_DEFINES],
             [[
{
  int result = 0;
  /* This test fails on HP-UX 11.00..11.11.  */
  {
    int fd = open ("conftest.c", O_RDONLY);
    if (fd < 0)
      result |= 1;
    else
      {
        char byte_buf;
        if (pread (fd, &byte_buf, 1, (off_t) 0) != 1)
          result |= 2;
        close (fd);
      }
  }
  /* This test fails on HP-UX 11.00..11.31.  */
  {
    FILE *fp = popen ("echo", "r");
    if (fp == NULL)
      result |= 1;
    else
      {
        int fd = fileno (fp);
        char byte_buf;
        if (pread (fd, &byte_buf, 1, (off_t) 0) >= 0)
          result |= 4;
        pclose (fp);
      }
  }
  return result;
}]])],
          [gl_cv_func_pread_works=yes],
          [gl_cv_func_pread_works=no],
          [:])
        CPPFLAGS="$gl_save_CPPFLAGS"
      ])
    case "$gl_cv_func_pread_works" in
      *yes) ;;
      *) REPLACE_PREAD=1 ;;
    esac
  else
    HAVE_PREAD=0
  fi
])
