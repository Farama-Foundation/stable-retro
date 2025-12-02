/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2016 Senquack (dansilsby <AT> gmail <DOT> com)          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#ifndef __GPU_UNAI_GPU_INNER_H__
#define __GPU_UNAI_GPU_INNER_H__

///////////////////////////////////////////////////////////////////////////////
// Inner loop driver instantiation file

///////////////////////////////////////////////////////////////////////////////
//  Option Masks (CF template paramter)
#define  CF_LIGHT     ((CF>> 0)&1) // Lighting
#define  CF_BLEND     ((CF>> 1)&1) // Blending
#define  CF_MASKCHECK ((CF>> 2)&1) // Mask bit check
#define  CF_BLENDMODE ((CF>> 3)&3) // Blend mode   0..3
#define  CF_TEXTMODE  ((CF>> 5)&3) // Texture mode 1..3 (0: texturing disabled)
#define  CF_GOURAUD   ((CF>> 7)&1) // Gouraud shading
#define  CF_MASKSET   ((CF>> 8)&1) // Mask bit set
#define  CF_DITHER    ((CF>> 9)&1) // Dithering
#define  CF_BLITMASK  ((CF>>10)&1) // blit_mask check (skip rendering pixels
                                   //  that wouldn't end up displayed on
                                   //  low-res screen using simple downscaler)

//#ifdef __arm__
//#ifndef ENABLE_GPU_ARMV7
/* ARMv5 */
//#include "gpu_inner_blend_arm5.h"
//#else
/* ARMv7 optimized */
//#include "gpu_inner_blend_arm7.h"
//#endif
//#else
//#include "gpu_inner_blend.h"
//#endif

#include "gpu_inner_blend.h"
#include "gpu_inner_quantization.h"
#include "gpu_inner_light.h"

#include "arm_features.h"
#include "compiler_features.h"
#ifdef __arm__
#include "gpu_arm.h"
#include "gpu_inner_blend_arm.h"
#include "gpu_inner_light_arm.h"
#define gpuBlending gpuBlendingARM
#define gpuLightingTXT gpuLightingTXTARM
#else
#define gpuBlending gpuBlendingGeneric
#define gpuLightingTXT gpuLightingTXTGeneric
#endif

// Non-dithering lighting and blending functions preserve uSrc
// MSB. This saves a few operations and useless load/stores.
#define MSB_PRESERVED (!CF_DITHER)

// If defined, Gouraud colors are fixed-point 5.11, otherwise they are 8.16
// This is only for debugging/verification of low-precision colors in C.
// Low-precision Gouraud is intended for use by SIMD-optimized inner drivers
// which get/use Gouraud colors in SIMD registers.
//#define GPU_GOURAUD_LOW_PRECISION

// How many bits of fixed-point precision GouraudColor uses
#ifdef GPU_GOURAUD_LOW_PRECISION
#define GPU_GOURAUD_FIXED_BITS 11
#else
#define GPU_GOURAUD_FIXED_BITS 16
#endif

// Used to pass Gouraud colors to gpuPixelSpanFn() (lines)
struct GouraudColor {
#ifdef GPU_GOURAUD_LOW_PRECISION
	u16 r, g, b;
	s16 r_incr, g_incr, b_incr;
#else
	u32 r, g, b;
	s32 r_incr, g_incr, b_incr;
#endif
};

static inline u16 gpuGouraudColor15bpp(u32 r, u32 g, u32 b)
{
	r >>= GPU_GOURAUD_FIXED_BITS;
	g >>= GPU_GOURAUD_FIXED_BITS;
	b >>= GPU_GOURAUD_FIXED_BITS;

#ifndef GPU_GOURAUD_LOW_PRECISION
	// High-precision Gouraud colors are 8-bit + fractional
	r >>= 3;  g >>= 3;  b >>= 3;
#endif

	return r | (g << 5) | (b << 10);
}

