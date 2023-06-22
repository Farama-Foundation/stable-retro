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

#ifndef __REGCACHE_H__
#define __REGCACHE_H__

#include "r4300/recomp.h"

void free_register(int reg);
void init_cache(struct precomp_instr* start);
#if defined(__x86_64__)
void free_registers_move_start(void);
int allocate_register_32(uint32_t *addr);
int allocate_register_64(uint64_t *addr);
int allocate_register_32_w(uint32_t *addr);
int allocate_register_64_w(uint64_t *addr);
void allocate_register_32_manually(int reg, uint32_t *addr);
void allocate_register_32_manually_w(int reg, uint32_t *addr);

#else
int lru_register_exc1(int exc1);
int allocate_register(uint32_t *addr);
int allocate_64_register1(uint32_t *addr);
int allocate_64_register2(uint32_t *addr);
int allocate_register_w(uint32_t *addr);
int allocate_64_register1_w(uint32_t *addr);
int allocate_64_register2_w(uint32_t *addr);
void allocate_register_manually(int reg, uint32_t *addr);
void allocate_register_manually_w(int reg, uint32_t *addr, int load);

void set_64_register_state(int reg1, int reg2, uint32_t *addr, int dirty);
#endif

void simplify_access(void);
void free_all_registers(void);
int is64(uint32_t *addr);
int lru_register(void);
int lru_base_register(void);
void set_register_state(int reg, uint32_t *addr, int dirty, int is64bits);
int lock_register(int reg);
void unlock_register(int reg);
void build_wrappers(struct precomp_instr*, int, int, struct precomp_block*);

#endif /* __REGCACHE_H__ */

