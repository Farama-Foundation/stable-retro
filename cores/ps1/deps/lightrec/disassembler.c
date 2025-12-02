// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lightrec-private.h"
#include "regcache.h"

static const char * const std_opcodes[] = {
	[OP_J]			= "j       ",
	[OP_JAL]		= "jal     ",
	[OP_BEQ]		= "beq     ",
	[OP_BNE]		= "bne     ",
	[OP_BLEZ]		= "blez    ",
	[OP_BGTZ]		= "bgtz    ",
	[OP_ADDI]		= "addi    ",
	[OP_ADDIU]		= "addiu   ",
	[OP_SLTI]		= "slti    ",
	[OP_SLTIU]		= "sltiu   ",
	[OP_ANDI]		= "andi    ",
	[OP_ORI]		= "ori     ",
	[OP_XORI]		= "xori    ",
	[OP_LUI]		= "lui     ",
	[OP_LB]			= "lb      ",
	[OP_LH]			= "lh      ",
	[OP_LWL]		= "lwl     ",
	[OP_LW]			= "lw      ",
	[OP_LBU]		= "lbu     ",
	[OP_LHU]		= "lhu     ",
	[OP_LWR]		= "lwr     ",
	[OP_SB]			= "sb      ",
	[OP_SH]			= "sh      ",
	[OP_SWL]		= "swl     ",
	[OP_SW]			= "sw      ",
	[OP_SWR]		= "swr     ",
	[OP_LWC2]		= "lwc2    ",
	[OP_SWC2]		= "swc2    ",
	[OP_META_BIOS]		= "bios    ",
	[OP_META_MULT2]		= "mult2   ",
	[OP_META_MULTU2]	= "multu2  ",
	[OP_META_LWU]		= "lwu     ",
	[OP_META_SWU]		= "swu     ",
};

static const char * const special_opcodes[] = {
	[OP_SPECIAL_SLL]	= "sll     ",
	[OP_SPECIAL_SRL]	= "srl     ",
	[OP_SPECIAL_SRA]	= "sra     ",
	[OP_SPECIAL_SLLV]	= "sllv    ",
	[OP_SPECIAL_SRLV]	= "srlv    ",
	[OP_SPECIAL_SRAV]	= "srav    ",
	[OP_SPECIAL_JR]		= "jr      ",
	[OP_SPECIAL_JALR]	= "jalr    ",
	[OP_SPECIAL_SYSCALL]	= "syscall ",
	[OP_SPECIAL_BREAK]	= "break   ",
	[OP_SPECIAL_MFHI]	= "mfhi    ",
	[OP_SPECIAL_MTHI]	= "mthi    ",
	[OP_SPECIAL_MFLO]	= "mflo    ",
	[OP_SPECIAL_MTLO]	= "mtlo    ",
	[OP_SPECIAL_MULT]	= "mult    ",
	[OP_SPECIAL_MULTU]	= "multu   ",
	[OP_SPECIAL_DIV]	= "div     ",
	[OP_SPECIAL_DIVU]	= "divu    ",
	[OP_SPECIAL_ADD]	= "add     ",
	[OP_SPECIAL_ADDU]	= "addu    ",
	[OP_SPECIAL_SUB]	= "sub     ",
	[OP_SPECIAL_SUBU]	= "subu    ",
	[OP_SPECIAL_AND]	= "and     ",
	[OP_SPECIAL_OR]		= "or      ",
	[OP_SPECIAL_XOR]	= "xor     ",
	[OP_SPECIAL_NOR]	= "nor     ",
	[OP_SPECIAL_SLT]	= "slt     ",
	[OP_SPECIAL_SLTU]	= "sltu    ",
};

static const char * const regimm_opcodes[] = {
	[OP_REGIMM_BLTZ]	= "bltz    ",
	[OP_REGIMM_BGEZ]	= "bgez    ",
	[OP_REGIMM_BLTZAL]	= "bltzal  ",
	[OP_REGIMM_BGEZAL]	= "bgezal  ",
};

