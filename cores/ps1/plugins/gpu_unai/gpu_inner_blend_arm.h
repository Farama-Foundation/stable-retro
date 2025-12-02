#ifndef _OP_BLEND_ARM_H_
#define _OP_BLEND_ARM_H_

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
GPU_INLINE uint_fast16_t gpuBlendingARM(uint_fast16_t uSrc, uint_fast16_t uDst)
{
	// These use Blargg's bitwise modulo-clamping:
	//  http://blargg.8bitalley.com/info/rgb_mixing.html
	//  http://blargg.8bitalley.com/info/rgb_clamped_add.html
	//  http://blargg.8bitalley.com/info/rgb_clamped_sub.html

	uint_fast16_t mix;

	// Clear preserved msb
	asm ("bic %[uDst], %[uDst], #0x8000" : [uDst] "+r" (uDst));

	if (BLENDMODE == 3) {
		// Prepare uSrc for blending ((0.25 * uSrc) & (0.25 * mask))
		asm ("and %[uSrc], %[mask], %[uSrc], lsr #0x2" : [uSrc] "+r" (uSrc) : [mask] "r" (0x1ce7));
	} else if (!SKIP_USRC_MSB_MASK) {
		asm ("bic %[uSrc], %[uSrc], #0x8000" : [uSrc] "+r" (uSrc));
	}


	// 0.5 x Back + 0.5 x Forward
	if (BLENDMODE==0) {
		// mix = ((uSrc + uDst) - ((uSrc ^ uDst) & 0x0421)) >> 1;
		asm ("eor %[mix], %[uSrc], %[uDst]\n\t" // uSrc ^ uDst
		     "and %[mix], %[mix], %[mask]\n\t"  // ... & 0x0421
		     "sub %[mix], %[uDst], %[mix]\n\t"  // uDst - ...
		#ifdef HAVE_ARMV6
		     "uhadd16 %[mix], %[uSrc], %[mix]\n\t"
		#else
		     "add %[mix], %[uSrc], %[mix]\n\t"  // uSrc + ...
		     "mov %[mix], %[mix], lsr #0x1\n\t" // ... >> 1
		#endif
		     : [mix] "=&r" (mix)
		     : [uSrc] "r" (uSrc), [uDst] "r" (uDst), [mask] "r" (0x0420)); // 421
	}

	if (BLENDMODE == 1 || BLENDMODE == 3) {
		// u32 sum      = uSrc + uDst;
		// u32 low_bits = (uSrc ^ uDst) & 0x0421;
		// u32 carries  = (sum - low_bits) & 0x8420;
		// u32 modulo   = sum - carries;
		// u32 clamp    = carries - (carries >> 5);
		// mix = modulo | clamp;

		u32 sum;

		asm ("add %[sum], %[uSrc], %[uDst]\n\t" // sum = uSrc + uDst
		     "eor %[mix], %[uSrc], %[uDst]\n\t" // uSrc ^ uDst
		     "and %[mix], %[mix], %[mask]\n\t"  // low_bits = (... & 0x0421)
		     "sub %[mix], %[sum], %[mix]\n\t"   // sum - low_bits
		     "and %[mix], %[mix], %[mask], lsl #0x05\n\t"  // carries = ... & 0x8420
		     "sub %[sum], %[sum], %[mix] \n\t"  // modulo = sum - carries
		     "sub %[mix], %[mix], %[mix], lsr #0x05\n\t" // clamp = carries - (carries >> 5)
		     "orr %[mix], %[sum], %[mix]"       // mix = modulo | clamp
		     : [sum] "=&r" (sum), [mix] "=&r" (mix)
		     : [uSrc] "r" (uSrc), [uDst] "r" (uDst), [mask] "r" (0x0421));
	}
    
	// 1.0 x Back - 1.0 x Forward
	if (BLENDMODE==2) {
		u32 diff;
		// u32 diff     = uDst - uSrc + 0x8420;
		// u32 low_bits = (uDst ^ uSrc) & 0x8420;
		// u32 borrows  = (diff - low_bits) & 0x8420;
		// u32 modulo   = diff - borrows;
		// u32 clamp    = borrows - (borrows >> 5);
		// mix = modulo & clamp;
		asm ("sub %[diff], %[uDst], %[uSrc]\n\t"  // uDst - uSrc
		     "add %[diff], %[diff], %[mask]\n\t"  // diff = ... + 0x8420
		     "eor %[mix], %[uDst], %[uSrc]\n\t"   // uDst ^ uSrc
		     "and %[mix], %[mix], %[mask]\n\t"    // low_bits = ... & 0x8420
		     "sub %[mix], %[diff], %[mix]\n\t"    // diff - low_bits
		     "and %[mix], %[mix], %[mask]\n\t"    // borrows = ... & 0x8420
		     "sub %[diff], %[diff], %[mix]\n\t"   // modulo = diff - borrows
		     "sub %[mix], %[mix], %[mix], lsr #0x05\n\t"  // clamp = borrows - (borrows >> 5)
		     "and %[mix], %[diff], %[mix]"        // mix = modulo & clamp
		     : [diff] "=&r" (diff), [mix] "=&r" (mix)
		     : [uSrc] "r" (uSrc), [uDst] "r" (uDst), [mask] "r" (0x8420));
	}

	// There's not a case where we can get into this function,
	// SKIP_USRC_MSB_MASK is false, and the msb of uSrc is unset.
	if (!SKIP_USRC_MSB_MASK) {
		asm ("orr %[mix], %[mix], #0x8000" : [mix] "+r" (mix));
	}
  
	return mix;
}

#endif  //_OP_BLEND_ARM_H_
