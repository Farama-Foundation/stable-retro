/*
 * Copyright (C) 2011 Gilead Kutnick "Exophase" <exophase@gmail.com>
 * Copyright (C) 2012 Gražvydas Ignotas "notaz" <notasas@gmail.com>
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

#define RENDER_INTERLACE_ENABLED                          0x1

#include "psx_gpu.h"
#include "psx_gpu_offsets.h"

#define psx_gpu_b_dx_offset (psx_gpu_b_block_span_offset + 4)

#define edge_data_left_x_offset                           0
#define edge_data_num_blocks_offset                       2
#define edge_data_right_mask_offset                       4
#define edge_data_y_offset                                6

.syntax unified
.text

#if 0
#define save_abi_regs() \
  vpush {q4-q7}
#define restore_abi_regs() \
  vpop  {q4-q7}
#else
#define save_abi_regs()
#define restore_abi_regs()
#endif

#define psx_gpu                                           r0
#define v_a                                               r1
#define v_b                                               r2
#define v_c                                               r3

#define x0                                                r4
#define x1                                                r5
#define x2                                                r6
#define x0_x1                                             r5
#define x1_x2                                             r6
#define y0                                                r7
#define y1                                                r8
#define y2                                                r9
#define y0_y1                                             r7
#define y1_y2                                             r8
#define b0                                                r9
#define b1                                                r10
#define b2                                                r11
#define b0_b1                                             r10
#define b1_b2                                             r11


#define area_r_s                                          r5

#define g_bx0                                             r2
#define g_bx                                              r3
#define g_bx2                                             r4
#define g_bx3                                             r5
#define b_base                                            r6
#define g_by                                              r8

#define gs_bx                                             r7
#define gs_by                                             r10

#define ga_bx                                             g_bx
#define ga_by                                             g_by

#define gw_bx_h                                           g_bx
#define gw_by_h                                           g_by

#define gw_bx_l                                           r11
#define gw_by_l                                           gw_bx_l

#define store_a                                           r0
#define store_b                                           r1
#define store_inc                                         r5


#define v0                                                q0
#define uvrgb0                                            d0
#define x0_y0                                             d1

#define v1                                                q1
#define uvrgb1                                            d2
#define x1_y1                                             d3

#define v2                                                q2
#define uvrgb2                                            d4
#define x2_y2                                             d5

#define x0_ab                                             q3
#define uvrg_xxxx0                                        q3
#define uvrg0                                             d6
#define xxxx0                                             d7

#define x1_ab                                             q4
#define uvrg_xxxx1                                        q4
#define uvrg1                                             d8
#define xxxx1                                             d9

#define x2_ab                                             q5
#define uvrg_xxxx2                                        q5
#define uvrg2                                             d10
#define xxxx2                                             d11

#define y0_ab                                             q6
#define yyyy_uvrg0                                        q6
#define yyyy0                                             d12
#define uvrg0b                                            d13

#define y1_ab                                             q7
#define yyyy_uvrg1                                        q7
#define yyyy1                                             d14
#define uvrg1b                                            d15

#define y2_ab                                             q8
#define yyyy_uvrg2                                        q8
#define yyyy2                                             d16
#define uvrg2b                                            d17

#define d0_ab                                             q9
#define d0_a                                              d18
#define d0_b                                              d19

#define d1_ab                                             q10
#define d1_a                                              d20
#define d1_b                                              d21

#define d2_ab                                             q11
#define d2_a                                              d22
#define d2_b                                              d23

#define d3_ab                                             q12
#define d3_a                                              d24
#define d3_b                                              d25

#define ga_uvrg_x                                         q1
#define ga_uvrg_y                                         q4

#define dx                                                x0_x1
#define dy                                                y0_y1
#define db                                                b0_b1

#define uvrg_base                                         q11

#define gs_uvrg_x                                         q5
#define gs_uvrg_y                                         q6

#define g_uvrg_x                                          q1
#define ga_uv_x                                           d2
#define g_uv_x                                            d2
#define ga_rg_x                                           d3
#define g_rg_x                                            d3

#define g_uvrg_y                                          q4
#define ga_uv_y                                           d8
#define g_uv_y                                            d8
#define ga_rg_y                                           d9
#define g_rg_y                                            d9

#define gw_uv_x                                           q1
#define gw_rg_x                                           q2
#define gw_uv_y                                           q4
#define gw_rg_y                                           q3

#define w_mask                                            q9
#define w_mask_l                                          d18

#define r_shift                                           q10

#define uvrg_dx0                                          q0
#define uvrg_dx0l                                         d0
#define uvrg_dx0h                                         d1

#define uvrg_dx1                                          q1
#define uvrg_dx1l                                         d2
#define uvrg_dx1h                                         d3

#define uvrg_dx2                                          q2
#define uvrg_dx2l                                         d4
#define uvrg_dx2h                                         d5

#define uvrg_dx3                                          q3
#define uvrg_dx3l                                         d6
#define uvrg_dx3h                                         d7

#define uvrgb_phase                                       q13

.align 4

#include "arm_features.h"

#define function(name) FUNCTION(name):

#ifndef TEXRELS_FORBIDDEN

#define JT_OP_REL(table_label, index_reg, temp)
#define JT_OP(x...) x
#define JTE(start, target) target

#else

#define JT_OP_REL(table_label, index_reg, temp)                                \
  adr temp, table_label;                                                       \
  ldr temp, [temp, index_reg, lsl #2];                                         \
  add pc, pc, temp                                                             \

#define JT_OP(x...)
#define JTE(start, target) (target - start)

#endif

#ifdef __MACH__
#define flush_render_block_buffer _flush_render_block_buffer
#define update_texture_8bpp_cache _update_texture_8bpp_cache
#define setup_blocks_uv_adj_hack _setup_blocks_uv_adj_hack
#endif

@ r0: psx_gpu
@ r1: v_a
@ r2: v_b
@ r3: v_c

function(compute_all_gradients)
  // First compute the triangle area reciprocal and shift. The division will
  // happen concurrently with much of the work which follows.
  @ r12 = psx_gpu->triangle_area
  ldr r12, [psx_gpu, #psx_gpu_triangle_area_offset]
  stmdb sp!, { r4 - r11, lr }
  save_abi_regs()

  @ load exponent of 62 into upper half of double
  movw r4, #0
  clz r14, r12                       @ r14 = shift

  movt r4, #((62 + 1023) << 4)
  mov r12, r12, lsl r14              @ r12 = triangle_area_normalized

  @ load area normalized into lower half of double
  mov r5, r12, lsr #10
  vmov.f64 d30, r5, r4               @ d30 = (1 << 62) + ta_n

  movt r4, #((1022 + 31) << 4)
  mov r5, r12, lsl #20

  add r4, r4, r12, lsr #11
  vmov.f64 d31, r5, r4

  vdiv.f64 d30, d30, d31             @ d30 = ((1 << 62) + ta_n) / ta_n

  // ((x1 - x0) * (y2 - y1)) - ((x2 - x1) * (y1 - y0)) =
  // ( d0       *  d1      ) - ( d2       *  d3      ) =
  // ( m0                  ) - ( m1                  ) = gradient

  // This is split to do 12 elements at a time over three sets: a, b, and c.
  // Technically we only need to do 10 elements (uvrgb_x and uvrgb_y), so
  // two of the slots are unused.

  // Inputs are all 16-bit signed. The m0/m1 results are 32-bit signed, as
  // is g.

  // First type is:  uvrg bxxx xxxx 
  // Second type is: yyyy ybyy uvrg 
  // Since x_a and y_c are the same the same variable is used for both. 

  vld1.u32 { v0 }, [v_a, :128]       @ v0 = { uvrg0, b0, x0, y0 }
  ldrsh x0, [v_a, #8]                @ load x0

  vld1.u32 { v1 }, [v_b, :128]       @ v1 = { uvrg1, b1, x1, y1}
  ldrh x1, [v_b, #8]                 @ load x1

  vld1.u32 { v2 }, [v_c, :128]       @ v2 = { uvrg2, b2, x2, y2 }
  ldrh x2, [v_c, #8]                 @ load x2

  vmovl.u8 uvrg_xxxx0, uvrgb0        @ uvrg_xxxx0 = { uv0, rg0, b0-, -- }
  ldrh y0, [v_a, #10]                @ load y0

  vmovl.u8 uvrg_xxxx1, uvrgb1        @ uvrg_xxxx1 = { uv1, rg1, b1-, -- }
  ldrh y1, [v_b, #10]                @ load y1

  vmovl.u8 uvrg_xxxx2, uvrgb2        @ uvrg_xxxx2 = { uv2, rg2, b2-, -- }
  ldrh y2, [v_c, #10]                @ load y2

  vmov.u8 uvrg0b, uvrg0              @ uvrg0b = { uv0, rg0 }
  vdup.u16 xxxx0, x0_y0[0]           @ xxxx0 = { xx0, xx0 }

  orr x1_x2, x1, x2, lsl #16         @ x1_x2 = { x1, x2 }
  pkhbt x0_x1, x0, x1, lsl #16       @ x0_x1 = { x0, x1 }

  vmov.u8 uvrg1b, uvrg1              @ uvrg1b = { uv1, rg1 }
  vdup.u16 xxxx1, x1_y1[0]           @ xxxx1 = { xx1, xx1 }

  vmov.u8 uvrg2b, uvrg2              @ uvrg2b = { uv2, rg2 }
  vdup.u16 xxxx2, x2_y2[0]           @ xxxx2 = { xx2, xx2 }

  ldrb b2, [v_c, #4]                 @ load b2
  orr y0_y1, y0, y1, lsl #16         @ y0_y1 = { y0, y1 }

  ldrb b1, [v_b, #4]                 @ load b1
  orr y1_y2, y1, y2, lsl #16         @ y1_y2 = { y1, y2 }

  vdup.u16 yyyy0, x0_y0[1]           @ yyyy0 = { yy0, yy0 }
  vsub.s16 d0_ab, x1_ab, x0_ab

  ldrb b0, [v_a, #4]                 @ load b0
  orr b1_b2, b1, b2, lsl #16         @ b1_b2 = { b1, b2 }

  vdup.u16 yyyy1, x1_y1[1]           @ yyyy1 = { yy1, yy1 }
  vsub.s16 d2_ab, x2_ab, x1_ab

  vdup.u16 yyyy2, x2_y2[1]           @ yyyy2 = { yy2, yy2 }
  vsub.s16 d1_ab, y2_ab, y1_ab

  orr b0_b1, b0, b1, lsl #16         @ b1_b2 = { b1, b2 }
  ssub16 dx, x1_x2, x0_x1            @ dx = { x1 - x0, x2 - x1 }

  ssub16 dy, y1_y2, y0_y1            @ dy = { y1 - y0, y2 - y1 }
  ssub16 db, b1_b2, b0_b1            @ db = { b1 - b0, b2 - b1 }

  vsub.s16 d3_ab, y1_ab, y0_ab
  smusdx ga_by, dx, db               @ ga_by = ((x1 - x0) * (b2 - b1)) -
                                     @         ((x2 - X1) * (b1 - b0)) 
  vmull.s16 ga_uvrg_x, d0_a, d1_a
  smusdx ga_bx, db, dy               @ ga_bx = ((b1 - b0) * (y2 - y1)) -
                                     @         ((b2 - b1) * (y1 - y0))
  vmlsl.s16 ga_uvrg_x, d2_a, d3_a
  movs gs_bx, ga_bx, asr #31

  vmull.s16 ga_uvrg_y, d0_b, d1_b
  rsbmi ga_bx, ga_bx, #0

  @ r12 = psx_gpu->uvrgb_phase
  ldr r12, [psx_gpu, #psx_gpu_uvrgb_phase_offset]

  vmlsl.s16 ga_uvrg_y, d2_b, d3_b
  movs gs_by, ga_by, asr #31

  vshr.u64 d0, d30, #22
  add b_base, r12, b0, lsl #16

  vdup.u32 uvrgb_phase, r12

  rsbmi ga_by, ga_by, #0
  vclt.s32 gs_uvrg_x, ga_uvrg_x, #0  @ gs_uvrg_x = ga_uvrg_x < 0

  @ r12 = psx_gpu->triangle_winding_offset
  ldrb r12, [psx_gpu, #psx_gpu_triangle_winding_offset]
  vclt.s32 gs_uvrg_y, ga_uvrg_y, #0  @ gs_uvrg_y = ga_uvrg_y < 0

  rsb r12, r12, #0                   @ r12 = -(triangle->winding)

  vdup.u32 w_mask, r12               @ w_mask = { -w, -w, -w, -w }
  sub r14, r14, #(62 - 12)           @ r14 = shift - (62 - FIXED_BITS)

  vshll.u16 uvrg_base, uvrg0, #16    @ uvrg_base = uvrg0 << 16
  vdup.u32 r_shift, r14              @ r_shift = { shift, shift*, shift, shift* }
                                     @ * - vshl.u64: ignored by hw
  vadd.u32 uvrg_base, uvrgb_phase
  vabs.s32 ga_uvrg_x, ga_uvrg_x      @ ga_uvrg_x = abs(ga_uvrg_x)

  vmov area_r_s, s0                  @ area_r_s = triangle_reciprocal
  vabs.s32 ga_uvrg_y, ga_uvrg_y      @ ga_uvrg_y = abs(ga_uvrg_y)

  vmull.u32 gw_rg_x, ga_rg_x, d0[0]
  vmull.u32 gw_uv_x, ga_uv_x, d0[0]
  vmull.u32 gw_rg_y, ga_rg_y, d0[0]
  vmull.u32 gw_uv_y, ga_uv_y, d0[0]

  vshl.u64 gw_rg_x, gw_rg_x, r_shift
  vshl.u64 gw_uv_x, gw_uv_x, r_shift
  vshl.u64 gw_rg_y, gw_rg_y, r_shift
  vshl.u64 gw_uv_y, gw_uv_y, r_shift

  veor.u32 gs_uvrg_x, gs_uvrg_x, w_mask
  vmovn.u64 g_uv_x, gw_uv_x

  veor.u32 gs_uvrg_y, gs_uvrg_y, w_mask
  vmovn.u64 g_rg_x, gw_rg_x

  veor.u32 g_uvrg_x, g_uvrg_x, gs_uvrg_x
  vmovn.u64 g_uv_y, gw_uv_y

  vsub.u32 g_uvrg_x, g_uvrg_x, gs_uvrg_x
  vmovn.u64 g_rg_y, gw_rg_y

  veor.u32 g_uvrg_y, g_uvrg_y, gs_uvrg_y
  mov ga_bx, ga_bx, lsl #13

  vsub.u32 g_uvrg_y, g_uvrg_y, gs_uvrg_y
  mov ga_by, ga_by, lsl #13

  vdup.u32 x0_y0, x0
  umull gw_bx_l, gw_bx_h, ga_bx, area_r_s

  vshl.u32 g_uvrg_x, g_uvrg_x, #4
  vshl.u32 g_uvrg_y, g_uvrg_y, #4

  umull gw_by_l, gw_by_h, ga_by, area_r_s
  vmls.s32 uvrg_base, ga_uvrg_x, x0_y0[0]

  eor gs_bx, gs_bx, r12
  vadd.u32 uvrg_dx2, uvrg_dx1, uvrg_dx1

  veor.u32 uvrg_dx0, uvrg_dx0, uvrg_dx0
  eor gs_by, gs_by, r12

  rsb r11, r14, #0                   @ r11 = negative shift for scalar lsr
  add store_a, psx_gpu, #psx_gpu_uvrg_offset

  sub r11, r11, #(32 - 13)

  add store_b, store_a, #16
  mov store_inc, #32

  vadd.u32 uvrg_dx3, uvrg_dx2, uvrg_dx1
  vst1.u32 { uvrg_base }, [store_a, :128], store_inc

  vst1.u32 { uvrg_dx1 }, [store_b, :128], store_inc
  mov g_bx, gw_bx_h, lsr r11

  vst1.u32 { g_uvrg_y }, [store_a, :128], store_inc
  mov g_by, gw_by_h, lsr r11

  vst4.u32 { uvrg_dx0l, uvrg_dx1l, uvrg_dx2l, uvrg_dx3l },                     \
   [store_b, :128], store_inc
  eor g_bx, g_bx, gs_bx

  vst4.u32 { uvrg_dx0h, uvrg_dx1h, uvrg_dx2h, uvrg_dx3h },                     \
   [store_b, :128], store_inc
  sub g_bx, g_bx, gs_bx

  lsl g_bx, g_bx, #4  
  eor g_by, g_by, gs_by

  mls b_base, g_bx, x0, b_base
  sub g_by, g_by, gs_by

  lsl g_by, g_by, #4
  mov g_bx0, #0

  add g_bx2, g_bx, g_bx
  add g_bx3, g_bx, g_bx2

  stmia store_b, { g_bx0, g_bx, g_bx2, g_bx3, b_base, g_by }

  restore_abi_regs()
  ldmia sp!, { r4 - r11, pc }


#define psx_gpu                                  r0
#define v_a                                      r1
#define v_b                                      r2
#define v_c                                      r3

#define temp                                     r14

#define x_a                                      r4
#define x_b                                      r5
#define x_c                                      r6
#define y_a                                      r1
#define y_b                                      r2
#define y_c                                      r3

#define height_minor_a                           r7
#define height_minor_b                           r8
#define height_major                             r9
#define height                                   r9

#define reciprocal_table_ptr                     r10

#define edge_alt_low                             r4
#define edge_alt_high                            r5
#define edge_dx_dy_alt                           r6
#define edge_shift_alt                           r10

#define edge_dx_dy_alt_low                       r4
#define edge_dx_dy_alt_high                      r5

#define span_edge_data                           r4
#define span_uvrg_offset                         r5
#define span_b_offset                            r6

#define clip                                     r14

#define b                                        r11
#define b_dy                                     r12


#define alternate_x                              q0
#define alternate_dx_dy                          q1
#define alternate_x_32                           q2

#define alternate_x_low                          d0
#define alternate_x_high                         d1
#define alternate_dx_dy_low                      d2
#define alternate_dx_dy_high                     d3
#define alternate_x_32_low                       d4
#define alternate_x_32_high                      d5

#define left_x                                   q3
#define right_x                                  q4
#define left_dx_dy                               q5
#define right_dx_dy                              q6
#define left_edge                                q7
#define right_edge                               q8

#define left_x_low                               d6
#define left_x_high                              d7
#define right_x_low                              d8
#define right_x_high                             d9
#define left_dx_dy_low                           d10
#define left_dx_dy_high                          d11
#define right_dx_dy_low                          d12
#define right_dx_dy_high                         d13
#define left_edge_low                            d14
#define left_edge_high                           d15
#define right_edge_low                           d16
#define right_edge_high                          d17

#define y_mid_point                              d18
#define c_0x0004                                 d19

#define left_right_x_16                          q11
#define span_shifts_y                            q12
#define c_0x0001                                 q13

#define span_shifts                              d24
#define y_x4                                     d25
#define c_0xFFFE                                 d26
#define c_0x0007                                 d27

#define left_right_x_16_low                      d22
#define left_right_x_16_high                     d23

#define uvrg                                     q14
#define uvrg_dy                                  q15
#define uv                                       d28

#define alternate_x_16                           d4

#define v_clip                                   q3
#define v_clip_low                               d6

#define right_x_32                               q10
#define left_x_32                                q11
#define alternate_select                         d24

#define right_x_32_low                           d20
#define right_x_32_high                          d21
#define left_x_32_low                            d22
#define left_x_32_high                           d23

#define tmp_max_blocks                           d20

#define edges_xy                                 q0
#define edges_dx_dy                              d2
#define edge_shifts                              d3
#define edge_shifts_64                           q2

#define edges_xy_left                            d0
#define edges_xy_right                           d1

#define height_reciprocals                       d6
#define heights                                  d7

#define widths                                   d8
#define c_0x01                                   d9
#define x_starts                                 d10
#define x_ends                                   d11

#define heights_b                                d12
#define edges_dx_dy_64                           q10

#define edges_dx_dy_64_left                      d20
#define edges_dx_dy_64_right                     d21


#define setup_spans_prologue()                                                 \
  stmdb sp!, { r4 - r11, lr };                                                 \
  save_abi_regs();                                                             \
                                                                               \
  ldrsh x_a, [v_a, #8];                                                        \
  ldrsh x_b, [v_b, #8];                                                        \
  ldrsh x_c, [v_c, #8];                                                        \
  ldrsh y_a, [v_a, #10];                                                       \
  ldrsh y_b, [v_b, #10];                                                       \
  ldrsh y_c, [v_c, #10];                                                       \
                                                                               \
  add temp, psx_gpu, #psx_gpu_uvrg_offset;                                     \
  vld1.32 { uvrg }, [temp];                                                    \
  add temp, psx_gpu, #psx_gpu_uvrg_dy_offset;                                  \
  vld1.32 { uvrg_dy }, [temp];                                                 \
  ldr reciprocal_table_ptr, [psx_gpu, #psx_gpu_reciprocal_table_ptr_offset];   \
                                                                               \
  vmov.u32 c_0x01, #0x01                                                       \

#define setup_spans_load_b()                                                   \
  ldr b, [psx_gpu, #psx_gpu_b_offset];                                         \
  ldr b_dy, [psx_gpu, #psx_gpu_b_dy_offset]                                    \

#define setup_spans_prologue_b()                                               \
  add span_uvrg_offset, psx_gpu, #psx_gpu_span_uvrg_offset_offset;             \
  add temp, psx_gpu, #psx_gpu_viewport_start_x_offset;                         \
                                                                               \
  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset;                 \
  vmov.u16 c_0x0004, #0x0004;                                                  \
                                                                               \
  add span_b_offset, psx_gpu, #psx_gpu_span_b_offset_offset;                   \
  vmov.u16 c_0x0001, #0x0001;                                                  \
                                                                               \
  vld1.u16 { left_edge_low[], left_edge_high[] }, [temp];                      \
  add temp, psx_gpu, #psx_gpu_viewport_end_x_offset;                           \
                                                                               \
  vld1.u16 { right_edge_low[], right_edge_high[] }, [temp];                    \
  vadd.u16 right_edge, right_edge, c_0x0001;                                   \
                                                                               \
  vmov.u16 c_0x0007, #0x0007;                                                  \
  vmvn.u16 c_0xFFFE, #0x0001                                                   \


#define compute_edge_delta_x2()                                                \
  ldr temp, [reciprocal_table_ptr, height, lsl #2];                            \
                                                                               \
  vdup.u32 heights, height;                                                    \
  vsub.u32 widths, x_ends, x_starts;                                           \
                                                                               \
  vdup.u32 edge_shifts, temp;                                                  \
  vsub.u32 heights_b, heights, c_0x01;                                         \
  vshr.u32 height_reciprocals, edge_shifts, #10;                               \
                                                                               \
  vmla.s32 heights_b, x_starts, heights;                                       \
  vbic.u16 edge_shifts, #0xE0;                                                 \
  vmul.s32 edges_dx_dy, widths, height_reciprocals;                            \
  vmull.s32 edges_xy, heights_b, height_reciprocals                            \

#define width_alt                 r6
#define height_reciprocal_alt     r11
#define height_b_alt              r12

#define compute_edge_delta_x3(start_c, height_a, height_b)                     \
  vmov heights, height_a, height_b;                                            \
  ldr temp, [reciprocal_table_ptr, height_a, lsl #2];                          \
  vmov.u32 edge_shifts[0], temp;                                               \
  ldr temp, [reciprocal_table_ptr, height_b, lsl #2];                          \
  vmov.u32 edge_shifts[1], temp;                                               \
  ldr edge_shift_alt, [reciprocal_table_ptr, height_minor_b, lsl #2];          \
                                                                               \
  vsub.u32 widths, x_ends, x_starts;                                           \
  sub width_alt, x_c, start_c;                                                 \
                                                                               \
  vsub.u32 heights_b, heights, c_0x01;                                         \
  sub height_b_alt, height_minor_b, #1;                                        \
                                                                               \
  vshr.u32 height_reciprocals, edge_shifts, #10;                               \
  lsr height_reciprocal_alt, edge_shift_alt, #10;                              \
                                                                               \
  vmla.s32 heights_b, x_starts, heights;                                       \
  mla height_b_alt, height_minor_b, start_c, height_b_alt;                     \
                                                                               \
  vbic.u16 edge_shifts, #0xE0;                                                 \
  and edge_shift_alt, edge_shift_alt, #0x1F;                                   \
                                                                               \
  vmul.s32 edges_dx_dy, widths, height_reciprocals;                            \
  mul edge_dx_dy_alt, width_alt, height_reciprocal_alt;                        \
                                                                               \
  vmull.s32 edges_xy, heights_b, height_reciprocals;                           \
  smull edge_alt_low, edge_alt_high, height_b_alt, height_reciprocal_alt       \


#define setup_spans_adjust_y_up()                                              \
  vsub.u32 y_x4, y_x4, c_0x0004                                                \

#define setup_spans_adjust_y_down()                                            \
  vadd.u32 y_x4, y_x4, c_0x0004                                                \

#define setup_spans_adjust_interpolants_up()                                   \
  vsub.u32 uvrg, uvrg, uvrg_dy;                                                \
  sub b, b, b_dy                                                               \

#define setup_spans_adjust_interpolants_down()                                 \
  vadd.u32 uvrg, uvrg, uvrg_dy;                                                \
  add b, b, b_dy                                                               \


#define setup_spans_clip_interpolants_increment()                              \
  mla b, b_dy, clip, b;                                                        \
  vmla.s32 uvrg, uvrg_dy, v_clip                                               \

#define setup_spans_clip_interpolants_decrement()                              \
  mls b, b_dy, clip, b;                                                        \
  vmls.s32 uvrg, uvrg_dy, v_clip                                               \

#define setup_spans_clip_alternate_yes()                                       \
  smlal edge_alt_low, edge_alt_high, edge_dx_dy_alt, clip                      \

#define setup_spans_clip_alternate_no()                                        \

#define setup_spans_clip(direction, alternate_active)                          \
  vdup.u32 v_clip, clip;                                                       \
  setup_spans_clip_alternate_##alternate_active();                             \
  setup_spans_clip_interpolants_##direction();                                 \
  vmlal.s32 edges_xy, edges_dx_dy, v_clip_low                                  \


#define setup_spans_adjust_edges_alternate_no(left_index, right_index)         \
  vmovl.s32 edge_shifts_64, edge_shifts;                                       \
  vmovl.s32 edges_dx_dy_64, edges_dx_dy;                                       \
                                                                               \
  vshl.s64 edges_xy, edges_xy, edge_shifts_64;                                 \
  vshl.s64 edges_dx_dy_64, edges_dx_dy_64, edge_shifts_64;                     \
                                                                               \
  vmov left_x_low, edges_xy_##left_index;                                      \
  vmov right_x_low, edges_xy_##right_index;                                    \
                                                                               \
  vmov left_dx_dy_low, edges_dx_dy_64_##left_index;                            \
  vmov left_dx_dy_high, edges_dx_dy_64_##left_index;                           \
  vmov right_dx_dy_low, edges_dx_dy_64_##right_index;                          \
  vmov right_dx_dy_high, edges_dx_dy_64_##right_index;                         \
                                                                               \
  vadd.u64 left_x_high, left_x_low, left_dx_dy_low;                            \
  vadd.u64 right_x_high, right_x_low, right_dx_dy_low;                         \
                                                                               \
  vadd.u64 left_dx_dy, left_dx_dy, left_dx_dy;                                 \
  vadd.u64 right_dx_dy, right_dx_dy, right_dx_dy                               \


#define setup_spans_adjust_edges_alternate_yes(left_index, right_index)        \
  setup_spans_adjust_edges_alternate_no(left_index, right_index);              \
                                                                               \
  vdup.u16 y_mid_point, y_b;                                                   \
  rsb temp, edge_shift_alt, #32;                                               \
                                                                               \
  lsl edge_alt_high, edge_alt_high, edge_shift_alt;                            \
  orr edge_alt_high, edge_alt_high, edge_alt_low, lsr temp;                    \
  lsl edge_alt_low, edge_alt_low, edge_shift_alt;                              \
  vmov alternate_x_low, edge_alt_low, edge_alt_high;                           \
                                                                               \
  asr edge_dx_dy_alt_high, edge_dx_dy_alt, temp;                               \
  lsl edge_dx_dy_alt_low, edge_dx_dy_alt, edge_shift_alt;                      \
  vmov alternate_dx_dy_low, edge_dx_dy_alt_low, edge_dx_dy_alt_high;           \
  vmov alternate_dx_dy_high, alternate_dx_dy_low;                              \
                                                                               \
  vadd.u64 alternate_x_high, alternate_x_low, alternate_dx_dy_low;             \
  vadd.u64 alternate_dx_dy, alternate_dx_dy, alternate_dx_dy                   \


#define setup_spans_y_select_up()                                              \
  vclt.s16 alternate_select, y_x4, y_mid_point                                 \

#define setup_spans_y_select_down()                                            \
  vcgt.s16 alternate_select, y_x4, y_mid_point                                 \


#define setup_spans_alternate_select_left()                                    \
  vbit.u16 left_right_x_16_low, alternate_x_16, alternate_select               \

#define setup_spans_alternate_select_right()                                   \
  vbit.u16 left_right_x_16_high, alternate_x_16, alternate_select              \


#define setup_spans_set_x4_alternate_yes(alternate, direction)                 \
  vshrn.s64 alternate_x_32_low, alternate_x, #32;                              \
  vshrn.s64 left_x_32_low, left_x, #32;                                        \
  vshrn.s64 right_x_32_low, right_x, #32;                                      \
                                                                               \
  vadd.u64 alternate_x, alternate_x, alternate_dx_dy;                          \
  vadd.u64 left_x, left_x, left_dx_dy;                                         \
  vadd.u64 right_x, right_x, right_dx_dy;                                      \
                                                                               \
  vshrn.s64 alternate_x_32_high, alternate_x, #32;                             \
  vshrn.s64 left_x_32_high, left_x, #32;                                       \
  vshrn.s64 right_x_32_high, right_x, #32;                                     \
                                                                               \
  vadd.u64 alternate_x, alternate_x, alternate_dx_dy;                          \
  vadd.u64 left_x, left_x, left_dx_dy;                                         \
  vadd.u64 right_x, right_x, right_dx_dy;                                      \
                                                                               \
  vmovn.u32 alternate_x_16, alternate_x_32;                                    \
  setup_spans_y_select_##direction();                                          \
  vmovn.u32 left_right_x_16_low, left_x_32;                                    \
                                                                               \
  vmovn.u32 left_right_x_16_high, right_x_32;                                  \
  setup_spans_alternate_select_##alternate();                                  \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vmax.s16 left_right_x_16, left_right_x_16, left_edge;                        \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vmin.s16 left_right_x_16, left_right_x_16, right_edge;                       \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vsub.u16 left_right_x_16_high, left_right_x_16_high, left_right_x_16_low;    \
  vadd.u16 left_right_x_16_high, left_right_x_16_high, c_0x0007;               \
  vand.u16 span_shifts, left_right_x_16_high, c_0x0007;                        \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vmov.u16 tmp_max_blocks, #MAX_BLOCKS_PER_ROW;                                \
  vshr.u16 left_right_x_16_high, left_right_x_16_high, #3;                     \
  vshl.u16 span_shifts, c_0xFFFE, span_shifts;                                 \
  vmin.u16 left_right_x_16_high, left_right_x_16_high, tmp_max_blocks;         \
                                                                               \
  vst4.u16 { left_right_x_16, span_shifts_y }, [span_edge_data]!;              \
                                                                               \
  setup_spans_adjust_y_##direction()                                           \


#define setup_spans_set_x4_alternate_no(alternate, direction)                  \
  vshrn.s64 left_x_32_low, left_x, #32;                                        \
  vshrn.s64 right_x_32_low, right_x, #32;                                      \
                                                                               \
  vadd.u64 left_x, left_x, left_dx_dy;                                         \
  vadd.u64 right_x, right_x, right_dx_dy;                                      \
                                                                               \
  vshrn.s64 left_x_32_high, left_x, #32;                                       \
  vshrn.s64 right_x_32_high, right_x, #32;                                     \
                                                                               \
  vadd.u64 left_x, left_x, left_dx_dy;                                         \
  vadd.u64 right_x, right_x, right_dx_dy;                                      \
                                                                               \
  vmovn.u32 left_right_x_16_low, left_x_32;                                    \
  vmovn.u32 left_right_x_16_high, right_x_32;                                  \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vmax.s16 left_right_x_16, left_right_x_16, left_edge;                        \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vmin.s16 left_right_x_16, left_right_x_16, right_edge;                       \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vsub.u16 left_right_x_16_high, left_right_x_16_high, left_right_x_16_low;    \
  vadd.u16 left_right_x_16_high, left_right_x_16_high, c_0x0007;               \
  vand.u16 span_shifts, left_right_x_16_high, c_0x0007;                        \
                                                                               \
  vst1.u32 { uvrg }, [span_uvrg_offset, :128]!;                                \
  str b, [span_b_offset], #4;                                                  \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  vmov.u16 tmp_max_blocks, #MAX_BLOCKS_PER_ROW;                                \
  vshr.u16 left_right_x_16_high, left_right_x_16_high, #3;                     \
  vshl.u16 span_shifts, c_0xFFFE, span_shifts;                                 \
  vmin.u16 left_right_x_16_high, left_right_x_16_high, tmp_max_blocks;         \
                                                                               \
  vst4.u16 { left_right_x_16, span_shifts_y }, [span_edge_data]!;              \
                                                                               \
  setup_spans_adjust_y_##direction()                                           \


#define edge_adjust_low           r11
#define edge_adjust_high          r12

#define setup_spans_alternate_adjust_yes()                                     \
  smull edge_adjust_low, edge_adjust_high, edge_dx_dy_alt, height_minor_a;     \
  subs edge_alt_low, edge_alt_low, edge_adjust_low;                            \
  sbc edge_alt_high, edge_alt_high, edge_adjust_high                           \

#define setup_spans_alternate_adjust_no()                                      \


#define setup_spans_down(left_index, right_index, alternate, alternate_active) \
  setup_spans_alternate_adjust_##alternate_active();                           \
  setup_spans_load_b();                                                        \
                                                                               \
  ldrsh temp, [psx_gpu, #psx_gpu_viewport_end_y_offset];                       \
  subs y_c, y_c, temp;                                                         \
  subgt height, height, y_c;                                                   \
  addgt height, height, #1;                                                    \
                                                                               \
  ldrsh temp, [psx_gpu, #psx_gpu_viewport_start_y_offset];                     \
  subs clip, temp, y_a;                                                        \
  ble 0f;                                                                      \
                                                                               \
  sub height, height, clip;                                                    \
  add y_a, y_a, clip;                                                          \
  setup_spans_clip(increment, alternate_active);                               \
                                                                               \
 0:                                                                            \
  cmp height, #0;                                                              \
  ble 1f;                                                                      \
                                                                               \
  orr temp, y_a, y_a, lsl #16;                                                 \
  cmp height, #512;                                                            \
  add temp, temp, #(1 << 16);                                                  \
  movgt height, #512;                                                          \
  add y_a, temp, #2;                                                           \
  add y_a, y_a, #(2 << 16);                                                    \
  vmov y_x4, temp, y_a;                                                        \
                                                                               \
  setup_spans_adjust_edges_alternate_##alternate_active(left_index,            \
   right_index);                                                               \
  setup_spans_prologue_b();                                                    \
                                                                               \
  strh height, [psx_gpu, #psx_gpu_num_spans_offset];                           \
                                                                               \
 2:                                                                            \
  setup_spans_set_x4_alternate_##alternate_active(alternate, down);            \
  subs height, height, #4;                                                     \
  bhi 2b;                                                                      \
                                                                               \
  nop;                                                                         \
  ldr temp, [psx_gpu, #psx_gpu_hacks_active_offset];                           \
  tst temp, #(AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V);                      \
  beq 1f;                                                                      \
  add temp, span_uvrg_offset, height, lsl #4;                                  \
  vldr uv, [temp, #(-16*2)];                                                   \
  vstr uv, [temp, #(-16)];                                                     \
                                                                               \
 1:                                                                            \


#define setup_spans_alternate_pre_increment_yes()                              \
  adds edge_alt_low, edge_alt_low, edge_dx_dy_alt;                             \
  adc edge_alt_high, edge_alt_high, edge_dx_dy_alt, asr #31                    \

#define setup_spans_alternate_pre_increment_no()                               \


#define setup_spans_up_decrement_yes()                                         \
  suble height, height, #1                                                     \

#define setup_spans_up_decrement_no()                                          \


#define setup_spans_up(left_index, right_index, alternate, alternate_active)   \
  setup_spans_alternate_adjust_##alternate_active();                           \
  setup_spans_load_b();                                                        \
  sub y_a, y_a, #1;                                                            \
                                                                               \
  ldrh temp, [psx_gpu, #psx_gpu_viewport_start_y_offset];                      \
  subs temp, temp, y_c;                                                        \
  subgt height, height, temp;                                                  \
  setup_spans_up_decrement_##alternate_active();                               \
                                                                               \
  ldrh temp, [psx_gpu, #psx_gpu_viewport_end_y_offset];                        \
  subs clip, y_a, temp;                                                        \
  ble 0f;                                                                      \
                                                                               \
  sub height, height, clip;                                                    \
  sub y_a, y_a, clip;                                                          \
  setup_spans_clip(decrement, alternate_active);                               \
                                                                               \
 0:                                                                            \
  cmp height, #0;                                                              \
  ble 1f;                                                                      \
                                                                               \
  orr temp, y_a, y_a, lsl #16;                                                 \
  cmp height, #512;                                                            \
  sub temp, temp, #(1 << 16);                                                  \
  movgt height, #512;                                                          \
  sub y_a, temp, #2;                                                           \
  sub y_a, y_a, #(2 << 16);                                                    \
  vmov y_x4, temp, y_a;                                                        \
                                                                               \
  vaddw.s32 edges_xy, edges_xy, edges_dx_dy;                                   \
                                                                               \
  setup_spans_alternate_pre_increment_##alternate_active();                    \
  setup_spans_adjust_edges_alternate_##alternate_active(left_index,            \
   right_index);                                                               \
  setup_spans_adjust_interpolants_up();                                        \
  setup_spans_prologue_b();                                                    \
                                                                               \
  strh height, [psx_gpu, #psx_gpu_num_spans_offset];                           \
                                                                               \
 2:                                                                            \
  setup_spans_set_x4_alternate_##alternate_active(alternate, up);              \
  subs height, height, #4;                                                     \
  bhi 2b;                                                                      \
                                                                               \
  nop;                                                                         \
  ldr temp, [psx_gpu, #psx_gpu_hacks_active_offset];                           \
  tst temp, #AHACK_TEXTURE_ADJ_V;                                              \
  beq 1f;                                                                      \
  add temp, psx_gpu, #psx_gpu_span_uvrg_offset_offset;                         \
  vldr uv, [temp, #16];                                                        \
  vstr uv, [temp, #0];                                                         \
                                                                               \
 1:                                                                            \


#define setup_spans_epilogue()                                                 \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4 - r11, pc }                                                  \


#define setup_spans_up_up(minor, major)                                        \
  setup_spans_prologue();                                                      \
  sub height_minor_a, y_a, y_b;                                                \
  sub height_minor_b, y_b, y_c;                                                \
  sub height, y_a, y_c;                                                        \
                                                                               \
  vdup.u32 x_starts, x_a;                                                      \
  vmov x_ends, x_c, x_b;                                                       \
                                                                               \
  compute_edge_delta_x3(x_b, height_major, height_minor_a);                    \
  setup_spans_up(major, minor, minor, yes);                                    \
  setup_spans_epilogue()                                                       \

function(setup_spans_up_left)
  setup_spans_up_up(left, right)

function(setup_spans_up_right)
  setup_spans_up_up(right, left)

#define setup_spans_down_down(minor, major)                                    \
  setup_spans_prologue();                                                      \
  sub height_minor_a, y_b, y_a;                                                \
  sub height_minor_b, y_c, y_b;                                                \
  sub height, y_c, y_a;                                                        \
                                                                               \
  vdup.u32 x_starts, x_a;                                                      \
  vmov x_ends, x_c, x_b;                                                       \
                                                                               \
  compute_edge_delta_x3(x_b, height_major, height_minor_a);                    \
  setup_spans_down(major, minor, minor, yes);                                  \
  setup_spans_epilogue()                                                       \

function(setup_spans_down_left)
  setup_spans_down_down(left, right)

function(setup_spans_down_right)
  setup_spans_down_down(right, left)


#define setup_spans_up_flat()                                                  \
  sub height, y_a, y_c;                                                        \
                                                                               \
  compute_edge_delta_x2();                                                     \
  setup_spans_up(left, right, none, no);                                       \
  setup_spans_epilogue()                                                       \

function(setup_spans_up_a)
  setup_spans_prologue()

  vmov x_starts, x_a, x_b
  vdup.u32 x_ends, x_c

  setup_spans_up_flat()

function(setup_spans_up_b)
  setup_spans_prologue()

  vdup.u32 x_starts, x_a
  vmov x_ends, x_b, x_c

  setup_spans_up_flat()

#define setup_spans_down_flat()                                                \
  sub height, y_c, y_a;                                                        \
                                                                               \
  compute_edge_delta_x2();                                                     \
  setup_spans_down(left, right, none, no);                                     \
  setup_spans_epilogue()                                                       \

function(setup_spans_down_a)
  setup_spans_prologue()

  vmov x_starts, x_a, x_b
  vdup.u32 x_ends, x_c

  setup_spans_down_flat()

function(setup_spans_down_b)
  setup_spans_prologue()

  vdup.u32 x_starts, x_a
  vmov x_ends, x_b, x_c

  setup_spans_down_flat()


#define middle_y                                          r9

#define edges_xy_b                                        q11
#define edges_dx_dy_b                                     d26
#define edge_shifts_b                                     d27
#define edges_dx_dy_and_shifts_b                          q13
#define height_increment                                  d20

#define edges_dx_dy_and_shifts                            q1

#define edges_xy_b_left                                   d22
#define edges_xy_b_right                                  d23

#define setup_spans_up_down_load_edge_set_b()                                  \
  vmov edges_xy, edges_xy_b;                                                   \
  vmov edges_dx_dy_and_shifts, edges_dx_dy_and_shifts_b                        \


function(setup_spans_up_down)
  setup_spans_prologue()

  // s32 middle_y = y_a;
  sub height_minor_a, y_a, y_b
  sub height_minor_b, y_c, y_a
  sub height_major, y_c, y_b

  vmov x_starts, x_a, x_c
  vdup.u32 x_ends, x_b

  compute_edge_delta_x3(x_a, height_minor_a, height_major)

  mov temp, #0
  vmov height_increment, temp, height_minor_b
  vmlal.s32 edges_xy, edges_dx_dy, height_increment

  vmov edges_xy_b_left, edge_alt_low, edge_alt_high
  vmov edges_xy_b_right, edges_xy_right

  vmov edge_shifts_b, edge_shifts
  vmov.u32 edge_shifts_b[0], edge_shift_alt

  vneg.s32 edges_dx_dy_b, edges_dx_dy
  vmov.u32 edges_dx_dy_b[0], edge_dx_dy_alt

  mov middle_y, y_a
  
  setup_spans_load_b()
  sub y_a, y_a, #1

  ldrh temp, [psx_gpu, #psx_gpu_viewport_start_y_offset]
  subs temp, temp, y_b
  subgt height_minor_a, height_minor_a, temp

  ldrh temp, [psx_gpu, #psx_gpu_viewport_end_y_offset]
  subs clip, y_a, temp
  ble 0f

  sub height_minor_a, height_minor_a, clip
  sub y_a, y_a, clip
  setup_spans_clip(decrement, no)

 0:                                                                
  cmp height_minor_a, #0
  ble 3f

  orr temp, y_a, y_a, lsl #16
  sub temp, temp, #(1 << 16)
  sub y_a, temp, #2
  sub y_a, y_a, #(2 << 16)
  vmov y_x4, temp, y_a

  vaddw.s32 edges_xy, edges_xy, edges_dx_dy

  strh height_minor_a, [psx_gpu, #psx_gpu_num_spans_offset]

  setup_spans_adjust_edges_alternate_no(left, right); 
  setup_spans_adjust_interpolants_up()
  setup_spans_up_down_load_edge_set_b()

  setup_spans_prologue_b()


 2: 
  setup_spans_set_x4_alternate_no(none, up)
  subs height_minor_a, height_minor_a, #4
  bhi 2b

  add span_edge_data, span_edge_data, height_minor_a, lsl #3
  add span_uvrg_offset, span_uvrg_offset, height_minor_a, lsl #4
  add span_b_offset, span_b_offset, height_minor_a, lsl #2

 4:
  add temp, psx_gpu, #psx_gpu_uvrg_offset
  vld1.32 { uvrg }, [temp]
  mov y_a, middle_y
  
  setup_spans_load_b()

  ldrh temp, [psx_gpu, #psx_gpu_viewport_end_y_offset]
  subs y_c, y_c, temp
  subgt height_minor_b, height_minor_b, y_c
  addgt height_minor_b, height_minor_b, #1

  ldrh temp, [psx_gpu, #psx_gpu_viewport_start_y_offset]
  subs clip, temp, y_a
  ble 0f

  sub height_minor_b, height_minor_b, clip
  add y_a, y_a, clip
  setup_spans_clip(increment, no)

 0:
  cmp height_minor_b, #0
  ble 1f

  orr temp, y_a, y_a, lsl #16
  add temp, temp, #(1 << 16) 
  add y_a, temp, #2
  add y_a, y_a, #(2 << 16)
  vmov y_x4, temp, y_a

  setup_spans_adjust_edges_alternate_no(left, right)

  ldrh temp, [psx_gpu, #psx_gpu_num_spans_offset]
  add temp, temp, height_minor_b

  cmp temp, #MAX_SPANS
  beq 5f

  strh temp, [psx_gpu, #psx_gpu_num_spans_offset]

 2:                                                     
  setup_spans_set_x4_alternate_no(none, down)
  subs height_minor_b, height_minor_b, #4
  bhi 2b

  nop
  ldr temp, [psx_gpu, #psx_gpu_hacks_active_offset]
  tst temp, #(AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V)
  beq 1f
  add temp, span_uvrg_offset, height, lsl #4
  vldr uv, [temp, #(-16*2)]
  vstr uv, [temp, #(-16)]

 1:
  setup_spans_epilogue()

 3:
  setup_spans_up_down_load_edge_set_b()
  setup_spans_prologue_b()
  bal 4b

 5:
  // FIXME: overflow corner case
  sub temp, temp, height_minor_b
  bics height_minor_b, #3
  add temp, temp, height_minor_b
  strh temp, [psx_gpu, #psx_gpu_num_spans_offset]
  bne 2b
  bal 1b

#undef span_uvrg_offset
#undef span_edge_data
#undef span_b_offset
#undef left_x
#undef b

#define psx_gpu                                  r0
#define num_spans                                r1
#define span_uvrg_offset                         r2
#define span_edge_data                           r3
#define span_b_offset                            r4
#define b_dx                                     r5
#define span_num_blocks                          r6
#define y                                        r7
#define left_x                                   r8
#define b                                        r9
#define dither_offset_ptr                        r10
#define block_ptr_a                              r11
#define fb_ptr                                   r12
#define num_blocks                               r14

#define uvrg_dx_ptr                              r2
#define texture_mask_ptr                         r3
#define hacks_active                             r6
#define dither_shift                             r8
#define dither_row                               r10

#define c_32                                     r7
#define b_dx4                                    r8
#define b_dx8                                    r9
#define block_ptr_b                              r10

#define block_span_ptr                           r10
#define right_mask                               r8

#define color                                    r2
#define color_r                                  r3
#define color_g                                  r4
#define color_b                                  r5

#undef uvrg
#undef uv

#define u_block                                  q0
#define v_block                                  q1
#define r_block                                  q2
#define g_block                                  q3
#define b_block                                  q4

#define uv_dx4                                   d10
#define rg_dx4                                   d11
#define uv_dx8                                   d12
#define rg_dx8                                   d13
#define b_whole_8                                d14
#define fb_mask_ptrs                             d15

#define uvrg_dx4                                 q5
#define uvrg_dx8                                 q6
#define uv_dx8                                   d12
#define rg_dx8                                   d13

#define u_whole                                  q8
#define v_whole                                  q9
#define r_whole                                  q10
#define g_whole                                  q11
#define b_whole                                  q12

#define u_whole_low                              d16
#define u_whole_high                             d17
#define v_whole_low                              d18
#define v_whole_high                             d19
#define r_whole_low                              d20
#define r_whole_high                             d21
#define g_whole_low                              d22
#define g_whole_high                             d23
#define b_whole_low                              d24
#define b_whole_high                             d25

#define dx4                                      q13
#define dx8                                      q13

#define u_whole_8                                d26
#define v_whole_8                                d27
#define u_whole_8b                               d24
#define r_whole_8                                d24
#define g_whole_8                                d25

#define uv_whole_8                               q13
#define uv_whole_8b                              q14

#define dither_offsets                           q14
#define texture_mask                             q15
#define texture_mask_u                           d30
#define texture_mask_v                           d31

#define dither_offsets_short                     d28

#define v_left_x                                 q8
#define uvrg                                     q9
#define block_span                               q10

#define uv                                       d18
#define rg                                       d19

#define draw_mask                                q1
#define draw_mask_edge                           q13
#define test_mask                                q0

#define uvrg_dx                                  q3

#define colors                                   q2

#define setup_blocks_texture_swizzled()                                        \
  vand.u8 u_whole_8b, u_whole_8, texture_mask_u;                               \
  vsli.u8 u_whole_8, v_whole_8, #4;                                            \
  vsri.u8 v_whole_8, u_whole_8b, #4                                            \

#define setup_blocks_texture_unswizzled()                                      \

#define setup_blocks_uv_adj_hack_textured(hacks_active)                        \
  tst hacks_active, #(AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V);              \
  beq 91f;                                                                     \
                                                                               \
  /* pushing odd num of regs here realigns our unaligned stack */              \
  vstr texture_mask_u, [r0, #psx_gpu_saved_tmp_offset];                        \
  vstr texture_mask_v, [r0, #psx_gpu_saved_tmp_offset + 8];                    \
  push { r0 - r4, EXTRA_UNSAVED_REGS r12, r14 };                               \
  mov r12, span_uvrg_offset;                                                   \
  sub r1, block_ptr_a, #64;                                                    \
  mov r2, span_edge_data;                                                      \
  mov r3, r12;                                                                 \
  bl setup_blocks_uv_adj_hack; /* psx_gpu=r0 */                                \
  pop  { r0 - r4, EXTRA_UNSAVED_REGS r12, r14 };                               \
  vldr texture_mask_u, [r0, #psx_gpu_saved_tmp_offset];                        \
  vldr texture_mask_v, [r0, #psx_gpu_saved_tmp_offset + 8];                    \
                                                                               \
  vadd.u32 uvrg_dx8, uvrg_dx4, uvrg_dx4;                                       \
91:                                                                            \


#define setup_blocks_shaded_textured_builder(swizzling)                        \
.align 3;                                                                      \
                                                                               \
function(setup_blocks_shaded_textured_dithered_##swizzling##_indirect)         \
  ldrh num_spans, [psx_gpu, #psx_gpu_num_spans_offset];                        \
  add uvrg_dx_ptr, psx_gpu, #psx_gpu_uvrg_dx_offset;                           \
                                                                               \
  vld1.u32 { uvrg_dx }, [uvrg_dx_ptr, :128];                                   \
  add texture_mask_ptr, psx_gpu, #psx_gpu_texture_mask_width_offset;           \
                                                                               \
  cmp num_spans, #0;                                                           \
  bxeq lr;                                                                     \
                                                                               \
  stmdb sp!, { r4 - r11, r14 };                                                \
  save_abi_regs();                                                             \
  vshl.u32 uvrg_dx4, uvrg_dx, #2;                                              \
                                                                               \
  ldr b_dx, [psx_gpu, #psx_gpu_b_dx_offset];                                   \
  vshl.u32 uvrg_dx8, uvrg_dx, #3;                                              \
                                                                               \
  vld2.u8 { texture_mask_u[], texture_mask_v[] }, [texture_mask_ptr, :16];     \
  add span_uvrg_offset, psx_gpu, #psx_gpu_span_uvrg_offset_offset;             \
                                                                               \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset;                 \
                                                                               \
  add span_b_offset, psx_gpu, #psx_gpu_span_b_offset_offset;                   \
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset;                            \
                                                                               \
  add block_ptr_a, block_ptr_a, num_blocks, lsl #6;                            \
                                                                               \
 0:                                                                            \
  vmov.u8 fb_mask_ptrs, #0;                                                    \
                                                                               \
  ldrh span_num_blocks, [span_edge_data, #edge_data_num_blocks_offset];        \
  add dither_offset_ptr, psx_gpu, #psx_gpu_dither_table_offset;                \
                                                                               \
  ldrh y, [span_edge_data, #edge_data_y_offset];                               \
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset];                         \
                                                                               \
  cmp span_num_blocks, #0;                                                     \
  beq 1f;                                                                      \
                                                                               \
  ldrh left_x, [span_edge_data, #edge_data_left_x_offset];                     \
  add num_blocks, span_num_blocks, num_blocks;                                 \
                                                                               \
  cmp num_blocks, #MAX_BLOCKS;                                                 \
  bgt 2f;                                                                      \
                                                                               \
 3:                                                                            \
  ldr b, [span_b_offset];                                                      \
  add fb_ptr, fb_ptr, y, lsl #11;                                              \
                                                                               \
  vdup.u32 v_left_x, left_x;                                                   \
  and y, y, #0x3;                                                              \
                                                                               \
  ldr dither_row, [dither_offset_ptr, y, lsl #2];                              \
  add fb_ptr, fb_ptr, left_x, lsl #1;                                          \
                                                                               \
  mla b, b_dx, left_x, b;                                                      \
  and dither_shift, left_x, #0x03;                                             \
                                                                               \
  vld1.u32 { uvrg }, [span_uvrg_offset, :128];                                 \
  vshr.u32 uvrg_dx, uvrg_dx4, #2;                                              \
                                                                               \
  mov dither_shift, dither_shift, lsl #3;                                      \
  vmla.u32 uvrg, uvrg_dx, v_left_x;                                            \
                                                                               \
  mov c_32, #32;                                                               \
  subs span_num_blocks, span_num_blocks, #1;                                   \
                                                                               \
  mov dither_row, dither_row, ror dither_shift;                                \
  mov b_dx4, b_dx, lsl #2;                                                     \
                                                                               \
  vdup.u32 dither_offsets_short, dither_row;                                   \
  add block_span_ptr, psx_gpu, #psx_gpu_u_block_span_offset;                   \
                                                                               \
  vdup.u32 b_block, b;                                                         \
  vshll.s8 dither_offsets, dither_offsets_short, #4;                           \
                                                                               \
  vdup.u32 u_block, uv[0];                                                     \
  mov b_dx8, b_dx, lsl #3;                                                     \
                                                                               \
  vdup.u32 v_block, uv[1];                                                     \
  vdup.u32 r_block, rg[0];                                                     \
  vdup.u32 g_block, rg[1];                                                     \
                                                                               \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 u_block, u_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 v_block, v_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 r_block, r_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 g_block, g_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128];                             \
                                                                               \
  vadd.u32 b_block, b_block, block_span;                                       \
  add block_ptr_b, block_ptr_a, #16;                                           \
                                                                               \
  vshrn.u32 u_whole_low, u_block, #16;                                         \
  vshrn.u32 v_whole_low, v_block, #16;                                         \
  vshrn.u32 r_whole_low, r_block, #16;                                         \
  vshrn.u32 g_whole_low, g_block, #16;                                         \
                                                                               \
  vdup.u32 dx4, uv_dx4[0];                                                     \
  vshrn.u32 b_whole_low, b_block, #16;                                         \
                                                                               \
  vaddhn.u32 u_whole_high, u_block, dx4;                                       \
  vdup.u32 dx4, uv_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 v_whole_high, v_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[0];                                                     \
                                                                               \
  vaddhn.u32 r_whole_high, r_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 g_whole_high, g_block, dx4;                                       \
  vdup.u32 dx4, b_dx4;                                                         \
                                                                               \
  vaddhn.u32 b_whole_high, b_block, dx4;                                       \
  vdup.u32 dx8, uv_dx8[0];                                                     \
                                                                               \
  vadd.u32 u_block, u_block, dx8;                                              \
  vdup.u32 dx8, uv_dx8[1];                                                     \
                                                                               \
  vadd.u32 v_block, v_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[0];                                                     \
                                                                               \
  vadd.u32 r_block, r_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[1];                                                     \
                                                                               \
  vadd.u32 g_block, g_block, dx8;                                              \
  vdup.u32 dx8, b_dx8;                                                         \
                                                                               \
  vadd.u32 b_block, b_block, dx8;                                              \
  vmovn.u16 u_whole_8, u_whole;                                                \
                                                                               \
  vmovn.u16 v_whole_8, v_whole;                                                \
                                                                               \
  vmovn.u16 b_whole_8, b_whole;                                                \
  pld [fb_ptr];                                                                \
  vmov.u32 fb_mask_ptrs[1], fb_ptr;                                            \
                                                                               \
  vand.u8 uv_whole_8, uv_whole_8, texture_mask;                                \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  vmovn.u16 r_whole_8, r_whole;                                                \
  beq 5f;                                                                      \
                                                                               \
 4:                                                                            \
  vmovn.u16 g_whole_8, g_whole;                                                \
  vshrn.u32 u_whole_low, u_block, #16;                                         \
                                                                               \
  vst2.u8 { u_whole_8, v_whole_8 }, [block_ptr_a, :128], c_32;                 \
  vshrn.u32 v_whole_low, v_block, #16;                                         \
                                                                               \
  vst1.u32 { r_whole_8, g_whole_8 }, [block_ptr_b, :128], c_32;                \
  vshrn.u32 r_whole_low, r_block, #16;                                         \
                                                                               \
  vst1.u32 { b_whole_8, fb_mask_ptrs }, [block_ptr_a, :128], c_32;             \
  vshrn.u32 g_whole_low, g_block, #16;                                         \
                                                                               \
  vdup.u32 dx4, uv_dx4[0];                                                     \
  vshrn.u32 b_whole_low, b_block, #16;                                         \
                                                                               \
  vaddhn.u32 u_whole_high, u_block, dx4;                                       \
  vdup.u32 dx4, uv_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 v_whole_high, v_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[0];                                                     \
                                                                               \
  vaddhn.u32 r_whole_high, r_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 g_whole_high, g_block, dx4;                                       \
  vdup.u32 dx4, b_dx4;                                                         \
                                                                               \
  vaddhn.u32 b_whole_high, b_block, dx4;                                       \
  vdup.u32 dx8, uv_dx8[0];                                                     \
                                                                               \
  vadd.u32 u_block, u_block, dx8;                                              \
  vdup.u32 dx8, uv_dx8[1];                                                     \
                                                                               \
  vadd.u32 v_block, v_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[0];                                                     \
                                                                               \
  vadd.u32 r_block, r_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[1];                                                     \
                                                                               \
  vadd.u32 g_block, g_block, dx8;                                              \
  vdup.u32 dx8, b_dx8;                                                         \
                                                                               \
  vadd.u32 b_block, b_block, dx8;                                              \
  vmovn.u16 u_whole_8, u_whole;                                                \
                                                                               \
  add fb_ptr, fb_ptr, #16;                                                     \
  vmovn.u16 v_whole_8, v_whole;                                                \
                                                                               \
  vst1.u32 { dither_offsets }, [block_ptr_b, :128], c_32;                      \
  vmovn.u16 b_whole_8, b_whole;                                                \
                                                                               \
  pld [fb_ptr];                                                                \
                                                                               \
  vmov.u32 fb_mask_ptrs[1], fb_ptr;                                            \
  subs span_num_blocks, span_num_blocks, #1;                                   \
                                                                               \
  vand.u8 uv_whole_8, uv_whole_8, texture_mask;                                \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  vmovn.u16 r_whole_8, r_whole;                                                \
  bne 4b;                                                                      \
                                                                               \
 5:                                                                            \
  vmovn.u16 g_whole_8, g_whole;                                                \
  ldrh right_mask, [span_edge_data, #edge_data_right_mask_offset];             \
                                                                               \
  vld1.u32 { test_mask }, [psx_gpu, :128];                                     \
  vdup.u8 draw_mask, right_mask;                                               \
                                                                               \
  ldr hacks_active, [psx_gpu, #psx_gpu_hacks_active_offset];                   \
  vmov.u32 fb_mask_ptrs[0], right_mask;                                        \
  vtst.u16 draw_mask, draw_mask, test_mask;                                    \
  vzip.u8 u_whole_8, v_whole_8;                                                \
                                                                               \
  vbic.u16 uv_whole_8, uv_whole_8, draw_mask;                                  \
  vst1.u32 { r_whole_8, g_whole_8 }, [block_ptr_b, :128], c_32;                \
  vst1.u32 { uv_whole_8 }, [block_ptr_a, :128], c_32;                          \
  vst1.u32 { dither_offsets }, [block_ptr_b, :128], c_32;                      \
  vst1.u32 { b_whole_8, fb_mask_ptrs }, [block_ptr_a, :128], c_32;             \
                                                                               \
  setup_blocks_uv_adj_hack_textured(hacks_active);                             \
                                                                               \
 1:                                                                            \
  add span_uvrg_offset, span_uvrg_offset, #16;                                 \
  add span_b_offset, span_b_offset, #4;                                        \
                                                                               \
  add span_edge_data, span_edge_data, #8;                                      \
  subs num_spans, num_spans, #1;                                               \
                                                                               \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  bne 0b;                                                                      \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4 - r11, pc };                                                 \
                                                                               \
 2:                                                                            \
  vstr texture_mask_u, [r0, #psx_gpu_saved_tmp_offset];                        \
  vstr texture_mask_v, [r0, #psx_gpu_saved_tmp_offset + 8];                    \
  /* pushing odd num of regs here realigns our unaligned stack */              \
  push  { r0 - r3, EXTRA_UNSAVED_REGS r12 }; /* r14=num_blocks */              \
  bl flush_render_block_buffer;                                                \
  pop   { r0 - r3, EXTRA_UNSAVED_REGS r12 };                                   \
  vldr texture_mask_u, [r0, #psx_gpu_saved_tmp_offset];                        \
  vldr texture_mask_v, [r0, #psx_gpu_saved_tmp_offset + 8];                    \
                                                                               \
  vadd.u32 uvrg_dx8, uvrg_dx4, uvrg_dx4;                                       \
  vmov.u8 fb_mask_ptrs, #0;                                                    \
                                                                               \
  mov num_blocks, span_num_blocks;                                             \
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset;                            \
  bal 3b                                                                       \


setup_blocks_shaded_textured_builder(swizzled)
setup_blocks_shaded_textured_builder(unswizzled)


#define setup_blocks_unshaded_textured_builder(swizzling)                      \
.align 3;                                                                      \
                                                                               \
function(setup_blocks_unshaded_textured_dithered_##swizzling##_indirect)       \
  ldrh num_spans, [psx_gpu, #psx_gpu_num_spans_offset];                        \
  add uvrg_dx_ptr, psx_gpu, #psx_gpu_uvrg_dx_offset;                           \
                                                                               \
  vld1.u32 { uvrg_dx }, [uvrg_dx_ptr, :128];                                   \
  add texture_mask_ptr, psx_gpu, #psx_gpu_texture_mask_width_offset;           \
                                                                               \
  cmp num_spans, #0;                                                           \
  bxeq lr;                                                                     \
                                                                               \
  stmdb sp!, { r4 - r11, r14 };                                                \
  save_abi_regs();                                                             \
  vshl.u32 uvrg_dx4, uvrg_dx, #2;                                              \
                                                                               \
  vshl.u32 uvrg_dx8, uvrg_dx, #3;                                              \
                                                                               \
  vld2.u8 { texture_mask_u[], texture_mask_v[] }, [texture_mask_ptr, :16];     \
  add span_uvrg_offset, psx_gpu, #psx_gpu_span_uvrg_offset_offset;             \
                                                                               \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset;                 \
                                                                               \
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset;                            \
                                                                               \
  add block_ptr_a, block_ptr_a, num_blocks, lsl #6;                            \
                                                                               \
 0:                                                                            \
  vmov.u8 fb_mask_ptrs, #0;                                                    \
                                                                               \
  ldrh span_num_blocks, [span_edge_data, #edge_data_num_blocks_offset];        \
  add dither_offset_ptr, psx_gpu, #psx_gpu_dither_table_offset;                \
                                                                               \
  ldrh y, [span_edge_data, #edge_data_y_offset];                               \
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset];                         \
                                                                               \
  cmp span_num_blocks, #0;                                                     \
  beq 1f;                                                                      \
                                                                               \
  ldrh left_x, [span_edge_data, #edge_data_left_x_offset];                     \
  add num_blocks, span_num_blocks, num_blocks;                                 \
                                                                               \
  cmp num_blocks, #MAX_BLOCKS;                                                 \
  bgt 2f;                                                                      \
                                                                               \
 3:                                                                            \
  add fb_ptr, fb_ptr, y, lsl #11;                                              \
                                                                               \
  vdup.u32 v_left_x, left_x;                                                   \
  and y, y, #0x3;                                                              \
                                                                               \
  ldr dither_row, [dither_offset_ptr, y, lsl #2];                              \
  add fb_ptr, fb_ptr, left_x, lsl #1;                                          \
                                                                               \
  and dither_shift, left_x, #0x03;                                             \
                                                                               \
  vld1.u32 { uvrg }, [span_uvrg_offset, :128];                                 \
  vshr.u32 uvrg_dx, uvrg_dx4, #2;                                              \
                                                                               \
  mov dither_shift, dither_shift, lsl #3;                                      \
  vmla.u32 uvrg, uvrg_dx, v_left_x;                                            \
                                                                               \
  mov c_32, #32;                                                               \
  subs span_num_blocks, span_num_blocks, #1;                                   \
                                                                               \
  mov dither_row, dither_row, ror dither_shift;                                \
                                                                               \
  vdup.u32 dither_offsets_short, dither_row;                                   \
  add block_span_ptr, psx_gpu, #psx_gpu_u_block_span_offset;                   \
                                                                               \
  vshll.s8 dither_offsets, dither_offsets_short, #4;                           \
                                                                               \
  vdup.u32 u_block, uv[0];                                                     \
                                                                               \
  vdup.u32 v_block, uv[1];                                                     \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 u_block, u_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 v_block, v_block, block_span;                                       \
  add block_ptr_b, block_ptr_a, #16;                                           \
                                                                               \
  vshrn.u32 u_whole_low, u_block, #16;                                         \
  vshrn.u32 v_whole_low, v_block, #16;                                         \
                                                                               \
  vdup.u32 dx4, uv_dx4[0];                                                     \
                                                                               \
  vaddhn.u32 u_whole_high, u_block, dx4;                                       \
  vdup.u32 dx4, uv_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 v_whole_high, v_block, dx4;                                       \
  vdup.u32 dx8, uv_dx8[0];                                                     \
                                                                               \
  vadd.u32 u_block, u_block, dx8;                                              \
  vdup.u32 dx8, uv_dx8[1];                                                     \
                                                                               \
  vadd.u32 v_block, v_block, dx8;                                              \
  vmovn.u16 u_whole_8, u_whole;                                                \
                                                                               \
  vmovn.u16 v_whole_8, v_whole;                                                \
                                                                               \
  pld [fb_ptr];                                                                \
  vmov.u32 fb_mask_ptrs[1], fb_ptr;                                            \
                                                                               \
  vand.u8 uv_whole_8, uv_whole_8, texture_mask;                                \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  beq 5f;                                                                      \
                                                                               \
 4:                                                                            \
  vshrn.u32 u_whole_low, u_block, #16;                                         \
                                                                               \
  vst2.u8 { u_whole_8, v_whole_8 }, [block_ptr_a, :128], c_32;                 \
  vshrn.u32 v_whole_low, v_block, #16;                                         \
                                                                               \
  add block_ptr_b, block_ptr_b, #32;                                           \
  vst1.u32 { b_whole_8, fb_mask_ptrs }, [block_ptr_a, :128], c_32;             \
                                                                               \
  vdup.u32 dx4, uv_dx4[0];                                                     \
  vaddhn.u32 u_whole_high, u_block, dx4;                                       \
  vdup.u32 dx4, uv_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 v_whole_high, v_block, dx4;                                       \
  vdup.u32 dx8, uv_dx8[0];                                                     \
                                                                               \
  vadd.u32 u_block, u_block, dx8;                                              \
  vdup.u32 dx8, uv_dx8[1];                                                     \
                                                                               \
  vadd.u32 v_block, v_block, dx8;                                              \
  vmovn.u16 u_whole_8, u_whole;                                                \
                                                                               \
  add fb_ptr, fb_ptr, #16;                                                     \
  vmovn.u16 v_whole_8, v_whole;                                                \
                                                                               \
  vst1.u32 { dither_offsets }, [block_ptr_b, :128], c_32;                      \
  pld [fb_ptr];                                                                \
                                                                               \
  vmov.u32 fb_mask_ptrs[1], fb_ptr;                                            \
  subs span_num_blocks, span_num_blocks, #1;                                   \
                                                                               \
  vand.u8 uv_whole_8, uv_whole_8, texture_mask;                                \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  bne 4b;                                                                      \
                                                                               \
 5:                                                                            \
  ldrh right_mask, [span_edge_data, #edge_data_right_mask_offset];             \
                                                                               \
  vld1.u32 { test_mask }, [psx_gpu, :128];                                     \
  vdup.u8 draw_mask, right_mask;                                               \
                                                                               \
  ldr hacks_active, [psx_gpu, #psx_gpu_hacks_active_offset];                   \
  vmov.u32 fb_mask_ptrs[0], right_mask;                                        \
  vtst.u16 draw_mask, draw_mask, test_mask;                                    \
  vzip.u8 u_whole_8, v_whole_8;                                                \
                                                                               \
  vbic.u16 uv_whole_8, uv_whole_8, draw_mask;                                  \
  add block_ptr_b, block_ptr_b, #32;                                           \
  vst1.u32 { uv_whole_8 }, [block_ptr_a, :128], c_32;                          \
  vst1.u32 { dither_offsets }, [block_ptr_b, :128], c_32;                      \
  vst1.u32 { b_whole_8, fb_mask_ptrs }, [block_ptr_a, :128], c_32;             \
                                                                               \
  setup_blocks_uv_adj_hack_textured(hacks_active);                             \
                                                                               \
 1:                                                                            \
  add span_uvrg_offset, span_uvrg_offset, #16;                                 \
  add span_edge_data, span_edge_data, #8;                                      \
  subs num_spans, num_spans, #1;                                               \
                                                                               \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  bne 0b;                                                                      \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4 - r11, pc };                                                 \
                                                                               \
 2:                                                                            \
  vstr texture_mask_u, [r0, #psx_gpu_saved_tmp_offset];                        \
  vstr texture_mask_v, [r0, #psx_gpu_saved_tmp_offset + 8];                    \
  push  { r0 - r3, EXTRA_UNSAVED_REGS r12 }; /* r14=num_blocks */              \
  bl flush_render_block_buffer;                                                \
  pop   { r0 - r3, EXTRA_UNSAVED_REGS r12 };                                   \
  vldr texture_mask_u, [r0, #psx_gpu_saved_tmp_offset];                        \
  vldr texture_mask_v, [r0, #psx_gpu_saved_tmp_offset + 8];                    \
                                                                               \
  vadd.u32 uvrg_dx8, uvrg_dx4, uvrg_dx4;                                       \
  vmov.u8 fb_mask_ptrs, #0;                                                    \
                                                                               \
  mov num_blocks, span_num_blocks;                                             \
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset;                            \
  bal 3b                                                                       \


setup_blocks_unshaded_textured_builder(swizzled)
setup_blocks_unshaded_textured_builder(unswizzled)


.align 3

function(setup_blocks_unshaded_untextured_undithered_unswizzled_indirect)
  ldrh num_spans, [psx_gpu, #psx_gpu_num_spans_offset]
  veor.u32 draw_mask, draw_mask, draw_mask

  cmp num_spans, #0
  bxeq lr

  stmdb sp!, { r4 - r11, r14 }
  save_abi_regs()
  vld1.u32 { test_mask }, [psx_gpu, :128]

  ldr color, [psx_gpu, #psx_gpu_triangle_color_offset]

  ubfx color_r, color, #3, #5
  ubfx color_g, color, #11, #5
  ubfx color_b, color, #19, #5

  orr color, color_r, color_b, lsl #10
  orr color, color, color_g, lsl #5

  vdup.u16 colors, color

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset

  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset
  add block_ptr_a, block_ptr_a, num_blocks, lsl #6

 0:
  ldrh span_num_blocks, [span_edge_data, #edge_data_num_blocks_offset]
  ldrh y, [span_edge_data, #edge_data_y_offset]

  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset]

  cmp span_num_blocks, #0
  beq 1f

  ldrh left_x, [span_edge_data, #edge_data_left_x_offset]
  add num_blocks, span_num_blocks, num_blocks

  cmp num_blocks, #MAX_BLOCKS
  bgt 2f

 3:
  add fb_ptr, fb_ptr, y, lsl #11
  and y, y, #0x3

  add fb_ptr, fb_ptr, left_x, lsl #1
  mov c_32, #32

  subs span_num_blocks, span_num_blocks, #1

  add block_ptr_b, block_ptr_a, #16
  pld [fb_ptr]

  vmov.u32 fb_mask_ptrs[1], fb_ptr
  beq 5f

 4:
  vst1.u32 { draw_mask }, [block_ptr_a, :128], c_32
  vst1.u32 { colors }, [block_ptr_b, :128], c_32
  vst1.u32 { b_whole_8, fb_mask_ptrs }, [block_ptr_a, :128], c_32

  add fb_ptr, fb_ptr, #16
  add block_ptr_b, block_ptr_b, #32

  pld [fb_ptr]

  vmov.u32 fb_mask_ptrs[1], fb_ptr
  subs span_num_blocks, span_num_blocks, #1

  bne 4b

 5:
  ldrh right_mask, [span_edge_data, #edge_data_right_mask_offset]

  vdup.u8 draw_mask_edge, right_mask
  vtst.u16 draw_mask_edge, draw_mask_edge, test_mask

  vst1.u32 { colors }, [block_ptr_b, :128], c_32
  vst1.u32 { draw_mask_edge }, [block_ptr_a, :128], c_32
  add block_ptr_b, block_ptr_b, #32
  vst1.u32 { b_whole_8, fb_mask_ptrs }, [block_ptr_a, :128], c_32

 1:
  add span_edge_data, span_edge_data, #8
  subs num_spans, num_spans, #1

  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  bne 0b

  restore_abi_regs()
  ldmia sp!, { r4 - r11, pc }
                                                                           
 2:
  vstr d4, [r0, #psx_gpu_saved_tmp_offset]       /* colors */
  vstr d5, [r0, #psx_gpu_saved_tmp_offset + 8]
  push { r0 - r3, EXTRA_UNSAVED_REGS r12 }
  bl flush_render_block_buffer
  pop  { r0 - r3, EXTRA_UNSAVED_REGS r12 }
  vldr d4, [r0, #psx_gpu_saved_tmp_offset]
  vldr d5, [r0, #psx_gpu_saved_tmp_offset + 8]

  vld1.u32 { test_mask }, [psx_gpu, :128]
  veor.u32 draw_mask, draw_mask, draw_mask

  mov num_blocks, span_num_blocks
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset
  bal 3b


#define mask_msb_scalar                                   r14

#define msb_mask                                          q15

#define pixels_low                                        d16

#define msb_mask_low                                      d30
#define msb_mask_high                                     d31


.align 3

function(setup_blocks_unshaded_untextured_undithered_unswizzled_direct)
  ldrh num_spans, [psx_gpu, #psx_gpu_num_spans_offset]

  cmp num_spans, #0
  bxeq lr

  stmdb sp!, { r4 - r11, r14 }

  ldr color, [psx_gpu, #psx_gpu_triangle_color_offset]

  ubfx color_r, color, #3, #5
  ubfx color_g, color, #11, #5

  ldrh mask_msb_scalar, [psx_gpu, #psx_gpu_mask_msb_offset]
  ubfx color_b, color, #19, #5

  orr color, color_r, color_b, lsl #10
  orr color, color, color_g, lsl #5
  orr color, color, mask_msb_scalar

  vdup.u16 colors, color

  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset
  orr color, color, color, lsl #16


 0:
  ldrh span_num_blocks, [span_edge_data, #edge_data_num_blocks_offset]
  ldrh y, [span_edge_data, #edge_data_y_offset]

  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset]

  cmp span_num_blocks, #0
  beq 1f

  ldrh left_x, [span_edge_data, #edge_data_left_x_offset]

  add fb_ptr, fb_ptr, y, lsl #11
  subs span_num_blocks, span_num_blocks, #1

  add fb_ptr, fb_ptr, left_x, lsl #1
  beq 3f

 2:
  vst1.u32 { colors }, [fb_ptr]!
  subs span_num_blocks, span_num_blocks, #1

  bne 2b

 3:
  ldrb right_mask, [span_edge_data, #edge_data_right_mask_offset]

  cmp right_mask, #0x0
  beq 5f

  tst right_mask, #0xF
  streq color, [fb_ptr], #4
  moveq right_mask, right_mask, lsr #4
  streq color, [fb_ptr], #4

  tst right_mask, #0x3
  streq color, [fb_ptr], #4
  moveq right_mask, right_mask, lsr #2

  tst right_mask, #0x1
  strheq color, [fb_ptr]

 1:
  add span_edge_data, span_edge_data, #8
  subs num_spans, num_spans, #1
  bne 0b

  ldmia sp!, { r4 - r11, pc }
                                                                           
 5:
  vst1.u32 { colors }, [fb_ptr]
  bal 1b


#undef c_64

#define c_64                                              r7
#define rg_dx_ptr                                         r2


#undef r_block
#undef g_block
#undef b_block
#undef r_whole
#undef g_whole
#undef b_whole
#undef r_whole_low
#undef r_whole_high
#undef g_whole_low
#undef g_whole_high
#undef b_whole_low
#undef b_whole_high
#undef r_whole_8
#undef g_whole_8
#undef b_whole_8
#undef dither_offsets
#undef rg_dx4
#undef rg_dx8
#undef dx4
#undef dx8
#undef v_left_x
#undef uvrg
#undef block_span
#undef rg
#undef draw_mask
#undef test_mask

#define r_block                                           q0
#define g_block                                           q1
#define b_block                                           q2

#define r_whole                                           q3
#define g_whole                                           q4
#define b_whole                                           q5

#define r_whole_low                                       d6
#define r_whole_high                                      d7
#define g_whole_low                                       d8
#define g_whole_high                                      d9
#define b_whole_low                                       d10
#define b_whole_high                                      d11

#define gb_whole_8                                        q6

#define g_whole_8                                         d12
#define b_whole_8                                         d13

#define r_whole_8                                         d14

#define pixels                                            q8

#define rg_dx4                                            d18
#define rg_dx8                                            d19

#define dx4                                               q10
#define dx8                                               q10

#define v_left_x                                          d6
#define uvrg                                              q4
#define block_span                                        q5

#define rg                                                d9

#define d64_1                                             d22
#define d64_128                                           d23

#define d128_4                                            q12
#define d128_0x7                                          q13

#define d64_4                                             d24

#define dither_offsets                                    q14
#define draw_mask                                         q15

#define dither_offsets_low                                d28

#define rg_dx                                             d0
#define test_mask                                         q10


#define setup_blocks_shaded_untextured_dither_a_dithered()                     \
  vqadd.u8 r_whole_8, r_whole_8, dither_offsets_low;                           \
  vqadd.u8 gb_whole_8, gb_whole_8, dither_offsets;                             \

#define setup_blocks_shaded_untextured_dither_b_dithered()                     \
  vqsub.u8 r_whole_8, r_whole_8, d64_4;                                        \
  vqsub.u8 gb_whole_8, gb_whole_8, d128_4                                      \

#define setup_blocks_shaded_untextured_dither_a_undithered()                   \

#define setup_blocks_shaded_untextured_dither_b_undithered()                   \


#define setup_blocks_shaded_untextured_indirect_builder(dithering)             \
.align 3;                                                                      \
                                                                               \
function(setup_blocks_shaded_untextured_##dithering##_unswizzled_indirect)     \
  ldrh num_spans, [psx_gpu, #psx_gpu_num_spans_offset];                        \
  add rg_dx_ptr, psx_gpu, #(psx_gpu_uvrg_dx_offset + 8);                       \
                                                                               \
  vld1.u32 { rg_dx }, [rg_dx_ptr, :64];                                        \
                                                                               \
  cmp num_spans, #0;                                                           \
  bxeq lr;                                                                     \
                                                                               \
  stmdb sp!, { r4 - r11, r14 };                                                \
  save_abi_regs();                                                             \
  vshl.u32 rg_dx4, rg_dx, #2;                                                  \
                                                                               \
  ldr b_dx, [psx_gpu, #psx_gpu_b_dx_offset];                                   \
  vshl.u32 rg_dx8, rg_dx, #3;                                                  \
                                                                               \
  add span_uvrg_offset, psx_gpu, #psx_gpu_span_uvrg_offset_offset;             \
                                                                               \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset;                 \
                                                                               \
  add span_b_offset, psx_gpu, #psx_gpu_span_b_offset_offset;                   \
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset;                            \
                                                                               \
  add block_ptr_a, block_ptr_a, num_blocks, lsl #6;                            \
  vmov.u8 d64_1, #1;                                                           \
                                                                               \
  vmov.u8 d128_4, #4;                                                          \
  vmov.u8 d64_128, #128;                                                       \
                                                                               \
  vmov.u8 d128_0x7, #0x7;                                                      \
                                                                               \
 0:                                                                            \
  ldrh span_num_blocks, [span_edge_data, #edge_data_num_blocks_offset];        \
  add dither_offset_ptr, psx_gpu, #psx_gpu_dither_table_offset;                \
                                                                               \
  ldrh y, [span_edge_data, #edge_data_y_offset];                               \
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset];                         \
                                                                               \
  cmp span_num_blocks, #0;                                                     \
  beq 1f;                                                                      \
                                                                               \
  ldrh left_x, [span_edge_data, #edge_data_left_x_offset];                     \
  add num_blocks, span_num_blocks, num_blocks;                                 \
                                                                               \
  cmp num_blocks, #MAX_BLOCKS;                                                 \
  bgt 2f;                                                                      \
                                                                               \
 3:                                                                            \
  ldr b, [span_b_offset];                                                      \
  add fb_ptr, fb_ptr, y, lsl #11;                                              \
                                                                               \
  vdup.u32 v_left_x, left_x;                                                   \
  and y, y, #0x3;                                                              \
                                                                               \
  ldr dither_row, [dither_offset_ptr, y, lsl #2];                              \
  add fb_ptr, fb_ptr, left_x, lsl #1;                                          \
                                                                               \
  mla b, b_dx, left_x, b;                                                      \
  and dither_shift, left_x, #0x03;                                             \
                                                                               \
  vld1.u32 { uvrg }, [span_uvrg_offset, :128];                                 \
  vshr.u32 rg_dx, rg_dx4, #2;                                                  \
                                                                               \
  mov dither_shift, dither_shift, lsl #3;                                      \
  vmla.u32 rg, rg_dx, v_left_x;                                                \
                                                                               \
  mov c_64, #64;                                                               \
  subs span_num_blocks, span_num_blocks, #1;                                   \
                                                                               \
  mov dither_row, dither_row, ror dither_shift;                                \
  mov b_dx4, b_dx, lsl #2;                                                     \
                                                                               \
  vdup.u32 dither_offsets, dither_row;                                         \
  add block_span_ptr, psx_gpu, #psx_gpu_r_block_span_offset;                   \
                                                                               \
  vdup.u32 b_block, b;                                                         \
  vadd.u8 dither_offsets, dither_offsets, d128_4;                              \
                                                                               \
  mov b_dx8, b_dx, lsl #3;                                                     \
  vdup.u32 r_block, rg[0];                                                     \
  vdup.u32 g_block, rg[1];                                                     \
                                                                               \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 r_block, r_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 g_block, g_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128];                             \
                                                                               \
  vadd.u32 b_block, b_block, block_span;                                       \
  add block_ptr_b, block_ptr_a, #16;                                           \
                                                                               \
  vshrn.u32 r_whole_low, r_block, #16;                                         \
  vshrn.u32 g_whole_low, g_block, #16;                                         \
  vshrn.u32 b_whole_low, b_block, #16;                                         \
  vdup.u32 dx4, rg_dx4[0];                                                     \
                                                                               \
  vaddhn.u32 r_whole_high, r_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 g_whole_high, g_block, dx4;                                       \
  vdup.u32 dx4, b_dx4;                                                         \
                                                                               \
  vaddhn.u32 b_whole_high, b_block, dx4;                                       \
  vdup.u32 dx8, rg_dx8[0];                                                     \
                                                                               \
  vadd.u32 r_block, r_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[1];                                                     \
                                                                               \
  vadd.u32 g_block, g_block, dx8;                                              \
  vdup.u32 dx8, b_dx8;                                                         \
                                                                               \
  vadd.u32 b_block, b_block, dx8;                                              \
                                                                               \
  vmovn.u16 r_whole_8, r_whole;                                                \
  vmovn.u16 g_whole_8, g_whole;                                                \
  vmovn.u16 b_whole_8, b_whole;                                                \
                                                                               \
  beq 5f;                                                                      \
  veor.u32 draw_mask, draw_mask, draw_mask;                                    \
                                                                               \
 4:                                                                            \
  setup_blocks_shaded_untextured_dither_a_##dithering();                       \
  vshrn.u32 r_whole_low, r_block, #16;                                         \
                                                                               \
  setup_blocks_shaded_untextured_dither_b_##dithering();                       \
  vshrn.u32 g_whole_low, g_block, #16;                                         \
                                                                               \
  vshrn.u32 b_whole_low, b_block, #16;                                         \
  str fb_ptr, [block_ptr_a, #44];                                              \
                                                                               \
  vdup.u32 dx4, rg_dx4[0];                                                     \
  vshr.u8 r_whole_8, r_whole_8, #3;                                            \
  vbic.u8 gb_whole_8, gb_whole_8, d128_0x7;                                    \
                                                                               \
  vaddhn.u32 r_whole_high, r_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 g_whole_high, g_block, dx4;                                       \
  vdup.u32 dx4, b_dx4;                                                         \
                                                                               \
  vaddhn.u32 b_whole_high, b_block, dx4;                                       \
  vdup.u32 dx8, rg_dx8[0];                                                     \
                                                                               \
  vmull.u8 pixels, r_whole_8, d64_1;                                           \
  vmlal.u8 pixels, g_whole_8, d64_4;                                           \
  vmlal.u8 pixels, b_whole_8, d64_128;                                         \
                                                                               \
  vadd.u32 r_block, r_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[1];                                                     \
                                                                               \
  vadd.u32 g_block, g_block, dx8;                                              \
  vdup.u32 dx8, b_dx8;                                                         \
                                                                               \
  vadd.u32 b_block, b_block, dx8;                                              \
  add fb_ptr, fb_ptr, #16;                                                     \
                                                                               \
  vmovn.u16 r_whole_8, r_whole;                                                \
  vmovn.u16 g_whole_8, g_whole;                                                \
  vmovn.u16 b_whole_8, b_whole;                                                \
                                                                               \
  vst1.u32 { draw_mask }, [block_ptr_a, :128], c_64;                           \
  vst1.u32 { pixels }, [block_ptr_b, :128], c_64;                              \
                                                                               \
  pld [fb_ptr];                                                                \
                                                                               \
  subs span_num_blocks, span_num_blocks, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
 5:                                                                            \
  str fb_ptr, [block_ptr_a, #44];                                              \
  setup_blocks_shaded_untextured_dither_a_##dithering();                       \
                                                                               \
  ldrh right_mask, [span_edge_data, #edge_data_right_mask_offset];             \
  setup_blocks_shaded_untextured_dither_b_##dithering();                       \
                                                                               \
  vshr.u8 r_whole_8, r_whole_8, #3;                                            \
  vdup.u8 draw_mask, right_mask;                                               \
                                                                               \
  vbic.u8 gb_whole_8, gb_whole_8, d128_0x7;                                    \
  vld1.u32 { test_mask }, [psx_gpu, :128];                                     \
                                                                               \
  vtst.u16 draw_mask, draw_mask, test_mask;                                    \
                                                                               \
  vmull.u8 pixels, r_whole_8, d64_1;                                           \
  vmlal.u8 pixels, g_whole_8, d64_4;                                           \
  vmlal.u8 pixels, b_whole_8, d64_128;                                         \
                                                                               \
  vst1.u32 { draw_mask }, [block_ptr_a, :128], c_64;                           \
  vst1.u32 { pixels }, [block_ptr_b, :128], c_64;                              \
                                                                               \
 1:                                                                            \
  add span_uvrg_offset, span_uvrg_offset, #16;                                 \
  add span_b_offset, span_b_offset, #4;                                        \
                                                                               \
  add span_edge_data, span_edge_data, #8;                                      \
  subs num_spans, num_spans, #1;                                               \
                                                                               \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  bne 0b;                                                                      \
                                                                               \
  restore_abi_regs();                                                          \
  pop   { r4 - r11, pc };                                                      \
                                                                               \
 2:                                                                            \
  vstr rg_dx4, [r0, #psx_gpu_saved_tmp_offset];                                \
  push  { r0 - r3, EXTRA_UNSAVED_REGS r12 };                                   \
  bl flush_render_block_buffer;                                                \
  pop   { r0 - r3, EXTRA_UNSAVED_REGS r12 };                                   \
  vldr rg_dx4, [r0, #psx_gpu_saved_tmp_offset];                                \
                                                                               \
  vmov.u8 d64_1, #1;                                                           \
  vmov.u8 d128_4, #4;                                                          \
  vmov.u8 d64_128, #128;                                                       \
  vmov.u8 d128_0x7, #0x7;                                                      \
                                                                               \
  vadd.u32 rg_dx8, rg_dx4, rg_dx4;                                             \
                                                                               \
  mov num_blocks, span_num_blocks;                                             \
  add block_ptr_a, psx_gpu, #psx_gpu_blocks_offset;                            \
  bal 3b                                                                       \


setup_blocks_shaded_untextured_indirect_builder(undithered)
setup_blocks_shaded_untextured_indirect_builder(dithered)


#undef draw_mask

#define mask_msb_ptr                                      r14

#define draw_mask                                         q0
#define pixels_low                                        d16
#define pixels_high                                       d17



#define setup_blocks_shaded_untextured_direct_builder(dithering)               \
.align 3;                                                                      \
                                                                               \
function(setup_blocks_shaded_untextured_##dithering##_unswizzled_direct)       \
  ldrh num_spans, [psx_gpu, #psx_gpu_num_spans_offset];                        \
  add rg_dx_ptr, psx_gpu, #(psx_gpu_uvrg_dx_offset + 8);                       \
                                                                               \
  vld1.u32 { rg_dx }, [rg_dx_ptr, :64];                                        \
                                                                               \
  cmp num_spans, #0;                                                           \
  bxeq lr;                                                                     \
                                                                               \
  stmdb sp!, { r4 - r11, r14 };                                                \
  save_abi_regs();                                                             \
  vshl.u32 rg_dx4, rg_dx, #2;                                                  \
                                                                               \
  ldr b_dx, [psx_gpu, #psx_gpu_b_dx_offset];                                   \
  vshl.u32 rg_dx8, rg_dx, #3;                                                  \
                                                                               \
  add span_uvrg_offset, psx_gpu, #psx_gpu_span_uvrg_offset_offset;             \
  add span_edge_data, psx_gpu, #psx_gpu_span_edge_data_offset;                 \
                                                                               \
  add span_b_offset, psx_gpu, #psx_gpu_span_b_offset_offset;                   \
  vmov.u8 d64_1, #1;                                                           \
                                                                               \
  vmov.u8 d128_4, #4;                                                          \
  vmov.u8 d64_128, #128;                                                       \
                                                                               \
  vmov.u8 d128_0x7, #0x7;                                                      \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
 0:                                                                            \
  ldrh span_num_blocks, [span_edge_data, #edge_data_num_blocks_offset];        \
  add dither_offset_ptr, psx_gpu, #psx_gpu_dither_table_offset;                \
                                                                               \
  ldrh y, [span_edge_data, #edge_data_y_offset];                               \
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset];                         \
                                                                               \
  cmp span_num_blocks, #0;                                                     \
  beq 1f;                                                                      \
                                                                               \
  ldrh left_x, [span_edge_data, #edge_data_left_x_offset];                     \
  add fb_ptr, fb_ptr, y, lsl #11;                                              \
                                                                               \
  ldr b, [span_b_offset];                                                      \
  vdup.u32 v_left_x, left_x;                                                   \
  and y, y, #0x3;                                                              \
                                                                               \
  ldr dither_row, [dither_offset_ptr, y, lsl #2];                              \
  add fb_ptr, fb_ptr, left_x, lsl #1;                                          \
                                                                               \
  mla b, b_dx, left_x, b;                                                      \
  and dither_shift, left_x, #0x03;                                             \
                                                                               \
  vld1.u32 { uvrg }, [span_uvrg_offset, :128];                                 \
  vshr.u32 rg_dx, rg_dx4, #2;                                                  \
                                                                               \
  mov dither_shift, dither_shift, lsl #3;                                      \
  vmla.u32 rg, rg_dx, v_left_x;                                                \
                                                                               \
  subs span_num_blocks, span_num_blocks, #1;                                   \
                                                                               \
  mov dither_row, dither_row, ror dither_shift;                                \
  mov b_dx4, b_dx, lsl #2;                                                     \
                                                                               \
  vdup.u32 dither_offsets, dither_row;                                         \
  add block_span_ptr, psx_gpu, #psx_gpu_r_block_span_offset;                   \
                                                                               \
  vdup.u32 b_block, b;                                                         \
  vadd.u8 dither_offsets, dither_offsets, d128_4;                              \
                                                                               \
  mov b_dx8, b_dx, lsl #3;                                                     \
  vdup.u32 r_block, rg[0];                                                     \
  vdup.u32 g_block, rg[1];                                                     \
                                                                               \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 r_block, r_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128]!;                            \
                                                                               \
  vadd.u32 g_block, g_block, block_span;                                       \
  vld1.u32 { block_span }, [block_span_ptr, :128];                             \
                                                                               \
  vadd.u32 b_block, b_block, block_span;                                       \
  add block_ptr_b, block_ptr_a, #16;                                           \
                                                                               \
  vshrn.u32 r_whole_low, r_block, #16;                                         \
  vshrn.u32 g_whole_low, g_block, #16;                                         \
  vshrn.u32 b_whole_low, b_block, #16;                                         \
  vdup.u32 dx4, rg_dx4[0];                                                     \
                                                                               \
  vaddhn.u32 r_whole_high, r_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[1];                                                     \
                                                                               \
  vaddhn.u32 g_whole_high, g_block, dx4;                                       \
  vdup.u32 dx4, b_dx4;                                                         \
                                                                               \
  vaddhn.u32 b_whole_high, b_block, dx4;                                       \
  vdup.u32 dx8, rg_dx8[0];                                                     \
                                                                               \
  vadd.u32 r_block, r_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[1];                                                     \
                                                                               \
  vadd.u32 g_block, g_block, dx8;                                              \
  vdup.u32 dx8, b_dx8;                                                         \
                                                                               \
  vadd.u32 b_block, b_block, dx8;                                              \
                                                                               \
  vmovn.u16 r_whole_8, r_whole;                                                \
  vmovn.u16 g_whole_8, g_whole;                                                \
  vmovn.u16 b_whole_8, b_whole;                                                \
                                                                               \
  beq 3f;                                                                      \
                                                                               \
 2:                                                                            \
  setup_blocks_shaded_untextured_dither_a_##dithering();                       \
  vshrn.u32 r_whole_low, r_block, #16;                                         \
                                                                               \
  setup_blocks_shaded_untextured_dither_b_##dithering();                       \
  vshrn.u32 g_whole_low, g_block, #16;                                         \
                                                                               \
  vshrn.u32 b_whole_low, b_block, #16;                                         \
                                                                               \
  vdup.u32 dx4, rg_dx4[0];                                                     \
  vshr.u8 r_whole_8, r_whole_8, #3;                                            \
  vbic.u8 gb_whole_8, gb_whole_8, d128_0x7;                                    \
                                                                               \
  vaddhn.u32 r_whole_high, r_block, dx4;                                       \
  vdup.u32 dx4, rg_dx4[1];                                                     \
                                                                               \
  vmov pixels, msb_mask;                                                       \
  vaddhn.u32 g_whole_high, g_block, dx4;                                       \
  vdup.u32 dx4, b_dx4;                                                         \
                                                                               \
  vaddhn.u32 b_whole_high, b_block, dx4;                                       \
  vdup.u32 dx8, rg_dx8[0];                                                     \
                                                                               \
  vmlal.u8 pixels, r_whole_8, d64_1;                                           \
  vmlal.u8 pixels, g_whole_8, d64_4;                                           \
  vmlal.u8 pixels, b_whole_8, d64_128;                                         \
                                                                               \
  vadd.u32 r_block, r_block, dx8;                                              \
  vdup.u32 dx8, rg_dx8[1];                                                     \
                                                                               \
  vadd.u32 g_block, g_block, dx8;                                              \
  vdup.u32 dx8, b_dx8;                                                         \
                                                                               \
  vadd.u32 b_block, b_block, dx8;                                              \
                                                                               \
  vmovn.u16 r_whole_8, r_whole;                                                \
  vmovn.u16 g_whole_8, g_whole;                                                \
  vmovn.u16 b_whole_8, b_whole;                                                \
                                                                               \
  vst1.u32 { pixels }, [fb_ptr]!;                                              \
  subs span_num_blocks, span_num_blocks, #1;                                   \
  bne 2b;                                                                      \
                                                                               \
 3:                                                                            \
  setup_blocks_shaded_untextured_dither_a_##dithering();                       \
                                                                               \
  ldrh right_mask, [span_edge_data, #edge_data_right_mask_offset];             \
  setup_blocks_shaded_untextured_dither_b_##dithering();                       \
                                                                               \
  vshr.u8 r_whole_8, r_whole_8, #3;                                            \
  rbit right_mask, right_mask;                                                 \
  vmov pixels, msb_mask;                                                       \
  vbic.u8 gb_whole_8, gb_whole_8, d128_0x7;                                    \
  clz right_mask, right_mask;                                                  \
                                                                               \
  vmlal.u8 pixels, r_whole_8, d64_1;                                           \
  vmlal.u8 pixels, g_whole_8, d64_4;                                           \
  vmlal.u8 pixels, b_whole_8, d64_128;                                         \
                                                                               \
  JT_OP_REL(100f, right_mask, temp);                                           \
  JT_OP(ldr pc, [pc, right_mask, lsl #2]);                                     \
  nop;                                                                         \
 100:                                                                          \
  nop;                                                                         \
  .word JTE(100b, 4f);                                                         \
  .word JTE(100b, 5f);                                                         \
  .word JTE(100b, 6f);                                                         \
  .word JTE(100b, 7f);                                                         \
  .word JTE(100b, 8f);                                                         \
  .word JTE(100b, 9f);                                                         \
  .word JTE(100b, 10f);                                                        \
  .word JTE(100b, 11f);                                                        \
                                                                               \
 4:                                                                            \
  vst1.u16 { pixels_low[0] }, [fb_ptr];                                        \
  bal 1f;                                                                      \
                                                                               \
 5:                                                                            \
  vst1.u32 { pixels_low[0] }, [fb_ptr];                                        \
  bal 1f;                                                                      \
                                                                               \
 6:                                                                            \
  vst1.u32 { pixels_low[0] }, [fb_ptr]!;                                       \
  vst1.u16 { pixels_low[2] }, [fb_ptr];                                        \
  bal 1f;                                                                      \
                                                                               \
 7:                                                                            \
  vst1.u32 { pixels_low }, [fb_ptr];                                           \
  bal 1f;                                                                      \
                                                                               \
 8:                                                                            \
  vst1.u32 { pixels_low }, [fb_ptr]!;                                          \
  vst1.u16 { pixels_high[0] }, [fb_ptr];                                       \
  bal 1f;                                                                      \
                                                                               \
 9:                                                                            \
  vst1.u32 { pixels_low }, [fb_ptr]!;                                          \
  vst1.u32 { pixels_high[0] }, [fb_ptr]!;                                      \
  bal 1f;                                                                      \
                                                                               \
 10:                                                                           \
  vst1.u32 { pixels_low }, [fb_ptr]!;                                          \
  vst1.u32 { pixels_high[0] }, [fb_ptr]!;                                      \
  vst1.u16 { pixels_high[2] }, [fb_ptr];                                       \
  bal 1f;                                                                      \
                                                                               \
 11:                                                                           \
  vst1.u32 { pixels }, [fb_ptr];                                               \
  bal 1f;                                                                      \
                                                                               \
 1:                                                                            \
  add span_uvrg_offset, span_uvrg_offset, #16;                                 \
  add span_b_offset, span_b_offset, #4;                                        \
                                                                               \
  add span_edge_data, span_edge_data, #8;                                      \
  subs num_spans, num_spans, #1;                                               \
                                                                               \
  bne 0b;                                                                      \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4 - r11, pc }                                                  \

setup_blocks_shaded_untextured_direct_builder(undithered)
setup_blocks_shaded_untextured_direct_builder(dithered)


#undef psx_gpu
#undef num_blocks
#undef triangle
#undef c_64

#define psx_gpu                                  r0
#define block_ptr                                r1
#define num_blocks                               r2
#define uv_01                                    r3
#define uv_23                                    r4
#define uv_45                                    r5
#define uv_67                                    r6
#define uv_0                                     r7
#define uv_1                                     r3
#define uv_2                                     r8
#define uv_3                                     r4
#define uv_4                                     r9
#define uv_5                                     r5
#define uv_6                                     r10
#define uv_7                                     r6
#define texture_ptr                              r11

#define pixel_0                                  r7
#define pixel_1                                  r3
#define pixel_2                                  r8
#define pixel_3                                  r4
#define pixel_4                                  r9
#define pixel_5                                  r5
#define pixel_6                                  r10
#define pixel_7                                  r6

#define pixels_a                                 r7
#define pixels_b                                 r9
#define pixels_c                                 r8
#define pixels_d                                 r10

#define c_64                                     r0

#define clut_ptr                                 r12
#define current_texture_mask                     r5
#define dirty_textures_mask                      r6

#define texels                                   d0

#define clut_low_a                               d2
#define clut_low_b                               d3
#define clut_high_a                              d4
#define clut_high_b                              d5

#define clut_a                                   q1
#define clut_b                                   q2

#define texels_low                               d6
#define texels_high                              d7

.align 3

function(texture_blocks_untextured)
  bx lr


.align 3

function(texture_blocks_4bpp)
  stmdb sp!, { r3 - r11, r14 }
  add block_ptr, psx_gpu, #psx_gpu_blocks_offset

  ldr texture_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset]
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]

  ldr clut_ptr, [psx_gpu, #psx_gpu_clut_ptr_offset]
  vld1.u32 { clut_a, clut_b }, [clut_ptr, :128]

  ldr current_texture_mask, [psx_gpu, #psx_gpu_current_texture_mask_offset]
  vuzp.u8 clut_a, clut_b

  ldr dirty_textures_mask, [psx_gpu, #psx_gpu_dirty_textures_4bpp_mask_offset]
  tst dirty_textures_mask, current_texture_mask

  bne 1f
  mov c_64, #64

0:
  ldm block_ptr, { uv_01, uv_23, uv_45, uv_67 }

  uxtah uv_0, texture_ptr, uv_01
  uxtah uv_1, texture_ptr, uv_01, ror #16

  uxtah uv_2, texture_ptr, uv_23
  uxtah uv_3, texture_ptr, uv_23, ror #16

  uxtah uv_4, texture_ptr, uv_45
  ldrb pixel_0, [uv_0]

  uxtah uv_5, texture_ptr, uv_45, ror #16
  ldrb pixel_1, [uv_1]

  uxtah uv_6, texture_ptr, uv_67
  ldrb pixel_2, [uv_2]

  uxtah uv_7, texture_ptr, uv_67, ror #16
  ldrb pixel_3, [uv_3]

  ldrb pixel_4, [uv_4]
  subs num_blocks, num_blocks, #1

  ldrb pixel_5, [uv_5]
  orr pixels_a, pixel_0, pixel_1, lsl #8

  ldrb pixel_6, [uv_6]
  orr pixels_b, pixel_4, pixel_5, lsl #8

  ldrb pixel_7, [uv_7]
  orr pixels_a, pixels_a, pixel_2, lsl #16

  orr pixels_b, pixels_b, pixel_6, lsl #16
  orr pixels_a, pixels_a, pixel_3, lsl #24

  orr pixels_b, pixels_b, pixel_7, lsl #24
  vmov texels, pixels_a, pixels_b

  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels

  vst2.u8 { texels_low, texels_high }, [block_ptr, :128], c_64
  bne 0b

  ldmia sp!, { r3 - r11, pc }

1:
  stmdb sp!, { r1 - r2 }  
  bl update_texture_4bpp_cache

  mov c_64, #64
  ldmia sp!, { r1 - r2 }
  bal 0b


.align 3

function(texture_blocks_8bpp)
  push { r4 - r11, lr }
  add block_ptr, psx_gpu, #psx_gpu_blocks_offset

  ldr texture_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset]
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]

  ldr clut_ptr, [psx_gpu, #psx_gpu_clut_ptr_offset]
  ldr current_texture_mask, [psx_gpu, #psx_gpu_current_texture_mask_offset]

  ldr dirty_textures_mask, [psx_gpu, #psx_gpu_dirty_textures_8bpp_mask_offset]
  tst dirty_textures_mask, current_texture_mask

  bne 1f
  nop

0:
  ldm block_ptr, { uv_01, uv_23, uv_45, uv_67 }

  uxtah uv_0, texture_ptr, uv_01
  uxtah uv_1, texture_ptr, uv_01, ror #16

  uxtah uv_2, texture_ptr, uv_23
  uxtah uv_3, texture_ptr, uv_23, ror #16

  uxtah uv_4, texture_ptr, uv_45
  ldrb pixel_0, [uv_0]

  uxtah uv_5, texture_ptr, uv_45, ror #16
  ldrb pixel_1, [uv_1]

  uxtah uv_6, texture_ptr, uv_67
  ldrb pixel_2, [uv_2]

  uxtah uv_7, texture_ptr, uv_67, ror #16
  ldrb pixel_3, [uv_3]

  ldrb pixel_4, [uv_4]
  add pixel_0, pixel_0, pixel_0

  ldrb pixel_5, [uv_5]
  add pixel_1, pixel_1, pixel_1

  ldrb pixel_6, [uv_6]
  add pixel_2, pixel_2, pixel_2

  ldrb pixel_7, [uv_7]
  add pixel_3, pixel_3, pixel_3

  ldrh pixel_0, [clut_ptr, pixel_0]
  add pixel_4, pixel_4, pixel_4

  ldrh pixel_1, [clut_ptr, pixel_1]
  add pixel_5, pixel_5, pixel_5

  ldrh pixel_2, [clut_ptr, pixel_2]
  add pixel_6, pixel_6, pixel_6

  ldrh pixel_3, [clut_ptr, pixel_3]
  add pixel_7, pixel_7, pixel_7

  ldrh pixel_4, [clut_ptr, pixel_4]
  orr pixels_a, pixel_0, pixel_1, lsl #16

  ldrh pixel_5, [clut_ptr, pixel_5]
  orr pixels_c, pixel_2, pixel_3, lsl #16

  ldrh pixel_6, [clut_ptr, pixel_6]
  subs num_blocks, num_blocks, #1

  ldrh pixel_7, [clut_ptr, pixel_7]
  orr pixels_b, pixel_4, pixel_5, lsl #16

  orr pixels_d, pixel_6, pixel_7, lsl #16
  stm block_ptr, { pixels_a, pixels_c, pixels_b, pixels_d } 

  add block_ptr, block_ptr, #64
  bne 0b

  pop { r4 - r11, pc }

1:
  /* pushing odd num of regs here realigns our unaligned stack */
  push { r1 - r2, EXTRA_UNSAVED_REGS r12 }
  bl   update_texture_8bpp_cache
  pop  { r1 - r2, EXTRA_UNSAVED_REGS r12 }
  bal  0b


#undef uv_0
#undef uv_1
#undef uv_2
#undef uv_3
#undef uv_4
#undef uv_5
#undef uv_6
#undef uv_7

#undef pixel_0
#undef pixel_1
#undef pixel_2
#undef pixel_3
#undef pixel_4
#undef pixel_5
#undef pixel_6
#undef pixel_7

#undef texture_ptr

#undef pixels_a
#undef pixels_b
#undef pixels_c
#undef pixels_d

#define psx_gpu                                  r0
#define block_ptr                                r1
#define num_blocks                               r2

#define uv_0                                     r3
#define uv_1                                     r4
#define u_0                                      r3
#define u_1                                      r4
#define v_0                                      r5
#define v_1                                      r6

#define uv_2                                     r5
#define uv_3                                     r6
#define u_2                                      r5
#define u_3                                      r6
#define v_2                                      r7
#define v_3                                      r8

#define uv_4                                     r7
#define uv_5                                     r8
#define u_4                                      r7
#define u_5                                      r8
#define v_4                                      r9
#define v_5                                      r10

#define uv_6                                     r9
#define uv_7                                     r10
#define u_6                                      r9
#define u_7                                      r10
#define v_6                                      r11
#define v_7                                      r0

#define pixel_0                                  r3
#define pixel_1                                  r4
#define pixel_2                                  r5
#define pixel_3                                  r6
#define pixel_4                                  r7
#define pixel_5                                  r8
#define pixel_6                                  r9
#define pixel_7                                  r10

#define pixels_a                                 r3
#define pixels_b                                 r5
#define pixels_c                                 r7
#define pixels_d                                 r9

#define texture_ptr                              r12


.align 3

function(texture_blocks_16bpp)
  stmdb sp!, { r3 - r11, r14 }
  add block_ptr, psx_gpu, #psx_gpu_blocks_offset

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  ldr texture_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset]

0:
  ldrh uv_0, [block_ptr]
  subs num_blocks, num_blocks, #1

  ldrh uv_1, [block_ptr, #2]

  and v_0, uv_0, #0xFF00
  and v_1, uv_1, #0xFF00

  and u_0, uv_0, #0xFF
  and u_1, uv_1, #0xFF

  add uv_0, u_0, v_0, lsl #2
  ldrh uv_2, [block_ptr, #4]

  add uv_1, u_1, v_1, lsl #2
  ldrh uv_3, [block_ptr, #6]

  add uv_0, uv_0, uv_0
  add uv_1, uv_1, uv_1

  and v_2, uv_2, #0xFF00
  and v_3, uv_3, #0xFF00

  and u_2, uv_2, #0xFF
  and u_3, uv_3, #0xFF

  add uv_2, u_2, v_2, lsl #2
  ldrh uv_4, [block_ptr, #8]

  add uv_3, u_3, v_3, lsl #2
  ldrh uv_5, [block_ptr, #10]

  add uv_2, uv_2, uv_2
  add uv_3, uv_3, uv_3

  and v_4, uv_4, #0xFF00
  and v_5, uv_5, #0xFF00

  and u_4, uv_4, #0xFF
  and u_5, uv_5, #0xFF

  add uv_4, u_4, v_4, lsl #2
  ldrh uv_6, [block_ptr, #12]

  add uv_5, u_5, v_5, lsl #2
  ldrh uv_7, [block_ptr, #14]

  add uv_4, uv_4, uv_4
  ldrh pixel_0, [texture_ptr, uv_0]

  add uv_5, uv_5, uv_5
  ldrh pixel_1, [texture_ptr, uv_1]

  and v_6, uv_6, #0xFF00
  ldrh pixel_2, [texture_ptr, uv_2]

  and v_7, uv_7, #0xFF00
  ldrh pixel_3, [texture_ptr, uv_3]

  and u_6, uv_6, #0xFF
  ldrh pixel_4, [texture_ptr, uv_4]

  and u_7, uv_7, #0xFF
  ldrh pixel_5, [texture_ptr, uv_5]

  add uv_6, u_6, v_6, lsl #2
  add uv_7, u_7, v_7, lsl #2

  add uv_6, uv_6, uv_6
  add uv_7, uv_7, uv_7

  orr pixels_a, pixel_0, pixel_1, lsl #16
  orr pixels_b, pixel_2, pixel_3, lsl #16

  ldrh pixel_6, [texture_ptr, uv_6]
  orr pixels_c, pixel_4, pixel_5, lsl #16

  ldrh pixel_7, [texture_ptr, uv_7]
  orr pixels_d, pixel_6, pixel_7, lsl #16

  stm block_ptr, { pixels_a, pixels_b, pixels_c, pixels_d }
  add block_ptr, block_ptr, #64

  bne 0b

  ldmia sp!, { r3 - r11, pc }


#undef num_blocks

#undef test_mask
#undef texels
#undef pixels_b
#undef pixels
#undef d64_1
#undef d64_4
#undef d64_128
#undef draw_mask
#undef msb_mask
#undef msb_mask_low
#undef msb_mask_high
#undef fb_pixels

#undef c_32
#undef fb_ptr
#undef mask_msb_ptr

#define psx_gpu                                  r0
#define num_blocks                               r1
#define color_ptr                                r2
#define colors_scalar                            r2
#define colors_scalar_compare                    r3
#define mask_msb_ptr                             r2

#define block_ptr_load_a                         r0
#define block_ptr_store                          r3
#define block_ptr_load_b                         r12
#define c_32                                     r2

#define c_48                                     r4
#define fb_ptr                                   r14
#define draw_mask_bits_scalar                    r5

#define d128_0x07                                q0
#define d128_0x1F                                q1
#define d128_0x8000                              q2
#define test_mask                                q3
#define texels                                   q4
#define colors_rg                                q5
#define colors_b_dm_bits                         q6
#define texels_rg                                q7
#define pixels_r                                 q8
#define pixels_g                                 q9
#define pixels_b                                 q10
#define pixels                                   q11
#define zero_mask                                q4
#define draw_mask                                q12
#define msb_mask                                 q13

#define fb_pixels                                q8

#define pixels_gb_low                            q9

#define colors_r                                 d10
#define colors_g                                 d11
#define colors_b                                 d12
#define draw_mask_bits                           d13
#define texels_r                                 d14
#define texels_g                                 d15
#define pixels_r_low                             d16
#define pixels_g_low                             d18
#define pixels_b_low                             d19
#define msb_mask_low                             d26
#define msb_mask_high                            d27

#define d64_1                                    d28
#define d64_4                                    d29
#define d64_128                                  d30
#define texels_b                                 d31

#define shade_blocks_textured_modulated_prologue_indirect()                    \
  mov c_48, #48;                                                               \
  add block_ptr_store, psx_gpu, #psx_gpu_blocks_offset                         \

#define shade_blocks_textured_modulated_prologue_direct()                      \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16]            \


#define shade_blocks_textured_modulated_prologue_shaded(dithering, target)     \
  
#define shade_blocks_textured_false_modulation_check_undithered(target)        \
  ldr colors_scalar, [psx_gpu, #psx_gpu_triangle_color_offset];                \
  movw colors_scalar_compare, #0x8080;                                         \
                                                                               \
  movt colors_scalar_compare, #0x80;                                           \
  cmp colors_scalar, colors_scalar_compare;                                    \
  beq shade_blocks_textured_unmodulated_##target                               \

#define shade_blocks_textured_false_modulation_check_dithered(target)          \

#define shade_blocks_textured_modulated_prologue_unshaded(dithering, target)   \
  shade_blocks_textured_false_modulation_check_##dithering(target);            \
  add color_ptr, psx_gpu, #psx_gpu_triangle_color_offset;                      \
  vld1.u32 { colors_r[] }, [color_ptr, :32];                                   \
  vdup.u8 colors_g, colors_r[1];                                               \
  vdup.u8 colors_b, colors_r[2];                                               \
  vdup.u8 colors_r, colors_r[0]                                                \


#define shade_blocks_textured_modulated_load_dithered(target)                  \
  vld1.u32 { target }, [block_ptr_load_b, :128]                                \

#define shade_blocks_textured_modulated_load_last_dithered(target)             \
  vld1.u32 { target }, [block_ptr_load_b, :128], c_32                          \

#define shade_blocks_textured_modulated_load_undithered(target)                \

#define shade_blocks_textured_modulated_load_last_undithered(target)           \
  add block_ptr_load_b, block_ptr_load_b, #32                                  \

#define shade_blocks_textured_modulate_dithered(channel)                       \
  vmlal.u8 pixels_##channel, texels_##channel, colors_##channel                \

#define shade_blocks_textured_modulate_undithered(channel)                     \
  vmull.u8 pixels_##channel, texels_##channel, colors_##channel                \


#define shade_blocks_textured_modulated_store_draw_mask_indirect(offset)       \
  vst1.u32 { draw_mask }, [block_ptr_store, :128]!                             \

#define shade_blocks_textured_modulated_store_draw_mask_direct(offset)         \
  ldr fb_ptr, [block_ptr_load_b, #(offset - 64)];                              \
  vld1.u32 { fb_pixels }, [fb_ptr];                                            \
  vbit.u16 pixels, fb_pixels, draw_mask                                        \

#define shade_blocks_textured_modulated_store_pixels_indirect()                \
  vst1.u32 { pixels }, [block_ptr_store, :128], c_48                           \

#define shade_blocks_textured_modulated_store_pixels_direct()                  \
  vst1.u32 { pixels }, [fb_ptr]                                                \


#define shade_blocks_textured_modulated_load_rg_shaded()                       \
  vld1.u32 { colors_r, colors_g }, [block_ptr_load_b, :128], c_32              \

#define shade_blocks_textured_modulated_load_rg_unshaded()                     \
  add block_ptr_load_b, block_ptr_load_b, #32                                  \

#define shade_blocks_textured_modulated_load_bdm_shaded()                      \
  vld1.u32 { colors_b, draw_mask_bits }, [block_ptr_load_a, :128], c_32        \

#define shade_blocks_textured_modulated_load_bdm_unshaded()                    \
  ldr draw_mask_bits_scalar, [block_ptr_load_a, #8];                           \
  add block_ptr_load_a, block_ptr_load_a, #32                                  \

#define shade_blocks_textured_modulated_expand_draw_mask_shaded()              \
  vdup.u16 draw_mask, draw_mask_bits[0]                                        \

#define shade_blocks_textured_modulated_expand_draw_mask_unshaded()            \
  vdup.u16 draw_mask, draw_mask_bits_scalar                                    \


#define shade_blocks_textured_modulated_apply_msb_mask_indirect()              \

#define shade_blocks_textured_modulated_apply_msb_mask_direct()                \
  vorr.u16 pixels, pixels, msb_mask                                            \


#define shade_blocks_textured_modulated_builder(shading, dithering, target)    \
.align 3;                                                                      \
                                                                               \
function(shade_blocks_##shading##_textured_modulated_##dithering##_##target)   \
  save_abi_regs();                                                             \
  shade_blocks_textured_modulated_prologue_##shading(dithering, target);       \
  stmdb sp!, { r4 - r5, lr };                                                  \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  vld1.u32 { test_mask }, [psx_gpu, :128];                                     \
                                                                               \
  shade_blocks_textured_modulated_prologue_##target();                         \
                                                                               \
  add block_ptr_load_a, psx_gpu, #psx_gpu_blocks_offset;                       \
  mov c_32, #32;                                                               \
                                                                               \
  add block_ptr_load_b, block_ptr_load_a, #16;                                 \
  vmov.u8 d64_1, #1;                                                           \
  vmov.u8 d64_4, #4;                                                           \
  vmov.u8 d64_128, #128;                                                       \
                                                                               \
  vld1.u32 { texels }, [block_ptr_load_a, :128], c_32;                         \
  vmov.u8 d128_0x07, #0x07;                                                    \
                                                                               \
  shade_blocks_textured_modulated_load_rg_##shading();                         \
  vmov.u8 d128_0x1F, #0x1F;                                                    \
                                                                               \
  shade_blocks_textured_modulated_load_bdm_##shading();                        \
  vmov.u16 d128_0x8000, #0x8000;                                               \
                                                                               \
  vmovn.u16 texels_r, texels;                                                  \
  vshrn.u16 texels_g, texels, #5;                                              \
                                                                               \
  vshrn.u16 texels_b, texels, #7;                                              \
  shade_blocks_textured_modulated_expand_draw_mask_##shading();                \
                                                                               \
  shade_blocks_textured_modulated_load_##dithering(pixels_r);                  \
  vtst.u16 draw_mask, draw_mask, test_mask;                                    \
                                                                               \
  shade_blocks_textured_modulated_load_##dithering(pixels_g);                  \
  vand.u8 texels_rg, texels_rg, d128_0x1F;                                     \
                                                                               \
  shade_blocks_textured_modulated_load_last_##dithering(pixels_b);             \
  vshr.u8 texels_b, texels_b, #3;                                              \
                                                                               \
  shade_blocks_textured_modulate_##dithering(r);                               \
  shade_blocks_textured_modulate_##dithering(g);                               \
  shade_blocks_textured_modulate_##dithering(b);                               \
                                                                               \
  vand.u16 pixels, texels, d128_0x8000;                                        \
  vceq.u16 zero_mask, texels, #0;                                              \
                                                                               \
  vqshrun.s16 pixels_r_low, pixels_r, #4;                                      \
  vqshrun.s16 pixels_g_low, pixels_g, #4;                                      \
  vqshrun.s16 pixels_b_low, pixels_b, #4;                                      \
                                                                               \
  shade_blocks_textured_modulated_apply_msb_mask_##target();                   \
  vorr.u16 draw_mask, draw_mask, zero_mask;                                    \
  vshr.u8 pixels_r_low, pixels_r_low, #3;                                      \
  vbic.u8 pixels_gb_low, pixels_gb_low, d128_0x07;                             \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 .align 3;                                                                     \
                                                                               \
 0:                                                                            \
  vld1.u32 { texels }, [block_ptr_load_a, :128], c_32;                         \
  shade_blocks_textured_modulated_load_rg_##shading();                         \
  vshrn.u16 texels_g, texels, #5;                                              \
                                                                               \
  shade_blocks_textured_modulated_load_bdm_##shading();                        \
  vshrn.u16 texels_b, texels, #7;                                              \
                                                                               \
  pld [block_ptr_load_a];                                                      \
  vmovn.u16 texels_r, texels;                                                  \
  vmlal.u8 pixels, pixels_r_low, d64_1;                                        \
                                                                               \
  vmlal.u8 pixels, pixels_g_low, d64_4;                                        \
  vmlal.u8 pixels, pixels_b_low, d64_128;                                      \
  shade_blocks_textured_modulated_store_draw_mask_##target(-4);                \
                                                                               \
  shade_blocks_textured_modulated_load_##dithering(pixels_r);                  \
  shade_blocks_textured_modulated_expand_draw_mask_##shading();                \
                                                                               \
  shade_blocks_textured_modulated_load_##dithering(pixels_g);                  \
  vand.u8 texels_rg, texels_rg, d128_0x1F;                                     \
                                                                               \
  shade_blocks_textured_modulated_load_last_##dithering(pixels_b);             \
  vtst.u16 draw_mask, draw_mask, test_mask;                                    \
                                                                               \
  shade_blocks_textured_modulated_store_pixels_##target();                     \
  vshr.u8 texels_b, texels_b, #3;                                              \
                                                                               \
  shade_blocks_textured_modulate_##dithering(r);                               \
  shade_blocks_textured_modulate_##dithering(g);                               \
  shade_blocks_textured_modulate_##dithering(b);                               \
                                                                               \
  vand.u16 pixels, texels, d128_0x8000;                                        \
  vceq.u16 zero_mask, texels, #0;                                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
                                                                               \
  vqshrun.s16 pixels_r_low, pixels_r, #4;                                      \
  vqshrun.s16 pixels_g_low, pixels_g, #4;                                      \
  vqshrun.s16 pixels_b_low, pixels_b, #4;                                      \
                                                                               \
  shade_blocks_textured_modulated_apply_msb_mask_##target();                   \
  vorr.u16 draw_mask, draw_mask, zero_mask;                                    \
  vshr.u8 pixels_r_low, pixels_r_low, #3;                                      \
  vbic.u8 pixels_gb_low, pixels_gb_low, d128_0x07;                             \
                                                                               \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  vmlal.u8 pixels, pixels_r_low, d64_1;                                        \
  vmlal.u8 pixels, pixels_g_low, d64_4;                                        \
  vmlal.u8 pixels, pixels_b_low, d64_128;                                      \
                                                                               \
  shade_blocks_textured_modulated_store_draw_mask_##target(28);                \
  shade_blocks_textured_modulated_store_pixels_##target();                     \
                                                                               \
  ldmia sp!, { r4 - r5, lr };                                                  \
  restore_abi_regs();                                                          \
  bx lr                                                                        \


shade_blocks_textured_modulated_builder(shaded, dithered, direct);
shade_blocks_textured_modulated_builder(shaded, undithered, direct);
shade_blocks_textured_modulated_builder(unshaded, dithered, direct);
shade_blocks_textured_modulated_builder(unshaded, undithered, direct);

shade_blocks_textured_modulated_builder(shaded, dithered, indirect);
shade_blocks_textured_modulated_builder(shaded, undithered, indirect);
shade_blocks_textured_modulated_builder(unshaded, dithered, indirect);
shade_blocks_textured_modulated_builder(unshaded, undithered, indirect);


#undef c_64
#undef fb_ptr
#undef color_ptr

#undef color_r
#undef color_g
#undef color_b

#undef test_mask
#undef pixels
#undef draw_mask
#undef zero_mask
#undef fb_pixels
#undef msb_mask
#undef msb_mask_low
#undef msb_mask_high

#define psx_gpu                                  r0
#define num_blocks                               r1
#define mask_msb_ptr                             r2
#define color_ptr                                r3

#define block_ptr_load                           r0
#define draw_mask_store_ptr                      r3
#define draw_mask_bits_ptr                       r12
#define draw_mask_ptr                            r12
#define pixel_store_ptr                          r14

#define fb_ptr_cmp                               r4

#define fb_ptr                                   r3
#define fb_ptr_next                              r14

#define c_64                                     r2

#define test_mask                                q0
#define pixels                                   q1
#define draw_mask                                q2
#define zero_mask                                q3
#define draw_mask_combined                       q4
#define fb_pixels                                q5
#define fb_pixels_next                           q6
#define msb_mask                                 q7

#define draw_mask_low                            d4
#define draw_mask_high                           d5
#define msb_mask_low                             d14
#define msb_mask_high                            d15

.align 3
function(shade_blocks_textured_unmodulated_indirect)
  stmdb sp!, { r4, r14 }
  save_abi_regs()
  add draw_mask_bits_ptr, psx_gpu, #(psx_gpu_blocks_offset + 40)

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  add pixel_store_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16)

  vld1.u32 { test_mask }, [psx_gpu, :128]
  add draw_mask_store_ptr, psx_gpu, #psx_gpu_blocks_offset

  mov c_64, #64
  add block_ptr_load, psx_gpu, #psx_gpu_blocks_offset

  vld1.u32 { pixels }, [block_ptr_load, :128], c_64
  vld1.u16 { draw_mask_low[], draw_mask_high[] },                              \
   [draw_mask_bits_ptr, :16], c_64
  vceq.u16 zero_mask, pixels, #0

  vtst.u16 draw_mask, draw_mask, test_mask
  vst1.u32 { pixels }, [pixel_store_ptr, :128], c_64

  subs num_blocks, num_blocks, #1
  beq 1f

 0:
  vld1.u32 { pixels }, [block_ptr_load, :128], c_64
  vorr.u16 draw_mask_combined, draw_mask, zero_mask

  vld1.u16 { draw_mask_low[], draw_mask_high[] },                              \
   [draw_mask_bits_ptr, :16], c_64
  vceq.u16 zero_mask, pixels, #0

  vtst.u16 draw_mask, draw_mask, test_mask
  vst1.u32 { pixels }, [pixel_store_ptr, :128], c_64

  vst1.u32 { draw_mask_combined }, [draw_mask_store_ptr, :128], c_64
  subs num_blocks, num_blocks, #1

  bne 0b

 1:
  vorr.u16 draw_mask_combined, draw_mask, zero_mask
  vst1.u32 { draw_mask_combined }, [draw_mask_store_ptr, :128], c_64

  restore_abi_regs()
  ldmia sp!, { r4, pc }


.align 3

function(shade_blocks_textured_unmodulated_direct)
  stmdb sp!, { r4, r14 }
  save_abi_regs()
  add draw_mask_bits_ptr, psx_gpu, #(psx_gpu_blocks_offset + 40)

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset

  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16]
  mov c_64, #64

  vld1.u32 { test_mask }, [psx_gpu, :128]
  add block_ptr_load, psx_gpu, #psx_gpu_blocks_offset

  vld1.u16 { draw_mask_low[], draw_mask_high[] },                              \
   [draw_mask_bits_ptr, :16], c_64
  ldr fb_ptr_next, [block_ptr_load, #44]

  vld1.u32 { pixels }, [block_ptr_load, :128], c_64
  vld1.u16 { fb_pixels_next }, [fb_ptr_next]
  vceq.u16 zero_mask, pixels, #0
  vtst.u16 draw_mask, draw_mask, test_mask

  subs num_blocks, num_blocks, #1
  beq 1f

 0:
  mov fb_ptr, fb_ptr_next
  ldr fb_ptr_next, [block_ptr_load, #44]

  vorr.u16 pixels, pixels, msb_mask

  vorr.u16 draw_mask_combined, draw_mask, zero_mask
  vmov fb_pixels, fb_pixels_next

  vld1.u16 { draw_mask_low[], draw_mask_high[] },                              \
   [draw_mask_bits_ptr, :16], c_64
  vbif.u16 fb_pixels, pixels, draw_mask_combined

  sub fb_ptr_cmp, fb_ptr_next, fb_ptr
  pld [fb_ptr_next, #64]

  add fb_ptr_cmp, fb_ptr_cmp, #14
  vld1.u32 { pixels }, [block_ptr_load, :128], c_64

  cmp fb_ptr_cmp, #28
  bls 4f

  vld1.u16 { fb_pixels_next }, [fb_ptr_next]
  vceq.u16 zero_mask, pixels, #0

  vst1.u16 { fb_pixels }, [fb_ptr]
  vtst.u16 draw_mask, draw_mask, test_mask

 3:
  subs num_blocks, num_blocks, #1
  bne 0b

 1:
  vorr.u16 draw_mask_combined, draw_mask, zero_mask
  vbif.u16 fb_pixels_next, pixels, draw_mask_combined

  vst1.u16 { fb_pixels_next }, [fb_ptr_next]

  restore_abi_regs()
  ldmia sp!, { r4, pc }

 4:
  vst1.u16 { fb_pixels }, [fb_ptr]
  vceq.u16 zero_mask, pixels, #0

  vld1.u16 { fb_pixels_next }, [fb_ptr_next]
  vtst.u16 draw_mask, draw_mask, test_mask

  bal 3b


function(shade_blocks_unshaded_untextured_indirect)
  bx lr

.align 3

function(shade_blocks_unshaded_untextured_direct)
  stmdb sp!, { r4, r14 }
  save_abi_regs()
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset

  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16]
  add color_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16)

  add block_ptr_load, psx_gpu, #(psx_gpu_blocks_offset + 44)
  vld1.u16 { pixels }, [color_ptr, :128]

  mov c_64, #64
  vld1.u16 { draw_mask }, [draw_mask_ptr, :128], c_64

  vorr.u16 pixels, pixels, msb_mask
  subs num_blocks, num_blocks, #1

  ldr fb_ptr_next, [block_ptr_load], #64

  vld1.u16 { fb_pixels_next }, [fb_ptr_next]
  beq 1f

 0:
  vmov fb_pixels, fb_pixels_next
  mov fb_ptr, fb_ptr_next
  ldr fb_ptr_next, [block_ptr_load], #64

  vbif.u16 fb_pixels, pixels, draw_mask
  vld1.u16 { draw_mask }, [draw_mask_ptr, :128], c_64

  sub fb_ptr_cmp, fb_ptr_next, fb_ptr
  add fb_ptr_cmp, fb_ptr_cmp, #14
  cmp fb_ptr_cmp, #28
  bls 4f

  vld1.u16 { fb_pixels_next }, [fb_ptr_next]
  vst1.u16 { fb_pixels }, [fb_ptr]

 3:
  subs num_blocks, num_blocks, #1
  bne 0b

 1:
  vbif.u16 fb_pixels_next, pixels, draw_mask
  vst1.u16 { fb_pixels_next }, [fb_ptr_next]

  restore_abi_regs()
  ldmia sp!, { r4, pc }

 4:
  vst1.u16 { fb_pixels }, [fb_ptr]
  vld1.u16 { fb_pixels_next }, [fb_ptr_next]
  bal 3b


#undef draw_mask_ptr
#undef c_64
#undef fb_ptr
#undef fb_ptr_next
#undef fb_ptr_cmp

#define psx_gpu                                  r0
#define num_blocks                               r1
#define msb_mask_ptr                             r2
#define pixel_ptr                                r3
#define draw_mask_ptr                            r0
#define c_64                                     r2
#define fb_ptr                                   r12
#define fb_ptr_next                              r14
#define fb_ptr_cmp                               r4

#undef msb_mask
#undef draw_mask
#undef pixels
#undef fb_pixels
#undef d128_0x8000
#undef msb_mask_low
#undef msb_mask_high
#undef draw_mask_next
#undef pixels_g
#undef blend_pixels
#undef fb_pixels_next

#define msb_mask                                 q0
#define draw_mask                                q1
#define pixels                                   q2
#define fb_pixels                                q3
#define blend_pixels                             q4
#define pixels_no_msb                            q5
#define blend_mask                               q6
#define fb_pixels_no_msb                         q7
#define d128_0x8000                              q8
#define d128_0x0421                              q9
#define fb_pixels_next                           q10
#define blend_pixels_next                        q11
#define pixels_next                              q12
#define draw_mask_next                           q13
#define write_mask                               q14

#define pixels_rb                                q5
#define pixels_mg                                q7
#define pixels_g                                 q7
#define d128_0x7C1F                              q8
#define d128_0x03E0                              q9
#define fb_pixels_rb                             q10
#define fb_pixels_g                              q11
#define fb_pixels_masked                         q11
#define d128_0x83E0                              q15
#define pixels_fourth                            q7
#define d128_0x1C07                              q12
#define d128_0x00E0                              q13
#define d128_0x80E0                              q13

#define msb_mask_low                             d0
#define msb_mask_high                            d1

#define blend_blocks_average_set_blend_mask_textured(source)                   \
  vclt.s16 blend_mask, source, #0                                              \

#define blend_blocks_average_set_stp_bit_textured()                            \
  vorr.u16 blend_pixels, #0x8000                                               \

#define blend_blocks_average_combine_textured(source)                          \
  vbif.u16 blend_pixels, source, blend_mask                                    \
  
#define blend_blocks_average_set_blend_mask_untextured(source)                 \

#define blend_blocks_average_set_stp_bit_untextured()                          \

#define blend_blocks_average_combine_untextured(source)                        \

#define blend_blocks_average_mask_set_on()                                     \
  vclt.s16 write_mask, fb_pixels_next, #0                                      \

#define blend_blocks_average_mask_copy_on()                                    \
  vorr.u16 draw_mask, draw_mask_next, write_mask                               \

#define blend_blocks_average_mask_copy_b_on()                                  \
  vorr.u16 draw_mask_next, draw_mask_next, write_mask                          \

#define blend_blocks_average_mask_set_off()                                    \

#define blend_blocks_average_mask_copy_off()                                   \
  vmov draw_mask, draw_mask_next                                               \

#define blend_blocks_average_mask_copy_b_off()                                 \

#define blend_blocks_average_builder(texturing, mask_evaluate)                 \
.align 3;                                                                      \
                                                                               \
function(blend_blocks_##texturing##_average_##mask_evaluate)                   \
  stmdb sp!, { r4, r14 };                                                      \
  save_abi_regs();                                                             \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16);                       \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset;                          \
  mov c_64, #64;                                                               \
                                                                               \
  vmov.u16 d128_0x8000, #0x8000;                                               \
  vld1.u32 { draw_mask_next }, [draw_mask_ptr, :128], c_64;                    \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vmov.u16 d128_0x0421, #0x0400;                                               \
  vld1.u32 { pixels_next }, [pixel_ptr, :128], c_64;                           \
                                                                               \
  vorr.u16 d128_0x0421, #0x0021;                                               \
  vld1.u16 { fb_pixels_next }, [fb_ptr_next];                                  \
                                                                               \
  veor.u16 blend_pixels_next, pixels_next, fb_pixels_next;                     \
  vbic.u16 pixels_no_msb, pixels_next, d128_0x8000;                            \
  vand.u16 blend_pixels_next, blend_pixels_next, d128_0x0421;                  \
  vsub.u16 blend_pixels_next, pixels_no_msb, blend_pixels_next;                \
  blend_blocks_average_mask_set_##mask_evaluate();                             \
  vbic.u16 fb_pixels_no_msb, fb_pixels_next, d128_0x8000;                      \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 0:                                                                            \
  mov fb_ptr, fb_ptr_next;                                                     \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vmov pixels, pixels_next;                                                    \
  vld1.u32 { pixels_next }, [pixel_ptr, :128], c_64;                           \
                                                                               \
  vhadd.u16 blend_pixels, fb_pixels_no_msb, blend_pixels_next;                 \
                                                                               \
  blend_blocks_average_mask_copy_##mask_evaluate();                            \
  vld1.u32 { draw_mask_next }, [draw_mask_ptr, :128], c_64;                    \
                                                                               \
  blend_blocks_average_set_blend_mask_##texturing(pixels);                     \
  blend_blocks_average_set_stp_bit_##texturing();                              \
  vmov fb_pixels, fb_pixels_next;                                              \
  blend_blocks_average_combine_##texturing(pixels);                            \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  add fb_ptr_cmp, fb_ptr_cmp, #14;                                             \
  cmp fb_ptr_cmp, #28;                                                         \
  bls 2f;                                                                      \
                                                                               \
  vld1.u16 { fb_pixels_next }, [fb_ptr_next];                                  \
  veor.u16 blend_pixels_next, pixels_next, fb_pixels_next;                     \
                                                                               \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vbic.u16 pixels_no_msb, pixels_next, d128_0x8000;                            \
                                                                               \
  vand.u16 blend_pixels_next, blend_pixels_next, d128_0x0421;                  \
  vbif.u16 fb_pixels, blend_pixels, draw_mask;                                 \
                                                                               \
  vbic.u16 fb_pixels_no_msb, fb_pixels_next, d128_0x8000;                      \
  vsub.u16 blend_pixels_next, pixels_no_msb, blend_pixels_next;                \
  blend_blocks_average_mask_set_##mask_evaluate();                             \
  vst1.u16 { fb_pixels }, [fb_ptr];                                            \
                                                                               \
 3:                                                                            \
  subs num_blocks, num_blocks, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  blend_blocks_average_mask_copy_b_##mask_evaluate();                          \
  vhadd.u16 blend_pixels, fb_pixels_no_msb, blend_pixels_next;                 \
                                                                               \
  blend_blocks_average_set_blend_mask_##texturing(pixels_next);                \
  blend_blocks_average_set_stp_bit_##texturing();                              \
  blend_blocks_average_combine_##texturing(pixels_next);                       \
                                                                               \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vbif.u16 fb_pixels_next, blend_pixels, draw_mask_next;                       \
  vst1.u16 { fb_pixels_next }, [fb_ptr_next];                                  \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4, pc };                                                       \
                                                                               \
 2:                                                                            \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vbif.u16 fb_pixels, blend_pixels, draw_mask;                                 \
  vst1.u16 { fb_pixels }, [fb_ptr];                                            \
                                                                               \
  vld1.u16 { fb_pixels_next }, [fb_ptr_next];                                  \
  veor.u16 blend_pixels_next, pixels_next, fb_pixels_next;                     \
  vbic.u16 pixels_no_msb, pixels_next, d128_0x8000;                            \
  vand.u16 blend_pixels_next, blend_pixels_next, d128_0x0421;                  \
  vsub.u16 blend_pixels_next, pixels_no_msb, blend_pixels_next;                \
  vbic.u16 fb_pixels_no_msb, fb_pixels_next, d128_0x8000;                      \
                                                                               \
  bal 3b                                                                       \

blend_blocks_average_builder(textured, off)
blend_blocks_average_builder(untextured, off)
blend_blocks_average_builder(textured, on)
blend_blocks_average_builder(untextured, on)


#define blend_blocks_add_mask_set_on()                                         \
  vclt.s16 write_mask, fb_pixels, #0                                           \

#define blend_blocks_add_mask_copy_on()                                        \
  vorr.u16 draw_mask, draw_mask, write_mask                                    \

#define blend_blocks_add_mask_set_off()                                        \

#define blend_blocks_add_mask_copy_off()                                       \


#define blend_blocks_add_textured_builder(mask_evaluate)                       \
.align 3;                                                                      \
                                                                               \
function(blend_blocks_textured_add_##mask_evaluate)                            \
  stmdb sp!, { r4, r14 };                                                      \
  save_abi_regs();                                                             \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16);                       \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset;                          \
  mov c_64, #64;                                                               \
                                                                               \
  vmov.u16 d128_0x7C1F, #0x7C00;                                               \
  vmov.u16 d128_0x03E0, #0x0300;                                               \
  vmov.u16 d128_0x83E0, #0x8000;                                               \
  vorr.u16 d128_0x03E0, #0x00E0;                                               \
  vorr.u16 d128_0x7C1F, #0x001F;                                               \
  vorr.u16 d128_0x83E0, d128_0x83E0, d128_0x03E0;                              \
                                                                               \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
  vclt.s16 blend_mask, pixels, #0;                                             \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  vand.u16 pixels_rb, pixels, d128_0x7C1F;                                     \
                                                                               \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vorr.u16 pixels, pixels, msb_mask;                                           \
  vand.u16 fb_pixels_masked, fb_pixels, blend_mask;                            \
  vand.u16 pixels_mg, pixels, d128_0x83E0;                                     \
  vand.u16 fb_pixels_rb, fb_pixels_masked, d128_0x7C1F;                        \
  vand.u16 fb_pixels_g, fb_pixels_masked, d128_0x03E0;                         \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_mg;                                \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x83E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 0:                                                                            \
  mov fb_ptr, fb_ptr_next;                                                     \
                                                                               \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
  vclt.s16 blend_mask, pixels, #0;                                             \
                                                                               \
  vorr.u16 pixels, pixels, msb_mask;                                           \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vand.u16 pixels_mg, pixels, d128_0x83E0;                                     \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  pld [fb_ptr_next, #64];                                                      \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
                                                                               \
  add fb_ptr_cmp, fb_ptr_cmp, #14;                                             \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
                                                                               \
  cmp fb_ptr_cmp, #28;                                                         \
  bls 2f;                                                                      \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  vand.u16 fb_pixels_masked, fb_pixels, blend_mask;                            \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_rb, pixels, d128_0x7C1F;                                     \
  vand.u16 fb_pixels_rb, fb_pixels_masked, d128_0x7C1F;                        \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
                                                                               \
 3:                                                                            \
  vand.u16 fb_pixels_g, fb_pixels_masked, d128_0x03E0;                         \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_mg;                                \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x83E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vst1.u16 { blend_pixels }, [fb_ptr_next];                                    \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4, pc };                                                       \
                                                                               \
 2:                                                                            \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
  vand.u16 pixels_rb, pixels, d128_0x7C1F;                                     \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  vand.u16 fb_pixels_masked, fb_pixels, blend_mask;                            \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 fb_pixels_rb, fb_pixels_masked, d128_0x7C1F;                        \
  bal 3b                                                                       \


#define blend_blocks_add_untextured_builder(mask_evaluate)                     \
.align 3;                                                                      \
                                                                               \
function(blend_blocks_untextured_add_##mask_evaluate)                          \
  stmdb sp!, { r4, r14 };                                                      \
  save_abi_regs();                                                             \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16);                       \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset;                          \
  mov c_64, #64;                                                               \
                                                                               \
  vmov.u16 d128_0x7C1F, #0x7C00;                                               \
  vmov.u16 d128_0x03E0, #0x0300;                                               \
  vorr.u16 d128_0x7C1F, #0x001F;                                               \
  vorr.u16 d128_0x03E0, #0x00E0;                                               \
                                                                               \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  vand.u16 pixels_rb, pixels, d128_0x7C1F;                                     \
                                                                               \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_g, pixels, d128_0x03E0;                                      \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                 \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x03E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 0:                                                                            \
  mov fb_ptr, fb_ptr_next;                                                     \
                                                                               \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
                                                                               \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vand.u16 pixels_g, pixels, d128_0x03E0;                                      \
                                                                               \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  add fb_ptr_cmp, fb_ptr_cmp, #14;                                             \
  cmp fb_ptr_cmp, #28;                                                         \
  bls 2f;                                                                      \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_rb, pixels, d128_0x7C1F;                                     \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
                                                                               \
 3:                                                                            \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                 \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x03E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vst1.u16 { blend_pixels }, [fb_ptr_next];                                    \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4, pc };                                                       \
                                                                               \
 2:                                                                            \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
  vand.u16 pixels_rb, pixels, d128_0x7C1F;                                     \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  bal 3b                                                                       \


blend_blocks_add_textured_builder(off)
blend_blocks_add_textured_builder(on)
blend_blocks_add_untextured_builder(off)
blend_blocks_add_untextured_builder(on)

#define blend_blocks_subtract_set_blend_mask_textured()                        \
  vclt.s16 blend_mask, pixels_next, #0                                         \

#define blend_blocks_subtract_combine_textured()                               \
  vbif.u16 blend_pixels, pixels, blend_mask                                    \

#define blend_blocks_subtract_set_stp_textured()                               \
  vorr.u16 blend_pixels, #0x8000                                               \

#define blend_blocks_subtract_msb_mask_textured()                              \
  vorr.u16 pixels, pixels_next, msb_mask                                       \

#define blend_blocks_subtract_set_blend_mask_untextured()                      \

#define blend_blocks_subtract_combine_untextured()                             \

#define blend_blocks_subtract_set_stp_untextured()                             \
  vorr.u16 blend_pixels, blend_pixels, msb_mask                                \

#define blend_blocks_subtract_msb_mask_untextured()                            \


#define blend_blocks_subtract_mask_set_on()                                    \
  vclt.s16 write_mask, fb_pixels, #0                                           \

#define blend_blocks_subtract_mask_copy_on()                                   \
  vorr.u16 draw_mask, draw_mask_next, write_mask                               \

#define blend_blocks_subtract_mask_set_off()                                   \

#define blend_blocks_subtract_mask_copy_off()                                  \
  vmov draw_mask, draw_mask_next                                               \


#define blend_blocks_subtract_builder(texturing, mask_evaluate)                \
.align 3;                                                                      \
                                                                               \
function(blend_blocks_##texturing##_subtract_##mask_evaluate)                  \
  stmdb sp!, { r4, r14 };                                                      \
  save_abi_regs();                                                             \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16);                       \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset;                          \
  mov c_64, #64;                                                               \
                                                                               \
  vmov.u16 d128_0x7C1F, #0x7C00;                                               \
  vmov.u16 d128_0x03E0, #0x0300;                                               \
  vorr.u16 d128_0x7C1F, #0x001F;                                               \
  vorr.u16 d128_0x03E0, #0x00E0;                                               \
                                                                               \
  vld1.u32 { draw_mask_next }, [draw_mask_ptr, :128], c_64;                    \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
  vld1.u32 { pixels_next }, [pixel_ptr, :128], c_64;                           \
  blend_blocks_subtract_set_blend_mask_##texturing();                          \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_subtract_mask_set_##mask_evaluate();                            \
  vand.u16 pixels_rb, pixels_next, d128_0x7C1F;                                \
                                                                               \
  vand.u16 pixels_g, pixels_next, d128_0x03E0;                                 \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vqsub.u8 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vqsub.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 0:                                                                            \
  blend_blocks_subtract_mask_copy_##mask_evaluate();                           \
  mov fb_ptr, fb_ptr_next;                                                     \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vld1.u32 { draw_mask_next }, [draw_mask_ptr, :128], c_64;                    \
  blend_blocks_subtract_msb_mask_##texturing();                                \
                                                                               \
  vld1.u32 { pixels_next }, [pixel_ptr, :128], c_64;                           \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vand.u16 pixels_rb, pixels_next, d128_0x7C1F;                                \
  blend_blocks_subtract_set_stp_##texturing();                                 \
  vand.u16 pixels_g, pixels_next, d128_0x03E0;                                 \
  blend_blocks_subtract_combine_##texturing();                                 \
  blend_blocks_subtract_set_blend_mask_##texturing();                          \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  add fb_ptr_cmp, fb_ptr_cmp, #14;                                             \
  cmp fb_ptr_cmp, #28;                                                         \
  bls 2f;                                                                      \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_subtract_mask_set_##mask_evaluate();                            \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vqsub.u8 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
  vqsub.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                \
                                                                               \
 3:                                                                            \
  subs num_blocks, num_blocks, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  blend_blocks_subtract_mask_copy_##mask_evaluate();                           \
                                                                               \
  blend_blocks_subtract_msb_mask_##texturing();                                \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  blend_blocks_subtract_set_stp_##texturing();                                 \
  blend_blocks_subtract_combine_##texturing();                                 \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vst1.u16 { blend_pixels }, [fb_ptr_next];                                    \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4, pc };                                                       \
                                                                               \
 2:                                                                            \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_subtract_mask_set_##mask_evaluate();                            \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vqsub.u8 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vqsub.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                \
  bal 3b                                                                       \


blend_blocks_subtract_builder(textured, off)
blend_blocks_subtract_builder(textured, on)
blend_blocks_subtract_builder(untextured, off)
blend_blocks_subtract_builder(untextured, on)


#define blend_blocks_add_fourth_textured_builder(mask_evaluate)                \
.align 3;                                                                      \
                                                                               \
function(blend_blocks_textured_add_fourth_##mask_evaluate)                     \
  stmdb sp!, { r4, r14 };                                                      \
  save_abi_regs();                                                             \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16);                       \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset;                          \
  mov c_64, #64;                                                               \
                                                                               \
  vmov.u16 d128_0x7C1F, #0x7C00;                                               \
  vmov.u16 d128_0x03E0, #0x0300;                                               \
  vmov.u16 d128_0x1C07, #0x1C00;                                               \
  vmov.u16 d128_0x00E0, #0x00E0;                                               \
  vorr.u16 d128_0x7C1F, #0x001F;                                               \
  vorr.u16 d128_0x03E0, #0x00E0;                                               \
  vorr.u16 d128_0x1C07, #0x0007;                                               \
                                                                               \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
  vclt.s16 blend_mask, pixels, #0;                                             \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  vshr.s16 pixels_fourth, pixels, #2;                                          \
  vand.u16 pixels_rb, pixels_fourth, d128_0x1C07;                              \
                                                                               \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_g, pixels_fourth, d128_0x00E0;                               \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                 \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x03E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 0:                                                                            \
  mov fb_ptr, fb_ptr_next;                                                     \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vorr.u16 blend_pixels, #0x8000;  /* stp */                                   \
  vbif.u16 blend_pixels, pixels, blend_mask;                                   \
                                                                               \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
  vclt.s16 blend_mask, pixels, #0;                                             \
  vshr.s16 pixels_fourth, pixels, #2;                                          \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vand.u16 pixels_rb, pixels_fourth, d128_0x1C07;                              \
                                                                               \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  add fb_ptr_cmp, fb_ptr_cmp, #14;                                             \
  cmp fb_ptr_cmp, #28;                                                         \
  bls 2f;                                                                      \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_g, pixels_fourth, d128_0x00E0;                               \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
                                                                               \
 3:                                                                            \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                 \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x03E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vorr.u16 blend_pixels, #0x8000;  /* stp */                                   \
  vbif.u16 blend_pixels, pixels, blend_mask;                                   \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vst1.u16 { blend_pixels }, [fb_ptr_next];                                    \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4, pc };                                                       \
                                                                               \
 2:                                                                            \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
  vand.u16 pixels_g, pixels_fourth, d128_0x00E0;                               \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  bal 3b                                                                       \



#define blend_blocks_add_fourth_untextured_builder(mask_evaluate)              \
.align 3;                                                                      \
                                                                               \
function(blend_blocks_untextured_add_fourth_##mask_evaluate)                   \
  stmdb sp!, { r4, r14 };                                                      \
  save_abi_regs();                                                             \
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset;                         \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
                                                                               \
  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16);                       \
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16];           \
                                                                               \
  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset;                          \
  mov c_64, #64;                                                               \
                                                                               \
  vmov.u16 d128_0x7C1F, #0x7C00;                                               \
  vmov.u16 d128_0x03E0, #0x0300;                                               \
  vmov.u16 d128_0x1C07, #0x1C00;                                               \
  vmov.u16 d128_0x00E0, #0x00E0;                                               \
  vorr.u16 d128_0x7C1F, #0x001F;                                               \
  vorr.u16 d128_0x03E0, #0x00E0;                                               \
  vorr.u16 d128_0x1C07, #0x0007;                                               \
                                                                               \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  vshr.s16 pixels_fourth, pixels, #2;                                          \
  vand.u16 pixels_rb, pixels_fourth, d128_0x1C07;                              \
                                                                               \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_g, pixels_fourth, d128_0x00E0;                               \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                 \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x03E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  beq 1f;                                                                      \
                                                                               \
 0:                                                                            \
  mov fb_ptr, fb_ptr_next;                                                     \
  ldr fb_ptr_next, [pixel_ptr, #28];                                           \
                                                                               \
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64;                                \
                                                                               \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vshr.s16 pixels_fourth, pixels, #2;                                          \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vand.u16 pixels_rb, pixels_fourth, d128_0x1C07;                              \
                                                                               \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64;                         \
                                                                               \
  sub fb_ptr_cmp, fb_ptr_next, fb_ptr;                                         \
  add fb_ptr_cmp, fb_ptr_cmp, #14;                                             \
  cmp fb_ptr_cmp, #28;                                                         \
  bls 2f;                                                                      \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 pixels_g, pixels_fourth, d128_0x00E0;                               \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
                                                                               \
 3:                                                                            \
  vand.u16 fb_pixels_g, fb_pixels, d128_0x03E0;                                \
  vadd.u16 fb_pixels_rb, fb_pixels_rb, pixels_rb;                              \
  vadd.u16 fb_pixels_g, fb_pixels_g, pixels_g;                                 \
  vmin.u8 fb_pixels_rb, fb_pixels_rb, d128_0x7C1F;                             \
  vmin.u16 fb_pixels_g, fb_pixels_g, d128_0x03E0;                              \
                                                                               \
  subs num_blocks, num_blocks, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  vorr.u16 blend_pixels, fb_pixels_rb, fb_pixels_g;                            \
  vorr.u16 blend_pixels, blend_pixels, msb_mask;                               \
  vbit.u16 blend_pixels, fb_pixels, draw_mask;                                 \
  vst1.u16 { blend_pixels }, [fb_ptr_next];                                    \
                                                                               \
  restore_abi_regs();                                                          \
  ldmia sp!, { r4, pc };                                                       \
                                                                               \
 2:                                                                            \
  vst1.u16 { blend_pixels }, [fb_ptr];                                         \
  vand.u16 pixels_g, pixels_fourth, d128_0x00E0;                               \
                                                                               \
  vld1.u16 { fb_pixels }, [fb_ptr_next];                                       \
  blend_blocks_add_mask_set_##mask_evaluate();                                 \
  blend_blocks_add_mask_copy_##mask_evaluate();                                \
  vand.u16 fb_pixels_rb, fb_pixels, d128_0x7C1F;                               \
  bal 3b                                                                       \


blend_blocks_add_fourth_textured_builder(off)
blend_blocks_add_fourth_textured_builder(on)
blend_blocks_add_fourth_untextured_builder(off)
blend_blocks_add_fourth_untextured_builder(on)

// TODO: Optimize this more. Need a scene that actually uses it for
// confirmation..

.align 3

function(blend_blocks_textured_unblended_on)         
  stmdb sp!, { r4, r14 }
  save_abi_regs()
  add mask_msb_ptr, psx_gpu, #psx_gpu_mask_msb_offset
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]

  add pixel_ptr, psx_gpu, #(psx_gpu_blocks_offset + 16)
  vld1.u16 { msb_mask_low[], msb_mask_high[] }, [mask_msb_ptr, :16]

  add draw_mask_ptr, psx_gpu, #psx_gpu_blocks_offset
  mov c_64, #64

  ldr fb_ptr, [pixel_ptr, #28]
  vld1.u16 { fb_pixels }, [fb_ptr]
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64
  vclt.s16 write_mask, fb_pixels, #0
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64

  subs num_blocks, num_blocks, #1
  beq 1f

 0:
  vorr.u16 pixels, pixels, msb_mask
  vorr.u16 draw_mask, draw_mask, write_mask
  vbif.u16 fb_pixels, pixels, draw_mask
  vst1.u16 { fb_pixels }, [fb_ptr]

  ldr fb_ptr, [pixel_ptr, #28]
  vld1.u16 { fb_pixels }, [fb_ptr]
  vld1.u32 { draw_mask }, [draw_mask_ptr, :128], c_64
  vclt.s16 write_mask, fb_pixels, #0
  vld1.u32 { pixels }, [pixel_ptr, :128], c_64

  subs num_blocks, num_blocks, #1
  bne 0b
 
 1:
  vorr.u16 pixels, pixels, msb_mask
  vorr.u16 draw_mask, draw_mask, write_mask
  vbif.u16 fb_pixels, pixels, draw_mask
  vst1.u16 { fb_pixels }, [fb_ptr]

  restore_abi_regs()
  ldmia sp!, { r4, pc }


function(blend_blocks_textured_unblended_off)
  bx lr


function(warmup)
  mov r3, #64
  cmp r0, #0
  bxeq lr

 0:
  vld1.u32 { u_whole_8, v_whole_8 }, [r1, :128], r3

  subs r0, r0, #1
  bne 0b

  bx lr

#undef vram_ptr
#undef color
#undef x
#undef y
#undef width
#undef height
#undef fb_ptr
#undef texture_mask
#undef num_blocks
#undef temp
#undef dirty_textures_mask
#undef clut_ptr
#undef current_texture_mask

#define psx_gpu                                           r0
#define x                                                 r1
#define y                                                 r2
#define u                                                 r3
#define v                                                 r4
#define width                                             r5
#define height                                            r6
#define offset_u                                          r8
#define offset_v                                          r9
#define offset_u_right                                    r10
#define width_rounded                                     r11
#define height_rounded                                    r12

#define texture_offset_base                               r1
#define tile_width                                        r2
#define tile_height                                       r3
#define num_blocks                                        r4
#define block                                             r5
#define sub_tile_height                                   r6
#define fb_ptr                                            r7
#define texture_mask                                      r8
#define column_data                                       r9
#define texture_offset                                    r10
#define tiles_remaining                                   r11
#define fb_ptr_advance_column                             r12
#define texture_block_ptr                                 r14

#define temp                                              r14

#define texture_page_ptr                                  r3
#define left_block_mask                                   r4
#define right_block_mask                                  r5
#define texture_mask_rev                                  r10
#define control_mask                                      r11

#define dirty_textures_mask                               r4
#define clut_ptr                                          r5
#define current_texture_mask                              r6


#undef texels
#undef clut_low_a
#undef clut_low_b
#undef clut_high_a
#undef clut_high_b
#undef clut_a
#undef clut_b
#undef texels_low
#undef texels_high

#define texels                                            d0
#define draw_masks_fb_ptrs                                q1

#define draw_mask_fb_ptr_left                             d2
#define draw_mask_fb_ptr_right                            d3

#define draw_mask_fb_ptr_left_a                           d2
#define draw_mask_fb_ptr_left_b                           d3
#define draw_mask_fb_ptr_right_a                          d10
#define draw_mask_fb_ptr_right_b                          d11
#define draw_masks_fb_ptrs2                               q5

#define clut_low_a                                        d4
#define clut_low_b                                        d5
#define clut_high_a                                       d6
#define clut_high_b                                       d7

#define block_masks                                       d8
#define block_masks_shifted                               d9

#define clut_a                                            q2
#define clut_b                                            q3

#define texels_low                                        d12
#define texels_high                                       d13

#define texels_wide_low                                   d14
#define texels_wide_high                                  d15
#define texels_wide                                       q7

.align 3

setup_sprite_flush_blocks:
  push   { r0 - r3, EXTRA_UNSAVED_REGS r12, lr }
  add    block, r0, #psx_gpu_saved_tmp_offset        /* r5 */
  vstmia block, { q1 - q3 }
  bl     flush_render_block_buffer
  vldmia block, { q1 - q3 }
  pop    { r0 - r3, EXTRA_UNSAVED_REGS r12, lr }

  add    block, psx_gpu, #psx_gpu_blocks_offset
  bx     lr


setup_sprite_update_texture_4bpp_cache:
  push { r0 - r4, lr }
  bl update_texture_4bpp_cache
  pop  { r0 - r4, pc }


setup_sprite_update_texture_8bpp_cache:
  push { r0 - r4, EXTRA_UNSAVED_REGS lr }
  bl update_texture_8bpp_cache
  pop  { r0 - r4, EXTRA_UNSAVED_REGS pc }


#define setup_sprite_tiled_initialize_4bpp()                                   \
  ldr dirty_textures_mask,                                                     \
   [psx_gpu, #psx_gpu_dirty_textures_4bpp_mask_offset];                        \
  ldr clut_ptr, [psx_gpu, #psx_gpu_clut_ptr_offset];                           \
                                                                               \
  ldr current_texture_mask, [psx_gpu, #psx_gpu_current_texture_mask_offset];   \
  vld1.u32 { clut_a, clut_b }, [clut_ptr, :128];                               \
                                                                               \
  tst current_texture_mask, dirty_textures_mask;                               \
  vuzp.u8 clut_a, clut_b;                                                      \
                                                                               \
  blne setup_sprite_update_texture_4bpp_cache                                  \

#define setup_sprite_tiled_initialize_8bpp()                                   \
  ldr dirty_textures_mask,                                                     \
   [psx_gpu, #psx_gpu_dirty_textures_8bpp_mask_offset];                        \
  ldr current_texture_mask, [psx_gpu, #psx_gpu_current_texture_mask_offset];   \
                                                                               \
  tst current_texture_mask, dirty_textures_mask;                               \
  blne setup_sprite_update_texture_8bpp_cache                                  \


#define setup_sprite_block_count_single()                                      \
  sub_tile_height                                                              \

#define setup_sprite_block_count_double()                                      \
  sub_tile_height, lsl #1                                                      \

#define setup_sprite_tile_add_blocks(type)                                     \
  add num_blocks, num_blocks, setup_sprite_block_count_##type();               \
  cmp num_blocks, #MAX_BLOCKS;                                                 \
                                                                               \
  movgt num_blocks, setup_sprite_block_count_##type();                         \
  blgt setup_sprite_flush_blocks                                               \


#define setup_sprite_tile_full_4bpp(edge)                                      \
  setup_sprite_tile_add_blocks(double);                                        \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  vmov.u32 draw_mask_fb_ptr_left[1], fb_ptr;                                   \
                                                                               \
  pld [fb_ptr];                                                                \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels;                       \
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels;                    \
                                                                               \
  vst2.u8 { texels_low, texels_high }, [block, :128];                          \
  add texture_block_ptr, texture_offset, #8;                                   \
                                                                               \
  and texture_block_ptr, texture_block_ptr, texture_mask;                      \
  add block, block, #40;                                                       \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  add fb_ptr, fb_ptr, #16;                                                     \
                                                                               \
  vst1.u32 { draw_mask_fb_ptr_left }, [block, :64];                            \
  add block, block, #24;                                                       \
                                                                               \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels;                       \
                                                                               \
  pld [fb_ptr];                                                                \
  vmov.u32 draw_mask_fb_ptr_right[1], fb_ptr;                                  \
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels;                    \
                                                                               \
  vst2.u8 { texels_low, texels_high }, [block, :128];                          \
  add block, block, #40;                                                       \
                                                                               \
  add texture_offset, texture_offset, #0x10;                                   \
  add fb_ptr, fb_ptr, #(2048 - 16);                                            \
                                                                               \
  vst1.u32 { draw_mask_fb_ptr_right }, [block, :64];                           \
  add block, block, #24;                                                       \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \

  
#define setup_sprite_tile_half_4bpp(edge)                                      \
  setup_sprite_tile_add_blocks(single);                                        \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  vmov.u32 draw_mask_fb_ptr_##edge[1], fb_ptr;                                 \
                                                                               \
  pld [fb_ptr];                                                                \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels;                       \
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels;                    \
                                                                               \
  vst2.u8 { texels_low, texels_high }, [block, :128];                          \
  add block, block, #40;                                                       \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vst1.u32 { draw_mask_fb_ptr_##edge }, [block, :64];                          \
                                                                               \
  add block, block, #24;                                                       \
  add texture_offset, texture_offset, #0x10;                                   \
                                                                               \
  add fb_ptr, fb_ptr, #2048;                                                   \
  subs sub_tile_height, sub_tile_height, #1;                                   \
                                                                               \
  bne 4b;                                                                      \
                                                                               \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \
 
 
#define setup_sprite_tile_full_8bpp(edge)                                      \
  setup_sprite_tile_add_blocks(double);                                        \
  add block, block, #16;                                                       \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  vmov.u32 draw_mask_fb_ptr_left[1], fb_ptr;                                   \
                                                                               \
  pld [fb_ptr];                                                                \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  add texture_block_ptr, texture_offset, #8;                                   \
  vst1.u32 { texels }, [block, :64];                                           \
                                                                               \
  and texture_block_ptr, texture_block_ptr, texture_mask;                      \
  add block, block, #24;                                                       \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
                                                                               \
  add fb_ptr, fb_ptr, #16;                                                     \
  vst1.u32 { draw_mask_fb_ptr_left }, [block, :64];                            \
                                                                               \
  add block, block, #40;                                                       \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
  pld [fb_ptr];                                                                \
                                                                               \
  vmov.u32 draw_mask_fb_ptr_right[1], fb_ptr;                                  \
  vst1.u32 { texels }, [block, :64];                                           \
  add block, block, #24;                                                       \
                                                                               \
  add texture_offset, texture_offset, #0x10;                                   \
  add fb_ptr, fb_ptr, #(2048 - 16);                                            \
                                                                               \
  vst1.u32 { draw_mask_fb_ptr_right }, [block, :64];                           \
  add block, block, #40;                                                       \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  sub block, block, #16;                                                       \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \

  
#define setup_sprite_tile_half_8bpp(edge)                                      \
  setup_sprite_tile_add_blocks(single);                                        \
  add block, block, #16;                                                       \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  vmov.u32 draw_mask_fb_ptr_##edge[1], fb_ptr;                                 \
  pld [fb_ptr];                                                                \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  vst1.u32 { texels }, [block, :64];                                           \
  add block, block, #24;                                                       \
                                                                               \
  vst1.u32 { draw_mask_fb_ptr_##edge }, [block, :64];                          \
  add block, block, #40;                                                       \
                                                                               \
  add texture_offset, texture_offset, #0x10;                                   \
  add fb_ptr, fb_ptr, #2048;                                                   \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  sub block, block, #16;                                                       \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \

 
#define setup_sprite_tile_column_edge_pre_adjust_half_right()                  \
  add texture_offset, texture_offset_base, #8;                                 \
  add fb_ptr, fb_ptr, #16                                                      \

#define setup_sprite_tile_column_edge_pre_adjust_half_left()                   \
  mov texture_offset, texture_offset_base                                      \

#define setup_sprite_tile_column_edge_pre_adjust_half(edge)                    \
  setup_sprite_tile_column_edge_pre_adjust_half_##edge()                       \

#define setup_sprite_tile_column_edge_pre_adjust_full(edge)                    \
  mov texture_offset, texture_offset_base                                      \

#define setup_sprite_tile_column_edge_post_adjust_half_right()                 \
  sub fb_ptr, fb_ptr, #16                                                      \

#define setup_sprite_tile_column_edge_post_adjust_half_left()                  \

#define setup_sprite_tile_column_edge_post_adjust_half(edge)                   \
  setup_sprite_tile_column_edge_post_adjust_half_##edge()                      \

#define setup_sprite_tile_column_edge_post_adjust_full(edge)                   \


#define setup_sprite_tile_column_height_single(edge_mode, edge, texture_mode,  \
 x4mode)                                                                       \
  mov sub_tile_height, column_data;                                            \
  setup_sprite_tile_column_edge_pre_adjust_##edge_mode##x4mode(edge);          \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
  setup_sprite_tile_column_edge_post_adjust_##edge_mode##x4mode(edge)          \

#define setup_sprite_tile_column_height_multi(edge_mode, edge, texture_mode,   \
 x4mode)                                                                       \
  and sub_tile_height, column_data, #0xFF;                                     \
  mov tiles_remaining, column_data, lsr #16;                                   \
  setup_sprite_tile_column_edge_pre_adjust_##edge_mode##x4mode(edge);          \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
                                                                               \
  subs tiles_remaining, tiles_remaining, #1;                                   \
  beq 2f;                                                                      \
                                                                               \
 3:                                                                            \
  mov sub_tile_height, #16;                                                    \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
  subs tiles_remaining, tiles_remaining, #1;                                   \
  bne 3b;                                                                      \
                                                                               \
 2:                                                                            \
  uxtb sub_tile_height, column_data, ror #8;                                   \
  setup_sprite_tile_##edge_mode##_##texture_mode##x4mode(edge);                \
  setup_sprite_tile_column_edge_post_adjust_##edge_mode##x4mode(edge)          \


#define setup_sprite_column_data_single()                                      \
  mov column_data, height;                                                     \
  ldr texture_page_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset]            \

#define setup_sprite_column_data_multi()                                       \
  and height_rounded, height_rounded, #0xF;                                    \
  rsb column_data, offset_v, #16;                                              \
                                                                               \
  add height_rounded, height_rounded, #1;                                      \
  sub tile_height, tile_height, #1;                                            \
                                                                               \
  orr column_data, column_data, tile_height, lsl #16;                          \
  ldr texture_page_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset];           \
                                                                               \
  orr column_data, column_data, height_rounded, lsl #8                         \

#define setup_sprite_setup_left_draw_mask_fb_ptr()                             \
  vdup.u8 draw_mask_fb_ptr_left, block_masks[0];                               \
  vdup.u8 draw_mask_fb_ptr_right, block_masks[1]                               \

#define setup_sprite_setup_left_draw_mask_fb_ptr_advance_column()              \
  mov fb_ptr_advance_column, #32;                                              \
  vdup.u8 draw_mask_fb_ptr_left, block_masks[0];                               \
                                                                               \
  sub fb_ptr_advance_column, fb_ptr_advance_column, height, lsl #11;           \
  vdup.u8 draw_mask_fb_ptr_right, block_masks[1]                               \

#define setup_sprite_setup_right_draw_mask_fb_ptr()                            \
  vdup.u8 draw_mask_fb_ptr_left, block_masks[4];                               \
  vdup.u8 draw_mask_fb_ptr_right, block_masks[5]                               \

#define setup_sprite_tile_column_width_single(tm, multi_height, edge_mode,     \
 edge, x4mode)                                                                 \
 setup_sprite_##tm##_single_##multi_height##_##edge_mode##_##edge##x4mode:     \
  setup_sprite_column_data_##multi_height();                                   \
  vext.32 block_masks_shifted, block_masks, block_masks, #1;                   \
  vorr.u32 block_masks, block_masks, block_masks_shifted;                      \
  setup_sprite_setup_left_draw_mask_fb_ptr##x4mode();                          \
                                                                               \
  setup_sprite_tile_column_height_##multi_height(edge_mode, edge, tm, x4mode); \
  vpop { q4 - q7 };                                                            \
  pop  { r3 - r11, pc }                                                        \

#define setup_sprite_tiled_advance_column()                                    \
  add texture_offset_base, texture_offset_base, #0x100;                        \
  tst texture_offset_base, #0xF00;                                             \
  subeq texture_offset_base, texture_offset_base, #(0x100 + 0xF00)             \

#define setup_sprite_tile_column_width_multi(tm, multi_height, left_mode,      \
 right_mode, x4mode)                                                           \
 setup_sprite_##tm##_multi_##multi_height##_##left_mode##_##right_mode##x4mode:\
  setup_sprite_column_data_##multi_height();                                   \
                                                                               \
  setup_sprite_setup_left_draw_mask_fb_ptr_advance_column##x4mode();           \
                                                                               \
  setup_sprite_tile_column_height_##multi_height(left_mode, right, tm, x4mode);\
                                                                               \
  subs tile_width, tile_width, #2;                                             \
  add fb_ptr, fb_ptr, fb_ptr_advance_column;                                   \
                                                                               \
  beq 1f;                                                                      \
                                                                               \
  vmov.u8 draw_masks_fb_ptrs, #0;                                              \
  vmov.u8 draw_masks_fb_ptrs2, #0;                                             \
                                                                               \
 0:                                                                            \
  setup_sprite_tiled_advance_column();                                         \
  setup_sprite_tile_column_height_##multi_height(full, none, tm, x4mode);      \
  add fb_ptr, fb_ptr, fb_ptr_advance_column;                                   \
  subs tile_width, tile_width, #1;                                             \
  bne 0b;                                                                      \
                                                                               \
 1:                                                                            \
  setup_sprite_setup_right_draw_mask_fb_ptr##x4mode();                         \
                                                                               \
  setup_sprite_tiled_advance_column();                                         \
  setup_sprite_tile_column_height_##multi_height(right_mode, left, tm, x4mode);\
  vpop { q4 - q7 };                                                            \
  pop  { r3 - r11, pc }                                                        \


#define setup_sprite_offset_u_adjust()                                         \

#define setup_sprite_get_left_block_mask()                                     \
  and left_block_mask, left_block_mask, #0xFF                                  \

#define setup_sprite_compare_left_block_mask()                                 \
  cmp left_block_mask, #0xFF                                                   \

#define setup_sprite_get_right_block_mask()                                    \
  uxtb right_block_mask, right_block_mask, ror #8                              \

#define setup_sprite_compare_right_block_mask()                                \
  cmp right_block_mask, #0xFF                                                  \



/* 4x stuff */
#define fb_ptr2 column_data

#define setup_sprite_offset_u_adjust_4x()                                      \
  sub fb_ptr, fb_ptr, offset_u, lsl #1;                                        \
  lsl offset_u_right, #1;                                                      \
  lsl offset_u, #1;                                                            \
  add offset_u_right, #1                                                       \

#define setup_sprite_get_left_block_mask_4x()                                  \
  sxth left_block_mask, left_block_mask                                        \

#define setup_sprite_compare_left_block_mask_4x()                              \
  cmp left_block_mask, #0xFFFFFFFF                                             \

#define setup_sprite_get_right_block_mask_4x()                                 \
  sxth right_block_mask, right_block_mask, ror #16                             \

#define setup_sprite_compare_right_block_mask_4x()                             \
  cmp right_block_mask, #0xFFFFFFFF                                            \


#define widen_texels_16bpp(texels_)                                            \
  vmov texels_wide_low, texels_;                                               \
  vmov texels_wide_high, texels_;                                              \
  vzip.16 texels_wide_low, texels_wide_high                                    \

#define widen_texels_8bpp(texels_)                                             \
  vmov texels_wide_low, texels_;                                               \
  vmov texels_wide_high, texels_;                                              \
  vzip.8 texels_wide_low, texels_wide_high                                     \

#define write_block_16bpp(texels_, block_, draw_mask_fb_ptr_, fb_ptr_)         \
  vst1.u32 { texels_ }, [block_, :128];                                        \
  add block_, block_, #40;                                                     \
                                                                               \
  vmov.u32 draw_mask_fb_ptr_[1], fb_ptr_;                                      \
  vst1.u32 { draw_mask_fb_ptr_ }, [block_, :64];                               \
  add block_, block_, #24                                                      \

/* assumes 16-byte offset already added to block_ */
#define write_block_8bpp(texels_, block_, draw_mask_fb_ptr_, fb_ptr_)          \
  vst1.u32 { texels_ }, [block_, :64];                                         \
  add block_, block_, #24;                                                     \
                                                                               \
  vmov.u32 draw_mask_fb_ptr_[1], fb_ptr_;                                      \
  vst1.u32 { draw_mask_fb_ptr_ }, [block_, :64];                               \
  add block_, block_, #40                                                      \

#define do_texture_block_16bpp_4x(fb_ptr_tmp, draw_mask_fb_ptr_a_,             \
 draw_mask_fb_ptr_b_)                                                          \
  widen_texels_16bpp(texels_low);                                              \
  add fb_ptr_tmp, fb_ptr, #1024*2;                                             \
                                                                               \
  write_block_16bpp(texels_wide, block, draw_mask_fb_ptr_a_, fb_ptr);          \
                                                                               \
  write_block_16bpp(texels_wide, block, draw_mask_fb_ptr_a_, fb_ptr_tmp);      \
  widen_texels_16bpp(texels_high);                                             \
                                                                               \
  add fb_ptr_tmp, fb_ptr, #8*2;                                                \
  write_block_16bpp(texels_wide, block, draw_mask_fb_ptr_b_, fb_ptr_tmp);      \
                                                                               \
  add fb_ptr_tmp, fb_ptr_tmp, #1024*2;                                         \
  write_block_16bpp(texels_wide, block, draw_mask_fb_ptr_b_, fb_ptr_tmp)       \

#define do_texture_block_8bpp_4x(fb_ptr_tmp, draw_mask_fb_ptr_a_,              \
 draw_mask_fb_ptr_b_)                                                          \
  widen_texels_8bpp(texels);                                                   \
  add fb_ptr_tmp, fb_ptr, #1024*2;                                             \
                                                                               \
  write_block_8bpp(texels_wide_low, block, draw_mask_fb_ptr_a_, fb_ptr);       \
  write_block_8bpp(texels_wide_low, block, draw_mask_fb_ptr_a_, fb_ptr_tmp);   \
                                                                               \
  add fb_ptr_tmp, fb_ptr, #8*2;                                                \
  write_block_8bpp(texels_wide_high, block, draw_mask_fb_ptr_b_, fb_ptr_tmp);  \
                                                                               \
  add fb_ptr_tmp, fb_ptr_tmp, #1024*2;                                         \
  write_block_8bpp(texels_wide_high, block, draw_mask_fb_ptr_b_, fb_ptr_tmp)   \


#define setup_sprite_tiled_initialize_4bpp_4x()                                \
  ldr clut_ptr, [psx_gpu, #psx_gpu_clut_ptr_offset];                           \
  vld1.u32 { clut_a, clut_b }, [clut_ptr, :128];                               \
                                                                               \
  vuzp.u8 clut_a, clut_b                                                       \

#define setup_sprite_tiled_initialize_8bpp_4x()                                \


#define setup_sprite_block_count_single_4x()                                   \
  sub_tile_height, lsl #2                                                      \

#define setup_sprite_block_count_double_4x()                                   \
  sub_tile_height, lsl #(1+2)                                                  \

#define setup_sprite_tile_full_4bpp_4x(edge)                                   \
  setup_sprite_tile_add_blocks(double_4x);                                     \
  str column_data, [sp, #-8]!; /* fb_ptr2 */                                   \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  pld [fb_ptr];                                                                \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  add texture_block_ptr, texture_offset, #8;                                   \
  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels;                       \
                                                                               \
  and texture_block_ptr, texture_block_ptr, texture_mask;                      \
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels;                    \
                                                                               \
  vzip.8 texels_low, texels_high;                                              \
  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_left_a,                  \
   draw_mask_fb_ptr_left_b);                                                   \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  pld [fb_ptr, #2048];                                                         \
                                                                               \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
  add fb_ptr, fb_ptr, #16*2;                                                   \
                                                                               \
  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels;                       \
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels;                    \
                                                                               \
  vzip.8 texels_low, texels_high;                                              \
  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_right_a,                 \
   draw_mask_fb_ptr_right_b);                                                  \
                                                                               \
  add texture_offset, texture_offset, #0x10;                                   \
  add fb_ptr, fb_ptr, #(2048 - 16) * 2;                                        \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  ldr column_data, [sp], #8; /* fb_ptr2 */                                     \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \


#define setup_sprite_tile_half_4bpp_4x(edge)                                   \
  setup_sprite_tile_add_blocks(single_4x);                                     \
  str column_data, [sp, #-8]!; /* fb_ptr2 */                                   \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  pld [fb_ptr];                                                                \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vtbl.8 texels_low, { clut_low_a, clut_low_b }, texels;                       \
                                                                               \
  vtbl.8 texels_high, { clut_high_a, clut_high_b }, texels;                    \
  add texture_offset, texture_offset, #0x10;                                   \
                                                                               \
  vzip.8 texels_low, texels_high;                                              \
  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_##edge##_a,              \
   draw_mask_fb_ptr_##edge##_b);                                               \
                                                                               \
  pld [fb_ptr, #2048];                                                         \
  add fb_ptr, fb_ptr, #2048 * 2;                                               \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  ldr column_data, [sp], #8; /* fb_ptr2 */                                     \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \


#define setup_sprite_tile_full_8bpp_4x(edge)                                   \
  setup_sprite_tile_add_blocks(double_4x);                                     \
  add block, block, #16;                                                       \
  str column_data, [sp, #-8]!; /* fb_ptr2 */                                   \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  pld [fb_ptr];                                                                \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  add texture_block_ptr, texture_offset, #8;                                   \
  do_texture_block_8bpp_4x(fb_ptr2, draw_mask_fb_ptr_left_a,                   \
   draw_mask_fb_ptr_left_b);                                                   \
                                                                               \
  pld [fb_ptr, #2048];                                                         \
  and texture_block_ptr, texture_block_ptr, texture_mask;                      \
                                                                               \
  add fb_ptr, fb_ptr, #16*2;                                                   \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
                                                                               \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  do_texture_block_8bpp_4x(fb_ptr2, draw_mask_fb_ptr_right_a,                  \
   draw_mask_fb_ptr_right_b);                                                  \
                                                                               \
  add texture_offset, texture_offset, #0x10;                                   \
  add fb_ptr, fb_ptr, #(2048 - 16) * 2;                                        \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  sub block, block, #16;                                                       \
  ldr column_data, [sp], #8; /* fb_ptr2 */                                     \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \

  
#define setup_sprite_tile_half_8bpp_4x(edge)                                   \
  setup_sprite_tile_add_blocks(single_4x);                                     \
  add block, block, #16;                                                       \
  str column_data, [sp, #-8]!; /* fb_ptr2 */                                   \
                                                                               \
 4:                                                                            \
  and texture_block_ptr, texture_offset, texture_mask;                         \
  pld [fb_ptr];                                                                \
                                                                               \
  add texture_block_ptr, texture_page_ptr, texture_block_ptr;                  \
  vld1.u32 { texels }, [texture_block_ptr, :64];                               \
                                                                               \
  pld [fb_ptr, #2048];                                                         \
  do_texture_block_8bpp_4x(fb_ptr2, draw_mask_fb_ptr_##edge##_a,               \
   draw_mask_fb_ptr_##edge##_b);                                               \
                                                                               \
  add texture_offset, texture_offset, #0x10;                                   \
  add fb_ptr, fb_ptr, #2048 * 2;                                               \
                                                                               \
  subs sub_tile_height, sub_tile_height, #1;                                   \
  bne 4b;                                                                      \
                                                                               \
  sub block, block, #16;                                                       \
  ldr column_data, [sp], #8; /* fb_ptr2 */                                     \
  add texture_offset, texture_offset, #0xF00;                                  \
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]                       \

 
#define setup_sprite_tile_column_edge_pre_adjust_half_right_4x()               \
  add texture_offset, texture_offset_base, #8;                                 \
  add fb_ptr, fb_ptr, #16 * 2                                                  \

#define setup_sprite_tile_column_edge_pre_adjust_half_left_4x()                \
  mov texture_offset, texture_offset_base                                      \

#define setup_sprite_tile_column_edge_pre_adjust_half_4x(edge)                 \
  setup_sprite_tile_column_edge_pre_adjust_half_##edge##_4x()                  \

#define setup_sprite_tile_column_edge_pre_adjust_full_4x(edge)                 \
  mov texture_offset, texture_offset_base                                      \

#define setup_sprite_tile_column_edge_post_adjust_half_right_4x()              \
  sub fb_ptr, fb_ptr, #16 * 2                                                  \

#define setup_sprite_tile_column_edge_post_adjust_half_left_4x()               \

#define setup_sprite_tile_column_edge_post_adjust_half_4x(edge)                \
  setup_sprite_tile_column_edge_post_adjust_half_##edge##_4x()                 \

#define setup_sprite_tile_column_edge_post_adjust_full_4x(edge)                \


#define setup_sprite_setup_left_draw_mask_fb_ptr_4x()                          \
  vdup.u8 draw_mask_fb_ptr_left_a, block_masks[0];                             \
  vdup.u8 draw_mask_fb_ptr_left_b, block_masks[1];                             \
  vdup.u8 draw_mask_fb_ptr_right_a, block_masks[2];                            \
  vdup.u8 draw_mask_fb_ptr_right_b, block_masks[3]                             \

#define setup_sprite_setup_left_draw_mask_fb_ptr_advance_column_4x()           \
  mov fb_ptr_advance_column, #32 * 2;                                          \
  vdup.u8 draw_mask_fb_ptr_left_a, block_masks[0];                             \
  vdup.u8 draw_mask_fb_ptr_left_b, block_masks[1];                             \
  sub fb_ptr_advance_column, fb_ptr_advance_column, height, lsl #11 + 1;       \
  vdup.u8 draw_mask_fb_ptr_right_a, block_masks[2];                            \
  vdup.u8 draw_mask_fb_ptr_right_b, block_masks[3]                             \

#define setup_sprite_setup_right_draw_mask_fb_ptr_4x()                         \
  vdup.u8 draw_mask_fb_ptr_left_a, block_masks[4];                             \
  vdup.u8 draw_mask_fb_ptr_left_b, block_masks[5];                             \
  vdup.u8 draw_mask_fb_ptr_right_a, block_masks[6];                            \
  vdup.u8 draw_mask_fb_ptr_right_b, block_masks[7]                             \


// r0: psx_gpu
// r1: x
// r2: y
// r3: u
// [sp]: v
// [sp + 4]: width
// [sp + 8]: height
// [sp + 12]: color (unused)

#define setup_sprite_tiled_builder(texture_mode, x4mode)                       \
                                                                               \
setup_sprite_tile_column_width_multi(texture_mode,  multi,  full, full,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_single(texture_mode, multi,  full, none,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  single, full, full,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_single(texture_mode, single, full, none,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  multi,  half, full,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_single(texture_mode, multi,  half, right,       \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  single, half, full,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_single(texture_mode, single, half, right,       \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  multi,  full, half,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_single(texture_mode, multi,  half, left,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  single, full, half,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_single(texture_mode, single, half, left,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  multi,  half, half,        \
  x4mode);                                                                     \
setup_sprite_tile_column_width_multi(texture_mode,  single, half, half,        \
  x4mode);                                                                     \
                                                                               \
.align 4;                                                                      \
                                                                               \
function(setup_sprite_##texture_mode##x4mode)                                  \
  push { r3 - r11, lr };                                                       \
  setup_sprite_tiled_initialize_##texture_mode##x4mode();                      \
                                                                               \
  ldr v, [sp, #4*(10+0)];                                                      \
  and offset_u, u, #0xF;                                                       \
                                                                               \
  ldr width, [sp, #4*(10+1)];                                                  \
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset];                         \
                                                                               \
  ldr height, [sp, #4*(10+2)];                                                 \
  add fb_ptr, fb_ptr, y, lsl #11;                                              \
                                                                               \
  vpush { q4 - q7 };                                                           \
                                                                               \
  add fb_ptr, fb_ptr, x, lsl #1;                                               \
  and offset_v, v, #0xF;                                                       \
                                                                               \
  sub fb_ptr, fb_ptr, offset_u, lsl #1;                                        \
  add width_rounded, offset_u, width;                                          \
                                                                               \
  add height_rounded, offset_v, height;                                        \
  add width_rounded, width_rounded, #15;                                       \
                                                                               \
  add height_rounded, height_rounded, #15;                                     \
  mov tile_width, width_rounded, lsr #4;                                       \
                                                                               \
  /* texture_offset_base = VH-VL-00-00                                       */\
  mov texture_offset_base, v, lsl #8;                                          \
  and offset_u_right, width_rounded, #0xF;                                     \
                                                                               \
  /* texture_offset_base = VH-UH-UL-00                                       */\
  bfi texture_offset_base, u, #4, #8;                                          \
  mov right_block_mask, #0xFFFFFFFE;                                           \
                                                                               \
  setup_sprite_offset_u_adjust##x4mode();                                      \
                                                                               \
  /* texture_offset_base = VH-UH-VL-00                                       */\
  bfi texture_offset_base, v, #4, #4;                                          \
  mov left_block_mask, #0xFFFFFFFF;                                            \
                                                                               \
  mov tile_height, height_rounded, lsr #4;                                     \
  mvn left_block_mask, left_block_mask, lsl offset_u;                          \
                                                                               \
  /* texture_mask = HH-HL-WH-WL                                              */\
  ldrh texture_mask, [psx_gpu, #psx_gpu_texture_mask_width_offset];            \
  mov right_block_mask, right_block_mask, lsl offset_u_right;                  \
                                                                               \
  /* texture_mask_rev = WH-WL-HH-HL                                          */\
  rev16 texture_mask_rev, texture_mask;                                        \
  vmov block_masks, left_block_mask, right_block_mask;                         \
                                                                               \
  /* texture_mask = HH-HL-HL-WL                                              */\
  bfi texture_mask, texture_mask_rev, #4, #4;                                  \
  /* texture_mask_rev = 00-00-00-WH                                          */\
  mov texture_mask_rev, texture_mask_rev, lsr #12;                             \
                                                                               \
  /* texture_mask = HH-WH-HL-WL                                              */\
  bfi texture_mask, texture_mask_rev, #8, #4;                                  \
  setup_sprite_get_left_block_mask##x4mode();                                  \
                                                                               \
  mov control_mask, #0;                                                        \
  setup_sprite_compare_left_block_mask##x4mode();                              \
                                                                               \
  setup_sprite_get_right_block_mask##x4mode();                                 \
  orreq control_mask, control_mask, #0x4;                                      \
                                                                               \
  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset];                      \
  setup_sprite_compare_right_block_mask##x4mode();                             \
                                                                               \
  orreq control_mask, control_mask, #0x8;                                      \
  cmp tile_width, #1;                                                          \
                                                                               \
  add block, psx_gpu, #psx_gpu_blocks_offset;                                  \
  orreq control_mask, control_mask, #0x1;                                      \
                                                                               \
  cmp tile_height, #1;                                                         \
  add block, block, num_blocks, lsl #6;                                        \
                                                                               \
  orreq control_mask, control_mask, #0x2;                                      \
  JT_OP_REL(9f, control_mask, temp);                                           \
  JT_OP(ldr pc, [pc, control_mask, lsl #2]);                                   \
  nop;                                                                         \
                                                                               \
 9:                                                                            \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_multi_full_full##x4mode);   \
 .word JTE(9b, setup_sprite_##texture_mode##_single_multi_full_none##x4mode);  \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_single_full_full##x4mode);  \
 .word JTE(9b, setup_sprite_##texture_mode##_single_single_full_none##x4mode); \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_multi_half_full##x4mode);   \
 .word JTE(9b, setup_sprite_##texture_mode##_single_multi_half_right##x4mode); \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_single_half_full##x4mode);  \
 .word JTE(9b, setup_sprite_##texture_mode##_single_single_half_right##x4mode);\
 .word JTE(9b, setup_sprite_##texture_mode##_multi_multi_full_half##x4mode);   \
 .word JTE(9b, setup_sprite_##texture_mode##_single_multi_half_left##x4mode);  \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_single_full_half##x4mode);  \
 .word JTE(9b, setup_sprite_##texture_mode##_single_single_half_left##x4mode); \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_multi_half_half##x4mode);   \
 .word 0x00000000;                                                             \
 .word JTE(9b, setup_sprite_##texture_mode##_multi_single_half_half##x4mode);  \


setup_sprite_tiled_builder(4bpp,);
setup_sprite_tiled_builder(8bpp,);

#undef draw_mask_fb_ptr_left
#undef draw_mask_fb_ptr_right

setup_sprite_tiled_builder(4bpp, _4x);
setup_sprite_tiled_builder(8bpp, _4x);


#undef block_ptr
#undef num_blocks
#undef clut_ptr

#define psx_gpu                                           r0
#define block_ptr                                         r0
#define num_blocks                                        r1
#define clut_ptr                                          r2
#define texel_shift_mask                                  r3
#define block_pixels_a                                    r4
#define block_pixels_b                                    r5
#define texel_0                                           r6
#define texel_2                                           r7
#define texel_4                                           r8
#define texel_6                                           r9
#define texel_1                                           r10
#define texel_3                                           r11
#define texel_5                                           r12
#define texel_7                                           r14
#define texels_01                                         r6
#define texels_23                                         r7
#define texels_45                                         r8
#define texels_67                                         r9

function(texture_sprite_blocks_8bpp)
  push { r4 - r11, r14 }
  movw texel_shift_mask, #(0xFF << 1)

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  ldr clut_ptr, [psx_gpu, #psx_gpu_clut_ptr_offset]

  add block_ptr, psx_gpu, #psx_gpu_blocks_offset
  ldr block_pixels_a, [block_ptr, #16]

 0:
  and texel_0, texel_shift_mask, block_pixels_a, lsl #1
  ldr block_pixels_b, [block_ptr, #20]

  and texel_1, texel_shift_mask, block_pixels_a, lsr #7
  ldrh texel_0, [clut_ptr, texel_0]

  and texel_2, texel_shift_mask, block_pixels_a, lsr #15
  ldrh texel_1, [clut_ptr, texel_1]

  and texel_3, texel_shift_mask, block_pixels_a, lsr #23
  ldr block_pixels_a, [block_ptr, #(64 + 16)]

  ldrh texel_2, [clut_ptr, texel_2]
  and texel_4, texel_shift_mask, block_pixels_b, lsl #1

  ldrh texel_3, [clut_ptr, texel_3]
  and texel_5, texel_shift_mask, block_pixels_b, lsr #7

  ldrh texel_4, [clut_ptr, texel_4]
  and texel_6, texel_shift_mask, block_pixels_b, lsr #15

  ldrh texel_5, [clut_ptr, texel_5]
  and texel_7, texel_shift_mask, block_pixels_b, lsr #23

  ldrh texel_6, [clut_ptr, texel_6]
  orr texels_01, texel_0, texel_1, lsl #16

  ldrh texel_7, [clut_ptr, texel_7]
  orr texels_23, texel_2, texel_3, lsl #16

  orr texels_45, texel_4, texel_5, lsl #16
  str texels_01, [block_ptr, #0]

  orr texels_67, texel_6, texel_7, lsl #16
  str texels_23, [block_ptr, #4]

  subs num_blocks, num_blocks, #1
  str texels_45, [block_ptr, #8]

  str texels_67, [block_ptr, #12]
  add block_ptr, block_ptr, #64

  bne 0b
  nop

  pop { r4 - r11, pc }


#undef width_rounded
#undef texture_mask
#undef num_blocks
#undef texture_offset
#undef texels_low
#undef texels_high
#undef texels_wide_low
#undef texels_wide_high
#undef texels_wide
#undef fb_ptr2
#undef temp

#define psx_gpu                                           r0
#define x                                                 r1
#define y                                                 r2
#define u                                                 r3
#define v                                                 r4
#define width                                             r5
#define height                                            r6
#define left_offset                                       r8
#define width_rounded                                     r9
#define right_width                                       r10

#define block_width                                       r11

#define texture_offset_base                               r1
#define texture_mask                                      r2
#define texture_page_ptr                                  r3
#define num_blocks                                        r4
#define block                                             r5
#define fb_ptr                                            r7
#define texture_offset                                    r8
#define blocks_remaining                                  r9
#define fb_ptr2                                           r10
#define fb_ptr_pitch                                      r12
#define texture_block_ptr                                 r14

#define texture_mask_width                                r2
#define texture_mask_height                               r3
#define left_mask_bits                                    r4
#define right_mask_bits                                   r5


#undef block_masks
#undef block_masks_shifted
#undef texels

#define block_masks                                       d0
#define block_masks_shifted                               d1
#define draw_mask_fb_ptr                                  d2
#define texels                                            q2

#define draw_mask_fb_ptr_a                                d2
#define draw_mask_fb_ptr_b                                d3
#define texels_low                                        d4
#define texels_high                                       d5
#define texels_wide_low                                   d6
#define texels_wide_high                                  d7
#define texels_wide                                       q3


setup_sprites_16bpp_flush:
  push   { r0 - r3, EXTRA_UNSAVED_REGS r12, r14 }
  add    r1, r0, #psx_gpu_saved_tmp_offset
  vstmia r1, { d0 - d3 }
  bl     flush_render_block_buffer
  pop    { r0 - r3, EXTRA_UNSAVED_REGS r12 }
  add    lr, r0, #psx_gpu_saved_tmp_offset
  vldmia lr, { d0 - d3 }

  add block, psx_gpu, #psx_gpu_blocks_offset
  mov num_blocks, block_width

  pop { pc }

function(setup_sprite_16bpp)
  push { r3 - r11, lr }
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset]

  ldr v, [sp, #4*(10+0)]
  add fb_ptr, fb_ptr, y, lsl #11

  ldr width, [sp, #4*(10+1)]
  add fb_ptr, fb_ptr, x, lsl #1

  ldr height, [sp, #4*(10+2)]
  and left_offset, u, #0x7

  add texture_offset_base, u, u
  add width_rounded, width, #7

  add texture_offset_base, texture_offset_base, v, lsl #11
  mov left_mask_bits, #0xFF
  
  ldrb texture_mask_width, [psx_gpu, #psx_gpu_texture_mask_width_offset]
  add width_rounded, width_rounded, left_offset

  ldrb texture_mask_height, [psx_gpu, #psx_gpu_texture_mask_height_offset]
  sub fb_ptr, fb_ptr, left_offset, lsl #1

  add texture_mask, texture_mask_width, texture_mask_width
  mov right_mask_bits, #0xFE

  and right_width, width_rounded, #0x7
  mvn left_mask_bits, left_mask_bits, lsl left_offset

  add texture_mask, texture_mask, texture_mask_height, lsl #11
  mov block_width, width_rounded, lsr #3

  mov right_mask_bits, right_mask_bits, lsl right_width
  movw fb_ptr_pitch, #(2048 + 16)

  sub fb_ptr_pitch, fb_ptr_pitch, block_width, lsl #4
  vmov block_masks, left_mask_bits, right_mask_bits

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  add block, psx_gpu, #psx_gpu_blocks_offset

  bic texture_offset_base, texture_offset_base, #0xF
  cmp block_width, #1

  ldr texture_page_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset]
  add block, block, num_blocks, lsl #6

  bne 0f

  vext.32 block_masks_shifted, block_masks, block_masks, #1
  vorr.u32 block_masks, block_masks, block_masks_shifted
  vdup.u8 draw_mask_fb_ptr, block_masks[0]

 1:
  add num_blocks, num_blocks, #1
  cmp num_blocks, #MAX_BLOCKS
  blgt setup_sprites_16bpp_flush

  and texture_block_ptr, texture_offset_base, texture_mask
  subs height, height, #1

  add texture_block_ptr, texture_page_ptr, texture_block_ptr
  vld1.u32 { texels }, [texture_block_ptr, :128]

  vst1.u32 { texels }, [block, :128]
  add block, block, #40

  vmov.u32 draw_mask_fb_ptr[1], fb_ptr
  pld [fb_ptr]

  vst1.u32 { draw_mask_fb_ptr }, [block, :64]

  add block, block, #24
  add texture_offset_base, texture_offset_base, #2048
  add fb_ptr, fb_ptr, #2048
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  bne 1b

  pop { r3 - r11, pc }

 0:
  add num_blocks, num_blocks, block_width
  mov texture_offset, texture_offset_base

  cmp num_blocks, #MAX_BLOCKS
  blgt setup_sprites_16bpp_flush

  add texture_offset_base, texture_offset_base, #2048
  and texture_block_ptr, texture_offset, texture_mask

  add texture_block_ptr, texture_page_ptr, texture_block_ptr
  vld1.u32 { texels }, [texture_block_ptr, :128]  

  vst1.u32 { texels }, [block, :128]
  add block, block, #40

  vdup.u8 draw_mask_fb_ptr, block_masks[0]
  vmov.u32 draw_mask_fb_ptr[1], fb_ptr
  pld [fb_ptr]

  vst1.u32 { draw_mask_fb_ptr }, [block, :64]
  subs blocks_remaining, block_width, #2

  add texture_offset, texture_offset, #16
  add fb_ptr, fb_ptr, #16

  vmov.u8 draw_mask_fb_ptr, #0

  add block, block, #24
  beq 2f

 1:
  and texture_block_ptr, texture_offset, texture_mask
  subs blocks_remaining, blocks_remaining, #1

  add texture_block_ptr, texture_page_ptr, texture_block_ptr
  vld1.u32 { texels }, [texture_block_ptr, :128]

  vst1.u32 { texels }, [block, :128]
  add block, block, #40

  vmov.u32 draw_mask_fb_ptr[1], fb_ptr
  pld [fb_ptr]

  vst1.u32 { draw_mask_fb_ptr }, [block, :64]
  
  add texture_offset, texture_offset, #16
  add fb_ptr, fb_ptr, #16

  add block, block, #24
  bne 1b

 2:
  and texture_block_ptr, texture_offset, texture_mask
  add texture_block_ptr, texture_page_ptr, texture_block_ptr

  vld1.u32 { texels }, [texture_block_ptr, :128]
  vdup.u8 draw_mask_fb_ptr, block_masks[4]

  vst1.u32 { texels }, [block, :128]
  add block, block, #40

  vmov.u32 draw_mask_fb_ptr[1], fb_ptr
  vst1.u32 { draw_mask_fb_ptr }, [block, :64]
  
  add block, block, #24
  subs height, height, #1

  add fb_ptr, fb_ptr, fb_ptr_pitch
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]

  bne 0b
  nop

  pop { r3 - r11, pc }


// 4x version
// FIXME: duplicate code with normal version :(
#undef draw_mask_fb_ptr

function(setup_sprite_16bpp_4x)
  push { r3 - r11, lr }
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset]

  ldr v, [sp, #4*(10+0)]
  add fb_ptr, fb_ptr, y, lsl #11

  ldr width, [sp, #4*(10+1)]
  add fb_ptr, fb_ptr, x, lsl #1

  ldr height, [sp, #4*(10+2)]
  and left_offset, u, #0x7

  add texture_offset_base, u, u
  add width_rounded, width, #7

  add texture_offset_base, texture_offset_base, v, lsl #11
  movw left_mask_bits, #0xFFFF
  
  ldrb texture_mask_width, [psx_gpu, #psx_gpu_texture_mask_width_offset]
  add width_rounded, width_rounded, left_offset

  lsl left_offset, #1

  ldrb texture_mask_height, [psx_gpu, #psx_gpu_texture_mask_height_offset]
  sub fb_ptr, fb_ptr, left_offset, lsl #1

  add texture_mask, texture_mask_width, texture_mask_width
  movw right_mask_bits, #0xFFFC

  and right_width, width_rounded, #0x7
  mvn left_mask_bits, left_mask_bits, lsl left_offset

  lsl right_width, #1

  add texture_mask, texture_mask, texture_mask_height, lsl #11
  mov block_width, width_rounded, lsr #3

  mov right_mask_bits, right_mask_bits, lsl right_width
  movw fb_ptr_pitch, #(2048 + 16) * 2

  sub fb_ptr_pitch, fb_ptr_pitch, block_width, lsl #4+1
  vmov block_masks, left_mask_bits, right_mask_bits

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  add block, psx_gpu, #psx_gpu_blocks_offset

  bic texture_offset_base, texture_offset_base, #0xF
  cmp block_width, #1

  ldr texture_page_ptr, [psx_gpu, #psx_gpu_texture_page_ptr_offset]
  add block, block, num_blocks, lsl #6

  lsl block_width, #2
  bne 0f

  vext.32 block_masks_shifted, block_masks, block_masks, #1
  vorr.u32 block_masks, block_masks, block_masks_shifted
  vdup.u8 draw_mask_fb_ptr_a, block_masks[0]
  vdup.u8 draw_mask_fb_ptr_b, block_masks[1]

 1:
  add num_blocks, num_blocks, block_width
  cmp num_blocks, #MAX_BLOCKS
  blgt setup_sprites_16bpp_flush

  and texture_block_ptr, texture_offset_base, texture_mask
  subs height, height, #1

  add texture_block_ptr, texture_page_ptr, texture_block_ptr
  vld1.u32 { texels }, [texture_block_ptr, :128]

  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_a, draw_mask_fb_ptr_b)

  add texture_offset_base, texture_offset_base, #2048
  add fb_ptr, fb_ptr, #2048*2
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  bne 1b

  pop { r3 - r11, pc }

 0:
  add num_blocks, num_blocks, block_width
  mov texture_offset, texture_offset_base

  vdup.u8 draw_mask_fb_ptr_a, block_masks[0] // left_mask_bits
  vdup.u8 draw_mask_fb_ptr_b, block_masks[1]

  cmp num_blocks, #MAX_BLOCKS
  blgt setup_sprites_16bpp_flush

  add texture_offset_base, texture_offset_base, #2048
  and texture_block_ptr, texture_offset, texture_mask

  add texture_block_ptr, texture_page_ptr, texture_block_ptr
  vld1.u32 { texels }, [texture_block_ptr, :128]

  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_a, draw_mask_fb_ptr_b)

  subs blocks_remaining, block_width, #2*4
  add texture_offset, texture_offset, #16

  vmov.u8 draw_mask_fb_ptr_a, #0
  vmov.u8 draw_mask_fb_ptr_b, #0

  add fb_ptr, fb_ptr, #16*2
  beq 2f

 1:
  and texture_block_ptr, texture_offset, texture_mask
  subs blocks_remaining, blocks_remaining, #4

  add texture_block_ptr, texture_page_ptr, texture_block_ptr
  vld1.u32 { texels }, [texture_block_ptr, :128]

  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_a, draw_mask_fb_ptr_b)
  add texture_offset, texture_offset, #16

  add fb_ptr, fb_ptr, #16*2
  bgt 1b

 2:
  vdup.u8 draw_mask_fb_ptr_a, block_masks[4] // right_mask_bits
  vdup.u8 draw_mask_fb_ptr_b, block_masks[5]

  and texture_block_ptr, texture_offset, texture_mask
  add texture_block_ptr, texture_page_ptr, texture_block_ptr

  vld1.u32 { texels }, [texture_block_ptr, :128]

  do_texture_block_16bpp_4x(fb_ptr2, draw_mask_fb_ptr_a, draw_mask_fb_ptr_b)
  subs height, height, #1

  add fb_ptr, fb_ptr, fb_ptr_pitch
  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]

  bne 0b
  nop

  pop { r3 - r11, pc }


#undef width
#undef right_width
#undef right_mask_bits
#undef color
#undef height
#undef blocks_remaining
#undef colors
#undef right_mask
#undef test_mask
#undef draw_mask

#define psx_gpu                                           r0
#define x                                                 r1
#define y                                                 r2
#define width                                             r3
#define right_width                                       r5
#define right_mask_bits                                   r6
#define fb_ptr                                            r7
#define color                                             r8
#define height                                            r9
#define fb_ptr_pitch                                      r12

// referenced by setup_sprites_16bpp_flush
#define num_blocks                                        r4
#define block                                             r5
#define block_width                                       r11

#define color_r                                           r1
#define color_g                                           r2
#define color_b                                           r8
#define blocks_remaining                                  r6

#define colors                                            q0
#define right_mask                                        q1
#define test_mask                                         q2
#define draw_mask                                         q2
#define draw_mask_bits_fb_ptr                             d6


.align 3

function(setup_sprite_untextured_512)
  push { r4 - r11, r14 }

  ldr width, [sp, #4*(9+1)]
  ldr fb_ptr, [psx_gpu, #psx_gpu_vram_out_ptr_offset]

  ldr height, [sp, #4*(9+2)]
  add fb_ptr, fb_ptr, y, lsl #11

  add fb_ptr, fb_ptr, x, lsl #1
  sub right_width, width, #1

  ldr color, [sp, #4*(9+3)]
  and right_width, #7

  add block_width, width, #7
  add right_width, #1

  lsr block_width, #3
  mov right_mask_bits, #0xff

  sub fb_ptr_pitch, block_width, #1
  lsl right_mask_bits, right_width

  lsl fb_ptr_pitch, #3+1
  ubfx color_r, color, #3, #5

  rsb fb_ptr_pitch, #1024*2
  ubfx color_g, color, #11, #5

  vld1.u32 { test_mask }, [psx_gpu, :128]
  ubfx color_b, color, #19, #5

  vdup.u16 right_mask, right_mask_bits
  orr color, color_r, color_b, lsl #10

  ldrh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  orr color, color, color_g, lsl #5

  vtst.u16 right_mask, right_mask, test_mask
  add block, psx_gpu, #psx_gpu_blocks_offset

  vdup.u16 colors, color
  add block, block, num_blocks, lsl #6


setup_sprite_untextured_height_loop:
  add num_blocks, block_width
  sub blocks_remaining, block_width, #1

  cmp num_blocks, #MAX_BLOCKS
  blgt setup_sprites_16bpp_flush

  cmp blocks_remaining, #0
  ble 1f

  vmov.u8 draw_mask, #0 /* zero_mask */
  vmov.u8 draw_mask_bits_fb_ptr, #0

 0:
  vst1.u32 { draw_mask }, [block, :128]!
  subs blocks_remaining, #1

  vst1.u32 { colors }, [block, :128]
  add block, block, #24

  vmov.u32 draw_mask_bits_fb_ptr[1], fb_ptr
  vst1.u32 { draw_mask_bits_fb_ptr }, [block, :64]
  
  add block, block, #24
  add fb_ptr, #8*2
  bgt 0b

 1:
  vst1.u32 { right_mask }, [block, :128]!
  subs height, #1

  vst1.u32 { colors }, [block, :128]
  add block, block, #24

  vmov.u32 draw_mask_bits_fb_ptr[1], fb_ptr
  vst1.u32 { draw_mask_bits_fb_ptr }, [block, :64]
  
  add block, block, #24
  add fb_ptr, fb_ptr_pitch

  strh num_blocks, [psx_gpu, #psx_gpu_num_blocks_offset]
  bgt setup_sprite_untextured_height_loop

  pop { r4 - r11, pc }



#undef texture_page_ptr
#undef vram_ptr
#undef dirty_textures_mask
#undef current_texture_mask

#define psx_gpu                                           r0
#define current_texture_page                              r1
#define texture_page_ptr                                  r2
#define vram_ptr_a                                        r3
#define current_texture_page_x                            r12
#define current_texture_page_y                            r4
#define dirty_textures_mask                               r5
#define tile_y                                            r6
#define tile_x                                            r7
#define sub_y                                             r8
#define current_texture_mask                              r9
#define c_4096                                            r10
#define vram_ptr_b                                        r11

#define texel_block_a                                     d0
#define texel_block_b                                     d1
#define texel_block_expanded_a                            q1
#define texel_block_expanded_b                            q2
#define texel_block_expanded_ab                           q2
#define texel_block_expanded_c                            q3
#define texel_block_expanded_d                            q0
#define texel_block_expanded_cd                           q3

function(update_texture_4bpp_cache)
  push  { r3 - r11, r14 }
  vpush { q0 - q3 }

  ldrb current_texture_page, [psx_gpu, #psx_gpu_current_texture_page_offset]

  ldr texture_page_ptr, [psx_gpu, #psx_gpu_texture_page_base_offset]
  ldr vram_ptr_a, [psx_gpu, #psx_gpu_vram_ptr_offset]

  and current_texture_page_x, current_texture_page, #0xF
  ldr current_texture_mask, [psx_gpu, #psx_gpu_current_texture_mask_offset]

  mov current_texture_page_y, current_texture_page, lsr #4
  ldr dirty_textures_mask, [psx_gpu, #psx_gpu_dirty_textures_4bpp_mask_offset]

  add vram_ptr_a, vram_ptr_a, current_texture_page_y, lsl #19
  mov tile_y, #16

  add vram_ptr_a, vram_ptr_a, current_texture_page_x, lsl #7  
  bic dirty_textures_mask, current_texture_mask
  
  mov tile_x, #16
  str dirty_textures_mask, [psx_gpu, #psx_gpu_dirty_textures_4bpp_mask_offset]

  mov sub_y, #8
  movw c_4096, #4096

  add vram_ptr_b, vram_ptr_a, #2048

 0:
  vld1.u32 { texel_block_a }, [vram_ptr_a, :64], c_4096
  vld1.u32 { texel_block_b }, [vram_ptr_b, :64], c_4096

  vmovl.u8 texel_block_expanded_a, texel_block_a
  vshll.u8 texel_block_expanded_b, texel_block_a, #4
  vmovl.u8 texel_block_expanded_c, texel_block_b
  vshll.u8 texel_block_expanded_d, texel_block_b, #4

  vbic.u16 texel_block_expanded_a, #0x00F0
  vbic.u16 texel_block_expanded_b, #0x00F0
  vbic.u16 texel_block_expanded_c, #0x00F0
  vbic.u16 texel_block_expanded_d, #0x00F0

  vorr.u16 texel_block_expanded_ab, texel_block_expanded_a,                    \
   texel_block_expanded_b
  vorr.u16 texel_block_expanded_cd, texel_block_expanded_c,                    \
   texel_block_expanded_d

  vst1.u32 { texel_block_expanded_ab, texel_block_expanded_cd },               \
   [texture_page_ptr, :256]!

  subs sub_y, sub_y, #1
  bne 0b

  mov sub_y, #8
  add vram_ptr_a, vram_ptr_a, #8
  add vram_ptr_b, vram_ptr_b, #8

  sub vram_ptr_a, vram_ptr_a, #(16 * 2048)
  sub vram_ptr_b, vram_ptr_b, #(16 * 2048)

  subs tile_x, tile_x, #1
  bne 0b

  mov tile_x, #16
  add vram_ptr_a, vram_ptr_a, #(16 * 2048)
  add vram_ptr_b, vram_ptr_b, #(16 * 2048)

  sub vram_ptr_a, vram_ptr_a, #(8 * 16)
  sub vram_ptr_b, vram_ptr_b, #(8 * 16)

  subs tile_y, tile_y, #1
  bne 0b

  vpop { q0 - q3 }
  pop  { r3 - r11, pc }


#undef current_texture_page

#define psx_gpu                                           r0
#define texture_page                                      r1
#define texture_page_ptr                                  r2
#define vram_ptr_a                                        r3
#define texture_page_x                                    r12
#define texture_page_y                                    r4
#define current_texture_page                              r5
#define tile_y                                            r6
#define tile_x                                            r7
#define sub_y                                             r8
#define c_4096                                            r10
#define vram_ptr_b                                        r11


#undef texels_a
#undef texels_b

#define texels_a                                          q0
#define texels_b                                          q1
#define texels_c                                          q2
#define texels_d                                          q3


function(update_texture_8bpp_cache_slice)
  stmdb sp!, { r4 - r11, r14 }

  ldrb current_texture_page, [psx_gpu, #psx_gpu_current_texture_page_offset]
  ldr vram_ptr_a, [psx_gpu, #psx_gpu_vram_ptr_offset]

  ldr texture_page_ptr, [psx_gpu, #psx_gpu_texture_page_base_offset]
  mov tile_y, #16

  and texture_page_x, texture_page, #0xF
  mov texture_page_y, texture_page, lsr #4

  add vram_ptr_a, vram_ptr_a, texture_page_x, lsl #7  
  mov tile_x, #8

  add vram_ptr_a, vram_ptr_a, texture_page_y, lsl #19
  eor current_texture_page, current_texture_page, texture_page

  ands current_texture_page, current_texture_page, #0x1
  mov sub_y, #4

  addne texture_page_ptr, texture_page_ptr, #(8 * 16 * 16)
  movw c_4096, #4096

  add vram_ptr_b, vram_ptr_a, #2048

 0:
  vld1.u32 { texels_a }, [vram_ptr_a, :128], c_4096
  vld1.u32 { texels_b }, [vram_ptr_b, :128], c_4096
  vld1.u32 { texels_c }, [vram_ptr_a, :128], c_4096
  vld1.u32 { texels_d }, [vram_ptr_b, :128], c_4096

  vst1.u32 { texels_a, texels_b }, [texture_page_ptr, :256]!
  vst1.u32 { texels_c, texels_d }, [texture_page_ptr, :256]!

  subs sub_y, sub_y, #1
  bne 0b

  mov sub_y, #4

  add vram_ptr_a, vram_ptr_a, #16
  add vram_ptr_b, vram_ptr_b, #16

  sub vram_ptr_a, vram_ptr_a, #(16 * 2048)
  sub vram_ptr_b, vram_ptr_b, #(16 * 2048)

  subs tile_x, tile_x, #1
  bne 0b

  mov tile_x, #8

  add vram_ptr_a, vram_ptr_a, #(16 * 2048)
  add vram_ptr_b, vram_ptr_b, #(16 * 2048)

  sub vram_ptr_a, vram_ptr_a, #(8 * 16)
  sub vram_ptr_b, vram_ptr_b, #(8 * 16)

  subs tile_y, tile_y, #1
  add texture_page_ptr, texture_page_ptr, #(8 * 16 * 16)

  bne 0b

  ldmia sp!, { r4 - r11, pc }


/* void scale2x_tiles8(void *dst, const void *src, int w8, int h) */
function(scale2x_tiles8)
  push { r4, r14 }

  mov r4, r1
  add r12, r0, #1024*2
  mov r14, r2

0:
  pld [r1, #1024*2]
  vld1.u16 { q0 }, [r1, :128]!
  vld1.u16 { q2 }, [r1, :128]!
  vmov q1, q0
  vmov q3, q2
  vzip.16 q0, q1
  vzip.16 q2, q3
  subs r14, #2
  vst1.u16 { q0, q1 }, [r0, :128]!
  vst1.u16 { q0, q1 }, [r12, :128]!
  blt 1f
  vst1.u16 { q2, q3 }, [r0, :128]!
  vst1.u16 { q2, q3 }, [r12, :128]!
  bgt 0b
1:
  subs r3, #1
  mov r14, r2
  add r0, #1024*2*2
  add r4, #1024*2
  sub r0, r0, r2, lsl #4+1
  mov r1, r4
  add r12, r0, #1024*2
  bgt 0b
  nop

  pop { r4, pc }

// vim:filetype=armasm
