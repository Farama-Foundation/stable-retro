/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
* PSX memory functions.
*/

// TODO: Implement caches & cycle penalty.

#include "psxmem.h"
#include "psxmem_map.h"
#include "r3000a.h"
#include "psxhw.h"
//#include "debug.h"
#define DebugCheckBP(...)

#include "lightrec/mem.h"
#include "memmap.h"

#ifdef USE_LIBRETRO_VFS
#include <streams/file_stream_transforms.h>
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static void * psxMapDefault(unsigned long addr, size_t size,
			    enum psxMapTag tag, int *can_retry_addr)
{
	void *ptr;
#if !P_HAVE_MMAP
	*can_retry_addr = 0;
	ptr = calloc(1, size);
	return ptr ? ptr : MAP_FAILED;
#else
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;

	*can_retry_addr = 1;
	ptr = mmap((void *)(uintptr_t)addr, size,
		    PROT_READ | PROT_WRITE, flags, -1, 0);
#ifdef MADV_HUGEPAGE
	if (size >= 2*1024*1024) {
		if (ptr != MAP_FAILED && ((uintptr_t)ptr & (2*1024*1024 - 1))) {
			// try to manually realign assuming decreasing addr alloc
			munmap(ptr, size);
			addr = (uintptr_t)ptr & ~(2*1024*1024 - 1);
			ptr = mmap((void *)(uintptr_t)addr, size,
				PROT_READ | PROT_WRITE, flags, -1, 0);
		}
		if (ptr != MAP_FAILED)
			madvise(ptr, size, MADV_HUGEPAGE);
	}
#endif
	return ptr;
#endif
}

static void psxUnmapDefault(void *ptr, size_t size, enum psxMapTag tag)
{
#if !P_HAVE_MMAP
	free(ptr);
#else
	munmap(ptr, size);
#endif
}

void *(*psxMapHook)(unsigned long addr, size_t size,
		enum psxMapTag tag, int *can_retry_addr) = psxMapDefault;
void (*psxUnmapHook)(void *ptr, size_t size,
		     enum psxMapTag tag) = psxUnmapDefault;

void *psxMap(unsigned long addr, size_t size, int is_fixed,
		enum psxMapTag tag)
{
	int try_, can_retry_addr = 0;
	void *ret = MAP_FAILED;

	for (try_ = 0; try_ < 3; try_++)
	{
		if (ret != MAP_FAILED)
			psxUnmap(ret, size, tag);
		ret = psxMapHook(addr, size, tag, &can_retry_addr);
		if (ret == MAP_FAILED)
			return MAP_FAILED;

		if (addr != 0 && ret != (void *)(uintptr_t)addr) {
			SysMessage("psxMap: tried to map @%08lx, got %p\n",
				addr, ret);
			if (is_fixed) {
				psxUnmap(ret, size, tag);
				return MAP_FAILED;
			}

			if (can_retry_addr && ((addr ^ (uintptr_t)ret) & ~0xff000000l)) {
				unsigned long mask;

				// try to use similarly aligned memory instead
				// (recompiler prefers this)
				mask = try_ ? 0xffff : 0xffffff;
				addr = ((uintptr_t)ret + mask) & ~mask;
				continue;
			}
		}
		break;
	}

	return ret;
}

void psxUnmap(void *ptr, size_t size, enum psxMapTag tag)
{
	psxUnmapHook(ptr, size, tag);
}

s8 *psxM = NULL; // Kernel & User Memory (2 Meg)
s8 *psxP = NULL; // Parallel Port (64K)
s8 *psxR = NULL; // BIOS ROM (512K)
s8 *psxH = NULL; // Scratch Pad (1K) & Hardware Registers (8K)

u8 **psxMemWLUT = NULL;
u8 **psxMemRLUT = NULL;

/*  Playstation Memory Map (from Playstation doc by Joshua Walker)
0x0000_0000-0x0000_ffff		Kernel (64K)
0x0001_0000-0x001f_ffff		User Memory (1.9 Meg)

0x1f00_0000-0x1f00_ffff		Parallel Port (64K)

0x1f80_0000-0x1f80_03ff		Scratch Pad (1024 bytes)

0x1f80_1000-0x1f80_2fff		Hardware Registers (8K)

0x1fc0_0000-0x1fc7_ffff		BIOS (512K)

0x8000_0000-0x801f_ffff		Kernel and User Memory Mirror (2 Meg) Cached
0x9fc0_0000-0x9fc7_ffff		BIOS Mirror (512K) Cached

0xa000_0000-0xa01f_ffff		Kernel and User Memory Mirror (2 Meg) Uncached
0xbfc0_0000-0xbfc7_ffff		BIOS Mirror (512K) Uncached
*/

