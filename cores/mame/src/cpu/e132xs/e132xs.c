/********************************************************************

 Hyperstone E1-32XS cpu emulator
 written by Pierpaolo Prazzoli

 All the types are compatible:
 - Hyperstone E1-32
 - Hyperstone E1-16
 - Hyperstone E1-32X
 - Hyperstone E1-16X
 - Hyperstone E1-32XN
 - Hyperstone E1-32XT
 - Hyperstone E1-16XT
 - Hyperstone E1-32XS
 - Hyperstone E1-16XS
 - Hyperstone E1-32XP (ever released?)
 - Hyperstone E1-32XSB (compatible?)
 - Hyperstone E1-16XSB (compatible?)

 TODO:
 - Fix the debug interface
 - Check if the inverted values are correct and also the sign is correct
 - Check the signs where it's not specified in the docs and if the range is good
 - Check register bounds when Lsf, Ldf, Rsf and Rdf are used
 - Check the delay
 - Check operation with 7 bits variables
 - Check if	L0 is always interpreted as L16
 - Load / Store instruction should use the pipeline
 - Add the latency
 - Add Interrupts
 - Add floating point opcodes
 - Add cast when 8/16 bits are read used?
 - All the TODO in the source
 - Are the read/write functions used correct?

 CHANGELOG:

 Pierpaolo Prazzoli - 10/26/03
	- Changed get_lrconst to get_const and changed it to use the removed GET_CONST_RR
	  macro.
	- Removed the High flag used in some opcodes, it should be used only in
	  MOV and MOVI instruction.
	- Fixed MOV and MOVI instruction.
	- Set to 1 FP is SR register at reset.
	  (From the doc: A Call, Trap or Software instruction increments the FP and sets FL
	  to 6, thus creating a new stack frame with the length of 6 registers).

 MooglyGuy - 10/25/03
 	- Fixed CALL enough that it at least jumps to the right address, no word
 	  yet as to whether or not it's working enough to return.
 	- Added get_lrconst() to get the const value for the CALL operand, since
 	  apparently using immediate_value() was wrong. The code is ugly, but it
 	  works properly. Vampire 1/2 now gets far enough to try to test its RAM.
 	- Just from looking at it, CALL apparently doesn't frame properly. I'm not
 	  sure about FRAME, but perhaps it doesn't work properly - I'm not entirely
 	  positive. The return address when vamphalf's memory check routine is
 	  called at FFFFFD7E is stored in register L8, and then the RET instruction
 	  at the end of the routine uses L1 as the return address, so that might
 	  provide some clues as to how it works.
 	- I'd almost be willing to bet money that there's no framing at all since
 	  the values in L0 - L15 as displayed by the debugger would change during a
 	  CALL or FRAME operation. I'll look when I'm in the mood.
 	- The mood struck me, and I took a look at SET_L_REG and GET_L_REG.
 	  Apparently no matter what the current frame pointer is they'll always use
 	  local_regs[0] through local_regs[15].

 MooglyGuy - 08/20/03
 	- Added H flag support for MOV and MOVI
 	- Changed init routine to set S flag on boot. Apparently the CPU defaults to
 	  supervisor mode as opposed to user mode when it powers on, as shown by the
 	  vamphalf power-on routines. Makes sense, too, since if the machine booted
 	  in user mode, it would be impossible to get into supervisor mode.

 Pierpaolo Prazzoli	- 08/19/03
	- Added check for D_BIT and S_BIT where PC or SR must or must not be denoted.
	  (movd, divu, divs, ldxx1, ldxx2, stxx1, stxx2, mulu, muls, set, mul
	  call, chk)

 MooglyGuy - 08/17/03
	- Working on support for H flag, nothing quite done yet
	- Added trap Range Error for CHK PC, PC
	- Fixed relative jumps, they have to be taken from the opcode following the
	  jump minstead of the jump opcode itself.

 Pierpaolo Prazzoli	- 08/17/03
	- Fixed get_pcrel() when OP & 0x80 is set.
	- Decremented PC by 2 also in MOV, ADD, ADDI, SUM, SUB and added the check if
	  D_BIT is not set. (when pc is changed they are implicit branch)

 MooglyGuy - 08/17/03
    - Implemented a crude hack to set FL in the SR to 6, since according to the docs
      that's supposed to happen each time a trap occurs, apparently including when
      the processor starts up. The 3rd opcode executed in vamphalf checks to see if
      the FL flag in SR 6, so it's apparently the "correct" behaviour despite the
      docs not saying anything on it. If FL is not 6, the branch falls through and
      encounters a CHK PC, L2, which at that point will always throw a range trap.
      The range trap vector contains 00000000 (CHK PC, PC), which according to the
      docs will always throw a range trap (which would effectively lock the system).
      This revealed a bug: CHK PC, PC apparently does not throw a range trap, which
      needs to be fixed. Now that the "correct" behaviour is hacked in with the FL
      flags, it reveals yet another bug in that the branch is interpreted as being
      +0x8700. This means that the PC then wraps around to 000082B0, give or take
      a few bytes. While it does indeed branch to valid code, I highly doubt that
      this is the desired effect. Check for signed/unsigned relative branch, maybe?

 MooglyGuy - 08/16/03
 	- Fixed the debugger at least somewhat so that it displays hex instead of decimal,
 	  and so that it disassembles opcodes properly.
 	- Fixed e132xs_execute() to increment PC *after* executing the opcode instead of
 	  before. This is probably why vamphalf was booting to fffffff8, but executing at
 	  fffffffa instead.
 	- Changed execute_trap to decrement PC by 2 so that the next opcode isn't skipped
 	  after a trap
 	- Changed execute_br to decrement PC by 2 so that the next opcode isn't skipped
 	  after a branch
 	- Changed e132xs_movi to decrement PC by 2 when G0 (PC) is modified so that the
 	  next opcode isn't skipped after a branch
 	- Changed e132xs_movi to default to a UINT32 being moved into the register
 	  as opposed to a UINT8. This is wrong, the bit width is quite likely to be
 	  dependent on the n field in the Rimm instruction type. However, vamphalf uses
 	  MOVI G0,[FFFF]FBAC (n=$13) since there's apparently no absolute branch opcode.
 	  What kind of CPU is this that it doesn't have an absolute jump in its branch
 	  instructions and you have to use an immediate MOV to do an abs. jump!?

*********************************************************************/

/*

Add in cpuintrf.c

#if (HAS_E132XS)
#include "cpu/e132xs/e132xs.h"
#endif

//TODO: check -> nirq,dirq,mem,shift,bits,align
#if (HAS_E132XS)
	CPU0(E132XS,   e132xs, 	 1,0,1.00, 32, 32bedw,	  0,32,BE,4, 6	),
#endif

Add in cpuintrf.h

#if (HAS_E132XS)
	CPU_E132XS,
#endif

Add in rules.mak

CPU=$(strip $(findstring E132XS@,$(CPUS)))
ifneq ($(CPU),)
OBJDIRS += $(OBJ)/cpu/e132xs
CPUDEFS += -DHAS_E132XS=1
CPUOBJS += $(OBJ)/cpu/e132xs/e132xs.o
DBGOBJS += $(OBJ)/cpu/e132xs/32xsdasm.o
$(OBJ)/cpu/e132xs/e132xs.o: e132xs.c e132xs.h
else
CPUDEFS += -DHAS_E132XS=0
endif

Add in mame.mak

CPUS+=E132XS@

*/

#include "driver.h"
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "e132xs.h"

int e132xs_ICount;
int h_clear;

void e132xs_chk(void);
void e132xs_movd(void);
void e132xs_divu(void);
void e132xs_divs(void);
void e132xs_xm(void);
void e132xs_mask(void);
void e132xs_sum(void);
void e132xs_sums(void);
void e132xs_cmp(void);
void e132xs_mov(void);
void e132xs_add(void);
void e132xs_adds(void);
void e132xs_cmpb(void);
void e132xs_andn(void);
void e132xs_or(void);
void e132xs_xor(void);
void e132xs_subc(void);
void e132xs_not(void);
void e132xs_sub(void);
void e132xs_subs(void);
void e132xs_addc(void);
void e132xs_and(void);
void e132xs_neg(void);
void e132xs_negs(void);
void e132xs_cmpi(void);
void e132xs_movi(void);
void e132xs_addi(void);
void e132xs_addsi(void);
void e132xs_cmpbi(void);
void e132xs_andni(void);
void e132xs_ori(void);
void e132xs_xori(void);
void e132xs_shrdi(void);
void e132xs_shrd(void);
void e132xs_shr(void);
void e132xs_sardi(void);
void e132xs_sard(void);
void e132xs_sar(void);
void e132xs_shldi(void);
void e132xs_shld(void);
void e132xs_shl(void);
void reserved(void);
void e132xs_testlz(void);
void e132xs_rol(void);
void e132xs_ldxx1(void);
void e132xs_ldxx2(void);
void e132xs_stxx1(void);
void e132xs_stxx2(void);
void e132xs_shri(void);
void e132xs_sari(void);
void e132xs_shli(void);
void e132xs_mulu(void);
void e132xs_muls(void);
void e132xs_set(void);
void e132xs_mul(void);
void e132xs_fadd(void);
void e132xs_faddd(void);
void e132xs_fsub(void);
void e132xs_fsubd(void);
void e132xs_fmul(void);
void e132xs_fmuld(void);
void e132xs_fdiv(void);
void e132xs_fdivd(void);
void e132xs_fcmp(void);
void e132xs_fcmpd(void);
void e132xs_fcmpu(void);
void e132xs_fcmpud(void);
void e132xs_fcvt(void);
void e132xs_fcvtd(void);
void e132xs_extend(void);
void e132xs_do(void);
void e132xs_ldwr(void);
void e132xs_lddr(void);
void e132xs_ldwp(void);
void e132xs_lddp(void);
void e132xs_stwr(void);
void e132xs_stdr(void);
void e132xs_stwp(void);
void e132xs_stdp(void);
void e132xs_dbv(void);
void e132xs_dbnv(void);
void e132xs_dbe(void);
void e132xs_dbne(void);
void e132xs_dbc(void);
void e132xs_dbnc(void);
void e132xs_dbse(void);
void e132xs_dbht(void);
void e132xs_dbn(void);
void e132xs_dbnn(void);
void e132xs_dble(void);
void e132xs_dbgt(void);
void e132xs_dbr(void);
void e132xs_frame(void);
void e132xs_call(void);
void e132xs_bv(void);
void e132xs_bnv(void);
void e132xs_be(void);
void e132xs_bne(void);
void e132xs_bc(void);
void e132xs_bnc(void);
void e132xs_bse(void);
void e132xs_bht(void);
void e132xs_bn(void);
void e132xs_bnn(void);
void e132xs_ble(void);
void e132xs_bgt(void);
void e132xs_br(void);
void e132xs_trap(void);

/* Registers */
enum
{

	E132XS_PC = 1,
	E132XS_SR,
	E132XS_FER,
	E132XS_SP,
	E132XS_UB,
	E132XS_BCR,
	E132XS_TPR,
	E132XS_TCR,
	E132XS_TR,
	E132XS_WCR,
	E132XS_ISR,
	E132XS_FCR,
	E132XS_MCR,
	E132XS_G0,  E132XS_G1,  E132XS_G2,  E132XS_G3,
	E132XS_G4,  E132XS_G5,  E132XS_G6,  E132XS_G7,
	E132XS_G8,  E132XS_G9,  E132XS_G10, E132XS_G11,
	E132XS_G12, E132XS_G13, E132XS_G14, E132XS_G15,
	E132XS_L0,  E132XS_L1,  E132XS_L2,  E132XS_L3,
	E132XS_L4,  E132XS_L5,  E132XS_L6,  E132XS_L7,
	E132XS_L8,  E132XS_L9,  E132XS_L10, E132XS_L11,
	E132XS_L12, E132XS_L13, E132XS_L14, E132XS_L15,
};

/* Internal registers */
typedef struct
{
	UINT32 global_regs[32];
	UINT32 local_regs[64]; /*stack registers which contain the most recent stack*/
						   /*current stack frame (maximun 16 registers) is always in registers*/

	/* internal stuff */
	UINT32			ppc; /*previous pc*/
	UINT16			op;	 /*opcode*/
	int				delay;
	int				delay_pc;

} e132xs_regs;

static e132xs_regs e132xs;

