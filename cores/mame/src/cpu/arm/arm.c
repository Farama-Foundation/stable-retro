/* arm.c

	ARM 2/3/6 Emulation

	Todo:
	Software interrupts unverified (nothing uses them so far, but they should be ok)
	Timing - Currently very approximated, nothing relies on proper timing so far.
	IRQ timing not yet correct (again, nothing is affected by this so far).

	By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino

*/

#include <stdio.h>
#include "arm.h"
#include "state.h"
#include "mamedbg.h"

#define READ8(addr)			cpu_read8(addr)
#define WRITE8(addr,data)	cpu_write8(addr,data)
#define READ32(addr)		cpu_read32(addr)
#define WRITE32(addr,data)	cpu_write32(addr,data)

#define ARM_DEBUG_CORE 0
#define ARM_DEBUG_COPRO 0

enum
{
	eARM_MODE_USER	= 0x0,
	eARM_MODE_FIQ	= 0x1,
	eARM_MODE_IRQ	= 0x2,
	eARM_MODE_SVC	= 0x3,

	kNumModes
};

/* There are 27 32 bit processor registers */
enum
{
	eR0=0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
	eR8,eR9,eR10,eR11,eR12,
	eR13, /* Stack Pointer */
	eR14, /* Link Register (holds return address) */
	eR15, /* Program Counter */

	/* Fast Interrupt */
	eR8_FIQ,eR9_FIQ,eR10_FIQ,eR11_FIQ,eR12_FIQ,eR13_FIQ,eR14_FIQ,

	/* IRQ */
	eR13_IRQ,eR14_IRQ,

	/* Software Interrupt */
	eR13_SVC,eR14_SVC,

	kNumRegisters
};

/* 16 processor registers are visible at any given time,
 * banked depending on processor mode.
 */
static const int sRegisterTable[kNumModes][16] =
{
	{ /* USR */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8,eR9,eR10,eR11,eR12,
		eR13,eR14,
		eR15
	},
	{ /* FIQ */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8_FIQ,eR9_FIQ,eR10_FIQ,eR11_FIQ,eR12_FIQ,
		eR13_FIQ,eR14_FIQ,
		eR15
	},
	{ /* IRQ */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8,eR9,eR10,eR11,eR12,
		eR13_IRQ,eR14_IRQ,
		eR15
	},
	{ /* SVC */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8,eR9,eR10,eR11,eR12,
		eR13_SVC,eR14_SVC,
		eR15
	}
};

#define N_BIT	31
#define Z_BIT	30
#define C_BIT	29
#define V_BIT	28
#define I_BIT	27
#define F_BIT	26

#define N_MASK	((data32_t)(1<<N_BIT)) /* Negative flag */
#define Z_MASK	((data32_t)(1<<Z_BIT)) /* Zero flag */
#define C_MASK	((data32_t)(1<<C_BIT)) /* Carry flag */
#define V_MASK	((data32_t)(1<<V_BIT)) /* oVerflow flag */
#define I_MASK	((data32_t)(1<<I_BIT)) /* Interrupt request disable */
#define F_MASK	((data32_t)(1<<F_BIT)) /* Fast interrupt request disable */

#define N_IS_SET(pc)	((pc) & N_MASK)
#define Z_IS_SET(pc)	((pc) & Z_MASK)
#define C_IS_SET(pc)	((pc) & C_MASK)
#define V_IS_SET(pc)	((pc) & V_MASK)
#define I_IS_SET(pc)	((pc) & I_MASK)
#define F_IS_SET(pc)	((pc) & F_MASK)

#define N_IS_CLEAR(pc)	(!N_IS_SET(pc))
#define Z_IS_CLEAR(pc)	(!Z_IS_SET(pc))
#define C_IS_CLEAR(pc)	(!C_IS_SET(pc))
#define V_IS_CLEAR(pc)	(!V_IS_SET(pc))
#define I_IS_CLEAR(pc)	(!I_IS_SET(pc))
#define F_IS_CLEAR(pc)	(!F_IS_SET(pc))

#define PSR_MASK		((data32_t) 0xf0000000u)
#define IRQ_MASK		((data32_t) 0x0c000000u)
#define ADDRESS_MASK	((data32_t) 0x03fffffcu)
#define MODE_MASK		((data32_t) 0x00000003u)

#define R15						arm.sArmRegister[eR15]
#define MODE					(R15&0x03)
#define SIGN_BIT				((data32_t)(1<<31))
#define SIGN_BITS_DIFFER(a,b)	(((a)^(b)) >> 31)

/* Deconstructing an instruction */

#define INSN_COND			((data32_t) 0xf0000000u)
#define INSN_SDT_L			((data32_t) 0x00100000u)
#define INSN_SDT_W			((data32_t) 0x00200000u)
#define INSN_SDT_B			((data32_t) 0x00400000u)
#define INSN_SDT_U			((data32_t) 0x00800000u)
#define INSN_SDT_P			((data32_t) 0x01000000u)
#define INSN_BDT_L			((data32_t) 0x00100000u)
#define INSN_BDT_W			((data32_t) 0x00200000u)
#define INSN_BDT_S			((data32_t) 0x00400000u)
#define INSN_BDT_U			((data32_t) 0x00800000u)
#define INSN_BDT_P			((data32_t) 0x01000000u)
#define INSN_BDT_REGS		((data32_t) 0x0000ffffu)
#define INSN_SDT_IMM		((data32_t) 0x00000fffu)
#define INSN_MUL_A			((data32_t) 0x00200000u)
#define INSN_MUL_RM			((data32_t) 0x0000000fu)
#define INSN_MUL_RS			((data32_t) 0x00000f00u)
#define INSN_MUL_RN			((data32_t) 0x0000f000u)
#define INSN_MUL_RD			((data32_t) 0x000f0000u)
#define INSN_I				((data32_t) 0x02000000u)
#define INSN_OPCODE			((data32_t) 0x01e00000u)
#define INSN_S				((data32_t) 0x00100000u)
#define INSN_BL				((data32_t) 0x01000000u)
#define INSN_BRANCH			((data32_t) 0x00ffffffu)
#define INSN_SWI			((data32_t) 0x00ffffffu)
#define INSN_RN				((data32_t) 0x000f0000u)
#define INSN_RD				((data32_t) 0x0000f000u)
#define INSN_OP2			((data32_t) 0x00000fffu)
#define INSN_OP2_SHIFT		((data32_t) 0x00000f80u)
#define INSN_OP2_SHIFT_TYPE	((data32_t) 0x00000070u)
#define INSN_OP2_RM			((data32_t) 0x0000000fu)
#define INSN_OP2_ROTATE		((data32_t) 0x00000f00u)
#define INSN_OP2_IMM		((data32_t) 0x000000ffu)
#define INSN_OP2_SHIFT_TYPE_SHIFT	4
#define INSN_OP2_SHIFT_SHIFT		7
#define INSN_OP2_ROTATE_SHIFT		8
#define INSN_MUL_RS_SHIFT			8
#define INSN_MUL_RN_SHIFT			12
#define INSN_MUL_RD_SHIFT			16
#define INSN_OPCODE_SHIFT			21
#define INSN_RN_SHIFT				16
#define INSN_RD_SHIFT				12
#define INSN_COND_SHIFT				28

