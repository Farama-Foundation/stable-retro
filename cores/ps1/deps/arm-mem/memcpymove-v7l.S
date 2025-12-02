/*
Copyright (c) 2015, RISC OS Open Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "arm-mem.h"

/* Prevent the stack from becoming executable */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

    .text
    .fpu neon
    .arch armv7a
    .object_arch armv4
    .arm
    .altmacro
    .p2align 2

.macro memcpy_leading_63bytes  backwards, align
        movs    TMP, LEAD, lsl #31
        bpl     1f
 .if backwards
        sub     S, S, #1
        sub     D, D, #1
        vld1.8  {d7[7]}, [S]
        vst1.8  {d7[7]}, [D]
 .else
        vld1.8  {d7[7]}, [S]!
        vst1.8  {d7[7]}, [D]!
 .endif
1:      bcc     1f
 .if backwards
  .if align == 0 || align == 2
        sub     S, S, #2
        sub     D, D, #2
        vld1.16 {d7[3]}, [S :16]
  .else
        sub     S, S, #1
        sub     D, D, #2
        vld1.8  {d7[7]}, [S]
        sub     S, S, #1
        vld1.8  {d7[6]}, [S]
  .endif
        vst1.16 {d7[3]}, [D :16]
 .else
  .if align == 0 || align == 2
        vld1.16 {d7[3]}, [S :16]!
  .else
        vld1.8  {d7[6]}, [S]!
        vld1.8  {d7[7]}, [S]!
  .endif
        vst1.16 {d7[3]}, [D :16]!
 .endif
1:
 .if align == 0
        movs    TMP, LEAD, lsl #29
  .if backwards
        vldmdbmi S!, {s13}
        vldmdbcs S!, {d7}
        vstmdbmi D!, {s13}
        vstmdbcs D!, {d7}
  .else
        vldmiami S!, {s13}
        vldmiacs S!, {d7}
        vstmiami D!, {s13}
        vstmiacs D!, {d7}
  .endif
        movs    TMP, LEAD, lsl #27
  .if backwards
        vldmdbmi S!, {d2-d3}
        vldmdbcs S!, {d4-d7}
        vstmdbmi D!, {d2-d3}
        vstmdbcs D!, {d4-d7}
  .else
        vldmiami S!, {d2-d3}
        vldmiacs S!, {d4-d7}
        vstmiami D!, {d2-d3}
        vstmiacs D!, {d4-d7}
  .endif
 .else
  .if backwards
        add     S, S, #4-align
        vldmdb  S!, {s0}
  .else
        sub     S, S, #align
        vldmia  S!, {s19}
  .endif
        movs    TMP, LEAD, lsl #29
        bpl     1f
  .if backwards
        vmov    s1, s0
        vldmdb  S!, {s0}
        vext.8  d1, d0, d1, #align
        vstmdb  D!, {s2}
  .else
        vmov    s18, s19
        vldmia  S!, {s19}
        vext.8  d8, d9, d10, #align
        vstmia  D!, {s16}
  .endif
1:      bcc     1f
  .if backwards
        vmov    s2, s0
        vldmdb  S!, {d0}
        vext.8  d1, d0, d1, #align
        vstmdb  D!, {d1}
  .else
        vmov    s17, s19
        vldmia  S!, {d9}
        vext.8  d8, d8, d9, #4+align
        vstmia  D!, {d8}
  .endif
1:      movs    TMP, LEAD, lsl #27
        bpl     1f
  .if backwards
        vmov    s4, s0
        vldmdb  S!, {d0-d1}
        vext.8  q1, q0, q1, #align
        vstmdb  D!, {d2-d3}
  .else
        vmov    s15, s19
        vldmia  S!, {d8-d9}
        vext.8  q3, q3, q4, #12+align
        vstmia  D!, {d6-d7}
  .endif
1:      bcc     1f
  .if backwards
        vmov    s8, s0
        vldmdb  S!, {d0-d3}
        vext.8  q2, q1, q2, #align
        vext.8  q1, q0, q1, #align
        vstmdb  D!, {d2-d5}
  .else
        vmov    s11, s19
        vldmia  S!, {d6-d9}
        vext.8  q2, q2, q3, #12+align
        vext.8  q3, q3, q4, #12+align
        vstmia  D!, {d4-d7}
  .endif
1:
 .endif
.endm