/* Opcodes table */
static void (*e132xs_op[0x100])(void) = {
	e132xs_chk,	 e132xs_chk,  e132xs_chk,   e132xs_chk,		/* CHK - CHKZ - NOP */
	e132xs_movd, e132xs_movd, e132xs_movd,  e132xs_movd,	/* MOVD - RET */
	e132xs_divu, e132xs_divu, e132xs_divu,  e132xs_divu,	/* DIVU */
	e132xs_divs, e132xs_divs, e132xs_divs,  e132xs_divs,	/* DIVS */
	e132xs_xm,	 e132xs_xm,   e132xs_xm,    e132xs_xm,		/* XMx - XXx */
	e132xs_mask, e132xs_mask, e132xs_mask,  e132xs_mask,	/* MASK */
	e132xs_sum,  e132xs_sum,  e132xs_sum,   e132xs_sum,		/* SUM */
	e132xs_sums, e132xs_sums, e132xs_sums,  e132xs_sums,	/* SUMS */
	e132xs_cmp,  e132xs_cmp,  e132xs_cmp,   e132xs_cmp,		/* CMP */
	e132xs_mov,  e132xs_mov,  e132xs_mov,   e132xs_mov,		/* MOV */
	e132xs_add,  e132xs_add,  e132xs_add,   e132xs_add,		/* ADD */
	e132xs_adds, e132xs_adds, e132xs_adds,  e132xs_adds,	/* ADDS */
	e132xs_cmpb, e132xs_cmpb, e132xs_cmpb,  e132xs_cmpb,	/* CMPB */
	e132xs_andn, e132xs_andn, e132xs_andn,  e132xs_andn,	/* ANDN */
	e132xs_or,   e132xs_or,   e132xs_or,    e132xs_or,		/* OR */
	e132xs_xor,  e132xs_xor,  e132xs_xor,   e132xs_xor,		/* XOR */
	e132xs_subc, e132xs_subc, e132xs_subc,  e132xs_subc,	/* SUBC */
	e132xs_not,  e132xs_not,  e132xs_not,   e132xs_not,		/* NOT */
	e132xs_sub,  e132xs_sub,  e132xs_sub,   e132xs_sub,		/* SUB */
	e132xs_subs, e132xs_subs, e132xs_subs,  e132xs_subs,	/* SUBS */
	e132xs_addc, e132xs_addc, e132xs_addc,  e132xs_addc,	/* ADDC */
	e132xs_and,  e132xs_and,  e132xs_and,   e132xs_and,		/* AND */
	e132xs_neg,  e132xs_neg,  e132xs_neg,   e132xs_neg,		/* NEG */
	e132xs_negs, e132xs_negs, e132xs_negs,  e132xs_negs,	/* NEGS */
	e132xs_cmpi, e132xs_cmpi, e132xs_cmpi,  e132xs_cmpi,	/* CMPI */
	e132xs_movi, e132xs_movi, e132xs_movi,  e132xs_movi,	/* MOVI */
	e132xs_addi, e132xs_addi, e132xs_addi,  e132xs_addi,	/* ADDI */
	e132xs_addsi,e132xs_addsi,e132xs_addsi, e132xs_addsi,	/* ADDSI */
	e132xs_cmpbi,e132xs_cmpbi,e132xs_cmpbi, e132xs_cmpbi,	/* CMPBI */
	e132xs_andni,e132xs_andni,e132xs_andni, e132xs_andni,	/* ANDNI */
	e132xs_ori,  e132xs_ori,  e132xs_ori,   e132xs_ori,		/* ORI */
	e132xs_xori, e132xs_xori, e132xs_xori,  e132xs_xori,	/* XORI */
	e132xs_shrdi,e132xs_shrdi,e132xs_shrd,  e132xs_shr,		/* SHRDI, SHRD, SHR */
	e132xs_sardi,e132xs_sardi,e132xs_sard,  e132xs_sar,		/* SARDI, SARD, SAR */
	e132xs_shldi,e132xs_shldi,e132xs_shld,  e132xs_shl,		/* SHLDI, SHLD, SHL */
	reserved,    reserved,	  e132xs_testlz,e132xs_rol,		/* RESERVED, TESTLZ, ROL */
	e132xs_ldxx1,e132xs_ldxx1,e132xs_ldxx1, e132xs_ldxx1,	/* LDxx.D/A/IOD/IOA */
	e132xs_ldxx2,e132xs_ldxx2,e132xs_ldxx2, e132xs_ldxx2,	/* LDxx.N/S */
	e132xs_stxx1,e132xs_stxx1,e132xs_stxx1,e132xs_stxx1,	/* STxx.D/A/IOD/IOA */
	e132xs_stxx2,e132xs_stxx2,e132xs_stxx2,e132xs_stxx2,	/* STxx.N/S */
	e132xs_shri, e132xs_shri, e132xs_shri,  e132xs_shri,	/* SHRI */
	e132xs_sari, e132xs_sari, e132xs_sari,  e132xs_sari,	/* SARI */
	e132xs_shli, e132xs_shli, e132xs_shli,  e132xs_shli,	/* SHLI */
	reserved,    reserved,    reserved,     reserved,		/* RESERVED */
	e132xs_mulu, e132xs_mulu, e132xs_mulu,  e132xs_mulu,	/* MULU */
	e132xs_muls, e132xs_muls, e132xs_muls,  e132xs_muls,	/* MULS */
	e132xs_set,  e132xs_set,  e132xs_set,   e132xs_set,		/* SETxx - SETADR - FETCH */
	e132xs_mul,  e132xs_mul,  e132xs_mul,   e132xs_mul,		/* MUL */
	e132xs_fadd, e132xs_faddd,e132xs_fsub,  e132xs_fsubd,	/* FADD, FADDD, FSUB, FSUBD */
	e132xs_fmul, e132xs_fmuld,e132xs_fdiv,  e132xs_fdivd,	/* FMUL, FMULD, FDIV, FDIVD */
	e132xs_fcmp, e132xs_fcmpd,e132xs_fcmpu, e132xs_fcmpud,	/* FCMP, FCMPD, FCMPU, FCMPUD */
	e132xs_fcvt, e132xs_fcvtd,e132xs_extend,e132xs_do,		/* FCVT, FCVTD, EXTEND, DO */
	e132xs_ldwr, e132xs_ldwr, e132xs_lddr,  e132xs_lddr,	/* LDW.R, LDD.R */
	e132xs_ldwp, e132xs_ldwp, e132xs_lddp,  e132xs_lddp,	/* LDW.P, LDD.P */
	e132xs_stwr, e132xs_stwr, e132xs_stdr,  e132xs_stdr,	/* STW.R, STD.R */
	e132xs_stwp, e132xs_stwp, e132xs_stdp,  e132xs_stdp,	/* STW.P, STD.P */
	e132xs_dbv,  e132xs_dbnv, e132xs_dbe,   e132xs_dbne,	/* DBV, DBNV, DBE, DBNE */
	e132xs_dbc,  e132xs_dbnc, e132xs_dbse,  e132xs_dbht,	/* DBC, DBNC, DBSE, DBHT */
	e132xs_dbn,  e132xs_dbnn, e132xs_dble,  e132xs_dbgt,	/* DBN, DBNN, DBLE, DBGT */
	e132xs_dbr,  e132xs_frame,e132xs_call,  e132xs_call,	/* DBR, FRAME, CALL */
	e132xs_bv,   e132xs_bnv,  e132xs_be,    e132xs_bne,		/* BV, BNV, BE, BNE */
	e132xs_bc,   e132xs_bnc,  e132xs_bse,   e132xs_bht,		/* BC, BNC, BSE, BHT */
	e132xs_bn,   e132xs_bnn,  e132xs_ble,   e132xs_bgt,		/* BN, BNN, BLE, BGT */
	e132xs_br,   e132xs_trap, e132xs_trap,  e132xs_trap		/* BR, TRAPxx - TRAP */
};

static UINT32 entry_point = 0xffffff00;	/*default @ MEM3*/

/* Return the entry point for a determinated trap */
UINT32 get_trap_addr(UINT8 trap_no)
{
	UINT32 addr;
	if( entry_point & 0xffffff00 ) /* @ MEM3 */
	{
		addr = trap_no * 4;
	}
	else
	{
		addr = (63 - trap_no) * 4;
	}
	addr |= entry_point;
	return addr;
}

/* Return the entry point for a determinated emulated code (the one for "extend" opcode is reserved) */
UINT32 get_emu_code_addr(UINT8 num) /* num is OP */
{
	UINT32 addr;
	if( entry_point & 0xffffff00 ) /* @ MEM3 */
	{
		addr = (entry_point - 0x100) | (num << 4);
	}
	else
	{
		addr = entry_point | (0x10c | ((0x0f - num) << 4));
	}
	return addr;
}

#define OP					e132xs.op

/* TODO: i need to use these 3 define?*/
#define SET_PC(val)		PC = (val & 0xfffffffe) /*PC(0) = 0*/
#define SET_SP(val)		SP = (val & 0xfffffffc) /*SP(0) = SP(1) = 0*/
#define SET_UB(val)		UB = (val & 0xfffffffc) /*UB(0) = UB(1) = 0*/

#define PPC				e132xs.ppc /*previous pc*/
#define PC				e132xs.global_regs[0] /*Program Counter*/
#define SR				e132xs.global_regs[1] /*Status Register*/
#define FER				e132xs.global_regs[2] /*Floating-Point Exception Register*/
/*#define ?? global_regs[3 - 15] */ /*General Purpose Registers*/
/*#define ?? global_regs[16] */ /*reserved*/
/*#define ?? global_regs[17] */ /*reserved*/
#define SP				e132xs.global_regs[18] /*Stack Pointer*/
#define UB				e132xs.global_regs[19] /*Upper Stack Bound*/
#define BCR				e132xs.global_regs[20] /*Bus Control Register*/
#define TPR				e132xs.global_regs[21] /*Timer Prescaler Register*/
#define TCR				e132xs.global_regs[22] /*Timer Compare Register*/
#define TR				e132xs.global_regs[23] /*Timer Register*/
#define WCR				e132xs.global_regs[24] /*Watchdog Compare Register*/
#define ISR				e132xs.global_regs[25] /*Input Status Register*/
#define FCR				e132xs.global_regs[26] /*Function Control Register*/
#define MCR				e132xs.global_regs[27] /*Memory Control Register*/
/*#define ??			e132xs.global_regs[28] */ /*reserved*/
/*#define ??			e132xs.global_regs[29] */ /*reserved*/
/*#define ??			e132xs.global_regs[30] */ /*reserved*/
/*#define ??			e132xs.global_regs[31] */ /*reserved*/

/* Registers Number	*/
#define REG_BCR			20
#define REG_TPR			21
#define REG_FCR			26
#define REG_MCR			27

#define GET_G_REG(code)			e132xs.global_regs[code]
#define GET_L_REG(code)			e132xs.local_regs[code]
#define SET_G_REG(code,val)		e132xs.global_regs[code] = val
#define SET_L_REG(code,val)		e132xs.local_regs[code] = val

#define S_BIT					((OP & 0x100) >> 8)
#define N_BIT					S_BIT	/*it's the same bit but with different name and use*/
#define D_BIT					((OP & 0x200) >> 9)
#define N_VALUE					((N_BIT << 4 ) | (OP & 0x0f))
#define D_CODE					((OP & 0xf0) >> 4)
#define S_CODE					(OP & 0x0f)
#define SIGN_BIT(val)			((val & 0x80000000) >> 31)
#define NOINC					0
#define INC						1

#define SET_RD(val,inc)										\
		if( D_BIT )											\
		{													\
			SET_L_REG(D_CODE + inc, val);					\
		}													\
		else												\
		{													\
			SET_G_REG(D_CODE + inc, val);					\
		}

#define SET_LD(val,inc)										\
		if( !D_CODE )										\
			SET_L_REG(16 + inc, val);						\
		else												\
			SET_L_REG(D_CODE + inc, val);

#define SET_RS(val,inc)										\
		if( S_BIT )											\
		{													\
			SET_L_REG(S_CODE + inc, val);					\
		}													\
		else												\
		{													\
			SET_G_REG(D_CODE + inc, val);	\
		}

/* SR flags */
#define GET_C					( SR & 0x00000001) /*bit 0 */ /*CARRY*/
#define GET_Z					((SR & 0x00000002)>>1) /*bit 1 */ /*ZERO*/
#define GET_N					((SR & 0x00000004)>>2) /*bit 2 */ /*NEGATIVE*/
#define GET_V					((SR & 0x00000008)>>3) /*bit 3 */ /*OVERFLOW*/
#define GET_M					((SR & 0x00000010)>>4) /*bit 4 */ /*CACHE-MODE*/
#define GET_H					((SR & 0x00000020)>>5) /*bit 5 */ /*HIGHGLOBAL*/
/*#define RESERVED				((SR & 0x00000040)>>6) */ /*bit 6 //always 0*/
#define GET_I					((SR & 0x00000080)>>7) /*bit 7 */ /*INTERRUPT-MODE*/
#define GET_FTE					((SR & 0x00001f00)>>8) /*bits 12 - 8 	*/ /*Floating-Point Trap Enable*/
#define	GET_FRM					((SR & 0x00006000)>>13) /*bits 14 - 13 */ /*Floating-Point Rounding Mode*/
#define GET_L					((SR & 0x00008000)>>15) /*bit 15 */ /*INTERRUPT-LOCK*/
#define GET_T					((SR & 0x00010000)>>16) /*bit 16 */ /*TRACE-MODE*/
#define GET_P					((SR & 0x00020000)>>17) /*bit 17 */ /*TRACE PENDING*/
#define GET_S					((SR & 0x00040000)>>18) /*bit 18 */ /*SUPERVISOR STATE*/
#define GET_ILC					((SR & 0x00180000)>>19) /*bits 20 - 19 */ /*INSTRUCTION-LENGTH*/
#define GET_FL					((SR & 0x01e00000)>>21) /*bits 24 - 21 */ /*FRAME LENGTH*/
#define GET_FP					((SR & 0xfe000000)>>25) /*bits 31 - 25 */ /*FRAME POINTER*/

/* TODO: do i need to use this?*/
#define SET_SR(val)				(SR = (SR & 0xffff0000) | (val & 0x0000ffff)) /*when SR is addressed, only low 15 bits can be changed*/

#define SET_C(val)				(SR = (SR & 0xfffffffe) |  val)
#define SET_Z(val)				(SR = (SR & 0xfffffffd) | (val << 1))
#define SET_N(val)				(SR = (SR & 0xfffffffb) | (val << 2))
#define SET_V(val)				(SR = (SR & 0xfffffff7) | (val << 3))
#define SET_M(val)				(SR = (SR & 0xffffffef) | (val << 4))
#define SET_H(val)				(SR = (SR & 0xffffffdf) | (val << 5))
/*#define RESERVED*/
#define SET_I(val)				(SR = (SR & 0xffffff7f) | (val << 7))
#define SET_FTE(val)			(SR = (SR & 0xffffe0ff) | (val << 8))
#define	SET_FRM(val)			(SR = (SR & 0xffff9fff) | (val << 13))
#define SET_L(val)				(SR = (SR & 0xffff7fff) | (val << 15))
#define SET_T(val)				(SR = (SR & 0xfffeffff) | (val << 16))
#define SET_P(val)				(SR = (SR & 0xfffdffff) | (val << 17))
#define SET_S(val)				(SR = (SR & 0xfffbffff) | (val << 18))
#define SET_ILC(val)			(SR = (SR & 0xffe7ffff) | (val << 19))
#define SET_FL(val)				(SR = (SR & 0xfe1fffff) | (val << 21))
#define SET_FP(val)				(SR = (SR & 0x1fffffff) | (val << 25))

