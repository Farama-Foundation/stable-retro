/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of GNU GPL version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "arm_features.h"
#include "new_dynarec/linkage_offsets.h"

.syntax unified
.text
.align 2

@ XXX: gteMAC calc shouldn't be saturating, but it is here

@ approximate gteMAC|123 flags
@ in: rr 123 as gteMAC|123
@ trash: nothing
.macro do_mac_flags rr1 rr2 rr3
    cmp         \rr1, #1
    orrvs       lr, #(1<<31)|(1<<27)
    cmp         \rr2, #1
    orrvs       lr, #(1<<31)|(1<<26)
    cmp         \rr3, #1
    orrvs       lr, #(1<<31)|(1<<25)
    cmn         \rr1, #1       @ same as adds ...
    orrvs       lr, #(1<<30)
    cmn         \rr2, #1
    orrvs       lr, #(1<<29)
    cmn         \rr3, #1
    orrvs       lr, #(1<<28)
.endm

@ approximate 3x gteMACn flags
@ in: rr 123 as 3 instances gteMACn, *flags
@ trash: nothing
.macro do_mac_flags3x rr1 rr2 rr3 nflags pflags
    cmp         \rr1, #1
    cmpvc       \rr2, #1
    cmpvc       \rr3, #1
    orrvs       lr, #\nflags
    cmn         \rr1, #1       @ adds ...
    cmnvc       \rr2, #1
    cmnvc       \rr3, #1
    orrvs       lr, #\pflags
.endm

@ get gteIR|123 flags from gteMAC|123
@ in: rr 123 as gteMAC|123
@ trash: r2,r3
.macro do_irs_flags rr1 rr2 rr3
    add         r2, \rr1, #0x8000
    add         r3, \rr2, #0x8000
    lsrs        r2, #16
    orrne       lr, #(1<<31)|(1<<24) @ IR1/limB1
    lsrs        r3, #16
    add         r2, \rr3, #0x8000
    orrne       lr, #(1<<31)
    orrne       lr, #(1<<23)   @ IR2/limB2
    lsrs        r2, #16
    orrne       lr, #(1<<22)   @ IR3/limB3
.endm


/*
 * RTPS/RTPT register map:
 *
 *  q | d | c code / phase 1   phase 2          scratch
 *  0   0   gteR1* [s16]       gteMAC3     =    gteMAC3  \ v=0 *
 *      1   gteR2*             gteIR1-3    =    gteIR1-3 /     *
 *  1   2   gteR3*             gteMAC3     =    gteMAC3  \ v=1
 *      3   *                  gteIR1-3    =    gteIR1-3 /
 *  2   4   gteTRX<<12 [s64]   gteOFX [s64]     gteMAC3  \ v=2
 *      5   gteTRY<<12         gteOFY [s64]     gteIR1-3 / 
 *  3   6   gteTRZ<<12         gteDQA [s64]     min gteMAC|12 v=012
 *      7   0                  gteDQB [s64]     max gteMAC|12
 *  4   8   VXYZ(v)  /    gteMAC1,2 [s32]       min gteIR|123
 *      9   *        /    gteMAC3               max gteIR|123
 *  5  10   gteIR1-3 [s16]     gteIR1-3 v=2     quotients 12
 *     11   0                                   quotient 3
 *  6  12   gteH (adj. for cmp)
 *     13   gteH (float for div)
 * ...      <scratch>
 * 15  30   0
 *     31   0
 */

@ load gteR*, gteTR* and gteH (see map above), clear q15
@ in: r0 - context
@ trash: r3
.macro rtpx_preload
    add         r3, r0, #4*32
    vldmia      r3, {d0-d2}    @ gteR*  [16*9]
    vmov.i32    q15, #0
    add         r3, r0, #4*(32+5)
    vldmia      r3, {d4-d5}    @ gteTR*
    vext.16     d2, d1, d2, #2 @ xxx3 -> x321
    vext.16     d1, d0, d1, #3 @ xx32 -> x321
    add         r3, r0, #4*(32+26)
    vld1.32     d11[0], [r3]   @ gteH
    vshll.s32   q3, d5, #12    @ gteTRZ
    vshll.s32   q2, d4, #12    @ gteTR|XY
    vmovl.s16   q6, d11        @ gteH
.endm

