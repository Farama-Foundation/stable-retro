/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef __DISASSEMBLER_H__
#define __DISASSEMBLER_H__

#include "debug.h"
#include "lightrec.h"
#include "lightrec-config.h"

#ifndef __packed
#define __packed __attribute__((packed))
#endif

#define BIT(x) (1ULL << (x))

/* Flags for all opcodes */
#define LIGHTREC_NO_DS		BIT(0)
#define LIGHTREC_SYNC		BIT(1)

/* Flags for LUI, ORI, ADDIU */
#define LIGHTREC_MOVI		BIT(2)

/* Flags for load/store opcodes */
#define LIGHTREC_SMC		BIT(2)
#define LIGHTREC_NO_INVALIDATE	BIT(3)
#define LIGHTREC_NO_MASK	BIT(4)
#define LIGHTREC_LOAD_DELAY	BIT(5)

/* I/O mode for load/store opcodes */
#define LIGHTREC_IO_MODE_LSB	6
#define LIGHTREC_IO_MODE(x)	((x) << LIGHTREC_IO_MODE_LSB)
#define LIGHTREC_IO_UNKNOWN	0x0
#define LIGHTREC_IO_DIRECT	0x1
#define LIGHTREC_IO_HW		0x2
#define LIGHTREC_IO_RAM		0x3
#define LIGHTREC_IO_BIOS	0x4
#define LIGHTREC_IO_SCRATCH	0x5
#define LIGHTREC_IO_DIRECT_HW	0x6
#define LIGHTREC_IO_MASK	LIGHTREC_IO_MODE(0x7)
#define LIGHTREC_FLAGS_GET_IO_MODE(x) \
	(((x) & LIGHTREC_IO_MASK) >> LIGHTREC_IO_MODE_LSB)

/* Flags for branches */
#define LIGHTREC_EMULATE_BRANCH	BIT(2)
#define LIGHTREC_LOCAL_BRANCH	BIT(3)

/* Flags for div/mult opcodes */
#define LIGHTREC_NO_LO		BIT(2)
#define LIGHTREC_NO_HI		BIT(3)
#define LIGHTREC_NO_DIV_CHECK	BIT(4)

#define LIGHTREC_REG_RS_LSB	26
#define LIGHTREC_REG_RS(x)	((x) << LIGHTREC_REG_RS_LSB)
#define LIGHTREC_REG_RS_MASK	LIGHTREC_REG_RS(0x3)
#define LIGHTREC_FLAGS_GET_RS(x) \
	(((x) & LIGHTREC_REG_RS_MASK) >> LIGHTREC_REG_RS_LSB)

#define LIGHTREC_REG_RT_LSB	28
#define LIGHTREC_REG_RT(x)	((x) << LIGHTREC_REG_RT_LSB)
#define LIGHTREC_REG_RT_MASK	LIGHTREC_REG_RT(0x3)
#define LIGHTREC_FLAGS_GET_RT(x) \
	(((x) & LIGHTREC_REG_RT_MASK) >> LIGHTREC_REG_RT_LSB)

#define LIGHTREC_REG_RD_LSB	30
#define LIGHTREC_REG_RD(x)	((x) << LIGHTREC_REG_RD_LSB)
#define LIGHTREC_REG_RD_MASK	LIGHTREC_REG_RD(0x3)
#define LIGHTREC_FLAGS_GET_RD(x) \
	(((x) & LIGHTREC_REG_RD_MASK) >> LIGHTREC_REG_RD_LSB)

#define LIGHTREC_REG_NOOP	0x0
#define LIGHTREC_REG_UNLOAD	0x1
#define LIGHTREC_REG_DISCARD	0x2
#define LIGHTREC_REG_CLEAN	0x3

struct block;