/* TODO: do i need to use these?*/
/* FER flags */
#define GET_ACCRUED				(FER & 0x0000001f) /*bits  4 - 0 */ /*Floating-Point Accrued Exceptions*/
#define GET_ACTUAL				(FER & 0x00001f00) /*bits 12 - 8 */ /*Floating-Point Actual  Exceptions*/
/*other bits are reversed, in particular 7 - 5 for the operating system.*/
/*the user program can only changes the above 2 flags*/


static UINT8 e132xs_reg_layout[] =
{
	E132XS_PC,  E132XS_SR,  E132XS_FER, E132XS_SP,  E132XS_UB,  -1,
	E132XS_BCR, E132XS_ISR, E132XS_FCR, E132XS_MCR, -1,
	E132XS_TR,  E132XS_TPR, E132XS_TCR, E132XS_WCR, -1,
	E132XS_G0,  E132XS_G1,  E132XS_G2,  E132XS_G3,  -1,
	E132XS_G4,  E132XS_G5,  E132XS_G6,  E132XS_G7,  -1,
	E132XS_G8,  E132XS_G9,  E132XS_G10, E132XS_G11, -1,
	E132XS_G12, E132XS_G13, E132XS_G14, E132XS_G15, -1,
	E132XS_L0,  E132XS_L1,  E132XS_L2,  E132XS_L3,  -1,
	E132XS_L4,  E132XS_L5,  E132XS_L6,  E132XS_L7,  -1,
	E132XS_L8,  E132XS_L9,  E132XS_L10, E132XS_L11, -1,
	E132XS_L12, E132XS_L13, E132XS_L14, E132XS_L15, 0
};

UINT8 e132xs_win_layout[] =
{
	0, 0,80, 8, /* register window (top rows) */
	0, 9,34,13, /* disassembler window (left, middle columns) */
	35, 9,46, 6, /* memory #1 window (right, upper middle) */
	35,16,46, 6, /* memory #2 window (right, lower middle) */
	0,23,80, 1  /* command line window (bottom row) */
};

void e132xs_set_entry_point(int which)
{
	switch( which )
	{
		case E132XS_ENTRY_MEM0:
			entry_point = 0x00000000;
			break;

		case E132XS_ENTRY_MEM1:
			entry_point = 0x40000000;
			break;

		case E132XS_ENTRY_MEM2:
			entry_point = 0x80000000;
			break;

		case E132XS_ENTRY_MEM3:
			entry_point = 0xffffff00;
			break;

		case E132XS_ENTRY_IRAM:
			entry_point = 0xc0000000;
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "E1-32XS: Entry Point Error. Target not defined (= %d)\n",which);
			break;
	}
}

INT32 immediate_value(void)
{
	INT16 imm1, imm2;
	INT32 ret;

	switch( N_VALUE )
	{
		case 0:	case 1:  case 2:  case 3:  case 4:  case 5:  case 6:  case 7: case 8:
		case 9:	case 10: case 11: case 12: case 13: case 14: case 15: case 16:
			return N_VALUE;

		case 17:
			PC += 2;
			imm1 = READ_OP(PC);
			PC += 2;
			imm2 = READ_OP(PC);
			ret = (imm1 << 16) | imm2;
			return ret;


		case 18:
			PC += 2;
			ret = (UINT32) READ_OP(PC);
			return ret;

		case 19:
			PC += 2;
			ret = 0xffff0000 | ((INT32) READ_OP(PC));
			return ret;

		case 20:
			return 32;	/*bit 5 = 1, others = 0*/

		case 21:
			return 64;	/*bit 6 = 1, others = 0*/

		case 22:
			return 128; /*bit 7 = 1, others = 0*/

		case 23:
			return 0x80000000; /*bit 31 = 1, others = 0 (2 at the power of 31)*/

		case 24:
			return -8;

		case 25:
			return -7;

		case 26:
			return -6;

		case 27:
			return -5;

		case 28:
			return -4;

		case 29:
			return -3;

		case 30:
			return -2;

		case 31:
			return -1;
	}

	return 0; /*it should never executed*/
}

INT32 get_const(void)
{
	INT32 const_val;
	INT16 imm1;

	PC += 2;
	imm1 = READ_OP(PC);

	if( E_BIT(imm1) )
	{
		INT16 imm2;

		PC += 2;
		imm2 = READ_OP(PC);

		const_val = imm2;
		const_val |= ((imm1 & 0x3fff) << 16 );

		if( S_BIT_CONST(imm1) )
		{
			const_val |= 0xc0000000;
		}
	}
	else
	{
		const_val = imm1 & 0x3fff;

		if( S_BIT_CONST(imm1) )
		{
			const_val |= 0xffffc000;
		}
	}

	return const_val;
}

INT32 get_pcrel(void)
{
	INT32 ret;

	if( OP & 0x80 )
	{
		UINT16 next;
		PC += 2;
		next = READ_OP(PC);

		ret = (OP & 0x7f) << 16;

		ret |= (next & 0xfffe);

		if( next & 1 )
			ret |= 0xff800000;
	}
	else
	{
		ret = OP & 0x7e;

		if( OP & 1 )
			ret |= 0xffffff80;
	}

	return ret;
}

INT32 get_dis(UINT32 val )
{
	INT32 ret;

	if( E_BIT(val) )
	{
		UINT16 next;

		PC += 2;

		next = READ_OP(PC);

		ret = next;
		ret |= ( ( val & 0xfff ) << 16 );

		if( S_BIT_CONST(val) )
		{
			ret |= 0xf0000000;
		}
	}
	else
	{
		ret = val & 0xfff;
		if( S_BIT_CONST(val) )
		{
			ret |= 0xfffff000;
		}
	}

	return ret;
}

void execute_br(INT32 rel)
{
	/* Reip - TODO: if in get_pcrel() is read the next pc, do i need to increment the pc?*/
	/* MooglyGuy - No, you need to decrement it by two, since after e132xs_execute() you'll
	               be incrementing PC by two. If you don't decrement, it'll skip over the
	               next opcode. See below. */
	PPC = PC;
	PC += rel;
/*	PC -= 2;*/
	SET_M(0);

	/*TODO: change when there's latency*/
/*	if target is on-chip cache*/
	e132xs_ICount -= 2;
/*	else 2 + memory read latency*/

}

void execute_dbr(INT32 rel)
{
	/* Reip - TODO: if in get_pcrel() is read the next pc, do i need to increment the pc?*/
	/*              block all the exception except the reset*/
	/* MooglyGuy - Well, I'm not quite sure about that. */

	e132xs.delay_pc = PC + rel;
	e132xs.delay    = DELAY_TAKEN;

	/*TODO: change when there's latency*/
/*	if target in on-chip cache*/
	e132xs_ICount -= 1;
/*	else 1 + memory read latency cycles exceeding (dealy instruction cycles - 1)*/
}

void execute_trap(UINT32 addr)
{
	UINT32 write_addr;

	write_addr = GET_FP + GET_FL;
	WRITE_W(write_addr, (PC & 0xfffffffe) | GET_S);
	WRITE_W(write_addr + 1, SR); /*TODO: or +4 because it's 32bit?*/
	SET_FP(write_addr);
	SET_FL(6);
	SET_M(0);
	SET_T(0);
	SET_L(1);
	SET_S(1);
	PPC = PC;
	PC = addr;
	PC -= 2;

	e132xs_ICount -= 2;	/* TODO: + delay...*/
}

void e132xs_init(void)
{
	int cpu = cpu_getactivecpu();

	/*TODO: add the reserved registers too? and the local registers too?*/

	state_save_register_UINT32("e132xs", cpu, "PC",  &PC,  1);
	state_save_register_UINT32("e132xs", cpu, "SR",  &SR,  1);
	state_save_register_UINT32("e132xs", cpu, "FER", &FER, 1);
	state_save_register_UINT32("e132xs", cpu, "SP",  &SP,  1);
	state_save_register_UINT32("e132xs", cpu, "UB",  &UB,  1);
	state_save_register_UINT32("e132xs", cpu, "BCR", &BCR, 1);
	state_save_register_UINT32("e132xs", cpu, "TPR", &TPR, 1);
	state_save_register_UINT32("e132xs", cpu, "TCR", &TCR, 1);
	state_save_register_UINT32("e132xs", cpu, "TR",  &TR,  1);
	state_save_register_UINT32("e132xs", cpu, "WCR", &WCR, 1);
	state_save_register_UINT32("e132xs", cpu, "ISR", &ISR, 1);
	state_save_register_UINT32("e132xs", cpu, "FCR", &FCR, 1);
	state_save_register_UINT32("e132xs", cpu, "MCR", &MCR, 1);
}

void e132xs_reset(void *param)
{
	/*TODO: other to do at reset?*/

	SET_S(1);
	SET_FL(6);
	SET_FP(1);

	PC = get_trap_addr(RESET);
}

void e132xs_exit(void)
{
	/* nothing */
}

int e132xs_execute(int cycles)
{
	e132xs_ICount = cycles;

	do
	{
		PPC = PC;	/* copy PC to previous PC */

		if( e132xs.delay == DELAY_EXECUTE )
		{
			/*TODO: should i use change_pc32bedw or something similar?*/
			PC = e132xs.delay_pc;
			e132xs.delay_pc = 0;
			e132xs.delay = NO_DELAY;
		}

		CALL_MAME_DEBUG;

		OP = READ_OP(PC);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Executing opcode %04x at PC %08x\n", OP, PC );

		if(GET_H)
		{
			h_clear = 1;
		}

		e132xs_op[(OP & 0xff00) >> 8]();

		if(h_clear == 1)
		{
			SET_H(0);
			h_clear = 0; 
		}

		PC += 2;

		if( e132xs.delay == DELAY_TAKEN )
		{
			e132xs.delay--;
		}

	} while( e132xs_ICount > 0 );

	return cycles - e132xs_ICount;  /*TODO: check this*/
}

unsigned e132xs_get_context(void *regs)
{
	/* copy the context */
	if( regs )
		*(e132xs_regs *)regs = e132xs;

	/* return the context size */
	return sizeof(e132xs_regs);
}

void e132xs_set_context(void *regs)
{
	/* copy the context */
	if (regs)
		e132xs = *(e132xs_regs *)regs;

	/*TODO: other to do? check interrupt?*/

}

unsigned e132xs_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:
		case E132XS_PC:			return PC;
		case E132XS_SR:			return SR;
		case E132XS_FER:		return FER;
		case REG_SP:
		case E132XS_SP:			return SP;
		case E132XS_UB:			return UB;
		case E132XS_BCR:		return BCR;
		case E132XS_TPR:		return TPR;
		case E132XS_TCR:		return TCR;
		case E132XS_TR:			return TR;
		case E132XS_WCR:		return WCR;
		case E132XS_ISR:		return ISR;
		case E132XS_FCR:		return FCR;

		case REG_PREVIOUSPC:	return PPC;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				/*TODO: add stack*/
			}
	}

	return 0;
}

void e132xs_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:
		case E132XS_PC:			PC  = val;	break;
		case E132XS_SR:			SR  = val;	break;
		case E132XS_FER:		FER = val;	break;
		case REG_SP:
		case E132XS_SP:			SP  = val;	break;
		case E132XS_UB:			UB  = val;	break;
		case E132XS_BCR:		BCR = val;	break;
		case E132XS_TPR:		TPR = val;	break;
		case E132XS_TCR:		TCR = val;	break;
		case E132XS_TR:			TR  = val;	break;
		case E132XS_WCR:		WCR = val;	break;
		case E132XS_ISR:		ISR = val;	break;
		case E132XS_FCR:		FCR = val;	break;

		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				/*TODO: add stack*/
			}
	}

}

void e132xs_set_irq_line(int irqline, int state)
{
}

void e132xs_set_irq_callback(int (*callback)(int irqline))
{
}

