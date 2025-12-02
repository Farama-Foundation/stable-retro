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

#ifndef FIXED_H
#define FIXED_H

typedef s32 fixed;

//senquack - The gpu_drhell poly routines I adapted use 22.10 fixed point,
//           while original Unai used 16.16: (see README_senquack.txt)
//#define FIXED_BITS 16
#define FIXED_BITS 10

#define fixed_ZERO ((fixed)0)
#define fixed_ONE  ((fixed)1<<FIXED_BITS)
#define fixed_TWO  ((fixed)2<<FIXED_BITS)
#define fixed_HALF ((fixed)((1<<FIXED_BITS)>>1))

#define fixed_LOMASK ((fixed)((1<<FIXED_BITS)-1))
#define fixed_HIMASK ((fixed)(~fixed_LOMASK))

// int<->fixed conversions:
#define i2x(x) ((x)<<FIXED_BITS)
#define x2i(x) ((x)>>FIXED_BITS)

INLINE fixed FixedCeil(const fixed x)
{
	return (x + (fixed_ONE - 1)) & fixed_HIMASK;
}

INLINE s32 FixedCeilToInt(const fixed x)
{
	return (x + (fixed_ONE - 1)) >> FIXED_BITS;
}

//senquack - float<->fixed conversions:
#define f2x(x) ((s32)((x) * (float)(1<<FIXED_BITS)))
#define x2f(x) ((float)(x) / (float)(1<<FIXED_BITS))

//senquack - floating point reciprocal:
//NOTE: These assume x is always != 0 !!!
#ifdef GPU_UNAI_USE_FLOATMATH
#if defined(_MIPS_ARCH_MIPS32R2) || (__mips == 64)
INLINE float FloatInv(const float x)
{
	float res;
	asm("recip.s %0,%1" : "=f" (res) : "f" (x));
	return res;
}
#else
INLINE float FloatInv(const float x)
{
	return (1.0f / x);
}
#endif
#endif

///////////////////////////////////////////////////////////////////////////
// --- BEGIN INVERSE APPROXIMATION SECTION ---
///////////////////////////////////////////////////////////////////////////
#if defined(GPU_UNAI_USE_INT_DIV_MULTINV) || (!defined(GPU_UNAI_NO_OLD) && !defined(GPU_UNAI_USE_FLOATMATH))

//  big precision inverse table.
#define TABLE_BITS 16
s32 s_invTable[(1<<TABLE_BITS)];
#endif

#ifdef GPU_UNAI_USE_INT_DIV_MULTINV
//senquack - MIPS32 happens to have same instruction/format:
#if defined(__arm__) || (__mips == 32)
INLINE u32 Log2(u32 x) { u32 res; asm("clz %0,%1" : "=r" (res) : "r" (x)); return 32-res; }
#else
INLINE u32 Log2(u32 x) { u32 i = 0; for ( ; x > 0; ++i, x >>= 1); return i - 1; }
#endif

INLINE  void  xInv (const fixed _b, s32& iFactor_, s32& iShift_)
{
  u32 uD = (_b<0) ? -_b : _b;
  if(uD>1)
  {
	u32 uLog = Log2(uD);
    uLog = uLog>(TABLE_BITS-1) ? uLog-(TABLE_BITS-1) : 0;
    u32 uDen = (uD>>uLog);
    iFactor_ = s_invTable[uDen];
    iFactor_ = (_b<0) ? -iFactor_ :iFactor_;
    //senquack - Adapted to 22.10 fixed point (originally 16.16):
    //iShift_  = 15+uLog;
    iShift_  = 21+uLog;
  }
  else
  {
    iFactor_=_b;
    iShift_ = 0;
  }
}

INLINE  fixed xInvMulx  (const fixed _a, const s32 _iFact, const s32 _iShift)
{
	#ifdef __arm__
		s64 res;
		asm ("smull %Q0, %R0, %1, %2" : "=&r" (res) : "r"(_a) , "r"(_iFact));
		return fixed(res>>_iShift);
	#else
		return fixed( ((s64)(_a)*(s64)(_iFact))>>(_iShift) );
	#endif
}

INLINE  fixed xLoDivx   (const fixed _a, const fixed _b)
{
  s32 iFact, iShift;
  xInv(_b, iFact, iShift);
  return xInvMulx(_a, iFact, iShift);
}
#endif // GPU_UNAI_USE_INT_DIV_MULTINV
///////////////////////////////////////////////////////////////////////////
// --- END INVERSE APPROXIMATION SECTION ---
///////////////////////////////////////////////////////////////////////////

#endif  //FIXED_H
