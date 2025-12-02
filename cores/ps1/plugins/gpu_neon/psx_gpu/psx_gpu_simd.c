/*
 * Copyright (C) 2011 Gilead Kutnick "Exophase" <exophase@gmail.com>
 * Copyright (C) 2022 Gražvydas Ignotas "notaz" <notasas@gmail.com>
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

#include <string.h>
#include "psx_gpu.h"
#include "psx_gpu_simd.h"
//#define ASM_PROTOTYPES
//#include "psx_gpu_simd.h"
#ifdef __SSE2__
#include <x86intrin.h>
#endif
#ifndef SIMD_BUILD
#error "please define SIMD_BUILD if you want this gpu_neon C simd implementation"
#endif

typedef u8  gvu8   __attribute__((vector_size(16)));
typedef u16 gvu16  __attribute__((vector_size(16)));
typedef u32 gvu32  __attribute__((vector_size(16)));
typedef u64 gvu64  __attribute__((vector_size(16)));
typedef s8  gvs8   __attribute__((vector_size(16)));
typedef s16 gvs16  __attribute__((vector_size(16)));
typedef s32 gvs32  __attribute__((vector_size(16)));
typedef s64 gvs64  __attribute__((vector_size(16)));

typedef u8  gvhu8  __attribute__((vector_size(8)));
typedef u16 gvhu16 __attribute__((vector_size(8)));
typedef u32 gvhu32 __attribute__((vector_size(8)));
typedef u64 gvhu64 __attribute__((vector_size(8)));
typedef s8  gvhs8  __attribute__((vector_size(8)));
typedef s16 gvhs16 __attribute__((vector_size(8)));
typedef s32 gvhs32 __attribute__((vector_size(8)));
typedef s64 gvhs64 __attribute__((vector_size(8)));

typedef union
{
  gvu8  u8;
  gvu16 u16;
  gvu32 u32;
  gvu64 u64;
  gvs8  s8;
  gvs16 s16;
  gvs32 s32;
  gvs64 s64;
#ifdef __SSE2__
  __m128i m;
#endif
  // this may be tempting, but it causes gcc to do lots of stack spills
  //gvhreg h[2];
} gvreg;

typedef gvreg    gvreg_ua    __attribute__((aligned(1)));
typedef uint64_t uint64_t_ua __attribute__((aligned(1)));
typedef gvu8     gvu8_ua     __attribute__((aligned(1)));
typedef gvu16    gvu16_ua    __attribute__((aligned(1)));

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>

typedef union
{
  gvhu8  u8;
  gvhu16 u16;
  gvhu32 u32;
  gvhu64 u64;
  //u64    u64;
  //uint64x1_t u64;
  gvhs8  s8;
  gvhs16 s16;
  gvhs32 s32;
  gvhs64 s64;
  //s64    s64;
  //int64x1_t s64;
} gvhreg;

#define gvaddhn_u32(d, a, b)     d.u16 = vaddhn_u32(a.u32, b.u32)
#define gvaddw_s32(d, a, b)      d.s64 = vaddw_s32(a.s64, b.s32)
#define gvabsq_s32(d, s)         d.s32 = vabsq_s32(s.s32)
#define gvbic_n_u16(d, n)        d.u16 = vbic_u16(d.u16, vmov_n_u16(n))
#define gvbifq(d, a, b)          d.u8  = vbslq_u8(b.u8, d.u8, a.u8)
#define gvbit(d, a, b)           d.u8  = vbsl_u8(b.u8, a.u8, d.u8)
#define gvceqq_u16(d, a, b)      d.u16 = vceqq_u16(a.u16, b.u16)
#define gvcgt_s16(d, a, b)       d.u16 = vcgt_s16(a.s16, b.s16)
#define gvclt_s16(d, a, b)       d.u16 = vclt_s16(a.s16, b.s16)
#define gvcreate_s32(d, a, b)    d.s32 = vcreate_s32((u32)(a) | ((u64)(b) << 32))
#define gvcreate_u32(d, a, b)    d.u32 = vcreate_u32((u32)(a) | ((u64)(b) << 32))
#define gvcreate_s64(d, s)       d.s64 = (gvhs64)vcreate_s64(s)
#define gvcreate_u64(d, s)       d.u64 = (gvhu64)vcreate_u64(s)
#define gvcombine_u16(d, l, h)   d.u16 = vcombine_u16(l.u16, h.u16)
#define gvcombine_u32(d, l, h)   d.u32 = vcombine_u32(l.u32, h.u32)
#define gvcombine_s64(d, l, h)   d.s64 = vcombine_s64((int64x1_t)l.s64, (int64x1_t)h.s64)
#define gvdup_l_u8(d, s, l)      d.u8  = vdup_lane_u8(s.u8, l)
#define gvdup_l_u16(d, s, l)     d.u16 = vdup_lane_u16(s.u16, l)
#define gvdup_l_u32(d, s, l)     d.u32 = vdup_lane_u32(s.u32, l)
#define gvdupq_l_s64(d, s, l)    d.s64 = vdupq_lane_s64((int64x1_t)s.s64, l)
#define gvdupq_l_u32(d, s, l)    d.u32 = vdupq_lane_u32(s.u32, l)
#define gvdup_n_s64(d, n)        d.s64 = vdup_n_s64(n)
#define gvdup_n_u8(d, n)         d.u8  = vdup_n_u8(n)
#define gvdup_n_u16(d, n)        d.u16 = vdup_n_u16(n)
#define gvdup_n_u32(d, n)        d.u32 = vdup_n_u32(n)
#define gvdupq_n_u16(d, n)       d.u16 = vdupq_n_u16(n)
#define gvdupq_n_u32(d, n)       d.u32 = vdupq_n_u32(n)
#define gvdupq_n_s64(d, n)       d.s64 = vdupq_n_s64(n)
#define gvhaddq_u16(d, a, b)     d.u16 = vhaddq_u16(a.u16, b.u16)
#define gvmax_s16(d, a, b)       d.s16 = vmax_s16(a.s16, b.s16)
#define gvmin_s16(d, a, b)       d.s16 = vmin_s16(a.s16, b.s16)
#define gvmin_u16(d, a, b)       d.u16 = vmin_u16(a.u16, b.u16)
#define gvminq_u8(d, a, b)       d.u8  = vminq_u8(a.u8, b.u8)
#define gvminq_u16(d, a, b)      d.u16 = vminq_u16(a.u16, b.u16)
#define gvmla_s32(d, a, b)       d.s32 = vmla_s32(d.s32, a.s32, b.s32)
#define gvmla_u32(d, a, b)       d.u32 = vmla_u32(d.u32, a.u32, b.u32)
#define gvmlaq_s32(d, a, b)      d.s32 = vmlaq_s32(d.s32, a.s32, b.s32)
#define gvmlaq_u32(d, a, b)      d.u32 = vmlaq_u32(d.u32, a.u32, b.u32)
#define gvmlal_s32(d, a, b)      d.s64 = vmlal_s32(d.s64, a.s32, b.s32)
#define gvmlal_u8(d, a, b)       d.u16 = vmlal_u8(d.u16, a.u8, b.u8)
#define gvmlsq_s32(d, a, b)      d.s32 = vmlsq_s32(d.s32, a.s32, b.s32)
#define gvmlsq_l_s32(d, a, b, l) d.s32 = vmlsq_lane_s32(d.s32, a.s32, b.s32, l)
#define gvmov_l_s32(d, s, l)     d.s32 = vset_lane_s32(s, d.s32, l)
#define gvmov_l_u32(d, s, l)     d.u32 = vset_lane_u32(s, d.u32, l)
#define gvmovl_u8(d, s)          d.u16 = vmovl_u8(s.u8)
#define gvmovl_s32(d, s)         d.s64 = vmovl_s32(s.s32)
#define gvmovn_u16(d, s)         d.u8  = vmovn_u16(s.u16)
#define gvmovn_u32(d, s)         d.u16 = vmovn_u32(s.u32)
#define gvmovn_u64(d, s)         d.u32 = vmovn_u64(s.u64)
#define gvmul_s32(d, a, b)       d.s32 = vmul_s32(a.s32, b.s32)
#define gvmull_s16(d, a, b)      d.s32 = vmull_s16(a.s16, b.s16)
#define gvmull_s32(d, a, b)      d.s64 = vmull_s32(a.s32, b.s32)
#define gvmull_u8(d, a, b)       d.u16 = vmull_u8(a.u8, b.u8)
#define gvmull_l_u32(d, a, b, l) d.u64 = vmull_lane_u32(a.u32, b.u32, l)
#define gvmlsl_s16(d, a, b)      d.s32 = vmlsl_s16(d.s32, a.s16, b.s16)
#define gvneg_s32(d, s)          d.s32 = vneg_s32(s.s32)
#define gvqadd_u8(d, a, b)       d.u8  = vqadd_u8(a.u8, b.u8)
#define gvqsub_u8(d, a, b)       d.u8  = vqsub_u8(a.u8, b.u8)
#define gvshl_u16(d, a, b)       d.u16 = vshl_u16(a.u16, b.s16)
#define gvshlq_u64(d, a, b)      d.u64 = vshlq_u64(a.u64, b.s64)
#define gvshrq_n_s16(d, s, n)    d.s16 = vshrq_n_s16(s.s16, n)
#define gvshrq_n_u16(d, s, n)    d.u16 = vshrq_n_u16(s.u16, n)
#define gvshl_n_u32(d, s, n)     d.u32 = vshl_n_u32(s.u32, n)
#define gvshlq_n_u16(d, s, n)    d.u16 = vshlq_n_u16(s.u16, n)
#define gvshlq_n_u32(d, s, n)    d.u32 = vshlq_n_u32(s.u32, n)
#define gvshll_n_s8(d, s, n)     d.s16 = vshll_n_s8(s.s8, n)
#define gvshll_n_u8(d, s, n)     d.u16 = vshll_n_u8(s.u8, n)
#define gvshll_n_u16(d, s, n)    d.u32 = vshll_n_u16(s.u16, n)
#define gvshr_n_u8(d, s, n)      d.u8  = vshr_n_u8(s.u8, n)
#define gvshr_n_u16(d, s, n)     d.u16 = vshr_n_u16(s.u16, n)
#define gvshr_n_u32(d, s, n)     d.u32 = vshr_n_u32(s.u32, n)
#define gvshr_n_u64(d, s, n)     d.u64 = (gvhu64)vshr_n_u64((uint64x1_t)s.u64, n)
#define gvshrn_n_u16(d, s, n)    d.u8  = vshrn_n_u16(s.u16, n)
#define gvshrn_n_u32(d, s, n)    d.u16 = vshrn_n_u32(s.u32, n)
#define gvsli_n_u8(d, s, n)      d.u8  = vsli_n_u8(d.u8, s.u8, n)
#define gvsri_n_u8(d, s, n)      d.u8  = vsri_n_u8(d.u8, s.u8, n)
#define gvtstq_u16(d, a, b)      d.u16 = vtstq_u16(a.u16, b.u16)
#define gvqshrun_n_s16(d, s, n)  d.u8  = vqshrun_n_s16(s.s16, n)
#define gvqsubq_u8(d, a, b)      d.u8  = vqsubq_u8(a.u8, b.u8)
#define gvqsubq_u16(d, a, b)     d.u16 = vqsubq_u16(a.u16, b.u16)

#define gvmovn_top_u64(d, s)     d.u32 = vshrn_n_u64(s.u64, 32)

#define gvget_lo(d, s)           d.u16 = vget_low_u16(s.u16)
#define gvget_hi(d, s)           d.u16 = vget_high_u16(s.u16)
#define gvlo(s)                  ({gvhreg t_; gvget_lo(t_, s); t_;})
#define gvhi(s)                  ({gvhreg t_; gvget_hi(t_, s); t_;})

#define gvset_lo(d, s)           d.u16 = vcombine_u16(s.u16, gvhi(d).u16)
#define gvset_hi(d, s)           d.u16 = vcombine_u16(gvlo(d).u16, s.u16)

#define gvtbl2_u8(d, a, b) { \
  uint8x8x2_t v_; \
  v_.val[0] = vget_low_u8(a.u8); v_.val[1] = vget_high_u8(a.u8); \
  d.u8 = vtbl2_u8(v_, b.u8); \
}

#define gvzip_u8(d, a, b) { \
  uint8x8x2_t v_ = vzip_u8(a.u8, b.u8); \
  d.u8 = vcombine_u8(v_.val[0], v_.val[1]); \
}
#define gvzipq_u16(d0, d1, s0, s1) { \
  uint16x8x2_t v_ = vzipq_u16(s0.u16, s1.u16); \
  d0.u16 = v_.val[0]; d1.u16 = v_.val[1]; \
}

#define gvld1_u8(d, s)           d.u8  = vld1_u8(s)
#define gvld1_u32(d, s)          d.u32 = vld1_u32((const u32 *)(s))
#define gvld1_u64(d, s)          d.u64 = vld1_u64((const u64 *)(s))
#define gvld1q_u8(d, s)          d.u8  = vld1q_u8(s)
#define gvld1q_u16(d, s)         d.u16 = vld1q_u16(s)
#define gvld1q_u32(d, s)         d.u32 = vld1q_u32((const u32 *)(s))
#define gvld2_u8_dup(v0, v1, p) { \
  uint8x8x2_t v_ = vld2_dup_u8(p); \
  v0.u8 = v_.val[0]; v1.u8 = v_.val[1]; \
}
#define gvld2q_u8(v0, v1, p) { \
  uint8x16x2_t v_ = vld2q_u8(p); \
  v0.u8 = v_.val[0]; v1.u8 = v_.val[1]; \
}

#define gvst1_u8(v, p) \
  vst1_u8(p, v.u8)
#define gvst1_u64(v, p) \
  vst1_u64((u64 *)(p), v.u64)
#define gvst1q_u16(v, p) \
  vst1q_u16(p, v.u16)
#define gvst1q_inc_u32(v, p, i) { \
  vst1q_u32((u32 *)(p), v.u32); \
  p += (i) / sizeof(*p); \
}
#define gvst2_u8(v0, v1, p) { \
  uint8x8x2_t v_; \
  v_.val[0] = v0.u8; v_.val[1] = v1.u8; \
  vst2_u8(p, v_); \
}
#define gvst2_u16(v0, v1, p) { \
  uint16x4x2_t v_; \
  v_.val[0] = v0.u16; v_.val[1] = v1.u16; \
  vst2_u16(p, v_); \
}
#define gvst2q_u8(v0, v1, p) { \
  uint8x16x2_t v_; \
  v_.val[0] = v0.u8; v_.val[1] = v1.u8; \
  vst2q_u8(p, v_); \
}
#define gvst4_4_inc_u32(v0, v1, v2, v3, p, i) { \
  uint32x2x4_t v_; \
  v_.val[0] = v0.u32; v_.val[1] = v1.u32; v_.val[2] = v2.u32; v_.val[3] = v3.u32; \
  vst4_u32(p, v_); p += (i) / sizeof(*p); \
}
#define gvst4_pi_u16(v0, v1, v2, v3, p) { \
  uint16x4x4_t v_; \
  v_.val[0] = v0.u16; v_.val[1] = v1.u16; v_.val[2] = v2.u16; v_.val[3] = v3.u16; \
  vst4_u16((u16 *)(p), v_); p += sizeof(v_) / sizeof(*p); \
}
#define gvst1q_pi_u32(v, p) \
  gvst1q_inc_u32(v, p, sizeof(v))
// could use vst1q_u32_x2 but that's not always available
#define gvst1q_2_pi_u32(v0, v1, p) { \
  gvst1q_inc_u32(v0, p, sizeof(v0)); \
  gvst1q_inc_u32(v1, p, sizeof(v1)); \
}

/* notes:
 - gcc > 9: (arm32) int64x1_t type produces ops on gp regs
            (also u64 __attribute__((vector_size(8))) :( )
 - gcc <11: (arm32) handles '<vec> == 0' poorly
*/

#elif defined(__SSE2__)

// use a full reg and discard the upper half
#define gvhreg gvreg

#define gv0()                    _mm_setzero_si128()

#ifdef __x86_64__
#define gvcreate_s32(d, a, b)    d.m = _mm_cvtsi64_si128((u32)(a) | ((u64)(b) << 32))
#define gvcreate_s64(d, s)       d.m = _mm_cvtsi64_si128(s)
#else
#define gvcreate_s32(d, a, b)    d.m = _mm_set_epi32(0, 0, b, a)
#define gvcreate_s64(d, s)       d.m = _mm_loadu_si64(&(s))
#endif