static const char * const cp0_opcodes[] = {
	[OP_CP0_MFC0]		= "mfc0    ",
	[OP_CP0_CFC0]		= "cfc0    ",
	[OP_CP0_MTC0]		= "mtc0    ",
	[OP_CP0_CTC0]		= "ctc0    ",
	[OP_CP0_RFE]		= "rfe",
};

static const char * const cp2_basic_opcodes[] = {
	[OP_CP2_BASIC_MFC2]	= "mfc2    ",
	[OP_CP2_BASIC_CFC2]	= "cfc2    ",
	[OP_CP2_BASIC_MTC2]	= "mtc2    ",
	[OP_CP2_BASIC_CTC2]	= "ctc2    ",
};

static const char * const cp2_opcodes[] = {
	[OP_CP2_RTPS]		= "rtps    ",
	[OP_CP2_NCLIP]		= "nclip   ",
	[OP_CP2_OP]		= "op      ",
	[OP_CP2_DPCS]		= "dpcs    ",
	[OP_CP2_INTPL]		= "intpl   ",
	[OP_CP2_MVMVA]		= "mvmva   ",
	[OP_CP2_NCDS]		= "ncds    ",
	[OP_CP2_CDP]		= "cdp     ",
	[OP_CP2_NCDT]		= "ncdt    ",
	[OP_CP2_NCCS]		= "nccs    ",
	[OP_CP2_CC]		= "cc      ",
	[OP_CP2_NCS]		= "ncs     ",
	[OP_CP2_NCT]		= "nct     ",
	[OP_CP2_SQR]		= "sqr     ",
	[OP_CP2_DCPL]		= "dcpl    ",
	[OP_CP2_DPCT]		= "dpct    ",
	[OP_CP2_AVSZ3]		= "avsz3   ",
	[OP_CP2_AVSZ4]		= "avsz4   ",
	[OP_CP2_RTPT]		= "rtpt    ",
	[OP_CP2_GPF]		= "gpf     ",
	[OP_CP2_GPL]		= "gpl     ",
	[OP_CP2_NCCT]		= "ncct    ",
};

static const char * const meta_opcodes[] = {
	[OP_META_MOV]		= "move    ",
	[OP_META_EXTC]		= "extc    ",
	[OP_META_EXTS]		= "exts    ",
	[OP_META_COM]		= "com     ",
};

static const char * const mult2_opcodes[] = {
	"mult2   ", "multu2  ",
};

static const char * const opcode_flags[] = {
	"switched branch/DS",
	"sync point",
};

static const char * const opcode_io_flags[] = {
	"self-modifying code",
	"no invalidation",
	"no mask",
	"load delay",
};

static const char * const opcode_io_modes[] = {
	"Memory access",
	"I/O access",
	"RAM access",
	"BIOS access",
	"Scratchpad access",
	"Mapped I/O access"
};

static const char * const opcode_branch_flags[] = {
	"emulate branch",
	"local branch",
};

static const char * const opcode_movi_flags[] = {
	"movi",
};

static const char * const opcode_multdiv_flags[] = {
	"No LO",
	"No HI",
	"No div check",
};

static size_t do_snprintf(char *buf, size_t len, bool *first,
			  const char *arg1, const char *arg2)
{
	size_t bytes;

	if (*first)
		bytes = snprintf(buf, len, "(%s%s", arg1, arg2);
	else
		bytes = snprintf(buf, len, ", %s%s", arg1, arg2);

	*first = false;

	return bytes;
}

static const char * const reg_op_token[3] = {
	"-", "*", "~",
};

