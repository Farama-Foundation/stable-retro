/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - recomp.h                                                *
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

#ifndef M64P_R4300_RECOMP_H
#define M64P_R4300_RECOMP_H

#include <stdint.h>

#if defined(__LIBRETRO_WIN64__)
#define __x86_64__
#endif

#include <stddef.h>

#include "recomp_types.h"

void recompile_block(const uint32_t *source, struct precomp_block *block, uint32_t func);
void init_block(struct precomp_block *block);
void free_block(struct precomp_block *block);
void recompile_opcode(void);
void dyna_jump(void);
void dyna_start(void *code);
void dyna_stop(void);
void *realloc_exec(void *ptr, size_t oldsize, size_t newsize);

extern struct precomp_instr *dst; /* precomp_instr structure for instruction being recompiled */

extern int no_compiled_jump;

#ifdef DYNAREC
#include "hacktarux_dynarec/assemble.h"
#endif
#include "hacktarux_dynarec/regcache.h"

#endif /* M64P_R4300_RECOMP_H */

