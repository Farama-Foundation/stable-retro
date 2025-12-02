#ifndef __ARM_FEATURES_H__
#define __ARM_FEATURES_H__

/* note: features only available since:
 * __ARM_ARCH     gcc 4.8/clang 3.2
 * ARMv8 support  gcc 4.8/clang 3.4
 * ARM64 support  gcc 4.8/clang 3.5
 */

#if defined(__aarch64__)

#elif (defined(__ARM_ARCH) && __ARM_ARCH >= 8)

#define HAVE_ARMV8
#define HAVE_ARMV7
#define HAVE_ARMV6
#define HAVE_ARMV5

#elif (defined(__ARM_ARCH) && __ARM_ARCH >= 7) \
    || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) \
    || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7S__)

#define HAVE_ARMV7
#define HAVE_ARMV6
#define HAVE_ARMV5

#elif (defined(__ARM_ARCH) && __ARM_ARCH >= 6) \
    || defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) \
    || defined(__ARM_ARCH_6M__)

#define HAVE_ARMV6
#define HAVE_ARMV5
#define HAVE_PRE_ARMV7

#elif defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5E__) \
   || defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__)

#define HAVE_ARMV5
#define HAVE_PRE_ARMV7

#elif defined(__arm__)

#define HAVE_PRE_ARMV7

#endif

/* gcc defines __ARM_NEON__ consistently for 32bit, but apple clang defines it for 64bit also... */
#if defined(HAVE_ARMV7) && defined(__ARM_NEON__)
#define HAVE_NEON32
#endif

#if defined(__APPLE__) && defined(__aarch64__)
#define ASM_SEPARATOR %%
#else
#define ASM_SEPARATOR ;
#endif

/* global function/external symbol */
#ifndef __APPLE__
#define ESYM(name) name

#define FUNCTION(name) \
  .globl name; \
  .type name, %function; \
  name

#define ESIZE(name_, size_) \
  .size name_, size_

#define EOBJECT(name_) \
  .type name_, %object

#define EXTRA_UNSAVED_REGS

#else
#define ESYM(name) _##name

#define FUNCTION(name) \
  name: ASM_SEPARATOR \
  .globl ESYM(name) ASM_SEPARATOR \
  ESYM(name)

#define ESIZE(name_, size_)
#define EOBJECT(name_)

// r7 is preserved, but add it for EABI alignment..
#define EXTRA_UNSAVED_REGS r7, r9,

#endif

#if defined(__MACH__) || defined(__PIC__)
#define TEXRELS_FORBIDDEN
#endif

#endif /* __ARM_FEATURES_H__ */
