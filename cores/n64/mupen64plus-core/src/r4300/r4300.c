/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - r4300.c                                                 *
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "api/callbacks.h"
#include "api/debugger.h"
#include "api/m64p_types.h"
#include "cached_interp.h"
#include "cp0_private.h"
#include "cp1_private.h"
#include "interrupt.h"
#include "main/main.h"
#include "main/device.h"
#include "main/rom.h"
#include "new_dynarec/new_dynarec.h"
#include "ops.h"
#include "pure_interp.h"
#include "r4300.h"
#include "r4300_core.h"
#include "recomp.h"
#include "recomph.h"
#include "tlb.h"

#ifdef DBG
#include "debugger/dbg_debugger.h"
#include "debugger/dbg_types.h"
#endif

unsigned int r4300emu = 0;
unsigned int count_per_op = COUNT_PER_OP_DEFAULT;
unsigned int llbit;
int stop;
#if NEW_DYNAREC < NEW_DYNAREC_ARM
int64_t reg[32], hi, lo;
uint32_t next_interrupt;
struct precomp_instr *PC;
#endif
long long int local_rs;
uint32_t skip_jump = 0;
unsigned int dyna_interp = 0;
uint32_t last_addr;

cpu_instruction_table current_instruction_table;

void generic_jump_to(uint32_t address)
{
   if (r4300emu == CORE_PURE_INTERPRETER)
      PC->addr = address;
   else {
#if NEW_DYNAREC
      if (r4300emu == CORE_DYNAREC)
         last_addr = pcaddr;
      else
         jump_to(address);
#else
      jump_to(address);
#endif
   }
}

#if !defined(NO_ASM)
static void dynarec_setup_code(void)
{
   // The dynarec jumps here after we call dyna_start and it prepares
   // Here we need to prepare the initial code block and jump to it
   jump_to(UINT32_C(0xa4000040));

   // Prevent segfault on failed jump_to
   if (!actual->block || !actual->code)
      dyna_stop();
}
#endif

void r4300_init(void)
{
    current_instruction_table = cached_interpreter_table;

    stop = 0;

    if (r4300emu == CORE_PURE_INTERPRETER)
    {
        DebugMessage(M64MSG_INFO, "Starting R4300 emulator: Pure Interpreter");
        r4300emu = CORE_PURE_INTERPRETER;
        pure_interpreter_init();
    }
#if defined(DYNAREC)
    else if (r4300emu >= 2)
    {
        DebugMessage(M64MSG_INFO, "Starting R4300 emulator: Dynamic Recompiler");
        r4300emu = CORE_DYNAREC;
        init_blocks();

#if NEW_DYNAREC
        new_dynarec_init();
#else
        dyna_start(dynarec_setup_code);
#endif
    }
#endif
    else /* if (r4300emu == CORE_INTERPRETER) */
    {
        DebugMessage(M64MSG_INFO, "Starting R4300 emulator: Cached Interpreter");
        r4300emu = CORE_INTERPRETER;
        init_blocks();
        jump_to(UINT32_C(0xa4000040));

        /* Prevent segfault on failed jump_to */
        if (!actual->block)
            return;

        last_addr = PC->addr;
    }
}

void r4300_execute(void)
{
    if (r4300emu == CORE_PURE_INTERPRETER)
    {
        pure_interpreter();
    }
#if defined(DYNAREC)
    else if (r4300emu >= 2)
    {
#if NEW_DYNAREC
        new_dyna_start();
        if (stop)
            new_dynarec_cleanup();
#else
        dyna_start(dynarec_setup_code);
        if (stop)
            PC++;
#endif
        if (stop)
            free_blocks();
    }
#endif
    else /* if (r4300emu == CORE_INTERPRETER) */
    {
        r4300_step();

        if (stop)
            free_blocks();
    }

    if (stop)
        DebugMessage(M64MSG_INFO, "R4300 emulator finished.");
}

int retro_stop_stepping(void);

void r4300_step(void)
{
   while (!stop && !retro_stop_stepping())
   {
      PC->ops();
   }
}