#define gvbic_n_u16(d, n)        d.m = _mm_andnot_si128(_mm_set1_epi16(n), d.m)
#define gvceqq_u16(d, a, b)      d.u16 = vceqq_u16(a.u16, b.u16)
#define gvcgt_s16(d, a, b)       d.m = _mm_cmpgt_epi16(a.m, b.m)
#define gvclt_s16(d, a, b)       d.m = _mm_cmpgt_epi16(b.m, a.m)
#define gvcreate_u32             gvcreate_s32
#define gvcreate_u64             gvcreate_s64
#define gvcombine_u16(d, l, h)   d.m = _mm_unpacklo_epi64(l.m, h.m)
#define gvcombine_u32            gvcombine_u16
#define gvcombine_s64            gvcombine_u16
#define gvdup_l_u8(d, s, l)      d.u8  = vdup_lane_u8(s.u8, l)
#define gvdup_l_u16(d, s, l)     d.m = _mm_shufflelo_epi16(s.m, (l)|((l)<<2)|((l)<<4)|((l)<<6))
#define gvdup_l_u32(d, s, l)     d.m =  vdup_lane_u32(s.u32, l)
#define gvdupq_l_s64(d, s, l)    d.m = _mm_unpacklo_epi64(s.m, s.m)
#define gvdupq_l_u32(d, s, l)    d.m = _mm_shuffle_epi32(s.m, (l)|((l)<<2)|((l)<<4)|((l)<<6))
#define gvdup_n_s64(d, n)        d.m = _mm_set1_epi64x(n)
#define gvdup_n_u8(d, n)         d.m = _mm_set1_epi8(n)
#define gvdup_n_u16(d, n)        d.m = _mm_set1_epi16(n)
#define gvdup_n_u32(d, n)        d.m = _mm_set1_epi32(n)
#define gvdupq_n_u16(d, n)       d.m = _mm_set1_epi16(n)
#define gvdupq_n_u32(d, n)       d.m = _mm_set1_epi32(n)
#define gvdupq_n_s64(d, n)       d.m = _mm_set1_epi64x(n)
#define gvmax_s16(d, a, b)       d.m = _mm_max_epi16(a.m, b.m)
#define gvmin_s16(d, a, b)       d.m = _mm_min_epi16(a.m, b.m)
#define gvminq_u8(d, a, b)       d.m = _mm_min_epu8(a.m, b.m)
#define gvmovn_u64(d, s)         d.m = _mm_shuffle_epi32(s.m, 0 | (2 << 2))
#define gvmovn_top_u64(d, s)     d.m = _mm_shuffle_epi32(s.m, 1 | (3 << 2))
#define gvmull_s16(d, a, b) { \
  __m128i lo_ = _mm_mullo_epi16(a.m, b.m); \
  __m128i hi_ = _mm_mulhi_epi16(a.m, b.m); \
  d.m = _mm_unpacklo_epi16(lo_, hi_); \
}
#define gvmull_l_u32(d, a, b, l) { \
  __m128i a_ = _mm_unpacklo_epi32(a.m, a.m); /* lanes 0,1 -> 0,2 */ \
  __m128i b_ = _mm_shuffle_epi32(b.m, (l) | ((l) << 4)); \
  d.m = _mm_mul_epu32(a_, b_); \
}
#define gvmlsl_s16(d, a, b) { \
  gvreg tmp_; \
  gvmull_s16(tmp_, a, b); \
  d.m = _mm_sub_epi32(d.m, tmp_.m); \
}
#define gvqadd_u8(d, a, b)       d.m = _mm_adds_epu8(a.m, b.m)
#define gvqsub_u8(d, a, b)       d.m = _mm_subs_epu8(a.m, b.m)
#define gvshrq_n_s16(d, s, n)    d.m = _mm_srai_epi16(s.m, n)
#define gvshrq_n_u16(d, s, n)    d.m = _mm_srli_epi16(s.m, n)
#define gvshrq_n_u32(d, s, n)    d.m = _mm_srli_epi32(s.m, n)
#define gvshl_n_u32(d, s, n)     d.m = _mm_slli_epi32(s.m, n)
#define gvshlq_n_u16(d, s, n)    d.m = _mm_slli_epi16(s.m, n)
#define gvshlq_n_u32(d, s, n)    d.m = _mm_slli_epi32(s.m, n)
#define gvshll_n_u16(d, s, n)    d.m = _mm_slli_epi32(_mm_unpacklo_epi16(s.m, gv0()), n)
#define gvshr_n_u16(d, s, n)     d.m = _mm_srli_epi16(s.m, n)
#define gvshr_n_u32(d, s, n)     d.m = _mm_srli_epi32(s.m, n)
#define gvshr_n_u64(d, s, n)     d.m = _mm_srli_epi64(s.m, n)
#define gvshrn_n_s64(d, s, n) { \
  gvreg tmp_; \
  gvshrq_n_s64(tmp_, s, n); \
  d.m = _mm_shuffle_epi32(tmp_.m, 0 | (2 << 2)); \
}
#define gvqshrun_n_s16(d, s, n) { \
  __m128i t_ = _mm_srai_epi16(s.m, n); \
  d.m = _mm_packus_epi16(t_, t_); \
}
#define gvqsubq_u8(d, a, b)      d.m = _mm_subs_epu8(a.m, b.m)
#define gvqsubq_u16(d, a, b)     d.m = _mm_subs_epu16(a.m, b.m)

#ifdef __SSSE3__
#define gvabsq_s32(d, s)         d.m = _mm_abs_epi32(s.m)
#define gvtbl2_u8(d, a, b)       d.m = _mm_shuffle_epi8(a.m, b.m)
#else
// must supply these here or else gcc will produce something terrible with __builtin_shuffle
#define gvmovn_u16(d, s) { \
  __m128i t2_ = _mm_and_si128(s.m, _mm_set1_epi16(0xff)); \
  d.m = _mm_packus_epi16(t2_, t2_); \
}
#define gvmovn_u32(d, s) { \
  __m128i t2_; \
  t2_ = _mm_shufflelo_epi16(s.m, (0 << 0) | (2 << 2)); \
  t2_ = _mm_shufflehi_epi16(t2_, (0 << 0) | (2 << 2)); \
  d.m = _mm_shuffle_epi32(t2_, (0 << 0) | (2 << 2)); \
}
#define gvmovn_top_u32(d, s) { \
  __m128i t2_; \
  t2_ = _mm_shufflelo_epi16(s.m, (1 << 0) | (3 << 2)); \
  t2_ = _mm_shufflehi_epi16(t2_, (1 << 0) | (3 << 2)); \
  d.m = _mm_shuffle_epi32(t2_, (0 << 0) | (2 << 2)); \
}
#endif // !__SSSE3__
#ifdef __SSE4_1__
#define gvmin_u16(d, a, b)       d.m = _mm_min_epu16(a.m, b.m)
#define gvminq_u16               gvmin_u16
#define gvmovl_u8(d, s)          d.m = _mm_cvtepu8_epi16(s.m)
#define gvmovl_s8(d, s)          d.m = _mm_cvtepi8_epi16(s.m)
#define gvmovl_s32(d, s)         d.m = _mm_cvtepi32_epi64(s.m)
#define gvmull_s32(d, a, b) { \
  __m128i a_ = _mm_unpacklo_epi32(a.m, a.m); /* lanes 0,1 -> 0,2 */ \
  __m128i b_ = _mm_unpacklo_epi32(b.m, b.m); \
  d.m = _mm_mul_epi32(a_, b_); \
}
#else
#define gvmovl_u8(d, s)          d.m = _mm_unpacklo_epi8(s.m, gv0())
#define gvmovl_s8(d, s)          d.m = _mm_unpacklo_epi8(s.m, _mm_cmpgt_epi8(gv0(), s.m))
#define gvmovl_s32(d, s)         d.m = _mm_unpacklo_epi32(s.m, _mm_srai_epi32(s.m, 31))
#endif // !__SSE4_1__
#ifndef __AVX2__
#define gvshlq_u64(d, a, b) { \
  gvreg t1_, t2_; \
  t1_.m = _mm_sll_epi64(a.m, b.m); \
  t2_.m = _mm_sll_epi64(a.m, _mm_shuffle_epi32(b.m, (2 << 0) | (3 << 2))); \
  d.u64 = (gvu64){ t1_.u64[0], t2_.u64[1] }; \
}
#endif // __AVX2__

#define gvlo(s)                  s
#define gvhi(s)                  ((gvreg)_mm_shuffle_epi32(s.m, (2 << 0) | (3 << 2)))
#define gvget_lo(d, s)           d = gvlo(s)
#define gvget_hi(d, s)           d = gvhi(s)

#define gvset_lo(d, s)           d.m = _mm_unpacklo_epi64(s.m, gvhi(d).m)
#define gvset_hi(d, s)           d.m = _mm_unpacklo_epi64(d.m, s.m)

#define gvld1_u8(d, s)           d.m = _mm_loadu_si64(s)
#define gvld1_u32                gvld1_u8
#define gvld1_u64                gvld1_u8
#define gvld1q_u8(d, s)          d.m = _mm_loadu_si128((__m128i *)(s))
#define gvld1q_u16               gvld1q_u8
#define gvld1q_u32               gvld1q_u8

#define gvst1_u8(v, p)           _mm_storeu_si64(p, v.m)
#define gvst1_u64                gvst1_u8

#define gvst4_4_inc_u32(v0, v1, v2, v3, p, i) { \
  __m128i t0 = _mm_unpacklo_epi32(v0.m, v1.m); \
  __m128i t1 = _mm_unpacklo_epi32(v2.m, v3.m); \
  _mm_storeu_si128(((__m128i *)(p)) + 0, _mm_unpacklo_epi64(t0, t1)); \
  _mm_storeu_si128(((__m128i *)(p)) + 1, _mm_unpackhi_epi64(t0, t1)); \
  p += (i) / sizeof(*p); \
}
#define gvst4_pi_u16(v0, v1, v2, v3, p) { \
  __m128i t0 = _mm_unpacklo_epi16(v0.m, v1.m); \
  __m128i t1 = _mm_unpacklo_epi16(v2.m, v3.m); \
  _mm_storeu_si128(((__m128i *)(p)) + 0, _mm_unpacklo_epi32(t0, t1)); \
  _mm_storeu_si128(((__m128i *)(p)) + 1, _mm_unpackhi_epi32(t0, t1)); \
  p += sizeof(t0) * 2 / sizeof(*p); \
}

#else
#error "arch not supported or SIMD support was not enabled by your compiler"
#endif

// the below have intrinsics but they evaluate to basic operations on both gcc and clang
#define gvadd_s64(d, a, b)       d.s64 = a.s64 + b.s64
#define gvadd_u8(d, a, b)        d.u8  = a.u8  + b.u8
#define gvadd_u16(d, a, b)       d.u16 = a.u16 + b.u16
#define gvadd_u32(d, a, b)       d.u32 = a.u32 + b.u32
#define gvaddq_s64               gvadd_s64
#define gvaddq_u16               gvadd_u16
#define gvaddq_u32               gvadd_u32
#define gvand(d, a, b)           d.u32 = a.u32 & b.u32
#define gvand_n_u32(d, n)        d.u32 &= n
#define gvbic(d, a, b)           d.u32 = a.u32 & ~b.u32
#define gvbicq                   gvbic
#define gveor(d, a, b)           d.u32 = a.u32 ^ b.u32
#define gveorq                   gveor
#define gvceqz_u16(d, s)         d.u16 = s.u16 == 0
#define gvceqzq_u16              gvceqz_u16
#define gvcltz_s16(d, s)         d.s16 = s.s16 < 0
#define gvcltzq_s16              gvcltz_s16
#define gvsub_u16(d, a, b)       d.u16 = a.u16 - b.u16
#define gvsub_u32(d, a, b)       d.u32 = a.u32 - b.u32
#define gvsubq_u16               gvsub_u16
#define gvsubq_u32               gvsub_u32
#define gvorr(d, a, b)           d.u32 = a.u32 | b.u32
#define gvorrq                   gvorr
#define gvorr_n_u16(d, n)        d.u16 |= n

// fallbacks
#if 1

#ifndef gvaddhn_u32
#define gvaddhn_u32(d, a, b) { \
  gvreg tmp1_ = { .u32 = a.u32 + b.u32 }; \
  gvmovn_top_u32(d, tmp1_); \
}
#endif
#ifndef gvabsq_s32
#define gvabsq_s32(d, s) { \
  gvreg tmp1_ = { .s32 = (gvs32){} - s.s32 }; \
  gvreg mask_ = { .s32 = s.s32 >> 31 }; \
  gvbslq_(d, mask_, tmp1_, s); \
}
#endif
#ifndef gvbit
#define gvbslq_(d, s, a, b)      d.u32 = (a.u32 & s.u32) | (b.u32 & ~s.u32)
#define gvbifq(d, a, b)          gvbslq_(d, b, d, a)
#define gvbit(d, a, b)           gvbslq_(d, b, a, d)
#endif
#ifndef gvaddw_s32
#define gvaddw_s32(d, a, b)     {gvreg t_; gvmovl_s32(t_, b); d.s64 += t_.s64;}
#endif
#ifndef gvhaddq_u16
// can do this because the caller needs the msb clear
#define gvhaddq_u16(d, a, b)     d.u16 = (a.u16 + b.u16) >> 1
#endif
#ifndef gvmin_u16
#define gvmin_u16(d, a, b) { \
  gvu16 t_ = a.u16 < b.u16; \
  d.u16 = (a.u16 & t_) | (b.u16 & ~t_); \
}
#define gvminq_u16               gvmin_u16
#endif
#ifndef gvmlsq_s32
#define gvmlsq_s32(d, a, b)      d.s32 -= a.s32 * b.s32
#endif
#ifndef gvmlsq_l_s32
#define gvmlsq_l_s32(d, a, b, l){gvreg t_; gvdupq_l_u32(t_, b, l); d.s32 -= a.s32 * t_.s32;}
#endif
#ifndef gvmla_s32
#define gvmla_s32(d, a, b)       d.s32 += a.s32 * b.s32
#endif
#ifndef gvmla_u32
#define gvmla_u32                gvmla_s32
#endif
#ifndef gvmlaq_s32
#define gvmlaq_s32(d, a, b)      d.s32 += a.s32 * b.s32
#endif
#ifndef gvmlaq_u32
#define gvmlaq_u32               gvmlaq_s32
#endif
#ifndef gvmlal_u8
#define gvmlal_u8(d, a, b)      {gvreg t_; gvmull_u8(t_, a, b); d.u16 += t_.u16;}
#endif
#ifndef gvmlal_s32
#define gvmlal_s32(d, a, b)     {gvreg t_; gvmull_s32(t_, a, b); d.s64 += t_.s64;}
#endif
#ifndef gvmov_l_s32
#define gvmov_l_s32(d, s, l)     d.s32[l] = s
#endif
#ifndef gvmov_l_u32
#define gvmov_l_u32(d, s, l)     d.u32[l] = s
#endif
#ifndef gvmul_s32
#define gvmul_s32(d, a, b)       d.s32 = a.s32 * b.s32
#endif
#ifndef gvmull_u8
#define gvmull_u8(d, a, b) { \
  gvreg t1_, t2_; \
  gvmovl_u8(t1_, a); \
  gvmovl_u8(t2_, b); \
  d.u16 = t1_.u16 * t2_.u16; \
}
#endif
#ifndef gvmull_s32
// note: compilers tend to use int regs here
#define gvmull_s32(d, a, b) { \
  d.s64[0] = (s64)a.s32[0] * b.s32[0]; \
  d.s64[1] = (s64)a.s32[1] * b.s32[1]; \
}
#endif
#ifndef gvneg_s32
#define gvneg_s32(d, s)          d.s32 = -s.s32
#endif
// x86 note: needs _mm_sllv_epi16 (avx512), else this sucks terribly
#ifndef gvshl_u16
#define gvshl_u16(d, a, b)       d.u16 = a.u16 << b.u16
#endif
// x86 note: needs _mm_sllv_* (avx2)
#ifndef gvshlq_u64
#define gvshlq_u64(d, a, b)      d.u64 = a.u64 << b.u64
#endif
#ifndef gvshll_n_s8
#define gvshll_n_s8(d, s, n)    {gvreg t_; gvmovl_s8(t_, s); gvshlq_n_u16(d, t_, n);}
#endif
#ifndef gvshll_n_u8
#define gvshll_n_u8(d, s, n)    {gvreg t_; gvmovl_u8(t_, s); gvshlq_n_u16(d, t_, n);}
#endif
#ifndef gvshr_n_u8
#define gvshr_n_u8(d, s, n)      d.u8  = s.u8 >> (n)
#endif
#ifndef gvshrq_n_s64
#define gvshrq_n_s64(d, s, n)    d.s64 = s.s64 >> (n)
#endif
#ifndef gvshrn_n_u16
#define gvshrn_n_u16(d, s, n)   {gvreg t_; gvshrq_n_u16(t_, s, n); gvmovn_u16(d, t_);}
#endif
#ifndef gvshrn_n_u32
#define gvshrn_n_u32(d, s, n)   {gvreg t_; gvshrq_n_u32(t_, s, n); gvmovn_u32(d, t_);}
#endif
#ifndef gvsli_n_u8
#define gvsli_n_u8(d, s, n)      d.u8 = (s.u8 << (n)) | (d.u8 & ((1u << (n)) - 1u))
#endif
#ifndef gvsri_n_u8
#define gvsri_n_u8(d, s, n)      d.u8 = (s.u8 >> (n)) | (d.u8 & ((0xff00u >> (n)) & 0xffu))
#endif
#ifndef gvtstq_u16
#define gvtstq_u16(d, a, b)      d.u16 = (a.u16 & b.u16) != 0
#endif

#ifndef gvld2_u8_dup
#define gvld2_u8_dup(v0, v1, p) { \
  gvdup_n_u8(v0, ((const u8 *)(p))[0]); \
  gvdup_n_u8(v1, ((const u8 *)(p))[1]); \
}
#endif
#ifndef gvst1_u8
#define gvst1_u8(v, p)           *(uint64_t_ua *)(p) = v.u64[0]
#endif
#ifndef gvst1q_u16
#define gvst1q_u16(v, p)         *(gvreg_ua *)(p) = v
#endif
#ifndef gvst1q_inc_u32
#define gvst1q_inc_u32(v, p, i) {*(gvreg_ua *)(p) = v; p += (i) / sizeof(*p);}
#endif
#ifndef gvst1q_pi_u32
#define gvst1q_pi_u32(v, p)      gvst1q_inc_u32(v, p, sizeof(v))
#endif
#ifndef gvst1q_2_pi_u32
#define gvst1q_2_pi_u32(v0, v1, p) { \
  gvst1q_inc_u32(v0, p, sizeof(v0)); \
  gvst1q_inc_u32(v1, p, sizeof(v1)); \
}
#endif
#ifndef gvst2_u8
#define gvst2_u8(v0, v1, p)     {gvreg t_; gvzip_u8(t_, v0, v1); *(gvu8_ua *)(p) = t_.u8;}
#endif
#ifndef gvst2_u16
#define gvst2_u16(v0, v1, p)    {gvreg t_; gvzip_u16(t_, v0, v1); *(gvu16_ua *)(p) = t_.u16;}
#endif

