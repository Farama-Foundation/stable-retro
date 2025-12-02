/* Test of isfinite() substitute.
   Copyright (C) 2007-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff, 2007, using Bruno Haible's code as a
   template. */

#include <config.h>

#include <math.h>

/* isfinite must be a macro.  */
#ifndef isfinite
# error missing declaration
#endif

#include <float.h>
#include <limits.h>

#include "infinity.h"
#include "macros.h"

float zerof = 0.0f;
double zerod = 0.0;
long double zerol = 0.0L;

static void
test_isfinitef ()
{
  /* Zero. */
  ASSERT (isfinite (0.0f));
  /* Subnormal values. */
  ASSERT (isfinite (FLT_MIN / 2));
  ASSERT (isfinite (-FLT_MIN / 2));
  /* Finite values.  */
  ASSERT (isfinite (3.141f));
  ASSERT (isfinite (3.141e30f));
  ASSERT (isfinite (3.141e-30f));
  ASSERT (isfinite (-2.718f));
  ASSERT (isfinite (-2.718e30f));
  ASSERT (isfinite (-2.718e-30f));
  /* Infinite values.  */
  ASSERT (!isfinite (Infinityf ()));
  ASSERT (!isfinite (- Infinityf ()));
  /* Quiet NaN.  */
  ASSERT (!isfinite (zerof / zerof));
#if defined FLT_EXPBIT0_WORD && defined FLT_EXPBIT0_BIT
  /* Signalling NaN.  */
  {
    #define NWORDS \
      ((sizeof (float) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
    typedef union { float value; unsigned int word[NWORDS]; } memory_float;
    memory_float m;
    m.value = zerof / zerof;
# if FLT_EXPBIT0_BIT > 0
    m.word[FLT_EXPBIT0_WORD] ^= (unsigned int) 1 << (FLT_EXPBIT0_BIT - 1);
# else
    m.word[FLT_EXPBIT0_WORD + (FLT_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    if (FLT_EXPBIT0_WORD < NWORDS / 2)
      m.word[FLT_EXPBIT0_WORD + 1] |= (unsigned int) 1 << FLT_EXPBIT0_BIT;
    else
      m.word[0] |= (unsigned int) 1;
    ASSERT (!isfinite (m.value));
    #undef NWORDS
  }
#endif
}

static void
test_isfinited ()
{
  /* Zero. */
  ASSERT (isfinite (0.0));
  /* Subnormal values. */
  ASSERT (isfinite (DBL_MIN / 2));
  ASSERT (isfinite (-DBL_MIN / 2));
  /* Finite values. */
  ASSERT (isfinite (3.141));
  ASSERT (isfinite (3.141e30));
  ASSERT (isfinite (3.141e-30));
  ASSERT (isfinite (-2.718));
  ASSERT (isfinite (-2.718e30));
  ASSERT (isfinite (-2.718e-30));
  /* Infinite values.  */
  ASSERT (!isfinite (Infinityd ()));
  ASSERT (!isfinite (- Infinityd ()));
  /* Quiet NaN.  */
  ASSERT (!isfinite (zerod / zerod));
#if defined DBL_EXPBIT0_WORD && defined DBL_EXPBIT0_BIT
  /* Signalling NaN.  */
  {
    #define NWORDS \
      ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
    typedef union { double value; unsigned int word[NWORDS]; } memory_double;
    memory_double m;
    m.value = zerod / zerod;
# if DBL_EXPBIT0_BIT > 0
    m.word[DBL_EXPBIT0_WORD] ^= (unsigned int) 1 << (DBL_EXPBIT0_BIT - 1);
# else
    m.word[DBL_EXPBIT0_WORD + (DBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    m.word[DBL_EXPBIT0_WORD + (DBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      |= (unsigned int) 1 << DBL_EXPBIT0_BIT;
    ASSERT (!isfinite (m.value));
    #undef NWORDS
  }
#endif
}

static void
test_isfinitel ()
{
  #define NWORDS \
    ((sizeof (long double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
  typedef union { unsigned int word[NWORDS]; long double value; }
          memory_long_double;

  /* Zero. */
  ASSERT (isfinite (0.0L));
  /* Subnormal values. */
  ASSERT (isfinite (LDBL_MIN / 2));
  ASSERT (isfinite (-LDBL_MIN / 2));
  /* Finite values. */
  ASSERT (isfinite (3.141L));
  ASSERT (isfinite (3.141e30L));
  ASSERT (isfinite (3.141e-30L));
  ASSERT (isfinite (-2.718L));
  ASSERT (isfinite (-2.718e30L));
  ASSERT (isfinite (-2.718e-30L));
  /* Infinite values.  */
  ASSERT (!isfinite (Infinityl ()));
  ASSERT (!isfinite (- Infinityl ()));
  /* Quiet NaN.  */
  ASSERT (!isfinite (zerol / zerol));

#if defined LDBL_EXPBIT0_WORD && defined LDBL_EXPBIT0_BIT
  /* A bit pattern that is different from a Quiet NaN.  With a bit of luck,
     it's a Signalling NaN.  */
  {
#if defined __powerpc__ && LDBL_MANT_DIG == 106
    /* This is PowerPC "double double", a pair of two doubles.  Inf and Nan are
       represented as the corresponding 64-bit IEEE values in the first double;
       the second is ignored.  Manipulate only the first double.  */
    #undef NWORDS
    #define NWORDS \
      ((sizeof (double) + sizeof (unsigned int) - 1) / sizeof (unsigned int))
#endif

    memory_long_double m;
    m.value = zerol / zerol;
# if LDBL_EXPBIT0_BIT > 0
    m.word[LDBL_EXPBIT0_WORD] ^= (unsigned int) 1 << (LDBL_EXPBIT0_BIT - 1);
# else
    m.word[LDBL_EXPBIT0_WORD + (LDBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      ^= (unsigned int) 1 << (sizeof (unsigned int) * CHAR_BIT - 1);
# endif
    m.word[LDBL_EXPBIT0_WORD + (LDBL_EXPBIT0_WORD < NWORDS / 2 ? 1 : - 1)]
      |= (unsigned int) 1 << LDBL_EXPBIT0_BIT;
    ASSERT (!isfinite (m.value));
  }
#endif

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
    ASSERT (!isfinite (x.value));
  }
  {
    /* Signalling NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x83333333, 0x00000000) };
    ASSERT (!isfinite (x.value));
  }
  /* isfinite should return something for noncanonical values.  */
  { /* Pseudo-NaN.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x40000001, 0x00000000) };
    ASSERT (isfinite (x.value) || !isfinite (x.value));
  }
  { /* Pseudo-Infinity.  */
    static memory_long_double x =
      { LDBL80_WORDS (0xFFFF, 0x00000000, 0x00000000) };
    ASSERT (isfinite (x.value) || !isfinite (x.value));
  }
  { /* Pseudo-Zero.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4004, 0x00000000, 0x00000000) };
    ASSERT (isfinite (x.value) || !isfinite (x.value));
  }
  { /* Unnormalized number.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x4000, 0x63333333, 0x00000000) };
    ASSERT (isfinite (x.value) || !isfinite (x.value));
  }
  { /* Pseudo-Denormal.  */
    static memory_long_double x =
      { LDBL80_WORDS (0x0000, 0x83333333, 0x00000000) };
    ASSERT (isfinite (x.value) || !isfinite (x.value));
  }
#endif

  #undef NWORDS
}

int
main ()
{
  test_isfinitef ();
  test_isfinited ();
  test_isfinitel ();
  return 0;
}
