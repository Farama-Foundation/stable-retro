/***************************************************************************
 *   PCSX-Revolution - PlayStation Emulator for Nintendo Wii               *
 *   Copyright (C) 2009-2010  PCSX-Revolution Dev Team                     *
 *   <http://code.google.com/p/pcsx-revolution/>                           *
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

/*
* GTE functions.
*/

#include "gte.h"
#include "psxmem.h"
#include "../include/compiler_features.h"

#define VX(n) (n < 3 ? regs->CP2D.p[n << 1].sw.l : regs->CP2D.p[9].sw.l)
#define VY(n) (n < 3 ? regs->CP2D.p[n << 1].sw.h : regs->CP2D.p[10].sw.l)
#define VZ(n) (n < 3 ? regs->CP2D.p[(n << 1) + 1].sw.l : regs->CP2D.p[11].sw.l)
#define MX11(n) (n < 3 ? regs->CP2C.p[(n << 3)].sw.l : 0)
#define MX12(n) (n < 3 ? regs->CP2C.p[(n << 3)].sw.h : 0)
#define MX13(n) (n < 3 ? regs->CP2C.p[(n << 3) + 1].sw.l : 0)
#define MX21(n) (n < 3 ? regs->CP2C.p[(n << 3) + 1].sw.h : 0)
#define MX22(n) (n < 3 ? regs->CP2C.p[(n << 3) + 2].sw.l : 0)
#define MX23(n) (n < 3 ? regs->CP2C.p[(n << 3) + 2].sw.h : 0)
#define MX31(n) (n < 3 ? regs->CP2C.p[(n << 3) + 3].sw.l : 0)
#define MX32(n) (n < 3 ? regs->CP2C.p[(n << 3) + 3].sw.h : 0)
#define MX33(n) (n < 3 ? regs->CP2C.p[(n << 3) + 4].sw.l : 0)
#define CV1(n) (n < 3 ? (s32)regs->CP2C.r[(n << 3) + 5] : 0)
#define CV2(n) (n < 3 ? (s32)regs->CP2C.r[(n << 3) + 6] : 0)
#define CV3(n) (n < 3 ? (s32)regs->CP2C.r[(n << 3) + 7] : 0)

#define fSX(n) ((regs->CP2D.p)[((n) + 12)].sw.l)
#define fSY(n) ((regs->CP2D.p)[((n) + 12)].sw.h)
#define fSZ(n) ((regs->CP2D.p)[((n) + 17)].w.l) /* (n == 0) => SZ1; */

#define gteVXY0 (regs->CP2D.r[0])
#define gteVX0  (regs->CP2D.p[0].sw.l)
#define gteVY0  (regs->CP2D.p[0].sw.h)
#define gteVZ0  (regs->CP2D.p[1].sw.l)
#define gteVXY1 (regs->CP2D.r[2])
#define gteVX1  (regs->CP2D.p[2].sw.l)
#define gteVY1  (regs->CP2D.p[2].sw.h)
#define gteVZ1  (regs->CP2D.p[3].sw.l)
#define gteVXY2 (regs->CP2D.r[4])
#define gteVX2  (regs->CP2D.p[4].sw.l)
#define gteVY2  (regs->CP2D.p[4].sw.h)
#define gteVZ2  (regs->CP2D.p[5].sw.l)
#define gteRGB  (regs->CP2D.r[6])
#define gteR    (regs->CP2D.p[6].b.l)
#define gteG    (regs->CP2D.p[6].b.h)
#define gteB    (regs->CP2D.p[6].b.h2)
#define gteCODE (regs->CP2D.p[6].b.h3)
#define gteOTZ  (regs->CP2D.p[7].w.l)
#define gteIR0  (regs->CP2D.p[8].sw.l)
#define gteIR1  (regs->CP2D.p[9].sw.l)
#define gteIR2  (regs->CP2D.p[10].sw.l)
#define gteIR3  (regs->CP2D.p[11].sw.l)
#define gteSXY0 (regs->CP2D.r[12])
#define gteSX0  (regs->CP2D.p[12].sw.l)
#define gteSY0  (regs->CP2D.p[12].sw.h)
#define gteSXY1 (regs->CP2D.r[13])
#define gteSX1  (regs->CP2D.p[13].sw.l)
#define gteSY1  (regs->CP2D.p[13].sw.h)
#define gteSXY2 (regs->CP2D.r[14])
#define gteSX2  (regs->CP2D.p[14].sw.l)
#define gteSY2  (regs->CP2D.p[14].sw.h)
#define gteSXYP (regs->CP2D.r[15])
#define gteSXP  (regs->CP2D.p[15].sw.l)
#define gteSYP  (regs->CP2D.p[15].sw.h)
#define gteSZ0  (regs->CP2D.p[16].w.l)
#define gteSZ1  (regs->CP2D.p[17].w.l)
#define gteSZ2  (regs->CP2D.p[18].w.l)
#define gteSZ3  (regs->CP2D.p[19].w.l)
#define gteRGB0  (regs->CP2D.r[20])
#define gteR0    (regs->CP2D.p[20].b.l)
#define gteG0    (regs->CP2D.p[20].b.h)
#define gteB0    (regs->CP2D.p[20].b.h2)
#define gteCODE0 (regs->CP2D.p[20].b.h3)
#define gteRGB1  (regs->CP2D.r[21])
#define gteR1    (regs->CP2D.p[21].b.l)
#define gteG1    (regs->CP2D.p[21].b.h)
#define gteB1    (regs->CP2D.p[21].b.h2)
#define gteCODE1 (regs->CP2D.p[21].b.h3)
#define gteRGB2  (regs->CP2D.r[22])
#define gteR2    (regs->CP2D.p[22].b.l)
#define gteG2    (regs->CP2D.p[22].b.h)
#define gteB2    (regs->CP2D.p[22].b.h2)
#define gteCODE2 (regs->CP2D.p[22].b.h3)
#define gteRES1  (regs->CP2D.r[23])
#define gteMAC0  (((s32 *)regs->CP2D.r)[24])
#define gteMAC1  (((s32 *)regs->CP2D.r)[25])
#define gteMAC2  (((s32 *)regs->CP2D.r)[26])
#define gteMAC3  (((s32 *)regs->CP2D.r)[27])
#define gteIRGB  (regs->CP2D.r[28])
#define gteORGB  (regs->CP2D.r[29])
#define gteLZCS  (regs->CP2D.r[30])
#define gteLZCR  (regs->CP2D.r[31])

