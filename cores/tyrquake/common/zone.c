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

#include "cmd.h"
#include "common.h"
#include "console.h"
#include "mathlib.h"
#include "quakedef.h"
#include "sys.h"
#include "zone.h"

#define	DYNAMIC_SIZE	0x40000		/* 256k */
#define	ZONEID		0x1d4a11
#define MINFRAGMENT	64

typedef struct memblock_s
{
    int size;		/* including the header and possibly tiny fragments */
    int tag;		/* a tag of 0 is a free block */
    int id;		/* should be ZONEID */
    int pad;		/* pad to 64 bit boundary */
    struct memblock_s *next, *prev;
} memblock_t;

typedef struct
{
    int size;			/* total bytes malloced, including header */
    memblock_t blocklist;	/* start/end cap for linked list */
    memblock_t *rover;
} memzone_t;

static void Cache_FreeLow(int new_low_hunk);
static void Cache_FreeHigh(int new_high_hunk);

/*
 * ============================================================================
 *
 * ZONE MEMORY ALLOCATION
 *
 * There is never any space between memblocks, and there will never be two
 * contiguous free memblocks.
 *
 * The rover can be left pointing at a non-empty block
 *
 * The zone calls are pretty much only used for small strings and structures,
 * all big things are allocated on the hunk.
 * ============================================================================
 */

static memzone_t *mainzone;

static void Z_ClearZone(memzone_t *zone, int size);


/*
 * ========================
 * Z_ClearZone
 * ========================
 */
static void
Z_ClearZone(memzone_t *zone, int size)
{
    memblock_t *block;

    /*
     * set the entire zone to one free block
     */
    zone->blocklist.next = zone->blocklist.prev = block =
	(memblock_t *)((byte *)zone + sizeof(memzone_t));
    zone->blocklist.tag  = 1;	/* in use block */
    zone->blocklist.id   = 0;
    zone->blocklist.size = 0;
    zone->rover          = block;

    block->prev          = block->next = &zone->blocklist;
    block->tag           = 0;		/* free block */
    block->id            = ZONEID;
    block->size          = size - sizeof(memzone_t);
}


/*
 * ========================
 * Z_CheckHeap
 *
 * Walk every block in the zone and verify:
 *   - block->id == ZONEID
 *   - block->next->prev == block (back-link integrity)
 *   - no two consecutive free blocks (allocator invariant)
 *   - allocated blocks have their trailing trash-tester word
 *     intact (set by Z_TagMalloc near "marker for memory trash
 *     testing").
 *
 * The trash-tester check catches the most common stomp shape:
 * a strcpy/strcat that walks a few bytes past the end of an
 * allocated region and overwrites the start of the next
 * block's header.  Without this check such corruption is
 * silent until a much later allocator traversal trips far
 * from the culprit.
 *
 * Cheap enough to invoke from a console command; on a 16 MB
 * zone with a few thousand blocks this is a few microseconds.
 * ========================
 */
void
Z_CheckHeap(void)
{
    memblock_t *block;

    if (!mainzone)
        return;

    for (block = mainzone->blocklist.next; ; block = block->next) {
        if (block->next == &mainzone->blocklist)
            break;          /* all blocks checked */
        if (block->id != ZONEID)
            Sys_Error("%s: block id != ZONEID", __func__);
        if (block->next->prev != block)
            Sys_Error("%s: next block doesn't have proper back link",
                      __func__);
        if (!block->tag && !block->next->tag)
            Sys_Error("%s: two consecutive free blocks", __func__);
        if (block->tag) {
            int *trash = (int *)((byte *)block + block->size - 4);
            if (*trash != ZONEID)
                Sys_Error("%s: trashed trash-tester at end of block "
                          "(tag=%i, size=%i)",
                          __func__, block->tag, block->size);
        }
    }
}


/*
 * ========================
 * Z_Free
 * ========================
 */
