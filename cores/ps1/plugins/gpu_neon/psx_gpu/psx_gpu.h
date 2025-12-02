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

#ifndef PSX_GPU_H
#define PSX_GPU_H

#define MAX_SPANS             512
#define MAX_BLOCKS            64
#define MAX_BLOCKS_PER_ROW    128

#define SPAN_DATA_BLOCKS_SIZE 32

#define AHACK_TEXTURE_ADJ_U   (1 << 0)
#define AHACK_TEXTURE_ADJ_V   (1 << 1)

#ifndef __ASSEMBLER__

#include "vector_types.h"

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

#define sign_extend_11bit(value) \
  (((s32)((value) << 21)) >> 21)

typedef enum
{
  PRIMITIVE_TYPE_TRIANGLE = 0,
  PRIMITIVE_TYPE_SPRITE = 1,
  PRIMITIVE_TYPE_LINE = 2,
  PRIMITIVE_TYPE_UNKNOWN = 3
} primitive_type_enum;

typedef enum
{
  TEXTURE_MODE_4BPP  = 0,
  TEXTURE_MODE_8BPP  = 1,
  TEXTURE_MODE_16BPP = 2
} texture_mode_enum;

typedef enum
{
  BLEND_MODE_AVERAGE    = 0,
  BLEND_MODE_ADD        = 1,
  BLEND_MODE_SUBTRACT   = 2,
  BLEND_MODE_ADD_FOURTH = 3
} blend_mode_enum;

typedef enum
{
  RENDER_FLAGS_MODULATE_TEXELS = 0x1,
  RENDER_FLAGS_BLEND           = 0x2,
  RENDER_FLAGS_TEXTURE_MAP     = 0x4,
  RENDER_FLAGS_QUAD            = 0x8,
  RENDER_FLAGS_SHADE           = 0x10,
} render_flags_enum;

typedef enum
{
  RENDER_STATE_DITHER          = 0x8,
  RENDER_STATE_MASK_EVALUATE   = 0x20,
} render_state_enum;

typedef enum
{
  RENDER_INTERLACE_ENABLED     = 0x1,
  RENDER_INTERLACE_ODD         = 0x2,
} render_mode_enum;

typedef struct
{
  u16 left_x;
  u16 num_blocks;
  u16 right_mask;
  u16 y;
} edge_data_struct;

// 64 (72) bytes total
typedef struct
{
  // 16 bytes
  union
  {
    vec_8x16u uv;
    vec_8x16u texels;
    vec_8x16u draw_mask;
  };

  // 24 bytes
  union
  {
    struct
    {
      vec_8x8u r;
      vec_8x8u g;
      vec_8x8u b;
    };

    vec_8x16u pixels;
  };

  // 8 (16) bytes
  u32 draw_mask_bits;
  u16 *fb_ptr;

  // 16 bytes
  vec_8x16u dither_offsets;  
} block_struct;

typedef struct render_block_handler_struct render_block_handler_struct;