#define gteR11R12 (((s32 *)regs->CP2C.r)[0])
#define gteR22R23 (((s32 *)regs->CP2C.r)[2])
#define gteR11 (regs->CP2C.p[0].sw.l)
#define gteR12 (regs->CP2C.p[0].sw.h)
#define gteR13 (regs->CP2C.p[1].sw.l)
#define gteR21 (regs->CP2C.p[1].sw.h)
#define gteR22 (regs->CP2C.p[2].sw.l)
#define gteR23 (regs->CP2C.p[2].sw.h)
#define gteR31 (regs->CP2C.p[3].sw.l)
#define gteR32 (regs->CP2C.p[3].sw.h)
#define gteR33 (regs->CP2C.p[4].sw.l)
#define gteTRX (((s32 *)regs->CP2C.r)[5])
#define gteTRY (((s32 *)regs->CP2C.r)[6])
#define gteTRZ (((s32 *)regs->CP2C.r)[7])
#define gteL11 (regs->CP2C.p[8].sw.l)
#define gteL12 (regs->CP2C.p[8].sw.h)
#define gteL13 (regs->CP2C.p[9].sw.l)
#define gteL21 (regs->CP2C.p[9].sw.h)
#define gteL22 (regs->CP2C.p[10].sw.l)
#define gteL23 (regs->CP2C.p[10].sw.h)
#define gteL31 (regs->CP2C.p[11].sw.l)
#define gteL32 (regs->CP2C.p[11].sw.h)
#define gteL33 (regs->CP2C.p[12].sw.l)
#define gteRBK (((s32 *)regs->CP2C.r)[13])
#define gteGBK (((s32 *)regs->CP2C.r)[14])
#define gteBBK (((s32 *)regs->CP2C.r)[15])
#define gteLR1 (regs->CP2C.p[16].sw.l)
#define gteLR2 (regs->CP2C.p[16].sw.h)
#define gteLR3 (regs->CP2C.p[17].sw.l)
#define gteLG1 (regs->CP2C.p[17].sw.h)
#define gteLG2 (regs->CP2C.p[18].sw.l)
#define gteLG3 (regs->CP2C.p[18].sw.h)
#define gteLB1 (regs->CP2C.p[19].sw.l)
#define gteLB2 (regs->CP2C.p[19].sw.h)
#define gteLB3 (regs->CP2C.p[20].sw.l)
#define gteRFC (((s32 *)regs->CP2C.r)[21])
#define gteGFC (((s32 *)regs->CP2C.r)[22])
#define gteBFC (((s32 *)regs->CP2C.r)[23])
#define gteOFX (((s32 *)regs->CP2C.r)[24])
#define gteOFY (((s32 *)regs->CP2C.r)[25])
// senquack - gteH register is u16, not s16, and used in GTE that way.
//  HOWEVER when read back by CPU using CFC2, it will be incorrectly
//  sign-extended by bug in original hardware, according to Nocash docs
//  GTE section 'Screen Offset and Distance'. The emulator does this
//  sign extension when it is loaded to GTE by CTC2.
//#define gteH   (regs->CP2C.p[26].sw.l)
#define gteH   (regs->CP2C.p[26].w.l)
#define gteDQA (regs->CP2C.p[27].sw.l)
#define gteDQB (((s32 *)regs->CP2C.r)[28])
#define gteZSF3 (regs->CP2C.p[29].sw.l)
#define gteZSF4 (regs->CP2C.p[30].sw.l)
#define gteFLAG (regs->CP2C.r[31])

