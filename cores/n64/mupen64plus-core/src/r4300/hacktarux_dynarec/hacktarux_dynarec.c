/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rjump.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2007 Richard Goedeken (Richard42)                       *
 *   Copyright (C) 2002 Hacktarux                                          *
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assemble.h"
#include "regcache.h"
#include "interpret.h"

#include "api/debugger.h"
#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "main/main.h"
#include "main/device.h"
#include "memory/memory.h"
#include "r4300/cached_interp.h"
#include "r4300/recomp.h"
#include "r4300/cp0_private.h"
#include "r4300/cp1_private.h"
#include "r4300/exception.h"
#include "r4300/interrupt.h"
#include "r4300/r4300.h"
#include "r4300/macros.h"
#include "r4300/ops.h"
#include "r4300/recomp.h"
#include "r4300/recomph.h"
#include "r4300/exception.h"

#if !defined(offsetof)
#define offsetof(TYPE,MEMBER) ((unsigned int) &((TYPE*)0)->MEMBER)
#endif

static struct precomp_instr fake_instr;

int branch_taken = 0;

/* that's where the dynarec will restart when going back from a C function */
#if defined(__x86_64__) || defined(_M_X64)
#if defined(_MSC_VER)
uint64_t *return_address;
int64_t save_rsp = 0;
int64_t save_rip = 0;
#else
static uint64_t *return_address;
static int64_t save_rsp = 0;
static int64_t save_rip = 0;
#endif
#else
#ifdef __GNUC__
# define ASM_NAME(name) __asm(name)
#else
# define ASM_NAME(name)
#endif

static int32_t save_ebp ASM_NAME("save_ebp") = 0;
static int32_t save_ebx ASM_NAME("save_ebx") = 0;
static int32_t save_esi ASM_NAME("save_esi") = 0;
static int32_t save_edi ASM_NAME("save_edi") = 0;
static int32_t save_esp ASM_NAME("save_esp") = 0;
static int32_t save_eip ASM_NAME("save_eip") = 0;
static uint32_t *return_address ASM_NAME("return_address");
#endif

/* These are constants with addresses so that FLDCW can read them.
 * They are declared 'extern' so that other files can do the same. */
const uint16_t trunc_mode = 0xF3F;
const uint16_t round_mode = 0x33F;
const uint16_t ceil_mode  = 0xB3F;
const uint16_t floor_mode = 0x73F;


void dyna_jump(void)
{
    if (stop == 1)
    {
        dyna_stop();
        return;
    }

    if (PC->reg_cache_infos.need_map)
        *return_address = (native_type)(PC->reg_cache_infos.jump_wrapper);
    else
        *return_address = (native_type)(actual->code + PC->local_addr);
}

#if defined(_M_X64) && defined(_WIN32) && defined(_MSC_VER)
#else
void dyna_start(void *code)
{
  /* save the base and stack pointers */
  /* make a call and a pop to retrieve the instruction pointer and save it too */
  /* then call the code(), which should theoretically never return.  */
  /* When dyna_stop() sets the *return_address to the saved RIP (x86_64) / EIP (x86),
   * the emulator thread will come back here. */
  /* It will jump to label 2, restore the base and stack pointers, and exit this function */
#if defined(__x86_64__)
#if defined(__GNUC__)
  asm volatile
    (" push %%rbx              \n"  /* we must push an even # of registers to keep stack 16-byte aligned */
     " push %%r12              \n"
     " push %%r13              \n"
     " push %%r14              \n"
     " push %%r15              \n"
     " push %%rbp              \n"
     " mov  %%rsp, %[save_rsp] \n"
     " lea  %[reg], %%r15      \n" /* store the base location of the r4300 registers in r15 for addressing */
     " call 1f                 \n"
     " jmp 2f                  \n"
     "1:                       \n"
     " pop  %%rax              \n"
     " mov  %%rax, %[save_rip] \n"

     " sub $0x10, %%rsp        \n"
     " and $-16, %%rsp         \n" /* ensure that stack is 16-byte aligned */
     " mov %%rsp, %%rax        \n"
     " sub $8, %%rax           \n"
     " mov %%rax, %[return_address]\n"

     " call *%%rbx             \n"
     "2:                       \n"
     " mov  %[save_rsp], %%rsp \n"
     " pop  %%rbp              \n"
     " pop  %%r15              \n"
     " pop  %%r14              \n"
     " pop  %%r13              \n"
     " pop  %%r12              \n"
     " pop  %%rbx              \n"
     : [save_rsp]"=m"(save_rsp), [save_rip]"=m"(save_rip), [return_address]"=m"(return_address)
     : "b" (code), [reg]"m"(*reg)
     : "%rax", "memory"
     );
#endif

    /* clear the registers so we don't return here a second time; that would be a bug */
    save_rsp=0;
    save_rip=0;
#else
#if defined(WIN32) && !defined(__GNUC__)
    __asm
    {
       mov save_ebp, ebp
          mov save_esp, esp
          mov save_ebx, ebx
          mov save_esi, esi
          mov save_edi, edi
          call point1
          jmp point2
          point1:
          pop eax
          mov save_eip, eax

          sub esp, 0x10
          and esp, 0xfffffff0
          mov return_address, esp
          sub return_address, 4

          mov eax, code
          call eax
          point2:
          mov ebp, save_ebp
          mov esp, save_esp
          mov ebx, save_ebx
          mov esi, save_esi
          mov edi, save_edi
    }
#elif defined(__GNUC__) && defined(__i386__)
#if defined(__PIC__)
#ifndef __GNUC_PREREQ
# if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
# else
#     define __GNUC_PREREQ(maj, min) 0
# endif
#endif

    /* for -fPIC (shared libraries) */
#if defined(ANDROID_X86) || __GNUC_PREREQ (4, 7)
#  define GET_PC_THUNK_STR(reg) "__x86.get_pc_thunk." #reg
#else
#  define GET_PC_THUNK_STR(reg) "__i686.get_pc_thunk." #reg
#endif
#define STORE_EBX
#define LOAD_EBX "call  " GET_PC_THUNK_STR(bx) "     \n" \
    "addl $_GLOBAL_OFFSET_TABLE_, %%ebx \n"
#else
    /* for non-PIC binaries */
#define STORE_EBX "movl %%ebx, %[save_ebx] \n"
#define LOAD_EBX  "movl %[save_ebx], %%ebx \n"
#endif

    __asm(STORE_EBX
          " movl %%ebp, %[save_ebp] \n"
          " movl %%esp, %[save_esp] \n"
          " movl %%esi, %[save_esi] \n"
          " movl %%edi, %[save_edi] \n"
          " call    1f              \n"
          " jmp     2f              \n"
          "1:                       \n"
          " popl %%eax              \n"
          " movl %%eax, %[save_eip] \n"

          " subl $16, %%esp         \n" /* save 16 bytes of padding just in case */
          " andl $-16, %%esp        \n" /* align stack on 16-byte boundary for OSX */
          " movl %%esp, %[return_address] \n"
          " subl $4, %[return_address] \n"

          " call *%[codeptr]        \n"
          "2:                       \n"
          LOAD_EBX
          " movl %[save_ebp], %%ebp \n"
          " movl %[save_esp], %%esp \n"
          " movl %[save_esi], %%esi \n"
          " movl %[save_edi], %%edi \n"
          : [save_ebp]"=m"(save_ebp), [save_esp]"=m"(save_esp), [save_ebx]"=m"(save_ebx), [save_esi]"=m"(save_esi), [save_edi]"=m"(save_edi), [save_eip]"=m"(save_eip), [return_address]"=m"(return_address)
          : [codeptr]"r"(code)
             : "eax", "ecx", "edx", "memory"
                );
#endif

    /* clear the registers so we don't return here a second time; that would be a bug */
    /* this is also necessary to prevent compiler from optimizing out the static variables */
    save_edi=0;
    save_esi=0;
    save_ebx=0;
    save_ebp=0;
    save_esp=0;
    save_eip=0;
#endif
}
#endif

void dyna_stop(void)
{
#if defined(__x86_64__) || (_M_X64)
   if (save_rip != 0)
   {
      *return_address = (native_type)save_rip;
      return;
   }
#else
   if (save_eip != 0)
   {
      *return_address = (native_type) save_eip;
      return;
   }
#endif

   DebugMessage(M64MSG_WARNING, "Instruction pointer is 0 at dyna_stop()");
}

/* M64P Pseudo instructions */

void gencheck_cop1_unusable(void)
{
#ifdef __x86_64__
   free_registers_move_start();

   test_m32rel_imm32((unsigned int*)&g_cp0_regs[CP0_STATUS_REG], CP0_STATUS_CU1);
#else
   free_all_registers();
   simplify_access();
   test_m32_imm32((unsigned int*)&g_cp0_regs[CP0_STATUS_REG], CP0_STATUS_CU1);
#endif
   jne_rj(0);
   jump_start_rel8();

   gencallinterp((native_type)check_cop1_unusable, 0);

   jump_end_rel8();
}

static void genupdate_count(unsigned int addr)
{
#ifndef DBG
   mov_reg32_imm32(EAX, addr);
#ifdef __x86_64__
   sub_xreg32_m32rel(EAX, (unsigned int*)(&last_addr));
   shr_reg32_imm8(EAX, 2);
   mov_xreg32_m32rel(EDX, (void*)&count_per_op);
   mul_reg32(EDX);
   add_m32rel_xreg32((unsigned int*)(&g_cp0_regs[CP0_COUNT_REG]), EAX);
#else
   sub_reg32_m32(EAX, (unsigned int*)(&last_addr));
   shr_reg32_imm8(EAX, 2);
   mov_reg32_imm32(EDX, count_per_op);
   mul_reg32(EDX);
   add_m32_reg32((unsigned int*)(&g_cp0_regs[CP0_COUNT_REG]), EAX);
#endif

#else

#ifdef __x86_64__
   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t)update_count);
   call_reg64(RAX);
#else
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)update_count);
   call_reg32(EAX);
#endif
#endif
}

static void gencheck_interrupt(uint64_t instr_structure)
{
#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (void*)(&next_interrupt));
   cmp_xreg32_m32rel(EAX, (void*)&g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(0);
   jump_start_rel8();

   mov_reg64_imm64(RAX, (uint64_t) instr_structure);
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) gen_interrupt);
   call_reg64(RAX);

   jump_end_rel8();
#else
   mov_eax_memoffs32(&next_interrupt);
   cmp_reg32_m32(EAX, &g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(17);
   mov_m32_imm32((unsigned int*)(&PC), instr_structure); // 10
   mov_reg32_imm32(EAX, (unsigned int)gen_interrupt); // 5
   call_reg32(EAX); // 2
#endif
}

static void gencheck_interrupt_out(unsigned int addr)
{
#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (void*)(&next_interrupt));
   cmp_xreg32_m32rel(EAX, (void*)&g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(0);
   jump_start_rel8();

   mov_m32rel_imm32((unsigned int*)(&fake_instr.addr), addr);
   mov_reg64_imm64(RAX, (uint64_t) (&fake_instr));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) gen_interrupt);
   call_reg64(RAX);

   jump_end_rel8();
#else
   mov_eax_memoffs32(&next_interrupt);
   cmp_reg32_m32(EAX, &g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(27);
   mov_m32_imm32((unsigned int*)(&fake_instr.addr), addr);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(&fake_instr));
   mov_reg32_imm32(EAX, (unsigned int)gen_interrupt);
   call_reg32(EAX);
#endif
}

void genmfc0(void)
{
   gencallinterp((native_type)cached_interpreter_table.MFC0, 0);
}

void genmtc0(void)
{
   gencallinterp((native_type)cached_interpreter_table.MTC0, 0);
}

void genmfc1(void)
{
#ifdef INTERPRET_MFC1
   gencallinterp((native_type)cached_interpreter_table.MFC1, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t*)(&reg_cop1_simple[dst->f.r.nrd]));
   mov_reg32_preg64(EBX, RAX);
   mov_m32rel_xreg32((uint32_t*)dst->f.r.rt, EBX);
   sar_reg32_imm8(EBX, 31);
   mov_m32rel_xreg32(((uint32_t*)dst->f.r.rt)+1, EBX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.r.nrd]));
   mov_reg32_preg32(EBX, EAX);
   mov_m32_reg32((unsigned int*)dst->f.r.rt, EBX);
   sar_reg32_imm8(EBX, 31);
   mov_m32_reg32(((unsigned int*)dst->f.r.rt)+1, EBX);
#endif
#endif
}

void gendmfc1(void)
{
#ifdef INTERPRET_DMFC1
   gencallinterp((native_type)cached_interpreter_table.DMFC1, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *) (&reg_cop1_double[dst->f.r.nrd]));
   mov_reg32_preg64(EBX, RAX);
   mov_reg32_preg64pimm32(ECX, RAX, 4);
   mov_m32rel_xreg32((unsigned int*)dst->f.r.rt, EBX);
   mov_m32rel_xreg32(((unsigned int*)dst->f.r.rt)+1, ECX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.r.nrd]));
   mov_reg32_preg32(EBX, EAX);
   mov_reg32_preg32pimm32(ECX, EAX, 4);
   mov_m32_reg32((unsigned int*)dst->f.r.rt, EBX);
   mov_m32_reg32(((unsigned int*)dst->f.r.rt)+1, ECX);
#endif
#endif
}

void gencfc1(void)
{
#ifdef INTERPRET_CFC1
   gencallinterp((native_type)cached_interpreter_table.CFC1, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   if(dst->f.r.nrd == 31)
      mov_xreg32_m32rel(EAX, (unsigned int*)&FCR31);
   else
      mov_xreg32_m32rel(EAX, (unsigned int*)&FCR0);
   mov_m32rel_xreg32((unsigned int*)dst->f.r.rt, EAX);
   sar_reg32_imm8(EAX, 31);
   mov_m32rel_xreg32(((unsigned int*)dst->f.r.rt)+1, EAX);
#else
   if(dst->f.r.nrd == 31)
      mov_eax_memoffs32((unsigned int*)&FCR31);
   else
      mov_eax_memoffs32((unsigned int*)&FCR0);
   mov_memoffs32_eax((unsigned int*)dst->f.r.rt);
   sar_reg32_imm8(EAX, 31);
   mov_memoffs32_eax(((unsigned int*)dst->f.r.rt)+1);
#endif
#endif
}

void genmtc1(void)
{
#ifdef INTERPRET_MTC1
   gencallinterp((native_type)cached_interpreter_table.MTC1, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int*)dst->f.r.rt);
   mov_xreg64_m64rel(RBX, (uint64_t *)(&reg_cop1_simple[dst->f.r.nrd]));
   mov_preg64_reg32(RBX, EAX);
#else
   mov_eax_memoffs32((unsigned int*)dst->f.r.rt);
   mov_reg32_m32(EBX, (unsigned int*)(&reg_cop1_simple[dst->f.r.nrd]));
   mov_preg32_reg32(EBX, EAX);
#endif
#endif
}

void gendmtc1(void)
{
#ifdef INTERPRET_DMTC1
   gencallinterp((native_type)cached_interpreter_table.DMTC1, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int*)dst->f.r.rt);
   mov_xreg32_m32rel(EBX, ((unsigned int*)dst->f.r.rt)+1);
   mov_xreg64_m64rel(RDX, (uint64_t *)(&reg_cop1_double[dst->f.r.nrd]));
   mov_preg64_reg32(RDX, EAX);
   mov_preg64pimm32_reg32(RDX, 4, EBX);
#else
   mov_eax_memoffs32((unsigned int*)dst->f.r.rt);
   mov_reg32_m32(EBX, ((unsigned int*)dst->f.r.rt)+1);
   mov_reg32_m32(EDX, (unsigned int*)(&reg_cop1_double[dst->f.r.nrd]));
   mov_preg32_reg32(EDX, EAX);
   mov_preg32pimm32_reg32(EDX, 4, EBX);
#endif
#endif
}

