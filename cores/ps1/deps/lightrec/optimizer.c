// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2014-2021 Paul Cercueil <paul@crapouillou.net>
 */

#include "constprop.h"
#include "lightrec-config.h"
#include "disassembler.h"
#include "lightrec.h"
#include "memmanager.h"
#include "optimizer.h"
#include "regcache.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define IF_OPT(opt, ptr) ((opt) ? (ptr) : NULL)

struct optimizer_list {
	void (**optimizers)(struct opcode *);
	unsigned int nb_optimizers;
};

static bool is_nop(union code op);

bool is_unconditional_jump(union code c)
{
	switch (c.i.op) {
	case OP_SPECIAL:
		return c.r.op == OP_SPECIAL_JR || c.r.op == OP_SPECIAL_JALR;
	case OP_J:
	case OP_JAL:
		return true;
	case OP_BEQ:
	case OP_BLEZ:
		return c.i.rs == c.i.rt;
	case OP_REGIMM:
		return (c.r.rt == OP_REGIMM_BGEZ ||
			c.r.rt == OP_REGIMM_BGEZAL) && c.i.rs == 0;
	default:
		return false;
	}
}

bool is_syscall(union code c)
{
	return (c.i.op == OP_SPECIAL && c.r.op == OP_SPECIAL_SYSCALL) ||
		(c.i.op == OP_CP0 && (c.r.rs == OP_CP0_MTC0 ||
					c.r.rs == OP_CP0_CTC0) &&
		 (c.r.rd == 12 || c.r.rd == 13));
}

static u64 opcode_read_mask(union code op)
{
	switch (op.i.op) {
	case OP_SPECIAL:
		switch (op.r.op) {
		case OP_SPECIAL_SYSCALL:
		case OP_SPECIAL_BREAK:
			return 0;
		case OP_SPECIAL_JR:
		case OP_SPECIAL_JALR:
		case OP_SPECIAL_MTHI:
		case OP_SPECIAL_MTLO:
			return BIT(op.r.rs);
		case OP_SPECIAL_MFHI:
			return BIT(REG_HI);
		case OP_SPECIAL_MFLO:
			return BIT(REG_LO);
		case OP_SPECIAL_SLL:
			if (!op.r.imm)
				return 0;
			fallthrough;
		case OP_SPECIAL_SRL:
		case OP_SPECIAL_SRA:
			return BIT(op.r.rt);
		default:
			return BIT(op.r.rs) | BIT(op.r.rt);
		}
	case OP_CP0:
		switch (op.r.rs) {
		case OP_CP0_MTC0:
		case OP_CP0_CTC0:
			return BIT(op.r.rt);
		default:
			return 0;
		}
	case OP_CP2:
		if (op.r.op == OP_CP2_BASIC) {
			switch (op.r.rs) {
			case OP_CP2_BASIC_MTC2:
			case OP_CP2_BASIC_CTC2:
				return BIT(op.r.rt);
			default:
				break;
			}
		}
		return 0;
	case OP_J:
	case OP_JAL:
	case OP_LUI:
	case OP_META_BIOS:
		return 0;
	case OP_BEQ:
		if (op.i.rs == op.i.rt)
			return 0;
		fallthrough;
	case OP_BNE:
	case OP_LWL:
	case OP_LWR:
	case OP_SB:
	case OP_SH:
	case OP_SWL:
	case OP_SW:
	case OP_SWR:
	case OP_META_LWU:
	case OP_META_SWU:
		return BIT(op.i.rs) | BIT(op.i.rt);
	case OP_META:
		return BIT(op.m.rs);
	default:
		return BIT(op.i.rs);
	}
}

static u64 mult_div_write_mask(union code op)
{
	u64 flags;

	if (!OPT_FLAG_MULT_DIV)
		return BIT(REG_LO) | BIT(REG_HI);

	if (op.r.rd)
		flags = BIT(op.r.rd);
	else
		flags = BIT(REG_LO);
	if (op.r.imm)
		flags |= BIT(op.r.imm);
	else
		flags |= BIT(REG_HI);

	return flags;
}

u64 opcode_write_mask(union code op)
{
	switch (op.i.op) {
	case OP_META_MULT2:
	case OP_META_MULTU2:
		return mult_div_write_mask(op);
	case OP_META:
		return BIT(op.m.rd);
	case OP_SPECIAL:
		switch (op.r.op) {
		case OP_SPECIAL_JR:
		case OP_SPECIAL_SYSCALL:
		case OP_SPECIAL_BREAK:
			return 0;
		case OP_SPECIAL_MULT:
		case OP_SPECIAL_MULTU:
		case OP_SPECIAL_DIV:
		case OP_SPECIAL_DIVU:
			return mult_div_write_mask(op);
		case OP_SPECIAL_MTHI:
			return BIT(REG_HI);
		case OP_SPECIAL_MTLO:
			return BIT(REG_LO);
		case OP_SPECIAL_SLL:
			if (!op.r.imm)
				return 0;
			fallthrough;
		default:
			return BIT(op.r.rd);
		}
	case OP_ADDI:
	case OP_ADDIU:
	case OP_SLTI:
	case OP_SLTIU:
	case OP_ANDI:
	case OP_ORI:
	case OP_XORI:
	case OP_LUI:
	case OP_LB:
	case OP_LH:
	case OP_LWL:
	case OP_LW:
	case OP_LBU:
	case OP_LHU:
	case OP_LWR:
	case OP_META_LWU:
		return BIT(op.i.rt);
	case OP_JAL:
		return BIT(31);
	case OP_CP0:
		switch (op.r.rs) {
		case OP_CP0_MFC0:
		case OP_CP0_CFC0:
			return BIT(op.i.rt);
		default:
			return 0;
		}
	case OP_CP2:
		if (op.r.op == OP_CP2_BASIC) {
			switch (op.r.rs) {
			case OP_CP2_BASIC_MFC2:
			case OP_CP2_BASIC_CFC2:
				return BIT(op.i.rt);
			default:
				break;
			}
		}
		return 0;
	case OP_REGIMM:
		switch (op.r.rt) {
		case OP_REGIMM_BLTZAL:
		case OP_REGIMM_BGEZAL:
			return BIT(31);
		default:
			return 0;
		}
	default:
		return 0;
	}
}

bool opcode_reads_register(union code op, u8 reg)
{
	return opcode_read_mask(op) & BIT(reg);
}

bool opcode_writes_register(union code op, u8 reg)
{
	return opcode_write_mask(op) & BIT(reg);
}

static int find_prev_writer(const struct opcode *list, unsigned int offset, u8 reg)
{
	union code c;
	unsigned int i;

	if (op_flag_sync(list[offset].flags))
		return -1;

	for (i = offset; i > 0; i--) {
		c = list[i - 1].c;

		if (opcode_writes_register(c, reg)) {
			if (i > 1 && has_delay_slot(list[i - 2].c))
				break;

			return i - 1;
		}

		if (op_flag_sync(list[i - 1].flags) ||
		    has_delay_slot(c) ||
		    opcode_reads_register(c, reg))
			break;
	}

	return -1;
}

static int find_next_reader(const struct opcode *list, unsigned int offset, u8 reg)
{
	unsigned int i;
	union code c;

	if (op_flag_sync(list[offset].flags))
		return -1;

	for (i = offset; ; i++) {
		c = list[i].c;

		if (opcode_reads_register(c, reg))
			return i;

		if (op_flag_sync(list[i].flags)
		    || (op_flag_no_ds(list[i].flags) && has_delay_slot(c))
		    || is_delay_slot(list, i)
		    || opcode_writes_register(c, reg))
			break;
	}

	return -1;
}

static bool reg_is_dead(const struct opcode *list, unsigned int offset, u8 reg)
{
	unsigned int i;

	if (op_flag_sync(list[offset].flags) || is_delay_slot(list, offset))
		return false;

	for (i = offset + 1; ; i++) {
		if (opcode_reads_register(list[i].c, reg))
			return false;

		if (opcode_writes_register(list[i].c, reg))
			return true;

		if (is_syscall(list[i].c))
			return false;

		if (has_delay_slot(list[i].c)) {
			if (op_flag_no_ds(list[i].flags) ||
			    opcode_reads_register(list[i + 1].c, reg))
				return false;

			return opcode_writes_register(list[i + 1].c, reg);
		}
	}
}

static bool reg_is_read(const struct opcode *list,
			unsigned int a, unsigned int b, u8 reg)
{
	/* Return true if reg is read in one of the opcodes of the interval
	 * [a, b[ */
	for (; a < b; a++) {
		if (!is_nop(list[a].c) && opcode_reads_register(list[a].c, reg))
			return true;
	}

	return false;
}

static bool reg_is_written(const struct opcode *list,
			   unsigned int a, unsigned int b, u8 reg)
{
	/* Return true if reg is written in one of the opcodes of the interval
	 * [a, b[ */

	for (; a < b; a++) {
		if (!is_nop(list[a].c) && opcode_writes_register(list[a].c, reg))
			return true;
	}

	return false;
}

