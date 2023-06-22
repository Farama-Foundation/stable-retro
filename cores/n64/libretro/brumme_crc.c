/*
 * Stephan Brumme's CRC32 implementation
 *
 * This file has been truncated because the original contents contain many
 * implementations and a test programs that we are not interested in.
 *
 *
 * crc32_4bytes and crc32_16bytes are based/derived from Intel's
 * Slicing-by-4 algorithm available at http://sourceforge.net/projects/slicing-by-8/
 *
 * See http://create.stephan-brumme.com/crc32/ for more information.
 *
 * -------------------------------
 *
 * LICENSE (retrieved in 2015-05-24)
 *
 * All source code published on http://create.stephan-brumme.com and its
 * sub-pages is licensed similar to the zlib license:
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the author be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *     The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software.
 *
 *     If you use this software in a product, an acknowledgment in the product
 *     documentation would be appreciated but is not required.
 *
 *     Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 * If you like / hate / ignore my software, send me an email or, even better, a
 * nice postcard. Thank you ! :)
 *
 * -------------------------------
 *
 * Author note:
 *    if running on an embedded system, you might consider shrinking the
 *    big Crc32Lookup table:
 *    - crc32_4bytes   needs only Crc32Lookup[0..3]
 *    - crc32_16bytes  needs all of Crc32Lookup
 */

#include <stddef.h>
#include <stdlib.h>

#include <retro_inline.h>
#include <boolean.h>

#include <encodings/crc32.h>

#ifdef _MSC_VER
typedef unsigned __int8  uint8_t;
typedef unsigned __int32 uint32_t;
typedef   signed __int32 int32_t;
#else
#include <stdint.h>
#endif

/// zlib's CRC32 polynomial
//static const uint32_t Polynomial = 0xEDB88320;
static const uint32_t Polynomial = 0x04C11DB7; /* same as libretro_crc.c */

#define MaxSlice 16
static uint32_t Crc32Lookup[MaxSlice][256];
static bool     table_initialized = false;

static INLINE uint32_t swap(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
   return __builtin_bswap32(x);
#else
#if defined(_MSC_VER)
   return _byteswap_ulong(x);
#else
   return (x >> 24) |
         ((x >>  8) & 0x0000FF00) |
         ((x <<  8) & 0x00FF0000) |
         (x << 24);
#endif
#endif
}

static uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32);
static uint32_t crc32_4bytes(const void* data, size_t length, uint32_t previousCrc32);
static uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32);

void CRC_BuildTable(void)
{
   int i, j, slice;

   if (table_initialized)
      return;

   table_initialized = true;

   for (i = 0; i <= 0xFF; i++)
   {
     uint32_t crc = i;
     for (j = 0; j < 8; j++)
       crc = (crc >> 1) ^ ((crc & 1) * Polynomial);
     Crc32Lookup[0][i] = crc;
   }

   for (slice = 1; slice < MaxSlice; slice++)
      for (i = 0; i <= 0xFF; i++)
         Crc32Lookup[slice][i] = (Crc32Lookup[slice - 1][i] >> 8) ^ Crc32Lookup[0][Crc32Lookup[slice - 1][i] & 0xFF];
}

/// compute CRC32 (standard algorithm)
static uint32_t crc32_1byte(const void* data, size_t length, uint32_t previousCrc32)
{
   uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
   const uint8_t* current = (const uint8_t*) data;

   while (length-- > 0)
      crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *current++];

   return ~crc; // same as crc ^ 0xFFFFFFFF
}

/// compute CRC32 (Slicing-by-4 algorithm)
static uint32_t crc32_4bytes(const void* data, size_t length, uint32_t previousCrc32)
{
   const uint8_t *currentChar;
   uint32_t  crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
   const uint32_t* current = (const uint32_t*) data;

   // process four bytes at once (Slicing-by-4)
   while (length >= 4)
   {
#ifdef MSB_FIRST
      uint32_t one = *current++ ^ swap(crc);
      crc = Crc32Lookup[0][ one      & 0xFF] ^
            Crc32Lookup[1][(one>> 8) & 0xFF] ^
            Crc32Lookup[2][(one>>16) & 0xFF] ^
            Crc32Lookup[3][(one>>24) & 0xFF];
#else
      uint32_t one = *current++ ^ crc;
      crc = Crc32Lookup[0][(one>>24) & 0xFF] ^
            Crc32Lookup[1][(one>>16) & 0xFF] ^
            Crc32Lookup[2][(one>> 8) & 0xFF] ^
            Crc32Lookup[3][ one      & 0xFF];
#endif

      length -= 4;
   }

   currentChar = (const uint8_t*) current;
   // remaining 1 to 3 bytes (standard algorithm)
   while (length-- != 0)
      crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

   return ~crc; // same as crc ^ 0xFFFFFFFF
}

