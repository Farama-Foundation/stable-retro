/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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

#ifndef __R3000A_H__
#define __R3000A_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"

enum R3000Aexception {
	R3000E_Int = 0,      // Interrupt
	R3000E_AdEL = 4,     // Address error (on load/I-fetch)
	R3000E_AdES = 5,     // Address error (on store)
	R3000E_IBE = 6,      // Bus error (instruction fetch)
	R3000E_DBE = 7,      // Bus error (data load/store)
	R3000E_Syscall = 8,  // syscall instruction
	R3000E_Bp = 9,       // Breakpoint - a break instruction
	R3000E_RI = 10,      // reserved instruction
	R3000E_CpU = 11,     // Co-Processor unusable
	R3000E_Ov = 12       // arithmetic overflow
};

enum R3000Anote {
	R3000ACPU_NOTIFY_CACHE_ISOLATED = 0,
	R3000ACPU_NOTIFY_CACHE_UNISOLATED = 1,
	R3000ACPU_NOTIFY_BEFORE_SAVE,  // data arg - hle if non-null
	R3000ACPU_NOTIFY_AFTER_LOAD,
};

enum blockExecCaller {
	EXEC_CALLER_BOOT,
	EXEC_CALLER_HLE,
	EXEC_CALLER_OTHER,
};

struct psxRegisters;

typedef struct {
	int  (*Init)();
	void (*Reset)();
	void (*Execute)(struct psxRegisters *regs);
	/* executes up to a jump */
	void (*ExecuteBlock)(struct psxRegisters *regs, enum blockExecCaller caller);
	void (*Clear)(u32 Addr, u32 Size);
	void (*Notify)(enum R3000Anote note, void *data);
	void (*ApplyConfig)();
	void (*Shutdown)();
} R3000Acpu;

extern R3000Acpu *psxCpu;
extern R3000Acpu psxInt;
extern R3000Acpu psxRec;

typedef union {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	struct { u8 h3, h2, h, l; } b;
	struct { s8 h3, h2, h, l; } sb;
	struct { u16 h, l; } w;
	struct { s16 h, l; } sw;
#else
	struct { u8 l, h, h2, h3; } b;
	struct { u16 l, h; } w;
	struct { s8 l, h, h2, h3; } sb;
	struct { s16 l, h; } sw;
#endif
} PAIR;

typedef union {
	struct {
		u32   r0, at, v0, v1, a0, a1, a2, a3,
						t0, t1, t2, t3, t4, t5, t6, t7,
						s0, s1, s2, s3, s4, s5, s6, s7,
						t8, t9, k0, k1, gp, sp, fp, ra, lo, hi;
	} n;
	u32 r[34]; /* Lo, Hi in r[32] and r[33] */
	PAIR p[34];
} psxGPRRegs;

typedef union psxCP0Regs_ {
	struct {
		u32 Reserved0, Reserved1, Reserved2,  BPC,
		    Reserved4, BDA,       Target,     DCIC,
		    BadVAddr,  BDAM,      Reserved10, BPCM,
		    SR,        Cause,     EPC,        PRid,
		    Reserved16[16];
	} n;
	u32 r[32];
	PAIR p[32];
} psxCP0Regs;

typedef struct {
	short x, y;
} SVector2D;

typedef struct {
	short z, pad;
} SVector2Dz;

typedef struct {
	short x, y, z, pad;
} SVector3D;

typedef struct {
	short x, y, z, pad;
} LVector3D;

typedef struct {
	unsigned char r, g, b, c;
} CBGR;

typedef struct {
	short m11, m12, m13, m21, m22, m23, m31, m32, m33, pad;
} SMatrix3D;

typedef union {
	struct {
		SVector3D     v0, v1, v2;
		CBGR          rgb;
		s32          otz;
		s32          ir0, ir1, ir2, ir3;
		SVector2D     sxy0, sxy1, sxy2, sxyp;
		SVector2Dz    sz0, sz1, sz2, sz3;
		CBGR          rgb0, rgb1, rgb2;
		s32          reserved;
		s32          mac0, mac1, mac2, mac3;
		u32 irgb, orgb;
		s32          lzcs, lzcr;
	} n;
	u32 r[32];
	PAIR p[32];
} psxCP2Data;

typedef union {
	struct {
		SMatrix3D rMatrix;
		s32      trX, trY, trZ;
		SMatrix3D lMatrix;
		s32      rbk, gbk, bbk;
		SMatrix3D cMatrix;
		s32      rfc, gfc, bfc;
		s32      ofx, ofy;
		s32      h;
		s32      dqa, dqb;
		s32      zsf3, zsf4;
		s32      flag;
	} n;
	u32 r[32];
	PAIR p[32];
} psxCP2Ctrl;

enum R3000Abdt {
	// corresponds to bits 31,30 of Cause reg
	R3000A_BRANCH_TAKEN = 3,
	R3000A_BRANCH_NOT_TAKEN = 2,
	// none or tells that there was an exception in DS back to doBranch
	R3000A_BRANCH_NONE_OR_EXCEPTION = 0,
};

typedef struct psxCP2Regs {
	psxCP2Data CP2D; 	/* Cop2 data registers */
	psxCP2Ctrl CP2C; 	/* Cop2 control registers */
} psxCP2Regs;

typedef struct psxRegisters {
	// note: some cores like lightrec don't keep their data here,
	// so use R3000ACPU_NOTIFY_BEFORE_SAVE to sync
	psxGPRRegs GPR;		/* General Purpose Registers */
	psxCP0Regs CP0;		/* Coprocessor0 Registers */
	union {
		struct {
			psxCP2Data CP2D; 	/* Cop2 data registers */
			psxCP2Ctrl CP2C; 	/* Cop2 control registers */
		};
		psxCP2Regs CP2;
	};
	u32 pc;				/* Program counter */
	u32 code;			/* The instruction */
	u32 cycle;
	u32 interrupt;
	struct { u32 sCycle, cycle; } intCycle[20];
	u32 event_cycles[20];
	u32 psxNextCounter;
	u32 psxNextsCounter;
	u32 next_interupt;  /* cycle */
	u32 unused;
	u32 gteBusyCycle;
	u32 muldivBusyCycle;
	u32 subCycle;       /* interpreter cycle counting */
	u32 subCycleStep;
	u32 biuReg;
	u8  stop;
	u8  branchSeen;     /* interp. */
	u8  branching;      /* interp. R3000A_BRANCH_TAKEN / not, 0 if not branch */
	u8  dloadSel;       /* interp. delay load state */
	u8  dloadReg[2];
	u8  unused2[2];
	u32 dloadVal[2];
	u32 biosBranchCheck;
	u32 cpuInRecursion;
	u32 gpuIdleAfter;
	u32 unused3[2];
	// warning: changing anything in psxRegisters requires update of all
	// asm in libpcsxcore/new_dynarec/ and may break savestates
} psxRegisters;

extern psxRegisters psxRegs;

/* new_dynarec stuff */
void ndrc_freeze(void *f, int mode);
void ndrc_clear_full(void);

int  psxInit();
void psxReset();
void psxShutdown();
void psxException(u32 code, enum R3000Abdt bdt, psxCP0Regs *cp0);
void psxBranchTest();
void psxExecuteBios();
int  psxExecuteBiosEnded(void);
void psxJumpTest();

void irq10Interrupt();
void psxScheduleIrq10(int irq_count, int x_cycles, int y);

#ifdef __cplusplus
}
#endif
#endif