#define S_CYCLE 1
#define N_CYCLE 2
#define I_CYCLE 1

enum
{
	OPCODE_AND,	/* 0000 */
	OPCODE_EOR,	/* 0001 */
	OPCODE_SUB,	/* 0010 */
	OPCODE_RSB,	/* 0011 */
	OPCODE_ADD,	/* 0100 */
	OPCODE_ADC,	/* 0101 */
	OPCODE_SBC,	/* 0110 */
	OPCODE_RSC,	/* 0111 */
	OPCODE_TST,	/* 1000 */
	OPCODE_TEQ,	/* 1001 */
	OPCODE_CMP,	/* 1010 */
	OPCODE_CMN,	/* 1011 */
	OPCODE_ORR,	/* 1100 */
	OPCODE_MOV,	/* 1101 */
	OPCODE_BIC,	/* 1110 */
	OPCODE_MVN	/* 1111 */
};

enum
{
	COND_EQ = 0,	/* Z: equal */
	COND_NE,		/* ~Z: not equal */
	COND_CS, COND_HS = 2,	/* C: unsigned higher or same */
	COND_CC, COND_LO = 3,	/* ~C: unsigned lower */
	COND_MI,		/* N: negative */
	COND_PL,		/* ~N: positive or zero */
	COND_VS,		/* V: overflow */
	COND_VC,		/* ~V: no overflow */
	COND_HI,		/* C && ~Z: unsigned higher */
	COND_LS,		/* ~C || Z: unsigned lower or same */
	COND_GE,		/* N == V: greater or equal */
	COND_LT,		/* N != V: less than */
	COND_GT,		/* ~Z && (N == V): greater than */
	COND_LE,		/* Z || (N != V): less than or equal */
	COND_AL,		/* always */
	COND_NV			/* never */
};

#define LSL(v,s) ((v) << (s))
#define LSR(v,s) ((v) >> (s))
#define ROL(v,s) (LSL((v),(s)) | (LSR((v),32u - (s))))
#define ROR(v,s) (LSR((v),(s)) | (LSL((v),32u - (s))))

/* Private Data */

/* sArmRegister defines the CPU state */
typedef struct
{
	data32_t sArmRegister[kNumRegisters];
	data32_t coproRegister[16];
	data8_t pendingIrq;
	data8_t pendingFiq;
} ARM_REGS;

static ARM_REGS arm;

int arm_ICount;

/* Prototypes */
static void HandleALU( data32_t insn);
static void HandleMul( data32_t insn);
static void HandleBranch( data32_t insn);
static void HandleMemSingle( data32_t insn);
static void HandleMemBlock( data32_t insn);
static void HandleCoPro( data32_t insn);
static data32_t decodeShift( data32_t insn, data32_t *pCarry);
static void arm_check_irq_state(void);

/***************************************************************************/

static INLINE void cpu_write32( int addr, data32_t data )
{
	/* Unaligned writes are treated as normal writes */
	cpu_writemem26ledw_dword(addr&ADDRESS_MASK,data);
	if (ARM_DEBUG_CORE && addr&3) log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: Unaligned write %08x\n",R15,addr);
}

static INLINE void cpu_write8( int addr, data8_t data )
{
	cpu_writemem26ledw(addr,data);
}

static INLINE data32_t cpu_read32( int addr )
{
	data32_t result = cpu_readmem26ledw_dword(addr&ADDRESS_MASK);

	/* Unaligned reads rotate the word, they never combine words */
	if (addr&3) {
		if (ARM_DEBUG_CORE && addr&1)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: Unaligned byte read %08x\n",R15,addr);

		if ((addr&3)==1)
			return ((result&0x000000ff)<<24)|((result&0xffffff00)>> 8);
		if ((addr&3)==2)
			return ((result&0x0000ffff)<<16)|((result&0xffff0000)>>16);
		if ((addr&3)==3)
			return ((result&0x00ffffff)<< 8)|((result&0xff000000)>>24);
	}

	return result;
}

static INLINE data8_t cpu_read8( int addr )
{
	return cpu_readmem26ledw(addr);
}

static INLINE data32_t GetRegister( int rIndex )
{
	return arm.sArmRegister[sRegisterTable[MODE][rIndex]];
}

static INLINE void SetRegister( int rIndex, data32_t value )
{
	arm.sArmRegister[sRegisterTable[MODE][rIndex]] = value;
}

/***************************************************************************/

void arm_reset(void *param)
{
	memset(&arm, 0, sizeof(arm));

	/* start up in SVC mode with interrupts disabled. */
	R15 = eARM_MODE_SVC|I_MASK|F_MASK;
}

void arm_exit(void)
{
	/* nothing to do here */
}

int arm_execute( int cycles )
{
	data32_t pc;
	data32_t insn;

	arm_ICount = cycles;
	do
	{

#ifdef MAME_DEBUG
		if (mame_debug)
			MAME_Debug();
#endif

		/* load instruction */
		pc = R15;
		insn = READ32( pc & ADDRESS_MASK );

		switch (insn >> INSN_COND_SHIFT)
		{
		case COND_EQ:
			if (Z_IS_CLEAR(pc)) goto L_Next;
			break;
		case COND_NE:
			if (Z_IS_SET(pc)) goto L_Next;
			break;
		case COND_CS:
			if (C_IS_CLEAR(pc)) goto L_Next;
			break;
		case COND_CC:
			if (C_IS_SET(pc)) goto L_Next;
			break;
		case COND_MI:
			if (N_IS_CLEAR(pc)) goto L_Next;
			break;
		case COND_PL:
			if (N_IS_SET(pc)) goto L_Next;
			break;
		case COND_VS:
			if (V_IS_CLEAR(pc)) goto L_Next;
			break;
		case COND_VC:
			if (V_IS_SET(pc)) goto L_Next;
			break;
		case COND_HI:
			if (C_IS_CLEAR(pc) || Z_IS_SET(pc)) goto L_Next;
			break;
		case COND_LS:
			if (C_IS_SET(pc) && Z_IS_CLEAR(pc)) goto L_Next;
			break;
		case COND_GE:
			if (!(pc & N_MASK) != !(pc & V_MASK)) goto L_Next; /* Use x ^ (x >> ...) method */
			break;
		case COND_LT:
			if (!(pc & N_MASK) == !(pc & V_MASK)) goto L_Next;
			break;
		case COND_GT:
			if (Z_IS_SET(pc) || (!(pc & N_MASK) != !(pc & V_MASK))) goto L_Next;
			break;
		case COND_LE:
			if (Z_IS_CLEAR(pc) && (!(pc & N_MASK) == !(pc & V_MASK))) goto L_Next;
			break;
		case COND_NV:
			goto L_Next;
		}
		/* Condition satisfied, so decode the instruction */
		if ((insn & 0x0fc000f0u) == 0x00000090u)	/* Multiplication */
		{
			HandleMul(insn);
			R15 += 4;
		}
		else if (!(insn & 0x0c000000u)) /* Data processing */
		{
			HandleALU(insn);
		}
		else if ((insn & 0x0c000000u) == 0x04000000u) /* Single data access */
		{
			HandleMemSingle(insn);
			R15 += 4;
		}
		else if ((insn & 0x0e000000u) == 0x08000000u ) /* Block data access */
		{
			HandleMemBlock(insn);
			R15 += 4;
		}
		else if ((insn & 0x0e000000u) == 0x0a000000u)	/* Branch */
		{
			HandleBranch(insn);
		}
		else if ((insn & 0x0f000000u) == 0x0e000000u)	/* Coprocessor */
		{
			HandleCoPro(insn);
			R15 += 4;
		}
		else if ((insn & 0x0f000000u) == 0x0f000000u)	/* Software interrupt */
		{
			pc=R15+4;
			R15 = eARM_MODE_SVC;	/* Set SVC mode so PC is saved to correct R14 bank */
			SetRegister( 14, pc );	/* save PC */
			R15 = (pc&PSR_MASK)|(pc&IRQ_MASK)|0x8|eARM_MODE_SVC|I_MASK|(pc&MODE_MASK);
		}
		else /* Undefined */
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Undefined instruction\n",R15);
		L_Next:
			arm_ICount -= S_CYCLE;
			R15 += 4;
		}
		
		arm_check_irq_state();
		
		arm_ICount -= 3;
	} while( arm_ICount > 0 );

	return cycles - arm_ICount;
} /* arm_execute */


