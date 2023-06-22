/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - linkage_arm64.S                                         *
 *   Copyright (C) 2009-2011 Ari64                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define GLOBAL_FUNCTION(name)  \
    .align 3;                  \
    .globl name;               \
    .hidden name;              \
    .type name, %function;     \
    name

#define LOCAL_FUNCTION(name)  \
    .align 3;                 \
    .hidden name;             \
    .type name, %function;    \
    name

#define GLOBAL_VARIABLE(name, size_) \
    .global name;                    \
    .hidden name;                    \
    .type   name, %object;           \
    .size   name, size_

#define BSS_SECTION .bss
#define TEXT_SECTION .text
#define END_SECTION

BSS_SECTION

    .align 12
    GLOBAL_VARIABLE(extra_memory, 33554432)
    GLOBAL_VARIABLE(dynarec_local, 256)
    GLOBAL_VARIABLE(next_interrupt, 4)
    GLOBAL_VARIABLE(cycle_count, 4)
    GLOBAL_VARIABLE(last_count, 4)
    GLOBAL_VARIABLE(pending_exception, 4)
    GLOBAL_VARIABLE(pcaddr, 4)
    GLOBAL_VARIABLE(stop, 4)
    GLOBAL_VARIABLE(invc_ptr, 8)
    GLOBAL_VARIABLE(address, 4)
    GLOBAL_VARIABLE(readmem_dword, 8)
    GLOBAL_VARIABLE(cpu_dword, 8)
    GLOBAL_VARIABLE(cpu_word, 4)
    GLOBAL_VARIABLE(cpu_hword, 2)
    GLOBAL_VARIABLE(cpu_byte, 1)
    GLOBAL_VARIABLE(FCR0, 4)
    GLOBAL_VARIABLE(FCR31, 4)
    GLOBAL_VARIABLE(reg, 256)
    GLOBAL_VARIABLE(hi, 8)
    GLOBAL_VARIABLE(lo, 8)
    GLOBAL_VARIABLE(g_cp0_regs, 128)
    GLOBAL_VARIABLE(reg_cop1_simple, 256)
    GLOBAL_VARIABLE(reg_cop1_double, 256)
    GLOBAL_VARIABLE(rounding_modes, 16)
    GLOBAL_VARIABLE(branch_target, 4)
    GLOBAL_VARIABLE(PC, 8)
    GLOBAL_VARIABLE(fake_pc, 208)
    GLOBAL_VARIABLE(ram_offset, 8)
    GLOBAL_VARIABLE(mini_ht, 512)
    GLOBAL_VARIABLE(restore_candidate, 512)
    GLOBAL_VARIABLE(instr_addr, 4)
    GLOBAL_VARIABLE(link_register, 8)
    GLOBAL_VARIABLE(memory_map, 8388608)

extra_memory:
    .space    33554432+256+4+4+4+4+4+4+8+8+8+8+4+2+4+4+4+256+8+8+128+256+256+16+8+8+208+8+512+512+8+8+8388608

    dynarec_local     = extra_memory      + 33554432
    next_interrupt     = dynarec_local     + 256
    cycle_count       = next_interrupt     + 4
    last_count        = cycle_count       + 4
    pending_exception = last_count        + 4
    pcaddr            = pending_exception + 4
    stop              = pcaddr            + 4
    invc_ptr          = stop              + 4
    address           = invc_ptr          + 8
    readmem_dword     = address           + 8 /* 4 bytes free */
    cpu_dword         = readmem_dword     + 8
    cpu_word          = cpu_dword         + 8
    cpu_hword         = cpu_word          + 4
    cpu_byte          = cpu_hword         + 2 /* 1 byte free */
    FCR0              = cpu_hword         + 4
    FCR31             = FCR0              + 4
    reg               = FCR31             + 4
    hi                = reg               + 256
    lo                = hi                + 8
    g_cp0_regs        = lo                + 8
    reg_cop1_simple   = g_cp0_regs        + 128
    reg_cop1_double   = reg_cop1_simple   + 256
    rounding_modes    = reg_cop1_double   + 256
    branch_target     = rounding_modes    + 16
    PC                = branch_target     + 8 /* 4 bytes free */
    fake_pc           = PC                + 8
    ram_offset        = fake_pc           + 208
    mini_ht           = ram_offset        + 8
    restore_candidate = mini_ht           + 512
    instr_addr        = restore_candidate + 512
    link_register     = instr_addr        + 8 /* 4 bytes free */
    memory_map        = link_register     + 8