void genctc1(void)
{
#ifdef INTERPRET_CTC1
   gencallinterp((native_type)cached_interpreter_table.CTC1, 0);
#else
   gencheck_cop1_unusable();
   
   if (dst->f.r.nrd != 31)
      return;
#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int*)dst->f.r.rt);
   mov_m32rel_xreg32((unsigned int*)&FCR31, EAX);
   and_eax_imm32(3);
   
   cmp_eax_imm32(0);
   jne_rj(13);
   mov_m32rel_imm32((unsigned int*)&rounding_mode, 0x33F); // 11
   jmp_imm_short(51); // 2
   
   cmp_eax_imm32(1); // 5
   jne_rj(13); // 2
   mov_m32rel_imm32((unsigned int*)&rounding_mode, 0xF3F); // 11
   jmp_imm_short(31); // 2
   
   cmp_eax_imm32(2); // 5
   jne_rj(13); // 2
   mov_m32rel_imm32((unsigned int*)&rounding_mode, 0xB3F); // 11
   jmp_imm_short(11); // 2
   
   mov_m32rel_imm32((unsigned int*)&rounding_mode, 0x73F); // 11
   
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   mov_eax_memoffs32((unsigned int*)dst->f.r.rt);
   mov_memoffs32_eax((unsigned int*)&FCR31);
   and_eax_imm32(3);
   
   cmp_eax_imm32(0);
   jne_rj(12);
   mov_m32_imm32((unsigned int*)&rounding_mode, 0x33F); // 10
   jmp_imm_short(48); // 2
   
   cmp_eax_imm32(1); // 5
   jne_rj(12); // 2
   mov_m32_imm32((unsigned int*)&rounding_mode, 0xF3F); // 10
   jmp_imm_short(29); // 2
   
   cmp_eax_imm32(2); // 5
   jne_rj(12); // 2
   mov_m32_imm32((unsigned int*)&rounding_mode, 0xB3F); // 10
   jmp_imm_short(10); // 2
   
   mov_m32_imm32((unsigned int*)&rounding_mode, 0x73F); // 10
   
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

static void genbeq_test(void)
{
   int rs_64bit = is64((unsigned int *)dst->f.i.rs);
   int rt_64bit = is64((unsigned int *)dst->f.i.rt);

   if (rs_64bit == 0 && rt_64bit == 0)
   {
#ifdef __x86_64__
      int rs = allocate_register_32((unsigned int *)dst->f.i.rs);
      int rt = allocate_register_32((unsigned int *)dst->f.i.rt);

      cmp_reg32_reg32(rs, rt);
      sete_m8rel((unsigned char *) &branch_taken);
#else
      int rs = allocate_register((unsigned int *)dst->f.i.rs);
      int rt = allocate_register((unsigned int *)dst->f.i.rt);

      cmp_reg32_reg32(rs, rt);
      jne_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else if (rs_64bit == -1)
   {
#ifdef __x86_64__
      int rt = allocate_register_64((uint64_t *)dst->f.i.rt);

      cmp_xreg64_m64rel(rt, (uint64_t *) dst->f.i.rs);
      sete_m8rel((unsigned char *) &branch_taken);
#else
      int rt1 = allocate_64_register1((unsigned int *)dst->f.i.rt);
      int rt2 = allocate_64_register2((unsigned int *)dst->f.i.rt);

      cmp_reg32_m32(rt1, (unsigned int *)dst->f.i.rs);
      jne_rj(20);
      cmp_reg32_m32(rt2, ((unsigned int *)dst->f.i.rs)+1); // 6
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else if (rt_64bit == -1)
   {
#ifdef __x86_64__
      int rs = allocate_register_64((uint64_t *)dst->f.i.rs);

      cmp_xreg64_m64rel(rs, (uint64_t *)dst->f.i.rt);
      sete_m8rel((unsigned char *) &branch_taken);
#else
      int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
      int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);

      cmp_reg32_m32(rs1, (unsigned int *)dst->f.i.rt);
      jne_rj(20);
      cmp_reg32_m32(rs2, ((unsigned int *)dst->f.i.rt)+1); // 6
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else
   {
#ifdef __x86_64__
      int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
      int rt = allocate_register_64((uint64_t *)dst->f.i.rt);
      cmp_reg64_reg64(rs, rt);
      sete_m8rel((unsigned char *) &branch_taken);
#else
      int rs1, rs2, rt1, rt2;
      if (!rs_64bit)
      {
         rt1 = allocate_64_register1((unsigned int *)dst->f.i.rt);
         rt2 = allocate_64_register2((unsigned int *)dst->f.i.rt);
         rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
         rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
      }
      else
      {
         rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
         rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
         rt1 = allocate_64_register1((unsigned int *)dst->f.i.rt);
         rt2 = allocate_64_register2((unsigned int *)dst->f.i.rt);
      }
      cmp_reg32_reg32(rs1, rt1);
      jne_rj(16);
      cmp_reg32_reg32(rs2, rt2); // 2
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
}

static void genbne_test(void)
{
   int rs_64bit = is64((unsigned int *)dst->f.i.rs);
   int rt_64bit = is64((unsigned int *)dst->f.i.rt);

#ifdef __x86_64__
   if (rs_64bit == 0 && rt_64bit == 0)
   {
      int rs = allocate_register_32((unsigned int *)dst->f.i.rs);
      int rt = allocate_register_32((unsigned int *)dst->f.i.rt);

      cmp_reg32_reg32(rs, rt);
      setne_m8rel((unsigned char *) &branch_taken);
   }
   else if (rs_64bit == -1)
   {
      int rt = allocate_register_64((uint64_t *) dst->f.i.rt);

      cmp_xreg64_m64rel(rt, (uint64_t *)dst->f.i.rs);
      setne_m8rel((unsigned char *) &branch_taken);
   }
   else if (rt_64bit == -1)
   {
      int rs = allocate_register_64((uint64_t *) dst->f.i.rs);

      cmp_xreg64_m64rel(rs, (uint64_t *)dst->f.i.rt);
      setne_m8rel((unsigned char *) &branch_taken);
   }
   else
   {
      int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
      int rt = allocate_register_64((uint64_t *)dst->f.i.rt);

      cmp_reg64_reg64(rs, rt);
      setne_m8rel((unsigned char *) &branch_taken);
   }
#else
   if (!rs_64bit && !rt_64bit)
   {
      int rs = allocate_register((unsigned int *)dst->f.i.rs);
      int rt = allocate_register((unsigned int *)dst->f.i.rt);

      cmp_reg32_reg32(rs, rt);
      je_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
   }
   else if (rs_64bit == -1)
   {
      int rt1 = allocate_64_register1((unsigned int *)dst->f.i.rt);
      int rt2 = allocate_64_register2((unsigned int *)dst->f.i.rt);

      cmp_reg32_m32(rt1, (unsigned int *)dst->f.i.rs);
      jne_rj(20);
      cmp_reg32_m32(rt2, ((unsigned int *)dst->f.i.rs)+1); // 6
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
   else if (rt_64bit == -1)
   {
      int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
      int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);

      cmp_reg32_m32(rs1, (unsigned int *)dst->f.i.rt);
      jne_rj(20);
      cmp_reg32_m32(rs2, ((unsigned int *)dst->f.i.rt)+1); // 6
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
   else
   {
      int rs1, rs2, rt1, rt2;
      if (!rs_64bit)
      {
         rt1 = allocate_64_register1((unsigned int *)dst->f.i.rt);
         rt2 = allocate_64_register2((unsigned int *)dst->f.i.rt);
         rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
         rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
      }
      else
      {
         rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
         rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
         rt1 = allocate_64_register1((unsigned int *)dst->f.i.rt);
         rt2 = allocate_64_register2((unsigned int *)dst->f.i.rt);
      }
      cmp_reg32_reg32(rs1, rt1);
      jne_rj(16);
      cmp_reg32_reg32(rs2, rt2); // 2
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
#endif
}

static void genblez_test(void)
{
   int rs_64bit = is64((unsigned int *)dst->f.i.rs);

#ifdef __x86_64__
   if (rs_64bit == 0)
   {
      int rs = allocate_register_32((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs, 0);
      setle_m8rel((unsigned char *) &branch_taken);
   }
   else
   {
      int rs = allocate_register_64((uint64_t *)dst->f.i.rs);

      cmp_reg64_imm8(rs, 0);
      setle_m8rel((unsigned char *) &branch_taken);
   }
#else
   if (!rs_64bit)
   {
      int rs = allocate_register((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs, 0);
      jg_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
   }
   else if (rs_64bit == -1)
   {
      cmp_m32_imm32(((unsigned int *)dst->f.i.rs)+1, 0);
      jg_rj(14);
      jne_rj(24); // 2
      cmp_m32_imm32((unsigned int *)dst->f.i.rs, 0); // 10
      je_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
   else
   {
      int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
      int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs2, 0);
      jg_rj(10);
      jne_rj(20); // 2
      cmp_reg32_imm32(rs1, 0); // 6
      je_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
#endif
}

static void genbgtz_test(void)
{
   int rs_64bit = is64((unsigned int *)dst->f.i.rs);
#ifdef __x86_64__
   if (rs_64bit == 0)
   {
      int rs = allocate_register_32((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs, 0);
      setg_m8rel((unsigned char *) &branch_taken);
   }
   else
   {
      int rs = allocate_register_64((uint64_t *)dst->f.i.rs);

      cmp_reg64_imm8(rs, 0);
      setg_m8rel((unsigned char *) &branch_taken);
   }
#else
   if (!rs_64bit)
   {
      int rs = allocate_register((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs, 0);
      jle_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
   }
   else if (rs_64bit == -1)
   {
      cmp_m32_imm32(((unsigned int *)dst->f.i.rs)+1, 0);
      jl_rj(14);
      jne_rj(24); // 2
      cmp_m32_imm32((unsigned int *)dst->f.i.rs, 0); // 10
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
   else
   {
      int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
      int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs2, 0);
      jl_rj(10);
      jne_rj(20); // 2
      cmp_reg32_imm32(rs1, 0); // 6
      jne_rj(12); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
   }
#endif
}

#ifdef __x86_64__
static void ld_register_alloc(int *pGpr1, int *pGpr2, int *pBase1, int *pBase2)
{
   int gpr1, gpr2, base1, base2 = 0;

   if (dst->f.i.rs == dst->f.i.rt)
   {
      allocate_register_32((unsigned int*)dst->f.r.rs);          // tell regcache we need to read RS register here
      gpr1 = allocate_register_32_w((unsigned int*)dst->f.r.rt); // tell regcache we will modify RT register during this instruction
      gpr2 = lock_register(lru_register());                      // free and lock least recently used register for usage here
      add_reg32_imm32(gpr1, (int)dst->f.i.immediate);
      mov_reg32_reg32(gpr2, gpr1);
   }
   else
   {
      gpr2 = allocate_register_32((unsigned int*)dst->f.r.rs);   // tell regcache we need to read RS register here
      gpr1 = allocate_register_32_w((unsigned int*)dst->f.r.rt); // tell regcache we will modify RT register during this instruction
      free_register(gpr2);                                       // write out gpr2 if dirty because I'm going to trash it right now
      add_reg32_imm32(gpr2, (int)dst->f.i.immediate);
      mov_reg32_reg32(gpr1, gpr2);
      lock_register(gpr2);                                       // lock the freed gpr2 it so it doesn't get returned in the lru query
   }
   base1 = lock_register(lru_base_register());                  // get another lru register
   if (!g_dev.r4300.recomp.fast_memory)
   {
      base2 = lock_register(lru_base_register());                // and another one if necessary
      unlock_register(base2);
   }
   unlock_register(base1);                                      // unlock the locked registers (they are 
   unlock_register(gpr2);
   set_register_state(gpr1, NULL, 0, 0);                        // clear gpr1 state because it hasn't been written yet -
   // we don't want it to be pushed/popped around read_rdramX call
   *pGpr1 = gpr1;
   *pGpr2 = gpr2;
   *pBase1 = base1;
   *pBase2 = base2;
}
#endif


/* global functions */

void gennotcompiled(void)
{
#ifdef __x86_64__
   free_registers_move_start();

   mov_reg64_imm64(RAX, (uint64_t) dst);
   mov_memoffs64_rax((uint64_t *) &PC); /* RIP-relative will not work here */
   mov_reg64_imm64(RAX, (uint64_t) cached_interpreter_table.NOTCOMPILED);
   call_reg64(RAX);
#else
   free_all_registers();
   simplify_access();

   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst));
   mov_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED);
   call_reg32(EAX);
#endif
}

void genlink_subblock(void)
{
   free_all_registers();
   jmp(dst->addr+4);
}

void gencallinterp(uintptr_t addr, int jump)
{
#ifdef __x86_64__
   free_registers_move_start();

   if (jump)
      mov_m32rel_imm32((unsigned int*)(&dyna_interp), 1);

   mov_reg64_imm64(RAX, (uint64_t) dst);
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, addr);
   call_reg64(RAX);

   if (jump)
   {
      mov_m32rel_imm32((unsigned int*)(&dyna_interp), 0);
      mov_reg64_imm64(RAX, (uint64_t)dyna_jump);
      call_reg64(RAX);
   }
#else
   free_all_registers();
   simplify_access();
   if (jump)
      mov_m32_imm32((unsigned int*)(&dyna_interp), 1);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst));
   mov_reg32_imm32(EAX, addr);
   call_reg32(EAX);
   if (jump)
   {
      mov_m32_imm32((unsigned int*)(&dyna_interp), 0);
      mov_reg32_imm32(EAX, (unsigned int)dyna_jump);
      call_reg32(EAX);
   }
#endif
}

void gendelayslot(void)
{
#ifdef __x86_64__
   mov_m32rel_imm32((void*)(&g_dev.r4300.delay_slot), 1);
   recompile_opcode();

   free_all_registers();
   genupdate_count(dst->addr+4);

   mov_m32rel_imm32((void*)(&g_dev.r4300.delay_slot), 0);
#else
   mov_m32_imm32(&g_dev.r4300.delay_slot, 1);
   recompile_opcode();

   free_all_registers();
   genupdate_count(dst->addr+4);

   mov_m32_imm32(&g_dev.r4300.delay_slot, 0);
#endif
}

void genni(void)
{
   gencallinterp((native_type)cached_interpreter_table.NI, 0);
}

void genreserved(void)
{
   gencallinterp((native_type)cached_interpreter_table.RESERVED, 0);
}

void genfin_block(void)
{
   gencallinterp((native_type)cached_interpreter_table.FIN_BLOCK, 0);
}

void gencheck_interrupt_reg(void) // addr is in EAX
{
#ifdef __x86_64__
   mov_xreg32_m32rel(EBX, (void*)&next_interrupt);
   cmp_xreg32_m32rel(EBX, (void*)&g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(0);
   jump_start_rel8();

   mov_m32rel_xreg32((unsigned int*)(&fake_instr.addr), EAX);
   mov_reg64_imm64(RAX, (uint64_t) (&fake_instr));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) gen_interrupt);
   call_reg64(RAX);

   jump_end_rel8();
#else
   mov_reg32_m32(EBX, &next_interrupt);
   cmp_reg32_m32(EBX, &g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(22);
   mov_memoffs32_eax((unsigned int*)(&fake_instr.addr)); // 5
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(&fake_instr)); // 10
   mov_reg32_imm32(EAX, (unsigned int)gen_interrupt); // 5
   call_reg32(EAX); // 2
#endif
}

void gennop(void)
{
}

void genj(void)
{
#ifdef INTERPRET_J
   gencallinterp((native_type)cached_interpreter_table.J, 1);
#else
   unsigned int naddr;

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.J, 1);
      return;
   }

   gendelayslot();
   naddr = ((dst-1)->f.j.inst_index<<2) | (dst->addr & 0xF0000000);

#ifdef __x86_64__
   mov_m32rel_imm32((void*)(&last_addr), naddr);
#else
   mov_m32_imm32(&last_addr, naddr);
#endif
   gencheck_interrupt((native_type) &actual->block[(naddr-actual->start)/4]);
   jmp(naddr);
#endif
}

void genj_out(void)
{
#ifdef INTERPRET_J_OUT
   gencallinterp((native_type)cached_interpreter_table.J_OUT, 1);
#else
   unsigned int naddr;

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.J_OUT, 1);
      return;
   }

   gendelayslot();
   naddr = ((dst-1)->f.j.inst_index<<2) | (dst->addr & 0xF0000000);

#ifdef __x86_64__
   mov_m32rel_imm32((void*)(&last_addr), naddr);
   gencheck_interrupt_out(naddr);
   mov_m32rel_imm32(&jump_to_address, naddr);
   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t)jump_to_func);
   call_reg64(RAX);
#else
   mov_m32_imm32(&last_addr, naddr);
   gencheck_interrupt_out(naddr);
   mov_m32_imm32(&jump_to_address, naddr);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)jump_to_func);
   call_reg32(EAX);
#endif
#endif
}

void genj_idle(void)
{
#ifdef INTERPRET_J_IDLE
   gencallinterp((native_type)cached_interpreter_table.J_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.J_IDLE, 1);
      return;
   }

#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int *)(&next_interrupt));
   sub_xreg32_m32rel(EAX, (unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(EAX, 3);
   jbe_rj(12);

   and_eax_imm32(0xFFFFFFFC);  // 5
   add_m32rel_xreg32((unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]), EAX); // 7
#else
   mov_eax_memoffs32((unsigned int *)(&next_interrupt));
   sub_reg32_m32(EAX, (unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(EAX, 3);
   jbe_rj(11);

   and_eax_imm32(0xFFFFFFFC);  // 5
   add_m32_reg32((unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]), EAX); // 6
#endif

   genj();
#endif
}

void genjal(void)
{
#ifdef INTERPRET_JAL
   gencallinterp((native_type)cached_interpreter_table.JAL, 1);
#else
   unsigned int naddr;

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.JAL, 1);
      return;
   }

   gendelayslot();

#ifdef __x86_64__
   mov_m32rel_imm32((unsigned int *)(reg + 31), dst->addr + 4);
   if (((dst->addr + 4) & 0x80000000))
      mov_m32rel_imm32((unsigned int *)(&reg[31])+1, 0xFFFFFFFF);
   else
      mov_m32rel_imm32((unsigned int *)(&reg[31])+1, 0);

   naddr = ((dst-1)->f.j.inst_index<<2) | (dst->addr & 0xF0000000);

   mov_m32rel_imm32((void*)(&last_addr), naddr);
#else
   mov_m32_imm32((unsigned int *)(reg + 31), dst->addr + 4);
   if (((dst->addr + 4) & 0x80000000))
      mov_m32_imm32((unsigned int *)(&reg[31])+1, 0xFFFFFFFF);
   else
      mov_m32_imm32((unsigned int *)(&reg[31])+1, 0);

   naddr = ((dst-1)->f.j.inst_index<<2) | (dst->addr & 0xF0000000);

   mov_m32_imm32(&last_addr, naddr);
#endif
   gencheck_interrupt((native_type) &actual->block[(naddr-actual->start)/4]);
   jmp(naddr);
#endif
}

void genjal_out(void)
{
#ifdef INTERPRET_JAL_OUT
   gencallinterp((native_type)cached_interpreter_table.JAL_OUT, 1);
#else
   unsigned int naddr;

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.JAL_OUT, 1);
      return;
   }

   gendelayslot();

#ifdef __x86_64__
   mov_m32rel_imm32((unsigned int *)(reg + 31), dst->addr + 4);
   if (((dst->addr + 4) & 0x80000000))
      mov_m32rel_imm32((unsigned int *)(&reg[31])+1, 0xFFFFFFFF);
   else
      mov_m32rel_imm32((unsigned int *)(&reg[31])+1, 0);

   naddr = ((dst-1)->f.j.inst_index<<2) | (dst->addr & 0xF0000000);

   mov_m32rel_imm32((void*)(&last_addr), naddr);
   gencheck_interrupt_out(naddr);
   mov_m32rel_imm32(&jump_to_address, naddr);
   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) jump_to_func);
   call_reg64(RAX);
#else
   mov_m32_imm32((unsigned int *)(reg + 31), dst->addr + 4);
   if (((dst->addr + 4) & 0x80000000))
      mov_m32_imm32((unsigned int *)(&reg[31])+1, 0xFFFFFFFF);
   else
      mov_m32_imm32((unsigned int *)(&reg[31])+1, 0);

   naddr = ((dst-1)->f.j.inst_index<<2) | (dst->addr & 0xF0000000);

   mov_m32_imm32(&last_addr, naddr);
   gencheck_interrupt_out(naddr);
   mov_m32_imm32(&jump_to_address, naddr);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)jump_to_func);
   call_reg32(EAX);
#endif
#endif
}

void genjal_idle(void)
{
#ifdef INTERPRET_JAL_IDLE
   gencallinterp((native_type)cached_interpreter_table.JAL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.JAL_IDLE, 1);
      return;
   }

#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int *)(&next_interrupt));
   sub_xreg32_m32rel(EAX, (unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(EAX, 3);
   jbe_rj(12);

   and_eax_imm32(0xFFFFFFFC);  // 5
   add_m32rel_xreg32((unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]), EAX); // 7
#else
   mov_eax_memoffs32((unsigned int *)(&next_interrupt));
   sub_reg32_m32(EAX, (unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(EAX, 3);
   jbe_rj(11);

   and_eax_imm32(0xFFFFFFFC);
   add_m32_reg32((unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]), EAX);
#endif

   genjal();
#endif
}

void gentest(void)
{
#ifdef __x86_64__
   cmp_m32rel_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);
   jump_start_rel32();

   mov_m32rel_imm32((void*)(&last_addr), dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt((uint64_t) (dst + (dst-1)->f.i.immediate));
   jmp(dst->addr + (dst-1)->f.i.immediate*4);

   jump_end_rel32();

   mov_m32rel_imm32((void*)(&last_addr), dst->addr + 4);
#else
   cmp_m32_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);

   jump_start_rel32();

   mov_m32_imm32(&last_addr, dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt((unsigned int)(dst + (dst-1)->f.i.immediate));
   jmp(dst->addr + (dst-1)->f.i.immediate*4);

   jump_end_rel32();

   mov_m32_imm32(&last_addr, dst->addr + 4);
#endif
   gencheck_interrupt((native_type)(dst + 1));
   jmp(dst->addr + 4);
}

void genbeq(void)
{
#ifdef INTERPRET_BEQ
   gencallinterp((native_type)cached_interpreter_table.BEQ, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BEQ, 1);
      return;
   }

   genbeq_test();
   gendelayslot();
   gentest();
#endif
}

void gentest_out(void)
{
#ifdef __x86_64__
   cmp_m32rel_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);
   jump_start_rel32();

   mov_m32rel_imm32((void*)(&last_addr), dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt_out(dst->addr + (dst-1)->f.i.immediate*4);
   mov_m32rel_imm32(&jump_to_address, dst->addr + (dst-1)->f.i.immediate*4);
   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) jump_to_func);
   call_reg64(RAX);
   jump_end_rel32();

   mov_m32rel_imm32((void*)(&last_addr), dst->addr + 4);
#else
   cmp_m32_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);

   jump_start_rel32();

   mov_m32_imm32(&last_addr, dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt_out(dst->addr + (dst-1)->f.i.immediate*4);
   mov_m32_imm32(&jump_to_address, dst->addr + (dst-1)->f.i.immediate*4);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)jump_to_func);
   call_reg32(EAX);

   jump_end_rel32();

   mov_m32_imm32(&last_addr, dst->addr + 4);
#endif
   gencheck_interrupt((native_type) (dst + 1));
   jmp(dst->addr + 4);
}

void genbeq_out(void)
{
#ifdef INTERPRET_BEQ_OUT
   gencallinterp((native_type)cached_interpreter_table.BEQ_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BEQ_OUT, 1);
      return;
   }

   genbeq_test();
   gendelayslot();
   gentest_out();
#endif
}

void gentest_idle(void)
{
   int reg;

   reg = lru_register();
   free_register(reg);

#ifdef __x86_64__
   cmp_m32rel_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);
   jump_start_rel32();

   mov_xreg32_m32rel(reg, (unsigned int *)(&next_interrupt));
   sub_xreg32_m32rel(reg, (unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(reg, 3);
   jbe_rj(0);
   jump_start_rel8();

   and_reg32_imm32(reg, 0xFFFFFFFC);
   add_m32rel_xreg32((unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]), reg);

   jump_end_rel8();
#else
   cmp_m32_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);

   jump_start_rel32();

   mov_reg32_m32(reg, (unsigned int *)(&next_interrupt));
   sub_reg32_m32(reg, (unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(reg, 5);
   jbe_rj(18);

   sub_reg32_imm32(reg, 2); // 6
   and_reg32_imm32(reg, 0xFFFFFFFC); // 6
   add_m32_reg32((unsigned int *)(&g_cp0_regs[CP0_COUNT_REG]), reg); // 6
#endif
   jump_end_rel32();
}

void genbeq_idle(void)
{
#ifdef INTERPRET_BEQ_IDLE
   gencallinterp((native_type)cached_interpreter_table.BEQ_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BEQ_IDLE, 1);
      return;
   }

   genbeq_test();
   gentest_idle();
   genbeq();
#endif
}

void genbne(void)
{
#ifdef INTERPRET_BNE
   gencallinterp((native_type)cached_interpreter_table.BNE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BNE, 1);
      return;
   }

   genbne_test();
   gendelayslot();
   gentest();
#endif
}

void genbne_out(void)
{
#ifdef INTERPRET_BNE_OUT
   gencallinterp((native_type)cached_interpreter_table.BNE_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BNE_OUT, 1);
      return;
   }

   genbne_test();
   gendelayslot();
   gentest_out();
