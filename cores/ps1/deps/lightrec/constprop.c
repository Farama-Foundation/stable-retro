// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#include "constprop.h"
#include "disassembler.h"
#include "lightrec-private.h"

#include <stdbool.h>
#include <string.h>

static u32 get_min_value(const struct constprop_data *d)
{
	/* Min value: all sign bits to 1, all unknown bits but MSB to 0 */
	return (d->value & d->known) | d->sign | (~d->known & BIT(31));
}

static u32 get_max_value(const struct constprop_data *d)
{
	/* Max value: all sign bits to 0, all unknown bits to 1 */
	return ((d->value & d->known) | ~d->known) & ~d->sign;
}

static u32 lightrec_same_sign(const struct constprop_data *d1,
			      const struct constprop_data *d2)
{
	u32 min1, min2, max1, max2, a, b, c, d;

	min1 = get_min_value(d1);
	max1 = get_max_value(d1);
	min2 = get_min_value(d2);
	max2 = get_max_value(d2);

	a = min1 + min2;
	b = min1 + max2;
	c = max1 + min2;
	d = max1 + max2;

	return ((a & b & c & d) | (~a & ~b & ~c & ~d)) & BIT(31);
}

static u32 lightrec_get_sign_mask(const struct constprop_data *d)
{
	u32 imm;

	if (d->sign)
		return d->sign;

	imm = (d->value & BIT(31)) ? d->value : ~d->value;
	imm = ~(imm & d->known);
	if (imm)
		imm = 32 - clz32(imm);

	return imm < 32 ? GENMASK(31, imm) : 0;
}

static void lightrec_propagate_addi(u32 rs, u32 rd,
				    const struct constprop_data *d,
				    struct constprop_data *v)
{
	u32 end, bit, sum, min, max, mask, imm, value;
	struct constprop_data result = {
		.value = v[rd].value,
		.known = v[rd].known,
		.sign = v[rd].sign,
	};
	bool carry = false;

	/* clear unknown bits to ease processing */
	v[rs].value &= v[rs].known;
	value = d->value & d->known;

	mask = ~(lightrec_get_sign_mask(d) & lightrec_get_sign_mask(&v[rs]));
	end = mask ? 32 - clz32(mask) : 0;

	for (bit = 0; bit < 32; bit++) {
		if (v[rs].known & d->known & BIT(bit)) {
			/* the bits are known - compute the resulting bit and
			 * the carry */
			sum = ((u32)carry << bit) + (v[rs].value & BIT(bit))
				+ (value & BIT(bit));

			if (sum & BIT(bit))
				result.value |= BIT(bit);
			else
				result.value &= ~BIT(bit);

			result.known |= BIT(bit);
			result.sign &= ~BIT(bit);
			carry = sum & BIT(bit + 1);
			continue;
		}

		if (bit >= end) {
			/* We're past the last significant bits of the values
			 * (extra sign bits excepted).
			 * The destination register will be sign-extended
			 * starting from here (if no carry) or from the next
			 * bit (if carry).
			 * If the source registers are not sign-extended and we
			 * have no carry, the algorithm is done here. */

			if ((v[rs].sign | d->sign) & BIT(bit)) {
				mask = GENMASK(31, bit);

				if (lightrec_same_sign(&v[rs], d)) {
					/* Theorical minimum and maximum values
					 * have the same sign; therefore the
					 * sign bits are known. */
					min = get_min_value(&v[rs])
						+ get_min_value(d);
					max = get_max_value(&v[rs])
						+ get_max_value(d);

					/* The sum may have less sign bits */
					if ((s32)min < 0)
						mask &= min & max;
					else
						mask &= ~(min | mask);

					result.value = (min & mask)
						| (result.value & ~mask);
					result.known |= mask << carry;
					result.sign = 0;
				} else {
					/* min/max have different signs. */
					result.sign = mask << 1;
					result.known &= ~mask;
				}
				break;
			} else if (!carry) {
				/* Past end bit, no carry; we're done here. */
				break;
			}
		}

		result.known &= ~BIT(bit);
		result.sign &= ~BIT(bit);

		/* Found an unknown bit in one of the registers.
		 * If the carry and the bit in the other register are both zero,
		 * we can continue the algorithm. */
		if (!carry && (((d->known & ~value)
				| (v[rs].known & ~v[rs].value)) & BIT(bit)))
			continue;

		/* We have an unknown bit in one of the source registers, and we
		 * may generate a carry: there's nothing to do. Everything from
		 * this bit till the next known 0 bit or sign bit will be marked
		 * as unknown. The algorithm can then restart at the following
		 * bit. */

		imm = (v[rs].known & d->known & ~v[rs].value & ~value)
			| v[rs].sign | d->sign;

		imm &= GENMASK(31, bit);
		imm = imm ? ctz32(imm) : 31;
		mask = GENMASK(imm, bit);
		result.known &= ~mask;
		result.sign &= ~mask;

		bit = imm;
		carry = false;
	}

