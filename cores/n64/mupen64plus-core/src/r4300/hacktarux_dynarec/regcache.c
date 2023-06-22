/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regcache.c                                              *
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

#include <stdio.h>
#include <boolean.h>

#include "regcache.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/recomph.h"

static struct precomp_instr *last_access[8];
static struct precomp_instr *free_since[8];
static int dirty[8];

static int r64[8];
static native_type *reg_content[8];
static native_type *r0;

void init_cache(struct precomp_instr* start)
{
   int i;

   for (i=0; i<8; i++)
   {
      last_access[i] = NULL;
      free_since[i] = start;
      reg_content[i] = NULL;
#if defined(__x86_64__)
      dirty[i] = 0;
      r64[i] = 0;
#endif
   }

   r0 = (native_type*)reg;
}

void free_all_registers(void)
{
  int i;

  for (i = 0; i < 8; i++)
  {
    if (last_access[i])
      free_register(i);
    else
    {
      while (free_since[i] <= dst)
      {
        free_since[i]->reg_cache_infos.needed_registers[i] = NULL;
        free_since[i]++;
      }
    }
  }
}

void simplify_access(void)
{
   int i;

   dst->local_addr = code_length;
   for(i=0; i<8; i++)
      dst->reg_cache_infos.needed_registers[i] = NULL;
}

#if defined(__x86_64__)
void free_registers_move_start(void)
{
  /* flush all dirty registers and clear needed_registers table */
  free_all_registers();

  /* now move the start of the new instruction down past the flushing instructions */
  simplify_access();
}
#endif

// this function frees a specific X86 GPR
void free_register(int reg)
{
  struct precomp_instr *last;

#if !defined(__x86_64__)
   if (last_access[reg] &&
         r64[reg] != -1 && (int)reg_content[reg] != (int)reg_content[r64[reg]]-4)
   {
      free_register(r64[reg]);
      return;
   }
#endif
   
  if (last_access[reg])
    last = last_access[reg]+1;
  else
    last = free_since[reg];
   
  while (last <= dst)
  {
     if (last_access[reg] && dirty[reg])
        last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
     else
        last->reg_cache_infos.needed_registers[reg] = NULL;

#if !defined(__x86_64__)
     if (last_access[reg] && r64[reg] != -1)
     {
        if (dirty[r64[reg]])
           last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
        else
           last->reg_cache_infos.needed_registers[r64[reg]] = NULL;
     }
#endif
     last++;
  }

  if (!last_access[reg]) 
  {
    free_since[reg] = dst+1;
    return;
  }

  if (dirty[reg]) 
  {
#if defined(__x86_64__)
     if (r64[reg])
        mov_m64rel_xreg64((uint64_t*)reg_content[reg], reg);
     else
     {
        movsxd_reg64_reg32(reg, reg);
        mov_m64rel_xreg64((uint64_t*)reg_content[reg], reg);
     }
#else
     mov_m32_reg32(reg_content[reg], reg);
     if (r64[reg] == -1)
     {
        sar_reg32_imm8(reg, 31);
        mov_m32_reg32((uint32_t*)reg_content[reg]+1, reg);
     }
     else
        mov_m32_reg32(reg_content[r64[reg]], r64[reg]);
#endif
  }

  last_access[reg] = NULL;
  free_since[reg] = dst+1;

#if !defined(__x86_64__)
  if (r64[reg] != -1)
  {
     last_access[r64[reg]] = NULL;
     free_since[r64[reg]] = dst+1;
  }
#endif
}

int lru_register(void)
{
#if defined(__x86_64__)
   uint64_t oldest_access = 0xFFFFFFFFFFFFFFFFULL;
#else
   uint32_t oldest_access = 0xFFFFFFFF;
#endif
   int i, reg = 0;

   for (i=0; i<8; i++)
   {
      bool ret  = i != ESP;
      bool ret2 = (native_type)last_access[i] < oldest_access;
      bool ret_fin = ret && ret2;

      if (ret_fin)
      {
         oldest_access = (native_type)last_access[i];
         reg = i;
      }
   }
   return reg;
}