unsigned arm_get_context(void *dst)
{
	if( dst )
	{
		memcpy( dst, &arm, sizeof(arm) );
	}
	return sizeof(arm);
}

void arm_set_context(void *src)
{
	if (src)
	{
		memcpy( &arm, src, sizeof(arm) );
	}
}

unsigned arm_get_pc(void)
{
	return R15;/*&ADDRESS_MASK;*/
}

void arm_set_pc(unsigned val)
{
	R15 = (R15&~ADDRESS_MASK)|val;
}

unsigned arm_get_sp(void)
{
	return GetRegister(13);
}

void arm_set_sp(unsigned val)
{
	SetRegister(13,val);
}

unsigned arm_get_reg(int regnum)
{
	switch( regnum )
	{
	case ARM32_R0: return arm.sArmRegister[ 0];
	case ARM32_R1: return arm.sArmRegister[ 1];
	case ARM32_R2: return arm.sArmRegister[ 2];
	case ARM32_R3: return arm.sArmRegister[ 3];
	case ARM32_R4: return arm.sArmRegister[ 4];
	case ARM32_R5: return arm.sArmRegister[ 5];
	case ARM32_R6: return arm.sArmRegister[ 6];
	case ARM32_R7: return arm.sArmRegister[ 7];
	case ARM32_R8: return arm.sArmRegister[ 8];
	case ARM32_R9: return arm.sArmRegister[ 9];
	case ARM32_R10: return arm.sArmRegister[10];
	case ARM32_R11: return arm.sArmRegister[11];
	case ARM32_R12: return arm.sArmRegister[12];
	case ARM32_R13: return arm.sArmRegister[13];
	case ARM32_R14: return arm.sArmRegister[14];
	case ARM32_R15: return arm.sArmRegister[15];

	case ARM32_FR8: return	arm.sArmRegister[eR8_FIQ];
	case ARM32_FR9:	return arm.sArmRegister[eR9_FIQ];
	case ARM32_FR10: return arm.sArmRegister[eR10_FIQ];
	case ARM32_FR11: return arm.sArmRegister[eR11_FIQ];
	case ARM32_FR12: return arm.sArmRegister[eR12_FIQ];
	case ARM32_FR13: return arm.sArmRegister[eR13_FIQ];
	case ARM32_FR14: return arm.sArmRegister[eR14_FIQ];
	case ARM32_IR13: return arm.sArmRegister[eR13_IRQ];
	case ARM32_IR14: return arm.sArmRegister[eR14_IRQ];
	case ARM32_SR13: return arm.sArmRegister[eR13_SVC];
	case ARM32_SR14: return arm.sArmRegister[eR14_SVC];
	case REG_PC: return arm.sArmRegister[15]&ADDRESS_MASK;
	}

	return 0;
}

void arm_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
	case ARM32_R0: arm.sArmRegister[ 0]= val; break;
	case ARM32_R1: arm.sArmRegister[ 1]= val; break;
	case ARM32_R2: arm.sArmRegister[ 2]= val; break;
	case ARM32_R3: arm.sArmRegister[ 3]= val; break;
	case ARM32_R4: arm.sArmRegister[ 4]= val; break;
	case ARM32_R5: arm.sArmRegister[ 5]= val; break;
	case ARM32_R6: arm.sArmRegister[ 6]= val; break;
	case ARM32_R7: arm.sArmRegister[ 7]= val; break;
	case ARM32_R8: arm.sArmRegister[ 8]= val; break;
	case ARM32_R9: arm.sArmRegister[ 9]= val; break;
	case ARM32_R10: arm.sArmRegister[10]= val; break;
	case ARM32_R11: arm.sArmRegister[11]= val; break;
	case ARM32_R12: arm.sArmRegister[12]= val; break;
	case ARM32_R13: arm.sArmRegister[13]= val; break;
	case ARM32_R14: arm.sArmRegister[14]= val; break;
	case ARM32_R15: arm.sArmRegister[15]= val; break;
	case ARM32_FR8: arm.sArmRegister[eR8_FIQ] = val; break;
	case ARM32_FR9: arm.sArmRegister[eR9_FIQ] = val; break;
	case ARM32_FR10: arm.sArmRegister[eR10_FIQ] = val; break;
	case ARM32_FR11: arm.sArmRegister[eR11_FIQ] = val; break;
	case ARM32_FR12: arm.sArmRegister[eR12_FIQ] = val; break;
	case ARM32_FR13: arm.sArmRegister[eR13_FIQ] = val; break;
	case ARM32_FR14: arm.sArmRegister[eR14_FIQ] = val; break;
	case ARM32_IR13: arm.sArmRegister[eR13_IRQ] = val; break;
	case ARM32_IR14: arm.sArmRegister[eR14_IRQ] = val; break;
	case ARM32_SR13: arm.sArmRegister[eR13_SVC] = val; break;
	case ARM32_SR14: arm.sArmRegister[eR14_SVC] = val; break;
	}
}