.macro memcpy_middle_64bytes  backwards, align, use_pld, add_nops
 .if align == 0
  .if backwards
        vldmdb  S!, {d0-d7}
   .if use_pld
        pld     [S, OFF]
   .endif
        vstmdb  D!, {d0-d7}
  .else
        vldmia  S!, {d0-d7}
   .if add_nops
    .rept 14
        nop
    .endr
   .endif
   .if use_pld
        pld     [S, OFF]
   .endif
        vstmia  D!, {d0-d7}
   .if add_nops
    .rept 7
        nop
    .endr
   .endif
  .endif
 .else
  .if backwards
        vmov    s16, s0
        vldmdb  S!, {d0-d7}
   .if use_pld
        pld     [S, OFF]
   .endif
        vext.8  q4, q3, q4, #align
        vext.8  q3, q2, q3, #align
        vext.8  q2, q1, q2, #align
        vext.8  q1, q0, q1, #align
        vstmdb  D!, {d2-d9}
  .else
        vmov    s3, s19
        vldmia  S!, {d2-d9}
   .if add_nops
    .rept 7
        nop
    .endr
   .endif
   .if use_pld
        pld     [S, OFF]
   .endif
        vext.8  q0, q0, q1, #12+align
        vext.8  q1, q1, q2, #12+align
        vext.8  q2, q2, q3, #12+align
        vext.8  q3, q3, q4, #12+align
   .if add_nops
        nop
        nop
        nop
   .endif
        vstmia  D!, {d0-d7}
   .if add_nops
        nop
        nop
   .endif
  .endif
 .endif
.endm

.macro memcpy_trailing_63bytes  backwards, align
        movs    TMP, N, lsl #27
 .if align == 0
  .if backwards
        vldmdbcs S!, {d4-d7}
        vldmdbmi S!, {d2-d3}
        vstmdbcs D!, {d4-d7}
        vstmdbmi D!, {d2-d3}
  .else
        vldmiacs S!, {d4-d7}
        vldmiami S!, {d2-d3}
        vstmiacs D!, {d4-d7}
        vstmiami D!, {d2-d3}
  .endif
        movs    TMP, N, lsl #29
  .if backwards
        vldmdbcs S!, {d7}
        vldmdbmi S!, {s13}
        vstmdbcs D!, {d7}
        vstmdbmi D!, {s13}
  .else
        vldmiacs S!, {d7}
        vldmiami S!, {s13}
        vstmiacs D!, {d7}
        vstmiami D!, {s13}
  .endif
 .else
        bcc     1f
  .if backwards
        vmov    s8, s0
        vldmdb  S!, {d0-d3}
        vext.8  q2, q1, q2, #align
        vext.8  q1, q0, q1, #align
        vstmdb  D!, {d2-d5}
  .else
        vmov    s11, s19
        vldmia  S!, {d6-d9}
        vext.8  q2, q2, q3, #12+align
        vext.8  q3, q3, q4, #12+align
        vstmia  D!, {d4-d7}
  .endif
1:      bpl     1f
  .if backwards
        vmov    s4, s0
        vldmdb  S!, {d0-d1}
        vext.8  q1, q0, q1, #align
        vstmdb  D!, {d2-d3}
  .else
        vmov    s15, s19
        vldmia  S!, {d8-d9}
        vext.8  q3, q3, q4, #12+align
        vstmia  D!, {d6-d7}
  .endif
1:      movs    TMP, N, lsl #29
        bcc     1f
  .if backwards
        vmov    s2, s0
        vldmdb  S!, {d0}
        vext.8  d1, d0, d1, #align
        vstmdb  D!, {d1}
  .else
        vmov    s17, s19
        vldmia  S!, {d9}
        vext.8  d8, d8, d9, #4+align
        vstmia  D!, {d8}
  .endif
1:      bpl     1f
  .if backwards
        vmov    s1, s0
        vldmdb  S!, {s0}
        vext.8  d1, d0, d1, #align
        vstmdb  D!, {s2}
1:      add     S, S, #align
  .else
        vmov    s18, s19
        vldmia  S!, {s19}
        vext.8  d8, d9, d10, #align
        vstmia  D!, {s16}
1:      sub     S, S, #4-align
  .endif
 .endif
        movs    TMP, N, lsl #31
        bcc     1f
 .if backwards
  .if align == 0 || align == 2
        sub     S, S, #2
        sub     D, D, #2
        vld1.16 {d7[3]}, [S :16]
  .else
        sub     S, S, #1
        sub     D, D, #2
        vld1.8  {d7[7]}, [S]
        sub     S, S, #1
        vld1.8  {d7[6]}, [S]
  .endif
        vst1.16 {d7[3]}, [D :16]
 .else
  .if align == 0 || align == 2
        vld1.16 {d7[3]}, [S :16]!
  .else
        vld1.8  {d7[6]}, [S]!
        vld1.8  {d7[7]}, [S]!
  .endif
        vst1.16 {d7[3]}, [D :16]!
 .endif
1:      bpl     1f
 .if backwards
        sub     S, S, #1
        sub     D, D, #1
        vld1.8  {d7[7]}, [S]
        vst1.8  {d7[7]}, [D]
 .else
        vld1.8  {d7[7]}, [S]!
        vst1.8  {d7[7]}, [D]!
 .endif
