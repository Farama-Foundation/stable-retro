#ifndef _OP_LIGHT_ARM_H_
#define _OP_LIGHT_ARM_H_

#include "arm_features.h"

////////////////////////////////////////////////////////////////////////////////
// Extract bgr555 color from Gouraud u32 fixed-pt 8.3:8.3:8.2 rgb triplet
//
// INPUT:
//  'gCol' input:  rrrrrrrrXXXggggggggXXXbbbbbbbbXX
//                 ^ bit 31
// RETURNS:
//    u16 output:  0bbbbbgggggrrrrr
//                 ^ bit 16
// Where 'r,g,b' are integer bits of colors, 'X' fixed-pt, and '0' zero
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE uint_fast16_t gpuLightingRGBARM(u32 gCol)
{
	uint_fast16_t out = 0x03E0; // don't need the mask after starting to write output
  	u32 tmp;
  
	asm ("and %[tmp], %[gCol], %[out]\n\t"              // tmp holds 0x000000bbbbb00000
	     "and %[out], %[out],  %[gCol], lsr #0x0B\n\t"  // out holds 0x000000ggggg00000
	     "orr %[tmp], %[out],  %[tmp],  lsl #0x05\n\t"  // tmp holds 0x0bbbbbggggg00000
	     "orr %[out], %[tmp],  %[gCol], lsr #0x1B\n\t"  // out holds 0x0bbbbbgggggrrrrr
	     : [out] "+&r" (out), [tmp] "=&r" (tmp)
	     : [gCol] "r"  (gCol)
	     );

	return out;
}