void
Z_Free(void *ptr)
{
   memblock_t *block, *other;

   if (!ptr)
      Sys_Error("%s: NULL pointer", __func__);

   block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));
   if (block->id != ZONEID)
      Sys_Error("%s: freed a pointer without ZONEID", __func__);
   if (block->tag == 0)
      Sys_Error("%s: freed a freed pointer", __func__);

   block->tag = 0;		/* mark as free */

   other = block->prev;
   if (!other->tag)
   {
      /* merge with previous free block */
      other->size += block->size;
      other->next = block->next;
      other->next->prev = other;
      if (block == mainzone->rover)
         mainzone->rover = other;
      block = other;
   }

   other = block->next;
   if (!other->tag)
   {
      /* merge the next free block onto the end */
      block->size += other->size;
      block->next = other->next;
      block->next->prev = block;
      if (other == mainzone->rover)
         mainzone->rover = block;
   }

    /*
     * Always start looking from the first available free block.
     * Slower, but not too bad and we don't fragment nearly as much.
     */
    if (block < mainzone->rover) {
	mainzone->rover = block;
    }
}

static void *Z_TagMalloc(int size, int tag)
{
   int extra;
   memblock_t *start, *rover, *newobj, *base;

   if (!tag)
      Sys_Error("%s: tried to use a 0 tag", __func__);

   /* Reject callers that pass a non-positive size or a size so
    * large that the +sizeof(memblock_t)+4 alignment math below
    * would overflow into a negative int.  The free-list scan
    * compares block->size (positive) against this size; with
    * a wrapped-negative size every free block looks "big enough"
    * and we hand back a pointer to whatever the rover lands on,
    * with nominal size 0.  Subsequent caller writes corrupt
    * adjacent allocations. */
   if (size <= 0
       || size > INT_MAX - (int)sizeof(memblock_t) - 4 - 7)
      Sys_Error("%s: bad size %i", __func__, size);

   /*
    * Scan through the block list looking for the first free block of
    * sufficient size
    */
   size += sizeof(memblock_t);	/* account for size of block header */
   size += 4;			/* space for memory trash tester */
   size = (size + 7) & ~7;	/* align to 8-byte boundary */

   /* If we ended on an allocated block, skip forward to the first free block */
    start = mainzone->rover->prev;
    while (mainzone->rover->tag && mainzone->rover != start)
	mainzone->rover = mainzone->rover->next;
 
    base = rover = mainzone->rover;

   do {
      if (rover == start)	/* scaned all the way around the list */
         return NULL;
      if (rover->tag)
         base = rover = rover->next;
      else
         rover = rover->next;
   } while (base->tag || base->size < size);

   /* found a block big enough */
   extra = base->size - size;
   if (extra > MINFRAGMENT)
   {
      /* there will be a free fragment after the allocated block */
      newobj = (memblock_t *)((byte *)base + size);
      newobj->size = extra;
      newobj->tag = 0;		/* free block */
      newobj->prev = base;
      newobj->id = ZONEID;
      newobj->next = base->next;
      newobj->next->prev = newobj;
      base->next = newobj;
      base->size = size;
   }

   base->tag = tag;		   /* no longer a free block */

   /*
     * If we just allocated the first available block, the next
     * allocation starts looking after this one.
     */
    if (base == mainzone->rover)
	mainzone->rover = base->next;

   base->id = ZONEID;

   /* marker for memory trash testing */
   *(int *)((byte *)base + base->size - 4) = ZONEID;

   return (void *)((byte *)base + sizeof(memblock_t));
}


/*
 * ========================
 * Z_Malloc
 * ========================
 */
void * Z_Malloc(int size)
{
   void *buf = Z_TagMalloc(size, 1);
   if (!buf)
      Sys_Error("%s: failed on allocation of %i bytes", __func__, size);
   memset(buf, 0, size);

   return buf;
}

/*
 * ========================
 * Z_Realloc
 * ========================
 */
