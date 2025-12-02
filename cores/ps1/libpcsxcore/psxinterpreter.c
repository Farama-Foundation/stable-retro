/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   Copyright (C) 2023 notaz                                              *
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
 * PSX assembly interpreter.
 */

#include "psxcommon.h"
#include "r3000a.h"
#include "gte.h"
#include "psxhle.h"
#include "psxinterpreter.h"
#include <stddef.h>
#include <assert.h>
#include "../include/compiler_features.h"

// these may cause issues: because of poor timing we may step
// on instructions that real hardware would never reach
#define DO_EXCEPTION_RESERVEDI
#define HANDLE_LOAD_DELAY

#ifdef __i386__
#define INT_ATTR __attribute__((regparm(2)))
#else
#define INT_ATTR
#endif
#ifndef INVALID_PTR
#define INVALID_PTR NULL
#endif

// Subsets
static void (INT_ATTR *psxBSC[64])(psxRegisters *regs_, u32 code);
static void (INT_ATTR *psxSPC[64])(psxRegisters *regs_, u32 code);

// load delay
static void doLoad(psxRegisters *regs, u32 r, u32 val)
{
#ifdef HANDLE_LOAD_DELAY
	int sel = regs->dloadSel ^ 1;
	assert(regs->dloadReg[sel] == 0);
	regs->dloadReg[sel] = r;
	regs->dloadVal[sel] = r ? val : 0;
	if (regs->dloadReg[sel ^ 1] == r)
		regs->dloadVal[sel ^ 1] = regs->dloadReg[sel ^ 1] = 0;
#else
	regs->GPR.r[r] = r ? val : 0;
#endif
}

static void dloadRt(psxRegisters *regs, u32 r, u32 val)
{
#ifdef HANDLE_LOAD_DELAY
	int sel = regs->dloadSel;
	if (unlikely(regs->dloadReg[sel] == r))
		regs->dloadVal[sel] = regs->dloadReg[sel] = 0;
#endif
	regs->GPR.r[r] = r ? val : 0;
}

static void dloadStep(psxRegisters *regs)
{
#ifdef HANDLE_LOAD_DELAY
	int sel = regs->dloadSel;
	regs->GPR.r[regs->dloadReg[sel]] = regs->dloadVal[sel];
	regs->dloadVal[sel] = regs->dloadReg[sel] = 0;
	regs->dloadSel ^= 1;
	assert(regs->GPR.r[0] == 0);
#endif
}

static void dloadFlush(psxRegisters *regs)
{
#ifdef HANDLE_LOAD_DELAY
	regs->GPR.r[regs->dloadReg[0]] = regs->dloadVal[0];
	regs->GPR.r[regs->dloadReg[1]] = regs->dloadVal[1];
	regs->dloadVal[0] = regs->dloadVal[1] = 0;
	regs->dloadReg[0] = regs->dloadReg[1] = 0;
	assert(regs->GPR.r[0] == 0);
#endif
}

static void dloadClear(psxRegisters *regs)
{
#ifdef HANDLE_LOAD_DELAY
	regs->dloadVal[0] = regs->dloadVal[1] = 0;
	regs->dloadReg[0] = regs->dloadReg[1] = 0;
	regs->dloadSel = 0;
#endif
}

static void intException(psxRegisters *regs, u32 pc, u32 cause)
{
	if (cause != 0x20) {
		//FILE *f = fopen("/tmp/psx_ram.bin", "wb");
		//fwrite(psxM, 1, 0x200000, f); fclose(f);
		log_unhandled("exception %08x @%08x ra=%08x\n",
			cause, pc, regs->GPR.n.ra);
	}
	dloadFlush(regs);
	regs->pc = pc;
	psxException(cause, regs->branching, &regs->CP0);
	regs->branching = R3000A_BRANCH_NONE_OR_EXCEPTION;
}

// exception caused by current instruction (excluding unkasking)
static void intExceptionInsn(psxRegisters *regs, u32 cause)
{
	cause |= (regs->code & 0x0c000000) << 2;
	intException(regs, regs->pc - 4, cause);
}

static noinline void intExceptionReservedInsn(psxRegisters *regs)
{
#ifdef DO_EXCEPTION_RESERVEDI
	static u32 ppc_ = ~0u;
	if (regs->pc != ppc_) {
		SysPrintf("reserved instruction %08x @%08x ra=%08x\n",
			regs->code, regs->pc - 4, regs->GPR.n.ra);
		ppc_ = regs->pc;
	}
	intExceptionInsn(regs, R3000E_RI << 2);
#endif
}

// 29  Enable for 80000000-ffffffff
// 30  Enable for 00000000-7fffffff
// 31  Enable exception
#define DBR_ABIT(dc, a)    ((dc) & (1u << (29+(((a)>>31)^1))))
#define DBR_EN_EXEC(dc, a) (((dc) & 0x01800000) == 0x01800000 && DBR_ABIT(dc, a))
#define DBR_EN_LD(dc, a)   (((dc) & 0x06800000) == 0x06800000 && DBR_ABIT(dc, a))
#define DBR_EN_ST(dc, a)   (((dc) & 0x0a800000) == 0x0a800000 && DBR_ABIT(dc, a))
static void intExceptionDebugBp(psxRegisters *regs, u32 pc)
{
	psxCP0Regs *cp0 = &regs->CP0;
	dloadFlush(regs);
	cp0->n.Cause &= 0x300;
	cp0->n.Cause |= (regs->branching << 30) | (R3000E_Bp << 2);
	cp0->n.SR = (cp0->n.SR & ~0x3f) | ((cp0->n.SR & 0x0f) << 2);
	cp0->n.EPC = regs->branching ? pc - 4 : pc;
	regs->pc = 0x80000040;
}

static int execBreakCheck(psxRegisters *regs, u32 pc)
{
	if (unlikely(DBR_EN_EXEC(regs->CP0.n.DCIC, pc) &&
	    ((pc ^ regs->CP0.n.BPC) & regs->CP0.n.BPCM) == 0))
	{
		regs->CP0.n.DCIC |= 0x03;
		if (regs->CP0.n.DCIC & (1u << 31)) {
			intExceptionDebugBp(regs, pc);
			return 1;
		}
	}
	return 0;
}

// get an opcode without triggering exceptions or affecting cache
u32 intFakeFetch(u32 pc)
{
	u32 *code = (u32 *)psxm(pc & ~0x3, 0);
	if (unlikely(code == INVALID_PTR))
		return 0; // nop
	return SWAP32(*code);

}

static u32 INT_ATTR fetchNoCache(psxRegisters *regs, u8 **memRLUT, u32 pc)
{
	u32 *code = (u32 *)psxm_lut(pc & ~0x3, 0, memRLUT);
	if (unlikely(code == INVALID_PTR)) {
		SysPrintf("game crash @%08x, ra=%08x\n", pc, regs->GPR.n.ra);
		intException(regs, pc, R3000E_IBE << 2);
		return 0; // execute as nop
	}
	return SWAP32(*code);
}