	v[rd] = result;
}

static void lightrec_propagate_sub(u32 rs, u32 rt, u32 rd,
				   struct constprop_data *v)
{
	struct constprop_data d = {
		.value = ~v[rt].value,
		.known = v[rt].known,
		.sign = v[rt].sign,
	};
	u32 imm, mask, bit;

	/* Negate the known Rt value, then propagate as a regular ADD. */

	for (bit = 0; bit < 32; bit++) {
		if (!(d.known & BIT(bit))) {
			/* Unknown bit - mark bits unknown up to the next known 0 */

			imm = (d.known & ~d.value) | d.sign;
			imm &= GENMASK(31, bit);
			imm = imm ? ctz32(imm) : 31;
			mask = GENMASK(imm, bit);
			d.known &= ~mask;
			d.sign &= ~mask;
			break;
		}

		if (!(d.value & BIT(bit))) {
			/* Bit is 0: we can set our carry, and the algorithm is done. */
			d.value |= BIT(bit);
			break;
		}

		/* Bit is 1 - set to 0 and continue algorithm */
		d.value &= ~BIT(bit);
	}

	lightrec_propagate_addi(rs, rd, &d, v);
}

static void lightrec_propagate_slt(u32 rs, u32 rd, bool is_signed,
				   const struct constprop_data *d,
				   struct constprop_data *v)
{
	unsigned int bit;

	if (is_signed && (v[rs].known & d->known
			  & (v[rs].value ^ d->value) & BIT(31))) {
		/* If doing a signed comparison and the two bits 31 are known
		 * to be opposite, we can deduce the value. */
		v[rd].value = v[rs].value >> 31;
		v[rd].known = 0xffffffff;
		v[rd].sign = 0;
		return;
	}

	for (bit = 32; bit > 0; bit--) {
		if (!(v[rs].known & d->known & BIT(bit - 1))) {
			/* One bit is unknown and we cannot figure out which
			 * value is smaller. We still know that the upper 31
			 * bits are zero. */
			v[rd].value = 0;
			v[rd].known = 0xfffffffe;
			v[rd].sign = 0;
			break;
		}

		/* The two bits are equal - continue to the next bit. */
		if (~(v[rs].value ^ d->value) & BIT(bit - 1))
			continue;

		/* The two bits aren't equal; we can therefore deduce which
		 * value is smaller. */
		v[rd].value = !(v[rs].value & BIT(bit - 1));
		v[rd].known = 0xffffffff;
		v[rd].sign = 0;
		break;
	}

	if (bit == 0) {
		/* rs == rt and all bits are known */
		v[rd].value = 0;
		v[rd].known = 0xffffffff;
		v[rd].sign = 0;
	}
}

void lightrec_consts_propagate(const struct block *block,
			       unsigned int idx,
			       struct constprop_data *v)
{
	const struct opcode *list = block->opcode_list;
	union code c;
	u32 imm, flags;

	if (idx == 0)
		return;

	/* Register $zero is always, well, zero */
	v[0].value = 0;
	v[0].sign = 0;
	v[0].known = 0xffffffff;

	if (op_flag_sync(list[idx].flags)) {
		memset(&v[1], 0, sizeof(*v) * 31);
		return;
	}

	flags = list[idx - 1].flags;