void *Z_Realloc(void *ptr, int size)
{
   memblock_t *block;
   int orig_size;
   void *ret;

   if (!ptr)
      return Z_Malloc(size);

   /* POSIX realloc(p, 0) is implementation-defined since C17.
    * Quake's allocator never asked for size 0 historically;
    * treat it as a request to shrink to a minimum block.
    * Don't free + return NULL because callers do not check
    * for NULL after Z_Realloc. */
   if (size <= 0)
      size = 1;

   block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));
   if (block->id != ZONEID)
      Sys_Error("%s: realloced a pointer without ZONEID", __func__);
   if (!block->tag)
      Sys_Error("%s: realloced a freed pointer", __func__);

   orig_size = block->size;
   orig_size -= sizeof(memblock_t);
   orig_size -= 4;

   /* The original implementation freed ptr first, then alloc'd a
    * new block, then memmove'd from the (already-freed) ptr.
    * That's a use-after-free: between Z_Free and Z_TagMalloc the
    * block headers are coalesced (Z_Free overwrites the freed
    * block's prev/next/size links with the merged-block layout),
    * and Z_TagMalloc itself can split the same region we just
    * freed and write a new memblock_t header partway through it.
    * Either path corrupts the source bytes before memmove reads
    * them.
    *
    * Allocate first, copy, then free.  In the size-shrink case
    * this slightly increases peak footprint; in exchange we no
    * longer read garbage when the shrink happens to be in a
    * region the new alloc reuses. */
   ret = Z_TagMalloc(size, 1);
   if (!ret)
      Sys_Error("%s: failed on allocation of %i bytes", __func__, size);
   if (ret != ptr)
      memmove(ret, ptr, qmin(orig_size, size));
   Z_Free(ptr);

   return ret;
}

/* ======================================================================= */

#define	HUNK_SENTINAL	0x1df001ed

#define HUNK_NAMELEN	8

/* sizeof(hunk_t) must be a multiple of 16 so that the pointer returned
 * to callers (h + 1) preserves the 16-byte alignment promised in zone.h
 * and required by SIMD code paths. */
typedef struct
{
   int sentinal;
   int size;		/* including sizeof(hunk_t), -1 = not allocated */
   int pad[2];
} hunk_t;

static byte *hunk_base;
static int hunk_size;

static int hunk_low_used;
static int hunk_high_used;

static qboolean hunk_tempactive;
static int hunk_tempmark;

/*
 * ==============
 * Hunk_Check
 *
 * Run consistancy and sentinal trashing checks
 * ==============
 */
void Hunk_Check(void)
{
   hunk_t *h;

   for (h = (hunk_t *)hunk_base; (byte *)h != hunk_base + hunk_low_used;)
   {
      if (h->sentinal != HUNK_SENTINAL)
         Sys_Error("%s: trashed sentinal", __func__);
      /* h->size includes sizeof(hunk_t) and is rounded up to a
       * 16-byte multiple (see Hunk_Alloc).  A zero or negative
       * size, or a size large enough that h + size walks past
       * hunk_low_used, indicates a stomped header -- diagnose
       * loudly rather than infinite-looping. */
      if (h->size < (int)sizeof(hunk_t) ||
            h->size + (byte *)h - hunk_base > hunk_size)
         Sys_Error("%s: bad size", __func__);
      h = (hunk_t *)((byte *)h + h->size);
   }
}

/*
 * ==============
 * Hunk_PointerInHunk
 *
 * Returns true iff p falls within the malloc'd backing
 * region of the hunk allocator.  Used as a sanity gate by
 * subsystems that hold long-lived qpic_t* pointers in
 * globals (notably sb_nums[][] in sbar.c) where stale or
 * stomped values have been observed crashing the renderer.
 * ==============
 */
int Hunk_PointerInHunk(const void *p)
{
   const byte *q = (const byte *)p;
   if (!p || !hunk_base)
      return 0;
   if (q < hunk_base || q >= hunk_base + hunk_size)
      return 0;
   return 1;
}

/*
 * ===================
 * Hunk_Alloc
 * ===================
 */