const char *e132xs_info(void *context, int regnum)
{
	static char buffer[16][47+1]; /* TODO: check the length!*/
	static int which = 0;
	e132xs_regs *r = context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &e132xs;

	switch (regnum)
	{
		case CPU_INFO_REG+E132XS_PC:  sprintf(buffer[which], "PC:%08X",  r->global_regs[0]); break;
		case CPU_INFO_REG+E132XS_SR:  sprintf(buffer[which], "SR:%08X",  r->global_regs[1]); break;
		case CPU_INFO_REG+E132XS_FER: sprintf(buffer[which], "FER:%08X", r->global_regs[2]); break;
		case CPU_INFO_REG+E132XS_SP:  sprintf(buffer[which], "SP:%08X",  r->global_regs[18]); break;
		case CPU_INFO_REG+E132XS_UB:  sprintf(buffer[which], "UB:%08X",  r->global_regs[19]); break;
		case CPU_INFO_REG+E132XS_BCR: sprintf(buffer[which], "BCR:%08X", r->global_regs[20]); break;
		case CPU_INFO_REG+E132XS_TPR: sprintf(buffer[which], "TPR:%08X", r->global_regs[21]); break;
		case CPU_INFO_REG+E132XS_TCR: sprintf(buffer[which], "TCR:%08X", r->global_regs[22]); break;
		case CPU_INFO_REG+E132XS_TR:  sprintf(buffer[which], "TR:%08X",  r->global_regs[23]); break;
		case CPU_INFO_REG+E132XS_WCR: sprintf(buffer[which], "WCR:%08X", r->global_regs[24]); break;
		case CPU_INFO_REG+E132XS_ISR: sprintf(buffer[which], "ISR:%08X", r->global_regs[25]); break;
		case CPU_INFO_REG+E132XS_FCR: sprintf(buffer[which], "FCR:%08X", r->global_regs[26]); break;
		case CPU_INFO_REG+E132XS_MCR: sprintf(buffer[which], "MCR:%08X", r->global_regs[27]); break;
		case CPU_INFO_REG+E132XS_G0:  sprintf(buffer[which], "G0 :%08X", r->global_regs[0]); break;
		case CPU_INFO_REG+E132XS_G1:  sprintf(buffer[which], "G1 :%08X", r->global_regs[1]); break;
		case CPU_INFO_REG+E132XS_G2:  sprintf(buffer[which], "G2 :%08X", r->global_regs[2]); break;
		case CPU_INFO_REG+E132XS_G3:  sprintf(buffer[which], "G3 :%08X", r->global_regs[3]); break;
		case CPU_INFO_REG+E132XS_G4:  sprintf(buffer[which], "G4 :%08X", r->global_regs[4]); break;
		case CPU_INFO_REG+E132XS_G5:  sprintf(buffer[which], "G5 :%08X", r->global_regs[5]); break;
		case CPU_INFO_REG+E132XS_G6:  sprintf(buffer[which], "G6 :%08X", r->global_regs[6]); break;
		case CPU_INFO_REG+E132XS_G7:  sprintf(buffer[which], "G7 :%08X", r->global_regs[7]); break;
		case CPU_INFO_REG+E132XS_G8:  sprintf(buffer[which], "G8 :%08X", r->global_regs[8]); break;
		case CPU_INFO_REG+E132XS_G9:  sprintf(buffer[which], "G9 :%08X", r->global_regs[9]); break;
		case CPU_INFO_REG+E132XS_G10: sprintf(buffer[which], "G10:%08X", r->global_regs[10]); break;
		case CPU_INFO_REG+E132XS_G11: sprintf(buffer[which], "G11:%08X", r->global_regs[11]); break;
		case CPU_INFO_REG+E132XS_G12: sprintf(buffer[which], "G12:%08X", r->global_regs[12]); break;
		case CPU_INFO_REG+E132XS_G13: sprintf(buffer[which], "G13:%08X", r->global_regs[13]); break;
		case CPU_INFO_REG+E132XS_G14: sprintf(buffer[which], "G14:%08X", r->global_regs[14]); break;
		case CPU_INFO_REG+E132XS_G15: sprintf(buffer[which], "G15:%08X", r->global_regs[15]); break;
		case CPU_INFO_REG+E132XS_L0:  sprintf(buffer[which], "L0 :%08X", r->local_regs[0]); break;
		case CPU_INFO_REG+E132XS_L1:  sprintf(buffer[which], "L1 :%08X", r->local_regs[1]); break;
		case CPU_INFO_REG+E132XS_L2:  sprintf(buffer[which], "L2 :%08X", r->local_regs[2]); break;
		case CPU_INFO_REG+E132XS_L3:  sprintf(buffer[which], "L3 :%08X", r->local_regs[3]); break;
		case CPU_INFO_REG+E132XS_L4:  sprintf(buffer[which], "L4 :%08X", r->local_regs[4]); break;
		case CPU_INFO_REG+E132XS_L5:  sprintf(buffer[which], "L5 :%08X", r->local_regs[5]); break;
		case CPU_INFO_REG+E132XS_L6:  sprintf(buffer[which], "L6 :%08X", r->local_regs[6]); break;
		case CPU_INFO_REG+E132XS_L7:  sprintf(buffer[which], "L7 :%08X", r->local_regs[7]); break;
		case CPU_INFO_REG+E132XS_L8:  sprintf(buffer[which], "L8 :%08X", r->local_regs[8]); break;
		case CPU_INFO_REG+E132XS_L9:  sprintf(buffer[which], "L9 :%08X", r->local_regs[9]); break;
		case CPU_INFO_REG+E132XS_L10: sprintf(buffer[which], "L10:%08X", r->local_regs[10]); break;
		case CPU_INFO_REG+E132XS_L11: sprintf(buffer[which], "L11:%08X", r->local_regs[11]); break;
		case CPU_INFO_REG+E132XS_L12: sprintf(buffer[which], "L12:%08X", r->local_regs[12]); break;
		case CPU_INFO_REG+E132XS_L13: sprintf(buffer[which], "L13:%08X", r->local_regs[13]); break;
		case CPU_INFO_REG+E132XS_L14: sprintf(buffer[which], "L14:%08X", r->local_regs[14]); break;
		case CPU_INFO_REG+E132XS_L15: sprintf(buffer[which], "L15:%08X", r->local_regs[15]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c FTE:%x FRM:%x ILC:%x FL:%x FP:%x",
				r->global_regs[1] & 0x40000 ? 'S':'.',
				r->global_regs[1] & 0x20000 ? 'P':'.',
				r->global_regs[1] & 0x10000 ? 'T':'.',
				r->global_regs[1] & 0x80000 ? 'L':'.',
				r->global_regs[1] & 0x00080 ? 'I':'.',
				r->global_regs[1] & 0x00040 ? '?':'.',
				r->global_regs[1] & 0x00020 ? 'H':'.',
				r->global_regs[1] & 0x00010 ? 'M':'.',
				r->global_regs[1] & 0x00008 ? 'V':'.',
				r->global_regs[1] & 0x00004 ? 'N':'.',
				r->global_regs[1] & 0x00002 ? 'Z':'.',
				r->global_regs[1] & 0x00001 ? 'C':'.',
				(r->global_regs[1] & 0x00001f00)>>8,
				(r->global_regs[1] & 0x00006000)>>13,
				(r->global_regs[1] & 0x00180000)>>19,
				(r->global_regs[1] & 0x01e00000)>>21,
				(r->global_regs[1] & 0xfe000000)>>25);
			break;

		case CPU_INFO_NAME: return "E1-32XS";
		case CPU_INFO_FAMILY: return "Hyperstone E1-32XS";
		case CPU_INFO_VERSION: return "0.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright Pierpaolo Prazzoli and Ryan Holtz";
		case CPU_INFO_REG_LAYOUT: return (const char *)e132xs_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)e132xs_win_layout;
	}
	return buffer[which];
}

unsigned e132xs_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return dasm_e132xs( buffer, pc );
#else
	sprintf(buffer, "$%08x", READ_OP(pc));
	return 1;
#endif
}


/* Opcodes */
/*TODO: Add the opcodes inline?*/

void e132xs_chk(void)
{
	UINT32 val1, val2;

	if( S_BIT )
	{
		val1 = GET_L_REG(S_CODE);
	}
	else
	{
		val1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		val2 = GET_L_REG(D_CODE);
	}
	else
	{
		val2 = GET_G_REG(D_CODE);
	}

	/*TODO: test it with Rs = PC and CHK, PC, PC*/

	/*if CHK, L0, L0 -> NOP, only in the debugger, here it's the same*/
	if( (!(S_CODE == SR_CODE && !S_BIT) && (val2 > val1)) || ((S_CODE == SR_CODE && !S_BIT) && (val2 == 0)) )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}
	if((S_CODE == PC_CODE && !S_BIT) && (D_CODE == PC_CODE && !D_BIT))
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}

	e132xs_ICount -= 1;
}

void e132xs_movd(void)
{
	/*Rd denotes PC*/
	if( D_CODE == PC_CODE && !D_BIT )
	{
		/* RET instruction*/

		unsigned char old_s, old_l;
		UINT8 difference; /*really it's 7 bits*/

		if( (S_CODE == PC_CODE && !S_BIT) || (S_CODE == SR_CODE && !S_BIT) )
		{	/*future expansion*/
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC or SR used in RET instruction @ %x\n", PC );
		}
		else
		{
			old_s = GET_S;
			old_l = GET_L;
			PPC = PC;

			if( S_BIT )
			{
				PC = GET_L_REG(S_CODE) & 0xfffffffe;
				SR = (GET_L_REG(S_CODE + INC) & 0xffe00000) | ((GET_L_REG(S_CODE) & 0x01) << 18 ) | (GET_L_REG(S_CODE + INC) & 0x3ffff);
				SET_S(GET_L_REG(S_CODE) & 0x01);
			}
			else
			{
				PC = GET_G_REG(S_CODE) & 0xfffffffe;
				SR = (GET_G_REG(S_CODE + INC) & 0xffe00000) | ((GET_G_REG(S_CODE) & 0x01) << 18 ) | (GET_G_REG(S_CODE + INC) & 0x3ffff);
				SET_S(GET_G_REG(S_CODE) & 0x01);
			}

			SET_ILC(0);

			if( (!old_s && GET_S) || (!GET_S && !old_l && GET_L))
			{
				UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
				execute_trap(addr);
			}

			difference = GET_FP - ( ( SP & 0x1fc ) >> 2 );

			/* bit 6 of difference is sign*/
			if( difference & 0x40 ) /*else it's finished*/
			{
				do
				{
					SP -= 4;

					SET_L_REG( ( SP & 0xfc ), READ_W( SP ) );

					difference++;
					difference = ((difference & 0x7f) + ((difference & 0x80) >> 7)) & 0x7f; /*TODO: ok?*/

				} while( !( difference & 0x40 ) );
			}
		}

		/*TODO: no 1!*/
		e132xs_ICount -= 1;
	}
	/*Rd doesn't denote PC and Rs denotes SR*/
	else if( S_CODE == SR_CODE && !S_BIT )
	{
		SET_RD(0, NOINC);
		SET_RD(0, INC);
		SET_Z(1);
		SET_N(0);
		/*SET_V(); */ /*undefined*/

		e132xs_ICount -= 2;
	}
	/*Rd doesn't denote PC and Rs doesn't denote SR*/
	else
	{
		UINT32 val1, val2;
		UINT64 tmp;

		if( S_BIT )
		{
			val1 = GET_L_REG(S_CODE);
			val2 = GET_L_REG(S_CODE + INC);
		}
		else
		{
			val1 = GET_G_REG(S_CODE);
			val2 = GET_G_REG(S_CODE + INC);
		}

		SET_RD(val1, NOINC);
		SET_RD(val2, INC);
		tmp = COMBINE_U64_U32_U32(val1, val2);
		SET_Z(tmp == 0 ? 1 : 0);
		SET_N(SIGN_BIT(val1));
		/*SET_V(); */ /*undefined*/

		e132xs_ICount -= 2;
	}
}

void e132xs_divu(void)
{
	UINT64 dividend;
	UINT32 dividend_low, dividend_high;
	UINT32 divisor;

	/*TODO: can D_CODE be PC or SR?*/

	if( S_CODE == D_CODE && S_CODE == (D_CODE + INC) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted the same register code in DIVU instruction @ %x\n", PC );
	}
	else
	{
		if( (S_CODE == PC_CODE && !S_BIT) && (S_CODE == SR_CODE && !S_BIT) )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC / SR as source register in DIVU instruction @ %x\n", PC );
		}
		else
		{
			if( S_BIT )
			{
				divisor = GET_L_REG(S_CODE);
			}
			else
			{
				divisor = GET_G_REG(S_CODE);
			}

			if( D_BIT )
			{
				dividend_high = GET_L_REG(D_CODE);
				dividend_low  = GET_L_REG(D_CODE + INC);
			}
			else
			{
				dividend_high = GET_G_REG(D_CODE);
				dividend_low  = GET_G_REG(D_CODE + INC);
			}

			dividend = COMBINE_U64_U32_U32(dividend_high, dividend_low);

			if( divisor == 0 || dividend > 0xffffffff )
			{
				/*Rd*/ /*Rdf -> undefined*/
				/*Z -> undefined*/
				/*N -> undefined*/
				UINT32 addr;
				SET_V(1);
				addr = get_trap_addr(RANGE_ERROR);
				execute_trap(addr);
			}
			else
			{
				UINT32 quotient, remainder;

				quotient = dividend / divisor;
				remainder = dividend % divisor;

				SET_RD( remainder, NOINC );
				SET_RD( quotient, INC );
				SET_Z((quotient == 0 ? 1 : 0));
				SET_N(SIGN_BIT(quotient));
				SET_V(0);
			}
		}
	}

	e132xs_ICount -= 36;
}

void e132xs_divs(void)
{
	INT64 dividend;
	INT32 dividend_low, dividend_high;
	INT32 divisor;

	/*TODO: can D_CODE be PC or SR?*/

	if( S_CODE == D_CODE && S_CODE == (D_CODE + INC) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted the same register code in DIVS instruction @ %x\n", PC );
	}
	else
	{
		if( (S_CODE == PC_CODE && !S_BIT) && (S_CODE == SR_CODE && !S_BIT) )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC / SR as source register in DIVS instruction @ %x\n", PC );
		}
		else
		{
			if( S_BIT )
			{
				divisor = GET_L_REG(S_CODE);
			}
			else
			{
				divisor = GET_G_REG(S_CODE);
			}

			if( D_BIT )
			{
				dividend_high = GET_L_REG(D_CODE);
				dividend_low  = GET_L_REG(D_CODE + INC);
			}
			else
			{
				dividend_high = GET_G_REG(D_CODE);
				dividend_low  = GET_G_REG(D_CODE + INC);
			}

			dividend = (INT64) COMBINE_64_32_32(dividend_high, dividend_low);

			if( divisor == 0 || dividend > 0xffffffff || (dividend_high & 0x80000000) )
			{
				/*Rd*/ /*Rdf -> undefined*/
				/*Z -> undefined*/
				/*N -> undefined*/
				UINT32 addr;
				SET_V(1);
				addr = get_trap_addr(RANGE_ERROR);
				execute_trap(addr);
			}
			else
			{
				INT32 quotient, remainder;

				quotient = dividend / divisor;
				remainder = dividend % divisor;
				/*a non-zero remainder has the sign bit of the dividend*/
				/*TODO: add the above comment? isn't the dividend non-negative signed?*/

				SET_RD( remainder, NOINC );
				SET_RD( quotient, INC );
				SET_Z((quotient == 0 ? 1 : 0));
				SET_N(SIGN_BIT(quotient));
				SET_V(0);
			}
		}
	}

	e132xs_ICount -= 36;
}

