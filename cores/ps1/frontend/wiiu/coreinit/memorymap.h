//SPDX-License-Identifier: GPL-2.0-or-later
/* From wut:
 * https://github.com/devkitPro/wut/blob/0b196e8abcedeb0238105f3ffab7cb0093638b86/include/coreinit/memorymap.h
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef bool BOOL;

/**
 * \defgroup coreinit_memorymap Memory Map
 * \ingroup coreinit
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OSMemoryMapMode
{
   OS_MAP_MEMORY_INVALID      = 0,
   OS_MAP_MEMORY_READ_ONLY    = 1,
   OS_MAP_MEMORY_READ_WRITE   = 2,
   OS_MAP_MEMORY_FREE         = 3,
   OS_MAP_MEMORY_ALLOCATED    = 4,
} OSMemoryMapMode;

#define OS_PAGE_SIZE (128 * 1024)

uint32_t
OSEffectiveToPhysical(uint32_t virtualAddress);

BOOL
OSIsAddressValid(uint32_t virtualAddress);

BOOL
__OSValidateAddressSpaceRange(int /* unused */,
                              uint32_t virtualAddress,
                              uint32_t size);

/**
 * Allocates virtual address range for later mapping.
 *
 * \param virtualAddress
 * Requested start address for the range. If there is no preference, NULL can be
 * used.
 *
 * \param size
 * Size of address range to allocate.
 *
 * \param align
 * Alignment of address range to allocate.
 *
 * \return
 * The starting address of the newly allocated range, or NULL on failure.
 *
 * \sa
 * - OSFreeVirtAddr()
 * - OSMapMemory()
 */
uint32_t
OSAllocVirtAddr(uint32_t virtualAddress,
                uint32_t size,
                uint32_t align);

/**
 * Frees a previously allocated virtual address range back to the system.
 *
 * \param virtualAddress
 * The start of the virtual address range to free.
 *
 * \param size
 * The size of the virtual address range to free.
 *
 * \return
 * \c true on success.
 */
BOOL
OSFreeVirtAddr(uint32_t virtualAddress,
               uint32_t size);

/**
 * Determines the status of the given virtual memory address - mapped read-write
 * or read-only, free, allocated or invalid.
 *
 * \param virtualAddress
 * The virtual address to query.
 *
 * \return
 * The status of the memory address - see #OSMemoryMapMode.
 */
OSMemoryMapMode
OSQueryVirtAddr(uint32_t virtualAddress);

/**
 * Maps a physical address to a virtual address, with a given size and set of
 * permissions.
 *
 * \param virtualAddress
 * The target virtual address for the mapping.
 *
 * \param physicalAddress
 * Physical address of the memory to back the mapping.
 *
 * \param size
 * Size, in bytes, of the desired mapping. Likely has an alignment requirement.
 *
 * \param mode
 * Permissions to map the memory with - see #OSMemoryMapMode.
 *
 * \return
 * \c true on success.
 *
 * \sa
 * - OSAllocVirtAddr()
 * - OSUnmapMemory()
 */
BOOL
OSMapMemory(uint32_t virtualAddress,
            uint32_t physicalAddress,
            uint32_t size,
            OSMemoryMapMode mode);

/**
 * Unmaps previously mapped memory.
 *
 * \param virtualAddress
 * Starting address of the area to unmap.
 *
 * \param size
 * Size of the memory area to unmap.
 *
 * \return
 * \c true on success.
 */
BOOL
OSUnmapMemory(uint32_t virtualAddress,
              uint32_t size);

/**
 * Gets the range of virtual addresses available for mapping.
 *
 * \param outVirtualAddress
 * Pointer to write the starting address of the memory area to.
 *
 * \param outSize
 * Pointer to write the size of the memory area to.
 *
 * \sa
 * - OSMapMemory()
 */
void
OSGetMapVirtAddrRange(uint32_t *outVirtualAddress,
                      uint32_t *outSize);

/**
 * Gets the range of available physical memory (not reserved for app code or
 * data).
 *
 * \param outPhysicalAddress
 * Pointer to write the starting physical address of the memory area to.
 *
 * \param outSize
 * Pointer to write the size of the memory area to.
 *
 * \if false
 * Is memory returned by this function actually safe to map and use? couldn't
 * get a straight answer from decaf-emu's kernel_memory.cpp...
 * \endif
 */
void
OSGetAvailPhysAddrRange(uint32_t *outPhysicalAddress,
                        uint32_t *outSize);

/**
 * Gets the range of physical memory used for the application's data.
 *
 * \param outPhysicalAddress
 * Pointer to write the starting physical address of the memory area to.
 *
 * \param outSize
 * Pointer to write the size of the memory area to.
 *
 * \if false
 * does this include the main heap?
 * \endif
 */
void
OSGetDataPhysAddrRange(uint32_t *outPhysicalAddress,
                       uint32_t *outSize);

#ifdef __cplusplus
}
#endif

/** @} */