#define GTE_OP(op) ((op >> 20) & 31)
#define GTE_SF(op) ((op >> 19) & 1)
#define GTE_MX(op) ((op >> 17) & 3)
#define GTE_V(op) ((op >> 15) & 3)
#define GTE_CV(op) ((op >> 13) & 3)
#define GTE_CD(op) ((op >> 11) & 3) /* not used */
#define GTE_LM(op) ((op >> 10) & 1)
#define GTE_CT(op) ((op >> 6) & 15) /* not used */
#define GTE_FUNCT(op) (op & 63)

#define gteop (psxRegs.code & 0x1ffffff)

#ifndef FLAGLESS

static inline s64 BOUNDS_(psxCP2Regs *regs, s64 n_value, s64 n_max, int n_maxflag, s64 n_min, int n_minflag) {
	if (n_value > n_max) {
		gteFLAG |= n_maxflag;
	} else if (n_value < n_min) {
		gteFLAG |= n_minflag;
	}
	return n_value;
}

static inline s32 LIM_(psxCP2Regs *regs, s32 value, s32 max, s32 min, u32 flag) {
	s32 ret = value;
	if (value > max) {
		gteFLAG |= flag;
		ret = max;
	} else if (value < min) {
		gteFLAG |= flag;
		ret = min;
	}
	return ret;
}

static inline u32 limE_(psxCP2Regs *regs, u32 result) {
	if (result > 0x1ffff) {
		gteFLAG |= (1u << 31) | (1u << 17);
		return 0x1ffff;
	}
	return result;
}

#else

#define BOUNDS_(regs, a, ...) (a)

static inline s32 LIM_(psxCP2Regs *regs, s32 value, s32 max, s32 min, u32 flag_unused) {
	s32 ret = value;
	if (value > max)
		ret = max;
	else if (value < min)
		ret = min;
	return ret;
}

static inline u32 limE_(psxCP2Regs *regs, u32 result) {
	if (result > 0x1ffff)
		return 0x1ffff;
	return result;
}

#endif

#define BOUNDS(n_value,n_max,n_maxflag,n_min,n_minflag) \
	BOUNDS_(regs,n_value,n_max,n_maxflag,n_min,n_minflag)
#define LIM(value,max,min,flag) \
	LIM_(regs,value,max,min,flag)
#define limE(result) \
	limE_(regs,result)

#define A1(a) BOUNDS((a), 0x7fffffff, (1u << 30), -(s64)0x80000000, (1u << 31) | (1u << 27))
#define A2(a) BOUNDS((a), 0x7fffffff, (1u << 29), -(s64)0x80000000, (1u << 31) | (1u << 26))
#define A3(a) BOUNDS((a), 0x7fffffff, (1u << 28), -(s64)0x80000000, (1u << 31) | (1u << 25))
#define limB1(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1u << 31) | (1u << 24))
#define limB2(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1u << 31) | (1u << 23))
#define limB3(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1u << 22))
#define limC1(a) LIM((a), 0x00ff, 0x0000, (1u << 21))
#define limC2(a) LIM((a), 0x00ff, 0x0000, (1u << 20))
#define limC3(a) LIM((a), 0x00ff, 0x0000, (1u << 19))
#define limD(a) LIM((a), 0xffff, 0x0000, (1u << 31) | (1u << 18))

#define F(a) BOUNDS((a), 0x7fffffff, (1u << 31) | (1u << 16), -(s64)0x80000000, (1u << 31) | (1u << 15))
#define limG1(a) LIM((a), 0x3ff, -0x400, (1u << 31) | (1u << 14))
#define limG2(a) LIM((a), 0x3ff, -0x400, (1u << 31) | (1u << 13))
#define limH(a) LIM((a), 0x1000, 0x0000, (1u << 12))

#ifndef __arm__
#define A1U A1
#define A2U A2
#define A3U A3
#else
/* these are unlikely to be hit and usually waste cycles, don't want them on ARM */
#define A1U(x) (x)
#define A2U(x) (x)
#define A3U(x) (x)
#endif


//senquack - n param should be unsigned (will be 'gteH' reg which is u16)
#ifdef GTE_USE_NATIVE_DIVIDE
INLINE u32 DIVIDE(u16 n, u16 d) {
	if (n < d * 2) {
		return ((u32)n << 16) / d;
	}
	return 0xffffffff;
}
#else
#include "gte_divider.h"
#endif // GTE_USE_NATIVE_DIVIDE

#ifndef FLAGLESS

const unsigned char gte_cycletab[64] = {
	/*   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f */
	 0, 15,  0,  0,  0,  0,  8,  0,  0,  0,  0,  0,  6,  0,  0,  0,
	 8,  8,  8, 19, 13,  0, 44,  0,  0,  0,  0, 17, 11,  0, 14,  0,
	30,  0,  0,  0,  0,  0,  0,  0,  5,  8, 17,  0,  0,  5,  6,  0,
	23,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  5, 39,
};