#endif
}

void genbne_idle(void)
{
#ifdef INTERPRET_BNE_IDLE
   gencallinterp((native_type)cached_interpreter_table.BNE_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BNE_IDLE, 1);
      return;
   }

   genbne_test();
   gentest_idle();
   genbne();
#endif
}

void genblez(void)
{
#ifdef INTERPRET_BLEZ
   gencallinterp((native_type)cached_interpreter_table.BLEZ, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLEZ, 1);
      return;
   }

   genblez_test();
   gendelayslot();
   gentest();
#endif
}

void genblez_out(void)
{
#ifdef INTERPRET_BLEZ_OUT
   gencallinterp((native_type)cached_interpreter_table.BLEZ_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLEZ_OUT, 1);
      return;
   }

   genblez_test();
   gendelayslot();
   gentest_out();
#endif
}

void genblez_idle(void)
{
#ifdef INTERPRET_BLEZ_IDLE
   gencallinterp((native_type)cached_interpreter_table.BLEZ_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLEZ_IDLE, 1);
      return;
   }

   genblez_test();
   gentest_idle();
   genblez();
#endif
}

void genbgtz(void)
{
#ifdef INTERPRET_BGTZ
   gencallinterp((native_type)cached_interpreter_table.BGTZ, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGTZ, 1);
      return;
   }

   genbgtz_test();
   gendelayslot();
   gentest();
#endif
}

void genbgtz_out(void)
{
#ifdef INTERPRET_BGTZ_OUT
   gencallinterp((native_type)cached_interpreter_table.BGTZ_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGTZ_OUT, 1);
      return;
   }

   genbgtz_test();
   gendelayslot();
   gentest_out();
#endif
}

void genbgtz_idle(void)
{
#ifdef INTERPRET_BGTZ_IDLE
   gencallinterp((native_type)cached_interpreter_table.BGTZ_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGTZ_IDLE, 1);
      return;
   }

   genbgtz_test();
   gentest_idle();
   genbgtz();
#endif
}

void genaddi(void)
{
#ifdef INTERPRET_ADDI
   gencallinterp((native_type)cached_interpreter_table.ADDI, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_32((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_32_w((unsigned int *)dst->f.i.rt);
#else
   int rs = allocate_register((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_w((unsigned int *)dst->f.i.rt);
#endif

   mov_reg32_reg32(rt, rs);
   add_reg32_imm32(rt,(int)dst->f.i.immediate);
#endif
}

void genaddiu(void)
{
#ifdef INTERPRET_ADDIU
   gencallinterp((native_type)cached_interpreter_table.ADDIU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_32((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_32_w((unsigned int *)dst->f.i.rt);
#else
   int rs = allocate_register((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_w((unsigned int *)dst->f.i.rt);
#endif

   mov_reg32_reg32(rt, rs);
   add_reg32_imm32(rt,(int)dst->f.i.immediate);
#endif
}

void genslti(void)
{
#ifdef INTERPRET_SLTI
   gencallinterp((native_type)cached_interpreter_table.SLTI, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *) dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *) dst->f.i.rt);
   int imm = (int) dst->f.i.immediate;

   cmp_reg64_imm32(rs, imm);
   setl_reg8(rt);
   and_reg64_imm8(rt, 1);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_w((unsigned int *)dst->f.i.rt);
   int64_t imm = (int64_t)dst->f.i.immediate;

   cmp_reg32_imm32(rs2, (unsigned int)(imm >> 32));
   jl_rj(17);
   jne_rj(8); // 2
   cmp_reg32_imm32(rs1, (unsigned int)imm); // 6
   jl_rj(7); // 2
   mov_reg32_imm32(rt, 0); // 5
   jmp_imm_short(5); // 2
   mov_reg32_imm32(rt, 1); // 5
#endif
#endif
}

void gensltiu(void)
{
#ifdef INTERPRET_SLTIU
   gencallinterp((native_type)cached_interpreter_table.SLTIU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *)dst->f.i.rt);
   int imm = (int) dst->f.i.immediate;

   cmp_reg64_imm32(rs, imm);
   setb_reg8(rt);
   and_reg64_imm8(rt, 1);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_w((unsigned int *)dst->f.i.rt);
   int64_t imm = (int64_t)dst->f.i.immediate;

   cmp_reg32_imm32(rs2, (unsigned int)(imm >> 32));
   jb_rj(17);
   jne_rj(8); // 2
   cmp_reg32_imm32(rs1, (unsigned int)imm); // 6
   jb_rj(7); // 2
   mov_reg32_imm32(rt, 0); // 5
   jmp_imm_short(5); // 2
   mov_reg32_imm32(rt, 1); // 5
#endif
#endif
}

void genandi(void)
{
#ifdef INTERPRET_ANDI
   gencallinterp((native_type)cached_interpreter_table.ANDI, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *)dst->f.i.rt);

   mov_reg64_reg64(rt, rs);
   and_reg64_imm32(rt, (unsigned short)dst->f.i.immediate);
#else
   int rs = allocate_register((unsigned int *)dst->f.i.rs);
   int rt = allocate_register_w((unsigned int *)dst->f.i.rt);

   mov_reg32_reg32(rt, rs);
   and_reg32_imm32(rt, (unsigned short)dst->f.i.immediate);
#endif
#endif
}

void genori(void)
{
#ifdef INTERPRET_ORI
   gencallinterp((native_type)cached_interpreter_table.ORI, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *) dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *) dst->f.i.rt);

   mov_reg64_reg64(rt, rs);
   or_reg64_imm32(rt, (unsigned short)dst->f.i.immediate);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
   int rt1 = allocate_64_register1_w((unsigned int *)dst->f.i.rt);
   int rt2 = allocate_64_register2_w((unsigned int *)dst->f.i.rt);

   mov_reg32_reg32(rt1, rs1);
   mov_reg32_reg32(rt2, rs2);
   or_reg32_imm32(rt1, (unsigned short)dst->f.i.immediate);
#endif
#endif
}

void genxori(void)
{
#ifdef INTERPRET_XORI
   gencallinterp((native_type)cached_interpreter_table.XORI, 0);
#else

#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *)dst->f.i.rt);

   mov_reg64_reg64(rt, rs);
   xor_reg64_imm32(rt, (unsigned short)dst->f.i.immediate);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
   int rt1 = allocate_64_register1_w((unsigned int *)dst->f.i.rt);
   int rt2 = allocate_64_register2_w((unsigned int *)dst->f.i.rt);

   mov_reg32_reg32(rt1, rs1);
   mov_reg32_reg32(rt2, rs2);
   xor_reg32_imm32(rt1, (unsigned short)dst->f.i.immediate);
#endif
#endif
}

void genlui(void)
{
#ifdef INTERPRET_LUI
   gencallinterp((native_type)cached_interpreter_table.LUI, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_32_w((unsigned int *)dst->f.i.rt);
#else
   int rt = allocate_register_w((unsigned int *)dst->f.i.rt);
#endif

   mov_reg32_imm32(rt, (unsigned int)dst->f.i.immediate << 16);
#endif
}

void gentestl(void)
{
#ifdef __x86_64__
   cmp_m32rel_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);
   jump_start_rel32();

   gendelayslot();
   mov_m32rel_imm32((void*)(&last_addr), dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt((uint64_t) (dst + (dst-1)->f.i.immediate));
   jmp(dst->addr + (dst-1)->f.i.immediate*4);

   jump_end_rel32();

   genupdate_count(dst->addr-4);
   mov_m32rel_imm32((void*)(&last_addr), dst->addr + 4);
#else
   cmp_m32_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);

   jump_start_rel32();

   gendelayslot();
   mov_m32_imm32(&last_addr, dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt((unsigned int)(dst + (dst-1)->f.i.immediate));
   jmp(dst->addr + (dst-1)->f.i.immediate*4);

   jump_end_rel32();

   genupdate_count(dst->addr+4);
   mov_m32_imm32(&last_addr, dst->addr + 4);
#endif

   gencheck_interrupt((native_type) (dst + 1));
   jmp(dst->addr + 4);
}

void genbeql(void)
{
#ifdef INTERPRET_BEQL
   gencallinterp((native_type)cached_interpreter_table.BEQL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BEQL, 1);
      return;
   }

   genbeq_test();
   free_all_registers();
   gentestl();
#endif
}

void gentestl_out(void)
{
#ifdef __x86_64__
   cmp_m32rel_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);
   jump_start_rel32();

   gendelayslot();
   mov_m32rel_imm32((void*)(&last_addr), dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt_out(dst->addr + (dst-1)->f.i.immediate*4);
   mov_m32rel_imm32(&jump_to_address, dst->addr + (dst-1)->f.i.immediate*4);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) jump_to_func);
   call_reg64(RAX);

   jump_end_rel32();

   genupdate_count(dst->addr-4);
   mov_m32rel_imm32((void*)(&last_addr), dst->addr + 4);
#else
   cmp_m32_imm32((unsigned int *)(&branch_taken), 0);
   je_near_rj(0);

   jump_start_rel32();

   gendelayslot();
   mov_m32_imm32(&last_addr, dst->addr + (dst-1)->f.i.immediate*4);
   gencheck_interrupt_out(dst->addr + (dst-1)->f.i.immediate*4);
   mov_m32_imm32(&jump_to_address, dst->addr + (dst-1)->f.i.immediate*4);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)jump_to_func);
   call_reg32(EAX);

   jump_end_rel32();

   genupdate_count(dst->addr+4);
   mov_m32_imm32(&last_addr, dst->addr + 4);
#endif
   gencheck_interrupt((native_type)(dst + 1));
   jmp(dst->addr + 4);
}

void genbeql_out(void)
{
#ifdef INTERPRET_BEQL_OUT
   gencallinterp((native_type)cached_interpreter_table.BEQL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BEQL_OUT, 1);
      return;
   }

   genbeq_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbeql_idle(void)
{
#ifdef INTERPRET_BEQL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BEQL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BEQL_IDLE, 1);
      return;
   }

   genbeq_test();
   gentest_idle();
   genbeql();
#endif
}

void genbnel(void)
{
#ifdef INTERPRET_BNEL
   gencallinterp((native_type)cached_interpreter_table.BNEL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BNEL, 1);
      return;
   }

   genbne_test();
   free_all_registers();
   gentestl();
#endif
}

void genbnel_out(void)
{
#ifdef INTERPRET_BNEL_OUT
   gencallinterp((native_type)cached_interpreter_table.BNEL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BNEL_OUT, 1);
      return;
   }

   genbne_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbnel_idle(void)
{
#ifdef INTERPRET_BNEL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BNEL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BNEL_IDLE, 1);
      return;
   }

   genbne_test();
   gentest_idle();
   genbnel();
#endif
}

void genblezl(void)
{
#ifdef INTERPRET_BLEZL
   gencallinterp((native_type)cached_interpreter_table.BLEZL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLEZL, 1);
      return;
   }

   genblez_test();
   free_all_registers();
   gentestl();
#endif
}

void genblezl_out(void)
{
#ifdef INTERPRET_BLEZL_OUT
   gencallinterp((native_type)cached_interpreter_table.BLEZL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLEZL_OUT, 1);
      return;
   }

   genblez_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genblezl_idle(void)
{
#ifdef INTERPRET_BLEZL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BLEZL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLEZL_IDLE, 1);
      return;
   }

   genblez_test();
   gentest_idle();
   genblezl();
#endif
}

void genbgtzl(void)
{
#ifdef INTERPRET_BGTZL
   gencallinterp((native_type)cached_interpreter_table.BGTZL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGTZL, 1);
      return;
   }

   genbgtz_test();
   free_all_registers();
   gentestl();
#endif
}

void genbgtzl_out(void)
{
#ifdef INTERPRET_BGTZL_OUT
   gencallinterp((native_type)cached_interpreter_table.BGTZL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGTZL_OUT, 1);
      return;
   }

   genbgtz_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbgtzl_idle(void)
{
#ifdef INTERPRET_BGTZL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BGTZL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGTZL_IDLE, 1);
      return;
   }

   genbgtz_test();
   gentest_idle();
   genbgtzl();
#endif
}

void gendaddi(void)
{
#ifdef INTERPRET_DADDI
   gencallinterp((native_type)cached_interpreter_table.DADDI, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *)dst->f.i.rt);

   mov_reg64_reg64(rt, rs);
   add_reg64_imm32(rt, (int) dst->f.i.immediate);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
   int rt1 = allocate_64_register1_w((unsigned int *)dst->f.i.rt);
   int rt2 = allocate_64_register2_w((unsigned int *)dst->f.i.rt);

   mov_reg32_reg32(rt1, rs1);
   mov_reg32_reg32(rt2, rs2);
   add_reg32_imm32(rt1, dst->f.i.immediate);
   adc_reg32_imm32(rt2, (int)dst->f.i.immediate>>31);
#endif
#endif
}

void gendaddiu(void)
{
#ifdef INTERPRET_DADDIU
   gencallinterp((native_type)cached_interpreter_table.DADDIU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.i.rs);
   int rt = allocate_register_64_w((uint64_t *)dst->f.i.rt);

   mov_reg64_reg64(rt, rs);
   add_reg64_imm32(rt, (int) dst->f.i.immediate);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.i.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);
   int rt1 = allocate_64_register1_w((unsigned int *)dst->f.i.rt);
   int rt2 = allocate_64_register2_w((unsigned int *)dst->f.i.rt);

   mov_reg32_reg32(rt1, rs1);
   mov_reg32_reg32(rt2, rs2);
   add_reg32_imm32(rt1, dst->f.i.immediate);
   adc_reg32_imm32(rt2, (int)dst->f.i.immediate>>31);
#endif
#endif
}

void gencache(void)
{
}

void genldl(void)
{
#ifdef INTERPRET_LD
   gencallinterp((unsigned int)cached_interpreter_table.LD, 0);
#else
#ifdef __x86_64__
   gencallinterp((native_type)cached_interpreter_table.LDL, 0);
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmemd);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdramd);
   }
   je_rj(51);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmemd); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(dst->f.i.rt)); // 5
   mov_reg32_m32(ECX, (unsigned int *)(dst->f.i.rt)+1); // 6
   jmp_imm_short(18); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(EAX, EBX, ((unsigned int)g_dev.ri.rdram.dram)+4); // 6
   mov_reg32_preg32pimm32(ECX, EBX, ((unsigned int)g_dev.ri.rdram.dram)); // 6

   set_64_register_state(EAX, ECX, (unsigned int*)dst->f.i.rt, 1);
#endif
#endif
}

void genldr(void)
{
   gencallinterp((native_type)cached_interpreter_table.LDR, 0);
}

void genlb(void)
{
#ifdef INTERPRET_LB
   gencallinterp((native_type)cached_interpreter_table.LB, 0);
#else

#ifdef __x86_64__
   int gpr1, gpr2, base1, base2 = 0;

   free_registers_move_start();

   ld_register_alloc(&gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(base1, (uint64_t) readmemb);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_reg32_imm32(gpr1, 0xDF800000);
      cmp_reg32_imm32(gpr1, 0x80000000);
   }
   else
   {
      mov_reg64_imm64(base2, (uint64_t) read_rdramb);
      shr_reg32_imm8(gpr1, 16);
      mov_reg64_preg64x8preg64(gpr1, gpr1, base1);
      cmp_reg64_reg64(gpr1, base2);
   }
   je_rj(0);
   jump_start_rel8();

   mov_reg64_imm64(gpr1, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), gpr1);
   mov_m32rel_xreg32((unsigned int *)(&address), gpr2);
   mov_reg64_imm64(gpr1, (uint64_t) dst->f.i.rt);
   mov_m64rel_xreg64((uint64_t *)(&rdword), gpr1);
   shr_reg32_imm8(gpr2, 16);
   mov_reg64_preg64x8preg64(gpr2, gpr2, base1);
   call_reg64(gpr2);
   movsx_xreg32_m8rel(gpr1, (unsigned char *)dst->f.i.rt);
   jmp_imm_short(24);

   jump_end_rel8();
   mov_reg64_imm64(base1, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(gpr2, 3); // 4
   movsx_reg32_8preg64preg64(gpr1, gpr2, base1); // 4

   set_register_state(gpr1, (unsigned int*)dst->f.i.rt, 1, 0);
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmemb);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdramb);
   }
   je_rj(47);

   mov_m32_imm32((unsigned int *)&PC, (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmemb); // 7
   call_reg32(EBX); // 2
   movsx_reg32_m8(EAX, (unsigned char *)dst->f.i.rt); // 7
   jmp_imm_short(16); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 3); // 3
   movsx_reg32_8preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 7

   set_register_state(EAX, (unsigned int*)dst->f.i.rt, 1, 0);
#endif
#endif
}

void genlh(void)
{
#ifdef INTERPRET_LH
   gencallinterp((native_type)cached_interpreter_table.LH, 0);
#else
#ifdef __x86_64__
   int gpr1, gpr2, base1, base2 = 0;
   free_registers_move_start();

   ld_register_alloc(&gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(base1, (uint64_t) readmemh);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_reg32_imm32(gpr1, 0xDF800000);
      cmp_reg32_imm32(gpr1, 0x80000000);
   }
   else
   {
      mov_reg64_imm64(base2, (uint64_t) read_rdramh);
      shr_reg32_imm8(gpr1, 16);
      mov_reg64_preg64x8preg64(gpr1, gpr1, base1);
      cmp_reg64_reg64(gpr1, base2);
   }
   je_rj(0);
   jump_start_rel8();

   mov_reg64_imm64(gpr1, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), gpr1);
   mov_m32rel_xreg32((unsigned int *)(&address), gpr2);
   mov_reg64_imm64(gpr1, (uint64_t) dst->f.i.rt);
   mov_m64rel_xreg64((uint64_t *)(&rdword), gpr1);
   shr_reg32_imm8(gpr2, 16);
   mov_reg64_preg64x8preg64(gpr2, gpr2, base1);
   call_reg64(gpr2);
   movsx_xreg32_m16rel(gpr1, (unsigned short *)dst->f.i.rt);
   jmp_imm_short(24);

   jump_end_rel8();   
   mov_reg64_imm64(base1, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(gpr2, 2); // 4
   movsx_reg32_16preg64preg64(gpr1, gpr2, base1); // 4

   set_register_state(gpr1, (unsigned int*)dst->f.i.rt, 1, 0);
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmemh);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdramh);
   }
   je_rj(47);

   mov_m32_imm32((unsigned int *)&PC, (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmemh); // 7
   call_reg32(EBX); // 2
   movsx_reg32_m16(EAX, (unsigned short *)dst->f.i.rt); // 7
   jmp_imm_short(16); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 2); // 3
   movsx_reg32_16preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 7

   set_register_state(EAX, (unsigned int*)dst->f.i.rt, 1, 0);
#endif
#endif
}

void genlwl(void)
{
   gencallinterp((native_type)cached_interpreter_table.LWL, 0);
}

void genlw(void)
{
#ifdef INTERPRET_LW
   gencallinterp((native_type)cached_interpreter_table.LW, 0);
#else
#ifdef __x86_64__
   int gpr1, gpr2, base1, base2 = 0;
   free_registers_move_start();

   ld_register_alloc(&gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(base1, (uint64_t) readmem);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_reg32_imm32(gpr1, 0xDF800000);
      cmp_reg32_imm32(gpr1, 0x80000000);
   }
   else
   {
      mov_reg64_imm64(base2, (uint64_t) read_rdram);
      shr_reg32_imm8(gpr1, 16);
      mov_reg64_preg64x8preg64(gpr1, gpr1, base1);
      cmp_reg64_reg64(gpr1, base2);
   }
   jne_rj(21);

   mov_reg64_imm64(base1, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(gpr2, 0x7FFFFF); // 6
   mov_reg32_preg64preg64(gpr1, gpr2, base1); // 3
   jmp_imm_short(0); // 2
   jump_start_rel8();

   mov_reg64_imm64(gpr1, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), gpr1);
   mov_m32rel_xreg32((unsigned int *)(&address), gpr2);
   mov_reg64_imm64(gpr1, (uint64_t) dst->f.i.rt);
   mov_m64rel_xreg64((uint64_t *)(&rdword), gpr1);
   shr_reg32_imm8(gpr2, 16);
   mov_reg64_preg64x8preg64(gpr1, gpr2, base1);
   call_reg64(gpr1);
   mov_xreg32_m32rel(gpr1, (unsigned int *)(dst->f.i.rt));

   jump_end_rel8();

   set_register_state(gpr1, (unsigned int*)dst->f.i.rt, 1, 0);     // set gpr1 state as dirty, and bound to r4300 reg RT
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmem);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdram);
   }
   je_rj(45);

   mov_m32_imm32((unsigned int *)&PC, (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmem); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(dst->f.i.rt)); // 5
   jmp_imm_short(12); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 6

   set_register_state(EAX, (unsigned int*)dst->f.i.rt, 1, 0);
