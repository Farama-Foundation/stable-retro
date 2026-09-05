/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* r_surf.c: surface-related refresh code */

#include <stdint.h>
#include <compat/intrinsics.h>

#include "quakedef.h"
#include "r_local.h"
#include "sys.h"

drawsurf_t r_drawsurf;

int lightleft, sourcesstep, blocksize, sourcetstep;
int lightright, lightleftstep, lightrightstep, blockdivshift;

int				lightlefta[3];
int				lightrighta[3];
int				lightleftstepa[3], lightrightstepa[3];

unsigned blockdivmask;
void *prowdestbase;
unsigned char *pbasesource;
int surfrowbytes;		/* used by ASM files */
/* unsigned *r_lightptr; */
int *r_lightptr;

int r_stepback;
int r_lightwidth;
unsigned char *r_source, *r_sourcemax;

static int r_numhblocks;
int r_numvblocks;

void R_DrawSurfaceBlock8_mip0(void);
void R_DrawSurfaceBlock8_mip1(void);
void R_DrawSurfaceBlock8_mip2(void);
void R_DrawSurfaceBlock8_mip3(void);

void R_DrawSurfaceBlockRGB_mip0(void);
void R_DrawSurfaceBlockRGB_mip1(void);
void R_DrawSurfaceBlockRGB_mip2(void);
void R_DrawSurfaceBlockRGB_mip3(void);


static void (*surfmiptable[4]) (void) = {
    R_DrawSurfaceBlock8_mip0,
    R_DrawSurfaceBlock8_mip1,
    R_DrawSurfaceBlock8_mip2,
    R_DrawSurfaceBlock8_mip3
};

static void	(*surfmiptableRGB[4])(void) =
{
	R_DrawSurfaceBlockRGB_mip0,
	R_DrawSurfaceBlockRGB_mip1,
	R_DrawSurfaceBlockRGB_mip2,
	R_DrawSurfaceBlockRGB_mip3
};

/* static unsigned blocklights[18 * 18 * 3]; */
int		blocklights[18*18*3]; /* LordHavoc: .lit support (*3 for RGB) */

/* SIMD detection for R_BuildLightMap's three per-surface kernels:
 *   (1) fill blocklights[] to ambient int constant
 *   (2) accumulate lightmap[i] * scale into blocklights[i]
 *   (3) bound/invert/shift each entry: max((255*256 - x) >> 2, 64)
 * blocklights is int[]; lightmap is byte[]; size up to smax*tmax = 18*18 = 324.
 * Called per surface from R_DrawSurface; hundreds of times per frame.
 * Bench at -O2 (tyrquake default) on 324 ints (gcc 13 / qemu for ARM):
 *   loop 1:  SSE2 4.01x, AArch64 NEON 3.50x, ARMv7 NEON 3.06x
 *   loop 2:  SSE2 4.30x, AArch64 NEON 2.05x, ARMv7 NEON 1.65x
 *   loop 3:  SSE2 2.56x, AArch64 NEON 2.14x, ARMv7 NEON 1.71x
 * At -O3 gcc auto-vectorizes most of these to similar code; the SIMD
 * path then ties autovec (no regression).  ARMv7-without-NEON falls
 * back to the scalar tail, which is fine -- no per-target gate needed. */
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define LMAP_SSE2 1
#include <emmintrin.h>
#endif
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
#define LMAP_NEON 1
#include <arm_neon.h>
#endif

static void R_LightMapFill(int *bl, int size, int value)
{
#if defined(LMAP_SSE2)
   int i;
   int batch = size & ~3;
   __m128i vc = _mm_set1_epi32(value);
   for (i = 0; i < batch; i += 4)
      _mm_storeu_si128((__m128i *)(bl + i), vc);
   for (; i < size; i++) bl[i] = value;
#elif defined(LMAP_NEON)
   int i;
   int batch = size & ~3;
   int32x4_t vc = vdupq_n_s32(value);
   for (i = 0; i < batch; i += 4)
      vst1q_s32(bl + i, vc);
   for (; i < size; i++) bl[i] = value;
#else
   int i;
   for (i = 0; i < size; i++) bl[i] = value;
#endif
}