void *Hunk_Alloc(int size)
{
   hunk_t *h;

   /* Reject negative sizes, and sizes large enough that
    * sizeof(hunk_t) + ((size + 15) & ~15) would overflow int.
    * Without the upper-bound check, a wrapped-negative size
    * passes the (hunk_size - ... < size) guard (because
    * negative < any positive), then hunk_low_used += size
    * walks backward and the next allocation overlaps the
    * previous one. */
   if (size < 0)
      Sys_Error("%s: bad size: %i", __func__, size);
   if (size > INT_MAX - (int)sizeof(hunk_t) - 15)
      Sys_Error("%s: size %i too large", __func__, size);

   size = sizeof(hunk_t) + ((size + 15) & ~15);

   if (hunk_size - hunk_low_used - hunk_high_used < size)
   {
      Sys_Error ("%s: failed on %i bytes", __func__, size);
   }

   h = (hunk_t *)(hunk_base + hunk_low_used);
   hunk_low_used += size;

   Cache_FreeLow(hunk_low_used);

   memset(h, 0, size);

   h->size = size;
   h->sentinal = HUNK_SENTINAL;

   return (void *)(h + 1);
}

int Hunk_LowMark(void)
{
   return hunk_low_used;
}

void Hunk_FreeToLowMark(int mark)
{
   if (mark < 0 || mark > hunk_low_used)
      Sys_Error("%s: bad mark %i", __func__, mark);
   memset(hunk_base + mark, 0, hunk_low_used - mark);
   hunk_low_used = mark;
}

int Hunk_HighMark(void)
{
   if (hunk_tempactive)
   {
      hunk_tempactive = false;
      Hunk_FreeToHighMark(hunk_tempmark);
   }

   return hunk_high_used;
}

void Hunk_FreeToHighMark(int mark)
{
   if (hunk_tempactive)
   {
      hunk_tempactive = false;
      Hunk_FreeToHighMark(hunk_tempmark);
   }
   if (mark < 0 || mark > hunk_high_used)
      Sys_Error("%s: bad mark %i", __func__, mark);
   memset(hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
   hunk_high_used = mark;
}


/*
 * ===================
 * Hunk_HighAlloc
 * ===================
 */
void *Hunk_HighAlloc(int size)
{
   hunk_t *h;

   if (size < 0)
      Sys_Error("%s: bad size: %i", __func__, size);
   if (size > INT_MAX - (int)sizeof(hunk_t) - 15)
      Sys_Error("%s: size %i too large", __func__, size);

   if (hunk_tempactive)
   {
      Hunk_FreeToHighMark(hunk_tempmark);
      hunk_tempactive = false;
   }

   size = sizeof(hunk_t) + ((size + 15) & ~15);

   if (hunk_size - hunk_low_used - hunk_high_used < size)
   {
      Con_Printf("Hunk_HighAlloc: failed on %i bytes\n", size);
      return NULL;
   }

   hunk_high_used += size;
   Cache_FreeHigh(hunk_high_used);

   h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);

   memset(h, 0, size);
   h->size = size;
   h->sentinal = HUNK_SENTINAL;

   return (void *)(h + 1);
}


/*
 * =================
 * Hunk_TempAlloc
 *
 * Return space from the top of the hunk
 * =================
 */
void *Hunk_TempAlloc(int size)
{
   void *buf;

   if (size < 0)
      Sys_Error("%s: bad size: %i", __func__, size);
   if (size > INT_MAX - 15)
      Sys_Error("%s: size %i too large", __func__, size);

   size = (size + 15) & ~15;

   if (hunk_tempactive)
   {
      Hunk_FreeToHighMark(hunk_tempmark);
      hunk_tempactive = false;
   }

   hunk_tempmark = Hunk_HighMark();

   buf = Hunk_HighAlloc(size);

   hunk_tempactive = true;

   return buf;
}