/*
Formula One 2001 :
Use old CPU cache code when the RAM location is updated with new code (affects in-game racing)
*/
static struct cache_entry {
	u32 tag;
	u32 data[4];
} ICache[256];

static u32 INT_ATTR fetchICache(psxRegisters *regs, u8 **memRLUT, u32 pc)
{
	// cached?
	if (pc < 0xa0000000)
	{
		// this is not how the hardware works but whatever
		struct cache_entry *entry = &ICache[(pc & 0xff0) >> 4];

		if (((entry->tag ^ pc) & 0xfffffff0) != 0 || pc < entry->tag)
		{
			const u32 *code = (u32 *)psxm_lut(pc & ~0xf, 0, memRLUT);
			if (unlikely(code == INVALID_PTR)) {
				SysPrintf("game crash @%08x, ra=%08x\n", pc, regs->GPR.n.ra);
				intException(regs, pc, R3000E_IBE << 2);
				return 0; // execute as nop
			}

			entry->tag = pc;
			// treat as 4 words, although other configurations are said to be possible
			switch (pc & 0x0c)
			{
				case 0x00: entry->data[0] = SWAP32(code[0]);
				case 0x04: entry->data[1] = SWAP32(code[1]);
				case 0x08: entry->data[2] = SWAP32(code[2]);
				case 0x0c: entry->data[3] = SWAP32(code[3]);
			}
		}
		return entry->data[(pc & 0x0f) >> 2];
	}

	return fetchNoCache(regs, memRLUT, pc);
}

static u32 (INT_ATTR *fetch)(psxRegisters *regs_, u8 **memRLUT, u32 pc) = fetchNoCache;

// Make the timing events trigger faster as we are currently assuming everything
// takes one cycle, which is not the case on real hardware.
// FIXME: count cache misses, memory latencies, stalls to get rid of this
static inline void addCycle(psxRegisters *regs)
{
	assert(regs->subCycleStep >= 0x10000);
	regs->subCycle += regs->subCycleStep;
	regs->cycle += regs->subCycle >> 16;
	regs->subCycle &= 0xffff;
}

/**** R3000A Instruction Macros ****/
#define _PC_            regs_->pc       // The next PC to be executed

#define _fOp_(code)     ((code >> 26)       )  // The opcode part of the instruction register
#define _fFunct_(code)  ((code      ) & 0x3F)  // The funct part of the instruction register
#define _fRd_(code)     ((code >> 11) & 0x1F)  // The rd part of the instruction register
#define _fRt_(code)     ((code >> 16) & 0x1F)  // The rt part of the instruction register
#define _fRs_(code)     ((code >> 21) & 0x1F)  // The rs part of the instruction register
#define _fSa_(code)     ((code >>  6) & 0x1F)  // The sa part of the instruction register
#define _fIm_(code)     ((u16)code)            // The immediate part of the instruction register
#define _fTarget_(code) (code & 0x03ffffff)    // The target part of the instruction register

#define _fImm_(code)    ((s16)code)            // sign-extended immediate
#define _fImmU_(code)   (code&0xffff)          // zero-extended immediate

#define _Op_     _fOp_(code)
#define _Funct_  _fFunct_(code)
#define _Rd_     _fRd_(code)
#define _Rt_     _fRt_(code)
#define _Rs_     _fRs_(code)
#define _Sa_     _fSa_(code)
#define _Im_     _fIm_(code)
#define _Target_ _fTarget_(code)

#define _Imm_    _fImm_(code)
#define _ImmU_   _fImmU_(code)

#define _rRs_   regs_->GPR.r[_Rs_]   // Rs register
#define _rRt_   regs_->GPR.r[_Rt_]   // Rt register
#define _rSa_   regs_->GPR.r[_Sa_]   // Sa register

#define _rHi_   regs_->GPR.n.hi   // The HI register
#define _rLo_   regs_->GPR.n.lo   // The LO register

#define _JumpTarget_    ((_Target_ * 4) + (_PC_ & 0xf0000000))   // Calculates the target during a jump instruction
#define _BranchTarget_  ((s16)_Im_ * 4 + _PC_)                 // Calculates the target during a branch instruction

#define _SetLink(x)     dloadRt(regs_, x, _PC_ + 4);       // Sets the return address in the link register

#define OP(name) \
	static inline INT_ATTR void name(psxRegisters *regs_, u32 code)

// this defines shall be used with the tmp 
// of the next func (instead of _Funct_...)
#define _tFunct_  ((tmp      ) & 0x3F)  // The funct part of the instruction register 
#define _tRd_     ((tmp >> 11) & 0x1F)  // The rd part of the instruction register 
#define _tRt_     ((tmp >> 16) & 0x1F)  // The rt part of the instruction register 
#define _tRs_     ((tmp >> 21) & 0x1F)  // The rs part of the instruction register 
#define _tSa_     ((tmp >>  6) & 0x1F)  // The sa part of the instruction register

#define _i32(x) (s32)(x)
#define _u32(x) (u32)(x)

#define isBranch(c_) \
	((1 <= ((c_) >> 26) && ((c_) >> 26) <= 7) || ((c_) & 0xfc00003e) == 8)
#define swap_(a_, b_) { u32 t_ = a_; a_ = b_; b_ = t_; }

// tar1 is main branch target, 'code' is opcode in DS
static u32 psxBranchNoDelay(psxRegisters *regs_, u32 tar1, u32 code, int *taken) {
	u32 temp, rt;

	assert(isBranch(code));
	*taken = 1;
	switch (code >> 26) {
		case 0x00: // SPECIAL
			switch (_Funct_) {
				case 0x08: // JR
					return _u32(_rRs_);
				case 0x09: // JALR
					temp = _u32(_rRs_);
					if (_Rd_)
						regs_->GPR.r[_Rd_] = tar1 + 4;
					return temp;
			}
			break;
		case 0x01: // REGIMM
			rt = _Rt_;
			switch (rt) {
				case 0x10: // BLTZAL
					regs_->GPR.n.ra = tar1 + 4;
					if (_i32(_rRs_) < 0)
						return tar1 + (s16)_Im_ * 4;
					break;
				case 0x11: // BGEZAL
					regs_->GPR.n.ra = tar1 + 4;
					if (_i32(_rRs_) >= 0)
						return tar1 + (s16)_Im_ * 4;
					break;
				default:
					if (rt & 1) { // BGEZ
						if (_i32(_rRs_) >= 0)
							return tar1 + (s16)_Im_ * 4;
					}
					else {        // BLTZ
						if (_i32(_rRs_) < 0)
							return tar1 + (s16)_Im_ * 4;
					}
					break;
			}
			break;
		case 0x02: // J
			return (tar1 & 0xf0000000u) + _Target_ * 4;
		case 0x03: // JAL
			regs_->GPR.n.ra = tar1 + 4;
			return (tar1 & 0xf0000000u) + _Target_ * 4;
		case 0x04: // BEQ
			if (_i32(_rRs_) == _i32(_rRt_))
				return tar1 + (s16)_Im_ * 4;
			break;
		case 0x05: // BNE
			if (_i32(_rRs_) != _i32(_rRt_))
				return tar1 + (s16)_Im_ * 4;
			break;
		case 0x06: // BLEZ
			if (_i32(_rRs_) <= 0)
				return tar1 + (s16)_Im_ * 4;
			break;
		case 0x07: // BGTZ
			if (_i32(_rRs_) > 0)
				return tar1 + (s16)_Im_ * 4;
			break;
	}

	*taken = 0;
	return tar1;
}