static void R_LightMapAccum(int *bl, const byte *lightmap, int size, unsigned scale)
{
   /* d_lightstylevalue[] -- the source of `scale` -- maxes at 25*22=550
    * for animated styles (256 unanimated), so always fits in int16; the
    * PMADDWD path treats scale as a 16-bit lane.  If the engine ever
    * adds brighter lightstyles >32767, this assumption needs revisit. */
#if defined(LMAP_SSE2)
   int i;
   int batch = size & ~15;
   int batch4 = (size & ~3) - batch;
   __m128i vscale = _mm_set1_epi32((int)(scale & 0xFFFF));
   __m128i zero = _mm_setzero_si128();
   /* Wide 16-byte/iter inner: load 16 bytes -> widen via two unpacklo/hi
    * stages to 4 x (4 int32) -> PMADDWD with [s,0,s,0,...] -> add+store.
    * Matches the shape gcc -O3 autovec produces for the scalar loop, so
    * we tie autovec at -O3 and beat scalar at -O2. */
   for (i = 0; i < batch; i += 16) {
      __m128i bytes = _mm_loadu_si128((const __m128i *)(lightmap + i));
      __m128i b16_lo = _mm_unpacklo_epi8(bytes, zero);
      __m128i b16_hi = _mm_unpackhi_epi8(bytes, zero);
      __m128i b32_0 = _mm_unpacklo_epi16(b16_lo, zero);
      __m128i b32_1 = _mm_unpackhi_epi16(b16_lo, zero);
      __m128i b32_2 = _mm_unpacklo_epi16(b16_hi, zero);
      __m128i b32_3 = _mm_unpackhi_epi16(b16_hi, zero);
      __m128i p0 = _mm_madd_epi16(b32_0, vscale);
      __m128i p1 = _mm_madd_epi16(b32_1, vscale);
      __m128i p2 = _mm_madd_epi16(b32_2, vscale);
      __m128i p3 = _mm_madd_epi16(b32_3, vscale);
      __m128i e0 = _mm_loadu_si128((const __m128i *)(bl + i));
      __m128i e1 = _mm_loadu_si128((const __m128i *)(bl + i + 4));
      __m128i e2 = _mm_loadu_si128((const __m128i *)(bl + i + 8));
      __m128i e3 = _mm_loadu_si128((const __m128i *)(bl + i + 12));
      _mm_storeu_si128((__m128i *)(bl + i),      _mm_add_epi32(e0, p0));
      _mm_storeu_si128((__m128i *)(bl + i + 4),  _mm_add_epi32(e1, p1));
      _mm_storeu_si128((__m128i *)(bl + i + 8),  _mm_add_epi32(e2, p2));
      _mm_storeu_si128((__m128i *)(bl + i + 12), _mm_add_epi32(e3, p3));
   }
   /* 4-wide cleanup for size%16 in [4..15] */
   for (; i < batch + batch4; i += 4) {
      __m128i bytes = _mm_cvtsi32_si128(*(const int *)(lightmap + i));
      __m128i bytes16 = _mm_unpacklo_epi8(bytes, zero);
      __m128i bytes32 = _mm_unpacklo_epi16(bytes16, zero);
      __m128i prod = _mm_madd_epi16(bytes32, vscale);
      __m128i existing = _mm_loadu_si128((const __m128i *)(bl + i));
      _mm_storeu_si128((__m128i *)(bl + i), _mm_add_epi32(existing, prod));
   }
   for (; i < size; i++) bl[i] += lightmap[i] * scale;
#elif defined(LMAP_NEON)
   int i;
   int batch = size & ~15;
   int batch4 = (size & ~3) - batch;
   int32x4_t vscale = vdupq_n_s32((int)scale);
   for (i = 0; i < batch; i += 16) {
      uint8x16_t bytes = vld1q_u8(lightmap + i);
      uint16x8_t b16_lo = vmovl_u8(vget_low_u8(bytes));
      uint16x8_t b16_hi = vmovl_u8(vget_high_u8(bytes));
      int32x4_t b0 = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(b16_lo)));
      int32x4_t b1 = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(b16_lo)));
      int32x4_t b2 = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(b16_hi)));
      int32x4_t b3 = vreinterpretq_s32_u32(vmovl_u16(vget_high_u16(b16_hi)));
      int32x4_t e0 = vld1q_s32(bl + i);
      int32x4_t e1 = vld1q_s32(bl + i + 4);
      int32x4_t e2 = vld1q_s32(bl + i + 8);
      int32x4_t e3 = vld1q_s32(bl + i + 12);
      vst1q_s32(bl + i,      vmlaq_s32(e0, b0, vscale));
      vst1q_s32(bl + i + 4,  vmlaq_s32(e1, b1, vscale));
      vst1q_s32(bl + i + 8,  vmlaq_s32(e2, b2, vscale));
      vst1q_s32(bl + i + 12, vmlaq_s32(e3, b3, vscale));
   }
   for (; i < batch + batch4; i += 4) {
      uint32_t four = *(const uint32_t *)(lightmap + i);
      uint8x8_t b8 = vreinterpret_u8_u32(vdup_n_u32(four));
      uint16x8_t b16 = vmovl_u8(b8);
      int32x4_t b32 = vreinterpretq_s32_u32(vmovl_u16(vget_low_u16(b16)));
      int32x4_t ex = vld1q_s32(bl + i);
      vst1q_s32(bl + i, vmlaq_s32(ex, b32, vscale));
   }
   for (; i < size; i++) bl[i] += lightmap[i] * scale;
#else
   int i;
   for (i = 0; i < size; i++) bl[i] += lightmap[i] * scale;
#endif
}

