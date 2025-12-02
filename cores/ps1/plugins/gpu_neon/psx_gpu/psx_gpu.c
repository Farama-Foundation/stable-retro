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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#ifndef NEON_BUILD
#include "vector_ops.h"
#endif
#include "psx_gpu_simd.h"
#include "psx_gpu_offsets.h"

#if 0
void dump_r_d(const char *name, void *dump);
void dump_r_q(const char *name, void *dump);
#define dumprd(n) dump_r_d(#n, n.e)
#define dumprq(n) dump_r_q(#n, n.e)
#endif

u32 span_pixels = 0;
u32 span_pixel_blocks = 0;
u32 spans = 0;
u32 triangles = 0;
u32 sprites = 0;
u32 sprites_4bpp = 0;
u32 sprites_8bpp = 0;
u32 sprites_16bpp = 0;
u32 sprite_blocks = 0;
u32 sprites_untextured = 0;
u32 lines = 0;
u32 trivial_rejects = 0;
u32 texels_4bpp = 0;
u32 texels_8bpp = 0;
u32 texels_16bpp = 0;
u32 texel_blocks_4bpp = 0;
u32 texel_blocks_8bpp = 0;
u32 texel_blocks_16bpp = 0;
u32 texel_blocks_untextured = 0;
u32 blend_blocks = 0;
u32 render_buffer_flushes = 0;
u32 state_changes = 0;
u32 left_split_triangles = 0;
u32 flat_triangles = 0;
u32 clipped_triangles = 0;
u32 zero_block_spans = 0;
u32 texture_cache_loads = 0;
u32 false_modulated_blocks = 0;

#define stats_add(stat, count) // stat += count

/* double size for enhancement */
u32 reciprocal_table[512 * 2];


typedef s32 fixed_type;

#define EDGE_STEP_BITS 32
#define FIXED_BITS     12

#define fixed_center(value)                                                    \
  ((((fixed_type)(value)) << FIXED_BITS) + (1 << (FIXED_BITS - 1)))            \

#define int_to_fixed(value)                                                    \
  (((fixed_type)(value)) << FIXED_BITS)                                        \

#define fixed_to_int(value)                                                    \
  ((value) >> FIXED_BITS)                                                      \

#define fixed_to_double(value)                                                 \
  ((value) / (double)(1 << FIXED_BITS))                                        \

#define double_to_fixed(value)                                                 \
  (fixed_type)(((value) * (double)(1 << FIXED_BITS)))                          \

typedef void (setup_blocks_function_type)(psx_gpu_struct *psx_gpu);
typedef void (texture_blocks_function_type)(psx_gpu_struct *psx_gpu);
typedef void (shade_blocks_function_type)(psx_gpu_struct *psx_gpu);
typedef void (blend_blocks_function_type)(psx_gpu_struct *psx_gpu);

typedef void (setup_sprite_function_type)(psx_gpu_struct *psx_gpu, s32 x,
 s32 y, s32 u, s32 v, s32 width, s32 height, u32 color);

struct render_block_handler_struct
{
  void *setup_blocks;
  texture_blocks_function_type *texture_blocks;
  shade_blocks_function_type *shade_blocks;
  blend_blocks_function_type *blend_blocks; 
};

#ifndef NEON_BUILD

u32 fixed_reciprocal(u32 denominator, u32 *_shift)
{
  u32 shift = __builtin_clz(denominator);
  u32 denominator_normalized = denominator << shift;

  double numerator = (1ULL << 62) + denominator_normalized;
  double numerator_b;

  double denominator_normalized_dp_b;
  u64 denominator_normalized_dp_u64;

  u32 reciprocal;
  double reciprocal_dp;

  u64 numerator_u64 = (denominator_normalized >> 10) |
   ((u64)(62 + 1023) << 52);
  *((u64 *)(&numerator_b)) = numerator_u64;

  denominator_normalized_dp_u64 =
   (u64)(denominator_normalized << 21) |
   ((u64)((denominator_normalized >> 11) + ((1022 + 31) << 20)) << 32);
  *((u64 *)(&denominator_normalized_dp_b)) = denominator_normalized_dp_u64;

  // Implement with a DP divide
  reciprocal_dp = numerator / denominator_normalized_dp_b;
  reciprocal = reciprocal_dp;

  if(reciprocal == 0x80000001)
    reciprocal = 0x80000000;

  *_shift = 62 - shift;
  return reciprocal;
}

double reciprocal_estimate(double a)
{
  int q, s;
  double r;

  q = (int)(a * 512.0);
  /* a in units of 1/512 rounded down */
  r = 1.0 / (((double)q + 0.5) / 512.0); /* reciprocal r */
  s = (int)(256.0 * r + 0.5);

  /* r in units of 1/256 rounded to nearest */
  
  return (double)s / 256.0;
}

u32 reciprocal_estimate_u32(u32 value)
{
  u64 dp_value_u64;
  volatile double dp_value;
  volatile u64 *dp_value_ptr = (volatile u64 *)&dp_value;

  if((value >> 31) == 0)
    return 0xFFFFFFFF;

  dp_value_u64 = (0x3FEULL << (31 + 21)) | ((u64)(value & 0x7FFFFFFF) << 21);

  *dp_value_ptr = dp_value_u64;

  dp_value = reciprocal_estimate(dp_value);
  dp_value_u64 = *dp_value_ptr;

  return (0x80000000 | ((dp_value_u64 >> 21) & 0x7FFFFFFF));
}

u32 fixed_reciprocal_nr(u32 value, u32 *_shift)
{
  u32 shift = __builtin_clz(value);
  u32 value_normalized = value << shift;

  *_shift = 62 - shift;

  value_normalized -= 2;

  u32 reciprocal_normalized = reciprocal_estimate_u32(value_normalized) >> 1;

  u32 temp = -(((u64)value_normalized * (u32)reciprocal_normalized) >> 31);
  reciprocal_normalized = (((u64)reciprocal_normalized * temp) >> 31);
  temp = -(((u64)value_normalized * (u32)reciprocal_normalized) >> 31);
  reciprocal_normalized = (((u64)reciprocal_normalized * temp) >> 31);
  temp = -(((u64)value_normalized * (u32)reciprocal_normalized) >> 31);
  reciprocal_normalized = (((u64)reciprocal_normalized * temp) >> 31);

  return reciprocal_normalized;
}

#endif


s32 triangle_signed_area_x2(s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2)
{
	return ((x1 - x0) * (y2 - y1)) - ((x2 - x1) * (y1 - y0));
}

u32 texture_region_mask(s32 x1, s32 y1, s32 x2, s32 y2)
{
  s32 coverage_x, coverage_y;

  u32 mask_up_left;
  u32 mask_down_right;

  coverage_x = x2 >> 6;
  coverage_y = y2 >> 8;

  if(coverage_x < 0)
    coverage_x = 0;

  if(coverage_x > 31)
    coverage_x = 31;

  mask_down_right = ~(0xFFFFFFFF << (coverage_x + 1)) & 0xFFFF;

  if(coverage_y >= 1)
    mask_down_right |= mask_down_right << 16;

  coverage_x = x1 >> 6;

  mask_up_left = 0xFFFF0000;
  if(coverage_x > 0)
    mask_up_left <<= coverage_x;

  coverage_y = y1 >> 8;
  if(coverage_y <= 0)
    mask_up_left |= mask_up_left >> 16;

  return mask_up_left & mask_down_right;
}

u32 invalidate_texture_cache_region(psx_gpu_struct *psx_gpu, u32 x1, u32 y1,
 u32 x2, u32 y2)
{
  u32 mask = texture_region_mask(x1, y1, x2, y2);

  psx_gpu->dirty_textures_4bpp_mask |= mask;
  psx_gpu->dirty_textures_8bpp_mask |= mask;
  psx_gpu->dirty_textures_8bpp_alternate_mask |= mask;

  return mask;
}

u32 invalidate_texture_cache_region_viewport(psx_gpu_struct *psx_gpu, u32 x1,
 u32 y1, u32 x2, u32 y2)
{
  u32 mask = texture_region_mask(x1, y1, x2, y2) &
   psx_gpu->viewport_mask;

  psx_gpu->dirty_textures_4bpp_mask |= mask;
  psx_gpu->dirty_textures_8bpp_mask |= mask;
  psx_gpu->dirty_textures_8bpp_alternate_mask |= mask;

  return mask;
}

static void update_texture_cache_region_(psx_gpu_struct *psx_gpu,
 u32 x1, u32 y1, u32 x2, u32 y2)
{
  u32 mask = texture_region_mask(x1, y1, x2, y2);
  u32 texture_page;
  u8 *texture_page_ptr;
  u16 *vram_ptr;
  u32 texel_block;
  u32 sub_x, sub_y;

  psx_gpu->dirty_textures_8bpp_mask |= mask;
  psx_gpu->dirty_textures_8bpp_alternate_mask |= mask;

  if ((psx_gpu->dirty_textures_4bpp_mask & mask) == 0 &&
      (x1 & 3) == 0 && (y1 & 15) == 0 && x2 - x1 < 4 && y2 - y1 < 16)
  {
    texture_page = ((x1 / 64) & 15) + (y1 / 256) * 16;
    texture_page_ptr = psx_gpu->texture_4bpp_cache[texture_page];
    texture_page_ptr += (x1 / 4 & 15) * 16*16 + (y1 / 16 & 15) * 16*16*16;
    vram_ptr = psx_gpu->vram_ptr + x1 + y1 * 1024;
    sub_x = 4;
    sub_y = 16;

    while(sub_y)
    {
      while(sub_x)
      {
        texel_block = *vram_ptr;

        texture_page_ptr[0] = texel_block & 0xF;
        texture_page_ptr[1] = (texel_block >> 4) & 0xF;
        texture_page_ptr[2] = (texel_block >> 8) & 0xF;
        texture_page_ptr[3] = texel_block >> 12;
        
        vram_ptr++;
        texture_page_ptr += 4;

        sub_x--;          
      }

      vram_ptr -= 4;
      sub_x = 4;

      sub_y--;
      vram_ptr += 1024;
    }
  }
  else
  {
    psx_gpu->dirty_textures_4bpp_mask |= mask;
  }
}

void update_texture_cache_region(psx_gpu_struct *psx_gpu, u32 x1, u32 y1,
 u32 x2, u32 y2)
{
  s32 w = x2 - x1;
  do
  {
    x2 = x1 + w;
    if (x2 > 1023)
      x2 = 1023;
    update_texture_cache_region_(psx_gpu, x1, y1, x2, y2);
    w -= x2 - x1;
    x1 = 0;
  }
  while (unlikely(w > 0));
}

#ifndef NEON_BUILD

void update_texture_4bpp_cache(psx_gpu_struct *psx_gpu)
{
  u32 current_texture_page = psx_gpu->current_texture_page;
  u8 *texture_page_ptr = psx_gpu->texture_page_base;
  u16 *vram_ptr = psx_gpu->vram_ptr;

  u32 texel_block;
  u32 tile_x, tile_y;
  u32 sub_x, sub_y;

  vram_ptr += (current_texture_page >> 4) * 256 * 1024;
  vram_ptr += (current_texture_page & 0xF) * 64;

  texture_cache_loads++;

  tile_y = 16;
  tile_x = 16;
  sub_x = 4;
  sub_y = 16;

  psx_gpu->dirty_textures_4bpp_mask &= ~(psx_gpu->current_texture_mask);

  while(tile_y)
  {
    while(tile_x)
    {
      while(sub_y)
      {
        while(sub_x)
        {
          texel_block = *vram_ptr;

          texture_page_ptr[0] = texel_block & 0xF;
          texture_page_ptr[1] = (texel_block >> 4) & 0xF;
          texture_page_ptr[2] = (texel_block >> 8) & 0xF;
          texture_page_ptr[3] = texel_block >> 12;
          
          vram_ptr++;
          texture_page_ptr += 4;

          sub_x--;          
        }

        vram_ptr -= 4;
        sub_x = 4;

        sub_y--;
        vram_ptr += 1024;
      }

      sub_y = 16;

      vram_ptr -= (1024 * 16) - 4;
      tile_x--;
    }

    tile_x = 16;

    vram_ptr += (16 * 1024) - (4 * 16);
    tile_y--;
  }
}

void update_texture_8bpp_cache_slice(psx_gpu_struct *psx_gpu,
 u32 texture_page)
{
  u16 *texture_page_ptr = psx_gpu->texture_page_base;
  u16 *vram_ptr = psx_gpu->vram_ptr;

  u32 tile_x, tile_y;
  u32 sub_y;

  vec_8x16u texels;

  texture_cache_loads++;

  vram_ptr += (texture_page >> 4) * 256 * 1024;
  vram_ptr += (texture_page & 0xF) * 64;

  if((texture_page ^ psx_gpu->current_texture_page) & 0x1)
    texture_page_ptr += (8 * 16) * 8;

  tile_x = 8;
  tile_y = 16;

  sub_y = 16;

  while(tile_y)
  {
    while(tile_x)
    {
      while(sub_y)
      {
        load_128b(texels, vram_ptr);
        store_128b(texels, texture_page_ptr);

        texture_page_ptr += 8;
        vram_ptr += 1024;

        sub_y--;
      }

      sub_y = 16;

      vram_ptr -= (1024 * 16);
      vram_ptr += 8;

      tile_x--;
    }

    tile_x = 8;

    vram_ptr -= (8 * 8);
    vram_ptr += (16 * 1024);

    texture_page_ptr += (8 * 16) * 8;
    tile_y--;
  }
}

#endif


void update_texture_8bpp_cache(psx_gpu_struct *psx_gpu)
{
  u32 current_texture_page = psx_gpu->current_texture_page;
  u32 update_textures =
   psx_gpu->dirty_textures_8bpp_mask & psx_gpu->current_texture_mask;

  psx_gpu->dirty_textures_8bpp_mask &= ~update_textures;

  if(update_textures & (1 << current_texture_page))
  {
    update_texture_8bpp_cache_slice(psx_gpu, current_texture_page);
    update_textures &= ~(1 << current_texture_page);
  }

  if(update_textures)
  {
    u32 adjacent_texture_page = ((current_texture_page + 1) & 0xF) |
     (current_texture_page & 0x10);

    update_texture_8bpp_cache_slice(psx_gpu, adjacent_texture_page);
  }
}

void flush_render_block_buffer(psx_gpu_struct *psx_gpu)
{
  if((psx_gpu->render_mode & RENDER_INTERLACE_ENABLED) &&
   (psx_gpu->primitive_type == PRIMITIVE_TYPE_SPRITE))
  {
    u32 num_blocks_dest = 0;
    block_struct *block_src = psx_gpu->blocks; 
    block_struct *block_dest = psx_gpu->blocks;

    u16 *vram_ptr = psx_gpu->vram_ptr;
    u32 i;

    if(psx_gpu->render_mode & RENDER_INTERLACE_ODD)
    {
      for(i = 0; i < psx_gpu->num_blocks; i++)
      {
        u32 fb_offset = (u32)((u8 *)block_src->fb_ptr - (u8 *)vram_ptr);
        if(fb_offset & (1 << 11))
        {
          *block_dest = *block_src;
          num_blocks_dest++;
          block_dest++;
        }
        block_src++;
      }
    }
    else
    {
      for(i = 0; i < psx_gpu->num_blocks; i++)
      {
        u32 fb_offset = (u32)((u8 *)block_src->fb_ptr - (u8 *)vram_ptr);
        if((fb_offset & (1 << 11)) == 0)
        {
          *block_dest = *block_src;
          num_blocks_dest++;
          block_dest++;
        }
        block_src++;
      }
    }

    psx_gpu->num_blocks = num_blocks_dest;
  }

  if(psx_gpu->num_blocks)
  {
    render_block_handler_struct *render_block_handler =
     psx_gpu->render_block_handler;

#if defined(__arm__) && defined(NEON_BUILD) && !defined(SIMD_BUILD)
    // the asm doesn't bother to save callee-save vector regs, so do it here
    __asm__ __volatile__("":::"q4","q5","q6","q7");
#endif

    render_block_handler->texture_blocks(psx_gpu);
    render_block_handler->shade_blocks(psx_gpu);
    render_block_handler->blend_blocks(psx_gpu);

#ifdef PROFILE
    span_pixel_blocks += psx_gpu->num_blocks;
    render_buffer_flushes++;
#endif

    psx_gpu->num_blocks = 0;
#if defined(__arm__) && defined(NEON_BUILD) && !defined(SIMD_BUILD)
    __asm__ __volatile__("":::"q4","q5","q6","q7");
#endif
  }
}


#ifndef NEON_BUILD