void set_register_state(int reg, uint32_t *addr, int _dirty, int _is64bits)
{
   last_access[reg]    = NULL;
   if (addr)
      last_access[reg] = dst;
   reg_content[reg]    = (native_type*) addr;
   r64[reg]            = -1;
#ifdef __x86_64__
   r64[reg]            = _is64bits;
#endif
   dirty[reg]          = _dirty;
}

#if defined(__x86_64__)
int lru_base_register(void) /* EBP cannot be used as a base register for SIB addressing byte */
{
   uint64_t oldest_access = 0xFFFFFFFFFFFFFFFFULL;
   int i, reg = 0;

   for (i=0; i<8; i++)
   {
      if (i != ESP && i != EBP && (uint64_t) last_access[i] < oldest_access)
      {
         oldest_access = (uint64_t)last_access[i];
         reg = i;
      }
   }
   return reg;
}

int lock_register(int reg)
{
   free_register(reg);
   last_access[reg] = (struct precomp_instr *) 0xFFFFFFFFFFFFFFFFULL;
   reg_content[reg] = NULL;
   return reg;
}

void unlock_register(int reg)
{
   last_access[reg] = NULL;
}
#else

int lru_register_exc1(int exc1)
{
   uint32_t oldest_access = 0xFFFFFFFF;
   int i, reg = 0;

   for (i=0; i<8; i++)
   {
      if (i != ESP && i != exc1 && (uint32_t)last_access[i] < oldest_access)
      {
         oldest_access = (int)last_access[i];
         reg = i;
      }
   }
   return reg;
}

#endif

// this function finds a register to put the data contained in addr,
// if there was another value before it's cleanly removed of the
// register cache. After that, the register number is returned.
// If data are already cached, the function only returns the register number
#if defined(__x86_64__)
// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the LSB part
int allocate_register_64(uint64_t *addr)
{
  int reg;

  /* is it already cached? */
  if (addr)
  {
     unsigned i;
     for (i = 0; i < 8; i++)
     {
        if (last_access[i] && reg_content[i] == addr)
        {
           struct precomp_instr *last = last_access[i]+1;

           while (last <= dst)
           {
              last->reg_cache_infos.needed_registers[i] = reg_content[i];
              last++;
           }
           last_access[i] = dst;
           if (r64[i] == 0)
           {
              movsxd_reg64_reg32(i, i);
              r64[i] = 1;
           }
           return i;
        }
     }
  }

  /* it's not cached, so take the least recently used register */
  reg = lru_register();
   
  if (last_access[reg])
    free_register(reg);
  else
  {
    while (free_since[reg] <= dst)
    {
      free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      free_since[reg]++;
    }
  }
   
  last_access[reg] = dst;
  reg_content[reg] = addr;
  dirty[reg] = 0;
  r64[reg] = 1;
   
  if (addr)
  {
    if (addr == r0)
      xor_reg64_reg64(reg, reg);
    else
      mov_xreg64_m64rel(reg, addr);
  }

  return reg;
}