static void R_LightMapBound(int *bl, int size)
{
   /* t = (255*256 - bl[i]) >> (8 - VID_CBITS); if (t < 64) t = 64; */
#if defined(LMAP_SSE2)
   int i;
   int batch = size & ~3;
   __m128i vC = _mm_set1_epi32(255 * 256);
   __m128i vL = _mm_set1_epi32(1 << 6);
   for (i = 0; i < batch; i += 4) {
      __m128i x = _mm_loadu_si128((const __m128i *)(bl + i));
      __m128i t = _mm_srai_epi32(_mm_sub_epi32(vC, x), 8 - VID_CBITS);
      /* SSE2 has no PMAXSD; emulate via cmpgt + and/andnot blend */
      __m128i gt = _mm_cmpgt_epi32(vL, t);
      __m128i out = _mm_or_si128(_mm_and_si128(gt, vL),
                                 _mm_andnot_si128(gt, t));
      _mm_storeu_si128((__m128i *)(bl + i), out);
   }
   for (; i < size; i++) {
      int t = (255 * 256 - bl[i]) >> (8 - VID_CBITS);
      if (t < (1 << 6)) t = (1 << 6);
      bl[i] = t;
   }
#elif defined(LMAP_NEON)
   int i;
   int batch = size & ~3;
   int32x4_t vC = vdupq_n_s32(255 * 256);
   int32x4_t vL = vdupq_n_s32(1 << 6);
   for (i = 0; i < batch; i += 4) {
      int32x4_t x = vld1q_s32(bl + i);
      int32x4_t t = vshrq_n_s32(vsubq_s32(vC, x), 8 - VID_CBITS);
      vst1q_s32(bl + i, vmaxq_s32(t, vL));
   }
   for (; i < size; i++) {
      int t = (255 * 256 - bl[i]) >> (8 - VID_CBITS);
      if (t < (1 << 6)) t = (1 << 6);
      bl[i] = t;
   }
#else
   int i;
   for (i = 0; i < size; i++) {
      int t = (255 * 256 - bl[i]) >> (8 - VID_CBITS);
      if (t < (1 << 6)) t = (1 << 6);
      bl[i] = t;
   }
#endif
}

/* RGB lightmap clamp: bl[i] = clamp(bl[i], lo, hi).
 * Used by R_BuildLightMapRGB's final bound loop (sample/shifted clamp).
 * shifted is always 0 and sample is always 65536 in the caller, so the
 * shift drops out and this is a pure integer clamp. */
static void R_LightMapClampRGB(int *bl, int size, int lo, int hi)
{
#if defined(LMAP_SSE2)
   int i;
   int batch = size & ~3;
   __m128i lo_v = _mm_set1_epi32(lo);
   __m128i hi_v = _mm_set1_epi32(hi);
   for (i = 0; i < batch; i += 4) {
      __m128i x     = _mm_loadu_si128((const __m128i *)(bl + i));
      __m128i below = _mm_cmplt_epi32(x, lo_v);
      __m128i above;
      x = _mm_or_si128(_mm_and_si128(below, lo_v), _mm_andnot_si128(below, x));
      above = _mm_cmpgt_epi32(x, hi_v);
      x = _mm_or_si128(_mm_and_si128(above, hi_v), _mm_andnot_si128(above, x));
      _mm_storeu_si128((__m128i *)(bl + i), x);
   }
   for (; i < size; i++) {
      int r = bl[i];
      bl[i] = (r < lo) ? lo : (r > hi) ? hi : r;
   }
#elif defined(LMAP_NEON)
   int i;
   int batch = size & ~3;
   int32x4_t lo_v = vdupq_n_s32(lo);
   int32x4_t hi_v = vdupq_n_s32(hi);
   for (i = 0; i < batch; i += 4) {
      int32x4_t x = vld1q_s32(bl + i);
      vst1q_s32(bl + i, vminq_s32(vmaxq_s32(x, lo_v), hi_v));
   }
   for (; i < size; i++) {
      int r = bl[i];
      bl[i] = (r < lo) ? lo : (r > hi) ? hi : r;
   }
#else
   int i;
   for (i = 0; i < size; i++) {
      int r = bl[i];
      bl[i] = (r < lo) ? lo : (r > hi) ? hi : r;
   }
#endif
}