#define setup_gradient_calculation_input(set, vertex)                          \
  /* First type is:  uvrg bxxx xxxx                                          */\
  /* Second type is: yyyy ybyy uvrg                                          */\
  /* Since x_a and y_c are the same the same variable is used for both.      */\
  x##set##_a_y##set##_c.e[0] = vertex->u;                                      \
  x##set##_a_y##set##_c.e[1] = vertex->v;                                      \
  x##set##_a_y##set##_c.e[2] = vertex->r;                                      \
  x##set##_a_y##set##_c.e[3] = vertex->g;                                      \
  dup_4x16b(x##set##_b, vertex->x);                                            \
  dup_4x16b(x##set##_c, vertex->x);                                            \
  dup_4x16b(y##set##_a, vertex->y);                                            \
  dup_4x16b(y##set##_b, vertex->y);                                            \
  x##set##_b.e[0] = vertex->b;                                                 \
  y##set##_b.e[1] = vertex->b                                                  \
  

void compute_all_gradients(psx_gpu_struct * __restrict__ psx_gpu,
 const vertex_struct * __restrict__ a, const vertex_struct * __restrict__ b,
 const vertex_struct * __restrict__ c)
{
  u32 triangle_area = psx_gpu->triangle_area;
  u32 winding_mask_scalar;

  u32 triangle_area_shift;
  u64 triangle_area_reciprocal =
   fixed_reciprocal(triangle_area, &triangle_area_shift);
  triangle_area_shift = -(triangle_area_shift - FIXED_BITS);

  // ((x1 - x0) * (y2 - y1)) - ((x2 - x1) * (y1 - y0)) =
  // ( d0       *  d1      ) - ( d2       *  d3      ) =
  // ( m0                  ) - ( m1                  ) = gradient

  // This is split to do 12 elements at a time over three sets: a, b, and c.
  // Technically we only need to do 10 elements (uvrgb_x and uvrgb_y), so
  // two of the slots are unused.

  // Inputs are all 16-bit signed. The m0/m1 results are 32-bit signed, as
  // is g.

  vec_4x16s x0_a_y0_c, x0_b, x0_c;
  vec_4x16s y0_a, y0_b;
  vec_4x16s x1_a_y1_c, x1_b, x1_c;
  vec_4x16s y1_a, y1_b;
  vec_4x16s x2_a_y2_c, x2_b, x2_c;
  vec_4x16s y2_a, y2_b;

  vec_4x32u uvrg_base;
  vec_4x32u b_base;
  vec_4x32u uvrgb_phase;

  vec_4x16s d0_a_d3_c, d0_b, d0_c;
  vec_4x16s d1_a, d1_b, d1_c_d2_a;
  vec_4x16s d2_b, d2_c;
  vec_4x16s d3_a, d3_b;

  vec_4x32s m0_a, m0_b, m0_c;
  vec_4x32s m1_a, m1_b, m1_c;

  vec_4x32u gradient_area_a, gradient_area_c;
  vec_2x32u gradient_area_b;  

  vec_4x32u gradient_area_sign_a, gradient_area_sign_c;
  vec_2x32u gradient_area_sign_b;
  vec_4x32u winding_mask;

  vec_2x64u gradient_wide_a0, gradient_wide_a1;
  vec_2x64u gradient_wide_c0, gradient_wide_c1;
  vec_2x64u gradient_wide_b;

  vec_4x32u gradient_a, gradient_c;
  vec_2x32u gradient_b;
  vec_16x8s gradient_shift;

  setup_gradient_calculation_input(0, a);
  setup_gradient_calculation_input(1, b);
  setup_gradient_calculation_input(2, c);

  dup_4x32b(uvrgb_phase, psx_gpu->uvrgb_phase);
  shl_long_4x16b(uvrg_base, x0_a_y0_c, 16);
  shl_long_4x16b(b_base, x0_b, 16);

  add_4x32b(uvrg_base, uvrg_base, uvrgb_phase);
  add_4x32b(b_base, b_base, uvrgb_phase);

  // Can probably pair these, but it'll require careful register allocation
  sub_4x16b(d0_a_d3_c, x1_a_y1_c, x0_a_y0_c);
  sub_4x16b(d1_c_d2_a, x2_a_y2_c, x1_a_y1_c);

  sub_4x16b(d0_b, x1_b, x0_b);
  sub_4x16b(d0_c, x1_c, x0_c);

  sub_4x16b(d1_a, y2_a, y1_a);
  sub_4x16b(d1_b, y2_b, y1_b);

  sub_4x16b(d2_b, x2_b, x1_b);
  sub_4x16b(d2_c, x2_c, x1_c);

  sub_4x16b(d3_a, y1_a, y0_a);
  sub_4x16b(d3_b, y1_b, y0_b);

  mul_long_4x16b(m0_a, d0_a_d3_c, d1_a);
  mul_long_4x16b(m0_b, d0_b, d1_b);
  mul_long_4x16b(m0_c, d0_c, d1_c_d2_a);

  mul_long_4x16b(m1_a, d1_c_d2_a, d3_a);
  mul_long_4x16b(m1_b, d2_b, d3_b);
  mul_long_4x16b(m1_c, d2_c, d0_a_d3_c);

  sub_4x32b(gradient_area_a, m0_a, m1_a);
  sub_2x32b(gradient_area_b, m0_b.low, m1_b.low);
  sub_4x32b(gradient_area_c, m0_c, m1_c);

  cmpltz_4x32b(gradient_area_sign_a, gradient_area_a);
  cmpltz_2x32b(gradient_area_sign_b, gradient_area_b);
  cmpltz_4x32b(gradient_area_sign_c, gradient_area_c);

  abs_4x32b(gradient_area_a, gradient_area_a);
  abs_2x32b(gradient_area_b, gradient_area_b);
  abs_4x32b(gradient_area_c, gradient_area_c);

  winding_mask_scalar = -psx_gpu->triangle_winding;

  dup_4x32b(winding_mask, winding_mask_scalar);
  eor_4x32b(gradient_area_sign_a, gradient_area_sign_a, winding_mask);
  eor_2x32b(gradient_area_sign_b, gradient_area_sign_b, winding_mask);
  eor_4x32b(gradient_area_sign_c, gradient_area_sign_c, winding_mask);

  mul_scalar_long_2x32b(gradient_wide_a0, 
   vector_cast(vec_2x32s, gradient_area_a.low), 
   (s64)triangle_area_reciprocal);
  mul_scalar_long_2x32b(gradient_wide_a1,
   vector_cast(vec_2x32s, gradient_area_a.high),
   (s64)triangle_area_reciprocal);
  mul_scalar_long_2x32b(gradient_wide_b, 
   vector_cast(vec_2x32s, gradient_area_b),
   (s64)triangle_area_reciprocal);
  mul_scalar_long_2x32b(gradient_wide_c0, 
   vector_cast(vec_2x32s, gradient_area_c.low),
   (s64)triangle_area_reciprocal);
  mul_scalar_long_2x32b(gradient_wide_c1, 
   vector_cast(vec_2x32s, gradient_area_c.high),
   (s64)triangle_area_reciprocal);

  dup_16x8b(gradient_shift, triangle_area_shift);
  shl_reg_2x64b(gradient_wide_a0, gradient_wide_a0,
   vector_cast(vec_2x64u, gradient_shift));
  shl_reg_2x64b(gradient_wide_a1, gradient_wide_a1,
   vector_cast(vec_2x64u, gradient_shift));
  shl_reg_2x64b(gradient_wide_b, gradient_wide_b,
   vector_cast(vec_2x64u, gradient_shift));
  shl_reg_2x64b(gradient_wide_c0, gradient_wide_c0,
   vector_cast(vec_2x64u, gradient_shift));
  shl_reg_2x64b(gradient_wide_c1, gradient_wide_c1,
   vector_cast(vec_2x64u, gradient_shift));

  mov_narrow_2x64b(gradient_a.low, gradient_wide_a0);
  mov_narrow_2x64b(gradient_a.high, gradient_wide_a1);
  mov_narrow_2x64b(gradient_b, gradient_wide_b);
  mov_narrow_2x64b(gradient_c.low, gradient_wide_c0);
  mov_narrow_2x64b(gradient_c.high, gradient_wide_c1);

  shl_4x32b(gradient_a, gradient_a, 4);
  shl_2x32b(gradient_b, gradient_b, 4);
  shl_4x32b(gradient_c, gradient_c, 4);

  eor_4x32b(gradient_a, gradient_a, gradient_area_sign_a);
  eor_2x32b(gradient_b, gradient_b, gradient_area_sign_b);
  eor_4x32b(gradient_c, gradient_c, gradient_area_sign_c);

  sub_4x32b(gradient_a, gradient_a, gradient_area_sign_a);
  sub_2x32b(gradient_b, gradient_b, gradient_area_sign_b);
  sub_4x32b(gradient_c, gradient_c, gradient_area_sign_c);

  u32 left_adjust = a->x;
  mls_scalar_4x32b(uvrg_base, gradient_a, left_adjust);
  mls_scalar_2x32b(b_base.low, gradient_b, left_adjust);

  vec_4x32u uvrg_dx2;
  vec_2x32u b_dx2;

  vec_4x32u uvrg_dx3;
  vec_2x32u b_dx3;

  vec_4x32u zero;

  eor_4x32b(zero, zero, zero);
  add_4x32b(uvrg_dx2, gradient_a, gradient_a);
  add_2x32b(b_dx2, gradient_b, gradient_b);
  add_4x32b(uvrg_dx3, gradient_a, uvrg_dx2);
  add_2x32b(b_dx3, gradient_b, b_dx2);

  // Can be done with vst4, assuming that the zero, dx, dx2, and dx3 are
  // lined up properly
  psx_gpu->u_block_span.e[0] = zero.e[0];
  psx_gpu->u_block_span.e[1] = gradient_a.e[0];
  psx_gpu->u_block_span.e[2] = uvrg_dx2.e[0];
  psx_gpu->u_block_span.e[3] = uvrg_dx3.e[0];

  psx_gpu->v_block_span.e[0] = zero.e[1];
  psx_gpu->v_block_span.e[1] = gradient_a.e[1];
  psx_gpu->v_block_span.e[2] = uvrg_dx2.e[1];
  psx_gpu->v_block_span.e[3] = uvrg_dx3.e[1];

  psx_gpu->r_block_span.e[0] = zero.e[2];
  psx_gpu->r_block_span.e[1] = gradient_a.e[2];
  psx_gpu->r_block_span.e[2] = uvrg_dx2.e[2];
  psx_gpu->r_block_span.e[3] = uvrg_dx3.e[2];

  psx_gpu->g_block_span.e[0] = zero.e[3];
  psx_gpu->g_block_span.e[1] = gradient_a.e[3];
  psx_gpu->g_block_span.e[2] = uvrg_dx2.e[3];
  psx_gpu->g_block_span.e[3] = uvrg_dx3.e[3];

  psx_gpu->b_block_span.e[0] = zero.e[0];
  psx_gpu->b_block_span.e[1] = gradient_b.e[0];
  psx_gpu->b_block_span.e[2] = b_dx2.e[0];
  psx_gpu->b_block_span.e[3] = b_dx3.e[0];

  psx_gpu->uvrg = uvrg_base;
  psx_gpu->b = b_base.e[0];

  psx_gpu->uvrg_dx = gradient_a;
  psx_gpu->uvrg_dy = gradient_c;
  psx_gpu->b_dy = gradient_b.e[1];
}
#endif

#define vector_check(_a, _b)                                                   \
  if(memcmp(&_a, &_b, sizeof(_b)))                                             \
  {                                                                            \
    if(sizeof(_b) == 8)                                                        \
    {                                                                          \
      printf("mismatch on %s vs %s: (%x %x) vs (%x %x)\n",                     \
       #_a, #_b, _a.e[0], _a.e[1], _b.e[0], _b.e[1]);                          \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      printf("mismatch on %s vs %s: (%x %x %x %x) vs (%x %x %x %x)\n",         \
       #_a, #_b, _a.e[0], _a.e[1], _a.e[2], _a.e[3], _b.e[0], _b.e[1],         \
       _b.e[2], _b.e[3]);                                                      \
    }                                                                          \
  }                                                                            \

#define scalar_check(_a, _b)                                                   \
  if(_a != _b)                                                                 \
    printf("mismatch on %s %s: %x vs %x\n", #_a, #_b, _a, _b)                  \


#if !defined(NEON_BUILD) && !defined(NDEBUG)
static void setup_spans_debug_check(psx_gpu_struct *psx_gpu,
  edge_data_struct *span_edge_data_element)
{
  u32 _num_spans = span_edge_data_element - psx_gpu->span_edge_data;
  if (_num_spans > MAX_SPANS)
    *(volatile int *)0 = 1;
  if (_num_spans < psx_gpu->num_spans)
  {
    if(span_edge_data_element->num_blocks > MAX_BLOCKS_PER_ROW)
      *(volatile int *)0 = 2;
    if(span_edge_data_element->y >= 2048)
      *(volatile int *)0 = 3;
  }
}
#else
#define setup_spans_debug_check(psx_gpu, span_edge_data_element)
#endif

#define setup_spans_prologue_alternate_yes()                                   \
  vec_2x64s alternate_x;                                                       \
  vec_2x64s alternate_dx_dy;                                                   \
  vec_4x32s alternate_x_32;                                                    \
  vec_4x16u alternate_x_16;                                                    \
                                                                               \
  vec_4x16u alternate_select;                                                  \
  vec_4x16s y_mid_point;                                                       \
                                                                               \
  s32 y_b = v_b->y;                                                            \
  s64 edge_alt;                                                                \
  s32 edge_dx_dy_alt;                                                          \
  u32 edge_shift_alt                                                           \

#define setup_spans_prologue_alternate_no()                                    \

#define setup_spans_prologue(alternate_active)                                 \
  edge_data_struct *span_edge_data;                                            \
  vec_4x32u *span_uvrg_offset;                                                 \
  u32 *span_b_offset;                                                          \
                                                                               \
  s32 clip;                                                                    \
                                                                               \
  vec_2x64s edges_xy;                                                          \
  vec_2x32s edges_dx_dy;                                                       \
  vec_2x32u edge_shifts;                                                       \
                                                                               \
  vec_2x64s left_x, right_x;                                                   \
  vec_2x64s left_dx_dy, right_dx_dy;                                           \
  vec_4x32s left_x_32, right_x_32;                                             \
  vec_8x16s left_right_x_16;                                                   \
  vec_4x16s y_x4;                                                              \
  vec_8x16s left_edge;                                                         \
  vec_8x16s right_edge;                                                        \
  vec_4x16u span_shift;                                                        \
                                                                               \
  vec_2x32u c_0x01;                                                            \
  vec_4x16u c_0x04;                                                            \
  vec_4x16u c_0xFFFE;                                                          \
  vec_4x16u c_0x07;                                                            \
                                                                               \
  vec_2x32s x_starts;                                                          \
  vec_2x32s x_ends;                                                            \
                                                                               \
  s32 x_a = v_a->x;                                                            \
  s32 x_b = v_b->x;                                                            \
  s32 x_c = v_c->x;                                                            \
  s32 y_a = v_a->y;                                                            \
  s32 y_c = v_c->y;                                                            \
                                                                               \
  vec_4x32u uvrg = psx_gpu->uvrg;                                              \
  vec_4x32u uvrg_dy = psx_gpu->uvrg_dy;                                        \
  u32 b = psx_gpu->b;                                                          \
  u32 b_dy = psx_gpu->b_dy;                                                    \
                                                                               \
  dup_2x32b(c_0x01, 0x01);                                                     \
  setup_spans_prologue_alternate_##alternate_active()                          \

#define setup_spans_prologue_b()                                               \
  span_edge_data = psx_gpu->span_edge_data;                                    \
  span_uvrg_offset = psx_gpu->span_uvrg_offset;                                \
  span_b_offset = psx_gpu->span_b_offset;                                      \
                                                                               \
  vec_8x16u c_0x0001;                                                          \
  vec_4x16u c_max_blocks_per_row;                                              \
                                                                               \
  dup_8x16b(c_0x0001, 0x0001);                                                 \
  dup_8x16b(left_edge, psx_gpu->viewport_start_x);                             \
  dup_8x16b(right_edge, psx_gpu->viewport_end_x);                              \
  add_8x16b(right_edge, right_edge, c_0x0001);                                 \
  dup_4x16b(c_0x04, 0x04);                                                     \
  dup_4x16b(c_0x07, 0x07);                                                     \
  dup_4x16b(c_0xFFFE, 0xFFFE);                                                 \
  dup_4x16b(c_max_blocks_per_row, MAX_BLOCKS_PER_ROW);                         \


#define compute_edge_delta_x2()                                                \
{                                                                              \
  vec_2x32s heights;                                                           \
  vec_2x32s height_reciprocals;                                                \
  vec_2x32s heights_b;                                                         \
  vec_4x32u widths;                                                            \
                                                                               \
  u32 edge_shift = reciprocal_table[height];                                   \
                                                                               \
  dup_2x32b(heights, height);                                                  \
  sub_2x32b(widths, x_ends, x_starts);                                         \
                                                                               \
  dup_2x32b(edge_shifts, edge_shift);                                          \
  sub_2x32b(heights_b, heights, c_0x01);                                       \
  shr_2x32b(height_reciprocals, edge_shifts, 10);                              \
                                                                               \
  mla_2x32b(heights_b, x_starts, heights);                                     \
  bic_immediate_4x16b(vector_cast(vec_4x16u, edge_shifts), 0xE0);              \
  mul_2x32b(edges_dx_dy, widths, height_reciprocals);                          \
  mul_long_2x32b(edges_xy, heights_b, height_reciprocals);                     \
}                                                                              \

#define compute_edge_delta_x3(start_c, height_a, height_b)                     \
{                                                                              \
  vec_2x32s heights;                                                           \
  vec_2x32s height_reciprocals;                                                \
  vec_2x32s heights_b;                                                         \
  vec_2x32u widths;                                                            \
                                                                               \
  u32 width_alt;                                                               \
  s32 height_b_alt;                                                            \
  u32 height_reciprocal_alt;                                                   \
                                                                               \
  heights.e[0] = height_a;                                                     \
  heights.e[1] = height_b;                                                     \
                                                                               \
  edge_shifts.e[0] = reciprocal_table[height_a];                               \
  edge_shifts.e[1] = reciprocal_table[height_b];                               \
  edge_shift_alt = reciprocal_table[height_minor_b];                           \
                                                                               \
  sub_2x32b(widths, x_ends, x_starts);                                         \
  width_alt = x_c - start_c;                                                   \
                                                                               \
  shr_2x32b(height_reciprocals, edge_shifts, 10);                              \
  height_reciprocal_alt = edge_shift_alt >> 10;                                \
                                                                               \
  bic_immediate_4x16b(vector_cast(vec_4x16u, edge_shifts), 0xE0);              \
  edge_shift_alt &= 0x1F;                                                      \
                                                                               \
  sub_2x32b(heights_b, heights, c_0x01);                                       \
  height_b_alt = height_minor_b - 1;                                           \
                                                                               \
  mla_2x32b(heights_b, x_starts, heights);                                     \
  height_b_alt += height_minor_b * start_c;                                    \
                                                                               \
  mul_long_2x32b(edges_xy, heights_b, height_reciprocals);                     \
  edge_alt = (s64)height_b_alt * height_reciprocal_alt;                        \
                                                                               \
  mul_2x32b(edges_dx_dy, widths, height_reciprocals);                          \
  edge_dx_dy_alt = width_alt * height_reciprocal_alt;                          \
}                                                                              \


#define setup_spans_adjust_y_up()                                              \
  sub_4x32b(y_x4, y_x4, c_0x04)                                                \

#define setup_spans_adjust_y_down()                                            \
  add_4x32b(y_x4, y_x4, c_0x04)                                                \

#define setup_spans_adjust_interpolants_up()                                   \
  sub_4x32b(uvrg, uvrg, uvrg_dy);                                              \
  b -= b_dy                                                                    \

#define setup_spans_adjust_interpolants_down()                                 \
  add_4x32b(uvrg, uvrg, uvrg_dy);                                              \
  b += b_dy                                                                    \


#define setup_spans_clip_interpolants_increment()                              \
  mla_scalar_4x32b(uvrg, uvrg_dy, clip);                                       \
  b += b_dy * clip                                                             \

#define setup_spans_clip_interpolants_decrement()                              \
  mls_scalar_4x32b(uvrg, uvrg_dy, clip);                                       \
  b -= b_dy * clip                                                             \

#define setup_spans_clip_alternate_yes()                                       \
  edge_alt += edge_dx_dy_alt * (s64)(clip)                                     \

#define setup_spans_clip_alternate_no()                                        \

#define setup_spans_clip(direction, alternate_active)                          \
{                                                                              \
  clipped_triangles++;                                                         \
  mla_scalar_long_2x32b(edges_xy, edges_dx_dy, (s64)clip);                     \
  setup_spans_clip_alternate_##alternate_active();                             \
  setup_spans_clip_interpolants_##direction();                                 \
}                                                                              \


#define setup_spans_adjust_edges_alternate_no(left_index, right_index)         \
{                                                                              \
  vec_2x64u edge_shifts_64;                                                    \
  vec_2x64s edges_dx_dy_64;                                                    \
                                                                               \
  mov_wide_2x32b(edge_shifts_64, edge_shifts);                                 \
  shl_variable_2x64b(edges_xy, edges_xy, edge_shifts_64);                      \
                                                                               \
  mov_wide_2x32b(edges_dx_dy_64, edges_dx_dy);                                 \
  shl_variable_2x64b(edges_dx_dy_64, edges_dx_dy_64, edge_shifts_64);          \
                                                                               \
  left_x.e[0] = edges_xy.e[left_index];                                        \
  right_x.e[0] = edges_xy.e[right_index];                                      \
                                                                               \
  left_dx_dy.e[0] = edges_dx_dy_64.e[left_index];                              \
  left_dx_dy.e[1] = edges_dx_dy_64.e[left_index];                              \
  right_dx_dy.e[0] = edges_dx_dy_64.e[right_index];                            \
  right_dx_dy.e[1] = edges_dx_dy_64.e[right_index];                            \
                                                                               \
  add_1x64b(left_x.high, left_x.low, left_dx_dy.low);                          \
  add_1x64b(right_x.high, right_x.low, right_dx_dy.low);                       \
                                                                               \
  add_2x64b(left_dx_dy, left_dx_dy, left_dx_dy);                               \
  add_2x64b(right_dx_dy, right_dx_dy, right_dx_dy);                            \
}                                                                              \

#define setup_spans_adjust_edges_alternate_yes(left_index, right_index)        \
{                                                                              \
  setup_spans_adjust_edges_alternate_no(left_index, right_index);              \
  s64 edge_dx_dy_alt_64;                                                       \
                                                                               \
  dup_4x16b(y_mid_point, y_b);                                                 \
                                                                               \
  edge_alt <<= edge_shift_alt;                                                 \
  edge_dx_dy_alt_64 = (s64)edge_dx_dy_alt << edge_shift_alt;                   \
                                                                               \
  alternate_x.e[0] = edge_alt;                                                 \
  alternate_dx_dy.e[0] = edge_dx_dy_alt_64;                                    \
  alternate_dx_dy.e[1] = edge_dx_dy_alt_64;                                    \
                                                                               \
  add_1x64b(alternate_x.high, alternate_x.low, alternate_dx_dy.low);           \
  add_2x64b(alternate_dx_dy, alternate_dx_dy, alternate_dx_dy);                \
}                                                                              \


#define setup_spans_y_select_up()                                              \
  cmplt_4x16b(alternate_select, y_x4, y_mid_point)                             \

#define setup_spans_y_select_down()                                            \
  cmpgt_4x16b(alternate_select, y_x4, y_mid_point)                             \

#define setup_spans_y_select_alternate_yes(direction)                          \
  setup_spans_y_select_##direction()                                           \

#define setup_spans_y_select_alternate_no(direction)                           \

#define setup_spans_alternate_select_left()                                    \
  bit_4x16b(left_right_x_16.low, alternate_x_16, alternate_select)             \

#define setup_spans_alternate_select_right()                                   \
  bit_4x16b(left_right_x_16.high, alternate_x_16, alternate_select)            \

#define setup_spans_alternate_select_none()                                    \

#define setup_spans_increment_alternate_yes()                                  \
  shr_narrow_2x64b(alternate_x_32.low, alternate_x, 32);                       \
  add_2x64b(alternate_x, alternate_x, alternate_dx_dy);                        \
  shr_narrow_2x64b(alternate_x_32.high, alternate_x, 32);                      \
  add_2x64b(alternate_x, alternate_x, alternate_dx_dy);                        \
  mov_narrow_4x32b(alternate_x_16, alternate_x_32)                             \

#define setup_spans_increment_alternate_no()                                   \

#define setup_spans_set_x4(alternate, direction, alternate_active)             \
{                                                                              \
  span_uvrg_offset[0] = uvrg;                                                  \
  span_b_offset[0] = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  span_uvrg_offset[1] = uvrg;                                                  \
  span_b_offset[1] = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  span_uvrg_offset[2] = uvrg;                                                  \
  span_b_offset[2] = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  span_uvrg_offset[3] = uvrg;                                                  \
  span_b_offset[3] = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  span_uvrg_offset += 4;                                                       \
  span_b_offset += 4;                                                          \
                                                                               \
  shr_narrow_2x64b(left_x_32.low, left_x, 32);                                 \
  shr_narrow_2x64b(right_x_32.low, right_x, 32);                               \
                                                                               \
  add_2x64b(left_x, left_x, left_dx_dy);                                       \
  add_2x64b(right_x, right_x, right_dx_dy);                                    \
                                                                               \
  shr_narrow_2x64b(left_x_32.high, left_x, 32);                                \
  shr_narrow_2x64b(right_x_32.high, right_x, 32);                              \
                                                                               \
  add_2x64b(left_x, left_x, left_dx_dy);                                       \
  add_2x64b(right_x, right_x, right_dx_dy);                                    \
                                                                               \
  mov_narrow_4x32b(left_right_x_16.low, left_x_32);                            \
  mov_narrow_4x32b(left_right_x_16.high, right_x_32);                          \
                                                                               \
  setup_spans_increment_alternate_##alternate_active();                        \
  setup_spans_y_select_alternate_##alternate_active(direction);                \
  setup_spans_alternate_select_##alternate();                                  \
                                                                               \
  max_8x16b(left_right_x_16, left_right_x_16, left_edge);                      \
  min_8x16b(left_right_x_16, left_right_x_16, right_edge);                     \
                                                                               \
  sub_4x16b(left_right_x_16.high, left_right_x_16.high, left_right_x_16.low);  \
  add_4x16b(left_right_x_16.high, left_right_x_16.high, c_0x07);               \
  and_4x16b(span_shift, left_right_x_16.high, c_0x07);                         \
  shl_variable_4x16b(span_shift, c_0xFFFE, span_shift);                        \
  shr_4x16b(left_right_x_16.high, left_right_x_16.high, 3);                    \
  min_4x16b(left_right_x_16.high, left_right_x_16.high, c_max_blocks_per_row); \
                                                                               \
  u32 i;                                                                       \
  for(i = 0; i < 4; i++)                                                       \
  {                                                                            \
    span_edge_data[i].left_x = left_right_x_16.low.e[i];                       \
    span_edge_data[i].num_blocks = left_right_x_16.high.e[i];                  \
    span_edge_data[i].right_mask = span_shift.e[i];                            \
    span_edge_data[i].y = y_x4.e[i];                                           \
    setup_spans_debug_check(psx_gpu, &span_edge_data[i]);                      \
  }                                                                            \
                                                                               \
  span_edge_data += 4;                                                         \
                                                                               \
  setup_spans_adjust_y_##direction();                                          \
}                                                                              \


#define setup_spans_alternate_adjust_yes()                                     \
  edge_alt -= edge_dx_dy_alt * (s64)height_minor_a                             \

#define setup_spans_alternate_adjust_no()                                      \


#define setup_spans_down(left_index, right_index, alternate, alternate_active) \
  setup_spans_alternate_adjust_##alternate_active();                           \
  if(y_c > psx_gpu->viewport_end_y)                                            \
    height -= y_c - psx_gpu->viewport_end_y - 1;                               \
                                                                               \
  clip = psx_gpu->viewport_start_y - y_a;                                      \
  if(clip > 0)                                                                 \
  {                                                                            \
    height -= clip;                                                            \
    y_a += clip;                                                               \
    setup_spans_clip(increment, alternate_active);                             \
  }                                                                            \
                                                                               \
  setup_spans_prologue_b();                                                    \
                                                                               \
  if (height > 512)                                                            \
    height = 512;                                                              \
  if (height > 0)                                                              \
  {                                                                            \
    y_x4.e[0] = y_a;                                                           \
    y_x4.e[1] = y_a + 1;                                                       \
    y_x4.e[2] = y_a + 2;                                                       \
    y_x4.e[3] = y_a + 3;                                                       \
    setup_spans_adjust_edges_alternate_##alternate_active(left_index,          \
     right_index);                                                             \
                                                                               \
    psx_gpu->num_spans = height;                                               \
    do                                                                         \
    {                                                                          \
      setup_spans_set_x4(alternate, down, alternate_active);                   \
      height -= 4;                                                             \
    } while(height > 0);                                                       \
    if (psx_gpu->hacks_active & (AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V))   \
      span_uvrg_offset[height - 1].low = span_uvrg_offset[height - 2].low;     \
  }                                                                            \


#define setup_spans_alternate_pre_increment_yes()                              \
  edge_alt += edge_dx_dy_alt                                                   \

#define setup_spans_alternate_pre_increment_no()                               \

#define setup_spans_up_decrement_height_yes()                                  \
  height--                                                                     \

#define setup_spans_up_decrement_height_no()                                   \
  {}                                                                           \

#define setup_spans_up(left_index, right_index, alternate, alternate_active)   \
  setup_spans_alternate_adjust_##alternate_active();                           \
  y_a--;                                                                       \
                                                                               \
  if(y_c < psx_gpu->viewport_start_y)                                          \
    height -= psx_gpu->viewport_start_y - y_c;                                 \
  else                                                                         \
    setup_spans_up_decrement_height_##alternate_active();                      \
                                                                               \
  clip = y_a - psx_gpu->viewport_end_y;                                        \
  if(clip > 0)                                                                 \
  {                                                                            \
    height -= clip;                                                            \
    y_a -= clip;                                                               \
    setup_spans_clip(decrement, alternate_active);                             \
  }                                                                            \
                                                                               \
  setup_spans_prologue_b();                                                    \
                                                                               \
  if (height > 512)                                                            \
    height = 512;                                                              \
  if (height > 0)                                                              \
  {                                                                            \
    y_x4.e[0] = y_a;                                                           \
    y_x4.e[1] = y_a - 1;                                                       \
    y_x4.e[2] = y_a - 2;                                                       \
    y_x4.e[3] = y_a - 3;                                                       \
    add_wide_2x32b(edges_xy, edges_xy, edges_dx_dy);                           \
    setup_spans_alternate_pre_increment_##alternate_active();                  \
    setup_spans_adjust_edges_alternate_##alternate_active(left_index,          \
     right_index);                                                             \
    setup_spans_adjust_interpolants_up();                                      \
                                                                               \
    psx_gpu->num_spans = height;                                               \
    while(height > 0)                                                          \
    {                                                                          \
      setup_spans_set_x4(alternate, up, alternate_active);                     \
      height -= 4;                                                             \
    }                                                                          \
    if (psx_gpu->hacks_active & AHACK_TEXTURE_ADJ_V)                           \
      psx_gpu->span_uvrg_offset[0].low = psx_gpu->span_uvrg_offset[1].low;     \
  }                                                                            \

#define index_left  0
#define index_right 1

#define setup_spans_up_up(minor, major)                                        \
  setup_spans_prologue(yes);                                                   \
  s32 height_minor_a = y_a - y_b;                                              \
  s32 height_minor_b = y_b - y_c;                                              \
  s32 height = y_a - y_c;                                                      \
                                                                               \
  dup_2x32b(x_starts, x_a);                                                    \
  x_ends.e[0] = x_c;                                                           \
  x_ends.e[1] = x_b;                                                           \
                                                                               \
  compute_edge_delta_x3(x_b, height, height_minor_a);                          \
  setup_spans_up(index_##major, index_##minor, minor, yes)                     \


#ifndef NEON_BUILD

void setup_spans_up_left(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_up_up(left, right);
}

void setup_spans_up_right(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_up_up(right, left);
}

#define setup_spans_down_down(minor, major)                                    \
  setup_spans_prologue(yes);                                                   \
  s32 height_minor_a = y_b - y_a;                                              \
  s32 height_minor_b = y_c - y_b;                                              \
  s32 height = y_c - y_a;                                                      \
                                                                               \
  dup_2x32b(x_starts, x_a);                                                    \
  x_ends.e[0] = x_c;                                                           \
  x_ends.e[1] = x_b;                                                           \
                                                                               \
  compute_edge_delta_x3(x_b, height, height_minor_a);                          \
  setup_spans_down(index_##major, index_##minor, minor, yes)                   \

void setup_spans_down_left(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_down_down(left, right);
}

void setup_spans_down_right(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_down_down(right, left);
}

#define setup_spans_up_flat()                                                  \
  s32 height = y_a - y_c;                                                      \
                                                                               \
  flat_triangles++;                                                            \
  compute_edge_delta_x2();                                                     \
  setup_spans_up(index_left, index_right, none, no)                            \

void setup_spans_up_a(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_prologue(no);
  x_starts.e[0] = x_a;
  x_starts.e[1] = x_b;
  dup_2x32b(x_ends, x_c);

  setup_spans_up_flat();
}

void setup_spans_up_b(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_prologue(no);
  dup_2x32b(x_starts, x_a);
  x_ends.e[0] = x_b;
  x_ends.e[1] = x_c;

  setup_spans_up_flat();
}

#define setup_spans_down_flat()                                                \
  s32 height = y_c - y_a;                                                      \
                                                                               \
  flat_triangles++;                                                            \
  compute_edge_delta_x2();                                                     \
  setup_spans_down(index_left, index_right, none, no)                          \

void setup_spans_down_a(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_prologue(no);
  x_starts.e[0] = x_a;
  x_starts.e[1] = x_b;
  dup_2x32b(x_ends, x_c);

  setup_spans_down_flat();
}

void setup_spans_down_b(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_prologue(no);
  dup_2x32b(x_starts, x_a);
  x_ends.e[0] = x_b;
  x_ends.e[1] = x_c;

  setup_spans_down_flat();
}

void setup_spans_up_down(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
  setup_spans_prologue(no);

  s32 y_b = v_b->y;
  s64 edge_alt;
  s32 edge_dx_dy_alt;
  u32 edge_shift_alt;

  s32 middle_y = y_a;
  s32 height_minor_a = y_a - y_b;
  s32 height_minor_b = y_c - y_a;
  s32 height_major = y_c - y_b;

  vec_2x64s edges_xy_b;
  vec_2x32s edges_dx_dy_b;
  vec_2x32u edge_shifts_b;

  vec_2x32s height_increment;

  x_starts.e[0] = x_a;
  x_starts.e[1] = x_c;
  dup_2x32b(x_ends, x_b);

  compute_edge_delta_x3(x_a, height_minor_a, height_major);

  height_increment.e[0] = 0;
  height_increment.e[1] = height_minor_b;

  mla_long_2x32b(edges_xy, edges_dx_dy, height_increment);

  edges_xy_b.e[0] = edge_alt;
  edges_xy_b.e[1] = edges_xy.e[1];

  edge_shifts_b = edge_shifts;
  edge_shifts_b.e[0] = edge_shift_alt;

  neg_2x32b(edges_dx_dy_b, edges_dx_dy);
  edges_dx_dy_b.e[0] = edge_dx_dy_alt;
  
  y_a--;

  if(y_b < psx_gpu->viewport_start_y)
    height_minor_a -= psx_gpu->viewport_start_y - y_b;

  clip = y_a - psx_gpu->viewport_end_y;
  if(clip > 0)
  {
    height_minor_a -= clip;
    y_a -= clip;
    setup_spans_clip(decrement, no);
  }

  setup_spans_prologue_b();

  if (height_minor_a > 512)
    height_minor_a = 512;
  if (height_minor_a > 0)
  {
    y_x4.e[0] = y_a;
    y_x4.e[1] = y_a - 1;
    y_x4.e[2] = y_a - 2;
    y_x4.e[3] = y_a - 3;
    add_wide_2x32b(edges_xy, edges_xy, edges_dx_dy);
    setup_spans_adjust_edges_alternate_no(index_left, index_right);
    setup_spans_adjust_interpolants_up();

    psx_gpu->num_spans = height_minor_a;
    while(height_minor_a > 0)
    {
      setup_spans_set_x4(none, up, no);
      height_minor_a -= 4;
    }

    span_edge_data += height_minor_a;
    span_uvrg_offset += height_minor_a;
    span_b_offset += height_minor_a;
  }
  
  edges_xy = edges_xy_b;
  edges_dx_dy = edges_dx_dy_b;
  edge_shifts = edge_shifts_b;

  uvrg = psx_gpu->uvrg;
  b = psx_gpu->b;

  y_a = middle_y;

  if(y_c > psx_gpu->viewport_end_y)
    height_minor_b -= y_c - psx_gpu->viewport_end_y - 1;

  clip = psx_gpu->viewport_start_y - y_a;
  if(clip > 0)
  {
    height_minor_b -= clip;
    y_a += clip;
    setup_spans_clip(increment, no);
  }

  if (height_minor_b > 512)
    height_minor_b = 512;
  if (height_minor_b > 0)
  {
    y_x4.e[0] = y_a;
    y_x4.e[1] = y_a + 1;
    y_x4.e[2] = y_a + 2;
    y_x4.e[3] = y_a + 3;
    setup_spans_adjust_edges_alternate_no(index_left, index_right);

    // FIXME: overflow corner case
    if(psx_gpu->num_spans + height_minor_b == MAX_SPANS)
      height_minor_b &= ~3;

    psx_gpu->num_spans += height_minor_b;
    while(height_minor_b > 0)
    {
      setup_spans_set_x4(none, down, no);
      height_minor_b -= 4;
    }
    if (psx_gpu->hacks_active & (AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V))
    {
      span_uvrg_offset[height_minor_b - 1].low =
        span_uvrg_offset[height_minor_b - 2].low;
    }
  }

  left_split_triangles++;
}

#endif

// this is some hacky mess, can this be improved somehow?
// ideally change things to not have to do this hack at all
void __attribute__((noinline))
setup_blocks_uv_adj_hack(psx_gpu_struct *psx_gpu, block_struct *block,
    edge_data_struct *span_edge_data, vec_4x32u *span_uvrg_offset)
{
  size_t span_i = span_uvrg_offset - psx_gpu->span_uvrg_offset;
  if (span_i != 0 && span_i != psx_gpu->num_spans - 1
      && !(psx_gpu->hacks_active & AHACK_TEXTURE_ADJ_U))
    return;
  u32 num_blocks = span_edge_data->num_blocks - 1;
  s32 offset = __builtin_ctz(span_edge_data->right_mask | 0x100) - 1;
  s32 toffset = 8 * num_blocks + offset - 1;
  if (toffset < 0 && !(psx_gpu->hacks_active & AHACK_TEXTURE_ADJ_U))
    return;

  toffset += span_edge_data->left_x;
  s32 u_dx = psx_gpu->uvrg_dx.low.e[0];
  s32 v_dx = psx_gpu->uvrg_dx.low.e[1];
  u32 u = span_uvrg_offset->low.e[0];
  u32 v = span_uvrg_offset->low.e[1];
  u += u_dx * toffset;
  v += v_dx * toffset;
  u = (u >> 16) & psx_gpu->texture_mask_width;
  v = (v >> 16) & psx_gpu->texture_mask_height;
  if (!(psx_gpu->render_state_base & (TEXTURE_MODE_16BPP << 8))) {
    // 4bpp 8bpp are swizzled
    u32 u_ = u;
    u = (u & 0x0f) | ((v & 0x0f) << 4);
    v = (v & 0xf0) | (u_ >> 4);
  }
  assert(offset >= 0);
  //assert(block->uv.e[offset] == ((v << 8) | u));
  block->uv.e[offset] = (v << 8) | u;
}

#define dither_table_entry_normal(value)                                       \
  (value)                                                                      \


#define setup_blocks_load_msb_mask_indirect()                                  \

#define setup_blocks_load_msb_mask_direct()                                    \
  vec_8x16u msb_mask;                                                          \
  dup_8x16b(msb_mask, psx_gpu->mask_msb);                                      \


#define setup_blocks_variables_shaded_textured(target)                         \
  vec_4x32u u_block;                                                           \
  vec_4x32u v_block;                                                           \
  vec_4x32u r_block;                                                           \
  vec_4x32u g_block;                                                           \
  vec_4x32u b_block;                                                           \
  vec_4x32u uvrg_dx = psx_gpu->uvrg_dx;                                        \
  vec_4x32u uvrg_dx4;                                                          \
  vec_4x32u uvrg_dx8;                                                          \
  vec_4x32u uvrg;                                                              \
  u32 b_dx = psx_gpu->b_block_span.e[1];                                       \
  u32 b_dx4 = b_dx << 2;                                                       \
  u32 b_dx8 = b_dx << 3;                                                       \
  u32 b;                                                                       \
                                                                               \
  vec_16x8u texture_mask;                                                      \
  shl_4x32b(uvrg_dx4, uvrg_dx, 2);                                             \
  shl_4x32b(uvrg_dx8, uvrg_dx, 3);                                             \
  dup_8x8b(texture_mask.low, psx_gpu->texture_mask_width);                     \
  dup_8x8b(texture_mask.high, psx_gpu->texture_mask_height)                    \

#define setup_blocks_variables_shaded_untextured(target)                       \
  vec_4x32u r_block;                                                           \
  vec_4x32u g_block;                                                           \
  vec_4x32u b_block;                                                           \
  vec_4x32u rgb_dx;                                                            \
  vec_4x32u rgb_dx4;                                                           \
  vec_4x32u rgb_dx8;                                                           \
  vec_4x32u rgb;                                                               \
                                                                               \
  vec_8x8u d64_0x07;                                                           \
  vec_8x8u d64_1;                                                              \
  vec_8x8u d64_4;                                                              \
  vec_8x8u d64_128;                                                            \
                                                                               \
  dup_8x8b(d64_0x07, 0x07);                                                    \
  dup_8x8b(d64_1, 1);                                                          \
  dup_8x8b(d64_4, 4);                                                          \
  dup_8x8b(d64_128, 128);                                                      \
                                                                               \
  rgb_dx.low = psx_gpu->uvrg_dx.high;                                          \
  rgb_dx.e[2] = psx_gpu->b_block_span.e[1];                                    \
  shl_4x32b(rgb_dx4, rgb_dx, 2);                                               \
  shl_4x32b(rgb_dx8, rgb_dx, 3)                                                \

#define setup_blocks_variables_unshaded_textured(target)                       \
  vec_4x32u u_block;                                                           \
  vec_4x32u v_block;                                                           \
  vec_2x32u uv_dx = psx_gpu->uvrg_dx.low;                                      \
  vec_2x32u uv_dx4;                                                            \
  vec_2x32u uv_dx8;                                                            \
  vec_2x32u uv = psx_gpu->uvrg.low;                                            \
                                                                               \
  vec_16x8u texture_mask;                                                      \
  shl_2x32b(uv_dx4, uv_dx, 2);                                                 \
  shl_2x32b(uv_dx8, uv_dx, 3);                                                 \
  dup_8x8b(texture_mask.low, psx_gpu->texture_mask_width);                     \
  dup_8x8b(texture_mask.high, psx_gpu->texture_mask_height)                    \


#define setup_blocks_variables_unshaded_untextured_direct()                    \
  or_8x16b(colors, colors, msb_mask)                                           \

#define setup_blocks_variables_unshaded_untextured_indirect()                  \

#define setup_blocks_variables_unshaded_untextured(target)                     \
  u32 color = psx_gpu->triangle_color;                                         \
  vec_8x16u colors;                                                            \
                                                                               \
  u32 color_r = color & 0xFF;                                                  \
  u32 color_g = (color >> 8) & 0xFF;                                           \
  u32 color_b = (color >> 16) & 0xFF;                                          \
                                                                               \
  color = (color_r >> 3) | ((color_g >> 3) << 5) |                             \
   ((color_b >> 3) << 10);                                                     \
  dup_8x16b(colors, color);                                                    \
  setup_blocks_variables_unshaded_untextured_##target()                        \

#define setup_blocks_span_initialize_dithered_textured()                       \
  vec_8x16u dither_offsets;                                                    \
  shl_long_8x8b(dither_offsets, dither_offsets_short, 4)                       \

#define setup_blocks_span_initialize_dithered_untextured()                     \
  vec_8x8u dither_offsets;                                                     \
  add_8x8b(dither_offsets, dither_offsets_short, d64_4)                        \

#define setup_blocks_span_initialize_dithered(texturing)                       \
  u32 dither_row = psx_gpu->dither_table[y & 0x3];                             \
  u32 dither_shift = (span_edge_data->left_x & 0x3) * 8;                       \
  vec_8x8s dither_offsets_short;                                               \
                                                                               \
  dither_row =                                                                 \
   (dither_row >> dither_shift) | ((u64)dither_row << (32 - dither_shift));    \
  dup_2x32b(vector_cast(vec_2x32u, dither_offsets_short), dither_row);         \
  setup_blocks_span_initialize_dithered_##texturing()                          \

#define setup_blocks_span_initialize_undithered(texturing)                     \


#define setup_blocks_span_initialize_shaded_textured()                         \
{                                                                              \
  vec_4x32u block_span;                                                        \
  u32 offset = span_edge_data->left_x;                                         \
                                                                               \
  uvrg = *span_uvrg_offset;                                                    \
  mla_scalar_4x32b(uvrg, uvrg_dx, offset);                                     \
  b = *span_b_offset;                                                          \
  b += b_dx * offset;                                                          \
                                                                               \
  dup_4x32b(u_block, uvrg.e[0]);                                               \
  dup_4x32b(v_block, uvrg.e[1]);                                               \
  dup_4x32b(r_block, uvrg.e[2]);                                               \
  dup_4x32b(g_block, uvrg.e[3]);                                               \
  dup_4x32b(b_block, b);                                                       \
                                                                               \
  block_span = psx_gpu->u_block_span;                                          \
  add_4x32b(u_block, u_block, block_span);                                     \
  block_span = psx_gpu->v_block_span;                                          \
  add_4x32b(v_block, v_block, block_span);                                     \
  block_span = psx_gpu->r_block_span;                                          \
  add_4x32b(r_block, r_block, block_span);                                     \
  block_span = psx_gpu->g_block_span;                                          \
  add_4x32b(g_block, g_block, block_span);                                     \
  block_span = psx_gpu->b_block_span;                                          \
  add_4x32b(b_block, b_block, block_span);                                     \
}
  
#define setup_blocks_span_initialize_shaded_untextured()                       \
{                                                                              \
  vec_4x32u block_span;                                                        \
  u32 offset = span_edge_data->left_x;                                         \
                                                                               \
  rgb.low = span_uvrg_offset->high;                                            \
  rgb.high.e[0] = *span_b_offset;                                              \
  mla_scalar_4x32b(rgb, rgb_dx, offset);                                       \
                                                                               \
  dup_4x32b(r_block, rgb.e[0]);                                                \
  dup_4x32b(g_block, rgb.e[1]);                                                \
  dup_4x32b(b_block, rgb.e[2]);                                                \
                                                                               \
  block_span = psx_gpu->r_block_span;                                          \
  add_4x32b(r_block, r_block, block_span);                                     \
  block_span = psx_gpu->g_block_span;                                          \
  add_4x32b(g_block, g_block, block_span);                                     \
  block_span = psx_gpu->b_block_span;                                          \
  add_4x32b(b_block, b_block, block_span);                                     \
}                                                                              \
  
#define setup_blocks_span_initialize_unshaded_textured()                       \
{                                                                              \
  vec_4x32u block_span;                                                        \
  u32 offset = span_edge_data->left_x;                                         \
                                                                               \
  uv = span_uvrg_offset->low;                                                  \
  mla_scalar_2x32b(uv, uv_dx, offset);                                         \
                                                                               \
  dup_4x32b(u_block, uv.e[0]);                                                 \
  dup_4x32b(v_block, uv.e[1]);                                                 \
                                                                               \
  block_span = psx_gpu->u_block_span;                                          \
  add_4x32b(u_block, u_block, block_span);                                     \
  block_span = psx_gpu->v_block_span;                                          \
  add_4x32b(v_block, v_block, block_span);                                     \
}                                                                              \

#define setup_blocks_span_initialize_unshaded_untextured()                     \


#define setup_blocks_texture_swizzled()                                        \
{                                                                              \
  vec_8x8u u_saved = u;                                                        \
  sli_8x8b(u, v, 4);                                                           \
  sri_8x8b(v, u_saved, 4);                                                     \
}                                                                              \

#define setup_blocks_texture_unswizzled()                                      \

#define setup_blocks_store_shaded_textured(swizzling, dithering, target,       \
 edge_type)                                                                    \
{                                                                              \
  vec_8x16u u_whole;                                                           \
  vec_8x16u v_whole;                                                           \
  vec_8x16u r_whole;                                                           \
  vec_8x16u g_whole;                                                           \
  vec_8x16u b_whole;                                                           \
                                                                               \
  vec_8x8u u;                                                                  \
  vec_8x8u v;                                                                  \
  vec_8x8u r;                                                                  \
  vec_8x8u g;                                                                  \
  vec_8x8u b;                                                                  \
  vec_8x16u uv;                                                                \
                                                                               \
  vec_4x32u dx4;                                                               \
  vec_4x32u dx8;                                                               \
                                                                               \
  shr_narrow_4x32b(u_whole.low, u_block, 16);                                  \
  shr_narrow_4x32b(v_whole.low, v_block, 16);                                  \
  shr_narrow_4x32b(r_whole.low, r_block, 16);                                  \
  shr_narrow_4x32b(g_whole.low, g_block, 16);                                  \
  shr_narrow_4x32b(b_whole.low, b_block, 16);                                  \
                                                                               \
  dup_4x32b(dx4, uvrg_dx4.e[0]);                                               \
  add_high_narrow_4x32b(u_whole.high, u_block, dx4);                           \
  dup_4x32b(dx4, uvrg_dx4.e[1]);                                               \
  add_high_narrow_4x32b(v_whole.high, v_block, dx4);                           \
  dup_4x32b(dx4, uvrg_dx4.e[2]);                                               \
  add_high_narrow_4x32b(r_whole.high, r_block, dx4);                           \
  dup_4x32b(dx4, uvrg_dx4.e[3]);                                               \
  add_high_narrow_4x32b(g_whole.high, g_block, dx4);                           \
  dup_4x32b(dx4, b_dx4);                                                       \
  add_high_narrow_4x32b(b_whole.high, b_block, dx4);                           \
                                                                               \
  mov_narrow_8x16b(u, u_whole);                                                \
  mov_narrow_8x16b(v, v_whole);                                                \
  mov_narrow_8x16b(r, r_whole);                                                \
  mov_narrow_8x16b(g, g_whole);                                                \
  mov_narrow_8x16b(b, b_whole);                                                \
                                                                               \
  dup_4x32b(dx8, uvrg_dx8.e[0]);                                               \
  add_4x32b(u_block, u_block, dx8);                                            \
  dup_4x32b(dx8, uvrg_dx8.e[1]);                                               \
  add_4x32b(v_block, v_block, dx8);                                            \
  dup_4x32b(dx8, uvrg_dx8.e[2]);                                               \
  add_4x32b(r_block, r_block, dx8);                                            \
  dup_4x32b(dx8, uvrg_dx8.e[3]);                                               \
  add_4x32b(g_block, g_block, dx8);                                            \
  dup_4x32b(dx8, b_dx8);                                                       \
  add_4x32b(b_block, b_block, dx8);                                            \
                                                                               \
  and_8x8b(u, u, texture_mask.low);                                            \
  and_8x8b(v, v, texture_mask.high);                                           \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  zip_8x16b(uv, u, v);                                                         \
  block->uv = uv;                                                              \
  block->r = r;                                                                \
  block->g = g;                                                                \
  block->b = b;                                                                \
  block->dither_offsets = vector_cast(vec_8x16u, dither_offsets);              \
  block->fb_ptr = fb_ptr;                                                      \
}                                                                              \

#define setup_blocks_store_unshaded_textured(swizzling, dithering, target,     \
 edge_type)                                                                    \
{                                                                              \
  vec_8x16u u_whole;                                                           \
  vec_8x16u v_whole;                                                           \
                                                                               \
  vec_8x8u u;                                                                  \
  vec_8x8u v;                                                                  \
  vec_8x16u uv;                                                                \
                                                                               \
  vec_4x32u dx4;                                                               \
  vec_4x32u dx8;                                                               \
                                                                               \
  shr_narrow_4x32b(u_whole.low, u_block, 16);                                  \
  shr_narrow_4x32b(v_whole.low, v_block, 16);                                  \
                                                                               \
  dup_4x32b(dx4, uv_dx4.e[0]);                                                 \
  add_high_narrow_4x32b(u_whole.high, u_block, dx4);                           \
  dup_4x32b(dx4, uv_dx4.e[1]);                                                 \
  add_high_narrow_4x32b(v_whole.high, v_block, dx4);                           \
                                                                               \
  mov_narrow_8x16b(u, u_whole);                                                \
  mov_narrow_8x16b(v, v_whole);                                                \
                                                                               \
  dup_4x32b(dx8, uv_dx8.e[0]);                                                 \
  add_4x32b(u_block, u_block, dx8);                                            \
  dup_4x32b(dx8, uv_dx8.e[1]);                                                 \
  add_4x32b(v_block, v_block, dx8);                                            \
                                                                               \
  and_8x8b(u, u, texture_mask.low);                                            \
  and_8x8b(v, v, texture_mask.high);                                           \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  zip_8x16b(uv, u, v);                                                         \
  block->uv = uv;                                                              \
  block->dither_offsets = vector_cast(vec_8x16u, dither_offsets);              \
  block->fb_ptr = fb_ptr;                                                      \
}                                                                              \

#define setup_blocks_store_shaded_untextured_dithered()                        \
  addq_8x8b(r, r, dither_offsets);                                             \
  addq_8x8b(g, g, dither_offsets);                                             \
  addq_8x8b(b, b, dither_offsets);                                             \
                                                                               \
  subq_8x8b(r, r, d64_4);                                                      \
  subq_8x8b(g, g, d64_4);                                                      \
  subq_8x8b(b, b, d64_4)                                                       \

#define setup_blocks_store_shaded_untextured_undithered()                      \
  

#define setup_blocks_store_untextured_pixels_indirect_full(_pixels)            \
  block->pixels = _pixels;                                                     \
  block->fb_ptr = fb_ptr                                                       \

#define setup_blocks_store_untextured_pixels_indirect_edge(_pixels)            \
  block->pixels = _pixels;                                                     \
  block->fb_ptr = fb_ptr                                                       \

#define setup_blocks_store_shaded_untextured_seed_pixels_indirect()            \
  mul_long_8x8b(pixels, r, d64_1)                                              \


#define setup_blocks_store_untextured_pixels_direct_full(_pixels)              \
  store_8x16b(_pixels, fb_ptr)                                                 \

#define setup_blocks_store_untextured_pixels_direct_edge(_pixels)              \
{                                                                              \
  vec_8x16u fb_pixels;                                                         \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u test_mask = psx_gpu->test_mask;                                    \
                                                                               \
  load_8x16b(fb_pixels, fb_ptr);                                               \
  dup_8x16b(draw_mask, span_edge_data->right_mask);                            \
  tst_8x16b(draw_mask, draw_mask, test_mask);                                  \
  bif_8x16b(fb_pixels, _pixels, draw_mask);                                    \
  store_8x16b(fb_pixels, fb_ptr);                                              \
}                                                                              \

#define setup_blocks_store_shaded_untextured_seed_pixels_direct()              \
  pixels = msb_mask;                                                           \
  mla_long_8x8b(pixels, r, d64_1)                                              \


#define setup_blocks_store_shaded_untextured(swizzling, dithering, target,     \
 edge_type)                                                                    \
{                                                                              \
  vec_8x16u r_whole;                                                           \
  vec_8x16u g_whole;                                                           \
  vec_8x16u b_whole;                                                           \
                                                                               \
  vec_8x8u r;                                                                  \
  vec_8x8u g;                                                                  \
  vec_8x8u b;                                                                  \
                                                                               \
  vec_4x32u dx4;                                                               \
  vec_4x32u dx8;                                                               \
                                                                               \
  vec_8x16u pixels;                                                            \
                                                                               \
  shr_narrow_4x32b(r_whole.low, r_block, 16);                                  \
  shr_narrow_4x32b(g_whole.low, g_block, 16);                                  \
  shr_narrow_4x32b(b_whole.low, b_block, 16);                                  \
                                                                               \
  dup_4x32b(dx4, rgb_dx4.e[0]);                                                \
  add_high_narrow_4x32b(r_whole.high, r_block, dx4);                           \
  dup_4x32b(dx4, rgb_dx4.e[1]);                                                \
  add_high_narrow_4x32b(g_whole.high, g_block, dx4);                           \
  dup_4x32b(dx4, rgb_dx4.e[2]);                                                \
  add_high_narrow_4x32b(b_whole.high, b_block, dx4);                           \
                                                                               \
  mov_narrow_8x16b(r, r_whole);                                                \
  mov_narrow_8x16b(g, g_whole);                                                \
  mov_narrow_8x16b(b, b_whole);                                                \
                                                                               \
  dup_4x32b(dx8, rgb_dx8.e[0]);                                                \
  add_4x32b(r_block, r_block, dx8);                                            \
  dup_4x32b(dx8, rgb_dx8.e[1]);                                                \
  add_4x32b(g_block, g_block, dx8);                                            \
  dup_4x32b(dx8, rgb_dx8.e[2]);                                                \
  add_4x32b(b_block, b_block, dx8);                                            \
                                                                               \
  setup_blocks_store_shaded_untextured_##dithering();                          \
                                                                               \
  shr_8x8b(r, r, 3);                                                           \
  bic_8x8b(g, g, d64_0x07);                                                    \
  bic_8x8b(b, b, d64_0x07);                                                    \
                                                                               \
  setup_blocks_store_shaded_untextured_seed_pixels_##target();                 \
  mla_long_8x8b(pixels, g, d64_4);                                             \
  mla_long_8x8b(pixels, b, d64_128)                                            \
                                                                               \
  setup_blocks_store_untextured_pixels_##target##_##edge_type(pixels);         \
}                                                                              \

#define setup_blocks_store_unshaded_untextured(swizzling, dithering, target,   \
 edge_type)                                                                    \
  setup_blocks_store_untextured_pixels_##target##_##edge_type(colors)          \


#define setup_blocks_store_draw_mask_textured_indirect(_block, bits)           \
  (_block)->draw_mask_bits = bits                                              \

#define setup_blocks_store_draw_mask_untextured_indirect(_block, bits)         \
{                                                                              \
  vec_8x16u bits_mask;                                                         \
  vec_8x16u test_mask = psx_gpu->test_mask;                                    \
  dup_8x16b(bits_mask, bits);                                                  \
  tst_8x16b(bits_mask, bits_mask, test_mask);                                  \
  (_block)->draw_mask = bits_mask;                                             \
}                                                                              \

#define setup_blocks_store_draw_mask_untextured_direct(_block, bits)           \

#define setup_blocks_uv_adj_hack_untextured(_block, edge_data, uvrg_offset)    \

#define setup_blocks_uv_adj_hack_textured(_block, edge_data, uvrg_offset)      \
{                                                                              \
  u32 m_ = AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V;                          \
  if (unlikely(psx_gpu->hacks_active & m_))                                    \
    setup_blocks_uv_adj_hack(psx_gpu, _block, edge_data, uvrg_offset);         \
}                                                                              \

#define setup_blocks_add_blocks_indirect()                                     \
  num_blocks += span_num_blocks;                                               \
                                                                               \
  if(num_blocks > MAX_BLOCKS)                                                  \
  {                                                                            \
    psx_gpu->num_blocks = num_blocks - span_num_blocks;                        \
    flush_render_block_buffer(psx_gpu);                                        \
    num_blocks = span_num_blocks;                                              \
    block = psx_gpu->blocks;                                                   \
  }                                                                            \

#define setup_blocks_add_blocks_direct()                                       \
  stats_add(texel_blocks_untextured, span_num_blocks);                         \
  stats_add(span_pixel_blocks, span_num_blocks);                               \


#define setup_blocks_builder(shading, texturing, dithering, sw, target)        \
void setup_blocks_##shading##_##texturing##_##dithering##_##sw##_##target(     \
 psx_gpu_struct *psx_gpu)                                                      \
{                                                                              \
  setup_blocks_load_msb_mask_##target();                                       \
  setup_blocks_variables_##shading##_##texturing(target);                      \
                                                                               \
  edge_data_struct *span_edge_data = psx_gpu->span_edge_data;                  \
  vec_4x32u *span_uvrg_offset = psx_gpu->span_uvrg_offset;                     \
  u32 *span_b_offset = psx_gpu->span_b_offset;                                 \
                                                                               \
  block_struct *block = psx_gpu->blocks + psx_gpu->num_blocks;                 \
                                                                               \
  u32 num_spans = psx_gpu->num_spans;                                          \
                                                                               \
  u16 *fb_ptr;                                                                 \
  u32 y;                                                                       \
                                                                               \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  u32 span_num_blocks;                                                         \
                                                                               \
  while(num_spans)                                                             \
  {                                                                            \
    span_num_blocks = span_edge_data->num_blocks;                              \
    if(span_num_blocks)                                                        \
    {                                                                          \
      y = span_edge_data->y;                                                   \
      fb_ptr = psx_gpu->vram_out_ptr + span_edge_data->left_x + (y * 1024);    \
                                                                               \
      setup_blocks_span_initialize_##shading##_##texturing();                  \
      setup_blocks_span_initialize_##dithering(texturing);                     \
                                                                               \
      setup_blocks_add_blocks_##target();                                      \
                                                                               \
      s32 pixel_span = span_num_blocks * 8;                                    \
      pixel_span -= __builtin_popcount(span_edge_data->right_mask & 0xFF);     \
      span_pixels += pixel_span;                                               \
                                                                               \
      span_num_blocks--;                                                       \
      while(span_num_blocks)                                                   \
      {                                                                        \
        setup_blocks_store_##shading##_##texturing(sw, dithering, target,      \
         full);                                                                \
        setup_blocks_store_draw_mask_##texturing##_##target(block, 0x00);      \
                                                                               \
        fb_ptr += 8;                                                           \
        block++;                                                               \
        span_num_blocks--;                                                     \
      }                                                                        \
                                                                               \
      setup_blocks_store_##shading##_##texturing(sw, dithering, target, edge); \
      setup_blocks_store_draw_mask_##texturing##_##target(block,               \
       span_edge_data->right_mask);                                            \
      setup_blocks_uv_adj_hack_##texturing(block, span_edge_data,              \
          span_uvrg_offset);                                                   \
                                                                               \
      block++;                                                                 \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      zero_block_spans++;                                                      \
    }                                                                          \
                                                                               \
    num_spans--;                                                               \
    span_edge_data++;                                                          \
    span_uvrg_offset++;                                                        \
    span_b_offset++;                                                           \
  }                                                                            \
                                                                               \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \


//setup_blocks_builder(unshaded, untextured, undithered, unswizzled, direct);

#ifndef NEON_BUILD

setup_blocks_builder(shaded, textured, dithered, swizzled, indirect);
setup_blocks_builder(shaded, textured, dithered, unswizzled, indirect);

setup_blocks_builder(unshaded, textured, dithered, unswizzled, indirect);
setup_blocks_builder(unshaded, textured, dithered, swizzled, indirect);

setup_blocks_builder(shaded, untextured, undithered, unswizzled, indirect);
setup_blocks_builder(shaded, untextured, dithered, unswizzled, indirect);
setup_blocks_builder(shaded, untextured, undithered, unswizzled, direct);
setup_blocks_builder(shaded, untextured, dithered, unswizzled, direct);

setup_blocks_builder(unshaded, untextured, undithered, unswizzled, indirect);
setup_blocks_builder(unshaded, untextured, undithered, unswizzled, direct);

void texture_blocks_untextured(psx_gpu_struct *psx_gpu)
{
  if(psx_gpu->primitive_type != PRIMITIVE_TYPE_SPRITE)
    stats_add(texel_blocks_untextured, psx_gpu->num_blocks);
}

void texture_blocks_4bpp(psx_gpu_struct *psx_gpu)
{
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;
  stats_add(texel_blocks_4bpp, num_blocks);

  vec_8x8u texels_low;
  vec_8x8u texels_high;
  vec_8x8u texels;
  vec_8x16u pixels;

  vec_8x16u clut_a;
  vec_8x16u clut_b;
  vec_16x8u clut_low;
  vec_16x8u clut_high;

  u8 *texture_ptr_8bpp = psx_gpu->texture_page_ptr;
  u16 *clut_ptr = psx_gpu->clut_ptr;

  // Can be done with one deinterleaving load on NEON
  load_8x16b(clut_a, clut_ptr);
  load_8x16b(clut_b, clut_ptr + 8);
  unzip_16x8b(clut_low, clut_high, clut_a, clut_b);

  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_4bpp_mask)
    update_texture_4bpp_cache(psx_gpu);

  while(num_blocks)
  {
    texels.e[0] = texture_ptr_8bpp[block->uv.e[0]];
    texels.e[1] = texture_ptr_8bpp[block->uv.e[1]];
    texels.e[2] = texture_ptr_8bpp[block->uv.e[2]];
    texels.e[3] = texture_ptr_8bpp[block->uv.e[3]];
    texels.e[4] = texture_ptr_8bpp[block->uv.e[4]];
    texels.e[5] = texture_ptr_8bpp[block->uv.e[5]];
    texels.e[6] = texture_ptr_8bpp[block->uv.e[6]];
    texels.e[7] = texture_ptr_8bpp[block->uv.e[7]];

    tbl_16(texels_low, texels, clut_low);
    tbl_16(texels_high, texels, clut_high);

    // Can be done with an interleaving store on NEON
    zip_8x16b(pixels, texels_low, texels_high);

    block->texels = pixels;

    num_blocks--;
    block++;
  }
}