int allocate_register_32(uint32_t *addr)
{
   int reg = 0;

   /* is it already cached ? */
   if (addr)
   {
      int i;

      for (i = 0; i < 8; i++)
      {
         if (last_access[i] && (uint32_t*) reg_content[i] == addr)
         {
            struct precomp_instr *last = (struct precomp_instr*)(last_access[i] + 1);

            while (last <= dst)
            {
               last->reg_cache_infos.needed_registers[i] = reg_content[i];
               last++;
            }

            last_access[i] = dst;
            r64[i]         = 0;

            return i;
         }
      }
   }

   /* it's not cached, so take the least recently used register */
   reg = lru_register();

   if (last_access[reg])
      free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   last_access[reg] = dst;
   reg_content[reg] = (uint64_t*) addr;
   dirty[reg]       = 0;
   r64[reg]         = 0;

   if (addr)
   {
      if (addr == (uint32_t*) r0)
         xor_reg32_reg32(reg, reg);
      else
         mov_xreg32_m32rel(reg, addr);
   }

   return reg;
}
#else
int allocate_register(uint32_t *addr)
{
   uint32_t oldest_access = 0xFFFFFFFF;
   int reg = 0, i;

   /* is it already cached ? */
   if (addr)
   {
      for (i=0; i<8; i++)
      {
         if (last_access[i] && reg_content[i] == addr)
         {
            struct precomp_instr *last = last_access[i]+1;

            while (last <= dst)
            {
               last->reg_cache_infos.needed_registers[i] = reg_content[i];
               last++;
            }
            last_access[i] = dst;
            if (r64[i] != -1) 
            {
               last = last_access[r64[i]]+1;

               while (last <= dst)
               {
                  last->reg_cache_infos.needed_registers[r64[i]] = reg_content[r64[i]];
                  last++;
               }
               last_access[r64[i]] = dst;
            }

            return i;
         }
      }
   }

   /* if it's not cached, we take the least recently used register */
   for (i=0; i<8; i++)
   {
      if (i != ESP && (uint32_t)last_access[i] < oldest_access)
      {
         oldest_access = (int)last_access[i];
         reg = i;
      }
   }

   if (last_access[reg])
      free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   last_access[reg] = dst;
   reg_content[reg] = addr;
   dirty[reg]       = 0;
   r64[reg]         = -1;

   if (addr)
   {
      if (addr == r0 || addr == r0+1)
         xor_reg32_reg32(reg, reg);
      else
         mov_reg32_m32(reg, addr);
   }

   return reg;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the LSB part
int allocate_64_register1(uint32_t *addr)
{
   int reg1, reg2, i;

   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         if (r64[i] == -1)
         {
            allocate_register(addr);
            reg2      = allocate_register(dirty[i] ? NULL : addr+1);
            r64[i]    = reg2;
            r64[reg2] = i;

            if (dirty[i])
            {
               reg_content[reg2] = addr+1;
               dirty[reg2]       = 1;

               mov_reg32_reg32(reg2, i);
               sar_reg32_imm8(reg2, 31);
            }

            return i;
         }
      }
   }

   reg1      = allocate_register(addr);
   reg2      = allocate_register(addr+1);
   r64[reg1] = reg2;
   r64[reg2] = reg1;

   return reg1;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the MSB part
int allocate_64_register2(uint32_t *addr)
{
   int reg1, reg2, i;

   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         if (r64[i] == -1)
         {
            allocate_register(addr);
            reg2      = allocate_register(dirty[i] ? NULL : addr+1);
            r64[i]    = reg2;
            r64[reg2] = i;

            if (dirty[i])
            {
               reg_content[reg2] = addr+1;
               dirty[reg2] = 1;
               mov_reg32_reg32(reg2, i);
               sar_reg32_imm8(reg2, 31);
            }

            return reg2;
         }
      }
   }

   reg1      = allocate_register(addr);
   reg2      = allocate_register(addr+1);
   r64[reg1] = reg2;
   r64[reg2] = reg1;

   return reg2;
}
#endif


// this function checks if the data located at addr are cached in a register
// and then, it returns 1  if it's a 64 bit value
//                      0  if it's a 32 bit value
//                      -1 if it's not cached
int is64(uint32_t *addr)
{
   int i;
   for (i = 0; i < 8; i++)
   {
      if (last_access[i] && reg_content[i] == (native_type*)addr)
      {
#if defined(__x86_64__)
         return r64[i];
#else
         if (r64[i] == -1)
            return 0;
         return 1;
#endif
      }
   }
   return -1;
}

#if defined(__x86_64__)
int allocate_register_32_w(uint32_t *addr)
{
  int reg = 0, i;
   
  /* is it already cached ? */
  for (i = 0; i < 8; i++)
  {
    if (last_access[i] && reg_content[i] == (uint64_t*) addr)
    {
      struct precomp_instr *last = last_access[i] + 1;
         
      while (last <= dst)
      {
        last->reg_cache_infos.needed_registers[i] = NULL;
        last++;
      }
      last_access[i] = dst;
      dirty[i] = 1;
      r64  [i] = 0;
      return i;
    }
  }

  // it's not cached, so take the least recently used register
  reg = lru_register();
   
  if (last_access[reg])
    free_register(reg);
  else
  {
    while (free_since[reg] <= dst)
    {
      free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      free_since[reg]++;
    }
  }
   
  last_access[reg] = dst;
  reg_content[reg] = (uint64_t*) addr;
  dirty[reg] = 1;
  r64[reg] = 0;

  return reg;
}