enum standard_opcodes {
	OP_SPECIAL		= 0x00,
	OP_REGIMM		= 0x01,
	OP_J			= 0x02,
	OP_JAL			= 0x03,
	OP_BEQ			= 0x04,
	OP_BNE			= 0x05,
	OP_BLEZ			= 0x06,
	OP_BGTZ			= 0x07,
	OP_ADDI			= 0x08,
	OP_ADDIU		= 0x09,
	OP_SLTI			= 0x0a,
	OP_SLTIU		= 0x0b,
	OP_ANDI			= 0x0c,
	OP_ORI			= 0x0d,
	OP_XORI			= 0x0e,
	OP_LUI			= 0x0f,
	OP_CP0			= 0x10,
	OP_CP2			= 0x12,
	OP_LB			= 0x20,
	OP_LH			= 0x21,
	OP_LWL			= 0x22,
	OP_LW			= 0x23,
	OP_LBU			= 0x24,
	OP_LHU			= 0x25,
	OP_LWR			= 0x26,
	OP_SB			= 0x28,
	OP_SH			= 0x29,
	OP_SWL			= 0x2a,
	OP_SW			= 0x2b,
	OP_SWR			= 0x2e,
	OP_LWC2			= 0x32,
	OP_SWC2			= 0x3a,

	OP_META_BIOS		= 0x3b,

	OP_META			= 0x3c,

	OP_META_MULT2		= 0x19,
	OP_META_MULTU2		= 0x1a,
	OP_META_LWU		= 0x1b,
	OP_META_SWU		= 0x1c,
};

enum special_opcodes {
	OP_SPECIAL_SLL		= 0x00,
	OP_SPECIAL_SRL		= 0x02,
	OP_SPECIAL_SRA		= 0x03,
	OP_SPECIAL_SLLV		= 0x04,
	OP_SPECIAL_SRLV		= 0x06,
	OP_SPECIAL_SRAV		= 0x07,
	OP_SPECIAL_JR		= 0x08,
	OP_SPECIAL_JALR		= 0x09,
	OP_SPECIAL_SYSCALL	= 0x0c,
	OP_SPECIAL_BREAK	= 0x0d,
	OP_SPECIAL_MFHI		= 0x10,
	OP_SPECIAL_MTHI		= 0x11,
	OP_SPECIAL_MFLO		= 0x12,
	OP_SPECIAL_MTLO		= 0x13,
	OP_SPECIAL_MULT		= 0x18,
	OP_SPECIAL_MULTU	= 0x19,
	OP_SPECIAL_DIV		= 0x1a,
	OP_SPECIAL_DIVU		= 0x1b,
	OP_SPECIAL_ADD		= 0x20,
	OP_SPECIAL_ADDU		= 0x21,
	OP_SPECIAL_SUB		= 0x22,
	OP_SPECIAL_SUBU		= 0x23,
	OP_SPECIAL_AND		= 0x24,
	OP_SPECIAL_OR		= 0x25,
	OP_SPECIAL_XOR		= 0x26,
	OP_SPECIAL_NOR		= 0x27,
	OP_SPECIAL_SLT		= 0x2a,
	OP_SPECIAL_SLTU		= 0x2b,
};

enum regimm_opcodes {
	OP_REGIMM_BLTZ		= 0x00,
	OP_REGIMM_BGEZ		= 0x01,
	OP_REGIMM_BLTZAL	= 0x10,
	OP_REGIMM_BGEZAL	= 0x11,
};

enum cp0_opcodes {
	OP_CP0_MFC0		= 0x00,
	OP_CP0_CFC0		= 0x02,
	OP_CP0_MTC0		= 0x04,
	OP_CP0_CTC0		= 0x06,
	OP_CP0_RFE		= 0x10,
};

enum cp2_opcodes {
	OP_CP2_BASIC		= 0x00,
	OP_CP2_RTPS		= 0x01,
	OP_CP2_NCLIP		= 0x06,
	OP_CP2_OP		= 0x0c,
	OP_CP2_DPCS		= 0x10,
	OP_CP2_INTPL		= 0x11,
	OP_CP2_MVMVA		= 0x12,
	OP_CP2_NCDS		= 0x13,
	OP_CP2_CDP		= 0x14,
	OP_CP2_NCDT		= 0x16,
	OP_CP2_NCCS		= 0x1b,
	OP_CP2_CC		= 0x1c,
	OP_CP2_NCS		= 0x1e,
	OP_CP2_NCT		= 0x20,
	OP_CP2_SQR		= 0x28,
	OP_CP2_DCPL		= 0x29,
	OP_CP2_DPCT		= 0x2a,
	OP_CP2_AVSZ3		= 0x2d,
	OP_CP2_AVSZ4		= 0x2e,
	OP_CP2_RTPT		= 0x30,
	OP_CP2_GPF		= 0x3d,
	OP_CP2_GPL		= 0x3e,
	OP_CP2_NCCT		= 0x3f,
};