void texture_blocks_8bpp(psx_gpu_struct *psx_gpu)
{
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;

  stats_add(texel_blocks_8bpp, num_blocks);

  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_8bpp_mask)
    update_texture_8bpp_cache(psx_gpu);

  vec_8x16u texels;
  u8 *texture_ptr_8bpp = psx_gpu->texture_page_ptr;

  u32 texel;
  u32 offset;
  u32 i;

  while(num_blocks)
  {
    for(i = 0; i < 8; i++)
    {
      offset = block->uv.e[i];

      texel = texture_ptr_8bpp[offset];
      texels.e[i] = psx_gpu->clut_ptr[texel];
    }

    block->texels = texels;

    num_blocks--;
    block++;
  }
}

void texture_blocks_16bpp(psx_gpu_struct *psx_gpu)
{
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;

  stats_add(texel_blocks_16bpp, num_blocks);

  vec_8x16u texels;

  u16 *texture_ptr_16bpp = psx_gpu->texture_page_ptr;
  u32 offset;
  u32 i;

  while(num_blocks)
  {
    for(i = 0; i < 8; i++)
    {
      offset = block->uv.e[i];
      offset += ((offset & 0xFF00) * 3);

      texels.e[i] = texture_ptr_16bpp[offset];
    }

    block->texels = texels;

    num_blocks--;
    block++;
  }
}

#endif


#define shade_blocks_load_msb_mask_indirect()                                  \

#define shade_blocks_load_msb_mask_direct()                                    \
  vec_8x16u msb_mask;                                                          \
  dup_8x16b(msb_mask, psx_gpu->mask_msb);                                      \

#define shade_blocks_store_indirect(_draw_mask, _pixels)                       \
  block->draw_mask = _draw_mask;                                               \
  block->pixels = _pixels                                                      \

#define shade_blocks_store_direct(_draw_mask, _pixels)                         \
{                                                                              \
  vec_8x16u fb_pixels;                                                         \
  or_8x16b(_pixels, _pixels, msb_mask);                                        \
  load_8x16b(fb_pixels, block->fb_ptr);                                        \
  bif_8x16b(fb_pixels, _pixels, _draw_mask);                                   \
  store_8x16b(fb_pixels, block->fb_ptr);                                       \
}                                                                              \


#define shade_blocks_textured_false_modulated_check_dithered(target)           \
  if(psx_gpu->triangle_color == 0x808080)                                      \
  {                                                                            \
    false_modulated_blocks += num_blocks;                                      \
  }                                                                            \

#define shade_blocks_textured_false_modulated_check_undithered(target)         \
  if(psx_gpu->triangle_color == 0x808080)                                      \
  {                                                                            \
                                                                               \
    shade_blocks_textured_unmodulated_##target(psx_gpu);                       \
    false_modulated_blocks += num_blocks;                                      \
    return;                                                                    \
  }                                                                            \


#define shade_blocks_textured_modulated_shaded_primitive_load(dithering,       \
 target)                                                                       \

#define shade_blocks_textured_modulated_unshaded_primitive_load(dithering,     \
 target)                                                                       \
{                                                                              \
  u32 color = psx_gpu->triangle_color;                                         \
  dup_8x8b(colors_r, color);                                                   \
  dup_8x8b(colors_g, color >> 8);                                              \
  dup_8x8b(colors_b, color >> 16);                                             \
  shade_blocks_textured_false_modulated_check_##dithering(target);             \
}                                                                              \

#define shade_blocks_textured_modulated_shaded_block_load()                    \
  colors_r = block->r;                                                         \
  colors_g = block->g;                                                         \
  colors_b = block->b                                                          \

#define shade_blocks_textured_modulated_unshaded_block_load()                  \

#define shade_blocks_textured_modulate_dithered(component)                     \
  pixels_##component = block->dither_offsets;                                  \
  mla_long_8x8b(pixels_##component, texels_##component, colors_##component)    \

#define shade_blocks_textured_modulate_undithered(component)                   \
  mul_long_8x8b(pixels_##component, texels_##component, colors_##component)    \

#define shade_blocks_textured_modulated_builder(shading, dithering, target)    \
void shade_blocks_##shading##_textured_modulated_##dithering##_##target(       \
 psx_gpu_struct *psx_gpu)                                                      \
{                                                                              \
  block_struct *block = psx_gpu->blocks;                                       \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  vec_8x16u texels;                                                            \
                                                                               \
  vec_8x8u texels_r;                                                           \
  vec_8x8u texels_g;                                                           \
  vec_8x8u texels_b;                                                           \
                                                                               \
  vec_8x8u colors_r;                                                           \
  vec_8x8u colors_g;                                                           \
  vec_8x8u colors_b;                                                           \
                                                                               \
  vec_8x8u pixels_r_low;                                                       \
  vec_8x8u pixels_g_low;                                                       \
  vec_8x8u pixels_b_low;                                                       \
  vec_8x16u pixels;                                                            \
                                                                               \
  vec_8x16u pixels_r;                                                          \
  vec_8x16u pixels_g;                                                          \
  vec_8x16u pixels_b;                                                          \
                                                                               \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u zero_mask;                                                         \
                                                                               \
  vec_8x8u d64_0x07;                                                           \
  vec_8x8u d64_0x1F;                                                           \
  vec_8x8u d64_1;                                                              \
  vec_8x8u d64_4;                                                              \
  vec_8x8u d64_128;                                                            \
                                                                               \
  vec_8x16u d128_0x8000;                                                       \
                                                                               \
  vec_8x16u test_mask = psx_gpu->test_mask;                                    \
  u32 draw_mask_bits;                                                          \
  shade_blocks_load_msb_mask_##target();                                       \
                                                                               \
  dup_8x8b(d64_0x07, 0x07);                                                    \
  dup_8x8b(d64_0x1F, 0x1F);                                                    \
  dup_8x8b(d64_1, 1);                                                          \
  dup_8x8b(d64_4, 4);                                                          \
  dup_8x8b(d64_128, 128);                                                      \
                                                                               \
  dup_8x16b(d128_0x8000, 0x8000);                                              \
                                                                               \
  shade_blocks_textured_modulated_##shading##_primitive_load(dithering,        \
   target);                                                                    \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    draw_mask_bits = block->draw_mask_bits;                                    \
    dup_8x16b(draw_mask, draw_mask_bits);                                      \
    tst_8x16b(draw_mask, draw_mask, test_mask);                                \
                                                                               \
    shade_blocks_textured_modulated_##shading##_block_load();                  \
                                                                               \
    texels = block->texels;                                                    \
                                                                               \
    mov_narrow_8x16b(texels_r, texels);                                        \
    shr_narrow_8x16b(texels_g, texels, 5);                                     \
    shr_narrow_8x16b(texels_b, texels, 7);                                     \
                                                                               \
    and_8x8b(texels_r, texels_r, d64_0x1F);                                    \
    and_8x8b(texels_g, texels_g, d64_0x1F);                                    \
    shr_8x8b(texels_b, texels_b, 3);                                           \
                                                                               \
    shade_blocks_textured_modulate_##dithering(r);                             \
    shade_blocks_textured_modulate_##dithering(g);                             \
    shade_blocks_textured_modulate_##dithering(b);                             \
                                                                               \
    cmpeqz_8x16b(zero_mask, texels);                                           \
    and_8x16b(pixels, texels, d128_0x8000);                                    \
                                                                               \
    shrq_narrow_signed_8x16b(pixels_r_low, pixels_r, 4);                       \
    shrq_narrow_signed_8x16b(pixels_g_low, pixels_g, 4);                       \
    shrq_narrow_signed_8x16b(pixels_b_low, pixels_b, 4);                       \
                                                                               \
    or_8x16b(zero_mask, draw_mask, zero_mask);                                 \
                                                                               \
    shr_8x8b(pixels_r_low, pixels_r_low, 3);                                   \
    bic_8x8b(pixels_g_low, pixels_g_low, d64_0x07);                            \
    bic_8x8b(pixels_b_low, pixels_b_low, d64_0x07);                            \
                                                                               \
    mla_long_8x8b(pixels, pixels_r_low, d64_1);                                \
    mla_long_8x8b(pixels, pixels_g_low, d64_4);                                \
    mla_long_8x8b(pixels, pixels_b_low, d64_128);                              \
                                                                               \
    shade_blocks_store_##target(zero_mask, pixels);                            \
                                                                               \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \
}                                                                              \

#ifndef NEON_BUILD

shade_blocks_textured_modulated_builder(shaded, dithered, direct);
shade_blocks_textured_modulated_builder(shaded, undithered, direct);
shade_blocks_textured_modulated_builder(unshaded, dithered, direct);
shade_blocks_textured_modulated_builder(unshaded, undithered, direct);

shade_blocks_textured_modulated_builder(shaded, dithered, indirect);
shade_blocks_textured_modulated_builder(shaded, undithered, indirect);
shade_blocks_textured_modulated_builder(unshaded, dithered, indirect);
shade_blocks_textured_modulated_builder(unshaded, undithered, indirect);

#endif


#define shade_blocks_textured_unmodulated_builder(target)                      \
void shade_blocks_textured_unmodulated_##target(psx_gpu_struct *psx_gpu)       \
{                                                                              \
  block_struct *block = psx_gpu->blocks;                                       \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u test_mask = psx_gpu->test_mask;                                    \
  u32 draw_mask_bits;                                                          \
                                                                               \
  vec_8x16u pixels;                                                            \
  shade_blocks_load_msb_mask_##target();                                       \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    vec_8x16u zero_mask;                                                       \
                                                                               \
    draw_mask_bits = block->draw_mask_bits;                                    \
    dup_8x16b(draw_mask, draw_mask_bits);                                      \
    tst_8x16b(draw_mask, draw_mask, test_mask);                                \
                                                                               \
    pixels = block->texels;                                                    \
                                                                               \
    cmpeqz_8x16b(zero_mask, pixels);                                           \
    or_8x16b(zero_mask, draw_mask, zero_mask);                                 \
                                                                               \
    shade_blocks_store_##target(zero_mask, pixels);                            \
                                                                               \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \
}                                                                              \

