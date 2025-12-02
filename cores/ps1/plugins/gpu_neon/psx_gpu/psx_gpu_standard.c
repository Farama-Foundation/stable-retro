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
#include <malloc.h>
#include <math.h>

#include "common.h"

typedef s32 fixed_type;

#define EDGE_STEP_BITS 32
#define FIXED_BITS     12

#define fixed_center(value)                                                    \
  ((((fixed_type)value) << FIXED_BITS) + (1 << (FIXED_BITS - 1)))              \

#define int_to_fixed(value)                                                    \
  (((fixed_type)value) << FIXED_BITS)                                          \

#define fixed_to_int(value)                                                    \
  ((value) >> FIXED_BITS)                                                      \

#define fixed_mul(_a, _b)                                                      \
  (((s64)(_a) * (_b)) >> FIXED_BITS)                                           \

#define fixed_to_double(value)                                                 \
  ((value) / (double)(1 << FIXED_BITS))                                        \

#define double_to_fixed(value)                                                 \
  (fixed_type)(((value) * (double)(1 << FIXED_BITS)))                          \

typedef struct
{
  fixed_type current_value;
  fixed_type step_dx;
  fixed_type step_dy;
  fixed_type gradient_area_x;
  fixed_type gradient_area_y;
} interpolant_struct;

typedef struct
{
  s32 base_x;

  s64 left_x;
  s64 left_dx_dy;

  s64 right_x;
  s64 right_dx_dy;

  u32 triangle_area;
  u32 triangle_winding;

  interpolant_struct u;
  interpolant_struct v;
  interpolant_struct r;
  interpolant_struct g;
  interpolant_struct b;
} _span_struct;


u32 span_pixels = 0;
u32 span_pixel_blocks = 0;
u32 spans = 0;
u32 triangles = 0;

u32 texels_4bpp = 0;
u32 texels_8bpp = 0;
u32 texels_16bpp = 0;
u32 untextured_pixels = 0;
u32 blend_pixels = 0;
u32 transparent_pixels = 0;

u32 state_changes = 0;
u32 render_buffer_flushes = 0;
u32 trivial_rejects = 0;

void flush_render_block_buffer(psx_gpu_struct *psx_gpu)
{

}


u32 fixed_reciprocal(u32 denominator, u32 *_shift)
{
  u32 shift = __builtin_clz(denominator);
  u32 denominator_normalized = denominator << shift;

  // Implement with a DP divide
  u32 reciprocal =
   (double)((1ULL << 62) + (denominator_normalized - 1)) / 
   (double)denominator_normalized;

  *_shift = 62 - shift;
  return reciprocal;
}

fixed_type fixed_reciprocal_multiply(s32 numerator, u32 reciprocal,
 u32 reciprocal_sign, u32 shift)
{
  u32 numerator_sign = (u32)numerator >> 31;
  u32 flip_sign = numerator_sign ^ reciprocal_sign;
  u32 flip_sign_mask = ~(flip_sign - 1);
  fixed_type value;

  numerator = abs(numerator);

  value = ((u64)numerator * reciprocal) >> shift;

  value ^= flip_sign_mask;
  value -= flip_sign_mask;

  return value;
}

s32 triangle_signed_area_x2(s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2)
{
	return ((x1 - x0) * (y2 - y1)) - ((x2 - x1) * (y1 - y0));
}

u32 fetch_texel_4bpp(psx_gpu_struct *psx_gpu, u32 u, u32 v)
{
  u8 *texture_ptr_8bpp = psx_gpu->texture_page_ptr;
  u32 texel = texture_ptr_8bpp[(v * 2048) + (u / 2)];

  if(u & 1)
    texel >>= 4;
  else
    texel &= 0xF;

  texels_4bpp++;

  return psx_gpu->clut_ptr[texel];
}

u32 fetch_texel_8bpp(psx_gpu_struct *psx_gpu, u32 u, u32 v)
{
  u8 *texture_ptr_8bpp = psx_gpu->texture_page_ptr;
  u32 texel = texture_ptr_8bpp[(v * 2048) + u];

  texels_8bpp++;

  return psx_gpu->clut_ptr[texel];
}

u32 fetch_texel_16bpp(psx_gpu_struct *psx_gpu, u32 u, u32 v)
{
  u16 *texture_ptr_16bpp = psx_gpu->texture_page_ptr;

  texels_16bpp++;

  return texture_ptr_16bpp[(v * 1024) + u];
}