// warning: ari64 drc stores it's negative cycles in gteBusyCycle
static int gteCheckStallRaw(u32 op_cycles, psxRegisters *regs) {
	u32 left = regs->gteBusyCycle - regs->cycle;
	int stall = 0;

	if (left <= 44) {
		//printf("c %2u stall %2u %u\n", op_cycles, left, regs->cycle);
		regs->cycle = regs->gteBusyCycle;
		stall = left;
	}
	regs->gteBusyCycle = regs->cycle + op_cycles;
	return stall;
}

void gteCheckStall(u32 op) {
	gteCheckStallRaw(gte_cycletab[op], &psxRegs);
}

u32 MFC2(struct psxCP2Regs *regs, int reg) {
	switch (reg) {
		case 1:
		case 3:
		case 5:
		case 8:
		case 9:
		case 10:
		case 11:
			regs->CP2D.r[reg] = (s32)regs->CP2D.p[reg].sw.l;
			break;

		case 7:
		case 16:
		case 17:
		case 18:
		case 19:
			regs->CP2D.r[reg] = (u32)regs->CP2D.p[reg].w.l;
			break;

		case 15:
			regs->CP2D.r[reg] = gteSXY2;
			break;

		case 28:
		case 29:
			regs->CP2D.r[reg] = LIM(gteIR1 >> 7, 0x1f, 0, 0) |
									(LIM(gteIR2 >> 7, 0x1f, 0, 0) << 5) |
									(LIM(gteIR3 >> 7, 0x1f, 0, 0) << 10);
			break;
	}
	return regs->CP2D.r[reg];
}

static u32 lzc(s32 val)
{
#if __has_builtin(__builtin_clrsb)
	return 1 + __builtin_clrsb(val);
#else
	val ^= val >> 31;
	return val ? __builtin_clz(val) : 32;
#endif
}

void MTC2(struct psxCP2Regs *regs, u32 value, int reg) {
	switch (reg) {
		case 15:
			gteSXY0 = gteSXY1;
			gteSXY1 = gteSXY2;
			gteSXY2 = value;
			gteSXYP = value;
			break;

		case 28:
			gteIRGB = value;
			// not gteIR1 etc. just to be consistent with dynarec
			regs->CP2D.n.ir1 = (value & 0x1f) << 7;
			regs->CP2D.n.ir2 = (value & 0x3e0) << 2;
			regs->CP2D.n.ir3 = (value & 0x7c00) >> 3;
			break;

		case 30:
			gteLZCS = value;
			gteLZCR = lzc(value);
			break;

		case 31:
			return;

		default:
			regs->CP2D.r[reg] = value;
	}
}

void CTC2(struct psxCP2Regs *regs, u32 value, int reg) {
	switch (reg) {
		case 4:
		case 12:
		case 20:
		case 26:
		case 27:
		case 29:
		case 30:
			value = (s32)(s16)value;
			break;

		case 31:
			value = value & 0x7ffff000;
			if (value & 0x7f87e000) value |= 0x80000000;
			break;
	}

	regs->CP2C.r[reg] = value;
}

#endif // FLAGLESS

#if 0
#define DIVIDE DIVIDE_
static u32 DIVIDE_(s16 n, u16 d) {
	if (n >= 0 && n < d * 2) {
		s32 n_ = n;
		return ((n_ << 16) + d / 2) / d;
		//return (u32)((float)(n_ << 16) / (float)d + (float)0.5);
	}
	return 0xffffffff;
}
#endif