END_SECTION

TEXT_SECTION

GLOBAL_FUNCTION(dyna_linker):
    brk 0
    
GLOBAL_FUNCTION(dyna_linker_ds):
    brk 0
    
GLOBAL_FUNCTION(jump_vaddr_x0):
    /*eor    w2, w0, w0, lsl #16*/
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x1):
    /*eor    w2, w1, w1, lsl #16*/
    mov    w0, w1
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x2):
    mov    w0, w2
    /*eor    w2, w2, w2, lsl #16*/
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x3):
    /*eor    w2, w3, w3, lsl #16*/
    mov    w0, w3
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x4):
    /*eor    w2, w4, w4, lsl #16*/
    mov    w0, w4
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x5):
    /*eor    w2, w5, w5, lsl #16*/
    mov    w0, w5
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x6):
    /*eor    w2, w6, w6, lsl #16*/
    mov    w0, w6
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x8):
    /*eor    w2, w8, w8, lsl #16*/
    mov    w0, w8
    b      jump_vaddr

GLOBAL_FUNCTION(jump_vaddr_x9):
    /*eor    w2, w9, w9, lsl #16*/
    mov    w0, w9
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x10):
    /*eor    w2, w10, w10, lsl #16*/
    mov    w0, w10
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x11):
    /*eor    w2, w11, w11, lsl #16*/
    mov    w0, w11
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x12):
    /*eor    w2, w12, w12, lsl #16*/
    mov    w0, w12
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x13):
    /*eor    w2, w13, w13, lsl #16*/
    mov    w0, w13
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x14):
    /*eor    w2, w14, w14, lsl #16*/
    mov    w0, w14
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x15):
    /*eor    w2, w15, w15, lsl #16*/
    mov    w0, w15
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x16):
    /*eor    w2, w16, w16, lsl #16*/
    mov    w0, w16
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x17):
    /*eor    w2, w17, w17, lsl #16*/
    mov    w0, w17
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x18):
    /*eor    w2, w18, w18, lsl #16*/
    mov    w0, w18
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x19):
    /*eor    w2, w19, w19, lsl #16*/
    mov    w0, w19
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x20):
    /*eor    w2, w20, w20, lsl #16*/
    mov    w0, w20
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x21):
    /*eor    w2, w21, w21, lsl #16*/
    mov    w0, w21
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x22):
    /*eor    w2, w22, w22, lsl #16*/
    mov    w0, w22
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x23):
    /*eor    w2, w23, w23, lsl #16*/
    mov    w0, w23
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x24):
    /*eor    w2, w24, w24, lsl #16*/
    mov    w0, w24
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x25):
    /*eor    w2, w25, w25, lsl #16*/
    mov    w0, w25
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x26):
    /*eor    w2, w26, w26, lsl #16*/
    mov    w0, w26
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x27):
    /*eor    w2, w27, w27, lsl #16*/
    mov    w0, w27
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x28):
    /*eor    w2, w28, w28, lsl #16*/
    mov    w0, w28
    b      jump_vaddr
    
GLOBAL_FUNCTION(jump_vaddr_x7):
    /*eor    w2, w7, w7, lsl #16*/
    mov    w0, w7
 
