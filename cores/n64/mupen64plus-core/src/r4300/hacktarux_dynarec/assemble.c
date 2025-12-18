/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assemble.c                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
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

#include <stdlib.h>
#include <stdio.h>

#include "assemble.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "osal/preproc.h"
#include "r4300/recomph.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"

/* (64-bit x86_64 only) */
typedef struct _riprelative_table
{
   unsigned int   pc_addr;     /* index in bytes from start of x86_64 code block to the displacement value to write */
   unsigned int   extra_bytes; /* number of remaining instruction bytes (immediate data) after 4-byte displacement */
   unsigned char *global_dst;  /* 64-bit pointer to the data object */
} riprelative_table;

typedef struct _jump_table
{
   unsigned int mi_addr;
   unsigned int pc_addr;
#ifdef __x86_64__
   unsigned int absolute64;
#endif
} jump_table;

#ifdef __x86_64__
#define JUMP_TABLE_SIZE 512
#else
#define JUMP_TABLE_SIZE 1000
#endif

static jump_table *jumps_table = NULL;
static int jumps_number, max_jumps_number;

static riprelative_table *riprel_table = NULL;
static int riprel_number = 0, max_riprel_number = 0;

void init_assembler(void *block_jumps_table, int block_jumps_number, void *block_riprel_table, int block_riprel_number)
{
   if (block_jumps_table)
   {
      jumps_table = (jump_table *) block_jumps_table;
      jumps_number = block_jumps_number;
#ifdef __x86_64__
      if (jumps_number <= JUMP_TABLE_SIZE)
         max_jumps_number = JUMP_TABLE_SIZE;
      else
         max_jumps_number = (jumps_number + (JUMP_TABLE_SIZE-1)) & 0xfffffe00;
#else
      max_jumps_number = jumps_number;
#endif
   }
   else
   {
      jumps_table = (jump_table *) malloc(JUMP_TABLE_SIZE * sizeof(jump_table));
      jumps_number     = 0;
      max_jumps_number = JUMP_TABLE_SIZE;
   }

#ifdef __x86_64__
   if (block_riprel_table)
   {
      riprel_table = block_riprel_table;
      riprel_number = block_riprel_number;
      if (riprel_number <= JUMP_TABLE_SIZE)
         max_riprel_number = JUMP_TABLE_SIZE;
      else
         max_riprel_number = (riprel_number + (JUMP_TABLE_SIZE-1)) & 0xfffffe00;
   }
   else
   {
      riprel_table = malloc(JUMP_TABLE_SIZE * sizeof(riprelative_table));
      riprel_number     = 0;
      max_riprel_number = JUMP_TABLE_SIZE; 
   }
#endif
}

void free_assembler(void **block_jumps_table, int *block_jumps_number, void **block_riprel_table, int *block_riprel_number)
{
   *block_jumps_table   = jumps_table;
   *block_jumps_number  = jumps_number;
   *block_riprel_table  = NULL;  /* RIP-relative addressing is only for x86-64 */
#ifdef __x86_64__
   *block_riprel_table  = riprel_table;
#endif
   *block_riprel_number = 0;
}

void add_jump(unsigned int pc_addr, unsigned int mi_addr, unsigned int absolute64)
{
   if (jumps_number == max_jumps_number)
   {
      jump_table *new_ptr = NULL;

      max_jumps_number += JUMP_TABLE_SIZE;
      new_ptr           = (jump_table *)
         realloc(jumps_table, max_jumps_number*sizeof(jump_table));
      if (!new_ptr)
         return;
      jumps_table = new_ptr;
   }
   jumps_table[jumps_number].pc_addr = pc_addr;
   jumps_table[jumps_number].mi_addr = mi_addr;
#ifdef __x86_64__
   jumps_table[jumps_number].absolute64 = absolute64;
#endif
   jumps_number++;
}