static void psxDoDelayBranch(psxRegisters *regs, u32 tar1, u32 code1) {
	u32 tar2, code;
	int taken, lim;

	tar2 = psxBranchNoDelay(regs, tar1, code1, &taken);
	regs->pc = tar1;
	if (!taken)
		return;

	/*
	 * taken branch in delay slot:
	 * - execute 1 instruction at tar1
	 * - jump to tar2 (target of branch in delay slot; this branch
	 *   has no normal delay slot, instruction at tar1 was fetched instead)
	 */
	for (lim = 0; lim < 8; lim++) {
		regs->code = code = fetch(regs, psxMemRLUT, tar1);
		addCycle(regs);
		if (likely(!isBranch(code))) {
			dloadStep(regs);
			psxBSC[code >> 26](regs, code);
			regs->pc = tar2;
			return;
		}
		tar1 = psxBranchNoDelay(regs, tar2, code, &taken);
		regs->pc = tar2;
		if (!taken)
			return;
		swap_(tar1, tar2);
	}
	SysPrintf("Evil chained DS branches @ %08x %08x %08x\n", regs->pc, tar1, tar2);
}

static void doBranch(psxRegisters *regs, u32 tar, enum R3000Abdt taken) {
	u32 code, pc, pc_final;

	regs->branchSeen = regs->branching = taken;
	pc_final = taken == R3000A_BRANCH_TAKEN ? tar : regs->pc + 4;

	// fetch the delay slot
	pc = regs->pc;
	regs->pc = pc + 4;
	regs->code = code = fetch(regs, psxMemRLUT, pc);

	addCycle(regs);

	// check for branch in delay slot
	if (unlikely(isBranch(code))) {
		regs->pc = pc;
		if (taken == R3000A_BRANCH_TAKEN)
			psxDoDelayBranch(regs, tar, code);
		log_unhandled("branch in DS: %08x->%08x\n", pc, regs->pc);
		regs->branching = 0;
		psxBranchTest();
		return;
	}

	dloadStep(regs);
	psxBSC[code >> 26](regs, code);

	if (likely(regs->branching != R3000A_BRANCH_NONE_OR_EXCEPTION))
		regs->pc = pc_final;
	else
		regs->CP0.n.Target = pc_final;
	regs->branching = 0;

	psxBranchTest();
}

static void doBranchReg(psxRegisters *regs, u32 tar) {
	doBranch(regs, tar & ~3, R3000A_BRANCH_TAKEN);
}

static void doBranchRegE(psxRegisters *regs, u32 tar) {
	if (unlikely(DBR_EN_EXEC(regs->CP0.n.DCIC, tar) &&
	    ((tar ^ regs->CP0.n.BPC) & regs->CP0.n.BPCM) == 0))
		regs->CP0.n.DCIC |= 0x03;
	if (unlikely(tar & 3)) {
		SysPrintf("game crash @%08x, ra=%08x\n", tar, regs->GPR.n.ra);
		regs->CP0.n.BadVAddr = tar;
		intException(regs, tar, R3000E_AdEL << 2);
		return;
	}
	doBranch(regs, tar, R3000A_BRANCH_TAKEN);
}

static void addExc(psxRegisters *regs, u32 rt, s32 a1, s32 a2) {
	s32 val;
	if (add_overflow(a1, a2, val)) {
		//printf("ov %08x + %08x = %08x\n", a1, a2, val);
		intExceptionInsn(regs, R3000E_Ov << 2);
		return;
	}
	dloadRt(regs, rt, val);
}

static void subExc(psxRegisters *regs, u32 rt, s32 a1, s32 a2) {
	s32 val;
	if (sub_overflow(a1, a2, val)) {
		intExceptionInsn(regs, R3000E_Ov << 2);
		return;
	}
	dloadRt(regs, rt, val);
}

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
OP(psxADDI)  { addExc (regs_, _Rt_, _i32(_rRs_), _Imm_); } // Rt = Rs + Im (Exception on Integer Overflow)
OP(psxADDIU) { dloadRt(regs_, _Rt_, _u32(_rRs_) + _Imm_ ); }  // Rt = Rs + Im
OP(psxANDI)  { dloadRt(regs_, _Rt_, _u32(_rRs_) & _ImmU_); }  // Rt = Rs And Im
OP(psxORI)   { dloadRt(regs_, _Rt_, _u32(_rRs_) | _ImmU_); }  // Rt = Rs Or  Im
OP(psxXORI)  { dloadRt(regs_, _Rt_, _u32(_rRs_) ^ _ImmU_); }  // Rt = Rs Xor Im
OP(psxSLTI)  { dloadRt(regs_, _Rt_, _i32(_rRs_) < _Imm_ ); }  // Rt = Rs < Im (Signed)
OP(psxSLTIU) { dloadRt(regs_, _Rt_, _u32(_rRs_) < ((u32)_Imm_)); } // Rt = Rs < Im (Unsigned)

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/
OP(psxADD)   { addExc (regs_, _Rd_, _i32(_rRs_), _i32(_rRt_)); } // Rd = Rs + Rt (Exception on Integer Overflow)
OP(psxSUB)   { subExc (regs_, _Rd_, _i32(_rRs_), _i32(_rRt_)); } // Rd = Rs - Rt (Exception on Integer Overflow)
OP(psxADDU)  { dloadRt(regs_, _Rd_, _u32(_rRs_) + _u32(_rRt_)); }  // Rd = Rs + Rt
OP(psxSUBU)  { dloadRt(regs_, _Rd_, _u32(_rRs_) - _u32(_rRt_)); }  // Rd = Rs - Rt
OP(psxAND)   { dloadRt(regs_, _Rd_, _u32(_rRs_) & _u32(_rRt_)); }  // Rd = Rs And Rt
OP(psxOR)    { dloadRt(regs_, _Rd_, _u32(_rRs_) | _u32(_rRt_)); }  // Rd = Rs Or  Rt
OP(psxXOR)   { dloadRt(regs_, _Rd_, _u32(_rRs_) ^ _u32(_rRt_)); }  // Rd = Rs Xor Rt
OP(psxNOR)   { dloadRt(regs_, _Rd_, ~_u32(_rRs_ | _u32(_rRt_))); } // Rd = Rs Nor Rt
OP(psxSLT)   { dloadRt(regs_, _Rd_, _i32(_rRs_) < _i32(_rRt_)); }  // Rd = Rs < Rt (Signed)
OP(psxSLTU)  { dloadRt(regs_, _Rd_, _u32(_rRs_) < _u32(_rRt_)); }  // Rd = Rs < Rt (Unsigned)

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/
OP(psxDIV) {
	if (!_rRt_) {
		_rHi_ = _rRs_;
		if (_rRs_ & 0x80000000) {
			_rLo_ = 1;
		} else {
			_rLo_ = 0xFFFFFFFF;
		}
	}
#if !defined(__arm__) && !defined(__aarch64__)
	else if (_rRs_ == 0x80000000 && _rRt_ == 0xFFFFFFFF) {
		_rLo_ = 0x80000000;
		_rHi_ = 0;
	}
#endif
	else {
		_rLo_ = _i32(_rRs_) / _i32(_rRt_);
		_rHi_ = _i32(_rRs_) % _i32(_rRt_);
	}
}