///////////////////////////////////////////////////////////////////////////////
//  GPU Pixel span operations generator gpuPixelSpanFn<>
//  Oct 2016: Created/adapted from old gpuPixelFn by senquack:
//  Original gpuPixelFn was used to draw lines one pixel at a time. I wrote
//  new line algorithms that draw lines using horizontal/vertical/diagonal
//  spans of pixels, necessitating new pixel-drawing function that could
//  not only render spans of pixels, but gouraud-shade them as well.
//  This speeds up line rendering and would allow tile-rendering (untextured
//  rectangles) to use the same set of functions. Since tiles are always
//  monochrome, they simply wouldn't use the extra set of 32 gouraud-shaded
//  gpuPixelSpanFn functions (TODO?).
template<int CF>
static le16_t* gpuPixelSpanFn(le16_t* pDst, uintptr_t data, ptrdiff_t incr, size_t len)
{
	// Blend func can save an operation if it knows uSrc MSB is
	//  unset. For untextured prims, this is always true.
	const bool skip_uSrc_mask = true;

	u16 col;
	struct GouraudColor * gcPtr;
	u32 r, g, b;
	s32 r_incr, g_incr, b_incr;

	// Caller counts in bytes, we count in pixels
	incr /= 2;

	if (CF_GOURAUD) {
		gcPtr = (GouraudColor*)data;
		r = gcPtr->r;  r_incr = gcPtr->r_incr;
		g = gcPtr->g;  g_incr = gcPtr->g_incr;
		b = gcPtr->b;  b_incr = gcPtr->b_incr;
	} else {
		col = (u16)data;
	}

	do {
		if (!CF_GOURAUD)
		{   // NO GOURAUD
			if (!CF_MASKCHECK && !CF_BLEND) {
				if (CF_MASKSET) { *pDst = u16_to_le16(col | 0x8000); }
				else            { *pDst = u16_to_le16(col);          }
			} else if (CF_MASKCHECK && !CF_BLEND) {
				if (!(le16_raw(*pDst) & HTOLE16(0x8000))) {
					if (CF_MASKSET) { *pDst = u16_to_le16(col | 0x8000); }
					else            { *pDst = u16_to_le16(col);          }
				}
			} else {
				uint_fast16_t uDst = le16_to_u16(*pDst);
				if (CF_MASKCHECK) { if (uDst & 0x8000) goto endpixel; }

				uint_fast16_t uSrc = col;

				if (CF_BLEND)
					uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);

				if (CF_MASKSET) { *pDst = u16_to_le16(uSrc | 0x8000); }
				else            { *pDst = u16_to_le16(uSrc);          }
			}

		} else
		{   // GOURAUD

			if (!CF_MASKCHECK && !CF_BLEND) {
				col = gpuGouraudColor15bpp(r, g, b);
				if (CF_MASKSET) { *pDst = u16_to_le16(col | 0x8000); }
				else            { *pDst = u16_to_le16(col);          }
			} else if (CF_MASKCHECK && !CF_BLEND) {
				col = gpuGouraudColor15bpp(r, g, b);
				if (!(le16_raw(*pDst) & HTOLE16(0x8000))) {
					if (CF_MASKSET) { *pDst = u16_to_le16(col | 0x8000); }
					else            { *pDst = u16_to_le16(col);          }
				}
			} else {
				uint_fast16_t uDst = le16_to_u16(*pDst);
				if (CF_MASKCHECK) { if (uDst & 0x8000) goto endpixel; }
				col = gpuGouraudColor15bpp(r, g, b);

				uint_fast16_t uSrc = col;

				// Blend func can save an operation if it knows uSrc MSB is
				//  unset. For untextured prims, this is always true.
				const bool skip_uSrc_mask = true;

				if (CF_BLEND)
					uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);

				if (CF_MASKSET) { *pDst = u16_to_le16(uSrc | 0x8000); }
				else            { *pDst = u16_to_le16(uSrc);          }
			}
		}

endpixel:
		if (CF_GOURAUD) {
			r += r_incr;
			g += g_incr;
			b += b_incr;
		}
		pDst += incr;
	} while (len-- > 1);

	// Note from senquack: Normally, I'd prefer to write a 'do {} while (--len)'
	//  loop, or even a for() loop, however, on MIPS platforms anything but the
	//  'do {} while (len-- > 1)' tends to generate very unoptimal asm, with
	//  many unneeded MULs/ADDs/branches at the ends of these functions.
	//  If you change the loop structure above, be sure to compare the quality
	//  of the generated code!!

	if (CF_GOURAUD) {
		gcPtr->r = r;
		gcPtr->g = g;
		gcPtr->b = b;
	}
	return pDst;
}

static le16_t* PixelSpanNULL(le16_t* pDst, uintptr_t data, ptrdiff_t incr, size_t len)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"PixelSpanNULL()\n");
	#endif
	return pDst;
}

///////////////////////////////////////////////////////////////////////////////
//  PixelSpan (lines) innerloops driver
typedef le16_t* (*PSD)(le16_t* dst, uintptr_t data, ptrdiff_t incr, size_t len);

const PSD gpuPixelSpanDrivers[64] =
{ 
	// Array index | 'CF' template field | Field value
	// ------------+---------------------+----------------
	// Bit 0       | CF_BLEND            | off (0), on (1)
	// Bit 1       | CF_MASKCHECK        | off (0), on (1)
	// Bit 3:2     | CF_BLENDMODE        | 0..3
	// Bit 4       | CF_MASKSET          | off (0), on (1)
	// Bit 5       | CF_GOURAUD          | off (0), on (1)
	//
	// NULL entries are ones for which blending is disabled and blend-mode
	//  field is non-zero, which is obviously invalid.

	// Flat-shaded
	gpuPixelSpanFn<0x00<<1>,         gpuPixelSpanFn<0x01<<1>,         gpuPixelSpanFn<0x02<<1>,         gpuPixelSpanFn<0x03<<1>,
	PixelSpanNULL,                   gpuPixelSpanFn<0x05<<1>,         PixelSpanNULL,                   gpuPixelSpanFn<0x07<<1>,
	PixelSpanNULL,                   gpuPixelSpanFn<0x09<<1>,         PixelSpanNULL,                   gpuPixelSpanFn<0x0B<<1>,
	PixelSpanNULL,                   gpuPixelSpanFn<0x0D<<1>,         PixelSpanNULL,                   gpuPixelSpanFn<0x0F<<1>,

	// Flat-shaded + PixelMSB (CF_MASKSET)
	gpuPixelSpanFn<(0x00<<1)|0x100>, gpuPixelSpanFn<(0x01<<1)|0x100>, gpuPixelSpanFn<(0x02<<1)|0x100>, gpuPixelSpanFn<(0x03<<1)|0x100>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x05<<1)|0x100>, PixelSpanNULL,                   gpuPixelSpanFn<(0x07<<1)|0x100>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x09<<1)|0x100>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0B<<1)|0x100>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x0D<<1)|0x100>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0F<<1)|0x100>,

	// Gouraud-shaded (CF_GOURAUD)
	gpuPixelSpanFn<(0x00<<1)|0x80>,  gpuPixelSpanFn<(0x01<<1)|0x80>,  gpuPixelSpanFn<(0x02<<1)|0x80>,  gpuPixelSpanFn<(0x03<<1)|0x80>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x05<<1)|0x80>,  PixelSpanNULL,                   gpuPixelSpanFn<(0x07<<1)|0x80>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x09<<1)|0x80>,  PixelSpanNULL,                   gpuPixelSpanFn<(0x0B<<1)|0x80>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x0D<<1)|0x80>,  PixelSpanNULL,                   gpuPixelSpanFn<(0x0F<<1)|0x80>,

	// Gouraud-shaded (CF_GOURAUD) + PixelMSB (CF_MASKSET)
	gpuPixelSpanFn<(0x00<<1)|0x180>, gpuPixelSpanFn<(0x01<<1)|0x180>, gpuPixelSpanFn<(0x02<<1)|0x180>, gpuPixelSpanFn<(0x03<<1)|0x180>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x05<<1)|0x180>, PixelSpanNULL,                   gpuPixelSpanFn<(0x07<<1)|0x180>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x09<<1)|0x180>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0B<<1)|0x180>,
	PixelSpanNULL,                   gpuPixelSpanFn<(0x0D<<1)|0x180>, PixelSpanNULL,                   gpuPixelSpanFn<(0x0F<<1)|0x180>
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Tiles innerloops generator