static int psxMemInitMap(void)
{
	psxM = psxMap(0x80000000, 0x00210000, 1, MAP_TAG_RAM);
	if (psxM == MAP_FAILED)
		psxM = psxMap(0x77000000, 0x00210000, 0, MAP_TAG_RAM);
	if (psxM == MAP_FAILED) {
		SysMessage("mapping main RAM failed");
		psxM = NULL;
		return -1;
	}
	psxP = &psxM[0x200000];

	psxH = psxMap(0x1f800000, 0x10000, 0, MAP_TAG_OTHER);
	if (psxH == MAP_FAILED) {
		SysMessage("Error allocating psxH");
		psxH = NULL;
		return -1;
	}

	psxR = psxMap(0x1fc00000, 0x80000, 0, MAP_TAG_OTHER);
	if (psxR == MAP_FAILED) {
		SysMessage("Error allocating psxR");
		psxR = NULL;
		return -1;
	}

	return 0;
}

static void psxMemFreeMap(void)
{
	if (psxM) psxUnmap(psxM, 0x00210000, MAP_TAG_RAM);
	if (psxH) psxUnmap(psxH, 0x10000, MAP_TAG_OTHER);
	if (psxR) psxUnmap(psxR, 0x80000, MAP_TAG_OTHER);
	psxM = psxH = psxR = NULL;
	psxP = NULL;
}

int psxMemInit(void)
{
	unsigned int i;
	int ret;

	if (LIGHTREC_CUSTOM_MAP)
		ret = lightrec_init_mmap();
	else
		ret = psxMemInitMap();
	if (ret) {
		if (LIGHTREC_CUSTOM_MAP)
			SysMessage("lightrec_init_mmap failed");
		psxMemShutdown();
		return -1;
	}

	if (DISABLE_MEM_LUTS)
		return 0;

	psxMemRLUT = (u8 **)malloc(0x10000 * sizeof(void *));
	psxMemWLUT = (u8 **)malloc(0x10000 * sizeof(void *));

	if (psxMemRLUT == NULL || psxMemWLUT == NULL) {
		SysMessage("Error allocating psxMem LUTs");
		psxMemShutdown();
		return -1;
	}

	memset(psxMemRLUT, (int)(uintptr_t)INVALID_PTR, 0x10000 * sizeof(void *));
	memset(psxMemWLUT, (int)(uintptr_t)INVALID_PTR, 0x10000 * sizeof(void *));

// MemR
	for (i = 0; i < 0x80; i++) psxMemRLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];

	memcpy(psxMemRLUT + 0x8000, psxMemRLUT, 0x80 * sizeof(void *));
	memcpy(psxMemRLUT + 0xa000, psxMemRLUT, 0x80 * sizeof(void *));

	psxMemRLUT[0x1f00] = (u8 *)psxP;
	psxMemRLUT[0x1f80] = (u8 *)psxH;

	for (i = 0; i < 0x08; i++) psxMemRLUT[i + 0x1fc0] = (u8 *)&psxR[i << 16];

	memcpy(psxMemRLUT + 0x9fc0, psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));
	memcpy(psxMemRLUT + 0xbfc0, psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));

// MemW
	for (i = 0; i < 0x80; i++) psxMemWLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];

	memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
	memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));

	// Don't allow writes to PIO Expansion region (psxP) to take effect.
	// NOTE: Not sure if this is needed to fix any games but seems wise,
	//       seeing as some games do read from PIO as part of copy-protection
	//       check. (See fix in psxMemReset() regarding psxP region reads).
	psxMemWLUT[0x1f00] = INVALID_PTR;
	psxMemWLUT[0x1f80] = (u8 *)psxH;

	return 0;
}

void psxMemReset() {
	FILE *f = NULL;
	char bios[1024];

	memset(psxM, 0, 0x00200000);
	memset(psxP, 0xff, 0x00010000);

	Config.HLE = TRUE;

	if (strcmp(Config.Bios, "HLE") != 0) {
		sprintf(bios, "%s/%s", Config.BiosDir, Config.Bios);
		f = fopen(bios, "rb");

		if (f == NULL) {
			SysMessage(_("Could not open BIOS:\"%s\". Enabling HLE Bios!\n"), bios);
		} else {
			if (fread(psxR, 1, 0x80000, f) == 0x80000) {
				Config.HLE = FALSE;
			} else {
				SysMessage(_("The selected BIOS:\"%s\" is of wrong size. Enabling HLE Bios!\n"), bios);
			}
			fclose(f);
		}
	}
	if (Config.HLE)
		memset(psxR, 0, 0x80000);
}

