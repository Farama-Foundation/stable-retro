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

#ifndef VECTOR_TYPES
#define VECTOR_TYPES

#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

#define build_vector_type_pair(sign, size, count, count_x2)                    \
typedef struct                                                                 \
{                                                                              \
  sign##size e[count];                                                         \
} vec_##count##x##size##sign;                                                  \
                                                                               \
typedef struct                                                                 \
{                                                                              \
  union                                                                        \
  {                                                                            \
    sign##size e[count_x2];                                                    \
    struct                                                                     \
    {                                                                          \
      vec_##count##x##size##sign low;                                          \
      vec_##count##x##size##sign high;                                         \
    };                                                                         \
  };                                                                           \
} vec_##count_x2##x##size##sign                                                \

#define build_vector_types(sign)                                               \
  build_vector_type_pair(sign, 8, 8, 16);                                      \
  build_vector_type_pair(sign, 16, 4, 8);                                      \
  build_vector_type_pair(sign, 32, 2, 4);                                      \
  build_vector_type_pair(sign, 64, 1, 2)                                       \

build_vector_types(u);
build_vector_types(s);

#endif // VECTOR_TYPES