1:
.endm

.macro memcpy_long_inner_loop  backwards, align, add_nops
 .if backwards
        /* Bug in GAS: it accepts, but mis-assembles the instruction
         * ands    LEAD, D, #252, 2
         * which sets LEAD to the number of leading bytes until destination is aligned and also clears C (sets borrow)
         */
        .word   0xE210C1FC
        beq     154f
 .else
        ands    LEAD, D, #63
        beq     154f
        rsb     LEAD, LEAD, #64 /* number of leading bytes until destination aligned */
 .endif
        preload_leading_step2  backwards, P, S, 6, LEAD, TMP
        memcpy_leading_63bytes backwards, align
        sub     N, N, LEAD
 .if align != 0
        b       155f
 .endif
154:
 .if align != 0
  .if backwards
        add     S, S, #4-align
        vldmdb  S!, {s0}
  .else
        sub     S, S, #align
        vldmia  S!, {s19}
  .endif
 .endif
155:    /* Destination now 64-byte aligned; we have at least one prefetch as well as at least one 64-byte output block */
        /* Prefetch offset is best selected such that it lies in the first 16 of each 64 bytes - but it's just as easy to aim for the first one */
 .if backwards
        rsb     OFF, S, #0
        and     OFF, OFF, #60
        sub     OFF, OFF, #64*(prefetch_distance+1)
 .else
        and     OFF, S, #60
        rsb     OFF, OFF, #64*prefetch_distance
 .endif
110:    memcpy_middle_64bytes  backwards, align, 1, add_nops
        subs    N, N, #64
        bhs     110b
        /* Just before the final (prefetch_distance+1) 32-byte blocks, deal with final preloads */
        preload_trailing  backwards, S, 6, N, OFF
        add     N, N, #(prefetch_distance+2)*64 - 64
120:    memcpy_middle_64bytes  backwards, align, 0, add_nops
        subs    N, N, #64
        bhs     120b
        /* Trailing words and bytes */
        tst      N, #63
        beq      199f
        memcpy_trailing_63bytes  backwards, align
199:
        vpop    {d8-d9}
        pop     {a1,pc}
.endm

.macro memcpy_medium_inner_loop  backwards, align
 .if backwards
        ands    LEAD, D, #63
        beq     164f
 .else
        ands    LEAD, D, #63
        beq     164f
        rsb     LEAD, LEAD, #64
 .endif
        memcpy_leading_63bytes backwards, align
        sub     N, N, LEAD
 .if align != 0
        b       165f
 .endif
164:
 .if align != 0
  .if backwards
        add     S, S, #4-align
        vldmdb  S!, {s0}
  .else
        sub     S, S, #align
        vldmia  S!, {s19}
  .endif
 .endif
165:    /* Destination now 64-byte aligned */
        subs    N, N, #64
        blo     129f
120:    memcpy_middle_64bytes  backwards, align, 0, 0
        subs    N, N, #64
        bhs     120b
129:    /* Trailing words and bytes */
        tst      N, #63
        beq      199f
        memcpy_trailing_63bytes  backwards, align
199:
        vpop    {d8-d9}
        pop     {a1,pc}
.endm

.macro memcpy_short_inner_loop  backwards, align
 .if align != 0
  .if backwards
        add     S, S, #4-align
        vldmdb  S!, {s0}
  .else
        sub     S, S, #align
        vldmia  S!, {s19}
  .endif
 .endif
        memcpy_trailing_63bytes  backwards, align
199:
        vpop    {d8-d9}
        pop     {a1,pc}
.endm

.macro memcpy backwards
        D       .req    a1
        S       .req    a2
        N       .req    a3
        P       .req    a4
        LEAD    .req    ip
        OFF     .req    ip
        TMP     .req    lr

        .cfi_startproc

        push    {a1,lr}
        vpush   {d8-d9}

        .cfi_def_cfa_offset 16
        .cfi_rel_offset D, 8
        .cfi_undefined  S
        .cfi_undefined  N
        .cfi_undefined  P
        .cfi_undefined  LEAD
        .cfi_rel_offset lr, 12

        add     ip, D, N
        /* See if we cross a 64-byte boundary at the destination */
 .if backwards
        /* Also point S and D at the buffer ends if working downwards */
        eor     D, ip, D
        add     S, S, N
        bics    D, D, #63
        mov     D, ip
        beq     170f
 .else
        eor     ip, ip, D
        bics    ip, ip, #63
        beq     170f
 .endif

        /* To preload ahead as we go, we need at least (prefetch_distance+2) 64-byte blocks */
 .if prefetch_distance > 1
        movw    ip, #(prefetch_distance+3)*64 - 1
        cmp     N, ip
 .else
        cmp     N, #(prefetch_distance+3)*64 - 1
 .endif
        blo     160f

 .if !backwards
        /* If the data is not in the L2 cache, we get up to a 5% speed
         * boost by spacing out the instructions with NOPs. Use data
         * length to estimate whether this is the case. */
        cmp     N, #512*1024 @ L2 cache size for BCM2836 Cortex-A7
        blo     150f

        sub     N, N, #(prefetch_distance+2)*64
        preload_leading_step1  backwards, P, S, 6

        sub     TMP, S, D
        movs    TMP, TMP, lsl #31
        bhi     148f
        bcs     147f
        bmi     146f
        memcpy_long_inner_loop  backwards, 0, 1
