/*
 * (C) Gražvydas "notaz" Ignotas, 2010
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include "arm_features.h"

/* sanity check */
#ifndef __ARM_NEON__
#error Compiling NEON code, but appropriate preprocessor flag is missing
#error This usually means -mfpu=neon or -mfloat-abi= is not correctly specified
#endif

.text
.align 2

FUNCTION(bgr555_to_rgb565): @ dst, src, bytes
    pld         [r1]
    mov         r3, #0x07c0
    vdup.16     q15, r3
    tst         r0, #8
    beq         0f
    @ align the dst
    vld1.16     {d0}, [r1]!
    sub         r2, r2, #8
    vshl.u16    d0, d0, #1
    vshl.u16    d1, d0, #10
    vsri.u16    d1, d0, #11
    vbit        d1, d0, d30
    vst1.16     {d1}, [r0]!
0:
    subs        r2, r2, #64
    blt         btr16_end64
0:
    pld         [r1, #64*2]
    @ Pulls 15-bit BGR color values (which are actually 16 bits) into q0-q3.
    @ example:  q0 = 0111 1110 0101 0011
    vld1.16     {d0-d3}, [r1]!
    vld1.16     {d4-d7}, [r1]!
    @ Shift BGR color 1 bit to the left, discarding MSB and preparing for vbit.
    @ MSB is used for transparency (not needed here, and can mess with green).
    @ example:  q0 = 1111 1100 1010 0110
    vshl.u16    q0,  q0, #1
    vshl.u16    q1,  q1, #1
    vshl.u16    q2,  q2, #1
    vshl.u16    q3,  q3, #1
    @ Places red value in left most bits, clears bits to the right.
    @ example:  q8 = 1001 1000 0000 0000
    vshl.u16    q8,  q0, #10
    vshl.u16    q9,  q1, #10
    vshl.u16    q10, q2, #10
    vshl.u16    q11, q3, #10
    @ Places blue value in right most bits, leaving bits to the left unchanged.
    @ example:  q8 = 1001 1000 0001 1111
    vsri.u16    q8,  q0, #11
    vsri.u16    q9,  q1, #11
    vsri.u16    q10, q2, #11
    vsri.u16    q11, q3, #11
    @ Sets green value from shifted BGR color by apply a mask.
    @ example: q15 = 0000 0111 1100 0000
    @           q8 = 1001 1100 1001 1111
    vbit        q8,  q0, q15
    vbit        q9,  q1, q15
    vbit        q10, q2, q15
    vbit        q11, q3, q15
    vstmia      r0!, {q8-q11}
    subs        r2, r2, #64
    bge         0b

btr16_end64:
    adds        r2, r2, #64
    bxeq        lr
    subs        r2, r2, #16
    blt         btr16_end16

    @ handle the remainder (reasonably rare)
0:
    vld1.16     {q0}, [r1]!
    vshl.u16    q0, q0, #1
    vshl.u16    q1, q0, #10
    vsri.u16    q1, q0, #11
    vbit        q1, q0, q15
    subs        r2, r2, #16
    vst1.16     {q1}, [r0]!
    bge         0b

btr16_end16:
    adds        r2, r2, #16
    bxeq        lr
    subs        r2, r2, #8
    bxlt        lr

    @ very rare
    vld1.16     {d0}, [r1]!
    vshl.u16    d0, d0, #1
    vshl.u16    d1, d0, #10
    vsri.u16    d1, d0, #11
    vbit        d1, d0, d30
    vst1.16     {d1}, [r0]!
    bx          lr


@ note: may overflow source
FUNCTION(bgr555_to_rgb565_b): @ dst, src, bytes, int brightness2k // 0-0x0800
    pld         [r1]
    vdup.16     q15, r3
    vpush       {q4-q7}
    mov         r3, #0x1f
    vdup.16     q14, r3
0:
    pld         [r1, #64*2]
    vld1.16     {d0-d3}, [r1]!
    vld1.16     {d4-d7}, [r1]!
    vand.u16    q8,  q0, q14
    vand.u16    q9,  q1, q14
    vand.u16    q10, q2, q14
    vand.u16    q11, q3, q14
    vmul.u16    q4, q8,  q15
    vmul.u16    q5, q9,  q15
    vmul.u16    q6, q10, q15
    vmul.u16    q7, q11, q15

    vshr.u16    q8,  q0, #5
    vshr.u16    q9,  q1, #5
    vshr.u16    q10, q2, #5
    vshr.u16    q11, q3, #5
    vand.u16    q8,  q14
    vand.u16    q9,  q14
    vand.u16    q10, q14
    vand.u16    q11, q14
    vmul.u16    q8,  q15
    vmul.u16    q9,  q15
    vmul.u16    q10, q15
    vmul.u16    q11, q15
    vsri.u16    q4, q8,  #5
    vsri.u16    q5, q9,  #5
    vsri.u16    q6, q10, #5
    vsri.u16    q7, q11, #5

    vshr.u16    q8,  q0, #10
    vshr.u16    q9,  q1, #10
    vshr.u16    q10, q2, #10
    vshr.u16    q11, q3, #10
    vand.u16    q8,  q14
    vand.u16    q9,  q14
    vand.u16    q10, q14
    vand.u16    q11, q14
    vmul.u16    q8,  q15
    vmul.u16    q9,  q15
    vmul.u16    q10, q15
    vmul.u16    q11, q15
    vsri.u16    q4, q8,  #11
    vsri.u16    q5, q9,  #11
    vsri.u16    q6, q10, #11
    vsri.u16    q7, q11, #11

    subs        r2, r2, #64
    ble         1f
    vstmia      r0!, {q4-q7}
    b           0b

1:
    blt         0f
    vstmia      r0!, {q4-q7}
    b           btr16b_end
0:
    subs        r2, r2, #8
    blt         btr16b_end
    vst1.16     {q4}, [r0]!
    subs        r2, r2, #8
    blt         btr16b_end
    vst1.16     {q5}, [r0]!
    subs        r2, r2, #8
    blt         btr16b_end
    vst1.16     {q6}, [r0]!
    subs        r2, r2, #8
    blt         btr16b_end
    vst1.16     {q7}, [r0]!

btr16b_end:
    vpop        {q4-q7}
    bx          lr


FUNCTION(bgr888_to_rgb888): @ dst, src, bytes
    pld         [r1]
    @ r2 /= 48
    mov         r2, r2, lsr #4
    movw        r3, #0x5556
    movt        r3, #0x5555
    umull       r12,r2, r3, r2
0:
    pld         [r1, #48*3]
    vld3.8      {d0-d2}, [r1]!
    vld3.8      {d3-d5}, [r1]!
    vswp        d0, d2
    vswp        d3, d5
    vst3.8      {d0-d2}, [r0, :64]!
    vst3.8      {d3-d5}, [r0, :64]!
    subs        r2, r2, #1
    bne         0b

    bx          lr


FUNCTION(bgr888_to_rgb565): @ dst, src, bytes
    pld         [r1]
    @ r2 /= 48
    mov         r2, r2, lsr #4
    movw        r3, #0x5556
    movt        r3, #0x5555
    umull       r12,r2, r3, r2

    mov         r3, #0x07e0
    vdup.16     q15, r3
0:
    pld         [r1, #48*3]
    vld3.8      {d1-d3}, [r1]!
    vld3.8      {d5-d7}, [r1]!

    vshll.u8    q8, d2, #3      @ g
    vshll.u8    q9, d6, #3
    vshr.u8     d0, d3, #3      @ b
    vshr.u8     d4, d7, #3
    vzip.8      d0, d1          @ rb
    vzip.8      d4, d5
    vbit        q0, q8, q15
    vbit        q2, q9, q15

    vstmia      r0!, {d0,d1}
    vstmia      r0!, {d4,d5}
    subs        r2, r2, #1
    bne         0b

    bx          lr


FUNCTION(rgb888_to_rgb565): @ dst, src, bytes
    pld         [r1]
    @ r2 /= 48
    mov         r2, r2, lsr #4
    movw        r3, #0x5556
    movt        r3, #0x5555
    umull       r12,r2, r3, r2

    mov         r3, #0x07e0
    vdup.16     q15, r3
0:
    pld         [r1, #48*3]
    vld3.8      {d1-d3}, [r1, :64]!
    vld3.8      {d5-d7}, [r1, :64]!

    vshll.u8    q8, d2, #3      @ g
    vshll.u8    q9, d6, #3
    vshr.u8     d2, d1, #3      @ b
    vshr.u8     d6, d5, #3
    vzip.8      d2, d3          @ rb
    vzip.8      d6, d7
    vbit        q1, q8, q15
    vbit        q3, q9, q15

    vstmia      r0!, {d2,d3}
    vstmia      r0!, {d6,d7}
    subs        r2, r2, #1
    bne         0b

    bx          lr


@ vim:filetype=armasm:expandtab