// note: these shuffles assume sizeof(gvhreg) == 16 && sizeof(gvreg) == 16
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

// prefer __builtin_shuffle on gcc as it handles -1 poorly
#if __has_builtin(__builtin_shufflevector) && !__has_builtin(__builtin_shuffle)

#ifndef gvld2q_u8
#define gvld2q_u8(v0, v1, p) { \
  gvu8 v0_ = ((gvu8_ua *)(p))[0]; \
  gvu8 v1_ = ((gvu8_ua *)(p))[1]; \
  v0.u8 = __builtin_shufflevector(v0_, v1_, 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30); \
  v1.u8 = __builtin_shufflevector(v0_, v1_, 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31); \
}
#endif
#ifndef gvmovn_u16
#define gvmovn_u16(d, s) \
  d.u8 = __builtin_shufflevector(s.u8, s.u8, 0,2,4,6,8,10,12,14,-1,-1,-1,-1,-1,-1,-1,-1)
#endif
#ifndef gvmovn_u32
#define gvmovn_u32(d, s) \
  d.u16 = __builtin_shufflevector(s.u16, s.u16, 0,2,4,6,-1,-1,-1,-1)
#endif
#ifndef gvmovn_top_u32
#define gvmovn_top_u32(d, s) \
  d.u16 = __builtin_shufflevector(s.u16, s.u16, 1,3,5,7,-1,-1,-1,-1)
#endif
#ifndef gvzip_u8
#define gvzip_u8(d, a, b) \
  d.u8 = __builtin_shufflevector(a.u8, b.u8, 0,16,1,17,2,18,3,19,4,20,5,21,6,22,7,23)
#endif
#ifndef gvzip_u16
#define gvzip_u16(d, a, b) \
  d.u16 = __builtin_shufflevector(a.u16, b.u16, 0,8,1,9,2,10,3,11)
#endif
#ifndef gvzipq_u16
#define gvzipq_u16(d0, d1, s0, s1) { \
  gvu16 t_ = __builtin_shufflevector(s0.u16, s1.u16, 0, 8, 1, 9, 2, 10, 3, 11); \
  d1.u16   = __builtin_shufflevector(s0.u16, s1.u16, 4,12, 5,13, 6, 14, 7, 15); \
  d0.u16 = t_; \
}
#endif

#else // !__has_builtin(__builtin_shufflevector)

#ifndef gvld2q_u8
#define gvld2q_u8(v0, v1, p) { \
  gvu8 v0_ = ((gvu8_ua *)(p))[0]; \
  gvu8 v1_ = ((gvu8_ua *)(p))[1]; \
  v0.u8 = __builtin_shuffle(v0_, v1_, (gvu8){0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30}); \
  v1.u8 = __builtin_shuffle(v0_, v1_, (gvu8){1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31}); \
}
#endif
#ifndef gvmovn_u16
#define gvmovn_u16(d, s) \
  d.u8 = __builtin_shuffle(s.u8, (gvu8){0,2,4,6,8,10,12,14,0,2,4,6,8,10,12,14})
#endif
#ifndef gvmovn_u32
#define gvmovn_u32(d, s) \
  d.u16 = __builtin_shuffle(s.u16, (gvu16){0,2,4,6,0,2,4,6})
#endif
#ifndef gvmovn_top_u32
#define gvmovn_top_u32(d, s) \
  d.u16 = __builtin_shuffle(s.u16, (gvu16){1,3,5,7,1,3,5,7})
#endif
#ifndef gvtbl2_u8
#define gvtbl2_u8(d, a, b)  d.u8 = __builtin_shuffle(a.u8, b.u8)
#endif
#ifndef gvzip_u8
#define gvzip_u8(d, a, b) \
  d.u8 = __builtin_shuffle(a.u8, b.u8, (gvu8){0,16,1,17,2,18,3,19,4,20,5,21,6,22,7,23})
#endif
#ifndef gvzip_u16
#define gvzip_u16(d, a, b) \
  d.u16 = __builtin_shuffle(a.u16, b.u16, (gvu16){0,8,1,9,2,10,3,11})
#endif
#ifndef gvzipq_u16
#define gvzipq_u16(d0, d1, s0, s1) { \
  gvu16 t_ = __builtin_shuffle(s0.u16, s1.u16, (gvu16){0, 8, 1, 9, 2, 10, 3, 11}); \
  d1.u16   = __builtin_shuffle(s0.u16, s1.u16, (gvu16){4,12, 5,13, 6, 14, 7, 15}); \
  d0.u16 = t_; \
}
#endif

#endif // __builtin_shufflevector || __builtin_shuffle

#ifndef gvtbl2_u8
#define gvtbl2_u8(d, a, b) { \
  int i_; \
  for (i_ = 0; i_ < 16; i_++) \
    d.u8[i_] = a.u8[b.u8[i_]]; \
}
#endif

#endif // fallbacks

#if defined(__arm__)

#define gssub16(d, a, b)    asm("ssub16 %0,%1,%2" : "=r"(d) : "r"(a), "r"(b))
#define gsmusdx(d, a, b)    asm("smusdx %0,%1,%2" : "=r"(d) : "r"(a), "r"(b))

#if 0
// gcc/config/arm/arm.c
#undef gvadd_s64
#define gvadd_s64(d, a, b)  asm("vadd.i64 %P0,%P1,%P2" : "=w"(d.s64) : "w"(a.s64), "w"(b.s64))
#endif

#else

#define gssub16(d, a, b)    d = (u16)((a) - (b)) | ((((a) >> 16) - ((b) >> 16)) << 16)
#define gsmusdx(d, a, b)    d = ((s32)(s16)(a) * ((s32)(b) >> 16)) \
                              - (((s32)(a) >> 16) * (s16)(b))

#endif

// for compatibility with the original psx_gpu.c code
#define vec_2x64s gvreg
#define vec_2x64u gvreg
#define vec_4x32s gvreg
#define vec_4x32u gvreg
#define vec_8x16s gvreg
#define vec_8x16u gvreg
#define vec_16x8s gvreg
#define vec_16x8u gvreg
#define vec_1x64s gvhreg
#define vec_1x64u gvhreg
#define vec_2x32s gvhreg
#define vec_2x32u gvhreg
#define vec_4x16s gvhreg
#define vec_4x16u gvhreg
#define vec_8x8s  gvhreg
#define vec_8x8u  gvhreg

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static int ccount, dump_enabled;
void cmpp(const char *name, const void *a_, const void *b_, size_t len)
{
  const uint32_t *a = a_, *b = b_, masks[] = { 0, 0xff, 0xffff, 0xffffff };
  size_t i, left;
  uint32_t mask;
  for (i = 0; i < (len + 3)/4; i++) {
    left = len - i*4;
    mask = left >= 4 ? ~0u : masks[left];
    if ((a[i] ^ b[i]) & mask) {
      printf("%s: %08x %08x [%03zx/%zu] #%d\n",
        name, a[i] & mask, b[i] & mask, i*4, i, ccount);
      exit(1);
    }
  }
  ccount++;
}
#define ccmpf(n)   cmpp(#n, &psx_gpu->n, &n##_c, sizeof(n##_c))
#define ccmpa(n,c) cmpp(#n, &psx_gpu->n, &n##_c, sizeof(n##_c[0]) * c)

void dump_r_(const char *name, void *dump, int is_q)
{
  unsigned long long *u = dump;
  if (!dump_enabled) return;
  //if (ccount > 1) return;
  printf("%20s %016llx ", name, u[0]);
  if (is_q)
    printf("%016llx", u[1]);
  puts("");
}
void __attribute__((noinline,noclone)) dump_r_d(const char *name, void *dump)
{ dump_r_(name, dump, 0); }
void __attribute__((noinline,noclone)) dump_r_q(const char *name, void *dump)
{ dump_r_(name, dump, 1); }
#define dumprd(n) { u8 dump_[8]; gvst1_u8(n, dump_); dump_r_d(#n, dump_); }
#define dumprq(n) { u16 dump_[8]; gvst1q_u16(n, dump_); dump_r_q(#n, dump_); }
#endif

void compute_all_gradients(psx_gpu_struct * __restrict__ psx_gpu,
 const vertex_struct * __restrict__ a, const vertex_struct * __restrict__ b,
 const vertex_struct * __restrict__ c)
{
  union { double d; struct { u32 l; u32 h; } i; } divident, divider;
  union { double d; gvhreg v; } d30;

#if 0
 compute_all_gradients_(psx_gpu, a, b, c);
 return;
#endif
  // First compute the triangle area reciprocal and shift. The division will
  // happen concurrently with much of the work which follows.

  // load exponent of 62 into upper half of double
  u32 shift = __builtin_clz(psx_gpu->triangle_area);
  u32 triangle_area_normalized = psx_gpu->triangle_area << shift;

  // load area normalized into lower half of double
  divident.i.l = triangle_area_normalized >> 10;
  divident.i.h = (62 + 1023) << 20;

  divider.i.l = triangle_area_normalized << 20;
  divider.i.h = ((1022 + 31) << 20) + (triangle_area_normalized >> 11);

  d30.d = divident.d / divider.d;       // d30 = ((1 << 62) + ta_n) / ta_n

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

  gvreg v0;
  gvreg v1;
  gvreg v2;
  gvreg uvrg_xxxx0;
  gvreg uvrg_xxxx1;
  gvreg uvrg_xxxx2;

  gvreg y0_ab;
  gvreg y1_ab;
  gvreg y2_ab;

  gvreg d0_ab;
  gvreg d1_ab;
  gvreg d2_ab;
  gvreg d3_ab;

  gvreg ga_uvrg_x;
  gvreg ga_uvrg_y;
  gvreg gw_rg_x;
  gvreg gw_rg_y;
  gvreg w_mask;
  gvreg r_shift;
  gvreg uvrg_dx2, uvrg_dx3;
  gvreg uvrgb_phase;
  gvhreg zero, tmp_lo, tmp_hi;

  gvld1q_u8(v0, (u8 *)a);               // v0 = { uvrg0, b0, x0, y0 }
  gvld1q_u8(v1, (u8 *)b);               // v1 = { uvrg1, b1, x1, y1 }
  gvld1q_u8(v2, (u8 *)c);               // v2 = { uvrg2, b2, x2, y2 }

  gvmovl_u8(uvrg_xxxx0, gvlo(v0));      // uvrg_xxxx0 = { uv0, rg0, b0-, -- }
  gvmovl_u8(uvrg_xxxx1, gvlo(v1));      // uvrg_xxxx1 = { uv1, rg1, b1-, -- }
  gvmovl_u8(uvrg_xxxx2, gvlo(v2));      // uvrg_xxxx2 = { uv2, rg2, b2-, -- }

  gvdup_l_u16(tmp_lo, gvhi(v0), 1);     // yyyy0 = { yy0, yy0 }
  gvcombine_u16(y0_ab, tmp_lo, gvlo(uvrg_xxxx0));

  gvdup_l_u16(tmp_lo, gvhi(v0), 0);     // xxxx0 = { xx0, xx0 }
  gvset_hi(uvrg_xxxx0, tmp_lo);

  u32 x1_x2 = (u16)b->x | (c->x << 16); // x1_x2 = { x1, x2 }
  u32 x0_x1 = (u16)a->x | (b->x << 16); // x0_x1 = { x0, x1 }

  gvdup_l_u16(tmp_lo, gvhi(v1), 1);     // yyyy1 = { yy1, yy1 }
  gvcombine_u16(y1_ab, tmp_lo, gvlo(uvrg_xxxx1));

  gvdup_l_u16(tmp_lo, gvhi(v1), 0);     // xxxx1 = { xx1, xx1 }
  gvset_hi(uvrg_xxxx1, tmp_lo);

  gvdup_l_u16(tmp_lo, gvhi(v2), 1);     // yyyy2 = { yy2, yy2 }
  gvcombine_u16(y2_ab, tmp_lo, gvlo(uvrg_xxxx2));

  gvdup_l_u16(tmp_lo, gvhi(v2), 0);     // xxxx2 = { xx2, xx2 }
  gvset_hi(uvrg_xxxx2, tmp_lo);

  u32 y0_y1 = (u16)a->y | (b->y << 16); // y0_y1 = { y0, y1 }
  u32 y1_y2 = (u16)b->y | (c->y << 16); // y1_y2 = { y1, y2 }

  gvsubq_u16(d0_ab, uvrg_xxxx1, uvrg_xxxx0);

  u32 b1_b2 = b->b | (c->b << 16);      // b1_b2 = { b1, b2 }

  gvsubq_u16(d2_ab, uvrg_xxxx2, uvrg_xxxx1);

  gvsubq_u16(d1_ab, y2_ab, y1_ab);

  u32 b0_b1 = a->b | (b->b << 16);      // b0_b1 = { b0, b1 }

  u32 dx, dy, db;
  gssub16(dx, x1_x2, x0_x1);            // dx = { x1 - x0, x2 - x1 }
  gssub16(dy, y1_y2, y0_y1);            // dy = { y1 - y0, y2 - y1 }
  gssub16(db, b1_b2, b0_b1);            // db = { b1 - b0, b2 - b1 }

  u32 ga_by, ga_bx;
  gvsubq_u16(d3_ab, y1_ab, y0_ab);
  gsmusdx(ga_by, dx, db);               // ga_by = ((x1 - x0) * (b2 - b1)) -
                                        //         ((x2 - X1) * (b1 - b0))
  gvmull_s16(ga_uvrg_x, gvlo(d0_ab), gvlo(d1_ab));
  gsmusdx(ga_bx, db, dy);               // ga_bx = ((b1 - b0) * (y2 - y1)) -
                                        //         ((b2 - b1) * (y1 - y0))
  gvmlsl_s16(ga_uvrg_x, gvlo(d2_ab), gvlo(d3_ab));
  u32 gs_bx = (s32)ga_bx >> 31;         // movs

  gvmull_s16(ga_uvrg_y, gvhi(d0_ab), gvhi(d1_ab));
  if ((s32)gs_bx < 0) ga_bx = -ga_bx;   // rsbmi

  gvmlsl_s16(ga_uvrg_y, gvhi(d2_ab), gvhi(d3_ab));
  u32 gs_by = (s32)ga_by >> 31;         // movs

  gvhreg d0;
  gvshr_n_u64(d0, d30.v, 22);           // note: on "d30 >> 22" gcc generates junk code

  gvdupq_n_u32(uvrgb_phase, psx_gpu->uvrgb_phase);
  u32 b_base = psx_gpu->uvrgb_phase + (a->b << 16);

  if ((s32)gs_by < 0) ga_by = -ga_by;   // rsbmi
  gvreg gs_uvrg_x, gs_uvrg_y;
  gs_uvrg_x.s32 = ga_uvrg_x.s32 < 0;    // gs_uvrg_x = ga_uvrg_x < 0
  gs_uvrg_y.s32 = ga_uvrg_y.s32 < 0;    // gs_uvrg_y = ga_uvrg_y < 0

  gvdupq_n_u32(w_mask, -psx_gpu->triangle_winding); // w_mask = { -w, -w, -w, -w }
  shift -= 62 - 12;                     // shift -= (62 - FIXED_BITS)

  gvreg uvrg_base;
  gvshll_n_u16(uvrg_base, gvlo(uvrg_xxxx0), 16); // uvrg_base = uvrg0 << 16

  gvaddq_u32(uvrg_base, uvrg_base, uvrgb_phase);
  gvabsq_s32(ga_uvrg_x, ga_uvrg_x);     // ga_uvrg_x = abs(ga_uvrg_x)

  u32 area_r_s = d0.u32[0];             // area_r_s = triangle_reciprocal
  gvabsq_s32(ga_uvrg_y, ga_uvrg_y);     // ga_uvrg_y = abs(ga_uvrg_y)

  gvmull_l_u32(gw_rg_x, gvhi(ga_uvrg_x), d0, 0);
  gvmull_l_u32(ga_uvrg_x, gvlo(ga_uvrg_x), d0, 0);
  gvmull_l_u32(gw_rg_y, gvhi(ga_uvrg_y), d0, 0);
  gvmull_l_u32(ga_uvrg_y, gvlo(ga_uvrg_y), d0, 0);

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  gvdupq_n_s64(r_shift, shift);         // r_shift = { shift, shift }
  gvshlq_u64(gw_rg_x, gw_rg_x, r_shift);
  gvshlq_u64(ga_uvrg_x, ga_uvrg_x, r_shift);
  gvshlq_u64(gw_rg_y, gw_rg_y, r_shift);
  gvshlq_u64(ga_uvrg_y, ga_uvrg_y, r_shift);
#elif defined(__SSE2__)
  r_shift.m   = _mm_cvtsi32_si128(-shift);
  gw_rg_x.m   = _mm_srl_epi64(gw_rg_x.m, r_shift.m);
  ga_uvrg_x.m = _mm_srl_epi64(ga_uvrg_x.m, r_shift.m);
  gw_rg_y.m   = _mm_srl_epi64(gw_rg_y.m, r_shift.m);
  ga_uvrg_y.m = _mm_srl_epi64(ga_uvrg_y.m, r_shift.m);
#else
  gvdupq_n_s64(r_shift, -shift);        // r_shift = { shift, shift }
  gvshrq_u64(gw_rg_x, gw_rg_x, r_shift);
  gvshrq_u64(ga_uvrg_x, ga_uvrg_x, r_shift);
  gvshrq_u64(gw_rg_y, gw_rg_y, r_shift);
  gvshrq_u64(ga_uvrg_y, ga_uvrg_y, r_shift);
#endif

  gveorq(gs_uvrg_x, gs_uvrg_x, w_mask);
  gvmovn_u64(tmp_lo, ga_uvrg_x);

  gveorq(gs_uvrg_y, gs_uvrg_y, w_mask);
  gvmovn_u64(tmp_hi, gw_rg_x);

  gvcombine_u32(ga_uvrg_x, tmp_lo, tmp_hi);

  gveorq(ga_uvrg_x, ga_uvrg_x, gs_uvrg_x);
  gvmovn_u64(tmp_lo, ga_uvrg_y);

  gvsubq_u32(ga_uvrg_x, ga_uvrg_x, gs_uvrg_x);
  gvmovn_u64(tmp_hi, gw_rg_y);

  gvcombine_u32(ga_uvrg_y, tmp_lo, tmp_hi);

  gveorq(ga_uvrg_y, ga_uvrg_y, gs_uvrg_y);
  ga_bx = ga_bx << 13;

  gvsubq_u32(ga_uvrg_y, ga_uvrg_y, gs_uvrg_y);
  ga_by = ga_by << 13;

  u32 gw_bx_h, gw_by_h;
  gw_bx_h = (u64)ga_bx * area_r_s >> 32;

  gvshlq_n_u32(ga_uvrg_x, ga_uvrg_x, 4);
  gvshlq_n_u32(ga_uvrg_y, ga_uvrg_y, 4);

  gw_by_h = (u64)ga_by * area_r_s >> 32;
  gvdup_n_u32(tmp_lo, a->x);
  gvmlsq_l_s32(uvrg_base, ga_uvrg_x, tmp_lo, 0);

  gs_bx = gs_bx ^ -psx_gpu->triangle_winding;
  gvaddq_u32(uvrg_dx2, ga_uvrg_x, ga_uvrg_x);

  gs_by = gs_by ^ -psx_gpu->triangle_winding;

  u32 r11 = -shift;                         // r11 = negative shift for scalar lsr
  u32 *store_a = psx_gpu->uvrg.e;
  r11 = r11 - (32 - 13);
  u32 *store_b = store_a + 16 / sizeof(u32);

  gvaddq_u32(uvrg_dx3, uvrg_dx2, ga_uvrg_x);
  gvst1q_inc_u32(uvrg_base, store_a, 32);

  gvst1q_inc_u32(ga_uvrg_x, store_b, 32);
  u32 g_bx = (u32)gw_bx_h >> r11;

  gvst1q_inc_u32(ga_uvrg_y, store_a, 32);
  u32 g_by = (u32)gw_by_h >> r11;

  gvdup_n_u32(zero, 0);

  gvst4_4_inc_u32(zero, gvlo(ga_uvrg_x), gvlo(uvrg_dx2), gvlo(uvrg_dx3), store_b, 32);
  g_bx = g_bx ^ gs_bx;

  gvst4_4_inc_u32(zero, gvhi(ga_uvrg_x), gvhi(uvrg_dx2), gvhi(uvrg_dx3), store_b, 32);
  g_bx = g_bx - gs_bx;

  g_bx = g_bx << 4;
  g_by = g_by ^ gs_by;

  b_base -= g_bx * a->x;
  g_by = g_by - gs_by;

  g_by = g_by << 4;

  u32 g_bx2 = g_bx + g_bx;
  u32 g_bx3 = g_bx + g_bx2;

  // 112
  store_b[0] = 0;
  store_b[1] = g_bx;
  store_b[2] = g_bx2;
  store_b[3] = g_bx3;
  store_b[4] = b_base;
  store_b[5] = g_by; // 132
}