static int print_flags(char *buf, size_t len, const struct opcode *op,
		       const char * const *array, size_t array_size,
		       bool is_io)
{
	const char *flag_name, *io_mode_name;
	unsigned int i, io_mode;
	size_t count = 0, bytes;
	bool first = true;
	u32 flags = op->flags;
	unsigned int reg_op;

	for (i = 0; i < array_size + ARRAY_SIZE(opcode_flags); i++) {
		if (!(flags & BIT(i)))
			continue;

		if (i < ARRAY_SIZE(opcode_flags))
			flag_name = opcode_flags[i];
		else
			flag_name = array[i - ARRAY_SIZE(opcode_flags)];

		bytes = do_snprintf(buf, len, &first, "", flag_name);
		buf += bytes;
		len -= bytes;
		count += bytes;
	}

	if (is_io) {
		io_mode = LIGHTREC_FLAGS_GET_IO_MODE(flags);
		if (io_mode > 0) {
			io_mode_name = opcode_io_modes[io_mode - 1];

			bytes = do_snprintf(buf, len, &first, "", io_mode_name);
			buf += bytes;
			len -= bytes;
			count += bytes;
		}
	}

	if (OPT_EARLY_UNLOAD) {
		reg_op = LIGHTREC_FLAGS_GET_RS(flags);
		if (reg_op) {
			bytes = do_snprintf(buf, len, &first,
					    reg_op_token[reg_op - 1],
					    lightrec_reg_name(op->i.rs));
			buf += bytes;
			len -= bytes;
			count += bytes;
		}

		reg_op = LIGHTREC_FLAGS_GET_RT(flags);
		if (reg_op) {
			bytes = do_snprintf(buf, len, &first,
					    reg_op_token[reg_op - 1],
					    lightrec_reg_name(op->i.rt));
			buf += bytes;
			len -= bytes;
			count += bytes;
		}

		reg_op = LIGHTREC_FLAGS_GET_RD(flags);
		if (reg_op) {
			bytes = do_snprintf(buf, len, &first,
					    reg_op_token[reg_op - 1],
					    lightrec_reg_name(op->r.rd));
			buf += bytes;
			len -= bytes;
			count += bytes;
		}
	}

	if (!first)
		count += snprintf(buf, len, ")");
	else
		*buf = '\0';

	return count;
}

static int print_op_special(union code c, char *buf, size_t len,
			    const char * const **flags_ptr, size_t *nb_flags)
{
	switch (c.r.op) {
	case OP_SPECIAL_SLL:
	case OP_SPECIAL_SRL:
	case OP_SPECIAL_SRA:
		return snprintf(buf, len, "%s%s,%s,%u",
				special_opcodes[c.r.op],
				lightrec_reg_name(c.r.rd),
				lightrec_reg_name(c.r.rt),
				c.r.imm);
	case OP_SPECIAL_SLLV:
	case OP_SPECIAL_SRLV:
	case OP_SPECIAL_SRAV:
	case OP_SPECIAL_ADD:
	case OP_SPECIAL_ADDU:
	case OP_SPECIAL_SUB:
	case OP_SPECIAL_SUBU:
	case OP_SPECIAL_AND:
	case OP_SPECIAL_OR:
	case OP_SPECIAL_XOR:
	case OP_SPECIAL_NOR:
	case OP_SPECIAL_SLT:
	case OP_SPECIAL_SLTU:
		return snprintf(buf, len, "%s%s,%s,%s",
				special_opcodes[c.r.op],
				lightrec_reg_name(c.r.rd),
				lightrec_reg_name(c.r.rt),
				lightrec_reg_name(c.r.rs));
	case OP_SPECIAL_JR:
		*flags_ptr = opcode_branch_flags;
		*nb_flags = ARRAY_SIZE(opcode_branch_flags);
		fallthrough;
	case OP_SPECIAL_MTHI:
	case OP_SPECIAL_MTLO:
		return snprintf(buf, len, "%s%s",
				special_opcodes[c.r.op],
				lightrec_reg_name(c.r.rs));
	case OP_SPECIAL_JALR:
		return snprintf(buf, len, "%s%s,%s",
				special_opcodes[c.r.op],
				lightrec_reg_name(c.r.rd),
				lightrec_reg_name(c.r.rs));
	case OP_SPECIAL_SYSCALL:
	case OP_SPECIAL_BREAK:
		return snprintf(buf, len, "%s", special_opcodes[c.r.op]);
	case OP_SPECIAL_MFHI:
	case OP_SPECIAL_MFLO:
		return snprintf(buf, len, "%s%s",
				special_opcodes[c.r.op],
				lightrec_reg_name(c.r.rd));
	case OP_SPECIAL_MULT:
	case OP_SPECIAL_MULTU:
	case OP_SPECIAL_DIV:
	case OP_SPECIAL_DIVU:
		*flags_ptr = opcode_multdiv_flags;
		*nb_flags = ARRAY_SIZE(opcode_multdiv_flags);
		return snprintf(buf, len, "%s%s,%s,%s,%s",
				special_opcodes[c.r.op],
				lightrec_reg_name(get_mult_div_hi(c)),
				lightrec_reg_name(get_mult_div_lo(c)),
				lightrec_reg_name(c.r.rs),
				lightrec_reg_name(c.r.rt));
	default:
		return snprintf(buf, len, "unknown ("X32_FMT")", c.opcode);
	}
}