u32 fetch_texel(psx_gpu_struct *psx_gpu, u32 u, u32 v)
{
  u &= psx_gpu->texture_mask_width;
  v &= psx_gpu->texture_mask_height;

  switch(psx_gpu->texture_mode)
  {
    case TEXTURE_MODE_4BPP:
      return fetch_texel_4bpp(psx_gpu, u, v);

    case TEXTURE_MODE_8BPP:
      return fetch_texel_8bpp(psx_gpu, u, v);

    case TEXTURE_MODE_16BPP:
      return fetch_texel_16bpp(psx_gpu, u, v);
  }

  return 0;
}

void draw_pixel(psx_gpu_struct *psx_gpu, s32 r, s32 g, s32 b, u32 texel,
 u32 x, u32 y, u32 flags)
{
  u32 pixel;

  if(r > 31)
    r = 31;

  if(g > 31)
    g = 31;

  if(b > 31)
    b = 31;

  if(flags & RENDER_FLAGS_BLEND)
  {
    if(((flags & RENDER_FLAGS_TEXTURE_MAP) == 0) || (texel & 0x8000))
    {
      s32 fb_pixel = psx_gpu->vram[(y * 1024) + x];
      s32 fb_r = fb_pixel & 0x1F;
      s32 fb_g = (fb_pixel >> 5) & 0x1F;
      s32 fb_b = (fb_pixel >> 10) & 0x1F;

      blend_pixels++;

      switch(psx_gpu->blend_mode)
      {
        case BLEND_MODE_AVERAGE:
          r = (r + fb_r) / 2;
          g = (g + fb_g) / 2;
          b = (b + fb_b) / 2;
          break;

        case BLEND_MODE_ADD:
          r += fb_r;
          g += fb_g;
          b += fb_b;

          if(r > 31)
            r = 31;

          if(g > 31)
            g = 31;

          if(b > 31)
            b = 31;

          break;

        case BLEND_MODE_SUBTRACT:
          r = fb_r - r;
          g = fb_g - g;
          b = fb_b - b;

          if(r < 0)
            r = 0;

          if(g < 0)
            g = 0;

          if(b < 0)
            b = 0;

          break;

        case BLEND_MODE_ADD_FOURTH:
          r = fb_r + (r / 4);
          g = fb_g + (g / 4);
          b = fb_b + (b / 4);

          if(r > 31)
            r = 31;

          if(g > 31)
            g = 31;

          if(b > 31)
            b = 31;

          break;      
      }
    }
  }

  pixel = r | (g << 5) | (b << 10);

  if(psx_gpu->mask_apply || (texel & 0x8000))
    pixel |= 0x8000;

  psx_gpu->vram[(y * 1024) + x] = pixel;
}

s32 dither_table[4][4] =
{
  { -4,  0, -3,  1 },
  {  2, -2,  3, -1 },
  { -3,  1, -4,  0 },
  {  3, -1,  2, -2 },
};