146:    memcpy_long_inner_loop  backwards, 1, 1
147:    memcpy_long_inner_loop  backwards, 2, 1
148:    memcpy_long_inner_loop  backwards, 3, 1
 .endif

150:    /* Long case */
        /* Adjust N so that the decrement instruction can also test for
         * inner loop termination. We want it to stop when there are
         * (prefetch_distance+1) complete blocks to go. */
        sub     N, N, #(prefetch_distance+2)*64
        preload_leading_step1  backwards, P, S, 6

        sub     TMP, S, D
        movs    TMP, TMP, lsl #31
        bhi     158f
        bcs     157f
        bmi     156f
        memcpy_long_inner_loop  backwards, 0, 0
156:    memcpy_long_inner_loop  backwards, 1, 0
157:    memcpy_long_inner_loop  backwards, 2, 0
158:    memcpy_long_inner_loop  backwards, 3, 0

160:    /* Medium case */
        preload_all  backwards, 0, 0, S, 6, N, OFF, TMP

        sub     TMP, S, D
        movs    TMP, TMP, lsl #31
        bhi     168f
        bcs     167f
        bmi     166f
        memcpy_medium_inner_loop  backwards, 0
166:    memcpy_medium_inner_loop  backwards, 1
167:    memcpy_medium_inner_loop  backwards, 2
168:    memcpy_medium_inner_loop  backwards, 3

170:    /* Short case, less than 127 bytes, so no guarantee of at least one 64-byte block */
        teq     N, #0
        beq     199f
        preload_all  backwards, 1, 0, S, 6, N, OFF, TMP

        tst     D, #3
        beq     174f
172:    subs    N, N, #1
        blo     199f
 .if backwards
        sub     S, S, #1
        sub     D, D, #1
        vld1.8  {d7[7]}, [S]
        vst1.8  {d7[7]}, [D]
 .else
        vld1.8  {d7[7]}, [S]!
        vst1.8  {d7[7]}, [D]!
 .endif
        tst     D, #3
        bne     172b
174:    /* Destination now 4-byte aligned; we have 1 or more output bytes to go */
        sub     TMP, S, D
        movs    TMP, TMP, lsl #31
        bhi     178f
        bcs     177f
        bmi     176f
        memcpy_short_inner_loop  backwards, 0
176:    memcpy_short_inner_loop  backwards, 1
177:    memcpy_short_inner_loop  backwards, 2
178:    memcpy_short_inner_loop  backwards, 3

        .cfi_endproc

        .unreq  D
        .unreq  S
        .unreq  N
        .unreq  P
        .unreq  LEAD
        .unreq  OFF
        .unreq  TMP
.endm

/*
 * void *memcpy(void * restrict s1, const void * restrict s2, size_t n);
 * On entry:
 * a1 = pointer to destination
 * a2 = pointer to source
 * a3 = number of bytes to copy
 * On exit:
 * a1 preserved
 */

.set prefetch_distance, 2

myfunc memcpy
1000:   memcpy  0
        .size memcpy,.-memcpy
.endfunc

/*
 * void *memmove(void *s1, const void *s2, size_t n);
 * On entry:
 * a1 = pointer to destination
 * a2 = pointer to source
 * a3 = number of bytes to copy
 * On exit:
 * a1 preserved
 */

.set prefetch_distance, 2

myfunc memmove
        cmp     a2, a1
        bpl     1000b   /* pl works even over -1 - 0 and 0x7fffffff - 0x80000000 boundaries */
        memcpy  1
        .size memmove,.-memmove
.endfunc

/*
 * void *mempcpy(void * restrict s1, const void * restrict s2, size_t n);
 * On entry:
 * a1 = pointer to destination
 * a2 = pointer to source
 * a3 = number of bytes to copy
 * On exit:
 * a1 = pointer to immediately after destination block
 */

myfunc mempcpy
.global __mempcpy
.type __mempcpy STT_FUNC
__mempcpy:
        push    {v1, lr}
        mov     v1, a3
        bl      1000b
        add     a1, a1, v1
        pop     {v1, pc}
        .size mempcpy,.-mempcpy
        .size __mempcpy,.-__mempcpy
