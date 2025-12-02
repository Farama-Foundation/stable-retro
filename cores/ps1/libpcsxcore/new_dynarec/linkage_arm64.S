/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   linkage_arm.s for PCSX                                                *
 *   Copyright (C) 2009-2011 Ari64                                         *
 *   Copyright (C) 2021 notaz                                              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "arm_features.h"
#include "new_dynarec_config.h"
#include "assem_arm64.h"
#include "linkage_offsets.h"

#ifdef __MACH__
#define dynarec_local		ESYM(dynarec_local)
#define ndrc_patch_link		ESYM(ndrc_patch_link)
#define ndrc_get_addr_ht	ESYM(ndrc_get_addr_ht)
#define ndrc_get_addr_ht_param	ESYM(ndrc_get_addr_ht_param)
#define gen_interupt		ESYM(gen_interupt)
#define psxException		ESYM(psxException)
#define execI			ESYM(execI)
#endif

#if (LO_mem_wtab & 7)
#error misligned pointers
#endif

.bss
	.align	4
	.global dynarec_local
	EOBJECT(dynarec_local)
	ESIZE(dynarec_local, LO_dynarec_local_size)
dynarec_local:
	.space	LO_dynarec_local_size

#define DRC_VAR_(name, vname, size_) \
	vname = dynarec_local + LO_##name ASM_SEPARATOR \
	.globl vname; \
	EOBJECT(vname); \
	ESIZE(vname, LO_dynarec_local_size)

#define DRC_VAR(name, size_) \
	DRC_VAR_(name, ESYM(name), size_)

#DRC_VAR(next_interupt, 4)
DRC_VAR(cycle_count, 4)
DRC_VAR(last_count, 4)
#DRC_VAR(stop, 4)
DRC_VAR(address, 4)
DRC_VAR(hack_addr, 4)
DRC_VAR(psxRegs, LO_psxRegs_end - LO_psxRegs)

/* psxRegs */
#DRC_VAR(lo, 4)
#DRC_VAR(hi, 4)
DRC_VAR(reg_cop2d, 128)
DRC_VAR(reg_cop2c, 128)
#DRC_VAR(code, 4)
#DRC_VAR(cycle, 4)
#DRC_VAR(interrupt, 4)
#DRC_VAR(intCycle, 256)

DRC_VAR(rcnts, 7*4*4)
DRC_VAR(inv_code_start, 4)
DRC_VAR(inv_code_end, 4)
DRC_VAR(mem_rtab, 8)
DRC_VAR(mem_wtab, 8)
DRC_VAR(psxH_ptr, 8)
DRC_VAR(invc_ptr, 8)
DRC_VAR(zeromem_ptr, 8)
DRC_VAR(scratch_buf_ptr, 8)
DRC_VAR(ram_offset, 8)
DRC_VAR(hash_table_ptr, 8)
DRC_VAR(mini_ht, 256)


	.text
	.align	2

FUNCTION(dyna_linker):
	/* w0 = virtual target address */
	/* x1 = instruction to patch */