#define setup_spans_debug_check(span_edge_data_element)                        \

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
  vec_4x32u v_clip;                                                            \
                                                                               \
  vec_2x64s edges_xy;                                                          \
  vec_2x32s edges_dx_dy;                                                       \
  vec_2x32u edge_shifts;                                                       \
                                                                               \
  vec_2x64s left_x, right_x;                                                   \
  vec_2x64s left_dx_dy, right_dx_dy;                                           \
  vec_4x32s left_x_32, right_x_32;                                             \
  vec_2x32s left_x_32_lo, right_x_32_lo;                                       \
  vec_2x32s left_x_32_hi, right_x_32_hi;                                       \
  vec_4x16s left_right_x_16_lo, left_right_x_16_hi;                            \
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
  vec_4x32u uvrg;                                                              \
  vec_4x32u uvrg_dy;                                                           \
  u32 b = psx_gpu->b;                                                          \
  u32 b_dy = psx_gpu->b_dy;                                                    \
  const u32 *reciprocal_table = psx_gpu->reciprocal_table_ptr;                 \
                                                                               \
  gvld1q_u32(uvrg, psx_gpu->uvrg.e);                                           \
  gvld1q_u32(uvrg_dy, psx_gpu->uvrg_dy.e);                                     \
  gvdup_n_u32(c_0x01, 0x01);                                                   \
  setup_spans_prologue_alternate_##alternate_active()                          \

#define setup_spans_prologue_b()                                               \
  span_edge_data = psx_gpu->span_edge_data;                                    \
  span_uvrg_offset = (vec_4x32u *)psx_gpu->span_uvrg_offset;                   \
  span_b_offset = psx_gpu->span_b_offset;                                      \
                                                                               \
  vec_8x16u c_0x0001;                                                          \
  vec_4x16u c_max_blocks_per_row;                                              \
                                                                               \
  gvdupq_n_u16(c_0x0001, 0x0001);                                              \
  gvdupq_n_u16(left_edge, psx_gpu->viewport_start_x);                          \
  gvdupq_n_u16(right_edge, psx_gpu->viewport_end_x);                           \
  gvaddq_u16(right_edge, right_edge, c_0x0001);                                \
  gvdup_n_u16(c_0x04, 0x04);                                                   \
  gvdup_n_u16(c_0x07, 0x07);                                                   \
  gvdup_n_u16(c_0xFFFE, 0xFFFE);                                               \
  gvdup_n_u16(c_max_blocks_per_row, MAX_BLOCKS_PER_ROW);                       \

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
// better encoding, remaining bits are unused anyway
#define mask_edge_shifts(edge_shifts)                                          \
  gvbic_n_u16(edge_shifts, 0xE0)
#else
#define mask_edge_shifts(edge_shifts)                                          \
  gvand_n_u32(edge_shifts, 0x1F)
#endif

#define compute_edge_delta_x2()                                                \
{                                                                              \
  vec_2x32s heights;                                                           \
  vec_2x32s height_reciprocals;                                                \
  vec_2x32s heights_b;                                                         \
  vec_2x32u widths;                                                            \
                                                                               \
  u32 edge_shift = reciprocal_table[height];                                   \
                                                                               \
  gvdup_n_u32(heights, height);                                                \
  gvsub_u32(widths, x_ends, x_starts);                                         \
                                                                               \
  gvdup_n_u32(edge_shifts, edge_shift);                                        \
  gvsub_u32(heights_b, heights, c_0x01);                                       \
  gvshr_n_u32(height_reciprocals, edge_shifts, 10);                            \
                                                                               \
  gvmla_s32(heights_b, x_starts, heights);                                     \
  mask_edge_shifts(edge_shifts);                                               \
  gvmul_s32(edges_dx_dy, widths, height_reciprocals);                          \
  gvmull_s32(edges_xy, heights_b, height_reciprocals);                         \
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
  gvcreate_u32(heights, height_a, height_b);                                   \
  gvcreate_u32(edge_shifts, reciprocal_table[height_a], reciprocal_table[height_b]); \
                                                                               \
  edge_shift_alt = reciprocal_table[height_minor_b];                           \
                                                                               \
  gvsub_u32(widths, x_ends, x_starts);                                         \
  width_alt = x_c - start_c;                                                   \
                                                                               \
  gvshr_n_u32(height_reciprocals, edge_shifts, 10);                            \
  height_reciprocal_alt = edge_shift_alt >> 10;                                \
                                                                               \
  mask_edge_shifts(edge_shifts);                                               \
  edge_shift_alt &= 0x1F;                                                      \
                                                                               \
  gvsub_u32(heights_b, heights, c_0x01);                                       \
  height_b_alt = height_minor_b - 1;                                           \
                                                                               \
  gvmla_s32(heights_b, x_starts, heights);                                     \
  height_b_alt += height_minor_b * start_c;                                    \
                                                                               \
  gvmull_s32(edges_xy, heights_b, height_reciprocals);                         \
  edge_alt = (s64)height_b_alt * height_reciprocal_alt;                        \
                                                                               \
  gvmul_s32(edges_dx_dy, widths, height_reciprocals);                          \
  edge_dx_dy_alt = width_alt * height_reciprocal_alt;                          \
}                                                                              \


#define setup_spans_adjust_y_up()                                              \
  gvsub_u32(y_x4, y_x4, c_0x04)                                                \

#define setup_spans_adjust_y_down()                                            \
  gvadd_u32(y_x4, y_x4, c_0x04)                                                \

#define setup_spans_adjust_interpolants_up()                                   \
  gvsubq_u32(uvrg, uvrg, uvrg_dy);                                             \
  b -= b_dy                                                                    \

#define setup_spans_adjust_interpolants_down()                                 \
  gvaddq_u32(uvrg, uvrg, uvrg_dy);                                             \
  b += b_dy                                                                    \


#define setup_spans_clip_interpolants_increment()                              \
  gvmlaq_s32(uvrg, uvrg_dy, v_clip);                                           \
  b += b_dy * clip                                                             \

#define setup_spans_clip_interpolants_decrement()                              \
  gvmlsq_s32(uvrg, uvrg_dy, v_clip);                                           \
  b -= b_dy * clip                                                             \

#define setup_spans_clip_alternate_yes()                                       \
  edge_alt += edge_dx_dy_alt * (s64)(clip)                                     \

#define setup_spans_clip_alternate_no()                                        \

#define setup_spans_clip(direction, alternate_active)                          \
{                                                                              \
  gvdupq_n_u32(v_clip, clip);                                                  \
  gvmlal_s32(edges_xy, edges_dx_dy, gvlo(v_clip));                             \
  setup_spans_clip_alternate_##alternate_active();                             \
  setup_spans_clip_interpolants_##direction();                                 \
}                                                                              \