void passe2(struct precomp_instr *dest, int start, int end, struct precomp_block *block)
{
   unsigned int real_code_length;
   int i;
   build_wrappers(dest, start, end, block);

#ifdef __x86_64__
   /* First, fix up all the jumps.  This involves a table lookup to find the offset into the block of x86_64 code for
    * for start of a recompiled r4300i instruction corresponding to the given jump destination address in the N64
    * address space.  Next, the relative offset between this destination and the location of the jump instruction is
    * computed and stored in memory, so that the jump will branch to the right place in the recompiled code.
    */
   for (i = 0; i < jumps_number; i++)
   {
      struct precomp_instr *jump_instr = dest + ((jumps_table[i].mi_addr - dest[0].addr) / 4);
      unsigned int   jmp_offset_loc = jumps_table[i].pc_addr;
      unsigned char *addr_dest = NULL;
      /* calculate the destination address to jump to */
      if (jump_instr->reg_cache_infos.need_map)
         addr_dest = jump_instr->reg_cache_infos.jump_wrapper;
      else
         addr_dest = block->code + jump_instr->local_addr;

      /* write either a 32-bit IP-relative offset or a 64-bit absolute address */
      if (jumps_table[i].absolute64)
         *((uint64_t *) (block->code + jmp_offset_loc)) = (uint64_t)addr_dest;
      else
      {
         long jump_rel_offset = (long) (addr_dest - (block->code + jmp_offset_loc + 4));
         *((int *) (block->code + jmp_offset_loc)) = (int) jump_rel_offset;
         if (jump_rel_offset >= 0x7fffffffLL || jump_rel_offset < -0x80000000LL)
         {
            DebugMessage(M64MSG_ERROR, "assembler pass2 error: offset too big for relative jump from %p to %p",
                  (block->code + jmp_offset_loc + 4), addr_dest);
#if !defined(_MSC_VER)
            asm(" int $3; ");
#endif
         }
      }
   }

   /* Next, fix up all of the RIP-relative memory accesses.  This is unique to the x86_64 architecture, because
    * the 32-bit absolute displacement addressing mode is not available (and there's no 64-bit absolute displacement
    * mode either).
    */
   for (i = 0; i < riprel_number; i++)
   {
      unsigned char *rel_offset_ptr = block->code + riprel_table[i].pc_addr;
      long rip_rel_offset = (long) (riprel_table[i].global_dst - (rel_offset_ptr + 4 + riprel_table[i].extra_bytes));
      if (rip_rel_offset >= 0x7fffffffLL || rip_rel_offset < -0x80000000LL)
      {
         DebugMessage(M64MSG_ERROR, "assembler pass2 error: offset too big between mem target: %p and code position: %p",
               riprel_table[i].global_dst, rel_offset_ptr);
#if !defined(_MSC_VER)
         asm(" int $3; ");
#endif
      }
      *((int *) rel_offset_ptr) = (int) rip_rel_offset;
   }
#else

   real_code_length = code_length;

   for (i=0; i < jumps_number; i++)
   {
      code_length = jumps_table[i].pc_addr;
      if (dest[(jumps_table[i].mi_addr - dest[0].addr)/4].reg_cache_infos.need_map)
      {
         unsigned int addr_dest = (unsigned int)dest[(jumps_table[i].mi_addr - dest[0].addr)/4].reg_cache_infos.jump_wrapper;
         put32(addr_dest-((unsigned int)block->code+code_length)-4);
      }
      else
      {
         unsigned int addr_dest = dest[(jumps_table[i].mi_addr - dest[0].addr)/4].local_addr;
         put32(addr_dest-code_length-4);
      }
   }
   code_length = real_code_length;
#endif
}

static unsigned int g_jump_start8 = 0;
static unsigned int g_jump_start32 = 0;

void jump_start_rel8(void)
{
   g_jump_start8 = code_length;
}

void jump_start_rel32(void)
{
   g_jump_start32 = code_length;
}

void jump_end_rel8(void)
{
   unsigned int jump_end = code_length;
   int jump_vec = jump_end - g_jump_start8;

   if (jump_vec > 127 || jump_vec < -128)
   {
      DebugMessage(M64MSG_ERROR, "8-bit relative jump too long! From %x to %x", g_jump_start8, jump_end);
#if !defined(_MSC_VER)
      asm(" int $3; ");
#endif
   }

   code_length = g_jump_start8 - 1;
   put8(jump_vec);
   code_length = jump_end;
}

void jump_end_rel32(void)
{
   unsigned int jump_end = code_length;
   int jump_vec = jump_end - g_jump_start32;

   code_length = g_jump_start32 - 4;
   put32(jump_vec);
   code_length = jump_end;
}