static bool reg_is_read_or_written(const struct opcode *list,
				   unsigned int a, unsigned int b, u8 reg)
{
	return reg_is_read(list, a, b, reg) || reg_is_written(list, a, b, reg);
}

static bool opcode_is_mfc(union code op)
{
	switch (op.i.op) {
	case OP_CP0:
		switch (op.r.rs) {
		case OP_CP0_MFC0:
		case OP_CP0_CFC0:
			return true;
		default:
			break;
		}

		break;
	case OP_CP2:
		if (op.r.op == OP_CP2_BASIC) {
			switch (op.r.rs) {
			case OP_CP2_BASIC_MFC2:
			case OP_CP2_BASIC_CFC2:
				return true;
			default:
				break;
			}
		}

		break;
	default:
		break;
	}

	return false;
}

static bool opcode_is_load(union code op)
{
	switch (op.i.op) {
	case OP_LB:
	case OP_LH:
	case OP_LWL:
	case OP_LW:
	case OP_LBU:
	case OP_LHU:
	case OP_LWR:
	case OP_LWC2:
	case OP_META_LWU:
		return true;
	default:
		return false;
	}
}

static bool opcode_is_store(union code op)
{
	switch (op.i.op) {
	case OP_SB:
	case OP_SH:
	case OP_SW:
	case OP_SWL:
	case OP_SWR:
	case OP_SWC2:
	case OP_META_SWU:
		return true;
	default:
		return false;
	}
}

bool opcode_has_load_delay(union code op)
{
	return (opcode_is_load(op) && op.i.rt && op.i.op != OP_LWC2)
		|| opcode_is_mfc(op);
}

static u8 opcode_get_io_size(union code op)
{
	switch (op.i.op) {
	case OP_LB:
	case OP_LBU:
	case OP_SB:
		return 8;
	case OP_LH:
	case OP_LHU:
	case OP_SH:
		return 16;
	default:
		return 32;
	}
}

bool opcode_is_io(union code op)
{
	return opcode_is_load(op) || opcode_is_store(op);
}

/* TODO: Complete */
static bool is_nop(union code op)
{
	if (opcode_writes_register(op, 0)) {
		switch (op.i.op) {
		case OP_CP0:
			return op.r.rs != OP_CP0_MFC0;
		case OP_LB:
		case OP_LH:
		case OP_LWL:
		case OP_LW:
		case OP_LBU:
		case OP_LHU:
		case OP_LWR:
		case OP_META_LWU:
			return false;
		default:
			return true;
		}
	}

	switch (op.i.op) {
	case OP_SPECIAL:
		switch (op.r.op) {
		case OP_SPECIAL_AND:
			return op.r.rd == op.r.rt && op.r.rd == op.r.rs;
		case OP_SPECIAL_ADD:
		case OP_SPECIAL_ADDU:
			return (op.r.rd == op.r.rt && op.r.rs == 0) ||
				(op.r.rd == op.r.rs && op.r.rt == 0);
		case OP_SPECIAL_SUB:
		case OP_SPECIAL_SUBU:
			return op.r.rd == op.r.rs && op.r.rt == 0;
		case OP_SPECIAL_OR:
			if (op.r.rd == op.r.rt)
				return op.r.rd == op.r.rs || op.r.rs == 0;
			else
				return (op.r.rd == op.r.rs) && op.r.rt == 0;
		case OP_SPECIAL_SLL:
		case OP_SPECIAL_SRA:
		case OP_SPECIAL_SRL:
			return op.r.rd == op.r.rt && op.r.imm == 0;
		case OP_SPECIAL_MFHI:
		case OP_SPECIAL_MFLO:
			return op.r.rd == 0;
		default:
			return false;
		}
	case OP_ORI:
	case OP_ADDI:
	case OP_ADDIU:
		return op.i.rt == op.i.rs && op.i.imm == 0;
	case OP_BGTZ:
		return (op.i.rs == 0 || op.i.imm == 1);
	case OP_REGIMM:
		return (op.i.op == OP_REGIMM_BLTZ ||
				op.i.op == OP_REGIMM_BLTZAL) &&
			(op.i.rs == 0 || op.i.imm == 1);
	case OP_BNE:
		return (op.i.rs == op.i.rt || op.i.imm == 1);
	default:
		return false;
	}
}

static void lightrec_optimize_sll_sra(struct opcode *list, unsigned int offset,
				      struct constprop_data *v)
{
	struct opcode *ldop = NULL, *curr = &list[offset], *next;
	struct opcode *to_change, *to_nop;
	int idx, idx2;

	if (curr->r.imm != 24 && curr->r.imm != 16)
		return;

	if (is_delay_slot(list, offset))
		return;

	idx = find_next_reader(list, offset + 1, curr->r.rd);
	if (idx < 0)
		return;

	next = &list[idx];

	if (next->i.op != OP_SPECIAL || next->r.op != OP_SPECIAL_SRA ||
	    next->r.imm != curr->r.imm || next->r.rt != curr->r.rd)
		return;

	if (curr->r.rd != curr->r.rt && next->r.rd != next->r.rt) {
		/* sll rY, rX, 16
		 * ...
		 * sra rZ, rY, 16 */

		if (!reg_is_dead(list, idx, curr->r.rd) ||
		    reg_is_read_or_written(list, offset, idx, next->r.rd))
			return;

		/* If rY is dead after the SRL, and rZ is not used after the SLL,
		 * we can change rY to rZ */

		pr_debug("Detected SLL/SRA with middle temp register\n");
		curr->r.rd = next->r.rd;
		next->r.rt = curr->r.rd;
	}

	/* We got a SLL/SRA combo. If imm #16, that's a cast to s16.
	 * If imm #24 that's a cast to s8.
	 *
	 * First of all, make sure that the target register of the SLL is not
	 * read after the SRA. */

	if (curr->r.rd == curr->r.rt) {
		/* sll rX, rX, 16
		 * ...
		 * sra rY, rX, 16 */
		to_change = next;
		to_nop = curr;

		/* rX is used after the SRA - we cannot convert it. */
		if (curr->r.rd != next->r.rd && !reg_is_dead(list, idx, curr->r.rd))
			return;
	} else {
		/* sll rY, rX, 16
		 * ...
		 * sra rY, rY, 16 */
		to_change = curr;
		to_nop = next;
	}

	idx2 = find_prev_writer(list, offset, curr->r.rt);
	if (idx2 >= 0) {
		/* Note that PSX games sometimes do casts after
		 * a LHU or LBU; in this case we can change the
		 * load opcode to a LH or LB, and the cast can
		 * be changed to a MOV or a simple NOP. */

		ldop = &list[idx2];

		if (next->r.rd != ldop->i.rt &&
		    !reg_is_dead(list, idx, ldop->i.rt))
			ldop = NULL;
		else if (curr->r.imm == 16 && ldop->i.op == OP_LHU)
			ldop->i.op = OP_LH;
		else if (curr->r.imm == 24 && ldop->i.op == OP_LBU)
			ldop->i.op = OP_LB;
		else
			ldop = NULL;

		if (ldop) {
			if (next->r.rd == ldop->i.rt) {
				to_change->opcode = 0;
			} else if (reg_is_dead(list, idx, ldop->i.rt) &&
				   !reg_is_read_or_written(list, idx2 + 1, idx, next->r.rd)) {
				/* The target register of the SRA is dead after the
				 * LBU/LHU; we can change the target register of the
				 * LBU/LHU to the one of the SRA. */
				v[ldop->i.rt].known = 0;
				v[ldop->i.rt].sign = 0;
				ldop->i.rt = next->r.rd;
				to_change->opcode = 0;
			} else {
				to_change->i.op = OP_META;
				to_change->m.op = OP_META_MOV;
				to_change->m.rd = next->r.rd;
				to_change->m.rs = ldop->i.rt;
			}

			if (to_nop->r.imm == 24)
				pr_debug("Convert LBU+SLL+SRA to LB\n");
			else
				pr_debug("Convert LHU+SLL+SRA to LH\n");

			v[ldop->i.rt].known = 0;
			v[ldop->i.rt].sign = 0xffffff80 << (24 - curr->r.imm);
		}
	}

	if (!ldop) {
		pr_debug("Convert SLL/SRA #%u to EXT%c\n",
			 curr->r.imm, curr->r.imm == 24 ? 'C' : 'S');

		to_change->m.rs = curr->r.rt;
		to_change->m.op = to_nop->r.imm == 24 ? OP_META_EXTC : OP_META_EXTS;
		to_change->i.op = OP_META;
	}

	to_nop->opcode = 0;
}

