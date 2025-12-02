# idpriv.m4 serial 1
dnl Copyright (C) 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_IDPRIV],
[
  dnl Persuade glibc <unistd.h> to declare {get,set}res{uid,gid}.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_FUNCS_ONCE([getuid geteuid getresuid getgid getegid getresgid])
  AC_CHECK_FUNCS_ONCE([setresuid setreuid seteuid setresgid setregid setegid])
])
