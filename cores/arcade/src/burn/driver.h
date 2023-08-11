/*
 * For the MAME sound cores
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if !defined (_WIN32)
 #define __cdecl
#endif

#if !defined (_MSC_VER) && defined(FASTCALL)
 #undef __fastcall
 #define __fastcall __attribute__((fastcall))
#endif

#ifndef INLINE
 #define INLINE __inline static
#endif

#define FBA
#define FBANEO

#ifndef M_PI // SDL2 is making this not found
#define M_PI 3.14159265358979323846264338327950288
#endif

typedef unsigned char						UINT8;
typedef signed char 						INT8;
typedef unsigned short						UINT16;
typedef signed short						INT16;
typedef unsigned int						UINT32;
typedef signed int							INT32;
#ifdef _MSC_VER
typedef signed __int64						INT64;
typedef unsigned __int64					UINT64;
#else
__extension__ typedef unsigned long long	UINT64;
__extension__ typedef long long				INT64;
#endif

// Macro to determine the size of a struct up to and including "member"
#define STRUCT_SIZE_HELPER(type, member) offsetof(type, member) + sizeof(((type*)0)->member)

#define OSD_CPU_H

/* U64 and S64 are used to wrap long integer constants. */
#if defined(__GNUC__) || defined(_MSC_VER)
#define U64(val) val##ULL
#define S64(val) val##LL
#else
#define U64(val) val
#define S64(val) val
#endif

/* OPN */
#define HAS_YM2203  1
#define HAS_YM2608  1
#define HAS_YM2610  1
#define HAS_YM2610B 1
#define HAS_YM2612  1
#define HAS_YM3438  1
/* OPL */
#define HAS_YM3812  1
#define HAS_YM3526  1
#define HAS_Y8950   1

enum {
	CLEAR_LINE = 0,
	ASSERT_LINE,
	HOLD_LINE,
	PULSE_LINE
};

#define timer_get_time() BurnTimerGetTime()

#define READ8_HANDLER(name) 	UINT8 name(void)
#define WRITE8_HANDLER(name) 	void  name(UINT8 data)

#ifdef __cplusplus
 extern "C" {
#endif
  double BurnTimerGetTime(void);

  typedef UINT8 (*read8_handler)(UINT32 offset);
  typedef void (*write8_handler)(UINT32 offset, UINT32 data);

 #ifdef MAME_USE_LOGERROR
  void __cdecl logerror(char* szFormat, ...);
 #else
  #define logerror
 #endif
#ifdef __cplusplus
 }
#endif

#endif /* DRIVER_H */