	if (idx > 1 && !op_flag_sync(flags)) {
		if (op_flag_no_ds(flags))
			c = list[idx - 1].c;
		else
			c = list[idx - 2].c;

		switch (c.i.op) {
		case OP_BNE:
			/* After a BNE $zero + delay slot, we know that the
			 * branch wasn't taken, and therefore the other register
			 * is zero. */
			if (c.i.rs == 0) {
				v[c.i.rt].value = 0;
				v[c.i.rt].sign = 0;
				v[c.i.rt].known = 0xffffffff;
			} else if (c.i.rt == 0) {
				v[c.i.rs].value = 0;
				v[c.i.rs].sign = 0;
				v[c.i.rs].known = 0xffffffff;
			}
			break;
		case OP_BLEZ:
			v[c.i.rs].value &= ~BIT(31);
			v[c.i.rs].known |= BIT(31);
			fallthrough;
		case OP_BEQ:
			/* TODO: handle non-zero? */
			break;
		case OP_REGIMM:
			switch (c.r.rt) {
			case OP_REGIMM_BLTZ:
			case OP_REGIMM_BLTZAL:
				v[c.i.rs].value &= ~BIT(31);
				v[c.i.rs].known |= BIT(31);
				break;
			case OP_REGIMM_BGEZ:
			case OP_REGIMM_BGEZAL:
				v[c.i.rs].value |= BIT(31);
				v[c.i.rs].known |= BIT(31);
				/* TODO: handle non-zero? */
				break;
			}
			break;
		default:
			break;
		}
	}

	c = list[idx - 1].c;

