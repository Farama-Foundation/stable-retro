// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2022 Paul Cercueil <paul@crapouillou.net>
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "../psxhw.h"
#include "../psxmem.h"
#include "../r3000a.h"

#include "mem.h"

#define ARRAY_SIZE(a) (sizeof(a) ? (sizeof(a) / sizeof((a)[0])) : 0)

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#ifndef MFD_HUGETLB
#define MFD_HUGETLB 0x0004
#endif

static const uintptr_t supported_io_bases[] = {
	0x0,
	0x10000000,
	0x40000000,
	0x80000000,
};

static void * mmap_huge(void *addr, size_t length, int prot, int flags,
			int fd, off_t offset)
{
	void *map = MAP_FAILED;

	if (length >= 0x200000) {
		map = mmap(addr, length, prot,
			   flags | MAP_HUGETLB | (21 << MAP_HUGE_SHIFT),
			   fd, offset);
		if (map != MAP_FAILED)
			printf("Hugetlb mmap to address 0x%" PRIxPTR " succeeded\n",
			       (uintptr_t) addr);
	}

	if (map == MAP_FAILED) {
		map = mmap(addr, length, prot, flags, fd, offset);
		if (map != MAP_FAILED) {
			printf("Regular mmap to address 0x%" PRIxPTR " succeeded\n",
			       (uintptr_t) addr);
#ifdef MADV_HUGEPAGE
			madvise(map, length, MADV_HUGEPAGE);
#endif
		}
	}

	return map;
}

static int lightrec_mmap_ram(bool hugetlb)
{
	unsigned int i, j;
	int err, memfd, flags = 0;
	uintptr_t base;
	void *map;

	if (hugetlb)
		flags |= MFD_HUGETLB;

        memfd = syscall(SYS_memfd_create, "/lightrec_memfd",
			flags);
	if (memfd < 0) {
		SysMessage("Failed to create memfd: %d", errno);
		err = -errno;
		return err;
	}

	err = ftruncate(memfd, 0x200000);
	if (err < 0) {
		SysMessage("Could not trim memfd: %d", errno);
		err = -errno;
		goto err_close_memfd;
	}

	for (i = 0; i < ARRAY_SIZE(supported_io_bases); i++) {
		base = supported_io_bases[i];

		for (j = 0; j < 4; j++) {
			void *base_ptr = (void *)(base + j * 0x200000);
			map = mmap_huge(base_ptr, 0x200000, PROT_READ | PROT_WRITE,
					MAP_SHARED | MAP_FIXED_NOREPLACE, memfd, 0);
			if (map == MAP_FAILED)
				break;
			// some systems ignore MAP_FIXED_NOREPLACE
			if (map != base_ptr) {
				munmap(map, 0x200000);
				break;
			}
		}

		/* Impossible to map using this base */
		if (j == 0)
			continue;

		/* All mirrors mapped - we got a match! */
		if (j == 4)
			break;

		/* Only some mirrors mapped - clean the mess and try again */
		for (; j > 0; j--)
			munmap((void *)(base + (j - 1) * 0x200000), 0x200000);
	}

	if (i == ARRAY_SIZE(supported_io_bases)) {
		err = -EINVAL;
		goto err_close_memfd;
	}

	err = 0;
	psxM = (s8 *)base;

err_close_memfd:
	close(memfd);
	return err;
}

int lightrec_init_mmap(void)
{
	unsigned int i;
	s8 *base, *target;
	void *map;
	int err = lightrec_mmap_ram(true);
	if (err) {
		err = lightrec_mmap_ram(false);
		if (err) {
			SysMessage("Unable to mmap RAM and mirrors");
			return err;
		}
	}

	base = psxM;

	target = base + 0x1f000000;
	map = mmap(target, 0x10000,
		   PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | /*MAP_FIXED_NOREPLACE |*/ MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		SysMessage("Unable to mmap parallel port: %d", errno);
		err = -EINVAL;
		goto err_unmap;
	}
	if (map != target)
		SysMessage("lightrec: mapped parallel port at %p, wanted %p", map, target);

	psxP = (s8 *)map;

	target = base + 0x1fc00000;
	map = mmap_huge(target, 0x200000,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | /*MAP_FIXED_NOREPLACE |*/ MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		SysMessage("Unable to mmap BIOS: %d", errno);
		err = -EINVAL;
		goto err_unmap_parallel;
	}
	if (map != target)
		SysMessage("lightrec: mapped bios at %p, wanted %p", map, target);

	psxR = (s8 *)map;

	target = base + 0x1f800000;
	map = mmap(target, 0x10000,
		   PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | /*MAP_FIXED_NOREPLACE |*/ MAP_ANONYMOUS, 0, 0);
	if (map == MAP_FAILED) {
		SysMessage("Unable to mmap scratchpad: %d", errno);
		err = -EINVAL;
		goto err_unmap_bios;
	}
	if (map != target)
		SysMessage("lightrec: mapped scratchpad at %p, wanted %p", map, target);

	psxH = (s8 *)map;

	target = base + 0x800000;
	map = mmap_huge(target, CODE_BUFFER_SIZE,
			PROT_EXEC | PROT_READ | PROT_WRITE,
			MAP_PRIVATE | /*MAP_FIXED_NOREPLACE |*/ MAP_ANONYMOUS,
			-1, 0);
	if (map == MAP_FAILED) {
		SysMessage("Unable to mmap code buffer: %d", errno);
		err = -EINVAL;
		goto err_unmap_scratch;
	}
	if (map != target)
		SysMessage("lightrec: mapped code at %p, wanted %p", map, target);

	code_buffer = map;

	return 0;

err_unmap_scratch:
	munmap(psxH, 0x10000);
err_unmap_bios:
	munmap(psxR, 0x200000);
err_unmap_parallel:
	munmap(psxP, 0x10000);
err_unmap:
	for (i = 0; i < 4; i++)
		munmap((void *)((uintptr_t)psxM + i * 0x200000), 0x200000);
	return err;
}

void lightrec_free_mmap(void)
{
	unsigned int i;

	munmap(code_buffer, CODE_BUFFER_SIZE);
	munmap(psxH, 0x10000);
	munmap(psxR, 0x200000);
	munmap(psxP, 0x10000);
	for (i = 0; i < 4; i++)
		munmap((void *)((uintptr_t)psxM + i * 0x200000), 0x200000);
}
