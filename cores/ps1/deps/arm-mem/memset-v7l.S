/*
Copyright (c) 2018, RISC OS Open Ltd
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
 *  void *memset(void *s, int c, size_t n);
 *  On entry:
 *  a1 = pointer to buffer to fill
 *  a2 = byte pattern to fill with (caller-narrowed)
 *  a3 = number of bytes to fill
 *  On exit:
 *  a1 preserved
 */
myfunc memset
        SJ      .req    a2
        N       .req    a3
        SI      .req    a4
        OFF     .req    ip

        mov     SI, a1
        vdup.8  q0, a2
        cmp     N, #15+64
        vdup.8  q1, a2
        blo     170f

161:    ands    ip, a1, #15
        beq     164f
        rsb     ip, ip, #16   /* number of leading bytes until 16-byte aligned */
        sub     N, N, ip
        rbit    ip, ip
        tst     a1, #1        /* bit 0 unaffected by rsb so can avoid register interlock */
        strneb  a2, [SI], #1
        movs    ip, ip, lsl #2
        strcsb  a2, [SI, #1]
        strcsb  a2, [SI], #2
        vstmmi  SI!, {s0}
        movs    ip, ip, lsl #2
        vstmcs  SI!, {d0}
164:    /* Setup for the inner loop */
        mov     OFF, #64
        sub     N, N, #64     /* simplifies inner loop termination */
        add     SJ, SI, #32
        /* Now the inner loop of 2x32-byte stores */
165:    vst1.8  {q0-q1}, [SI :128], OFF
        subs    N, N, #64
        vst1.8  {q0-q1}, [SJ :128], OFF
        bhs     165b
        /* Trailing words and bytes */
166:    vmov.32 a2, d0[0]
        movs    N, N, lsl #27
        bcc     167f
        vst1.8  {q0-q1}, [SI]!
167:    bpl     168f
        vst1.8  {q0}, [SI]!
168:    movs    N, N, lsl #2
        vstmcs  SI!, {d0}
        strmi   a2, [SI], #4
        movs    N, N, lsl #2
        strcsh  a2, [SI], #2
        strmib  a2, [SI]
199:    bx      lr

170:    /* Short case */
        tst     SI, #3
        beq     174f
172:    subs    N, N, #1
        blo     199b
        strb    a2, [SI], #1
        tst     SI, #3
        bne     172b
174:    cmp     N, #32
        bcc     166b
        vst1.8  {q0-q1}, [SI]!
        sub     N, N, #32
        b       166b
        .size memset,.-memset

        .unreq  SJ
        .unreq  N
        .unreq  SI
        .unreq  OFF
.endfunc