static int print_op_cp(union code c, char *buf, size_t len, unsigned int cp)
{
	if (cp == 2) {
		switch (c.r.op) {
		case OP_CP2_BASIC:
			return snprintf(buf, len, "%s%s,%u",
					cp2_basic_opcodes[c.i.rs],
					lightrec_reg_name(c.i.rt),
					c.r.rd);
		default:
			return snprintf(buf, len, "%s", cp2_opcodes[c.r.op]);
		}
	} else {
		switch (c.i.rs) {
		case OP_CP0_MFC0:
		case OP_CP0_CFC0:
		case OP_CP0_MTC0:
		case OP_CP0_CTC0:
			return snprintf(buf, len, "%s%s,%u",
					cp0_opcodes[c.i.rs],
					lightrec_reg_name(c.i.rt),
					c.r.rd);
		case OP_CP0_RFE:
			return snprintf(buf, len, "rfe     ");
		default:
			return snprintf(buf, len, "unknown ("X32_FMT")", c.opcode);
		}
	}
}

static int print_op(union code c, u32 pc, char *buf, size_t len,
		    const char * const **flags_ptr, size_t *nb_flags,
		    bool *is_io)
{
	if (c.opcode == 0)
		return snprintf(buf, len, "nop     ");

	switch (c.i.op) {
	case OP_SPECIAL:
		return print_op_special(c, buf, len, flags_ptr, nb_flags);
	case OP_REGIMM:
		*flags_ptr = opcode_branch_flags;
		*nb_flags = ARRAY_SIZE(opcode_branch_flags);
		return snprintf(buf, len, "%s%s,0x%"PRIx32,
				regimm_opcodes[c.i.rt],
				lightrec_reg_name(c.i.rs),
				pc + 4 + ((s16)c.i.imm << 2));
	case OP_J:
	case OP_JAL:
		*flags_ptr = opcode_branch_flags;
		*nb_flags = ARRAY_SIZE(opcode_branch_flags);
		return snprintf(buf, len, "%s0x%"PRIx32,
				std_opcodes[c.i.op],
				(pc & 0xf0000000) | (c.j.imm << 2));
	case OP_BEQ:
		if (c.i.rs == c.i.rt) {
			*flags_ptr = opcode_branch_flags;
			*nb_flags = ARRAY_SIZE(opcode_branch_flags);
			return snprintf(buf, len, "b       0x%"PRIx32,
					pc + 4 + ((s16)c.i.imm << 2));
		}
		fallthrough;
	case OP_BNE:
	case OP_BLEZ:
	case OP_BGTZ:
		*flags_ptr = opcode_branch_flags;
		*nb_flags = ARRAY_SIZE(opcode_branch_flags);
		return snprintf(buf, len, "%s%s,%s,0x%"PRIx32,
				std_opcodes[c.i.op],
				lightrec_reg_name(c.i.rs),
				lightrec_reg_name(c.i.rt),
				pc + 4 + ((s16)c.i.imm << 2));
	case OP_ADDI:
	case OP_ADDIU:
	case OP_ORI:
		*flags_ptr = opcode_movi_flags;
		*nb_flags = ARRAY_SIZE(opcode_movi_flags);
		fallthrough;
	case OP_SLTI:
	case OP_SLTIU:
	case OP_ANDI:
	case OP_XORI:
		return snprintf(buf, len, "%s%s,%s,0x%04hx",
				std_opcodes[c.i.op],
				lightrec_reg_name(c.i.rt),
				lightrec_reg_name(c.i.rs),
				(u16)c.i.imm);

	case OP_LUI:
		*flags_ptr = opcode_movi_flags;
		*nb_flags = ARRAY_SIZE(opcode_movi_flags);
		return snprintf(buf, len, "%s%s,0x%04hx",
				std_opcodes[c.i.op],
				lightrec_reg_name(c.i.rt),
				(u16)c.i.imm);
	case OP_CP0:
		return print_op_cp(c, buf, len, 0);
	case OP_CP2:
		return print_op_cp(c, buf, len, 2);
	case OP_LB:
	case OP_LH:
	case OP_LWL:
	case OP_LW:
	case OP_LBU:
	case OP_LHU:
	case OP_LWR:
	case OP_SB:
	case OP_SH:
	case OP_SWL:
	case OP_SW:
	case OP_SWR:
	case OP_META_LWU:
	case OP_META_SWU:
		*flags_ptr = opcode_io_flags;
		*nb_flags = ARRAY_SIZE(opcode_io_flags);
		*is_io = true;
		return snprintf(buf, len, "%s%s,%hd(%s)",
				std_opcodes[c.i.op],
				lightrec_reg_name(c.i.rt),
				(s16)c.i.imm,
				lightrec_reg_name(c.i.rs));
	case OP_LWC2:
	case OP_SWC2:
		*flags_ptr = opcode_io_flags;
		*nb_flags = ARRAY_SIZE(opcode_io_flags);
		return snprintf(buf, len, "%s%s,%hd(%s)",
				std_opcodes[c.i.op],
				lightrec_reg_name(c.i.rt),
				(s16)c.i.imm,
				lightrec_reg_name(c.i.rs));
	case OP_META_BIOS:
		return snprintf(buf, len, "%s0x%x",
				std_opcodes[c.i.op],
				c.opcode & 0x03ffffff);
	case OP_META:
		return snprintf(buf, len, "%s%s,%s",
				meta_opcodes[c.m.op],
				lightrec_reg_name(c.m.rd),
				lightrec_reg_name(c.m.rs));
	case OP_META_MULT2:
	case OP_META_MULTU2:
		*flags_ptr = opcode_multdiv_flags;
		*nb_flags = ARRAY_SIZE(opcode_multdiv_flags);
		return snprintf(buf, len, "%s%s,%s,%s,%u",
				mult2_opcodes[c.i.op == OP_META_MULTU2],
				lightrec_reg_name(get_mult_div_hi(c)),
				lightrec_reg_name(get_mult_div_lo(c)),
				lightrec_reg_name(c.r.rs), c.r.op);
	default:
		return snprintf(buf, len, "unknown ("X32_FMT")", c.opcode);
	}
}