int allocate_register_64_w(uint64_t *addr)
{
   int reg, i;

   // is it already cached?
   for (i = 0; i < 8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         struct precomp_instr *last = last_access[i] + 1;

         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[i] = NULL;
            last++;
         }
         last_access[i] = dst;
         r64[i]   = 1;
         dirty[i] = 1;
         return i;
      }
   }

   // it's not cached, so take the least recently used register
   reg = lru_register();

   if (last_access[reg])
      free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   last_access[reg] = dst;
   reg_content[reg] = addr;
   dirty[reg] = 1;
   r64[reg] = 1;

   return reg;
}

void allocate_register_32_manually(int reg, uint32_t *addr)
{
   int i;

   /* check if we just happen to already have this r4300 reg cached in the requested x86 reg */
   if (last_access[reg] && reg_content[reg] == (uint64_t*) addr)
   {
      struct precomp_instr *last = last_access[reg] + 1;
      while (last <= dst)
      {
         last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
         last++;
      }
      last_access[reg] = dst;
      /* we won't touch r64 or dirty; the register returned is "read-only" */
      return;
   }

   /* otherwise free up the requested x86 register */
   if (last_access[reg])
      free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   /* if the r4300 register is already cached in a different x86 register, then copy it to the requested x86 register */
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == (uint64_t*) addr)
      {
         struct precomp_instr *last = last_access[i]+1;
         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[i] = reg_content[i];
            last++;
         }
         last_access[i] = dst;
         if (r64[i])
            mov_reg64_reg64(reg, i);
         else
            mov_reg32_reg32(reg, i);
         last_access[reg] = dst;
         r64[reg] = r64[i];
         dirty[reg] = dirty[i];
         reg_content[reg] = reg_content[i];
         /* free the previous x86 register used to cache this r4300 register */
         free_since[i] = dst + 1;
         last_access[i] = NULL;
         return;
      }
   }

   /* otherwise just load the 32-bit value into the requested register */
   last_access[reg] = dst;
   reg_content[reg] = (uint64_t*) addr;
   dirty[reg] = 0;
   r64[reg] = 0;

   if ((uint64_t*) addr == r0)
      xor_reg32_reg32(reg, reg);
   else
      mov_xreg32_m32rel(reg, addr);
}

void allocate_register_32_manually_w(int reg, uint32_t *addr)
{
   int i;

   /* check if we just happen to already have this r4300 reg cached in the requested x86 reg */
   if (last_access[reg] && reg_content[reg] == (uint64_t*) addr)
   {
      struct precomp_instr *last = last_access[reg]+1;
      while (last <= dst)
      {
         last->reg_cache_infos.needed_registers[reg] = NULL;
         last++;
      }
      last_access[reg] = dst;
      r64[reg] = 0;
      dirty[reg] = 1;
      return;
   }

   /* otherwise free up the requested x86 register */
   if (last_access[reg])
      free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   /* if the r4300 register is already cached in a different x86 register, then free it and bind to the requested x86 register */
   for (i = 0; i < 8; i++)
   {
      if (last_access[i] && reg_content[i] == (uint64_t*) addr)
      {
         struct precomp_instr *last = last_access[i] + 1;
         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[i] = NULL;
            last++;
         }
         last_access[reg] = dst;
         reg_content[reg] = reg_content[i];
         dirty[reg] = 1;
         r64[reg] = 0;
         /* free the previous x86 register used to cache this r4300 register */
         free_since[i] = dst+1;
         last_access[i] = NULL;
         return;
      }
   }

   /* otherwise just set up the requested register as 32-bit */
   last_access[reg] = dst;
   reg_content[reg] = (uint64_t*) addr;
   dirty[reg] = 1;
   r64[reg] = 0;
}