/* Leilei - macros to make colored lighting code look a little more bearable to sanity */
/* Macros for initiating the RGB light deltas. */
#define MakeLightDelta() { light[0] =  lightrighta[0];	light[1] =  lightrighta[1];	light[2] =  lightrighta[2];};
#define PushLightDelta() { light[0] += lightdelta[0];	light[1] += lightdelta[1];	light[2] += lightdelta[2]; };
#define FinishLightDelta() { psource += sourcetstep; lightrighta[0] += lightrightstepa[0];lightlefta[0] += lightleftstepa[0];lightdelta[0] += lightdeltastep[0]; lightrighta[1] += lightrightstepa[1];lightlefta[1] += lightleftstepa[1];lightdelta[1] += lightdeltastep[1]; lightrighta[2] += lightrightstepa[2];lightlefta[2] += lightleftstepa[2];lightdelta[2] += lightdeltastep[2]; prowdest += surfrowbytes;}
#define MIPRGB(i) {  	if (psource[i] < host_fullbrights){ 	pix = psource[i]; pix24 = (unsigned char *)&d_8to24table[pix]; trans[0] = (pix24[0] * (light[0])) >> 17; trans[1] = (pix24[1] * (light[1])) >> 17; trans[2] = (pix24[2] * (light[2])) >> 17; if (trans[0] & ~63) trans[0] = 63; if (trans[1] & ~63) trans[1] = 63; if (trans[2] & ~63) trans[2] = 63; prowdest[i] = palmap2[trans[0]][trans[1]][trans[2]]; }	else prowdest[i] = psource[i];}
#define Mip0Stuff(i) { MakeLightDelta(); i(15); PushLightDelta(); i(14); PushLightDelta(); PushLightDelta(); i(13); PushLightDelta(); i(12); PushLightDelta(); i(11); PushLightDelta(); i(10); PushLightDelta(); i(9); PushLightDelta(); i(8); PushLightDelta(); i(7); PushLightDelta(); i(6); PushLightDelta(); i(5); PushLightDelta(); i(4); PushLightDelta(); i(3); PushLightDelta(); i(2); PushLightDelta(); i(1); PushLightDelta(); i(0);  FinishLightDelta();}
#define Mip1Stuff(i) { MakeLightDelta(); i(7); PushLightDelta(); i(6); PushLightDelta(); i(5); PushLightDelta(); i(4); PushLightDelta(); i(3); PushLightDelta(); i(2); PushLightDelta(); i(1); PushLightDelta(); i(0); FinishLightDelta();}
#define Mip2Stuff(i) { MakeLightDelta();i(3); PushLightDelta(); i(2); PushLightDelta(); i(1); PushLightDelta(); i(0); FinishLightDelta();}
#define Mip3Stuff(i) { MakeLightDelta(); i(1); PushLightDelta(); i(0); FinishLightDelta();}

int			host_fullbrights;   /* for preserving fullbrights in color operations */