template<int CF>
static inline void gpuTileSpanFn(le16_t *pDst, u16 data, u32 count)
{
	le16_t ldata;

	if (!CF_MASKCHECK && !CF_BLEND) {
		if (CF_MASKSET)
			ldata = u16_to_le16(data | 0x8000);
		else
			ldata = u16_to_le16(data);
		do { *pDst++ = ldata; } while (--count);
	} else if (CF_MASKCHECK && !CF_BLEND) {
		if (CF_MASKSET)
			ldata = u16_to_le16(data | 0x8000);
		else
			ldata = u16_to_le16(data);
		do {
			if (!(le16_raw(*pDst) & HTOLE16(0x8000)))
				*pDst = ldata;
			pDst++;
		} while (--count);
	} else
	{
		// Blend func can save an operation if it knows uSrc MSB is
		//  unset. For untextured prims, this is always true.
		const bool skip_uSrc_mask = true;

		uint_fast16_t uSrc, uDst;
		do
		{
			if (CF_MASKCHECK || CF_BLEND) { uDst = le16_to_u16(*pDst); }
			if (CF_MASKCHECK) if (uDst&0x8000) { goto endtile; }

			uSrc = data;

			if (CF_BLEND)
				uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);

			if (CF_MASKSET) { *pDst = u16_to_le16(uSrc | 0x8000); }
			else            { *pDst = u16_to_le16(uSrc);          }

			//senquack - Did not apply "Silent Hill" mask-bit fix to here.
			// It is hard to tell from scarce documentation available and
			//  lack of comments in code, but I believe the tile-span
			//  functions here should not bother to preserve any source MSB,
			//  as they are not drawing from a texture.
endtile:
			pDst++;
		}
		while (--count);
	}
}

template<int CF>
static noinline void gpuTileDriverFn(le16_t *pDst, u16 data, u32 count,
	const gpu_unai_inner_t &inn)
{
	const int li=gpu_unai.inn.ilace_mask;
	const int pi=(ProgressiveInterlaceEnabled()?(gpu_unai.inn.ilace_mask+1):0);
	const int pif=(ProgressiveInterlaceEnabled()?(gpu_unai.prog_ilace_flag?(gpu_unai.inn.ilace_mask+1):0):1);
	const int y1 = inn.y1;
	int y0 = inn.y0;

	for (; y0 < y1; ++y0) {
		if (!(y0&li) && (y0&pi) != pif)
			gpuTileSpanFn<CF>(pDst, data, count);
		pDst += FRAME_WIDTH;
	}
}

#ifdef __arm__

template<int CF>
static void TileAsm(le16_t *pDst, u16 data, u32 count, const gpu_unai_inner_t &inn)
{
	switch (CF) {
	case 0x02: tile_driver_st0_asm(pDst, data, count, &inn); return;
	case 0x0a: tile_driver_st1_asm(pDst, data, count, &inn); return;
	case 0x1a: tile_driver_st3_asm(pDst, data, count, &inn); return;
#ifdef HAVE_ARMV6
	case 0x12: tile_driver_st2_asm(pDst, data, count, &inn); return;
#endif
	}
	gpuTileDriverFn<CF>(pDst, data, count, inn);
}

#endif

static void TileNULL(le16_t *pDst, u16 data, u32 count, const gpu_unai_inner_t &inn)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"TileNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Tiles innerloops driver
typedef void (*PT)(le16_t *pDst, u16 data, u32 count, const gpu_unai_inner_t &inn);

// Template instantiation helper macros
#define TI(cf) gpuTileDriverFn<(cf)>
#define TN     TileNULL
#ifdef __arm__
#define TA(cf) TileAsm<(cf)>
#else
#define TA(cf) TI(cf)
#endif
#ifdef HAVE_ARMV6
#define TA6(cf) TileAsm<(cf)>
#else
#define TA6(cf) TI(cf)
#endif
#define TIBLOCK(ub) \
	TI((ub)|0x00), TA6((ub)|0x02), TI((ub)|0x04), TI((ub)|0x06), \
	TN,            TA ((ub)|0x0a), TN,            TI((ub)|0x0e), \
	TN,            TA6((ub)|0x12), TN,            TI((ub)|0x16), \
	TN,            TA ((ub)|0x1a), TN,            TI((ub)|0x1e)