void gteRTPS(psxCP2Regs *regs) {
	int quotient;
	s64 tmp;

#ifdef GTE_LOG
	GTE_LOG("GTE RTPS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteTRX << 12) + (gteR11 * gteVX0) + (gteR12 * gteVY0) + (gteR13 * gteVZ0)) >> 12);
	gteMAC2 = A2((((s64)gteTRY << 12) + (gteR21 * gteVX0) + (gteR22 * gteVY0) + (gteR23 * gteVZ0)) >> 12);
	gteMAC3 = A3((((s64)gteTRZ << 12) + (gteR31 * gteVX0) + (gteR32 * gteVY0) + (gteR33 * gteVZ0)) >> 12);
	gteIR1 = limB1(gteMAC1, 0);
	gteIR2 = limB2(gteMAC2, 0);
	gteIR3 = limB3(gteMAC3, 0);
	gteSZ0 = gteSZ1;
	gteSZ1 = gteSZ2;
	gteSZ2 = gteSZ3;
	gteSZ3 = limD(gteMAC3);
	quotient = limE(DIVIDE(gteH, gteSZ3));
	gteSXY0 = gteSXY1;
	gteSXY1 = gteSXY2;
	gteSX2 = limG1(F((s64)gteOFX + ((s64)gteIR1 * quotient)) >> 16);
	gteSY2 = limG2(F((s64)gteOFY + ((s64)gteIR2 * quotient)) >> 16);

	tmp = (s64)gteDQB + ((s64)gteDQA * quotient);
	gteMAC0 = F(tmp);
	gteIR0 = limH(tmp >> 12);
}

void gteRTPT(psxCP2Regs *regs) {
	int quotient;
	int v;
	s32 vx, vy, vz;
	s64 tmp;

#ifdef GTE_LOG
	GTE_LOG("GTE RTPT\n");
#endif
	gteFLAG = 0;

	gteSZ0 = gteSZ3;
	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
		gteMAC2 = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
		gteMAC3 = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
		gteIR1 = limB1(gteMAC1, 0);
		gteIR2 = limB2(gteMAC2, 0);
		gteIR3 = limB3(gteMAC3, 0);
		fSZ(v) = limD(gteMAC3);
		quotient = limE(DIVIDE(gteH, fSZ(v)));
		fSX(v) = limG1(F((s64)gteOFX + ((s64)gteIR1 * quotient)) >> 16);
		fSY(v) = limG2(F((s64)gteOFY + ((s64)gteIR2 * quotient)) >> 16);
	}

	tmp = (s64)gteDQB + ((s64)gteDQA * quotient);
	gteMAC0 = F(tmp);
	gteIR0 = limH(tmp >> 12);
}

void gteMVMVA(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);
	int mx = GTE_MX(gteop);
	int v = GTE_V(gteop);
	int cv = GTE_CV(gteop);
	int lm = GTE_LM(gteop);
	s32 vx = VX(v);
	s32 vy = VY(v);
	s32 vz = VZ(v);

#ifdef GTE_LOG
	GTE_LOG("GTE MVMVA\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)CV1(cv) << 12) + (MX11(mx) * vx) + (MX12(mx) * vy) + (MX13(mx) * vz)) >> shift);
	gteMAC2 = A2((((s64)CV2(cv) << 12) + (MX21(mx) * vx) + (MX22(mx) * vy) + (MX23(mx) * vz)) >> shift);
	gteMAC3 = A3((((s64)CV3(cv) << 12) + (MX31(mx) * vx) + (MX32(mx) * vy) + (MX33(mx) * vz)) >> shift);

	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
}

void gteNCLIP(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE NCLIP\n");
#endif
	gteFLAG = 0;

	gteMAC0 = F((s64)gteSX0 * (gteSY1 - gteSY2) +
				gteSX1 * (gteSY2 - gteSY0) +
				gteSX2 * (gteSY0 - gteSY1));
}

void gteAVSZ3(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE AVSZ3\n");
#endif
	gteFLAG = 0;

	gteMAC0 = F((s64)gteZSF3 * (gteSZ1 + gteSZ2 + gteSZ3));
	gteOTZ = limD(gteMAC0 >> 12);
}

void gteAVSZ4(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE AVSZ4\n");
#endif
	gteFLAG = 0;

	gteMAC0 = F((s64)gteZSF4 * (gteSZ0 + gteSZ1 + gteSZ2 + gteSZ3));
	gteOTZ = limD(gteMAC0 >> 12);
}

void gteSQR(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE SQR\n");
#endif
	gteFLAG = 0;

	gteMAC1 = (gteIR1 * gteIR1) >> shift;
	gteMAC2 = (gteIR2 * gteIR2) >> shift;
	gteMAC3 = (gteIR3 * gteIR3) >> shift;
	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
}