/*
 * =====================
 * Hunk_TempAllocExtend
 *
 * Grow the active temp hunk allocation by `size` more bytes
 * and return a pointer to the start of the newly-added
 * region.  The original temp data remains valid through any
 * pointers the caller already holds: the high hunk grows
 * downward (toward the low hunk), so the new region lands
 * IMMEDIATELY BELOW the original allocation in memory, not
 * at the same address with extended size.
 *
 * Layout after extend (high hunk grows downward):
 *   [hunk_high_used grew by `size`]
 *   newobj header   <-- returned pointer is newobj+1
 *   newly-added region (size bytes, uninitialised)
 *   ... old user data (still at original addresses) ...
 *   ... rest of hunk ...
 *
 * STree_AllocNode / STree_AllocString are the only callers;
 * they treat the returned pointer as a fresh chunk and never
 * reach back into the old region through it.
 *
 * Returns NULL if there isn't enough free hunk space.
 * =====================
 */
void *Hunk_TempAllocExtend(int size)
{
   hunk_t *old, *newobj;

   if (!hunk_tempactive)
      Sys_Error("%s: temp hunk not active", __func__);
   if (size < 0)
      Sys_Error("%s: bad size: %i", __func__, size);
   if (size > INT_MAX - 15)
      Sys_Error("%s: size %i too large", __func__, size);

   old = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);

   if (old->sentinal != HUNK_SENTINAL)
      Sys_Error("%s: old sentinal trashed\n", __func__);

   size = (size + 15) & ~15;
   if (hunk_size - hunk_low_used - hunk_high_used < size) {
      Con_Printf("%s: failed on %i bytes\n", __func__, size);
      return NULL;
   }

   hunk_high_used += size;
   Cache_FreeHigh(hunk_high_used);

   newobj = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
   memmove(newobj, old, sizeof(hunk_t));
   newobj->size += size;

   return (void *)(newobj + 1);
}

/*
 * ===========================================================================
 *
 * CACHE MEMORY
 *
 * ===========================================================================
 */

#define CACHE_NAMELEN 32
#define CACHE_SENTINAL 0x10ad1ed5    /* "loadied5" — distinct from HUNK_SENTINAL */

typedef struct cache_system_s
{
   int sentinal;		/* trash detector; see Cache_CheckSentinal */
   int size;			/* including this header */
   cache_user_t *user;
   struct cache_system_s *prev, *next;
   struct cache_system_s *lru_prev, *lru_next;	/* for LRU flushing */
} cache_system_t;

static cache_system_t cache_head;

/* Phase 5b-06 follow-up: optional callback fired right before any
 * cache payload becomes invalid (Cache_Free, or the Cache_Free
 * inside Cache_Move's relocate-then-discard-old path).  Registered
 * by the renderer's backend init via Cache_SetInvalidateCallback;
 * NULL means "no consumer" and the cache notify path is a no-op. */
static cache_invalidate_cb_t cache_invalidate_cb;
static cache_system_t *Cache_TryAlloc(int size, qboolean nobottom);

/*
 * Cache headers live in the middle of the hunk between the
 * low and high marks.  A bug or hostile content that stomps
 * a header would normally surface as a wild pointer deref
 * during the linked-list walk in Cache_TryAlloc / Cache_Free
 * Low / Cache_FreeHigh, or a wrong-size memcpy in
 * Cache_Move -- the cache walks cs->size and cs->next
 * unconditionally.  Validate the sentinal at each handoff
 * to convert silent corruption into a named error.
 *
 * Mirrors the HUNK_SENTINAL pattern in Hunk_Check / Hunk_
 * TempAllocExtend.
 */
static INLINE void Cache_CheckSentinal(const cache_system_t *cs,
                                       const char *where)
{
   if (cs->sentinal != CACHE_SENTINAL)
      Sys_Error("%s: trashed cache sentinal", where);
}

static INLINE cache_system_t *Cache_System(const cache_user_t *c)
{
   return (cache_system_t *)((byte *)c->data - c->pad) - 1;
}