OP(psxDIV_stall) {
	regs_->muldivBusyCycle = regs_->cycle + 37;
	psxDIV(regs_, code);
}

OP(psxDIVU) {
	if (_rRt_ != 0) {
		_rLo_ = _rRs_ / _rRt_;
		_rHi_ = _rRs_ % _rRt_;
	}
	else {
		_rLo_ = 0xffffffff;
		_rHi_ = _rRs_;
	}
}

OP(psxDIVU_stall) {
	regs_->muldivBusyCycle = regs_->cycle + 37;
	psxDIVU(regs_, code);
}

OP(psxMULT) {
	u64 res = (s64)_i32(_rRs_) * _i32(_rRt_);

	regs_->GPR.n.lo = (u32)res;
	regs_->GPR.n.hi = (u32)(res >> 32);
}

OP(psxMULT_stall) {
	// approximate, but maybe good enough
	u32 rs = _rRs_;
	u32 lz = __builtin_clz(((rs ^ ((s32)rs >> 21)) | 1));
	u32 c = 7 + (2 - (lz / 11)) * 4;
	regs_->muldivBusyCycle = regs_->cycle + c;
	psxMULT(regs_, code);
}

OP(psxMULTU) {
	u64 res = (u64)_u32(_rRs_) * _u32(_rRt_);

	regs_->GPR.n.lo = (u32)(res & 0xffffffff);
	regs_->GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}

OP(psxMULTU_stall) {
	// approximate, but maybe good enough
	u32 lz = __builtin_clz(_rRs_ | 1);
	u32 c = 7 + (2 - (lz / 11)) * 4;
	regs_->muldivBusyCycle = regs_->cycle + c;
	psxMULTU(regs_, code);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/
#define BrCond(c) (c) ? R3000A_BRANCH_TAKEN : R3000A_BRANCH_NOT_TAKEN
#define RepZBranchi32(op) \
	doBranch(regs_, _BranchTarget_, BrCond(_i32(_rRs_) op 0));
#define RepZBranchLinki32(op)  { \
	s32 temp = _i32(_rRs_); \
	dloadFlush(regs_); \
	_SetLink(31); \
	doBranch(regs_, _BranchTarget_, BrCond(temp op 0)); \
}

OP(psxBGEZ)   { RepZBranchi32(>=) }      // Branch if Rs >= 0
OP(psxBGEZAL) { RepZBranchLinki32(>=) }  // Branch if Rs >= 0 and link
OP(psxBGTZ)   { RepZBranchi32(>) }       // Branch if Rs >  0
OP(psxBLEZ)   { RepZBranchi32(<=) }      // Branch if Rs <= 0
OP(psxBLTZ)   { RepZBranchi32(<) }       // Branch if Rs <  0
OP(psxBLTZAL) { RepZBranchLinki32(<) }   // Branch if Rs <  0 and link

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
OP(psxSLL) { dloadRt(regs_, _Rd_, _u32(_rRt_) << _Sa_); } // Rd = Rt << sa
OP(psxSRA) { dloadRt(regs_, _Rd_, _i32(_rRt_) >> _Sa_); } // Rd = Rt >> sa (arithmetic)
OP(psxSRL) { dloadRt(regs_, _Rd_, _u32(_rRt_) >> _Sa_); } // Rd = Rt >> sa (logical)

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/
OP(psxSLLV) { dloadRt(regs_, _Rd_, _u32(_rRt_) << (_u32(_rRs_) & 0x1F)); } // Rd = Rt << rs
OP(psxSRAV) { dloadRt(regs_, _Rd_, _i32(_rRt_) >> (_u32(_rRs_) & 0x1F)); } // Rd = Rt >> rs (arithmetic)
OP(psxSRLV) { dloadRt(regs_, _Rd_, _u32(_rRt_) >> (_u32(_rRs_) & 0x1F)); } // Rd = Rt >> rs (logical)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
OP(psxLUI) { dloadRt(regs_, _Rt_, code << 16); } // Upper halfword of Rt = Im

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/
OP(psxMFHI) { dloadRt(regs_, _Rd_, _rHi_); } // Rd = Hi
OP(psxMFLO) { dloadRt(regs_, _Rd_, _rLo_); } // Rd = Lo

static void mflohiCheckStall(psxRegisters *regs_)
{
	u32 left = regs_->muldivBusyCycle - regs_->cycle;
	if (left <= 37) {
		//printf("muldiv stall %u\n", left);
		regs_->cycle = regs_->muldivBusyCycle;
	}
}

OP(psxMFHI_stall) { mflohiCheckStall(regs_); psxMFHI(regs_, code); }
OP(psxMFLO_stall) { mflohiCheckStall(regs_); psxMFLO(regs_, code); }

/*********************************************************
* Move to GPR to HI/LO & Register jump                   *
* Format:  OP rs                                         *
*********************************************************/
OP(psxMTHI) { _rHi_ = _rRs_; } // Hi = Rs
OP(psxMTLO) { _rLo_ = _rRs_; } // Lo = Rs

/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/
OP(psxBREAK) {
	intExceptionInsn(regs_, R3000E_Bp << 2);
}

OP(psxSYSCALL) {
	intExceptionInsn(regs_, R3000E_Syscall << 2);
}

static inline void execI_(u8 **memRLUT, psxRegisters *regs_);

static inline void psxTestSWInts(psxRegisters *regs_, int step) {
	if ((regs_->CP0.n.Cause & regs_->CP0.n.SR & 0x0300) &&
	    (regs_->CP0.n.SR & 0x1)) {
		if (step)
			execI_(psxMemRLUT, regs_);
		regs_->CP0.n.Cause &= ~0x7c;
		intException(regs_, regs_->pc, regs_->CP0.n.Cause);
	}
}