#if 1
	mov	w19, w0
	mov	x20, x1
	/* must not compile - that might expire the caller block */
	ldr	x0, [rFP, #LO_hash_table_ptr]
	mov	w1, w19
	mov	w2, #0 /* ndrc_compile_mode=ndrc_cm_no_compile */
	bl	ndrc_get_addr_ht_param
	cbz	x0, 0f

	ldr	w2, [x20]
	mov	x3, x0
	sbfiz	x2, x2, 2, 26
	add	x2, x2, x20
	mov	x1, x20
	mov	w0, w19
	mov	x19, x3
	bl	ndrc_patch_link
	br	x19
0:
	mov	w0, w19
#endif
	ldr     x1, [rFP, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	br	x0
	ESIZE(dyna_linker, .-dyna_linker)

	.align	2
FUNCTION(cc_interrupt):
	ldr	w0, [rFP, #LO_last_count]
	ldr	w22, [rFP, #LO_pcaddr]
	add	rCC, w0, rCC
	str	rCC, [rFP, #LO_cycle]		/* PCSX cycles */
	mov	x21, lr
1:
	add	x0, rFP, #LO_reg_cop0           /* CP0 */
	bl	gen_interupt
	mov	lr, x21
	ldr	rCC, [rFP, #LO_cycle]
	ldr	w0, [rFP, #LO_pcaddr]
	ldr	w1, [rFP, #LO_next_interupt]
	ldrb	w2, [rFP, #LO_stop]
	str	w1, [rFP, #LO_last_count]
	sub	rCC, rCC, w1
	cbnz	w2, new_dyna_leave
	cmp 	w0, w22
	bne	2f
	ret
2:
	ldr     x1, [rFP, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	br	x0
	ESIZE(cc_interrupt, .-cc_interrupt)

	.align	2
FUNCTION(jump_addrerror_ds): /* R3000E_AdEL / R3000E_AdES in w0 */
	str	w1, [rFP, #(LO_psxRegs + (34+8)*4)]  /* BadVaddr */
	mov	w1, #1
	b	call_psxException
FUNCTION(jump_addrerror):
	str	w1, [rFP, #(LO_psxRegs + (34+8)*4)]  /* BadVaddr */
	mov	w1, #0
	b	call_psxException
FUNCTION(jump_overflow_ds):
	mov	w0, #(12<<2)  /* R3000E_Ov */
	mov	w1, #1
	b	call_psxException
FUNCTION(jump_overflow):
	mov	w0, #(12<<2)
	mov	w1, #0
	b	call_psxException
FUNCTION(jump_break_ds):
	mov	w0, #(9<<2)  /* R3000E_Bp */
	mov	w1, #1
	b	call_psxException
FUNCTION(jump_break):
	mov	w0, #(9<<2)
	mov	w1, #0
	b	call_psxException
FUNCTION(jump_syscall_ds):
	mov	w0, #(8<<2)  /* R3000E_Syscall */
	mov	w1, #2
	b	call_psxException
FUNCTION(jump_syscall):
	mov	w0, #(8<<2)
	mov	w1, #0

call_psxException:
	ldr	w3, [rFP, #LO_last_count]
	str	w2, [rFP, #LO_pcaddr]
	add	rCC, w3, rCC
	str	rCC, [rFP, #LO_cycle]           /* PCSX cycles */
	add	x2, rFP, #LO_reg_cop0           /* CP0 */
	bl	psxException

	/* note: psxException might do recursive recompiler call from it's HLE code,
	 * so be ready for this */
FUNCTION(jump_to_new_pc):
	ldrb	w2, [rFP, #LO_stop]
	ldr	w1, [rFP, #LO_next_interupt]
	ldr	rCC, [rFP, #LO_cycle]
	ldr	w0, [rFP, #LO_pcaddr]
	sub	rCC, rCC, w1
	str	w1, [rFP, #LO_last_count]
	cbnz	w2, new_dyna_leave
	ldr     x1, [rFP, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	br	x0
	ESIZE(jump_to_new_pc, .-jump_to_new_pc)

	/* stack must be aligned by 16, and include space for save_regs() use */
	.align	2
FUNCTION(new_dyna_start_at):
	stp	x29, x30, [sp, #-SSP_ALL]!
	mov	rFP, x0
	b	new_dyna_start_at_e

FUNCTION(new_dyna_start):
	stp	x29, x30, [sp, #-SSP_ALL]!
	mov	rFP, x0
	ldr	w0,  [rFP, #LO_pcaddr]
	ldr     x1,  [rFP, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	mov	x1,  x0
new_dyna_start_at_e:
	ldr	w3,  [rFP, #LO_next_interupt]
	ldr	w2,  [rFP, #LO_cycle]
	stp	x19, x20, [sp, #16*1]
	stp	x21, x22, [sp, #16*2]
	stp	x23, x24, [sp, #16*3]
	stp	x25, x26, [sp, #16*4]
	stp	x27, x28, [sp, #16*5]
	str	w3,  [rFP, #LO_last_count]
	sub	rCC, w2, w3
	br	x1
	ESIZE(new_dyna_start, .-new_dyna_start)

	.align	2
FUNCTION(new_dyna_leave):
	ldr	w0,  [rFP, #LO_last_count]
	add	rCC, rCC, w0
	str	rCC, [rFP, #LO_cycle]
	ldp	x19, x20, [sp, #16*1]
	ldp	x21, x22, [sp, #16*2]
	ldp	x23, x24, [sp, #16*3]
	ldp	x25, x26, [sp, #16*4]
	ldp	x27, x28, [sp, #16*5]
	ldp	x29, x30, [sp], #SSP_ALL
	ret
	ESIZE(new_dyna_leave, .-new_dyna_leave)

/* --------------------------------------- */

.align	2

.macro memhandler_pre
	/* w0 = addr/data, x1 = rhandler, w2 = cycles, x3 = whandler */
	ldr	w4, [rFP, #LO_last_count]
	add	w4, w4, w2
	str	w4, [rFP, #LO_cycle]
.endm

.macro memhandler_post
	/* w2 = cycles_out, x3 = tmp */
	ldr	w3, [rFP, #LO_next_interupt]
	ldr	w2, [rFP, #LO_cycle]        // memhandlers can modify cc, like dma
	str	w3, [rFP, #LO_last_count]
	sub	w2, w2, w3
.endm

FUNCTION(do_memhandler_pre):
	memhandler_pre
	ret

FUNCTION(do_memhandler_post):
	memhandler_post
	ret

.macro pcsx_read_mem readop tab_shift
	/* w0 = address, x1 = handler_tab, w2 = cycles */
	ubfm	w4, w0, #\tab_shift, #11
	ldr	x3, [x1, w4, uxtw #3]
	adds	x3, x3, x3
	bcs	0f
	\readop	w0, [x3, w4, uxtw #\tab_shift]
	ret
0:
	stp	xzr, x30, [sp, #-16]!
	memhandler_pre
	blr	x3
.endm

FUNCTION(jump_handler_read8):
	add     x1, x1, #0x1000/4*8 + 0x1000/2*8  /* shift to r8 part */
	pcsx_read_mem ldrb, 0
	ldp	xzr, x30, [sp], #16
	ret

FUNCTION(jump_handler_read16):
	add     x1, x1, #0x1000/4*8               /* shift to r16 part */
	pcsx_read_mem ldrh, 1
	ldp	xzr, x30, [sp], #16
	ret

FUNCTION(jump_handler_read32):
	pcsx_read_mem ldr, 2
	/* memhandler_post */
	ldp	xzr, x30, [sp], #16
	ret

.macro pcsx_write_mem wrtop movop tab_shift
	/* w0 = address, w1 = data, w2 = cycles, x3 = handler_tab */
	ubfm	w4, w0, #\tab_shift, #11
	ldr	x3, [x3, w4, uxtw #3]
	adds	x3, x3, x3
	bcs	0f
	\wrtop	w1, [x3, w4, uxtw #\tab_shift]
	ret
0:
	stp	xzr, x30, [sp, #-16]!
	str	w0, [rFP, #LO_address]    /* some handlers still need it... */
	\movop	w0, w1
	memhandler_pre
	blr	x3
.endm

FUNCTION(jump_handler_write8):
	add     x3, x3, #0x1000/4*8 + 0x1000/2*8  /* shift to r8 part */
	pcsx_write_mem strb, uxtb, 0
	b	handler_write_end

FUNCTION(jump_handler_write16):
	add     x3, x3, #0x1000/4*8               /* shift to r16 part */
	pcsx_write_mem strh, uxth, 1
	b	handler_write_end

FUNCTION(jump_handler_write32):
	pcsx_write_mem str, mov, 2

handler_write_end:
	memhandler_post
	ldp	xzr, x30, [sp], #16
	ret

FUNCTION(jump_handle_swl):
	/* w0 = address, w1 = data, w2 = cycles */
	ldr	x3, [rFP, #LO_mem_wtab]
	orr	w4, wzr, w0, lsr #12
	ldr	x3, [x3, w4, uxtw #3]
	adds	x3, x3, x3
	bcs	jump_handle_swx_interp
	add	x3, x0, x3
	mov	w0, w2
	tbz	x3, #1, 10f	// & 2
	tbz	x3, #0, 2f	// & 1
3:
	stur	w1, [x3, #-3]
	ret
2:
	lsr	w2, w1, #8
	lsr	w1, w1, #24
	sturh	w2, [x3, #-2]
	strb	w1, [x3]
	ret
10:
	tbz	x3, #0, 0f	// & 1
1:
	lsr	w1, w1, #16
	sturh	w1, [x3, #-1]
	ret
0:
	lsr	w2, w1, #24
	strb	w2, [x3]
	ret

FUNCTION(jump_handle_swr):
	/* w0 = address, w1 = data, w2 = cycles */
	ldr	x3, [rFP, #LO_mem_wtab]
	orr	w4, wzr, w0, lsr #12
	ldr	x3, [x3, w4, uxtw #3]
	adds	x3, x3, x3
	bcs	jump_handle_swx_interp
	add	x3, x0, x3
	mov	w0, w2
	tbz	x3, #1, 10f	// & 2
	tbz	x3, #0, 2f	// & 1
3:
	strb	w1, [x3]
	ret
2:
	strh	w1, [x3]
	ret
10:
	tbz	x3, #0, 0f	// & 1
1:
	lsr	w2, w1, #8
	strb	w1, [x3]
	sturh	w2, [x3, #1]
	ret
0:
	str	w1, [x3]
	ret

jump_handle_swx_interp: /* almost never happens */
	ldr	w3, [rFP, #LO_last_count]
	add	x0, rFP, #LO_psxRegs
	add	w2, w3, w2
	str	w2, [rFP, #LO_cycle]           /* PCSX cycles */
	bl	execI
	b	jump_to_new_pc

#ifdef DRC_DBG
#undef do_insn_cmp
FUNCTION(do_insn_cmp_arm64):
	stp	x2,  x3,  [sp, #(SSP_CALLEE_REGS + 2*8)]
	stp	x4,  x5,  [sp, #(SSP_CALLEE_REGS + 4*8)]
	stp	x6,  x7,  [sp, #(SSP_CALLEE_REGS + 6*8)]
	stp	x8,  x9,  [sp, #(SSP_CALLEE_REGS + 8*8)]
	stp	x10, x11, [sp, #(SSP_CALLEE_REGS + 10*8)]
	stp	x12, x13, [sp, #(SSP_CALLEE_REGS + 12*8)]
	stp	x14, x15, [sp, #(SSP_CALLEE_REGS + 14*8)]
	stp	x16, x17, [sp, #(SSP_CALLEE_REGS + 16*8)]
	stp	x18, x30, [sp, #(SSP_CALLEE_REGS + 18*8)]
	bl	do_insn_cmp
	ldp	x2,  x3,  [sp, #(SSP_CALLEE_REGS + 2*8)]
	ldp	x4,  x5,  [sp, #(SSP_CALLEE_REGS + 4*8)]
	ldp	x6,  x7,  [sp, #(SSP_CALLEE_REGS + 6*8)]
	ldp	x8,  x9,  [sp, #(SSP_CALLEE_REGS + 8*8)]
	ldp	x10, x11, [sp, #(SSP_CALLEE_REGS + 10*8)]
	ldp	x12, x13, [sp, #(SSP_CALLEE_REGS + 12*8)]
	ldp	x14, x15, [sp, #(SSP_CALLEE_REGS + 14*8)]
	ldp	x16, x17, [sp, #(SSP_CALLEE_REGS + 16*8)]
	ldp	x18, x30, [sp, #(SSP_CALLEE_REGS + 18*8)]
	ret
#endif
