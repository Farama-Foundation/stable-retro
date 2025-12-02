# isfinite.m4 serial 17
dnl Copyright (C) 2007-2021 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_ISFINITE],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  AC_REQUIRE([AC_CANONICAL_HOST])
  dnl Persuade glibc <math.h> to declare isfinite.
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_DECLS([isfinite], , , [[#include <math.h>]])
  if test "$ac_cv_have_decl_isfinite" = yes; then
    gl_CHECK_MATH_LIB([ISFINITE_LIBM],
     [x = isfinite (x) + isfinite ((float) x);])
    if test "$ISFINITE_LIBM" != missing; then
      dnl Test whether isfinite() on 'long double' works.
      gl_ISFINITEL_WORKS
      case "$gl_cv_func_isfinitel_works" in
        *yes) ;;
        *)    ISFINITE_LIBM=missing;;
      esac
      dnl Also, isfinite() on 'double' does not work on Linux/ia64 (because of
      dnl signalling NaNs). But this does not have to be tested, since
      dnl isfinite(long double) also does not work in this situation.
    fi
  fi
  dnl On Solaris 10, with CC in C++ mode, isfinite is not available although
  dnl is with cc in C mode. This cannot be worked around by defining
  dnl _XOPEN_SOURCE=600, because the latter does not work in C++ mode on
  dnl Solaris 11.0. Therefore use the replacement functions on Solaris.
  if test "$ac_cv_have_decl_isfinite" != yes \
     || test "$ISFINITE_LIBM" = missing \
     || { case "$host_os" in solaris*) true;; *) false;; esac; }; then
    REPLACE_ISFINITE=1
    dnl No libraries are needed to link lib/isfinite.c.
    ISFINITE_LIBM=
  fi
  AC_SUBST([ISFINITE_LIBM])
])

dnl Test whether isfinite() on 'long double' recognizes all canonical values
dnl which are neither finite nor infinite.
AC_DEFUN([gl_ISFINITEL_WORKS],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gl_BIGENDIAN])
  AC_REQUIRE([gl_LONG_DOUBLE_VS_DOUBLE])
  AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
  AC_CACHE_CHECK([whether isfinite(long double) works], [gl_cv_func_isfinitel_works],
    [
      AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <float.h>
#include <limits.h>
#include <math.h>
#define NWORDS \
  ((sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
typedef union { unsigned int word[NWORDS]; long double value; }
        memory_long_double;
/* On Irix 6.5, gcc 3.4.3 can't compute compile-time NaN, and needs the
   runtime type conversion.  */
#ifdef __sgi
static long double NaNl ()
{
  double zero = 0.0;
  return zero / zero;
}
#else
# define NaNl() (0.0L / 0.0L)
#endif
int main ()
{
  int result = 0;

  {
    memory_long_double m;
    unsigned int i;

    /* The isfinite macro should be immune against changes in the sign bit and
       in the mantissa bits.  The xor operation twiddles a bit that can only be
       a sign bit or a mantissa bit (since the exponent never extends to
       bit 31).  */
    m.value = NaNl ();
    m.word[NWORDS / 2] ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
    for (i = 0; i < NWORDS; i++)
      m.word[i] |= 1;
    if (isfinite (m.value))
      result |= 1;
  }

#if ((defined __ia64 && LDBL_MANT_DIG == 64) || (defined __x86_64__ || defined __amd64__) || (defined __i386 || defined __i386__ || defined _I386 || defined _M_IX86 || defined _X86_)) && !HAVE_SAME_LONG_DOUBLE_AS_DOUBLE
/* Representation of an 80-bit 'long double' as an initializer for a sequence
   of 'unsigned int' words.  */
# ifdef WORDS_BIGENDIAN
#  define LDBL80_WORDS(exponent,manthi,mantlo) \
     { ((unsigned int) (exponent) << 16) | ((unsigned int) (manthi) >> 16), \
       ((unsigned int) (manthi) << 16) | ((unsigned int) (mantlo) >> 16),   \
       (unsigned int) (mantlo) << 16                                        \
     }
# else
#  define LDBL80_WORDS(exponent,manthi,mantlo) \
     { mantlo, manthi, exponent }
# endif
  { /* Quiet NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0xC3333333, 0x00000000) };
    if (isfinite (x.value))
      result |= 2;
  }
  {
    /* Signalling NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x83333333, 0x00000000) };
    if (isfinite (x.value))
      result |= 2;
  }
  /* isfinite should return something even for noncanonical values.  */
  { /* Pseudo-NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x40000001, 0x00000000) };
    if (isfinite (x.value) && !isfinite (x.value))
      result |= 4;
  }
  { /* Pseudo-Infinity.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x00000000, 0x00000000) };
    if (isfinite (x.value) && !isfinite (x.value))
      result |= 8;
  }
  { /* Pseudo-Zero.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4004, 0x00000000, 0x00000000) };
    if (isfinite (x.value) && !isfinite (x.value))
      result |= 16;
  }
  { /* Unnormalized number.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4000, 0x63333333, 0x00000000) };
    if (isfinite (x.value) && !isfinite (x.value))
      result |= 32;
  }
  { /* Pseudo-Denormal.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x0000, 0x83333333, 0x00000000) };
    if (isfinite (x.value) && !isfinite (x.value))
      result |= 64;
  }
#endif

  return result;
}]])],
      [gl_cv_func_isfinitel_works=yes],
      [gl_cv_func_isfinitel_works=no],
      [case "$host_os" in
                 # Guess no on native Windows.
         mingw*) gl_cv_func_isfinitel_works="guessing no" ;;
         *)      gl_cv_func_isfinitel_works="guessing yes" ;;
       esac
      ])
    ])
])
