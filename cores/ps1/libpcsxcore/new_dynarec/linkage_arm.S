/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   linkage_arm.s for PCSX                                                *
 *   Copyright (C) 2009-2011 Ari64                                         *
 *   Copyright (C) 2010-2013 Gražvydas "notaz" Ignotas                     *
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
#include "linkage_offsets.h"


#ifdef __MACH__
#define dynarec_local		ESYM(dynarec_local)
#define ndrc_patch_link		ESYM(ndrc_patch_link)
#define ndrc_get_addr_ht	ESYM(ndrc_get_addr_ht)
#define ndrc_get_addr_ht_param	ESYM(ndrc_get_addr_ht_param)
#define ndrc_write_invalidate_one ESYM(ndrc_write_invalidate_one)
#define gen_interupt		ESYM(gen_interupt)
#define psxException		ESYM(psxException)
#define execI			ESYM(execI)
#endif

/* make mini_ht reachable with a single armv4 insn */
#if (LO_mini_ht & ~0xff0)
#error misligned mini_ht
#endif

	.bss
	.align	4
	.global dynarec_local
	.type	dynarec_local, %object
	.size	dynarec_local, LO_dynarec_local_size
dynarec_local:
	.space	LO_dynarec_local_size

#define DRC_VAR_(name, vname, size_) \
	vname = dynarec_local + LO_##name; \
	.global vname; \
	.type	vname, %object; \
	.size	vname, size_

#define DRC_VAR(name, size_) \
	DRC_VAR_(name, ESYM(name), size_)

@DRC_VAR(next_interupt, 4)
DRC_VAR(cycle_count, 4)
DRC_VAR(last_count, 4)
@DRC_VAR(stop, 4)
DRC_VAR(address, 4)
DRC_VAR(hack_addr, 4)
DRC_VAR(psxRegs, LO_psxRegs_end - LO_psxRegs)

/* psxRegs */
@DRC_VAR(lo, 4)
@DRC_VAR(hi, 4)
DRC_VAR(reg_cop2d, 128)
DRC_VAR(reg_cop2c, 128)
@DRC_VAR(code, 4)
@DRC_VAR(cycle, 4)
@DRC_VAR(interrupt, 4)
@DRC_VAR(intCycle, 256)

DRC_VAR(rcnts, 7*4*4)
DRC_VAR(inv_code_start, 4)
DRC_VAR(inv_code_end, 4)
DRC_VAR(mem_rtab, 4)
DRC_VAR(mem_wtab, 4)
DRC_VAR(psxH_ptr, 4)
DRC_VAR(zeromem_ptr, 4)
DRC_VAR(invc_ptr, 4)
DRC_VAR(scratch_buf_ptr, 4)
DRC_VAR(ram_offset, 4)
DRC_VAR(hash_table_ptr, 4)
DRC_VAR(mini_ht, 256)


	.syntax unified
	.text
	.align	2

#ifndef HAVE_ARMV5
.macro blx rd
	mov	lr, pc
	bx	\rd
.endm
#endif

.macro load_varadr reg var
#if defined(HAVE_ARMV7) && defined(TEXRELS_FORBIDDEN)
	movw	\reg, #:lower16:(\var-(1678f+8))
	movt	\reg, #:upper16:(\var-(1678f+8))
1678:
	add	\reg, pc
#elif defined(HAVE_ARMV7) && !defined(__PIC__)
	movw	\reg, #:lower16:\var
	movt	\reg, #:upper16:\var
#else
	ldr	\reg, =\var
#endif
.endm

.macro load_varadr_ext reg var
#if defined(HAVE_ARMV7) && defined(TEXRELS_FORBIDDEN)
	movw	\reg, #:lower16:(ptr_\var-(1678f+8))
	movt	\reg, #:upper16:(ptr_\var-(1678f+8))
1678:
	ldr	\reg, [pc, \reg]
#else
	load_varadr \reg \var