OP(psxRFE) {
	regs_->CP0.n.SR = (regs_->CP0.n.SR & ~0x0f) | ((regs_->CP0.n.SR & 0x3c) >> 2);
	psxTestSWInts(regs_, 0);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#define RepBranchi32(op) \
	doBranch(regs_, _BranchTarget_, BrCond(_i32(_rRs_) op _i32(_rRt_)));

OP(psxBEQ) { RepBranchi32(==) }  // Branch if Rs == Rt
OP(psxBNE) { RepBranchi32(!=) }  // Branch if Rs != Rt

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
OP(psxJ)   { doBranch(regs_, _JumpTarget_, R3000A_BRANCH_TAKEN); }
OP(psxJAL) {
	dloadFlush(regs_);
	_SetLink(31);
	doBranch(regs_, _JumpTarget_, R3000A_BRANCH_TAKEN);
}

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
OP(psxJR) {
	doBranchReg(regs_, _rRs_);
	psxJumpTest();
}

OP(psxJRe) {
	doBranchRegE(regs_, _rRs_);
	psxJumpTest();
}

OP(psxJALR) {
	u32 temp = _u32(_rRs_);
	dloadFlush(regs_);
	if (_Rd_) { _SetLink(_Rd_); }
	doBranchReg(regs_, temp);
}

OP(psxJALRe) {
	u32 temp = _u32(_rRs_);
	dloadFlush(regs_);
	if (_Rd_) { _SetLink(_Rd_); }
	doBranchRegE(regs_, temp);
}

/*********************************************************
*********************************************************/

// revisit: incomplete
#define BUS_LOCKED_ADDR(a) \
	((0x1fc80000u <= (a) && (a) < 0x80000000u) || \
	 (0xc0000000u <= (a) && (a) < 0xfffe0000u))

// exception checking order is important
static inline int checkLD(psxRegisters *regs, u32 addr, u32 m) {
	int bpException = 0;
	if (unlikely(DBR_EN_LD(regs->CP0.n.DCIC, addr) &&
	    ((addr ^ regs->CP0.n.BDA) & regs->CP0.n.BDAM) == 0)) {
		regs->CP0.n.DCIC |= 0x0d;
		bpException = regs->CP0.n.DCIC >> 31;
	}
	if (unlikely(addr & m)) {
		regs->CP0.n.BadVAddr = addr;
		intExceptionInsn(regs, R3000E_AdEL << 2);
		return 0;
	}
	if (unlikely(bpException)) {
		intExceptionDebugBp(regs, regs->pc - 4);
		return 0;
	}
	if (unlikely(BUS_LOCKED_ADDR(addr))) {
		log_unhandled("bus error read addr=%08x @%08x ra=%08x\n",
			addr, regs->pc - 4, regs->GPR.n.ra);
		intException(regs, regs->pc - 4, R3000E_DBE << 2);
		return 0;
	}
	return 1;
}

static inline int checkST(psxRegisters *regs, u32 addr, u32 m) {
	int bpException = 0;
	if (unlikely(DBR_EN_ST(regs->CP0.n.DCIC, addr) &&
	    ((addr ^ regs->CP0.n.BDA) & regs->CP0.n.BDAM) == 0)) {
		regs->CP0.n.DCIC |= 0x15;
		bpException = regs->CP0.n.DCIC >> 31;
	}
	if (unlikely(addr & m)) {
		regs->CP0.n.BadVAddr = addr;
		intExceptionInsn(regs, R3000E_AdES << 2);
		return 0;
	}
	if (unlikely(bpException)) {
		intExceptionDebugBp(regs, regs->pc - 4);
		return 0;
	}
	if (unlikely(BUS_LOCKED_ADDR(addr))) {
		log_unhandled("bus error write addr=%08x @%08x ra=%08x\n",
			addr, regs->pc - 4, regs->GPR.n.ra);
		intException(regs, regs->pc - 4, R3000E_DBE << 2);
		return 0;
	}
	return 1;
}

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _oB_ (regs_->GPR.r[_Rs_] + _Imm_)

OP(psxLB)  { doLoad(regs_, _Rt_,  (s8)psxMemRead8(_oB_)); }
OP(psxLBU) { doLoad(regs_, _Rt_,      psxMemRead8(_oB_)); }
OP(psxLH)  { doLoad(regs_, _Rt_, (s16)psxMemRead16(_oB_ & ~1)); }
OP(psxLHU) { doLoad(regs_, _Rt_,      psxMemRead16(_oB_ & ~1)); }
OP(psxLW)  { doLoad(regs_, _Rt_,      psxMemRead32(_oB_ & ~3)); }

OP(psxLBe)  { if (checkLD(regs_, _oB_, 0)) doLoad(regs_, _Rt_,  (s8)psxMemRead8(_oB_)); }
OP(psxLBUe) { if (checkLD(regs_, _oB_, 0)) doLoad(regs_, _Rt_,      psxMemRead8(_oB_)); }
OP(psxLHe)  { if (checkLD(regs_, _oB_, 1)) doLoad(regs_, _Rt_, (s16)psxMemRead16(_oB_)); }
OP(psxLHUe) { if (checkLD(regs_, _oB_, 1)) doLoad(regs_, _Rt_,      psxMemRead16(_oB_)); }
OP(psxLWe)  { if (checkLD(regs_, _oB_, 3)) doLoad(regs_, _Rt_,      psxMemRead32(_oB_)); }

static void doLWL(psxRegisters *regs, u32 rt, u32 addr) {
	static const u32 LWL_MASK[4] = { 0xffffff, 0xffff, 0xff, 0 };
	static const u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };
	u32 shift = addr & 3;
	u32 val, mem;
	u32 oldval = regs->GPR.r[rt];

#ifdef HANDLE_LOAD_DELAY
	int sel = regs->dloadSel;
	if (regs->dloadReg[sel] == rt)
		oldval = regs->dloadVal[sel];
#endif
	mem = psxMemRead32(addr & ~3);
	val = (oldval & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]);
	doLoad(regs, rt, val);

	/*
	Mem = 1234.  Reg = abcd

	0   4bcd   (mem << 24) | (reg & 0x00ffffff)
	1   34cd   (mem << 16) | (reg & 0x0000ffff)
	2   234d   (mem <<  8) | (reg & 0x000000ff)
	3   1234   (mem      ) | (reg & 0x00000000)
	*/
}

static void doLWR(psxRegisters *regs, u32 rt, u32 addr) {
	static const u32 LWR_MASK[4] = { 0, 0xff000000, 0xffff0000, 0xffffff00 };
	static const u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };
	u32 shift = addr & 3;
	u32 val, mem;
	u32 oldval = regs->GPR.r[rt];

#ifdef HANDLE_LOAD_DELAY
	int sel = regs->dloadSel;
	if (regs->dloadReg[sel] == rt)
		oldval = regs->dloadVal[sel];
#endif
	mem = psxMemRead32(addr & ~3);
	val = (oldval & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]);
	doLoad(regs, rt, val);

	/*
	Mem = 1234.  Reg = abcd

	0   1234   (mem      ) | (reg & 0x00000000)
	1   a123   (mem >>  8) | (reg & 0xff000000)
	2   ab12   (mem >> 16) | (reg & 0xffff0000)
	3   abc1   (mem >> 24) | (reg & 0xffffff00)
	*/
}

