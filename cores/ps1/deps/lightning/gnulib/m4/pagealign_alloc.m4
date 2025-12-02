#serial 6
dnl Copyright (C) 2005-2007, 2009-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PAGEALIGN_ALLOC],
[
  dnl Persuade glibc <sys/mman.h> to define MAP_ANONYMOUS.
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  gl_PREREQ_PAGEALIGN_ALLOC
])

# Prerequisites of lib/pagealign_alloc.c.
AC_DEFUN([gl_PREREQ_PAGEALIGN_ALLOC],
[
  AC_REQUIRE([gl_FUNC_MMAP_ANON])
  AC_CHECK_FUNCS_ONCE([posix_memalign])
  AC_CHECK_HEADERS_ONCE([unistd.h])
])