static INLINE void *Cache_Data(const cache_system_t *c)
{
   return (byte *)(c + 1) + c->user->pad;
}

/*
 * ===========
 * Cache_Move
 * ===========
 */
static void Cache_Move(cache_system_t *c)
{
   /* we are clearing up space at the bottom, so only allocate it late */
   cache_system_t *newobj;

   Cache_CheckSentinal(c, __func__);
   newobj = Cache_TryAlloc(c->size, true);

   if (newobj)
   {
      int pad;
      memcpy(newobj + 1, c + 1, c->size - sizeof(cache_system_t));
      newobj->user = c->user;
      pad = c->user->pad;
      Cache_Free(c->user);
      newobj->user->pad = pad;
      newobj->user->data = Cache_Data(newobj);
   }
   else
   {
      /* tough luck... */
      Cache_Free(c->user);
   }
}

/*
 * ============
 * Cache_FreeLow
 *
 * Throw things out until the hunk can be expanded to the given point
 * ============
 */
static void Cache_FreeLow(int new_low_hunk)
{
   cache_system_t *c;

   while (1)
   {
      c = cache_head.next;
      if (c == &cache_head)
         return;		/* nothing in cache at all */
      Cache_CheckSentinal(c, __func__);
      if ((byte *)c >= hunk_base + new_low_hunk)
         return;		/* there is space to grow the hunk */
      Cache_Move(c);		/* reclaim the space */
   }
}

/*
 * ============
 * Cache_FreeHigh
 *
 * Throw things out until the hunk can be expanded to the given point
 * ============
 */
static void Cache_FreeHigh(int new_high_hunk)
{
   cache_system_t *c;
   cache_system_t *prev = NULL;

   while (1)
   {
      c = cache_head.prev;
      if (c == &cache_head)
         return;		/* nothing in cache at all */
      Cache_CheckSentinal(c, __func__);
      if ((byte *)c + c->size <= hunk_base + hunk_size - new_high_hunk)
         return;		/* there is space to grow the hunk */
      if (c == prev)
         Cache_Free(c->user);	/* didn't move out of the way */
      else
      {
         Cache_Move(c);	/* try to move it */
         prev = c;
      }
   }
}

static void Cache_UnlinkLRU(cache_system_t *cs)
{
   if (!cs->lru_next || !cs->lru_prev)
      Sys_Error("%s: NULL link", __func__);

   cs->lru_next->lru_prev = cs->lru_prev;
   cs->lru_prev->lru_next = cs->lru_next;

   cs->lru_prev = cs->lru_next = NULL;
}

static void Cache_MakeLRU(cache_system_t *cs)
{
   if (cs->lru_next || cs->lru_prev)
      Sys_Error("%s: active link", __func__);

   cache_head.lru_next->lru_prev = cs;
   cs->lru_next = cache_head.lru_next;
   cs->lru_prev = &cache_head;
   cache_head.lru_next = cs;
}

/*
 * ============
 * Cache_TryAlloc
 *
 * Looks for a free block of memory between the high and low hunk marks
 * Size should already include the header and padding
 * ============
 */
