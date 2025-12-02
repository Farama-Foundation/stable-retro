/*
Copyright (c) 2019, RISC OS Open Ltd
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

    .altmacro

/* Load 32 bytes from both buffers (8-byte aligned) post-incrementing the pointers
 * r0q-r1q are unused, but retained so we have identical parameters to load_32b_x2_unaligned
 * r0d-r3d are filled with data from S_1
 * r4d-r7d are filled with data from S_2
 * switch_loads indicates that we should re-order the loads to assist with scheduling a following pld
 * I1-I8 are optional instructions to insert into stalls
 */
.macro load_32b_x2_aligned r0q, r1q, r0d, r1d, r2d, r3d, r4d, r5d, r6d, r7d, switch_loads, I1, I2, I3, I4, I5, I6, I7, I8
 .if switch_loads == 1
        vld1.32     {\r4d}, [S_2 :64]!
        \I1
        vld1.32     {\r0d}, [S_1 :64]!
        \I2
        vld1.32     {\r5d}, [S_2 :64]!
        \I3
        vld1.32     {\r1d}, [S_1 :64]!
        \I4
        vld1.32     {\r6d}, [S_2 :64]!
        \I5
        vld1.32     {\r2d}, [S_1 :64]!
        \I6
        vld1.32     {\r7d}, [S_2 :64]!
        \I7
        vld1.32     {\r3d}, [S_1 :64]!
        \I8
 .else
        vld1.32     {\r0d}, [S_1 :64]!
        \I1
        vld1.32     {\r4d}, [S_2 :64]!
        \I2
        vld1.32     {\r1d}, [S_1 :64]!
        \I3
        vld1.32     {\r5d}, [S_2 :64]!
        \I4
        vld1.32     {\r2d}, [S_1 :64]!
        \I5
        vld1.32     {\r6d}, [S_2 :64]!
        \I6
        vld1.32     {\r3d}, [S_1 :64]!
        \I7
        vld1.32     {\r7d}, [S_2 :64]!
        \I8
 .endif
.endm

/* Load 32 bytes from both buffers (S_1 rounded up to 8-byte boundary, S_2 8-byte aligned), post-incrementing the pointers
 * S_1A, S_2A are 8 bytes on from S_1, S_2
 * SIXTEEN is constant #16
 * r0q-r1q are Q-reg names for r0d-r3d
 * r0d-r3d are filled with data from S_1
 * r4d-r7d are filled with data from S_2
 * switch_loads is ignored in this case
 * I1-I8 are optional instructions to insert into stalls
 * d2-d6 are used as temporaries
 * d7 on entry and exit holds the content of aligned 8-byte block containing "true" value of S_1
 * d8.u8[0] = - ((("true" S_1) & 7) * 8)
 * d9.u8[0] = 64 + d8.u8[0]
 */
.macro load_32b_x2_unaligned r0q, r1q, r0d, r1d, r2d, r3d, r4d, r5d, r6d, r7d, switch_loads, I1, I2, I3, I4, I5, I6, I7, I8
       vld1.32     {d4}, [S_1 :64], SIXTEEN
       \I1
       vld1.32     {d5}, [S_1A :64], SIXTEEN
       vshl.u64    \r0d, d7, d8
       vld1.32     {d6}, [S_1 :64], SIXTEEN
       \I2
       vld1.32     {d7}, [S_1A :64], SIXTEEN
       vshl.u64    d2, d4, d9
       vld1.32     {\r4d}, [S_2 :64], SIXTEEN
       vshl.u64    \r1d, d4, d8
       vld1.32     {\r5d}, [S_2A :64], SIXTEEN
       vshl.u64    d3, d5, d9
       vld1.32     {\r6d}, [S_2 :64], SIXTEEN
       vshl.u64    \r2d, d5, d8
       vld1.32     {\r7d}, [S_2A :64], SIXTEEN
       vshl.u64    d4, d6, d9
       vshl.u64    \r3d, d6, d8
       vshl.u64    d5, d7, d9
       vorr        \r0q, q1
       \I8
       \I3
       \I4
       \I5
       \I6
       \I7
       vorr        \r1q, q2
.endm

.macro process_32b_blocks load_macro
        // Process these as an odd number of 32-byte full blocks,
        // then a partial block of up to 63 trailing bytes
        cmp         N, #32
        sub         N, #64
        bmi         20f
        \load_macro q8, q9, d16, d17, d18, d19, d20, d21, d22, d23, 0
        veor.u8     q0, q8, q10
        subs        N, #32
        veor.u8     q1, q9, q11
        bmi         9f