#endif
#endif
}

void genlbu(void)
{
#ifdef INTERPRET_LBU
   gencallinterp((native_type)cached_interpreter_table.LBU, 0);
#else
#ifdef __x86_64__
   int gpr1, gpr2, base1, base2 = 0;
   free_registers_move_start();

   ld_register_alloc(&gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(base1, (uint64_t) readmemb);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_reg32_imm32(gpr1, 0xDF800000);
      cmp_reg32_imm32(gpr1, 0x80000000);
   }
   else
   {
      mov_reg64_imm64(base2, (uint64_t) read_rdramb);
      shr_reg32_imm8(gpr1, 16);
      mov_reg64_preg64x8preg64(gpr1, gpr1, base1);
      cmp_reg64_reg64(gpr1, base2);
   }
   je_rj(0);
   jump_start_rel8();

   mov_reg64_imm64(gpr1, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), gpr1);
   mov_m32rel_xreg32((unsigned int *)(&address), gpr2);
   mov_reg64_imm64(gpr1, (uint64_t) dst->f.i.rt);
   mov_m64rel_xreg64((uint64_t *)(&rdword), gpr1);
   shr_reg32_imm8(gpr2, 16);
   mov_reg64_preg64x8preg64(gpr2, gpr2, base1);
   call_reg64(gpr2);
   mov_xreg32_m32rel(gpr1, (unsigned int *)dst->f.i.rt);
   jmp_imm_short(23);

   jump_end_rel8();
   mov_reg64_imm64(base1, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(gpr2, 3); // 4
   mov_reg32_preg64preg64(gpr1, gpr2, base1); // 3

   and_reg32_imm32(gpr1, 0xFF);
   set_register_state(gpr1, (unsigned int*)dst->f.i.rt, 1, 0);
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmemb);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdramb);
   }
   je_rj(46);

   mov_m32_imm32((unsigned int *)&PC, (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmemb); // 7
   call_reg32(EBX); // 2
   mov_reg32_m32(EAX, (unsigned int *)dst->f.i.rt); // 6
   jmp_imm_short(15); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 3); // 3
   mov_reg32_preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 6

   and_eax_imm32(0xFF);

   set_register_state(EAX, (unsigned int*)dst->f.i.rt, 1, 0);
#endif
#endif
}

void genlhu(void)
{
#ifdef INTERPRET_LHU
   gencallinterp((native_type)cached_interpreter_table.LHU, 0);
#else
#ifdef __x86_64__
   int gpr1, gpr2, base1, base2 = 0;
   free_registers_move_start();

   ld_register_alloc(&gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(base1, (uint64_t) readmemh);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_reg32_imm32(gpr1, 0xDF800000);
      cmp_reg32_imm32(gpr1, 0x80000000);
   }
   else
   {
      mov_reg64_imm64(base2, (uint64_t) read_rdramh);
      shr_reg32_imm8(gpr1, 16);
      mov_reg64_preg64x8preg64(gpr1, gpr1, base1);
      cmp_reg64_reg64(gpr1, base2);
   }
   je_rj(0);
   jump_start_rel8();

   mov_reg64_imm64(gpr1, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), gpr1);
   mov_m32rel_xreg32((unsigned int *)(&address), gpr2);
   mov_reg64_imm64(gpr1, (uint64_t) dst->f.i.rt);
   mov_m64rel_xreg64((uint64_t *)(&rdword), gpr1);
   shr_reg32_imm8(gpr2, 16);
   mov_reg64_preg64x8preg64(gpr2, gpr2, base1);
   call_reg64(gpr2);
   mov_xreg32_m32rel(gpr1, (unsigned int *)dst->f.i.rt);
   jmp_imm_short(23);

   jump_end_rel8();
   mov_reg64_imm64(base1, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(gpr2, 2); // 4
   mov_reg32_preg64preg64(gpr1, gpr2, base1); // 3

   and_reg32_imm32(gpr1, 0xFFFF);
   set_register_state(gpr1, (unsigned int*)dst->f.i.rt, 1, 0);
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmemh);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdramh);
   }
   je_rj(46);

   mov_m32_imm32((unsigned int *)&PC, (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmemh); // 7
   call_reg32(EBX); // 2
   mov_reg32_m32(EAX, (unsigned int *)dst->f.i.rt); // 6
   jmp_imm_short(15); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 2); // 3
   mov_reg32_preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 6

   and_eax_imm32(0xFFFF);

   set_register_state(EAX, (unsigned int*)dst->f.i.rt, 1, 0);
#endif
#endif
}

void genlwr(void)
{
   gencallinterp((native_type)cached_interpreter_table.LWR, 0);
}

void genlwu(void)
{
#ifdef INTERPRET_LWU
   gencallinterp((native_type)cached_interpreter_table.LWU, 0);
#else
#ifdef __x86_64__
   int gpr1, gpr2, base1, base2 = 0;
   free_registers_move_start();

   ld_register_alloc(&gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(base1, (uint64_t) readmem);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_reg32_imm32(gpr1, 0xDF800000);
      cmp_reg32_imm32(gpr1, 0x80000000);
   }
   else
   {
      mov_reg64_imm64(base2, (uint64_t) read_rdram);
      shr_reg32_imm8(gpr1, 16);
      mov_reg64_preg64x8preg64(gpr1, gpr1, base1);
      cmp_reg64_reg64(gpr1, base2);
   }
   je_rj(0);
   jump_start_rel8();

   mov_reg64_imm64(gpr1, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), gpr1);
   mov_m32rel_xreg32((unsigned int *)(&address), gpr2);
   mov_reg64_imm64(gpr1, (uint64_t) dst->f.i.rt);
   mov_m64rel_xreg64((uint64_t *)(&rdword), gpr1);
   shr_reg32_imm8(gpr2, 16);
   mov_reg64_preg64x8preg64(gpr2, gpr2, base1);
   call_reg64(gpr2);
   mov_xreg32_m32rel(gpr1, (unsigned int *)dst->f.i.rt);
   jmp_imm_short(19);

   jump_end_rel8();
   mov_reg64_imm64(base1, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(gpr2, 0x7FFFFF); // 6
   mov_reg32_preg64preg64(gpr1, gpr2, base1); // 3

   set_register_state(gpr1, (unsigned int*)dst->f.i.rt, 1, 1);
#else
   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmem);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdram);
   }
   je_rj(45);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_imm32((unsigned int *)(&rdword), (unsigned int)dst->f.i.rt); // 10
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmem); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(dst->f.i.rt)); // 5
   jmp_imm_short(12); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 6

   xor_reg32_reg32(EBX, EBX);

   set_64_register_state(EAX, EBX, (unsigned int*)dst->f.i.rt, 1);
#endif
#endif
}

void gensb(void)
{
#ifdef INTERPRET_SB
   gencallinterp((native_type)cached_interpreter_table.SB, 0);
#else
#ifdef __x86_64__
   free_registers_move_start();

   mov_xreg8_m8rel(CL, (unsigned char *)dst->f.i.rt);
   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) writememb);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) write_rdramb);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(49);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_m8rel_xreg8((unsigned char *)(&cpu_byte), CL); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg32_m32rel(EAX, (unsigned int *)(&address)); // 7
   jmp_imm_short(25); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 3); // 4
   mov_preg64preg64_reg8(RBX, RSI, CL); // 3

   mov_reg64_imm64(RSI, (uint64_t) invalid_code);
   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg64preg64_imm8(RBX, RSI, 0);
   jne_rj(65);

   mov_reg64_imm64(RDI, (uint64_t) blocks); // 10
   mov_reg32_reg32(ECX, EBX); // 2
   mov_reg64_preg64x8preg64(RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(RBX, RBX, (int) offsetof(struct precomp_block, block)); // 7
   mov_reg64_imm64(RDI, (uint64_t) cached_interpreter_table.NOTCOMPILED); // 10
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg64_preg64preg64pimm32(RAX, RAX, RBX, (int) offsetof(struct precomp_instr, ops)); // 8
   cmp_reg64_reg64(RAX, RDI); // 3
   je_rj(4); // 2
   mov_preg64preg64_imm8(RCX, RSI, 1); // 4
#else
   free_all_registers();
   simplify_access();
   mov_reg8_m8(CL, (unsigned char *)dst->f.i.rt);
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)writememb);
      cmp_reg32_imm32(EAX, (unsigned int)write_rdramb);
   }
   je_rj(41);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m8_reg8((unsigned char *)(&cpu_byte), CL); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)writememb); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(&address)); // 5
   jmp_imm_short(17); // 2

   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 3); // 3
   mov_preg32pimm32_reg8(EBX, (unsigned int)g_dev.ri.rdram.dram, CL); // 6

   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg32pimm32_imm8(EBX, (unsigned int)invalid_code, 0);
   jne_rj(54);
   mov_reg32_reg32(ECX, EBX); // 2
   shl_reg32_imm8(EBX, 2); // 3
   mov_reg32_preg32pimm32(EBX, EBX, (unsigned int)blocks); // 6
   mov_reg32_preg32pimm32(EBX, EBX, (int)&actual->block - (int)actual); // 6
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg32_preg32preg32pimm32(EAX, EAX, EBX, (int)&dst->ops - (int)dst); // 7
   cmp_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED); // 6
   je_rj(7); // 2
   mov_preg32pimm32_imm8(ECX, (unsigned int)invalid_code, 1); // 7
#endif
#endif
}

void gensh(void)
{
#ifdef INTERPRET_SH
   gencallinterp((native_type)cached_interpreter_table.SH, 0);
#else
#if defined(__x86_64__)
   free_registers_move_start();

   mov_xreg16_m16rel(CX, (unsigned short *)dst->f.i.rt);
   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) writememh);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) write_rdramh);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(50);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_m16rel_xreg16((unsigned short *)(&cpu_hword), CX); // 8
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg32_m32rel(EAX, (unsigned int *)(&address)); // 7
   jmp_imm_short(26); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 2); // 4
   mov_preg64preg64_reg16(RBX, RSI, CX); // 4

   mov_reg64_imm64(RSI, (uint64_t) invalid_code);
   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg64preg64_imm8(RBX, RSI, 0);
   jne_rj(65);

   mov_reg64_imm64(RDI, (uint64_t) blocks); // 10
   mov_reg32_reg32(ECX, EBX); // 2
   mov_reg64_preg64x8preg64(RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(RBX, RBX, (int) offsetof(struct precomp_block, block)); // 7
   mov_reg64_imm64(RDI, (uint64_t) cached_interpreter_table.NOTCOMPILED); // 10
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg64_preg64preg64pimm32(RAX, RAX, RBX, (int) offsetof(struct precomp_instr, ops)); // 8
   cmp_reg64_reg64(RAX, RDI); // 3
   je_rj(4); // 2
   mov_preg64preg64_imm8(RCX, RSI, 1); // 4
#else
   free_all_registers();
   simplify_access();
   mov_reg16_m16(CX, (unsigned short *)dst->f.i.rt);
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)writememh);
      cmp_reg32_imm32(EAX, (unsigned int)write_rdramh);
   }
   je_rj(42);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m16_reg16((unsigned short *)(&cpu_hword), CX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)writememh); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(&address)); // 5
   jmp_imm_short(18); // 2

   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(BL, 2); // 3
   mov_preg32pimm32_reg16(EBX, (unsigned int)g_dev.ri.rdram.dram, CX); // 7

   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg32pimm32_imm8(EBX, (unsigned int)invalid_code, 0);
   jne_rj(54);
   mov_reg32_reg32(ECX, EBX); // 2
   shl_reg32_imm8(EBX, 2); // 3
   mov_reg32_preg32pimm32(EBX, EBX, (unsigned int)blocks); // 6
   mov_reg32_preg32pimm32(EBX, EBX, (int)&actual->block - (int)actual); // 6
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg32_preg32preg32pimm32(EAX, EAX, EBX, (int)&dst->ops - (int)dst); // 7
   cmp_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED); // 6
   je_rj(7); // 2
   mov_preg32pimm32_imm8(ECX, (unsigned int)invalid_code, 1); // 7
#endif
#endif
}

void genswl(void)
{
   gencallinterp((native_type)cached_interpreter_table.SWL, 0);
}

void gensw(void)
{
#ifdef INTERPRET_SW
   gencallinterp((native_type)cached_interpreter_table.SW, 0);
#else
#ifdef __x86_64__
   free_registers_move_start();

   mov_xreg32_m32rel(ECX, (unsigned int *)dst->f.i.rt);
   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) writemem);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) write_rdram);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(49);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_m32rel_xreg32((unsigned int *)(&cpu_word), ECX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg32_m32rel(EAX, (unsigned int *)(&address)); // 7
   jmp_imm_short(21); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg64preg64_reg32(RBX, RSI, ECX); // 3

   mov_reg64_imm64(RSI, (uint64_t) invalid_code);
   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg64preg64_imm8(RBX, RSI, 0);
   jne_rj(65);

   mov_reg64_imm64(RDI, (uint64_t) blocks); // 10
   mov_reg32_reg32(ECX, EBX); // 2
   mov_reg64_preg64x8preg64(RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(RBX, RBX, (int) offsetof(struct precomp_block, block)); // 7
   mov_reg64_imm64(RDI, (uint64_t) cached_interpreter_table.NOTCOMPILED); // 10
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg64_preg64preg64pimm32(RAX, RAX, RBX, (int) offsetof(struct precomp_instr, ops)); // 8
   cmp_reg64_reg64(RAX, RDI); // 3
   je_rj(4); // 2
   mov_preg64preg64_imm8(RCX, RSI, 1); // 4
#else
   free_all_registers();
   simplify_access();
   mov_reg32_m32(ECX, (unsigned int *)dst->f.i.rt);
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)writemem);
      cmp_reg32_imm32(EAX, (unsigned int)write_rdram);
   }
   je_rj(41);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_reg32((unsigned int *)(&cpu_word), ECX); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)writemem); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(&address)); // 5
   jmp_imm_short(14); // 2

   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(EBX, (unsigned int)g_dev.ri.rdram.dram, ECX); // 6

   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg32pimm32_imm8(EBX, (unsigned int)invalid_code, 0);
   jne_rj(54);
   mov_reg32_reg32(ECX, EBX); // 2
   shl_reg32_imm8(EBX, 2); // 3
   mov_reg32_preg32pimm32(EBX, EBX, (unsigned int)blocks); // 6
   mov_reg32_preg32pimm32(EBX, EBX, (int)&actual->block - (int)actual); // 6
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg32_preg32preg32pimm32(EAX, EAX, EBX, (int)&dst->ops - (int)dst); // 7
   cmp_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED); // 6
   je_rj(7); // 2
   mov_preg32pimm32_imm8(ECX, (unsigned int)invalid_code, 1); // 7
#endif
#endif
}

void gensdl(void)
{
   gencallinterp((native_type)cached_interpreter_table.SDL, 0);
}

void gensdr(void)
{
   gencallinterp((native_type)cached_interpreter_table.SDR, 0);
}

void genswr(void)
{
   gencallinterp((native_type)cached_interpreter_table.SWR, 0);
}

void genlwc1(void)
{
#ifdef INTERPRET_LWC1
   gencallinterp((native_type)cached_interpreter_table.LWC1, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) readmem);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) read_rdram);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(49);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_xreg64_m64rel(RDX, (uint64_t *)(&reg_cop1_simple[dst->f.lf.ft])); // 7
   mov_m64rel_xreg64((uint64_t *)(&rdword), RDX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   jmp_imm_short(28); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg32_preg64preg64(EAX, RBX, RSI); // 3
   mov_xreg64_m64rel(RBX, (uint64_t *)(&reg_cop1_simple[dst->f.lf.ft])); // 7
   mov_preg64_reg32(RBX, EAX); // 2
#else
   mov_eax_memoffs32((unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmem);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdram);
   }
   je_rj(42);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_reg32_m32(EDX, (unsigned int*)(&reg_cop1_simple[dst->f.lf.ft])); // 6
   mov_m32_reg32((unsigned int *)(&rdword), EDX); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmem); // 7
   call_reg32(EBX); // 2
   jmp_imm_short(20); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(EAX, EBX, (unsigned int)g_dev.ri.rdram.dram); // 6
   mov_reg32_m32(EBX, (unsigned int*)(&reg_cop1_simple[dst->f.lf.ft])); // 6
   mov_preg32_reg32(EBX, EAX); // 2
#endif
#endif
}

void genldc1(void)
{
#ifdef INTERPRET_LDC1
   gencallinterp((native_type)cached_interpreter_table.LDC1, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg32_m32rel(EAX, (unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) readmemd);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) read_rdramd);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(49);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_xreg64_m64rel(RDX, (uint64_t *)(&reg_cop1_double[dst->f.lf.ft])); // 7
   mov_m64rel_xreg64((uint64_t *)(&rdword), RDX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   jmp_imm_short(39); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg64_preg64preg64(RAX, RBX, RSI); // 4
   mov_xreg64_m64rel(RBX, (uint64_t *)(&reg_cop1_double[dst->f.lf.ft])); // 7
   mov_preg64pimm32_reg32(RBX, 4, EAX); // 6
   shr_reg64_imm8(RAX, 32); // 4
   mov_preg64_reg32(RBX, EAX); // 2
#else
   mov_eax_memoffs32((unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)readmemd);
      cmp_reg32_imm32(EAX, (unsigned int)read_rdramd);
   }
   je_rj(42);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_reg32_m32(EDX, (unsigned int*)(&reg_cop1_double[dst->f.lf.ft])); // 6
   mov_m32_reg32((unsigned int *)(&rdword), EDX); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)readmemd); // 7
   call_reg32(EBX); // 2
   jmp_imm_short(32); // 2

   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(EAX, EBX, ((unsigned int)g_dev.ri.rdram.dram)+4); // 6
   mov_reg32_preg32pimm32(ECX, EBX, ((unsigned int)g_dev.ri.rdram.dram)); // 6
   mov_reg32_m32(EBX, (unsigned int*)(&reg_cop1_double[dst->f.lf.ft])); // 6
   mov_preg32_reg32(EBX, EAX); // 2
   mov_preg32pimm32_reg32(EBX, 4, ECX); // 6
#endif
#endif
}

void genld(void)
{
#if defined(INTERPRET_LD) || !defined(__x86_64__)
   gencallinterp((native_type)cached_interpreter_table.LD, 0);
#else
   free_registers_move_start();

   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) readmemd);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) read_rdramd);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(59);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_reg64_imm64(RAX, (uint64_t) dst->f.i.rt); // 10
   mov_m64rel_xreg64((uint64_t *)(&rdword), RAX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg64_m64rel(RAX, (uint64_t *)(dst->f.i.rt)); // 7
   jmp_imm_short(33); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   and_reg32_imm32(EBX, 0x7FFFFF); // 6

   mov_reg32_preg64preg64(EAX, RBX, RSI); // 3
   mov_reg32_preg64preg64pimm32(EBX, RBX, RSI, 4); // 7
   shl_reg64_imm8(RAX, 32); // 4
   or_reg64_reg64(RAX, RBX); // 3

   set_register_state(RAX, (unsigned int*)dst->f.i.rt, 1, 1);
#endif
}

void genswc1(void)
{
#ifdef INTERPRET_SWC1
   gencallinterp((native_type)cached_interpreter_table.SWC1, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg64_m64rel(RDX, (uint64_t *)(&reg_cop1_simple[dst->f.lf.ft]));
   mov_reg32_preg64(ECX, RDX);
   mov_xreg32_m32rel(EAX, (unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) writemem);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) write_rdram);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(49);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_m32rel_xreg32((unsigned int *)(&cpu_word), ECX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg32_m32rel(EAX, (unsigned int *)(&address)); // 7
   jmp_imm_short(21); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg64preg64_reg32(RBX, RSI, ECX); // 3

   mov_reg64_imm64(RSI, (uint64_t) invalid_code);
   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg64preg64_imm8(RBX, RSI, 0);
   jne_rj(65);

   mov_reg64_imm64(RDI, (uint64_t) blocks); // 10
   mov_reg32_reg32(ECX, EBX); // 2
   mov_reg64_preg64x8preg64(RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(RBX, RBX, (int) offsetof(struct precomp_block, block)); // 7
   mov_reg64_imm64(RDI, (uint64_t) cached_interpreter_table.NOTCOMPILED); // 10
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg64_preg64preg64pimm32(RAX, RAX, RBX, (int) offsetof(struct precomp_instr, ops)); // 8
   cmp_reg64_reg64(RAX, RDI); // 3
   je_rj(4); // 2
   mov_preg64preg64_imm8(RCX, RSI, 1); // 4
#else
   mov_reg32_m32(EDX, (unsigned int*)(&reg_cop1_simple[dst->f.lf.ft]));
   mov_reg32_preg32(ECX, EDX);
   mov_eax_memoffs32((unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)writemem);
      cmp_reg32_imm32(EAX, (unsigned int)write_rdram);
   }
   je_rj(41);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_reg32((unsigned int *)(&cpu_word), ECX); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)writemem); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(&address)); // 5
   jmp_imm_short(14); // 2

   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(EBX, (unsigned int)g_dev.ri.rdram.dram, ECX); // 6

   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg32pimm32_imm8(EBX, (unsigned int)invalid_code, 0);
   jne_rj(54);
   mov_reg32_reg32(ECX, EBX); // 2
   shl_reg32_imm8(EBX, 2); // 3
   mov_reg32_preg32pimm32(EBX, EBX, (unsigned int)blocks); // 6
   mov_reg32_preg32pimm32(EBX, EBX, (int)&actual->block - (int)actual); // 6
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg32_preg32preg32pimm32(EAX, EAX, EBX, (int)&dst->ops - (int)dst); // 7
   cmp_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED); // 6
   je_rj(7); // 2
   mov_preg32pimm32_imm8(ECX, (unsigned int)invalid_code, 1); // 7