const PT gpuTileDrivers[32] = {
	TIBLOCK(0<<8), TIBLOCK(1<<8)
};

#undef TI
#undef TN
#undef TA
#undef TA6
#undef TIBLOCK


///////////////////////////////////////////////////////////////////////////////
//  GPU Sprites innerloops generator

typedef void (*PS)(le16_t *pPixel, u32 count, const u8 *pTxt,
	const gpu_unai_inner_t &inn);

template<int CF>
static noinline void gpuSpriteDriverFn(le16_t *pPixel, u32 count, const u8 *pTxt_base,
	const gpu_unai_inner_t &inn)
{
	// Blend func can save an operation if it knows uSrc MSB is unset.
	//  Untextured prims can always skip (source color always comes with MSB=0).
	//  For textured prims, the generic lighting funcs always return it unset. (bonus!)
	const bool skip_uSrc_mask = MSB_PRESERVED ? (!CF_TEXTMODE) : (!CF_TEXTMODE) || CF_LIGHT;

	uint_fast16_t uSrc, uDst, srcMSB;
	bool should_blend;
	u32 u0_mask = inn.u_msk >> 10;

	u8 r5, g5, b5;
	if (CF_LIGHT) {
		r5 = inn.r5;
		g5 = inn.g5;
		b5 = inn.b5;
	}

	const le16_t *CBA_; if (CF_TEXTMODE!=3) CBA_ = inn.CBA;
	const u32 v0_mask = inn.v_msk >> 10;
	s32 y0 = inn.y0, y1 = inn.y1, li = inn.ilace_mask;
	u32 u0_ = inn.u, v0 = inn.v;

	if (CF_TEXTMODE==3) {
		// Texture is accessed byte-wise, so adjust to 16bpp
		u0_ <<= 1;
		u0_mask <<= 1;
	}

	for (; y0 < y1; ++y0, pPixel += FRAME_WIDTH, ++v0)
	{
	  if (y0 & li) continue;
	  const u8 *pTxt = pTxt_base + ((v0 & v0_mask) * 2048);
	  le16_t *pDst = pPixel;
	  u32 u0 = u0_;
	  u32 count1 = count;
	  do
	  {
		if (CF_MASKCHECK || CF_BLEND) { uDst = le16_to_u16(*pDst); }
		if (CF_MASKCHECK) if (uDst&0x8000) { goto endsprite; }

		if (CF_TEXTMODE==1) {  //  4bpp (CLUT)
			u8 rgb = pTxt[(u0 & u0_mask)>>1];
			uSrc = le16_to_u16(CBA_[(rgb>>((u0&1)<<2))&0xf]);
		}
		if (CF_TEXTMODE==2) {  //  8bpp (CLUT)
			uSrc = le16_to_u16(CBA_[pTxt[u0 & u0_mask]]);
		}
		if (CF_TEXTMODE==3) {  // 16bpp
			uSrc = le16_to_u16(*(le16_t*)(&pTxt[u0 & u0_mask]));
		}

		if (!uSrc) goto endsprite;

		//senquack - save source MSB, as blending or lighting macros will not
		//           (Silent Hill gray rectangles mask bit bug)
		if (CF_BLEND || CF_LIGHT) srcMSB = uSrc & 0x8000;
		
		if (CF_LIGHT)
			uSrc = gpuLightingTXT(uSrc, r5, g5, b5);

		should_blend = MSB_PRESERVED ? uSrc & 0x8000 : srcMSB;

		if (CF_BLEND && should_blend)
			uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);

		if (CF_MASKSET)                                    { *pDst = u16_to_le16(uSrc | 0x8000); }
		else if (!MSB_PRESERVED && (CF_BLEND || CF_LIGHT)) { *pDst = u16_to_le16(uSrc | srcMSB); }
		else                                               { *pDst = u16_to_le16(uSrc);          }

endsprite:
		u0 += (CF_TEXTMODE==3) ? 2 : 1;
		pDst++;
	  }
	  while (--count1);
	}
}

#ifdef __arm__

template<int CF>
static void SpriteMaybeAsm(le16_t *pPixel, u32 count, const u8 *pTxt_base,
        const gpu_unai_inner_t &inn)
{
#if 1
  s32 lines = inn.y1 - inn.y0;
  u32 u1m = inn.u + count - 1, v1m = inn.v + lines - 1;
  if (u1m == (u1m & (inn.u_msk >> 10)) && v1m == (v1m & (inn.v_msk >> 10))) {
    const u8 *pTxt = pTxt_base + inn.v * 2048;
    switch (CF) {
    case 0x20: sprite_driver_4bpp_asm (pPixel, pTxt + inn.u / 2, count, &inn); return;
    case 0x40: sprite_driver_8bpp_asm (pPixel, pTxt + inn.u,     count, &inn); return;
    case 0x60: sprite_driver_16bpp_asm(pPixel, pTxt + inn.u * 2, count, &inn); return;
    }
  }
  if (v1m == (v1m & (inn.v_msk >> 10))) {
    const u8 *pTxt = pTxt_base + inn.v * 2048;
    switch (CF) {
    case 0x20: sprite_driver_4bpp_l0_std_asm(pPixel, pTxt, count, &inn); return;
    case 0x22: sprite_driver_4bpp_l0_st0_asm(pPixel, pTxt, count, &inn); return;
    case 0x40: sprite_driver_8bpp_l0_std_asm(pPixel, pTxt, count, &inn); return;
    case 0x42: sprite_driver_8bpp_l0_st0_asm(pPixel, pTxt, count, &inn); return;
#ifdef HAVE_ARMV6
    case 0x21: sprite_driver_4bpp_l1_std_asm(pPixel, pTxt, count, &inn); return;
    case 0x23: sprite_driver_4bpp_l1_st0_asm(pPixel, pTxt, count, &inn); return;
    case 0x2b: sprite_driver_4bpp_l1_st1_asm(pPixel, pTxt, count, &inn); return;
    case 0x41: sprite_driver_8bpp_l1_std_asm(pPixel, pTxt, count, &inn); return;
    case 0x43: sprite_driver_8bpp_l1_st0_asm(pPixel, pTxt, count, &inn); return;
    case 0x4b: sprite_driver_8bpp_l1_st1_asm(pPixel, pTxt, count, &inn); return;
#endif
    }
  }
#endif
  gpuSpriteDriverFn<CF>(pPixel, count, pTxt_base, inn);
}
#endif // __arm__

