/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 *
 * This code tries to make better use of pollux/arm926 store buffer
 * by fusing words instead of using strhs.
 */

.text
.align 2

.macro lhw_str rl rt
    lsl     \rl, #16
    lsr     \rl, #16
    orr     \rl, \rt, lsl #16
    str     \rl, [r0], #4
.endm

.global blit320_640
blit320_640:
    stmfd   sp!, {r4-r8,lr}
    mov     r12, #40
    bic     r1, r1, #3
0:
    ldmia   r1!, {r2-r8,lr}
    lhw_str r2, r3
    lhw_str r4, r5
    lhw_str r6, r7
    subs    r12, #1
    lhw_str r8, lr
    bgt     0b
    ldmfd   sp!, {r4-r8,pc}


.global blit320_512
blit320_512:
    stmfd   sp!, {r4-r8,lr}
    mov     r12, #32
    bic     r1, r1, #3
0:
    ldmia   r1!, {r2-r8,lr}
    lsl     r2, #16
    lsr     r2, #16
    orr     r2, r3, lsl #16
    str     r2, [r0], #4         @ 0,2
    lsr     r4, #16
    lsr     r3, #16
    orr     r3, r4, lsl #16
    str     r3, [r0], #4         @ 3,5
    lsr     r5, #16
    orr     r5, r6, lsl #16
    str     r5, [r0], #4         @ 7,8
    lsr     r8, #16
    lsr     lr, #16
    str     r7, [r0], #4         @ 10,11
    orr     r8, lr, lsl #16
    subs    r12, #1
    str     r8, [r0], #4         @ 13,15
    bgt     0b
    ldmfd   sp!, {r4-r8,pc}


.macro unaligned_str rl rt
    lsr     \rl, #16
    orr     \rl, \rt, lsl #16
    str     \rl, [r0], #4
.endm

.global blit320_368
blit320_368:
    stmfd   sp!, {r4-r8,lr}
    mov     r12, #23
    bic     r1, r1, #3
0:
    ldmia   r1!, {r2-r8,lr}
    unaligned_str r2, r3         @ 1,2
    unaligned_str r3, r4         @ 3,4
    unaligned_str r4, r5         @ 5,6
    subs    r12, #1
    stmia   r0!, {r6-r8,lr}      @ 8-15
    bgt     0b
    ldmfd   sp!, {r4-r8,pc}


@ vim:filetype=armasm
