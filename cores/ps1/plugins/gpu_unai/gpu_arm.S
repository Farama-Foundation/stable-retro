/*
 * (C) Gražvydas "notaz" Ignotas, 2011,2024
 *
 * This work is licensed under the terms of  GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "arm_features.h"

.syntax unified
.text
.align 2

.macro pld_ reg offs=#0
#ifdef HAVE_ARMV6
    pld      [\reg, \offs]
#endif
.endm

#ifdef HAVE_ARMV6

.macro modulate rp mbr mg t0 t1 t2
    and     \t0, \rp, #0x001f
    and     \t1, \rp, #0x03e0
    and     \t2, \rp, #0x7c00
    smulbb  \t0, \t0, \mbr       @ -> 0000 0000 0000 orrr  rrxx xxxx xxxx xxxx
    smulbt  \t1, \t1, \mg        @ -> 0000 000o gggg gxxx  xxxx xxxx xxx0 0000
    smulbt  \t2, \t2, \mbr       @ -> 00ob bbbb xxxx xxxx  xxxx xx00 0000 0000
    and     \rp, \rp, #0x8000    @ retain msb
    usat    \t0, #5, \t0, asr #14
    usat    \t1, #5, \t1, asr #19
    usat    \t2, #5, \t2, asr #24
    orr     \rp, \rp, \t0
    orr     \rp, \rp, \t1, lsl #5
    orr     \rp, \rp, \t2, lsl #10
.endm

@ http://www.slack.net/~ant/info/rgb_mixing.html
@ p0 = (p0 + p1) / 2; p1 |= 0x8000
@ msb of input p0 is assumed to be set
.macro semitrans0 p0 p1 t
    eor     \t,  \p0, \p1
    and     \t,  \t,  #0x0420
    sub     \p0, \p0, \t
    orr     \p1, \p1, #0x8000
    uhadd16 \p0, \p0, \p1
.endm

.macro semitrans0p p0 p1 m421 t
    eor     \t,  \p0, \p1
    and     \t,  \t,  \m421
    add     \p0, \p0, \p1
    uhsub16 \p0, \p0, \t           @ sub because of borrow into hi16
.endm

@ p0 - {p1|r,g,b}   // p1* - premasked rgb
.macro semitrans2p p0 p1r p1g p1b m1f t0 t1
    and     \t0, \p0, \m1f
    and     \t1, \p0, \m1f, lsl #5
    and     \p0, \p0, \m1f, lsl #10
    uqsub16 \t0, \t0, \p1r
    uqsub16 \t1, \t1, \p1g
    uqsub16 \p0, \p0, \p1b
    orr     \t0, \t0, \t1
    orr     \p0, \p0, \t0
.endm

#else

@ msb of input p0 is assumed to be set
.macro semitrans0 p0 p1 t
    eor     \t,  \p0, \p1
    and     \t,  \t,  #0x0420
    orr     \p1, \p1, #0x8000
    sub     \p0, \p0, \t
    add     \p0, \p0, \p1
    orr     \p0, \p0, #0x10000
    mov     \p0, \p0, lsr #1
.endm

.macro semitrans0p p0 p1 m421 t
    eor     \t,  \p0, \p1
    and     \t,  \t,  \m421
    add     \p0, \p0, \p1
    sub     \p0, \p0, \t
    mov     \p0, \p0, lsr #1
.endm

#endif // HAVE_ARMV6

.macro semitrans13p p0 p1 m421 t0
    add     \t0, \p0, \p1
    eor     \p0, \p0, \p1
    and     \p0, \p0, \m421          @ low_bits
    sub     \p0, \t0, \p0
    and     \p0, \p0, \m421, lsl #5  @ carries
    sub     \t0, \t0, \p0            @ modulo
    sub     \p0, \p0, \p0, lsr #5    @ clamp
    orr     \p0, \t0, \p0
.endm


@ in: r0=dst, r2=pal, r12=0x1e
@ trashes r6-r8,lr,flags
.macro do_4x_4bpp rs ibase obase
.if \ibase - 1 < 0
    and     r6, r12, \rs, lsl #1
.else
    and     r6, r12, \rs, lsr #\ibase-1
.endif
    and     r7, r12, \rs, lsr #\ibase+3
    and     r8, r12, \rs, lsr #\ibase+7
    and     lr, r12, \rs, lsr #\ibase+11
    ldrh    r6, [r2, r6]
    ldrh    r7, [r2, r7]
    ldrh    r8, [r2, r8]
    ldrh    lr, [r2, lr]
    tst     r6, r6
    strhne  r6, [r0, #\obase+0]
    tst     r7, r7
    strhne  r7, [r0, #\obase+2]
    tst     r8, r8
    strhne  r8, [r0, #\obase+4]
    tst     lr, lr
    strhne  lr, [r0, #\obase+6]
.endm

@ in: r0=dst, r2=pal, r12=0x1fe
@ loads/stores \rs,r6-r8
.macro do_4x_8bpp rs
    and      r6, r12, \rs, lsl #1
    and      r7, r12, \rs, lsr #7
    and      r8, r12, \rs, lsr #15
    and      \rs,r12, \rs, lsr #23
    ldrh     r6, [r2, r6]
    ldrh     r7, [r2, r7]
    ldrh     r8, [r2, r8]
    ldrh     \rs,[r2, \rs]
    tst      r6, r6
    strhne   r6, [r0, #0]
    tst      r7, r7
    strhne   r7, [r0, #2]
    tst      r8, r8
    strhne   r8, [r0, #4]
    tst      \rs,\rs
    strhne   \rs,[r0, #6]
.endm


@ (void *d, u16 c, u32 cnt, const struct gpu_unai_inner_t *inn)
@ see also poly_untex_st_m
.macro tile_driver_st_m name semit
FUNCTION(\name):
    .cfi_startproc
    stmfd   sp!, {r4-r9,lr}
    .cfi_def_cfa_offset 4*7
    .cfi_rel_offset lr, 4*6
    ldr     r7, [r3, #0x18]        @ y0
    ldr     r8, [r3, #0x1c]        @ y1
.if \semit != 2
    mov     r4, #0x8000
    orr     r4, r4, r4, lsl #16    @ mask 8000
    mov     r6, #0x420
    orr     r6, r6, #1
    orr     r6, r6, r6, lsl #16    @ mask 0421
.endif
.if \semit == 2
    and     r4, r1, #0x03e0
    and     r5, r1, #0x7c00
    and     r1, r1, #0x001f
    orr     r4, r4, r4, lsl #16    @ premasked g
    orr     r5, r5, r5, lsl #16    @ premasked b
    mov     r6, #0x00001f
    orr     r6, #0x1f0000          @ mask
.elseif \semit == 3
    mov     r1, r1, lsr #2
    bic     r1, r1, #(0x0c60>>2)
.endif
    orr     r1, r1, r1, lsl #16
    sub     r3, r8, r7             @ h
    mov     r7, r2                 @ save w
0:
    ldrh    r8, [r0]
    pld_    r0, #2048
    tst     r0, #2
    beq     1f
    sub     r2, #1
.if \semit == 0
    bic     r8, r8, r4
    semitrans0p  r8, r1, r6, lr
.elseif \semit == 1 || \semit == 3
    bic     r8, r8, r4
    semitrans13p r8, r1, r6, lr
.elseif \semit == 2
    semitrans2p  r8, r1, r4, r5, r6, r9, lr
.endif
    strh    r8, [r0], #2
1:
    ldr     r8, [r0]
    pld_    r0, #32
    subs    r2, r2, #2
.if \semit == 0
    bic     r8, r8, r4
    semitrans0p  r8, r1, r6, lr
.elseif \semit == 1 || \semit == 3
    bic     r8, r8, r4
    semitrans13p r8, r1, r6, lr
.elseif \semit == 2
    semitrans2p  r8, r1, r4, r5, r6, r9, lr
.endif
    strpl   r8, [r0], #4
    bpl     1b
2:
    tst     r2, #1
    strhne  r8, [r0], #2
    mov     r2, r7                 @ w
    add     r0, r0, #2048
    sub     r0, r0, r7, lsl #1
    subs    r3, r3, #1
    bgt     0b

    ldmfd   sp!, {r4-r9,pc}
    .cfi_endproc
.endm


tile_driver_st_m tile_driver_st0_asm, 0
tile_driver_st_m tile_driver_st1_asm, 1
tile_driver_st_m tile_driver_st3_asm, 3
#ifdef HAVE_ARMV6
tile_driver_st_m tile_driver_st2_asm, 2
#endif

@ (u16 *d, void *s, u16 *pal, int lines)
sprite_4bpp_x16_asm_:
    ldr     r12,[r3, #0x18]        @ y0
    ldr     r2, [r3, #0x04]        @ pal
    ldr     r3, [r3, #0x1c]        @ y1
    sub     r3, r3, r12
FUNCTION(sprite_4bpp_x16_asm):
    .cfi_startproc
    stmfd   sp!, {r4-r8,lr}
    .cfi_def_cfa_offset 4*6
    .cfi_rel_offset lr, 4*5
    mov     r12, #0x1e

0:
    ldmia   r1, {r4,r5}
    pld_    r1, #2048
    do_4x_4bpp r4, 0,  0
    do_4x_4bpp r4, 16, 8
    do_4x_4bpp r5, 0,  16
    do_4x_4bpp r5, 16, 24
    subs    r3, r3, #1
    add     r0, r0, #2048
    add     r1, r1, #2048
    bgt     0b

    ldmfd   sp!, {r4-r8,pc}
    .cfi_endproc


@
.macro sprite_driver_part1 is8bpp
    stmfd   sp!, {r4-r11,lr}
    .cfi_def_cfa_offset 4*9
    .cfi_rel_offset lr, 4*8
    mov     r12, #0x01e
.if \is8bpp
    orr     r12, r12, #0x1f0   @ mask=0x01fe
.endif
    ldr     r4, [r3, #0x08]    @ u
    ldr     r5, [r3, #0x1c]    @ v1
    ldr     r6, [r3, #0x18]    @ v0
    and     r4, r4, #((8 >> \is8bpp) - 1)
    sub     r5, r5, r6
    sub     r5, r5, #1
    orr     r5, r4, r5, lsl #8 @ ((h-1) << 8) | u0_fraction
    mov     r9, r2             @ saved_w
    mov     r10, r0            @ saved_dst
    mov     r11, r1            @ saved_src
    ldr     r2, [r3, #0x04]    @ pal
11: @ line_loop:
    pld_    r11, #2048
    mov     r0, r10
    mov     r1, r11
    mov     r3, r9
    ands    r6, r5, #(7 >> \is8bpp)
    bne     15f @ fractional_u
12:
    subs    r3, r3, #(8 >> \is8bpp) @ w
    bmi     14f @ fractional_w
.endm
.macro sprite_driver_part2 is8bpp
    cmn     r3, #(8 >> \is8bpp)
    bne     14f @ fractional_w
13: @ eol:
    add     r10, r10, #2048
    add     r11, r11, #2048
    subs    r5, r5, #0x100
    bpl     11b @ line_loop
    ldmfd   sp!, {r4-r11,pc}
14: @ fractional_w:
    ldr     r4, [r1], #4    
    add     r8, r3, #(8 >> \is8bpp)
    mov     r3, #0
    mov     r4, r4, lsl #1
    b       16f @ fractional_loop
15: @ fractional_u:
    bic     r1, r1, #3
    rsb     r8, r6, #(8 >> \is8bpp)
    ldr     r4, [r1], #4    
    cmp     r8, r3
    movgt   r8, r3
    mov     r7, r6, lsl #(2 + \is8bpp)
    sub     r3, r3, r8
    sub     r7, r7, #1
    mov     r4, r4, lsr r7
16: @ fractional_loop:
.endm
.macro sprite_driver_part3
    tst     r3, r3
    beq     13b @ sprd4_eol
    b       12b @ return from fractional_u
.endm

@ (u16 *d, const void *s, int width, const gpu_unai_inner_t *)
FUNCTION(sprite_driver_4bpp_asm):
    .cfi_startproc
    ldr     r12, [r3, #8]      @ u
    mov     r12, r12, lsl #29
    orr     r12, r12, r2       @ w
    cmp     r12, #16
    beq     sprite_4bpp_x16_asm_ @ use specialized aligned x16 version
    sprite_driver_part1 0
0:
    ldr     r4, [r1], #4
    pld_    r1, #28
    do_4x_4bpp r4, 0,  0
    do_4x_4bpp r4, 16, 8
    add     r0, r0, #16
    subs    r3, r3, #8
    bpl     0b
    sprite_driver_part2 0
0:
    and     r7, r12, r4
    mov     r4, r4, lsr #4
    ldrh    r7, [r2, r7]
    add     r0, r0, #2
    tst     r7, r7
    strhne  r7, [r0, #-2]
    subs    r8, r8, #1
    bgt     0b
    sprite_driver_part3
    .cfi_endproc


@ (u16 *d, const void *s, int width, const gpu_unai_inner_t *)
FUNCTION(sprite_driver_8bpp_asm):
    .cfi_startproc
    sprite_driver_part1 1
0:
    ldr     r4, [r1], #4
    pld_    r1, #28
    do_4x_8bpp r4
    add     r0, r0, #8
    subs    r3, r3, #4
    bpl     0b
    sprite_driver_part2 1
0:
    and     r7, r12, r4
    mov     r4, r4, lsr #8
    ldrh    r7, [r2, r7]
    add     r0, r0, #2
    tst     r7, r7
    strhne  r7, [r0, #-2]
    subs    r8, r8, #1
    bgt     0b
    sprite_driver_part3
    .cfi_endproc


@ (u16 *d, const void *s, int width, const gpu_unai_inner_t *)
.macro sprite_driver_l_st name bpp light semit
FUNCTION(\name):
    .cfi_startproc
    stmfd   sp!, {r4-r11,lr}
    .cfi_def_cfa_offset 4*4
    .cfi_rel_offset lr, 4*3
    ldr     r5, [r3, #0x18]    @ y0
    ldr     r7, [r3, #0x1c]    @ y1
    ldr     r8, [r3, #0x20]    @ rbg5
    mov     r6, r2             @ saved_w
    ldr     r2, [r3, #0x04]    @ pal
    ldr     r10,[r3, #0x08]    @ u
    ldr     r11,[r3, #0x10]    @ u_msk
    sub     r5, r7, r5         @ h
    mov     r7, r8, lsl #(8+2) @ 0bbb bb00 0ggg gg00 0rrr rr00 0000 0000
    mov     r8, r8, lsl #(16+2)@ 0ggg gg00 ...
    mov     r3, r11,lsr #10
    orr     r6, r3, r6, lsl #16 @ (w << 16) | u_mask
    mov     r3, r6
    and     r10,r10,r6

3: @ line_loop:
.if \bpp == 4
    add     r9, r1, r10, lsr #1
.elseif \bpp == 8
    add     r9, r1, r10
    pld_    r9, #2048
.endif
0:
.if \bpp == 4
    ldrb    r4, [r1, r10, lsr #1]
.elseif \bpp == 8
    ldrb    r4, [r1, r10]
.endif
    subs    r3, r3, #1<<16
    bmi     1f
.if \bpp == 4
    tst     r10, #1
    movne   r4, r4, lsr #3
    addeq   r4, r4, r4
    and     r4, r4, #0x1e
.elseif \bpp == 8
    add     r4, r4, r4         @ <<= 1
.endif
    ldrsh   r12,[r2, r4]
    add     r10,r10,#1
    and     r10,r10,r6
    add     r0, r0, #2
    tst     r12,r12
    beq     0b
.if \light && \semit != 1
    modulate r12, r7, r8, r4, r9, lr
.endif
.if \semit == 0
    ldrhmi  lr, [r0, #-2]
    strhpl  r12,[r0, #-2]
    bpl     0b
    semitrans0 r12, lr, r9
.elseif \light && \semit == 1
    and     r4,  r12, #0x001f
    and     r9,  r12, #0x03e0
    and     r12, r12, #0x7c00
    ldrhmi  r11, [r0, #-2]
    smulbb  r4,  r4,  r7       @ -> 0000 0000 0000 orrr  rrxx xxxx xxxx xxxx
    smulbt  r9,  r9,  r8       @ -> 0000 000o gggg gxxx  xxxx xxxx xxx0 0000
    smulbt  r12, r12, r7       @ -> 00ob bbbb xxxx xxxx  xxxx xx00 0000 0000
    and     r8,  r11, #0x001f
    and     lr,  r11, #0x03e0
    and     r11, r11, #0x7c00
    addmi   r4,  r4,  r8,  lsl #14
    addmi   r9,  r9,  lr,  lsl #14
    addmi   r12, r12, r11, lsl #14
    usat    r4,  #5,  r4,  asr #14
    usat    r9,  #5,  r9,  asr #19
    usat    r12, #5,  r12, asr #24
    orrmi   r4,  r4,  #0x8000
    orr     r4,  r4,  r9,  lsl #5
    orr     r12, r4,  r12, lsl #10
    mov     r8,  r7,  lsl #8       @ restore r8
.endif
    strh    r12,[r0, #-2]
    b       0b
1:
    add     r0, r0, #2048
    add     r1, r1, #2048
    sub     r0, r0, r6, lsr #15    @ dst
    sub     r10,r10,r6, lsr #16    @ u
    mov     r3, r6                 @ (w << 16) | u_mask
    and     r10,r6, r10
    subs    r5, r5, #1
    and     r10,r10,#0xff
    bgt     3b @ line_loop

    ldmfd   sp!, {r4-r11,pc}
    .cfi_endproc
.endm

sprite_driver_l_st sprite_driver_4bpp_l0_std_asm, 4, 0, -1
sprite_driver_l_st sprite_driver_4bpp_l0_st0_asm, 4, 0,  0
sprite_driver_l_st sprite_driver_8bpp_l0_std_asm, 8, 0, -1
sprite_driver_l_st sprite_driver_8bpp_l0_st0_asm, 8, 0,  0

#ifdef HAVE_ARMV6

sprite_driver_l_st sprite_driver_4bpp_l1_std_asm, 4, 1, -1
sprite_driver_l_st sprite_driver_4bpp_l1_st0_asm, 4, 1,  0
sprite_driver_l_st sprite_driver_4bpp_l1_st1_asm, 4, 1,  1
sprite_driver_l_st sprite_driver_8bpp_l1_std_asm, 8, 1, -1
sprite_driver_l_st sprite_driver_8bpp_l1_st0_asm, 8, 1,  0
sprite_driver_l_st sprite_driver_8bpp_l1_st1_asm, 8, 1,  1

#endif // HAVE_ARMV6


@ (u16 *d, const void *s, int width, const gpu_unai_inner_t *)
FUNCTION(sprite_driver_16bpp_asm):
    .cfi_startproc
    stmfd   sp!, {r4-r6,lr}
    .cfi_def_cfa_offset 4*4
    .cfi_rel_offset lr, 4*3
    ldr     r4, [r3, #0x1c]    @ v1
    ldr     r5, [r3, #0x18]    @ v0
    mov     r12,      #0x00ff
    orr     r12, r12, #0xff00  @ mask
    mov     r6, r2             @ saved_w
    sub     r5, r4, r5
    sub     r5, r5, #1         @ h-1
3: @ line_loop:
    pld_    r1, #2048
    mov     r2, r6             @ w
    tst     r1, #2
    beq     0f
2: @ 1pix:
    ldrh    lr, [r1], #2
    add     r0, r0, #2
    sub     r2, r2, #1
    tst     lr, lr
    strhne  lr, [r0, #-2]
0:
    subs    r2, r2, #4
    bmi     1f
0:
    ldmia   r1!, {r3,r4}
    add     r0, r0, #2*4
    pld_    r1, #24
    tst     r3, r12
    strhne  r3, [r0, #-8]
    movs    lr, r3, lsr #16
    strhne  lr, [r0, #-6]
    tst     r4, r12
    strhne  r4, [r0, #-4]
    movs    lr, r4, lsr #16
    strhne  lr, [r0, #-2]
    subs    r2, r2, #4
    bpl     0b
1:
    adds    r2, r2, #4
    bne     2b @ 1pix
    add     r0, r0, #2048
    add     r1, r1, #2048
    sub     r0, r0, r6, lsl #1 @ dst
    sub     r1, r1, r6, lsl #1
    subs    r5, r5, #1
    bpl     3b @ line_loop

    ldmfd   sp!, {r4-r6,pc}
    .cfi_endproc


@ (void *d, const gpu_unai_inner_t *inn, int count)
@ see also tile_driver_st_m
.macro poly_untex_st_m name semit
FUNCTION(\name):
    .cfi_startproc
    ldrh    r1, [r1, #0x38]        @ rgb
    stmfd   sp!, {r4-r7,lr}
    .cfi_def_cfa_offset 4*5
    .cfi_rel_offset lr, 4*4
.if \semit != 2
    mov     r4, #0x8000
    orr     r4, r4, r4, lsl #16    @ mask 8000
    mov     r6, #0x420
    orr     r6, r6, #1
    orr     r6, r6, r6, lsl #16    @ mask 0421
.endif
.if \semit == 2
    and     r4, r1, #0x03e0
    and     r5, r1, #0x7c00
    and     r1, r1, #0x001f
    orr     r4, r4, r4, lsl #16    @ premasked g
    orr     r5, r5, r5, lsl #16    @ premasked b
    mov     r6, #0x00001f
    orr     r6, #0x1f0000          @ mask
.elseif \semit == 3
    mov     r1, r1, lsr #2
    bic     r1, r1, #(0x0c60>>2)
.endif
    orr     r1, r1, r1, lsl #16
0:
    ldrh    r3, [r0]
    pld_    r0, #2048
    tst     r0, #2
    beq     1f
    sub     r2, #1
.if \semit == 0
    bic     r3, r3, r4
    semitrans0p  r3, r1, r6, lr
.elseif \semit == 1 || \semit == 3
    bic     r3, r3, r4
    semitrans13p r3, r1, r6, lr
.elseif \semit == 2
    semitrans2p  r3, r1, r4, r5, r6, r7, lr
.endif
    strh    r3, [r0], #2
1:
    ldr     r3, [r0]
    pld_    r0, #32
    subs    r2, r2, #2
.if \semit == 0
    bic     r3, r3, r4
    semitrans0p  r3, r1, r6, lr
.elseif \semit == 1 || \semit == 3
    bic     r3, r3, r4
    semitrans13p r3, r1, r6, lr
.elseif \semit == 2
    semitrans2p  r3, r1, r4, r5, r6, r7, lr
.endif
    strpl   r3, [r0], #4
    bpl     1b
2:
    tst     r2, #1
    strhne  r3, [r0], #2

    ldmfd   sp!, {r4-r7,pc}
    .cfi_endproc
.endm

poly_untex_st_m poly_untex_st0_asm, 0
poly_untex_st_m poly_untex_st1_asm, 1
poly_untex_st_m poly_untex_st3_asm, 3
#ifdef HAVE_ARMV6
poly_untex_st_m poly_untex_st2_asm, 2
#endif


.macro poly_4_8bpp_asm_m name bpp light semit
FUNCTION(\name): @ (void *d, const gpu_unai_inner_t *inn, int count)
    .cfi_startproc
    stmfd   sp!, {r4-r11,lr}
    .cfi_def_cfa_offset 4*9
    .cfi_rel_offset lr, 4*8
    add     r12, r1, #4
    ldmia   r12, {r3, r4, r7, r12, lr} @ clut, u, v, u_msk, v_msk
    ldr     r5, [r1, #0x18]    @ u_inc
.if \light
    ldr     r10,[r1, #0x24]    @ rbg
.endif
    mov     r6, r12            @ u_msk
    ldr     r12,[r1, #0x1c]    @ v_inc
.if \light
    mov     r10,r10,lsl #7     @ 0bbb bbbb 0ggg gggg 0rrr rrrr r000 0000
    bic     r10,r10,#1<<23
    bic     r10,r10,#1<<15
    mov     r11,r10,lsl #8     @ 0ggg gggg ...
.endif
    and     r4, r4, r6
    and     lr, lr, r7         @ v_msk & v
    and     lr, lr, #0xff<<10
    tst     r12,r12
    bne     v_\name
    ldr     r1, [r1]           @ src
    mov     r7, r4, lsr #(13 - (\bpp / 8 * 3))
    add     r1, r1, lr, lsl #1
#ifdef HAVE_ARMV6
    add     r12,r1, r7, lsl #(2 - (\bpp / 8 * 2))
    pld_    r12,#2048          @ next line
#endif
0:
.if \light || \semit >= 0
    mov     r7, r4, lsr #(13 - (\bpp / 8 * 3))
    subs    r2, r2, #1
    bmi     1f
.endif
.if \bpp == 4
    ldr     lr, [r1, r7, lsl #2]
    lsr     r12,r4, #8
    and     r12,r12,#0x1c
    sub     r12,r12,#1
    mov     r12,lr, ror r12
    add     r4, r4, r5
    and     r12,r12,#0x1e
.else
    ldrb    r12,[r1, r7]
    add     r4, r4, r5
    add     r12,r12,r12
.endif
    and     r4, r4, r6
    ldrsh   r12,[r3, r12]
    add     r0, r0, #2
.if !\light && \semit < 0
    mov     r7, r4, lsr #(13 - (\bpp / 8 * 3))
    tst     r12,r12
    strhne  r12,[r0, #-2]
    subs    r2, r2, #1
    bgt     0b
    @ end
.else
    tst     r12,r12
    beq     0b
.if \light && \semit != 1
    modulate r12, r10, r11, r7, r8, lr
.endif
.if \semit == 0
    ldrhmi  r7, [r0, #-2]
    strhpl  r12,[r0, #-2]
    bpl     0b
    semitrans0 r12, r7, lr
.endif
    strh    r12,[r0, #-2]
    b       0b
.endif                         @ \light || \semit >= 0
1:
    ldmfd   sp!, {r4-r11,pc}

v_\name: @ r3=clut, r4=u, r5=u_inc, r6=u_msk, r7=v, lr=v_masked
.if \light || \semit >= 0
    sub     sp, sp, #4*2
    stmia   sp, {r5,r6}
    .cfi_def_cfa_offset 4*(9+2)
    .cfi_rel_offset lr, 4*(8+2)
.endif
    ldr     r9, [r1, #0x14]    @ v_msk
    ldr     r1, [r1]           @ src
    mov     r8, r12            @ v_inc
    and     r9, r9, #0xff<<10  @ v_msk_final
.if !\light && \semit < 0
    and     lr, r7, r9
    mov     r12,r4, lsr #(13 - (\bpp / 8 * 3))
    add     lr, r1, lr, lsl #1
.endif
0:
.if \light || \semit >= 0
    and     lr, r7, r9
    mov     r12,r4, lsr #(13 - (\bpp / 8 * 3))
    add     lr, r1, lr, lsl #1
    subs    r2, r2, #1
    bmi     1f
.endif
.if \bpp == 4
    ldr     lr, [lr, r12, lsl #2]
    lsr     r12,r4, #8
    and     r12,r12,#0x1c
    sub     r12,r12,#1
    mov     r12,lr, ror r12
    add     r4, r4, r5
    and     r12,r12,#0x1e
.else
    ldrb    r12,[lr, r12]
    add     r4, r4, r5
    add     r12,r12,r12
.endif
    and     r4, r4, r6
    ldrsh   r12,[r3, r12]
    add     r0, r0, #2
    add     r7, r7, r8
.if !\light && \semit < 0
    and     lr, r7, r9
    tst     r12,r12
    add     lr, r1, lr, lsl #1
    strhne  r12,[r0, #-2]
    mov     r12,r4, lsr #(13 - (\bpp / 8 * 3))
    subs    r2, r2, #1
    bgt     0b
    @ end
.else
    tst     r12,r12
    beq     0b
.if \light && \semit != 1
    modulate r12, r10, r11, r5, r6, lr
.endif
.if \semit == 0
    ldrhmi  r6, [r0, #-2]
    strhpl  r12,[r0, #-2]
    ldmiapl sp, {r5,r6}
    bpl     0b
    semitrans0 r12, r6, lr
.endif
    strh    r12,[r0, #-2]
    ldmia   sp, {r5,r6}
    b       0b
.endif                         @ \light || \semit >= 0
1:
.if \light || \semit >= 0
    add     sp, sp, #4*2
.endif
    ldmfd   sp!, {r4-r11,pc}
    .cfi_endproc
.endm

poly_4_8bpp_asm_m poly_4bpp_asm,        4, 0, -1
poly_4_8bpp_asm_m poly_4bpp_l0_st0_asm, 4, 0,  0
poly_4_8bpp_asm_m poly_8bpp_asm,        8, 0, -1
poly_4_8bpp_asm_m poly_8bpp_l0_st0_asm, 8, 0,  0

#ifdef HAVE_ARMV6

poly_4_8bpp_asm_m poly_4bpp_l1_std_asm, 4, 1, -1
poly_4_8bpp_asm_m poly_4bpp_l1_st0_asm, 4, 1,  0
poly_4_8bpp_asm_m poly_8bpp_l1_std_asm, 8, 1, -1
poly_4_8bpp_asm_m poly_8bpp_l1_st0_asm, 8, 1,  0

#endif // HAVE_ARMV6

@ vim:filetype=armasm