#define setup_spans_adjust_edges_alternate_no(left_half, right_half)           \
{                                                                              \
  vec_2x64s edge_shifts_64;                                                    \
  vec_2x64s edges_dx_dy_64;                                                    \
  vec_1x64s left_x_hi, right_x_hi;                                             \
                                                                               \
  gvmovl_s32(edge_shifts_64, edge_shifts);                                     \
  gvshlq_u64(edges_xy, edges_xy, edge_shifts_64);                              \
                                                                               \
  gvmovl_s32(edges_dx_dy_64, edges_dx_dy);                                     \
  gvshlq_u64(edges_dx_dy_64, edges_dx_dy_64, edge_shifts_64);                  \
                                                                               \
  gvdupq_l_s64(left_x, gv##left_half(edges_xy), 0);                            \
  gvdupq_l_s64(right_x, gv##right_half(edges_xy), 0);                          \
                                                                               \
  gvdupq_l_s64(left_dx_dy, gv##left_half(edges_dx_dy_64), 0);                  \
  gvdupq_l_s64(right_dx_dy, gv##right_half(edges_dx_dy_64), 0);                \
                                                                               \
  gvadd_s64(left_x_hi, gvlo(left_x), gvlo(left_dx_dy));                        \
  gvadd_s64(right_x_hi, gvlo(right_x), gvlo(right_dx_dy));                     \
                                                                               \
  gvset_hi(left_x, left_x_hi);                                                 \
  gvset_hi(right_x, right_x_hi);                                               \
                                                                               \
  gvaddq_s64(left_dx_dy, left_dx_dy, left_dx_dy);                              \
  gvaddq_s64(right_dx_dy, right_dx_dy, right_dx_dy);                           \
}                                                                              \

#define setup_spans_adjust_edges_alternate_yes(left_half, right_half)          \
{                                                                              \
  setup_spans_adjust_edges_alternate_no(left_half, right_half);                \
  s64 edge_dx_dy_alt_64;                                                       \
  vec_1x64s alternate_x_hi;                                                    \
                                                                               \
  gvdup_n_u16(y_mid_point, y_b);                                               \
                                                                               \
  edge_alt <<= edge_shift_alt;                                                 \
  edge_dx_dy_alt_64 = (s64)edge_dx_dy_alt << edge_shift_alt;                   \
                                                                               \
  gvdupq_n_s64(alternate_x, edge_alt);                                         \
  gvdupq_n_s64(alternate_dx_dy, edge_dx_dy_alt_64);                            \
                                                                               \
  gvadd_s64(alternate_x_hi, gvlo(alternate_x), gvlo(alternate_dx_dy));         \
  gvaddq_s64(alternate_dx_dy, alternate_dx_dy, alternate_dx_dy);               \
  gvset_hi(alternate_x, alternate_x_hi);                                       \
}                                                                              \


#define setup_spans_y_select_up()                                              \
  gvclt_s16(alternate_select, y_x4, y_mid_point)                               \

#define setup_spans_y_select_down()                                            \
  gvcgt_s16(alternate_select, y_x4, y_mid_point)                               \

#define setup_spans_y_select_alternate_yes(direction)                          \
  setup_spans_y_select_##direction()                                           \

#define setup_spans_y_select_alternate_no(direction)                           \

#define setup_spans_alternate_select_left()                                    \
  gvbit(left_right_x_16_lo, alternate_x_16, alternate_select);                 \

#define setup_spans_alternate_select_right()                                   \
  gvbit(left_right_x_16_hi, alternate_x_16, alternate_select);                 \

#define setup_spans_alternate_select_none()                                    \

#define setup_spans_increment_alternate_yes()                                  \
{                                                                              \
  vec_2x32s alternate_x_32_lo, alternate_x_32_hi;                              \
  gvmovn_top_u64(alternate_x_32_lo, alternate_x);                              \
  gvaddq_s64(alternate_x, alternate_x, alternate_dx_dy);                       \
  gvmovn_top_u64(alternate_x_32_hi, alternate_x);                              \
  gvaddq_s64(alternate_x, alternate_x, alternate_dx_dy);                       \
  gvcombine_u32(alternate_x_32, alternate_x_32_lo, alternate_x_32_hi);         \
  gvmovn_u32(alternate_x_16, alternate_x_32);                                  \
}                                                                              \

#define setup_spans_increment_alternate_no()                                   \

#if defined(__SSE2__) && !(defined(__AVX512BW__) && defined(__AVX512VL__))
#define setup_spans_make_span_shift(span_shift) {                              \
  gvreg tab1_ = { .u8 = { 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00 } };  \
  gvtbl2_u8(span_shift, tab1_, span_shift);                                    \
  gvorr_n_u16(span_shift, 0xff00);                                             \
  (void)c_0xFFFE;                                                              \
}
#else
#define setup_spans_make_span_shift(span_shift)                                \
  gvshl_u16(span_shift, c_0xFFFE, span_shift)
#endif

#define setup_spans_set_x4(alternate, direction, alternate_active)             \
{                                                                              \
  gvst1q_pi_u32(uvrg, span_uvrg_offset);                                       \
  *span_b_offset++ = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  gvst1q_pi_u32(uvrg, span_uvrg_offset);                                       \
  *span_b_offset++ = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  gvst1q_pi_u32(uvrg, span_uvrg_offset);                                       \
  *span_b_offset++ = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  gvst1q_pi_u32(uvrg, span_uvrg_offset);                                       \
  *span_b_offset++ = b;                                                        \
  setup_spans_adjust_interpolants_##direction();                               \
                                                                               \
  gvmovn_top_u64(left_x_32_lo, left_x);                                        \
  gvmovn_top_u64(right_x_32_lo, right_x);                                      \
                                                                               \
  gvaddq_s64(left_x, left_x, left_dx_dy);                                      \
  gvaddq_s64(right_x, right_x, right_dx_dy);                                   \
                                                                               \
  gvmovn_top_u64(left_x_32_hi, left_x);                                        \
  gvmovn_top_u64(right_x_32_hi, right_x);                                      \
                                                                               \
  gvaddq_s64(left_x, left_x, left_dx_dy);                                      \
  gvaddq_s64(right_x, right_x, right_dx_dy);                                   \
                                                                               \
  gvcombine_s64(left_x_32, left_x_32_lo, left_x_32_hi);                        \
  gvcombine_s64(right_x_32, right_x_32_lo, right_x_32_hi);                     \
                                                                               \
  gvmovn_u32(left_right_x_16_lo, left_x_32);                                   \
  gvmovn_u32(left_right_x_16_hi, right_x_32);                                  \
                                                                               \
  setup_spans_increment_alternate_##alternate_active();                        \
  setup_spans_y_select_alternate_##alternate_active(direction);                \
  setup_spans_alternate_select_##alternate();                                  \
                                                                               \
  gvmax_s16(left_right_x_16_lo, left_right_x_16_lo, gvlo(left_edge));          \
  gvmax_s16(left_right_x_16_hi, left_right_x_16_hi, gvhi(left_edge));          \
  gvmin_s16(left_right_x_16_lo, left_right_x_16_lo, gvlo(right_edge));         \
  gvmin_s16(left_right_x_16_hi, left_right_x_16_hi, gvhi(right_edge));         \
                                                                               \
  gvsub_u16(left_right_x_16_hi, left_right_x_16_hi, left_right_x_16_lo);       \
  gvadd_u16(left_right_x_16_hi, left_right_x_16_hi, c_0x07);                   \
  gvand(span_shift, left_right_x_16_hi, c_0x07);                               \
  setup_spans_make_span_shift(span_shift);                                     \
  gvshr_n_u16(left_right_x_16_hi, left_right_x_16_hi, 3);                      \
  gvmin_u16(left_right_x_16_hi, left_right_x_16_hi, c_max_blocks_per_row);     \
                                                                               \
  gvst4_pi_u16(left_right_x_16_lo, left_right_x_16_hi, span_shift, y_x4,       \
    span_edge_data);                                                           \
                                                                               \
  setup_spans_adjust_y_##direction();                                          \
}                                                                              \


#define setup_spans_alternate_adjust_yes()                                     \
  edge_alt -= edge_dx_dy_alt * (s64)height_minor_a                             \

#define setup_spans_alternate_adjust_no()                                      \


#define setup_spans_down(left_half, right_half, alternate, alternate_active)   \
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
    u64 y_x4_ = ((u64)(y_a + 3) << 48) | ((u64)(u16)(y_a + 2) << 32)           \
              | (u32)((y_a + 1) << 16) | (u16)y_a;                             \
    gvcreate_u64(y_x4, y_x4_);                                                 \
    setup_spans_adjust_edges_alternate_##alternate_active(left_half, right_half); \
                                                                               \
    psx_gpu->num_spans = height;                                               \
    do                                                                         \
    {                                                                          \
      setup_spans_set_x4(alternate, down, alternate_active);                   \
      height -= 4;                                                             \
    } while(height > 0);                                                       \
    if (psx_gpu->hacks_active & (AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V))   \
    {                                                                          \
      vec_2x32u tmp;                                                           \
      gvld1_u64(tmp, &span_uvrg_offset[height - 2]);                           \
      gvst1_u64(tmp, &span_uvrg_offset[height - 1]);                           \
    }                                                                          \
  }                                                                            \


#define setup_spans_alternate_pre_increment_yes()                              \
  edge_alt += edge_dx_dy_alt                                                   \

#define setup_spans_alternate_pre_increment_no()                               \

#define setup_spans_up_decrement_height_yes()                                  \
  height--                                                                     \

#define setup_spans_up_decrement_height_no()                                   \
  {}                                                                           \

#define setup_spans_up(left_half, right_half, alternate, alternate_active)     \
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
    u64 y_x4_ = ((u64)(y_a - 3) << 48) | ((u64)(u16)(y_a - 2) << 32)           \
              | (u32)((y_a - 1) << 16) | (u16)y_a;                             \
    gvcreate_u64(y_x4, y_x4_);                                                 \
    gvaddw_s32(edges_xy, edges_xy, edges_dx_dy);                               \
    setup_spans_alternate_pre_increment_##alternate_active();                  \
    setup_spans_adjust_edges_alternate_##alternate_active(left_half, right_half); \
    setup_spans_adjust_interpolants_up();                                      \
                                                                               \
    psx_gpu->num_spans = height;                                               \
    while(height > 0)                                                          \
    {                                                                          \
      setup_spans_set_x4(alternate, up, alternate_active);                     \
      height -= 4;                                                             \
    }                                                                          \
    if (psx_gpu->hacks_active & AHACK_TEXTURE_ADJ_V)                           \
    {                                                                          \
      vec_2x32u tmp;                                                           \
      gvld1_u64(tmp, &psx_gpu->span_uvrg_offset[1]);                           \
      gvst1_u64(tmp, &psx_gpu->span_uvrg_offset[0]);                           \
    }                                                                          \
  }                                                                            \

#define half_left  lo
#define half_right hi

#define setup_spans_up_up(minor, major)                                        \
  setup_spans_prologue(yes);                                                   \
  s32 height_minor_a = y_a - y_b;                                              \
  s32 height_minor_b = y_b - y_c;                                              \
  s32 height = y_a - y_c;                                                      \
                                                                               \
  gvdup_n_u32(x_starts, x_a);                                                  \
  gvcreate_u32(x_ends, x_c, x_b);                                              \
                                                                               \
  compute_edge_delta_x3(x_b, height, height_minor_a);                          \
  setup_spans_up(half_##major, half_##minor, minor, yes)                       \

void setup_spans_up_left(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_up_left_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_up_up(left, right)
}

void setup_spans_up_right(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_up_right_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_up_up(right, left)
}

#define setup_spans_down_down(minor, major)                                    \
  setup_spans_prologue(yes);                                                   \
  s32 height_minor_a = y_b - y_a;                                              \
  s32 height_minor_b = y_c - y_b;                                              \
  s32 height = y_c - y_a;                                                      \
                                                                               \
  gvdup_n_u32(x_starts, x_a);                                                  \
  gvcreate_u32(x_ends, x_c, x_b);                                              \
                                                                               \
  compute_edge_delta_x3(x_b, height, height_minor_a);                          \
  setup_spans_down(half_##major, half_##minor, minor, yes)                     \

void setup_spans_down_left(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_down_left_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_down_down(left, right)
}

void setup_spans_down_right(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_down_right_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_down_down(right, left)
}

#define setup_spans_up_flat()                                                  \
  s32 height = y_a - y_c;                                                      \
                                                                               \
  compute_edge_delta_x2();                                                     \
  setup_spans_up(half_left, half_right, none, no)                              \

void setup_spans_up_a(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_up_a_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_prologue(no);

  gvcreate_u32(x_starts, x_a, x_b);
  gvdup_n_u32(x_ends, x_c);

  setup_spans_up_flat()
}

void setup_spans_up_b(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_up_b_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_prologue(no);

  gvdup_n_u32(x_starts, x_a);
  gvcreate_u32(x_ends, x_b, x_c);

  setup_spans_up_flat()
}

#define setup_spans_down_flat()                                                \
  s32 height = y_c - y_a;                                                      \
                                                                               \
  compute_edge_delta_x2();                                                     \
  setup_spans_down(half_left, half_right, none, no)                            \

void setup_spans_down_a(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_down_a_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_prologue(no);

  gvcreate_u32(x_starts, x_a, x_b);
  gvdup_n_u32(x_ends, x_c);

  setup_spans_down_flat()
}

void setup_spans_down_b(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_down_b_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
  setup_spans_prologue(no)

  gvdup_n_u32(x_starts, x_a);
  gvcreate_u32(x_ends, x_b, x_c);

  setup_spans_down_flat()
}

void setup_spans_up_down(psx_gpu_struct *psx_gpu, vertex_struct *v_a,
 vertex_struct *v_b, vertex_struct *v_c)
{
#if 0
  setup_spans_up_down_(psx_gpu, v_a, v_b, v_c);
  return;
#endif
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
  vec_1x64s edges_xy_b_left;
  vec_2x32s edges_dx_dy_b;
  vec_2x32u edge_shifts_b;

  vec_2x32s height_increment;

  gvcreate_u32(x_starts, x_a, x_c);
  gvdup_n_u32(x_ends, x_b);

  compute_edge_delta_x3(x_a, height_minor_a, height_major);

  gvcreate_s32(height_increment, 0, height_minor_b);

  gvmlal_s32(edges_xy, edges_dx_dy, height_increment);

  gvcreate_s64(edges_xy_b_left, edge_alt);
  gvcombine_s64(edges_xy_b, edges_xy_b_left, gvhi(edges_xy));

  edge_shifts_b = edge_shifts;
  gvmov_l_u32(edge_shifts_b, edge_shift_alt, 0);

  gvneg_s32(edges_dx_dy_b, edges_dx_dy);
  gvmov_l_s32(edges_dx_dy_b, edge_dx_dy_alt, 0);

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
    u64 y_x4_ = ((u64)(y_a - 3) << 48) | ((u64)(u16)(y_a - 2) << 32)
              | (u32)((y_a - 1) << 16) | (u16)y_a;
    gvcreate_u64(y_x4, y_x4_);
    gvaddw_s32(edges_xy, edges_xy, edges_dx_dy);
    setup_spans_adjust_edges_alternate_no(lo, hi);
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

  gvld1q_u32(uvrg, psx_gpu->uvrg.e);
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
    u64 y_x4_ = ((u64)(y_a + 3) << 48) | ((u64)(u16)(y_a + 2) << 32)
              | (u32)((y_a + 1) << 16) | (u16)y_a;
    gvcreate_u64(y_x4, y_x4_);
    setup_spans_adjust_edges_alternate_no(lo, hi);

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
      vec_2x32u tmp;
      gvld1_u64(tmp, &span_uvrg_offset[height_minor_b - 2]);
      gvst1_u64(tmp, &span_uvrg_offset[height_minor_b - 1]);
    }
  }
}


#define dither_table_entry_normal(value)                                       \
  (value)                                                                      \

#define setup_blocks_load_msb_mask_indirect()                                  \

#define setup_blocks_load_msb_mask_direct()                                    \
  vec_8x16u msb_mask;                                                          \
  gvdupq_n_u16(msb_mask, psx_gpu->mask_msb);                                   \

#define setup_blocks_variables_shaded_textured(target)                         \
  vec_4x32u u_block;                                                           \
  vec_4x32u v_block;                                                           \
  vec_4x32u r_block;                                                           \
  vec_4x32u g_block;                                                           \
  vec_4x32u b_block;                                                           \
  vec_4x32u uvrg_dx;                                                           \
  vec_4x32u uvrg_dx4;                                                          \
  vec_4x32u uvrg_dx8;                                                          \
  vec_4x32u uvrg;                                                              \
  vec_16x8u texture_mask;                                                      \
  vec_8x8u texture_mask_lo, texture_mask_hi;                                   \
  u32 b_dx = psx_gpu->b_block_span.e[1];                                       \
  u32 b_dx4 = b_dx << 2;                                                       \
  u32 b_dx8 = b_dx << 3;                                                       \
  u32 b;                                                                       \
                                                                               \
  gvld1q_u32(uvrg_dx, psx_gpu->uvrg_dx.e);                                     \
  gvshlq_n_u32(uvrg_dx4, uvrg_dx, 2);                                          \
  gvshlq_n_u32(uvrg_dx8, uvrg_dx, 3);                                          \
  gvld2_u8_dup(texture_mask_lo, texture_mask_hi, &psx_gpu->texture_mask_width); \
  gvcombine_u16(texture_mask, texture_mask_lo, texture_mask_hi)                \

#define setup_blocks_variables_shaded_untextured(target)                       \
  vec_4x32u r_block;                                                           \
  vec_4x32u g_block;                                                           \
  vec_4x32u b_block;                                                           \
  vec_4x32u rgb_dx;                                                            \
  vec_2x32u rgb_dx_lo, rgb_dx_hi;                                              \
  vec_4x32u rgb_dx4;                                                           \
  vec_4x32u rgb_dx8;                                                           \
  vec_4x32u rgb;                                                               \
  vec_2x32u rgb_lo, rgb_hi;                                                    \
                                                                               \
  vec_8x8u d64_0x07;                                                           \
  vec_8x8u d64_1;                                                              \
  vec_8x8u d64_4;                                                              \
  vec_8x8u d64_128;                                                            \
                                                                               \
  gvdup_n_u8(d64_0x07, 0x07);                                                  \
  gvdup_n_u8(d64_1, 1);                                                        \
  gvdup_n_u8(d64_4, 4);                                                        \
  gvdup_n_u8(d64_128, 128u);                                                   \
                                                                               \
  gvld1_u32(rgb_dx_lo, &psx_gpu->uvrg_dx.e[2]);                                \
  gvcreate_u32(rgb_dx_hi, psx_gpu->b_block_span.e[1], 0);                      \
  gvcombine_u32(rgb_dx, rgb_dx_lo, rgb_dx_hi);                                 \
  gvshlq_n_u32(rgb_dx4, rgb_dx, 2);                                            \
  gvshlq_n_u32(rgb_dx8, rgb_dx, 3)                                             \

#define setup_blocks_variables_unshaded_textured(target)                       \
  vec_4x32u u_block;                                                           \
  vec_4x32u v_block;                                                           \
  vec_2x32u uv_dx;                                                             \
  vec_2x32u uv_dx4;                                                            \
  vec_2x32u uv_dx8;                                                            \
  vec_2x32u uv;                                                                \
  vec_16x8u texture_mask;                                                      \
  vec_8x8u texture_mask_lo, texture_mask_hi;                                   \
                                                                               \
  gvld1_u32(uv_dx, psx_gpu->uvrg_dx.e);                                        \
  gvld1_u32(uv, psx_gpu->uvrg.e);                                              \
  gvshl_n_u32(uv_dx4, uv_dx, 2);                                               \
  gvshl_n_u32(uv_dx8, uv_dx, 3);                                               \
  gvld2_u8_dup(texture_mask_lo, texture_mask_hi, &psx_gpu->texture_mask_width); \
  gvcombine_u16(texture_mask, texture_mask_lo, texture_mask_hi)                \

#define setup_blocks_variables_unshaded_untextured_direct()                    \
  gvorrq(colors, colors, msb_mask)                                             \

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
  gvdupq_n_u16(colors, color);                                                 \
  setup_blocks_variables_unshaded_untextured_##target()                        \

#define setup_blocks_span_initialize_dithered_textured()                       \
  vec_8x16u dither_offsets;                                                    \
  gvshll_n_s8(dither_offsets, dither_offsets_short, 4)                         \

#define setup_blocks_span_initialize_dithered_untextured()                     \
  vec_8x8u dither_offsets;                                                     \
  gvadd_u8(dither_offsets, dither_offsets_short, d64_4)                        \

#define setup_blocks_span_initialize_dithered(texturing)                       \
  u32 dither_row = psx_gpu->dither_table[y & 0x3];                             \
  u32 dither_shift = (span_edge_data->left_x & 0x3) * 8;                       \
  vec_8x8s dither_offsets_short;                                               \
                                                                               \
  dither_row =                                                                 \
   (dither_row >> dither_shift) | ((u64)dither_row << (32 - dither_shift));    \
  gvdup_n_u32(dither_offsets_short, dither_row);                               \
  setup_blocks_span_initialize_dithered_##texturing()                          \

#define setup_blocks_span_initialize_undithered(texturing)                     \

#define setup_blocks_span_initialize_shaded_textured()                         \
{                                                                              \
  u32 left_x = span_edge_data->left_x;                                         \
  vec_4x32u block_span;                                                        \
  vec_4x32u v_left_x;                                                          \
                                                                               \
  gvld1q_u32(uvrg, span_uvrg_offset);                                          \
  gvdupq_n_u32(v_left_x, left_x);                                              \
  gvmlaq_u32(uvrg, uvrg_dx, v_left_x);                                         \
  b = *span_b_offset;                                                          \
  b += b_dx * left_x;                                                          \
                                                                               \
  gvdupq_l_u32(u_block, gvlo(uvrg), 0);                                        \
  gvdupq_l_u32(v_block, gvlo(uvrg), 1);                                        \
  gvdupq_l_u32(r_block, gvhi(uvrg), 0);                                        \
  gvdupq_l_u32(g_block, gvhi(uvrg), 1);                                        \
  gvdupq_n_u32(b_block, b);                                                    \
                                                                               \
  gvld1q_u32(block_span, psx_gpu->u_block_span.e);                             \
  gvaddq_u32(u_block, u_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->v_block_span.e);                             \
  gvaddq_u32(v_block, v_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->r_block_span.e);                             \
  gvaddq_u32(r_block, r_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->g_block_span.e);                             \
  gvaddq_u32(g_block, g_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->b_block_span.e);                             \
  gvaddq_u32(b_block, b_block, block_span);                                    \
}

#define setup_blocks_span_initialize_shaded_untextured()                       \
{                                                                              \
  u32 left_x = span_edge_data->left_x;                                         \
  u32 *span_uvrg_offset_high = (u32 *)span_uvrg_offset + 2;                    \
  vec_4x32u block_span;                                                        \
  vec_4x32u v_left_x;                                                          \
                                                                               \
  gvld1_u32(rgb_lo, span_uvrg_offset_high);                                    \
  gvcreate_u32(rgb_hi, *span_b_offset, 0);                                     \
  gvcombine_u32(rgb, rgb_lo, rgb_hi);                                          \
  gvdupq_n_u32(v_left_x, left_x);                                              \
  gvmlaq_u32(rgb, rgb_dx, v_left_x);                                           \
                                                                               \
  gvdupq_l_u32(r_block, gvlo(rgb), 0);                                         \
  gvdupq_l_u32(g_block, gvlo(rgb), 1);                                         \
  gvdupq_l_u32(b_block, gvhi(rgb), 0);                                         \
                                                                               \
  gvld1q_u32(block_span, psx_gpu->r_block_span.e);                             \
  gvaddq_u32(r_block, r_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->g_block_span.e);                             \
  gvaddq_u32(g_block, g_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->b_block_span.e);                             \
  gvaddq_u32(b_block, b_block, block_span);                                    \
}                                                                              \

#define setup_blocks_span_initialize_unshaded_textured()                       \
{                                                                              \
  u32 left_x = span_edge_data->left_x;                                         \
  vec_4x32u block_span;                                                        \
  vec_2x32u v_left_x;                                                          \
                                                                               \
  gvld1_u32(uv, span_uvrg_offset);                                             \
  gvdup_n_u32(v_left_x, left_x);                                               \
  gvmla_u32(uv, uv_dx, v_left_x);                                              \
                                                                               \
  gvdupq_l_u32(u_block, uv, 0);                                                \
  gvdupq_l_u32(v_block, uv, 1);                                                \
                                                                               \
  gvld1q_u32(block_span, psx_gpu->u_block_span.e);                             \
  gvaddq_u32(u_block, u_block, block_span);                                    \
  gvld1q_u32(block_span, psx_gpu->v_block_span.e);                             \
  gvaddq_u32(v_block, v_block, block_span);                                    \
  (void)(span_b_offset);                                                       \
}                                                                              \

#define setup_blocks_span_initialize_unshaded_untextured()                     \
  (void)(span_uvrg_offset);                                                    \
  (void)(span_b_offset)                                                        \

#define setup_blocks_texture_swizzled()                                        \
{                                                                              \
  vec_8x8u u_saved = u;                                                        \
  gvsli_n_u8(u, v, 4);                                                         \
  gvsri_n_u8(v, u_saved, 4);                                                   \
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
  vec_4x16u u_whole_lo, u_whole_hi;                                            \
  vec_4x16u v_whole_lo, v_whole_hi;                                            \
  vec_4x16u r_whole_lo, r_whole_hi;                                            \
  vec_4x16u g_whole_lo, g_whole_hi;                                            \
  vec_4x16u b_whole_lo, b_whole_hi;                                            \
                                                                               \
  vec_8x8u u;                                                                  \
  vec_8x8u v;                                                                  \
  vec_8x8u r;                                                                  \
  vec_8x8u g;                                                                  \
  vec_8x8u b;                                                                  \
                                                                               \
  vec_4x32u dx4;                                                               \
  vec_4x32u dx8;                                                               \
                                                                               \
  gvshrn_n_u32(u_whole_lo, u_block, 16);                                       \
  gvshrn_n_u32(v_whole_lo, v_block, 16);                                       \
  gvshrn_n_u32(r_whole_lo, r_block, 16);                                       \
  gvshrn_n_u32(g_whole_lo, g_block, 16);                                       \
  gvshrn_n_u32(b_whole_lo, b_block, 16);                                       \
                                                                               \
  gvdupq_l_u32(dx4, gvlo(uvrg_dx4), 0);                                        \
  gvaddhn_u32(u_whole_hi, u_block, dx4);                                       \
  gvdupq_l_u32(dx4, gvlo(uvrg_dx4), 1);                                        \
  gvaddhn_u32(v_whole_hi, v_block, dx4);                                       \
  gvdupq_l_u32(dx4, gvhi(uvrg_dx4), 0);                                        \
  gvaddhn_u32(r_whole_hi, r_block, dx4);                                       \
  gvdupq_l_u32(dx4, gvhi(uvrg_dx4), 1);                                        \
  gvaddhn_u32(g_whole_hi, g_block, dx4);                                       \
  gvdupq_n_u32(dx4, b_dx4);                                                    \
  gvaddhn_u32(b_whole_hi, b_block, dx4);                                       \
                                                                               \
  gvcombine_u16(u_whole, u_whole_lo, u_whole_hi);                              \
  gvcombine_u16(v_whole, v_whole_lo, v_whole_hi);                              \
  gvcombine_u16(r_whole, r_whole_lo, r_whole_hi);                              \
  gvcombine_u16(g_whole, g_whole_lo, g_whole_hi);                              \
  gvcombine_u16(b_whole, b_whole_lo, b_whole_hi);                              \
  gvmovn_u16(u, u_whole);                                                      \
  gvmovn_u16(v, v_whole);                                                      \
  gvmovn_u16(r, r_whole);                                                      \
  gvmovn_u16(g, g_whole);                                                      \
  gvmovn_u16(b, b_whole);                                                      \
                                                                               \
  gvdupq_l_u32(dx8, gvlo(uvrg_dx8), 0);                                        \
  gvaddq_u32(u_block, u_block, dx8);                                           \
  gvdupq_l_u32(dx8, gvlo(uvrg_dx8), 1);                                        \
  gvaddq_u32(v_block, v_block, dx8);                                           \
  gvdupq_l_u32(dx8, gvhi(uvrg_dx8), 0);                                        \
  gvaddq_u32(r_block, r_block, dx8);                                           \
  gvdupq_l_u32(dx8, gvhi(uvrg_dx8), 1);                                        \
  gvaddq_u32(g_block, g_block, dx8);                                           \
  gvdupq_n_u32(dx8, b_dx8);                                                    \
  gvaddq_u32(b_block, b_block, dx8);                                           \
                                                                               \
  gvand(u, u, gvlo(texture_mask));                                             \
  gvand(v, v, gvhi(texture_mask));                                             \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  gvst2_u8(u, v, (u8 *)block->uv.e);                                           \
  gvst1_u8(r, block->r.e);                                                     \
  gvst1_u8(g, block->g.e);                                                     \
  gvst1_u8(b, block->b.e);                                                     \
  gvst1q_u16(dither_offsets, (u16 *)block->dither_offsets.e);                  \
  block->fb_ptr = fb_ptr;                                                      \
}                                                                              \

#define setup_blocks_store_unshaded_textured(swizzling, dithering, target,     \
 edge_type)                                                                    \
{                                                                              \
  vec_8x16u u_whole;                                                           \
  vec_8x16u v_whole;                                                           \
  vec_4x16u u_whole_lo, u_whole_hi;                                            \
  vec_4x16u v_whole_lo, v_whole_hi;                                            \
                                                                               \
  vec_8x8u u;                                                                  \
  vec_8x8u v;                                                                  \
                                                                               \
  vec_4x32u dx4;                                                               \
  vec_4x32u dx8;                                                               \
                                                                               \
  gvshrn_n_u32(u_whole_lo, u_block, 16);                                       \
  gvshrn_n_u32(v_whole_lo, v_block, 16);                                       \
                                                                               \
  gvdupq_l_u32(dx4, uv_dx4, 0);                                                \
  gvaddhn_u32(u_whole_hi, u_block, dx4);                                       \
  gvdupq_l_u32(dx4, uv_dx4, 1);                                                \
  gvaddhn_u32(v_whole_hi, v_block, dx4);                                       \
                                                                               \
  gvcombine_u16(u_whole, u_whole_lo, u_whole_hi);                              \
  gvcombine_u16(v_whole, v_whole_lo, v_whole_hi);                              \
  gvmovn_u16(u, u_whole);                                                      \
  gvmovn_u16(v, v_whole);                                                      \
                                                                               \
  gvdupq_l_u32(dx8, uv_dx8, 0);                                                \
  gvaddq_u32(u_block, u_block, dx8);                                           \
  gvdupq_l_u32(dx8, uv_dx8, 1);                                                \
  gvaddq_u32(v_block, v_block, dx8);                                           \
                                                                               \
  gvand(u, u, gvlo(texture_mask));                                             \
  gvand(v, v, gvhi(texture_mask));                                             \
  setup_blocks_texture_##swizzling();                                          \
                                                                               \
  gvst2_u8(u, v, (u8 *)block->uv.e);                                           \
  gvst1q_u16(dither_offsets, (u16 *)block->dither_offsets.e);                  \
  block->fb_ptr = fb_ptr;                                                      \
}                                                                              \