static void arm_check_irq_state(void)
{
	data32_t pc = R15+4; /* save old pc (already incremented in pipeline) */;

	/* Exception priorities (from ARM6, not specifically ARM2/3):

		Reset
		Data abort
		FIRQ
		IRQ
		Prefetch abort
		Undefined instruction
	*/

	if (arm.pendingFiq && (pc&F_MASK)==0) {
		R15 = eARM_MODE_FIQ;	/* Set FIQ mode so PC is saved to correct R14 bank */
		SetRegister( 14, pc );	/* save PC */
		R15 = (pc&PSR_MASK)|(pc&IRQ_MASK)|0x1c|eARM_MODE_FIQ|I_MASK|F_MASK; /* Mask both IRQ & FIRQ, set PC=0x1c */
		arm.pendingFiq=0;
		return;
	}

	if (arm.pendingIrq && (pc&I_MASK)==0) {
		R15 = eARM_MODE_IRQ;	/* Set IRQ mode so PC is saved to correct R14 bank */
		SetRegister( 14, pc );	/* save PC */
		R15 = (pc&PSR_MASK)|(pc&IRQ_MASK)|0x18|eARM_MODE_IRQ|I_MASK|(pc&F_MASK); /* Mask only IRQ, set PC=0x18 */
		arm.pendingIrq=0;
		return;
	}
}

void arm_set_nmi_line(int state)
{
}

void arm_set_irq_line(int irqline, int state)
{
	switch (irqline) {

	case ARM_IRQ_LINE: /* IRQ */
		if (state && (R15&0x3)!=eARM_MODE_IRQ) /* Don't allow nested IRQs */
			arm.pendingIrq=1;
		else
			arm.pendingIrq=0;
		break;

	case ARM_FIRQ_LINE: /* FIRQ */
		if (state && (R15&0x3)!=eARM_MODE_FIQ) /* Don't allow nested FIRQs */
			arm.pendingFiq=1;
		else
			arm.pendingFiq=0;
		break;
	}

	arm_check_irq_state();
}

void arm_set_irq_callback(int (*callback)(int irqline))
{
}

static const data8_t arm_reg_layout[] =
{
	ARM32_R0, -2,    -1,
	ARM32_R1, -2,    -1,
	ARM32_R2, ARM32_IR13,    -1,
	ARM32_R3, ARM32_IR14,    -1,
	ARM32_R4, -2,  -1,
	ARM32_R5, ARM32_SR13,  -1,
	ARM32_R6, ARM32_SR14,  -1,
	ARM32_R7, -2,  -1,
	ARM32_R8, ARM32_FR8,   -1,
	ARM32_R9, ARM32_FR9,   -1,
	ARM32_R10,ARM32_FR10,  -1,
	ARM32_R11,ARM32_FR11,  -1,
	ARM32_R12,ARM32_FR12,  -1,
	ARM32_R13,ARM32_FR13,  -1,
	ARM32_R14,ARM32_FR14,  -1,
	ARM32_R15,0
};