void render_span(psx_gpu_struct *psx_gpu, _span_struct *span, s32 y,
 u32 flags)
{
  s32 left_x = span->left_x >> EDGE_STEP_BITS;
  s32 right_x = span->right_x >> EDGE_STEP_BITS;
  s32 current_x = left_x;
  s32 delta_x;

  fixed_type current_u = span->u.current_value;
  fixed_type current_v = span->v.current_value;
  fixed_type current_r = span->r.current_value;
  fixed_type current_g = span->g.current_value;
  fixed_type current_b = span->b.current_value;

  if(y < psx_gpu->viewport_start_y)
    return;

  if(y > psx_gpu->viewport_end_y)
    return;

  if(right_x < psx_gpu->viewport_start_x)
    return;

  if(current_x > psx_gpu->viewport_end_x)
    return;

  spans++;

  if(current_x < psx_gpu->viewport_start_x)
    current_x = psx_gpu->viewport_start_x;  

  if(right_x > psx_gpu->viewport_end_x + 1)
    right_x = psx_gpu->viewport_end_x + 1;

  delta_x = current_x - span->base_x;

  current_u += delta_x * span->u.step_dx;
  current_v += delta_x * span->v.step_dx;
  current_r += delta_x * span->r.step_dx;
  current_g += delta_x * span->g.step_dx;
  current_b += delta_x * span->b.step_dx;

  span_pixels += right_x - current_x;
  span_pixel_blocks += ((right_x / 8) - (current_x / 8)) + 1;

  while(current_x < right_x)
  {
    s32 color_r, color_g, color_b;
    u32 texel = 0;

    if(psx_gpu->mask_evaluate &&
     (psx_gpu->vram[(y * 1024) + current_x] & 0x8000))
    {
      goto skip_pixel;
    }

    if(flags & RENDER_FLAGS_SHADE)
    {
      color_r = fixed_to_int(current_r);
      color_g = fixed_to_int(current_g);
      color_b = fixed_to_int(current_b);
    }
    else
    {
      color_r = psx_gpu->primitive_color & 0xFF;
      color_g = (psx_gpu->primitive_color >> 8) & 0xFF;
      color_b = (psx_gpu->primitive_color >> 16) & 0xFF;
    }      

    if(flags & RENDER_FLAGS_TEXTURE_MAP)
    {
      u32 texel_r, texel_g, texel_b;
      u32 u = fixed_to_int(current_u);
      u32 v = fixed_to_int(current_v);

      texel = fetch_texel(psx_gpu, u, v);

      if(texel == 0)
      {
        transparent_pixels++;
        goto skip_pixel;
      }

      texel_r = texel & 0x1F;
      texel_g = (texel >> 5) & 0x1F;
      texel_b = (texel >> 10) & 0x1F;

      if((flags & RENDER_FLAGS_MODULATE_TEXELS) == 0)
      {
        color_r *= texel_r;
        color_g *= texel_g;
        color_b *= texel_b;
      }
      else
      {
        color_r = texel_r << 7;
        color_g = texel_g << 7;
        color_b = texel_b << 7;
      }

      color_r >>= 4;
      color_g >>= 4;
      color_b >>= 4;
    }
    else
    {
      untextured_pixels++;
    }

    if(psx_gpu->dither_mode && ((flags & RENDER_FLAGS_SHADE) ||
     ((flags & RENDER_FLAGS_TEXTURE_MAP) &&
     ((flags & RENDER_FLAGS_MODULATE_TEXELS) == 0))))
    {
      s32 dither_offset = dither_table[y % 4][current_x % 4];
      color_r += dither_offset;
      color_g += dither_offset;
      color_b += dither_offset;

      if(color_r < 0)
        color_r = 0;
  
      if(color_g < 0)
        color_g = 0;
  
      if(color_b < 0)
        color_b = 0;
    }

    color_r >>= 3;
    color_g >>= 3;
    color_b >>= 3;

    draw_pixel(psx_gpu, color_r, color_g, color_b, texel, current_x, y, flags);

  skip_pixel:
  
    current_u += span->u.step_dx;
    current_v += span->v.step_dx;
    current_r += span->r.step_dx;
    current_g += span->g.step_dx;
    current_b += span->b.step_dx;

    current_x++;
  }
}

void increment_span(_span_struct *span)
{
  span->left_x += span->left_dx_dy;
  span->right_x += span->right_dx_dy;

  span->u.current_value += span->u.step_dy;
  span->v.current_value += span->v.step_dy;
  span->r.current_value += span->r.step_dy;
  span->g.current_value += span->g.step_dy;
  span->b.current_value += span->b.step_dy;
}

void decrement_span(_span_struct *span)
{
  span->left_x += span->left_dx_dy;
  span->right_x += span->right_dx_dy;

  span->u.current_value -= span->u.step_dy;
  span->v.current_value -= span->v.step_dy;
  span->r.current_value -= span->r.step_dy;
  span->g.current_value -= span->g.step_dy;
  span->b.current_value -= span->b.step_dy;
}


#define compute_gradient_area_x(interpolant)                                   \
{                                                                              \
  span.interpolant.gradient_area_x =                                           \
   triangle_signed_area_x2(a->interpolant, a->y, b->interpolant, b->y,         \
   c->interpolant, c->y);                                                      \
}                                                                              \

#define compute_gradient_area_y(interpolant)                                   \
{                                                                              \
  span.interpolant.gradient_area_y =                                           \
   triangle_signed_area_x2(a->x, a->interpolant,  b->x, b->interpolant,        \
   c->x, c->interpolant);                                                      \
}                                                                              \