#define setup_blocks_store_shaded_untextured_dithered()                        \
  gvqadd_u8(r, r, dither_offsets);                                             \
  gvqadd_u8(g, g, dither_offsets);                                             \
  gvqadd_u8(b, b, dither_offsets);                                             \
                                                                               \
  gvqsub_u8(r, r, d64_4);                                                      \
  gvqsub_u8(g, g, d64_4);                                                      \
  gvqsub_u8(b, b, d64_4)                                                       \

#define setup_blocks_store_shaded_untextured_undithered()                      \

#define setup_blocks_store_untextured_pixels_indirect_full(_pixels)            \
  gvst1q_u16(_pixels, block->pixels.e);                                        \
  block->fb_ptr = fb_ptr                                                       \

#define setup_blocks_store_untextured_pixels_indirect_edge(_pixels)            \
  gvst1q_u16(_pixels, block->pixels.e);                                        \
  block->fb_ptr = fb_ptr                                                       \

#define setup_blocks_store_shaded_untextured_seed_pixels_indirect()            \
  gvmull_u8(pixels, r, d64_1)                                                  \

#define setup_blocks_store_untextured_pixels_direct_full(_pixels)              \
  gvst1q_u16(_pixels, fb_ptr)                                                  \

#define setup_blocks_store_untextured_pixels_direct_edge(_pixels)              \
{                                                                              \
  vec_8x16u fb_pixels;                                                         \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u test_mask;                                                         \
                                                                               \
  gvld1q_u16(test_mask, psx_gpu->test_mask.e);                                 \
  gvld1q_u16(fb_pixels, fb_ptr);                                               \
  gvdupq_n_u16(draw_mask, span_edge_data->right_mask);                         \
  gvtstq_u16(draw_mask, draw_mask, test_mask);                                 \
  gvbifq(fb_pixels, _pixels, draw_mask);                                       \
  gvst1q_u16(fb_pixels, fb_ptr);                                               \
}                                                                              \

#define setup_blocks_store_shaded_untextured_seed_pixels_direct()              \
  pixels = msb_mask;                                                           \
  gvmlal_u8(pixels, r, d64_1)                                                  \

#define setup_blocks_store_shaded_untextured(swizzling, dithering, target,     \
 edge_type)                                                                    \
{                                                                              \
  vec_8x16u r_whole;                                                           \
  vec_8x16u g_whole;                                                           \
  vec_8x16u b_whole;                                                           \
  vec_4x16u r_whole_lo, r_whole_hi;                                            \
  vec_4x16u g_whole_lo, g_whole_hi;                                            \
  vec_4x16u b_whole_lo, b_whole_hi;                                            \
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
  gvshrn_n_u32(r_whole_lo, r_block, 16);                                       \
  gvshrn_n_u32(g_whole_lo, g_block, 16);                                       \
  gvshrn_n_u32(b_whole_lo, b_block, 16);                                       \
                                                                               \
  gvdupq_l_u32(dx4, gvlo(rgb_dx4), 0);                                         \
  gvaddhn_u32(r_whole_hi, r_block, dx4);                                       \
  gvdupq_l_u32(dx4, gvlo(rgb_dx4), 1);                                         \
  gvaddhn_u32(g_whole_hi, g_block, dx4);                                       \
  gvdupq_l_u32(dx4, gvhi(rgb_dx4), 0);                                         \
  gvaddhn_u32(b_whole_hi, b_block, dx4);                                       \
                                                                               \
  gvcombine_u16(r_whole, r_whole_lo, r_whole_hi);                              \
  gvcombine_u16(g_whole, g_whole_lo, g_whole_hi);                              \
  gvcombine_u16(b_whole, b_whole_lo, b_whole_hi);                              \
  gvmovn_u16(r, r_whole);                                                      \
  gvmovn_u16(g, g_whole);                                                      \
  gvmovn_u16(b, b_whole);                                                      \
                                                                               \
  gvdupq_l_u32(dx8, gvlo(rgb_dx8), 0);                                         \
  gvaddq_u32(r_block, r_block, dx8);                                           \
  gvdupq_l_u32(dx8, gvlo(rgb_dx8), 1);                                         \
  gvaddq_u32(g_block, g_block, dx8);                                           \
  gvdupq_l_u32(dx8, gvhi(rgb_dx8), 0);                                         \
  gvaddq_u32(b_block, b_block, dx8);                                           \
                                                                               \
  setup_blocks_store_shaded_untextured_##dithering();                          \
                                                                               \
  gvshr_n_u8(r, r, 3);                                                         \
  gvbic(g, g, d64_0x07);                                                       \
  gvbic(b, b, d64_0x07);                                                       \
                                                                               \
  setup_blocks_store_shaded_untextured_seed_pixels_##target();                 \
  gvmlal_u8(pixels, g, d64_4);                                                 \
  gvmlal_u8(pixels, b, d64_128);                                               \
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
  vec_8x16u test_mask;                                                         \
                                                                               \
  gvld1q_u16(test_mask, psx_gpu->test_mask.e);                                 \
  gvdupq_n_u16(bits_mask, bits);                                               \
  gvtstq_u16(bits_mask, bits_mask, test_mask);                                 \
  gvst1q_u16(bits_mask, (_block)->draw_mask.e);                                \
}                                                                              \

#define setup_blocks_store_draw_mask_untextured_direct(_block, bits)           \
  (void)(_block)                                                               \

#define setup_blocks_uv_adj_hack_untextured(_block, edge_data, uvrg_offset)    \

#define setup_blocks_uv_adj_hack_textured(_block, edge_data, uvrg_offset)      \
{                                                                              \
  u32 m_ = AHACK_TEXTURE_ADJ_U | AHACK_TEXTURE_ADJ_V;                          \
  if (unlikely(psx_gpu->hacks_active & m_))                                    \
    setup_blocks_uv_adj_hack(psx_gpu, _block, edge_data, (void *)uvrg_offset); \
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

#define setup_blocks_do(shading, texturing, dithering, sw, target)             \
  setup_blocks_load_msb_mask_##target();                                       \
  setup_blocks_variables_##shading##_##texturing(target);                      \
                                                                               \
  edge_data_struct *span_edge_data = psx_gpu->span_edge_data;                  \
  vec_4x32u *span_uvrg_offset = (vec_4x32u *)psx_gpu->span_uvrg_offset;        \
  u32 *span_b_offset = psx_gpu->span_b_offset;                                 \
                                                                               \
  block_struct *block = psx_gpu->blocks + psx_gpu->num_blocks;                 \
                                                                               \
  u32 num_spans = psx_gpu->num_spans;                                          \
                                                                               \
  u16 * __restrict__ fb_ptr;                                                   \
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
                                                                               \
    num_spans--;                                                               \
    span_edge_data++;                                                          \
    span_uvrg_offset++;                                                        \
    span_b_offset++;                                                           \
  }                                                                            \
                                                                               \
  psx_gpu->num_blocks = num_blocks                                             \

void setup_blocks_shaded_textured_dithered_swizzled_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_shaded_textured_dithered_swizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(shaded, textured, dithered, swizzled, indirect);
}

void setup_blocks_shaded_textured_dithered_unswizzled_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_shaded_textured_dithered_unswizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(shaded, textured, dithered, unswizzled, indirect);
}

void setup_blocks_unshaded_textured_dithered_swizzled_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_unshaded_textured_dithered_swizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(unshaded, textured, dithered, swizzled, indirect);
}

void setup_blocks_unshaded_textured_dithered_unswizzled_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_unshaded_textured_dithered_unswizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(unshaded, textured, dithered, unswizzled, indirect);
}

void setup_blocks_unshaded_untextured_undithered_unswizzled_indirect(
 psx_gpu_struct *psx_gpu)
{
#if 0
  setup_blocks_unshaded_untextured_undithered_unswizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(unshaded, untextured, undithered, unswizzled, indirect);
}

void setup_blocks_unshaded_untextured_undithered_unswizzled_direct(
 psx_gpu_struct *psx_gpu)
{
#if 0
  setup_blocks_unshaded_untextured_undithered_unswizzled_direct_(psx_gpu);
  return;
#endif
  setup_blocks_do(unshaded, untextured, undithered, unswizzled, direct);
}

void setup_blocks_shaded_untextured_undithered_unswizzled_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_shaded_untextured_undithered_unswizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(shaded, untextured, undithered, unswizzled, indirect);
}

void setup_blocks_shaded_untextured_dithered_unswizzled_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_shaded_untextured_dithered_unswizzled_indirect_(psx_gpu);
  return;
#endif
  setup_blocks_do(shaded, untextured, dithered, unswizzled, indirect);
}

void setup_blocks_shaded_untextured_undithered_unswizzled_direct(
 psx_gpu_struct *psx_gpu)
{
#if 0
  setup_blocks_shaded_untextured_undithered_unswizzled_direct_(psx_gpu);
  return;
#endif
  setup_blocks_do(shaded, untextured, undithered, unswizzled, direct);
}

void setup_blocks_shaded_untextured_dithered_unswizzled_direct(psx_gpu_struct
 *psx_gpu)
{
#if 0
  setup_blocks_shaded_untextured_dithered_unswizzled_direct_(psx_gpu);
  return;
#endif
  setup_blocks_do(shaded, untextured, dithered, unswizzled, direct);
}

static void update_texture_4bpp_cache(psx_gpu_struct *psx_gpu)
{
  u32 current_texture_page = psx_gpu->current_texture_page;
  u8 *texture_page_ptr = psx_gpu->texture_page_base;
  const u16 *vram_ptr = psx_gpu->vram_ptr;
  u32 tile_x, tile_y;
  u32 sub_y;
  vec_8x16u c_0x00f0;

  vram_ptr += (current_texture_page >> 4) * 256 * 1024;
  vram_ptr += (current_texture_page & 0xF) * 64;

  gvdupq_n_u16(c_0x00f0, 0x00f0);

  psx_gpu->dirty_textures_4bpp_mask &= ~(psx_gpu->current_texture_mask);

  for (tile_y = 16; tile_y; tile_y--)
  {
    for (tile_x = 16; tile_x; tile_x--)
    {
      for (sub_y = 8; sub_y; sub_y--)
      {
        vec_8x8u texel_block_a, texel_block_b;
        vec_8x16u texel_block_expanded_a, texel_block_expanded_b;
        vec_8x16u texel_block_expanded_c, texel_block_expanded_d;
        vec_8x16u texel_block_expanded_ab, texel_block_expanded_cd;

        gvld1_u8(texel_block_a, (u8 *)vram_ptr); vram_ptr += 1024;
        gvld1_u8(texel_block_b, (u8 *)vram_ptr); vram_ptr += 1024;

        gvmovl_u8(texel_block_expanded_a, texel_block_a);
        gvshll_n_u8(texel_block_expanded_b, texel_block_a, 4);
        gvmovl_u8(texel_block_expanded_c, texel_block_b);
        gvshll_n_u8(texel_block_expanded_d, texel_block_b, 4);

        gvbicq(texel_block_expanded_a, texel_block_expanded_a, c_0x00f0);
        gvbicq(texel_block_expanded_b, texel_block_expanded_b, c_0x00f0);
        gvbicq(texel_block_expanded_c, texel_block_expanded_c, c_0x00f0);
        gvbicq(texel_block_expanded_d, texel_block_expanded_d, c_0x00f0);

        gvorrq(texel_block_expanded_ab, texel_block_expanded_a, texel_block_expanded_b);
        gvorrq(texel_block_expanded_cd, texel_block_expanded_c, texel_block_expanded_d);

        gvst1q_2_pi_u32(texel_block_expanded_ab, texel_block_expanded_cd, texture_page_ptr);
      }

      vram_ptr -= (1024 * 16) - 4;
    }

    vram_ptr += (16 * 1024) - (4 * 16);
  }
}

