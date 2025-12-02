/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of GNU GPL version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "arm_features.h"

.text
.align 2

.macro sgnxt16 rd rs
#ifdef HAVE_ARMV6
    sxth     \rd, \rs
#else
    lsl      \rd, \rs, #16
    asr      \rd, \rd, #16
#endif
.endm

@ prepare work reg for ssatx
@ in: wr reg, bit to saturate to
.macro ssatx_prep wr bit
#ifndef HAVE_ARMV7
    mov      \wr, #(1<<(\bit-1))
#endif
.endm

.macro ssatx rd wr bit
#ifdef HAVE_ARMV6
    ssat     \rd, #\bit, \rd
#else
    cmp      \rd, \wr
    subge    \rd, \wr, #1
    cmn      \rd, \wr
    rsblt    \rd, \wr, #0
#endif
.endm

@ prepare work reg for ssatx0 (sat to 0..2^(bit-1))
@ in: wr reg, bit to saturate to
.macro ssatx0_prep wr bit
    mov      \wr, #(1<<(\bit-1))
.endm

.macro ssatx0 rd wr bit
    cmp      \rd, \wr
    subge    \rd, \wr, #1
    cmn      \rd, #0
    movlt    \rd, #0
.endm

.macro usat16_ rd rs
#ifdef HAVE_ARMV6
    usat     \rd, #16, \rs
#else
    subs     \rd, \rs, #0
    movlt    \rd, #0
    cmp      \rd, #0x10000
    movge    \rd, #0x0ff00
    orrge    \rd, #0x000ff
#endif
.endm

#ifdef HAVE_ARMV5

.macro udiv_ rd rm rs
    lsl      \rm, #16
    clz      \rd, \rs
    lsl      \rs, \rs, \rd        @ shift up divisor
    orr      \rd, \rd, #1<<31
    lsr      \rd, \rd, \rd
0:
    cmp      \rm, \rs
    subcs    \rm, \rs
    adcs     \rd, \rd, \rd
    lsr      \rs, #1
    bcc      0b
.endm

.macro newton_step rcp den zero t1 t2
    umull    \t2, \t1, \den, \rcp  @ \t2 is dummy
    sub      \t1, \zero, \t1, lsl #2
    smlal    \t2, \rcp, \t1, \rcp
.endm

.macro udiv_newton rd rm rs t1 t2 t3 t4
    lsl      \rd, \rm, #16
    clz      \t1, \rs
    mov      \t2, #0
    lsl      \rs, \t1             @ normalize for the algo
    mov      \rm, #0x4d000000     @ initial estimate ~1.2

    newton_step \rm, \rs, \t2, \t3, \t4
    newton_step \rm, \rs, \t2, \t3, \t4
    newton_step \rm, \rs, \t2, \t3, \t4
    newton_step \rm, \rs, \t2, \t3, \t4

    umull    \t4, \rd, \rm, \rd
    rsb      \t2, \t1, #30        @ here t1 is 1..15
    mov      \rd, \rd, lsr \t2
.endm

@ unsigned divide rd = rm / rs; 16.16 result
@ no div by 0 check
@ in: rm, rs
@ trash: rm rs t*
.macro udiv rd rm rs t1 t2 t3 t4
    @udiv_        \rd, \rm, \rs
    udiv_newton  \rd, \rm, \rs, \t1, \t2, \t3, \t4
.endm

@ calculate RTPS/RTPT MAC values
@ in: r0 context, r8,r9 VXYZ
@ out: r10-r12 MAC123
@ trash: r1-r7
.macro do_rtpx_mac
    add      r1, r0, #4*32
    add      r2, r0, #4*(32+5)    @ gteTRX
    ldmia    r1!,{r5-r7}          @ gteR1*,gteR2*
    ldmia    r2, {r10-r12}
    smulbb   r2, r5, r8           @ gteR11 * gteVX0
    smultt   r3, r5, r8           @ gteR12 * gteVY0
    smulbb   r4, r6, r9           @ gteR13 * gteVZ0
    qadd     r2, r2, r3
    asr      r4, r4, #1           @ prevent oflow, lose a bit
    add      r3, r4, r2, asr #1
    add      r10,r10,r3, asr #11  @ gteMAC1
    smultb   r2, r6, r8           @ gteR21 * gteVX0
    smulbt   r3, r7, r8           @ gteR22 * gteVY0
    smultb   r4, r7, r9           @ gteR23 * gteVZ0
    ldmia    r1!,{r5-r6}          @ gteR3*
    qadd     r2, r2, r3
    asr      r4, r4, #1
    add      r3, r4, r2, asr #1
    add      r11,r11,r3, asr #11  @ gteMAC2
    @ be more accurate for gteMAC3, since it's also a divider
    smulbb   r2, r5, r8           @ gteR31 * gteVX0
    smultt   r3, r5, r8           @ gteR32 * gteVY0
    smulbb   r4, r6, r9           @ gteR33 * gteVZ0
    qadd     r2, r2, r3
    asr      r3, r4, #31          @ expand to 64bit
    adds     r1, r2, r4
    adc      r3, r2, asr #31      @ 64bit sum in r3,r1
    add      r12,r12,r3, lsl #20
    add      r12,r12,r1, lsr #12  @ gteMAC3