#define compute_all_gradient_areas()                                           \
  compute_gradient_area_x(u);                                                  \
  compute_gradient_area_x(v);                                                  \
  compute_gradient_area_x(r);                                                  \
  compute_gradient_area_x(g);                                                  \
  compute_gradient_area_x(b);                                                  \
  compute_gradient_area_y(u);                                                  \
  compute_gradient_area_y(v);                                                  \
  compute_gradient_area_y(r);                                                  \
  compute_gradient_area_y(g);                                                  \
  compute_gradient_area_y(b)                                                   \

#define set_interpolant_base(interpolant, base_vertex)                         \
  span->interpolant.step_dx =                                                  \
   fixed_reciprocal_multiply(span->interpolant.gradient_area_x, reciprocal,    \
   span->triangle_winding, shift);                                             \
  span->interpolant.step_dy =                                                  \
   fixed_reciprocal_multiply(span->interpolant.gradient_area_y, reciprocal,    \
   span->triangle_winding, shift);                                             \
  span->interpolant.current_value = fixed_center(base_vertex->interpolant)     \

#define set_interpolant_bases(base_vertex)                                     \
{                                                                              \
  u32 shift;                                                                   \
  u32 reciprocal = fixed_reciprocal(span->triangle_area, &shift);              \
  shift -= FIXED_BITS;                                                         \
  set_interpolant_base(u, base_vertex);                                        \
  set_interpolant_base(v, base_vertex);                                        \
  set_interpolant_base(r, base_vertex);                                        \
  set_interpolant_base(g, base_vertex);                                        \
  set_interpolant_base(b, base_vertex);                                        \
  span->base_x = span->left_x >> EDGE_STEP_BITS;                               \
}                                                                              \

#define compute_edge_delta(edge, start, end, height)                           \
{                                                                              \
  s32 x_start = start->x;                                                      \
  s32 x_end = end->x;                                                          \
  s32 width = x_end - x_start;                                                 \
                                                                               \
  s32 shift = __builtin_clz(height);                                           \
  u32 height_normalized = height << shift;                                     \
  u32 height_reciprocal = ((1ULL << 50) + (height_normalized - 1)) /           \
   height_normalized;                                                          \
                                                                               \
  shift -= (50 - EDGE_STEP_BITS);                                              \
                                                                               \
  span->edge##_x =                                                             \
   ((((s64)x_start * height) + (height - 1)) * height_reciprocal) << shift;    \
  span->edge##_dx_dy = ((s64)width * height_reciprocal) << shift;              \
}                                                                              \


#define render_spans_up(height)                                                \
  do                                                                           \
  {                                                                            \
    decrement_span(span);                                                      \
    render_span(psx_gpu, span, current_y, flags);                              \
    current_y--;                                                               \
    height--;                                                                  \
  } while(height)                                                              \

#define render_spans_down(height)                                              \
  do                                                                           \
  {                                                                            \
    render_span(psx_gpu, span, current_y, flags);                              \
    increment_span(span);                                                      \
    current_y++;                                                               \
    height--;                                                                  \
  } while(height)                                                              \

#define render_spans_up_up(minor, major)                                       \
  s32 current_y = bottom->y - 1;                                               \
  s32 height_minor_a = bottom->y - middle->y;                                  \
  s32 height_minor_b = middle->y - top->y;                                     \
  s32 height_major = height_minor_a + height_minor_b;                          \
                                                                               \
  compute_edge_delta(major, bottom, top, height_major);                        \
  compute_edge_delta(minor, bottom, middle, height_minor_a);                   \
  set_interpolant_bases(bottom);                                               \
                                                                               \
  render_spans_up(height_minor_a);                                             \
                                                                               \
  compute_edge_delta(minor, middle, top, height_minor_b);                      \
  render_spans_up(height_minor_b)                                              \

void render_spans_up_left(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *bottom, vertex_struct *middle, vertex_struct *top, u32 flags)
{
  render_spans_up_up(left, right);
}

void render_spans_up_right(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *bottom, vertex_struct *middle, vertex_struct *top, u32 flags)
{
  render_spans_up_up(right, left);
}