static void
lightrec_remove_useless_lui(struct block *block, unsigned int offset,
			    const struct constprop_data *v)
{
	struct opcode *list = block->opcode_list,
		      *op = &block->opcode_list[offset];
	int reader;

	if (!op_flag_sync(op->flags) && is_known(v, op->i.rt) &&
	    v[op->i.rt].value == op->i.imm << 16) {
		pr_debug("Converting duplicated LUI to NOP\n");
		op->opcode = 0x0;
		return;
	}

	if (op->i.imm != 0 || op->i.rt == 0 || is_delay_slot(list, offset))
		return;

	reader = find_next_reader(list, offset + 1, op->i.rt);
	if (reader <= 0)
		return;

	if (opcode_writes_register(list[reader].c, op->i.rt) ||
	    reg_is_dead(list, reader, op->i.rt)) {
		pr_debug("Removing useless LUI 0x0\n");

		if (list[reader].i.rs == op->i.rt)
			list[reader].i.rs = 0;
		if (list[reader].i.op == OP_SPECIAL &&
		    list[reader].i.rt == op->i.rt)
			list[reader].i.rt = 0;
		op->opcode = 0x0;
	}
}

static void lightrec_lui_to_movi(struct block *block, unsigned int offset)
{
	struct opcode *ori, *lui = &block->opcode_list[offset];
	int next;

	if (lui->i.op != OP_LUI)
		return;

	next = find_next_reader(block->opcode_list, offset + 1, lui->i.rt);
	if (next > 0) {
		ori = &block->opcode_list[next];

		switch (ori->i.op) {
		case OP_ORI:
		case OP_ADDI:
		case OP_ADDIU:
			if (ori->i.rs == ori->i.rt && ori->i.imm) {
				ori->flags |= LIGHTREC_MOVI;
				lui->flags |= LIGHTREC_MOVI;
			}
			break;
		}
	}
}

static void lightrec_modify_lui(struct block *block, unsigned int offset)
{
	union code c, *lui = &block->opcode_list[offset].c;
	bool stop = false, stop_next = false;
	unsigned int i;

	for (i = offset + 1; !stop && i < block->nb_ops; i++) {
		c = block->opcode_list[i].c;
		stop = stop_next;

		if ((opcode_is_store(c) && c.i.rt == lui->i.rt)
		    || (!opcode_is_load(c) && opcode_reads_register(c, lui->i.rt)))
			break;

		if (opcode_writes_register(c, lui->i.rt)) {
			if (c.i.op == OP_LWL || c.i.op == OP_LWR) {
				/* LWL/LWR only partially write their target register;
				 * therefore the LUI should not write a different value. */
				break;
			}

			pr_debug("Convert LUI at offset 0x%x to kuseg\n",
				 (i - 1) << 2);
			lui->i.imm = kunseg(lui->i.imm << 16) >> 16;
			break;
		}

		if (has_delay_slot(c))
			stop_next = true;
	}
}

static int lightrec_transform_branches(struct lightrec_state *state,
				       struct block *block)
{
	struct opcode *op;
	unsigned int i;
	s32 offset;

	for (i = 0; i < block->nb_ops; i++) {
		op = &block->opcode_list[i];

		switch (op->i.op) {
		case OP_J:
			if (is_delay_slot(block->opcode_list, i)) {
				/* Jumps in delay slots cannot be converted to
				 * branches, as they have a different behaviour
				 * there. */
				continue;
			}

			/* Transform J opcode into BEQ $zero, $zero if possible. */
			offset = (s32)((block->pc & 0xf0000000) >> 2 | op->j.imm)
				- (s32)(block->pc >> 2) - (s32)i - 1;

			if (offset == (s16)offset) {
				pr_debug("Transform J into BEQ $zero, $zero\n");
				op->i.op = OP_BEQ;
				op->i.rs = 0;
				op->i.rt = 0;
				op->i.imm = offset;

			}
			fallthrough;
		default:
			break;
		}
	}

	return 0;
}

static inline bool is_power_of_two(u32 value)
{
	return popcount32(value) == 1;
}

static void lightrec_patch_known_zero(struct opcode *op,
				      const struct constprop_data *v)
{
	switch (op->i.op) {
	case OP_SPECIAL:
		switch (op->r.op) {
		case OP_SPECIAL_JR:
		case OP_SPECIAL_JALR:
		case OP_SPECIAL_MTHI:
		case OP_SPECIAL_MTLO:
			if (is_known_zero(v, op->r.rs))
				op->r.rs = 0;
			break;
		default:
			if (is_known_zero(v, op->r.rs))
				op->r.rs = 0;
			fallthrough;
		case OP_SPECIAL_SLL:
		case OP_SPECIAL_SRL:
		case OP_SPECIAL_SRA:
			if (is_known_zero(v, op->r.rt))
				op->r.rt = 0;
			break;
		case OP_SPECIAL_SYSCALL:
		case OP_SPECIAL_BREAK:
		case OP_SPECIAL_MFHI:
		case OP_SPECIAL_MFLO:
			break;
		}
		break;
	case OP_CP0:
		switch (op->r.rs) {
		case OP_CP0_MTC0:
		case OP_CP0_CTC0:
			if (is_known_zero(v, op->r.rt))
				op->r.rt = 0;
			break;
		default:
			break;
		}
		break;
	case OP_CP2:
		if (op->r.op == OP_CP2_BASIC) {
			switch (op->r.rs) {
			case OP_CP2_BASIC_MTC2:
			case OP_CP2_BASIC_CTC2:
				if (is_known_zero(v, op->r.rt))
					op->r.rt = 0;
				break;
			default:
				break;
			}
		}
		break;
	case OP_BEQ:
	case OP_BNE:
		if (is_known_zero(v, op->i.rt))
			op->i.rt = 0;
		fallthrough;
	case OP_REGIMM:
	case OP_BLEZ:
	case OP_BGTZ:
	case OP_ADDI:
	case OP_ADDIU:
	case OP_SLTI:
	case OP_SLTIU:
	case OP_ANDI:
	case OP_ORI:
	case OP_XORI:
	case OP_META_MULT2:
	case OP_META_MULTU2:
	case OP_META:
		if (is_known_zero(v, op->m.rs))
			op->m.rs = 0;
		break;
	case OP_SB:
	case OP_SH:
	case OP_SWL:
	case OP_SW:
	case OP_SWR:
	case OP_META_SWU:
		if (is_known_zero(v, op->i.rt))
			op->i.rt = 0;
		fallthrough;
	case OP_LB:
	case OP_LH:
	case OP_LWL:
	case OP_LW:
	case OP_LBU:
	case OP_LHU:
	case OP_LWR:
	case OP_LWC2:
	case OP_SWC2:
	case OP_META_LWU:
		if (is_known(v, op->i.rs)
		    && kunseg(v[op->i.rs].value) == 0)
			op->i.rs = 0;
		break;
	default:
		break;
	}
}

static void lightrec_reset_syncs(struct block *block)
{
	struct opcode *op, *list = block->opcode_list;
	unsigned int i;
	s32 offset;

	for (i = 0; i < block->nb_ops; i++)
		list[i].flags &= ~LIGHTREC_SYNC;

	for (i = 0; i < block->nb_ops; i++) {
		op = &list[i];

		if (has_delay_slot(op->c)) {
			if (op_flag_local_branch(op->flags)) {
				offset = i + 1 - op_flag_no_ds(op->flags) + (s16)op->i.imm;
				list[offset].flags |= LIGHTREC_SYNC;
			}

			if (op_flag_emulate_branch(op->flags) && i + 2 < block->nb_ops)
				list[i + 2].flags |= LIGHTREC_SYNC;
		}
	}
}

static void maybe_remove_load_delay(struct opcode *op)
{
	if (op_flag_load_delay(op->flags) && opcode_is_load(op->c))
		op->flags &= ~LIGHTREC_LOAD_DELAY;
}