OP(psxLWL) { doLWL(regs_, _Rt_, _oB_); }
OP(psxLWR) { doLWR(regs_, _Rt_, _oB_); }

OP(psxLWLe) { if (checkLD(regs_, _oB_ & ~3, 0)) doLWL(regs_, _Rt_, _oB_); }
OP(psxLWRe) { if (checkLD(regs_, _oB_     , 0)) doLWR(regs_, _Rt_, _oB_); }

OP(psxSB) { psxMemWrite8 (_oB_, _rRt_); }
OP(psxSH) { psxMemWrite16(_oB_, _rRt_); }
OP(psxSW) { psxMemWrite32(_oB_, _rRt_); }

OP(psxSBe) { if (checkST(regs_, _oB_, 0)) psxMemWrite8 (_oB_, _rRt_); }
OP(psxSHe) { if (checkST(regs_, _oB_, 1)) psxMemWrite16(_oB_, _rRt_); }
OP(psxSWe) { if (checkST(regs_, _oB_, 3)) psxMemWrite32(_oB_, _rRt_); }

static void doSWL(psxRegisters *regs, u32 rt, u32 addr) {
	u32 val = regs->GPR.r[rt];
	switch (addr & 3) {
		case 0: psxMemWrite8( addr     , val >> 24); break;
		case 1: psxMemWrite16(addr & ~3, val >> 16); break;
		case 2: // revisit: should be a single 24bit write
			psxMemWrite16(addr & ~3, (val >> 8) & 0xffff);
		        psxMemWrite8( addr     , val >> 24); break;
		case 3: psxMemWrite32(addr & ~3, val);       break;
	}
	/*
	Mem = 1234.  Reg = abcd

	0   123a   (reg >> 24) | (mem & 0xffffff00)
	1   12ab   (reg >> 16) | (mem & 0xffff0000)
	2   1abc   (reg >>  8) | (mem & 0xff000000)
	3   abcd   (reg      ) | (mem & 0x00000000)
	*/
}

static void doSWR(psxRegisters *regs, u32 rt, u32 addr) {
	u32 val = regs->GPR.r[rt];
	switch (addr & 3) {
		case 0: psxMemWrite32(addr    , val); break;
		case 1: // revisit: should be a single 24bit write
		        psxMemWrite8 (addr    , val & 0xff);
		        psxMemWrite16(addr + 1, (val >> 8) & 0xffff); break;
		case 2: psxMemWrite16(addr    , val & 0xffff); break;
		case 3: psxMemWrite8 (addr    , val & 0xff); break;
	}

	/*
	Mem = 1234.  Reg = abcd

	0   abcd   (reg      ) | (mem & 0x00000000)
	1   bcd4   (reg <<  8) | (mem & 0x000000ff)
	2   cd34   (reg << 16) | (mem & 0x0000ffff)
	3   d234   (reg << 24) | (mem & 0x00ffffff)
	*/
}

OP(psxSWL) { doSWL(regs_, _Rt_, _oB_); }
OP(psxSWR) { doSWR(regs_, _Rt_, _oB_); }

OP(psxSWLe) { if (checkST(regs_, _oB_ & ~3, 0)) doSWL(regs_, _Rt_, _oB_); }
OP(psxSWRe) { if (checkST(regs_, _oB_     , 0)) doSWR(regs_, _Rt_, _oB_); }

/*********************************************************
* Moves between GPR and COPx                             *
* Format:  OP rt, fs                                     *
*********************************************************/
OP(psxMFC0) {
	u32 r = _Rd_;
	if (unlikely(0x00000417u & (1u << r)))
		intExceptionReservedInsn(regs_);
	doLoad(regs_, _Rt_, regs_->CP0.r[r]);
}

static void setupCop(u32 sr);

void MTC0(psxRegisters *regs_, int reg, u32 val) {
//	SysPrintf("MTC0 %d: %x\n", reg, val);
	switch (reg) {
		case 12: // SR
			if (unlikely((regs_->CP0.n.SR ^ val) & (1u << 16)))
				psxMemOnIsolate((val >> 16) & 1);
			if (unlikely((regs_->CP0.n.SR ^ val) & (7u << 29)))
				setupCop(val);
			regs_->CP0.n.SR = val;
			psxTestSWInts(regs_, 1);
			break;

		case 13: // Cause
			regs_->CP0.n.Cause &= ~0x0300;
			regs_->CP0.n.Cause |= val & 0x0300;
			psxTestSWInts(regs_, 0);
			break;

		case 7:
			if ((regs_->CP0.n.DCIC ^ val) & 0xff800000)
				log_unhandled("DCIC: %08x->%08x\n", regs_->CP0.n.DCIC, val);
			goto default_;
		case 3:
			if (regs_->CP0.n.BPC != val)
				log_unhandled("BPC: %08x->%08x\n", regs_->CP0.n.BPC, val);
			goto default_;

		default:
		default_:
			regs_->CP0.r[reg] = val;
			break;
	}
}

OP(psxMTC0) { MTC0(regs_, _Rd_, _u32(_rRt_)); }

// no exception
static inline void psxNULLne(psxRegisters *regs) {
	log_unhandled("unhandled op %08x @%08x\n", regs->code, regs->pc - 4);
}

/*********************************************************
* Unknown instruction (would generate an exception)      *
* Format:  ?                                             *
*********************************************************/

OP(psxNULL) {
	psxNULLne(regs_);
	intExceptionReservedInsn(regs_);
}

void gteNULL(struct psxCP2Regs *regs) {
	psxRegisters *regs_ = (psxRegisters *)((u8 *)regs - offsetof(psxRegisters, CP2));
	psxNULLne(regs_);
}

OP(psxSPECIAL) {
	psxSPC[_Funct_](regs_, code);
}

OP(psxCOP0) {
	u32 rs = _Rs_;
	if (rs & 0x10) {
		u32 op2 = code & 0x1f;
		switch (op2) {
			case 0x01:
			case 0x02:
			case 0x06:
			case 0x08: psxNULL(regs_, code); break;
			case 0x10: psxRFE(regs_, code);  break;
			default:   psxNULLne(regs_);     break;
		}
		return;
	}
	switch (rs) {
		case 0x00: psxMFC0(regs_, code); break;
		case 0x04: psxMTC0(regs_, code); break;
		case 0x02:                              // CFC
		case 0x06: psxNULL(regs_, code); break; // CTC -> exception
		case 0x08:
		case 0x0c: log_unhandled("BC0 %08x @%08x\n", code, regs_->pc - 4);
		default:   psxNULLne(regs_);     break;
	}
}

OP(psxCOP1) {
	// ??? what actually happens here?
	log_unhandled("COP1 %08x @%08x\n", code, regs_->pc - 4);
}

