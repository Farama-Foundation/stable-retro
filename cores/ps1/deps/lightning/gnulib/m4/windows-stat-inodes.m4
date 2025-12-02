# windows-stat-inodes.m4 serial 1
dnl Copyright (C) 2017-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Enable inode identification in 'struct stat' on native Windows platforms.
dnl Set WINDOWS_STAT_INODES to
dnl - 0 -> keep the default (dev_t = 32-bit, ino_t = 16-bit),
dnl - 1 -> override types normally (dev_t = 32-bit, ino_t = 64-bit),
dnl - 2 -> override types in an extended way (dev_t = 64-bit, ino_t = 128-bit).
AC_DEFUN([gl_WINDOWS_STAT_INODES],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  case "$host_os" in
    mingw*) WINDOWS_STAT_INODES=1 ;;
    *)      WINDOWS_STAT_INODES=0 ;;
  esac
])