#endif
#endif
}

void gensdc1(void)
{
#ifdef INTERPRET_SDC1
   gencallinterp((native_type)cached_interpreter_table.SDC1, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg64_m64rel(RSI, (uint64_t *)(&reg_cop1_double[dst->f.lf.ft]));
   mov_reg32_preg64(ECX, RSI);
   mov_reg32_preg64pimm32(EDX, RSI, 4);
   mov_xreg32_m32rel(EAX, (unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) writememd);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) write_rdramd);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(56);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_m32rel_xreg32((unsigned int *)(&cpu_dword), ECX); // 7
   mov_m32rel_xreg32((unsigned int *)(&cpu_dword)+1, EDX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg32_m32rel(EAX, (unsigned int *)(&address)); // 7
   jmp_imm_short(28); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg64preg64pimm32_reg32(RBX, RSI, 4, ECX); // 7
   mov_preg64preg64_reg32(RBX, RSI, EDX); // 3

   mov_reg64_imm64(RSI, (uint64_t) invalid_code);
   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg64preg64_imm8(RBX, RSI, 0);
   jne_rj(65);

   mov_reg64_imm64(RDI, (uint64_t) blocks); // 10
   mov_reg32_reg32(ECX, EBX); // 2
   mov_reg64_preg64x8preg64(RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(RBX, RBX, (int) offsetof(struct precomp_block, block)); // 7
   mov_reg64_imm64(RDI, (uint64_t) cached_interpreter_table.NOTCOMPILED); // 10
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg64_preg64preg64pimm32(RAX, RAX, RBX, (int) offsetof(struct precomp_instr, ops)); // 8
   cmp_reg64_reg64(RAX, RDI); // 3
   je_rj(4); // 2
   mov_preg64preg64_imm8(RCX, RSI, 1); // 4
#else
   mov_reg32_m32(ESI, (unsigned int*)(&reg_cop1_double[dst->f.lf.ft]));
   mov_reg32_preg32(ECX, ESI);
   mov_reg32_preg32pimm32(EDX, ESI, 4);
   mov_eax_memoffs32((unsigned int *)(&reg[dst->f.lf.base]));
   add_eax_imm32((int)dst->f.lf.offset);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)writememd);
      cmp_reg32_imm32(EAX, (unsigned int)write_rdramd);
   }
   je_rj(47);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_reg32((unsigned int *)(&cpu_dword), ECX); // 6
   mov_m32_reg32((unsigned int *)(&cpu_dword)+1, EDX); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)writememd); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(&address)); // 5
   jmp_imm_short(20); // 2

   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(EBX, ((unsigned int)g_dev.ri.rdram.dram)+4, ECX); // 6
   mov_preg32pimm32_reg32(EBX, ((unsigned int)g_dev.ri.rdram.dram)+0, EDX); // 6

   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg32pimm32_imm8(EBX, (unsigned int)invalid_code, 0);
   jne_rj(54);
   mov_reg32_reg32(ECX, EBX); // 2
   shl_reg32_imm8(EBX, 2); // 3
   mov_reg32_preg32pimm32(EBX, EBX, (unsigned int)blocks); // 6
   mov_reg32_preg32pimm32(EBX, EBX, (int)&actual->block - (int)actual); // 6
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg32_preg32preg32pimm32(EAX, EAX, EBX, (int)&dst->ops - (int)dst); // 7
   cmp_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED); // 6
   je_rj(7); // 2
   mov_preg32pimm32_imm8(ECX, (unsigned int)invalid_code, 1); // 7
#endif
#endif
}

void gensd(void)
{
#ifdef INTERPRET_SD
   gencallinterp((native_type)cached_interpreter_table.SD, 0);
#else

#ifdef __x86_64__
   free_registers_move_start();

   mov_xreg32_m32rel(ECX, (unsigned int *)dst->f.i.rt);
   mov_xreg32_m32rel(EDX, ((unsigned int *)dst->f.i.rt)+1);
   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   mov_reg64_imm64(RSI, (uint64_t) writememd);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      mov_reg64_imm64(RDI, (uint64_t) write_rdramd);
      shr_reg32_imm8(EAX, 16);
      mov_reg64_preg64x8preg64(RAX, RAX, RSI);
      cmp_reg64_reg64(RAX, RDI);
   }
   je_rj(56);

   mov_reg64_imm64(RAX, (uint64_t) (dst+1)); // 10
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX); // 7
   mov_m32rel_xreg32((unsigned int *)(&address), EBX); // 7
   mov_m32rel_xreg32((unsigned int *)(&cpu_dword), ECX); // 7
   mov_m32rel_xreg32((unsigned int *)(&cpu_dword)+1, EDX); // 7
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg64_preg64x8preg64(RBX, RBX, RSI);  // 4
   call_reg64(RBX); // 2
   mov_xreg32_m32rel(EAX, (unsigned int *)(&address)); // 7
   jmp_imm_short(28); // 2

   mov_reg64_imm64(RSI, (uint64_t) g_dev.ri.rdram.dram); // 10
   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg64preg64pimm32_reg32(RBX, RSI, 4, ECX); // 7
   mov_preg64preg64_reg32(RBX, RSI, EDX); // 3

   mov_reg64_imm64(RSI, (uint64_t) invalid_code);
   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg64preg64_imm8(RBX, RSI, 0);
   jne_rj(65);

   mov_reg64_imm64(RDI, (uint64_t) blocks); // 10
   mov_reg32_reg32(ECX, EBX); // 2
   mov_reg64_preg64x8preg64(RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(RBX, RBX, (int) offsetof(struct precomp_block, block)); // 7
   mov_reg64_imm64(RDI, (uint64_t) cached_interpreter_table.NOTCOMPILED); // 10
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg64_preg64preg64pimm32(RAX, RAX, RBX, (int) offsetof(struct precomp_instr, ops)); // 8
   cmp_reg64_reg64(RAX, RDI); // 3
   je_rj(4); // 2
   mov_preg64preg64_imm8(RCX, RSI, 1); // 4
#else
   free_all_registers();
   simplify_access();

   mov_reg32_m32(ECX, (unsigned int *)dst->f.i.rt);
   mov_reg32_m32(EDX, ((unsigned int *)dst->f.i.rt)+1);
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   add_eax_imm32((int)dst->f.i.immediate);
   mov_reg32_reg32(EBX, EAX);
   if(g_dev.r4300.recomp.fast_memory)
   {
      and_eax_imm32(0xDF800000);
      cmp_eax_imm32(0x80000000);
   }
   else
   {
      shr_reg32_imm8(EAX, 16);
      mov_reg32_preg32x4pimm32(EAX, EAX, (unsigned int)writememd);
      cmp_reg32_imm32(EAX, (unsigned int)write_rdramd);
   }
   je_rj(47);

   mov_m32_imm32((unsigned int *)(&PC), (unsigned int)(dst+1)); // 10
   mov_m32_reg32((unsigned int *)(&address), EBX); // 6
   mov_m32_reg32((unsigned int *)(&cpu_dword), ECX); // 6
   mov_m32_reg32((unsigned int *)(&cpu_dword)+1, EDX); // 6
   shr_reg32_imm8(EBX, 16); // 3
   mov_reg32_preg32x4pimm32(EBX, EBX, (unsigned int)writememd); // 7
   call_reg32(EBX); // 2
   mov_eax_memoffs32((unsigned int *)(&address)); // 5
   jmp_imm_short(20); // 2

   mov_reg32_reg32(EAX, EBX); // 2
   and_reg32_imm32(EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(EBX, ((unsigned int)g_dev.ri.rdram.dram)+4, ECX); // 6
   mov_preg32pimm32_reg32(EBX, ((unsigned int)g_dev.ri.rdram.dram)+0, EDX); // 6

   mov_reg32_reg32(EBX, EAX);
   shr_reg32_imm8(EBX, 12);
   cmp_preg32pimm32_imm8(EBX, (unsigned int)invalid_code, 0);
   jne_rj(54);
   mov_reg32_reg32(ECX, EBX); // 2
   shl_reg32_imm8(EBX, 2); // 3
   mov_reg32_preg32pimm32(EBX, EBX, (unsigned int)blocks); // 6
   mov_reg32_preg32pimm32(EBX, EBX, (int)&actual->block - (int)actual); // 6
   and_eax_imm32(0xFFF); // 5
   shr_reg32_imm8(EAX, 2); // 3
   mov_reg32_imm32(EDX, sizeof(struct precomp_instr)); // 5
   mul_reg32(EDX); // 2
   mov_reg32_preg32preg32pimm32(EAX, EAX, EBX, (int)&dst->ops - (int)dst); // 7
   cmp_reg32_imm32(EAX, (unsigned int)cached_interpreter_table.NOTCOMPILED); // 6
   je_rj(7); // 2
   mov_preg32pimm32_imm8(ECX, (unsigned int)invalid_code, 1); // 7
#endif
#endif
}

void genll(void)
{
   gencallinterp((native_type)cached_interpreter_table.LL, 0);
}

void gensc(void)
{
   gencallinterp((native_type)cached_interpreter_table.SC, 0);
}

static const unsigned int precomp_instr_size = sizeof(struct precomp_instr);

/* Multiply/Divide instructions */

void gensll(void)
{
#ifdef INTERPRET_SLL
   gencallinterp((native_type)cached_interpreter_table.SLL, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);
#else
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);
#endif

   mov_reg32_reg32(rd, rt);
   shl_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensrl(void)
{
#ifdef INTERPRET_SRL
   gencallinterp((native_type)cached_interpreter_table.SRL, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);
#else
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);
#endif

   mov_reg32_reg32(rd, rt);
   shr_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensra(void)
{
#ifdef INTERPRET_SRA
   gencallinterp((native_type)cached_interpreter_table.SRA, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);
#else
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);
#endif

   mov_reg32_reg32(rd, rt);
   sar_reg32_imm8(rd, dst->f.r.sa);
#endif
}

void gensllv(void)
{
#ifdef INTERPRET_SLLV
   gencallinterp((native_type)cached_interpreter_table.SLLV, 0);
#else
   int rt, rd;
#ifdef __x86_64__
   allocate_register_32_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);
#else
   allocate_register_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register((unsigned int *)dst->f.r.rt);
   rd = allocate_register_w((unsigned int *)dst->f.r.rd);
#endif

   if (rd != ECX)
   {
      mov_reg32_reg32(rd, rt);
      shl_reg32_cl(rd);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rt);
      shl_reg32_cl(temp);
      mov_reg32_reg32(rd, temp);
   }
#endif
}

void gensrlv(void)
{
#ifdef INTERPRET_SRLV
   gencallinterp((native_type)cached_interpreter_table.SRLV, 0);
#else
   int rt, rd;
#ifdef __x86_64__
   allocate_register_32_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);
#else
   allocate_register_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register((unsigned int *)dst->f.r.rt);
   rd = allocate_register_w((unsigned int *)dst->f.r.rd);
#endif

   if (rd != ECX)
   {
      mov_reg32_reg32(rd, rt);
      shr_reg32_cl(rd);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rt);
      shr_reg32_cl(temp);
      mov_reg32_reg32(rd, temp);
   }
#endif
}

void gensrav(void)
{
#ifdef INTERPRET_SRAV
   gencallinterp((native_type)cached_interpreter_table.SRAV, 0);
#else
   int rt, rd;
#ifdef __x86_64__
   allocate_register_32_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);
#else
   allocate_register_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register((unsigned int *)dst->f.r.rt);
   rd = allocate_register_w((unsigned int *)dst->f.r.rd);
#endif

   if (rd != ECX)
   {
      mov_reg32_reg32(rd, rt);
      sar_reg32_cl(rd);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rt);
      sar_reg32_cl(temp);
      mov_reg32_reg32(rd, temp);
   }
#endif
}

void genjr(void)
{
#ifdef INTERPRET_JR
   gencallinterp((native_type)cached_interpreter_table.JR, 1);
#else
#ifdef __x86_64__
   unsigned int diff = (unsigned int) offsetof(struct precomp_instr, local_addr);
   unsigned int diff_need = (unsigned int) offsetof(struct precomp_instr, reg_cache_infos.need_map);
   unsigned int diff_wrap = (unsigned int) offsetof(struct precomp_instr, reg_cache_infos.jump_wrapper);

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.JR, 1);
      return;
   }

   free_registers_move_start();

   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.i.rs);
   mov_m32rel_xreg32((unsigned int *)&local_rs, EAX);

   gendelayslot();

   mov_xreg32_m32rel(EAX, (unsigned int *)&local_rs);
   mov_m32rel_xreg32((unsigned int *)&last_addr, EAX);

   gencheck_interrupt_reg();

   mov_xreg32_m32rel(EAX, (unsigned int *)&local_rs);
   mov_reg32_reg32(EBX, EAX);
   and_eax_imm32(0xFFFFF000);
   cmp_eax_imm32(dst_block->start & 0xFFFFF000);
   je_near_rj(0);

   jump_start_rel32();

   mov_m32rel_xreg32(&jump_to_address, EBX);
   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) jump_to_func);
   call_reg64(RAX);  /* will never return from call */

   jump_end_rel32();

   mov_reg64_imm64(RSI, (uint64_t) dst_block->block);
   mov_reg32_reg32(EAX, EBX);
   sub_eax_imm32(dst_block->start);
   shr_reg32_imm8(EAX, 2);
   mul_m32rel((unsigned int *)(&precomp_instr_size));

   mov_reg32_preg64preg64pimm32(EBX, RAX, RSI, diff_need);
   cmp_reg32_imm32(EBX, 1);
   jne_rj(11);

   add_reg32_imm32(EAX, diff_wrap); // 6
   add_reg64_reg64(RAX, RSI); // 3
   jmp_reg64(RAX); // 2

   mov_reg32_preg64preg64pimm32(EBX, RAX, RSI, diff);
   mov_rax_memoffs64((uint64_t *) &dst_block->code);
   add_reg64_reg64(RAX, RBX);
   jmp_reg64(RAX);
#else
   unsigned int diff =
      (unsigned int)(&dst->local_addr) - (unsigned int)(dst);
   unsigned int diff_need =
      (unsigned int)(&dst->reg_cache_infos.need_map) - (unsigned int)(dst);
   unsigned int diff_wrap =
      (unsigned int)(&dst->reg_cache_infos.jump_wrapper) - (unsigned int)(dst);

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((unsigned int)cached_interpreter_table.JR, 1);
      return;
   }

   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.i.rs);
   mov_memoffs32_eax((unsigned int *)&local_rs);

   gendelayslot();

   mov_eax_memoffs32((unsigned int *)&local_rs);
   mov_memoffs32_eax((unsigned int *)&last_addr);

   gencheck_interrupt_reg();

   mov_eax_memoffs32((unsigned int *)&local_rs);
   mov_reg32_reg32(EBX, EAX);
   and_eax_imm32(0xFFFFF000);
   cmp_eax_imm32(dst_block->start & 0xFFFFF000);
   je_near_rj(0);

   jump_start_rel32();

   mov_m32_reg32(&jump_to_address, EBX);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)jump_to_func);
   call_reg32(EAX);

   jump_end_rel32();

   mov_reg32_reg32(EAX, EBX);
   sub_eax_imm32(dst_block->start);
   shr_reg32_imm8(EAX, 2);
   mul_m32((unsigned int *)(&precomp_instr_size));

   mov_reg32_preg32pimm32(EBX, EAX, (unsigned int)(dst_block->block)+diff_need);
   cmp_reg32_imm32(EBX, 1);
   jne_rj(7);

   add_eax_imm32((unsigned int)(dst_block->block)+diff_wrap); // 5
   jmp_reg32(EAX); // 2

   mov_reg32_preg32pimm32(EAX, EAX, (unsigned int)(dst_block->block)+diff);
   add_reg32_m32(EAX, (unsigned int *)(&dst_block->code));

   jmp_reg32(EAX);
#endif
#endif
}

void genjalr(void)
{
#ifdef INTERPRET_JALR
   gencallinterp((native_type)cached_interpreter_table.JALR, 0);
#else
#ifdef __x86_64__
   unsigned int diff = (unsigned int) offsetof(struct precomp_instr, local_addr);
   unsigned int diff_need = (unsigned int) offsetof(struct precomp_instr, reg_cache_infos.need_map);
   unsigned int diff_wrap = (unsigned int) offsetof(struct precomp_instr, reg_cache_infos.jump_wrapper);

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.JALR, 1);
      return;
   }

   free_registers_move_start();

   mov_xreg32_m32rel(EAX, (unsigned int *)dst->f.r.rs);
   mov_m32rel_xreg32((unsigned int *)&local_rs, EAX);

   gendelayslot();

   mov_m32rel_imm32((unsigned int *)(dst-1)->f.r.rd, dst->addr+4);
   if ((dst->addr+4) & 0x80000000)
      mov_m32rel_imm32(((unsigned int *)(dst-1)->f.r.rd)+1, 0xFFFFFFFF);
   else
      mov_m32rel_imm32(((unsigned int *)(dst-1)->f.r.rd)+1, 0);

   mov_xreg32_m32rel(EAX, (unsigned int *)&local_rs);
   mov_m32rel_xreg32((unsigned int *)&last_addr, EAX);

   gencheck_interrupt_reg();

   mov_xreg32_m32rel(EAX, (unsigned int *)&local_rs);
   mov_reg32_reg32(EBX, EAX);
   and_eax_imm32(0xFFFFF000);
   cmp_eax_imm32(dst_block->start & 0xFFFFF000);
   je_near_rj(0);

   jump_start_rel32();

   mov_m32rel_xreg32(&jump_to_address, EBX);
   mov_reg64_imm64(RAX, (uint64_t) (dst+1));
   mov_m64rel_xreg64((uint64_t *)(&PC), RAX);
   mov_reg64_imm64(RAX, (uint64_t) jump_to_func);
   call_reg64(RAX);  /* will never return from call */

   jump_end_rel32();

   mov_reg64_imm64(RSI, (uint64_t) dst_block->block);
   mov_reg32_reg32(EAX, EBX);
   sub_eax_imm32(dst_block->start);
   shr_reg32_imm8(EAX, 2);
   mul_m32rel((unsigned int *)(&precomp_instr_size));

   mov_reg32_preg64preg64pimm32(EBX, RAX, RSI, diff_need);
   cmp_reg32_imm32(EBX, 1);
   jne_rj(11);

   add_reg32_imm32(EAX, diff_wrap); // 6
   add_reg64_reg64(RAX, RSI); // 3
   jmp_reg64(RAX); // 2

   mov_reg32_preg64preg64pimm32(EBX, RAX, RSI, diff);
   mov_rax_memoffs64((uint64_t *) &dst_block->code);
   add_reg64_reg64(RAX, RBX);
   jmp_reg64(RAX);