static const UINT8 arm_win_layout[] = {
	 0, 0,29,17,	/* register window (top rows) */
	30, 0,50,17,	/* disassembler window (left colums) */
	 0,18,48, 4,	/* memory #1 window (right, upper middle) */
	49,18,31, 4,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

const char *arm_info(void *context, int regnum)
{
	static char buffer[32][63+1];
	static int which = 0;

	ARM_REGS *pRegs = context;
	if( !context )
		pRegs = &arm;

	which = (which + 1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
	case CPU_INFO_REG + ARM32_R0: sprintf( buffer[which], "R0  :%08x", pRegs->sArmRegister[ 0] );  break;
	case CPU_INFO_REG + ARM32_R1: sprintf( buffer[which], "R1  :%08x", pRegs->sArmRegister[ 1] );  break;
	case CPU_INFO_REG + ARM32_R2: sprintf( buffer[which], "R2  :%08x", pRegs->sArmRegister[ 2] );  break;
	case CPU_INFO_REG + ARM32_R3: sprintf( buffer[which], "R3  :%08x", pRegs->sArmRegister[ 3] );  break;
	case CPU_INFO_REG + ARM32_R4: sprintf( buffer[which], "R4  :%08x", pRegs->sArmRegister[ 4] );  break;
	case CPU_INFO_REG + ARM32_R5: sprintf( buffer[which], "R5  :%08x", pRegs->sArmRegister[ 5] );  break;
	case CPU_INFO_REG + ARM32_R6: sprintf( buffer[which], "R6  :%08x", pRegs->sArmRegister[ 6] );  break;
	case CPU_INFO_REG + ARM32_R7: sprintf( buffer[which], "R7  :%08x", pRegs->sArmRegister[ 7] );  break;
	case CPU_INFO_REG + ARM32_R8: sprintf( buffer[which], "R8  :%08x", pRegs->sArmRegister[ 8] );  break;
	case CPU_INFO_REG + ARM32_R9: sprintf( buffer[which], "R9  :%08x", pRegs->sArmRegister[ 9] );  break;
	case CPU_INFO_REG + ARM32_R10:sprintf( buffer[which], "R10 :%08x", pRegs->sArmRegister[10] );  break;
	case CPU_INFO_REG + ARM32_R11:sprintf( buffer[which], "R11 :%08x", pRegs->sArmRegister[11] );  break;
	case CPU_INFO_REG + ARM32_R12:sprintf( buffer[which], "R12 :%08x", pRegs->sArmRegister[12] );  break;
	case CPU_INFO_REG + ARM32_R13:sprintf( buffer[which], "R13 :%08x", pRegs->sArmRegister[13] );  break;
	case CPU_INFO_REG + ARM32_R14:sprintf( buffer[which], "R14 :%08x", pRegs->sArmRegister[14] );  break;
	case CPU_INFO_REG + ARM32_R15:sprintf( buffer[which], "R15 :%08x", pRegs->sArmRegister[15] );  break;
	case CPU_INFO_REG + ARM32_FR8: sprintf( buffer[which], "FR8 :%08x", pRegs->sArmRegister[eR8_FIQ] );  break;
	case CPU_INFO_REG + ARM32_FR9: sprintf( buffer[which], "FR9 :%08x", pRegs->sArmRegister[eR9_FIQ] );  break;
	case CPU_INFO_REG + ARM32_FR10:sprintf( buffer[which], "FR10:%08x", pRegs->sArmRegister[eR10_FIQ] );  break;
	case CPU_INFO_REG + ARM32_FR11:sprintf( buffer[which], "FR11:%08x", pRegs->sArmRegister[eR11_FIQ]);  break;
	case CPU_INFO_REG + ARM32_FR12:sprintf( buffer[which], "FR12:%08x", pRegs->sArmRegister[eR12_FIQ] );  break;
	case CPU_INFO_REG + ARM32_FR13:sprintf( buffer[which], "FR13:%08x", pRegs->sArmRegister[eR13_FIQ] );  break;
	case CPU_INFO_REG + ARM32_FR14:sprintf( buffer[which], "FR14:%08x", pRegs->sArmRegister[eR14_FIQ] );  break;
	case CPU_INFO_REG + ARM32_IR13:sprintf( buffer[which], "IR13:%08x", pRegs->sArmRegister[eR13_IRQ] );  break;
	case CPU_INFO_REG + ARM32_IR14:sprintf( buffer[which], "IR14:%08x", pRegs->sArmRegister[eR14_IRQ] );  break;
	case CPU_INFO_REG + ARM32_SR13:sprintf( buffer[which], "SR13:%08x", pRegs->sArmRegister[eR13_SVC] );  break;
	case CPU_INFO_REG + ARM32_SR14:sprintf( buffer[which], "SR14:%08x", pRegs->sArmRegister[eR14_SVC] );  break;


	case CPU_INFO_FLAGS:
		sprintf(buffer[which], "%c%c%c%c%c%c",
			(pRegs->sArmRegister[15] & N_MASK) ? 'N' : '-',
			(pRegs->sArmRegister[15] & Z_MASK) ? 'Z' : '-',
			(pRegs->sArmRegister[15] & C_MASK) ? 'C' : '-',
			(pRegs->sArmRegister[15] & V_MASK) ? 'V' : '-',
			(pRegs->sArmRegister[15] & I_MASK) ? 'I' : '-',
			(pRegs->sArmRegister[15] & F_MASK) ? 'F' : '-');
		switch (pRegs->sArmRegister[15] & 3)
		{
		case 0:
			strcat(buffer[which], " USER");
			break;
		case 1:
			strcat(buffer[which], " FIRQ");
			break;
		case 2:
			strcat(buffer[which], " IRQ ");
			break;
		default:
			strcat(buffer[which], " SVC ");
			break;
		}
		break;
	case CPU_INFO_NAME: 		return "ARM";
	case CPU_INFO_FAMILY:		return "Acorn Risc Machine";
	case CPU_INFO_VERSION:		return "1.2";
	case CPU_INFO_FILE: 		return __FILE__;
	case CPU_INFO_CREDITS:		return "Copyright 2002 Bryan McPhail, bmcphail@tendril.co.uk";
	case CPU_INFO_REG_LAYOUT:	return (const char*)arm_reg_layout;
	case CPU_INFO_WIN_LAYOUT:	return (const char*)arm_win_layout;
	}

	return buffer[which];
}

unsigned arm_dasm(char *buffer, unsigned int pc)
{
#ifdef MAME_DEBUG
	arm_disasm( buffer, pc, cpu_read32(pc&ADDRESS_MASK) );
	return 4;
#else
	sprintf(buffer, "$%08x", READ32(pc));
	return 4;
#endif
}

void arm_init(void)
{
	int cpu = cpu_getactivecpu(),i;
	char buf[8];

	for (i=0; i<kNumRegisters; i++) {
		sprintf(buf,"R%d",i);
		state_save_register_UINT32("arm", cpu, buf, &arm.sArmRegister[i], 4);
	}
	state_save_register_UINT8("arm", cpu, "IRQ", &arm.pendingIrq, 1);
	state_save_register_UINT8("arm", cpu, "FIQ", &arm.pendingFiq, 1);

	return;
}

/***************************************************************************/

static void HandleBranch(  data32_t insn )
{
	data32_t off = (insn & INSN_BRANCH) << 2;

	/* Save PC into LR if this is a branch with link */
	if (insn & INSN_BL)
	{
		SetRegister(14,R15 + 4);
	}

	/* Sign-extend the 24-bit offset in our calculations */
	if (off & 0x2000000u)
	{
		R15 -= ((~(off | 0xfc000000u)) + 1) - 8;
	}
	else
	{
		R15 += off + 8;
	}
}

static void HandleMemSingle( data32_t insn )
{
	data32_t rn, rnv, off, rd;

	/* Fetch the offset */
	if (insn & INSN_I)
	{
		off = decodeShift(insn, NULL);
	}
	else
	{
		off = insn & INSN_SDT_IMM;
	}

	/* Calculate Rn, accounting for PC */
	rn = (insn & INSN_RN) >> INSN_RN_SHIFT;

/*	if (rn==0xf) log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Source R15\n",R15);*/

	if (insn & INSN_SDT_P)
	{
		/* Pre-indexed addressing */
		if (insn & INSN_SDT_U)
		{
			rnv = (GetRegister(rn) + off);
		}
		else
		{
			rnv = (GetRegister(rn) - off);
		}

		if (insn & INSN_SDT_W)
		{
			SetRegister(rn,rnv);
	        if (ARM_DEBUG_CORE && rn == eR15)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "writeback R15 %08x\n", R15);
		}
		else if (rn == eR15)
		{
			rnv = (rnv & ADDRESS_MASK) + 8;
		}
	}
	else
	{
		/* Post-indexed addressing */
		if (rn == eR15)
		{
			rnv = (R15 & ADDRESS_MASK) + 8;
		}
		else
		{
			rnv = GetRegister(rn);
		}
	}

	/* Do the transfer */
	rd = (insn & INSN_RD) >> INSN_RD_SHIFT;
	if (insn & INSN_SDT_L)
	{
		/* Load */
		if (insn & INSN_SDT_B)
		{
			if (ARM_DEBUG_CORE && rd == eR15)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "read byte R15 %08x\n", R15);
			SetRegister(rd,(data32_t) READ8(rnv));
		}
		else
		{
			if (rd == eR15)
			{
				R15 = (READ32(rnv) & ADDRESS_MASK) | (R15 & PSR_MASK) | (R15 & MODE_MASK);
				/*
                The docs are explicit in that the bottom bits should be masked off
                when writing to R15 in this way, however World Cup Volleyball 95 has
                an example of an unaligned jump (bottom bits = 2) where execution
                should definitely continue from the rounded up address.

                In other cases, 4 is subracted from R15 here to account for pipelining.
                */
				if ((READ32(rnv)&3)==0)
					R15 -= 4;
			}
			else
			{
				SetRegister(rd,READ32(rnv));
			}
		}
	}
	else
	{
		/* Store */
		if (insn & INSN_SDT_B)
		{
			if (ARM_DEBUG_CORE && rd==eR15)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Wrote R15 in byte mode\n");

			WRITE8(rnv, (data8_t) GetRegister(rd) & 0xffu);
		}
		else
		{
			if (ARM_DEBUG_CORE && rd==eR15)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Wrote R15 in 32bit mode\n");

			WRITE32(rnv, rd == eR15 ? R15 + 8 : GetRegister(rd));
		}
	}

	/* Do post-indexing writeback */
	if (!(insn & INSN_SDT_P)/* && (insn&INSN_SDT_W)*/)
	{
		if (insn & INSN_SDT_U)
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SetRegister(rn,GetRegister(rd));
			}
			else {

				if ((insn&INSN_SDT_W)!=0)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  RegisterWritebackIncrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0);

				SetRegister(rn,(rnv + off));
			}
		}
		else
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SetRegister(rn,GetRegister(rd));
			}
			else {
				SetRegister(rn,(rnv - off));

				if ((insn&INSN_SDT_W)!=0)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  RegisterWritebackDecrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0);
			}
		}
	}

} /* HandleMemSingle */