#define shade_blocks_textured_unmodulated_dithered_builder(target)             \
void shade_blocks_textured_unmodulated_dithered_##target(psx_gpu_struct        \
 *psx_gpu)                                                                     \
{                                                                              \
  block_struct *block = psx_gpu->blocks;                                       \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u test_mask = psx_gpu->test_mask;                                    \
  u32 draw_mask_bits;                                                          \
                                                                               \
  vec_8x16u pixels;                                                            \
  shade_blocks_load_msb_mask_##target();                                       \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    vec_8x16u zero_mask;                                                       \
                                                                               \
    draw_mask_bits = block->draw_mask_bits;                                    \
    dup_8x16b(draw_mask, draw_mask_bits);                                      \
    tst_8x16b(draw_mask, draw_mask, test_mask);                                \
                                                                               \
    pixels = block->texels;                                                    \
                                                                               \
    cmpeqz_8x16b(zero_mask, pixels);                                           \
    or_8x16b(zero_mask, draw_mask, zero_mask);                                 \
                                                                               \
    shade_blocks_store_##target(zero_mask, pixels);                            \
                                                                               \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \
}                                                                              \

#ifndef NEON_BUILD

shade_blocks_textured_unmodulated_builder(indirect)
shade_blocks_textured_unmodulated_builder(direct)

void shade_blocks_unshaded_untextured_indirect(psx_gpu_struct *psx_gpu)
{
}

void shade_blocks_unshaded_untextured_direct(psx_gpu_struct *psx_gpu)
{
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;

  vec_8x16u pixels = block->pixels;
  shade_blocks_load_msb_mask_direct();

  while(num_blocks)
  {
    shade_blocks_store_direct(block->draw_mask, pixels);

    num_blocks--;
    block++;
  }
}

#endif

void shade_blocks_shaded_untextured(psx_gpu_struct *psx_gpu)
{
}


#define blend_blocks_mask_evaluate_on()                                        \
  vec_8x16u mask_pixels;                                                       \
  cmpltz_8x16b(mask_pixels, framebuffer_pixels);                               \
  or_8x16b(draw_mask, draw_mask, mask_pixels)                                  \

#define blend_blocks_mask_evaluate_off()                                       \

#define blend_blocks_average()                                                 \
{                                                                              \
  vec_8x16u pixels_no_msb;                                                     \
  vec_8x16u fb_pixels_no_msb;                                                  \
                                                                               \
  vec_8x16u d128_0x0421;                                                       \
  vec_8x16u d128_0x8000;                                                       \
                                                                               \
  dup_8x16b(d128_0x0421, 0x0421);                                              \
  dup_8x16b(d128_0x8000, 0x8000);                                              \
                                                                               \
  eor_8x16b(blend_pixels, pixels, framebuffer_pixels);                         \
  bic_8x16b(pixels_no_msb, pixels, d128_0x8000);                               \
  and_8x16b(blend_pixels, blend_pixels, d128_0x0421);                          \
  sub_8x16b(blend_pixels, pixels_no_msb, blend_pixels);                        \
  bic_8x16b(fb_pixels_no_msb, framebuffer_pixels, d128_0x8000);                \
  average_8x16b(blend_pixels, fb_pixels_no_msb, blend_pixels);                 \
}                                                                              \

#define blend_blocks_add()                                                     \
{                                                                              \
  vec_8x16u pixels_rb, pixels_g;                                               \
  vec_8x16u fb_rb, fb_g;                                                       \
                                                                               \
  vec_8x16u d128_0x7C1F;                                                       \
  vec_8x16u d128_0x03E0;                                                       \
                                                                               \
  dup_8x16b(d128_0x7C1F, 0x7C1F);                                              \
  dup_8x16b(d128_0x03E0, 0x03E0);                                              \
                                                                               \
  and_8x16b(pixels_rb, pixels, d128_0x7C1F);                                   \
  and_8x16b(pixels_g, pixels, d128_0x03E0);                                    \
                                                                               \
  and_8x16b(fb_rb, framebuffer_pixels, d128_0x7C1F);                           \
  and_8x16b(fb_g, framebuffer_pixels, d128_0x03E0);                            \
                                                                               \
  add_8x16b(fb_rb, fb_rb, pixels_rb);                                          \
  add_8x16b(fb_g, fb_g, pixels_g);                                             \
                                                                               \
  min_16x8b(vector_cast(vec_16x8u, fb_rb), vector_cast(vec_16x8u, fb_rb),      \
   vector_cast(vec_16x8u, d128_0x7C1F));                                       \
  min_8x16b(fb_g, fb_g, d128_0x03E0);                                          \
                                                                               \
  or_8x16b(blend_pixels, fb_rb, fb_g);                                         \
}                                                                              \

#define blend_blocks_subtract()                                                \
{                                                                              \
  vec_8x16u pixels_rb, pixels_g;                                               \
  vec_8x16u fb_rb, fb_g;                                                       \
                                                                               \
  vec_8x16u d128_0x7C1F;                                                       \
  vec_8x16u d128_0x03E0;                                                       \
                                                                               \
  dup_8x16b(d128_0x7C1F, 0x7C1F);                                              \
  dup_8x16b(d128_0x03E0, 0x03E0);                                              \
                                                                               \
  and_8x16b(pixels_rb, pixels, d128_0x7C1F);                                   \
  and_8x16b(pixels_g, pixels, d128_0x03E0);                                    \
                                                                               \
  and_8x16b(fb_rb, framebuffer_pixels, d128_0x7C1F);                           \
  and_8x16b(fb_g, framebuffer_pixels, d128_0x03E0);                            \
                                                                               \
  subs_16x8b(vector_cast(vec_16x8u, fb_rb),                                    \
   vector_cast(vec_16x8u, fb_rb), vector_cast(vec_16x8u, pixels_rb));          \
  subs_8x16b(fb_g, fb_g, pixels_g);                                            \
                                                                               \
  or_8x16b(blend_pixels, fb_rb, fb_g);                                         \
}                                                                              \

#define blend_blocks_add_fourth()                                              \
{                                                                              \
  vec_8x16u pixels_rb, pixels_g;                                               \
  vec_8x16u pixels_fourth;                                                     \
  vec_8x16u fb_rb, fb_g;                                                       \
                                                                               \
  vec_8x16u d128_0x7C1F;                                                       \
  vec_8x16u d128_0x1C07;                                                       \
  vec_8x16u d128_0x03E0;                                                       \
  vec_8x16u d128_0x00E0;                                                       \
                                                                               \
  dup_8x16b(d128_0x7C1F, 0x7C1F);                                              \
  dup_8x16b(d128_0x1C07, 0x1C07);                                              \
  dup_8x16b(d128_0x03E0, 0x03E0);                                              \
  dup_8x16b(d128_0x00E0, 0x00E0);                                              \
                                                                               \
  shr_8x16b(pixels_fourth, vector_cast(vec_8x16s, pixels), 2);                 \
                                                                               \
  and_8x16b(fb_rb, framebuffer_pixels, d128_0x7C1F);                           \
  and_8x16b(fb_g, framebuffer_pixels, d128_0x03E0);                            \
                                                                               \
  and_8x16b(pixels_rb, pixels_fourth, d128_0x1C07);                            \
  and_8x16b(pixels_g, pixels_fourth, d128_0x00E0);                             \
                                                                               \
  add_8x16b(fb_rb, fb_rb, pixels_rb);                                          \
  add_8x16b(fb_g, fb_g, pixels_g);                                             \
                                                                               \
  min_16x8b(vector_cast(vec_16x8u, fb_rb), vector_cast(vec_16x8u, fb_rb),      \
   vector_cast(vec_16x8u, d128_0x7C1F));                                       \
  min_8x16b(fb_g, fb_g, d128_0x03E0);                                          \
                                                                               \
  or_8x16b(blend_pixels, fb_rb, fb_g);                                         \
}                                                                              \

#define blend_blocks_blended_combine_textured()                                \
{                                                                              \
  vec_8x16u blend_mask;                                                        \
  cmpltz_8x16b(blend_mask, pixels);                                            \
                                                                               \
  or_immediate_8x16b(blend_pixels, blend_pixels, 0x8000);                      \
  bif_8x16b(blend_pixels, pixels, blend_mask);                                 \
}                                                                              \

#define blend_blocks_blended_combine_untextured()                              \


#define blend_blocks_body_blend(blend_mode, texturing)                         \
{                                                                              \
  blend_blocks_##blend_mode();                                                 \
  blend_blocks_blended_combine_##texturing();                                  \
}                                                                              \

#define blend_blocks_body_average(texturing)                                   \
  blend_blocks_body_blend(average, texturing)                                  \

#define blend_blocks_body_add(texturing)                                       \
  blend_blocks_body_blend(add, texturing)                                      \

#define blend_blocks_body_subtract(texturing)                                  \
  blend_blocks_body_blend(subtract, texturing)                                 \

#define blend_blocks_body_add_fourth(texturing)                                \
  blend_blocks_body_blend(add_fourth, texturing)                               \

#define blend_blocks_body_unblended(texturing)                                 \
  blend_pixels = pixels                                                        \


#define blend_blocks_builder(texturing, blend_mode, mask_evaluate)             \
void                                                                           \
 blend_blocks_##texturing##_##blend_mode##_##mask_evaluate(psx_gpu_struct      \
 *psx_gpu)                                                                     \
{                                                                              \
  block_struct *block = psx_gpu->blocks;                                       \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u pixels;                                                            \
  vec_8x16u blend_pixels;                                                      \
  vec_8x16u framebuffer_pixels;                                                \
  vec_8x16u msb_mask;                                                          \
                                                                               \
  u16 *fb_ptr;                                                                 \
                                                                               \
  dup_8x16b(msb_mask, psx_gpu->mask_msb);                                      \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    pixels = block->pixels;                                                    \
    draw_mask = block->draw_mask;                                              \
    fb_ptr = block->fb_ptr;                                                    \
                                                                               \
    load_8x16b(framebuffer_pixels, fb_ptr);                                    \
                                                                               \
    blend_blocks_mask_evaluate_##mask_evaluate();                              \
    blend_blocks_body_##blend_mode(texturing);                                 \
                                                                               \
    or_8x16b(blend_pixels, blend_pixels, msb_mask);                            \
    bif_8x16b(framebuffer_pixels, blend_pixels, draw_mask);                    \
    store_8x16b(framebuffer_pixels, fb_ptr);                                   \
                                                                               \
    blend_blocks++;                                                            \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \
}                                                                              \

#ifndef NEON_BUILD

void blend_blocks_textured_unblended_off(psx_gpu_struct *psx_gpu)
{
}

blend_blocks_builder(textured, average, off);
blend_blocks_builder(textured, average, on);
blend_blocks_builder(textured, add, off);
blend_blocks_builder(textured, add, on);
blend_blocks_builder(textured, subtract, off);
blend_blocks_builder(textured, subtract, on);
blend_blocks_builder(textured, add_fourth, off);
blend_blocks_builder(textured, add_fourth, on);

blend_blocks_builder(untextured, average, off);
blend_blocks_builder(untextured, average, on);
blend_blocks_builder(untextured, add, off);
blend_blocks_builder(untextured, add, on);
blend_blocks_builder(untextured, subtract, off);
blend_blocks_builder(untextured, subtract, on);
blend_blocks_builder(untextured, add_fourth, off);
blend_blocks_builder(untextured, add_fourth, on);

blend_blocks_builder(textured, unblended, on);

#endif

                                                                               
#define vertex_swap(_a, _b)                                                    \
{                                                                              \
  vertex_struct *temp_vertex = _a;                                             \
  _a = _b;                                                                     \
  _b = temp_vertex;                                                            \
  triangle_winding ^= 1;                                                       \
}                                                                              \


// Setup blocks parametric-variables:
// SHADE  TEXTURE_MAP SWIZZLING
// 0      0           x          
// 0      1           0
// 0      1           1
// 1      0           x
// 1      1           0
// 1      1           1
// 8 inputs, 6 combinations

#define setup_blocks_switch_untextured_unshaded(dithering, target)             \
  setup_blocks_unshaded_untextured_undithered_unswizzled_##target              \

#define setup_blocks_switch_untextured_shaded(dithering, target)               \
  setup_blocks_shaded_untextured_##dithering##_unswizzled_##target             \

#define setup_blocks_switch_untextured(shading, texture_mode, dithering,       \
 target)                                                                       \
  setup_blocks_switch_untextured_##shading(dithering, target)                  \

#define setup_blocks_switch_texture_mode_4bpp(shading)                         \
  setup_blocks_##shading##_textured_dithered_swizzled_indirect                 \

#define setup_blocks_switch_texture_mode_8bpp(shading)                         \
  setup_blocks_##shading##_textured_dithered_swizzled_indirect                 \

#define setup_blocks_switch_texture_mode_16bpp(shading)                        \
  setup_blocks_##shading##_textured_dithered_unswizzled_indirect               \

#define setup_blocks_switch_textured(shading, texture_mode, dithering, target) \
  setup_blocks_switch_texture_mode_##texture_mode(shading)                     \

#define setup_blocks_switch_blended(shading, texturing, texture_mode,          \
 dithering, mask_evaluate)                                                     \
  setup_blocks_switch_##texturing(shading, texture_mode, dithering, indirect)  \

#define setup_blocks_switch_unblended_on(shading, texturing, texture_mode,     \
 dithering)                                                                    \
  setup_blocks_switch_##texturing(shading, texture_mode, dithering, indirect)  \

#define setup_blocks_switch_unblended_off(shading, texturing, texture_mode,    \
 dithering)                                                                    \
  setup_blocks_switch_##texturing(shading, texture_mode, dithering, direct)    \

#define setup_blocks_switch_unblended(shading, texturing, texture_mode,        \
 dithering, mask_evaluate)                                                     \
  setup_blocks_switch_unblended_##mask_evaluate(shading, texturing,            \
   texture_mode, dithering)                                                    \

#define setup_blocks_switch(shading, texturing, texture_mode, dithering,       \
 blending, mask_evaluate)                                                      \
  setup_blocks_switch_##blending(shading, texturing, texture_mode,             \
   dithering, mask_evaluate)                                                   \


// Texture blocks:

#define texture_blocks_switch_untextured(texture_mode)                         \
  texture_blocks_untextured                                                    \

#define texture_blocks_switch_textured(texture_mode)                           \
  texture_blocks_##texture_mode                                                \

#define texture_blocks_switch(texturing, texture_mode)                         \
  texture_blocks_switch_##texturing(texture_mode)                              \


// Shade blocks parametric-variables:
// SHADE  TEXTURE_MAP  MODULATE_TEXELS  dither_mode
// 0      0            x                x
// 0      1            0                0
// 0      1            0                1
// x      1            1                x
// 1      0            x                0
// 1      0            x                1
// 1      1            0                0
// 1      1            0                1
// 16 inputs, 8 combinations

#define shade_blocks_switch_unshaded_untextured(modulation, dithering, target) \
  shade_blocks_unshaded_untextured_##target                                    \

#define shade_blocks_switch_unshaded_textured_unmodulated(dithering, target)   \
  shade_blocks_textured_unmodulated_##target                                   \

#define shade_blocks_switch_unshaded_textured_modulated(dithering, target)     \
  shade_blocks_unshaded_textured_modulated_##dithering##_##target              \

#define shade_blocks_switch_unshaded_textured(modulation, dithering, target)   \
  shade_blocks_switch_unshaded_textured_##modulation(dithering, target)        \

#define shade_blocks_switch_unshaded(texturing, modulation, dithering, target) \
  shade_blocks_switch_unshaded_##texturing(modulation, dithering, target)      \

#define shade_blocks_switch_shaded_untextured(modulation, dithering, target)   \
  shade_blocks_shaded_untextured                                               \

#define shade_blocks_switch_shaded_textured_unmodulated(dithering, target)     \
  shade_blocks_textured_unmodulated_##target                                   \

#define shade_blocks_switch_shaded_textured_modulated(dithering, target)       \
  shade_blocks_shaded_textured_modulated_##dithering##_##target                \

#define shade_blocks_switch_shaded_textured(modulation, dithering, target)     \
  shade_blocks_switch_shaded_textured_##modulation(dithering, target)          \

#define shade_blocks_switch_shaded(texturing, modulation, dithering, target)   \
  shade_blocks_switch_shaded_##texturing(modulation, dithering, target)        \

#define shade_blocks_switch_mask_off(shading, texturing, modulation,           \
 dithering)                                                                    \
  shade_blocks_switch_##shading(texturing, modulation, dithering, direct)      \

#define shade_blocks_switch_mask_on(shading, texturing, modulation,            \
 dithering)                                                                    \
  shade_blocks_switch_##shading(texturing, modulation, dithering, indirect)    \

#define shade_blocks_switch_blended(shading, texturing, modulation, dithering, \
 mask_evaluate)                                                                \
  shade_blocks_switch_##shading(texturing, modulation, dithering, indirect)    \

#define shade_blocks_switch_unblended(shading, texturing, modulation,          \
 dithering, mask_evaluate)                                                     \
  shade_blocks_switch_mask_##mask_evaluate(shading, texturing, modulation,     \
   dithering)                                                                  \

#define shade_blocks_switch(shading, texturing, modulation, dithering,         \
 blending, mask_evaluate)                                                      \
  shade_blocks_switch_##blending(shading, texturing, modulation, dithering,    \
   mask_evaluate)                                                              \


// Blend blocks parametric-variables:
// TEXTURE_MAP BLEND  BM_A BM_B mask_evaluate
// x           0      x    x    0
// x           0      x    x    1
// 0           1      0    0    0
// 0           1      0    0    1
// 0           1      0    1    0
// 0           1      0    1    1
// 0           1      1    0    0
// 0           1      1    0    1
// 0           1      1    1    0
// 0           1      1    1    1
// 1           1      0    0    0
// 1           1      0    0    1
// 1           1      0    1    0
// 1           1      0    1    1
// 1           1      1    0    0
// 1           1      1    0    1
// 1           1      1    1    0
// 1           1      1    1    1
// 32 inputs, 18 combinations

#define blend_blocks_switch_unblended(texturing, blend_mode, mask_evaluate)    \
  blend_blocks_textured_unblended_##mask_evaluate                              \

#define blend_blocks_switch_blended(texturing, blend_mode, mask_evaluate)      \
  blend_blocks_##texturing##_##blend_mode##_##mask_evaluate                    \

#define blend_blocks_switch(texturing, blending, blend_mode, mask_evaluate)    \
  blend_blocks_switch_##blending(texturing, blend_mode, mask_evaluate)         \


#define render_blocks_switch_block_modulation(texture_mode, blend_mode,        \
 mask_evaluate, shading, dithering, texturing, blending, modulation)           \
{                                                                              \
  setup_blocks_switch(shading, texturing, texture_mode, dithering, blending,   \
   mask_evaluate),                                                             \
  texture_blocks_switch(texturing, texture_mode),                              \
  shade_blocks_switch(shading, texturing, modulation, dithering, blending,     \
   mask_evaluate),                                                             \
  blend_blocks_switch(texturing, blending, blend_mode, mask_evaluate)          \
}                                                                              \

#define render_blocks_switch_block_blending(texture_mode, blend_mode,          \
 mask_evaluate, shading, dithering, texturing, blending)                       \
  render_blocks_switch_block_modulation(texture_mode, blend_mode,              \
   mask_evaluate, shading, dithering, texturing, blending, modulated),         \
  render_blocks_switch_block_modulation(texture_mode, blend_mode,              \
   mask_evaluate, shading, dithering, texturing, blending, unmodulated)        \

#define render_blocks_switch_block_texturing(texture_mode, blend_mode,         \
 mask_evaluate, shading, dithering, texturing)                                 \
  render_blocks_switch_block_blending(texture_mode, blend_mode,                \
   mask_evaluate, shading, dithering, texturing, unblended),                   \
  render_blocks_switch_block_blending(texture_mode, blend_mode,                \
   mask_evaluate, shading, dithering, texturing, blended)                      \

#define render_blocks_switch_block_dithering(texture_mode, blend_mode,         \
 mask_evaluate, shading, dithering)                                            \
  render_blocks_switch_block_texturing(texture_mode, blend_mode,               \
   mask_evaluate, shading, dithering, untextured),                             \
  render_blocks_switch_block_texturing(texture_mode, blend_mode,               \
   mask_evaluate, shading, dithering, textured)                                \

#define render_blocks_switch_block_shading(texture_mode, blend_mode,           \
 mask_evaluate, shading)                                                       \
  render_blocks_switch_block_dithering(texture_mode, blend_mode,               \
   mask_evaluate, shading, undithered),                                        \
  render_blocks_switch_block_dithering(texture_mode, blend_mode,               \
   mask_evaluate, shading, dithered)                                           \

#define render_blocks_switch_block_mask_evaluate(texture_mode, blend_mode,     \
 mask_evaluate)                                                                \
  render_blocks_switch_block_shading(texture_mode, blend_mode, mask_evaluate,  \
   unshaded),                                                                  \
  render_blocks_switch_block_shading(texture_mode, blend_mode, mask_evaluate,  \
   shaded)                                                                     \

#define render_blocks_switch_block_blend_mode(texture_mode, blend_mode)        \
  render_blocks_switch_block_mask_evaluate(texture_mode, blend_mode, off),     \
  render_blocks_switch_block_mask_evaluate(texture_mode, blend_mode, on)       \

#define render_blocks_switch_block_texture_mode(texture_mode)                  \
  render_blocks_switch_block_blend_mode(texture_mode, average),                \
  render_blocks_switch_block_blend_mode(texture_mode, add),                    \
  render_blocks_switch_block_blend_mode(texture_mode, subtract),               \
  render_blocks_switch_block_blend_mode(texture_mode, add_fourth)              \

#define render_blocks_switch_block()                                           \
  render_blocks_switch_block_texture_mode(4bpp),                               \
  render_blocks_switch_block_texture_mode(8bpp),                               \
  render_blocks_switch_block_texture_mode(16bpp),                              \
  render_blocks_switch_block_texture_mode(16bpp)                               \


render_block_handler_struct render_triangle_block_handlers[] =
{
  render_blocks_switch_block()
};

#undef render_blocks_switch_block_modulation

#define render_blocks_switch_block_modulation(texture_mode, blend_mode,        \
 mask_evaluate, shading, dithering, texturing, blending, modulation)           \
  "render flags:\n"                                                            \
  "texture mode:     " #texture_mode "\n"                                      \
  "blend mode:       " #blend_mode "\n"                                        \
  "mask evaluation:  " #mask_evaluate "\n"                                     \
  #shading "\n"                                                                \
  #dithering "\n"                                                              \
  #texturing "\n"                                                              \
  #blending "\n"                                                               \
  #modulation "\n"                                                             \

char *render_block_flag_strings[] =
{                                                                               
  render_blocks_switch_block()
};


#define triangle_y_direction_up   1
#define triangle_y_direction_flat 2
#define triangle_y_direction_down 0

