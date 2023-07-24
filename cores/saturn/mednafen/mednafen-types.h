#ifndef __MDFN_TYPES
#define __MDFN_TYPES

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
#include <type_traits>
#endif

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#include <retro_inline.h>

#ifdef __GNUC__
#define MDFN_UNLIKELY(n) __builtin_expect((n) != 0, 0)
#define MDFN_LIKELY(n) __builtin_expect((n) != 0, 1)

  #define NO_INLINE __attribute__((noinline))

  #if defined(__386__) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(_M_I386)
    #define MDFN_FASTCALL __attribute__((fastcall))
  #else
    #define MDFN_FASTCALL
  #endif

  #define MDFN_ALIGN(n)	__attribute__ ((aligned (n)))
  #define MDFN_FORMATSTR(a,b,c) __attribute__ ((format (a, b, c)));
  #define MDFN_WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
  #define MDFN_NOWARN_UNUSED __attribute__((unused))

#elif defined(_MSC_VER)
  #define NO_INLINE
#define MDFN_LIKELY(n) ((n) != 0)
#define MDFN_UNLIKELY(n) ((n) != 0)

  #define MDFN_FASTCALL

  //#define MDFN_ALIGN(n) __declspec(align(n))
#define MDFN_ALIGN(n)

  #define MDFN_FORMATSTR(a,b,c)

  #define MDFN_WARN_UNUSED_RESULT
  #define MDFN_NOWARN_UNUSED

#else
  #error "Not compiling with GCC nor MSVC"
  #define NO_INLINE

  #define MDFN_FASTCALL

  #define MDFN_ALIGN(n)

  #define MDFN_FORMATSTR(a,b,c)

  #define MDFN_WARN_UNUSED_RESULT

#endif


typedef struct
{
 union
 {
  struct
  {
   #ifdef MSB_FIRST
   uint8   High;
   uint8   Low;
   #else
   uint8   Low;
   uint8   High;
   #endif
  } Union8;
  uint16 Val16;
 };
} Uuint16;

typedef struct
{
 union
 {
  struct
  {
   #ifdef MSB_FIRST
   Uuint16   High;
   Uuint16   Low;
   #else
   Uuint16   Low;
   Uuint16   High;
   #endif
  } Union16;
  uint32  Val32;
 };
} Uuint32;

#define MDFN_HOT
#define MDFN_COLD
#define MDFN_HIDE
#define NO_CLONE

#ifdef __cplusplus
template<typename T> typename std::remove_all_extents<T>::type* MDAP(T* v) { return (typename std::remove_all_extents<T>::type*)v; }
#include "error.h"
#endif

#endif
