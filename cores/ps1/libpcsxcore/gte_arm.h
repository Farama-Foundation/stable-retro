/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2016  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#ifndef __GTE_ARM_H__
#define __GTE_ARM_H__

void gteRTPS_nf_arm(void *cp2_regs, int opcode);
void gteRTPT_nf_arm(void *cp2_regs, int opcode);
void gteNCLIP_arm(void *cp2_regs, int opcode);

// decomposed ops, nonstd calling convention
void gteMVMVA_part_arm(void *cp2_regs, int is_shift12);
void gteMVMVA_part_nf_arm(void *cp2_regs, int is_shift12);
void gteMVMVA_part_cv3sh12_arm(void *cp2_regs);

void gteMACtoIR_lm0(void *cp2_regs);
void gteMACtoIR_lm1(void *cp2_regs);
void gteMACtoIR_lm0_nf(void *cp2_regs);
void gteMACtoIR_lm1_nf(void *cp2_regs);

#endif /* __GTE_ARM_H__ */