#define triangle_winding_positive 0
#define triangle_winding_negative 1

#define triangle_set_direction(direction_variable, value)                      \
  u32 direction_variable = (u32)(value) >> 31;                                 \
  if(value == 0)                                                               \
    direction_variable = 2                                                     \

#define triangle_case(direction_a, direction_b, direction_c, winding)          \
  case (triangle_y_direction_##direction_a |                                   \
   (triangle_y_direction_##direction_b << 2) |                                 \
   (triangle_y_direction_##direction_c << 4) |                                 \
   (triangle_winding_##winding << 6))                                          \

static int prepare_triangle(psx_gpu_struct *psx_gpu, vertex_struct *vertexes,
 prepared_triangle *triangle_out)
{
  s32 y_top, y_bottom, offset_x, offset_y, i;
  s32 triangle_area;
  u32 triangle_winding = 0;

  vertex_struct *a = &(vertexes[0]);
  vertex_struct *b = &(vertexes[1]);
  vertex_struct *c = &(vertexes[2]);

  triangle_area = triangle_signed_area_x2(a->x, a->y, b->x, b->y, c->x, c->y);

#ifdef PROFILE
  triangles++;
#endif

  if(triangle_area == 0)
  {
#ifdef PROFILE
    trivial_rejects++;
#endif
    return 0;
  }

  if(b->y < a->y)
    vertex_swap(a, b);

  if(c->y < b->y)
  {
    vertex_swap(b, c);

    if(b->y < a->y)
      vertex_swap(a, b);
  }

  y_bottom = c->y;
  y_top = a->y;
  offset_y = sign_extend_11bit(y_top + psx_gpu->offset_y) - y_top;

  if((y_bottom - y_top) >= 512)
  {
#ifdef PROFILE
    trivial_rejects++;
#endif
    return 0;
  }

  if(triangle_area < 0)
  {
    triangle_area = -triangle_area;
    triangle_winding ^= 1;
    vertex_swap(a, c);
  }

  if(b->x < a->x)
    vertex_swap(a, b);

  if(c->x < b->x) 
  {
    vertex_swap(b, c);

    if(b->x < a->x)
      vertex_swap(a, b);
  }

  if(c->x - a->x >= 1024)
  {
#ifdef PROFILE
    trivial_rejects++;
#endif
    return 0;
  }

  offset_x = sign_extend_11bit(a->x + psx_gpu->offset_x) - a->x;
  if(invalidate_texture_cache_region_viewport(psx_gpu,
      a->x + offset_x, y_top + offset_y,
      c->x + offset_x, y_bottom + offset_y) == 0)
  {
#ifdef PROFILE
    trivial_rejects++;
#endif
    return 0;
  }

  for (i = 0; i < 3; i++)
  {
    vertexes[i].x += offset_x;
    vertexes[i].y += offset_y;
  }

  psx_gpu->triangle_area = triangle_area;
  psx_gpu->triangle_winding = triangle_winding;

  triangle_out->vertexes[0] = a;
  triangle_out->vertexes[1] = b;
  triangle_out->vertexes[2] = c;
  triangle_out->offset_x = offset_x;
  triangle_out->offset_y = offset_y;

  return 1;
}

static void render_triangle_p(psx_gpu_struct *psx_gpu,
 vertex_struct *vertex_ptrs[3], u32 flags)
{
  psx_gpu->num_spans = 0;

  vertex_struct *a = vertex_ptrs[0];
  vertex_struct *b = vertex_ptrs[1];
  vertex_struct *c = vertex_ptrs[2];

  s32 y_delta_a = b->y - a->y;
  s32 y_delta_b = c->y - b->y;
  s32 y_delta_c = c->y - a->y;

  triangle_set_direction(y_direction_a, y_delta_a);
  triangle_set_direction(y_direction_b, y_delta_b);
  triangle_set_direction(y_direction_c, y_delta_c);

#if defined(__arm__) && defined(NEON_BUILD) && !defined(SIMD_BUILD)
  // the asm doesn't bother to save callee-save vector regs, so do it here
  __asm__ __volatile__("vstmia %0, {q4-q7}" :: "r"(psx_gpu->saved_q4_q7) : "memory");
#endif

  compute_all_gradients(psx_gpu, a, b, c);

  switch(y_direction_a | (y_direction_b << 2) | (y_direction_c << 4) |
   (psx_gpu->triangle_winding << 6))
  {
    triangle_case(up, up, up, negative):
    triangle_case(up, up, flat, negative):
    triangle_case(up, up, down, negative):
      setup_spans_up_right(psx_gpu, a, b, c);
      break;

    triangle_case(flat, up, up, negative):
    triangle_case(flat, up, flat, negative):
    triangle_case(flat, up, down, negative):
      setup_spans_up_a(psx_gpu, a, b, c);
      break;

    triangle_case(down, up, up, negative):
      setup_spans_up_down(psx_gpu, a, c, b);
      break;

    triangle_case(down, up, flat, negative):
      setup_spans_down_a(psx_gpu, a, c, b);
      break;

    triangle_case(down, up, down, negative):
      setup_spans_down_right(psx_gpu, a, c, b);
      break;

    triangle_case(down, flat, up, negative):
    triangle_case(down, flat, flat, negative):
    triangle_case(down, flat, down, negative):
      setup_spans_down_b(psx_gpu, a, b, c);
      break;

    triangle_case(down, down, up, negative):
    triangle_case(down, down, flat, negative):
    triangle_case(down, down, down, negative):
      setup_spans_down_left(psx_gpu, a, b, c);
      break;

    triangle_case(up, up, up, positive):
    triangle_case(up, up, flat, positive):
    triangle_case(up, up, down, positive):
      setup_spans_up_left(psx_gpu, a, b, c);
      break;

    triangle_case(up, flat, up, positive):
    triangle_case(up, flat, flat, positive):
    triangle_case(up, flat, down, positive):
      setup_spans_up_b(psx_gpu, a, b, c);
      break;

    triangle_case(up, down, up, positive):
      setup_spans_up_right(psx_gpu, a, c, b);
      break;

    triangle_case(up, down, flat, positive):
      setup_spans_up_a(psx_gpu, a, c, b);
      break;

    triangle_case(up, down, down, positive):
      setup_spans_up_down(psx_gpu, a, b, c);
      break;

    triangle_case(flat, down, up, positive):
    triangle_case(flat, down, flat, positive):
    triangle_case(flat, down, down, positive):
      setup_spans_down_a(psx_gpu, a, b, c);
      break;

    triangle_case(down, down, up, positive):
    triangle_case(down, down, flat, positive):
    triangle_case(down, down, down, positive):
      setup_spans_down_right(psx_gpu, a, b, c);
      break;
  }

#ifdef PROFILE
  spans += psx_gpu->num_spans;
#endif

  if(unlikely(psx_gpu->render_mode & RENDER_INTERLACE_ENABLED))
  {
    u32 i;

    if(psx_gpu->render_mode & RENDER_INTERLACE_ODD)
    {
      for(i = 0; i < psx_gpu->num_spans; i++)
      {
        if((psx_gpu->span_edge_data[i].y & 1) == 0)
          psx_gpu->span_edge_data[i].num_blocks = 0;
      }
    }
    else
    {
      for(i = 0; i < psx_gpu->num_spans; i++)
      {
        if(psx_gpu->span_edge_data[i].y & 1)
          psx_gpu->span_edge_data[i].num_blocks = 0;
      }
    }
  }
  assert(psx_gpu->span_edge_data[0].y < 1024u);

  u32 render_state = flags &
   (RENDER_FLAGS_MODULATE_TEXELS | RENDER_FLAGS_BLEND | 
   RENDER_FLAGS_TEXTURE_MAP | RENDER_FLAGS_SHADE);
  render_state |= psx_gpu->render_state_base;
  
  if((psx_gpu->render_state != render_state) ||
   (psx_gpu->primitive_type != PRIMITIVE_TYPE_TRIANGLE))
  {
    psx_gpu->render_state = render_state;
    flush_render_block_buffer(psx_gpu);
#ifdef PROFILE
    state_changes++;
#endif
  }

  psx_gpu->primitive_type = PRIMITIVE_TYPE_TRIANGLE;

  psx_gpu->render_block_handler =
   &(render_triangle_block_handlers[render_state]);
  ((setup_blocks_function_type *)psx_gpu->render_block_handler->setup_blocks)
   (psx_gpu);

#if defined(__arm__) && defined(NEON_BUILD) && !defined(SIMD_BUILD)
  __asm__ __volatile__("vldmia %0, {q4-q7}" :: "r"(psx_gpu->saved_q4_q7));
#endif
}

void render_triangle(psx_gpu_struct *psx_gpu, vertex_struct *vertexes,
 u32 flags)
{
  prepared_triangle triangle;
  if (prepare_triangle(psx_gpu, vertexes, &triangle))
    render_triangle_p(psx_gpu, triangle.vertexes, flags);
}

#if !defined(NEON_BUILD) || defined(SIMD_BUILD)

void texture_sprite_blocks_8bpp(psx_gpu_struct *psx_gpu)
{
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;

  vec_8x16u texels;
  vec_8x8u texel_indexes;

  u16 *clut_ptr = psx_gpu->clut_ptr;
  u32 i;

  while(num_blocks)
  {
    texel_indexes = block->r;

    for(i = 0; i < 8; i++)
    {
      texels.e[i] = clut_ptr[texel_indexes.e[i]];
    }

    block->texels = texels;

    num_blocks--;
    block++;
  }
}

#endif


#define setup_sprite_tiled_initialize_4bpp_clut()                              \
  u16 *clut_ptr = psx_gpu->clut_ptr;                                           \
  vec_8x16u clut_a, clut_b;                                                    \
  vec_16x8u clut_low, clut_high;                                               \
                                                                               \
  load_8x16b(clut_a, clut_ptr);                                                \
  load_8x16b(clut_b, clut_ptr + 8);                                            \
  unzip_16x8b(clut_low, clut_high, clut_a, clut_b)                             \

#define setup_sprite_tiled_initialize_4bpp()                                   \
  setup_sprite_tiled_initialize_4bpp_clut();                                   \
                                                                               \
  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_4bpp_mask)        \
    update_texture_4bpp_cache(psx_gpu)                                         \

#define setup_sprite_tiled_initialize_8bpp()                                   \
  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_8bpp_mask)        \
    update_texture_8bpp_cache(psx_gpu)                                         \


#define setup_sprite_tile_fetch_texel_block_8bpp(offset)                       \
  texture_block_ptr = (u8 *)psx_gpu->texture_page_ptr +                        \
   ((texture_offset + offset) & texture_mask);                                 \
                                                                               \
  load_64b(texels, texture_block_ptr)                                          \


#define setup_sprite_tile_add_blocks(tile_num_blocks)                          \
  num_blocks += tile_num_blocks;                                               \
  sprite_blocks += tile_num_blocks;                                            \
                                                                               \
  if(num_blocks > MAX_BLOCKS)                                                  \
  {                                                                            \
    flush_render_block_buffer(psx_gpu);                                        \
    num_blocks = tile_num_blocks;                                              \
    block = psx_gpu->blocks;                                                   \
  }                                                                            \

#define setup_sprite_tile_full_4bpp(edge)                                      \
{                                                                              \
  vec_8x8u texels_low, texels_high;                                            \
  vec_8x16u pixels;                                                            \
  setup_sprite_tile_add_blocks(sub_tile_height * 2);                           \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    tbl_16(texels_low, texels, clut_low);                                      \
    tbl_16(texels_high, texels, clut_high);                                    \
    zip_8x16b(pixels, texels_low, texels_high);                                \
                                                                               \
    block->texels = pixels;                                                    \
    block->draw_mask_bits = left_mask_bits;                                    \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    tbl_16(texels_low, texels, clut_low);                                      \
    tbl_16(texels_high, texels, clut_high);                                    \
    zip_8x16b(pixels, texels_low, texels_high);                                \
                                                                               \
    block->texels = pixels;                                                    \
    block->draw_mask_bits = right_mask_bits;                                   \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    fb_ptr += 1024;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

#define setup_sprite_tile_half_4bpp(edge)                                      \
{                                                                              \
  vec_8x8u texels_low, texels_high;                                            \
  vec_8x16u pixels;                                                            \
  setup_sprite_tile_add_blocks(sub_tile_height);                               \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    tbl_16(texels_low, texels, clut_low);                                      \
    tbl_16(texels_high, texels, clut_high);                                    \
    zip_8x16b(pixels, texels_low, texels_high);                                \
                                                                               \
    block->texels = pixels;                                                    \
    block->draw_mask_bits = edge##_mask_bits;                                  \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    fb_ptr += 1024;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

  
#define setup_sprite_tile_full_8bpp(edge)                                      \
{                                                                              \
  setup_sprite_tile_add_blocks(sub_tile_height * 2);                           \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    block->r = texels;                                                         \
    block->draw_mask_bits = left_mask_bits;                                    \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    block->r = texels;                                                         \
    block->draw_mask_bits = right_mask_bits;                                   \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    fb_ptr += 1024;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

#define setup_sprite_tile_half_8bpp(edge)                                      \
{                                                                              \
  setup_sprite_tile_add_blocks(sub_tile_height);                               \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    block->r = texels;                                                         \
    block->draw_mask_bits = edge##_mask_bits;                                  \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    fb_ptr += 1024;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

  
#define setup_sprite_tile_column_edge_pre_adjust_half_right()                  \
  texture_offset = texture_offset_base + 8;                                    \
  fb_ptr += 8                                                                  \

#define setup_sprite_tile_column_edge_pre_adjust_half_left()                   \
  texture_offset = texture_offset_base                                         \

#define setup_sprite_tile_column_edge_pre_adjust_half(edge)                    \
  setup_sprite_tile_column_edge_pre_adjust_half_##edge()                       \

#define setup_sprite_tile_column_edge_pre_adjust_full(edge)                    \
  texture_offset = texture_offset_base                                         \

#define setup_sprite_tile_column_edge_post_adjust_half_right()                 \
  fb_ptr -= 8                                                                  \

#define setup_sprite_tile_column_edge_post_adjust_half_left()                  \

#define setup_sprite_tile_column_edge_post_adjust_half(edge)                   \
  setup_sprite_tile_column_edge_post_adjust_half_##edge()                      \

#define setup_sprite_tile_column_edge_post_adjust_full(edge)                   \


#define setup_sprite_tile_column_height_single(edge_mode, edge, texture_mode,  \
 x4mode)                                                                       \
do                                                                             \
{                                                                              \
  sub_tile_height = column_data;                                               \
  setup_sprite_tile_column_edge_pre_adjust_##edge_mode##x4mode(edge);          \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
  setup_sprite_tile_column_edge_post_adjust_##edge_mode##x4mode(edge);         \
} while(0)                                                                     \

#define setup_sprite_tile_column_height_multi(edge_mode, edge, texture_mode,   \
 x4mode)                                                                       \
do                                                                             \
{                                                                              \
  u32 tiles_remaining = column_data >> 16;                                     \
  sub_tile_height = column_data & 0xFF;                                        \
  setup_sprite_tile_column_edge_pre_adjust_##edge_mode##x4mode(edge);          \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
  tiles_remaining -= 1;                                                        \
                                                                               \
  while(tiles_remaining)                                                       \
  {                                                                            \
    sub_tile_height = 16;                                                      \
    setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);              \
    tiles_remaining--;                                                         \
  }                                                                            \
                                                                               \
  sub_tile_height = (column_data >> 8) & 0xFF;                                 \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
  setup_sprite_tile_column_edge_post_adjust_##edge_mode##x4mode(edge);         \
} while(0)                                                                     \


#define setup_sprite_column_data_single()                                      \
  column_data = height                                                         \

#define setup_sprite_column_data_multi()                                       \
  column_data = 16 - offset_v;                                                 \
  column_data |= ((height_rounded & 0xF) + 1) << 8;                            \
  column_data |= (tile_height - 1) << 16                                       \


#define RIGHT_MASK_BIT_SHIFT 8
#define RIGHT_MASK_BIT_SHIFT_4x 16

#define setup_sprite_tile_column_width_single(texture_mode, multi_height,      \
 edge_mode, edge, x4mode)                                                      \
{                                                                              \
  setup_sprite_column_data_##multi_height();                                   \
  left_mask_bits = left_block_mask | right_block_mask;                         \
  right_mask_bits = left_mask_bits >> RIGHT_MASK_BIT_SHIFT##x4mode;            \
                                                                               \
  setup_sprite_tile_column_height_##multi_height(edge_mode, edge,              \
   texture_mode, x4mode);                                                      \
}                                                                              \

#define setup_sprite_tiled_advance_column()                                    \
  texture_offset_base += 0x100;                                                \
  if((texture_offset_base & 0xF00) == 0)                                       \
    texture_offset_base -= (0x100 + 0xF00)                                     \

#define FB_PTR_MULTIPLIER 1
#define FB_PTR_MULTIPLIER_4x 2

#define setup_sprite_tile_column_width_multi(texture_mode, multi_height,       \
 left_mode, right_mode, x4mode)                                                \
{                                                                              \
  setup_sprite_column_data_##multi_height();                                   \
  s32 fb_ptr_advance_column = (16 - (1024 * height))                           \
    * FB_PTR_MULTIPLIER##x4mode;                                               \
                                                                               \
  tile_width -= 2;                                                             \
  left_mask_bits = left_block_mask;                                            \
  right_mask_bits = left_mask_bits >> RIGHT_MASK_BIT_SHIFT##x4mode;            \
                                                                               \
  setup_sprite_tile_column_height_##multi_height(left_mode, right,             \
   texture_mode, x4mode);                                                      \
  fb_ptr += fb_ptr_advance_column;                                             \
                                                                               \
  left_mask_bits = 0x00;                                                       \
  right_mask_bits = 0x00;                                                      \
                                                                               \
  while(tile_width)                                                            \
  {                                                                            \
    setup_sprite_tiled_advance_column();                                       \
    setup_sprite_tile_column_height_##multi_height(full, none,                 \
     texture_mode, x4mode);                                                    \
    fb_ptr += fb_ptr_advance_column;                                           \
    tile_width--;                                                              \
  }                                                                            \
                                                                               \
  left_mask_bits = right_block_mask;                                           \
  right_mask_bits = left_mask_bits >> RIGHT_MASK_BIT_SHIFT##x4mode;            \
                                                                               \
  setup_sprite_tiled_advance_column();                                         \
  setup_sprite_tile_column_height_##multi_height(right_mode, left,             \
   texture_mode, x4mode);                                                      \
}                                                                              \


/* 4x stuff */
#define setup_sprite_tiled_initialize_4bpp_4x()                                \
  setup_sprite_tiled_initialize_4bpp_clut()                                    \

#define setup_sprite_tiled_initialize_8bpp_4x()                                \