/// compute CRC32 (Slicing-by-16 algorithm)
static uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32)
{
   uint32_t crc = ~previousCrc32; // same as previousCrc32 ^ 0xFFFFFFFF
   const uint32_t* current = (const uint32_t*) data;
   const uint8_t* currentChar = NULL;
   // enabling optimization (at least -O2) automatically unrolls the inner for-loop
   const size_t Unroll = 4;
   const size_t BytesAtOnce = 16 * Unroll;

   while (length >= BytesAtOnce)
   {
      size_t unrolling;
      for (unrolling = 0; unrolling < Unroll; unrolling++)
      {
#ifdef MSB_FIRST
         uint32_t one   = *current++ ^ swap(crc);
         uint32_t two   = *current++;
         uint32_t three = *current++;
         uint32_t four  = *current++;
         crc  = Crc32Lookup[ 0][ four         & 0xFF] ^
               Crc32Lookup[ 1][(four  >>  8) & 0xFF] ^
               Crc32Lookup[ 2][(four  >> 16) & 0xFF] ^
               Crc32Lookup[ 3][(four  >> 24) & 0xFF] ^
               Crc32Lookup[ 4][ three        & 0xFF] ^
               Crc32Lookup[ 5][(three >>  8) & 0xFF] ^
               Crc32Lookup[ 6][(three >> 16) & 0xFF] ^
               Crc32Lookup[ 7][(three >> 24) & 0xFF] ^
               Crc32Lookup[ 8][ two          & 0xFF] ^
               Crc32Lookup[ 9][(two   >>  8) & 0xFF] ^
               Crc32Lookup[10][(two   >> 16) & 0xFF] ^
               Crc32Lookup[11][(two   >> 24) & 0xFF] ^
               Crc32Lookup[12][ one          & 0xFF] ^
               Crc32Lookup[13][(one   >>  8) & 0xFF] ^
               Crc32Lookup[14][(one   >> 16) & 0xFF] ^
               Crc32Lookup[15][(one   >> 24) & 0xFF];
#else
         uint32_t one   = *current++ ^ crc;
         uint32_t two   = *current++;
         uint32_t three = *current++;
         uint32_t four  = *current++;
         crc  = Crc32Lookup[ 0][(four  >> 24) & 0xFF] ^
               Crc32Lookup[ 1][(four  >> 16) & 0xFF] ^
               Crc32Lookup[ 2][(four  >>  8) & 0xFF] ^
               Crc32Lookup[ 3][ four         & 0xFF] ^
               Crc32Lookup[ 4][(three >> 24) & 0xFF] ^
               Crc32Lookup[ 5][(three >> 16) & 0xFF] ^
               Crc32Lookup[ 6][(three >>  8) & 0xFF] ^
               Crc32Lookup[ 7][ three        & 0xFF] ^
               Crc32Lookup[ 8][(two   >> 24) & 0xFF] ^
               Crc32Lookup[ 9][(two   >> 16) & 0xFF] ^
               Crc32Lookup[10][(two   >>  8) & 0xFF] ^
               Crc32Lookup[11][ two          & 0xFF] ^
               Crc32Lookup[12][(one   >> 24) & 0xFF] ^
               Crc32Lookup[13][(one   >> 16) & 0xFF] ^
               Crc32Lookup[14][(one   >>  8) & 0xFF] ^
               Crc32Lookup[15][ one          & 0xFF];
#endif
      }

      length -= BytesAtOnce;
   }

   currentChar = (const uint8_t*) current;
   // remaining 1 to 63 bytes (standard algorithm)
   while (length-- != 0)
      crc = (crc >> 8) ^ Crc32Lookup[0][(crc & 0xFF) ^ *currentChar++];

   return ~crc; // same as crc ^ 0xFFFFFFFF
}