#else
   unsigned int diff =
      (unsigned int)(&dst->local_addr) - (unsigned int)(dst);
   unsigned int diff_need =
      (unsigned int)(&dst->reg_cache_infos.need_map) - (unsigned int)(dst);
   unsigned int diff_wrap =
      (unsigned int)(&dst->reg_cache_infos.jump_wrapper) - (unsigned int)(dst);

   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((unsigned int)cached_interpreter_table.JALR, 1);
      return;
   }

   free_all_registers();
   simplify_access();
   mov_eax_memoffs32((unsigned int *)dst->f.r.rs);
   mov_memoffs32_eax((unsigned int *)&local_rs);

   gendelayslot();

   mov_m32_imm32((unsigned int *)(dst-1)->f.r.rd, dst->addr+4);
   if ((dst->addr+4) & 0x80000000)
      mov_m32_imm32(((unsigned int *)(dst-1)->f.r.rd)+1, 0xFFFFFFFF);
   else
      mov_m32_imm32(((unsigned int *)(dst-1)->f.r.rd)+1, 0);

   mov_eax_memoffs32((unsigned int *)&local_rs);
   mov_memoffs32_eax((unsigned int *)&last_addr);

   gencheck_interrupt_reg();

   mov_eax_memoffs32((unsigned int *)&local_rs);
   mov_reg32_reg32(EBX, EAX);
   and_eax_imm32(0xFFFFF000);
   cmp_eax_imm32(dst_block->start & 0xFFFFF000);
   je_near_rj(0);

   jump_start_rel32();

   mov_m32_reg32(&jump_to_address, EBX);
   mov_m32_imm32((unsigned int*)(&PC), (unsigned int)(dst+1));
   mov_reg32_imm32(EAX, (unsigned int)jump_to_func);
   call_reg32(EAX);

   jump_end_rel32();

   mov_reg32_reg32(EAX, EBX);
   sub_eax_imm32(dst_block->start);
   shr_reg32_imm8(EAX, 2);
   mul_m32((unsigned int *)(&precomp_instr_size));

   mov_reg32_preg32pimm32(EBX, EAX, (unsigned int)(dst_block->block)+diff_need);
   cmp_reg32_imm32(EBX, 1);
   jne_rj(7);

   add_eax_imm32((unsigned int)(dst_block->block)+diff_wrap); // 5
   jmp_reg32(EAX); // 2

   mov_reg32_preg32pimm32(EAX, EAX, (unsigned int)(dst_block->block)+diff);
   add_reg32_m32(EAX, (unsigned int *)(&dst_block->code));

   jmp_reg32(EAX);
#endif
#endif
}

void gensyscall(void)
{
#ifdef INTERPRET_SYSCALL
   gencallinterp((native_type)cached_interpreter_table.SYSCALL, 0);
#else
#ifdef __x86_64__
   free_registers_move_start();

   mov_m32rel_imm32(&g_cp0_regs[CP0_CAUSE_REG], 8 << 2);
#else
   free_all_registers();
   simplify_access();
   mov_m32_imm32(&g_cp0_regs[CP0_CAUSE_REG], 8 << 2);
#endif
   gencallinterp((native_type)exception_general, 0);
#endif
}

void gensync(void)
{
}

void genmfhi(void)
{
#ifdef INTERPRET_MFHI
   gencallinterp((native_type)cached_interpreter_table.MFHI, 0);
#else
#ifdef __x86_64__
   int rd = allocate_register_64_w((uint64_t *) dst->f.r.rd);
   int _hi = allocate_register_64((uint64_t *) &hi);

   mov_reg64_reg64(rd, _hi);
#else
   int rd1 = allocate_64_register1_w((unsigned int*)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int*)dst->f.r.rd);
   int hi1 = allocate_64_register1((unsigned int*)&hi);
   int hi2 = allocate_64_register2((unsigned int*)&hi);

   mov_reg32_reg32(rd1, hi1);
   mov_reg32_reg32(rd2, hi2);
#endif
#endif
}

void genmthi(void)
{
#ifdef INTERPRET_MTHI
   gencallinterp((native_type)cached_interpreter_table.MTHI, 0);
#else
#ifdef __x86_64__
   int _hi = allocate_register_64_w((uint64_t *) &hi);
   int rs = allocate_register_64((uint64_t *) dst->f.r.rs);

   mov_reg64_reg64(_hi, rs);
#else
   int hi1 = allocate_64_register1_w((unsigned int*)&hi);
   int hi2 = allocate_64_register2_w((unsigned int*)&hi);
   int rs1 = allocate_64_register1((unsigned int*)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int*)dst->f.r.rs);

   mov_reg32_reg32(hi1, rs1);
   mov_reg32_reg32(hi2, rs2);
#endif
#endif
}

void genmflo(void)
{
#ifdef INTERPRET_MFLO
   gencallinterp((native_type)cached_interpreter_table.MFLO, 0);
#else
#ifdef __x86_64__
   int rd = allocate_register_64_w((uint64_t *) dst->f.r.rd);
   int _lo = allocate_register_64((uint64_t *) &lo);

   mov_reg64_reg64(rd, _lo);
#else
   int rd1 = allocate_64_register1_w((unsigned int*)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int*)dst->f.r.rd);
   int lo1 = allocate_64_register1((unsigned int*)&lo);
   int lo2 = allocate_64_register2((unsigned int*)&lo);

   mov_reg32_reg32(rd1, lo1);
   mov_reg32_reg32(rd2, lo2);
#endif
#endif
}

void genmtlo(void)
{
#ifdef INTERPRET_MTLO
   gencallinterp((native_type)cached_interpreter_table.MTLO, 0);
#else
#ifdef __x86_64__
   int _lo = allocate_register_64_w((uint64_t *)&lo);
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);

   mov_reg64_reg64(_lo, rs);
#else
   int lo1 = allocate_64_register1_w((unsigned int*)&lo);
   int lo2 = allocate_64_register2_w((unsigned int*)&lo);
   int rs1 = allocate_64_register1((unsigned int*)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int*)dst->f.r.rs);

   mov_reg32_reg32(lo1, rs1);
   mov_reg32_reg32(lo2, rs2);
#endif
#endif
}

void gendsllv(void)
{
#ifdef INTERPRET_DSLLV
   gencallinterp((native_type)cached_interpreter_table.DSLLV, 0);
#else
#ifdef __x86_64__
   int rt, rd;
   allocate_register_32_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rd != ECX)
   {
      mov_reg64_reg64(rd, rt);
      shl_reg64_cl(rd);
   }
   else
   {
      int temp;
      temp = lru_register();
      free_register(temp);

      mov_reg64_reg64(temp, rt);
      shl_reg64_cl(temp);
      mov_reg64_reg64(rd, temp);
   }
#else
   int rt1, rt2, rd1, rd2;
   allocate_register_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rd1 != ECX && rd2 != ECX)
   {
      mov_reg32_reg32(rd1, rt1);
      mov_reg32_reg32(rd2, rt2);
      shld_reg32_reg32_cl(rd2,rd1);
      shl_reg32_cl(rd1);
      test_reg32_imm32(ECX, 0x20);
      je_rj(4);
      mov_reg32_reg32(rd2, rd1); // 2
      xor_reg32_reg32(rd1, rd1); // 2
   }
   else
   {
      int temp1, temp2;
      temp1 = lru_register();
      temp2 = lru_register_exc1(temp1);
      free_register(temp1);
      free_register(temp2);

      mov_reg32_reg32(temp1, rt1);
      mov_reg32_reg32(temp2, rt2);
      shld_reg32_reg32_cl(temp2, temp1);
      shl_reg32_cl(temp1);
      test_reg32_imm32(ECX, 0x20);
      je_rj(4);
      mov_reg32_reg32(temp2, temp1); // 2
      xor_reg32_reg32(temp1, temp1); // 2

      mov_reg32_reg32(rd1, temp1);
      mov_reg32_reg32(rd2, temp2);
   }
#endif
#endif
}

void gendsrlv(void)
{
#ifdef INTERPRET_DSRLV
   gencallinterp((native_type)cached_interpreter_table.DSRLV, 0);
#else
#ifdef __x86_64__
   int rt, rd;
   allocate_register_32_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rd != ECX)
   {
      mov_reg64_reg64(rd, rt);
      shr_reg64_cl(rd);
   }
   else
   {
      int temp;
      temp = lru_register();
      free_register(temp);

      mov_reg64_reg64(temp, rt);
      shr_reg64_cl(temp);
      mov_reg64_reg64(rd, temp);
   }
#else
   int rt1, rt2, rd1, rd2;
   allocate_register_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rd1 != ECX && rd2 != ECX)
   {
      mov_reg32_reg32(rd1, rt1);
      mov_reg32_reg32(rd2, rt2);
      shrd_reg32_reg32_cl(rd1,rd2);
      shr_reg32_cl(rd2);
      test_reg32_imm32(ECX, 0x20);
      je_rj(4);
      mov_reg32_reg32(rd1, rd2); // 2
      xor_reg32_reg32(rd2, rd2); // 2
   }
   else
   {
      int temp1, temp2;
      temp1 = lru_register();
      temp2 = lru_register_exc1(temp1);
      free_register(temp1);
      free_register(temp2);

      mov_reg32_reg32(temp1, rt1);
      mov_reg32_reg32(temp2, rt2);
      shrd_reg32_reg32_cl(temp1, temp2);
      shr_reg32_cl(temp2);
      test_reg32_imm32(ECX, 0x20);
      je_rj(4);
      mov_reg32_reg32(temp1, temp2); // 2
      xor_reg32_reg32(temp2, temp2); // 2

      mov_reg32_reg32(rd1, temp1);
      mov_reg32_reg32(rd2, temp2);
   }
#endif
#endif
}

void gendsrav(void)
{
#ifdef INTERPRET_DSRAV
   gencallinterp((native_type)cached_interpreter_table.DSRAV, 0);
#else
#ifdef __x86_64__
   int rt, rd;
   allocate_register_32_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rd != ECX)
   {
      mov_reg64_reg64(rd, rt);
      sar_reg64_cl(rd);
   }
   else
   {
      int temp;
      temp = lru_register();
      free_register(temp);

      mov_reg64_reg64(temp, rt);
      sar_reg64_cl(temp);
      mov_reg64_reg64(rd, temp);
   }
#else
   int rt1, rt2, rd1, rd2;
   allocate_register_manually(ECX, (unsigned int *)dst->f.r.rs);

   rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rd1 != ECX && rd2 != ECX)
   {
      mov_reg32_reg32(rd1, rt1);
      mov_reg32_reg32(rd2, rt2);
      shrd_reg32_reg32_cl(rd1,rd2);
      sar_reg32_cl(rd2);
      test_reg32_imm32(ECX, 0x20);
      je_rj(5);
      mov_reg32_reg32(rd1, rd2); // 2
      sar_reg32_imm8(rd2, 31); // 3
   }
   else
   {
      int temp1, temp2;
      temp1 = lru_register();
      temp2 = lru_register_exc1(temp1);
      free_register(temp1);
      free_register(temp2);

      mov_reg32_reg32(temp1, rt1);
      mov_reg32_reg32(temp2, rt2);
      shrd_reg32_reg32_cl(temp1, temp2);
      sar_reg32_cl(temp2);
      test_reg32_imm32(ECX, 0x20);
      je_rj(5);
      mov_reg32_reg32(temp1, temp2); // 2
      sar_reg32_imm8(temp2, 31); // 3

      mov_reg32_reg32(rd1, temp1);
      mov_reg32_reg32(rd2, temp2);
   }
#endif
#endif
}

void genmult(void)
{
#ifdef INTERPRET_MULT
   gencallinterp((native_type)cached_interpreter_table.MULT, 0);
#else
   int rs, rt;
#ifdef __x86_64__
   allocate_register_32_manually_w(EAX, (unsigned int *)&lo); /* these must be done first so they are not assigned by allocate_register() */
   allocate_register_32_manually_w(EDX, (unsigned int *)&hi);
   rs = allocate_register_32((unsigned int*)dst->f.r.rs);
   rt = allocate_register_32((unsigned int*)dst->f.r.rt);
#else
   allocate_register_manually_w(EAX, (unsigned int *)&lo, 0);
   allocate_register_manually_w(EDX, (unsigned int *)&hi, 0);
   rs = allocate_register((unsigned int*)dst->f.r.rs);
   rt = allocate_register((unsigned int*)dst->f.r.rt);
#endif
   mov_reg32_reg32(EAX, rs);
   imul_reg32(rt);
#endif
}

void genmultu(void)
{
#ifdef INTERPRET_MULTU
   gencallinterp((native_type)cached_interpreter_table.MULTU, 0);
#else
   int rs, rt;
#ifdef __x86_64__
   allocate_register_32_manually_w(EAX, (unsigned int *)&lo);
   allocate_register_32_manually_w(EDX, (unsigned int *)&hi);
   rs = allocate_register_32((unsigned int*)dst->f.r.rs);
   rt = allocate_register_32((unsigned int*)dst->f.r.rt);
#else
   allocate_register_manually_w(EAX, (unsigned int *)&lo, 0);
   allocate_register_manually_w(EDX, (unsigned int *)&hi, 0);
   rs = allocate_register((unsigned int*)dst->f.r.rs);
   rt = allocate_register((unsigned int*)dst->f.r.rt);
#endif
   mov_reg32_reg32(EAX, rs);
   mul_reg32(rt);
#endif
}

void gendiv(void)
{
#ifdef INTERPRET_DIV
   gencallinterp((native_type)cached_interpreter_table.DIV, 0);
#else
   int rs, rt;
#ifdef __x86_64__
   allocate_register_32_manually_w(EAX, (unsigned int *)&lo);
   allocate_register_32_manually_w(EDX, (unsigned int *)&hi);
   rs = allocate_register_32((unsigned int*)dst->f.r.rs);
   rt = allocate_register_32((unsigned int*)dst->f.r.rt);
#else
   allocate_register_manually_w(EAX, (unsigned int *)&lo, 0);
   allocate_register_manually_w(EDX, (unsigned int *)&hi, 0);
   rs = allocate_register((unsigned int*)dst->f.r.rs);
   rt = allocate_register((unsigned int*)dst->f.r.rt);
#endif
   cmp_reg32_imm32(rt, 0);
   je_rj((rs == EAX ? 0 : 2) + 1 + 2);
   mov_reg32_reg32(EAX, rs); // 0 or 2
   cdq(); // 1
   idiv_reg32(rt); // 2
#endif
}

void gendivu(void)
{
#ifdef INTERPRET_DIVU
   gencallinterp((native_type)cached_interpreter_table.DIVU, 0);
#else
   int rs, rt;
#ifdef __x86_64__
   allocate_register_32_manually_w(EAX, (unsigned int *)&lo);
   allocate_register_32_manually_w(EDX, (unsigned int *)&hi);
   rs = allocate_register_32((unsigned int*)dst->f.r.rs);
   rt = allocate_register_32((unsigned int*)dst->f.r.rt);
#else
   allocate_register_manually_w(EAX, (unsigned int *)&lo, 0);
   allocate_register_manually_w(EDX, (unsigned int *)&hi, 0);
   rs = allocate_register((unsigned int*)dst->f.r.rs);
   rt = allocate_register((unsigned int*)dst->f.r.rt);
#endif
   cmp_reg32_imm32(rt, 0);
   je_rj((rs == EAX ? 0 : 2) + 2 + 2);
   mov_reg32_reg32(EAX, rs); // 0 or 2
   xor_reg32_reg32(EDX, EDX); // 2
   div_reg32(rt); // 2
#endif
}

void gendmult(void)
{
   gencallinterp((native_type)cached_interpreter_table.DMULT, 0);
}

void gendmultu(void)
{
#ifdef INTERPRET_DMULTU
   gencallinterp((native_type)cached_interpreter_table.DMULTU, 0);
#else
#ifdef __x86_64__
   free_registers_move_start();

   mov_xreg64_m64rel(RAX, (uint64_t *) dst->f.r.rs);
   mov_xreg64_m64rel(RDX, (uint64_t *) dst->f.r.rt);
   mul_reg64(RDX);
   mov_m64rel_xreg64((uint64_t *) &lo, RAX);
   mov_m64rel_xreg64((uint64_t *) &hi, RDX);
#else
   free_all_registers();
   simplify_access();

   mov_eax_memoffs32((unsigned int *)dst->f.r.rs);
   mul_m32((unsigned int *)dst->f.r.rt); // EDX:EAX = temp1
   mov_memoffs32_eax((unsigned int *)(&lo));

   mov_reg32_reg32(EBX, EDX); // EBX = temp1>>32
   mov_eax_memoffs32((unsigned int *)dst->f.r.rs);
   mul_m32((unsigned int *)(dst->f.r.rt)+1);
   add_reg32_reg32(EBX, EAX);
   adc_reg32_imm32(EDX, 0);
   mov_reg32_reg32(ECX, EDX); // ECX:EBX = temp2

   mov_eax_memoffs32((unsigned int *)(dst->f.r.rs)+1);
   mul_m32((unsigned int *)dst->f.r.rt); // EDX:EAX = temp3

   add_reg32_reg32(EBX, EAX);
   adc_reg32_imm32(ECX, 0); // ECX:EBX = result2
   mov_m32_reg32((unsigned int*)(&lo)+1, EBX);

   mov_reg32_reg32(ESI, EDX); // ESI = temp3>>32
   mov_eax_memoffs32((unsigned int *)(dst->f.r.rs)+1);
   mul_m32((unsigned int *)(dst->f.r.rt)+1);
   add_reg32_reg32(EAX, ESI);
   adc_reg32_imm32(EDX, 0); // EDX:EAX = temp4

   add_reg32_reg32(EAX, ECX);
   adc_reg32_imm32(EDX, 0); // EDX:EAX = result3
   mov_memoffs32_eax((unsigned int *)(&hi));
   mov_m32_reg32((unsigned int *)(&hi)+1, EDX);
#endif
#endif
}

void genddiv(void)
{
   gencallinterp((native_type)cached_interpreter_table.DDIV, 0);
}

void genddivu(void)
{
   gencallinterp((native_type)cached_interpreter_table.DDIVU, 0);
}

void genadd(void)
{
#ifdef INTERPRET_ADD
   gencallinterp((native_type)cached_interpreter_table.ADD, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_32((unsigned int *)dst->f.r.rs);
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);

   if (rs == rd)
      add_reg32_reg32(rd, rt);
   else if (rt == rd)
      add_reg32_reg32(rd, rs);
   else
   {
      mov_reg32_reg32(rd, rs);
      add_reg32_reg32(rd, rt);
   }
#else
   int rs = allocate_register((unsigned int *)dst->f.r.rs);
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   if (rt != rd && rs != rd)
   {
      mov_reg32_reg32(rd, rs);
      add_reg32_reg32(rd, rt);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs);
      add_reg32_reg32(temp, rt);
      mov_reg32_reg32(rd, temp);
   }
#endif
#endif
}

void genaddu(void)
{
#ifdef INTERPRET_ADDU
   gencallinterp((native_type)cached_interpreter_table.ADDU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_32((unsigned int *)dst->f.r.rs);
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);

   if (rs == rd)
      add_reg32_reg32(rd, rt);
   else if (rt == rd)
      add_reg32_reg32(rd, rs);
   else
   {
      mov_reg32_reg32(rd, rs);
      add_reg32_reg32(rd, rt);
   }
#else
   int rs = allocate_register((unsigned int *)dst->f.r.rs);
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   if (rt != rd && rs != rd)
   {
      mov_reg32_reg32(rd, rs);
      add_reg32_reg32(rd, rt);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs);
      add_reg32_reg32(temp, rt);
      mov_reg32_reg32(rd, temp);
   }
#endif
#endif
}

void gensub(void)
{
#ifdef INTERPRET_SUB
   gencallinterp((native_type)cached_interpreter_table.SUB, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_32((unsigned int *)dst->f.r.rs);
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);

   if (rs == rd)
      sub_reg32_reg32(rd, rt);
   else if (rt == rd)
   {
      neg_reg32(rd);
      add_reg32_reg32(rd, rs);
   }
   else
   {
      mov_reg32_reg32(rd, rs);
      sub_reg32_reg32(rd, rt);
   }
#else
   int rs = allocate_register((unsigned int *)dst->f.r.rs);
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   if (rt != rd && rs != rd)
   {
      mov_reg32_reg32(rd, rs);
      sub_reg32_reg32(rd, rt);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs);
      sub_reg32_reg32(temp, rt);
      mov_reg32_reg32(rd, temp);
   }
#endif
#endif
}