#define render_spans_down_down(minor, major)                                   \
  s32 current_y = top->y;                                                      \
  s32 height_minor_a = middle->y - top->y;                                     \
  s32 height_minor_b = bottom->y - middle->y;                                  \
  s32 height_major = height_minor_a + height_minor_b;                          \
                                                                               \
  compute_edge_delta(minor, top, middle, height_minor_a);                      \
  compute_edge_delta(major, top, bottom, height_major);                        \
  set_interpolant_bases(top);                                                  \
                                                                               \
  render_spans_down(height_minor_a);                                           \
                                                                               \
  compute_edge_delta(minor, middle, bottom, height_minor_b);                   \
  render_spans_down(height_minor_b)                                            \

void render_spans_down_left(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *top, vertex_struct *middle, vertex_struct *bottom, u32 flags)
{
  render_spans_down_down(left, right);
}

void render_spans_down_right(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *top, vertex_struct *middle, vertex_struct *bottom, u32 flags)
{
  render_spans_down_down(right, left);
}

#define render_spans_up_flat(bottom_left, bottom_right, top_left, top_right)   \
  s32 current_y = bottom_left->y - 1;                                          \
  s32 height = bottom_left->y - top_left->y;                                   \
                                                                               \
  compute_edge_delta(left, bottom_left, top_left, height);                     \
  compute_edge_delta(right, bottom_right, top_right, height);                  \
  set_interpolant_bases(bottom_left);                                          \
  render_spans_up(height)                                                      \

void render_spans_up_a(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *bottom_left, vertex_struct *bottom_right, vertex_struct *top,
 u32 flags)
{
  render_spans_up_flat(bottom_left, bottom_right, top, top);
}

void render_spans_up_b(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *bottom, vertex_struct *top_left, vertex_struct *top_right,
 u32 flags)
{
  render_spans_up_flat(bottom, bottom, top_left, top_right);
}

#define render_spans_down_flat(top_left, top_right, bottom_left, bottom_right) \
  s32 current_y = top_left->y;                                                 \
  s32 height = bottom_left->y - top_left->y;                                   \
                                                                               \
  compute_edge_delta(left, top_left, bottom_left, height);                     \
  compute_edge_delta(right, top_right, bottom_right, height);                  \
  set_interpolant_bases(top_left);                                             \
  render_spans_down(height)                                                    \

void render_spans_down_a(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *top_left, vertex_struct *top_right, vertex_struct *bottom,
 u32 flags)
{
  render_spans_down_flat(top_left, top_right, bottom, bottom);
}

void render_spans_down_b(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *top, vertex_struct *bottom_left, vertex_struct *bottom_right,
 u32 flags)
{
  render_spans_down_flat(top, top, bottom_left, bottom_right);
}

void render_spans_up_down(psx_gpu_struct *psx_gpu, _span_struct *span,
 vertex_struct *middle, vertex_struct *top, vertex_struct *bottom, u32 flags)
{
  s32 middle_y = middle->y;
  s32 current_y = middle_y - 1;
  s32 height_minor_a = middle->y - top->y;
  s32 height_minor_b = bottom->y - middle->y;
  s32 height_major = height_minor_a + height_minor_b;

  u64 right_x_mid;

  compute_edge_delta(left, middle, top, height_minor_a);
  compute_edge_delta(right, bottom, top, height_major);
  set_interpolant_bases(middle);

  right_x_mid = span->right_x + (span->right_dx_dy * height_minor_b);
  span->right_x = right_x_mid;

  render_spans_up(height_minor_a);  

  compute_edge_delta(left, middle, bottom, height_minor_b);
  set_interpolant_bases(middle);

  span->right_dx_dy *= -1;
  span->right_x = right_x_mid;
  current_y = middle_y;

  render_spans_down(height_minor_b);
}

#define vertex_swap(_a, _b)                                                    \
{                                                                              \
  vertex_struct *temp_vertex = _a;                                             \
  _a = _b;                                                                     \
  _b = temp_vertex;                                                            \
  triangle_winding ^= 1;                                                       \
}                                                                              \


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