static int lightrec_transform_ops(struct lightrec_state *state, struct block *block)
{
	struct opcode *op, *list = block->opcode_list;
	struct constprop_data v[32] = LIGHTREC_CONSTPROP_INITIALIZER;
	unsigned int i;
	bool local;
	int idx;
	u8 tmp;

	for (i = 0; i < block->nb_ops; i++) {
		op = &list[i];

		lightrec_consts_propagate(block, i, v);

		lightrec_patch_known_zero(op, v);

		/* Transform all opcodes detected as useless to real NOPs
		 * (0x0: SLL r0, r0, #0) */
		if (op->opcode != 0 && is_nop(op->c)) {
			pr_debug("Converting useless opcode "X32_FMT" to NOP\n",
				 op->opcode);
			op->opcode = 0x0;
		}

		if (!op->opcode)
			continue;

		switch (op->i.op) {
		case OP_BEQ:
			if (op->i.rs == op->i.rt ||
			    (is_known(v, op->i.rs) && is_known(v, op->i.rt) &&
			     v[op->i.rs].value == v[op->i.rt].value)) {
				if (op->i.rs != op->i.rt)
					pr_debug("Found always-taken BEQ\n");

				op->i.rs = 0;
				op->i.rt = 0;
			} else if (v[op->i.rs].known & v[op->i.rt].known &
				   (v[op->i.rs].value ^ v[op->i.rt].value)) {
				pr_debug("Found never-taken BEQ\n");

				if (!op_flag_no_ds(op->flags))
					maybe_remove_load_delay(&list[i + 1]);

				local = op_flag_local_branch(op->flags);
				op->opcode = 0;
				op->flags = 0;

				if (local)
					lightrec_reset_syncs(block);
			} else if (op->i.rs == 0) {
				op->i.rs = op->i.rt;
				op->i.rt = 0;
			}
			break;

		case OP_BNE:
			if (v[op->i.rs].known & v[op->i.rt].known &
			    (v[op->i.rs].value ^ v[op->i.rt].value)) {
				pr_debug("Found always-taken BNE\n");

				op->i.op = OP_BEQ;
				op->i.rs = 0;
				op->i.rt = 0;
			} else if (is_known(v, op->i.rs) && is_known(v, op->i.rt) &&
				   v[op->i.rs].value == v[op->i.rt].value) {
				pr_debug("Found never-taken BNE\n");

				if (!op_flag_no_ds(op->flags))
					maybe_remove_load_delay(&list[i + 1]);

				local = op_flag_local_branch(op->flags);
				op->opcode = 0;
				op->flags = 0;

				if (local)
					lightrec_reset_syncs(block);
			} else if (op->i.rs == 0) {
				op->i.rs = op->i.rt;
				op->i.rt = 0;
			}
			break;

		case OP_BLEZ:
			if (v[op->i.rs].known & BIT(31) &&
			    v[op->i.rs].value & BIT(31)) {
				pr_debug("Found always-taken BLEZ\n");

				op->i.op = OP_BEQ;
				op->i.rs = 0;
				op->i.rt = 0;
			}
			break;

		case OP_BGTZ:
			if (v[op->i.rs].known & BIT(31) &&
			    v[op->i.rs].value & BIT(31)) {
				pr_debug("Found never-taken BGTZ\n");

				if (!op_flag_no_ds(op->flags))
					maybe_remove_load_delay(&list[i + 1]);

				local = op_flag_local_branch(op->flags);
				op->opcode = 0;
				op->flags = 0;

				if (local)
					lightrec_reset_syncs(block);
			}
			break;

		case OP_LUI:
			if (!is_delay_slot(list, i))
				lightrec_modify_lui(block, i);
			lightrec_remove_useless_lui(block, i, v);
			if (!is_delay_slot(list, i))
				lightrec_lui_to_movi(block, i);
			break;

		/* Transform ORI/ADDI/ADDIU with imm #0 or ORR/ADD/ADDU/SUB/SUBU
		 * with register $zero to the MOV meta-opcode */
		case OP_ORI:
		case OP_ADDI:
		case OP_ADDIU:
			if (op->i.imm == 0) {
				pr_debug("Convert ORI/ADDI/ADDIU #0 to MOV\n");
				op->m.rd = op->i.rt;
				op->m.op = OP_META_MOV;
				op->i.op = OP_META;
			}
			break;
		case OP_ANDI:
			if (bits_are_known_zero(v, op->i.rs, ~op->i.imm)) {
				pr_debug("Found useless ANDI 0x%x\n", op->i.imm);

				if (op->i.rs == op->i.rt) {
					op->opcode = 0;
				} else {
					op->m.rd = op->i.rt;
					op->m.op = OP_META_MOV;
					op->i.op = OP_META;
				}
			}
			break;
		case OP_LWL:
		case OP_LWR:
			if (i == 0 || !has_delay_slot(list[i - 1].c)) {
				idx = find_next_reader(list, i + 1, op->i.rt);
				if (idx > 0 && list[idx].i.op == (op->i.op ^ 0x4)
				    && list[idx].i.rs == op->i.rs
				    && list[idx].i.rt == op->i.rt
				    && abs((s16)op->i.imm - (s16)list[idx].i.imm) == 3) {
					/* Replace a LWL/LWR combo with a META_LWU */
					if (op->i.op == OP_LWL)
						op->i.imm -= 3;
					op->i.op = OP_META_LWU;
					list[idx].opcode = 0;
					pr_debug("Convert LWL/LWR to LWU\n");
				}
			}
			break;
		case OP_SWL:
		case OP_SWR:
			if (i == 0 || !has_delay_slot(list[i - 1].c)) {
				idx = find_next_reader(list, i + 1, op->i.rt);
				if (idx > 0 && list[idx].i.op == (op->i.op ^ 0x4)
				    && list[idx].i.rs == op->i.rs
				    && list[idx].i.rt == op->i.rt
				    && abs((s16)op->i.imm - (s16)list[idx].i.imm) == 3) {
					/* Replace a SWL/SWR combo with a META_SWU */
					if (op->i.op == OP_SWL)
						op->i.imm -= 3;
					op->i.op = OP_META_SWU;
					list[idx].opcode = 0;
					pr_debug("Convert SWL/SWR to SWU\n");
				}
			}
			break;
		case OP_REGIMM:
			switch (op->r.rt) {
			case OP_REGIMM_BLTZ:
			case OP_REGIMM_BGEZ:
				if (!(v[op->r.rs].known & BIT(31)))
					break;

				if (!!(v[op->r.rs].value & BIT(31))
				    ^ (op->r.rt == OP_REGIMM_BGEZ)) {
					pr_debug("Found always-taken BLTZ/BGEZ\n");
					op->i.op = OP_BEQ;
					op->i.rs = 0;
					op->i.rt = 0;
				} else {
					pr_debug("Found never-taken BLTZ/BGEZ\n");

					if (!op_flag_no_ds(op->flags))
						maybe_remove_load_delay(&list[i + 1]);

					local = op_flag_local_branch(op->flags);
					op->opcode = 0;
					op->flags = 0;

					if (local)
						lightrec_reset_syncs(block);
				}
				break;
			case OP_REGIMM_BLTZAL:
			case OP_REGIMM_BGEZAL:
				/* TODO: Detect always-taken and replace with JAL */
				break;
			}
			break;
		case OP_SPECIAL:
			switch (op->r.op) {
			case OP_SPECIAL_SRAV:
				if ((v[op->r.rs].known & 0x1f) != 0x1f)
					break;

				pr_debug("Convert SRAV to SRA\n");
				op->r.imm = v[op->r.rs].value & 0x1f;
				op->r.op = OP_SPECIAL_SRA;

				fallthrough;
			case OP_SPECIAL_SRA:
				if (op->r.imm == 0) {
					pr_debug("Convert SRA #0 to MOV\n");
					op->m.rs = op->r.rt;
					op->m.op = OP_META_MOV;
					op->i.op = OP_META;
					break;
				}
				break;

			case OP_SPECIAL_SLLV:
				if ((v[op->r.rs].known & 0x1f) != 0x1f)
					break;

				pr_debug("Convert SLLV to SLL\n");
				op->r.imm = v[op->r.rs].value & 0x1f;
				op->r.op = OP_SPECIAL_SLL;

				fallthrough;
			case OP_SPECIAL_SLL:
				if (op->r.imm == 0) {
					pr_debug("Convert SLL #0 to MOV\n");
					op->m.rs = op->r.rt;
					op->m.op = OP_META_MOV;
					op->i.op = OP_META;
				}

				lightrec_optimize_sll_sra(block->opcode_list, i, v);
				break;

			case OP_SPECIAL_SRLV:
				if ((v[op->r.rs].known & 0x1f) != 0x1f)
					break;

				pr_debug("Convert SRLV to SRL\n");
				op->r.imm = v[op->r.rs].value & 0x1f;
				op->r.op = OP_SPECIAL_SRL;

				fallthrough;
			case OP_SPECIAL_SRL:
				if (op->r.imm == 0) {
					pr_debug("Convert SRL #0 to MOV\n");
					op->m.rs = op->r.rt;
					op->m.op = OP_META_MOV;
					op->i.op = OP_META;
				}
				break;

			case OP_SPECIAL_MULT:
			case OP_SPECIAL_MULTU:
				if (is_known(v, op->r.rs) &&
				    is_power_of_two(v[op->r.rs].value)) {
					tmp = op->c.i.rs;
					op->c.i.rs = op->c.i.rt;
					op->c.i.rt = tmp;
				} else if (!is_known(v, op->r.rt) ||
					   !is_power_of_two(v[op->r.rt].value)) {
					break;
				}

				pr_debug("Multiply by power-of-two: %"PRIu32"\n",
					 v[op->r.rt].value);

				if (op->r.op == OP_SPECIAL_MULT)
					op->i.op = OP_META_MULT2;
				else
					op->i.op = OP_META_MULTU2;

				op->r.op = ctz32(v[op->r.rt].value);
				break;
			case OP_SPECIAL_NOR:
				if (op->r.rs == 0 || op->r.rt == 0) {
					pr_debug("Convert NOR $zero to COM\n");
					op->i.op = OP_META;
					op->m.op = OP_META_COM;
					if (!op->m.rs)
						op->m.rs = op->r.rt;
				}
				break;
			case OP_SPECIAL_OR:
			case OP_SPECIAL_ADD:
			case OP_SPECIAL_ADDU:
				if (op->r.rs == 0) {
					pr_debug("Convert OR/ADD $zero to MOV\n");
					op->m.rs = op->r.rt;
					op->m.op = OP_META_MOV;
					op->i.op = OP_META;
				}
				fallthrough;
			case OP_SPECIAL_SUB:
			case OP_SPECIAL_SUBU:
				if (op->r.rt == 0) {
					pr_debug("Convert OR/ADD/SUB $zero to MOV\n");
					op->m.op = OP_META_MOV;
					op->i.op = OP_META;
				}
				fallthrough;
			default:
				break;
			}
			fallthrough;
		default:
			break;
		}
	}

	return 0;
}