#define IsNeg(i) ((i) >> 31)
#define IsPos(i) ((~(i)) >> 31)

/* Set NZCV flags for ADDS / SUBS */

#define HandleALUAddFlags(rd, rn, op2) \
  if (insn & INSN_S) \
    R15 = \
      ((R15 &~ (N_MASK | Z_MASK | V_MASK | C_MASK)) \
      | (((!SIGN_BITS_DIFFER(rn, op2)) && SIGN_BITS_DIFFER(rn, rd)) \
          << V_BIT) \
      | (((IsNeg(rn) & IsPos(op2)) | (IsNeg(rn) & IsPos(rd)) | (IsPos(op2) & IsPos(rd))) ? C_MASK : 0) \
      | HandleALUNZFlags(rd)) \
      + 4; \
  else R15 += 4;

#define HandleALUSubFlags(rd, rn, op2) \
  if (insn & INSN_S) \
    R15 = \
      ((R15 &~ (N_MASK | Z_MASK | V_MASK | C_MASK)) \
      | ((SIGN_BITS_DIFFER(rn, op2) && SIGN_BITS_DIFFER(rn, rd)) \
          << V_BIT) \
      | (((op2) <= (rn)) << C_BIT) \
      | HandleALUNZFlags(rd)) \
      + 4; \
  else R15 += 4;

/* Set NZC flags for logical operations. */

#define HandleALUNZFlags(rd) \
  (((rd) & SIGN_BIT) | ((!(rd)) << Z_BIT))

#define HandleALULogicalFlags(rd, sc) \
  if (insn & INSN_S) \
    R15 = ((R15 &~ (N_MASK | Z_MASK | C_MASK)) \
                     | HandleALUNZFlags(rd) \
                     | (((sc) != 0) << C_BIT)) + 4; \
  else R15 += 4;

static void HandleALU( data32_t insn )
{
	data32_t op2, sc=0, rd, rn, opcode;
	data32_t by, rdn;

	opcode = (insn & INSN_OPCODE) >> INSN_OPCODE_SHIFT;

	rd = 0;
	rn = 0;

	/* Construct Op2 */
	if (insn & INSN_I)
	{
		/* Immediate constant */
		by = (insn & INSN_OP2_ROTATE) >> INSN_OP2_ROTATE_SHIFT;
		if (by)
		{
			op2 = ROR(insn & INSN_OP2_IMM, by << 1);
			sc = op2 & SIGN_BIT;
		}
		else
		{
			op2 = insn & INSN_OP2;
			sc = R15 & C_MASK;
		}
	}
	else
	{
		op2 = decodeShift(insn, (insn & INSN_S && (opcode & 4) == 4)? &sc : NULL);

		if (!(insn & INSN_S && (opcode & 4) == 4))
			sc=0;
	}

	/* Calculate Rn to account for pipelining */
	if ((opcode & 0xd) != 0xd) /* No Rn in MOV */
	{
		if ((rn = (insn & INSN_RN) >> INSN_RN_SHIFT) == eR15)
		{
			if (ARM_DEBUG_CORE)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Pipelined R15 (Shift %d)\n",R15,(insn&INSN_I?8:insn&0x10u?12:12));

			/* Docs strongly suggest the mode bits should be included here, but it breaks Captain
			America, as it starts doing unaligned reads */
			rn=(R15+8)&ADDRESS_MASK;
		}
		else
		{
			rn = GetRegister(rn);
		}
	}

	/* Perform the operation */
	switch ((insn & INSN_OPCODE) >> INSN_OPCODE_SHIFT)
	{
	/* Arithmetic operations */
	case OPCODE_SBC:
		rd = (rn - op2 - (R15 & C_MASK ? 0 : 1));
		HandleALUSubFlags(rd, rn, op2);
		break;
	case OPCODE_CMP:
	case OPCODE_SUB:
		rd = (rn - op2);
		HandleALUSubFlags(rd, rn, op2);
		break;
	case OPCODE_RSC:
		rd = (op2 - rn - (R15 & C_MASK ? 0 : 1));
		HandleALUSubFlags(rd, op2, rn);
		break;
	case OPCODE_RSB:
		rd = (op2 - rn);
		HandleALUSubFlags(rd, op2, rn);
		break;
	case OPCODE_ADC:
		rd = (rn + op2 + ((R15 & C_MASK) >> C_BIT));
		HandleALUAddFlags(rd, rn, op2);
		break;
	case OPCODE_CMN:
	case OPCODE_ADD:
		rd = (rn + op2);
		HandleALUAddFlags(rd, rn, op2);
		break;

	/* Logical operations */
	case OPCODE_AND:
	case OPCODE_TST:
		rd = rn & op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_BIC:
		rd = rn &~ op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_TEQ:
	case OPCODE_EOR:
		rd = rn ^ op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_ORR:
		rd = rn | op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_MOV:
		rd = op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_MVN:
		rd = (~op2);
		HandleALULogicalFlags(rd, sc);
		break;
	}

	/* Put the result in its register if not a test */
	rdn = (insn & INSN_RD) >> INSN_RD_SHIFT;
	if ((opcode & 0xc) != 0x8)
	{
		if (rdn == eR15 && !(insn & INSN_S))
		{
			/* Merge the old NZCV flags into the new PC value */
			R15 = (rd & ADDRESS_MASK) | (R15 & PSR_MASK) | (R15 & IRQ_MASK) | (R15&MODE_MASK);  

		}
		else
		{
			if (rdn==eR15)
			{
				/* S Flag is set - update PSR & mode if in non-user mode only */
				if ((R15&MODE_MASK)!=0)
				{
					SetRegister(rdn,rd);
				}
				else
				{
					SetRegister(rdn,(rd&ADDRESS_MASK) | (rd&PSR_MASK) | (R15&IRQ_MASK) | (R15&MODE_MASK));
				}
			}
			else
			{
				SetRegister(rdn,rd);
			}
		}
	/* TST & TEQ can affect R15 (the condition code register) with the S bit set */
	} else if (rdn==eR15) {
		if (insn & INSN_S) {
			if (ARM_DEBUG_CORE)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: TST class on R15 s bit set\n",R15);
			
			/* Dubious hack for 'TEQS R15, #$3', the docs suggest execution
                should continue two instructions later (because pipelined R15
                is read back as already being incremented), but it seems the
                hardware should execute the instruction in the delay slot.
                Simulate it by just setting the PC back to the previously
                skipped instruction.

                See Heavy Smash (Data East) at 0x1c4
            */
			if (insn==0xe33ff003)
				rd-=4;

			if ((R15&MODE_MASK)!=0)
			{
				SetRegister(15, rd);
			}
			else
			{
				SetRegister(15, (rd&ADDRESS_MASK) | (rd&PSR_MASK) | (R15&IRQ_MASK) | (R15&MODE_MASK));
			}
		} else {
			if (ARM_DEBUG_CORE)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: TST class on R15 no s bit set\n",R15);
		}
	}
}