void gensubu(void)
{
#ifdef INTERPRET_SUBU
   gencallinterp((native_type)cached_interpreter_table.SUBU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_32((unsigned int *)dst->f.r.rs);
   int rt = allocate_register_32((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_32_w((unsigned int *)dst->f.r.rd);

   if (rs == rd)
      sub_reg32_reg32(rd, rt);
   else if (rt == rd)
   {
      neg_reg32(rd);
      add_reg32_reg32(rd, rs);
   }
   else
   {
      mov_reg32_reg32(rd, rs);
      sub_reg32_reg32(rd, rt);
   }
#else
   int rs = allocate_register((unsigned int *)dst->f.r.rs);
   int rt = allocate_register((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   if (rt != rd && rs != rd)
   {
      mov_reg32_reg32(rd, rs);
      sub_reg32_reg32(rd, rt);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs);
      sub_reg32_reg32(temp, rt);
      mov_reg32_reg32(rd, temp);
   }
#endif
#endif
}

void genand(void)
{
#ifdef INTERPRET_AND
   gencallinterp((native_type)cached_interpreter_table.AND, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      and_reg64_reg64(rd, rt);
   else if (rt == rd)
      and_reg64_reg64(rd, rs);
   else
   {
      mov_reg64_reg64(rd, rs);
      and_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      and_reg32_reg32(rd1, rt1);
      and_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      and_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      and_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void genor(void)
{
#ifdef INTERPRET_OR
   gencallinterp((native_type)cached_interpreter_table.OR, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      or_reg64_reg64(rd, rt);
   else if (rt == rd)
      or_reg64_reg64(rd, rs);
   else
   {
      mov_reg64_reg64(rd, rs);
      or_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      or_reg32_reg32(rd1, rt1);
      or_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      or_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      or_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void genxor(void)
{
#ifdef INTERPRET_XOR
   gencallinterp((native_type)cached_interpreter_table.XOR, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      xor_reg64_reg64(rd, rt);
   else if (rt == rd)
      xor_reg64_reg64(rd, rs);
   else
   {
      mov_reg64_reg64(rd, rs);
      xor_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      xor_reg32_reg32(rd1, rt1);
      xor_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      xor_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      xor_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void gennor(void)
{
#ifdef INTERPRET_NOR
   gencallinterp((native_type)cached_interpreter_table.NOR, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
   {
      or_reg64_reg64(rd, rt);
      not_reg64(rd);
   }
   else if (rt == rd)
   {
      or_reg64_reg64(rd, rs);
      not_reg64(rd);
   }
   else
   {
      mov_reg64_reg64(rd, rs);
      or_reg64_reg64(rd, rt);
      not_reg64(rd);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      or_reg32_reg32(rd1, rt1);
      or_reg32_reg32(rd2, rt2);
      not_reg32(rd1);
      not_reg32(rd2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      or_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      or_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
      not_reg32(rd1);
      not_reg32(rd2);
   }
#endif
#endif
}

void genslt(void)
{
#ifdef INTERPRET_SLT
   gencallinterp((native_type)cached_interpreter_table.SLT, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   cmp_reg64_reg64(rs, rt);
   setl_reg8(rd);
   and_reg64_imm8(rd, 1);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   cmp_reg32_reg32(rs2, rt2);
   jl_rj(13);
   jne_rj(4); // 2
   cmp_reg32_reg32(rs1, rt1); // 2
   jl_rj(7); // 2
   mov_reg32_imm32(rd, 0); // 5
   jmp_imm_short(5); // 2
   mov_reg32_imm32(rd, 1); // 5
#endif
#endif
}

void gensltu(void)
{
#ifdef INTERPRET_SLTU
   gencallinterp((native_type)cached_interpreter_table.SLTU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   cmp_reg64_reg64(rs, rt);
   setb_reg8(rd);
   and_reg64_imm8(rd, 1);
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   cmp_reg32_reg32(rs2, rt2);
   jb_rj(13);
   jne_rj(4); // 2
   cmp_reg32_reg32(rs1, rt1); // 2
   jb_rj(7); // 2
   mov_reg32_imm32(rd, 0); // 5
   jmp_imm_short(5); // 2
   mov_reg32_imm32(rd, 1); // 5
#endif
#endif
}

void gendadd(void)
{
#ifdef INTERPRET_DADD
   gencallinterp((native_type)cached_interpreter_table.DADD, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      add_reg64_reg64(rd, rt);
   else if (rt == rd)
      add_reg64_reg64(rd, rs);
   else
   {
      mov_reg64_reg64(rd, rs);
      add_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      add_reg32_reg32(rd1, rt1);
      adc_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      add_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      adc_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void gendaddu(void)
{
#ifdef INTERPRET_DADDU
   gencallinterp((native_type)cached_interpreter_table.DADDU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      add_reg64_reg64(rd, rt);
   else if (rt == rd)
      add_reg64_reg64(rd, rs);
   else
   {
      mov_reg64_reg64(rd, rs);
      add_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      add_reg32_reg32(rd1, rt1);
      adc_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      add_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      adc_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void gendsub(void)
{
#ifdef INTERPRET_DSUB
   gencallinterp((native_type)cached_interpreter_table.DSUB, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      sub_reg64_reg64(rd, rt);
   else if (rt == rd)
   {
      neg_reg64(rd);
      add_reg64_reg64(rd, rs);
   }
   else
   {
      mov_reg64_reg64(rd, rs);
      sub_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      sub_reg32_reg32(rd1, rt1);
      sbb_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      sub_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      sbb_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void gendsubu(void)
{
#ifdef INTERPRET_DSUBU
   gencallinterp((native_type)cached_interpreter_table.DSUBU, 0);
#else
#ifdef __x86_64__
   int rs = allocate_register_64((uint64_t *)dst->f.r.rs);
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   if (rs == rd)
      sub_reg64_reg64(rd, rt);
   else if (rt == rd)
   {
      neg_reg64(rd);
      add_reg64_reg64(rd, rs);
   }
   else
   {
      mov_reg64_reg64(rd, rs);
      sub_reg64_reg64(rd, rt);
   }
#else
   int rs1 = allocate_64_register1((unsigned int *)dst->f.r.rs);
   int rs2 = allocate_64_register2((unsigned int *)dst->f.r.rs);
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   if (rt1 != rd1 && rs1 != rd1)
   {
      mov_reg32_reg32(rd1, rs1);
      mov_reg32_reg32(rd2, rs2);
      sub_reg32_reg32(rd1, rt1);
      sbb_reg32_reg32(rd2, rt2);
   }
   else
   {
      int temp = lru_register();
      free_register(temp);
      mov_reg32_reg32(temp, rs1);
      sub_reg32_reg32(temp, rt1);
      mov_reg32_reg32(rd1, temp);
      mov_reg32_reg32(temp, rs2);
      sbb_reg32_reg32(temp, rt2);
      mov_reg32_reg32(rd2, temp);
   }
#endif
#endif
}

void genteq(void)
{
   gencallinterp((native_type)cached_interpreter_table.TEQ, 0);
}

void gendsll(void)
{
#ifdef INTERPRET_DSLL
   gencallinterp((native_type)cached_interpreter_table.DSLL, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   mov_reg64_reg64(rd, rt);
   shl_reg64_imm8(rd, dst->f.r.sa);
#else
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   mov_reg32_reg32(rd1, rt1);
   mov_reg32_reg32(rd2, rt2);
   shld_reg32_reg32_imm8(rd2, rd1, dst->f.r.sa);
   shl_reg32_imm8(rd1, dst->f.r.sa);
   if (dst->f.r.sa & 0x20)
   {
      mov_reg32_reg32(rd2, rd1);
      xor_reg32_reg32(rd1, rd1);
   }
#endif
#endif
}

void gendsrl(void)
{
#ifdef INTERPRET_DSRL
   gencallinterp((native_type)cached_interpreter_table.DSRL, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   mov_reg64_reg64(rd, rt);
   shr_reg64_imm8(rd, dst->f.r.sa);
#else
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   mov_reg32_reg32(rd1, rt1);
   mov_reg32_reg32(rd2, rt2);
   shrd_reg32_reg32_imm8(rd1, rd2, dst->f.r.sa);
   shr_reg32_imm8(rd2, dst->f.r.sa);
   if (dst->f.r.sa & 0x20)
   {
      mov_reg32_reg32(rd1, rd2);
      xor_reg32_reg32(rd2, rd2);
   }
#endif
#endif
}

void gendsra(void)
{
#ifdef INTERPRET_DSRA
   gencallinterp((native_type)cached_interpreter_table.DSRA, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   mov_reg64_reg64(rd, rt);
   sar_reg64_imm8(rd, dst->f.r.sa);
#else
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   mov_reg32_reg32(rd1, rt1);
   mov_reg32_reg32(rd2, rt2);
   shrd_reg32_reg32_imm8(rd1, rd2, dst->f.r.sa);
   sar_reg32_imm8(rd2, dst->f.r.sa);
   if (dst->f.r.sa & 0x20)
   {
      mov_reg32_reg32(rd1, rd2);
      sar_reg32_imm8(rd2, 31);
   }
#endif
#endif
}

void gendsll32(void)
{
#ifdef INTERPRET_DSLL32
   gencallinterp((native_type)cached_interpreter_table.DSLL32, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   mov_reg64_reg64(rd, rt);
   shl_reg64_imm8(rd, dst->f.r.sa + 32);
#else
   int rt1 = allocate_64_register1((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   mov_reg32_reg32(rd2, rt1);
   shl_reg32_imm8(rd2, dst->f.r.sa);
   xor_reg32_reg32(rd1, rd1);
#endif
#endif
}

void gendsrl32(void)
{
#ifdef INTERPRET_DSRL32
   gencallinterp((native_type)cached_interpreter_table.DSRL32, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   mov_reg64_reg64(rd, rt);
   shr_reg64_imm8(rd, dst->f.r.sa + 32);
#else
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd1 = allocate_64_register1_w((unsigned int *)dst->f.r.rd);
   int rd2 = allocate_64_register2_w((unsigned int *)dst->f.r.rd);

   mov_reg32_reg32(rd1, rt2);
   shr_reg32_imm8(rd1, dst->f.r.sa);
   xor_reg32_reg32(rd2, rd2);
#endif
#endif
}

void gendsra32(void)
{
#ifdef INTERPRET_DSRA32
   gencallinterp((native_type)cached_interpreter_table.DSRA32, 0);
#else
#ifdef __x86_64__
   int rt = allocate_register_64((uint64_t *)dst->f.r.rt);
   int rd = allocate_register_64_w((uint64_t *)dst->f.r.rd);

   mov_reg64_reg64(rd, rt);
   sar_reg64_imm8(rd, dst->f.r.sa + 32);
#else
   int rt2 = allocate_64_register2((unsigned int *)dst->f.r.rt);
   int rd = allocate_register_w((unsigned int *)dst->f.r.rd);

   mov_reg32_reg32(rd, rt2);
   sar_reg32_imm8(rd, dst->f.r.sa);
#endif
#endif
}

/* Global functions */

static void genbc1f_test(void)
{
#if defined(__x86_64__)
   test_m32rel_imm32((uint32_t*)&FCR31, 0x800000);
   sete_m8rel((uint8_t *) &branch_taken);
#else
   test_m32_imm32((uint32_t*)&FCR31, 0x800000);
   jne_rj(12);
   mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
   jmp_imm_short(10); // 2
   mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
#endif
}

static void genbc1t_test(void)
{
#if defined(__x86_64__)
   test_m32rel_imm32((uint32_t*)&FCR31, 0x800000);
   setne_m8rel((uint8_t *) &branch_taken);
#else
   test_m32_imm32((uint32_t*)&FCR31, 0x800000);
   je_rj(12);
   mov_m32_imm32((uint32_t*)(&branch_taken), 1); // 10
   jmp_imm_short(10); // 2
   mov_m32_imm32((uint32_t*)(&branch_taken), 0); // 10
#endif
}

void genbc1f(void)
{
#ifdef INTERPRET_BC1F
   gencallinterp((native_type)cached_interpreter_table.BC1F, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
       (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1F, 1);
      return;
   }
   
   gencheck_cop1_unusable();
   genbc1f_test();
   gendelayslot();
   gentest();
#endif
}

void genbc1f_out(void)
{
#ifdef INTERPRET_BC1F_OUT
   gencallinterp((native_type)cached_interpreter_table.BC1F_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1F_OUT, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1f_test();
   gendelayslot();
   gentest_out();
#endif
}

void genbc1f_idle(void)
{
#ifdef INTERPRET_BC1F_IDLE
   gencallinterp((native_type)cached_interpreter_table.BC1F_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1F_IDLE, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1f_test();
   gentest_idle();
   genbc1f();
#endif
}

void genbc1t(void)
{
#ifdef INTERPRET_BC1T
   gencallinterp((native_type)cached_interpreter_table.BC1T, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1T, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1t_test();
   gendelayslot();
   gentest();
#endif
}

void genbc1t_out(void)
{
#ifdef INTERPRET_BC1T_OUT
   gencallinterp((native_type)cached_interpreter_table.BC1T_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1T_OUT, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1t_test();
   gendelayslot();
   gentest_out();
#endif
}

void genbc1t_idle(void)
{
#ifdef INTERPRET_BC1T_IDLE
   gencallinterp((native_type)cached_interpreter_table.BC1T_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1T_IDLE, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1t_test();
   gentest_idle();
   genbc1t();
#endif
}

void genbc1fl(void)
{
#ifdef INTERPRET_BC1FL
   gencallinterp((native_type)cached_interpreter_table.BC1FL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1FL, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1f_test();
   free_all_registers();
   gentestl();
#endif
}

void genbc1fl_out(void)
{
#ifdef INTERPRET_BC1FL_OUT
   gencallinterp((native_type)cached_interpreter_table.BC1FL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1FL_OUT, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1f_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbc1fl_idle(void)
{
#ifdef INTERPRET_BC1FL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BC1FL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1FL_IDLE, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1f_test();
   gentest_idle();
   genbc1fl();
#endif
}

void genbc1tl(void)
{
#ifdef INTERPRET_BC1TL
   gencallinterp((native_type)cached_interpreter_table.BC1TL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1TL, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1t_test();
   free_all_registers();
   gentestl();
#endif
}

void genbc1tl_out(void)
{
#ifdef INTERPRET_BC1TL_OUT
   gencallinterp((native_type)cached_interpreter_table.BC1TL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1TL_OUT, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1t_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbc1tl_idle(void)
{
#ifdef INTERPRET_BC1TL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BC1TL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC &&
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BC1TL_IDLE, 1);
      return;
   }

   gencheck_cop1_unusable();
   genbc1t_test();
   gentest_idle();
   genbc1tl();
#endif
}

static void genbltz_test(void)
{
   int rs_64bit = is64((unsigned int *)dst->f.i.rs);

   if (rs_64bit == 0)
   {
#ifdef __x86_64__
      int rs = allocate_register_32((unsigned int *)dst->f.i.rs);
#else
      int rs = allocate_register((unsigned int *)dst->f.i.rs);
#endif

      cmp_reg32_imm32(rs, 0);
#ifdef __x86_64__
      setl_m8rel((unsigned char *) &branch_taken);
#else
      jge_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else if (rs_64bit == -1)
   {
#ifdef __x86_64__
      cmp_m32rel_imm32(((unsigned int *)dst->f.i.rs)+1, 0);
      setl_m8rel((unsigned char *) &branch_taken);
#else
      cmp_m32_imm32(((unsigned int *)dst->f.i.rs)+1, 0);
      jge_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else
   {
#ifdef __x86_64__
      int rs = allocate_register_64((uint64_t*)dst->f.i.rs);

      cmp_reg64_imm8(rs, 0);
      setl_m8rel((unsigned char *) &branch_taken);
#else
      int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs2, 0);
      jge_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
}

void genbltz(void)
{
#ifdef INTERPRET_BLTZ
   gencallinterp((native_type)cached_interpreter_table.BLTZ, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZ, 1);
      return;
   }

   genbltz_test();
   gendelayslot();
   gentest();
#endif
}

void genbltz_out(void)
{
#ifdef INTERPRET_BLTZ_OUT
   gencallinterp((native_type)cached_interpreter_table.BLTZ_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZ_OUT, 1);
      return;
   }

   genbltz_test();
   gendelayslot();
   gentest_out();
#endif
}

void genbltz_idle(void)
{
#ifdef INTERPRET_BLTZ_IDLE
   gencallinterp((native_type)cached_interpreter_table.BLTZ_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZ_IDLE, 1);
      return;
   }

   genbltz_test();
   gentest_idle();
   genbltz();
#endif
}

static void genbgez_test(void)
{
   int rs_64bit = is64((unsigned int *)dst->f.i.rs);

   if (rs_64bit == 0)
   {
#ifdef __x86_64__
      int rs = allocate_register_32((unsigned int *)dst->f.i.rs);
      cmp_reg32_imm32(rs, 0);
      setge_m8rel((unsigned char *) &branch_taken);
#else
      int rs = allocate_register((unsigned int*)dst->f.i.rs);
      cmp_reg32_imm32(rs, 0);
      jl_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else if (rs_64bit == -1)
   {
#ifdef __x86_64__
      cmp_m32rel_imm32(((unsigned int *)dst->f.i.rs)+1, 0);
      setge_m8rel((unsigned char *) &branch_taken);
#else
      cmp_m32_imm32(((unsigned int *)dst->f.i.rs)+1, 0);
      jl_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
   else
   {
#ifdef __x86_64__
      int rs = allocate_register_64((uint64_t*)dst->f.i.rs);
      cmp_reg64_imm8(rs, 0);
      setge_m8rel((unsigned char *) &branch_taken);
#else
      int rs2 = allocate_64_register2((unsigned int *)dst->f.i.rs);

      cmp_reg32_imm32(rs2, 0);
      jl_rj(12);
      mov_m32_imm32((unsigned int *)(&branch_taken), 1); // 10
      jmp_imm_short(10); // 2
      mov_m32_imm32((unsigned int *)(&branch_taken), 0); // 10
#endif
   }
}

void genbgez(void)
{
#ifdef INTERPRET_BGEZ
   gencallinterp((native_type)cached_interpreter_table.BGEZ, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZ, 1);
      return;
   }

   genbgez_test();
   gendelayslot();
   gentest();
#endif
}

void genbgez_out(void)
{
#ifdef INTERPRET_BGEZ_OUT
   gencallinterp((native_type)cached_interpreter_table.BGEZ_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZ_OUT, 1);
      return;
   }

   genbgez_test();
   gendelayslot();
   gentest_out();
#endif
}

void genbgez_idle(void)
{
#ifdef INTERPRET_BGEZ_IDLE
   gencallinterp((native_type)cached_interpreter_table.BGEZ_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZ_IDLE, 1);
      return;
   }

   genbgez_test();
   gentest_idle();
   genbgez();
#endif
}

void genbltzl(void)
{
#ifdef INTERPRET_BLTZL
   gencallinterp((native_type)cached_interpreter_table.BLTZL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZL, 1);
      return;
   }

   genbltz_test();
   free_all_registers();
   gentestl();
#endif
}

void genbltzl_out(void)
{
#ifdef INTERPRET_BLTZL_OUT
   gencallinterp((native_type)cached_interpreter_table.BLTZL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZL_OUT, 1);
      return;
   }

   genbltz_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbltzl_idle(void)
{
#ifdef INTERPRET_BLTZL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BLTZL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZL_IDLE, 1);
      return;
   }

   genbltz_test();
   gentest_idle();
   genbltzl();
#endif
}

void genbgezl(void)
{
#ifdef INTERPRET_BGEZL
   gencallinterp((native_type)cached_interpreter_table.BGEZL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZL, 1);
      return;
   }

   genbgez_test();
   free_all_registers();
   gentestl();
#endif
}

void genbgezl_out(void)
{
#ifdef INTERPRET_BGEZL_OUT
   gencallinterp((native_type)cached_interpreter_table.BGEZL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZL_OUT, 1);
      return;
   }

   genbgez_test();
   free_all_registers();
   gentestl_out();
#endif
}

void genbgezl_idle(void)
{
#ifdef INTERPRET_BGEZL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BGEZL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZL_IDLE, 1);
      return;
   }

   genbgez_test();
   gentest_idle();
   genbgezl();
#endif
}

static void genbranchlink(void)
{
   int r31_64bit = is64((unsigned int*)&reg[31]);

   if (r31_64bit == 0)
   {
#ifdef __x86_64__
      int r31 = allocate_register_32_w((unsigned int *)&reg[31]);

      mov_reg32_imm32(r31, dst->addr+8);
#else
      int r31 = allocate_register_w((unsigned int *)&reg[31]);

      mov_reg32_imm32(r31, dst->addr+8);
#endif
   }
   else if (r31_64bit == -1)
   {
#ifdef __x86_64__
      mov_m32rel_imm32((unsigned int *)&reg[31], dst->addr + 8);
      if (dst->addr & 0x80000000)
         mov_m32rel_imm32(((unsigned int *)&reg[31])+1, 0xFFFFFFFF);
      else
         mov_m32rel_imm32(((unsigned int *)&reg[31])+1, 0);
#else
      mov_m32_imm32((unsigned int *)&reg[31], dst->addr + 8);
      if (dst->addr & 0x80000000)
         mov_m32_imm32(((unsigned int *)&reg[31])+1, 0xFFFFFFFF);
      else
         mov_m32_imm32(((unsigned int *)&reg[31])+1, 0);
#endif
   }
   else
   {
#ifdef __x86_64__
      int r31 = allocate_register_64_w((uint64_t*)&reg[31]);

      mov_reg32_imm32(r31, dst->addr+8);
      movsxd_reg64_reg32(r31, r31);
#else
      int r311 = allocate_64_register1_w((unsigned int *)&reg[31]);
      int r312 = allocate_64_register2_w((unsigned int *)&reg[31]);

      mov_reg32_imm32(r311, dst->addr+8);
      if (dst->addr & 0x80000000)
         mov_reg32_imm32(r312, 0xFFFFFFFF);
      else
         mov_reg32_imm32(r312, 0);
#endif
   }
}

void genbltzal(void)
{
#ifdef INTERPRET_BLTZAL
   gencallinterp((native_type)cached_interpreter_table.BLTZAL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZAL, 1);
      return;
   }

   genbltz_test();
   genbranchlink();
   gendelayslot();
   gentest();
#endif
}

void genbltzal_out(void)
{
#ifdef INTERPRET_BLTZAL_OUT
   gencallinterp((native_type)cached_interpreter_table.BLTZAL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZAL_OUT, 1);
      return;
   }

   genbltz_test();
   genbranchlink();
   gendelayslot();
   gentest_out();
#endif
}

void genbltzal_idle(void)
{
#ifdef INTERPRET_BLTZAL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BLTZAL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZAL_IDLE, 1);
      return;
   }

   genbltz_test();
   genbranchlink();
   gentest_idle();
   genbltzal();
#endif
}

void genbgezal(void)
{
#ifdef INTERPRET_BGEZAL
   gencallinterp((native_type)cached_interpreter_table.BGEZAL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZAL, 1);
      return;
   }

   genbgez_test();
   genbranchlink();
   gendelayslot();
   gentest();
#endif
}

void genbgezal_out(void)
{
#ifdef INTERPRET_BGEZAL_OUT
   gencallinterp((native_type)cached_interpreter_table.BGEZAL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZAL_OUT, 1);
      return;
   }

   genbgez_test();
   genbranchlink();
   gendelayslot();
   gentest_out();
#endif
}

void genbgezal_idle(void)
{
#ifdef INTERPRET_BGEZAL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BGEZAL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZAL_IDLE, 1);
      return;
   }

   genbgez_test();
   genbranchlink();
   gentest_idle();
   genbgezal();
#endif
}

void genbltzall(void)
{
#ifdef INTERPRET_BLTZALL
   gencallinterp((native_type)cached_interpreter_table.BLTZALL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZALL, 1);
      return;
   }

   genbltz_test();
   genbranchlink();
   free_all_registers();
   gentestl();
#endif
}

void genbltzall_out(void)
{
#ifdef INTERPRET_BLTZALL_OUT
   gencallinterp((native_type)cached_interpreter_table.BLTZALL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZALL_OUT, 1);
      return;
   }

   genbltz_test();
   genbranchlink();
   free_all_registers();
   gentestl_out();
#endif
}

void genbltzall_idle(void)
{
#ifdef INTERPRET_BLTZALL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BLTZALL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BLTZALL_IDLE, 1);
      return;
   }

   genbltz_test();
   genbranchlink();
   gentest_idle();
   genbltzall();
#endif
}

void genbgezall(void)
{
#ifdef INTERPRET_BGEZALL
   gencallinterp((native_type)cached_interpreter_table.BGEZALL, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZALL, 1);
      return;
   }

   genbgez_test();
   genbranchlink();
   free_all_registers();
   gentestl();
#endif
}

void genbgezall_out(void)
{
#ifdef INTERPRET_BGEZALL_OUT
   gencallinterp((native_type)cached_interpreter_table.BGEZALL_OUT, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZALL_OUT, 1);
      return;
   }

   genbgez_test();
   genbranchlink();
   free_all_registers();
   gentestl_out();
#endif
}

void genbgezall_idle(void)
{
#ifdef INTERPRET_BGEZALL_IDLE
   gencallinterp((native_type)cached_interpreter_table.BGEZALL_IDLE, 1);
#else
   if (((dst->addr & 0xFFF) == 0xFFC && 
            (dst->addr < 0x80000000 || dst->addr >= 0xC0000000))||no_compiled_jump)
   {
      gencallinterp((native_type)cached_interpreter_table.BGEZALL_IDLE, 1);
      return;
   }

   genbgez_test();
   genbranchlink();
   gentest_idle();
   genbgezall();
#endif
}


/* TLB instructions */

void gentlbp(void)
{
   gencallinterp((native_type)cached_interpreter_table.TLBP, 0);
}

void gentlbr(void)
{
   gencallinterp((native_type)cached_interpreter_table.TLBR, 0);
}

void gentlbwr(void)
{
   gencallinterp((native_type)cached_interpreter_table.TLBWR, 0);
}

void gentlbwi(void)
{
   gencallinterp((native_type)cached_interpreter_table.TLBWI, 0);
}

/* special instructions */

void generet(void)
{
   gencallinterp((native_type)cached_interpreter_table.ERET, 1);
}

/* COP1 D instructions */

void genadd_d(void)
{
#ifdef INTERPRET_ADD_D
   gencallinterp((native_type)cached_interpreter_table.ADD_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fadd_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.ft]));
   fadd_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void gensub_d(void)
{
#ifdef INTERPRET_SUB_D
   gencallinterp((native_type)cached_interpreter_table.SUB_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fsub_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.ft]));
   fsub_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void genmul_d(void)
{
#ifdef INTERPRET_MUL_D
   gencallinterp((native_type)cached_interpreter_table.MUL_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fmul_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.ft]));
   fmul_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void gendiv_d(void)
{
#ifdef INTERPRET_DIV_D
   gencallinterp((native_type)cached_interpreter_table.DIV_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fdiv_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.ft]));
   fdiv_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void gensqrt_d(void)
{
#ifdef INTERPRET_SQRT_D
   gencallinterp((native_type)cached_interpreter_table.SQRT_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fsqrt();
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fsqrt();
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void genabs_d(void)
{
#ifdef INTERPRET_ABS_D
   gencallinterp((native_type)cached_interpreter_table.ABS_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fabs_();
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fabs_();
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void genmov_d(void)
{
#ifdef INTERPRET_MOV_D
   gencallinterp((native_type)cached_interpreter_table.MOV_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   mov_reg32_preg64(EBX, RAX);
   mov_reg32_preg64pimm32(ECX, RAX, 4);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   mov_preg64_reg32(RAX, EBX);
   mov_preg64pimm32_reg32(RAX, 4, ECX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   mov_reg32_preg32(EBX, EAX);
   mov_reg32_preg32pimm32(ECX, EAX, 4);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   mov_preg32_reg32(EAX, EBX);
   mov_preg32pimm32_reg32(EAX, 4, ECX);
#endif
#endif
}

void genneg_d(void)
{
#ifdef INTERPRET_NEG_D
   gencallinterp((native_type)cached_interpreter_table.NEG_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fchs();
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fchs();
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void genround_l_d(void)
{
#ifdef INTERPRET_ROUND_L_D
   gencallinterp((native_type)cached_interpreter_table.ROUND_L_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&round_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&round_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void gentrunc_l_d(void)
{
#ifdef INTERPRET_TRUNC_L_D
   gencallinterp((native_type)cached_interpreter_table.TRUNC_L_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&trunc_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&trunc_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genceil_l_d(void)
{
#ifdef INTERPRET_CEIL_L_D
   gencallinterp((native_type)cached_interpreter_table.CEIL_L_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&ceil_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&ceil_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genfloor_l_d(void)
{
#ifdef INTERPRET_FLOOR_L_D
   gencallinterp((native_type)cached_interpreter_table.FLOOR_L_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&floor_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&floor_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genround_w_d(void)
{
#ifdef INTERPRET_ROUND_W_D
   gencallinterp((native_type)cached_interpreter_table.ROUND_W_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&round_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&round_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void gentrunc_w_d(void)
{
#ifdef INTERPRET_TRUNC_W_D
   gencallinterp((native_type)cached_interpreter_table.TRUNC_W_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&trunc_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&trunc_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genceil_w_d(void)
{
#ifdef INTERPRET_CEIL_W_D
   gencallinterp((native_type)cached_interpreter_table.CEIL_W_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&ceil_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&ceil_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genfloor_w_d(void)
{
#ifdef INTERPRET_FLOOR_W_D
   gencallinterp((native_type)cached_interpreter_table.FLOOR_W_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&floor_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&floor_mode);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void gencvt_s_d(void)
{
#ifdef INTERPRET_CVT_S_D
   gencallinterp((native_type)cached_interpreter_table.CVT_S_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void gencvt_w_d(void)
{
#ifdef INTERPRET_CVT_W_D
   gencallinterp((native_type)cached_interpreter_table.CVT_W_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
#endif
#endif
}

void gencvt_l_d(void)
{
#ifdef INTERPRET_CVT_L_D
   gencallinterp((native_type)cached_interpreter_table.CVT_L_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
#endif
#endif
}

void genc_f_d(void)
{
#ifdef INTERPRET_C_F_D
   gencallinterp((native_type)cached_interpreter_table.C_F_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000);
#else
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000);
#endif
#endif
}

void genc_un_d(void)
{
#ifdef INTERPRET_C_UN_D
   gencallinterp((native_type)cached_interpreter_table.C_UN_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(13);
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
   jmp_imm_short(11); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(12);
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
   jmp_imm_short(10); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
#endif
#endif
}

void genc_eq_d(void)
{
#ifdef INTERPRET_C_EQ_D
   gencallinterp((native_type)cached_interpreter_table.C_EQ_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ueq_d(void)
{
#ifdef INTERPRET_C_UEQ_D
   gencallinterp((native_type)cached_interpreter_table.C_UEQ_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jne_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jne_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_olt_d(void)
{
#ifdef INTERPRET_C_OLT_D
   gencallinterp((native_type)cached_interpreter_table.C_OLT_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ult_d(void)
{
#ifdef INTERPRET_C_ULT_D
   gencallinterp((native_type)cached_interpreter_table.C_ULT_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jae_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jae_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ole_d(void)
{
#ifdef INTERPRET_C_OLE_D
   gencallinterp((native_type)cached_interpreter_table.C_OLE_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ule_d(void)
{
#ifdef INTERPRET_C_ULE_D
   gencallinterp((native_type)cached_interpreter_table.C_ULE_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   ja_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   ja_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_sf_d(void)
{
#ifdef INTERPRET_C_SF_D
   gencallinterp((native_type)cached_interpreter_table.C_SF_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000);
#endif
#endif
}

void genc_ngle_d(void)
{
#ifdef INTERPRET_C_NGLE_D
   gencallinterp((native_type)cached_interpreter_table.C_NGLE_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(13);
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
   jmp_imm_short(11); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(12);
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
   jmp_imm_short(10); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
#endif
#endif
}

void genc_seq_d(void)
{
#ifdef INTERPRET_C_SEQ_D
   gencallinterp((native_type)cached_interpreter_table.C_SEQ_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ngl_d(void)
{
#ifdef INTERPRET_C_NGL_D
   gencallinterp((native_type)cached_interpreter_table.C_NGL_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jne_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jne_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_lt_d(void)
{
#ifdef INTERPRET_C_LT_D
   gencallinterp((native_type)cached_interpreter_table.C_LT_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_nge_d(void)
{
#ifdef INTERPRET_C_NGE_D
   gencallinterp((native_type)cached_interpreter_table.C_NGE_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jae_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jae_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_le_d(void)
{
#ifdef INTERPRET_C_LE_D
   gencallinterp((native_type)cached_interpreter_table.C_LE_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ngt_d(void)
{
#ifdef INTERPRET_C_NGT_D
   gencallinterp((native_type)cached_interpreter_table.C_NGT_D, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg64_qword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   ja_rj(13); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.ft]));
   fld_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fld_preg32_qword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   ja_rj(12); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

/* COP1 L instructions */

void gencvt_s_l(void)
{
#ifdef INTERPRET_CVT_S_L
   gencallinterp((native_type)cached_interpreter_table.CVT_S_L, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fild_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fild_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void gencvt_d_l(void)
{
#ifdef INTERPRET_CVT_D_L
   gencallinterp((native_type)cached_interpreter_table.CVT_D_L, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fs]));
   fild_preg64_qword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fs]));
   fild_preg32_qword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

/* COP1 S instructions */

void genadd_s(void)
{
#ifdef INTERPRET_ADD_S
   gencallinterp((native_type)cached_interpreter_table.ADD_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fadd_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fadd_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void gensub_s(void)
{
#ifdef INTERPRET_SUB_S
   gencallinterp((native_type)cached_interpreter_table.SUB_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fsub_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fsub_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void genmul_s(void)
{
#ifdef INTERPRET_MUL_S
   gencallinterp((native_type)cached_interpreter_table.MUL_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fmul_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fmul_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void gendiv_s(void)
{
#ifdef INTERPRET_DIV_S
   gencallinterp((native_type)cached_interpreter_table.DIV_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fdiv_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fdiv_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void gensqrt_s(void)
{
#ifdef INTERPRET_SQRT_S
   gencallinterp((native_type)cached_interpreter_table.SQRT_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fsqrt();
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fsqrt();
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void genabs_s(void)
{
#ifdef INTERPRET_ABS_S
   gencallinterp((native_type)cached_interpreter_table.ABS_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fabs_();
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fabs_();
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void genmov_s(void)
{
#ifdef INTERPRET_MOV_S
   gencallinterp((native_type)cached_interpreter_table.MOV_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   mov_reg32_preg64(EBX, RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   mov_preg64_reg32(RAX, EBX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   mov_reg32_preg32(EBX, EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   mov_preg32_reg32(EAX, EBX);
#endif
#endif
}

void genneg_s(void)
{
#ifdef INTERPRET_NEG_S
   gencallinterp((native_type)cached_interpreter_table.NEG_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fchs();
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fchs();
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void genround_l_s(void)
{
#ifdef INTERPRET_ROUND_L_S
   gencallinterp((native_type)cached_interpreter_table.ROUND_L_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&round_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&round_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void gentrunc_l_s(void)
{
#ifdef INTERPRET_TRUNC_L_S
   gencallinterp((native_type)cached_interpreter_table.TRUNC_L_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&trunc_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&trunc_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genceil_l_s(void)
{
#ifdef INTERPRET_CEIL_L_S
   gencallinterp((native_type)cached_interpreter_table.CEIL_L_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&ceil_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&ceil_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genfloor_l_s(void)
{
#ifdef INTERPRET_FLOOR_L_S
   gencallinterp((native_type)cached_interpreter_table.FLOOR_L_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&floor_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&floor_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genround_w_s(void)
{
#ifdef INTERPRET_ROUND_W_S
   gencallinterp((native_type)cached_interpreter_table.ROUND_W_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&round_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&round_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void gentrunc_w_s(void)
{
#ifdef INTERPRET_TRUNC_W_S
   gencallinterp((native_type)cached_interpreter_table.TRUNC_W_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&trunc_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&trunc_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genceil_w_s(void)
{
#ifdef INTERPRET_CEIL_W_S
   gencallinterp((native_type)cached_interpreter_table.CEIL_W_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&ceil_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&ceil_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void genfloor_w_s(void)
{
#ifdef INTERPRET_FLOOR_W_S
   gencallinterp((native_type)cached_interpreter_table.FLOOR_W_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   fldcw_m16rel((uint16_t*)&floor_mode);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
   fldcw_m16rel((uint16_t*)&rounding_mode);
#else
   fldcw_m16((uint16_t*)&floor_mode);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
   fldcw_m16((uint16_t*)&rounding_mode);
#endif
#endif
}

void gencvt_d_s(void)
{
#ifdef INTERPRET_CVT_D_S
   gencallinterp((native_type)cached_interpreter_table.CVT_D_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

void gencvt_w_s(void)
{
#ifdef INTERPRET_CVT_W_S
   gencallinterp((native_type)cached_interpreter_table.CVT_W_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fd]));
   fistp_preg32_dword(EAX);
#endif
#endif
}

void gencvt_l_s(void)
{
#ifdef INTERPRET_CVT_L_S
   gencallinterp((native_type)cached_interpreter_table.CVT_L_S, 0);
#else
   gencheck_cop1_unusable();

#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_double[dst->f.cf.fd]));
   fistp_preg32_qword(EAX);
#endif
#endif
}

void genc_f_s(void)
{
#ifdef INTERPRET_C_F_S
   gencallinterp((native_type)cached_interpreter_table.C_F_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000);
#else
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000);
#endif
#endif
}

void genc_un_s(void)
{
#ifdef INTERPRET_C_UN_S
   gencallinterp((native_type)cached_interpreter_table.C_UN_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(13);
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
   jmp_imm_short(11); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(12);
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
   jmp_imm_short(10); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
#endif
#endif
}

void genc_eq_s(void)
{
#ifdef INTERPRET_C_EQ_S
   gencallinterp((native_type)cached_interpreter_table.C_EQ_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ueq_s(void)
{
#ifdef INTERPRET_C_UEQ_S
   gencallinterp((native_type)cached_interpreter_table.C_UEQ_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jne_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jne_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_olt_s(void)
{
#ifdef INTERPRET_C_OLT_S
   gencallinterp((native_type)cached_interpreter_table.C_OLT_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ult_s(void)
{
#ifdef INTERPRET_C_ULT_S
   gencallinterp((native_type)cached_interpreter_table.C_ULT_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jae_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jae_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ole_s(void)
{
#ifdef INTERPRET_C_OLE_S
   gencallinterp((native_type)cached_interpreter_table.C_OLE_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ule_s(void)
{
#ifdef INTERPRET_C_ULE_S
   gencallinterp((native_type)cached_interpreter_table.C_ULE_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   ja_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fucomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   ja_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_sf_s(void)
{
#ifdef INTERPRET_C_SF_S
   gencallinterp((native_type)cached_interpreter_table.C_SF_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000);
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000);
#endif
#endif
}

void genc_ngle_s(void)
{
#ifdef INTERPRET_C_NGLE_S
   gencallinterp((native_type)cached_interpreter_table.C_NGLE_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(13);
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
   jmp_imm_short(11); // 2
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(12);
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
   jmp_imm_short(10); // 2
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
#endif
#endif
}

void genc_seq_s(void)
{
#ifdef INTERPRET_C_SEQ_S
   gencallinterp((native_type)cached_interpreter_table.C_SEQ_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jne_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ngl_s(void)
{
#ifdef INTERPRET_C_NGL_S
   gencallinterp((native_type)cached_interpreter_table.C_NGL_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jne_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jne_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_lt_s(void)
{
#ifdef INTERPRET_C_LT_S
   gencallinterp((native_type)cached_interpreter_table.C_LT_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jae_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_nge_s(void)
{
#ifdef INTERPRET_C_NGE_S
   gencallinterp((native_type)cached_interpreter_table.C_NGE_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   jae_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   jae_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_le_s(void)
{
#ifdef INTERPRET_C_LE_S
   gencallinterp((native_type)cached_interpreter_table.C_LE_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   ja_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

void genc_ngt_s(void)
{
#ifdef INTERPRET_C_NGT_S
   gencallinterp((native_type)cached_interpreter_table.C_NGT_S, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg64_dword(RAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(15);
   ja_rj(13);
   or_m32rel_imm32((unsigned int*)&FCR31, 0x800000); // 11
   jmp_imm_short(11); // 2
   and_m32rel_imm32((unsigned int*)&FCR31, ~0x800000); // 11
#else
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.ft]));
   fld_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int *)(&reg_cop1_simple[dst->f.cf.fs]));
   fld_preg32_dword(EAX);
   fcomip_fpreg(1);
   ffree_fpreg(0);
   jp_rj(14);
   ja_rj(12);
   or_m32_imm32((unsigned int*)&FCR31, 0x800000); // 10
   jmp_imm_short(10); // 2
   and_m32_imm32((unsigned int*)&FCR31, ~0x800000); // 10
#endif
#endif
}

/* COP1 W instructions */

void gencvt_s_w(void)
{
#ifdef INTERPRET_CVT_S_W
   gencallinterp((native_type)cached_interpreter_table.CVT_S_W, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fild_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg64_dword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fs]));
   fild_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fd]));
   fstp_preg32_dword(EAX);
#endif
#endif
}

void gencvt_d_w(void)
{
#ifdef INTERPRET_CVT_D_W
   gencallinterp((native_type)cached_interpreter_table.CVT_D_W, 0);
#else
   gencheck_cop1_unusable();
#ifdef __x86_64__
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_simple[dst->f.cf.fs]));
   fild_preg64_dword(RAX);
   mov_xreg64_m64rel(RAX, (uint64_t *)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg64_qword(RAX);
#else
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_simple[dst->f.cf.fs]));
   fild_preg32_dword(EAX);
   mov_eax_memoffs32((unsigned int*)(&reg_cop1_double[dst->f.cf.fd]));
   fstp_preg32_qword(EAX);
#endif
#endif
}