@ do RTP* gteMAC* calculation
@ in: gteR*, gteTR* as in map, d8 - VXYZ, r12 - 0
@ out: d8,d9 - gteMAC|123, d10 - gteIR|123
@ trash: d16-d21
.macro rtpx_mac
    vmull.s16   q8, d0, d8
    vmull.s16   q9, d1, d8
    vmull.s16   q10, d2, d8
    vpaddl.s32  q8, q8
    vpaddl.s32  q9, q9
    vpaddl.s32  q10, q10
    vadd.s64    d16, d17       @ d16=d0.16[2]*d8.16[2], as
    vadd.s64    d18, d19       @ d8[3]==0, so won't affect
    vadd.s64    d20, d21       @ QC
    vadd.s64    d16, d4
    vadd.s64    d18, d5
    vadd.s64    d20, d6
    vqshrn.s64  d8, q8, #12    @ gteMAC1
    vqshrn.s64  d18, q9, #12   @ gteMAC2
    vqshrn.s64  d9, q10, #12   @ gteMAC3
    vsli.u64    d8, d18, #32   @ gteMAC|12
    vmov.32     d9[1], r12
    vqmovn.s32  d10, q4        @ gteIR|123; losing 2 cycles?
.endm

FUNCTION(gteRTPS_neon): @ r0=CP2 (d,c),
    push        {r4-r6,lr}