static void HandleMul( data32_t insn)
{
	data32_t r;

	/* Do the basic multiply of Rm and Rs */
	r =	GetRegister( insn&INSN_MUL_RM ) *
	  	GetRegister( (insn&INSN_MUL_RS)>>INSN_MUL_RS_SHIFT );

	if (ARM_DEBUG_CORE && ((insn&INSN_MUL_RM)==0xf
		|| ((insn&INSN_MUL_RS)>>INSN_MUL_RS_SHIFT )==0xf
		|| ((insn&INSN_MUL_RN)>>INSN_MUL_RN_SHIFT)==0xf)
		)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  R15 used in mult\n",R15);

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		r += GetRegister((insn&INSN_MUL_RN)>>INSN_MUL_RN_SHIFT);
	}

	/* Write the result */
	SetRegister((insn&INSN_MUL_RD)>>INSN_MUL_RD_SHIFT,r);

	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		R15 = (R15 &~ (N_MASK | Z_MASK)) | HandleALUNZFlags(r);
	}
}

static int loadInc ( data32_t pat, data32_t rbv, data32_t s)
{
	int i,result;

	result = 0;
	for( i=0; i<16; i++ )
	{
		if( (pat>>i)&1 )
		{
			if (i==15) {
				if (s) /* Pull full contents from stack */
					SetRegister( 15, READ32(rbv+=4) );
				else /* Pull only address, preserve mode & status flags */
					SetRegister( 15, (R15&PSR_MASK) | (R15&IRQ_MASK) | (R15&MODE_MASK) | ((READ32(rbv+=4))&ADDRESS_MASK) );
			} else
				SetRegister( i, READ32(rbv+=4) );

			result++;
		}
	}
	return result;
}

static int loadDec( data32_t pat, data32_t rbv, data32_t s, data32_t* deferredR15, int* defer)
{
	int i,result;

	result = 0;
	for( i=15; i>=0; i-- )
	{
		if( (pat>>i)&1 )
		{
			if (i==15) {
				*defer=1;
				if (s) /* Pull full contents from stack */
					*deferredR15=READ32(rbv-=4);
				else /* Pull only address, preserve mode & status flags */
					*deferredR15=(R15&PSR_MASK) | (R15&IRQ_MASK) | (R15&MODE_MASK) | ((READ32(rbv-=4))&ADDRESS_MASK);
			}
			else
				SetRegister( i, READ32(rbv -=4) );
			result++;
		}
	}
	return result;
}

static int storeInc( data32_t pat, data32_t rbv)
{
	int i,result;

	result = 0;
	for( i=0; i<16; i++ )
	{
		if( (pat>>i)&1 )
		{
			if (ARM_DEBUG_CORE && i==15) /* R15 is plus 12 from address of STM */
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: StoreInc on R15\n",R15);

			WRITE32( rbv += 4, GetRegister(i) );
			result++;
		}
	}
	return result;
} /* storeInc */

static int storeDec( data32_t pat, data32_t rbv)
{
	int i,result;

	result = 0;
	for( i=15; i>=0; i-- )
	{
		if( (pat>>i)&1 )
		{
			if (ARM_DEBUG_CORE && i==15) /* R15 is plus 12 from address of STM */
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: StoreDec on R15\n",R15);

			WRITE32( rbv -= 4, GetRegister(i) );
			result++;
		}
	}
	return result;
} /* storeDec */