// 0x48 0x83 0xEC 0x8                     sub rsp, byte 8
// 0x48 0xA1           0xXXXXXXXXXXXXXXXX mov rax, qword (&code start)
// 0x48 0x05                   0xXXXXXXXX add rax, dword (local_addr)
// 0x48 0x89 0x04 0x24                    mov [rsp], rax
// 0x48 0xB8           0xXXXXXXXXXXXXXXXX mov rax, &reg[0]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rdi, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rsi, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rbp, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rdx, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rcx, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rbx, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rax, [rax + XXXXXXXX]
// 0xC3 ret
// total : 84 bytes

static void build_wrapper(struct precomp_instr *instr, unsigned char* pCode, struct precomp_block* block)
{
   int i;

   *pCode++ = 0x48;
   *pCode++ = 0x83;
   *pCode++ = 0xEC;
   *pCode++ = 0x08;
   
   *pCode++ = 0x48;
   *pCode++ = 0xA1;
   *((uint64_t*) pCode) = (uint64_t)(&block->code);
   pCode += 8;
   
   *pCode++ = 0x48;
   *pCode++ = 0x05;
   *((uint32_t*) pCode) = (uint32_t)instr->local_addr;
   pCode += 4;
   
   *pCode++ = 0x48;
   *pCode++ = 0x89;
   *pCode++ = 0x04;
   *pCode++ = 0x24;

   *pCode++ = 0x48;
   *pCode++ = 0xB8;
   *((uint64_t*) pCode) = (uint64_t)&reg[0];
   pCode += 8;

   for (i=7; i>=0; i--)
   {
     if (instr->reg_cache_infos.needed_registers[i])
     {
        int64_t riprel;

        *pCode++ = 0x48;
        *pCode++ = 0x8B;
        *pCode++ = 0x80 | (i << 3);
        riprel = (int64_t) ((unsigned char *) instr->reg_cache_infos.needed_registers[i] - (unsigned char *) &reg[0]);
        *((int *) pCode) = (int) riprel;
        pCode += 4;
        if (riprel >= 0x7fffffffLL || riprel < -0x80000000LL)
        {
           DebugMessage(M64MSG_ERROR, "build_wrapper error: reg[%i] offset too big for relative address from %p to %p",
                 i, (&reg[0]), instr->reg_cache_infos.needed_registers[i]);
#if !defined(_MSC_VER)
           asm(" int $3; ");
#endif
        }
     }
   }
   *pCode++ = 0xC3;
}

#else
int allocate_register_w(uint32_t *addr)
{
   uint32_t oldest_access = 0xFFFFFFFF;
   int reg = 0, i;

   // is it already cached ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         struct precomp_instr *last = last_access[i]+1;

         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[i] = NULL;
            last++;
         }
         last_access[i] = dst;
         dirty[i] = 1;
         if (r64[i] != -1)
         {
            last = last_access[r64[i]]+1;
            while (last <= dst)
            {
               last->reg_cache_infos.needed_registers[r64[i]] = NULL;
               last++;
            }
            free_since[r64[i]] = dst+1;
            last_access[r64[i]] = NULL;
            r64[i] = -1;
         }

         return i;
      }
   }

   // if it's not cached, we take the least recently used register
   for (i=0; i<8; i++)
   {
      if (i != ESP && (uint32_t)last_access[i] < oldest_access)
      {
         oldest_access = (int)last_access[i];
         reg = i;
      }
   }

   if (last_access[reg]) free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   last_access[reg] = dst;
   reg_content[reg] = addr;
   dirty[reg] = 1;
   r64[reg] = -1;

   return reg;
}

int allocate_64_register1_w(uint32_t *addr)
{
   int reg1, reg2, i;

   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         if (r64[i] == -1)
         {
            allocate_register_w(addr);
            reg2 = lru_register();
            if (last_access[reg2]) free_register(reg2);
            else
            {
               while (free_since[reg2] <= dst)
               {
                  free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
                  free_since[reg2]++;
               }
            }
            r64[i] = reg2;
            r64[reg2] = i;
            last_access[reg2] = dst;

            reg_content[reg2] = addr+1;
            dirty[reg2] = 1;
            mov_reg32_reg32(reg2, i);
            sar_reg32_imm8(reg2, 31);

            return i;
         }
         else
         {
            last_access[i] = dst;
            last_access[r64[i]] = dst;
            dirty[i] = dirty[r64[i]] = 1;
            return i;
         }
      }
   }

   reg1 = allocate_register_w(addr);
   reg2 = lru_register();
   if (last_access[reg2]) free_register(reg2);
   else
   {
      while (free_since[reg2] <= dst)
      {
         free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
         free_since[reg2]++;
      }
   }
   r64[reg1] = reg2;
   r64[reg2] = reg1;
   last_access[reg2] = dst;
   reg_content[reg2] = addr+1;
   dirty[reg2] = 1;

   return reg1;
}