GLOBAL_FUNCTION(jump_vaddr):
    str    w20, [x29, #cycle_count-dynarec_local]
    ldr    w18, [x29, #last_count-dynarec_local]
    add    w20, w18, w20 /* Count */
    str    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    bl     get_addr_ht
    ldr    w20, [x29, #cycle_count-dynarec_local]
    br     x0
    /*adrp   x1, hash_table
    add    x1, x1, :lo12:hash_table
    movn   w3, #15
    and    w2, w3, w2, lsr #12
    add    x1, x1, x2
    ldr    w2, [x1]
    cmp    w2, w0
    b.eq   .A1
    add    x1, x1, #16
    ldr    w2, [x1]
    cmp    w2, w0
    b.eq   .A1
    str    w20, [x29, #cycle_count-dynarec_local]
    bl     get_addr
    ldr    w20, [x29, #cycle_count-dynarec_local]
    br     x0
.A1:
    ldr    x1, [x1, #8]
    br     x1*/
    
GLOBAL_FUNCTION(verify_code_ds):
    str    w19, [x29, #branch_target-dynarec_local]
    
GLOBAL_FUNCTION(verify_code_vm):
    /* w0 = instruction pointer (virtual address) */
    /* w1 = source (virtual address) */
    /* x2 = copy */
    /* w3 = length */
    mov    w4, #0xC0000000
    cmp    w1, w4
    b.lt   verify_code
    add    x8, x29, #memory_map-dynarec_local
    lsr    w4, w1, #12
    add    w5, w1, w3
    sub    w5, w5, #1
    ldr    x6, [x8, x4, lsl #3]
    lsr    w5, w5, #12
    tst    x6, x6
    b.mi   .D5
    mov    x7, x6
    add    x1, x1, x6, lsl #2
    lsl    x6, x6, #2
.C1:
    add    w4, w4, #1
    cmp    x6, x7, lsl #2
    b.ne   .D5
    ldr    x7, [x8, x4, lsl #3]
    cmp    w4, w5
    b.ls   .C1
    
/*TOBEDONE: Optimize for 64bit*/
GLOBAL_FUNCTION(verify_code):
    /* x1 = source */
    /* x2 = copy */
    /* w3 = length */
    tst    w3, #4
    add    x3, x1, x3
    mov    w4, #0
    mov    w5, #0
    mov    w12, #0
    b.eq   .D1
    ldr    w4, [x1], #4
    ldr    w5, [x2], #4
.D1:
    cmp    x1, x3
    b.eq   .D3
.D2:
    ldr    w7, [x1], #4
    eor    w9, w4, w5
    ldr    w8, [x2], #4
    orr    w9, w9, w12
    tst    w9, w9
    b.ne   .D4
    ldr    w4, [x1], #4
    eor    w12, w7, w8
    ldr    w5, [x2], #4
    cmp    x1, x3
    b.cc   .D2
    cmp    w7, w8
    b.ne   .D4
.D3:
    cmp    w4, w5
.D4:
    ldr    w19, [x29, #branch_target-dynarec_local]
    b.eq   .D6
.D5:
    bl     get_addr
    br     x0
.D6:
    ret
    
GLOBAL_FUNCTION(cc_interrupt):
    ldr    w0, [x29, #last_count-dynarec_local]
    add    w20, w0, w20 /* Count */
    str    wzr, [x29, #pending_exception-dynarec_local]
    mov    w1, #0x1fc
    lsr    w0, w20, #19
    and    w1, w1, w0 
    add    x0, x29, #restore_candidate-dynarec_local
    str    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    ldr    w22, [x1, x0]
    mov    x20, x30 /* Save link register */
    tst    w22, w22
    b.ne   .E4
.E1:
    bl     gen_interrupt
    mov    x30, x20 /* Restore link register */
    ldr    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    ldr    w0, [x29, #next_interrupt-dynarec_local]
    ldr    w1, [x29, #pending_exception-dynarec_local]
    ldr    w2, [x29, #stop-dynarec_local]
    str    w0, [x29, #last_count-dynarec_local]
    sub    w20, w20, w0
    tst    w2, w2
    b.ne   .E3
    tst    w1, w1
    b.ne   .E2
    ret
.E2:
    ldr    w0, [x29, #pcaddr-dynarec_local]
    bl     get_addr_ht
    br     x0
.E3:
    add    x16, x29, #152
    ldp    x19,x20,[x16,#0]
    ldp    x21,x22,[x16,#16]
    ldp    x23,x24,[x16,#32]
    ldp    x25,x26,[x16,#48]
    ldp    x27,x28,[x16,#64]
    ldp    x29,x30,[x16,#80]
    ret
.E4:
    /* Move 'dirty' blocks to the 'clean' list */
    str    wzr, [x1, x0]
    lsl    w21, w1, #3
    mov    w23, #0
.E5:
    tst    w22, #1
    b.eq   .E6
    add    w0, w21, w23
    bl     clean_blocks
.E6:
    lsr    w22, w22, #1
    add    w23, w23, #1
    tst    w23, #31
    b.ne   .E5
    b      .E1
    
GLOBAL_FUNCTION(do_interrupt):
    ldr    w0, [x29, #pcaddr-dynarec_local]
    bl     get_addr_ht
    ldr    w1, [x29, #next_interrupt-dynarec_local]
    ldr    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    str    w1, [x29, #last_count-dynarec_local]
    sub    w20, w20, w1
    add    w20, w20, #2
    br     x0
    
GLOBAL_FUNCTION(fp_exception):
    mov    w2, #0x10000000
.F1:
    ldr    w1, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    mov    w3, #0x80000000
    str    w0, [x29, #g_cp0_regs+56-dynarec_local] /* EPC */
    orr    w1, w1, #2
    add    w2, w2, #0x2c
    str    w1, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    str    w2, [x29, #g_cp0_regs+52-dynarec_local] /* Cause */
    add    w0, w3, #0x180
    bl     get_addr_ht
    br     x0
    
GLOBAL_FUNCTION(fp_exception_ds):
    brk    0
    mov    w2, #0x90000000 /* Set high bit if delay slot */
    b      .F1
    
GLOBAL_FUNCTION(jump_syscall):
    ldr    w1, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    mov    w3, #0x80000000
    str    w0, [x29, #g_cp0_regs+56-dynarec_local] /* EPC */
    orr    w1, w1, #2
    mov    w2, #0x20
    str    w1, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    str    w2, [x29, #g_cp0_regs+52-dynarec_local] /* Cause */
    add    w0, w3, #0x180
    bl     get_addr_ht
    br     x0
    
GLOBAL_FUNCTION(indirect_jump_indexed):
    ldr    x0, [x0, x1, lsl #3]

GLOBAL_FUNCTION(indirect_jump):
    ldr    w18, [x29, #last_count-dynarec_local]
    add    w2, w2, w18 
    str    w2, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    br     x0
    
GLOBAL_FUNCTION(jump_eret):
    ldr    w1, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    ldr    w0, [x29, #last_count-dynarec_local]
    bic    w1, w1, #2
    add    w20, w0, w20
    str    w1, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    str    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    bl     check_interrupt
    ldr    w1, [x29, #next_interrupt-dynarec_local]
    ldr    w0, [x29, #g_cp0_regs+56-dynarec_local] /* EPC */
    str    w1, [x29, #last_count-dynarec_local]
    subs   w20, w20, w1
    b.pl   .E11
.E8:
    add    x6, x29, #reg+256-dynarec_local
    mov    w5, #248
    mov    w1, #0
.E9:
    /*TOBEDONE: Optimize for 64bit*/
    sub    x6, x6, #8
    ldr    w2, [x6]
    ldr    w3, [x6, #4]
    eor    w3, w3, w2, asr #31
    subs   w3, w3, #1
    adc    w1, w1, w1
    subs   w5, w5, #8
    b.ne    .E9
    ldr    w2, [x29, #hi-dynarec_local]
    ldr    w3, [x29, #hi+4-dynarec_local]
    eor    w3, w3, w2, asr #31
    tst    w3, w3
    b.ne   .E10
    ldr    w2, [x29, #lo-dynarec_local]
    ldr    w3, [x29, #lo+4-dynarec_local]
    eor    w3, w3, w2, asr #31
.E10:
    subs   w3, w3, #1
    adc    w1, w1, w1
    bl     get_addr_32
    br     x0
.E11:
    str    w0, [x29, #pcaddr-dynarec_local]
    bl     cc_interrupt
    ldr    w0, [x29, #pcaddr-dynarec_local]
    b      .E8
    
GLOBAL_FUNCTION(new_dyna_start):
    adrp   x16, dynarec_local
    add    x16, x16, :lo12:dynarec_local
    add    x16, x16, #152
    adrp   x1, base_addr
    add    x1, x1, :lo12:base_addr
    mov    w0, #0xa4000000
    stp    x19,x20,[x16,#0]
    stp    x21,x22,[x16,#16]
    stp    x23,x24,[x16,#32]
    stp    x25,x26,[x16,#48]
    stp    x27,x28,[x16,#64]
    stp    x29,x30,[x16,#80]
    sub    x29, x16, #152
    ldr    x19, [x1]
    add    w0, w0, #0x40
    bl     new_recompile_block
    ldr    w0, [x29, #next_interrupt-dynarec_local]
    ldr    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    str    w0, [x29, #last_count-dynarec_local]
    sub    w20, w20, w0
    br     x19
    
GLOBAL_FUNCTION(invalidate_addr_x0):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x1):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x2):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x3):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x4):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x5):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x6):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x7):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x8):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x9):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x10):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x11):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x12):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x13):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x14):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x15):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x16):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x17):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x18):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x19):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x20):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x21):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x22):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x23):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x24):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x25):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x26):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x27):
    brk 0
    
GLOBAL_FUNCTION(invalidate_addr_x28):
    brk 0
    
GLOBAL_FUNCTION(write_rdram_new):
    ldr    x3, [x29, #ram_offset-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    ldr    w0, [x29, #cpu_word-dynarec_local]
    str    w0, [x2, x3, lsl #2]
    b      .E12
    
GLOBAL_FUNCTION(write_rdramb_new):
    ldr    x3, [x29, #ram_offset-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    ldrb   w0, [x29, #cpu_byte-dynarec_local]
    eor    w2, w2, #3
    lsl    x3, x3, #2
    strb   w0, [x2, x3]
    b      .E12
    
GLOBAL_FUNCTION(write_rdramh_new):
    ldr    x3, [x29, #ram_offset-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    ldrh   w0, [x29, #cpu_hword-dynarec_local]
    eor    w2, w2, #2
    lsl    x3, x3, #2
    strh   w0, [x2, x3]
    b      .E12
    
GLOBAL_FUNCTION(write_rdramd_new):
    ldr    x3, [x29, #ram_offset-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    ldr    w0, [x29, #cpu_dword-dynarec_local]
    ldr    w1, [x29, #cpu_dword+4-dynarec_local]
    add    x3, x2, x3, lsl #2
    str    w0, [x3, #4]
    str    w1, [x3]
    b      .E12

LOCAL_FUNCTION(do_invalidate):
    ldr    w2, [x29, #address-dynarec_local]
.E12:
    ldr    x1, [x29, #invc_ptr-dynarec_local]
    lsr    w0, w2, #12
    ldrb   w2, [x1, x0]
    tst    w2, w2
    b.eq   invalidate_block
    ret
    
GLOBAL_FUNCTION(read_nomem_new):
    /* w3 = instr addr/flags */
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #8
    tst    x4, x4
    b.mi   tlb_exception
    ldr    w0, [x2, x4, lsl #2]
    str    w0, [x29, #readmem_dword-dynarec_local]
    ret
    
GLOBAL_FUNCTION(read_nomemb_new):
    /* w3 = instr addr/flags */
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #8
    tst    x4, x4
    b.mi   tlb_exception
    eor    w2, w2, #3
    lsl    x4, x4, #2
    ldrb   w0, [x2, x4]
    str    w0, [x29, #readmem_dword-dynarec_local]
    ret
    
GLOBAL_FUNCTION(read_nomemh_new):
    /* w3 = instr addr/flags */
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #8
    tst    x4, x4
    b.mi   tlb_exception
    lsl    x4, x4, #2
    eor    w2, w2, #2
    ldrh   w0, [x2, x4]
    str    w0, [x29, #readmem_dword-dynarec_local]
    ret
    
GLOBAL_FUNCTION(read_nomemd_new):
    /* w3 = instr addr/flags */
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #8
    tst    x4, x4
    b.mi   tlb_exception
    add    x4, x2, x4, lsl #2
    ldr    w0, [x4]
    ldr    w1, [x4, #4]
    str    w0, [x29, #readmem_dword+4-dynarec_local]
    str    w1, [x29, #readmem_dword-dynarec_local]
    ret
    
GLOBAL_FUNCTION(write_nomem_new):
    /* w3 = instr addr/flags */
    str    w3, [x29, #instr_addr-dynarec_local]
    str    x30, [x29, #link_register-dynarec_local]
    bl     do_invalidate
    ldr    w3, [x29, #instr_addr-dynarec_local]
    ldr    x30, [x29, #link_register-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #0xc
    tst    x4, #0x4000000000000000
    b.ne   tlb_exception
    ldr    w0, [x29, #cpu_word-dynarec_local]
    str    w0, [x2, x4, lsl #2]
    ret
    
GLOBAL_FUNCTION(write_nomemb_new):
    /* w3 = instr addr/flags */
    str    w3, [x29, #instr_addr-dynarec_local]
    str    x30, [x29, #link_register-dynarec_local]
    bl     do_invalidate
    ldr    w3, [x29, #instr_addr-dynarec_local]
    ldr    x30, [x29, #link_register-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #0xc
    tst    x4, #0x4000000000000000
    b.ne   tlb_exception
    lsl    x4, x4, #2
    eor    w2, w2, #3
    ldrb   w0, [x29, #cpu_byte-dynarec_local]
    strb   w0, [x2, x4]
    ret
    
GLOBAL_FUNCTION(write_nomemh_new):
    /* w3 = instr addr/flags */
    str    w3, [x29, #instr_addr-dynarec_local]
    str    x30, [x29, #link_register-dynarec_local]
    bl     do_invalidate
    ldr    w3, [x29, #instr_addr-dynarec_local]
    ldr    x30, [x29, #link_register-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #0xc
    tst    x4, #0x4000000000000000
    b.ne   tlb_exception
    lsl    x4, x4, #2
    eor    w2, w2, #2
    ldrh   w0, [x29, #cpu_hword-dynarec_local]
    strh   w0, [x2, x4]
    ret
    
GLOBAL_FUNCTION(write_nomemd_new):
    /* w3 = instr addr/flags */
    str    w3, [x29, #instr_addr-dynarec_local]
    str    x30, [x29, #link_register-dynarec_local]
    bl     do_invalidate
    ldr    w3, [x29, #instr_addr-dynarec_local]
    ldr    x30, [x29, #link_register-dynarec_local]
    ldr    w2, [x29, #address-dynarec_local]
    add    x4, x29, #memory_map-dynarec_local
    lsr    w0, w2, #12
    ldr    x4, [x4, x0, lsl #3]
    mov    w1, #0xc
    tst    x4, #0x4000000000000000
    b.ne   tlb_exception
    lsl    x4, x4, #2
    ldr    w0, [x29, #cpu_dword+4-dynarec_local]
    ldr    w1, [x29, #cpu_dword-dynarec_local]
    str    w0, [x4]
    str    w1, [x4, #4]
    ret

LOCAL_FUNCTION(tlb_exception):
    /* w1 = cause */
    /* w2 = address */
    /* w3 = instr addr/flags */
    ldr    w4, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    add    x5, x29, #memory_map-dynarec_local
    lsr    w6, w3, #12
    orr    w1, w1, w3, lsl #31
    orr    w4, w4, #2
    ldr    x7, [x5, x6, lsl #3]
    bic    w8, w3, #3
    str    w4, [x29, #g_cp0_regs+48-dynarec_local] /* Status */
    mov    w6, #0x6000000
    movk   w6, #0x22
    str    w1, [x29, #g_cp0_regs+52-dynarec_local] /* Cause */
    ldr    w0, [x8, x7, lsl #2]
    add    w4, w8, w1, asr #29
    add    x5, x29, #reg-dynarec_local
    str    w4, [x29, #g_cp0_regs+56-dynarec_local] /* EPC */
    mov    w7, #0xf8
    ldr    w8, [x29, #g_cp0_regs+16-dynarec_local] /* Context */
    lsl    w1, w0, #16
    lsr    w4, w0, #26
    and    w4, w4, #31
    and    w7, w7, w0, lsr #18
    mov    w9, #0x0FFFFFF0
    sub    w2, w2, w1, asr #16
    bic    w9, w9, #0x0F800000
    ror    w6, w6, w4
    mov    w0, #0x80000000
    tst    w6, w0
    b.eq   .G1
    ldr    w2, [x5, x7]
.G1:
    bic    w8, w8, w9
    tst    w3, #2
    str    w2, [x5, x7]
    add    w4, w2, w1, asr #16
    add    x6, x29, #reg+4-dynarec_local
    asr    w3, w2, #31
    str    w4, [x29, #g_cp0_regs+32-dynarec_local] /* BadVAddr */
    add    w0, w0, #0x180
    and    w4, w9, w4, lsr #9
    b.eq   .G2
    str    w3, [x6, x7]
.G2:
    orr    w8, w8, w4
    str    w8, [x29, #g_cp0_regs+16-dynarec_local] /* Context */
    bl     get_addr_ht
    ldr    w1, [x29, #next_interrupt-dynarec_local]
    ldr    w20, [x29, #g_cp0_regs+36-dynarec_local] /* Count */
    str    w1, [x29, #last_count-dynarec_local]
    sub    w20, w20, w1
    br     x0
    
GLOBAL_FUNCTION(breakpoint):
    brk 0

END_SECTION