OP(psxCOP2) {
	u32 rt = _Rt_, rd = _Rd_, rs = _Rs_;
	if (rs & 0x10) {
		psxCP2[_Funct_](&regs_->CP2);
		return;
	}
	switch (rs) {
		case 0x00: doLoad(regs_, rt, MFC2(&regs_->CP2, rd)); break; // MFC2
		case 0x02: doLoad(regs_, rt, regs_->CP2C.r[rd]);     break; // CFC2
		case 0x04: MTC2(&regs_->CP2, regs_->GPR.r[rt], rd);  break; // MTC2
		case 0x06: CTC2(&regs_->CP2, regs_->GPR.r[rt], rd);  break; // CTC2
		case 0x08:
		case 0x0c: log_unhandled("BC2 %08x @%08x\n", code, regs_->pc - 4);
		default:   psxNULLne(regs_); break;
	}
}

OP(psxCOP2_stall) {
	u32 f = _Funct_;
	gteCheckStall(f);
	psxCOP2(regs_, code);
}

OP(gteLWC2) {
	MTC2(&regs_->CP2, psxMemRead32(_oB_), _Rt_);
}

OP(gteLWC2_stall) {
	gteCheckStall(0);
	gteLWC2(regs_, code);
}

OP(gteLWC2e_stall) {
	gteCheckStall(0);
	if (checkLD(regs_, _oB_, 3))
		MTC2(&regs_->CP2, psxMemRead32(_oB_), _Rt_);
}

OP(gteSWC2) {
	psxMemWrite32(_oB_, MFC2(&regs_->CP2, _Rt_));
}

OP(gteSWC2_stall) {
	gteCheckStall(0);
	gteSWC2(regs_, code);
}

OP(gteSWC2e_stall) {
	gteCheckStall(0);
	if (checkST(regs_, _oB_, 3))
		gteSWC2(regs_, code);
}

OP(psxCOP3) {
	// ??? what actually happens here?
	log_unhandled("COP3 %08x @%08x\n", code, regs_->pc - 4);
}

OP(psxCOPd) {
	log_unhandled("disabled cop%d @%08x\n", (code >> 26) & 3, regs_->pc - 4);
#ifdef DO_EXCEPTION_RESERVEDI
	intExceptionInsn(regs_, R3000E_CpU << 2);
#endif
}

OP(psxLWCx) {
	log_unhandled("LWCx %08x @%08x\n", code, regs_->pc - 4);
	checkLD(regs_, _oB_, 3);
}

OP(psxSWCx) {
	// does this write something to memory?
	log_unhandled("SWCx %08x @%08x\n", code, regs_->pc - 4);
	checkST(regs_, _oB_, 3);
}

OP(psxREGIMM) {
	u32 rt = _Rt_;
	switch (rt) {
		case 0x10: psxBLTZAL(regs_, code); break;
		case 0x11: psxBGEZAL(regs_, code); break;
		default:
			if (rt & 1)
				psxBGEZ(regs_, code);
			else
				psxBLTZ(regs_, code);
	}
}

OP(psxHLE) {
	u32 hleCode;
	if (unlikely(!Config.HLE)) {
		psxSWCx(regs_, code);
		return;
	}
	hleCode = code & 0x03ffffff;
	if (hleCode >= (sizeof(psxHLEt) / sizeof(psxHLEt[0]))) {
		psxSWCx(regs_, code);
		return;
	}
	dloadFlush(regs_);
	psxHLEt[hleCode]();
	regs_->branchSeen = 1;
}

static void (INT_ATTR *psxBSC[64])(psxRegisters *regs_, u32 code) = {
	psxSPECIAL, psxREGIMM, psxJ   , psxJAL  , psxBEQ , psxBNE , psxBLEZ, psxBGTZ,
	psxADDI   , psxADDIU , psxSLTI, psxSLTIU, psxANDI, psxORI , psxXORI, psxLUI ,
	psxCOP0   , psxCOPd  , psxCOP2, psxCOPd,  psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL   , psxNULL  , psxNULL, psxNULL,  psxNULL, psxNULL, psxNULL, psxNULL,
	psxLB     , psxLH    , psxLWL , psxLW   , psxLBU , psxLHU , psxLWR , psxNULL,
	psxSB     , psxSH    , psxSWL , psxSW   , psxNULL, psxNULL, psxSWR , psxNULL,
	psxLWCx   , psxLWCx  , gteLWC2, psxLWCx , psxNULL, psxNULL, psxNULL, psxNULL,
	psxSWCx   , psxSWCx  , gteSWC2, psxHLE  , psxNULL, psxNULL, psxNULL, psxNULL,
};

static void (INT_ATTR *psxSPC[64])(psxRegisters *regs_, u32 code) = {
	psxSLL , psxNULL , psxSRL , psxSRA , psxSLLV   , psxNULL , psxSRLV, psxSRAV,
	psxJR  , psxJALR , psxNULL, psxNULL, psxSYSCALL, psxBREAK, psxNULL, psxNULL,
	psxMFHI, psxMTHI , psxMFLO, psxMTLO, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxMULT, psxMULTU, psxDIV , psxDIVU, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxADD , psxADDU , psxSUB , psxSUBU, psxAND    , psxOR   , psxXOR , psxNOR ,
	psxNULL, psxNULL , psxSLT , psxSLTU, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxNULL, psxNULL , psxNULL, psxNULL, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxNULL, psxNULL , psxNULL, psxNULL, psxNULL   , psxNULL , psxNULL, psxNULL
};

void (*psxCP2[64])(struct psxCP2Regs *regs) = {
	gteNULL , gteRTPS , gteNULL , gteNULL, gteNULL, gteNULL , gteNCLIP, gteNULL, // 00
	gteNULL , gteNULL , gteNULL , gteNULL, gteOP  , gteNULL , gteNULL , gteNULL, // 08
	gteDPCS , gteINTPL, gteMVMVA, gteNCDS, gteCDP , gteNULL , gteNCDT , gteNULL, // 10
	gteNULL , gteNULL , gteNULL , gteNCCS, gteCC  , gteNULL , gteNCS  , gteNULL, // 18
	gteNCT  , gteNULL , gteNULL , gteNULL, gteNULL, gteNULL , gteNULL , gteNULL, // 20
	gteSQR  , gteDCPL , gteDPCT , gteNULL, gteNULL, gteAVSZ3, gteAVSZ4, gteNULL, // 28
	gteRTPT , gteNULL , gteNULL , gteNULL, gteNULL, gteNULL , gteNULL , gteNULL, // 30
	gteNULL , gteNULL , gteNULL , gteNULL, gteNULL, gteGPF  , gteGPL  , gteNCCT  // 38
};

///////////////////////////////////////////

static int intInit() {
	intApplyConfig();
	return 0;
}

static void intReset() {
	dloadClear(&psxRegs);
	psxRegs.subCycle = 0;
}

static inline void execI_(u8 **memRLUT, psxRegisters *regs) {
	u32 pc = regs->pc;

	addCycle(regs);
	dloadStep(regs);

	regs->pc += 4;
	regs->code = fetch(regs, memRLUT, pc);
	psxBSC[regs->code >> 26](regs, regs->code);
}

