/*
 * (C) Gražvydas "notaz" Ignotas, 2013
 *
 * This work is licensed under the terms of GNU GPL version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "arm_features.h"

.text
.align 2

@ lr=0x001f001f
@ trashes r11, r12
.macro bgr555_to_rgb565_one rn
    and      r11, lr, \rn
    and      r12, lr, \rn, lsr #5
    and      \rn, lr, \rn, lsr #10
    orr      r12, r11, lsl #5
    orr      \rn, r12, lsl #6
.endm

.macro bgr555_to_rgb565_one_i rn1 rn2
    and      r12, lr, \rn1, lsr #5
    and      \rn1,lr, \rn1, lsr #10
    orr      r12, r11, lsl #5
    and      r11, lr, \rn2
    orr      \rn1,r12, lsl #6
.endm

.macro pld_ reg offs=#0
#ifdef HAVE_ARMV6
    pld      [\reg, \offs]
#endif
.endm

FUNCTION(bgr555_to_rgb565): @ void *dst, const void *src, int bytes
    push     {r4-r11,lr}
    mov      lr, #0x001f
    subs     r2, #4*8
    orr      lr, lr, lsl #16
    blt      1f

    @ src can be unaligned, but that's very rare, so just force it.
    @ The manual says unaligned ldm should fault, and it does on
    @ cortex-a78's 32bit mode, but curiously on cortex-a8 it just
    @ works and loads the data correctly.
    bic      r1, r1, #3

0:
    ldmia    r1!, {r3-r10}
    subs     r2, #4*8
    bic      r12, r1, #0x1f
    pld_     r12, #32*1
    and      r11, lr, r3
    bgr555_to_rgb565_one_i r3 r4
    bgr555_to_rgb565_one_i r4 r5
    bgr555_to_rgb565_one_i r5 r6
    bgr555_to_rgb565_one_i r6 r7
    bgr555_to_rgb565_one_i r7 r8
    bgr555_to_rgb565_one_i r8 r9
    bgr555_to_rgb565_one_i r9 r10
    bgr555_to_rgb565_one_i r10 r10
    stmia    r0!, {r3-r10}
    bge      0b

1:
    adds     r2, #4*8
    popeq    {r4-r11,pc}

2:
    ldr      r3, [r1], #4
    subs     r2, #4
    bgr555_to_rgb565_one r3
    str      r3, [r0], #4
    bgt      2b

    pop      {r4-r11,pc}


#ifdef HAVE_ARMV6 /* v6-only due to potential misaligned reads */

# r1b0g0r0 g2r2b1g1 b3g3r3b2
FUNCTION(bgr888_to_rgb565):
    pld      [r1]
    push     {r4-r10,lr}

    mov      r10, #0x001f          @ b mask
    mov      r12, #0x07e0          @ g mask
    mov      lr,  #0xf800          @ r mask

0:
    ldr      r3, [r1], #4          @ may be unaligned
    ldr      r4, [r1], #4
    ldr      r5, [r1], #4
    pld      [r1, #32*1]
    and      r6, r10,r3, lsr #16+3 @ b0
    and      r7, r12,r3, lsr #5    @ g0
    and      r8, lr, r3, lsl #8    @ r0
    and      r9, lr, r3, lsr #16   @ r1
    orr      r6, r6, r7
    orr      r6, r6, r8            @ r0g0b0

    and      r7, r12,r4, lsl #3    @ g1
    and      r8, r10,r4, lsr #11   @ b1
    orr      r9, r9, r7
    orr      r9, r9, r8            @ r1g1b1
    and      r7, lr, r4, lsr #8    @ r2
    and      r8, r12,r4, lsr #21   @ g2
    pkhbt    r9, r6, r9, lsl #16
    str      r9, [r0], #4

    and      r6, r10,r5, lsr #3    @ b2
    orr      r7, r7, r8
    orr      r6, r6, r7            @ r2g2b2
    and      r7, lr, r5            @ r3
    and      r8, r12,r5, lsr #13   @ g3
    orr      r7, r7, r5, lsr #27   @ r3b3
    orr      r7, r7, r8            @ r3g3b3
    pkhbt    r7, r6, r7, lsl #16
    str      r7, [r0], #4
    subs     r2, r2, #12
    bgt      0b

    pop      {r4-r10,pc}

#endif /* HAVE_ARMV6 */