#define setup_sprite_tile_full_4bpp_4x(edge)                                   \
{                                                                              \
  vec_8x8u texels_low, texels_high;                                            \
  vec_8x16u pixels, pixels_wide;                                               \
  setup_sprite_tile_add_blocks(sub_tile_height * 2 * 4);                       \
  u32 left_mask_bits_a = left_mask_bits & 0xFF;                                \
  u32 left_mask_bits_b = left_mask_bits >> 8;                                  \
  u32 right_mask_bits_a = right_mask_bits & 0xFF;                              \
  u32 right_mask_bits_b = right_mask_bits >> 8;                                \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    tbl_16(texels_low, texels, clut_low);                                      \
    tbl_16(texels_high, texels, clut_high);                                    \
    zip_8x16b(pixels, texels_low, texels_high);                                \
                                                                               \
    zip_4x32b(vector_cast(vec_4x32u, pixels_wide), pixels.low, pixels.low);    \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    zip_4x32b(vector_cast(vec_4x32u, pixels_wide), pixels.high, pixels.high);  \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 1024 + 8;                                         \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    tbl_16(texels_low, texels, clut_low);                                      \
    tbl_16(texels_high, texels, clut_high);                                    \
    zip_8x16b(pixels, texels_low, texels_high);                                \
                                                                               \
    zip_4x32b(vector_cast(vec_4x32u, pixels_wide), pixels.low, pixels.low);    \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 16;                                               \
    block++;                                                                   \
                                                                               \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 1024 + 16;                                        \
    block++;                                                                   \
                                                                               \
    zip_4x32b(vector_cast(vec_4x32u, pixels_wide), pixels.high, pixels.high);  \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = right_mask_bits_b;                                 \
    block->fb_ptr = fb_ptr + 24;                                               \
    block++;                                                                   \
                                                                               \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = right_mask_bits_b;                                 \
    block->fb_ptr = fb_ptr + 1024 + 24;                                        \
    block++;                                                                   \
                                                                               \
    fb_ptr += 2048;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

#define setup_sprite_tile_half_4bpp_4x(edge)                                   \
{                                                                              \
  vec_8x8u texels_low, texels_high;                                            \
  vec_8x16u pixels, pixels_wide;                                               \
  setup_sprite_tile_add_blocks(sub_tile_height * 4);                           \
  u32 edge##_mask_bits_a = edge##_mask_bits & 0xFF;                            \
  u32 edge##_mask_bits_b = edge##_mask_bits >> 8;                              \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    tbl_16(texels_low, texels, clut_low);                                      \
    tbl_16(texels_high, texels, clut_high);                                    \
    zip_8x16b(pixels, texels_low, texels_high);                                \
                                                                               \
    zip_4x32b(vector_cast(vec_4x32u, pixels_wide), pixels.low, pixels.low);    \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    zip_4x32b(vector_cast(vec_4x32u, pixels_wide), pixels.high, pixels.high);  \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = edge##_mask_bits_b;                                \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    block->texels = pixels_wide;                                               \
    block->draw_mask_bits = edge##_mask_bits_b;                                \
    block->fb_ptr = fb_ptr + 1024 + 8;                                         \
    block++;                                                                   \
                                                                               \
    fb_ptr += 2048;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

  
#define setup_sprite_tile_full_8bpp_4x(edge)                                   \
{                                                                              \
  setup_sprite_tile_add_blocks(sub_tile_height * 2 * 4);                       \
  vec_16x8u texels_wide;                                                       \
  u32 left_mask_bits_a = left_mask_bits & 0xFF;                                \
  u32 left_mask_bits_b = left_mask_bits >> 8;                                  \
  u32 right_mask_bits_a = right_mask_bits & 0xFF;                              \
  u32 right_mask_bits_b = right_mask_bits >> 8;                                \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    zip_8x16b(vector_cast(vec_8x16u, texels_wide), texels, texels);            \
    block->r = texels_wide.low;                                                \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.low;                                                \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.high;                                               \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.high;                                               \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 1024 + 8;                                         \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    zip_8x16b(vector_cast(vec_8x16u, texels_wide), texels, texels);            \
    block->r = texels_wide.low;                                                \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 16;                                               \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.low;                                                \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 1024 + 16;                                        \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.high;                                               \
    block->draw_mask_bits = right_mask_bits_b;                                 \
    block->fb_ptr = fb_ptr + 24;                                               \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.high;                                               \
    block->draw_mask_bits = right_mask_bits_b;                                 \
    block->fb_ptr = fb_ptr + 24 + 1024;                                        \
    block++;                                                                   \
                                                                               \
    fb_ptr += 2048;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

#define setup_sprite_tile_half_8bpp_4x(edge)                                   \
{                                                                              \
  setup_sprite_tile_add_blocks(sub_tile_height * 4);                           \
  vec_16x8u texels_wide;                                                       \
  u32 edge##_mask_bits_a = edge##_mask_bits & 0xFF;                            \
  u32 edge##_mask_bits_b = edge##_mask_bits >> 8;                              \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    zip_8x16b(vector_cast(vec_8x16u, texels_wide), texels, texels);            \
    block->r = texels_wide.low;                                                \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.low;                                                \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.high;                                               \
    block->draw_mask_bits = edge##_mask_bits_b;                                \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    block->r = texels_wide.high;                                               \
    block->draw_mask_bits = edge##_mask_bits_b;                                \
    block->fb_ptr = fb_ptr + 8 + 1024;                                         \
    block++;                                                                   \
                                                                               \
    fb_ptr += 2048;                                                            \
    texture_offset += 0x10;                                                    \
    sub_tile_height--;                                                         \
  }                                                                            \
  texture_offset += 0xF00;                                                     \
  psx_gpu->num_blocks = num_blocks;                                            \
}                                                                              \

  
#define setup_sprite_tile_column_edge_pre_adjust_half_right_4x()               \
  texture_offset = texture_offset_base + 8;                                    \
  fb_ptr += 16                                                                 \

#define setup_sprite_tile_column_edge_pre_adjust_half_left_4x()                \
  texture_offset = texture_offset_base                                         \

#define setup_sprite_tile_column_edge_pre_adjust_half_4x(edge)                 \
  setup_sprite_tile_column_edge_pre_adjust_half_##edge##_4x()                  \

#define setup_sprite_tile_column_edge_pre_adjust_full_4x(edge)                 \
  texture_offset = texture_offset_base                                         \

#define setup_sprite_tile_column_edge_post_adjust_half_right_4x()              \
  fb_ptr -= 16                                                                 \

#define setup_sprite_tile_column_edge_post_adjust_half_left_4x()               \

#define setup_sprite_tile_column_edge_post_adjust_half_4x(edge)                \
  setup_sprite_tile_column_edge_post_adjust_half_##edge##_4x()                 \

#define setup_sprite_tile_column_edge_post_adjust_full_4x(edge)                \


#define setup_sprite_offset_u_adjust()                                         \

#define setup_sprite_comapre_left_block_mask()                                 \
  ((left_block_mask & 0xFF) == 0xFF)                                           \

#define setup_sprite_comapre_right_block_mask()                                \
  (((right_block_mask >> 8) & 0xFF) == 0xFF)                                   \


#define setup_sprite_offset_u_adjust_4x()                                      \
  offset_u *= 2;                                                               \
  offset_u_right = offset_u_right * 2 + 1                                      \

#define setup_sprite_comapre_left_block_mask_4x()                              \
  ((left_block_mask & 0xFFFF) == 0xFFFF)                                       \

#define setup_sprite_comapre_right_block_mask_4x()                             \
  (((right_block_mask >> 16) & 0xFFFF) == 0xFFFF)                              \


#define setup_sprite_tiled_builder(texture_mode, x4mode)                       \
void setup_sprite_##texture_mode##x4mode(psx_gpu_struct *psx_gpu, s32 x, s32 y,\
 s32 u, s32 v, s32 width, s32 height, u32 color)                               \
{                                                                              \
  s32 offset_u = u & 0xF;                                                      \
  s32 offset_v = v & 0xF;                                                      \
                                                                               \
  s32 width_rounded = offset_u + width + 15;                                   \
  s32 height_rounded = offset_v + height + 15;                                 \
  s32 tile_height = height_rounded / 16;                                       \
  s32 tile_width = width_rounded / 16;                                         \
  u32 offset_u_right = width_rounded & 0xF;                                    \
                                                                               \
  setup_sprite_offset_u_adjust##x4mode();                                      \
                                                                               \
  u32 left_block_mask = ~(0xFFFFFFFF << offset_u);                             \
  u32 right_block_mask = 0xFFFFFFFE << offset_u_right;                         \
                                                                               \
  u32 left_mask_bits;                                                          \
  u32 right_mask_bits;                                                         \
                                                                               \
  u32 sub_tile_height;                                                         \
  u32 column_data;                                                             \
                                                                               \
  u32 texture_mask = (psx_gpu->texture_mask_width & 0xF) |                     \
   ((psx_gpu->texture_mask_height & 0xF) << 4) |                               \
   ((psx_gpu->texture_mask_width >> 4) << 8) |                                 \
   ((psx_gpu->texture_mask_height >> 4) << 12);                                \
  u32 texture_offset = ((v & 0xF) << 4) | ((u & 0xF0) << 4) |                  \
   ((v & 0xF0) << 8);                                                          \
  u32 texture_offset_base = texture_offset;                                    \
  u32 control_mask;                                                            \
                                                                               \
  u16 *fb_ptr = psx_gpu->vram_out_ptr + (y * 1024) + (x - offset_u);           \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  block_struct *block = psx_gpu->blocks + num_blocks;                          \
                                                                               \
  u8 *texture_block_ptr;                                                       \
  vec_8x8u texels;                                                             \
                                                                               \
  setup_sprite_tiled_initialize_##texture_mode##x4mode();                      \
                                                                               \
  control_mask = tile_width == 1;                                              \
  control_mask |= (tile_height == 1) << 1;                                     \
  control_mask |= setup_sprite_comapre_left_block_mask##x4mode() << 2;         \
  control_mask |= setup_sprite_comapre_right_block_mask##x4mode() << 3;        \
                                                                               \
  sprites_##texture_mode++;                                                    \
                                                                               \
  switch(control_mask)                                                         \
  {                                                                            \
    default:                                                                   \
    case 0x0:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, multi, full, full,    \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x1:                                                                  \
      setup_sprite_tile_column_width_single(texture_mode, multi, full, none,   \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x2:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, single, full, full,   \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x3:                                                                  \
      setup_sprite_tile_column_width_single(texture_mode, single, full, none,  \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x4:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, multi, half, full,    \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x5:                                                                  \
      setup_sprite_tile_column_width_single(texture_mode, multi, half, right,  \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x6:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, single, half, full,   \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x7:                                                                  \
      setup_sprite_tile_column_width_single(texture_mode, single, half, right, \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x8:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, multi, full, half,    \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0x9:                                                                  \
      setup_sprite_tile_column_width_single(texture_mode, multi, half, left,   \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0xA:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, single, full, half,   \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0xB:                                                                  \
      setup_sprite_tile_column_width_single(texture_mode, single, half, left,  \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0xC:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, multi, half, half,    \
       x4mode);                                                                \
      break;                                                                   \
                                                                               \
    case 0xE:                                                                  \
      setup_sprite_tile_column_width_multi(texture_mode, single, half, half,   \
       x4mode);                                                                \
      break;                                                                   \
  }                                                                            \
}                                                                              \

#ifndef NEON_BUILD
setup_sprite_tiled_builder(4bpp,);
setup_sprite_tiled_builder(8bpp,);

setup_sprite_tiled_builder(4bpp,_4x);
setup_sprite_tiled_builder(8bpp,_4x);
#endif

#if !defined(NEON_BUILD) || defined(SIMD_BUILD)

void setup_sprite_16bpp(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color)
{
  u32 left_offset = u & 0x7;
  u32 width_rounded = width + left_offset + 7;

  u16 *fb_ptr = psx_gpu->vram_out_ptr + (y * 1024) + (s32)(x - left_offset);
  u32 right_width = width_rounded & 0x7;
  u32 block_width = width_rounded / 8;
  u32 fb_ptr_pitch = (1024 + 8) - (block_width * 8);

  u32 left_mask_bits = ~(0xFF << left_offset);
  u32 right_mask_bits = 0xFE << right_width;

  u32 texture_offset_base = u + (v * 1024);
  u32 texture_mask =
   psx_gpu->texture_mask_width | (psx_gpu->texture_mask_height * 1024);

  u32 blocks_remaining;
  u32 num_blocks = psx_gpu->num_blocks;
  block_struct *block = psx_gpu->blocks + num_blocks;

  u16 *texture_page_ptr = psx_gpu->texture_page_ptr;
  u16 *texture_block_ptr;

  texture_offset_base &= ~0x7;

  stats_add(sprites_16bpp, 1);

  if(block_width == 1)
  {
    u32 mask_bits = left_mask_bits | right_mask_bits;

    while(height)
    {
      num_blocks++;
      sprite_blocks++;

      if(num_blocks > MAX_BLOCKS)
      {
        flush_render_block_buffer(psx_gpu);
        num_blocks = 1;
        block = psx_gpu->blocks;
      }
      
      texture_block_ptr =
       texture_page_ptr + (texture_offset_base & texture_mask);

      block->texels = *(vec_8x16u *)texture_block_ptr;
      block->draw_mask_bits = mask_bits;
      block->fb_ptr = fb_ptr;

      block++;

      texture_offset_base += 1024;
      fb_ptr += 1024;

      height--;
      psx_gpu->num_blocks = num_blocks;
    }
  }
  else
  {
    u32 texture_offset;

    while(height)
    {
      blocks_remaining = block_width - 2;
      num_blocks += block_width;
      sprite_blocks += block_width;

      if(num_blocks > MAX_BLOCKS)
      {
        flush_render_block_buffer(psx_gpu);
        num_blocks = block_width;
        block = psx_gpu->blocks;
      }

      texture_offset = texture_offset_base;
      texture_offset_base += 1024;

      texture_block_ptr = texture_page_ptr + (texture_offset & texture_mask);
      block->texels = *(vec_8x16u *)texture_block_ptr;

      block->draw_mask_bits = left_mask_bits;
      block->fb_ptr = fb_ptr;

      texture_offset += 8;
      fb_ptr += 8;
      block++;

      while(blocks_remaining)
      {
        texture_block_ptr = texture_page_ptr + (texture_offset & texture_mask);
        block->texels = *(vec_8x16u *)texture_block_ptr;

        block->draw_mask_bits = 0;
        block->fb_ptr = fb_ptr;

        texture_offset += 8;
        fb_ptr += 8;
        block++;

        blocks_remaining--;
      }

      texture_block_ptr = texture_page_ptr + (texture_offset & texture_mask);
      block->texels = *(vec_8x16u *)texture_block_ptr;

      block->draw_mask_bits = right_mask_bits;
      block->fb_ptr = fb_ptr;

      fb_ptr += fb_ptr_pitch;
      block++;

      height--;
      psx_gpu->num_blocks = num_blocks;
    }
  }
}

#endif

#ifndef NEON_BUILD

void setup_sprite_untextured_512(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color)
{
  u32 right_width = ((width - 1) & 0x7) + 1;
  u32 right_mask_bits = (0xFF << right_width);
  u16 *fb_ptr = psx_gpu->vram_out_ptr + (y * 1024) + x;
  u32 block_width = (width + 7) / 8;
  u32 fb_ptr_pitch = 1024 - ((block_width - 1) * 8);
  u32 blocks_remaining;
  u32 num_blocks = psx_gpu->num_blocks;
  block_struct *block = psx_gpu->blocks + num_blocks;

  u32 color_r = color & 0xFF;
  u32 color_g = (color >> 8) & 0xFF;
  u32 color_b = (color >> 16) & 0xFF;
  vec_8x16u colors;
  vec_8x16u right_mask;
  vec_8x16u test_mask = psx_gpu->test_mask;
  vec_8x16u zero_mask;

  sprites_untextured++;

  color = (color_r >> 3) | ((color_g >> 3) << 5) | ((color_b >> 3) << 10);

  dup_8x16b(colors, color);
  dup_8x16b(zero_mask, 0x00);
  dup_8x16b(right_mask, right_mask_bits);
  tst_8x16b(right_mask, right_mask, test_mask);

  while(height)
  {
    blocks_remaining = block_width - 1;
    num_blocks += block_width;

#ifdef PROFILE
    sprite_blocks += block_width;
#endif

    if(num_blocks > MAX_BLOCKS)
    {
      flush_render_block_buffer(psx_gpu);
      num_blocks = block_width;
      block = psx_gpu->blocks;
    }

    while(blocks_remaining)
    {
      block->pixels = colors;
      block->draw_mask = zero_mask;
      block->fb_ptr = fb_ptr;

      fb_ptr += 8;
      block++;
      blocks_remaining--;
    }

    block->pixels = colors;
    block->draw_mask = right_mask;
    block->fb_ptr = fb_ptr;

    block++;
    fb_ptr += fb_ptr_pitch;

    height--;
    psx_gpu->num_blocks = num_blocks;
  }
}

#endif

static void __attribute__((noinline))
setup_sprite_untextured_simple(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color)
{
  u32 r = color & 0xFF;
  u32 g = (color >> 8) & 0xFF;
  u32 b = (color >> 16) & 0xFF;
  u32 color_16bpp = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) |
   psx_gpu->mask_msb;
  u32 color_32bpp = color_16bpp | (color_16bpp << 16);

  u16 *vram_ptr16 = psx_gpu->vram_out_ptr + x + (y * 1024);
  u32 *vram_ptr;

  u32 num_width;

  if(psx_gpu->num_blocks)
  {
    flush_render_block_buffer(psx_gpu);
  }

  while(height)
  {
    num_width = width;

    vram_ptr = (void *)vram_ptr16;
    if((uintptr_t)vram_ptr16 & 2)
    {
      *vram_ptr16 = color_32bpp;
      vram_ptr = (void *)(vram_ptr16 + 1);
      num_width--;
    }

    while(num_width >= 4 * 2)
    {
      vram_ptr[0] = color_32bpp;
      vram_ptr[1] = color_32bpp;
      vram_ptr[2] = color_32bpp;
      vram_ptr[3] = color_32bpp;

      vram_ptr += 4;
      num_width -= 4 * 2;
    }

    while(num_width >= 2)
    {
      *vram_ptr++ = color_32bpp;
      num_width -= 2;
    }

    if(num_width > 0)
    {
      *(u16 *)vram_ptr = color_32bpp;
    }

    vram_ptr16 += 1024;
    height--;
  }
}

void setup_sprite_untextured_512(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color);

void setup_sprite_untextured(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color)
{
  if((psx_gpu->render_state & (RENDER_STATE_MASK_EVALUATE |
   RENDER_FLAGS_MODULATE_TEXELS | RENDER_FLAGS_BLEND)) == 0 &&
   (psx_gpu->render_mode & RENDER_INTERLACE_ENABLED) == 0)
  {
    setup_sprite_untextured_simple(psx_gpu, x, y, u, v, width, height, color);
    return;
  }

  while (width > 0)
  {
    s32 w1 = width > 512 ? 512 : width;
    setup_sprite_untextured_512(psx_gpu, x, y, 0, 0, w1, height, color);
    x += 512;
    width -= 512;
  }
}


#define setup_sprite_blocks_switch_textured(texture_mode)                      \
  setup_sprite_##texture_mode                                                  \

#define setup_sprite_blocks_switch_untextured(texture_mode)                    \
  setup_sprite_untextured                                                      \

#define setup_sprite_blocks_switch(texturing, texture_mode)                    \
  setup_sprite_blocks_switch_##texturing(texture_mode)                         \


#define texture_sprite_blocks_switch_4bpp()                                    \
  texture_blocks_untextured                                                    \

#define texture_sprite_blocks_switch_8bpp()                                    \
  texture_sprite_blocks_8bpp                                                   \

#define texture_sprite_blocks_switch_16bpp()                                   \
  texture_blocks_untextured                                                    \

#define texture_sprite_blocks_switch_untextured(texture_mode)                  \
  texture_blocks_untextured                                                    \

#define texture_sprite_blocks_switch_textured(texture_mode)                    \
  texture_sprite_blocks_switch_##texture_mode()                                \

#define render_sprite_blocks_switch_block_modulation(texture_mode, blend_mode, \
 mask_evaluate, shading, dithering, texturing, blending, modulation)           \
{                                                                              \
  setup_sprite_blocks_switch(texturing, texture_mode),                         \
  texture_sprite_blocks_switch_##texturing(texture_mode),                      \
  shade_blocks_switch(unshaded, texturing, modulation, undithered, blending,   \
   mask_evaluate),                                                             \
  blend_blocks_switch(texturing, blending, blend_mode, mask_evaluate)          \
}                                                                              \

#define render_sprite_blocks_switch_block_blending(texture_mode, blend_mode,   \
 mask_evaluate, shading, dithering, texturing, blending)                       \
  render_sprite_blocks_switch_block_modulation(texture_mode, blend_mode,       \
   mask_evaluate, shading, dithering, texturing, blending, modulated),         \
  render_sprite_blocks_switch_block_modulation(texture_mode, blend_mode,       \
   mask_evaluate, shading, dithering, texturing, blending, unmodulated)        \

#define render_sprite_blocks_switch_block_texturing(texture_mode, blend_mode,  \
 mask_evaluate, shading, dithering, texturing)                                 \
  render_sprite_blocks_switch_block_blending(texture_mode, blend_mode,         \
   mask_evaluate, shading, dithering, texturing, unblended),                   \
  render_sprite_blocks_switch_block_blending(texture_mode, blend_mode,         \
   mask_evaluate, shading, dithering, texturing, blended)                      \

#define render_sprite_blocks_switch_block_dithering(texture_mode, blend_mode,  \
 mask_evaluate, shading, dithering)                                            \
  render_sprite_blocks_switch_block_texturing(texture_mode, blend_mode,        \
   mask_evaluate, shading, dithering, untextured),                             \
  render_sprite_blocks_switch_block_texturing(texture_mode, blend_mode,        \
   mask_evaluate, shading, dithering, textured)                                \

#define render_sprite_blocks_switch_block_shading(texture_mode, blend_mode,    \
 mask_evaluate, shading)                                                       \
  render_sprite_blocks_switch_block_dithering(texture_mode, blend_mode,        \
   mask_evaluate, shading, undithered),                                        \
  render_sprite_blocks_switch_block_dithering(texture_mode, blend_mode,        \
   mask_evaluate, shading, dithered)                                           \

#define render_sprite_blocks_switch_block_mask_evaluate(texture_mode,          \
 blend_mode, mask_evaluate)                                                    \
  render_sprite_blocks_switch_block_shading(texture_mode, blend_mode,          \
   mask_evaluate, unshaded),                                                   \
  render_sprite_blocks_switch_block_shading(texture_mode, blend_mode,          \
   mask_evaluate, shaded)                                                      \

#define render_sprite_blocks_switch_block_blend_mode(texture_mode, blend_mode) \
  render_sprite_blocks_switch_block_mask_evaluate(texture_mode, blend_mode,    \
   off),                                                                       \
  render_sprite_blocks_switch_block_mask_evaluate(texture_mode, blend_mode,    \
   on)                                                                         \

#define render_sprite_blocks_switch_block_texture_mode(texture_mode)           \
  render_sprite_blocks_switch_block_blend_mode(texture_mode, average),         \
  render_sprite_blocks_switch_block_blend_mode(texture_mode, add),             \
  render_sprite_blocks_switch_block_blend_mode(texture_mode, subtract),        \
  render_sprite_blocks_switch_block_blend_mode(texture_mode, add_fourth)       \

#define render_sprite_blocks_switch_block()                                    \
  render_sprite_blocks_switch_block_texture_mode(4bpp),                        \
  render_sprite_blocks_switch_block_texture_mode(8bpp),                        \
  render_sprite_blocks_switch_block_texture_mode(16bpp),                       \
  render_sprite_blocks_switch_block_texture_mode(16bpp)                        \


render_block_handler_struct render_sprite_block_handlers[] =
{
  render_sprite_blocks_switch_block()
};


void render_sprite(psx_gpu_struct *psx_gpu, s32 x, s32 y, u32 u, u32 v,
 s32 *width, s32 *height, u32 flags, u32 color)
{
  s32 x_right = x + *width - 1;
  s32 y_bottom = y + *height - 1;

#ifdef PROFILE
  sprites++;
#endif

  if(invalidate_texture_cache_region_viewport(psx_gpu, x, y, x_right,
   y_bottom) == 0)
  {
    *width = *height = 0;
    return;
  }

  if(x < psx_gpu->viewport_start_x)
  {
    u32 clip = psx_gpu->viewport_start_x - x;
    x += clip;
    u += clip;
    *width -= clip;
  }

  if(y < psx_gpu->viewport_start_y)
  {
    s32 clip = psx_gpu->viewport_start_y - y;
    y += clip;
    v += clip;
    *height -= clip;
  }

  if(x_right > psx_gpu->viewport_end_x)
    *width -= x_right - psx_gpu->viewport_end_x;

  if(y_bottom > psx_gpu->viewport_end_y)
    *height -= y_bottom - psx_gpu->viewport_end_y;

  if((*width <= 0) || (*height <= 0))
  {
    *width = *height = 0;
    return;
  }

#ifdef PROFILE
  span_pixels += *width * *height;
  spans += *height;
#endif

  u32 render_state = flags &
   (RENDER_FLAGS_MODULATE_TEXELS | RENDER_FLAGS_BLEND |
   RENDER_FLAGS_TEXTURE_MAP);
  render_state |=
   (psx_gpu->render_state_base & ~RENDER_STATE_DITHER);

  if((psx_gpu->render_state != render_state) ||
   (psx_gpu->primitive_type != PRIMITIVE_TYPE_SPRITE))
  {
    psx_gpu->render_state = render_state;
    flush_render_block_buffer(psx_gpu);
#ifdef PROFILE
    state_changes++;
#endif
  }

  psx_gpu->primitive_type = PRIMITIVE_TYPE_SPRITE;

  color &= 0xFFFFFF;

  if(psx_gpu->triangle_color != color)
  {
    flush_render_block_buffer(psx_gpu);
    psx_gpu->triangle_color = color;
  }

  if(color == 0x808080)
    render_state |= RENDER_FLAGS_MODULATE_TEXELS;

  render_block_handler_struct *render_block_handler =
   &(render_sprite_block_handlers[render_state]);
  psx_gpu->render_block_handler = render_block_handler;

  ((setup_sprite_function_type *)render_block_handler->setup_blocks)
   (psx_gpu, x, y, u, v, *width, *height, color);
}

#define draw_pixel_line_mask_evaluate_yes()                                    \
  if((*vram_ptr & 0x8000) == 0)                                                \

#define draw_pixel_line_mask_evaluate_no()                                     \
    

#define draw_pixel_line_shaded()                                               \
{                                                                              \
  color_r = fixed_to_int(current_r);                                           \
  color_g = fixed_to_int(current_g);                                           \
  color_b = fixed_to_int(current_b);                                           \
                                                                               \
  current_r += gradient_r;                                                     \
  current_g += gradient_g;                                                     \
  current_b += gradient_b;                                                     \
}                                                                              \

#define draw_pixel_line_unshaded()                                             \
{                                                                              \
  color_r = color & 0xFF;                                                      \
  color_g = (color >> 8) & 0xFF;                                               \
  color_b = (color >> 16) & 0xFF;                                              \
}                                                                              \