static cache_system_t *Cache_TryAlloc(int size, qboolean nobottom)
{
   cache_system_t *cs, *newobj;

   /* is the cache completely empty? */
   if (!nobottom && cache_head.prev == &cache_head)
   {
      if (hunk_size - hunk_high_used - hunk_low_used < size)
         Sys_Error("%s: %i is greater than free hunk", __func__, size);

      newobj = (cache_system_t *)(hunk_base + hunk_low_used);
      memset(newobj, 0, sizeof(*newobj));
      newobj->sentinal = CACHE_SENTINAL;
      newobj->size = size;

      cache_head.prev = cache_head.next = newobj;
      newobj->prev = newobj->next = &cache_head;

      Cache_MakeLRU(newobj);
      return newobj;
   }

   /* search from the bottom up for space */
   newobj = (cache_system_t *)(hunk_base + hunk_low_used);
   cs = cache_head.next;

   do
   {
      /* Sentinal-validate every cs we read cs->size /
       * cs->next from -- a stomped header here makes the
       * subsequent (byte *)cs + cs->size walk go anywhere. */
      Cache_CheckSentinal(cs, __func__);

      if (!nobottom || cs != cache_head.next)
      {
         if ((byte *)cs - (byte *)newobj >= size)
         {	/* found space */
            memset(newobj, 0, sizeof(*newobj));
            newobj->sentinal = CACHE_SENTINAL;
            newobj->size = size;

            newobj->next = cs;
            newobj->prev = cs->prev;
            cs->prev->next = newobj;
            cs->prev = newobj;

            Cache_MakeLRU(newobj);

            return newobj;
         }
      }

      /* continue looking */
      newobj = (cache_system_t *)((byte *)cs + cs->size);
      cs = cs->next;

   } while (cs != &cache_head);

   /* try to allocate one at the very end */
   if (hunk_base + hunk_size - hunk_high_used - (byte *)newobj >= size) {
      memset(newobj, 0, sizeof(*newobj));
      newobj->sentinal = CACHE_SENTINAL;
      newobj->size = size;

      newobj->next = &cache_head;
      newobj->prev = cache_head.prev;
      cache_head.prev->next = newobj;
      cache_head.prev = newobj;

      Cache_MakeLRU(newobj);

      return newobj;
   }

   return NULL;		/* couldn't allocate */
}

/*
 * ============
 * Cache_Flush
 *
 * Throw everything out, so new data will be demand cached
 * ============
 */
void Cache_Flush(void)
{
   while (cache_head.next != &cache_head)
      Cache_Free(cache_head.next->user);	/* reclaim the space */
}

/*
 * ============
 * Cache_Report
 * ============
 */
void Cache_Report(void)
{
   Con_DPrintf("%4.1f megabyte data cache\n",
         (hunk_size - hunk_high_used -
          hunk_low_used) / (float)(1024 * 1024));
}

/*
 * ============
 * Cache_Init
 * ============
 */
static void Cache_Init(void)
{
   cache_head.next = cache_head.prev = &cache_head;
   cache_head.lru_next = cache_head.lru_prev = &cache_head;

   Cmd_AddCommand ("flush", Cache_Flush);
}

/*
 * ==============
 * Cache_Free
 *
 * Frees the memory and removes it from the LRU list
 * ==============
 */
void Cache_Free(cache_user_t *c)
{
   cache_system_t *cs;
   int             payload;

   if (!c->data)
      Sys_Error("%s: not allocated", __func__);

   cs = Cache_System(c);
   Cache_CheckSentinal(cs, __func__);

   /* Notify external pointer-cache consumers (overlay slot
    * cache in the Vulkan backend, etc.) BEFORE we invalidate
    * the data pointer.  c->data is the live payload address
    * and cs->size - sizeof(cache_system_t) is the payload
    * extent; any cached pointer falling in this range is
    * about to dangle.  Phase 5b-06 follow-up: this is the
    * primary fix for the qplaque-stripes corruption -- the
    * 7e88887 dim-check downstream catches the post-hoc
    * collision, but invalidating on the source side means
    * the collision can't even arise.  payload may be zero
    * for a malformed entry; guard the call so a zero/
    * negative range doesn't reach the renderer. */
   payload = cs->size - (int)sizeof(cache_system_t);
   if (cache_invalidate_cb && payload > 0)
      cache_invalidate_cb(c->data, payload);

   /* Invalidate the sentinal before unlinking so any later
    * use-after-free that re-derefs this header trips the
    * check rather than getting at stale-but-plausible
    * pointers. */
   cs->sentinal = 0;
   cs->prev->next = cs->next;
   cs->next->prev = cs->prev;
   cs->next = cs->prev = NULL;

   c->pad = 0;
   c->data = NULL;

   Cache_UnlinkLRU(cs);
}

