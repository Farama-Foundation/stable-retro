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

/*
 *  size_t strlen (const char *__s);
 *  On entry:
 *  a1 = pointer to string
 *  On exit:
 *  a1 = length of string, exclusing terminator
 */
myfunc strlen
        PTR     .req    a1
        OPTR    .req    a2
        MASK    .req    a3
        TMP0    .req    a4
        TMP1    .req    v1
        TMP2    .req    ip
        TMP3    .req    lr

        push        {v1,lr}
        mov         OPTR, PTR
        movw        MASK, #0xff8
        tst         PTR, #7
        bne         20f
        bics        TMP0, MASK, PTR
        beq         20f

10:     /* Handle 16 SIMD bytes per iteration until we hit a load that crosses a page boundary */
        /* Loop rotated so that termination test is in what would otherwise be a stall */
        vld1.8      {d0,d1}, [PTR :64]!
        bics        TMP0, MASK, PTR
        beq         12f
11:     vceq.i8     d0, #0
        vceq.i8     d1, #0
        vmov        TMP0, s0
        vmov        TMP1, s1
        vmov        TMP2, s2
        vmov        TMP3, s3
        teq         TMP0, #0
        teqeq       TMP1, #0
        teqeq       TMP2, #0
        teqeq       TMP3, #0
        bne         33f
        vld1.8      {d0,d1}, [PTR :64]!
        bics        TMP0, MASK, PTR
        bne         11b
12:     vceq.i8     d0, #0
        vceq.i8     d1, #0
        vmov        TMP0, s0
        vmov        TMP1, s1
        vmov        TMP2, s2
        vmov        TMP3, s3
        teq         TMP0, #0
        teqeq       TMP1, #0
        teqeq       TMP2, #0
        teqeq       TMP3, #0
        bne         33f
        /* Drop through... */

20:     /* Handle one byte per iteration, for leading unaligned bytes or when approaching a page boundary */
        ldrb        TMP0, [PTR], #1
21:     tst         PTR, #7
        beq         22f
        teq         TMP0, #0
        beq         23f
        ldrb        TMP0, [PTR], #1
        b           21b

22:     teq         TMP0, #0
        beq         23f
        bics        TMP0, MASK, PTR
        bne         10b
        b           20b

23:     /* Terminating null found during single-byte iteration */
        sub         a1, PTR, OPTR
        sub         a1, #1
        pop         {v1,pc}

30:     /* Terminating null found within TMP0 during SIMD iteration */
        rev         TMP0, TMP0
        clz         TMP0, TMP0
        sub         a1, PTR, OPTR
        sub         a1, #16
        add         a1, TMP0, lsr #3
        pop         {v1,pc}

31:     /* Terminating null found within TMP1 during SIMD iteration */
        rev         TMP1, TMP1
        clz         TMP1, TMP1
        sub         a1, PTR, OPTR
        sub         a1, #12
        add         a1, TMP1, lsr #3
        pop         {v1,pc}

32:     /* Terminating null found within TMP2 during SIMD iteration */
        rev         TMP2, TMP2
        clz         TMP2, TMP2
        sub         a1, PTR, OPTR
        sub         a1, #8
        add         a1, TMP2, lsr #3
        pop         {v1,pc}

33:     teq         TMP0, #0
        bne         30b
        teq         TMP1, #0
        bne         31b
        teq         TMP2, #0
        bne         32b

        /* Terminating null found within TMP3 during SIMD iteration */
        rev         TMP3, TMP3
        clz         TMP3, TMP3
        sub         a1, PTR, OPTR
        sub         a1, #4
        add         a1, TMP3, lsr #3
        pop         {v1,pc}
        .size strlen,.-strlen