void e132xs_xm(void)
{
	UINT32 val;
	UINT16 next_source;
	unsigned int x_code, lim;

	if( S_BIT )
	{
		val = GET_L_REG(S_CODE);
	}
	else
	{
		val = GET_G_REG(S_CODE);
	}

	PC += 2;

	next_source = READ_OP(PC);

	x_code = X_CODE(next_source);

	if( E_BIT(next_source) )
	{
		UINT16 next_source_2;

		PC += 2;

		next_source_2 = READ_OP(PC);

		lim = ((next_source & 0xfff) << 5 ) | next_source_2;
	}
	else
	{
		lim = next_source & 0xfff;
	}

	switch( x_code )
	{
		case 0:
		case 1:
		case 2:
		case 3:
			if( val > lim )
			{
				UINT32 addr = get_trap_addr(RANGE_ERROR);
				execute_trap(addr);
			}
			else
			{
				val <<= x_code;
			}

			break;

		case 4:
		case 5:
		case 6:
		case 7:
			x_code -= 4;
			val <<= x_code;

			break;
	}

	SET_RD(val, NOINC);

	e132xs_ICount -= 1;
}

void e132xs_mask(void)
{
	INT32 val, const_val;

	if( S_BIT )
	{
		val = GET_L_REG(S_CODE);
	}
	else
	{
		val = GET_G_REG(S_CODE);
	}

	const_val = get_const();

	val &= const_val;

	SET_RD(val, NOINC);

	SET_Z((val == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_sum(void)
{
	UINT32 op1;
	INT32 const_val;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	const_val = get_const();

	if(D_CODE == PC_CODE && !D_BIT)
		PC -= 2;

	op1 += const_val;
	SET_RD(op1, NOINC);
	SET_Z((op1 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op1)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*carry //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_sums(void)
{
	INT32 op1, const_val;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	const_val = get_const();

	op1 += const_val;
	SET_RD(op1, NOINC);
	SET_Z((op1 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op1)); /*sign*/
/*	SET_V(); */ /*TODO!*/

	e132xs_ICount -= 1;

	if( GET_V && S_CODE != 1 )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}
}

void e132xs_cmp(void)
{
	UINT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	if( op1 == op2 )
		SET_Z(1);
	else
		SET_Z(0);

	if( (INT32) op2 < (INT32) op1 )	/*TODO: should it be if( op2 < (INT32) op1 ) or is the one already implemented?*/
		SET_N(1);
	else
		SET_N(0);

/*	SET_V(); */ /*TODO!*/

	if( op2 < op1 )
		SET_C(1); /*borrow*/
	else
		SET_C(0); /*borrow*/

	e132xs_ICount -= 1;
}

void e132xs_mov(void)
{
	UINT32 val;

	if( S_BIT )
	{
		val = GET_L_REG(S_CODE);
	}
	else
	{
		if( !GET_H )
		{
			val = GET_G_REG(S_CODE);
		}
		else
		{
			UINT8 s_code = S_CODE + 16;

			if( !(s_code == REG_BCR || s_code == REG_TPR || s_code == REG_FCR || s_code == REG_MCR) ) 
			{
				val = GET_G_REG(s_code);
			}
			else
			{
				/* Write only registers */
				val = 0;
			}
		}
	}

	if( D_BIT )
	{
		SET_L_REG(D_CODE, val);
	}
	else
	{
		if( !GET_S && GET_H )
		{
			UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
			execute_trap(addr);
		}
		else
		{
			SET_G_REG(D_CODE + GET_H * 16, val);
		}
	}

	if(D_CODE == PC_CODE && !D_BIT && !GET_H)
		PC -= 2;

	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(val));

	e132xs_ICount -= 1;
}

void e132xs_add(void)
{
	UINT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	if(D_CODE == PC_CODE && !D_BIT)
		PC -= 2;

	op2 += op1;
	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*carry //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_adds(void)
{
	INT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	op2 += op1;
	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/

	e132xs_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}
}

void e132xs_cmpb(void)
{
	UINT32 val1, val2;

	if( S_BIT )
	{
		val1 = GET_L_REG(S_CODE);
	}
	else
	{
		val1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		val2 = GET_L_REG(D_CODE);
	}
	else
	{
		val2 = GET_G_REG(D_CODE);
	}

	SET_Z(((val1 & val2) == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_andn(void)
{
	UINT32 op1, op2, ret;

	if( S_BIT )
	{
		op1 = ~GET_L_REG(S_CODE);
	}
	else
	{
		op1 = ~GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 & op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_or(void)
{
	UINT32 op1, op2, ret;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 | op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_xor(void)
{
	UINT32 op1, op2, ret;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 ^ op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_subc(void)
{
	UINT32 op1, op2;

	op1 = GET_C;
	if( S_BIT )
	{
		op1 += GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE != SR_CODE ) /*source doesn't denote SR*/
			op1 += GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	op2 -= op1;
	SET_RD(op2, NOINC);
	SET_Z(GET_Z & (op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*borrow //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_not(void)
{
	UINT32 ret;

	if( S_BIT )
	{
		ret = ~GET_L_REG(S_CODE);
	}
	else
	{
		ret = ~GET_G_REG(S_CODE);
	}

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_sub(void)
{
	UINT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	if(D_CODE == PC_CODE && !D_BIT)
		PC -= 2;

	op2 -= op1;
	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*borrow //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_subs(void)
{
	INT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	op2 -= op1;
	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/

	e132xs_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}
}

void e132xs_addc(void)
{
	UINT32 op1, op2;

	op1 = GET_C;
	if( S_BIT )
	{
		op1 += GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE != SR_CODE ) /*source doesn't denote SR*/
			op1 += GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	op2 += op1;
	SET_RD(op2, NOINC);
	SET_Z(GET_Z & (op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*carry //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_and(void)
{
	UINT32 op1, op2, ret;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		op1 = GET_G_REG(S_CODE);
	}

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 & op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_neg(void)
{
	UINT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);

	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denote SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	op2 = -op1;

	SET_RD(op2, NOINC);
	SET_Z(GET_Z & (op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*carry //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_negs(void)
{
	INT32 op1, op2;

	if( S_BIT )
	{
		op1 = GET_L_REG(S_CODE);
	}
	else
	{
		if( S_CODE == SR_CODE ) /*source denotes SR*/
			op1 = GET_C;
		else
			op1 = GET_G_REG(S_CODE);
	}

	op2 = -op1;

	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/

	e132xs_ICount -= 1;

	if( GET_V && S_CODE != 1 ) /*trap doesn't occur when source is SR*/
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}
}

void e132xs_cmpi(void)
{
	UINT32 op1, op2;

	op1 = immediate_value();

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	if( op1 == op2 )
		SET_Z(1);
	else
		SET_Z(0);

	if( (INT32) op2 < (INT32) op1 )	/*TODO: should it be if( op2 < (INT32) op1 ) or is the one already implemented?*/
		SET_N(1);
	else
		SET_N(0);

/*	SET_V(); */ /*TODO!*/

	if( op2 < op1 )
		SET_C(1); /*borrow*/
	else
		SET_C(0); /*borrow*/

	e132xs_ICount -= 1;
}

void e132xs_movi(void)
{
	UINT32 val;

	val = immediate_value();

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Setting register %02x to value %08x\n", D_CODE, val );

	if( D_BIT )
	{
		SET_L_REG(D_CODE, val);
	}
	else
	{
		if( !GET_S && GET_H )
		{
			UINT32 addr = get_trap_addr(PRIVILEGE_ERROR);
			execute_trap(addr);
		}
		else
		{
			SET_G_REG(D_CODE + GET_H * 16, val);
		}
	}

	if(D_CODE == PC_CODE && !D_BIT && !GET_H)
		PC -= 2;

	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(val));

	e132xs_ICount -= 1;
}

void e132xs_addi(void)
{
	UINT32 op1, op2;

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	if( !N_VALUE )
		op1 = GET_C & ((GET_Z == 0 ? 1 : 0) | (op2 & 0x01));
	else
		op1 = immediate_value();

	if(D_CODE == PC_CODE && !D_BIT)
		PC -= 2;

	op2 += op1;
	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/
/*	SET_C(); */ /*carry //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_addsi(void)
{
	INT32 op1, op2;

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	if( !N_VALUE )
		op1 = GET_C & ((GET_Z == 0 ? 1 : 0) | (op2 & 0x01));
	else
		op1 = immediate_value();

	op2 += op1;
	SET_RD(op2, NOINC);
	SET_Z((op2 == 0 ? 1: 0));
	SET_N(SIGN_BIT(op2)); /*sign*/
/*	SET_V(); */ /*TODO!*/

	e132xs_ICount -= 1;

	if( GET_V )
	{
		UINT32 addr = get_trap_addr(RANGE_ERROR);
		execute_trap(addr);
	}
}

void e132xs_cmpbi(void)
{
	UINT32 val1, val2;

	val1 = 0;
	if( N_VALUE )
	{
		if( N_VALUE == 31 )
			val1 = 0x7fffffff; /*bit 31 = 0, others = 1*/
		else
			val1 = (UINT32) immediate_value();
	}

	if( D_BIT )
	{
		val2 = GET_L_REG(D_CODE);
	}
	else
	{
		val2 = GET_G_REG(D_CODE);
	}

	if( N_VALUE )
	{
		SET_Z(((val1 & val2) == 0 ? 1: 0));
	}
	else
	{
		if( !(val2 & 0xff000000) || !(val2 & 0x00ff0000) || !(val2 & 0x0000ff00) || !(val2 & 0x000000ff) )
			SET_Z(1);
		else
			SET_Z(0);

	}

	e132xs_ICount -= 1;
}

void e132xs_andni(void)
{
	UINT32 op1, op2, ret;

	if( N_VALUE == 31 )
		op1 = ~0x7fffffff; /*bit 31 = 0, others = 1*/
	else
		op1 = ~immediate_value();

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 & op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_ori(void)
{
	UINT32 op1, op2, ret;

	op1 = immediate_value();

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 | op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_xori(void)
{
	UINT32 op1, op2, ret;

	op1 = immediate_value();

	if( D_BIT )
	{
		op2 = GET_L_REG(D_CODE);
	}
	else
	{
		op2 = GET_G_REG(D_CODE);
	}

	ret = op1 ^ op2;

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));

	e132xs_ICount -= 1;
}

void e132xs_shrdi(void)
{
	UINT32 low_order, high_order;
	UINT64 val;

	high_order = GET_L_REG(D_CODE);
	low_order = GET_L_REG(D_CODE + INC);

	val = COMBINE_U64_U32_U32(high_order, low_order);

	val >>= N_VALUE;

	high_order = (val & 0xffffffff00000000) >> 32;
	low_order = val & 0x00000000ffffffff;

	SET_RD(high_order, NOINC);
	SET_RD(low_order, INC);
	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(high_order));
/*	SET_C(); */ /*TODO!*/

	e132xs_ICount -= 2;
}

void e132xs_shrd(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/

	/*RESULT UNDEFINED IF LS DENOTES THE SAME REGISTER AS LD OR LDF*/
	if( S_CODE != D_CODE && S_CODE != (D_CODE + INC) )
	{
		high_order = GET_L_REG(D_CODE);
		low_order = GET_L_REG(D_CODE + INC);

		val = COMBINE_U64_U32_U32(high_order, low_order);

		val >>= n;

		high_order = (val & 0xffffffff00000000) >> 32;
		low_order = val & 0x00000000ffffffff;

		SET_RD(high_order, NOINC);
		SET_RD(low_order, INC);
		SET_Z((val == 0 ? 1: 0));
		SET_N(SIGN_BIT(high_order));
/*		SET_C(); */ /*TODO!*/
	}

	e132xs_ICount -= 2;
}

void e132xs_shr(void)
{
	UINT32 ret;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/

	ret = GET_L_REG(D_CODE);

	ret >>= n;
	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));
	SET_N(SIGN_BIT(ret));
/*	SET_C(); */ /*??? //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_sardi(void)
{
	INT32 low_order, high_order;
	INT64 val;
	int sign_bit;

	high_order = GET_L_REG(D_CODE);
	low_order = GET_L_REG(D_CODE + INC);

	val = (INT64) COMBINE_64_32_32(high_order, low_order);

	sign_bit = (val & 0x8000000000000000) >> 63;
	val >>= N_VALUE;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < N_VALUE; i++ )
		{
			val |= (0x8000000000000000 >> i);
		}
	}

	high_order = (val & 0xffffffff00000000) >> 32;
	low_order = val & 0x00000000ffffffff;

	SET_RD(high_order, NOINC);
	SET_RD(low_order, INC);
	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(high_order));
/*	SET_C(); */ /*?? //TODO!*/

	e132xs_ICount -= 2;
}

void e132xs_sard(void)
{
	INT32 low_order, high_order;
	INT64 val;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/
	int sign_bit;

	/*RESULT UNDEFINED IF LS DENOTES THE SAME REGISTER AS LD OR LDF*/
	if( S_CODE != D_CODE && S_CODE != (D_CODE + INC) )
	{
		high_order = GET_L_REG(D_CODE);
		low_order = GET_L_REG(D_CODE + INC);

		val = (INT64) COMBINE_64_32_32(high_order, low_order);
		sign_bit = (val & 0x8000000000000000) >> 63;

		val >>= n;

		if( sign_bit )
		{
			int i;
			for( i = 0; i < n; i++ )
			{
				val |= (0x8000000000000000 >> i);
			}
		}

		high_order = (val & 0xffffffff00000000) >> 32;
		low_order = val & 0x00000000ffffffff;

		SET_RD(high_order, NOINC);
		SET_RD(low_order, INC);
		SET_Z((val == 0 ? 1: 0));
		SET_N(SIGN_BIT(high_order));
/*		SET_C(); */ /*TODO!*/
	}

	e132xs_ICount -= 2;
}

void e132xs_sar(void)
{
	INT32 ret;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/
	int sign_bit;

	ret = GET_L_REG(D_CODE);
	sign_bit = (ret & 0x80000000) >> 31;

	ret >>= n;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < n; i++ )
		{
			ret |= (0x80000000 >> i);
		}
	}

	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));
	SET_N(SIGN_BIT(ret));
/*	SET_C(); */ /*TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_shldi(void)
{
	UINT32 low_order, high_order;
	UINT64 val;

	high_order = GET_L_REG(D_CODE);
	low_order = GET_L_REG(D_CODE + INC);

	val = COMBINE_U64_U32_U32(high_order, low_order);

	val <<= N_VALUE;

	high_order = (val & 0xffffffff00000000) >> 32;
	low_order = val & 0x00000000ffffffff;

	SET_RD(high_order, NOINC);
	SET_RD(low_order, INC);
	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(high_order));
/*	SET_V(); */ /* = ~GET_N ? //TODO!*/
/*	SET_C(); */ /*undefined*/

	e132xs_ICount -= 2;
}

void e132xs_shld(void)
{
	UINT32 low_order, high_order;
	UINT64 val;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/

	/*RESULT UNDEFINED IF LS DENOTES THE SAME REGISTER AS LD OR LDF*/
	if( S_CODE != D_CODE && S_CODE != (D_CODE + INC) )
	{
		high_order = GET_L_REG(D_CODE);
		low_order = GET_L_REG(D_CODE + INC);

		val = COMBINE_U64_U32_U32(high_order, low_order);

		val <<= n;

		high_order = (val & 0xffffffff00000000) >> 32;
		low_order = val & 0x00000000ffffffff;

		SET_RD(high_order, NOINC);
		SET_RD(low_order, INC);
		SET_Z((val == 0 ? 1: 0));
		SET_N(SIGN_BIT(high_order));
	/*	SET_V(); */ /* = ~GET_N ? //TODO!*/
	/*	SET_C(); */ /*undefined*/
	}

	e132xs_ICount -= 2;
}

void e132xs_shl(void)
{
	UINT32 ret;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/

	ret = GET_L_REG(D_CODE);

	ret <<= n;
	SET_RD(ret, NOINC);
	SET_Z((ret == 0 ? 1: 0));
	SET_N(SIGN_BIT(ret));
/*	SET_V(); */ /* = ~GET_N ? //TODO!*/
/*	SET_C(); */ /*undefined*/

	e132xs_ICount -= 1;
}

void reserved(void)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "- Reserved opcode executed @ %x, OP = %x\n", OP, PC );
}

void e132xs_testlz(void)
{
	UINT8 zeros = 0;
	UINT32 code = GET_L_REG(S_CODE);
	int mask;

	for( mask = 0x80000000; ; mask >>= 1 )
	{
		if( code & mask )
			break;
		else
			zeros++;

		if( zeros == 32 )
			break;
	}

	SET_L_REG(D_CODE, zeros);

	e132xs_ICount -= 2;
}

void e132xs_rol(void)
{
	UINT32 val;
	unsigned int n = OP & 0x1f; /*TODO: is it correct? documentation says bits 4..0 of source, but source uses bits are 3..0*/

	val = GET_L_REG(D_CODE);

	/*TODO: if n = 0 skip it ?*/
	while( n > 0 )
	{
		val = (val << 1) | ((val & 0x80000000) >> 31);
		n--;
	}

	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(val));
	/*V -> undefined*/
	/*C -> undefined*/

	e132xs_ICount -= 1;
}

/*TODO: add trap error*/
void e132xs_ldxx1(void)
{
	UINT32 load;
	UINT16 next_op;
	INT32 dis;

	PC += 2;
	next_op = READ_OP(PC);
	dis = get_dis( next_op );

	if( D_CODE == SR_CODE && !D_BIT )
	{
		switch( DD( next_op ) )
		{
			case 0:
				/* LDBS.A*/
				load = (INT8) READ_B(dis);
				SET_RS( load, NOINC );

				break;

			case 1:
				/* LDBU.A*/
				load = (UINT8) READ_B(dis);
				SET_RS( load, NOINC );

				break;

			case 2:
				/* LDHS.A*/
				if( dis & 1 )
				{
					load = (INT16) READ_HW(dis);
					SET_RS( load, NOINC );
				}
				/* LDHU.A*/
				else
				{
					load = (UINT16) READ_HW(dis);
					SET_RS( load, NOINC );
				}

				break;

			case 3:
				/* LDD.IOA*/
				if( ( dis & 2 ) && ( dis & 1 ) )
				{
					/* used in an I/O address*/
					load = READ_W(dis);
					SET_RS( load, NOINC );
					load = READ_W(dis + 4);
					SET_RS( load, INC );

				}
				/* LDW.IOA*/
				else if( ( dis & 2 ) && !( dis & 1 ) )
				{
					/* used in an I/O address*/
					load = READ_W(dis);
					SET_RS( load, NOINC );
				}
				/* LDD.A*/
				else if( !( dis & 2 ) && ( dis & 1 ) )
				{
					load = READ_W(dis);
					SET_RS( load, NOINC );
					load = READ_W(dis + 4);
					SET_RS( load, INC );
				}
				/* LDW.A*/
				else
				{
					load = READ_W(dis);
					SET_RS( load, NOINC );
				}

				break;
		}
	}
	else
	{
		switch( DD( next_op ) )
		{
			case 0:
				/* LDBS.D*/
				if( D_BIT )
				{
					load = (INT8) READ_B( GET_L_REG(D_CODE) + dis );
				}
				else
				{
					load = (INT8) READ_B( GET_G_REG(D_CODE) + dis );
				}
				SET_RS( load, NOINC );

				break;

			case 1:
				/* LDBU.D*/
				if( D_BIT )
				{
					load = (UINT8) READ_B( GET_L_REG(D_CODE) + dis );
				}
				else
				{
					load = (UINT8) READ_B( GET_G_REG(D_CODE) + dis );
				}
				SET_RS( load, NOINC );

				break;

			case 2:
				/* LDHS.D*/
				if( dis & 1 )
				{
					if( D_BIT )
					{
						load = (INT16) READ_HW( GET_L_REG(D_CODE) + dis );
					}
					else
					{
						load = (INT16) READ_HW( GET_G_REG(D_CODE) + dis );
					}
					SET_RS( load, NOINC );
				}
				/* LDHU.D*/
				else
				{
					if( D_BIT )
					{
						load = (UINT16) READ_HW( GET_L_REG(D_CODE) + dis );
					}
					else
					{
						load = (UINT16) READ_HW( GET_G_REG(D_CODE) + dis );
					}
					SET_RS( load, NOINC );
				}

				break;

			case 3:
				/* LDD.IOD*/
				if( ( dis & 2 ) && ( dis & 1 ) )
				{
					/* used in an I/O address*/

					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) + dis );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) + dis );
					}
					SET_RS( load, NOINC );

					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) + dis + 4 );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) + dis + 4 );
					}
					SET_RS( load, INC );

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* LDW.IOD*/
				else if( ( dis & 2 ) && !( dis & 1 ) )
				{
					/* used in an I/O address*/

					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) + dis );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) + dis );
					}
					SET_RS( load, NOINC );
				}
				/* LDD.D*/
				else if( !( dis & 2 ) && ( dis & 1 ) )
				{
					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) + dis );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) + dis );
					}
					SET_RS( load, NOINC );

					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) + dis + 4 );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) + dis + 4 );
					}
					SET_RS( load, INC );

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* LDW.D*/
				else
				{
					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) + dis );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) + dis );
					}
					SET_RS( load, NOINC );
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_ldxx2(void)
{
	UINT32 load;
	UINT16 next_op;
	INT32 dis;

	PC += 2;
	next_op = READ_OP(PC);
	dis = get_dis( next_op );

	if( (D_CODE == PC_CODE && !D_BIT) || (D_CODE == SR_CODE && !D_BIT) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "- In e132xs_ldxx2 must not denote PC or SR. PC = %x\n", PC );
	}
	else
	{

		switch( DD( next_op ) )
		{
			case 0:
				/* LDBS.N*/
				if( D_BIT )
				{
					load = (INT8) READ_B( GET_L_REG(D_CODE) );
					SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
				}
				else
				{
					load = (INT8) READ_B( GET_G_REG(D_CODE) );
					SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
				}
				SET_RS( load, NOINC );

				break;

			case 1:
				/* LDBU.N*/
				if( D_BIT )
				{
					load = (UINT8) READ_B( GET_L_REG(D_CODE) );
					SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
				}
				else
				{
					load = (UINT8) READ_B( GET_G_REG(D_CODE) );
					SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
				}
				SET_RS( load, NOINC );

				break;

			case 2:
				/* LDHS.N*/
				if( dis & 1 )
				{
					if( D_BIT )
					{
						load = (INT16) READ_HW( GET_L_REG(D_CODE) );
						SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
					}
					else
					{
						load = (INT16) READ_HW( GET_G_REG(D_CODE) );
						SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
					}
					SET_RS( load, NOINC );
				}
				/* LDHU.N*/
				else
				{
					if( D_BIT )
					{
						load = (UINT16) READ_HW( GET_L_REG(D_CODE) );
						SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
					}
					else
					{
						load = (UINT16) READ_HW( GET_G_REG(D_CODE) );
						SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
					}
					SET_RS( load, NOINC );
				}

				break;

			case 3:
				/* LDW.S*/
				if( ( dis & 2 ) && ( dis & 1 ) )
				{
					/*TODO: other? read notes in the docs*/
					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) );
						SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) );
						SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
					}
					SET_RS( load, NOINC );

					e132xs_ICount -= 2; /*extra cycles*/
				}
				/* Reserved*/
				else if( ( dis & 2 ) && !( dis & 1 ) )
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "- Reserved Load instruction @ %x\n", PC );
				}
				/* LDD.N*/
				else if( !( dis & 2 ) && ( dis & 1 ) )
				{
					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) );
						SET_RS( READ_W( GET_L_REG(D_CODE) + 4 ), INC );
						SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) );
						SET_RS( READ_W( GET_G_REG(D_CODE) + 4 ), INC );
						SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
					}
					SET_RS( load, NOINC );

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* LDW.N*/
				else
				{
					if( D_BIT )
					{
						load = READ_W( GET_L_REG(D_CODE) );
						SET_RD( GET_L_REG(D_CODE) + dis, NOINC );
					}
					else
					{
						load = READ_W( GET_G_REG(D_CODE) );
						SET_RD( GET_G_REG(D_CODE) + dis, NOINC );
					}
					SET_RS( load, NOINC );
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}


