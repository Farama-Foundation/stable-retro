/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* memory.h:
**  Copyright (C) 2010-2016 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __MDFN_SS_MEMORY_H
#define __MDFN_SS_MEMORY_H

#include "../error.h"

#include "ss_endian.h"

#include <new>
#include <stdlib.h>

// "Array" is a bit of a misnomer, but it helps avoid confusion with memset() semantics hopefully.
static INLINE void MDFN_FastArraySet(uint64* const dst, const uint64 value, const size_t count)
{
 #if defined(__x86_64__) && !defined(_MSC_VER)
 {
  uint32 dummy_output0, dummy_output1;

  asm volatile(
        "cld\n\t"
        "rep stosq\n\t"
        : "=D" (dummy_output0), "=c" (dummy_output1)
        : "a" (value), "D" (dst), "c" (count)
        : "cc", "memory");
 }
 #else

 for(uint64 *ai = dst; MDFN_LIKELY(ai != (dst + count)); ai++)
  MDFN_ennsb<uint64, true>(ai, value);

 #endif
}

static INLINE void MDFN_FastArraySet(uint32* const dst, const uint32 value, const size_t count)
{
 #if defined(__x86_64__) && !defined(_MSC_VER)
 {
  uint32 dummy_output0, dummy_output1;

  asm volatile(
        "cld\n\t"
        "rep stosl\n\t"
        : "=D" (dummy_output0), "=c" (dummy_output1)
        : "a" (value), "D" (dst), "c"(count)
        : "cc", "memory");

  return;
 }
 #else
 if(0 == ((uintptr_t)dst & (sizeof(uint64) - 1)) && !(count & 1))
  MDFN_FastArraySet((uint64*)dst, value | ((uint64)value << 32), count >> 1);
 else
 {
  for(uint32 *ai = dst; MDFN_LIKELY(ai != (dst + count)); ai++)
   MDFN_ennsb<uint32, true>(ai, value);
 }
 #endif
}

static INLINE void MDFN_FastArraySet(uint16* const dst, const uint16 value, const size_t count)
{
 if(0 == ((uintptr_t)dst & (sizeof(uint32) - 1)) && !(count & 1))
  MDFN_FastArraySet((uint32*)dst, value | (value << 16), count >> 1);
 else
 {
  for(uint16 *ai = dst; MDFN_LIKELY(ai != (dst + count)); ai++)
   MDFN_ennsb<uint16, true>(ai, value);
 }
}

static INLINE void MDFN_FastArraySet(uint8* const dst, const uint16 value, const size_t count)
{
 if(0 == ((uintptr_t)dst & (sizeof(uint16) - 1)) && !(count & 1))
  MDFN_FastArraySet((uint16*)dst, value | (value << 8), count >> 1);
 else
 {
  for(uint8 *ai = dst; MDFN_LIKELY(ai != (dst + count)); ai++)
   *ai = value;
 }
}

// memset() replacement that will work on uncachable memory.
//void *MDFN_memset_safe(void *s, int c, size_t n);


#endif