void update_texture_8bpp_cache_slice(psx_gpu_struct *psx_gpu,
 u32 texture_page)
{
#if 0
  update_texture_8bpp_cache_slice_(psx_gpu, texture_page);
  return;
#endif
  u16 *texture_page_ptr = psx_gpu->texture_page_base;
  u16 *vram_ptr = psx_gpu->vram_ptr;

  u32 tile_x, tile_y;
  u32 sub_y;

  vram_ptr += (texture_page >> 4) * 256 * 1024;
  vram_ptr += (texture_page & 0xF) * 64;

  if((texture_page ^ psx_gpu->current_texture_page) & 0x1)
    texture_page_ptr += (8 * 16) * 8;

  for (tile_y = 16; tile_y; tile_y--)
  {
    for (tile_x = 8; tile_x; tile_x--)
    {
      for (sub_y = 4; sub_y; sub_y--)
      {
        vec_4x32u texels_a, texels_b, texels_c, texels_d = {};
        gvld1q_u32(texels_a, vram_ptr); vram_ptr += 1024;
        gvld1q_u32(texels_b, vram_ptr); vram_ptr += 1024;
        gvld1q_u32(texels_c, vram_ptr); vram_ptr += 1024;
        gvld1q_u32(texels_d, vram_ptr); vram_ptr += 1024;

        gvst1q_2_pi_u32(texels_a, texels_b, texture_page_ptr);
        gvst1q_2_pi_u32(texels_c, texels_d, texture_page_ptr);
      }

      vram_ptr -= (1024 * 16) - 8;
    }

    vram_ptr -= (8 * 8);
    vram_ptr += (16 * 1024);

    texture_page_ptr += (8 * 16) * 8;
  }
}

void texture_blocks_untextured(psx_gpu_struct *psx_gpu)
{
}

void texture_blocks_4bpp(psx_gpu_struct *psx_gpu)
{
#if 0
  texture_blocks_4bpp_(psx_gpu);
  return;
#endif
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;

  vec_8x8u texels_low;
  vec_8x8u texels_high;

  vec_16x8u clut_low;
  vec_16x8u clut_high;

  const u8 *texture_ptr_8bpp = psx_gpu->texture_page_ptr;

  gvld2q_u8(clut_low, clut_high, (u8 *)psx_gpu->clut_ptr);

  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_4bpp_mask)
    update_texture_4bpp_cache(psx_gpu);

  while(num_blocks)
  {
    vec_8x8u texels =
    {
      .u8 =
      {
        texture_ptr_8bpp[block->uv.e[0]],
        texture_ptr_8bpp[block->uv.e[1]],
        texture_ptr_8bpp[block->uv.e[2]],
        texture_ptr_8bpp[block->uv.e[3]],
        texture_ptr_8bpp[block->uv.e[4]],
        texture_ptr_8bpp[block->uv.e[5]],
        texture_ptr_8bpp[block->uv.e[6]],
        texture_ptr_8bpp[block->uv.e[7]]
      }
    };

    gvtbl2_u8(texels_low, clut_low, texels);
    gvtbl2_u8(texels_high, clut_high, texels);

    gvst2_u8(texels_low, texels_high, (u8 *)block->texels.e);

    num_blocks--;
    block++;
  }
}

void texture_blocks_8bpp(psx_gpu_struct *psx_gpu)
{
#if 0
  texture_blocks_8bpp_(psx_gpu);
  return;
#endif
  u32 num_blocks = psx_gpu->num_blocks;

  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_8bpp_mask)
    update_texture_8bpp_cache(psx_gpu);

  const u8 * __restrict__ texture_ptr_8bpp = psx_gpu->texture_page_ptr;
  const u16 * __restrict__ clut_ptr = psx_gpu->clut_ptr;
  block_struct * __restrict__ block = psx_gpu->blocks;

  while(num_blocks)
  {
    u16 offset;
    #define load_one(i_) \
      offset = block->uv.e[i_]; u16 texel##i_ = texture_ptr_8bpp[offset]
    #define store_one(i_) \
      block->texels.e[i_] = clut_ptr[texel##i_]
    load_one(0); load_one(1); load_one(2); load_one(3);
    load_one(4); load_one(5); load_one(6); load_one(7);
    store_one(0); store_one(1); store_one(2); store_one(3);
    store_one(4); store_one(5); store_one(6); store_one(7);
    #undef load_one
    #undef store_one

    num_blocks--;
    block++;
  }
}

void texture_blocks_16bpp(psx_gpu_struct *psx_gpu)
{
#if 0
  texture_blocks_16bpp_(psx_gpu);
  return;
#endif
  u32 num_blocks = psx_gpu->num_blocks;
  const u16 * __restrict__ texture_ptr_16bpp = psx_gpu->texture_page_ptr;
  block_struct * __restrict__ block = psx_gpu->blocks;

  while(num_blocks)
  {
    u32 offset;
    #define load_one(i_) \
      offset = block->uv.e[i_]; \
      offset += ((offset & 0xFF00) * 3); \
      u16 texel##i_ = texture_ptr_16bpp[offset]
    #define store_one(i_) \
      block->texels.e[i_] = texel##i_
    load_one(0); load_one(1); load_one(2); load_one(3);
    load_one(4); load_one(5); load_one(6); load_one(7);
    store_one(0); store_one(1); store_one(2); store_one(3);
    store_one(4); store_one(5); store_one(6); store_one(7);
    #undef load_one
    #undef store_one

    num_blocks--;
    block++;
  }
}

#define shade_blocks_load_msb_mask_indirect()                                  \

#define shade_blocks_load_msb_mask_direct()                                    \
  vec_8x16u msb_mask;                                                          \
  gvdupq_n_u16(msb_mask, psx_gpu->mask_msb);                                   \

#define shade_blocks_store_indirect(_draw_mask, _pixels)                       \
  gvst1q_u16(_draw_mask, block->draw_mask.e);                                  \
  gvst1q_u16(_pixels, block->pixels.e);                                        \

#define shade_blocks_store_direct(_draw_mask, _pixels)                         \
{                                                                              \
  u16 * __restrict__ fb_ptr = block->fb_ptr;                                   \
  vec_8x16u fb_pixels;                                                         \
  gvld1q_u16(fb_pixels, fb_ptr);                                               \
  gvorrq(_pixels, _pixels, msb_mask);                                          \
  gvbifq(fb_pixels, _pixels, _draw_mask);                                      \
  gvst1q_u16(fb_pixels, fb_ptr);                                               \
}                                                                              \

#define shade_blocks_textured_false_modulated_check_dithered(target)           \

#define shade_blocks_textured_false_modulated_check_undithered(target)         \
  if(psx_gpu->triangle_color == 0x808080)                                      \
  {                                                                            \
    shade_blocks_textured_unmodulated_##target(psx_gpu);                       \
    return;                                                                    \
  }                                                                            \

#define shade_blocks_textured_modulated_shaded_primitive_load(dithering,       \
 target)                                                                       \

#define shade_blocks_textured_modulated_unshaded_primitive_load(dithering,     \
 target)                                                                       \
{                                                                              \
  u32 color = psx_gpu->triangle_color;                                         \
  gvdup_n_u8(colors_r, color);                                                 \
  gvdup_n_u8(colors_g, color >> 8);                                            \
  gvdup_n_u8(colors_b, color >> 16);                                           \
  shade_blocks_textured_false_modulated_check_##dithering(target);             \
}                                                                              \

#define shade_blocks_textured_modulated_shaded_block_load()                    \
  gvld1_u8(colors_r, block->r.e);                                              \
  gvld1_u8(colors_g, block->g.e);                                              \
  gvld1_u8(colors_b, block->b.e)                                               \

#define shade_blocks_textured_modulated_unshaded_block_load()                  \

#define shade_blocks_textured_modulate_dithered(component)                     \
  gvld1q_u16(pixels_##component, block->dither_offsets.e);                     \
  gvmlal_u8(pixels_##component, texels_##component, colors_##component)        \

#define shade_blocks_textured_modulate_undithered(component)                   \
  gvmull_u8(pixels_##component, texels_##component, colors_##component)        \

#define shade_blocks_textured_modulated_do(shading, dithering, target)         \
  block_struct * __restrict__ block = psx_gpu->blocks;                         \
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
  vec_8x16u test_mask;                                                         \
  u32 draw_mask_bits;                                                          \
                                                                               \
  gvld1q_u16(test_mask, psx_gpu->test_mask.e);                                 \
  shade_blocks_load_msb_mask_##target();                                       \
                                                                               \
  gvdup_n_u8(d64_0x07, 0x07);                                                  \
  gvdup_n_u8(d64_0x1F, 0x1F);                                                  \
  gvdup_n_u8(d64_1, 1);                                                        \
  gvdup_n_u8(d64_4, 4);                                                        \
  gvdup_n_u8(d64_128, 128u);                                                   \
                                                                               \
  gvdupq_n_u16(d128_0x8000, 0x8000);                                           \
                                                                               \
  shade_blocks_textured_modulated_##shading##_primitive_load(dithering,        \
   target);                                                                    \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    draw_mask_bits = block->draw_mask_bits;                                    \
    gvdupq_n_u16(draw_mask, draw_mask_bits);                                   \
    gvtstq_u16(draw_mask, draw_mask, test_mask);                               \
                                                                               \
    shade_blocks_textured_modulated_##shading##_block_load();                  \
                                                                               \
    gvld1q_u16(texels, block->texels.e);                                       \
                                                                               \
    gvmovn_u16(texels_r, texels);                                              \
    gvshrn_n_u16(texels_g, texels, 5);                                         \
    gvshrn_n_u16(texels_b, texels, 7);                                         \
                                                                               \
    gvand(texels_r, texels_r, d64_0x1F);                                       \
    gvand(texels_g, texels_g, d64_0x1F);                                       \
    gvshr_n_u8(texels_b, texels_b, 3);                                         \
                                                                               \
    shade_blocks_textured_modulate_##dithering(r);                             \
    shade_blocks_textured_modulate_##dithering(g);                             \
    shade_blocks_textured_modulate_##dithering(b);                             \
                                                                               \
    gvceqzq_u16(zero_mask, texels);                                            \
    gvand(pixels, texels, d128_0x8000);                                        \
                                                                               \
    gvqshrun_n_s16(pixels_r_low, pixels_r, 4);                                 \
    gvqshrun_n_s16(pixels_g_low, pixels_g, 4);                                 \
    gvqshrun_n_s16(pixels_b_low, pixels_b, 4);                                 \
                                                                               \
    gvorrq(zero_mask, draw_mask, zero_mask);                                   \
                                                                               \
    gvshr_n_u8(pixels_r_low, pixels_r_low, 3);                                 \
    gvbic(pixels_g_low, pixels_g_low, d64_0x07);                               \
    gvbic(pixels_b_low, pixels_b_low, d64_0x07);                               \
                                                                               \
    gvmlal_u8(pixels, pixels_r_low, d64_1);                                    \
    gvmlal_u8(pixels, pixels_g_low, d64_4);                                    \
    gvmlal_u8(pixels, pixels_b_low, d64_128);                                  \
                                                                               \
    shade_blocks_store_##target(zero_mask, pixels);                            \
                                                                               \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \

void shade_blocks_shaded_textured_modulated_dithered_direct(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_shaded_textured_modulated_dithered_direct_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(shaded, dithered, direct);
}

void shade_blocks_shaded_textured_modulated_undithered_direct(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_shaded_textured_modulated_undithered_direct_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(shaded, undithered, direct);
}

void shade_blocks_unshaded_textured_modulated_dithered_direct(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_unshaded_textured_modulated_dithered_direct_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(unshaded, dithered, direct);
}

void shade_blocks_unshaded_textured_modulated_undithered_direct(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_unshaded_textured_modulated_undithered_direct_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(unshaded, undithered, direct);
}

void shade_blocks_shaded_textured_modulated_dithered_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_shaded_textured_modulated_dithered_indirect_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(shaded, dithered, indirect);
}

void shade_blocks_shaded_textured_modulated_undithered_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_shaded_textured_modulated_undithered_indirect_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(shaded, undithered, indirect);
}

void shade_blocks_unshaded_textured_modulated_dithered_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_unshaded_textured_modulated_dithered_indirect_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(unshaded, dithered, indirect);
}

void shade_blocks_unshaded_textured_modulated_undithered_indirect(psx_gpu_struct
 *psx_gpu)
{
#if 0
  shade_blocks_unshaded_textured_modulated_undithered_indirect_(psx_gpu);
  return;
#endif
  shade_blocks_textured_modulated_do(unshaded, undithered, indirect);
}

#define shade_blocks_textured_unmodulated_do(target)                           \
  block_struct *block = psx_gpu->blocks;                                       \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u test_mask;                                                         \
  u32 draw_mask_bits;                                                          \
                                                                               \
  vec_8x16u pixels;                                                            \
                                                                               \
  gvld1q_u16(test_mask, psx_gpu->test_mask.e);                                 \
  shade_blocks_load_msb_mask_##target();                                       \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    vec_8x16u zero_mask;                                                       \
                                                                               \
    draw_mask_bits = block->draw_mask_bits;                                    \
    gvdupq_n_u16(draw_mask, draw_mask_bits);                                   \
    gvtstq_u16(draw_mask, draw_mask, test_mask);                               \
                                                                               \
    gvld1q_u16(pixels, block->texels.e);                                       \
                                                                               \
    gvceqzq_u16(zero_mask, pixels);                                            \
    gvorrq(zero_mask, draw_mask, zero_mask);                                   \
                                                                               \
    shade_blocks_store_##target(zero_mask, pixels);                            \
                                                                               \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \

void shade_blocks_textured_unmodulated_indirect(psx_gpu_struct *psx_gpu)
{
#if 0
  shade_blocks_textured_unmodulated_indirect_(psx_gpu);
  return;
#endif
  shade_blocks_textured_unmodulated_do(indirect)
}

void shade_blocks_textured_unmodulated_direct(psx_gpu_struct *psx_gpu)
{
#if 0
  shade_blocks_textured_unmodulated_direct_(psx_gpu);
  return;
#endif
  shade_blocks_textured_unmodulated_do(direct)
}

void shade_blocks_unshaded_untextured_indirect(psx_gpu_struct *psx_gpu)
{
}

void shade_blocks_unshaded_untextured_direct(psx_gpu_struct *psx_gpu)
{
#if 0
  shade_blocks_unshaded_untextured_direct_(psx_gpu);
  return;
#endif
  block_struct *block = psx_gpu->blocks;
  u32 num_blocks = psx_gpu->num_blocks;

  vec_8x16u pixels;
  gvld1q_u16(pixels, block->pixels.e);
  shade_blocks_load_msb_mask_direct();

  while(num_blocks)
  {
    vec_8x16u draw_mask;
    gvld1q_u16(draw_mask, block->draw_mask.e);
    shade_blocks_store_direct(draw_mask, pixels);

    num_blocks--;
    block++;
  }
}

#define blend_blocks_mask_evaluate_on()                                        \
  vec_8x16u mask_pixels;                                                       \
  gvcltzq_s16(mask_pixels, framebuffer_pixels);                                \
  gvorrq(draw_mask, draw_mask, mask_pixels)                                    \

#define blend_blocks_mask_evaluate_off()                                       \

#define blend_blocks_average()                                                 \
{                                                                              \
  vec_8x16u pixels_no_msb;                                                     \
  vec_8x16u fb_pixels_no_msb;                                                  \
                                                                               \
  vec_8x16u d128_0x0421;                                                       \
                                                                               \
  gvdupq_n_u16(d128_0x0421, 0x0421);                                           \
                                                                               \
  gveorq(blend_pixels, pixels, framebuffer_pixels);                            \
  gvbicq(pixels_no_msb, pixels, d128_0x8000);                                  \
  gvand(blend_pixels, blend_pixels, d128_0x0421);                              \
  gvsubq_u16(blend_pixels, pixels_no_msb, blend_pixels);                       \
  gvbicq(fb_pixels_no_msb, framebuffer_pixels, d128_0x8000);                   \
  gvhaddq_u16(blend_pixels, fb_pixels_no_msb, blend_pixels);                   \
}                                                                              \

#define blend_blocks_add()                                                     \
{                                                                              \
  vec_8x16u pixels_rb, pixels_g;                                               \
  vec_8x16u fb_rb, fb_g;                                                       \
                                                                               \
  vec_8x16u d128_0x7C1F;                                                       \
  vec_8x16u d128_0x03E0;                                                       \
                                                                               \
  gvdupq_n_u16(d128_0x7C1F, 0x7C1F);                                           \
  gvdupq_n_u16(d128_0x03E0, 0x03E0);                                           \
                                                                               \
  gvand(pixels_rb, pixels, d128_0x7C1F);                                       \
  gvand(pixels_g, pixels, d128_0x03E0);                                        \
                                                                               \
  gvand(fb_rb, framebuffer_pixels, d128_0x7C1F);                               \
  gvand(fb_g, framebuffer_pixels, d128_0x03E0);                                \
                                                                               \
  gvaddq_u16(fb_rb, fb_rb, pixels_rb);                                         \
  gvaddq_u16(fb_g, fb_g, pixels_g);                                            \
                                                                               \
  gvminq_u8(fb_rb, fb_rb, d128_0x7C1F);                                        \
  gvminq_u16(fb_g, fb_g, d128_0x03E0);                                         \
                                                                               \
  gvorrq(blend_pixels, fb_rb, fb_g);                                           \
}                                                                              \