static void SpriteNULL(le16_t *pPixel, u32 count, const u8 *pTxt_base,
	const gpu_unai_inner_t &inn)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"SpriteNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Sprite innerloops driver

// Template instantiation helper macros
#define TI(cf) gpuSpriteDriverFn<(cf)>
#define TN     SpriteNULL
#ifdef __arm__
#define TA(cf) SpriteMaybeAsm<(cf)>
#else
#define TA(cf) TI(cf)
#endif
#ifdef HAVE_ARMV6
#define TA6(cf) SpriteMaybeAsm<(cf)>
#else
#define TA6(cf) TI(cf)
#endif
#define TIBLOCK(ub) \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TN,            TN,            TN,            TN,            TN,            TN,            TN,            TN,            \
	TA((ub)|0x20), TA6((ub)|0x21),TA6((ub)|0x22),TA6((ub)|0x23),TI((ub)|0x24), TI((ub)|0x25), TI((ub)|0x26), TI((ub)|0x27), \
	TN,            TN,            TI((ub)|0x2a), TA6((ub)|0x2b),TN,            TN,            TI((ub)|0x2e), TI((ub)|0x2f), \
	TN,            TN,            TI((ub)|0x32), TI((ub)|0x33), TN,            TN,            TI((ub)|0x36), TI((ub)|0x37), \
	TN,            TN,            TI((ub)|0x3a), TI((ub)|0x3b), TN,            TN,            TI((ub)|0x3e), TI((ub)|0x3f), \
	TA((ub)|0x40), TA6((ub)|0x41),TA6((ub)|0x42),TA6((ub)|0x43),TI((ub)|0x44), TI((ub)|0x45), TI((ub)|0x46), TI((ub)|0x47), \
	TN,            TN,            TI((ub)|0x4a), TA6((ub)|0x4b),TN,            TN,            TI((ub)|0x4e), TI((ub)|0x4f), \
	TN,            TN,            TI((ub)|0x52), TI((ub)|0x53), TN,            TN,            TI((ub)|0x56), TI((ub)|0x57), \
	TN,            TN,            TI((ub)|0x5a), TI((ub)|0x5b), TN,            TN,            TI((ub)|0x5e), TI((ub)|0x5f), \
	TA((ub)|0x60), TI((ub)|0x61), TI((ub)|0x62), TI((ub)|0x63), TI((ub)|0x64), TI((ub)|0x65), TI((ub)|0x66), TI((ub)|0x67), \
	TN,            TN,            TI((ub)|0x6a), TI((ub)|0x6b), TN,            TN,            TI((ub)|0x6e), TI((ub)|0x6f), \
	TN,            TN,            TI((ub)|0x72), TI((ub)|0x73), TN,            TN,            TI((ub)|0x76), TI((ub)|0x77), \
	TN,            TN,            TI((ub)|0x7a), TI((ub)|0x7b), TN,            TN,            TI((ub)|0x7e), TI((ub)|0x7f)

const PS gpuSpriteDrivers[256] = {
	TIBLOCK(0<<8), TIBLOCK(1<<8)
};

#undef TI
#undef TN
#undef TIBLOCK
#undef TA
#undef TA6

///////////////////////////////////////////////////////////////////////////////
//  GPU Polygon innerloops generator