void psxMemShutdown() {
	if (LIGHTREC_CUSTOM_MAP)
		lightrec_free_mmap();
	else
		psxMemFreeMap();

	free(psxMemRLUT); psxMemRLUT = NULL;
	free(psxMemWLUT); psxMemWLUT = NULL;
}

int cache_isolated;

void psxMemOnIsolate(int enable)
{
	if (!DISABLE_MEM_LUTS) {
		if (enable) {
			memset(psxMemWLUT + 0x0000, (int)(uintptr_t)INVALID_PTR, 0x80 * sizeof(void *));
			memset(psxMemWLUT + 0x8000, (int)(uintptr_t)INVALID_PTR, 0x80 * sizeof(void *));
			//memset(psxMemWLUT + 0xa000, (int)(uintptr_t)INVALID_PTR, 0x80 * sizeof(void *));
		} else {
			int i;
			for (i = 0; i < 0x80; i++)
				psxMemWLUT[i + 0x0000] = (void *)&psxM[(i & 0x1f) << 16];
			memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
			memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));
		}
	}

	cache_isolated = enable;
	psxCpu->Notify(enable ? R3000ACPU_NOTIFY_CACHE_ISOLATED
			: R3000ACPU_NOTIFY_CACHE_UNISOLATED, NULL);
}

u8 psxMemRead8(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return psxHu8(mem);
		else
			return psxHwRead8(mem);
	} else {
		p = psxm(mem, 0);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, R1);
			return *(u8 *)p;
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lb %8.8lx\n", mem);
#endif
			return 0xFF;
		}
	}
}

u16 psxMemRead16(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return psxHu16(mem);
		else
			return psxHwRead16(mem);
	} else {
		p = psxm(mem, 0);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, R2);
			return SWAPu16(*(u16 *)p);
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lh %8.8lx\n", mem);
#endif
			return 0xFFFF;
		}
	}
}

u32 psxMemRead32(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return psxHu32(mem);
		else
			return psxHwRead32(mem);
	} else {
		p = psxm(mem, 0);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, R4);
			return SWAPu32(*(u32 *)p);
		} else {
			if (mem == 0xfffe0130)
				return psxRegs.biuReg;
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err lw %8.8lx\n", mem);
#endif
			return 0xFFFFFFFF;
		}
	}
}

void psxMemWrite8(u32 mem, u32 value) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			psxHu8(mem) = value;
		else
			psxHwWrite8(mem, value);
	} else {
		p = psxm(mem, 1);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, W1);
			*(u8 *)p = value;
#ifndef DRC_DISABLE
			psxCpu->Clear((mem & (~3)), 1);
#endif
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sb %8.8lx\n", mem);
#endif
		}
	}
}

void psxMemWrite16(u32 mem, u32 value) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			psxHu16ref(mem) = SWAPu16(value);
		else
			psxHwWrite16(mem, value);
	} else {
		p = psxm(mem, 1);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, W2);
			*(u16 *)p = SWAPu16(value);
#ifndef DRC_DISABLE
			psxCpu->Clear((mem & (~3)), 1);
#endif
		} else {
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sh %8.8lx\n", mem);
#endif
		}
	}
}

void psxMemWrite32(u32 mem, u32 value) {
	char *p;
	u32 t;

//	if ((mem&0x1fffff) == 0x71E18 || value == 0x48088800) SysPrintf("t2fix!!\n");
	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			psxHu32ref(mem) = SWAPu32(value);
		else
			psxHwWrite32(mem, value);
	} else {
		p = psxm(mem, 1);
		if (p != INVALID_PTR) {
			if (Config.Debug)
				DebugCheckBP((mem & 0xffffff) | 0x80000000, W4);
			*(u32 *)p = SWAPu32(value);
#ifndef DRC_DISABLE
			psxCpu->Clear(mem, 1);
#endif
		} else {
			if (mem == 0xfffe0130) {
				psxRegs.biuReg = value;
				return;
			}
#ifdef PSXMEM_LOG
			PSXMEM_LOG("err sw %8.8lx\n", mem);
#endif
		}
	}
}

void *psxMemPointer(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80 || t == 0x9f80 || t == 0xbf80) {
		if ((mem & 0xffff) < 0x400)
			return (void *)&psxH[mem];
		else
			return NULL;
	} else {
		p = psxm(mem, 1);
		if (p != INVALID_PTR) {
			return (void *)p;
		}
		return NULL;
	}
}
