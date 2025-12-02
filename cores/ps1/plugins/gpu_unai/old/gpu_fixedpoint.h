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

#include "arm_features.h"

typedef s32 fixed;

#ifdef GPU_TABLE_10_BITS
#define TABLE_BITS 10
#else
#define TABLE_BITS 16
#endif

#define FIXED_BITS 16

#define fixed_ZERO ((fixed)0)
#define fixed_ONE  ((fixed)1<<FIXED_BITS)
#define fixed_TWO  ((fixed)2<<FIXED_BITS)
#define fixed_HALF ((fixed)((1<<FIXED_BITS)>>1))

INLINE  fixed i2x(const int   _x) { return  ((_x)<<FIXED_BITS); }
INLINE  fixed x2i(const fixed _x) { return  ((_x)>>FIXED_BITS); }

/*
INLINE u32 Log2(u32 _a)
{
  u32 c = 0; // result of log2(v) will go here
  if (_a & 0xFFFF0000) { _a >>= 16; c |= 16;  }
  if (_a & 0xFF00) { _a >>= 8; c |= 8;  }
  if (_a & 0xF0) { _a >>= 4; c |= 4;  }
  if (_a & 0xC) { _a >>= 2; c |= 2;  }
  if (_a & 0x2) { _a >>= 1; c |= 1;  }
  return c;
}
*/

#ifdef GPU_UNAI_USE_FLOATMATH

#define inv_type float

INLINE  void  xInv (const fixed _b, float & factor_, float & shift_)
{
	factor_ = 1.0f / _b;
	shift_ = 0.0f; // not used
}

INLINE  fixed xInvMulx  (const fixed _a, const float fact, const float shift)
{
	return (fixed)((_a << FIXED_BITS) * fact);
}

INLINE  fixed xLoDivx   (const fixed _a, const fixed _b)
{
	return (fixed)((_a << FIXED_BITS) / (float)_b);
}

#else

#define inv_type s32

#ifdef HAVE_ARMV5
INLINE u32 Log2(u32 x) { u32 res; asm("clz %0,%1" : "=r" (res) : "r" (x)); return 32-res; }
#else
INLINE u32 Log2(u32 x) { u32 i = 0; for ( ; x > 0; ++i, x >>= 1); return i - 1; }
#endif

//  big precision inverse table.
extern s32 s_invTable[(1<<TABLE_BITS)];

#ifdef GPU_TABLE_10_BITS
INLINE  void  xInv (const fixed _b, s32& iFactor_, s32& iShift_)
{
    u32 uD   = (_b<0) ? -_b : _b ;
    u32 uLog = Log2(uD);
    uLog = uLog>(TABLE_BITS-1) ? uLog-(TABLE_BITS-1) : 0;
    u32 uDen = uD>>uLog;
    iFactor_ = s_invTable[uDen];
    iFactor_ = (_b<0) ? -iFactor_ :iFactor_;
    iShift_  = 15+uLog;
}
#else
INLINE  void  xInv (const fixed _b, s32& iFactor_, s32& iShift_)
{
  u32 uD = (_b<0) ? -_b : _b;
  if(uD>1)
  {
	u32 uLog = Log2(uD);
    uLog = uLog>(TABLE_BITS-1) ? uLog-(TABLE_BITS-1) : 0;
    u32 uDen = (uD>>uLog)-1;
    iFactor_ = s_invTable[uDen];
    iFactor_ = (_b<0) ? -iFactor_ :iFactor_;
    iShift_  = 15+uLog;
  }
  else
  {
    iFactor_=_b;
    iShift_ = 0;
  }
}
#endif

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

#endif // GPU_UNAI_USE_FLOATMATH

///////////////////////////////////////////////////////////////////////////
template<typename T>
INLINE  T Min2 (const T _a, const T _b)             { return (_a<_b)?_a:_b; }

template<typename T>
INLINE  T Min3 (const T _a, const T _b, const T _c) { return  Min2(Min2(_a,_b),_c); }

///////////////////////////////////////////////////////////////////////////
template<typename T>
INLINE  T Max2 (const T _a, const T _b)             { return  (_a>_b)?_a:_b; }

template<typename T>
INLINE  T Max3 (const T _a, const T _b, const T _c) { return  Max2(Max2(_a,_b),_c); }

///////////////////////////////////////////////////////////////////////////
#endif  //FIXED_H