void lightrec_print_disassembly(const struct block *block, const u32 *code_ptr)
{
	const struct opcode *op;
	const char * const *flags_ptr;
	size_t nb_flags, count, count2;
	char buf[256], buf2[256], buf3[256];
	unsigned int i, nb_spaces1, nb_spaces2;
	u32 pc, branch_pc, code;
	bool is_io;

	for (i = 0; i < block->nb_ops; i++) {
		op = &block->opcode_list[i];
		branch_pc = get_branch_pc(block, i, 0);
		pc = block->pc + (i << 2);
		code = LE32TOH(code_ptr[i]);

		count = print_op((union code)code, pc, buf, sizeof(buf),
				 &flags_ptr, &nb_flags, &is_io);

		flags_ptr = NULL;
		nb_flags = 0;
		is_io = false;
		count2 = print_op(op->c, branch_pc, buf2, sizeof(buf2),
				  &flags_ptr, &nb_flags, &is_io);

		if (code == op->c.opcode) {
			*buf2 = '\0';
			count2 = 0;
		}

		print_flags(buf3, sizeof(buf3), op, flags_ptr, nb_flags, is_io);

		nb_spaces1 = (*buf2 || *buf3) ? 30 - (int)count : 0;
		nb_spaces2 = *buf3 ? 30 - (int)count2 : 0;

		printf(X32_FMT" (0x%x)\t%s%*c%s%*c%s\n", pc, i << 2,
		       buf, nb_spaces1, ' ', buf2, nb_spaces2, ' ', buf3);
	}
}