typedef struct
{
  // 144 bytes
  vec_8x16u test_mask;

  vec_4x32u uvrg;
  vec_4x32u uvrg_dx;
  vec_4x32u uvrg_dy;

  vec_4x32u u_block_span;
  vec_4x32u v_block_span;
  vec_4x32u r_block_span;
  vec_4x32u g_block_span;
  vec_4x32u b_block_span;

  u32 b;
  u32 b_dy;

  u32 triangle_area;

  u32 texture_window_settings;
  u32 current_texture_mask;
  u32 viewport_mask;
  u32 dirty_textures_4bpp_mask;
  u32 dirty_textures_8bpp_mask;
  u32 dirty_textures_8bpp_alternate_mask;

  u32 triangle_color;
  u32 dither_table[4];

  struct render_block_handler_struct *render_block_handler;
  void *texture_page_ptr;
  void *texture_page_base;
  u16 *clut_ptr;
  u16 *vram_ptr;
  u16 *vram_out_ptr;

  u32 uvrgb_phase;

  u16 render_state_base;
  u16 render_state;

  u16 num_spans;
  u16 num_blocks;

  s16 viewport_start_x;
  s16 viewport_start_y;
  s16 viewport_end_x;
  s16 viewport_end_y;

  u16 mask_msb;

  u8 triangle_winding;

  u8 display_area_draw_enable;

  u8 current_texture_page;
  u8 last_8bpp_texture_page;

  u8 texture_mask_width;
  u8 texture_mask_height;
  u8 texture_window_x;
  u8 texture_window_y;

  u8 primitive_type;
  u8 render_mode;

  s16 offset_x;
  s16 offset_y;

  u16 clut_settings;
  u16 texture_settings;

  u32 *reciprocal_table_ptr;

  // enhancement stuff
  u16 *enhancement_buf_ptr;          // main alloc
  u16 *enhancement_current_buf_ptr;  // offset into above, 4 bufs
  u32 hacks_active;                  // AHACK_TEXTURE_ADJ_U ...
  u32 saved_hres;
  s16 saved_viewport_start_x;
  s16 saved_viewport_start_y;
  s16 saved_viewport_end_x;
  s16 saved_viewport_end_y;
  struct psx_gpu_scanout {
    u16 x, y, w, h;
  } enhancement_scanouts[4];         // 0-3 specifying which buf to use
  u16 enhancement_scanout_eselect;   // eviction selector
  u16 enhancement_current_buf;

  u32 allow_dithering:1;
  u32 force_dithering:1;
  u32 hack_disable_main:1;
  u32 hack_texture_adj:1;

  // Align up to 64 byte boundary to keep the upcoming buffers cache line
  // aligned, also make reachable with single immediate addition
  u8 reserved_a[68 + 9*4 - 9*sizeof(void *)];

  // space for saving regs on c call to flush_render_block_buffer() and asm
  u32 saved_tmp[48 / sizeof(u32)];
  u32 saved_q4_q7[64 / sizeof(u32)];

  // 8KB
  block_struct blocks[MAX_BLOCKS_PER_ROW];

  // 14336 bytes
  vec_4x32u span_uvrg_offset[MAX_SPANS];
  edge_data_struct span_edge_data[MAX_SPANS];
  u32 span_b_offset[MAX_SPANS];

  u8 texture_4bpp_cache[32][256 * 256];
  u8 texture_8bpp_even_cache[16][256 * 256];
  u8 texture_8bpp_odd_cache[16][256 * 256];
} psx_gpu_struct;

typedef struct __attribute__((aligned(16)))
{
  u8 u;
  u8 v;

  u8 r;
  u8 g;
  u8 b;

  u8 reserved[3];

  s16 x;
  s16 y;

  u32 padding;
} vertex_struct;

typedef struct
{
  vertex_struct *vertexes[3];
  s16 offset_x;
  s16 offset_y;
} prepared_triangle;

void render_block_fill(psx_gpu_struct *psx_gpu, u32 color, u32 x, u32 y,
 u32 width, u32 height);
void render_block_copy(psx_gpu_struct *psx_gpu, u16 *source, u32 x, u32 y,
 u32 width, u32 height, u32 pitch);
void render_block_move(psx_gpu_struct *psx_gpu, u32 source_x, u32 source_y,
 u32 dest_x, u32 dest_y, u32 width, u32 height);

void render_triangle(psx_gpu_struct *psx_gpu, vertex_struct *vertexes,
 u32 flags);
void render_sprite(psx_gpu_struct *psx_gpu, s32 x, s32 y, u32 u, u32 v,
 s32 *width, s32 *height, u32 flags, u32 color);
void render_line(psx_gpu_struct *gpu, vertex_struct *vertexes, u32 flags,
 u32 color, int double_resolution);

u32 texture_region_mask(s32 x1, s32 y1, s32 x2, s32 y2);

void update_texture_8bpp_cache(psx_gpu_struct *psx_gpu);
void flush_render_block_buffer(psx_gpu_struct *psx_gpu);

void setup_blocks_uv_adj_hack(psx_gpu_struct *psx_gpu, block_struct *block,
    edge_data_struct *span_edge_data, vec_4x32u *span_uvrg_offset);

void initialize_psx_gpu(psx_gpu_struct *psx_gpu, u16 *vram);
u32 gpu_parse(psx_gpu_struct *psx_gpu, u32 *list, u32 size,
 s32 *cpu_cycles_sum_out, s32 *cpu_cycles_last, u32 *last_command);

void triangle_benchmark(psx_gpu_struct *psx_gpu);

void compute_all_gradients(psx_gpu_struct * __restrict__ psx_gpu,
 const vertex_struct * __restrict__ a, const vertex_struct * __restrict__ b,
 const vertex_struct * __restrict__ c);

#endif // __ASSEMBLER__
#endif // PSX_GPU_H
