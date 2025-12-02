/*
 * Copyright (C) 2011 Gilead Kutnick "Exophase" <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef VECTOR_OPS
#define VECTOR_OPS

#include "vector_types.h"


#define foreach_element(iterations, operation)                                 \
{                                                                              \
  u32 _i;                                                                      \
  for(_i = 0; _i < iterations; _i++)                                           \
  {                                                                            \
    operation;                                                                 \
  }                                                                            \
}                                                                              \

#define load_64b(dest, source)                                                 \
 *((u64 *)(dest).e) = *((u64 *)(source))                                       \

#define load_128b(dest, source)                                                \
 *((u64 *)(dest).e) = *((u64 *)(source));                                      \
 *((u64 *)(dest).e + 1) = *(((u64 *)(source)) + 1)                             \

#define load_8x16b(dest, source)                                               \
  foreach_element(8, (dest).e[_i] = ((u16 *)(source))[_i])                     \

#define store_64b(source, dest)                                                \
 *((u64 *)(dest)) = *((u64 *)(source).e)                                       \

#define store_128b(source, dest)                                               \
 *((u64 *)(dest)) = *((u64 *)(source).e);                                      \
 *(((u64 *)(dest)) + 1) = *((u64 *)(source).e + 1)                             \

#define store_8x16b(source, dest)                                              \
  foreach_element(8, ((u16 *)dest)[_i] = (source).e[_i])                       \


#define split_8x16b(dest, source)                                              \
  foreach_element(8,                                                           \
  {                                                                            \
    (dest).e[_i * 2] = (source).e[_i];                                         \
    (dest).e[(_i * 2) + 1] = (source).e[_i] >> 8;                              \
  })                                                                           \

#define merge_16x8b(dest, source)                                              \
  foreach_element(8,                                                           \
    (dest).e[_i] = (source).e[_i * 2] | ((source).e[(_i * 2) + 1] << 8))       \

#define vector_cast(vec_to, source)                                            \
  (*((volatile vec_to *)(&(source))))                                          \

#define vector_cast_high(vec_to, source)                                       \
  (*((volatile vec_to *)((u8 *)source.e + (sizeof(source.e) / 2))))            \


#define dup_8x8b(dest, value)                                                  \
  foreach_element(8, (dest).e[_i] = value)                                     \

#define dup_16x8b(dest, value)                                                 \
  foreach_element(16, (dest).e[_i] = value)                                    \

#define dup_4x16b(dest, value)                                                 \
  foreach_element(4, (dest).e[_i] = value)                                     \

#define dup_8x16b(dest, value)                                                 \
  foreach_element(8, (dest).e[_i] = value)                                     \

#define dup_2x32b(dest, value)                                                 \
  foreach_element(2, (dest).e[_i] = value)                                     \

#define dup_4x32b(dest, value)                                                 \
  foreach_element(4, (dest).e[_i] = value)                                     \

#define shr_narrow_8x16b(dest, source, shift)                                  \
  foreach_element(8, (dest).e[_i] = (u16)(source).e[_i] >> (shift))            \

#define shr_narrow_2x64b(dest, source, shift)                                  \
  foreach_element(2, (dest).e[_i] = (source).e[_i] >> (shift))                 \

#define shr_8x8b(dest, source, shift)                                          \
  foreach_element(8, (dest).e[_i] = (u8)(source).e[_i] >> (shift))             \

#define shl_8x8b(dest, source, shift)                                          \
  foreach_element(8, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shr_8x16b(dest, source, shift)                                         \
  foreach_element(8, (dest).e[_i] = (u16)(source).e[_i] >> (shift))            \

#define shr_2x32b(dest, source, shift)                                         \
  foreach_element(2, (dest).e[_i] = (u32)(source).e[_i] >> (shift))            \

#define shr_4x16b(dest, source, shift)                                         \
  foreach_element(4, (dest).e[_i] = (u16)(source).e[_i] >> (shift))            \

#define shl_4x16b(dest, source, shift)                                         \
  foreach_element(4, (dest).e[_i] = (u32)(source).e[_i] << (shift))            \

#define shr_4x32b(dest, source, shift)                                         \
  foreach_element(4, (dest).e[_i] = (u32)(source).e[_i] >> (shift))            \

#define shr_narrow_4x32b(dest, source, shift)                                  \
  foreach_element(4, (dest).e[_i] = (u32)(source).e[_i] >> (shift))            \

#define shl_8x16b(dest, source, shift)                                         \
  foreach_element(8, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shl_4x32b(dest, source, shift)                                         \
  foreach_element(4, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shl_2x32b(dest, source, shift)                                         \
  foreach_element(2, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shl_1x64b(dest, source, shift)                                         \
  ((dest).e[0] = (source).e[0] << (shift))                                     \

#define shl_2x64b(dest, source, shift)                                         \
  foreach_element(2, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shl_variable_2x64b(dest, source_a, source_b)                           \
  foreach_element(2,                                                           \
   (dest).e[_i] = (source_a).e[_i] << ((source_b).e[_i] & 0xFF))               \

#define shl_variable_8x16b(dest, source_a, source_b)                           \
  foreach_element(8,                                                           \
   (dest).e[_i] = (source_a).e[_i] << ((source_b).e[_i] & 0xFF))               \

#define shl_variable_4x16b(dest, source_a, source_b)                           \
  foreach_element(4,                                                           \
   (dest).e[_i] = (source_a).e[_i] << ((source_b).e[_i] & 0xFF))               \

#define shr_1x64b(dest, source, shift)                                         \
  ((dest).e[0] = (source).e[0] >> (shift))                                     \

#define shl_long_8x8b(dest, source, shift)                                     \
  foreach_element(8, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shl_long_4x16b(dest, source, shift)                                    \
  foreach_element(4, (dest).e[_i] = (source).e[_i] << (shift))                 \

#define shrq_narrow_signed_8x16b(dest, source, shift)                          \
  foreach_element(8,                                                           \
  {                                                                            \
    s32 result = ((s16)(source).e[_i]) >> shift;                               \
    if(result < 0)                                                             \
      result = 0;                                                              \
    if(result > 0xFF)                                                          \
      result = 0xFF;                                                           \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define shl_reg_4x32b(dest, source_a, source_b)                                \
  foreach_element(4,                                                           \
  {                                                                            \
    s8 shift  = (source_b).e[_i];                                              \
    if(shift < 0)                                                              \
      dest.e[_i] = (source_a).e[_i] >> (-shift);                               \
    else                                                                       \
      dest.e[_i] = (source_a).e[_i] << shift;                                  \
  })                                                                           \

#define shl_reg_2x32b(dest, source_a, source_b)                                \
  foreach_element(2,                                                           \
  {                                                                            \
    s8 shift  = (source_b).e[_i];                                              \
    if(shift < 0)                                                              \
      dest.e[_i] = (source_a).e[_i] >> (-shift);                               \
    else                                                                       \
      dest.e[_i] = (source_a).e[_i] << shift;                                  \
  })                                                                           \

#define shl_reg_2x64b(dest, source_a, source_b)                                \
  foreach_element(2,                                                           \
  {                                                                            \
    s8 shift  = (source_b).e[_i];                                              \
    if(shift < 0)                                                              \
      dest.e[_i] = (source_a).e[_i] >> (-shift);                               \
    else                                                                       \
      dest.e[_i] = (source_a).e[_i] << shift;                                  \
  })                                                                           \


#define sri_8x8b(dest, source, shift)                                          \
  foreach_element(8, (dest).e[_i] = ((dest).e[_i] & ~(0xFF >> (shift))) |      \
   ((u8)(source).e[_i] >> (shift)))                                            \

#define sli_8x8b(dest, source, shift)                                          \
  foreach_element(8, (dest).e[_i] = ((dest).e[_i] & ~(0xFF << (shift))) |      \
   ((source).e[_i] << (shift)))                                                \



#define mov_narrow_8x16b(dest, source)                                         \
  foreach_element(8, (dest).e[_i] = (source).e[_i])                            \

#define mov_narrow_4x32b(dest, source)                                         \
  foreach_element(4, (dest).e[_i] = (source).e[_i])                            \

#define mov_narrow_2x64b(dest, source)                                         \
  foreach_element(2, (dest).e[_i] = (source).e[_i])                            \

#define mov_wide_8x8b(dest, source)                                            \
  foreach_element(8, (dest).e[_i] = (source).e[_i])                            \

#define mov_wide_2x32b(dest, source)                                           \
  foreach_element(2, (dest).e[_i] = (source).e[_i])                            \

#define mvn_4x16b(dest, source)                                                \
  foreach_element(4, (dest).e[_i] = ~((source).e[_i]))                         \

#define add_4x16b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_2x32b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_16x8b(dest, source_a, source_b)                                    \
  foreach_element(16, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])      \

#define add_8x8b(dest, source_a, source_b)                                     \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_1x64b(dest, source_a, source_b)                                    \
  (dest).e[0] = (source_a).e[0] + (source_b).e[0]                              \

#define add_2x64b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_high_narrow_2x64b(dest, source_a, source_b)                        \
  foreach_element(2,                                                           \
   ((dest).e[_i] = (source_a).e[_i] + (source_b).e[_i]) >> 32)                 \

#define add_high_narrow_4x32b(dest, source_a, source_b)                        \
  foreach_element(4,                                                           \
   ((dest).e[_i] = ((source_a).e[_i] + (source_b).e[_i]) >> 16))               \

#define sub_4x16b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] - (source_b).e[_i])       \

#define sub_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] - (source_b).e[_i])       \

#define sub_2x32b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] - (source_b).e[_i])       \

#define sub_wide_8x8b(dest, source_a, source_b)                                \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] - (source_b).e[_i])       \

#define add_wide_8x8b(dest, source_a, source_b)                                \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define add_wide_2x32b(dest, source_a, source_b)                               \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] + (source_b).e[_i])       \

#define addq_8x8b(dest, source_a, source_b)                                    \
  foreach_element(8,                                                           \
  {                                                                            \
    u32 result = (source_a).e[_i] + (source_b).e[_i];                          \
    if(result > 0xFF)                                                          \
      result = 0xFF;                                                           \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define subq_8x8b(dest, source_a, source_b)                                    \
  foreach_element(8,                                                           \
  {                                                                            \
    u32 result = (source_a).e[_i] - (source_b).e[_i];                          \
    if(result > 0xFF)                                                          \
      result = 0;                                                              \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define subs_long_8x8b(dest, source_a, source_b)                               \
  subs_8x8b(dest, source_a, source_b)                                          \

#define subs_16x8b(dest, source_a, source_b)                                   \
  foreach_element(16,                                                          \
  {                                                                            \
    u32 result = (source_a).e[_i] - (source_b).e[_i];                          \
    if(result > 0xFF)                                                          \
      result = 0;                                                              \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define subs_8x16b(dest, source_a, source_b)                                   \
  foreach_element(8,                                                           \
  {                                                                            \
    s32 result = (source_a).e[_i] - (source_b).e[_i];                          \
    if(result < 0)                                                             \
      result = 0;                                                              \
                                                                               \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define sub_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] - (source_b).e[_i])       \

#define sub_16x8b(dest, source_a, source_b)                                    \
  foreach_element(16, (dest).e[_i] = (source_a).e[_i] - (source_b).e[_i])      \

#define orn_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] | ~((source_b).e[_i]))    \

#define and_4x16b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] & (source_b).e[_i])       \

#define and_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] & (source_b).e[_i])       \

#define and_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] & (source_b).e[_i])       \

#define and_16x8b(dest, source_a, source_b)                                    \
  foreach_element(16, (dest).e[_i] = (source_a).e[_i] & (source_b).e[_i])      \

#define and_8x8b(dest, source_a, source_b)                                     \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] & (source_b).e[_i])       \

#define and_2x32b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] & (source_b).e[_i])       \

#define bic_8x8b(dest, source_a, source_b)                                     \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] & ~((source_b).e[_i]))    \

#define bic_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] & ~((source_b).e[_i]))    \

#define bic_immediate_4x16b(dest, value)                                       \
  foreach_element(4, (dest).e[_i] = (dest).e[_i] & ~(value))                   \

#define bic_immediate_8x16b(dest, value)                                       \
  foreach_element(8, (dest).e[_i] = (dest).e[_i] & ~(value))                   \

#define or_8x16b(dest, source_a, source_b)                                     \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] | (source_b).e[_i])       \

#define or_immediate_8x16b(dest, source_a, value)                              \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] | (value))                \

#define eor_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] ^ (source_b).e[_i])       \

#define eor_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] ^ (source_b).e[_i])       \

#define eor_2x32b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] ^ (source_b).e[_i])       \

#define zip_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] =                                            \
   (u8)(source_a).e[_i] | ((u8)(source_b).e[_i] << 8))                         \

#define zip_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] =                                            \
   (u16)(source_a).e[_i] | ((u16)(source_b).e[_i] << 16))                      \

#define zip_2x64b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] =                                            \
   (u64)(source_a).e[_i] | ((u64)(source_b).e[_i] << 32))                      \

#define unzip_8x8b(dest_a, dest_b, source)                                     \
  foreach_element(8,                                                           \
  {                                                                            \
    (dest_a).e[_i] = (source).e[_i];                                           \
    (dest_b).e[_i] = ((source).e[_i]) >> 8;                                    \
  })                                                                           \

#define unzip_16x8b(dest_a, dest_b, source_a, source_b)                        \
  foreach_element(8,                                                           \
  {                                                                            \
    (dest_a).e[_i] = (source_a).e[_i];                                         \
    (dest_b).e[_i] = (source_a).e[_i] >> 8;                                    \
  });                                                                          \
  foreach_element(8,                                                           \
  {                                                                            \
    (dest_a).e[_i + 8] = (source_b).e[_i];                                     \
    (dest_b).e[_i + 8] = (source_b).e[_i] >> 8;                                \
  })                                                                           \

#define tbl_16(dest, indexes, table)                                           \
  foreach_element(8,                                                           \
  {                                                                            \
    u32 index = indexes.e[_i];                                                 \
    if(index < 16)                                                             \
      (dest).e[_i] = table.e[index];                                           \
    else                                                                       \
      (dest).e[_i] = 0;                                                        \
  })                                                                           \

#define cmpeqz_8x16b(dest, source)                                             \
  foreach_element(8, (dest).e[_i] = ~(((source).e[_i] == 0) - 1))              \

#define cmpltz_8x16b(dest, source)                                             \
  foreach_element(8, (dest).e[_i] = ((s16)(source).e[_i] >> 15))               \

#define cmpltz_4x32b(dest, source)                                             \
  foreach_element(4, (dest).e[_i] = ((s32)(source).e[_i] >> 31))               \

#define cmpltz_2x32b(dest, source)                                             \
  foreach_element(2, (dest).e[_i] = ((s32)(source).e[_i] >> 31))               \

#define cmplte_4x16b(dest, source_a, source_b)                                 \
  foreach_element(4, (dest).e[_i] = ~((source_a.e[_i] <= source_b.e[_i]) - 1)) \

#define cmplt_4x16b(dest, source_a, source_b)                                  \
  foreach_element(4, (dest).e[_i] = ~((source_a.e[_i] < source_b.e[_i]) - 1))  \

#define cmpgt_4x16b(dest, source_a, source_b)                                  \
  foreach_element(4, (dest).e[_i] = ~((source_a.e[_i] > source_b.e[_i]) - 1))  \

#define tst_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8,                                                           \
   (dest).e[_i] = ~(((source_a.e[_i] & source_b.e[_i]) != 0) - 1))             \

#define andi_8x8b(dest, source_a, value)                                       \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] & value)                  \

#define average_8x16b(dest, source_a, source_b)                                \
  foreach_element(8,                                                           \
   (dest).e[_i] = ((source_a).e[_i] + (source_b).e[_i]) >> 1)                  \


#define mul_8x8b(dest, source_a, source_b)                                     \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] * (source_b).e[_i])       \

#define mul_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] * (source_b).e[_i])       \

#define mul_2x32b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] = (source_a).e[_i] * (source_b).e[_i])       \

#define mul_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] * (source_b).e[_i])       \

#define mul_long_8x8b(dest, source_a, source_b)                                \
  foreach_element(8, (dest).e[_i] = (source_a).e[_i] * (source_b).e[_i])       \

#define mul_long_4x16b(dest, source_a, source_b)                               \
  foreach_element(4, (dest).e[_i] = (source_a).e[_i] * (source_b).e[_i])       \

#define mul_long_2x32b(dest, source_a, source_b)                               \
  foreach_element(2,                                                           \
   (dest).e[_i] = (source_a).e[_i] * (s64)((source_b).e[_i]))                  \

#define mul_scalar_2x32b(dest, source, value)                                  \
  foreach_element(2, (dest).e[_i] = (source).e[_i] * value)                    \

#define mul_scalar_long_8x16b(dest, source, value)                             \
  foreach_element(8, (dest).e[_i] = (source).e[_i] * value)                    \

#define mul_scalar_long_2x32b(dest, source, value)                             \
  foreach_element(2, (dest).e[_i] = (source).e[_i] * value)                    \

#define mla_2x32b(dest, source_a, source_b)                                    \
  foreach_element(2, (dest).e[_i] += (source_a).e[_i] * (source_b).e[_i])      \

#define mla_4x32b(dest, source_a, source_b)                                    \
  foreach_element(4, (dest).e[_i] += (source_a).e[_i] * (source_b).e[_i])      \

#define mla_scalar_long_2x32b(dest, source, value)                             \
  foreach_element(2, (dest).e[_i] += (source).e[_i] * value)                   \

#define mla_long_8x8b(dest, source_a, source_b)                                \
  foreach_element(8, (dest).e[_i] += (source_a).e[_i] * (source_b).e[_i])      \

#define mla_long_2x32b(dest, source_a, source_b)                               \
  foreach_element(2, (dest).e[_i] += (source_a).e[_i] * (s64)(source_b).e[_i]) \

#define mla_scalar_4x32b(dest, source, value)                                  \
  foreach_element(4, (dest).e[_i] += (source).e[_i] * value)                   \

#define mla_scalar_2x32b(dest, source, value)                                  \
  foreach_element(2, (dest).e[_i] += (source).e[_i] * value)                   \

#define mls_scalar_4x32b(dest, source, value)                                  \
  foreach_element(4, (dest).e[_i] -= (source).e[_i] * value)                   \

#define mls_scalar_2x32b(dest, source, value)                                  \
  foreach_element(2, (dest).e[_i] -= (source).e[_i] * value)                   \

#define mls_scalar_long_2x32b(dest, source, value)                             \
  foreach_element(2, (dest).e[_i] -= (source).e[_i] * value)                   \

#define rev_2x32b(dest, source)                                                \
{                                                                              \
  u32 tmp = source.e[1];                                                       \
  (dest).e[1] = source.e[0];                                                   \
  (dest).e[0] = tmp;                                                           \
}                                                                              \

#define abs_4x32b(dest, source)                                                \
  foreach_element(4, (dest).e[_i] = abs(source.e[_i]))                         \

#define abs_2x32b(dest, source)                                                \
  foreach_element(2, (dest).e[_i] = abs(source.e[_i]))                         \

#define neg_2x32b(dest, source)                                                \
  foreach_element(2, (dest).e[_i] = -((source).e[_i]))                         \


#define shrq_narrow_8x16b(dest, source, shift)                                 \
  foreach_element(8,                                                           \
  {                                                                            \
    u32 result = ((source).e[_i]) >> shift;                                    \
    if(result > 0xFF)                                                          \
      result = 0xFF;                                                           \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define min_4x16b(dest, source_a, source_b)                                    \
  foreach_element(4,                                                           \
  {                                                                            \
    s32 result = (source_a).e[_i];                                             \
    if((source_b).e[_i] < result)                                              \
      result = (source_b).e[_i];                                               \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define min_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8,                                                           \
  {                                                                            \
    s32 result = (source_a).e[_i];                                             \
    if((source_b).e[_i] < result)                                              \
      result = (source_b).e[_i];                                               \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define min_8x8b(dest, source_a, source_b)                                     \
  foreach_element(8,                                                           \
  {                                                                            \
    u32 result = (source_a).e[_i];                                             \
    if((source_b).e[_i] < result)                                              \
      result = (source_b).e[_i];                                               \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define min_16x8b(dest, source_a, source_b)                                    \
  foreach_element(16,                                                          \
  {                                                                            \
    u32 result = (source_a).e[_i];                                             \
    if((source_b).e[_i] < result)                                              \
      result = (source_b).e[_i];                                               \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define max_8x16b(dest, source_a, source_b)                                    \
  foreach_element(8,                                                           \
  {                                                                            \
    s32 result = (source_a).e[_i];                                             \
    if((source_b).e[_i] > result)                                              \
      result = (source_b).e[_i];                                               \
    (dest).e[_i] = result;                                                     \
  })                                                                           \

#define bsl_8x16b(dest_mask, source_a, source_b)                               \
  foreach_element(8, dest_mask.e[_i] = ((source_a).e[_i] & dest_mask.e[_i]) |  \
   ((source_b).e[_i] & ~(dest_mask.e[_i])))                                    \

#define bif_8x16b(dest, source, mask)                                          \
  foreach_element(8, dest.e[_i] = ((source).e[_i] & ~(mask.e[_i])) |           \
   ((dest).e[_i] & mask.e[_i]))                                                \

#define bsl_4x32b(dest_mask, source_a, source_b)                               \
  foreach_element(4, dest_mask.e[_i] = ((source_a).e[_i] & dest_mask.e[_i]) |  \
   ((source_b).e[_i] & ~(dest_mask.e[_i])))                                    \

#define bit_4x16b(dest, source, mask)                                          \
  foreach_element(4, dest.e[_i] = ((source).e[_i] & mask.e[_i]) |              \
   ((dest).e[_i] & ~(mask.e[_i])))                                             \

#endif