#endif
.endm

.macro mov_16 reg imm
#ifdef HAVE_ARMV7
	movw	\reg, #\imm
#else
	mov	\reg, #(\imm & 0x00ff)
	orr	\reg, #(\imm & 0xff00)
#endif
.endm

.macro mov_24 reg imm
#ifdef HAVE_ARMV7
	movw	\reg, #(\imm & 0xffff)
	movt	\reg, #(\imm >> 16)
#else
	mov	\reg, #(\imm & 0x0000ff)
	orr	\reg, #(\imm & 0x00ff00)
	orr	\reg, #(\imm & 0xff0000)
#endif
.endm

FUNCTION(dyna_linker):
	/* r0 = virtual target address */
	/* r1 = pointer to an instruction to patch */
#if 1
	ldr	r7, [r1]
	mov	r4, r0
	add	r6, r7, #2
	mov	r5, r1
	lsl	r6, r6, #8
	/* must not compile - that might expire the caller block */
	ldr	r0, [fp, #LO_hash_table_ptr]
	mov	r1, r4
	mov	r2, #0 /* ndrc_compile_mode=ndrc_cm_no_compile */
	bl	ndrc_get_addr_ht_param

	movs	r8, r0
	beq	0f
	add	r6, r5, r6, asr #6  /* old target */
	teq	r0, r6
	bxeq	r0     /* Stale i-cache */
	mov	r0, r4
	mov	r1, r5
	mov	r2, r6
	mov	r3, r8
	bl	ndrc_patch_link
	bx	r8
0:
	mov	r0, r4
#endif
	ldr	r1, [fp, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	bx	r0
	.size	dyna_linker, .-dyna_linker

	.align	2
FUNCTION(jump_vaddr_r1):
	mov	r0, r1
	b	jump_vaddr_r0
	.size	jump_vaddr_r1, .-jump_vaddr_r1
FUNCTION(jump_vaddr_r2):
	mov	r0, r2
	b	jump_vaddr_r0
	.size	jump_vaddr_r2, .-jump_vaddr_r2
FUNCTION(jump_vaddr_r3):
	mov	r0, r3
	b	jump_vaddr_r0
	.size	jump_vaddr_r3, .-jump_vaddr_r3
FUNCTION(jump_vaddr_r4):
	mov	r0, r4
	b	jump_vaddr_r0
	.size	jump_vaddr_r4, .-jump_vaddr_r4
FUNCTION(jump_vaddr_r5):
	mov	r0, r5
	b	jump_vaddr_r0
	.size	jump_vaddr_r5, .-jump_vaddr_r5
FUNCTION(jump_vaddr_r6):
	mov	r0, r6
	b	jump_vaddr_r0
	.size	jump_vaddr_r6, .-jump_vaddr_r6
FUNCTION(jump_vaddr_r8):
	mov	r0, r8
	b	jump_vaddr_r0
	.size	jump_vaddr_r8, .-jump_vaddr_r8
FUNCTION(jump_vaddr_r9):
	mov	r0, r9
	b	jump_vaddr_r0
	.size	jump_vaddr_r9, .-jump_vaddr_r9
FUNCTION(jump_vaddr_r10):
	mov	r0, r10
	b	jump_vaddr_r0
	.size	jump_vaddr_r10, .-jump_vaddr_r10
FUNCTION(jump_vaddr_r12):
	mov	r0, r12
	b	jump_vaddr_r0
	.size	jump_vaddr_r12, .-jump_vaddr_r12
FUNCTION(jump_vaddr_r7):
	add	r0, r7, #0
	.size	jump_vaddr_r7, .-jump_vaddr_r7
FUNCTION(jump_vaddr_r0):
	ldr     r1, [fp, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	bx	r0
	.size	jump_vaddr_r0, .-jump_vaddr_r0

	.align	2
FUNCTION(cc_interrupt):
	ldr	r0, [fp, #LO_last_count]
	ldr	r9, [fp, #LO_pcaddr]
	add	r1, r0, r10
	str	r1, [fp, #LO_cycle]		/* PCSX cycles */
	mov	r10, lr

	add	r0, fp, #LO_reg_cop0            /* CP0 */
	bl	gen_interupt
	mov	lr, r10
	ldr	r10, [fp, #LO_cycle]
	ldr	r0, [fp, #LO_pcaddr]
	ldr	r1, [fp, #LO_next_interupt]
	ldrb	r2, [fp, #LO_stop]
	str	r1, [fp, #LO_last_count]
	sub	r10, r10, r1
	tst	r2, r2
	ldmfdne	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, pc}
	cmp	r0, r9
	bxeq	lr
	ldr     r1, [fp, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	bx	r0
	.size	cc_interrupt, .-cc_interrupt

	.align	2
FUNCTION(jump_addrerror_ds): /* R3000E_AdEL / R3000E_AdES in r0 */
	str	r1, [fp, #(LO_psxRegs + (34+8)*4)]  /* BadVaddr */
	mov	r1, #1
	b	call_psxException
FUNCTION(jump_addrerror):
	str	r1, [fp, #(LO_psxRegs + (34+8)*4)]  /* BadVaddr */
	mov	r1, #0
	b	call_psxException
FUNCTION(jump_overflow_ds):
	mov	r0, #(12<<2)  /* R3000E_Ov */
	mov	r1, #1
	b	call_psxException
FUNCTION(jump_overflow):
	mov	r0, #(12<<2)
	mov	r1, #0
	b	call_psxException
FUNCTION(jump_break_ds):
	mov	r0, #(9<<2)  /* R3000E_Bp */
	mov	r1, #1
	b	call_psxException
FUNCTION(jump_break):
	mov	r0, #(9<<2)
	mov	r1, #0
	b	call_psxException
FUNCTION(jump_syscall_ds):
	mov	r0, #(8<<2)  /* R3000E_Syscall */
	mov	r1, #2
	b	call_psxException
FUNCTION(jump_syscall):
	mov	r0, #(8<<2)
	mov	r1, #0

call_psxException:
	ldr	r3, [fp, #LO_last_count]
	str	r2, [fp, #LO_pcaddr]
	add	r10, r3, r10
	str	r10, [fp, #LO_cycle]            /* PCSX cycles */
	add	r2, fp, #LO_reg_cop0            /* CP0 */
	bl	psxException

	/* note: psxException might do recursive recompiler call from it's HLE code,
	 * so be ready for this */
FUNCTION(jump_to_new_pc):
	ldrb	r2, [fp, #LO_stop]
	ldr	r1, [fp, #LO_next_interupt]
	ldr	r10, [fp, #LO_cycle]
	ldr	r0, [fp, #LO_pcaddr]
	tst	r2, r2
	str	r1, [fp, #LO_last_count]
	sub	r10, r10, r1
	bne	new_dyna_leave
	ldr     r1, [fp, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
	bx	r0
	.size	jump_to_new_pc, .-jump_to_new_pc

	.align	2
FUNCTION(new_dyna_leave):
	ldr	r0, [fp, #LO_last_count]
	add	r10, r0, r10
	str	r10, [fp, #LO_cycle]
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, pc}
	.size	new_dyna_leave, .-new_dyna_leave

	.align	2
FUNCTION(invalidate_addr_r0):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	b	invalidate_addr_call
	.size	invalidate_addr_r0, .-invalidate_addr_r0
	.align	2
FUNCTION(invalidate_addr_r1):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r1
	b	invalidate_addr_call
	.size	invalidate_addr_r1, .-invalidate_addr_r1
	.align	2
FUNCTION(invalidate_addr_r2):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r2
	b	invalidate_addr_call
	.size	invalidate_addr_r2, .-invalidate_addr_r2
	.align	2
FUNCTION(invalidate_addr_r3):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r3
	b	invalidate_addr_call
	.size	invalidate_addr_r3, .-invalidate_addr_r3
	.align	2
FUNCTION(invalidate_addr_r4):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r4
	b	invalidate_addr_call
	.size	invalidate_addr_r4, .-invalidate_addr_r4
	.align	2
FUNCTION(invalidate_addr_r5):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r5
	b	invalidate_addr_call
	.size	invalidate_addr_r5, .-invalidate_addr_r5
	.align	2
FUNCTION(invalidate_addr_r6):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r6
	b	invalidate_addr_call
	.size	invalidate_addr_r6, .-invalidate_addr_r6
	.align	2
FUNCTION(invalidate_addr_r7):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r7
	b	invalidate_addr_call
	.size	invalidate_addr_r7, .-invalidate_addr_r7
	.align	2
FUNCTION(invalidate_addr_r8):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r8
	b	invalidate_addr_call
	.size	invalidate_addr_r8, .-invalidate_addr_r8
	.align	2
FUNCTION(invalidate_addr_r9):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r9
	b	invalidate_addr_call
	.size	invalidate_addr_r9, .-invalidate_addr_r9
	.align	2
FUNCTION(invalidate_addr_r10):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r10
	b	invalidate_addr_call
	.size	invalidate_addr_r10, .-invalidate_addr_r10
	.align	2
FUNCTION(invalidate_addr_r12):
	stmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, lr}
	mov	r0, r12
	.size	invalidate_addr_r12, .-invalidate_addr_r12
	.align	2
invalidate_addr_call:
	ldr	r12, [fp, #LO_inv_code_start]
	ldr	lr, [fp, #LO_inv_code_end]
	cmp	r0, r12
	cmpcs   lr, r0
	blcc	ndrc_write_invalidate_one
	ldmia	fp, {r0, r1, r2, r3, EXTRA_UNSAVED_REGS r12, pc}
	.size	invalidate_addr_call, .-invalidate_addr_call

	.align	2
FUNCTION(new_dyna_start_at):
	/* ip is stored to conform EABI alignment */
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	mov	fp, r0 /* dynarec_local */
	mov     r0, r1
	b       new_dyna_start_at_e

FUNCTION(new_dyna_start):
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}
	mov	fp, r0 /* dynarec_local */
	ldr	r0, [fp, #LO_pcaddr]
	ldr     r1, [fp, #LO_hash_table_ptr]
	bl	ndrc_get_addr_ht
new_dyna_start_at_e:
	ldr	r1, [fp, #LO_next_interupt]
	ldr	r10, [fp, #LO_cycle]
	str	r1, [fp, #LO_last_count]
	sub	r10, r10, r1
	bx	r0
	.size	new_dyna_start, .-new_dyna_start

/* --------------------------------------- */

.macro memhandler_post
	/* r2 = cycles_out, r3 = tmp */
	ldr	r3, [fp, #LO_next_interupt]
	ldr	r2, [fp, #LO_cycle]        @ memhandlers can modify cc, like dma
	str	r3, [fp, #LO_last_count]
	sub	r2, r2, r3
.endm

.align 2

.macro pcsx_read_mem_part readop tab_shift
	/* r0 = address, r1 = handler_tab, r2 = cycles */
	lsl	r3, r0, #20
	lsr	r3, #(20+\tab_shift)
	ldr	r12, [fp, #LO_last_count]
	ldr	r1, [r1, r3, lsl #2]
	add	r12, r2, r12
	lsls	r1, #1
.if \tab_shift == 1
	lsl	r3, #1
	\readop	r0, [r1, r3]
.else
	\readop	r0, [r1, r3, lsl #\tab_shift]
.endif
	bxcc	lr
	mov	r2, r12
	str	r12, [fp, #LO_cycle]
.endm

FUNCTION(jump_handler_read8):
	add     r1, #0x1000/4*4 + 0x1000/2*4 @ shift to r8 part
	pcsx_read_mem_part ldrbcc, 0
	bx	r1                           @ addr, unused, cycles

FUNCTION(jump_handler_read16):
	add     r1, #0x1000/4*4              @ shift to r16 part
	pcsx_read_mem_part ldrhcc, 1
	bx	r1                           @ addr, unused, cycles

FUNCTION(jump_handler_read32):
	pcsx_read_mem_part ldrcc, 2
	bx	r1                           @ addr, unused, cycles
#if 0
	str	lr, [fp, #LO_saved_lr]
	blx	r1
	ldr	lr, [fp, #LO_saved_lr]
	memhandler_post
	bx	lr
#endif

.macro pcsx_write_mem wrtop tab_shift
	/* r0 = address, r1 = data, r2 = cycles, r3 = handler_tab */
	lsl	r12,r0, #20
	lsr	r12, #(20+\tab_shift)
	ldr	r3, [r3, r12, lsl #2]
	str	r0, [fp, #LO_address]      @ some handlers still need it..
	lsls	r3, #1
.if \tab_shift == 1
	lsl	r12, #1
	\wrtop	r1, [r3, r12]
.else
	\wrtop	r1, [r3, r12, lsl #\tab_shift]
.endif
	bxcc	lr
	ldr	r12, [fp, #LO_last_count]
	mov     r0, r1
	add	r2, r2, r12
	str	r2, [fp, #LO_cycle]

	str	lr, [fp, #LO_saved_lr]
	blx	r3
	ldr	lr, [fp, #LO_saved_lr]

	memhandler_post
	bx	lr
.endm

FUNCTION(jump_handler_write8):
	add     r3, #0x1000/4*4 + 0x1000/2*4 @ shift to r8 part
	pcsx_write_mem strbcc, 0

FUNCTION(jump_handler_write16):
	add     r3, #0x1000/4*4              @ shift to r16 part
	pcsx_write_mem strhcc, 1

FUNCTION(jump_handler_write32):
	pcsx_write_mem strcc, 2

FUNCTION(jump_handler_write_h):
	/* r0 = address, r1 = data, r2 = cycles, r3 = handler */
	ldr	r12, [fp, #LO_last_count]
	str	r0, [fp, #LO_address]      @ some handlers still need it..
	add	r2, r2, r12
	mov     r0, r1
	str	r2, [fp, #LO_cycle]

	str	lr, [fp, #LO_saved_lr]
	blx	r3
	ldr	lr, [fp, #LO_saved_lr]

	memhandler_post
	bx	lr

FUNCTION(jump_handle_swl):
	/* r0 = address, r1 = data, r2 = cycles */
	ldr	r3, [fp, #LO_mem_wtab]
	mov	r12,r0,lsr #12
	ldr	r3, [r3, r12, lsl #2]
	lsls	r3, #1
	bcs	jump_handle_swx_interp
	add	r3, r0, r3
	mov	r0, r2
	tst	r3, #2
	beq	101f
	tst	r3, #1
	beq	2f
3:
	str	r1, [r3, #-3]
	bx	lr
2:
	lsr	r2, r1, #8
	lsr	r1, #24
	strh	r2, [r3, #-2]
	strb	r1, [r3]
	bx	lr
101:
	tst	r3, #1
	lsrne	r1, #16		@ 1
	lsreq	r12, r1, #24	@ 0
	strhne	r1, [r3, #-1]
	strbeq	r12, [r3]
	bx	lr

FUNCTION(jump_handle_swr):
	/* r0 = address, r1 = data, r2 = cycles */
	ldr	r3, [fp, #LO_mem_wtab]
	mov	r12,r0,lsr #12
	ldr	r3, [r3, r12, lsl #2]
	lsls	r3, #1
	bcs	jump_handle_swx_interp
	add	r3, r0, r3
	and	r12,r3, #3
	mov	r0, r2
	cmp	r12,#2
	strbgt	r1, [r3]	@ 3
	strheq	r1, [r3]	@ 2
	cmp	r12,#1
	strlt	r1, [r3]	@ 0
	bxne	lr
	lsr	r2, r1, #8	@ 1
	strb	r1, [r3]
	strh	r2, [r3, #1]
	bx	lr

jump_handle_swx_interp: /* almost never happens */
	ldr	r3, [fp, #LO_last_count]
	add	r0, fp, #LO_psxRegs
	add	r2, r3, r2
	str	r2, [fp, #LO_cycle]           /* PCSX cycles */
	bl	execI
	b	jump_to_new_pc

.macro rcntx_read_mode0 num
	/* r0 = address, r2 = cycles */
	ldr	r3, [fp, #LO_rcnts+6*4+7*4*\num] @ cycleStart
	mov	r0, r2, lsl #16
	sub	r0, r0, r3, lsl #16
	lsr	r0, #16
	bx	lr
.endm

FUNCTION(rcnt0_read_count_m0):
	rcntx_read_mode0 0

FUNCTION(rcnt1_read_count_m0):
	rcntx_read_mode0 1

FUNCTION(rcnt2_read_count_m0):
	rcntx_read_mode0 2

FUNCTION(rcnt0_read_count_m1):
	/* r0 = address, r2 = cycles */
	ldr	r3, [fp, #LO_rcnts+6*4+7*4*0] @ cycleStart
	mov_16	r1, 0x3334
	sub	r2, r2, r3
	mul	r0, r1, r2		@ /= 5
	lsr	r0, #16
	bx	lr

FUNCTION(rcnt1_read_count_m1):
	/* r0 = address, r2 = cycles */
	ldr	r3, [fp, #LO_rcnts+6*4+7*4*1]
	mov_24	r1, 0x1e6cde
	sub	r2, r2, r3
	umull	r3, r0, r1, r2		@ ~ /= hsync_cycles, max ~0x1e6cdd
	bx	lr

FUNCTION(rcnt2_read_count_m1):
	/* r0 = address, r2 = cycles */
	ldr	r3, [fp, #LO_rcnts+6*4+7*4*2]
	mov	r0, r2, lsl #16-3
	sub	r0, r0, r3, lsl #16-3
	lsr	r0, #16			@ /= 8
	bx	lr

#ifdef HAVE_ARMV6

FUNCTION(get_reg):
	ldr	r12, [r0]
	and	r1, r1, #0xff
	ldr	r2, [r0, #4]
	orr	r1, r1, r1, lsl #8
	ldr	r3, [r0, #8]
	orr	r1, r1, r1, lsl #16   @ searched char in every byte
	ldrb    r0, [r0, #12]         @ last byte
	eor	r12, r12, r1
	eor	r2, r2, r1
	eor	r3, r3, r1
	cmp     r0, r1, lsr #24
	mov	r0, #12
	mvn	r1, #0                @ r1=~0
	bxeq	lr
	orr	r3, r3, #0xff000000   @ EXCLUDE_REG
	uadd8	r0, r12, r1           @ add and set GE bits when not 0 (match)
	mov	r12, #0
	sel	r0, r12, r1           @ 0 if no match, else ff in some byte
	uadd8	r2, r2, r1
	sel	r2, r12, r1
	uadd8	r3, r3, r1
	sel	r3, r12, r1
	mov	r12, #3
	clz     r0, r0                @ 0, 8, 16, 24 or 32
	clz     r2, r2
	clz     r3, r3
	sub	r0, r12, r0, lsr #3   @ 3, 2, 1, 0 or -1
	sub	r2, r12, r2, lsr #3
	sub	r3, r12, r3, lsr #3
	orr	r2, r2, #4
	orr	r3, r3, #8
	and	r0, r0, r2
	and	r0, r0, r3
	bx	lr

#endif /* HAVE_ARMV6 */

@ vim:filetype=armasm