#define blend_blocks_subtract()                                                \
{                                                                              \
  vec_8x16u pixels_rb, pixels_g;                                               \
  vec_8x16u fb_rb, fb_g;                                                       \
                                                                               \
  vec_8x16u d128_0x7C1F;                                                       \
  vec_8x16u d128_0x03E0;                                                       \
                                                                               \
  gvdupq_n_u16(d128_0x7C1F, 0x7C1F);                                           \
  gvdupq_n_u16(d128_0x03E0, 0x03E0);                                           \
                                                                               \
  gvand(pixels_rb, pixels, d128_0x7C1F);                                       \
  gvand(pixels_g, pixels, d128_0x03E0);                                        \
                                                                               \
  gvand(fb_rb, framebuffer_pixels, d128_0x7C1F);                               \
  gvand(fb_g, framebuffer_pixels, d128_0x03E0);                                \
                                                                               \
  gvqsubq_u8(fb_rb, fb_rb, pixels_rb);                                         \
  gvqsubq_u16(fb_g, fb_g, pixels_g);                                           \
                                                                               \
  gvorrq(blend_pixels, fb_rb, fb_g);                                           \
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
  gvdupq_n_u16(d128_0x7C1F, 0x7C1F);                                           \
  gvdupq_n_u16(d128_0x1C07, 0x1C07);                                           \
  gvdupq_n_u16(d128_0x03E0, 0x03E0);                                           \
  gvdupq_n_u16(d128_0x00E0, 0x00E0);                                           \
                                                                               \
  gvshrq_n_u16(pixels_fourth, pixels, 2);                                      \
                                                                               \
  gvand(fb_rb, framebuffer_pixels, d128_0x7C1F);                               \
  gvand(fb_g, framebuffer_pixels, d128_0x03E0);                                \
                                                                               \
  gvand(pixels_rb, pixels_fourth, d128_0x1C07);                                \
  gvand(pixels_g, pixels_fourth, d128_0x00E0);                                 \
                                                                               \
  gvaddq_u16(fb_rb, fb_rb, pixels_rb);                                         \
  gvaddq_u16(fb_g, fb_g, pixels_g);                                            \
                                                                               \
  gvminq_u8(fb_rb, fb_rb, d128_0x7C1F);                                        \
  gvminq_u16(fb_g, fb_g, d128_0x03E0);                                         \
                                                                               \
  gvorrq(blend_pixels, fb_rb, fb_g);                                           \
}                                                                              \

#define blend_blocks_blended_combine_textured()                                \
{                                                                              \
  vec_8x16u blend_mask;                                                        \
  gvcltzq_s16(blend_mask, pixels);                                             \
                                                                               \
  gvorrq(blend_pixels, blend_pixels, d128_0x8000);                             \
  gvbifq(blend_pixels, pixels, blend_mask);                                    \
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

#define blend_blocks_do(texturing, blend_mode, mask_evaluate)                  \
  block_struct *block = psx_gpu->blocks;                                       \
  u32 num_blocks = psx_gpu->num_blocks;                                        \
  vec_8x16u draw_mask;                                                         \
  vec_8x16u pixels;                                                            \
  vec_8x16u blend_pixels;                                                      \
  vec_8x16u framebuffer_pixels;                                                \
  vec_8x16u msb_mask;                                                          \
  vec_8x16u d128_0x8000;                                                       \
                                                                               \
  u16 *fb_ptr;                                                                 \
                                                                               \
  gvdupq_n_u16(d128_0x8000, 0x8000);                                           \
  gvdupq_n_u16(msb_mask, psx_gpu->mask_msb);                                   \
  (void)d128_0x8000; /* sometimes unused */                                    \
                                                                               \
  while(num_blocks)                                                            \
  {                                                                            \
    gvld1q_u16(pixels, block->pixels.e);                                       \
    gvld1q_u16(draw_mask, block->draw_mask.e);                                 \
    fb_ptr = block->fb_ptr;                                                    \
                                                                               \
    gvld1q_u16(framebuffer_pixels, fb_ptr);                                    \
                                                                               \
    blend_blocks_mask_evaluate_##mask_evaluate();                              \
    blend_blocks_body_##blend_mode(texturing);                                 \
                                                                               \
    gvorrq(blend_pixels, blend_pixels, msb_mask);                              \
    gvbifq(framebuffer_pixels, blend_pixels, draw_mask);                       \
    gvst1q_u16(framebuffer_pixels, fb_ptr);                                    \
                                                                               \
    num_blocks--;                                                              \
    block++;                                                                   \
  }                                                                            \


void blend_blocks_textured_average_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_average_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, average, off);
}

void blend_blocks_untextured_average_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_average_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, average, off);
}

void blend_blocks_textured_average_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_average_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, average, on);
}

void blend_blocks_untextured_average_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_average_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, average, on);
}

void blend_blocks_textured_add_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_add_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, add, off);
}

void blend_blocks_textured_add_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_add_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, add, on);
}

void blend_blocks_untextured_add_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_add_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, add, off);
}

void blend_blocks_untextured_add_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_add_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, add, on);
}

void blend_blocks_textured_subtract_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_subtract_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, subtract, off);
}

void blend_blocks_textured_subtract_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_subtract_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, subtract, on);
}

void blend_blocks_untextured_subtract_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_subtract_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, subtract, off);
}

void blend_blocks_untextured_subtract_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_subtract_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, subtract, on);
}

void blend_blocks_textured_add_fourth_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_add_fourth_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, add_fourth, off);
}

void blend_blocks_textured_add_fourth_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_add_fourth_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, add_fourth, on);
}

void blend_blocks_untextured_add_fourth_off(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_add_fourth_off_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, add_fourth, off);
}

void blend_blocks_untextured_add_fourth_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_untextured_add_fourth_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(untextured, add_fourth, on);
}

void blend_blocks_textured_unblended_on(psx_gpu_struct *psx_gpu)
{
#if 0
  blend_blocks_textured_unblended_on_(psx_gpu);
  return;
#endif
  blend_blocks_do(textured, unblended, on);
}

void blend_blocks_textured_unblended_off(psx_gpu_struct *psx_gpu)
{
}

void setup_sprite_untextured_512(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u,
 s32 v, s32 width, s32 height, u32 color)
{
#if 0
  setup_sprite_untextured_512_(psx_gpu, x, y, u, v, width, height, color);
  return;
#endif
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
  vec_8x16u test_mask;
  vec_8x16u zero_mask;

  gvld1q_u16(test_mask, psx_gpu->test_mask.e);
  color = (color_r >> 3) | ((color_g >> 3) << 5) | ((color_b >> 3) << 10);

  gvdupq_n_u16(colors, color);
  gvdupq_n_u16(zero_mask, 0x00);
  gvdupq_n_u16(right_mask, right_mask_bits);
  gvtstq_u16(right_mask, right_mask, test_mask);

  while(height)
  {
    blocks_remaining = block_width - 1;
    num_blocks += block_width;

    if(num_blocks > MAX_BLOCKS)
    {
      flush_render_block_buffer(psx_gpu);
      num_blocks = block_width;
      block = psx_gpu->blocks;
    }

    while(blocks_remaining)
    {
      gvst1q_u16(colors, block->pixels.e);
      gvst1q_u16(zero_mask, block->draw_mask.e);
      block->fb_ptr = fb_ptr;

      fb_ptr += 8;
      block++;
      blocks_remaining--;
    }

    gvst1q_u16(colors, block->pixels.e);
    gvst1q_u16(right_mask, block->draw_mask.e);
    block->fb_ptr = fb_ptr;

    block++;
    fb_ptr += fb_ptr_pitch;

    height--;
    psx_gpu->num_blocks = num_blocks;
  }
}

#define setup_sprite_tiled_initialize_4bpp_clut()                              \
  vec_16x8u clut_low, clut_high;                                               \
                                                                               \
  gvld2q_u8(clut_low, clut_high, (u8 *)psx_gpu->clut_ptr)                      \

#define setup_sprite_tiled_initialize_4bpp()                                   \
  setup_sprite_tiled_initialize_4bpp_clut();                                   \
                                                                               \
  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_4bpp_mask)        \
    update_texture_4bpp_cache(psx_gpu)                                         \

#define setup_sprite_tiled_initialize_8bpp()                                   \
  if(psx_gpu->current_texture_mask & psx_gpu->dirty_textures_8bpp_mask)        \
    update_texture_8bpp_cache(psx_gpu)                                         \

#define setup_sprite_tile_fetch_texel_block_8bpp(offset)                       \
  texture_block_ptr = psx_gpu->texture_page_ptr +                              \
   ((texture_offset + offset) & texture_mask);                                 \
                                                                               \
  gvld1_u8(texels, (u8 *)texture_block_ptr)                                    \

#define setup_sprite_tile_add_blocks(tile_num_blocks)                          \
  num_blocks += tile_num_blocks;                                               \
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
  setup_sprite_tile_add_blocks(sub_tile_height * 2);                           \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    gvtbl2_u8(texels_low, clut_low, texels);                                   \
    gvtbl2_u8(texels_high, clut_high, texels);                                 \
                                                                               \
    gvst2_u8(texels_low, texels_high, (u8 *)block->texels.e);                  \
    block->draw_mask_bits = left_mask_bits;                                    \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    gvtbl2_u8(texels_low, clut_low, texels);                                   \
    gvtbl2_u8(texels_high, clut_high, texels);                                 \
                                                                               \
    gvst2_u8(texels_low, texels_high, (u8 *)block->texels.e);                  \
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
  setup_sprite_tile_add_blocks(sub_tile_height);                               \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    gvtbl2_u8(texels_low, clut_low, texels);                                   \
    gvtbl2_u8(texels_high, clut_high, texels);                                 \
                                                                               \
    gvst2_u8(texels_low, texels_high, (u8 *)block->texels.e);                  \
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
    gvst1_u8(texels, block->r.e);                                              \
    block->draw_mask_bits = left_mask_bits;                                    \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    gvst1_u8(texels, block->r.e);                                              \
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
    gvst1_u8(texels, block->r.e);                                              \
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
  vec_8x16u pixels;                                                            \
  vec_4x16u pixels_half;                                                       \
  setup_sprite_tile_add_blocks(sub_tile_height * 2 * 4);                       \
  u32 left_mask_bits_a = left_mask_bits & 0xFF;                                \
  u32 left_mask_bits_b = left_mask_bits >> 8;                                  \
  u32 right_mask_bits_a = right_mask_bits & 0xFF;                              \
  u32 right_mask_bits_b = right_mask_bits >> 8;                                \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    gvtbl2_u8(texels_low, clut_low, texels);                                   \
    gvtbl2_u8(texels_high, clut_high, texels);                                 \
    gvzip_u8(pixels, texels_low, texels_high);                                 \
                                                                               \
    gvget_lo(pixels_half, pixels);                                             \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    gvget_hi(pixels_half, pixels);                                             \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 1024 + 8;                                         \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    gvtbl2_u8(texels_low, clut_low, texels);                                   \
    gvtbl2_u8(texels_high, clut_high, texels);                                 \
    gvzip_u8(pixels, texels_low, texels_high);                                 \
                                                                               \
    gvget_lo(pixels_half, pixels);                                             \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 16;                                               \
    block++;                                                                   \
                                                                               \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 1024 + 16;                                        \
    block++;                                                                   \
                                                                               \
    gvget_hi(pixels_half, pixels);                                             \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = right_mask_bits_b;                                 \
    block->fb_ptr = fb_ptr + 24;                                               \
    block++;                                                                   \
                                                                               \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
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
  vec_8x16u pixels;                                                            \
  vec_4x16u pixels_half;                                                       \
  setup_sprite_tile_add_blocks(sub_tile_height * 4);                           \
  u32 edge##_mask_bits_a = edge##_mask_bits & 0xFF;                            \
  u32 edge##_mask_bits_b = edge##_mask_bits >> 8;                              \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    gvtbl2_u8(texels_low, clut_low, texels);                                   \
    gvtbl2_u8(texels_high, clut_high, texels);                                 \
    gvzip_u8(pixels, texels_low, texels_high);                                 \
                                                                               \
    gvget_lo(pixels_half, pixels);                                             \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    gvget_hi(pixels_half, pixels);                                             \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
    block->draw_mask_bits = edge##_mask_bits_b;                                \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    gvst2_u16(pixels_half, pixels_half, block->texels.e);                      \
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
  vec_8x16u texels_wide;                                                       \
  vec_4x16u texels_half;                                                       \
  u32 left_mask_bits_a = left_mask_bits & 0xFF;                                \
  u32 left_mask_bits_b = left_mask_bits >> 8;                                  \
  u32 right_mask_bits_a = right_mask_bits & 0xFF;                              \
  u32 right_mask_bits_b = right_mask_bits >> 8;                                \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    gvzip_u8(texels_wide, texels, texels);                                     \
    gvget_lo(texels_half, texels_wide);                                        \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = left_mask_bits_a;                                  \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    gvget_hi(texels_half, texels_wide);                                        \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = left_mask_bits_b;                                  \
    block->fb_ptr = fb_ptr + 1024 + 8;                                         \
    block++;                                                                   \
                                                                               \
    setup_sprite_tile_fetch_texel_block_8bpp(8);                               \
    gvzip_u8(texels_wide, texels, texels);                                     \
    gvget_lo(texels_half, texels_wide);                                        \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 16;                                               \
    block++;                                                                   \
                                                                               \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = right_mask_bits_a;                                 \
    block->fb_ptr = fb_ptr + 1024 + 16;                                        \
    block++;                                                                   \
                                                                               \
    gvget_hi(texels_half, texels_wide);                                        \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = right_mask_bits_b;                                 \
    block->fb_ptr = fb_ptr + 24;                                               \
    block++;                                                                   \
                                                                               \
    gvst1_u8(texels_half, block->r.e);                                         \
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
  vec_8x16u texels_wide;                                                       \
  vec_4x16u texels_half;                                                       \
  u32 edge##_mask_bits_a = edge##_mask_bits & 0xFF;                            \
  u32 edge##_mask_bits_b = edge##_mask_bits >> 8;                              \
                                                                               \
  while(sub_tile_height)                                                       \
  {                                                                            \
    setup_sprite_tile_fetch_texel_block_8bpp(0);                               \
    gvzip_u8(texels_wide, texels, texels);                                     \
    gvget_lo(texels_half, texels_wide);                                        \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr;                                                    \
    block++;                                                                   \
                                                                               \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = edge##_mask_bits_a;                                \
    block->fb_ptr = fb_ptr + 1024;                                             \
    block++;                                                                   \
                                                                               \
    gvget_hi(texels_half, texels_wide);                                        \
    gvst1_u8(texels_half, block->r.e);                                         \
    block->draw_mask_bits = edge##_mask_bits_b;                                \
    block->fb_ptr = fb_ptr + 8;                                                \
    block++;                                                                   \
                                                                               \
    gvst1_u8(texels_half, block->r.e);                                         \
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


#define setup_sprite_tiled_do(texture_mode, x4mode)                            \
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
  u16 *texture_block_ptr;                                                      \
  vec_8x8u texels;                                                             \
                                                                               \
  setup_sprite_tiled_initialize_##texture_mode##x4mode();                      \
                                                                               \
  control_mask = tile_width == 1;                                              \
  control_mask |= (tile_height == 1) << 1;                                     \
  control_mask |= setup_sprite_comapre_left_block_mask##x4mode() << 2;         \
  control_mask |= setup_sprite_comapre_right_block_mask##x4mode() << 3;        \
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

void setup_sprite_4bpp(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u, s32 v,
 s32 width, s32 height, u32 color)
{
#if 0
  setup_sprite_4bpp_(psx_gpu, x, y, u, v, width, height, color);
  return;
#endif
  setup_sprite_tiled_do(4bpp,)
}

void setup_sprite_8bpp(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u, s32 v,
 s32 width, s32 height, u32 color)
{
#if 0
  setup_sprite_8bpp_(psx_gpu, x, y, u, v, width, height, color);
  return;
#endif
  setup_sprite_tiled_do(8bpp,)
}

#undef draw_mask_fb_ptr_left
#undef draw_mask_fb_ptr_right

void setup_sprite_4bpp_4x(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u, s32 v,
 s32 width, s32 height, u32 color)
{
#if 0
  setup_sprite_4bpp_4x_(psx_gpu, x, y, u, v, width, height, color);
  return;
#endif
  setup_sprite_tiled_do(4bpp, _4x)
}

void setup_sprite_8bpp_4x(psx_gpu_struct *psx_gpu, s32 x, s32 y, s32 u, s32 v,
 s32 width, s32 height, u32 color)
{
#if 0
  setup_sprite_8bpp_4x_(psx_gpu, x, y, u, v, width, height, color);
  return;
#endif
  setup_sprite_tiled_do(8bpp, _4x)
}


void scale2x_tiles8(void * __restrict__ dst_, const void * __restrict__ src_, int w8, int h)
{
#if 0
  scale2x_tiles8_(dst_, src_, w8, h);
  return;
#endif
  const u16 * __restrict__ src = src_;
  const u16 * __restrict__ src1;
  u16 * __restrict__ dst = dst_;
  u16 * __restrict__ dst1;
  gvreg a, b;
  int w;
  for (; h > 0; h--, src += 1024, dst += 1024*2)
  {
    src1 = src;
    dst1 = dst;
    for (w = w8; w > 0; w--, src1 += 8, dst1 += 8*2)
    {
      gvld1q_u16(a, src1);
      gvzipq_u16(a, b, a, a);
      gvst1q_u16(a, dst1);
      gvst1q_u16(b, dst1 + 8);
      gvst1q_u16(a, dst1 + 1024);
      gvst1q_u16(b, dst1 + 1024 + 8);
    }
  }
}

// vim:ts=2:sw=2:expandtab
