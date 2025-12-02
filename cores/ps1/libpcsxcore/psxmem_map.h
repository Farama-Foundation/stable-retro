/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2016  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#ifndef __PSXMEM_MAP_H__
#define __PSXMEM_MAP_H__

#ifdef __cplusplus
extern "C" {
#endif

enum psxMapTag {
	MAP_TAG_OTHER = 0,
	MAP_TAG_RAM,
	MAP_TAG_VRAM,
	MAP_TAG_LUTS,
};

extern void *(*psxMapHook)(unsigned long addr, size_t size,
	enum psxMapTag tag, int *can_retry_addr);
extern void (*psxUnmapHook)(void *ptr, size_t size, enum psxMapTag tag);

void *psxMap(unsigned long addr, size_t size, int is_fixed,
		enum psxMapTag tag);
void psxUnmap(void *ptr, size_t size, enum psxMapTag tag);

#ifdef __cplusplus
}
#endif
#endif