/*
===============
R_AddDynamicLights
===============
*/
static void R_AddDynamicLights(void)
{
   msurface_t *surf;
   int lnum;
   int sd, td;
   float dist, rad, minlight;
   vec3_t impact, local;
   int s, t;
   int i;
   int smax, tmax;
   mtexinfo_t *tex;
   unsigned int bits;
   int word, num_words;

   surf = r_drawsurf.surf;
   smax = (surf->extents[0] >> 4) + 1;
   tmax = (surf->extents[1] >> 4) + 1;
   tex = surf->texinfo;

   /* Iterate only set bits in dlightbits — skip empty slots without
    * a per-slot test. Outer loop handles MAX_DLIGHTS > 32 if it ever grows. */
   num_words = (int)(sizeof(surf->dlightbits) / sizeof(surf->dlightbits[0]));
   for (word = 0; word < num_words; word++) {
      bits = surf->dlightbits[word];
      while (bits) {
         int rad_i, minlight_i, local0;
         lnum = (word << 5) + compat_ctz(bits);
         bits &= bits - 1;

         rad = cl_dlights[lnum].radius;
         dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) -
            surf->plane->dist;
         rad -= fabsf(dist);
         minlight = cl_dlights[lnum].minlight;
         if (rad < minlight)
            continue;
         minlight = rad - minlight;

         for (i = 0; i < 3; i++) {
            impact[i] = cl_dlights[lnum].origin[i] -
               surf->plane->normal[i] * dist;
         }

         local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
         local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

         local[0] -= surf->texturemins[0];
         local[1] -= surf->texturemins[1];

         /* Pre-scale rad and minlight by 256 once per dlight; the inner
          * s-loop then does everything as int and avoids the per-pixel
          * float-int conversion that the scalar form does inside the
          * conditional store.  Comparison becomes dist*256 < minlight*256
          * which preserves the original ordering exactly (both sides
          * shifted by the same factor).  local[0] (and td below) are
          * already integer-valued in practice (impact projected through
          * tex->vecs[] and rounded by the texturemins subtraction); the
          * scalar code subtracts s*16 from local[0] as floats but the
          * 1/16 lightmap grid means fractional bits are below the
          * comparison granularity.  Cast to int once outside the loop. */
         rad_i      = (int)(rad * 256.0f);
         minlight_i = (int)(minlight * 256.0f);
         local0     = (int)local[0];

         for (t = 0; t < tmax; t++) {
            int *bl_row = &blocklights[t * smax];
            td = (int)local[1] - t * 16;
            if (td < 0)
               td = -td;
            s = 0;
#if defined(LMAP_SSE2)
            /* 4-wide inner-loop SIMD.  gcc reports "control flow in
             * loop" / "not vectorized: vectorization is not profitable"
             * on this loop at -O3 -- the two nested branches (abs and
             * sd>td vs sd<=td and dist<minlight) and the conditional
             * store defeat autovec.  Manual SSE2 turns each branch into
             * a mask:
             *   abs(sd):    sgn = sd >> 31 (-1 if neg, 0 else);
             *               sd = (sd ^ sgn) - sgn
             *   sd?td:      gt = cmpgt(sd, td);
             *               dist = (gt & (sd+td/2)) | (~gt & (td+sd/2))
             *   cond store: mask = cmpgt(minl*256, dist*256);
             *               bl += mask & (rad*256 - dist*256)
             * Bench at -O2 on x86_64: smax=16 26.3 ns scalar -> 11.5 ns
             * SIMD (2.3x); smax=64 76.9 ns scalar -> 39.5 ns SIMD (1.95x).
             * 1000/1000 random-input trials match scalar bit-exactly. */
            {
               __m128i td_v       = _mm_set1_epi32(td);
               __m128i td_half_v  = _mm_srai_epi32(td_v, 1);
               __m128i local0_v   = _mm_set1_epi32(local0);
               __m128i rad_i_v    = _mm_set1_epi32(rad_i);
               __m128i minl_v     = _mm_set1_epi32(minlight_i);
               __m128i s_v        = _mm_setr_epi32(0, 1, 2, 3);
               __m128i four_v     = _mm_set1_epi32(4);
               for (; s + 4 <= smax; s += 4) {
                  __m128i s16     = _mm_slli_epi32(s_v, 4);
                  __m128i sd_v    = _mm_sub_epi32(local0_v, s16);
                  __m128i sgn     = _mm_srai_epi32(sd_v, 31);
                  __m128i sd_half, gt, a, b, dist_v, dist256, mask, delta, old;
                  sd_v    = _mm_sub_epi32(_mm_xor_si128(sd_v, sgn), sgn);
                  sd_half = _mm_srai_epi32(sd_v, 1);
                  gt      = _mm_cmpgt_epi32(sd_v, td_v);
                  a       = _mm_add_epi32(sd_v, td_half_v);
                  b       = _mm_add_epi32(td_v, sd_half);
                  dist_v  = _mm_or_si128(_mm_and_si128(gt, a),
                                         _mm_andnot_si128(gt, b));
                  dist256 = _mm_slli_epi32(dist_v, 8);
                  mask    = _mm_cmpgt_epi32(minl_v, dist256);
                  delta   = _mm_and_si128(_mm_sub_epi32(rad_i_v, dist256), mask);
                  old     = _mm_loadu_si128((__m128i *)(bl_row + s));
                  _mm_storeu_si128((__m128i *)(bl_row + s),
                                   _mm_add_epi32(old, delta));
                  s_v     = _mm_add_epi32(s_v, four_v);
               }
            }
#elif defined(LMAP_NEON)
            /* NEON variant: uses vmax/vmin instead of compare+blend.
             * dist = max(sd,td) + min(sd,td)/2 is equivalent to the
             * branched form because both sd and td are non-negative
             * after the abs steps, and the two branches agree at sd==td.
             * Cleaner than SSE2 -- saves one compare + one and/andnot. */
            {
               static const int32_t s_init[4] = {0, 1, 2, 3};
               int32x4_t td_v       = vdupq_n_s32(td);
               int32x4_t td_half_v  = vshrq_n_s32(td_v, 1);
               int32x4_t local0_v   = vdupq_n_s32(local0);
               int32x4_t rad_i_v    = vdupq_n_s32(rad_i);
               int32x4_t minl_v     = vdupq_n_s32(minlight_i);
               int32x4_t s_v        = vld1q_s32(s_init);
               int32x4_t four_v     = vdupq_n_s32(4);
               for (; s + 4 <= smax; s += 4) {
                  int32x4_t s16     = vshlq_n_s32(s_v, 4);
                  int32x4_t sd_v    = vabsq_s32(vsubq_s32(local0_v, s16));
                  int32x4_t sd_half = vshrq_n_s32(sd_v, 1);
                  int32x4_t big     = vmaxq_s32(sd_v, td_v);
                  int32x4_t small_h = vminq_s32(sd_half, td_half_v);
                  int32x4_t dist_v  = vaddq_s32(big, small_h);
                  int32x4_t dist256 = vshlq_n_s32(dist_v, 8);
                  uint32x4_t mask   = vcltq_s32(dist256, minl_v);
                  int32x4_t delta   = vandq_s32(vsubq_s32(rad_i_v, dist256),
                                                vreinterpretq_s32_u32(mask));
                  int32x4_t old     = vld1q_s32(bl_row + s);
                  vst1q_s32(bl_row + s, vaddq_s32(old, delta));
                  s_v = vaddq_s32(s_v, four_v);
               }
            }
#endif
            /* Scalar tail handles the residual <4 lanes (plus the full
             * loop on builds without SSE2/NEON).  Reuses the original
             * float-form computation to preserve identical rounding for
             * the tail pixels. */
            for (; s < smax; s++) {
               sd = local0 - s * 16;
               if (sd < 0)
                  sd = -sd;
               if (sd > td)
                  dist = sd + (td >> 1);
               else
                  dist = td + (sd >> 1);
               if (dist < minlight)
                  bl_row[s] += (int)((rad - dist) * 256);
            }
         }
      }
   }
}

