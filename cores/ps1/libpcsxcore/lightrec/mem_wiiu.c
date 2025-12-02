// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Ash Logan <ash@heyquark.com>
 */

#include <coreinit/memorymap.h>
#include <malloc.h>
#include <stdbool.h>

#include "../memmap.h"
#include "../psxhw.h"
#include "../psxmem.h"
#include "../r3000a.h"

#include "mem.h"

void wiiu_clear_cache(void *start, void *end);

static void* wiiu_mmap(uint32_t requested_va, size_t length, void* backing_mem) {
	if (length < OS_PAGE_SIZE) length = OS_PAGE_SIZE;

	uint32_t va = OSAllocVirtAddr(requested_va, length, 0);
	if (!va) return MAP_FAILED;

	BOOL mapped = OSMapMemory(va, OSEffectiveToPhysical((uint32_t)backing_mem),
	                          length, OS_MAP_MEMORY_READ_WRITE);
	if (!mapped) {
		OSFreeVirtAddr(va, length);
		return MAP_FAILED;
	}

	return (void*)va;
}

static void wiiu_unmap(void* va, size_t length) {
	if (va == MAP_FAILED) return;
	OSUnmapMemory((uint32_t)va, length);
	OSFreeVirtAddr((uint32_t)va, length);
}

static void* psx_mem;
static void* psx_parallel;
static void* psx_scratch;
static void* psx_bios;

int lightrec_init_mmap(void) {
	psx_mem      = memalign(OS_PAGE_SIZE, 0x200000);
	psx_parallel = memalign(OS_PAGE_SIZE, 0x10000);
	psx_scratch  = memalign(OS_PAGE_SIZE, 0x10000);
	psx_bios     = memalign(OS_PAGE_SIZE, 0x80000);
	if (!psx_mem || !psx_parallel || !psx_scratch || !psx_bios)
		goto cleanup_allocations;

	uint32_t avail_va;
	uint32_t avail_va_size;
	OSGetMapVirtAddrRange(&avail_va, &avail_va_size);
	if (!avail_va || avail_va_size < 0x20000000)
		goto cleanup_allocations;

	// Map 4x ram mirrors
	int i;
	for (i = 0; i < 4; i++) {
		void* ret = wiiu_mmap(avail_va + 0x200000 * i, 0x200000, psx_mem);
		if (ret == MAP_FAILED) break;
	}
	if (i != 4) {
		for (int i = 0; i < 4; i++)
			wiiu_unmap(avail_va + 0x200000 * i, 0x200000);
		goto cleanup_allocations;
	}
	psxM = (void*)avail_va;

	psxP = wiiu_mmap(avail_va + 0x1f000000, 0x10000, psx_parallel);
	psxH = wiiu_mmap(avail_va + 0x1f800000, 0x10000, psx_scratch);
	psxR = wiiu_mmap(avail_va + 0x1fc00000, 0x80000, psx_bios);

	if (psxP == MAP_FAILED || psxH == MAP_FAILED || psxR == MAP_FAILED) {
		for (int i = 0; i < 4; i++)
			wiiu_unmap(psxM + 0x200000 * i, 0x200000);
		wiiu_unmap(psxP, 0x10000);
		wiiu_unmap(psxH, 0x10000);
		wiiu_unmap(psxR, 0x80000);
		goto cleanup_allocations;
	}

	code_buffer = WUP_RWX_MEM_BASE;

	return 0;

cleanup_allocations:
	free(psx_mem);
	free(psx_parallel);
	free(psx_scratch);
	free(psx_bios);
	return -1;
}

void lightrec_free_mmap(void) {
	for (int i = 0; i < 4; i++)
		wiiu_unmap(psxM + 0x200000 * i, 0x200000);
	wiiu_unmap(psxP, 0x10000);
	wiiu_unmap(psxH, 0x10000);
	wiiu_unmap(psxR, 0x80000);
	free(psx_mem);
	free(psx_parallel);
	free(psx_scratch);
	free(psx_bios);
}

void lightrec_code_inv(void *ptr, uint32_t len)
{
	wiiu_clear_cache(ptr, (void *)((uintptr_t)ptr + len));
}