void render_triangle(psx_gpu_struct *psx_gpu, vertex_struct *vertexes,
 u32 flags)
{
  s32 triangle_area;
  u32 triangle_winding = 0;
  _span_struct span;

  vertex_struct *a = &(vertexes[0]);
  vertex_struct *b = &(vertexes[1]);
  vertex_struct *c = &(vertexes[2]);

  triangle_area = triangle_signed_area_x2(a->x, a->y, b->x, b->y, c->x, c->y);

  triangles++;

  if(triangle_area == 0)
    return;

  if(b->y < a->y)
    vertex_swap(a, b);

  if(c->y < b->y)
  {
    vertex_swap(b, c);

    if(b->y < a->y)
      vertex_swap(a, b);
  }

  if((c->y - a->y) >= 512)
    return;

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

  if((c->x - a->x) >= 1024)
    return;

  s32 y_delta_a = b->y - a->y;
  s32 y_delta_b = c->y - b->y;
  s32 y_delta_c = c->y - a->y;

  triangle_set_direction(y_direction_a, y_delta_a);
  triangle_set_direction(y_direction_b, y_delta_b);
  triangle_set_direction(y_direction_c, y_delta_c);

  compute_all_gradient_areas();
  span.triangle_area = triangle_area;
  span.triangle_winding = triangle_winding;

  switch(y_direction_a | (y_direction_b << 2) | (y_direction_c << 4) |
   (triangle_winding << 6))
  {
    triangle_case(up, up, up, negative):
    triangle_case(up, up, flat, negative):
    triangle_case(up, up, down, negative):
      render_spans_up_right(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(flat, up, up, negative):
    triangle_case(flat, up, flat, negative):
    triangle_case(flat, up, down, negative):
      render_spans_up_a(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(down, up, up, negative):
      render_spans_up_down(psx_gpu, &span, a, c, b, flags);
      break;

    triangle_case(down, up, flat, negative):
      render_spans_down_a(psx_gpu, &span, a, c, b, flags);
      break;

    triangle_case(down, up, down, negative):
      render_spans_down_right(psx_gpu, &span, a, c, b, flags);
      break;

    triangle_case(down, flat, up, negative):
    triangle_case(down, flat, flat, negative):
    triangle_case(down, flat, down, negative):
      render_spans_down_b(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(down, down, up, negative):
    triangle_case(down, down, flat, negative):
    triangle_case(down, down, down, negative):
      render_spans_down_left(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(up, up, up, positive):
    triangle_case(up, up, flat, positive):
    triangle_case(up, up, down, positive):
      render_spans_up_left(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(up, flat, up, positive):
    triangle_case(up, flat, flat, positive):
    triangle_case(up, flat, down, positive):
      render_spans_up_b(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(up, down, up, positive):
      render_spans_up_right(psx_gpu, &span, a, c, b, flags);
      break;

    triangle_case(up, down, flat, positive):
      render_spans_up_a(psx_gpu, &span, a, c, b, flags);
      break;

    triangle_case(up, down, down, positive):
      render_spans_up_down(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(flat, down, up, positive):
    triangle_case(flat, down, flat, positive):
    triangle_case(flat, down, down, positive):
      render_spans_down_a(psx_gpu, &span, a, b, c, flags);
      break;

    triangle_case(down, down, up, positive):
    triangle_case(down, down, flat, positive):
    triangle_case(down, down, down, positive):
      render_spans_down_right(psx_gpu, &span, a, b, c, flags);
      break;
  }
  
}


void render_sprite(psx_gpu_struct *psx_gpu, s32 x, s32 y, u32 u, u32 v,
 s32 width, s32 height, u32 flags)
{
  // TODO: Flip/mirror
  s32 current_x, current_y;
  u32 current_u, current_v;
  u32 primitive_color = psx_gpu->primitive_color;
  u32 sprite_r, sprite_g, sprite_b;
  s32 color_r = 0;
  s32 color_g = 0; 
  s32 color_b = 0;
  u32 texel = 0;

  sprite_r = primitive_color & 0xFF;
  sprite_g = (primitive_color >> 8) & 0xFF;
  sprite_b = (primitive_color >> 16) & 0xFF;

  static u32 sprites = 0;

  sprites++;

  for(current_y = y, current_v = v; 
   current_y < y + height; current_y++, current_v++)
  {
    for(current_x = x, current_u = u;
     current_x < x + width; current_x++, current_u++)
    {
      if((current_x >= psx_gpu->viewport_start_x) &&
       (current_y >= psx_gpu->viewport_start_y) &&
       (current_x <= psx_gpu->viewport_end_x) &&
       (current_y <= psx_gpu->viewport_end_y))
      { 
        if(psx_gpu->mask_evaluate &&
         (psx_gpu->vram[(y * 1024) + current_x] & 0x8000))
        {
          continue;
        }

        if(flags & RENDER_FLAGS_TEXTURE_MAP)
        {
          texel = fetch_texel(psx_gpu, current_u, current_v);
          if(texel == 0)
            continue;

          color_r = texel & 0x1F;
          color_g = (texel >> 5) & 0x1F;
          color_b = (texel >> 10) & 0x1F;

          if((flags & RENDER_FLAGS_MODULATE_TEXELS) == 0)
          {
            color_r *= sprite_r;
            color_g *= sprite_g;
            color_b *= sprite_b;

            color_r >>= 7;
            color_g >>= 7;
            color_b >>= 7;
          }
        }
        else
        {
          color_r = sprite_r >> 3;
          color_g = sprite_g >> 3;
          color_b = sprite_b >> 3;
        }

        draw_pixel(psx_gpu, color_r, color_g, color_b, texel, current_x,
         current_y, flags);
      }
    }
  }
}


#define draw_pixel_line(_x, _y)                                                \
  if((_x >= psx_gpu->viewport_start_x) && (_y >= psx_gpu->viewport_start_y) && \
   (_x <= psx_gpu->viewport_end_x) && (_y <= psx_gpu->viewport_end_y))         \
  {                                                                            \
    if(flags & RENDER_FLAGS_SHADE)                                             \
    {                                                                          \
      color_r = fixed_to_int(current_r);                                       \
      color_g = fixed_to_int(current_g);                                       \
      color_b = fixed_to_int(current_b);                                       \
                                                                               \
      current_r += gradient_r;                                                 \
      current_g += gradient_g;                                                 \
      current_b += gradient_b;                                                 \
    }                                                                          \
    else                                                                       \
    {                                                                          \
      color_r = primitive_color & 0xFF;                                        \
      color_g = (primitive_color >> 8) & 0xFF;                                 \
      color_b = (primitive_color >> 16) & 0xFF;                                \
    }                                                                          \
                                                                               \
    if(psx_gpu->dither_mode)                                                   \
    {                                                                          \
      s32 dither_offset = dither_table[_y % 4][_x % 4];                        \
                                                                               \
      color_r += dither_offset;                                                \
      color_g += dither_offset;                                                \
      color_b += dither_offset;                                                \
                                                                               \
      if(color_r < 0)                                                          \
        color_r = 0;                                                           \
                                                                               \
      if(color_g < 0)                                                          \
        color_g = 0;                                                           \
                                                                               \
      if(color_b < 0)                                                          \
        color_b = 0;                                                           \
    }                                                                          \
    color_r >>= 3;                                                             \
    color_g >>= 3;                                                             \
    color_b >>= 3;                                                             \
                                                                               \
    span_pixels++;                                                             \
                                                                               \
    draw_pixel(psx_gpu, color_r, color_g, color_b, 0, _x, _y, flags);          \
  }                                                                            \

#define update_increment(value)                                                \
  value++                                                                      \

#define update_decrement(value)                                                \
  value--                                                                      \

#define compare_increment(a, b)                                                \
  (a <= b)                                                                     \

#define compare_decrement(a, b)                                                \
  (a >= b)                                                                     \

#define set_line_gradients(minor)                                              \
{                                                                              \
  s32 gradient_divisor = delta_##minor;                                        \
  gradient_r = int_to_fixed(vertex_b->r - vertex_a->r) / gradient_divisor;     \
  gradient_g = int_to_fixed(vertex_b->g - vertex_a->g) / gradient_divisor;     \
  gradient_b = int_to_fixed(vertex_b->b - vertex_a->b) / gradient_divisor;     \
  current_r = fixed_center(vertex_a->r);                                       \
  current_g = fixed_center(vertex_a->g);                                       \
  current_b = fixed_center(vertex_a->b);                                       \
}

#define draw_line_span_horizontal(direction)                                   \
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
    draw_pixel_line(current_x, current_y);                                     \
    error += error_step;                                                       \
                                                                               \
    if(error >= error_wrap)                                                    \
    {                                                                          \
      update_##direction(current_y);                                           \
      error -= error_wrap;                                                     \
    }                                                                          \
  }                                                                            \
} while(0)                                                                     \

#define draw_line_span_vertical(direction)                                     \
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
    draw_pixel_line(current_x, current_y);                                     \
    error += error_step;                                                       \
                                                                               \
    if(error > error_wrap)                                                     \
    {                                                                          \
      current_x++;                                                             \
      error -= error_wrap;                                                     \
    }                                                                          \
  }                                                                            \
} while(0)                                                                     \
                                                                               
void render_line(psx_gpu_struct *psx_gpu, vertex_struct *vertexes, u32 flags)
{
  u32 primitive_color = psx_gpu->primitive_color;
  s32 color_r, color_g, color_b;

  fixed_type gradient_r = 0;
  fixed_type gradient_g = 0;
  fixed_type gradient_b = 0;
  fixed_type current_r = 0;
  fixed_type current_g = 0;
  fixed_type current_b = 0;

  s32 y_a, y_b;
  s32 x_a, x_b;

  s32 delta_x, delta_y;
  u32 triangle_winding = 0;

  s32 current_x;
  s32 current_y;

  u32 error_step;
  u32 error;
  u32 error_wrap;

  vertex_struct *vertex_a = &(vertexes[0]);
  vertex_struct *vertex_b = &(vertexes[1]);

  if(vertex_a->x >= vertex_b->x)
  {
    vertex_swap(vertex_a, vertex_b);
  }

  x_a = vertex_a->x;
  x_b = vertex_b->x;

  y_a = vertex_a->y;
  y_b = vertex_b->y;

  delta_x = x_b - x_a;
  delta_y = y_b - y_a;

  if(delta_x >= 1024)
    return;

  flags &= ~RENDER_FLAGS_TEXTURE_MAP;

  if(delta_y < 0)
  {
    delta_y *= -1;

    if(delta_y >= 512)
      return;

    if(delta_x > delta_y)
      draw_line_span_horizontal(decrement);
    else
      draw_line_span_vertical(decrement);
  }
  else
  {
    if(delta_y >= 512)
      return;

    if(delta_x > delta_y)
      draw_line_span_horizontal(increment);
    else
      draw_line_span_vertical(increment);
  }
}


void render_block_fill(psx_gpu_struct *psx_gpu, u32 color, u32 x, u32 y,
 u32 width, u32 height)
{
  u32 r = color & 0xFF;
  u32 g = (color >> 8) & 0xFF;
  u32 b = (color >> 16) & 0xFF;
  u32 color_16bpp = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);

  u16 *vram_ptr = psx_gpu->vram + x + (y * 1024);
  u32 draw_x, draw_y;

  for(draw_y = 0; draw_y < height; draw_y++)
  {
    for(draw_x = 0; draw_x < width; draw_x++)
    {
      vram_ptr[draw_x] = color_16bpp;
    }

    vram_ptr += 1024;
  }
}

void render_block_copy(psx_gpu_struct *psx_gpu, u16 *source, u32 x, u32 y,
 u32 width, u32 height, u32 pitch)
{
  u16 *vram_ptr = psx_gpu->vram + x + (y * 1024);
  u32 draw_x, draw_y;

  for(draw_y = 0; draw_y < height; draw_y++)
  {
    for(draw_x = 0; draw_x < width; draw_x++)
    {
      vram_ptr[draw_x] = source[draw_x];
    }

    source += pitch;
    vram_ptr += 1024;
  }
}

void render_block_move(psx_gpu_struct *psx_gpu, u32 source_x, u32 source_y,
 u32 dest_x, u32 dest_y, u32 width, u32 height)
{
  render_block_copy(psx_gpu, psx_gpu->vram + source_x + (source_y * 1024),
   dest_x, dest_y, width, height, 1024);
}

void initialize_psx_gpu(psx_gpu_struct *psx_gpu)
{
  psx_gpu->pixel_count_mode = 0;
  psx_gpu->pixel_compare_mode = 0;

  psx_gpu->vram_pixel_counts_a = malloc(sizeof(u8) * 1024 * 512);
  psx_gpu->vram_pixel_counts_b = malloc(sizeof(u8) * 1024 * 512);
  memset(psx_gpu->vram_pixel_counts_a, 0, sizeof(u8) * 1024 * 512);
  memset(psx_gpu->vram_pixel_counts_b, 0, sizeof(u8) * 1024 * 512);
  psx_gpu->compare_vram = malloc(sizeof(u16) * 1024 * 512);
}