1:      \load_macro q12, q13, d24, d25, d26, d27, d28, d29, d30, d31, 0, \
           <vorr        d0, d2>,       \
           <vorr        d1, d3>,       \
           <vorr        d0, d1>,       \
           <vmov        TMP1, s0>,     \
           <vmov        TMP2, s1>,     \
           <veor.u8     d0, d24, d28>, \
           <veor.u8     d1, d25, d29>, \
           <pld         [S_1, #prefetch_distance]>
        orrs        RES, TMP1, TMP2
        veor.u8     q1, q13, q15
        bne         33f
        \load_macro q8, q9, d16, d17, d18, d19, d20, d21, d22, d23, 1, \
           <vorr        d0, d2>,       \
           <vorr        d1, d3>,       \
           <vorr        d0, d1>,       \
           <vmov        TMP1, s0>,     \
           <vmov        TMP2, s1>,     \
           <veor.u8     d0, d16, d20>, \
           <veor.u8     d1, d17, d21>, \
           <pld         [S_2, #prefetch_distance]>
        orrs        RES, TMP1, TMP2
        veor.u8     q1, q9, q11
        bne         31f
        subs        N, #64
        bpl         1b
9:      vorr        q0, q1
        vorr        d0, d1
        vmov        TMP1, s0
        vmov        TMP2, s1
        orrs        RES, TMP1, TMP2
        bne         33f
10:     tst         N, #32
        beq         14f
        \load_macro q8, q9, d16, d17, d18, d19, d20, d21, d22, d23, 0
        veor.u8     q0, q8, q10
        veor.u8     q1, q9, q11
        vorr        q0, q1
        vorr        d0, d1
        vmov        TMP1, s0
        vmov        TMP2, s1
        orrs        RES, TMP1, TMP2
        bne         33f
14:
.endm

/*
 * int memcmp(const void *s1, const void *s2, size_t n);
 * On entry:
 * a1 = pointer to buffer 1
 * a2 = pointer to buffer 2
 * a3 = number of bytes to compare (as unsigned chars)
 * On exit:
 * a1 = >0/=0/<0 if s1 >/=/< s2
 */

.set prefetch_distance, 63

myfunc memcmp
        RES     .req    a1
        S_2     .req    a2
        N       .req    a3
        S_1     .req    a4
        S_1A    .req    v1
        S_2A    .req    v2
        SIXTEEN .req    v3
        TMP1    .req    ip
        TMP2    .req    lr

        // Based on real-world data, we are actually very likely to find a
        // difference within the first few bytes, so it's unlikely to be
        // beneficial to vectorise these. Test first 1+ bytes individually,
        // stopping when we have at least the s2 pointer 8-byte aligned.
        mov         S_1, a1
        and         RES, S_2, #7
        push        {lr}
        rsb         RES, #7
        subs        N, #1
        ldrcsb      TMP2, [S_2], #1
        ldrcsb      TMP1, [S_1], #1
        bcc         43f
        cmp         RES, N
        movcs       RES, N
        teq         RES, #0
        beq         9f
        sub         N, RES
1:      cmp         TMP1, TMP2
        ldrb        TMP1, [S_1], #1
        bne         41f
        ldrb        TMP2, [S_2], #1
        subs        RES, #1
        bne         1b
9:      cmp         TMP1, TMP2
        bne         41f
        teq         N, #0
        beq         43f // because it's very common to have found a match by now

        tst         S_1, #7
        bne         50f

        // Both aligned
        process_32b_blocks load_32b_x2_aligned
        lsls        N, #32-5
        beq         43f
        bpl         15f
        vld1.32     {d16}, [S_1 :64]!
        vld1.32     {d20}, [S_2 :64]!
        vld1.32     {d17}, [S_1 :64]!
        vld1.32     {d21}, [S_2 :64]!
15:     lsls        N, #2
        bcc         16f
        vld1.32     {d18}, [S_1 :64]!
        vld1.32     {d22}, [S_2 :64]!
16:     bpl         17f
        vld1.32     {d19[0]}, [S_1 :32]!
        vld1.32     {d23[0]}, [S_2 :32]!
17:     lsls        N, #2
        bcc         18f
        vld1.16     {d19[2]}, [S_1 :16]!
        vld1.16     {d23[2]}, [S_2 :16]!
18:     bpl         19f
        vld1.8      {d19[6]}, [S_1]!
        vld1.8      {d23[6]}, [S_2]!
19:     veor.u8     q0, q8, q10
        veor.u8     q1, q9, q11
        vorr        q0, q1
        vorr        d0, d1
        vmov        TMP1, s0
        vmov        TMP2, s1
        orrs        RES, TMP1, TMP2
        bne         33f
        pop         {pc}

20:     // Make both banks match so the holes between loads won't affect result
        vmov        q8, q10
        vmov        q9, q11
        b           10b

31:     // Diff found in q12-q15
        push        {v1,v2}
        vrev32.8    q0, q12
        vrev32.8    q1, q14
        vmov        a1, a2, d0
        vmov        a3, a4, d2
        vmov        v1, v2, d1
        vmov        ip, lr, d3
        cmp         a3, a1
        vrev32.8    q0, q13
        cmpeq       a4, a2
        vrev32.8    q1, q15
        cmpeq       ip, v1
        vmov        a1, a2, d0
        cmpeq       lr, v2
        vmov        a3, a4, d2
        movne       RES, #1
        vmov        v1, v2, d1
        bne         32f
        vmov        ip, lr, d3
        cmp         a3, a1
        cmpeq       a4, a2
        mov         RES, #1
        cmpeq       ip, v1
        cmpeq       lr, v2
32:     subcs       RES, #2
        pop         {v1,v2,pc}

33:     // Diff found in q8-q11
        push        {v1,v2}
        vrev32.8    q0, q8
        vrev32.8    q1, q10
        vmov        a1, a2, d0
        vmov        a3, a4, d2
        vmov        v1, v2, d1
        vmov        ip, lr, d3
        cmp         a3, a1
        vrev32.8    q0, q9
        cmpeq       a4, a2
        vrev32.8    q1, q11
        cmpeq       ip, v1
        vmov        a1, a2, d0
        cmpeq       lr, v2
        vmov        a3, a4, d2
        movne       RES, #1
        vmov        v1, v2, d1
        bne         34f
        vmov        ip, lr, d3
        cmp         a3, a1
        cmpeq       a4, a2
        mov         RES, #1
        cmpeq       ip, v1
        cmpeq       lr, v2
34:     subcs       RES, #2
        pop         {v1,v2,pc}

41:     movcc       RES, #-1
        movcs       RES, #1
        pop         {pc}

43:     mov         RES, #0
        pop         {pc}


50:     // Only S_2 is aligned
        push        {v1-v3}
        and         v3, S_1, #7
        bic         S_1, #7
        add         S_1A, S_1, #16
        add         S_2A, S_2, #8
        vpush       {q4}
        lsl         v3, #3
        rsb         v3, #0
        vld1.32     {d7}, [S_1 :64]!
        vmov        s16, v3
        add         v3, #64
        vmov        s18, v3
        mov         SIXTEEN, #16
        process_32b_blocks load_32b_x2_unaligned
        lsls        N, #32-5
        beq         43f
        // Reapply the offset to S_1 and use unaligned loads from here on
        vmov        TMP1, s16
        sub         S_1, #8
        sub         S_1, TMP1, asr #3
        bpl         15f
        vld1.32     {d16}, [S_1]!
        vld1.32     {d20}, [S_2 :64]!
        vld1.32     {d17}, [S_1]!
        vld1.32     {d21}, [S_2 :64]!
15:     lsls        N, #2
        bcc         16f
        vld1.32     {d18}, [S_1]!
        vld1.32     {d22}, [S_2 :64]!
16:     bpl         17f
        vld1.32     {d19[0]}, [S_1]!
        vld1.32     {d23[0]}, [S_2 :32]!
17:     lsls        N, #2
        bcc         18f
        vld1.16     {d19[2]}, [S_1]!
        vld1.16     {d23[2]}, [S_2 :16]!
18:     bpl         19f
        vld1.8      {d19[6]}, [S_1]!
        vld1.8      {d23[6]}, [S_2]!
19:     veor.u8     q0, q8, q10
        veor.u8     q1, q9, q11
        vorr        q0, q1
        vorr        d0, d1
        vmov        TMP1, s0
        vmov        TMP2, s1
        orrs        RES, TMP1, TMP2
        bne         33f
        vpop        {q4}
        pop         {v1-v3,pc}

20:     // Make both banks match so the holes between loads won't affect result
        vmov        q8, q10
        vmov        q9, q11
        b           10b

31:     // Diff found in q12-q15
        vrev32.8    q0, q12
        vrev32.8    q1, q14
        vmov        a1, a2, d0
        vmov        a3, a4, d2
        vmov        v1, v2, d1
        vmov        ip, lr, d3
        cmp         a3, a1
        vrev32.8    q0, q13
        cmpeq       a4, a2
        vrev32.8    q1, q15
        cmpeq       ip, v1
        vmov        a1, a2, d0
        cmpeq       lr, v2
        vmov        a3, a4, d2
        movne       RES, #1
        vmov        v1, v2, d1
        bne         32f
        vmov        ip, lr, d3
        cmp         a3, a1
        cmpeq       a4, a2
        mov         RES, #1
        cmpeq       ip, v1
        cmpeq       lr, v2
32:     vpop        {q4}
        subcs       RES, #2
        pop         {v1-v3,pc}

33:     // Diff found in q8-q11
        vrev32.8    q0, q8
        vrev32.8    q1, q10
        vmov        a1, a2, d0
        vmov        a3, a4, d2
        vmov        v1, v2, d1
        vmov        ip, lr, d3
        cmp         a3, a1
        vrev32.8    q0, q9
        cmpeq       a4, a2
        vrev32.8    q1, q11
        cmpeq       ip, v1
        vmov        a1, a2, d0
        cmpeq       lr, v2
        vmov        a3, a4, d2
        movne       RES, #1
        vmov        v1, v2, d1
        bne         34f
        vmov        ip, lr, d3
        cmp         a3, a1
        cmpeq       a4, a2
        mov         RES, #1
        cmpeq       ip, v1
        cmpeq       lr, v2
34:     vpop        {q4}
        subcs       RES, #2
        pop         {v1-v3,pc}

43:     vpop        {q4}
        mov         RES, #0
        pop         {v1-v3,pc}
        .size memcmp,.-memcmp