static bool lightrec_can_switch_delay_slot(union code op, union code next_op)
{
	switch (op.i.op) {
	case OP_SPECIAL:
		switch (op.r.op) {
		case OP_SPECIAL_JALR:
			if (opcode_reads_register(next_op, op.r.rd) ||
			    opcode_writes_register(next_op, op.r.rd))
				return false;
			fallthrough;
		case OP_SPECIAL_JR:
			if (opcode_writes_register(next_op, op.r.rs))
				return false;
			fallthrough;
		default:
			break;
		}
		fallthrough;
	case OP_J:
		break;
	case OP_JAL:
		if (opcode_reads_register(next_op, 31) ||
		    opcode_writes_register(next_op, 31))
			return false;;

		break;
	case OP_BEQ:
	case OP_BNE:
		if (op.i.rt && opcode_writes_register(next_op, op.i.rt))
			return false;
		fallthrough;
	case OP_BLEZ:
	case OP_BGTZ:
		if (op.i.rs && opcode_writes_register(next_op, op.i.rs))
			return false;
		break;
	case OP_REGIMM:
		switch (op.r.rt) {
		case OP_REGIMM_BLTZAL:
		case OP_REGIMM_BGEZAL:
			if (opcode_reads_register(next_op, 31) ||
			    opcode_writes_register(next_op, 31))
				return false;
			fallthrough;
		case OP_REGIMM_BLTZ:
		case OP_REGIMM_BGEZ:
			if (op.i.rs && opcode_writes_register(next_op, op.i.rs))
				return false;
			break;
		}
		fallthrough;
	default:
		break;
	}

	return true;
}

static int lightrec_switch_delay_slots(struct lightrec_state *state, struct block *block)
{
	struct opcode *list, *next = &block->opcode_list[0];
	unsigned int i;
	union code op, next_op;
	u32 flags;

	for (i = 0; i < block->nb_ops - 1; i++) {
		list = next;
		next = &block->opcode_list[i + 1];
		next_op = next->c;
		op = list->c;

		if (!has_delay_slot(op) || op_flag_no_ds(list->flags) ||
		    op_flag_emulate_branch(list->flags) ||
		    op.opcode == 0 || next_op.opcode == 0)
			continue;

		if (is_delay_slot(block->opcode_list, i))
			continue;

		if (op_flag_sync(next->flags))
			continue;

		if (op_flag_load_delay(next->flags) && opcode_is_load(next_op))
			continue;

		if (!lightrec_can_switch_delay_slot(list->c, next_op))
			continue;

		pr_debug("Swap branch and delay slot opcodes "
			 "at offsets 0x%x / 0x%x\n",
			 i << 2, (i + 1) << 2);

		flags = next->flags | (list->flags & LIGHTREC_SYNC);
		list->c = next_op;
		next->c = op;
		next->flags = (list->flags | LIGHTREC_NO_DS) & ~LIGHTREC_SYNC;
		list->flags = flags | LIGHTREC_NO_DS;
	}

	return 0;
}

static int lightrec_detect_impossible_branches(struct lightrec_state *state,
					       struct block *block)
{
	struct opcode *op, *list = block->opcode_list, *next = &list[0];
	unsigned int i;
	int ret = 0;

	for (i = 0; i < block->nb_ops - 1; i++) {
		op = next;
		next = &list[i + 1];

		if (!has_delay_slot(op->c) ||
		    (!has_delay_slot(next->c) &&
		     !opcode_is_mfc(next->c) &&
		     !(next->i.op == OP_CP0 && next->r.rs == OP_CP0_RFE)))
			continue;

		if (op->c.opcode == next->c.opcode) {
			/* The delay slot is the exact same opcode as the branch
			 * opcode: this is effectively a NOP */
			next->c.opcode = 0;
			continue;
		}

		op->flags |= LIGHTREC_EMULATE_BRANCH;

		if (OPT_LOCAL_BRANCHES && i + 2 < block->nb_ops) {
			/* The interpreter will only emulate the branch, then
			 * return to the compiled code. Add a SYNC after the
			 * branch + delay slot in the case where the branch
			 * was not taken. */
			list[i + 2].flags |= LIGHTREC_SYNC;
		}
	}

	return ret;
}

static bool is_local_branch(const struct block *block, unsigned int idx)
{
	const struct opcode *op = &block->opcode_list[idx];
	s32 offset;

	switch (op->c.i.op) {
	case OP_BEQ:
	case OP_BNE:
	case OP_BLEZ:
	case OP_BGTZ:
	case OP_REGIMM:
		offset = idx + 1 + (s16)op->c.i.imm;
		if (offset >= 0 && offset < block->nb_ops)
			return true;
		fallthrough;
	default:
		return false;
	}
}

static int lightrec_handle_load_delays(struct lightrec_state *state,
				       struct block *block)
{
	struct opcode *op, *list = block->opcode_list;
	unsigned int i;
	s16 imm;

	for (i = 0; i < block->nb_ops; i++) {
		op = &list[i];

		if (!opcode_has_load_delay(op->c))
			continue;

		if (!is_delay_slot(list, i)) {
			/* Only handle load delays in delay slots.
			 * PSX games never abused load delay slots otherwise. */
			continue;
		}

		if (is_local_branch(block, i - 1)) {
			imm = (s16)list[i - 1].c.i.imm;

			if (!opcode_reads_register(list[i + imm].c, op->c.i.rt)) {
				/* The target opcode of the branch is inside
				 * the block, and it does not read the register
				 * written to by the load opcode; we can ignore
				 * the load delay. */
				continue;
			}
		}

		op->flags |= LIGHTREC_LOAD_DELAY;
	}

	return 0;
}

static int lightrec_swap_load_delays(struct lightrec_state *state,
				     struct block *block)
{
	unsigned int i;
	union code c, next;
	bool in_ds = false, skip_next = false;
	struct opcode op;

	if (block->nb_ops < 2)
		return 0;

	for (i = 0; i < block->nb_ops - 2; i++) {
		c = block->opcode_list[i].c;

		if (skip_next) {
			skip_next = false;
		} else if (!in_ds && opcode_is_load(c) && c.i.op != OP_LWC2) {
			next = block->opcode_list[i + 1].c;

			switch (next.i.op) {
			case OP_LWL:
			case OP_LWR:
				continue;
			}

			if (has_delay_slot(next))
				continue;

			if (opcode_reads_register(next, c.i.rt)
			    && !opcode_writes_register(next, c.i.rs)) {
				pr_debug("Swapping opcodes at offset 0x%x to "
					 "respect load delay\n", i << 2);

				op = block->opcode_list[i];
				block->opcode_list[i] = block->opcode_list[i + 1];
				block->opcode_list[i + 1] = op;
				skip_next = true;
			}
		}

		in_ds = has_delay_slot(c);
	}

	return 0;
}

static int lightrec_local_branches(struct lightrec_state *state, struct block *block)
{
	const struct opcode *ds;
	struct opcode *list;
	unsigned int i;
	s32 offset;

	for (i = 0; i < block->nb_ops; i++) {
		list = &block->opcode_list[i];

		if (should_emulate(list) || !is_local_branch(block, i))
			continue;

		offset = i + 1 + (s16)list->c.i.imm;

		pr_debug("Found local branch to offset 0x%"PRIx32"\n", offset << 2);

		ds = get_delay_slot(block->opcode_list, i);
		if (op_flag_load_delay(ds->flags) && opcode_is_load(ds->c)) {
			pr_debug("Branch delay slot has a load delay - skip\n");
			continue;
		}

		if (should_emulate(&block->opcode_list[offset])) {
			pr_debug("Branch target must be emulated - skip\n");
			continue;
		}

		if (offset && has_delay_slot(block->opcode_list[offset - 1].c)) {
			pr_debug("Branch target is a delay slot - skip\n");
			continue;
		}

		list->flags |= LIGHTREC_LOCAL_BRANCH;
	}

	lightrec_reset_syncs(block);

	return 0;
}

bool has_delay_slot(union code op)
{
	switch (op.i.op) {
	case OP_SPECIAL:
		switch (op.r.op) {
		case OP_SPECIAL_JR:
		case OP_SPECIAL_JALR:
			return true;
		default:
			return false;
		}
	case OP_J:
	case OP_JAL:
	case OP_BEQ:
	case OP_BNE:
	case OP_BLEZ:
	case OP_BGTZ:
	case OP_REGIMM:
		return true;
	default:
		return false;
	}
}