int allocate_64_register2_w(uint32_t *addr)
{
   int reg1, reg2, i;

   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         if (r64[i] == -1)
         {
            allocate_register_w(addr);
            reg2 = lru_register();
            if (last_access[reg2]) free_register(reg2);
            else
            {
               while (free_since[reg2] <= dst)
               {
                  free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
                  free_since[reg2]++;
               }
            }
            r64[i] = reg2;
            r64[reg2] = i;
            last_access[reg2] = dst;

            reg_content[reg2] = addr+1;
            dirty[reg2] = 1;
            mov_reg32_reg32(reg2, i);
            sar_reg32_imm8(reg2, 31);

            return reg2;
         }
         else
         {
            last_access[i] = dst;
            last_access[r64[i]] = dst;
            dirty[i] = dirty[r64[i]] = 1;
            return r64[i];
         }
      }
   }

   reg1 = allocate_register_w(addr);
   reg2 = lru_register();
   if (last_access[reg2]) free_register(reg2);
   else
   {
      while (free_since[reg2] <= dst)
      {
         free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
         free_since[reg2]++;
      }
   }
   r64[reg1] = reg2;
   r64[reg2] = reg1;
   last_access[reg2] = dst;
   reg_content[reg2] = addr+1;
   dirty[reg2] = 1;

   return reg2;
}


void set_64_register_state(int reg1, int reg2, uint32_t *addr, int d)
{
   last_access[reg1] = dst;
   last_access[reg2] = dst;
   reg_content[reg1] = addr;
   reg_content[reg2] = addr+1;
   r64[reg1] = reg2;
   r64[reg2] = reg1;
   dirty[reg1] = d;
   dirty[reg2] = d;
}

void allocate_register_manually(int reg, uint32_t *addr)
{
   int i;

   if (last_access[reg] && reg_content[reg] == addr)
   {
      struct precomp_instr *last = last_access[reg]+1;

      while (last <= dst)
      {
         last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
         last++;
      }
      last_access[reg] = dst;
      if (r64[reg] != -1) 
      {
         last = last_access[r64[reg]]+1;

         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
            last++;
         }
         last_access[r64[reg]] = dst;
      }
      return;
   }

   if (last_access[reg]) free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   // is it already cached ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         struct precomp_instr *last = last_access[i]+1;

         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[i] = reg_content[i];
            last++;
         }
         last_access[i] = dst;
         if (r64[i] != -1) 
         {
            last = last_access[r64[i]]+1;

            while (last <= dst)
            {
               last->reg_cache_infos.needed_registers[r64[i]] = reg_content[r64[i]];
               last++;
            }
            last_access[r64[i]] = dst;
         }

         mov_reg32_reg32(reg, i);
         last_access[reg] = dst;
         r64[reg] = r64[i];
         if (r64[reg] != -1) r64[r64[reg]] = reg;
         dirty[reg] = dirty[i];
         reg_content[reg] = reg_content[i];
         free_since[i] = dst+1;
         last_access[i] = NULL;

         return;
      }
   }

   last_access[reg] = dst;
   reg_content[reg] = addr;
   dirty[reg] = 0;
   r64[reg] = -1;

   if (addr)
   {
      if (addr == r0 || addr == r0+1)
         xor_reg32_reg32(reg, reg);
      else
         mov_reg32_m32(reg, addr);
   }
}