//senquack - Newer version with following changes:
//           * Adapted to work with new poly routings in gpu_raster_polygon.h
//             adapted from DrHell GPU. They are less glitchy and use 22.10
//             fixed-point instead of original UNAI's 16.16.
//           * Texture coordinates are no longer packed together into one
//             unsigned int. This seems to lose too much accuracy (they each
//             end up being only 8.7 fixed-point that way) and pixel-droupouts
//             were noticeable both with original code and current DrHell
//             adaptations. An example would be the sky in NFS3. Now, they are
//             stored in separate ints, using separate masks.
//           * Function is no longer INLINE, as it was always called
//             through a function pointer.
//           * Function now ensures the mask bit of source texture is preserved
//             across calls to blending functions (Silent Hill rectangles fix)
//           * November 2016: Large refactoring of blending/lighting when
//             JohnnyF added dithering. See gpu_inner_quantization.h and
//             relevant blend/light headers.
// (see README_senquack.txt)
template<int CF>
static noinline void gpuPolySpanFn(const gpu_unai_t &gpu_unai, le16_t *pDst, u32 count)
{
	// Blend func can save an operation if it knows uSrc MSB is unset.
	//  Untextured prims can always skip this (src color MSB is always 0).
	//  For textured prims, the generic lighting funcs always return it unset. (bonus!)
	const bool skip_uSrc_mask = MSB_PRESERVED ? (!CF_TEXTMODE) : (!CF_TEXTMODE) || CF_LIGHT;
	bool should_blend;

	u32 bMsk; if (CF_BLITMASK) bMsk = gpu_unai.inn.blit_mask;

	if (!CF_TEXTMODE)
	{
		if (!CF_GOURAUD)
		{
			// UNTEXTURED, NO GOURAUD
			const u16 pix15 = gpu_unai.inn.PixelData;
			do {
				uint_fast16_t uSrc, uDst;

				// NOTE: Don't enable CF_BLITMASK  pixel skipping (speed hack)
				//  on untextured polys. It seems to do more harm than good: see
				//  gravestone text at end of Medieval intro sequence. -senquack
				//if (CF_BLITMASK) { if ((bMsk>>((((uintptr_t)pDst)>>1)&7))&1) { goto endpolynotextnogou; } }

				if (CF_BLEND || CF_MASKCHECK) uDst = le16_to_u16(*pDst);
				if (CF_MASKCHECK) { if (uDst&0x8000) { goto endpolynotextnogou; } }

				uSrc = pix15;

				if (CF_BLEND)
					uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);

				if (CF_MASKSET) { *pDst = u16_to_le16(uSrc | 0x8000); }
				else            { *pDst = u16_to_le16(uSrc);          }

endpolynotextnogou:
				pDst++;
			} while(--count);
		}
		else
		{
			// UNTEXTURED, GOURAUD
			gcol_t l_gCol = gpu_unai.inn.gCol;
			gcol_t l_gInc = gpu_unai.inn.gInc;

			do {
				uint_fast16_t uDst, uSrc;

				// See note in above loop regarding CF_BLITMASK
				//if (CF_BLITMASK) { if ((bMsk>>((((uintptr_t)pDst)>>1)&7))&1) goto endpolynotextgou; }

				if (CF_BLEND || CF_MASKCHECK) uDst = le16_to_u16(*pDst);
				if (CF_MASKCHECK) { if (uDst&0x8000) goto endpolynotextgou; }

				if (CF_DITHER) {
					// GOURAUD, DITHER

					u32 uSrc24 = gpuLightingRGB24(l_gCol);
					if (CF_BLEND)
						uSrc24 = gpuBlending24<CF_BLENDMODE>(uSrc24, uDst);
					uSrc = gpuColorQuantization24<CF_DITHER>(uSrc24, pDst);
				} else {
					// GOURAUD, NO DITHER

					uSrc = gpuLightingRGB(l_gCol);

					if (CF_BLEND)
						uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);
				}

				if (CF_MASKSET) { *pDst = u16_to_le16(uSrc | 0x8000); }
				else            { *pDst = u16_to_le16(uSrc);          }

endpolynotextgou:
				pDst++;
				l_gCol.raw += l_gInc.raw;
			}
			while (--count);
		}
	}
	else
	{
		// TEXTURED

		uint_fast16_t uDst, uSrc, srcMSB;

		//senquack - note: original UNAI code had gpu_unai.{u4/v4} packed into
		// one 32-bit unsigned int, but this proved to lose too much accuracy
		// (pixel drouputs noticeable in NFS3 sky), so now are separate vars.
		u32 l_u_msk = gpu_unai.inn.u_msk;     u32 l_v_msk = gpu_unai.inn.v_msk;
		u32 l_u = gpu_unai.inn.u & l_u_msk;   u32 l_v = gpu_unai.inn.v & l_v_msk;
		s32 l_u_inc = gpu_unai.inn.u_inc;     s32 l_v_inc = gpu_unai.inn.v_inc;
		l_v <<= 1;
		l_v_inc <<= 1;
		l_v_msk = (l_v_msk & (0xff<<10)) << 1;

		const le16_t* TBA_ = gpu_unai.inn.TBA;
		const le16_t* CBA_; if (CF_TEXTMODE!=3) CBA_ = gpu_unai.inn.CBA;

		u8 r5, g5, b5;
		u8 r8, g8, b8;

		gcol_t l_gInc, l_gCol;

		if (CF_LIGHT) {
			if (CF_GOURAUD) {
				l_gInc = gpu_unai.inn.gInc;
				l_gCol = gpu_unai.inn.gCol;
			} else {
				if (CF_DITHER) {
					r8 = gpu_unai.inn.r8;
					g8 = gpu_unai.inn.g8;
					b8 = gpu_unai.inn.b8;
				} else {
					r5 = gpu_unai.inn.r5;
					g5 = gpu_unai.inn.g5;
					b5 = gpu_unai.inn.b5;
				}
			}
		}

		do
		{
			if (CF_BLITMASK) { if ((bMsk>>((((uintptr_t)pDst)>>1)&7))&1) goto endpolytext; }
			if (CF_MASKCHECK || CF_BLEND) { uDst = le16_to_u16(*pDst); }
			if (CF_MASKCHECK) if (uDst&0x8000) { goto endpolytext; }

			//senquack - adapted to work with new 22.10 fixed point routines:
			//           (UNAI originally used 16.16)
			if (CF_TEXTMODE==1) {  //  4bpp (CLUT)
				u32 tu=(l_u>>10);
				u32 tv=l_v&l_v_msk;
				u8 rgb=((u8*)TBA_)[tv+(tu>>1)];
				uSrc=le16_to_u16(CBA_[(rgb>>((tu&1)<<2))&0xf]);
				if (!uSrc) goto endpolytext;
			}
			if (CF_TEXTMODE==2) {  //  8bpp (CLUT)
				u32 tv=l_v&l_v_msk;
				uSrc = le16_to_u16(CBA_[((u8*)TBA_)[tv+(l_u>>10)]]);
				if (!uSrc) goto endpolytext;
			}
			if (CF_TEXTMODE==3) {  // 16bpp
				u32 tv=(l_v&l_v_msk)>>1;
				uSrc = le16_to_u16(TBA_[tv+(l_u>>10)]);
				if (!uSrc) goto endpolytext;
			}

			// Save source MSB, as blending or lighting will not (Silent Hill)
			if (CF_BLEND || CF_LIGHT) srcMSB = uSrc & 0x8000;

			// When textured, only dither when LIGHT (texture blend) is enabled
			// LIGHT &&  BLEND => dither
			// LIGHT && !BLEND => dither
			//!LIGHT &&  BLEND => no dither
			//!LIGHT && !BLEND => no dither

			if (CF_DITHER && CF_LIGHT) {
				u32 uSrc24;
				if ( CF_GOURAUD)
					uSrc24 = gpuLightingTXT24Gouraud(uSrc, l_gCol);
				if (!CF_GOURAUD)
					uSrc24 = gpuLightingTXT24(uSrc, r8, g8, b8);

				if (CF_BLEND && srcMSB)
					uSrc24 = gpuBlending24<CF_BLENDMODE>(uSrc24, uDst);

				uSrc = gpuColorQuantization24<CF_DITHER>(uSrc24, pDst);
			} else
			{
				if (CF_LIGHT) {
					if ( CF_GOURAUD)
						uSrc = gpuLightingTXTGouraud(uSrc, l_gCol);
					if (!CF_GOURAUD)
						uSrc = gpuLightingTXT(uSrc, r5, g5, b5);
				}

				should_blend = MSB_PRESERVED ? uSrc & 0x8000 : srcMSB;
				if (CF_BLEND && should_blend)
					uSrc = gpuBlending<CF_BLENDMODE, skip_uSrc_mask>(uSrc, uDst);
			}

			if (CF_MASKSET)                                    { *pDst = u16_to_le16(uSrc | 0x8000); }
			else if (!MSB_PRESERVED && (CF_BLEND || CF_LIGHT)) { *pDst = u16_to_le16(uSrc | srcMSB); }
			else                                               { *pDst = u16_to_le16(uSrc);          }
endpolytext:
			pDst++;
			l_u = (l_u + l_u_inc) & l_u_msk;
			l_v += l_v_inc;
			if (CF_LIGHT && CF_GOURAUD)
				l_gCol.raw += l_gInc.raw;
		}
		while (--count);
	}
}