static void HandleMemBlock( data32_t insn)
{
	data32_t rb = (insn & INSN_RN) >> INSN_RN_SHIFT;
	data32_t rbp = GetRegister(rb);
	int result;

	if (ARM_DEBUG_CORE && insn & INSN_BDT_S)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  S Bit set in MEMBLOCK\n",R15);

	if (insn & INSN_BDT_L)
	{
		/* Loading */
		if (insn & INSN_BDT_U)
		{
			/* Incrementing */
			if (!(insn & INSN_BDT_P)) rbp = rbp + (- 4);

			result = loadInc( insn & 0xffff, rbp, insn&INSN_BDT_S );

			if (insn & 0x8000) {
				R15-=4;
			}

			if (insn & INSN_BDT_W)
			{
			   /* Arm docs notes: The base register can always be loaded without any problems.
                           However, don't specify writeback if the base register is being loaded -
                           you can't end up with both a written-back value and a loaded value in the base register!
                           However - Fighter's History does exactly that at 0x121e4 (LDMUW [R13], { R13-R15 })!

                          This emulator implementation skips applying writeback in this case, which is confirmed
                          correct for this situation, but that is not necessarily true for all ARM hardware
                          implementations (the results are officially undefined).
                          */
			  if (ARM_DEBUG_CORE && rb==15)
			      log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Illegal LDRM writeback to r15\n",R15);

			  if ((insn&(1<<rb))==0)
			      SetRegister(rb,GetRegister(rb)+result*4);
			  else if (ARM_DEBUG_CORE)
			       log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Illegal LDRM writeback to base register (%d)\n",R15, rb);
			}
		}
		else
		{
			data32_t deferredR15=0;
			int defer=0;
			
			/* Decrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp - (- 4);
			}

			result = loadDec( insn&0xffff, rbp, insn&INSN_BDT_S, &deferredR15, &defer );

			if (insn & INSN_BDT_W)
			{
				if (rb==0xf)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Illegal LDRM writeback to r15\n",R15);
				SetRegister(rb,GetRegister(rb)-result*4);
			}
			/* If R15 is pulled from memory we defer setting it until after writeback
			   is performed, else we may writeback to the wrong context (ie, the new
			   context if the mode has changed as a result of the R15 read) */
			if (defer)
				SetRegister(15, deferredR15);

			if (insn & 0x8000) {
				R15-=4;
			}
		}
	} /* Loading */
	else
	{
		/* Storing */
		if (insn & (1<<eR15))
		{
			if (ARM_DEBUG_CORE)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: Writing R15 in strm\n",R15);

			/* special case handling if writing to PC */
			R15 += 12;
		}
		if (insn & INSN_BDT_U)
		{
			/* Incrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp + (- 4);
			}
			result = storeInc( insn&0xffff, rbp );
			if( insn & INSN_BDT_W )
			{
				SetRegister(rb,GetRegister(rb)+result*4);
			}
		}
		else
		{
			/* Decrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp - (- 4);
			}
			result = storeDec( insn&0xffff, rbp );
			if( insn & INSN_BDT_W )
			{
				SetRegister(rb,GetRegister(rb)-result*4);
			}
		}
		if( insn & (1<<eR15) )
			R15 -= 12;
	}
} /* HandleMemBlock */



/* Decodes an Op2-style shifted-register form.  If @carry@ is non-zero the
 * shifter carry output will manifest itself as @*carry == 0@ for carry clear
 * and @*carry != 0@ for carry set.
 */
static data32_t decodeShift( data32_t insn, data32_t *pCarry)
{
	data32_t k	= (insn & INSN_OP2_SHIFT) >> INSN_OP2_SHIFT_SHIFT;
	data32_t rm	= GetRegister( insn & INSN_OP2_RM );
	data32_t t	= (insn & INSN_OP2_SHIFT_TYPE) >> INSN_OP2_SHIFT_TYPE_SHIFT;

	if ((insn & INSN_OP2_RM)==0xf) {
		/* If hardwired shift, then PC is 8 bytes ahead, else if register shift
		is used, then 12 bytes - TODO?? */
		rm+=8;
	}

	/* All shift types ending in 1 are Rk, not #k */
	if( t & 1 )
	{
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  RegShift %02x %02x\n",R15, k>>1,GetRegister(k >> 1));*/
		if (ARM_DEBUG_CORE && (insn&0x80)==0x80)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  RegShift ERROR (p36)\n",R15);

		/*see p35 for check on this*/
		k = GetRegister(k >> 1)&0x1f;
		if( k == 0 ) /* Register shift by 0 is a no-op */
		{
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  NO-OP Regshift\n",R15);*/
			if (pCarry) *pCarry = R15 & C_MASK;
			return rm;
		}
	}
	/* Decode the shift type and perform the shift */
	switch (t >> 1)
	{
	case 0:						/* LSL */
		if (pCarry)
		{
			*pCarry = k ? (rm & (1 << (32 - k))) : (R15 & C_MASK);
		}
		return k ? LSL(rm, k) : rm;
		break;

	case 1:			       			/* LSR */
		if (k == 0 || k == 32)
		{
			if (pCarry) *pCarry = rm & SIGN_BIT;
			return 0;
		}
		else if (k > 32)
		{
			if (pCarry) *pCarry = 0;
			return 0;
		}
		else
		{
			if (pCarry) *pCarry = (rm & (1 << (k - 1)));
			return LSR(rm, k);
		}
		break;

	case 2:						/* ASR */
		if (k == 0 || k > 32)
			k = 32;
		if (pCarry) *pCarry = (rm & (1 << (k - 1)));
		if (k >= 32)
			return rm & SIGN_BIT ? 0xffffffffu : 0;
		else
		{
			if (rm & SIGN_BIT)
				return LSR(rm, k) | (0xffffffffu << (32 - k));
			else
				return LSR(rm, k);
		}
		break;

	case 3:						/* ROR and RRX */
		if (k)
		{
			while (k > 32) k -= 32;
			if (pCarry) *pCarry = rm & SIGN_BIT;
			return ROR(rm, k);
		}
		else
		{
			if (pCarry) *pCarry = (rm & 1);
			return LSR(rm, 1) | ((R15 & C_MASK) << 2);
		}
		break;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: Decodeshift error\n",R15);
	return 0;
} /* decodeShift */

static data32_t BCDToDecimal(data32_t value)
{
	data32_t	accumulator = 0;
	data32_t	multiplier = 1;
	int		i;

	for(i = 0; i < 8; i++)
	{
		accumulator += (value & 0xF) * multiplier;

		multiplier *= 10;
		value >>= 4;
	}

	return accumulator;
}

static data32_t DecimalToBCD(data32_t value)
{
	data32_t	accumulator = 0;
	data32_t	divisor = 10;
	int		i;

	for(i = 0; i < 8; i++)
	{
		data32_t temp;

		temp = value % divisor;
		value -= temp;
		temp /= divisor / 10;

		accumulator += temp << (i * 4);

		divisor *= 10;
	}

	return accumulator;
}

static void HandleCoPro( data32_t insn)
{
	data32_t rn=(insn>>12)&0xf;
	data32_t crn=(insn>>16)&0xf;

	/* MRC - transfer copro register to main register */
	if( (insn&0x0f100010)==0x0e100010 )
	{
		SetRegister(rn, arm.coproRegister[crn]);

		if (ARM_DEBUG_COPRO)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Copro read CR%d (%08x) to R%d\n", R15, crn, arm.coproRegister[crn], rn);
	}
	/* MCR - transfer main register to copro register */
	else if( (insn&0x0f100010)==0x0e000010 )
	{
		arm.coproRegister[crn]=GetRegister(rn);

		/* Data East 156 copro specific - trigger BCD operation */
		if (crn==2)
		{
			if (arm.coproRegister[crn]==0)
			{
				/* Unpack BCD */
				int v0=BCDToDecimal(arm.coproRegister[0]);
				int v1=BCDToDecimal(arm.coproRegister[1]);

				/* Repack vcd */
				arm.coproRegister[5]=DecimalToBCD(v0+v1);

				if (ARM_DEBUG_COPRO)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Cmd:  Add 0 + 1, result in 5 (%08x + %08x == %08x)\n", v0, v1, arm.coproRegister[5]);
			}
			else if (arm.coproRegister[crn]==1)
			{
				/* Unpack BCD */
				int v0=BCDToDecimal(arm.coproRegister[0]);
				int v1=BCDToDecimal(arm.coproRegister[1]);

				/* Repack vcd */
				arm.coproRegister[5]=DecimalToBCD(v0*v1);

				if (ARM_DEBUG_COPRO)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Cmd:  Multiply 0 * 1, result in 5 (%08x * %08x == %08x)\n", v0, v1, arm.coproRegister[5]);
			}
			else if (arm.coproRegister[crn]==3)
			{
				/* Unpack BCD */
				int v0=BCDToDecimal(arm.coproRegister[0]);
				int v1=BCDToDecimal(arm.coproRegister[1]);

				/* Repack vcd */
				arm.coproRegister[5]=DecimalToBCD(v0-v1);

				if (ARM_DEBUG_COPRO)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Cmd:  Sub 0 - 1, result in 5 (%08x - %08x == %08x)\n", v0, v1, arm.coproRegister[5]);
			}
			else
			{
				usrintf_showmessage("Unknown bcd copro command %08x\n", arm.coproRegister[crn]);
			}
		}

		if (ARM_DEBUG_COPRO)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Copro write R%d (%08x) to CR%d\n", R15, rn, GetRegister(rn), crn);
	}
	/* CDP - perform copro operation */
	else if( (insn&0x0f000010)==0x0e000000 )
	{
		/* Data East 156 copro specific divider - result in reg 3/4 */
		if (arm.coproRegister[1])
		{
			arm.coproRegister[3]=arm.coproRegister[0] / arm.coproRegister[1];
			arm.coproRegister[4]=arm.coproRegister[0] % arm.coproRegister[1];
		}
		else
		{
			/* Unverified */
			arm.coproRegister[3]=0xffffffff;
			arm.coproRegister[4]=0xffffffff;
		}

		if (ARM_DEBUG_COPRO)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Copro cdp (%08x) (3==> %08x, 4==> %08x)\n", R15, insn, arm.coproRegister[3], arm.coproRegister[4]);
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x:  Unimplemented copro instruction %08x\n", R15, insn);
	}
}