	switch (c.i.op) {
	case OP_SPECIAL:
		switch (c.r.op) {
		case OP_SPECIAL_SLL:
			v[c.r.rd].value = v[c.r.rt].value << c.r.imm;
			v[c.r.rd].known = (v[c.r.rt].known << c.r.imm)
				| (BIT(c.r.imm) - 1);
			v[c.r.rd].sign = v[c.r.rt].sign << c.r.imm;
			break;

		case OP_SPECIAL_SRL:
			v[c.r.rd].value = v[c.r.rt].value >> c.r.imm;
			v[c.r.rd].known = (v[c.r.rt].known >> c.r.imm)
				| ((BIT(c.r.imm) - 1) << (32 - c.r.imm));
			v[c.r.rd].sign = c.r.imm ? 0 : v[c.r.rt].sign;
			break;

		case OP_SPECIAL_SRA:
			v[c.r.rd].value = (s32)v[c.r.rt].value >> c.r.imm;
			v[c.r.rd].sign = (s32)(v[c.r.rt].sign
					       | (~v[c.r.rt].known & 0x80000000)) >> c.r.imm;
			v[c.r.rd].known = (s32)v[c.r.rt].known >> c.r.imm;
			break;

		case OP_SPECIAL_SLLV:
			if ((v[c.r.rs].known & 0x1f) == 0x1f) {
				imm = v[c.r.rs].value & 0x1f;
				v[c.r.rd].value = v[c.r.rt].value << imm;
				v[c.r.rd].known = (v[c.r.rt].known << imm)
					| (BIT(imm) - 1);
				v[c.r.rd].sign = v[c.r.rt].sign << imm;
			} else {
				v[c.r.rd].known = 0;
				v[c.r.rd].sign = 0;
			}
			break;

		case OP_SPECIAL_SRLV:
			if ((v[c.r.rs].known & 0x1f) == 0x1f) {
				imm = v[c.r.rs].value & 0x1f;
				v[c.r.rd].value = v[c.r.rt].value >> imm;
				v[c.r.rd].known = (v[c.r.rt].known >> imm)
					| ((BIT(imm) - 1) << (32 - imm));
				if (imm)
					v[c.r.rd].sign = 0;
			} else {
				v[c.r.rd].known = 0;
				v[c.r.rd].sign = 0;
			}
			break;

		case OP_SPECIAL_SRAV:
			if ((v[c.r.rs].known & 0x1f) == 0x1f) {
				imm = v[c.r.rs].value & 0x1f;
				v[c.r.rd].value = (s32)v[c.r.rt].value >> imm;
				v[c.r.rd].sign = (s32)(v[c.r.rt].sign
						       | (~v[c.r.rt].known & 0x80000000)) >> imm;
				v[c.r.rd].known = (s32)v[c.r.rt].known >> imm;
			} else {
				v[c.r.rd].known = 0;
				v[c.r.rd].sign = 0;
			}
			break;

		case OP_SPECIAL_ADD:
		case OP_SPECIAL_ADDU:
			if (is_known_zero(v, c.r.rs))
				v[c.r.rd] = v[c.r.rt];
			else if (is_known_zero(v, c.r.rt))
				v[c.r.rd] = v[c.r.rs];
			else
				lightrec_propagate_addi(c.r.rs, c.r.rd, &v[c.r.rt], v);
			break;

		case OP_SPECIAL_SUB:
		case OP_SPECIAL_SUBU:
			if (c.r.rs == c.r.rt) {
				v[c.r.rd].value = 0;
				v[c.r.rd].known = 0xffffffff;
				v[c.r.rd].sign = 0;
			} else {
				lightrec_propagate_sub(c.r.rs, c.r.rt, c.r.rd, v);
			}
			break;

		case OP_SPECIAL_AND:
			v[c.r.rd].known = (v[c.r.rt].known & v[c.r.rs].known)
				| (~v[c.r.rt].value & v[c.r.rt].known)
				| (~v[c.r.rs].value & v[c.r.rs].known);
			v[c.r.rd].value = v[c.r.rt].value & v[c.r.rs].value & v[c.r.rd].known;
			v[c.r.rd].sign = v[c.r.rt].sign & v[c.r.rs].sign;
			break;

		case OP_SPECIAL_OR:
			v[c.r.rd].known = (v[c.r.rt].known & v[c.r.rs].known)
				| (v[c.r.rt].value & v[c.r.rt].known)
				| (v[c.r.rs].value & v[c.r.rs].known);
			v[c.r.rd].value = (v[c.r.rt].value | v[c.r.rs].value) & v[c.r.rd].known;
			v[c.r.rd].sign = v[c.r.rt].sign & v[c.r.rs].sign;
			break;

		case OP_SPECIAL_XOR:
			v[c.r.rd].value = v[c.r.rt].value ^ v[c.r.rs].value;
			v[c.r.rd].known = v[c.r.rt].known & v[c.r.rs].known;
			v[c.r.rd].sign = v[c.r.rt].sign & v[c.r.rs].sign;
			break;

		case OP_SPECIAL_NOR:
			v[c.r.rd].known = (v[c.r.rt].known & v[c.r.rs].known)
				| (v[c.r.rt].value & v[c.r.rt].known)
				| (v[c.r.rs].value & v[c.r.rs].known);
			v[c.r.rd].value = ~(v[c.r.rt].value | v[c.r.rs].value) & v[c.r.rd].known;
			v[c.r.rd].sign = v[c.r.rt].sign & v[c.r.rs].sign;
			break;

		case OP_SPECIAL_SLT:
		case OP_SPECIAL_SLTU:
			lightrec_propagate_slt(c.r.rs, c.r.rd,
					       c.r.op ==  OP_SPECIAL_SLT,
					       &v[c.r.rt], v);
			break;

		case OP_SPECIAL_MULT:
		case OP_SPECIAL_MULTU:
		case OP_SPECIAL_DIV:
		case OP_SPECIAL_DIVU:
			if (OPT_FLAG_MULT_DIV && c.r.rd) {
				v[c.r.rd].known = 0;
				v[c.r.rd].sign = 0;
			}
			if (OPT_FLAG_MULT_DIV && c.r.imm) {
				v[c.r.imm].known = 0;
				v[c.r.imm].sign = 0;
			}
			break;

		case OP_SPECIAL_MFLO:
		case OP_SPECIAL_MFHI:
			v[c.r.rd].known = 0;
			v[c.r.rd].sign = 0;
			break;

		case OP_SPECIAL_JALR:
			v[c.r.rd].known = 0xffffffff;
			v[c.r.rd].sign = 0;
			v[c.r.rd].value = block->pc + ((idx + 2) << 2);
			break;

		default:
			break;
		}
		break;

	case OP_META_MULT2:
	case OP_META_MULTU2:
		if (OPT_FLAG_MULT_DIV && c.r.rd) {
			if (c.r.op < 32) {
				v[c.r.rd].value = v[c.r.rs].value << c.r.op;
				v[c.r.rd].known = (v[c.r.rs].known << c.r.op)
					| (BIT(c.r.op) - 1);
				v[c.r.rd].sign = v[c.r.rs].sign << c.r.op;
			} else {
				v[c.r.rd].value = 0;
				v[c.r.rd].known = 0xffffffff;
				v[c.r.rd].sign = 0;
			}
		}

		if (OPT_FLAG_MULT_DIV && c.r.imm) {
			if (c.r.op >= 32) {
				v[c.r.imm].value = v[c.r.rs].value << (c.r.op - 32);
				v[c.r.imm].known = (v[c.r.rs].known << (c.r.op - 32))
					| (BIT(c.r.op - 32) - 1);
				v[c.r.imm].sign = v[c.r.rs].sign << (c.r.op - 32);
			} else if (c.i.op == OP_META_MULT2) {
				v[c.r.imm].value = (s32)v[c.r.rs].value >> (32 - c.r.op);
				v[c.r.imm].known = (s32)v[c.r.rs].known >> (32 - c.r.op);
				v[c.r.imm].sign = (s32)v[c.r.rs].sign >> (32 - c.r.op);
			} else {
				v[c.r.imm].value = v[c.r.rs].value >> (32 - c.r.op);
				v[c.r.imm].known = v[c.r.rs].known >> (32 - c.r.op);
				v[c.r.imm].sign = v[c.r.rs].sign >> (32 - c.r.op);
			}
		}
		break;

	case OP_REGIMM:
		break;

	case OP_ADDI:
	case OP_ADDIU:
		if (c.i.imm) {
			struct constprop_data d = {
				.value = (s32)(s16)c.i.imm,
				.known = 0xffffffff,
				.sign = 0,
			};

			lightrec_propagate_addi(c.i.rs, c.i.rt, &d, v);
		} else {
			/* immediate is zero - that's just a register copy. */
			v[c.i.rt] = v[c.i.rs];
		}
		break;

	case OP_SLTI:
	case OP_SLTIU:
		{
			struct constprop_data d = {
				.value = (s32)(s16)c.i.imm,
				.known = 0xffffffff,
				.sign = 0,
			};

			lightrec_propagate_slt(c.i.rs, c.i.rt,
					       c.i.op == OP_SLTI, &d, v);
		}
		break;

	case OP_ANDI:
		v[c.i.rt].value = v[c.i.rs].value & c.i.imm;
		v[c.i.rt].known = v[c.i.rs].known | ~c.i.imm;
		v[c.i.rt].sign = 0;
		break;

	case OP_ORI:
		v[c.i.rt].value = v[c.i.rs].value | c.i.imm;
		v[c.i.rt].known = v[c.i.rs].known | c.i.imm;
		v[c.i.rt].sign = (v[c.i.rs].sign & 0xffff) ? 0xffff0000 : v[c.i.rs].sign;
		break;

	case OP_XORI:
		v[c.i.rt].value = v[c.i.rs].value ^ c.i.imm;
		v[c.i.rt].known = v[c.i.rs].known;
		v[c.i.rt].sign = (v[c.i.rs].sign & 0xffff) ? 0xffff0000 : v[c.i.rs].sign;
		break;

	case OP_LUI:
		v[c.i.rt].value = c.i.imm << 16;
		v[c.i.rt].known = 0xffffffff;
		v[c.i.rt].sign = 0;
		break;

	case OP_CP0:
		switch (c.r.rs) {
		case OP_CP0_MFC0:
		case OP_CP0_CFC0:
			v[c.r.rt].known = 0;
			v[c.r.rt].sign = 0;
			break;
		default:
			break;
		}
		break;

	case OP_CP2:
		if (c.r.op == OP_CP2_BASIC) {
			switch (c.r.rs) {
			case OP_CP2_BASIC_MFC2:
				switch (c.r.rd) {
				case 1:
				case 3:
				case 5:
				case 8:
				case 9:
				case 10:
				case 11:
					/* Signed 16-bit */
					v[c.r.rt].known = 0;
					v[c.r.rt].sign = 0xffff8000;
					break;
				case 7:
				case 16:
				case 17:
				case 18:
				case 19:
					/* Unsigned 16-bit */
					v[c.r.rt].value = 0;
					v[c.r.rt].known = 0xffff0000;
					v[c.r.rt].sign = 0;
					break;
				default:
					/* 32-bit */
					v[c.r.rt].known = 0;
					v[c.r.rt].sign = 0;
					break;
				}
				break;
			case OP_CP2_BASIC_CFC2:
				switch (c.r.rd) {
				case 4:
				case 12:
				case 20:
				case 26:
				case 27:
				case 29:
				case 30:
					/* Signed 16-bit */
					v[c.r.rt].known = 0;
					v[c.r.rt].sign = 0xffff8000;
					break;
				default:
					/* 32-bit */
					v[c.r.rt].known = 0;
					v[c.r.rt].sign = 0;
					break;
				}
				break;
			}
		}
		break;
	case OP_LB:
		v[c.i.rt].known = 0;
		v[c.i.rt].sign = 0xffffff80;
		break;
	case OP_LH:
		v[c.i.rt].known = 0;
		v[c.i.rt].sign = 0xffff8000;
		break;
	case OP_LBU:
		v[c.i.rt].value = 0;
		v[c.i.rt].known = 0xffffff00;
		v[c.i.rt].sign = 0;
		break;
	case OP_LHU:
		v[c.i.rt].value = 0;
		v[c.i.rt].known = 0xffff0000;
		v[c.i.rt].sign = 0;
		break;
	case OP_LWL:
	case OP_LWR:
		/* LWL/LWR don't write the full register if the address is
		 * unaligned, so we only need to know the low 2 bits */
		if (v[c.i.rs].known & 0x3) {
			imm = (v[c.i.rs].value & 0x3) * 8;

			if (c.i.op == OP_LWL) {
				imm = BIT(24 - imm) - 1;
				v[c.i.rt].sign &= ~imm;
			} else {
				imm = imm ? GENMASK(31, 32 - imm) : 0;
				v[c.i.rt].sign = 0;
			}
			v[c.i.rt].known &= imm;
			break;
		}
		fallthrough;
	case OP_LW:
	case OP_META_LWU:
		v[c.i.rt].known = 0;
		v[c.i.rt].sign = 0;
		break;
	case OP_META:
		switch (c.m.op) {
		case OP_META_MOV:
			v[c.m.rd] = v[c.m.rs];
			break;

		case OP_META_EXTC:
			v[c.m.rd].value = (s32)(s8)v[c.m.rs].value;
			if (v[c.m.rs].known & BIT(7)) {
				v[c.m.rd].known = v[c.m.rs].known | 0xffffff00;
				v[c.m.rd].sign = 0;
			} else {
				v[c.m.rd].known = v[c.m.rs].known & 0x7f;
				v[c.m.rd].sign = 0xffffff80;
			}
			break;

		case OP_META_EXTS:
			v[c.m.rd].value = (s32)(s16)v[c.m.rs].value;
			if (v[c.m.rs].known & BIT(15)) {
				v[c.m.rd].known = v[c.m.rs].known | 0xffff0000;
				v[c.m.rd].sign = 0;
			} else {
				v[c.m.rd].known = v[c.m.rs].known & 0x7fff;
				v[c.m.rd].sign = 0xffff8000;
			}
			break;

		case OP_META_COM:
			v[c.m.rd].known = v[c.m.rs].known;
			v[c.m.rd].value = ~v[c.m.rs].value;
			v[c.m.rd].sign = v[c.m.rs].sign;
			break;
		default:
			break;
		}
		break;
	case OP_JAL:
		v[31].known = 0xffffffff;
		v[31].sign = 0;
		v[31].value = block->pc + ((idx + 2) << 2);
		break;

	default:
		break;
	}

	/* Reset register 0 which may have been used as a target */
	v[0].value = 0;
	v[0].sign = 0;
	v[0].known = 0xffffffff;
}

enum psx_map
lightrec_get_constprop_map(const struct lightrec_state *state,
			   const struct constprop_data *v, u8 reg, s16 imm)
{
	const struct lightrec_mem_map *map;
	unsigned int i;
	u32 min, max;

	min = get_min_value(&v[reg]) + imm;
	max = get_max_value(&v[reg]) + imm;

	/* Handle the case where max + imm overflows */
	if ((min & 0xe0000000) != (max & 0xe0000000))
		return PSX_MAP_UNKNOWN;

	pr_debug("Min: "X32_FMT" max: "X32_FMT" Known: "X32_FMT" Sign: "X32_FMT"\n",
		 min, max, v[reg].known, v[reg].sign);

	min = kunseg(min);
	max = kunseg(max);

	for (i = 0; i < state->nb_maps; i++) {
		map = &state->maps[i];

		if (min >= map->pc && min < map->pc + map->length
		    && max >= map->pc && max < map->pc + map->length)
			return (enum psx_map) i;
	}

	return PSX_MAP_UNKNOWN;
}