@    fmrx        r4, fpscr      @ vmrs? at least 40 cycle hit
    ldr		r1, [r0, #LO_cop2_to_scratch_buf]
    mov         r12, #0

    vldmia      r0, {d8}       @ VXYZ(0)
    rtpx_preload

@    rtpx_mac                  @ slower here, faster in RTPT?
    vmov.16     d8[3], r12     @ kill unused upper vector
    vmull.s16   q8, d0, d8
    vmull.s16   q9, d1, d8
    vmull.s16   q10, d2, d8
    vpadd.s32   d16, d16, d17
    vpadd.s32   d17, d18, d19
    vpadd.s32   d18, d20, d21
    vpadal.s32  q2, q8
    vpadal.s32  q3, q9         @ d6, d18 is slow?
    vqshrn.s64  d8, q2, #12    @ gteMAC|12
    vqshrn.s64  d9, q3, #12    @ gteMAC3

    add         r3, r0, #4*25
    vst1.32     d8, [r3]!
    vst1.32     d9[0], [r3]    @ wb gteMAC|123
    vqmovn.s32  d10, q4        @ gteIR|123

    add         r3, r0, #4*17  @ gteSZ*
    vldmia      r3, {q7}       @ d14,d15 gteSZ|123x
    vmov.i32    d28, #0xffff   @ 0xffff[32]
    vmax.s32    d11, d9, d31
    vshr.s32    d16, d12, #1   @ | gteH/2 (adjust for cmp)
    vmov.i32    d26, #1
    vmin.u32    d11, d28       @ saturate to 0..0xffff limD/fSZ3
    vmovl.s16   q9, d10        @ || expand gteIR|123
    vshl.u32    d13, d12, #16  @ | preparing gteH
    add         r3, r0, #4*9
    vst1.32     d18, [r3]!
    vst1.32     d19[0], [r3]

    vsli.u64    d15, d11, #32  @ new gteSZ|0123 in q7
    vclt.u32    d16, d16, d11  @ gteH/2 < fSZ3?

    add         r3, r0, #4*(32+24)
    vld1.32     d4, [r3]       @ || gteOF|XY
    add         r3, r0, #4*(32+27)
    vld1.32     d6, [r3]       @ || gteDQ|AB

    vand        d11, d16
    vmovl.s32   q2, d4         @ || gteOF|XY [64]
    vmax.u32    d11, d26       @ make divisor 1 if not
    vmovl.s32   q3, d6         @ || gteDQ|AB [64]
    add         r3, r0, #4*16  @ | gteSZ*
    vstmia      r3, {q7}       @ | d14,d15 gteSZ|123x

    vcvt.f32.u32 d13, d13      @ gteH (float for div)
    vcvt.f32.u32 d11, d11      @ divisor

    @ divide.. it's not worth messing with reciprocals here
    @ just for 1 value, let's just use VFP divider here
    vdiv.f32    s22, s26, s22

    vmov.f32    d20, #0.5
    vadd.f32    d11, d20
    vcvt.u32.f32 d11, d11      @ quotient

    @ while NEON's busy we calculate some flags on ARM
    add         r3, r0, #4*25
    mov         lr, #0         @ gteFLAG
    ldmia       r3, {r4-r6}    @ gteMAC|123

    vst1.32     d11, [r1, :64] @ wb quotient for flags (pre-limE)
    vqshl.u32   d11, #15

    do_mac_flags r4, r5, r6

    vshr.u32    d11, #15       @ quotient (limE)

    do_irs_flags r4, r5, r6

    vmlal.s32   q2, d18, d11[0]@ gteOF|XY + gteIR|12 * quotient
    add         r3, r0, #4*13
    vld1.32     d16, [r3]      @ || load fS|XY12, new 01
    vqmovn.s64  d18, q2        @ saturate to 32
    vmull.s32   q10, d6, d11[0]@ | d20 = gteDQA * quotient
    vqshl.s32   d19, d18, #5   @ 11bit precision

    ldr         r4, [r1]       @ quotient
    movs        r3, r6, lsr #16
    orrne       lr, #(1<<31)
    orrne       lr, #(1<<18)   @ fSZ (limD)

    vst1.32     d18, [r1, :64] @ writeback fS|XY2 before limG

    vshr.s32    d18, d19, #16+5@ can't vqshrn because of insn
    vadd.s64    d20, d7        @ | gteDQB + gteDQA * quotient
    vmovn.s32   d18, q9        @ fS|XY2 [s16]

    vqmovn.s64  d20, q10       @ | gteMAC0
    add         r3, r0, #4*12
    vst1.32     d16, [r3]!     @ writeback fS|XY01
    vst1.32     d18[0], [r3]   @ ...2
    add         r3, r0, #4*24
    vshr.s32    d21, d20, #12
    vst1.32     d20[0], [r3]   @ gteMAC0

    movs        r4, r4, lsr #17
    orrne       lr, #(1<<31)
    orrne       lr, #(1<<17)   @ limE

    vmax.s32    d21, d31
    vmov.i32    d22, #0x1000
    vmin.s32    d21, d22
    add         r3, r0, #4*8
    vst1.16     d21[0], [r3]   @ gteIR0

    ldmia       r1, {r4,r5}    @ fS|XY2 before limG, after 11bit sat
    add         r2, r4, #0x400<<16
    add         r3, r5, #0x400<<16
    lsrs        r2, #16+11
    orrne       lr, #(1<<14)   @ limG1
    orrne       lr, #(1<<31)
    lsrs        r3, #16+11
    orrne       lr, #(1<<13)   @ limG2
    orrne       lr, #(1<<31)
    adds        r2, r4, #1
    addsvc      r3, r5, #1
    orrvs       lr, #(1<<16)   @ F
    orrvs       lr, #(1<<31)
    subs        r2, r4, #1
    subsvc      r3, r5, #1
    orrvs       lr, #(1<<31)

    ldr         r4, [r0, #4*24] @ gteMAC0
    orrvs       lr, #(1<<15)

    adds        r3, r4, #1
    orrvs       lr, #(1<<16)   @ F
    orrvs       lr, #(1<<31)
    subs        r2, r4, #1
    orrvs       lr, #(1<<15)   @ F
    orrvs       lr, #(1<<31)
    cmp         r4, #0x1000<<12
    orrhi       lr, #(1<<12)   @ limH

    str         lr, [r0, #4*(32+31)] @ gteFLAG

    pop         {r4-r6,pc}
    .size	gteRTPS_neon, .-gteRTPS_neon
 


FUNCTION(gteRTPT_neon): @ r0=CP2 (d,c),
    push        {r4-r11,lr}

    ldr		r1, [r0, #LO_cop2_to_scratch_buf]
    mov         r12, #0

    rtpx_preload

    vmvn.i32    d22, #0x80000000 @ #0x7fffffff
    vmov.i32    d23, #0x80000000
    mov         r3, #3         @ counter
    mov         r2, r0         @ VXYZ(0)
0:
    vldmia      r2!, {d8}      @ VXYZ(v)
    vmov.16     d8[3], r12     @ kill unused upper vector

    rtpx_mac
    vmin.s32    d22, d8        @ min gteMAC|12
    vmax.s32    d23, d8        @ max gteMAC|12
    subs        r3, #1
    vst1.32     {d9,d10}, [r1, :128]!
    bgt         0b

    vst1.32     {d22,d23}, [r1, :128]! @ min/max gteMAC|12, for flags

    @ - phase2 -
    sub         r1, r1, #8*2*4
    vldmia      r1, {d0-d3}    @ note: d4,d5 is for gteOF|XY

    vmov        d20, d0        @ gteMAC3 v=0
    vmin.s16    d24, d1, d3    @ | find min IR
    vshr.s32    d22, d12, #1   @ || gteH/2 (adjust for cmp)
    vmax.s16    d25, d1, d3    @ | .. also max, for flag gen
    vsli.u64    d20, d2, #32   @ gteMAC3 v=1
    vmov        d21, d9        @ ... v=2

    vmov.i32    q14, #0xffff   @ 0xffff[32]
    vmax.s32    q10, q15
    vmov.i32    q13, #1
    vdup.32     q11, d22[0]    @ gteH/2
    vmin.u32    q10, q14       @ saturate to 0..0xffff limD/fSZ(v)
    vmin.s16    d24, d10       @ | find min/max IR
    vmax.s16    d25, d10       @ |

    add         r3, r0, #4*19  @ ||
    vld1.32     d14[0], [r3]   @ || gteSZ3

    vclt.u32    q11, q11, q10  @ gteH/2 < fSZ(v)?
    add         r3, r0, #4*17
    vst1.32     d20, [r3]!     @ | writeback fSZ(v)
    vand        q11, q10, q11
    vst1.32     d21[0], [r3]   @ |
    vmax.u32    q10, q11, q13  @ make divisor 1 if not
    add         r3, r1, #8*8
    vstmia      r3, {q12}      @ min/max IR for flags
    vcvt.f32.u32 q10, q10
    vshl.u32    d13, d12, #16  @ | preparing gteH

    @ while NEON's busy we calculate some flags on ARM
    add         r2, r1, #8*2*3
    mov         lr, #0         @ gteFLAG
    ldmia       r2, {r4-r7}    @ min/max gteMAC|12
    subs        r2, r4, #1
    orrvs       lr, #(1<<31)|(1<<27)
    subs        r3, r5, #1
    orrvs       lr, #(1<<31)|(1<<26)
    adds        r2, r6, #1
    orrvs       lr, #(1<<30)
    adds        r3, r7, #1
    orrvs       lr, #(1<<29)
    ldr         r4, [r1, #0]   @ gteMAC3 v=0
    ldr         r5, [r1, #8*2] @ ... v=1
    ldr         r6, [r1, #8*4] @ ... v=2

    add         r3, r0, #4*(32+24)
    vld1.32     d4, [r3]       @ || gteOF|XY
    add         r3, r0, #4*(32+27)
    vld1.32     d6, [r3]       @ || gteDQ|AB

    @ divide
.if 1
    vrecpe.f32  q11, q10       @ inv
    vmovl.s32   q2, d4         @ || gteOF|XY [64]
    vmovl.s32   q3, d6         @ || gteDQ|AB [64]
    vrecps.f32  q12, q10, q11  @ step
    vcvt.f32.u32 d13, d13      @ | gteH (float for div)
    vmov.f32    q8, #0.5       @ |||
    vmul.f32    q11, q12, q11  @ better inv
    add         r3, r0, #4*16
    vst1.32     d14[0], [r3]   @ gteSZ0 = gteSZ3
    vdup.32     q13, d13[0]    @ |
@    vrecps.f32  q12, q10, q11  @ step
@    vmul.f32    q11, q12, q11  @ better inv
    vmul.f32    q10, q13, q11  @ result
.else
    vmov.f32    q8, #0.5       @ |||
    vmovl.s32   q2, d4         @ || gteOF|XY [64]
    vmovl.s32   q3, d6         @ || gteDQ|AB [64]
    vcvt.f32.u32 d13, d13      @ | gteH (float for div)
    vdup.32     q13, d13[0]    @ |
    add         r3, r0, #4*16
    vst1.32     d14[0], [r3]   @ gteSZ0 = gteSZ3

    vpush       {q0}
    vmov        q0, q10        @ to test against C code
    vdiv.f32    s0, s26, s0
    vdiv.f32    s1, s26, s1
    vdiv.f32    s2, s26, s2
    vmov        q10, q0
    vpop        {q0}
.endif

    do_mac_flags3x r4, r5, r6, (1<<31)|(1<<25), (1<<27) @ MAC3
    orr         r7, r4, r5
    add         r4, r1, #8*8
    orr         r3, r7, r6
    ldmia       r4, {r7,r8,r10,r11} @ min/max IR

    movs        r3, r3, lsr #16
    orrne       lr, #(1<<31)
    orrne       lr, #(1<<18)   @ fSZ (limD)

    vadd.f32     q10, q8       @ adjust for vcvt rounding mode
    vcvt.u32.f32 q8, q10
    vmovl.s16   q9, d1         @ expand gteIR|12 v=0
    vmovl.s16   q10, d3        @ expand gteIR|12 v=1
    add         r6, r1, #8*10
    vstmia      r6, {q8}       @ wb quotients for flags (pre-limE)
    vqshl.u32   q8, #15
    vmovl.s16   q11, d10       @ expand gteIR|12 v=2
    vshr.u32    q8, #15        @ quotients (limE)
    vdup.32     d24, d16[0]
    vdup.32     d25, d16[1]
    vdup.32     d26, d17[0]    @ quotient (dup)

    @ flags for minIR012 (r7,r8), maxIR012 (r10,r11)
    mov         r4, #0x10000
    cmp         r7, #1<<16
    cmnvc       r10, #1<<16
    orrvs       lr, #(1<<31)
    orrvs       lr, #(1<<23)   @ IR2/limB2
    rsbs        r2, r4, r7, lsl #16
    cmnvc       r4, r10, lsl #16
    orrvs       lr, #(1<<31)|(1<<24) @ IR1/limB1
    rsbs        r2, r4, r8, lsl #16
    cmnvc       r4, r11, lsl #16
    orrvs       lr, #(1<<22)   @ IR3/limB3

    vmull.s32   q9, d18, d24   @ gteIR|12 * quotient v=0
    vmull.s32   q10, d20, d25  @ ... v=1
    vmull.s32   q11, d22, d26  @ ... v=2
    vadd.s64    q9, q2         @ gteOF|XY + gteIR|12 * quotient
    vadd.s64    q10, q2        @ ... v=1
    vadd.s64    q11, q2        @ ... v=2
    vqmovn.s64  d18, q9        @ saturate to 32 v=0
    vqmovn.s64  d19, q10       @ ... v=1
    vqmovn.s64  d20, q11       @ ... v=2
    vmin.s32    d14, d18, d19  @ || find min/max fS|XY(v) [32]
    vmax.s32    d15, d18, d19  @ || for flags
    vmin.s32    d14, d20
    vmax.s32    d15, d20
    vqshl.s32   q11, q9, #5    @ 11bit precision, v=0,1
    vqshl.s32   d24, d20, #5   @ ... v=2
    vmull.s32   q13, d6, d17   @ | gteDQA * quotient v=2
    vpmin.s32   d16, d14, d31  @ || also find min/max in pair
    vpmax.s32   d17, d15, d31  @ ||
    vshr.s32    q11, #16+5     @ can't vqshrn because of insn
    vshr.s32    d24, #16+5     @ encoding doesn't allow 21 :(
    vsli.u64    d16, d17, #32  @ || pack in-pair min/max
    vadd.s64    d26, d7        @ | gteDQB + gteDQA * quotient
    vmovn.s32   d12, q11       @ fS|XY(v) [s16] v=0,1
    vmovn.s32   d13, q12       @ 3
    vstmia      r1, {d14-d16}  @ || other cacheline than quotients
    add         r3, r0, #4*12
    vst1.32     d12, [r3]!     @ writeback fS|XY v=0,1
    vst1.32     d13[0], [r3]

    vqmovn.s64  d26, q13       @ | gteMAC0
    vmovl.u16   q5, d10        @ expand gteIR|123 v=2

    vmov.i32    d13, #0x1000
    vshr.s32    d12, d26, #12

    add         r3, r0, #4*24
    vst1.32     d26[0], [r3]!  @ gteMAC0
    vmax.s32    d12, d30
    vst1.32     d8, [r3]!      @ gteMAC123 (last iteration)
    vst1.32     d9[0], [r3]

    vmin.s32    d12, d13       @ | gteIR0

    ldmia       r6, {r4-r6}    @ quotients
    orr         r4, r5
    orr         r4, r6
    add         r3, r0, #4*8
    movs        r4, r4, lsr #17

    vst1.32     d12[0], [r3]!  @ gteIR0
    vst1.32     d10, [r3]!     @ gteIR12
    vst1.32     d11[0], [r3]   @ ..3

    @ ~23 cycles
    orrne       lr, #(1<<31)   @ limE
    orrne       lr, #(1<<17)   @ limE
    ldmia       r1, {r4-r9}
    add         r2, r4, #0x400<<16 @ min fSX
    add         r3, r6, #0x400<<16 @ max fSX
    lsrs        r2, #16+11
    lsrseq      r3, #16+11
    orrne       lr, #(1<<31)   @ limG1
    orrne       lr, #(1<<14)
    add         r2, r5, #0x400<<16 @ min fSY
    add         r3, r7, #0x400<<16 @ max fSY
    lsrs        r2, #16+11
    lsrseq      r3, #16+11
    orrne       lr, #(1<<31)   @ limG2
    orrne       lr, #(1<<13)
    adds        r2, r9, #1
    orrvs       lr, #(1<<16)   @ F (31 already done by above)
    subs        r3, r8, #1

    ldr         r4, [r0, #4*24] @ gteMAC0
    orrvs       lr, #(1<<15)

    adds        r3, r4, #1
    orrvs       lr, #(1<<16)
    orrvs       lr, #(1<<31)   @ F
    subs        r2, r4, #1
    orrvs       lr, #(1<<15)
    orrvs       lr, #(1<<31)   @ F
    cmp         r4, #0x1000<<12
    orrhi       lr, #(1<<12)   @ limH

    str         lr, [r0, #4*(32+31)] @ gteFLAG

    pop         {r4-r11,pc}
    .size	gteRTPT_neon, .-gteRTPT_neon



@ note: non-std calling convention used
@ r0 = CP2 (d,c)  (must preserve)
@ r1 = op
@ r4,r5 = VXYZ(v) packed
@ r6 = &MX11(mx)
@ r7 = &CV1(cv)
FUNCTION(gteMVMVA_part_neon):
    uxth        r5, r5
    vmov.32     d8[0], r4
    vmov.32     d8[1], r5       @ VXYZ(v)
    vldmia      r6, {d0-d2}     @ MXxy/gteR*  [16*9]
    vldmia      r7, {d4-d5}     @ CVx/gteTR*

    vmov.i32    q15, #0
    vext.16     d2, d1, d2, #2 @ xxx3 -> x321
    vext.16     d1, d0, d1, #3 @ xx32 -> x321
    vshll.s32   q3, d5, #12    @ gteTRZ/CV3
    vshll.s32   q2, d4, #12    @ gteTR|XY/CV12

    vmull.s16   q8, d0, d8
    vmull.s16   q9, d1, d8
    vmull.s16   q10, d2, d8
    vpadd.s32   d16, d16, d17
    vpadd.s32   d17, d18, d19
    vpadd.s32   d18, d20, d21
    vpadal.s32  q2, q8
    vpadal.s32  q3, q9
    tst         r1, #1<<19
    beq         0f
    vshr.s64    q2, q2, #12
    vshr.s64    q3, q3, #12
0:
    vqmovn.s64  d8, q2         @ gteMAC|12
    vqmovn.s64  d9, q3         @ gteMAC3

    tst         r1, #1<<10
    add         r3, r0, #4*25
    vqmovn.s32  d10, q4        @ gteIR|123
    vst1.32     d8, [r3]!
    vst1.32     d9[0], [r3]    @ wb gteMAC|123

    beq         0f
    vmax.s16    d10, d31
0:
    vmovl.s16   q9, d10        @ expand gteIR|123
    add         r3, r0, #4*9
    vst1.32     d18, [r3]!
    vst1.32     d19[0], [r3]
    bx          lr
    .size       gteMVMVA_part_neon, .-gteMVMVA_part_neon


@ get flags after gteMVMVA_part_neon operation
FUNCTION(gteMACtoIR_flags_neon): @ r0=CP2 (d,c), r1=lm
    push        {r4,r5,lr}
    tst         r1, r1         @ lm
    mov         lr, #0         @ gteFLAG
    mov         r2, #0
    mov         r12, #15
    moveq       r2, #0x8000    @ adj
    moveq       r12, #16       @ shift

    add         r3, r0, #4*25
    ldmia       r3, {r3-r5}    @ gteMAC|123

    do_mac_flags r3, r4, r5

    add         r3, r2
    add         r4, r2
    add         r5, r2
    asrs        r3, r12
    orrne       lr, #(1<<31)|(1<<24) @ IR1/limB1
    asrs        r4, r12
    orrne       lr, #(1<<31)
    orrne       lr, #(1<<23)   @ IR2/limB2
    asrs        r5, r12
    orrne       lr, #(1<<22)   @ IR3/limB3
    str         lr, [r0, #4*(32+31)] @ gteFLAG

    pop         {r4,r5,pc}
    .size       gteMACtoIR_flags_neon, .-gteMACtoIR_flags_neon



@ vim:filetype=armasm