////////////////////////////////////////////////////////////////////////////////
// Apply fast (low-precision) 5-bit lighting to bgr555 texture color:
//
// INPUT:
//	  'r5','g5','b5' are unsigned 5-bit color values, value of 15
//	    is midpoint that doesn't modify that component of texture
//	  'uSrc' input:	 mbbbbbgggggrrrrr
//			 ^ bit 16
// RETURNS:
//	    u16 output:	 mbbbbbgggggrrrrr
// Where 'X' are fixed-pt bits.
////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_ARMV6
// clang uses smulbb but not gcc, so we need this
GPU_INLINE int_fast16_t smulbb(int_fast16_t a, int_fast16_t b)
{
	int_fast16_t r;
	asm("smulbb %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
	return r;
}

GPU_INLINE uint_fast16_t gpuLightingTXTARM(uint_fast16_t uSrc, u8 r5, u8 g5, u8 b5)
{
	// on v6 we have single-cycle mul and sat which is better than the lut
	int_fast16_t r = smulbb(uSrc & 0x001f, r5);
	int_fast16_t g = smulbb(uSrc & 0x03e0, g5);
	int_fast16_t b = smulbb(uSrc & 0x7c00, b5);
	asm volatile("usat %0, #5, %0, asr #4"  : "=r"(r) : "0"(r));
	asm volatile("usat %0, #5, %0, asr #9"  : "=r"(g) : "0"(g));
	asm volatile("usat %0, #5, %0, asr #14" : "=r"(b) : "0"(b));
	return (uSrc & 0x8000) | (b << 10) | (g << 5) | r;
}
#else
GPU_INLINE uint_fast16_t gpuLightingTXTARM(uint_fast16_t uSrc, u8 r5, u8 g5, u8 b5)
{
	uint_fast16_t out = 0x03E0;
	u32 db, dg;

	// Using `g` for src, `G` for dest
	asm ("and    %[dg],  %[out],    %[src]  \n\t"             // dg holds 0x000000ggggg00000
	     "orr    %[dg],  %[dg],     %[g5]   \n\t"             // dg holds 0x000000gggggGGGGG
	     "and    %[db],  %[out],    %[src], lsr #0x05 \n\t"   // db holds 0x000000bbbbb00000
	     "ldrb   %[dg],  [%[lut],   %[dg]]  \n\t"             // dg holds result 0x00000000000ggggg
	     "and    %[out], %[out],    %[src], lsl #0x05 \n\t"   // out holds 0x000000rrrrr00000
	     "orr    %[out], %[out],    %[r5]   \n\t"             // out holds 0x000000rrrrrRRRRR
	     "orr    %[db],  %[db],     %[b5]   \n\t"             // db holds 0x000000bbbbbBBBBB
	     "ldrb   %[out], [%[lut],   %[out]] \n\t"             // out holds result 0x00000000000rrrrr
	     "ldrb   %[db],  [%[lut],   %[db]]  \n\t"             // db holds result 0x00000000000bbbbb
	     "tst    %[src], #0x8000\n\t"                         // check whether msb was set on uSrc
	     "orr    %[out], %[out],    %[dg],  lsl #0x05   \n\t" // out holds 0x000000gggggrrrrr
	     "orrne  %[out], %[out],    #0x8000\n\t"              // add msb to out if set on uSrc
	     "orr    %[out], %[out],    %[db],  lsl #0x0A   \n\t" // out holds 0xmbbbbbgggggrrrrr
	     : [out] "=&r" (out), [db] "=&r" (db), [dg] "=&r" (dg)
	     : [r5] "r" (r5), [g5] "r" (g5),  [b5] "r" (b5),
	       [lut] "r" (gpu_unai.LightLUT), [src] "r" (uSrc), "0" (out)
	     : "cc");
	return out;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Apply fast (low-precision) 5-bit Gouraud lighting to bgr555 texture color:
//
// INPUT:
//  'gCol' is a packed Gouraud u32 fixed-pt 8.3:8.3:8.2 rgb triplet, value of
//     15.0 is midpoint that does not modify color of texture
//	   gCol input :	 rrrrrXXXXXXgggggXXXXXXbbbbbXXXXX
//			 ^ bit 31
//	  'uSrc' input:	 mbbbbbgggggrrrrr
//			 ^ bit 16
// RETURNS:
//	    u16 output:	 mbbbbbgggggrrrrr
// Where 'X' are fixed-pt bits, '0' is zero-padding, and '-' is don't care
////////////////////////////////////////////////////////////////////////////////
GPU_INLINE uint_fast16_t gpuLightingTXTGouraudARM(uint_fast16_t uSrc, u32 gCol)
{
	uint_fast16_t out = 0x03E0; // don't need the mask after starting to write output
	u32 db,dg,gtmp;

	// Using `g` for src, `G` for dest
	asm ("and    %[dg],  %[out],  %[src]   \n\t"           // dg holds 0x000000ggggg00000
	     "and    %[gtmp],%[out],  %[gCol], lsr #0x0B \n\t" // gtmp holds 0x000000GGGGG00000
	     "and    %[db],  %[out],  %[src],  lsr #0x05 \n\t" // db holds 0x000000bbbbb00000
	     "orr    %[dg],  %[dg],   %[gtmp], lsr #0x05 \n\t" // dg holds 0x000000gggggGGGGG
	     "and    %[gtmp],%[out],  %[gCol]  \n\t"           // gtmp holds 0x000000BBBBB00000
	     "ldrb   %[dg],  [%[lut], %[dg]]   \n\t"           // dg holds result 0x00000000000ggggg
	     "and    %[out], %[out],  %[src],  lsl #0x05 \n\t" // out holds 0x000000rrrrr00000
	     "orr    %[out], %[out],  %[gCol], lsr #0x1B \n\t" // out holds 0x000000rrrrrRRRRR
	     "orr    %[db],  %[db],   %[gtmp], lsr #0x05 \n\t" // db holds 0x000000bbbbbBBBBB
	     "ldrb   %[out], [%[lut], %[out]]  \n\t"           // out holds result 0x00000000000rrrrr
	     "ldrb   %[db],  [%[lut], %[db]]   \n\t"           // db holds result 0x00000000000bbbbb
	     "tst    %[src], #0x8000\n\t"                      // check whether msb was set on uSrc
	     "orr    %[out], %[out],  %[dg],   lsl #0x05 \n\t" // out holds 0x000000gggggrrrrr
	     "orrne  %[out], %[out],  #0x8000\n\t"             // add msb to out if set on uSrc
	     "orr    %[out], %[out],  %[db],   lsl #0x0A \n\t" // out holds 0xmbbbbbgggggrrrrr
	     : [out] "=&r" (out), [db] "=&r" (db), [dg] "=&r" (dg),
	       [gtmp] "=&r" (gtmp) \
	     : [gCol] "r" (gCol), [lut] "r" (gpu_unai.LightLUT), "0" (out), [src] "r" (uSrc)
	     : "cc");

	return out;
}

#endif  //_OP_LIGHT_ARM_H_