#ifdef __arm__
template<int CF>
static void PolySpanMaybeAsm(const gpu_unai_t &gpu_unai, le16_t *pDst, u32 count)
{
	switch (CF) {
	case 0x02: poly_untex_st0_asm  (pDst, &gpu_unai.inn, count); break;
	case 0x0a: poly_untex_st1_asm  (pDst, &gpu_unai.inn, count); break;
	case 0x1a: poly_untex_st3_asm  (pDst, &gpu_unai.inn, count); break;
	case 0x20: poly_4bpp_asm       (pDst, &gpu_unai.inn, count); break;
	case 0x22: poly_4bpp_l0_st0_asm(pDst, &gpu_unai.inn, count); break;
	case 0x40: poly_8bpp_asm       (pDst, &gpu_unai.inn, count); break;
	case 0x42: poly_8bpp_l0_st0_asm(pDst, &gpu_unai.inn, count); break;
#ifdef HAVE_ARMV6
	case 0x12: poly_untex_st2_asm  (pDst, &gpu_unai.inn, count); break;
	case 0x21: poly_4bpp_l1_std_asm(pDst, &gpu_unai.inn, count); break;
	case 0x23: poly_4bpp_l1_st0_asm(pDst, &gpu_unai.inn, count); break;
	case 0x41: poly_8bpp_l1_std_asm(pDst, &gpu_unai.inn, count); break;
	case 0x43: poly_8bpp_l1_st0_asm(pDst, &gpu_unai.inn, count); break;
#endif
	default:   gpuPolySpanFn<CF>(gpu_unai, pDst, count);
	}
}
#endif

static void PolyNULL(const gpu_unai_t &gpu_unai, le16_t *pDst, u32 count)
{
	#ifdef ENABLE_GPU_LOG_SUPPORT
		fprintf(stdout,"PolyNULL()\n");
	#endif
}

///////////////////////////////////////////////////////////////////////////////
//  Polygon innerloops driver
typedef void (*PP)(const gpu_unai_t &gpu_unai, le16_t *pDst, u32 count);