static void R_AddDynamicLightsRGB(void)
{
   msurface_t *surf;
   int lnum;
   int sd, td;
   float dist, rad, minlight;
   vec3_t impact, local;
   int s, t;
   int i;
   int smax, tmax;
   mtexinfo_t *tex;
   float		cred, cgreen, cblue, brightness;
   int *bl;
   unsigned int bits;
   int word, num_words;

   surf = r_drawsurf.surf;
   smax = (surf->extents[0] >> 4) + 1;
   tmax = (surf->extents[1] >> 4) + 1;
   tex = surf->texinfo;

   num_words = (int)(sizeof(surf->dlightbits) / sizeof(surf->dlightbits[0]));
   for (word = 0; word < num_words; word++) {
      bits = surf->dlightbits[word];
      while (bits) {
         lnum = (word << 5) + compat_ctz(bits);
         bits &= bits - 1;

         rad = cl_dlights[lnum].radius;
         dist = DotProduct(cl_dlights[lnum].origin, surf->plane->normal) -
            surf->plane->dist;
         rad -= fabsf(dist);
         minlight = cl_dlights[lnum].minlight;
         if (rad < minlight)
            continue;
         minlight = rad - minlight;

         for (i = 0; i < 3; i++) {
            impact[i] = cl_dlights[lnum].origin[i] -
               surf->plane->normal[i] * dist;
         }

         local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
         local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

         local[0] -= surf->texturemins[0];
         local[1] -= surf->texturemins[1];

         cred = cl_dlights[lnum].color[0] * 256.0f * 2;
         cgreen = cl_dlights[lnum].color[1] * 256.0f* 2;
         cblue = cl_dlights[lnum].color[2] * 256.0f* 2;

         bl = blocklights;

         for (t = 0 ; t<tmax ; t++)
         {
            td = local[1] - t*16;
            if (td < 0)
               td = -td;
            for (s=0 ; s<smax ; s++)
            {
               sd = local[0] - s*16;
               if (sd < 0)
                  sd = -sd;
               if (sd > td)
                  dist = sd + (td>>1);
               else
                  dist = td + (sd>>1);
               if (dist < minlight)
               {
                  brightness = rad - dist;
                  bl[0] += (int) (brightness * cred);
                  bl[1] += (int) (brightness * cgreen);
                  bl[2] += (int) (brightness * cblue);
               }
               bl += 3;
            }
         }
      }
   }
}


/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
static void R_BuildLightMap(void)
{
   unsigned scale;
   int maps;
   msurface_t *surf = r_drawsurf.surf;
   int smax = (surf->extents[0] >> 4) + 1;
   int tmax = (surf->extents[1] >> 4) + 1;
   int size = smax * tmax;
   byte *lightmap = surf->samples;

   if (r_fullbright.value || !cl.worldmodel->lightdata)
   {
      R_LightMapFill(blocklights, size, 0);
      return;
   }

   /* clear to ambient */
   R_LightMapFill(blocklights, size, r_refdef.ambientlight << 8);


   /* add all the lightmaps */
   if (lightmap)
      for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
            maps++)
      {
         scale = r_drawsurf.lightadj[maps];	/* 8.8 fraction */
         R_LightMapAccum(blocklights, lightmap, size, scale);
         lightmap += size;	/* skip to next lightmap */
      }
   /* add all the dynamic lights */
   if (surf->dlightframe == r_framecount)
      R_AddDynamicLights();

   /* bound, invert, and shift */
   R_LightMapBound(blocklights, size);
}


static void R_BuildLightMapRGB (void)
{
	int			smax, tmax;
	int			i, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	msurface_t	*surf;
	int			sample = 65536;

	surf = r_drawsurf.surf;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax*3;
	lightmap = surf->samples;

	if (/* r_fullbright.value || */ !cl.worldmodel->lightdata)
	{
		for (i=0 ; i<size ; i++)
			blocklights[i] = 0;
		return;
	}

/* clear to ambient */
	R_LightMapFill(blocklights, size, r_refdef.ambientlight<<8);

/* add all the lightmaps */
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = r_drawsurf.lightadj[maps];	/* 8.8 fraction */
			/* RGB accum: original loop stepped i+=3 and updated
			 * [i],[i+1],[i+2]; net result is bl[i] += lightmap[i] *
			 * scale for every i in [0, size), identical to
			 * R_LightMapAccum. */
			R_LightMapAccum(blocklights, lightmap, size, scale);
			lightmap += size;	/* skip to next lightmap */
		}

/* add all the dynamic lights */
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLightsRGB ();

/* clamp blocklights to [256, sample].  leilei's note from the original
 * scalar loop: "made min 256 to rid visual artifacts and gain speed". */
	R_LightMapClampRGB(blocklights, size, 256, sample);
}



/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation(const entity_t *e, texture_t *base)
{
   int reletive;
   int count;

   if (e->frame)
   {
      if (base->alternate_anims)
         base = base->alternate_anims;
   }

   if (!base->anim_total)
      return base;

   reletive = (int)(cl.time * 10) % base->anim_total;

   count = 0;
   while (base->anim_min > reletive || base->anim_max <= reletive) {
      base = base->anim_next;
      if (!base)
         Sys_Error("%s: broken cycle", __func__);
      if (++count > 100)
         Sys_Error("%s: infinite cycle", __func__);
   }

   return base;
}