.endm


FUNCTION(gteRTPS_nf_arm): @ r0=CP2 (d,c),
    push     {r4-r11,lr}

    ldmia    r0, {r8,r9}          @ VXYZ(0)
    do_rtpx_mac
    add      r1, r0, #4*25        @ gteMAC1
    add      r2, r0, #4*17        @ gteSZ1
    stmia    r1, {r10-r12}        @ gteMAC123 save
    ldmia    r2, {r3-r5}
    add      r1, r0, #4*16        @ gteSZ0
    add      r2, r0, #4*9         @ gteIR1
    ssatx_prep r6, 16
    usat16_  lr, r12              @ limD
    ssatx    r10,r6, 16
    ssatx    r11,r6, 16
    ssatx    r12,r6, 16
    stmia    r1, {r3-r5,lr}       @ gteSZ*
    ldr      r3, [r0,#4*(32+26)]  @ gteH
    stmia    r2, {r10,r11,r12}    @ gteIR123 save
    cmp      r3, lr, lsl #1       @ gteH < gteSZ3*2 ?
    mov      r9, #1<<30
    bhs      1f
.if 1
    udiv     r9, r3, lr, r1, r2, r6, r7
.else
    push     {r0, r12}
    mov      r0, r3
    mov      r1, lr
    bl       DIVIDE
    mov      r9, r0
    pop      {r0, r12}
.endif
1:
    ldrd     r6, r7, [r0, #4*(32+24)] @ gteOFXY
                                  cmp      r9, #0x20000
    add      r1, r0, #4*12        @ gteSXY0
                                  movhs    r9, #0x20000
    ldmia    r1, {r2-r4}
                   /* quotient */ subhs    r9, #1
    mov      r2, r6, asr #31
    smlal    r6, r2, r10, r9
    stmia    r1!,{r3,r4}          @ shift gteSXY
    mov      r3, r7, asr #31
    smlal    r7, r3, r11, r9
    lsr      r6, #16
             /* gteDQA, gteDQB */ ldrd     r10,r11, [r0, #4*(32+27)]
    orr      r6, r2, lsl #16      @ (gteOFX + gteIR1 * q) >> 16
    ssatx_prep r2, 11
    lsr      r7, #16
        /* gteDQB + gteDQA * q */ mla      r4, r10, r9, r11
    orr      r7, r3, lsl #16      @ (gteOFY + gteIR2 * q) >> 16
    ssatx    r6, r2, 11           @ gteSX2
    ssatx    r7, r2, 11           @ gteSY2
    strh     r6, [r1]
    strh     r7, [r1, #2]
    str      r4, [r0,#4*24]       @ gteMAC0
    asrs     r4, #12
    movmi    r4, #0
    cmp      r4, #0x1000          @ limH
    movgt    r4, #0x1000
    str      r4, [r0,#4*8]        @ gteIR0

    pop      {r4-r11,pc}
    .size    gteRTPS_nf_arm, .-gteRTPS_nf_arm


FUNCTION(gteRTPT_nf_arm): @ r0=CP2 (d,c),
    ldr      r1, [r0, #4*19]      @ gteSZ3
    push     {r4-r11,lr}
    str      r1, [r0, #4*16]      @ gteSZ0
    mov      lr, #0

rtpt_arm_loop:
    add      r1, r0, lr, lsl #1
    ldrd     r8, r9, [r1]         @ VXYZ(v)
    do_rtpx_mac

    ssatx_prep r6, 16
    usat16_  r2, r12              @ limD
    add      r1, r0, #4*25        @ gteMAC1
    ldr      r3, [r0,#4*(32+26)]  @ gteH
    stmia    r1, {r10-r12}        @ gteMAC123 save
    add      r1, r0, #4*17
    ssatx    r10,r6, 16
    ssatx    r11,r6, 16
    ssatx    r12,r6, 16
    str      r2, [r1, lr]         @ fSZ(v)
    cmp      r3, r2, lsl #1       @ gteH < gteSZ3*2 ?
    mov      r9, #1<<30
    bhs      1f
.if 1
    udiv     r9, r3, r2, r1, r4, r6, r7
.else
    push     {r0, r12, lr}
    mov      r0, r3
    mov      r1, r2
    bl       DIVIDE
    mov      r9, r0
    pop      {r0, r12, lr}
.endif
1:                                cmp      r9, #0x20000
    add      r1, r0, #4*12
                                  movhs    r9, #0x20000
    ldrd     r6, r7, [r0,#4*(32+24)] @ gteOFXY
                   /* quotient */ subhs    r9, #1
    mov      r2, r6, asr #31
    smlal    r6, r2, r10, r9
    mov      r3, r7, asr #31
    smlal    r7, r3, r11, r9
    lsr      r6, #16
    orr      r6, r2, lsl #16      @ (gteOFX + gteIR1 * q) >> 16
    ssatx_prep r2, 11
    lsr      r7, #16
    orr      r7, r3, lsl #16      @ (gteOFY + gteIR2 * q) >> 16
    ssatx    r6, r2, 11           @ gteSX(v)
    ssatx    r7, r2, 11           @ gteSY(v)
    strh     r6, [r1, lr]!
    add      lr, #4
    strh     r7, [r1, #2]
    cmp      lr, #12
    blt      rtpt_arm_loop

    ldrd     r4, r5, [r0, #4*(32+27)] @ gteDQA, gteDQB
    add      r1, r0, #4*9         @ gteIR1
    mla      r3, r4, r9, r5       @ gteDQB + gteDQA * q
    stmia    r1, {r10,r11,r12}    @ gteIR123 save

    str      r3, [r0,#4*24]       @ gteMAC0
    asrs     r3, #12
    movmi    r3, #0
    cmp      r3, #0x1000          @ limH
    movgt    r3, #0x1000
    str      r3, [r0,#4*8]        @ gteIR0

    pop      {r4-r11,pc}
    .size    gteRTPT_nf_arm, .-gteRTPT_nf_arm


@ note: not std calling convention used
@ r0 = CP2 (d,c)  (must preserve)
@ r1 = needs_shift12
@ r4,r5 = VXYZ(v) packed
@ r6 = &MX11(mx)
@ r7 = &CV1(cv)
.macro mvma_op do_flags
    push     {r8-r11}

.if \do_flags
    ands     r3, r1, #1           @ gteFLAG, shift_need
.else
    tst      r1, #1
.endif
    ldmia    r7, {r7-r9}          @ CV123
    ldmia    r6!,{r10-r12}        @ MX1*,MX2*
    asr      r1, r7, #20
    lsl      r7, #12              @ expand to 64bit
    smlalbb  r7, r1, r10, r4      @ MX11 * vx
    smlaltt  r7, r1, r10, r4      @ MX12 * vy
    smlalbb  r7, r1, r11, r5      @ MX13 * vz
    lsrne    r7, #12
    orrne    r7, r1, lsl #20      @ gteMAC0
.if \do_flags
    asrne    r1, #20
    adds     r2, r7, #0x80000000
    adcs     r1, #0
    orrgt    r3, #(1<<30)
    orrmi    r3, #(1<<31)|(1<<27)
    tst      r3, #1               @ repeat shift test
.endif
    asr      r1, r8, #20
    lsl      r8, #12              @ expand to 64bit
    smlaltb  r8, r1, r11, r4      @ MX21 * vx
    smlalbt  r8, r1, r12, r4      @ MX22 * vy
    smlaltb  r8, r1, r12, r5      @ MX23 * vz
    lsrne    r8, #12
    orrne    r8, r1, lsl #20      @ gteMAC1
.if \do_flags
    asrne    r1, #20
    adds     r2, r8, #0x80000000
    adcs     r1, #0
    orrgt    r3, #(1<<29)
    orrmi    r3, #(1<<31)|(1<<26)
    tst      r3, #1               @ repeat shift test
.endif
    ldmia    r6!,{r10-r11}        @ MX3*
    asr      r1, r9, #20
    lsl      r9, #12              @ expand to 64bit
    smlalbb  r9, r1, r10, r4      @ MX31 * vx
    smlaltt  r9, r1, r10, r4      @ MX32 * vy
    smlalbb  r9, r1, r11, r5      @ MX33 * vz
    lsrne    r9, #12
    orrne    r9, r1, lsl #20      @ gteMAC2
.if \do_flags
    asrne    r1, #20
    adds     r2, r9, #0x80000000
    adcs     r1, #0
    orrgt    r3, #(1<<28)
    orrmi    r3, #(1<<31)|(1<<25)
    bic      r3, #1
.else
    mov      r3, #0
.endif
    str      r3, [r0, #4*(32+31)] @ gteFLAG
    add      r1, r0, #4*25
    stmia    r1, {r7-r9}

    pop      {r8-r11}
    bx       lr
.endm

FUNCTION(gteMVMVA_part_arm):
    mvma_op 1
    .size    gteMVMVA_part_arm, .-gteMVMVA_part_arm
 
FUNCTION(gteMVMVA_part_nf_arm):
    mvma_op 0
    .size    gteMVMVA_part_nf_arm, .-gteMVMVA_part_nf_arm
 
@ common version of MVMVA with cv3 (== 0) and shift12,
@ can't overflow so no gteMAC flags needed
@ note: not std calling convention used
@ r0 = CP2 (d,c)  (must preserve)
@ r4,r5 = VXYZ(v) packed
@ r6 = &MX11(mx)
FUNCTION(gteMVMVA_part_cv3sh12_arm):
    push     {r8-r9}
    ldmia    r6!,{r7-r9}          @ MX1*,MX2*
    smulbb   r1, r7, r4           @ MX11 * vx
    smultt   r2, r7, r4           @ MX12 * vy
    smulbb   r3, r8, r5           @ MX13 * vz
    qadd     r1, r1, r2
    asr      r3, #1               @ prevent oflow, lose a bit
    add      r1, r3, r1, asr #1
    asr      r7, r1, #11
    smultb   r1, r8, r4           @ MX21 * vx
    smulbt   r2, r9, r4           @ MX22 * vy
    smultb   r3, r9, r5           @ MX23 * vz
    qadd     r1, r1, r2
    asr      r3, #1
    add      r1, r3, r1, asr #1
    asr      r8, r1, #11
    ldmia    r6, {r6,r9}          @ MX3*
    smulbb   r1, r6, r4           @ MX31 * vx
    smultt   r2, r6, r4           @ MX32 * vy
    smulbb   r3, r9, r5           @ MX33 * vz
    qadd     r1, r1, r2
    asr      r3, #1
    add      r1, r3, r1, asr #1
    asr      r9, r1, #11
    add      r1, r0, #4*25
    mov      r2, #0
    stmia    r1, {r7-r9}
    str      r2, [r0, #4*(32+31)] @ gteFLAG
    pop      {r8-r9}
    bx       lr
    .size    gteMVMVA_part_cv3sh12_arm, .-gteMVMVA_part_cv3sh12_arm

#endif /* HAVE_ARMV5 */

FUNCTION(gteNCLIP_arm): @ r0=CP2 (d,c),
    push        {r4-r6,lr}
    ldrsh       r4, [r0, #4*12+2]
    ldrsh       r5, [r0, #4*13+2]
    ldrsh       r6, [r0, #4*14+2]
    ldrsh       lr, [r0, #4*12]
    ldrsh       r2, [r0, #4*13]
    sub         r12, r4, r5       @ 3: gteSY0 - gteSY1
    sub         r5, r5, r6        @ 1: gteSY1 - gteSY2
    smull       r1, r5, lr, r5    @ RdLo, RdHi
    sub         r6, r4            @ 2: gteSY2 - gteSY0
    ldrsh       r3, [r0, #4*14]
    smlal       r1, r5, r2, r6
    mov         lr, #0            @ gteFLAG
    smlal       r1, r5, r3, r12
    mov         r6, #1<<31
    orr         r6, #1<<15
    movs        r2, r1, lsl #1
    adc         r5, r5
    cmp         r5, #0
#ifdef HAVE_ARMV7
    movtgt      lr, #((1<<31)|(1<<16))>>16
#else
    movgt       lr, #(1<<31)
    orrgt       lr, #(1<<16)
#endif
    cmn         r5, #1
    orrmi       lr, r6
    str         r1, [r0, #4*24]
    str         lr, [r0, #4*(32+31)] @ gteFLAG

    pop         {r4-r6,pc}
    .size	gteNCLIP_arm, .-gteNCLIP_arm


.macro gteMACtoIR lm
    ldr      r2, [r0, #4*25]      @ gteMAC1
    mov      r1, #1<<15
    ldr      r12,[r0, #4*(32+31)] @ gteFLAG
    cmp      r2, r1
    subge    r2, r1, #1
    orrge    r12, #(1<<31)|(1<<24)
.if \lm
    cmp      r2, #0
    movlt    r2, #0
.else
    cmn      r2, r1
    rsblt    r2, r1, #0
.endif
    str      r2, [r0, #4*9]
#ifdef HAVE_ARMV5
    ldrd     r2, r3, [r0, #4*26]  @ gteMAC23
#else
    ldr      r2, [r0, #4*26]
    ldr      r3, [r0, #4*27]
#endif
    orrlt    r12, #(1<<31)|(1<<24)
    cmp      r2, r1
    subge    r2, r1, #1
    orrge    r12, #1<<23
    orrge    r12, #1<<31
.if \lm
    cmp      r2, #0
    movlt    r2, #0
.else
    cmn      r2, r1
    rsblt    r2, r1, #0
.endif
    orrlt    r12, #1<<23
    orrlt    r12, #1<<31
    cmp      r3, r1
    subge    r3, r1, #1
    orrge    r12, #1<<22
.if \lm
    cmp      r3, #0
    movlt    r3, #0
.else
    cmn      r3, r1
    rsblt    r3, r1, #0
.endif
    orrlt    r12, #1<<22
#ifdef HAVE_ARMV5
    strd     r2, r3, [r0, #4*10]  @ gteIR23
#else
    str      r2, [r0, #4*10]
    str      r3, [r0, #4*11]
#endif
    str      r12,[r0, #4*(32+31)] @ gteFLAG
    bx       lr
.endm

FUNCTION(gteMACtoIR_lm0): @ r0=CP2 (d,c)
    gteMACtoIR 0
    .size    gteMACtoIR_lm0, .-gteMACtoIR_lm0

FUNCTION(gteMACtoIR_lm1): @ r0=CP2 (d,c)
    gteMACtoIR 1
    .size    gteMACtoIR_lm1, .-gteMACtoIR_lm1


FUNCTION(gteMACtoIR_lm0_nf): @ r0=CP2 (d,c)
    add      r12, r0, #4*25
    ldmia    r12, {r1-r3}
    ssatx_prep r12, 16
    ssatx    r1, r12, 16
    ssatx    r2, r12, 16
    ssatx    r3, r12, 16
    add      r12, r0, #4*9
    stmia    r12, {r1-r3}
    bx       lr
    .size    gteMACtoIR_lm0_nf, .-gteMACtoIR_lm0_nf


FUNCTION(gteMACtoIR_lm1_nf): @ r0=CP2 (d,c)
    add      r12, r0, #4*25
    ldmia    r12, {r1-r3}
    ssatx0_prep r12, 16
    ssatx0   r1, r12, 16
    ssatx0   r2, r12, 16
    ssatx0   r3, r12, 16
    add      r12, r0, #4*9
    stmia    r12, {r1-r3}
    bx       lr
    .size    gteMACtoIR_lm1_nf, .-gteMACtoIR_lm1_nf


.if 0
FUNCTION(gteMVMVA_test):
    push     {r4-r7,lr}
    push     {r1}
    and      r2, r1, #0x18000     @ v
    cmp      r2, #0x18000         @ v == 3?
    addeq    r4, r0, #4*9
    addne    r3, r0, r2, lsr #12
    ldmeqia  r4, {r3-r5}
    ldmneia  r3, {r4,r5}
    lsleq    r3, #16
    lsreq    r3, #16
    orreq    r4, r3, r4, lsl #16  @ r4,r5 = VXYZ(v)
    @and     r5, #0xffff
    add      r12, r0, #4*32
    and      r3, r1, #0x60000 @ mx
    lsr      r3, #17
    add      r6, r12, r3, lsl #5
    cmp      r3, #3
    adreq    r6, zeroes
    and      r2, r1, #0x06000 @ cv
    lsr      r2, #13
    add      r7, r12, r2, lsl #5
    add      r7, #4*5
    cmp      r2, #3
    adreq    r7, zeroes
.if 1
    adr      lr, 1f
    bne      0f
    tst      r1, #1<<19
    bne      gteMVMVA_part_cv3sh12_arm
0:
    and      r1, #1<<19
    lsr      r1, #19
    b        gteMVMVA_part_arm
1:
    pop      {r1}
    tst      r1, #1<<10
    adr      lr, 0f
    beq      gteMACtoIR_lm0
    bne      gteMACtoIR_lm1
0:
.else
    bl       gteMVMVA_part_neon
    pop      {r1}
    and      r1, #1<<10
    bl       gteMACtoIR_flags_neon
.endif
    pop      {r4-r7,pc}

zeroes:
    .word 0,0,0,0,0
.endif


@ vim:filetype=armasm