void allocate_register_manually_w(int reg, uint32_t *addr, int load)
{
   int i;

   if (last_access[reg] && reg_content[reg] == addr)
   {
      struct precomp_instr *last = last_access[reg]+1;

      while (last <= dst)
      {
         last->reg_cache_infos.needed_registers[reg] = reg_content[reg];
         last++;
      }
      last_access[reg] = dst;

      if (r64[reg] != -1) 
      {
         last = last_access[r64[reg]]+1;

         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[r64[reg]] = reg_content[r64[reg]];
            last++;
         }
         last_access[r64[reg]] = NULL;
         free_since[r64[reg]] = dst+1;
         r64[reg] = -1;
      }
      dirty[reg] = 1;
      return;
   }

   if (last_access[reg]) free_register(reg);
   else
   {
      while (free_since[reg] <= dst)
      {
         free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         free_since[reg]++;
      }
   }

   // is it already cached ?
   for (i=0; i<8; i++)
   {
      if (last_access[i] && reg_content[i] == addr)
      {
         struct precomp_instr *last = last_access[i]+1;

         while (last <= dst)
         {
            last->reg_cache_infos.needed_registers[i] = reg_content[i];
            last++;
         }
         last_access[i] = dst;
         if (r64[i] != -1)
         {
            last = last_access[r64[i]]+1;
            while (last <= dst)
            {
               last->reg_cache_infos.needed_registers[r64[i]] = NULL;
               last++;
            }
            free_since[r64[i]] = dst+1;
            last_access[r64[i]] = NULL;
            r64[i] = -1;
         }

         if (load)
            mov_reg32_reg32(reg, i);
         last_access[reg] = dst;
         dirty[reg] = 1;
         r64[reg] = -1;
         reg_content[reg] = reg_content[i];
         free_since[i] = dst+1;
         last_access[i] = NULL;

         return;
      }
   }

   last_access[reg] = dst;
   reg_content[reg] = addr;
   dirty[reg]       = 1;
   r64[reg]         = -1;

   if (addr && load)
   {
      if (addr == r0 || addr == r0+1)
         xor_reg32_reg32(reg, reg);
      else
         mov_reg32_m32(reg, addr);
   }
}

// 0x81 0xEC 0x4 0x0 0x0 0x0  sub esp, 4
// 0xA1            0xXXXXXXXX mov eax, XXXXXXXX (&code start)
// 0x05            0xXXXXXXXX add eax, XXXXXXXX (local_addr)
// 0x89 0x04 0x24             mov [esp], eax
// 0x8B (reg<<3)|5 0xXXXXXXXX mov eax, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ebx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ecx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov edx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ebp, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov esi, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov edi, [XXXXXXXX]
// 0xC3 ret
// total : 62 bytes
static void build_wrapper(struct precomp_instr *instr, unsigned char* pCode, struct precomp_block* block)
{
   int i;
   int j=0;

   pCode[j++] = 0x81;
   pCode[j++] = 0xEC;
   pCode[j++] = 0x04;
   pCode[j++] = 0x00;
   pCode[j++] = 0x00;
   pCode[j++] = 0x00;

   pCode[j++] = 0xA1;
   *((uint32_t*)&pCode[j]) = (uint32_t)(&block->code);
   j+=4;

   pCode[j++] = 0x05;
   *((uint32_t*)&pCode[j]) = (uint32_t)instr->local_addr;
   j+=4;

   pCode[j++] = 0x89;
   pCode[j++] = 0x04;
   pCode[j++] = 0x24;

   for (i=0; i<8; i++)
   {
      if (instr->reg_cache_infos.needed_registers[i])
      {
         pCode[j++] = 0x8B;
         pCode[j++] = (i << 3) | 5;
         *((uint32_t*)&pCode[j]) =
            (uint32_t)instr->reg_cache_infos.needed_registers[i];
         j+=4;
      }
   }

   pCode[j++] = 0xC3;
}
#endif

void build_wrappers(struct precomp_instr *instr, int start, int end, struct precomp_block* block)
{
   int i, reg;

   for (i=start; i<end; i++)
   {
      instr[i].reg_cache_infos.need_map = 0;
      for (reg=0; reg<8; reg++)
      {
         if (instr[i].reg_cache_infos.needed_registers[reg])
         {
            instr[i].reg_cache_infos.need_map = 1;
            build_wrapper(&instr[i], instr[i].reg_cache_infos.jump_wrapper, block);
            break;
         }
      }
   }
}