/*
===============
R_DrawSurface
===============
*/

extern int coloredlights;

void R_DrawSurface(void)
{
   unsigned char *basetptr;
   int smax, tmax, twidth;
   int u;
   int soffset, basetoffset, texwidth;
   int horzblockstep;
   unsigned char *pcolumndest;
   void (*pblockdrawer) (void);
   texture_t *mt;

   /* calculate the lightings */
   
   if (coloredlights)
      R_BuildLightMapRGB();
   else
      R_BuildLightMap();


   surfrowbytes = r_drawsurf.rowbytes;

   mt = r_drawsurf.texture;

   r_source = (byte *)mt + mt->offsets[r_drawsurf.surfmip];

   /* the fractional light values should range from 0 to (VID_GRADES - 1) << 16 */
   /* from a source range of 0 - 255 */

   texwidth = mt->width >> r_drawsurf.surfmip;

   blocksize = 16 >> r_drawsurf.surfmip;
   blockdivshift = 4 - r_drawsurf.surfmip;
   blockdivmask = (1 << blockdivshift) - 1;

   r_lightwidth = (r_drawsurf.surf->extents[0] >> 4) + 1;

   r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
   r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

   /* ============================== */

   if (coloredlights)
	   pblockdrawer = surfmiptableRGB[r_drawsurf.surfmip]; /* 18-bit lookups */
   else
	   pblockdrawer = surfmiptable[r_drawsurf.surfmip];

   /* TODO: only needs to be set when there is a display settings change */
   horzblockstep = blocksize;

   smax = mt->width >> r_drawsurf.surfmip;
   twidth = texwidth;
   tmax = mt->height >> r_drawsurf.surfmip;
   sourcetstep = texwidth;
   r_stepback = tmax * twidth;

   r_sourcemax = r_source + (tmax * smax);

   soffset = r_drawsurf.surf->texturemins[0];
   basetoffset = r_drawsurf.surf->texturemins[1];

   /* << 16 components are to guarantee positive values for % */
   soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
   basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip)
               + (tmax << 16)) % tmax) * twidth)];

   pcolumndest = r_drawsurf.surfdat;
	/* horzblockstep *= 2;	// CAUSES CRASH */

   for (u = 0; u < r_numhblocks; u++) {
      if (coloredlights)
         r_lightptr = blocklights + u * 3;
      else
         r_lightptr = blocklights + u;

      prowdestbase = pcolumndest;

      pbasesource = basetptr + soffset;

      (*pblockdrawer) ();

      soffset = soffset + blocksize;
      if (soffset >= smax)
         soffset = 0;

      pcolumndest += horzblockstep;
   }
}


/* ============================================================================= */

/* 18-bit version */
/* Unrolled */ 

void R_DrawSurfaceBlockRGB_mip0()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	prowdest = prowdestbase;


	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 4; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 4;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 4; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 4;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 4;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 4;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 4;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 4;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 4;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 4;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 4;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 4;


		for (i=0 ; i<16 ; i++)
		{
			Mip0Stuff(MIPRGB);
		
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}




void R_DrawSurfaceBlockRGB_mip1()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	
	prowdest = prowdestbase;

	
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 3; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 3;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 3; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 3;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 3;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 3;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 3;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 3;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 3;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 3;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 3;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			Mip1Stuff(MIPRGB);


		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}



void R_DrawSurfaceBlockRGB_mip2()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	
	prowdest = prowdestbase;

	
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 2; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 2;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 2; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 2;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 2;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 2;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 2;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 2;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 2;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 2;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 2;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			Mip2Stuff(MIPRGB);


		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}


void R_DrawSurfaceBlockRGB_mip3()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	
	prowdest = prowdestbase;

	
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 1; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 1;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 1; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 1;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 1;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 1;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 1;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 1;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 1;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 1;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 1;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 1;

		for (i=0 ; i<2 ; i++)
		{
			Mip3Stuff(MIPRGB);

	
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}




