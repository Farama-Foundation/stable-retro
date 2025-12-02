# threads.m4 serial 7
dnl Copyright (C) 2019-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Tests whether the <threads.h> facility is available.
dnl Sets the variable LIBSTDTHREAD to the linker options for use in a Makefile
dnl for a program that uses the <threads.h> functions.
dnl Sets the variable LIBTHREADLOCAL to the linker options for use in a Makefile
dnl for a program that uses the 'thread_local' macro.
AC_DEFUN([gl_THREADS_H],
[
  AC_REQUIRE([gl_THREADS_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([gl_STDTHREADLIB])

  gl_CHECK_NEXT_HEADERS([threads.h])
  if test $ac_cv_header_threads_h = yes; then
    HAVE_THREADS_H=1
  else
    HAVE_THREADS_H=0
  fi
  AC_SUBST([HAVE_THREADS_H])

  if test $HAVE_THREADS_H = 1; then
    dnl AIX 7.1..7.2 defines thrd_start_t incorrectly, namely as
    dnl 'void * (*) (void *)' instead of 'int (*) (void *)'.
    AC_CACHE_CHECK([whether thrd_start_t is correct],
      [gl_cv_thrd_start_t_correct],
      [AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[
            #include <threads.h>
            #ifdef __cplusplus
            extern "C" {
            #endif
            extern void foo (thrd_start_t);
            extern void foo (int (*) (void *));
            #ifdef __cplusplus
            }
            #endif
          ]], [[]])],
         [gl_cv_thrd_start_t_correct=yes],
         [gl_cv_thrd_start_t_correct=no])
      ])
    if test $gl_cv_thrd_start_t_correct != yes; then
      BROKEN_THRD_START_T=1
    fi
  fi

  dnl Define _Thread_local.
  dnl GCC, for example, supports '__thread' since version 3.3, but it supports
  dnl '_Thread_local' only starting with version 4.9.
  AH_VERBATIM([thread_local], gl_THREAD_LOCAL_DEFINITION)

  dnl Test whether _Thread_local is supported in the compiler and linker.
  AC_CACHE_CHECK([whether _Thread_local works],
    [gl_cv_thread_local_works],
    [dnl On AIX 7.1 with GCC 4.8.1, this test program compiles fine, but the
     dnl 'test-thread-local' test misbehaves.
     dnl On Android 4.3, this test program compiles fine, but the
     dnl 'test-thread-local' test crashes.
     if case "$host_os" in
          aix*) test -n "$GCC" ;;
          linux*-android*) true ;;
          *) false ;;
        esac
     then
       gl_cv_thread_local_works="guessing no"
     else
       AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([gl_THREAD_LOCAL_DEFINITION[
            int _Thread_local x;
          ]], [[
            x = 42;
          ]])],
         [if test "$cross_compiling" != yes; then
            gl_cv_thread_local_works=yes
          else
            gl_cv_thread_local_works="guessing yes"
          fi
         ],
         [if test "$cross_compiling" != yes; then
            gl_cv_thread_local_works=no
          else
            gl_cv_thread_local_works="guessing no"
          fi
         ])
     fi
    ])
  case "$gl_cv_thread_local_works" in
    *yes) ;;
    *) HAVE_THREAD_LOCAL=0 ;;
  esac

  dnl Determine the link dependencies of '_Thread_local'.
  LIBTHREADLOCAL=
  dnl On AIX 7.2 with "xlc -qthreaded -qtls", programs that use _Thread_local
  dnl as defined above produce link errors regarding the symbols
  dnl '.__tls_get_mod' and '__tls_get_addr'. Similarly, on AIX 7.2 with gcc,
  dnl 32-bit programs that use _Thread_local produce link errors regarding the
  dnl symbol '__get_tpointer'. The fix is to link with -lpthread.
  case "$host_os" in
    aix*)
      LIBTHREADLOCAL=-lpthread
      ;;
  esac
  AC_SUBST([LIBTHREADLOCAL])

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use, and which is not
  dnl guaranteed by C89.
  gl_WARN_ON_USE_PREPARE([[#include <threads.h>
    ]], [call_once
    cnd_broadcast cnd_destroy cnd_init cnd_signal cnd_timedwait cnd_wait
    mtx_destroy mtx_init mtx_lock mtx_timedlock mtx_trylock mtx_unlock
    thrd_create thrd_current thrd_detach thrd_equal thrd_exit thrd_join
    thrd_sleep thrd_yield
    tss_create tss_delete tss_get tss_set])
])

dnl Expands to C preprocessor statements that define _Thread_local.
AC_DEFUN([gl_THREAD_LOCAL_DEFINITION],
[[/* The _Thread_local keyword of C11.  */
/* GNU C: <https://gcc.gnu.org/onlinedocs/gcc-3.3.1/gcc/Thread-Local.html> */
/* ARM C: <https://developer.arm.com/docs/dui0472/latest/compiler-specific-features/__declspecthread> */
/* IBM C: supported only with compiler option -qtls, see
   <https://www.ibm.com/support/knowledgecenter/SSGH2K_12.1.0/com.ibm.xlc121.aix.doc/compiler_ref/opt_tls.html> */
/* Oracle Solaris Studio C: <https://docs.oracle.com/cd/E18659_01/html/821-1384/bjabr.html> */
/* MSVC: <https://docs.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls> */
#ifndef _Thread_local
# if defined __GNUC__ || defined __CC_ARM || defined __xlC__ || defined __SUNPRO_C
#  define _Thread_local __thread
# elif defined _MSC_VER
#  define _Thread_local __declspec (thread)
# endif
#endif
]])

AC_DEFUN([gl_THREADS_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_THREADS_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_THREADS_H_DEFAULTS],
[
  GNULIB_CND=0;           AC_SUBST([GNULIB_CND])
  GNULIB_MTX=0;           AC_SUBST([GNULIB_MTX])
  GNULIB_THRD=0;          AC_SUBST([GNULIB_THRD])
  GNULIB_TSS=0;           AC_SUBST([GNULIB_TSS])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_THREAD_LOCAL=1;    AC_SUBST([HAVE_THREAD_LOCAL])
  BROKEN_THRD_START_T=0;  AC_SUBST([BROKEN_THRD_START_T])
  REPLACE_THRD_CREATE=0;  AC_SUBST([REPLACE_THRD_CREATE])
  REPLACE_THRD_CURRENT=0; AC_SUBST([REPLACE_THRD_CURRENT])
  REPLACE_THRD_DETACH=0;  AC_SUBST([REPLACE_THRD_DETACH])
  REPLACE_THRD_EQUAL=0;   AC_SUBST([REPLACE_THRD_EQUAL])
  REPLACE_THRD_JOIN=0;    AC_SUBST([REPLACE_THRD_JOIN])
])