bool is_delay_slot(const struct opcode *list, unsigned int offset)
{
	return offset > 0
		&& !op_flag_no_ds(list[offset - 1].flags)
		&& has_delay_slot(list[offset - 1].c);
}

bool should_emulate(const struct opcode *list)
{
	return op_flag_emulate_branch(list->flags) && has_delay_slot(list->c);
}

static bool op_writes_rd(union code c)
{
	switch (c.i.op) {
	case OP_SPECIAL:
	case OP_META:
		return true;
	default:
		return false;
	}
}

static void lightrec_add_reg_op(struct opcode *op, u8 reg, u32 reg_op)
{
	if (op_writes_rd(op->c) && reg == op->r.rd)
		op->flags |= LIGHTREC_REG_RD(reg_op);
	else if (op->i.rs == reg)
		op->flags |= LIGHTREC_REG_RS(reg_op);
	else if (op->i.rt == reg)
		op->flags |= LIGHTREC_REG_RT(reg_op);
	else
		pr_debug("Cannot add unload/clean/discard flag: "
			 "opcode does not touch register %s!\n",
			 lightrec_reg_name(reg));
}

static void lightrec_add_unload(struct opcode *op, u8 reg)
{
	lightrec_add_reg_op(op, reg, LIGHTREC_REG_UNLOAD);
}

static void lightrec_add_discard(struct opcode *op, u8 reg)
{
	lightrec_add_reg_op(op, reg, LIGHTREC_REG_DISCARD);
}

static void lightrec_add_clean(struct opcode *op, u8 reg)
{
	lightrec_add_reg_op(op, reg, LIGHTREC_REG_CLEAN);
}

static void
lightrec_early_unload_sync(struct opcode *list, s16 *last_r, s16 *last_w)
{
	unsigned int reg;
	s16 offset;

	for (reg = 0; reg < 34; reg++) {
		offset = s16_max(last_w[reg], last_r[reg]);

		if (offset >= 0)
			lightrec_add_unload(&list[offset], reg);
	}

	memset(last_r, 0xff, sizeof(*last_r) * 34);
	memset(last_w, 0xff, sizeof(*last_w) * 34);
}

static int lightrec_early_unload(struct lightrec_state *state, struct block *block)
{
	u16 i, offset;
	struct opcode *op;
	s16 last_r[34], last_w[34], last_sync = 0, next_sync = 0;
	u64 mask_r, mask_w, dirty = 0, loaded = 0;
	u8 reg, load_delay_reg = 0;

	memset(last_r, 0xff, sizeof(last_r));
	memset(last_w, 0xff, sizeof(last_w));

	/*
	 * Clean if:
	 * - the register is dirty, and is read again after a branch opcode
	 *
	 * Unload if:
	 * - the register is dirty or loaded, and is not read again
	 * - the register is dirty or loaded, and is written again after a branch opcode
	 * - the next opcode has the SYNC flag set
	 *
	 * Discard if:
	 * - the register is dirty or loaded, and is written again
	 */

	for (i = 0; i < block->nb_ops; i++) {
		op = &block->opcode_list[i];

		if (OPT_HANDLE_LOAD_DELAYS && load_delay_reg) {
			/* Handle delayed register write from load opcodes in
			 * delay slots */
			last_w[load_delay_reg] = i;
			load_delay_reg = 0;
		}

		if (op_flag_sync(op->flags) || should_emulate(op)) {
			/* The next opcode has the SYNC flag set, or is a branch
			 * that should be emulated: unload all registers. */
			lightrec_early_unload_sync(block->opcode_list, last_r, last_w);
			dirty = 0;
			loaded = 0;
		}

		if (next_sync == i) {
			last_sync = i;
			pr_debug("Last sync: 0x%x\n", last_sync << 2);
		}

		if (has_delay_slot(op->c)) {
			next_sync = i + 1 + !op_flag_no_ds(op->flags);
			pr_debug("Next sync: 0x%x\n", next_sync << 2);
		}

		mask_r = opcode_read_mask(op->c);
		mask_w = opcode_write_mask(op->c);

		if (op_flag_load_delay(op->flags) && opcode_is_load(op->c)) {
			/* If we have a load opcode in a delay slot, its target
			 * register is actually not written there but at a
			 * later point, in the dispatcher. Prevent the algorithm
			 * from discarding its previous value. */
			load_delay_reg = op->c.i.rt;
			mask_w &= ~BIT(op->c.i.rt);
		}

		for (reg = 0; reg < 34; reg++) {
			if (mask_r & BIT(reg)) {
				if (dirty & BIT(reg) && last_w[reg] < last_sync) {
					/* The register is dirty, and is read
					 * again after a branch: clean it */

					lightrec_add_clean(&block->opcode_list[last_w[reg]], reg);
					dirty &= ~BIT(reg);
					loaded |= BIT(reg);
				}

				last_r[reg] = i;
			}

			if (mask_w & BIT(reg)) {
				if ((dirty & BIT(reg) && last_w[reg] < last_sync) ||
				    (loaded & BIT(reg) && last_r[reg] < last_sync)) {
					/* The register is dirty or loaded, and
					 * is written again after a branch:
					 * unload it */

					offset = s16_max(last_w[reg], last_r[reg]);
					lightrec_add_unload(&block->opcode_list[offset], reg);
					dirty &= ~BIT(reg);
					loaded &= ~BIT(reg);
				} else if (!(mask_r & BIT(reg)) &&
					   ((dirty & BIT(reg) && last_w[reg] > last_sync) ||
					   (loaded & BIT(reg) && last_r[reg] > last_sync))) {
					/* The register is dirty or loaded, and
					 * is written again: discard it */

					offset = s16_max(last_w[reg], last_r[reg]);
					lightrec_add_discard(&block->opcode_list[offset], reg);
					dirty &= ~BIT(reg);
					loaded &= ~BIT(reg);
				}

				last_w[reg] = i;
			}

		}

		dirty |= mask_w;
		loaded |= mask_r;
	}

	/* Unload all registers that are dirty or loaded at the end of block. */
	lightrec_early_unload_sync(block->opcode_list, last_r, last_w);

	return 0;
}

static int lightrec_flag_io(struct lightrec_state *state, struct block *block)
{
	struct opcode *list;
	enum psx_map psx_map;
	struct constprop_data v[32] = LIGHTREC_CONSTPROP_INITIALIZER;
	unsigned int i;
	u32 val, kunseg_val;
	bool no_mask;

	for (i = 0; i < block->nb_ops; i++) {
		list = &block->opcode_list[i];

		lightrec_consts_propagate(block, i, v);

		switch (list->i.op) {
		case OP_SB:
		case OP_SH:
		case OP_SW:
			/* Mark all store operations that target $sp or $gp
			 * as not requiring code invalidation. This is based
			 * on the heuristic that stores using one of these
			 * registers as address will never hit a code page. */
			if (list->i.rs >= 28 && list->i.rs <= 29 &&
			    !state->maps[PSX_MAP_KERNEL_USER_RAM].ops) {
				pr_debug("Flaging opcode "X32_FMT" as not requiring invalidation\n",
					 list->opcode);
				list->flags |= LIGHTREC_NO_INVALIDATE;
			}

			/* Detect writes whose destination address is inside the
			 * current block, using constant propagation. When these
			 * occur, we mark the blocks as not compilable. */
			if (is_known(v, list->i.rs) &&
			    kunseg(v[list->i.rs].value) >= kunseg(block->pc) &&
			    kunseg(v[list->i.rs].value) < (kunseg(block->pc) + block->nb_ops * 4)) {
				pr_debug("Self-modifying block detected\n");
				block_set_flags(block, BLOCK_NEVER_COMPILE);
				list->flags |= LIGHTREC_SMC;
			}
			fallthrough;
		case OP_SWL:
		case OP_SWR:
		case OP_SWC2:
		case OP_LB:
		case OP_LBU:
		case OP_LH:
		case OP_LHU:
		case OP_LW:
		case OP_LWL:
		case OP_LWR:
		case OP_LWC2:
			if (v[list->i.rs].known | v[list->i.rs].sign) {
				psx_map = lightrec_get_constprop_map(state, v,
								     list->i.rs,
								     (s16) list->i.imm);

				if (psx_map != PSX_MAP_UNKNOWN && !is_known(v, list->i.rs))
					pr_debug("Detected map thanks to bit-level const propagation!\n");

				list->flags &= ~LIGHTREC_IO_MASK;

				val = v[list->i.rs].value + (s16) list->i.imm;
				kunseg_val = kunseg(val);

				no_mask = (v[list->i.rs].known & ~v[list->i.rs].value
					   & 0xe0000000) == 0xe0000000;

				switch (psx_map) {
				case PSX_MAP_KERNEL_USER_RAM:
					if (no_mask)
						list->flags |= LIGHTREC_NO_MASK;
					fallthrough;
				case PSX_MAP_MIRROR1:
				case PSX_MAP_MIRROR2:
				case PSX_MAP_MIRROR3:
					pr_debug("Flaging opcode %u as RAM access\n", i);
					list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_RAM);
					if (no_mask && state->mirrors_mapped)
						list->flags |= LIGHTREC_NO_MASK;
					break;
				case PSX_MAP_BIOS:
					pr_debug("Flaging opcode %u as BIOS access\n", i);
					list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_BIOS);
					if (no_mask)
						list->flags |= LIGHTREC_NO_MASK;
					break;
				case PSX_MAP_SCRATCH_PAD:
					pr_debug("Flaging opcode %u as scratchpad access\n", i);
					list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_SCRATCH);
					if (no_mask)
						list->flags |= LIGHTREC_NO_MASK;

					/* Consider that we're never going to run code from
					 * the scratchpad. */
					list->flags |= LIGHTREC_NO_INVALIDATE;
					break;
				case PSX_MAP_HW_REGISTERS:
					if (state->ops.hw_direct &&
					    state->ops.hw_direct(kunseg_val,
								 opcode_is_store(list->c),
								 opcode_get_io_size(list->c))) {
						pr_debug("Flagging opcode %u as direct I/O access\n",
							 i);
						list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_DIRECT_HW);

						if (no_mask)
							list->flags |= LIGHTREC_NO_MASK;
					} else {
						pr_debug("Flagging opcode %u as I/O access\n",
							 i);
						list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_HW);
					}
					break;
				default:
					break;
				}
			}

			if (!LIGHTREC_FLAGS_GET_IO_MODE(list->flags)
			    && list->i.rs >= 28 && list->i.rs <= 29
			    && !state->maps[PSX_MAP_KERNEL_USER_RAM].ops) {
				/* Assume that all I/O operations that target
				 * $sp or $gp will always only target a mapped
				 * memory (RAM, BIOS, scratchpad). */
				if (state->opt_flags & LIGHTREC_OPT_SP_GP_HIT_RAM)
					list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_RAM);
				else
					list->flags |= LIGHTREC_IO_MODE(LIGHTREC_IO_DIRECT);
			}

			fallthrough;
		default:
			break;
		}
	}

	return 0;
}