/*TODO: add trap error*/
void e132xs_stxx1(void)
{
	UINT32 val;
	UINT16 next_op;
	INT32 dis;

	PC += 2;
	next_op = READ_OP(PC);
	dis = get_dis( next_op );

	if( S_BIT )
	{
		val = GET_L_REG(S_CODE);
	}
	else
	{
		val = GET_G_REG(S_CODE);
	}

	if( D_CODE == SR_CODE && !D_BIT )
	{
		switch( DD( next_op ) )
		{
			case 0:
				/* STBS.A*/
				WRITE_B( dis, (INT8) val );

				break;

			case 1:
				/* STBU.A*/
				WRITE_B( dis, (UINT8) val );

				break;

			case 2:
				/* STHS.A*/
				if( dis & 1 )
				{
					WRITE_HW( dis, (INT16) val );
				}
				/* STHU.A*/
				else
				{
					WRITE_HW( dis, (UINT16) val );
				}

				break;

			case 3:
				/* STD.IOA*/
				if( ( dis & 2 ) && ( dis & 1 ) )
				{
					/* used in an I/O address*/
					UINT32 val2;

					if( S_BIT )
					{
						val2 = GET_L_REG(S_CODE + INC);
					}
					else
					{
						val2 = GET_G_REG(S_CODE + INC);
					}

					WRITE_W( dis, val );
					WRITE_W( dis + 4, val2 );

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* STW.IOA*/
				else if( ( dis & 2 ) && !( dis & 1 ) )
				{
					/* used in an I/O address*/
					WRITE_W( dis, val );
				}
				/* STD.A*/
				else if( !( dis & 2 ) && ( dis & 1 ) )
				{
					UINT32 val2;

					if( S_BIT )
					{
						val2 = GET_L_REG(S_CODE + INC);
					}
					else
					{
						val2 = GET_G_REG(S_CODE + INC);
					}

					WRITE_W( dis, val );
					WRITE_W( dis + 4, val2 );

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* STW.A*/
				else
				{
					WRITE_W( dis, val );
				}

				break;
		}
	}
	else
	{
		switch( DD( next_op ) )
		{
			case 0:
				/* STBS.D*/
				if( D_BIT )
				{
					WRITE_B( GET_L_REG(D_CODE) + dis, (INT8) val );
				}
				else
				{
					WRITE_B( GET_G_REG(D_CODE) + dis, (INT8) val );
				}

				break;

			case 1:
				/* STBU.D*/
				if( D_BIT )
				{
					WRITE_B( GET_L_REG(D_CODE) + dis, (UINT8) val );
				}
				else
				{
					WRITE_B( GET_G_REG(D_CODE) + dis, (UINT8) val );
				}

				break;

			case 2:
				/* STHS.D*/
				if( dis & 1 )
				{
					if( D_BIT )
					{
						WRITE_HW( GET_L_REG(D_CODE) + dis, (INT16) val );
					}
					else
					{
						WRITE_HW( GET_G_REG(D_CODE) + dis, (INT16) val );
					}
				}
				/* STHU.D*/
				else
				{
					if( D_BIT )
					{
						WRITE_HW( GET_L_REG(D_CODE) + dis, (UINT16) val );
					}
					else
					{
						WRITE_HW( GET_G_REG(D_CODE) + dis, (UINT16) val );
					}
				}

				break;

			case 3:
				/* STD.IOD*/
				if( ( dis & 2 ) && ( dis & 1 ) )
				{
					/* used in an I/O address*/
					UINT32 val2;

					if( S_BIT )
					{
						val2 = GET_L_REG(S_CODE + INC);
					}
					else
					{
						val2 = GET_G_REG(S_CODE + INC);
					}

					if( D_BIT )
					{
						WRITE_W( GET_L_REG(D_CODE) + dis, val );
						WRITE_W( GET_L_REG(D_CODE) + dis + 4, val2 );
					}
					else
					{
						WRITE_W( GET_G_REG(D_CODE) + dis, val );
						WRITE_W( GET_G_REG(D_CODE) + dis + 4, val2 );
					}

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* STW.IOD*/
				else if( ( dis & 2 ) && !( dis & 1 ) )
				{
					/* used in an I/O address*/
					if( D_BIT )
					{
						WRITE_W( GET_L_REG(D_CODE) + dis, val );
					}
					else
					{
						WRITE_W( GET_G_REG(D_CODE) + dis, val );
					}
				}
				/* STD.D*/
				else if( !( dis & 2 ) && ( dis & 1 ) )
				{
					UINT32 val2;

					if( S_BIT )
					{
						val2 = GET_L_REG(S_CODE + INC);
					}
					else
					{
						val2 = GET_G_REG(S_CODE + INC);
					}

					if( D_BIT )
					{
						WRITE_W( GET_L_REG(D_CODE) + dis, val );
						WRITE_W( GET_L_REG(D_CODE) + dis + 4, val2 );
					}
					else
					{
						WRITE_W( GET_G_REG(D_CODE) + dis, val );
						WRITE_W( GET_G_REG(D_CODE) + dis + 4, val2 );
					}

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* STW.D*/
				else
				{
					if( D_BIT )
					{
						WRITE_W( GET_L_REG(D_CODE) + dis, val );
					}
					else
					{
						WRITE_W( GET_G_REG(D_CODE) + dis, val );
					}
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_stxx2(void)
{
	UINT32 val, addr;
	UINT16 next_op;
	INT32 dis;

	PC += 2;
	next_op = READ_OP(PC);
	dis = get_dis( next_op );

	if( (D_CODE == PC_CODE && !D_BIT) || (D_CODE == SR_CODE && !D_BIT) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "In e132xs_stxx2 must not denote PC or SR. PC = %x\n", PC );
	}
	else
	{

		if( S_BIT )
		{
			val = GET_L_REG(S_CODE);
		}
		else
		{
			val = GET_G_REG(S_CODE);
		}

		if( D_BIT )
		{
			addr = GET_L_REG(D_CODE);
			SET_L_REG( D_CODE, addr + dis );
		}
		else
		{
			addr = GET_G_REG(D_CODE);
			SET_G_REG( D_CODE, addr + dis );
		}

		switch( DD( next_op ) )
		{
			case 0:
				/* STBS.N*/
				WRITE_B( addr, (INT8) val );

				break;

			case 1:
				/* STBU.N*/
				WRITE_B( addr, (UINT8) val );

				break;

			case 2:
				/* STHS.N*/
				if( dis & 1 )
				{
					WRITE_HW( addr, (INT16) val );
				}
				/* STHU.N*/
				else
				{
					WRITE_HW( addr, (UINT16) val );
				}

				break;

			case 3:
				/* STW.S*/
				if( ( dis & 2 ) && ( dis & 1 ) )
				{
					WRITE_W( addr, val );

					e132xs_ICount -= 2; /*extra cycles*/
				}
				/* Reserved*/
				else if( ( dis & 2 ) && !( dis & 1 ) )
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Reserved Store instruction @ %x\n", PC );
				}
				/* STD.N*/
				else if( !( dis & 2 ) && ( dis & 1 ) )
				{
					UINT32 val2;

					if( S_BIT )
					{
						val2 = GET_L_REG(S_CODE + INC);
					}
					else
					{
						val2 = GET_G_REG(S_CODE + INC);
					}

					WRITE_W( addr, val );
					WRITE_W( addr + 4, val2 );

					e132xs_ICount -= 1; /*extra cycle*/
				}
				/* STW.N*/
				else
				{
					WRITE_W( addr, val );
				}

				break;
		}
	}

	e132xs_ICount -= 1;
}

void e132xs_shri(void)
{
	UINT32 val;

	if( D_BIT )
	{
		val = GET_L_REG(D_CODE);
	}
	else
	{
		val = GET_G_REG(D_CODE);
	}

	val >>= N_VALUE;

	SET_RD(val, NOINC);
	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(val));
/*	SET_C(); */ /* 1 if( val & n ) ? //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_sari(void)
{
	INT32 val;
	int sign_bit;

	if( D_BIT )
	{
		val = GET_L_REG(D_CODE);
	}
	else
	{
		val = GET_G_REG(D_CODE);
	}

	sign_bit = (val & 0x80000000) >> 31;
	val >>= N_VALUE;

	if( sign_bit )
	{
		int i;
		for( i = 0; i < N_VALUE; i++ )
		{
			val |= (0x80000000 >> i);
		}
	}

	SET_RD(val, NOINC);
	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(val));
/*	SET_C(); */ /* 1 if( val & n ) ? //TODO!*/

	e132xs_ICount -= 1;
}

void e132xs_shli(void)
{
	UINT32 val;

	if( D_BIT )
	{
		val = GET_L_REG(D_CODE);
	}
	else
	{
		val = GET_G_REG(D_CODE);
	}

	val <<= N_VALUE;
	SET_RD(val, NOINC);
	SET_Z((val == 0 ? 1: 0));
	SET_N(SIGN_BIT(val)); /*sign*/
/*	SET_V(); */ /* = ~GET_N ? //TODO!*/
/*	SET_C(); */ /*undefined*/

	e132xs_ICount -= 1;
}

void e132xs_mulu(void)
{
	UINT32 op1, op2, low_order, high_order;
	UINT64 double_word;

	op1 = op2 = 0;

	/*PC or SR aren't denoted, else result is undefined*/
	if( (S_CODE == PC_CODE && !S_BIT) || (S_CODE == SR_CODE && !S_BIT) || (D_CODE == PC_CODE && !D_BIT) || (D_CODE == SR_CODE && !D_BIT) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC or SR in MULU instruction @ x\n", PC );
	}
	else
	{
		if( S_BIT )
		{
			op1 = GET_L_REG(S_CODE);
		}
		else
		{
			op1 = GET_G_REG(S_CODE);
		}

		if( D_BIT )
		{
			op2 = GET_L_REG(D_CODE);
		}
		else
		{
			op2 = GET_G_REG(D_CODE);
		}

		double_word = (op1 * op2);
		low_order = double_word & 0x00000000ffffffff;
		high_order = (double_word & 0xffffffff00000000) >> 32;

		SET_RD(high_order, NOINC);
		SET_RD(low_order, INC);
		SET_Z((double_word == 0 ? 1: 0));
		SET_N(SIGN_BIT(high_order)); /*sign*/
	/*	SET_V(); */ /*undefined*/
	/*	SET_C(); */ /*undefined*/
	}

	if( op1 <= 0xffff  && op2 <= 0xffff )
		e132xs_ICount -= 4;
	else
		e132xs_ICount -= 6;
}

void e132xs_muls(void)
{
	INT32 op1, op2, low_order, high_order;
	INT64 double_word;

	op1 = op2 = 0;

	/*PC or SR aren't denoted, else result is undefined*/
	if( (S_CODE == PC_CODE && !S_BIT) || (S_CODE == SR_CODE && !S_BIT) || (D_CODE == PC_CODE && !D_BIT) || (D_CODE == SR_CODE && !D_BIT) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC or SR in MULS instruction @ x\n", PC );
	}
	else
	{
		if( S_BIT )
		{
			op1 = GET_L_REG(S_CODE);
		}
		else
		{
			op1 = GET_G_REG(S_CODE);
		}

		if( D_BIT )
		{
			op2 = GET_L_REG(D_CODE);
		}
		else
		{
			op2 = GET_G_REG(D_CODE);
		}

		double_word = (op1 * op2);
		low_order = double_word & 0x00000000ffffffff;
		high_order = (double_word & 0xffffffff00000000) >> 32;

		SET_RD(high_order, NOINC);
		SET_RD(low_order, INC);
		SET_Z((double_word == 0 ? 1: 0));
		SET_N(SIGN_BIT(high_order)); /*sign*/
	/*	SET_V(); */ /*undefined*/
	/*	SET_C(); */ /*undefined*/
	}

	if( ( op1 >= 0xffff8000 && op1 <= 0x7fff ) && ( op2 >= 0xffff8000 && op2 <= 0x7fff ) )
		e132xs_ICount -= 4;
	else
		e132xs_ICount -= 6;
}

void e132xs_set(void)
{
	int n = N_VALUE;

	if( D_CODE == PC_CODE && !D_BIT )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC in e132xs_set @ %x, it is reserved for future use\n", PC );
	}
	else if( D_CODE == SR_CODE && !D_BIT )
	{
		/*TODO: add fetch opcode (when is there the pipeline?)*/

		/*TODO: no 1!*/
		e132xs_ICount -= 1;
	}
	else
	{
		switch( n )
		{
			/* SETADR*/
			case 0:
			{
				UINT32 val;
				val =  (SP & 0xfffffe00) | (GET_FP << 2);

				/*plus carry into bit 9*/
				val += (((SP & 0x100) && !SIGN_BIT(SR)) ? 1 : 0);

				SET_RD(val, NOINC);

				break;
			}
			/* Reserved*/
			case 1:
			case 16:
			case 17:
			case 19:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Used reserved N value (%d) in e132xs_set @ %x\n", n, PC );
				break;

			/* SETxx*/
			case 2:
				SET_RD(1, NOINC);
				break;

			case 3:
				SET_RD(0, NOINC);
				break;

			case 4:
				if( GET_N || GET_Z )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 5:
				if( !GET_N && !GET_Z )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 6:
				if( GET_N )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 7:
				if( !GET_N )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 8:
				if( GET_C || GET_Z )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 9:
				if( !GET_C && !GET_Z )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 10:
				if( GET_C )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 11:
				if( !GET_C )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 12:
				if( GET_Z )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 13:
				if( !GET_Z )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 14:
				if( GET_V )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 15:
				if( !GET_V )
				{
					SET_RD(1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 18:
				SET_RD(-1, NOINC);

				break;

			case 20:
				if( GET_N || GET_Z )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 21:
				if( !GET_N && !GET_Z )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 22:
				if( GET_N )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 23:
				if( !GET_N )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 24:
				if( GET_C || GET_Z )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 25:
				if( !GET_C && !GET_Z )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 26:
				if( GET_C )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 27:
				if( !GET_C )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 28:
				if( GET_Z )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 29:
				if( !GET_Z )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 30:
				if( GET_V )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			case 31:
				if( !GET_V )
				{
					SET_RD(-1, NOINC);
				}
				else
				{
					SET_RD(0, NOINC);
				}

				break;

			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "N value (%d) non defined in e132xs_set @ %x\n", n, PC );
		}

		e132xs_ICount -= 1;
	}
}

void e132xs_mul(void)
{
	INT32 op1, op2;
	INT32 single_word;

	op1 = op2 = 0;

	/*PC or SR aren't denoted, else result is undefined*/
	if( (S_CODE == PC_CODE && !S_BIT) || (S_CODE == SR_CODE && !S_BIT) || (D_CODE == PC_CODE && !D_BIT) || (D_CODE == SR_CODE && !D_BIT) )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Denoted PC or SR in MUL instruction @ x\n", PC );
	}
	else
	{
		if( S_BIT )
		{
			op1 = GET_L_REG(S_CODE);
		}
		else
		{
			op1 = GET_G_REG(S_CODE);
		}

		if( D_BIT )
		{
			op2 = GET_L_REG(D_CODE);
		}
		else
		{
			op2 = GET_G_REG(D_CODE);
		}

		single_word = (op1 * op2) & 0xffffffff; /*only the low-order word is taken*/

		SET_RD(single_word, NOINC);
		SET_Z((single_word == 0 ? 1: 0));
		SET_N(SIGN_BIT(single_word)); /*sign*/
	/*	SET_V(); */ /*undefined*/
	/*	SET_C(); */ /*undefined*/
	}

	if( ( op1 >= 0xffff8000 && op1 <= 0x7fff ) && ( op2 >= 0xffff8000 && op2 <= 0x7fff ) )
		e132xs_ICount -= 3;
	else
		e132xs_ICount -= 5;
}

void e132xs_fadd(void)
{
}

void e132xs_faddd(void)
{
}

void e132xs_fsub(void)
{
}

void e132xs_fsubd(void)
{
}

void e132xs_fmul(void)
{
}

void e132xs_fmuld(void)
{
}

void e132xs_fdiv(void)
{
}

void e132xs_fdivd(void)
{
}

void e132xs_fcmp(void)
{
}

void e132xs_fcmpd(void)
{
}

void e132xs_fcmpu(void)
{
}

void e132xs_fcmpud(void)
{
}

void e132xs_fcvt(void)
{
}

void e132xs_fcvtd(void)
{
}

void e132xs_extend(void)
{
	/*TODO: add locks, overflow error and other things*/
	UINT16 ext_opcode;
	UINT32 vals, vald;

	vals = GET_L_REG(S_CODE);
	vald = GET_L_REG(D_CODE);

	PC += 2;
	ext_opcode = READ_OP(PC);

	switch( ext_opcode )
	{
		/* signed or unsigned multiplication, single word product*/
		case EMUL:
		{
			UINT32 result;

			result = vals * vald;
			SET_G_REG(15, result);

			break;
		}
		/* unsigned multiplication, double word product*/
		case EMULU:
		{
			UINT64 result;

			result = vals * vald;
			vals = (result & 0xffffffff00000000) >> 32;
			vald = result & 0x00000000ffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		/* signed multiplication, double word product*/
		case EMULS:
		{
			INT64 result;

			result = (INT32)vals * (INT32)vald;
			vals = (result & 0xffffffff00000000) >> 32;
			vald = result & 0x00000000ffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		/* signed multiply/add, single word product sum*/
		case EMAC:
		{
			INT32 result;

			result = GET_G_REG(15) + ((INT32)vals * (INT32)vald);
			SET_G_REG(15, result);

			break;
		}
		/* signed multiply/add, double word product sum*/
		case EMACD:
		{
			INT64 result;

			result = COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) + ((INT32)vals * (INT32)vald);

			vals = (result & 0xffffffff00000000) >> 32;
			vald = result & 0x00000000ffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		/* signed multiply/substract, single word product difference*/
		case EMSUB:
		{
			INT32 result;

			result = GET_G_REG(15) - ((INT32)vals * (INT32)vald);
			SET_G_REG(15, result);

			break;
		}
		/* signed multiply/substract, double word product difference*/
		case EMSUBD:
		{
			INT64 result;

			result = COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) - ((INT32)vals * (INT32)vald);

			vals = (result & 0xffffffff00000000) >> 32;
			vald = result & 0x00000000ffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		/* signed half-word multiply/add, single word product sum*/
		case EHMAC:
		{
			INT32 result;

			result = GET_G_REG(15) + (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) + ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(15, result);

			break;
		}
		/* signed half-word multiply/add, double word product sum*/
		case EHMACD:
		{
			INT64 result;

			result = COMBINE_64_32_32(GET_G_REG(14), GET_G_REG(15)) + (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) + ((vald & 0xffff) * (vals & 0xffff));

			vals = (result & 0xffffffff00000000) >> 32;
			vald = result & 0x00000000ffffffff;
			SET_G_REG(14, vals);
			SET_G_REG(15, vald);

			break;
		}
		/* half-word complex multiply*/
		case EHCMULD:
		{
			UINT32 result;

			result = (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) - ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(14, result);

			result = (((vald & 0xffff0000) >> 16) * (vals & 0xffff)) + ((vald & 0xffff) * ((vals & 0xffff0000) >> 16));
			SET_G_REG(15, result);

			break;
		}
		/* half-word complex multiply/add*/
		case EHCMACD:
		{
			UINT32 result;

			result = GET_G_REG(14) + (((vald & 0xffff0000) >> 16) * ((vals & 0xffff0000) >> 16)) - ((vald & 0xffff) * (vals & 0xffff));
			SET_G_REG(14, result);

			result = GET_G_REG(15) + (((vald & 0xffff0000) >> 16) * (vals & 0xffff)) + ((vald & 0xffff) * ((vals & 0xffff0000) >> 16));
			SET_G_REG(15, result);

			break;
		}
		/* half-word (complex) add/substract*/
		/* Ls is not used and should denote the same register as Ld*/
		case EHCSUMD:
		{
			UINT32 result;

			result = ((((vals & 0xffff0000) >> 16) + GET_G_REG(14)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) + GET_G_REG(15)) & 0xffff;
			SET_G_REG(14, result);

			result = ((((vals & 0xffff0000) >> 16) - GET_G_REG(14)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) - GET_G_REG(15)) & 0xffff;
			SET_G_REG(15, result);

			break;
		}
		/* half-word (complex) add/substract with fixed point adjustment*/
		/* Ls is not used and should denote the same register as Ld*/
		case EHCFFTD:
		{
			UINT32 result;

			result = ((((vals & 0xffff0000) >> 16) + (GET_G_REG(14) >> 15)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) + (GET_G_REG(15) >> 15)) & 0xffff;
			SET_G_REG(14, result);

			result = ((((vals & 0xffff0000) >> 16) - (GET_G_REG(14) >> 15)) << 16) & 0xffff0000;
			result |= ((vals & 0xffff) - (GET_G_REG(15) >> 15)) & 0xffff;
			SET_G_REG(15, result);

			break;
		}
		/* half-word (complex) add/substract with fixed point adjustment and shift*/
		/* Ls is not used and should denote the same register as Ld*/
		case EHCFFTSD:
		{
			UINT32 result;

			result = (((((vals & 0xffff0000) >> 16) + (GET_G_REG(14) >> 15)) >> 1) << 16) & 0xffff0000;
			result |= ((((vals & 0xffff) + (GET_G_REG(15) >> 15)) >> 1) & 0xffff);
			SET_G_REG(14, result);

			result = (((((vals & 0xffff0000) >> 16) - (GET_G_REG(14) >> 15)) >> 1) << 16) & 0xffff0000;
			result |= ((((vals & 0xffff) - (GET_G_REG(15) >> 15)) >> 1) & 0xffff);
			SET_G_REG(15, result);

			break;
		}
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Illegal extended opcode (%x) @ %x\n", ext_opcode, PC );
			break;
	}

	e132xs_ICount -= 1; /*TODO: with the latency it can change*/
}

void e132xs_do(void)
{
}

void e132xs_ldwr(void)
{
	UINT32 val;

	val = GET_L_REG( D_CODE );

	if( S_BIT )
	{
		SET_L_REG( S_CODE, READ_W(val) );
	}
	else
	{
		SET_G_REG( S_CODE, READ_W(val) );
	}

	e132xs_ICount -= 1;
}

void e132xs_lddr(void)
{
	UINT32 val_high, val_low;

	val_high = GET_L_REG( D_CODE );
	val_low = GET_L_REG( D_CODE ) + 4;

	if( S_BIT )
	{
		SET_L_REG( S_CODE, READ_W(val_high) );
		SET_L_REG( S_CODE + INC, READ_W(val_low) );
	}
	else
	{
		SET_G_REG( S_CODE, READ_W(val_high) );
		SET_G_REG( S_CODE + INC, READ_W(val_low) );
	}

	e132xs_ICount -= 2;
}

void e132xs_ldwp(void)
{
	UINT32 val;

	val = GET_L_REG( D_CODE );

	if( S_BIT )
	{
		SET_L_REG( S_CODE, READ_W(val) );
	}
	else
	{
		SET_G_REG( S_CODE, READ_W(val) );
	}

	SET_L_REG( D_CODE, val + 4 );

	e132xs_ICount -= 1;
}

void e132xs_lddp(void)
{
	UINT32 val_high, val_low;

	val_high = GET_L_REG( D_CODE );
	val_low = GET_L_REG( D_CODE ) + 4;

	if( S_BIT )
	{
		SET_L_REG( S_CODE, READ_W(val_high) );
		SET_L_REG( S_CODE + INC, READ_W(val_low) );
	}
	else
	{
		SET_G_REG( S_CODE, READ_W(val_high) );
		SET_G_REG( S_CODE + INC, READ_W(val_low) );
	}

	SET_L_REG( D_CODE, GET_L_REG( D_CODE ) + 8 );

	e132xs_ICount -= 2;
}

void e132xs_stwr(void)
{
	UINT32 val;

	if( S_BIT )
	{
		val = GET_L_REG(S_CODE);
	}
	else
	{
		val = GET_G_REG(S_CODE);
	}

	WRITE_W( GET_L_REG(D_CODE), val );

	e132xs_ICount -= 1;
}

void e132xs_stdr(void)
{
	UINT32 val_high, val_low;

	if( S_BIT )
	{
		val_high = GET_L_REG(S_CODE);
		val_low  = GET_L_REG(S_CODE + INC);
	}
	else
	{
		val_high = GET_G_REG(S_CODE);
		val_low  = GET_G_REG(S_CODE + INC);
	}

	WRITE_W( GET_L_REG(D_CODE), val_high );
	WRITE_W( GET_L_REG(D_CODE) + 4, val_low );

	e132xs_ICount -= 2;
}

void e132xs_stwp(void)
{
	UINT32 val;

	if( S_BIT )
	{
		val = GET_L_REG(S_CODE);
	}
	else
	{
		val = GET_G_REG(S_CODE);
	}

	WRITE_W( GET_L_REG(D_CODE), val );
	SET_L_REG( D_CODE, GET_L_REG(D_CODE) + 4 );

	e132xs_ICount -= 1;
}

void e132xs_stdp(void)
{
	UINT32 val_high, val_low;

	if( S_BIT )
	{
		val_high = GET_L_REG(S_CODE);
		val_low  = GET_L_REG(S_CODE + INC);
	}
	else
	{
		val_high = GET_G_REG(S_CODE);
		val_low  = GET_G_REG(S_CODE + INC);
	}

	WRITE_W( GET_L_REG(D_CODE), val_high );
	WRITE_W( GET_L_REG(D_CODE) + 4, val_low );
	SET_L_REG( D_CODE, GET_L_REG(D_CODE) + 8 );

	e132xs_ICount -= 2;
}

void e132xs_dbv(void)
{
	if( GET_V )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbnv(void)
{
	if( !GET_V )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbe(void) /*or DBZ*/
{
	if( GET_Z )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbne(void) /*or DBNZ*/
{
	if( !GET_Z )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbc(void) /*or DBST*/
{
	if( GET_C )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbnc(void) /*or DBHE*/
{
	if( !GET_C )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbse(void)
{
	if( GET_C || GET_Z )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbht(void)
{
	if( !GET_C && !GET_Z )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbn(void) /*or DBLT*/
{
	if( GET_N )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbnn(void) /*or DBGE*/
{
	if( !GET_N )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dble(void)
{
	if( GET_N || GET_Z )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbgt(void)
{
	if( !GET_N && !GET_Z )
		execute_dbr(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_dbr(void)
{
	execute_dbr(get_pcrel());
}

void e132xs_frame(void)
{
	/*TODO: check the bounds?*/

	UINT8 difference; /*really it's 7 bits*/

	SET_FP((GET_FP - S_CODE));

	if( D_CODE )
		SET_FL( D_CODE );
	else
		SET_FL( 16 ); /*when Ld is L0*/

	SET_M( 0 );

	/*TODO: SP uses bits 8..0 as in the doc or 7..2 ?*/
	if( GET_FL )
		difference = ( SP & 0x1fc ) + ( 64 - 10 ) - ( GET_FP + GET_FL );
	else
		difference = ( SP & 0x1fc ) + ( 64 - 10 ) - ( GET_FP + 16 );

	/* bit 6 of difference is sign*/
	if( difference & 0x40 ) /*else it's finished*/
	{
		unsigned char tmp_flag;

		tmp_flag = ( SP >= UB ? 1 : 0 );

		do
		{
			WRITE_W(SP, ( SP & 0xfc ) );
			SP += 4;
			difference++;
			difference = ((difference & 0x7f) + ((difference & 0x80) >> 7)) & 0x7f;

		} while( !( difference & 0x40 ) );

		if( tmp_flag )
		{
			UINT32 addr = get_trap_addr(FRAME_ERROR);
			execute_trap(addr);
		}
	}

	/*TODO: no 1!*/
	e132xs_ICount -= 1;
}

void e132xs_call(void)
{
	INT32 const_val;

	/*TODO: add -> bit 0 of const must be 0 ?*/
	const_val = get_const();

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Immediate value for CALL: %04x\n", const_val );

	if( !(S_CODE == SR_CODE && !S_BIT) )
	{
		if( S_BIT )
		{
			const_val += GET_L_REG(S_CODE);
		}
		else
		{
			const_val += GET_G_REG(S_CODE);
		}
	}

	SET_LD(((PC & 0xfffffffe) | GET_S), NOINC);
	SET_LD(SR, INC);

	if( D_CODE )
		SET_FP((GET_FP + D_CODE));
	else
		SET_FP((GET_FP + 16));

	SET_FL(6);
	SET_M(0);

	PPC = PC;
	PC = const_val;

	/*TODO: add interrupt locks, errors, ....*/

	/*TODO: no 1!*/
	e132xs_ICount -= 1;
}

void e132xs_bv(void)
{
	if( GET_V )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bnv(void)
{
	if( !GET_V )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_be(void) /*or BZ*/
{
	if( GET_Z )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bne(void) /*or BNZ*/
{
	if( !GET_Z )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bc(void) /*or BST*/
{
	if( GET_C )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bnc(void) /*or BHE*/
{
	if( !GET_C )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bse(void)
{
	if( GET_C || GET_Z )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bht(void)
{
	if( !GET_C && !GET_Z )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bn(void) /*or BLT*/
{
	if( GET_N )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bnn(void) /*or BGE*/
{
	if( !GET_N )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_ble(void)
{
	if( GET_N || GET_Z )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_bgt(void)
{
	if( !GET_N && !GET_Z )
		execute_br(get_pcrel());
	else
		e132xs_ICount -= 1;
}

void e132xs_br(void)
{
	execute_br(get_pcrel());
}

void e132xs_trap(void)
{
	unsigned int code;
	UINT32 addr;

	addr = 0xffffff00 | (OP & 0xfc); /*TODO: what do i need to use? 0xffffff00 or entry_point or GET_TRAP_ADDR ?*/
	/*TODO: what is the trapno ?*/
	code = ((OP & 0x300) >> 6) | (OP & 0x03);

	switch( code )
	{
	/*	case 0:
		case 1:
		case 2:
		case 3:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "- Trap code not available (%d) @ %x\n", trapno, PC );
			break;
	*/
		case TRAPLE:
			if( GET_N || GET_Z )
				execute_trap(addr);

			break;

		case TRAPGT:
			if( !GET_N && !GET_Z )
				execute_trap(addr);

			break;

		case TRAPLT:
			if( GET_N )
				execute_trap(addr);

			break;

		case TRAPGE:
			if( !GET_N )
				execute_trap(addr);

			break;

		case TRAPSE:
			if( GET_C || GET_Z )
				execute_trap(addr);

			break;

		case TRAPHT:
			if( !GET_C && !GET_Z )
				execute_trap(addr);

			break;

		case TRAPST:
			if( GET_C )
				execute_trap(addr);

			break;

		case TRAPHE:
			if( !GET_C )
				execute_trap(addr);

			break;

		case TRAPE:
			if( GET_Z )
				execute_trap(addr);

			break;

		case TRAPNE:
			if( !GET_Z )
				execute_trap(addr);

			break;

		case TRAPV:
			if( GET_V )
				execute_trap(addr);

			break;

		case TRAP:
			execute_trap(addr);

			break;

		default:
			e132xs_ICount -= 1;
	}
}
