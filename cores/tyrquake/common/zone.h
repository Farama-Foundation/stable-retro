/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef ZONE_H
#define ZONE_H

/*
 memory allocation


H_??? The hunk manages the entire memory block given to quake.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.


Z_??? Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object


Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.


------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

Zone block

----- Bottom of Memory -----



*/

void Memory_Init(void *buf, int size);

void Z_Free(void *ptr);
void *Z_Malloc(int size);	/* returns 0 filled memory */
void *Z_Realloc(void *ptr, int size);

void *Hunk_Alloc(int size);	/* returns 0 filled memory */

void *Hunk_HighAlloc(int size);

int Hunk_LowMark(void);
void Hunk_FreeToLowMark(int mark);

int Hunk_HighMark(void);
void Hunk_FreeToHighMark(int mark);

void *Hunk_TempAlloc(int size);
void *Hunk_TempAllocExtend(int size);

void Hunk_Check(void);

/*
 * Hunk_PointerInHunk
 * - returns true iff p falls within the malloc'd backing
 *   region of the hunk allocator (i.e. inside [hunk_base,
 *   hunk_base + hunk_size)).  Useful as a sanity gate before
 *   dereferencing pointers of uncertain provenance --
 *   notably qpic_t* values stored in long-lived globals like
 *   sb_nums[][] which can hold stale pointers across content
 *   reloads.
 */
int Hunk_PointerInHunk(const void *p);

/*
 * Z_CheckHeap
 * - walks the zone block list and verifies every block's id
 *   field, the next/prev pointers, and the trailing
 *   trash-tester word written by Z_TagMalloc.  Sys_Errors on
 *   the first inconsistency.  Cheap enough to call from a
 *   console command for live debugging of heap stomps.
 */
void Z_CheckHeap(void);

typedef struct cache_user_s {
    void *data;
    int pad;
} cache_user_t;

void Cache_Flush(void);

/*
 * Cache_Check
 * - returns the cached data + saved offset, and moves to the head of
 *   the LRU list if present, otherwise returns NULL
 */
void *Cache_Check(const cache_user_t *c);

/*
 * Cache_Alloc
 * - Returns NULL if all purgable data was tossed and there still
 *   wasn't enough room. Otherwise returns a pointer to the cached
 *   data requested.
 */
void *Cache_Alloc(cache_user_t *c, int size);

/*
 * Cache_AllocPadded
 * - Same as Cache_Alloc, but pad the allocation with space before the returned
 *   pointer for extra data to be accessed via e.g. container_of(x).
 *
 */
void *Cache_AllocPadded(cache_user_t *c, int pad, int size);

void Cache_Free(cache_user_t *c);

void Cache_Report(void);

/*
 * Cache_SetInvalidateCallback
 *
 * Register a callback invoked just before a cache payload becomes
 * invalid (Cache_Free, or the Cache_Free inside Cache_Move's relocate-
 * then-discard-old path).  The callback receives the OLD payload
 * pointer and its byte size; the renderer uses this to drop any
 * external references it has cached on that address (Vulkan overlay-
 * slot cache, GL texture pointer caches, etc.).
 *
 * The callback fires from a single-threaded context (the libretro
 * core's main thread); the renderer can synchronously update its
 * own bookkeeping.  Passing NULL clears the callback.
 *
 * Decoupling the cache from any specific renderer keeps zone.c
 * renderer-agnostic; whichever backend rhi_init stands up registers
 * its hook right after backend init succeeds.
 */
typedef void (*cache_invalidate_cb_t)(const void *data, int size);
void Cache_SetInvalidateCallback(cache_invalidate_cb_t cb);

#endif /* ZONE_H */