// Template instantiation helper macros
#define TI(cf) gpuPolySpanFn<(cf)>
#define TN     PolyNULL
#ifdef __arm__
#define TA(cf) PolySpanMaybeAsm<(cf)>
#else
#define TA(cf) TI(cf)
#endif
#ifdef HAVE_ARMV6
#define TA6(cf) PolySpanMaybeAsm<(cf)>
#else
#define TA6(cf) TI(cf)
#endif
#define TIBLOCK(ub) \
	TI((ub)|0x00), TI((ub)|0x01), TA6((ub)|0x02),TI((ub)|0x03), TI((ub)|0x04), TI((ub)|0x05), TI((ub)|0x06), TI((ub)|0x07), \
	TN,            TN,            TA((ub)|0x0a), TI((ub)|0x0b), TN,            TN,            TI((ub)|0x0e), TI((ub)|0x0f), \
	TN,            TN,            TA6((ub)|0x12),TI((ub)|0x13), TN,            TN,            TI((ub)|0x16), TI((ub)|0x17), \
	TN,            TN,            TA((ub)|0x1a), TI((ub)|0x1b), TN,            TN,            TI((ub)|0x1e), TI((ub)|0x1f), \
	TA((ub)|0x20), TA6((ub)|0x21),TA6((ub)|0x22),TA6((ub)|0x23),TI((ub)|0x24), TI((ub)|0x25), TI((ub)|0x26), TI((ub)|0x27), \
	TN,            TN,            TI((ub)|0x2a), TI((ub)|0x2b), TN,            TN,            TI((ub)|0x2e), TI((ub)|0x2f), \
	TN,            TN,            TI((ub)|0x32), TI((ub)|0x33), TN,            TN,            TI((ub)|0x36), TI((ub)|0x37), \
	TN,            TN,            TI((ub)|0x3a), TI((ub)|0x3b), TN,            TN,            TI((ub)|0x3e), TI((ub)|0x3f), \
	TA((ub)|0x40), TA6((ub)|0x41),TA6((ub)|0x42),TA6((ub)|0x43),TI((ub)|0x44), TI((ub)|0x45), TI((ub)|0x46), TI((ub)|0x47), \
	TN,            TN,            TI((ub)|0x4a), TI((ub)|0x4b), TN,            TN,            TI((ub)|0x4e), TI((ub)|0x4f), \
	TN,            TN,            TI((ub)|0x52), TI((ub)|0x53), TN,            TN,            TI((ub)|0x56), TI((ub)|0x57), \
	TN,            TN,            TI((ub)|0x5a), TI((ub)|0x5b), TN,            TN,            TI((ub)|0x5e), TI((ub)|0x5f), \
	TI((ub)|0x60), TI((ub)|0x61), TI((ub)|0x62), TI((ub)|0x63), TI((ub)|0x64), TI((ub)|0x65), TI((ub)|0x66), TI((ub)|0x67), \
	TN,            TN,            TI((ub)|0x6a), TI((ub)|0x6b), TN,            TN,            TI((ub)|0x6e), TI((ub)|0x6f), \
	TN,            TN,            TI((ub)|0x72), TI((ub)|0x73), TN,            TN,            TI((ub)|0x76), TI((ub)|0x77), \
	TN,            TN,            TI((ub)|0x7a), TI((ub)|0x7b), TN,            TN,            TI((ub)|0x7e), TI((ub)|0x7f), \
	TN,            TI((ub)|0x81), TN,            TI((ub)|0x83), TN,            TI((ub)|0x85), TN,            TI((ub)|0x87), \
	TN,            TN,            TN,            TI((ub)|0x8b), TN,            TN,            TN,            TI((ub)|0x8f), \
	TN,            TN,            TN,            TI((ub)|0x93), TN,            TN,            TN,            TI((ub)|0x97), \
	TN,            TN,            TN,            TI((ub)|0x9b), TN,            TN,            TN,            TI((ub)|0x9f), \
	TN,            TI((ub)|0xa1), TN,            TI((ub)|0xa3), TN,            TI((ub)|0xa5), TN,            TI((ub)|0xa7), \
	TN,            TN,            TN,            TI((ub)|0xab), TN,            TN,            TN,            TI((ub)|0xaf), \
	TN,            TN,            TN,            TI((ub)|0xb3), TN,            TN,            TN,            TI((ub)|0xb7), \
	TN,            TN,            TN,            TI((ub)|0xbb), TN,            TN,            TN,            TI((ub)|0xbf), \
	TN,            TI((ub)|0xc1), TN,            TI((ub)|0xc3), TN,            TI((ub)|0xc5), TN,            TI((ub)|0xc7), \
	TN,            TN,            TN,            TI((ub)|0xcb), TN,            TN,            TN,            TI((ub)|0xcf), \
	TN,            TN,            TN,            TI((ub)|0xd3), TN,            TN,            TN,            TI((ub)|0xd7), \
	TN,            TN,            TN,            TI((ub)|0xdb), TN,            TN,            TN,            TI((ub)|0xdf), \
	TN,            TI((ub)|0xe1), TN,            TI((ub)|0xe3), TN,            TI((ub)|0xe5), TN,            TI((ub)|0xe7), \
	TN,            TN,            TN,            TI((ub)|0xeb), TN,            TN,            TN,            TI((ub)|0xef), \
	TN,            TN,            TN,            TI((ub)|0xf3), TN,            TN,            TN,            TI((ub)|0xf7), \
	TN,            TN,            TN,            TI((ub)|0xfb), TN,            TN,            TN,            TI((ub)|0xff)

const PP gpuPolySpanDrivers[2048] = {
	TIBLOCK(0<<8), TIBLOCK(1<<8), TIBLOCK(2<<8), TIBLOCK(3<<8),
	TIBLOCK(4<<8), TIBLOCK(5<<8), TIBLOCK(6<<8), TIBLOCK(7<<8)
};

#undef TI
#undef TN
#undef TIBLOCK
#undef TA
#undef TA6

#endif /* __GPU_UNAI_GPU_INNER_H__ */
