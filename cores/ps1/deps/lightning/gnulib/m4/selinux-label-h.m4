# Copyright 2020-2021 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# From Paul Eggert
# Provide <selinux/label.h>, if necessary.

AC_DEFUN([gl_HEADERS_SELINUX_LABEL_H],
[
  AC_REQUIRE([gl_LIBSELINUX])
  SELINUX_LABEL_H=selinux/label.h
  if test "$with_selinux" != no; then
    AC_CHECK_HEADERS([selinux/label.h],
                     [SELINUX_LABEL_H=])
  fi
  AC_SUBST([SELINUX_LABEL_H])
  AM_CONDITIONAL([GL_GENERATE_SELINUX_LABEL_H], [test -n "$SELINUX_LABEL_H"])
])