static u8 get_mfhi_mflo_reg(const struct block *block, u16 offset,
			    const struct opcode *last,
			    u32 mask, bool sync, bool mflo, bool another)
{
	const struct opcode *op, *next = &block->opcode_list[offset];
	u32 old_mask;
	u8 reg2, reg = mflo ? REG_LO : REG_HI;
	u16 branch_offset;
	unsigned int i;

	for (i = offset; i < block->nb_ops; i++) {
		op = next;
		next = &block->opcode_list[i + 1];
		old_mask = mask;

		/* If any other opcode writes or reads to the register
		 * we'd use, then we cannot use it anymore. */
		mask |= opcode_read_mask(op->c);
		mask |= opcode_write_mask(op->c);

		if (op_flag_sync(op->flags))
			sync = true;

		switch (op->i.op) {
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
		case OP_REGIMM:
			/* TODO: handle backwards branches too */
			if (!last && op_flag_local_branch(op->flags) &&
			    (s16)op->c.i.imm >= 0) {
				branch_offset = i + 1 + (s16)op->c.i.imm
					- !!op_flag_no_ds(op->flags);

				reg = get_mfhi_mflo_reg(block, branch_offset, NULL,
							mask, sync, mflo, false);
				reg2 = get_mfhi_mflo_reg(block, offset + 1, next,
							 mask, sync, mflo, false);
				if (reg > 0 && reg == reg2)
					return reg;
				if (!reg && !reg2)
					return 0;
			}

			return mflo ? REG_LO : REG_HI;
		case OP_META_MULT2:
		case OP_META_MULTU2:
			return 0;
		case OP_SPECIAL:
			switch (op->r.op) {
			case OP_SPECIAL_MULT:
			case OP_SPECIAL_MULTU:
			case OP_SPECIAL_DIV:
			case OP_SPECIAL_DIVU:
				return 0;
			case OP_SPECIAL_MTHI:
				if (!mflo)
					return 0;
				continue;
			case OP_SPECIAL_MTLO:
				if (mflo)
					return 0;
				continue;
			case OP_SPECIAL_JR:
				if (op->r.rs != 31)
					return reg;

				if (!sync && !op_flag_no_ds(op->flags) &&
				    (next->i.op == OP_SPECIAL) &&
				    ((!mflo && next->r.op == OP_SPECIAL_MFHI) ||
				    (mflo && next->r.op == OP_SPECIAL_MFLO)))
					return next->r.rd;

				return 0;
			case OP_SPECIAL_JALR:
				return reg;
			case OP_SPECIAL_MFHI:
				if (!mflo) {
					if (another)
						return op->r.rd;
					/* Must use REG_HI if there is another MFHI target*/
					reg2 = get_mfhi_mflo_reg(block, i + 1, next,
							 0, sync, mflo, true);
					if (reg2 > 0 && reg2 != REG_HI)
						return REG_HI;

					if (!sync && !(old_mask & BIT(op->r.rd)))
						return op->r.rd;
					else
						return REG_HI;
				}
				continue;
			case OP_SPECIAL_MFLO:
				if (mflo) {
					if (another)
						return op->r.rd;
					/* Must use REG_LO if there is another MFLO target*/
					reg2 = get_mfhi_mflo_reg(block, i + 1, next,
							 0, sync, mflo, true);
					if (reg2 > 0 && reg2 != REG_LO)
						return REG_LO;

					if (!sync && !(old_mask & BIT(op->r.rd)))
						return op->r.rd;
					else
						return REG_LO;
				}
				continue;
			default:
				break;
			}

			fallthrough;
		default:
			continue;
		}
	}

	return reg;
}

static void lightrec_replace_lo_hi(struct block *block, u16 offset,
				   u16 last, bool lo)
{
	unsigned int i;
	u32 branch_offset;

	/* This function will remove the following MFLO/MFHI. It must be called
	 * only if get_mfhi_mflo_reg() returned a non-zero value. */

	for (i = offset; i < last; i++) {
		struct opcode *op = &block->opcode_list[i];

		switch (op->i.op) {
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
		case OP_REGIMM:
			/* TODO: handle backwards branches too */
			if (op_flag_local_branch(op->flags) && (s16)op->c.i.imm >= 0) {
				branch_offset = i + 1 + (s16)op->c.i.imm
					- !!op_flag_no_ds(op->flags);

				lightrec_replace_lo_hi(block, branch_offset, last, lo);
				lightrec_replace_lo_hi(block, i + 1, branch_offset, lo);
			}
			break;

		case OP_SPECIAL:
			if (lo && op->r.op == OP_SPECIAL_MFLO) {
				pr_debug("Removing MFLO opcode at offset 0x%x\n",
					 i << 2);
				op->opcode = 0;
				return;
			} else if (!lo && op->r.op == OP_SPECIAL_MFHI) {
				pr_debug("Removing MFHI opcode at offset 0x%x\n",
					 i << 2);
				op->opcode = 0;
				return;
			}

			fallthrough;
		default:
			break;
		}
	}
}

static bool lightrec_always_skip_div_check(void)
{
#ifdef __mips__
	return true;
#else
	return false;
#endif
}

