# pwrite.m4 serial 6
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_PWRITE],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles

  dnl Persuade glibc <unistd.h> to declare pwrite().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([pwrite])
  if test $ac_cv_func_pwrite = yes; then
    dnl On HP-UX 11.11 with _FILE_OFFSET_BITS=64, pwrite() on a file does not
    dnl fail when an invalid (negative) offset is passed and uses an arbitrary
    dnl offset instead of the argument.
    AC_CACHE_CHECK([whether pwrite works],
      [gl_cv_func_pwrite_works],
      [
        dnl Initial guess, used when cross-compiling.
changequote(,)dnl
        case "$host_os" in
                 # Guess no on HP-UX.
          hpux*) gl_cv_func_pwrite_works="guessing no" ;;
                 # Guess yes otherwise.
          *)     gl_cv_func_pwrite_works="guessing yes" ;;
        esac
changequote([,])dnl
        gl_save_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64"
        rm -f conftest.out
        AC_RUN_IFELSE(
          [AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
             ]GL_MDA_DEFINES],
             [[
{
  /* This test fails on HP-UX 11.00..11.11.  */
  {
    int fd = open ("conftest.out", O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
      return 1;
    if (pwrite (fd, "b", 1, (off_t) -1) >= 0)
      return 2;
  }
  /* This test fails on HP-UX 11.00..11.11.  */
  {
    int fd;
    char buf[] = "01";

    fd = open ("conftest.out", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
      return 3;
    if (write (fd, buf, 2) < 2)
      return 4;
    if (close (fd) < 0)
      return 5;

    fd = open ("conftest.out", O_WRONLY, 0600);
    if (fd < 0)
      return 6;
    if (pwrite (fd, "W", 1, (off_t) 0) != 1)
      return 7;
    if (close (fd) < 0)
      return 8;

    fd = open ("conftest.out", O_RDONLY);
    if (fd < 0)
      return 9;
    if (read (fd, buf, 2) < 2)
      return 10;
    if (close (fd) < 0)
      return 11;
    if (buf[0] != 'W')
      return 12;
    if (buf[1] != '1')
      return 13;
  }
  return 0;
}]])],
          [gl_cv_func_pwrite_works=yes],
          [gl_cv_func_pwrite_works=no],
          [:])
        rm -f conftest.out
        CPPFLAGS="$gl_save_CPPFLAGS"
      ])
    case "$gl_cv_func_pwrite_works" in
      *yes) ;;
      *) REPLACE_PWRITE=1 ;;
    esac
  else
    HAVE_PWRITE=0
  fi
])
