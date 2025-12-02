/***************************************************************************
*   Copyright (C) 2016 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
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

#ifndef _OP_LIGHT_H_
#define _OP_LIGHT_H_

//  GPU color operations for lighting calculations

static void SetupLightLUT()
{
	// 1024-entry lookup table that modulates 5-bit texture + 5-bit light value.
	// A light value of 15 does not modify the incoming texture color.
	// LightLUT[32*32] array is initialized to following values:
	//  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	//  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	//  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	//  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
	//  0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
	//  0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9,
	//  0, 0, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9,10,10,10,11,11,
	//  0, 0, 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7, 8, 8, 9, 9,10,10,10,11,11,12,12,13,13,
	//  0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14,14,15,15,
	//  0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7, 8, 9, 9,10,10,11,11,12,12,13,14,14,15,15,16,16,17,
	//  0, 0, 1, 1, 2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9,10,10,11,11,12,13,13,14,15,15,16,16,17,18,18,19,
	//  0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9,10,11,11,12,13,13,14,15,15,16,17,17,18,19,19,20,21,
	//  0, 0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9,10,11,12,12,13,14,15,15,16,17,18,18,19,20,21,21,22,23,
	//  0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9,10,11,12,13,13,14,15,16,17,17,18,19,20,21,21,22,23,24,25,
	//  0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9,10,11,12,13,14,14,15,16,17,18,19,20,21,21,22,23,24,25,26,27,
	//  0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
	//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,31,
	//  0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,27,28,29,30,31,31,31,31,
	//  0, 1, 2, 3, 4, 5, 7, 8, 9,10,11,13,14,15,16,17,19,20,21,22,23,24,26,27,28,29,30,31,31,31,31,31,
	//  0, 1, 2, 3, 5, 6, 7, 8,10,11,12,13,15,16,17,18,20,21,22,23,25,26,27,28,30,31,31,31,31,31,31,31,
	//  0, 1, 2, 3, 5, 6, 7, 9,10,11,13,14,15,17,18,19,21,22,23,24,26,27,28,30,31,31,31,31,31,31,31,31,
	//  0, 1, 2, 4, 5, 6, 8, 9,11,12,13,15,16,17,19,20,22,23,24,26,27,28,30,31,31,31,31,31,31,31,31,31,
	//  0, 1, 2, 4, 5, 7, 8,10,11,12,14,15,17,18,20,21,23,24,25,27,28,30,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 4, 6, 7, 9,10,12,13,15,16,18,19,21,22,24,25,27,28,30,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 4, 6, 7, 9,10,12,14,15,17,18,20,21,23,25,26,28,29,31,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 4, 6, 8, 9,11,13,14,16,17,19,21,22,24,26,27,29,30,31,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 5, 6, 8,10,11,13,15,16,18,20,21,23,25,27,28,30,31,31,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 5, 7, 8,10,12,14,15,17,19,21,22,24,26,28,29,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 5, 7, 9,10,12,14,16,18,19,21,23,25,27,29,30,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 5, 7, 9,11,13,15,16,18,20,22,24,26,28,30,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
	//  0, 1, 3, 5, 7, 9,11,13,15,17,19,21,23,25,27,29,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31

	for (int j=0; j < 32; ++j) {
		for (int i=0; i < 32; ++i) {
			int val = i * j / 16;
			if (val > 31) val = 31;
			gpu_unai.LightLUT[(j*32) + i] = val;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Create packed Gouraud fixed-pt 8.8 rgb triplet
//
// INPUT:
// 'r','g','b' are 8.10 fixed-pt color components (r shown here)
//     'r' input:  --------------rrrrrrrrXXXXXXXXXX
//                 ^ bit 31
// RETURNS:
//    gcol_t output:  ccccccccXXXXXXXX for c in [r, g, b]
//                    ^ bit 16
// Where 'r,g,b' are integer bits of colors, 'X' fixed-pt, and '-' don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE gcol_t gpuPackGouraudCol(u32 r, u32 g, u32 b)
{
	return (gcol_t){
		(u16)(r >> 2),
		(u16)(g >> 2),
		(u16)(b >> 2),
	};
}

////////////////////////////////////////////////////////////////////////////////
// Create packed increment for Gouraud fixed-pt 8.8 rgb triplet
//
// INPUT:
//  Sign-extended 8.10 fixed-pt r,g,b color increment values (only dr is shown)
//   'dr' input:  ssssssssssssssrrrrrrrrXXXXXXXXXX
//                ^ bit 31
// RETURNS:
//   gcol_t output:  ccccccccXXXXXXXX for c in [r, g, b]
//                   ^ bit 16
// Where 'r,g,b' are integer bits of colors, 'X' fixed-pt, and 's' sign bits
//
// NOTE: The correctness of this code/method has not been fully verified,
//       having been merely factored out from original code in
//       poly-drawing functions. Feel free to check/improve it -senquack
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE gcol_t gpuPackGouraudColInc(s32 dr, s32 dg, s32 db)
{
	return (gcol_t){
		(u16)((dr >> 2) + (dr < 0)),
		(u16)((dg >> 2) + (dg < 0)),
		(u16)((db >> 2) + (db < 0)),
	};
}

////////////////////////////////////////////////////////////////////////////////
// Extract bgr555 color from Gouraud u32 fixed-pt 8.8 rgb triplet
//
// INPUT:
//  'gCol' input:  ccccccccXXXXXXXX for c in [r, g, b]
//                 ^ bit 16
// RETURNS:
//    u16 output:  0bbbbbgggggrrrrr
//                 ^ bit 16
// Where 'r,g,b' are integer bits of colors, 'X' fixed-pt, and '0' zero
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE uint_fast16_t gpuLightingRGB(gcol_t gCol)
{
	return (gCol.c.r >> 11) |
		((gCol.c.g >> 6) & 0x3e0) |
		((gCol.c.b >> 1) & 0x7c00);
}

////////////////////////////////////////////////////////////////////////////////
// Convert packed Gouraud u32 fixed-pt 8.8 rgb triplet in 'gCol'
//  to padded u32 5.4 bgr fixed-pt triplet, suitable for use
//  with HQ 24-bit lighting/quantization.
//
// INPUT:
//       'gCol' input:  ccccccccXXXXXXXX for c in [r, g, b]
//                      ^ bit 16
// RETURNS:
//         u32 output:  000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                      ^ bit 31
//  Where 'X' are fixed-pt bits, '0' zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE u32 gpuLightingRGB24(gcol_t gCol)
{
	return (gCol.c.r >> 7)
		| ((gCol.c.g >> 7) << 10)
		| ((gCol.c.b >> 7) << 20);
}

////////////////////////////////////////////////////////////////////////////////
// Apply fast (low-precision) 5-bit lighting to bgr555 texture color:
//
// INPUT:
//        'r5','g5','b5' are unsigned 5-bit color values, value of 15
//          is midpoint that doesn't modify that component of texture
//        'uSrc' input:  -bbbbbgggggrrrrr
//                       ^ bit 16
// RETURNS:
//          u16 output:  0bbbbbgggggrrrrr
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE uint_fast16_t gpuLightingTXTGeneric(uint_fast16_t uSrc, u8 r5, u8 g5, u8 b5)
{
	return (gpu_unai.LightLUT[((uSrc&0x7C00)>>5) | b5] << 10) |
	       (gpu_unai.LightLUT[ (uSrc&0x03E0)     | g5] <<  5) |
	       (gpu_unai.LightLUT[((uSrc&0x001F)<<5) | r5]      ) |
	       (uSrc & 0x8000);
}


////////////////////////////////////////////////////////////////////////////////
// Apply fast (low-precision) 5-bit Gouraud lighting to bgr555 texture color:
//
// INPUT:
//  'gCol' is a Gouraud fixed-pt 8.8 rgb triplet
//        'gCol' input:  ccccccccXXXXXXXX for c in [r, g, b]
//                       ^ bit 16
//        'uSrc' input:  -bbbbbgggggrrrrr
//                       ^ bit 16
// RETURNS:
//          u16 output:  0bbbbbgggggrrrrr
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE uint_fast16_t gpuLightingTXTGouraud(uint_fast16_t uSrc, gcol_t gCol)
{
	return (gpu_unai.LightLUT[((uSrc&0x7C00)>>5) | (gCol.c.b >> 11)] << 10) |
	       (gpu_unai.LightLUT[ (uSrc&0x03E0)     | (gCol.c.g >> 11)] << 5) |
	       (gpu_unai.LightLUT[((uSrc&0x001F)<<5) | (gCol.c.r >> 11)]) |
	       (uSrc & 0x8000);
}

////////////////////////////////////////////////////////////////////////////////
// Apply high-precision 8-bit lighting to bgr555 texture color,
//  returning a padded u32 5.4:5.4:5.4 bgr fixed-pt triplet
//  suitable for use with HQ 24-bit lighting/quantization.
//
// INPUT:
//        'r8','g8','b8' are unsigned 8-bit color component values, value of
//          127 is midpoint that doesn't modify that component of texture
//
//         uSrc input: -bbbbbgggggrrrrr
//                     ^ bit 16
// RETURNS:
//         u32 output: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE u32 gpuLightingTXT24(uint_fast16_t uSrc, u8 r8, u8 g8, u8 b8)
{
	uint_fast16_t r1 = uSrc&0x001F;
	uint_fast16_t g1 = uSrc&0x03E0;
	uint_fast16_t b1 = uSrc&0x7C00;

	uint_fast16_t r2 = r8;
	uint_fast16_t g2 = g8;
	uint_fast16_t b2 = b8;

	u32 r3 = r1 * r2; if (r3 & 0xFFFFF000) r3 = ~0xFFFFF000;
	u32 g3 = g1 * g2; if (g3 & 0xFFFE0000) g3 = ~0xFFFE0000;
	u32 b3 = b1 * b2; if (b3 & 0xFFC00000) b3 = ~0xFFC00000;

	return ((r3>> 3)    ) |
	       ((g3>> 8)<<10) |
	       ((b3>>13)<<20);
}


////////////////////////////////////////////////////////////////////////////////
// Apply high-precision 8-bit lighting to bgr555 texture color in 'uSrc',
//  returning a padded u32 5.4:5.4:5.4 bgr fixed-pt triplet
//  suitable for use with HQ 24-bit lighting/quantization.
//
// INPUT:
//       'uSrc' input: -bbbbbgggggrrrrr
//                     ^ bit 16
//       'gCol' input: ccccccccXXXXXXXX for c in [r, g, b]
//                     ^ bit 16
// RETURNS:
//         u32 output: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE u32 gpuLightingTXT24Gouraud(uint_fast16_t uSrc, gcol_t gCol)
{
	uint_fast16_t r1 = uSrc&0x001F;
	uint_fast16_t g1 = uSrc&0x03E0;
	uint_fast16_t b1 = uSrc&0x7C00;

	uint_fast16_t r2 = gCol.c.r >> 8;
	uint_fast16_t g2 = gCol.c.g >> 8;
	uint_fast16_t b2 = gCol.c.b >> 8;

	u32 r3 = r1 * r2; if (r3 & 0xFFFFF000) r3 = ~0xFFFFF000;
	u32 g3 = g1 * g2; if (g3 & 0xFFFE0000) g3 = ~0xFFFE0000;
	u32 b3 = b1 * b2; if (b3 & 0xFFC00000) b3 = ~0xFFC00000;

	return ((r3>> 3)    ) |
	       ((g3>> 8)<<10) |
	       ((b3>>13)<<20);
}

#endif  //_OP_LIGHT_H_
