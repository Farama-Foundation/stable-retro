# malloc-align.m4 serial 1
dnl Copyright (C) 2020-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Defines a C macro MALLOC_ALIGNMENT, whose value is a numeric constant,
dnl a power of 2, with the property that
dnl   (uintptr_t) malloc (N)
dnl is always guaranteed to be a multiple of MALLOC_ALIGNMENT.

AC_DEFUN([gl_MALLOC_ALIGNMENT],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CACHE_CHECK([for the malloc() alignment],
    [gl_cv_malloc_alignment],
    [gl_cv_malloc_alignment=
     AC_RUN_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <stdio.h>
            #include <stdlib.h>
            #if defined _WIN32 && !defined __CYGWIN__
            # include <inttypes.h>
            /* uintptr_t is equivalent to 'unsigned long long' if _WIN64,
               or to 'unsigned long' otherwise.  */
            #else
            # undef uintptr_t
            # define uintptr_t unsigned long
            #endif
          ]],
          [[FILE *fp = fopen ("conftest.out", "w");
            if (fp == NULL)
              return 1;
            {
              uintptr_t bits = 0;
              bits |= (uintptr_t) malloc (1);
              bits |= (uintptr_t) malloc (1);
              bits |= (uintptr_t) malloc (1);
              bits |= (uintptr_t) malloc (2);
              bits |= (uintptr_t) malloc (2);
              bits |= (uintptr_t) malloc (2);
              bits |= (uintptr_t) malloc (3);
              bits |= (uintptr_t) malloc (3);
              bits |= (uintptr_t) malloc (3);
              bits |= (uintptr_t) malloc (5);
              bits |= (uintptr_t) malloc (8);
              bits |= (uintptr_t) malloc (8);
              bits |= (uintptr_t) malloc (13);
              bits |= (uintptr_t) malloc (13);
              bits |= (uintptr_t) malloc (19);
              bits |= (uintptr_t) malloc (19);
              bits |= (uintptr_t) malloc (28);
              bits |= (uintptr_t) malloc (28);
              bits |= (uintptr_t) malloc (37);
              bits |= (uintptr_t) malloc (37);
              bits |= (uintptr_t) malloc (73);
              bits |= (uintptr_t) malloc (73);
              bits |= (uintptr_t) malloc (117);
              bits |= (uintptr_t) malloc (117);
              bits |= (uintptr_t) malloc (351);
              bits |= (uintptr_t) malloc (351);
              bits |= (uintptr_t) malloc (914);
              bits |= (uintptr_t) malloc (914);
              bits |= (uintptr_t) malloc (1712);
              bits |= (uintptr_t) malloc (1712);
              bits |= (uintptr_t) malloc (4021);
              bits |= (uintptr_t) malloc (4021);
              bits |= (uintptr_t) malloc (7641);
              bits |= (uintptr_t) malloc (7641);
              bits |= (uintptr_t) malloc (17027);
              bits |= (uintptr_t) malloc (17027);
              bits |= (uintptr_t) malloc (81231);
              bits |= (uintptr_t) malloc (81231);
              fprintf (fp, "%u\n", (unsigned int) (((bits ^ (bits - 1)) + 1) >> 1));
            }
            if (fclose (fp) != 0)
              return 2;
            return 0;
          ]])
       ],
       [gl_cv_malloc_alignment=`cat conftest.out`],
       [gl_cv_malloc_alignment="unknown"],
       [dnl When cross-compiling, guess a value. Note that it's OK to return
        dnl a smaller value (e.g. 4 instead of 8 or 16).
        gl_cv_malloc_alignment="unknown"
        case "$host_os" in
          linux* | mingw*)
            dnl On Linux:
            dnl - It's 8 on most 32-bit platforms, except 16 on x86_64-x32 and
            dnl   (with newer versions of glibc) on i386 and powerpc.  8 is a
            dnl   safe guess.
            dnl - It's 16 on all 64-bit platforms.
            dnl On Windows: It's 8 on 32-bit Windows, 16 on 64-bit Windows.
            for nn in 4 8 16 32; do
              AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM([[
                   #define MALLOC_ALIGN (2 * sizeof (void *))
                   int test [MALLOC_ALIGN <= $nn ? 1 : -1];
                   ]])
                ],
                [gl_cv_malloc_alignment="guessing $nn"
                 break
                ],
                [:])
            done
            ;;
          *)
            dnl If we don't know, assume the worst.
            dnl This minimum is e.g. reached on NetBSD/i386 and NetBSD/sparc.
            for nn in 4 8 16; do
              AC_COMPILE_IFELSE(
                [AC_LANG_PROGRAM([[
                   #define MALLOC_ALIGN (sizeof (void *))
                   int test [MALLOC_ALIGN <= $nn ? 1 : -1];
                   ]])
                ],
                [gl_cv_malloc_alignment="guessing $nn"
                 break
                ],
                [:])
            done
            ;;
        esac
       ])
    ])
  case "$gl_cv_malloc_alignment" in
    "unknown")
      dnl Assume the worst.
      value=4
    ;;
    "guessing "*)
      value=`echo "$gl_cv_malloc_alignment" | sed -e 's/guessing //'`
      ;;
    *)
      value="$gl_cv_malloc_alignment"
      ;;
  esac
  AC_DEFINE_UNQUOTED([MALLOC_ALIGNMENT], [$value],
    [Define to the guaranteed alignment of malloc() return values.])
])