#define draw_pixel_line_dithered(_x, _y)                                       \
{                                                                              \
  u32 dither_xor = _x ^ _y;                                                    \
  s32 dither_offset = (dither_xor >> 1) & 0x1;                                 \
  dither_offset |= (_y & 0x1) << 1;                                            \
  dither_offset |= (dither_xor & 0x1) << 2;                                    \
  dither_offset -= 4;                                                          \
                                                                               \
  color_r += dither_offset;                                                    \
  color_g += dither_offset;                                                    \
  color_b += dither_offset;                                                    \
                                                                               \
  if(color_r < 0)                                                              \
    color_r = 0;                                                               \
                                                                               \
  if(color_g < 0)                                                              \
    color_g = 0;                                                               \
                                                                               \
  if(color_b < 0)                                                              \
    color_b = 0;                                                               \
                                                                               \
  if(color_r > 255)                                                            \
    color_r = 255;                                                             \
                                                                               \
  if(color_g > 255)                                                            \
    color_g = 255;                                                             \
                                                                               \
  if(color_b > 255)                                                            \
    color_b = 255;                                                             \
}                                                                              \

#define draw_pixel_line_undithered(_x, _y)                                     \


#define draw_pixel_line_average()                                              \
  color_r = (color_r + fb_r) / 2;                                              \
  color_g = (color_g + fb_g) / 2;                                              \
  color_b = (color_b + fb_b) / 2                                               \

#define draw_pixel_line_add()                                                  \
  color_r += fb_r;                                                             \
  color_g += fb_g;                                                             \
  color_b += fb_b;                                                             \
                                                                               \
  if(color_r > 31)                                                             \
    color_r = 31;                                                              \
                                                                               \
  if(color_g > 31)                                                             \
    color_g = 31;                                                              \
                                                                               \
  if(color_b > 31)                                                             \
    color_b = 31                                                               \
                                                                               \

#define draw_pixel_line_subtract()                                             \
  color_r = fb_r - color_r;                                                    \
  color_g = fb_g - color_g;                                                    \
  color_b = fb_b - color_b;                                                    \
                                                                               \
  if(color_r < 0)                                                              \
    color_r = 0;                                                               \
                                                                               \
  if(color_g < 0)                                                              \
    color_g = 0;                                                               \
                                                                               \
  if(color_b < 0)                                                              \
    color_b = 0                                                                \

#define draw_pixel_line_add_fourth()                                           \
  color_r = fb_r + (color_r / 4);                                              \
  color_g = fb_g + (color_g / 4);                                              \
  color_b = fb_b + (color_b / 4);                                              \
                                                                               \
  if(color_r > 31)                                                             \
    color_r = 31;                                                              \
                                                                               \
  if(color_g > 31)                                                             \
    color_g = 31;                                                              \
                                                                               \
  if(color_b > 31)                                                             \
    color_b = 31                                                               \


#define draw_pixel_line_blended(blend_mode)                                    \
  s32 fb_pixel = *vram_ptr;                                                    \
  s32 fb_r = fb_pixel & 0x1F;                                                  \
  s32 fb_g = (fb_pixel >> 5) & 0x1F;                                           \
  s32 fb_b = (fb_pixel >> 10) & 0x1F;                                          \
                                                                               \
  draw_pixel_line_##blend_mode()                                               \

#define draw_pixel_line_unblended(blend_mode)                                  \


#define draw_pixel_line(_x, _y, shading, blending, dithering, mask_evaluate,   \
 blend_mode)                                                                   \
  if((_x >= psx_gpu->viewport_start_x) && (_y >= psx_gpu->viewport_start_y) && \
   (_x <= psx_gpu->viewport_end_x) && (_y <= psx_gpu->viewport_end_y))         \
  {                                                                            \
    draw_pixel_line_mask_evaluate_##mask_evaluate()                            \
    {                                                                          \
      draw_pixel_line_##shading();                                             \
      draw_pixel_line_##dithering(_x, _y);                                     \
                                                                               \
      color_r >>= 3;                                                           \
      color_g >>= 3;                                                           \
      color_b >>= 3;                                                           \
                                                                               \
      draw_pixel_line_##blending(blend_mode);                                  \
                                                                               \
      *vram_ptr = color_r | (color_g << 5) | (color_b << 10) |                 \
       psx_gpu->mask_msb;                                                      \
    }                                                                          \
  }                                                                            \

#define update_increment(value)                                                \
  value++                                                                      \

#define update_decrement(value)                                                \
  value--                                                                      \

#define update_vram_row_increment(value)                                       \
  vram_ptr += 1024                                                             \

#define update_vram_row_decrement(value)                                       \
  vram_ptr -= 1024                                                             \

#define compare_increment(a, b)                                                \
  (a <= b)                                                                     \

#define compare_decrement(a, b)                                                \
  (a >= b)                                                                     \

#define set_line_gradients(minor)                                              \
{                                                                              \
  s32 gradient_divisor = delta_##minor;                                        \
  if(gradient_divisor != 0)                                                    \
  {                                                                            \
    gradient_r = int_to_fixed(vertex_b->r - vertex_a->r) / gradient_divisor;   \
    gradient_g = int_to_fixed(vertex_b->g - vertex_a->g) / gradient_divisor;   \
    gradient_b = int_to_fixed(vertex_b->b - vertex_a->b) / gradient_divisor;   \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    gradient_r = 0;                                                            \
    gradient_g = 0;                                                            \
    gradient_b = 0;                                                            \
  }                                                                            \
  current_r = fixed_center(vertex_a->r);                                       \
  current_g = fixed_center(vertex_a->g);                                       \
  current_b = fixed_center(vertex_a->b);                                       \
}

#define draw_line_span_horizontal(direction, shading, blending, dithering,     \
 mask_evaluate, blend_mode)                                                    \
do                                                                             \
{                                                                              \
  error_step = delta_y * 2;                                                    \
  error_wrap = delta_x * 2;                                                    \
  error = delta_x;                                                             \
                                                                               \
  current_y = y_a;                                                             \
  set_line_gradients(x);                                                       \
                                                                               \
  for(current_x = x_a; current_x <= x_b; current_x++)                          \
  {                                                                            \
    draw_pixel_line(current_x, current_y, shading, blending, dithering,        \
     mask_evaluate, blend_mode);                                               \
    error += error_step;                                                       \
    vram_ptr++;                                                                \
                                                                               \
    if(error >= error_wrap)                                                    \
    {                                                                          \
      update_##direction(current_y);                                           \
      update_vram_row_##direction();                                           \
      error -= error_wrap;                                                     \
    }                                                                          \
  }                                                                            \
} while(0)                                                                     \

#define draw_line_span_vertical(direction, shading, blending, dithering,       \
 mask_evaluate, blend_mode)                                                    \
do                                                                             \
{                                                                              \
  error_step = delta_x * 2;                                                    \
  error_wrap = delta_y * 2;                                                    \
  error = delta_y;                                                             \
                                                                               \
  current_x = x_a;                                                             \
  set_line_gradients(y);                                                       \
                                                                               \
  for(current_y = y_a; compare_##direction(current_y, y_b);                    \
   update_##direction(current_y))                                              \
  {                                                                            \
    draw_pixel_line(current_x, current_y, shading, blending, dithering,        \
     mask_evaluate, blend_mode);                                               \
    error += error_step;                                                       \
    update_vram_row_##direction();                                             \
                                                                               \
    if(error > error_wrap)                                                     \
    {                                                                          \
      vram_ptr++;                                                              \
      current_x++;                                                             \
      error -= error_wrap;                                                     \
    }                                                                          \
  }                                                                            \
} while(0)                                                                     \


#define render_line_body(shading, blending, dithering, mask_evaluate,          \
 blend_mode)                                                                   \
  if(delta_y < 0)                                                              \
  {                                                                            \
    delta_y *= -1;                                                             \
                                                                               \
    if(delta_x > delta_y)                                                      \
    {                                                                          \
      draw_line_span_horizontal(decrement, shading, blending, dithering,       \
       mask_evaluate, blend_mode);                                             \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      draw_line_span_vertical(decrement, shading, blending, dithering,         \
       mask_evaluate, blend_mode);                                             \
    }                                                                          \
  }                                                                            \
  else                                                                         \
  {                                                                            \
    if(delta_x > delta_y)                                                      \
    {                                                                          \
      draw_line_span_horizontal(increment, shading, blending, dithering,       \
       mask_evaluate, blend_mode);                                             \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      draw_line_span_vertical(increment, shading, blending, dithering,         \
       mask_evaluate, blend_mode);                                             \
    }                                                                          \
  }                                                                            \

                                                                                
void render_line(psx_gpu_struct *psx_gpu, vertex_struct *vertexes, u32 flags,
 u32 color, int double_resolution)
{
  s32 color_r, color_g, color_b;
  u32 triangle_winding = 0;

  fixed_type gradient_r = 0;
  fixed_type gradient_g = 0;
  fixed_type gradient_b = 0;
  fixed_type current_r = 0;
  fixed_type current_g = 0;
  fixed_type current_b = 0;

  s32 y_a, y_b;
  s32 x_a, x_b;

  s32 delta_x, delta_y;

  s32 current_x;
  s32 current_y;

  u32 error_step;
  u32 error;
  u32 error_wrap;

  u16 *vram_ptr;

  flush_render_block_buffer(psx_gpu);
  psx_gpu->primitive_type = PRIMITIVE_TYPE_LINE;

  vertex_struct *vertex_a = &(vertexes[0]);
  vertex_struct *vertex_b = &(vertexes[1]);

  u32 control_mask;

#ifdef PROFILE
  lines++;
#endif

  if(vertex_a->x >= vertex_b->x)
  {
    vertex_swap(vertex_a, vertex_b);
    (void)triangle_winding;
  }

  x_a = vertex_a->x;
  x_b = vertex_b->x;

  y_a = vertex_a->y;
  y_b = vertex_b->y;

  delta_x = x_b - x_a;
  delta_y = y_b - y_a;

  if(delta_x >= 1024 || delta_y >= 512 || delta_y <= -512)
    return;

  if(double_resolution)
  {
    x_a *= 2;
    x_b *= 2;
    y_a *= 2;
    y_b *= 2;
    delta_x *= 2;
    delta_y *= 2;
  }

  flags &= ~RENDER_FLAGS_TEXTURE_MAP;

  vram_ptr = psx_gpu->vram_out_ptr + (y_a * 1024) + x_a;

  control_mask = 0x0;

  if(flags & RENDER_FLAGS_SHADE)
    control_mask |= 0x1;

  if(flags & RENDER_FLAGS_BLEND)
  {
    control_mask |= 0x2;
    control_mask |= ((psx_gpu->render_state_base >> 6) & 0x3) << 4;
  }

  if(psx_gpu->render_state_base & RENDER_STATE_DITHER)
    control_mask |= 0x4;

  if(psx_gpu->render_state_base & RENDER_STATE_MASK_EVALUATE)
    control_mask |= 0x8;

  switch(control_mask)
  {
    case 0x0:
      render_line_body(unshaded, unblended, undithered, no, none);
      break;

    case 0x1:
      render_line_body(shaded, unblended, undithered, no, none);
      break;

    case 0x2:
      render_line_body(unshaded, blended, undithered, no, average);
      break;

    case 0x3:
      render_line_body(shaded, blended, undithered, no, average);
      break;

    case 0x4:
      render_line_body(unshaded, unblended, dithered, no, none);
      break;

    case 0x5:
      render_line_body(shaded, unblended, dithered, no, none);
      break;

    case 0x6:
      render_line_body(unshaded, blended, dithered, no, average);
      break;

    case 0x7:
      render_line_body(shaded, blended, dithered, no, average);
      break;

    case 0x8:
      render_line_body(unshaded, unblended, undithered, yes, none);
      break;

    case 0x9:
      render_line_body(shaded, unblended, undithered, yes, none);
      break;

    case 0xA:
      render_line_body(unshaded, blended, undithered, yes, average);
      break;

    case 0xB:
      render_line_body(shaded, blended, undithered, yes, average);
      break;

    case 0xC:
      render_line_body(unshaded, unblended, dithered, yes, none);
      break;

    case 0xD:
      render_line_body(shaded, unblended, dithered, yes, none);
      break;

    case 0xE:
      render_line_body(unshaded, blended, dithered, yes, average);
      break;

    case 0xF:
      render_line_body(shaded, blended, dithered, yes, average);
      break;

    case 0x12:
      render_line_body(unshaded, blended, undithered, no, add);
      break;

    case 0x13:
      render_line_body(shaded, blended, undithered, no, add);
      break;

    case 0x16:
      render_line_body(unshaded, blended, dithered, no, add);
      break;

    case 0x17:
      render_line_body(shaded, blended, dithered, no, add);
      break;

    case 0x1A:
      render_line_body(unshaded, blended, undithered, yes, add);
      break;

    case 0x1B:
      render_line_body(shaded, blended, undithered, yes, add);
      break;

    case 0x1E:
      render_line_body(unshaded, blended, dithered, yes, add);
      break;

    case 0x1F:
      render_line_body(shaded, blended, dithered, yes, add);
      break;

    case 0x22:
      render_line_body(unshaded, blended, undithered, no, subtract);
      break;

    case 0x23:
      render_line_body(shaded, blended, undithered, no, subtract);
      break;

    case 0x26:
      render_line_body(unshaded, blended, dithered, no, subtract);
      break;

    case 0x27:
      render_line_body(shaded, blended, dithered, no, subtract);
      break;

    case 0x2A:
      render_line_body(unshaded, blended, undithered, yes, subtract);
      break;

    case 0x2B:
      render_line_body(shaded, blended, undithered, yes, subtract);
      break;

    case 0x2E:
      render_line_body(unshaded, blended, dithered, yes, subtract);
      break;

    case 0x2F:
      render_line_body(shaded, blended, dithered, yes, subtract);
      break;

    case 0x32:
      render_line_body(unshaded, blended, undithered, no, add_fourth);
      break;

    case 0x33:
      render_line_body(shaded, blended, undithered, no, add_fourth);
      break;

    case 0x36:
      render_line_body(unshaded, blended, dithered, no, add_fourth);
      break;

    case 0x37:
      render_line_body(shaded, blended, dithered, no, add_fourth);
      break;

    case 0x3A:
      render_line_body(unshaded, blended, undithered, yes, add_fourth);
      break;

    case 0x3B:
      render_line_body(shaded, blended, undithered, yes, add_fourth);
      break;

    case 0x3E:
      render_line_body(unshaded, blended, dithered, yes, add_fourth);
      break;

    case 0x3F:
      render_line_body(shaded, blended, dithered, yes, add_fourth);
      break;
  }
}


void render_block_fill(psx_gpu_struct *psx_gpu, u32 color, u32 x, u32 y,
 u32 width, u32 height)
{
  if((width == 0) || (height == 0))
    return;

  invalidate_texture_cache_region(psx_gpu, x, y, x + width - 1, y + height - 1);

  u32 r = color & 0xFF;
  u32 g = (color >> 8) & 0xFF;
  u32 b = (color >> 16) & 0xFF;
  u32 color_16bpp = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
  u32 color_32bpp = color_16bpp | (color_16bpp << 16);

  u32 *vram_ptr = (u32 *)(psx_gpu->vram_out_ptr + x + (y * 1024));

  u32 pitch = 512 - (width / 2);
  u32 num_width;

  if(psx_gpu->render_mode & RENDER_INTERLACE_ENABLED)
  {
    pitch += 512;
    height /= 2;

    if(psx_gpu->render_mode & RENDER_INTERLACE_ODD)
      vram_ptr += 512; 
  }

  while(height)
  {
    num_width = width;
    while(num_width)
    {
      vram_ptr[0] = color_32bpp;
      vram_ptr[1] = color_32bpp;
      vram_ptr[2] = color_32bpp;
      vram_ptr[3] = color_32bpp;
      vram_ptr[4] = color_32bpp;
      vram_ptr[5] = color_32bpp;
      vram_ptr[6] = color_32bpp;
      vram_ptr[7] = color_32bpp;

      vram_ptr += 8;
      num_width -= 16;
    }

    vram_ptr += pitch;
    height--;
  }
}

void render_block_fill_enh(psx_gpu_struct *psx_gpu, u32 color, u32 x, u32 y,
 u32 width, u32 height)
{
  if((width == 0) || (height == 0))
    return;

  if(width > 1024)
    width = 1024;

  u32 r = color & 0xFF;
  u32 g = (color >> 8) & 0xFF;
  u32 b = (color >> 16) & 0xFF;
  u32 color_16bpp = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
  u32 color_32bpp = color_16bpp | (color_16bpp << 16);

  u32 *vram_ptr = (u32 *)(psx_gpu->vram_out_ptr + x + (y * 1024));

  u32 pitch = 1024 / 2 - (width / 2);
  u32 num_width;

  while(height)
  {
    num_width = width;
    while(num_width)
    {
      vram_ptr[0] = color_32bpp;
      vram_ptr[1] = color_32bpp;
      vram_ptr[2] = color_32bpp;
      vram_ptr[3] = color_32bpp;
      vram_ptr[4] = color_32bpp;
      vram_ptr[5] = color_32bpp;
      vram_ptr[6] = color_32bpp;
      vram_ptr[7] = color_32bpp;

      vram_ptr += 8;
      num_width -= 16;
    }

    vram_ptr += pitch;
    height--;
  }
}

#ifndef PCSX
void render_block_copy(psx_gpu_struct *psx_gpu, u16 *source, u32 x, u32 y,
 u32 width, u32 height, u32 pitch)
{
  u16 *vram_ptr = psx_gpu->vram_ptr + x + (y * 1024);
  u32 draw_x, draw_y;
  u32 mask_msb = psx_gpu->mask_msb;

  if((width == 0) || (height == 0))
    return;

  flush_render_block_buffer(psx_gpu);
  invalidate_texture_cache_region(psx_gpu, x, y, x + width - 1, y + height - 1);

  for(draw_y = 0; draw_y < height; draw_y++)
  {
    for(draw_x = 0; draw_x < width; draw_x++)
    {
      vram_ptr[draw_x] = source[draw_x] | mask_msb;
    }

    source += pitch;
    vram_ptr += 1024;
  }
}

void render_block_move(psx_gpu_struct *psx_gpu, u32 source_x, u32 source_y,
 u32 dest_x, u32 dest_y, u32 width, u32 height)
{
  render_block_copy(psx_gpu, psx_gpu->vram_ptr + source_x + (source_y * 1024),
   dest_x, dest_y, width, height, 1024);
}
#endif

void initialize_reciprocal_table(void)
{
  u32 height;
  u32 height_normalized;
  u32 height_reciprocal;
  s32 shift;

  for(height = 1; height < sizeof(reciprocal_table)
       / sizeof(reciprocal_table[0]); height++)
  {
    shift = __builtin_clz(height);
    height_normalized = height << shift;
    height_reciprocal = ((1ULL << 51) + (height_normalized - 1)) /
     height_normalized;

    shift = 32 - (51 - shift);

    reciprocal_table[height] = (height_reciprocal << 10) | shift;
  }
}


#define dither_table_row(a, b, c, d)                                           \
 ((a & 0xFF) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16) | ((u32)(d & 0xFF) << 24)) \

void initialize_psx_gpu(psx_gpu_struct *psx_gpu, u16 *vram)
{
  vec_8x16u test_mask =
   { { { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 } } };

  psx_gpu->test_mask = test_mask;

  psx_gpu->dirty_textures_4bpp_mask = 0xFFFFFFFF;
  psx_gpu->dirty_textures_8bpp_mask = 0xFFFFFFFF;
  psx_gpu->dirty_textures_8bpp_alternate_mask = 0xFFFFFFFF;
  psx_gpu->viewport_mask = 0;
  psx_gpu->current_texture_page = 0;
  psx_gpu->current_texture_mask = 0;
  psx_gpu->last_8bpp_texture_page = 0;

  psx_gpu->clut_settings = 0;
  psx_gpu->texture_settings = 0;
  psx_gpu->render_state = 0;
  psx_gpu->render_state_base = 0;
  psx_gpu->num_blocks = 0;
  psx_gpu->uvrgb_phase = 0x8000;

  psx_gpu->vram_ptr = vram;
  psx_gpu->vram_out_ptr = vram;

  psx_gpu->texture_page_base = psx_gpu->vram_ptr;
  psx_gpu->texture_page_ptr = psx_gpu->vram_ptr;
  psx_gpu->clut_ptr = psx_gpu->vram_ptr;

  psx_gpu->viewport_start_x = psx_gpu->viewport_start_y = 0;
  psx_gpu->viewport_end_x = psx_gpu->viewport_end_y = 0;
  psx_gpu->mask_msb = 0;

  psx_gpu->texture_window_x = 0;
  psx_gpu->texture_window_y = 0;
  psx_gpu->texture_mask_width = 0xFF;
  psx_gpu->texture_mask_height = 0xFF;

  psx_gpu->render_mode = 0;

  memset(psx_gpu->vram_ptr, 0, sizeof(u16) * 1024 * 512);

  initialize_reciprocal_table();
  psx_gpu->reciprocal_table_ptr = reciprocal_table;

  //    00 01 10 11
  // 00  0  4  1  5
  // 01  6  2  7  3
  // 10  1  5  0  4
  // 11  7  3  6  2
  // (minus ones(4) * 4)

  // d0: (1 3 5 7): x1 ^ y1
  // d1: (2 3 6 7): y0
  // d2: (4 5 6 7): x0 ^ y0

  psx_gpu->dither_table[0] = dither_table_row(-4, 0, -3, 1);
  psx_gpu->dither_table[1] = dither_table_row(2, -2, 3, -1);
  psx_gpu->dither_table[2] = dither_table_row(-3, 1, -4, 0);
  psx_gpu->dither_table[3] = dither_table_row(3, -1, 2, -2);
  psx_gpu->allow_dithering = 1;

  psx_gpu->primitive_type = PRIMITIVE_TYPE_UNKNOWN;

  psx_gpu->saved_hres = 256;
  psx_gpu->hacks_active = 0;

  // check some offsets, asm relies on these
  psx_gpu->reserved_a[(offsetof(psx_gpu_struct, test_mask) == 0) - 1] = 0;
  psx_gpu->reserved_a[(offsetof(psx_gpu_struct, blocks) == psx_gpu_blocks_offset) - 1] = 0;
}

u64 get_us(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  
  return (tv.tv_sec * 1000000ULL) + tv.tv_usec;
}

#if 0 //def NEON_BUILD

u32 get_counter()
{
  u32 counter;
  __asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(counter));

  return counter;
}

void init_counter(void)
{
  u32 value;
  asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(value));
  value |= 5; // master enable, ccnt reset
  value &= ~8; // ccnt divider 0
  asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(value));
  // enable cycle counter
  asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(1 << 31));
}

void triangle_benchmark(psx_gpu_struct *psx_gpu)
{
  u32 i;

  u32 ticks;
  u32 ticks_elapsed;

  const u32 iterations = 500000;

  psx_gpu->num_blocks = 64;
  psx_gpu->clut_ptr = psx_gpu->vram_ptr;

  for(i = 0; i < 64; i++)
  {
    memset(&(psx_gpu->blocks[i].r), 0, 16);
  }

  init_counter();

  ticks = get_counter();

  for(i = 0; i < iterations; i++)
  {
    texture_sprite_blocks_8bpp(psx_gpu);
  }

  ticks_elapsed = get_counter() - ticks;

  printf("benchmark: %lf cycles\n", (double)ticks_elapsed / (iterations * 64));
}

#endif

#include "psx_gpu_4x.c"

// vim:ts=2:sw=2:expandtab
