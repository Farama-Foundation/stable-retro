/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
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

#ifndef _OP_BLEND_H_
#define _OP_BLEND_H_

//  GPU Blending operations functions

////////////////////////////////////////////////////////////////////////////////
// Blend bgr555 color in 'uSrc' (foreground) with bgr555 color
//  in 'uDst' (background), returning resulting color.
//
// INPUT:
//  'uSrc','uDst' input: -bbbbbgggggrrrrr
//                       ^ bit 16
// OUTPUT:
//           u16 output: 0bbbbbgggggrrrrr
//                       ^ bit 16
// RETURNS:
// Where '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
template <int BLENDMODE, bool SKIP_USRC_MSB_MASK>
GPU_INLINE uint_fast16_t gpuBlendingGeneric(uint_fast16_t uSrc, uint_fast16_t uDst)
{
	// These use Blargg's bitwise modulo-clamping:
	//  http://blargg.8bitalley.com/info/rgb_mixing.html
	//  http://blargg.8bitalley.com/info/rgb_clamped_add.html
	//  http://blargg.8bitalley.com/info/rgb_clamped_sub.html

	uint_fast16_t mix;

	// 0.5 x Back + 0.5 x Forward
	if (BLENDMODE==0) {
#ifdef GPU_UNAI_USE_ACCURATE_BLENDING
		// Slower, but more accurate (doesn't lose LSB data)
		uDst &= 0x7fff;
		if (!SKIP_USRC_MSB_MASK)
			uSrc &= 0x7fff;
		mix = ((uSrc + uDst) - ((uSrc ^ uDst) & 0x0421)) >> 1;
#else
		mix = ((uDst & 0x7bde) + (uSrc & 0x7bde)) >> 1;
#endif
	}

	// 1.0 x Back + 1.0 x Forward
	if (BLENDMODE==1) {
		uDst &= 0x7fff;
		if (!SKIP_USRC_MSB_MASK)
			uSrc &= 0x7fff;
		u32 sum      = uSrc + uDst;
		u32 low_bits = (uSrc ^ uDst) & 0x0421;
		u32 carries  = (sum - low_bits) & 0x8420;
		u32 modulo   = sum - carries;
		u32 clamp    = carries - (carries >> 5);
		mix = modulo | clamp;
	}

	// 1.0 x Back - 1.0 x Forward
	if (BLENDMODE==2) {
		uDst &= 0x7fff;
		if (!SKIP_USRC_MSB_MASK)
			uSrc &= 0x7fff;
		u32 diff     = uDst - uSrc + 0x8420;
		u32 low_bits = (uDst ^ uSrc) & 0x8420;
		u32 borrows  = (diff - low_bits) & 0x8420;
		u32 modulo   = diff - borrows;
		u32 clamp    = borrows - (borrows >> 5);
		mix = modulo & clamp;
	}

	// 1.0 x Back + 0.25 x Forward
	if (BLENDMODE==3) {
		uDst &= 0x7fff;
		uSrc = ((uSrc >> 2) & 0x1ce7);
		u32 sum      = uSrc + uDst;
		u32 low_bits = (uSrc ^ uDst) & 0x0421;
		u32 carries  = (sum - low_bits) & 0x8420;
		u32 modulo   = sum - carries;
		u32 clamp    = carries - (carries >> 5);
		mix = modulo | clamp;
	}

	return mix;
}


////////////////////////////////////////////////////////////////////////////////
// Convert bgr555 color in uSrc to padded u32 5.4:5.4:5.4 bgr fixed-pt
//  color triplet suitable for use with HQ 24-bit quantization.
//
// INPUT:
//       'uDst' input: -bbbbbgggggrrrrr
//                     ^ bit 16
// RETURNS:
//         u32 output: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE u32 gpuGetRGB24(uint_fast16_t uSrc)
{
	return ((uSrc & 0x7C00)<<14)
	     | ((uSrc & 0x03E0)<< 9)
	     | ((uSrc & 0x001F)<< 4);
}


////////////////////////////////////////////////////////////////////////////////
// Blend padded u32 5.4:5.4:5.4 bgr fixed-pt color triplet in 'uSrc24'
//  (foreground color) with bgr555 color in 'uDst' (background color),
//  returning the resulting u32 5.4:5.4:5.4 color.
//
// INPUT:
//     'uSrc24' input: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
//       'uDst' input: -bbbbbgggggrrrrr
//                     ^ bit 16
// RETURNS:
//         u32 output: 000bbbbbXXXX0gggggXXXX0rrrrrXXXX
//                     ^ bit 31
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
template <int BLENDMODE>
GPU_INLINE u32 gpuBlending24(u32 uSrc24, uint_fast16_t uDst)
{
	// These use techniques adapted from Blargg's techniques mentioned in
	//  in gpuBlending() comments above. Not as much bitwise trickery is
	//  necessary because of presence of 0 padding in uSrc24 format.

	u32 uDst24 = gpuGetRGB24(uDst);
	u32 mix;

	// 0.5 x Back + 0.5 x Forward
	if (BLENDMODE==0) {
		const u32 uMsk = 0x1FE7F9FE;
		// Only need to mask LSBs of uSrc24, uDst24's LSBs are 0 already
		mix = (uDst24 + (uSrc24 & uMsk)) >> 1;
	}

	// 1.0 x Back + 1.0 x Forward
	if (BLENDMODE==1) {
		u32 sum     = uSrc24 + uDst24;
		u32 carries = sum & 0x20080200;
		u32 modulo  = sum - carries;
		u32 clamp   = carries - (carries >> 9);
		mix = modulo | clamp;
	}

	// 1.0 x Back - 1.0 x Forward
	if (BLENDMODE==2) {
		// Insert ones in 0-padded borrow slot of color to be subtracted from
		uDst24 |= 0x20080200;
		u32 diff    = uDst24 - uSrc24;
		u32 borrows = diff & 0x20080200;
		u32 clamp   = borrows - (borrows >> 9);
		mix = diff & clamp;
	}

	// 1.0 x Back + 0.25 x Forward
	if (BLENDMODE==3) {
		uSrc24 = (uSrc24 & 0x1FC7F1FC) >> 2;
		u32 sum     = uSrc24 + uDst24;
		u32 carries = sum & 0x20080200;
		u32 modulo  = sum - carries;
		u32 clamp   = carries - (carries >> 9);
		mix = modulo | clamp;
	}

	return mix;
}

#endif  //_OP_BLEND_H_