void gteNCCS(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE NCCS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = ((s64)(gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12;
	gteMAC2 = ((s64)(gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12;
	gteMAC3 = ((s64)(gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12;
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = ((s32)gteR * gteIR1) >> 8;
	gteMAC2 = ((s32)gteG * gteIR2) >> 8;
	gteMAC3 = ((s32)gteB * gteIR3) >> 8;
	gteIR1 = gteMAC1;
	gteIR2 = gteMAC2;
	gteIR3 = gteMAC3;

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteNCCT(psxCP2Regs *regs) {
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE NCCT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = ((s64)(gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12;
		gteMAC2 = ((s64)(gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12;
		gteMAC3 = ((s64)(gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12;
		gteIR1 = limB1(gteMAC1, 1);
		gteIR2 = limB2(gteMAC2, 1);
		gteIR3 = limB3(gteMAC3, 1);
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
		gteIR1 = limB1(gteMAC1, 1);
		gteIR2 = limB2(gteMAC2, 1);
		gteIR3 = limB3(gteMAC3, 1);
		gteMAC1 = ((s32)gteR * gteIR1) >> 8;
		gteMAC2 = ((s32)gteG * gteIR2) >> 8;
		gteMAC3 = ((s32)gteB * gteIR3) >> 8;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = gteMAC1;
	gteIR2 = gteMAC2;
	gteIR3 = gteMAC3;
}

void gteNCDS(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE NCDS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = ((s64)(gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12;
	gteMAC2 = ((s64)(gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12;
	gteMAC3 = ((s64)(gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12;
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = (((gteR << 4) * gteIR1) + (gteIR0 * limB1(A1U((s64)gteRFC - ((gteR * gteIR1) >> 8)), 0))) >> 12;
	gteMAC2 = (((gteG << 4) * gteIR2) + (gteIR0 * limB2(A2U((s64)gteGFC - ((gteG * gteIR2) >> 8)), 0))) >> 12;
	gteMAC3 = (((gteB << 4) * gteIR3) + (gteIR0 * limB3(A3U((s64)gteBFC - ((gteB * gteIR3) >> 8)), 0))) >> 12;
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteNCDT(psxCP2Regs *regs) {
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE NCDT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = ((s64)(gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12;
		gteMAC2 = ((s64)(gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12;
		gteMAC3 = ((s64)(gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12;
		gteIR1 = limB1(gteMAC1, 1);
		gteIR2 = limB2(gteMAC2, 1);
		gteIR3 = limB3(gteMAC3, 1);
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
		gteIR1 = limB1(gteMAC1, 1);
		gteIR2 = limB2(gteMAC2, 1);
		gteIR3 = limB3(gteMAC3, 1);
		gteMAC1 = (((gteR << 4) * gteIR1) + (gteIR0 * limB1(A1U((s64)gteRFC - ((gteR * gteIR1) >> 8)), 0))) >> 12;
		gteMAC2 = (((gteG << 4) * gteIR2) + (gteIR0 * limB2(A2U((s64)gteGFC - ((gteG * gteIR2) >> 8)), 0))) >> 12;
		gteMAC3 = (((gteB << 4) * gteIR3) + (gteIR0 * limB3(A3U((s64)gteBFC - ((gteB * gteIR3) >> 8)), 0))) >> 12;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
}

void gteOP(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE OP\n");
#endif
	gteFLAG = 0;

	gteMAC1 = ((gteR22 * gteIR3) - (gteR33 * gteIR2)) >> shift;
	gteMAC2 = ((gteR33 * gteIR1) - (gteR11 * gteIR3)) >> shift;
	gteMAC3 = ((gteR11 * gteIR2) - (gteR22 * gteIR1)) >> shift;
	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
}

void gteDCPL(psxCP2Regs *regs) {
	int lm = GTE_LM(gteop);

	s32 RIR1 = ((s32)gteR * gteIR1) >> 8;
	s32 GIR2 = ((s32)gteG * gteIR2) >> 8;
	s32 BIR3 = ((s32)gteB * gteIR3) >> 8;

#ifdef GTE_LOG
	GTE_LOG("GTE DCPL\n");
#endif
	gteFLAG = 0;

	gteMAC1 = RIR1 + ((gteIR0 * limB1(A1U((s64)gteRFC - RIR1), 0)) >> 12);
	gteMAC2 = GIR2 + ((gteIR0 * limB1(A2U((s64)gteGFC - GIR2), 0)) >> 12);
	gteMAC3 = BIR3 + ((gteIR0 * limB1(A3U((s64)gteBFC - BIR3), 0)) >> 12);

	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteGPF(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE GPF\n");
#endif
	gteFLAG = 0;

	gteMAC1 = (gteIR0 * gteIR1) >> shift;
	gteMAC2 = (gteIR0 * gteIR2) >> shift;
	gteMAC3 = (gteIR0 * gteIR3) >> shift;
	gteIR1 = limB1(gteMAC1, 0);
	gteIR2 = limB2(gteMAC2, 0);
	gteIR3 = limB3(gteMAC3, 0);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteGPL(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE GPL\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteMAC1 << shift) + (gteIR0 * gteIR1)) >> shift);
	gteMAC2 = A2((((s64)gteMAC2 << shift) + (gteIR0 * gteIR2)) >> shift);
	gteMAC3 = A3((((s64)gteMAC3 << shift) + (gteIR0 * gteIR3)) >> shift);
	gteIR1 = limB1(gteMAC1, 0);
	gteIR2 = limB2(gteMAC2, 0);
	gteIR3 = limB3(gteMAC3, 0);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteDPCS(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE DPCS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = ((gteR << 16) + (gteIR0 * limB1(A1U(((s64)gteRFC - (gteR << 4)) << (12 - shift)), 0))) >> 12;
	gteMAC2 = ((gteG << 16) + (gteIR0 * limB2(A2U(((s64)gteGFC - (gteG << 4)) << (12 - shift)), 0))) >> 12;
	gteMAC3 = ((gteB << 16) + (gteIR0 * limB3(A3U(((s64)gteBFC - (gteB << 4)) << (12 - shift)), 0))) >> 12;

	gteIR1 = limB1(gteMAC1, 0);
	gteIR2 = limB2(gteMAC2, 0);
	gteIR3 = limB3(gteMAC3, 0);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteDPCT(psxCP2Regs *regs) {
	int v;

#ifdef GTE_LOG
	GTE_LOG("GTE DPCT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		gteMAC1 = ((gteR0 << 16) + (gteIR0 * limB1(A1U((s64)gteRFC - (gteR0 << 4)), 0))) >> 12;
		gteMAC2 = ((gteG0 << 16) + (gteIR0 * limB1(A2U((s64)gteGFC - (gteG0 << 4)), 0))) >> 12;
		gteMAC3 = ((gteB0 << 16) + (gteIR0 * limB1(A3U((s64)gteBFC - (gteB0 << 4)), 0))) >> 12;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB1(gteMAC1, 0);
	gteIR2 = limB2(gteMAC2, 0);
	gteIR3 = limB3(gteMAC3, 0);
}

void gteNCS(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE NCS\n");
#endif
	gteFLAG = 0;

	gteMAC1 = ((s64)(gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12;
	gteMAC2 = ((s64)(gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12;
	gteMAC3 = ((s64)(gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12;
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteNCT(psxCP2Regs *regs) {
	int v;
	s32 vx, vy, vz;

#ifdef GTE_LOG
	GTE_LOG("GTE NCT\n");
#endif
	gteFLAG = 0;

	for (v = 0; v < 3; v++) {
		vx = VX(v);
		vy = VY(v);
		vz = VZ(v);
		gteMAC1 = ((s64)(gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12;
		gteMAC2 = ((s64)(gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12;
		gteMAC3 = ((s64)(gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12;
		gteIR1 = limB1(gteMAC1, 1);
		gteIR2 = limB2(gteMAC2, 1);
		gteIR3 = limB3(gteMAC3, 1);
		gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
		gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
		gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteCODE2 = gteCODE;
		gteR2 = limC1(gteMAC1 >> 4);
		gteG2 = limC2(gteMAC2 >> 4);
		gteB2 = limC3(gteMAC3 >> 4);
	}
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
}

void gteCC(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE CC\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = ((s32)gteR * gteIR1) >> 8;
	gteMAC2 = ((s32)gteG * gteIR2) >> 8;
	gteMAC3 = ((s32)gteB * gteIR3) >> 8;
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteINTPL(psxCP2Regs *regs) {
	int shift = 12 * GTE_SF(gteop);
	int lm = GTE_LM(gteop);

#ifdef GTE_LOG
	GTE_LOG("GTE INTPL\n");
#endif
	gteFLAG = 0;

	gteMAC1 = ((gteIR1 << 12) + (gteIR0 * limB1(A1U((s64)gteRFC - gteIR1), 0))) >> shift;
	gteMAC2 = ((gteIR2 << 12) + (gteIR0 * limB2(A2U((s64)gteGFC - gteIR2), 0))) >> shift;
	gteMAC3 = ((gteIR3 << 12) + (gteIR0 * limB3(A3U((s64)gteBFC - gteIR3), 0))) >> shift;
	gteIR1 = limB1(gteMAC1, lm);
	gteIR2 = limB2(gteMAC2, lm);
	gteIR3 = limB3(gteMAC3, lm);
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

void gteCDP(psxCP2Regs *regs) {
#ifdef GTE_LOG
	GTE_LOG("GTE CDP\n");
#endif
	gteFLAG = 0;

	gteMAC1 = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
	gteMAC2 = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
	gteMAC3 = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);
	gteMAC1 = (((gteR << 4) * gteIR1) + (gteIR0 * limB1(A1U((s64)gteRFC - ((gteR * gteIR1) >> 8)), 0))) >> 12;
	gteMAC2 = (((gteG << 4) * gteIR2) + (gteIR0 * limB2(A2U((s64)gteGFC - ((gteG * gteIR2) >> 8)), 0))) >> 12;
	gteMAC3 = (((gteB << 4) * gteIR3) + (gteIR0 * limB3(A3U((s64)gteBFC - ((gteB * gteIR3) >> 8)), 0))) >> 12;
	gteIR1 = limB1(gteMAC1, 1);
	gteIR2 = limB2(gteMAC2, 1);
	gteIR3 = limB3(gteMAC3, 1);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

/* decomposed/parametrized versions for the recompiler */

#ifndef FLAGLESS

void gteSQR_part_noshift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = gteIR1 * gteIR1;
	gteMAC2 = gteIR2 * gteIR2;
	gteMAC3 = gteIR3 * gteIR3;
}

void gteSQR_part_shift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = (gteIR1 * gteIR1) >> 12;
	gteMAC2 = (gteIR2 * gteIR2) >> 12;
	gteMAC3 = (gteIR3 * gteIR3) >> 12;
}

void gteOP_part_noshift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = (gteR22 * gteIR3) - (gteR33 * gteIR2);
	gteMAC2 = (gteR33 * gteIR1) - (gteR11 * gteIR3);
	gteMAC3 = (gteR11 * gteIR2) - (gteR22 * gteIR1);
}

void gteOP_part_shift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = ((gteR22 * gteIR3) - (gteR33 * gteIR2)) >> 12;
	gteMAC2 = ((gteR33 * gteIR1) - (gteR11 * gteIR3)) >> 12;
	gteMAC3 = ((gteR11 * gteIR2) - (gteR22 * gteIR1)) >> 12;
}

void gteDCPL_part(psxCP2Regs *regs) {
	s32 RIR1 = ((s32)gteR * gteIR1) >> 8;
	s32 GIR2 = ((s32)gteG * gteIR2) >> 8;
	s32 BIR3 = ((s32)gteB * gteIR3) >> 8;

	gteFLAG = 0;

	gteMAC1 = RIR1 + ((gteIR0 * limB1(A1U((s64)gteRFC - RIR1), 0)) >> 12);
	gteMAC2 = GIR2 + ((gteIR0 * limB1(A2U((s64)gteGFC - GIR2), 0)) >> 12);
	gteMAC3 = BIR3 + ((gteIR0 * limB1(A3U((s64)gteBFC - BIR3), 0)) >> 12);
}

void gteGPF_part_noshift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = gteIR0 * gteIR1;
	gteMAC2 = gteIR0 * gteIR2;
	gteMAC3 = gteIR0 * gteIR3;
}

void gteGPF_part_shift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = (gteIR0 * gteIR1) >> 12;
	gteMAC2 = (gteIR0 * gteIR2) >> 12;
	gteMAC3 = (gteIR0 * gteIR3) >> 12;
}

#endif // !FLAGLESS

void gteGPL_part_noshift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = A1((s64)gteMAC1 + (gteIR0 * gteIR1));
	gteMAC2 = A2((s64)gteMAC2 + (gteIR0 * gteIR2));
	gteMAC3 = A3((s64)gteMAC3 + (gteIR0 * gteIR3));
}

void gteGPL_part_shift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = A1((s64)gteMAC1 + ((gteIR0 * gteIR1) >> 12));
	gteMAC2 = A2((s64)gteMAC2 + ((gteIR0 * gteIR2) >> 12));
	gteMAC3 = A3((s64)gteMAC3 + ((gteIR0 * gteIR3) >> 12));
}

void gteDPCS_part_noshift(psxCP2Regs *regs) {
	int shift = 0;

	gteFLAG = 0;

	gteMAC1 = ((gteR << 16) + (gteIR0 * limB1(A1U((s64)gteRFC - (gteR << 4)) << (12 - shift), 0))) >> 12;
	gteMAC2 = ((gteG << 16) + (gteIR0 * limB2(A2U((s64)gteGFC - (gteG << 4)) << (12 - shift), 0))) >> 12;
	gteMAC3 = ((gteB << 16) + (gteIR0 * limB3(A3U((s64)gteBFC - (gteB << 4)) << (12 - shift), 0))) >> 12;
}

void gteDPCS_part_shift(psxCP2Regs *regs) {
	int shift = 12;

	gteFLAG = 0;

	gteMAC1 = ((gteR << 16) + (gteIR0 * limB1(A1U((s64)gteRFC - (gteR << 4)) << (12 - shift), 0))) >> 12;
	gteMAC2 = ((gteG << 16) + (gteIR0 * limB2(A2U((s64)gteGFC - (gteG << 4)) << (12 - shift), 0))) >> 12;
	gteMAC3 = ((gteB << 16) + (gteIR0 * limB3(A3U((s64)gteBFC - (gteB << 4)) << (12 - shift), 0))) >> 12;
}

void gteINTPL_part_noshift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = ((gteIR1 << 12) + (gteIR0 * limB1(A1U((s64)gteRFC - gteIR1), 0)));
	gteMAC2 = ((gteIR2 << 12) + (gteIR0 * limB2(A2U((s64)gteGFC - gteIR2), 0)));
	gteMAC3 = ((gteIR3 << 12) + (gteIR0 * limB3(A3U((s64)gteBFC - gteIR3), 0)));
}

void gteINTPL_part_shift(psxCP2Regs *regs) {
	gteFLAG = 0;

	gteMAC1 = ((gteIR1 << 12) + (gteIR0 * limB1(A1U((s64)gteRFC - gteIR1), 0))) >> 12;
	gteMAC2 = ((gteIR2 << 12) + (gteIR0 * limB2(A2U((s64)gteGFC - gteIR2), 0))) >> 12;
	gteMAC3 = ((gteIR3 << 12) + (gteIR0 * limB3(A3U((s64)gteBFC - gteIR3), 0))) >> 12;
}

void gteMACtoRGB(psxCP2Regs *regs) {
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteCODE2 = gteCODE;
	gteR2 = limC1(gteMAC1 >> 4);
	gteG2 = limC2(gteMAC2 >> 4);
	gteB2 = limC3(gteMAC3 >> 4);
}