/*
 * ==============
 * Cache_SetInvalidateCallback
 * ==============
 */
void Cache_SetInvalidateCallback(cache_invalidate_cb_t cb)
{
   cache_invalidate_cb = cb;
}

/*
 * ==============
 * Cache_Check
 * ==============
 */
void *Cache_Check(const cache_user_t *c)
{
   cache_system_t *cs;

   if (!c->data)
      return NULL;

   cs = Cache_System(c);
   /* No sentinal check here -- Cache_Check is per-visible-
    * alias-model per frame (called from Mod_Extradata in
    * the renderer hot path).  Validation lives at the rare
    * sites that read cs->size / cs->next during a list
    * walk: Cache_TryAlloc, Cache_FreeLow, Cache_FreeHigh,
    * Cache_Move, Cache_Free.  A header that gets stomped
    * between two Cache_Check calls (with no intervening
    * reorganisation) will be caught at the next walk. */

   /* move to head of LRU */
   Cache_UnlinkLRU(cs);
   Cache_MakeLRU(cs);

   return c->data;
}


/*
 * ==============
 * Cache_Alloc
 * ==============
 */
void *Cache_Alloc(cache_user_t *c, int size)
{
   return Cache_AllocPadded(c, 0, size);
}

/*
 * ==============
 * Cache_AllocPadded
 * ==============
 */
void *Cache_AllocPadded(cache_user_t *c, int pad, int size)
{
   if (c->data)
      Sys_Error("%s: allready allocated", __func__);

   if (size <= 0)
      Sys_Error("%s: size %i", __func__, size);
   /* Reject sizes that would overflow the alignment math below.
    * pad is typically a small alignment fudge but isn't bounded
    * from above; reject negative pad explicitly and reject any
    * size + pad that would walk past INT_MAX after adding
    * sizeof(cache_system_t) and the 15-byte rounding slack. */
   if (pad < 0)
      Sys_Error("%s: bad pad %i", __func__, pad);
   if (size > INT_MAX - pad - (int)sizeof(cache_system_t) - 15)
      Sys_Error("%s: size %i (pad %i) too large", __func__, size, pad);

   size = (size + pad + sizeof(cache_system_t) + 15) & ~15;

   /* find memory for it */
   while (1)
   {
      cache_system_t *cs = Cache_TryAlloc(size, false);

      if (cs)
      {
         cs->user = c;
         c->pad = pad;
         c->data = Cache_Data(cs);
         break;
      }
      /* free the least recently used cache data */
      if (cache_head.lru_prev == &cache_head)
         Sys_Error("%s: out of memory", __func__);
      /* not enough memory at all */
      Cache_Free(cache_head.lru_prev->user);
   }

   return Cache_Check(c);
}

static void Cache_f(void)
{
   if (Cmd_Argc() == 2)
   {
      if (!strcmp(Cmd_Argv(1), "flush"))
      {
         Cache_Flush();
         return;
      }
   }
   Con_Printf("Usage: cache print|flush\n");
}

/* ========================================================================= */


/*
 * ========================
 * Memory_Init
 * ========================
 */
void Memory_Init(void *buf, int size)
{
   int p;
   int zonesize = DYNAMIC_SIZE;

   hunk_base = (byte*)buf;
   hunk_size = size;
   hunk_low_used = 0;
   hunk_high_used = 0;

   Cache_Init();
   p = COM_CheckParm("-zone");
   if (p) {
      if (p < com_argc - 1)
         zonesize = Q_atoi(com_argv[p + 1]) * 1024;
      else
         Sys_Error("%s: you must specify a size in KB after -zone",
               __func__);
   }
   mainzone = (memzone_t*)Hunk_Alloc(zonesize);
   Z_ClearZone(mainzone, zonesize);

   /* Needs to be added after the zone init... */
   Cmd_AddCommand("flush", Cache_Flush);
   Cmd_AddCommand("cache", Cache_f);
   Cmd_AddCommand("zone_check", Z_CheckHeap);
}
