# minus-zero.m4 serial 2
dnl Copyright (C) 2010-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Macros for floating-point negative zero.
# Keep in sync with tests/minus-zero.h!

# Expands to code that defines a variable or macro minus_zerof.
AC_DEFUN([gl_FLOAT_MINUS_ZERO_CODE],
[[
#include <float.h>
/* minus_zerof represents the value -0.0f.  */
/* HP cc on HP-UX 10.20 has a bug with the constant expression -0.0f.
   ICC 10.0 has a bug when optimizing the expression -zero.
   The expression -FLT_MIN * FLT_MIN does not work when cross-compiling
   to PowerPC on Mac OS X 10.5.  */
#if defined __hpux || defined __sgi || defined __ICC
static float
compute_minus_zerof (void)
{
  return -FLT_MIN * FLT_MIN;
}
# define minus_zerof compute_minus_zerof ()
#else
float minus_zerof = -0.0f;
#endif
]])

# Expands to code that defines a variable or macro minus_zerod.
AC_DEFUN([gl_DOUBLE_MINUS_ZERO_CODE],
[[
#include <float.h>
/* minus_zerod represents the value -0.0.  */
/* HP cc on HP-UX 10.20 has a bug with the constant expression -0.0.
   ICC 10.0 has a bug when optimizing the expression -zero.
   The expression -DBL_MIN * DBL_MIN does not work when cross-compiling
   to PowerPC on Mac OS X 10.5.  */
#if defined __hpux || defined __sgi || defined __ICC
static double
compute_minus_zerod (void)
{
  return -DBL_MIN * DBL_MIN;
}
# define minus_zerod compute_minus_zerod ()
#else
double minus_zerod = -0.0;
#endif
]])

# Expands to code that defines a variable or macro minus_zerol.
AC_DEFUN([gl_LONG_DOUBLE_MINUS_ZERO_CODE],
[[
#include <float.h>
#if defined __i386__ && (defined __BEOS__ || defined __OpenBSD__)
# undef LDBL_MIN
# define LDBL_MIN        3.3621031431120935063E-4932L
#endif
/* minus_zerol represents the value -0.0L.  */
/* HP cc on HP-UX 10.20 has a bug with the constant expression -0.0L.
   IRIX cc can't put -0.0L into .data, but can compute at runtime.
   ICC 10.0 has a bug when optimizing the expression -zero.
   The expression -LDBL_MIN * LDBL_MIN does not work when cross-compiling
   to PowerPC on Mac OS X 10.5.  */
#if defined __hpux || defined __sgi || defined __ICC
static long double
compute_minus_zerol (void)
{
  return -LDBL_MIN * LDBL_MIN;
}
# define minus_zerol compute_minus_zerol ()
#else
long double minus_zerol = -0.0L;
#endif
]])