static int lightrec_flag_mults_divs(struct lightrec_state *state, struct block *block)
{
	struct opcode *list = NULL;
	struct constprop_data v[32] = LIGHTREC_CONSTPROP_INITIALIZER;
	u8 reg_hi, reg_lo;
	unsigned int i;

	for (i = 0; i < block->nb_ops - 1; i++) {
		list = &block->opcode_list[i];

		lightrec_consts_propagate(block, i, v);

		switch (list->i.op) {
		case OP_SPECIAL:
			switch (list->r.op) {
			case OP_SPECIAL_DIV:
			case OP_SPECIAL_DIVU:
				/* If we are dividing by a non-zero constant, don't
				 * emit the div-by-zero check. */
				if (lightrec_always_skip_div_check() ||
				    (v[list->r.rt].known & v[list->r.rt].value)) {
					list->flags |= LIGHTREC_NO_DIV_CHECK;
				}
				fallthrough;
			case OP_SPECIAL_MULT:
			case OP_SPECIAL_MULTU:
				break;
			default:
				continue;
			}
			fallthrough;
		case OP_META_MULT2:
		case OP_META_MULTU2:
			break;
		default:
			continue;
		}

		/* Don't support opcodes in delay slots */
		if (is_delay_slot(block->opcode_list, i) ||
		    op_flag_no_ds(list->flags)) {
			continue;
		}

		reg_lo = get_mfhi_mflo_reg(block, i + 1, NULL, 0, false, true, false);
		if (reg_lo == 0) {
			pr_debug("Mark MULT(U)/DIV(U) opcode at offset 0x%x as"
				 " not writing LO\n", i << 2);
			list->flags |= LIGHTREC_NO_LO;
		}

		reg_hi = get_mfhi_mflo_reg(block, i + 1, NULL, 0, false, false, false);
		if (reg_hi == 0) {
			pr_debug("Mark MULT(U)/DIV(U) opcode at offset 0x%x as"
				 " not writing HI\n", i << 2);
			list->flags |= LIGHTREC_NO_HI;
		}

		if (!reg_lo && !reg_hi) {
			pr_debug("Both LO/HI unused in this block, they will "
				 "probably be used in parent block - removing "
				 "flags.\n");
			list->flags &= ~(LIGHTREC_NO_LO | LIGHTREC_NO_HI);
		}

		if (0/* Broken */ && reg_lo > 0 && reg_lo != REG_LO) {
			pr_debug("Found register %s to hold LO (rs = %u, rt = %u)\n",
				 lightrec_reg_name(reg_lo), list->r.rs, list->r.rt);

			lightrec_replace_lo_hi(block, i + 1, block->nb_ops, true);
			list->r.rd = reg_lo;
		} else {
			list->r.rd = 0;
		}

		if (0/* Broken */ && reg_hi > 0 && reg_hi != REG_HI) {
			pr_debug("Found register %s to hold HI (rs = %u, rt = %u)\n",
				 lightrec_reg_name(reg_hi), list->r.rs, list->r.rt);

			lightrec_replace_lo_hi(block, i + 1, block->nb_ops, false);
			list->r.imm = reg_hi;
		} else {
			list->r.imm = 0;
		}
	}

	return 0;
}

static bool remove_div_sequence(struct block *block, unsigned int offset)
{
	struct opcode *op;
	unsigned int i, found = 0;

	/*
	 * Scan for the zero-checking sequence that GCC automatically introduced
	 * after most DIV/DIVU opcodes. This sequence checks the value of the
	 * divisor, and if zero, executes a BREAK opcode, causing the BIOS
	 * handler to crash the PS1.
	 *
	 * For DIV opcodes, this sequence additionally checks that the signed
	 * operation does not overflow.
	 *
	 * With the assumption that the games never crashed the PS1, we can
	 * therefore assume that the games never divided by zero or overflowed,
	 * and these sequences can be removed.
	 */

	for (i = offset; i < block->nb_ops; i++) {
		op = &block->opcode_list[i];

		if (!found) {
			if (op->i.op == OP_SPECIAL &&
			    (op->r.op == OP_SPECIAL_DIV || op->r.op == OP_SPECIAL_DIVU))
				break;

			if ((op->opcode & 0xfc1fffff) == 0x14000002) {
				/* BNE ???, zero, +8 */
				found++;
			} else {
				offset++;
			}
		} else if (found == 1 && !op->opcode) {
			/* NOP */
			found++;
		} else if (found == 2 && op->opcode == 0x0007000d) {
			/* BREAK 0x1c00 */
			found++;
		} else if (found == 3 && op->opcode == 0x2401ffff) {
			/* LI at, -1 */
			found++;
		} else if (found == 4 && (op->opcode & 0xfc1fffff) == 0x14010004) {
			/* BNE ???, at, +16 */
			found++;
		} else if (found == 5 && op->opcode == 0x3c018000) {
			/* LUI at, 0x8000 */
			found++;
		} else if (found == 6 && (op->opcode & 0x141fffff) == 0x14010002) {
			/* BNE ???, at, +16 */
			found++;
		} else if (found == 7 && !op->opcode) {
			/* NOP */
			found++;
		} else if (found == 8 && op->opcode == 0x0006000d) {
			/* BREAK 0x1800 */
			found++;
			break;
		} else {
			break;
		}
	}

	if (found >= 3) {
		if (found != 9)
			found = 3;

		pr_debug("Removing DIV%s sequence at offset 0x%x\n",
			 found == 9 ? "" : "U", offset << 2);

		for (i = 0; i < found; i++)
			block->opcode_list[offset + i].opcode = 0;

		return true;
	}

	return false;
}

static int lightrec_remove_div_by_zero_check_sequence(struct lightrec_state *state,
						      struct block *block)
{
	struct opcode *op;
	unsigned int i;

	for (i = 0; i < block->nb_ops; i++) {
		op = &block->opcode_list[i];

		if (op->i.op == OP_SPECIAL &&
		    (op->r.op == OP_SPECIAL_DIVU || op->r.op == OP_SPECIAL_DIV) &&
		    remove_div_sequence(block, i + 1))
			op->flags |= LIGHTREC_NO_DIV_CHECK;
	}

	return 0;
}

static const u32 memset_code[] = {
	0x10a00006,	// beqz		a1, 2f
	0x24a2ffff,	// addiu	v0,a1,-1
	0x2403ffff,	// li		v1,-1
	0xac800000,	// 1: sw	zero,0(a0)
	0x2442ffff,	// addiu	v0,v0,-1
	0x1443fffd,	// bne		v0,v1, 1b
	0x24840004,	// addiu	a0,a0,4
	0x03e00008,	// 2: jr	ra
	0x00000000,	// nop
};

static int lightrec_replace_memset(struct lightrec_state *state, struct block *block)
{
	unsigned int i;
	union code c;

	for (i = 0; i < block->nb_ops; i++) {
		c = block->opcode_list[i].c;

		if (c.opcode != memset_code[i])
			return 0;

		if (i == ARRAY_SIZE(memset_code) - 1) {
			/* success! */
			pr_debug("Block at "PC_FMT" is a memset\n", block->pc);
			block_set_flags(block,
					BLOCK_IS_MEMSET | BLOCK_NEVER_COMPILE);

			/* Return non-zero to skip other optimizers. */
			return 1;
		}
	}

	return 0;
}

static int lightrec_test_preload_pc(struct lightrec_state *state, struct block *block)
{
	unsigned int i;
	union code c;
	u32 flags;

	for (i = 0; i < block->nb_ops; i++) {
		c = block->opcode_list[i].c;
		flags = block->opcode_list[i].flags;

		if (op_flag_sync(flags))
			break;

		switch (c.i.op) {
		case OP_J:
		case OP_JAL:
			block->flags |= BLOCK_PRELOAD_PC;
			return 0;

		case OP_REGIMM:
			switch (c.r.rt) {
			case OP_REGIMM_BLTZAL:
			case OP_REGIMM_BGEZAL:
				block->flags |= BLOCK_PRELOAD_PC;
				return 0;
			default:
				break;
			}
			fallthrough;
		case OP_BEQ:
		case OP_BNE:
		case OP_BLEZ:
		case OP_BGTZ:
			if (!op_flag_local_branch(flags)) {
				block->flags |= BLOCK_PRELOAD_PC;
				return 0;
			}

		case OP_SPECIAL:
			switch (c.r.op) {
			case OP_SPECIAL_JALR:
				if (c.r.rd) {
					block->flags |= BLOCK_PRELOAD_PC;
					return 0;
				}
				break;
			case OP_SPECIAL_SYSCALL:
			case OP_SPECIAL_BREAK:
				block->flags |= BLOCK_PRELOAD_PC;
				return 0;
			default:
				break;
			}
			break;
		}
	}

	return 0;
}

static int (*lightrec_optimizers[])(struct lightrec_state *state, struct block *) = {
	IF_OPT(OPT_REMOVE_DIV_BY_ZERO_SEQ, &lightrec_remove_div_by_zero_check_sequence),
	IF_OPT(OPT_REPLACE_MEMSET, &lightrec_replace_memset),
	IF_OPT(OPT_DETECT_IMPOSSIBLE_BRANCHES, &lightrec_detect_impossible_branches),
	IF_OPT(OPT_HANDLE_LOAD_DELAYS, &lightrec_handle_load_delays),
	IF_OPT(OPT_HANDLE_LOAD_DELAYS, &lightrec_swap_load_delays),
	IF_OPT(OPT_TRANSFORM_OPS, &lightrec_transform_branches),
	IF_OPT(OPT_LOCAL_BRANCHES, &lightrec_local_branches),
	IF_OPT(OPT_TRANSFORM_OPS, &lightrec_transform_ops),
	IF_OPT(OPT_SWITCH_DELAY_SLOTS, &lightrec_switch_delay_slots),
	IF_OPT(OPT_FLAG_IO, &lightrec_flag_io),
	IF_OPT(OPT_FLAG_MULT_DIV, &lightrec_flag_mults_divs),
	IF_OPT(OPT_EARLY_UNLOAD, &lightrec_early_unload),
	IF_OPT(OPT_PRELOAD_PC, &lightrec_test_preload_pc),
};

int lightrec_optimize(struct lightrec_state *state, struct block *block)
{
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(lightrec_optimizers); i++) {
		if (lightrec_optimizers[i]) {
			ret = (*lightrec_optimizers[i])(state, block);
			if (ret)
				return ret;
		}
	}

	return 0;
}