static inline void execIbp(u8 **memRLUT, psxRegisters *regs) {
	u32 pc = regs->pc;

	addCycle(regs);
	dloadStep(regs);

	if (execBreakCheck(regs, pc))
		return;

	regs->pc += 4;
	regs->code = fetch(regs, memRLUT, pc);
	psxBSC[regs->code >> 26](regs, regs->code);
}

static void intExecute(psxRegisters *regs) {
	u8 **memRLUT = psxMemRLUT;

	while (!regs->stop)
		execI_(memRLUT, regs);
}

static void intExecuteBp(psxRegisters *regs) {
	u8 **memRLUT = psxMemRLUT;

	while (!regs->stop)
		execIbp(memRLUT, regs);
}

static void intExecuteBlock(psxRegisters *regs, enum blockExecCaller caller) {
	u8 **memRLUT = psxMemRLUT;

	regs->branchSeen = 0;
	while (!regs->branchSeen)
		execI_(memRLUT, regs);
}

static void intExecuteBlockBp(psxRegisters *regs, enum blockExecCaller caller) {
	u8 **memRLUT = psxMemRLUT;

	regs->branchSeen = 0;
	while (!regs->branchSeen)
		execIbp(memRLUT, regs);
}

static void intClear(u32 Addr, u32 Size) {
}

static void intNotify(enum R3000Anote note, void *data) {
	switch (note) {
	case R3000ACPU_NOTIFY_BEFORE_SAVE:
		dloadFlush(&psxRegs);
		break;
	case R3000ACPU_NOTIFY_AFTER_LOAD:
		dloadClear(&psxRegs);
		psxRegs.subCycle = 0;
		setupCop(psxRegs.CP0.n.SR);
		// fallthrough
	case R3000ACPU_NOTIFY_CACHE_ISOLATED: // Armored Core?
		if (fetch == fetchICache)
			memset(&ICache, 0xff, sizeof(ICache));
		break;
	case R3000ACPU_NOTIFY_CACHE_UNISOLATED:
		break;
	}
}

static void setupCop(u32 sr)
{
	if (sr & (1u << 29))
		psxBSC[17] = psxCOP1;
	else
		psxBSC[17] = psxCOPd;
	if (sr & (1u << 30))
		psxBSC[18] = Config.DisableStalls ? psxCOP2 : psxCOP2_stall;
	else
		psxBSC[18] = psxCOPd;
	if (sr & (1u << 31))
		psxBSC[19] = psxCOP3;
	else
		psxBSC[19] = psxCOPd;
}

void intApplyConfig() {
	int cycle_mult;

	assert(psxSPC[16] == psxMFHI  || psxSPC[16] == psxMFHI_stall);
	assert(psxSPC[18] == psxMFLO  || psxSPC[18] == psxMFLO_stall);
	assert(psxSPC[24] == psxMULT  || psxSPC[24] == psxMULT_stall);
	assert(psxSPC[25] == psxMULTU || psxSPC[25] == psxMULTU_stall);
	assert(psxSPC[26] == psxDIV   || psxSPC[26] == psxDIV_stall);
	assert(psxSPC[27] == psxDIVU  || psxSPC[27] == psxDIVU_stall);

	if (Config.DisableStalls) {
		psxBSC[18] = psxCOP2;
		psxBSC[50] = gteLWC2;
		psxBSC[58] = gteSWC2;
		psxSPC[16] = psxMFHI;
		psxSPC[18] = psxMFLO;
		psxSPC[24] = psxMULT;
		psxSPC[25] = psxMULTU;
		psxSPC[26] = psxDIV;
		psxSPC[27] = psxDIVU;
	} else {
		psxBSC[18] = psxCOP2_stall;
		psxBSC[50] = gteLWC2_stall;
		psxBSC[58] = gteSWC2_stall;
		psxSPC[16] = psxMFHI_stall;
		psxSPC[18] = psxMFLO_stall;
		psxSPC[24] = psxMULT_stall;
		psxSPC[25] = psxMULTU_stall;
		psxSPC[26] = psxDIV_stall;
		psxSPC[27] = psxDIVU_stall;
	}
	setupCop(psxRegs.CP0.n.SR);

	if (Config.PreciseExceptions) {
		psxBSC[0x20] = psxLBe;
		psxBSC[0x21] = psxLHe;
		psxBSC[0x22] = psxLWLe;
		psxBSC[0x23] = psxLWe;
		psxBSC[0x24] = psxLBUe;
		psxBSC[0x25] = psxLHUe;
		psxBSC[0x26] = psxLWRe;
		psxBSC[0x28] = psxSBe;
		psxBSC[0x29] = psxSHe;
		psxBSC[0x2a] = psxSWLe;
		psxBSC[0x2b] = psxSWe;
		psxBSC[0x2e] = psxSWRe;
		psxBSC[0x32] = gteLWC2e_stall;
		psxBSC[0x3a] = gteSWC2e_stall;
		psxSPC[0x08] = psxJRe;
		psxSPC[0x09] = psxJALRe;
		psxInt.Execute = intExecuteBp;
		psxInt.ExecuteBlock = intExecuteBlockBp;
	} else {
		psxBSC[0x20] = psxLB;
		psxBSC[0x21] = psxLH;
		psxBSC[0x22] = psxLWL;
		psxBSC[0x23] = psxLW;
		psxBSC[0x24] = psxLBU;
		psxBSC[0x25] = psxLHU;
		psxBSC[0x26] = psxLWR;
		psxBSC[0x28] = psxSB;
		psxBSC[0x29] = psxSH;
		psxBSC[0x2a] = psxSWL;
		psxBSC[0x2b] = psxSW;
		psxBSC[0x2e] = psxSWR;
		// LWC2, SWC2 handled by Config.DisableStalls
		psxSPC[0x08] = psxJR;
		psxSPC[0x09] = psxJALR;
		psxInt.Execute = intExecute;
		psxInt.ExecuteBlock = intExecuteBlock;
	}

	// the dynarec may occasionally call the interpreter, in such a case the
	// cache won't work (cache only works right if all fetches go through it)
	if (!Config.icache_emulation || psxCpu != &psxInt) {
		fetch = fetchNoCache;
		memset(&ICache, 0xff, sizeof(ICache));
	}
	else
		fetch = fetchICache;

	cycle_mult = Config.cycle_multiplier_override && Config.cycle_multiplier == CYCLE_MULT_DEFAULT
		? Config.cycle_multiplier_override : Config.cycle_multiplier;
	psxRegs.subCycleStep = 0x10000 * cycle_mult / 100;
}

static void intShutdown() {
	dloadClear(&psxRegs);
}

// single step (may do several ops in case of a branch or load delay)
// called by asm/dynarec
void execI(psxRegisters *regs) {
	do {
		execIbp(psxMemRLUT, regs);
	} while (regs->dloadReg[0] || regs->dloadReg[1]);
}

R3000Acpu psxInt = {
	intInit,
	intReset,
	intExecute,
	intExecuteBlock,
	intClear,
	intNotify,
	intApplyConfig,
	intShutdown
};