enum cp2_basic_opcodes {
	OP_CP2_BASIC_MFC2	= 0x00,
	OP_CP2_BASIC_CFC2	= 0x02,
	OP_CP2_BASIC_MTC2	= 0x04,
	OP_CP2_BASIC_CTC2	= 0x06,
};

enum meta_opcodes {
	OP_META_MOV		= 0x00,

	OP_META_EXTC		= 0x01,
	OP_META_EXTS		= 0x02,

	OP_META_COM		= 0x03,
};

struct opcode_r {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u32 zero :6;
	u32 rs   :5;
	u32 rt   :5;
	u32 rd   :5;
	u32 imm  :5;
	u32 op   :6;
#else
	u32 op   :6;
	u32 imm  :5;
	u32 rd   :5;
	u32 rt   :5;
	u32 rs   :5;
	u32 zero :6;
#endif
} __packed;

struct opcode_i {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u32 op  :6;
	u32 rs  :5;
	u32 rt  :5;
	u32 imm :16;
#else
	u32 imm :16;
	u32 rt  :5;
	u32 rs  :5;
	u32 op  :6;
#endif
} __packed;

struct opcode_j {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u32 op  :6;
	u32 imm :26;
#else
	u32 imm :26;
	u32 op  :6;
#endif
} __packed;

struct opcode_m {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	u32 meta :6;
	u32 rs   :5;
	u32 rt   :5;
	u32 rd   :5;
	u32 imm  :6;
	u32 op   :5;
#else
	u32 op   :5;
	u32 imm  :6;
	u32 rd   :5;
	u32 rt   :5;
	u32 rs   :5;
	u32 meta :6;
#endif
};

union code {
	/* Keep in sync with struct opcode */
	u32 opcode;
	struct opcode_r r;
	struct opcode_i i;
	struct opcode_j j;
	struct opcode_m m;
};

struct opcode {
	/* Keep this union at the first position */
	union {
		union code c;

		/* Keep in sync with union code */
		u32 opcode;
		struct opcode_r r;
		struct opcode_i i;
		struct opcode_j j;
		struct opcode_m m;
	};
	u32 flags;
};

struct opcode_list {
	u16 nb_ops;
	struct opcode ops[];
};

void lightrec_print_disassembly(const struct block *block, const u32 *code);

static inline _Bool op_flag_no_ds(u32 flags)
{
	return OPT_SWITCH_DELAY_SLOTS && (flags & LIGHTREC_NO_DS);
}

static inline _Bool op_flag_sync(u32 flags)
{
	return OPT_LOCAL_BRANCHES && (flags & LIGHTREC_SYNC);
}

static inline _Bool op_flag_smc(u32 flags)
{
	return OPT_FLAG_IO && (flags & LIGHTREC_SMC);
}

static inline _Bool op_flag_no_invalidate(u32 flags)
{
	return OPT_FLAG_IO && (flags & LIGHTREC_NO_INVALIDATE);
}

static inline _Bool op_flag_no_mask(u32 flags)
{
	return OPT_FLAG_IO && (flags & LIGHTREC_NO_MASK);
}

static inline _Bool op_flag_load_delay(u32 flags)
{
	return OPT_HANDLE_LOAD_DELAYS && (flags & LIGHTREC_LOAD_DELAY);
}

static inline _Bool op_flag_emulate_branch(u32 flags)
{
	return OPT_DETECT_IMPOSSIBLE_BRANCHES &&
		(flags & LIGHTREC_EMULATE_BRANCH);
}

static inline _Bool op_flag_local_branch(u32 flags)
{
	return OPT_LOCAL_BRANCHES && (flags & LIGHTREC_LOCAL_BRANCH);
}

static inline _Bool op_flag_no_lo(u32 flags)
{
	return OPT_FLAG_MULT_DIV && (flags & LIGHTREC_NO_LO);
}

static inline _Bool op_flag_no_hi(u32 flags)
{
	return OPT_FLAG_MULT_DIV && (flags & LIGHTREC_NO_HI);
}

static inline _Bool op_flag_no_div_check(u32 flags)
{
	return OPT_FLAG_MULT_DIV && (flags & LIGHTREC_NO_DIV_CHECK);
}

#endif /* __DISASSEMBLER_H__ */