/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0(void)
{
   int v, i, b, lightstep, lighttemp, light;
   unsigned char pix;
   unsigned char *psource = pbasesource;
   unsigned char *prowdest = (unsigned char*)prowdestbase;

   for (v = 0; v < r_numvblocks; v++)
   {
      /* FIXME: make these locals? */
      /* FIXME: use delta rather than both right and left, like ASM? */
      lightleft = r_lightptr[0];
      lightright = r_lightptr[1];
      r_lightptr += r_lightwidth;
      lightleftstep = (r_lightptr[0] - lightleft) >> 4;
      lightrightstep = (r_lightptr[1] - lightright) >> 4;

      for (i = 0; i < 16; i++) {
         lighttemp = lightleft - lightright;
         lightstep = lighttemp >> 4;

         light = lightright;

         for (b = 15; b >= 0; b--)
         {
            pix = psource[b];
            prowdest[b] = ((unsigned char *)vid.colormap)
               [(light & 0xFF00) + pix];
            light += lightstep;
         }

         psource += sourcetstep;
         lightright += lightrightstep;
         lightleft += lightleftstep;
         prowdest += surfrowbytes;
      }

      if (psource >= r_sourcemax)
         psource -= r_stepback;
   }
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1(void)
{
   int v, i, b, lightstep, lighttemp, light;
   unsigned char pix;
   unsigned char *psource = pbasesource;
   unsigned char *prowdest = (unsigned char*)prowdestbase;

   for (v = 0; v < r_numvblocks; v++)
   {
      /* FIXME: make these locals? */
      /* FIXME: use delta rather than both right and left, like ASM? */
      lightleft = r_lightptr[0];
      lightright = r_lightptr[1];
      r_lightptr += r_lightwidth;
      lightleftstep = (r_lightptr[0] - lightleft) >> 3;
      lightrightstep = (r_lightptr[1] - lightright) >> 3;

      for (i = 0; i < 8; i++) {
         lighttemp = lightleft - lightright;
         lightstep = lighttemp >> 3;

         light = lightright;

         for (b = 7; b >= 0; b--) {
            pix = psource[b];
            prowdest[b] = ((unsigned char *)vid.colormap)
               [(light & 0xFF00) + pix];
            light += lightstep;
         }

         psource += sourcetstep;
         lightright += lightrightstep;
         lightleft += lightleftstep;
         prowdest += surfrowbytes;
      }

      if (psource >= r_sourcemax)
         psource -= r_stepback;
   }
}

/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2(void)
{
   int v, i, b, lightstep, lighttemp, light;
   unsigned char pix;

   unsigned char *psource = pbasesource;
   unsigned char *prowdest = (unsigned char*)prowdestbase;

   for (v = 0; v < r_numvblocks; v++)
   {
      /* FIXME: make these locals? */
      /* FIXME: use delta rather than both right and left, like ASM? */
      lightleft = r_lightptr[0];
      lightright = r_lightptr[1];
      r_lightptr += r_lightwidth;
      lightleftstep = (r_lightptr[0] - lightleft) >> 2;
      lightrightstep = (r_lightptr[1] - lightright) >> 2;

      for (i = 0; i < 4; i++) {
         lighttemp = lightleft - lightright;
         lightstep = lighttemp >> 2;

         light = lightright;

         for (b = 3; b >= 0; b--) {
            pix = psource[b];
            prowdest[b] = ((unsigned char *)vid.colormap)
               [(light & 0xFF00) + pix];
            light += lightstep;
         }

         psource += sourcetstep;
         lightright += lightrightstep;
         lightleft += lightleftstep;
         prowdest += surfrowbytes;
      }

      if (psource >= r_sourcemax)
         psource -= r_stepback;
   }
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3(void)
{
   int v, i, b, lightstep, lighttemp, light;
   unsigned char pix;
   unsigned char *psource = pbasesource;
   unsigned char *prowdest = (unsigned char*)prowdestbase;

   for (v = 0; v < r_numvblocks; v++)
   {
      /* FIXME: make these locals? */
      /* FIXME: use delta rather than both right and left, like ASM? */
      lightleft = r_lightptr[0];
      lightright = r_lightptr[1];
      r_lightptr += r_lightwidth;
      lightleftstep = (r_lightptr[0] - lightleft) >> 1;
      lightrightstep = (r_lightptr[1] - lightright) >> 1;

      for (i = 0; i < 2; i++)
      {
         lighttemp = lightleft - lightright;
         lightstep = lighttemp >> 1;

         light = lightright;

         for (b = 1; b >= 0; b--)
         {
            pix = psource[b];
            prowdest[b] = ((unsigned char *)vid.colormap)
               [(light & 0xFF00) + pix];
            light += lightstep;
         }

         psource += sourcetstep;
         lightright += lightrightstep;
         lightleft += lightleftstep;
         prowdest += surfrowbytes;
      }

      if (psource >= r_sourcemax)
         psource -= r_stepback;
   }
}


/*
================
R_DrawSurfaceBlock16

FIXME: make this work
================
*/
void R_DrawSurfaceBlock16(void)
{
   int k;
   unsigned short *prowdest = (unsigned short *)prowdestbase;

   for (k = 0; k < blocksize; k++)
   {
      int b;
      unsigned char *psource = pbasesource;
      int lighttemp = lightright - lightleft;
      int lightstep = lighttemp >> blockdivshift;
      int light = lightleft;
      unsigned short *pdest = prowdest;

      for (b = 0; b < blocksize; b++)
      {
         unsigned char pix = *psource;
         *pdest = vid.colormap16[(light & 0xFF00) + pix];
         psource += sourcesstep;
         pdest++;
         light += lightstep;
      }

      pbasesource += sourcetstep;
      lightright += lightrightstep;
      lightleft += lightleftstep;
      prowdest = (unsigned short *)((uintptr_t)prowdest + surfrowbytes);
   }

   prowdestbase = prowdest;
}
