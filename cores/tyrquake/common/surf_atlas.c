/*
 * surf_atlas.c -- RHI-agnostic surface texture atlas.
 *
 * See surf_atlas.h for API contracts and design notes.
 *
 * Implementation overview.  The atlas is partitioned into a
 * fixed set of horizontal strips at create time.  Each strip
 * owns a free-list of (x, width) extents and an in-use list
 * of cache entries placed within it.  Allocation walks the
 * strip whose height best fits the requested height (the
 * smallest strip-height >= request), looking for the first
 * free extent at least as wide as needed.  On a hit the
 * extent is split (allocated_x .. allocated_x + req_w
 * carved off the front, remainder pushed back into the free-
 * list).  On free, the released extent is coalesced with any
 * adjacent free extents in the same strip.
 *
 * The strip-set is fixed at create -- once a strip's
 * descriptor (height + count) is set, it doesn't change.
 * Allocating across strip-height tiers is the only
 * cross-strip path (the LRU evictor walks all strips when
 * no strip of the right height has free space).
 *
 * Eviction.  Each entry tracks `last_used_frame`.  When the
 * preferred strip has no free extent of sufficient width,
 * the LRU entry in that strip is freed and we retry.  If the
 * LRU entry was used THIS frame (last_used_frame ==
 * cur_frame), it's considered un-evictable -- another in-
 * flight dispatch may rely on it -- and we widen the search
 * to less-preferred strips (taller ones).  If everything is
 * in-use, hard failure: return rect{0,0,0,0,fresh=0}.
 *
 * Entry storage.  A fixed-size array of `entry` structs
 * (max_entries elements).  Lookup by key is currently
 * linear-scan -- adequate up to a few hundred entries; we
 * can swap in an open-addressed hash table later if profiling
 * shows it matters.
 *
 * C89-clean (no // comments, declarations at block start, no
 * mixed decls/code).  No libretro / quake-specific
 * dependencies beyond qboolean from qtypes.h.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "surf_atlas.h"

/* -------------------------------------------------------------------- */
/* Internal types.                                                      */
/* -------------------------------------------------------------------- */

/* A free extent inside a strip: [x, x + w).  Stored as a
 * sorted singly-linked list head per strip; sort key is x.
 * Coalesce happens at free-time so the list is always
 * minimally fragmented. */
typedef struct surf_atlas_extent_s {
   uint16_t                    x;
   uint16_t                    w;
   struct surf_atlas_extent_s *next;
} surf_atlas_extent_t;

/* A strip occupies a horizontal band at vertical offset
 * `y` with height `h`.  Width of every strip equals the
 * atlas width.  free_head is the head of the sorted free-
 * extent list (ascending x).  entry_head links the in-use
 * entries that live in this strip via entry.next_in_strip,
 * for the LRU eviction walk. */
typedef struct surf_atlas_strip_s {
   uint16_t             y;
   uint16_t             h;
   surf_atlas_extent_t *free_head;
   /* in-use entry linked list (intrusive via entry->next_in_strip;
    * head is the entry index, -1 = empty). */
   int32_t              entry_head;
} surf_atlas_strip_t;

/* One cached entry. */
typedef struct surf_atlas_entry_s {
   const void *key;             /* NULL = free slot in the entry pool */
   uint16_t    x;
   uint16_t    y;
   uint16_t    w;
   uint16_t    h;
   uint16_t    strip_idx;       /* which strip this lives in */
   uint32_t    last_used_frame;
   int32_t     next_in_strip;   /* next entry in strip's in-use list, -1 = tail */
   int32_t     prev_in_strip;   /* prev entry in strip's in-use list, -1 = head */
} surf_atlas_entry_t;

struct surf_atlas_s {
   uint16_t            width;
   uint16_t            height;
   uint16_t            max_entries;
   uint16_t            strip_count;
   surf_atlas_strip_t *strips;
   surf_atlas_entry_t *entries;
   uint32_t            cur_frame;

   /* Pool of pre-allocated extent nodes for the strips' free
    * lists.  Sized to a generous upper bound: at worst, every
    * entry fragments its strip and every entry-eviction
    * creates one extent node.  Realistic bound: 4 *
    * max_entries.  Free nodes form a singly-linked stack via
    * .next; head is `extent_freelist`. */
   surf_atlas_extent_t *extent_pool;
   uint32_t             extent_pool_capacity;
   surf_atlas_extent_t *extent_freelist;

   /* Stats. */
   surf_atlas_stats_t stats;
};

/* -------------------------------------------------------------------- */
/* Extent-node allocator (single contiguous pool, freelist).            */
/* -------------------------------------------------------------------- */

static surf_atlas_extent_t *
extent_alloc(surf_atlas_t *a)
{
   surf_atlas_extent_t *e;
   if (!a->extent_freelist)
      return NULL;
   e                  = a->extent_freelist;
   a->extent_freelist = e->next;
   e->next            = NULL;
   return e;
}

static void
extent_free(surf_atlas_t *a, surf_atlas_extent_t *e)
{
   if (!e)
      return;
   e->next            = a->extent_freelist;
   a->extent_freelist = e;
}

/* -------------------------------------------------------------------- */
/* Strip free-list operations.                                          */
/* -------------------------------------------------------------------- */

/* Find the leftmost free extent in `strip` whose width is
 * >= req_w.  Returns the extent node (head pointer in caller's
 * variable updated through *prev_out to point to the pointer
 * field that owns it -- this is the standard linked-list
 * remove-by-pointer-to-pointer idiom; *prev_out is &strip->
 * free_head or &previous->next).  Returns NULL on no fit.
 */
static surf_atlas_extent_t *
strip_find_fit(surf_atlas_strip_t   *strip,
               uint16_t              req_w,
               surf_atlas_extent_t ***prev_out)
{
   surf_atlas_extent_t **prev = &strip->free_head;
   surf_atlas_extent_t  *e;
   for (e = strip->free_head; e; e = e->next) {
      if (e->w >= req_w) {
         *prev_out = prev;
         return e;
      }
      prev = &e->next;
   }
   return NULL;
}

/* Insert a free extent into strip's sorted free-list, coalescing
 * with neighbours.  Takes ownership of `node` (may free it if it
 * coalesces into an existing extent). */
static void
strip_free_extent(surf_atlas_t        *a,
                  surf_atlas_strip_t  *strip,
                  surf_atlas_extent_t *node)
{
   surf_atlas_extent_t **prev = &strip->free_head;
   surf_atlas_extent_t  *cur;
   uint16_t              ins_x = node->x;
   uint16_t              ins_w = node->w;

   /* Walk to insertion point (sorted ascending by x). */
   for (cur = strip->free_head; cur; cur = cur->next) {
      if (cur->x >= ins_x)
         break;
      prev = &cur->next;
   }

   /* Coalesce with predecessor (if any).  `prev` points at
    * `cur` (next slot); the predecessor extent is whatever
    * stored its .next into the slot above *prev.  We need to
    * look at it explicitly via the strip head or by tracking
    * a separate pred pointer. */
   {
      surf_atlas_extent_t  *pred  = NULL;
      surf_atlas_extent_t **scan  = &strip->free_head;
      while (*scan != cur) {
         pred = *scan;
         scan = &(*scan)->next;
      }
      if (pred && pred->x + pred->w == ins_x) {
         /* Merge into predecessor. */
         pred->w = (uint16_t)(pred->w + ins_w);
         extent_free(a, node);
         node    = pred;
         ins_x   = pred->x;
         ins_w   = pred->w;
         /* Re-point prev to pred->next slot so the
          * successor-coalesce below works the same way. */
         prev    = &pred->next;
      } else {
         /* No predecessor merge; splice node into list at *prev. */
         node->x    = ins_x;
         node->w    = ins_w;
         node->next = cur;
         *prev      = node;
      }
   }

   /* Coalesce with successor (cur), if it's adjacent. */
   if (cur && ins_x + ins_w == cur->x) {
      node->w    = (uint16_t)(node->w + cur->w);
      node->next = cur->next;
      extent_free(a, cur);
   }
}

/* Carve [carve_x, carve_x + carve_w) out of a known-fitting
 * extent `e` (returned by strip_find_fit) whose owning slot
 * is *e_slot.  carve_x is always e->x (leftmost-fit).  Side
 * effects: shrinks/removes e from the free-list; on remove,
 * returns the node to the extent pool. */
static void
strip_carve(surf_atlas_t          *a,
            surf_atlas_extent_t   *e,
            surf_atlas_extent_t  **e_slot,
            uint16_t               carve_w)
{
   if (e->w == carve_w) {
      /* Exact fit: remove the extent entirely. */
      *e_slot = e->next;
      extent_free(a, e);
   } else {
      /* Shrink from the left. */
      e->x = (uint16_t)(e->x + carve_w);
      e->w = (uint16_t)(e->w - carve_w);
   }
}

/* -------------------------------------------------------------------- */
/* Per-strip entry list (intrusive doubly-linked).                       */
/* -------------------------------------------------------------------- */

static void
strip_list_insert(surf_atlas_t       *a,
                  surf_atlas_strip_t *strip,
                  int32_t             entry_idx)
{
   surf_atlas_entry_t *e   = &a->entries[entry_idx];
   int32_t             old = strip->entry_head;
   e->prev_in_strip        = -1;
   e->next_in_strip        = old;
   if (old != -1)
      a->entries[old].prev_in_strip = entry_idx;
   strip->entry_head       = entry_idx;
}

static void
strip_list_remove(surf_atlas_t       *a,
                  surf_atlas_strip_t *strip,
                  int32_t             entry_idx)
{
   surf_atlas_entry_t *e = &a->entries[entry_idx];
   if (e->prev_in_strip != -1)
      a->entries[e->prev_in_strip].next_in_strip = e->next_in_strip;
   else
      strip->entry_head = e->next_in_strip;
   if (e->next_in_strip != -1)
      a->entries[e->next_in_strip].prev_in_strip = e->prev_in_strip;
   e->prev_in_strip = -1;
   e->next_in_strip = -1;
}

/* -------------------------------------------------------------------- */
/* Entry pool helpers.                                                  */
/* -------------------------------------------------------------------- */

static int32_t
find_entry_by_key(const surf_atlas_t *a, const void *key)
{
   uint16_t i;
   for (i = 0; i < a->max_entries; i++) {
      if (a->entries[i].key == key)
         return (int32_t)i;
   }
   return -1;
}

static int32_t
find_free_entry_slot(const surf_atlas_t *a)
{
   uint16_t i;
   for (i = 0; i < a->max_entries; i++) {
      if (a->entries[i].key == NULL)
         return (int32_t)i;
   }
   return -1;
}

/* Strip selection: smallest strip-height >= req_h.  Returns
 * strip index or -1 if no strip is tall enough (shouldn't
 * happen if caller's request fits within the atlas's max
 * strip height, but the check is cheap). */
static int32_t
pick_strip_idx(const surf_atlas_t *a, uint16_t req_h)
{
   uint16_t i;
   int32_t  best     = -1;
   uint16_t best_h   = 0xFFFF;
   for (i = 0; i < a->strip_count; i++) {
      uint16_t h = a->strips[i].h;
      if (h >= req_h && h < best_h) {
         best   = (int32_t)i;
         best_h = h;
      }
   }
   return best;
}

/* Release an entry's rect back to its strip's free-list and
 * detach it from the strip's in-use list.  Marks the entry
 * slot free (key = NULL). */
static void
release_entry(surf_atlas_t *a, int32_t entry_idx)
{
   surf_atlas_entry_t  *e;
   surf_atlas_strip_t  *strip;
   surf_atlas_extent_t *node;

   if (entry_idx < 0 || entry_idx >= (int32_t)a->max_entries)
      return;
   e = &a->entries[entry_idx];
   if (!e->key)
      return;

   strip = &a->strips[e->strip_idx];

   /* Need an extent node to insert into the free-list.  In
    * the worst case this is unavailable (extent pool
    * exhausted -- shouldn't happen with the conservative
    * sizing in create, but guard for it).  If we can't get
    * a node, the rect is leaked from the strip's free-list
    * until an existing node can coalesce against it, which
    * never happens.  Mitigation: pool sizing in create()
    * uses 4 * max_entries which is provably sufficient
    * (each entry contributes at most 2 extent nodes -- one
    * to the free-list it carved from, one to coalesce
    * against later -- with 2x slack for transient mid-
    * coalesce states). */
   node = extent_alloc(a);
   if (node) {
      node->x = e->x;
      node->w = e->w;
      strip_free_extent(a, strip, node);
   }
   /* else: leak this rect; correctness preserved (other
    * allocations just see less free space). */

   strip_list_remove(a, strip, entry_idx);

   /* Update stats. */
   a->stats.pixels_allocated -= (uint64_t)e->w * (uint64_t)e->h;
   a->stats.entries_used--;

   /* Mark slot free. */
   e->key = NULL;
}

/* Find the LRU entry in `strip` whose last_used_frame is
 * strictly less than cur_frame (i.e., not in use this
 * frame).  Returns entry index or -1 if none qualifies. */
static int32_t
find_lru_in_strip(const surf_atlas_t       *a,
                  const surf_atlas_strip_t *strip,
                  uint32_t                  cur_frame)
{
   int32_t  cur     = strip->entry_head;
   int32_t  lru_idx = -1;
   uint32_t lru_age = 0;
   while (cur != -1) {
      const surf_atlas_entry_t *e = &a->entries[cur];
      if (e->last_used_frame < cur_frame) {
         uint32_t age = cur_frame - e->last_used_frame;
         if (lru_idx == -1 || age > lru_age) {
            lru_idx = cur;
            lru_age = age;
         }
      }
      cur = e->next_in_strip;
   }
   return lru_idx;
}

/* -------------------------------------------------------------------- */
/* Public API.                                                          */
/* -------------------------------------------------------------------- */

surf_atlas_t *
surf_atlas_create(const surf_atlas_config_t *cfg)
{
   surf_atlas_t *a;
   uint32_t      strip_h_total;
   uint16_t      i;
   uint16_t      strip_y;
   uint32_t      extent_pool_capacity;
   uint32_t      ext_i;

   if (!cfg || !cfg->strip_desc || cfg->strip_desc_count == 0)
      return NULL;
   if (cfg->width == 0 || cfg->height == 0 || cfg->max_entries == 0)
      return NULL;

   /* Validate: sum of strip heights * counts must equal
    * atlas height. */
   strip_h_total = 0;
   {
      uint16_t di;
      for (di = 0; di < cfg->strip_desc_count; di++) {
         strip_h_total += (uint32_t)cfg->strip_desc[di].height
                        * (uint32_t)cfg->strip_desc[di].count;
      }
   }
   if (strip_h_total != cfg->height)
      return NULL;

   /* Compute total strip count and validate it fits uint16_t. */
   {
      uint32_t total_strips = 0;
      uint16_t di;
      for (di = 0; di < cfg->strip_desc_count; di++)
         total_strips += cfg->strip_desc[di].count;
      if (total_strips == 0 || total_strips > 0xFFFFu)
         return NULL;

      a = (surf_atlas_t *)calloc(1, sizeof(*a));
      if (!a)
         return NULL;

      a->width       = cfg->width;
      a->height      = cfg->height;
      a->max_entries = cfg->max_entries;
      a->strip_count = (uint16_t)total_strips;
      a->cur_frame   = 1; /* 0 reserved for "never used" */
   }

   a->strips  = (surf_atlas_strip_t *)calloc(a->strip_count,
                                             sizeof(*a->strips));
   a->entries = (surf_atlas_entry_t *)calloc(a->max_entries,
                                             sizeof(*a->entries));
   if (!a->strips || !a->entries) {
      surf_atlas_destroy(a);
      return NULL;
   }

   /* Extent pool: bound by the per-strip free-list maximum
    * size plus eviction transient room.  Each in-use entry
    * can contribute up to 2 extents (one ahead, one behind);
    * 4x is a safe multiplier. */
   extent_pool_capacity = (uint32_t)a->max_entries * 4u
                        + (uint32_t)a->strip_count;
   if (extent_pool_capacity > 0xFFFFFFFFu)
      extent_pool_capacity = 0xFFFFFFFFu;
   a->extent_pool_capacity = extent_pool_capacity;
   a->extent_pool          = (surf_atlas_extent_t *)calloc(
         extent_pool_capacity, sizeof(*a->extent_pool));
   if (!a->extent_pool) {
      surf_atlas_destroy(a);
      return NULL;
   }
   /* Build the freelist: link every node into a stack. */
   a->extent_freelist = NULL;
   for (ext_i = 0; ext_i < extent_pool_capacity; ext_i++) {
      a->extent_pool[ext_i].next = a->extent_freelist;
      a->extent_freelist         = &a->extent_pool[ext_i];
   }

   /* Lay out strips top-to-bottom from the strip descriptors.
    * Each descriptor gives (height, count) and we materialise
    * `count` consecutive strips of `height` rows. */
   strip_y = 0;
   {
      uint16_t out = 0;
      uint16_t di;
      for (di = 0; di < cfg->strip_desc_count; di++) {
         uint16_t ci;
         for (ci = 0; ci < cfg->strip_desc[di].count; ci++) {
            surf_atlas_strip_t  *s    = &a->strips[out];
            surf_atlas_extent_t *e0;
            s->y          = strip_y;
            s->h          = cfg->strip_desc[di].height;
            s->entry_head = -1;
            /* Initial free extent spans the entire strip width. */
            e0 = extent_alloc(a);
            if (!e0) {
               surf_atlas_destroy(a);
               return NULL;
            }
            e0->x        = 0;
            e0->w        = a->width;
            e0->next     = NULL;
            s->free_head = e0;
            strip_y      = (uint16_t)(strip_y + s->h);
            out++;
         }
      }
   }

   /* Init entry slots to "free" and -1 list links. */
   for (i = 0; i < a->max_entries; i++) {
      a->entries[i].key             = NULL;
      a->entries[i].prev_in_strip   = -1;
      a->entries[i].next_in_strip   = -1;
   }

   /* Stats baseline. */
   a->stats.entries_total = a->max_entries;
   a->stats.pixels_total  = (uint64_t)a->width * (uint64_t)a->height;

   return a;
}

void
surf_atlas_destroy(surf_atlas_t *a)
{
   if (!a)
      return;
   free(a->extent_pool);
   free(a->strips);
   free(a->entries);
   free(a);
}

void
surf_atlas_begin_frame(surf_atlas_t *a)
{
   if (!a)
      return;
   a->cur_frame++;
}

/* Try to allocate a rect of (req_w, req_h) in strip
 * `strip_idx`.  Returns 1 on success (carves the extent,
 * fills *out_x, *out_y) or 0 on no fit. */
static int
try_alloc_in_strip(surf_atlas_t *a,
                   uint16_t       strip_idx,
                   uint16_t       req_w,
                   uint16_t       req_h,
                   uint16_t      *out_x,
                   uint16_t      *out_y)
{
   surf_atlas_strip_t   *strip = &a->strips[strip_idx];
   surf_atlas_extent_t  *e;
   surf_atlas_extent_t **slot;
   (void)req_h;
   e = strip_find_fit(strip, req_w, &slot);
   if (!e)
      return 0;
   *out_x = e->x;
   *out_y = strip->y;
   strip_carve(a, e, slot, req_w);
   return 1;
}

/* Evict the LRU entry from `strip` (if one exists not in-
 * use this frame), returning its key into evicted_keys[] /
 * evicted_count.  Returns 1 if an eviction happened. */
static int
evict_lru_from_strip(surf_atlas_t      *a,
                     uint16_t           strip_idx,
                     surf_atlas_rect_t *rect_out)
{
   surf_atlas_strip_t *strip = &a->strips[strip_idx];
   int32_t             idx;
   const void         *evicted_key;
   idx = find_lru_in_strip(a, strip, a->cur_frame);
   if (idx < 0)
      return 0;
   evicted_key = a->entries[idx].key;
   release_entry(a, idx);
   a->stats.evictions_total++;
   if (rect_out
       && rect_out->evicted_count < SURF_ATLAS_MAX_EVICTIONS_PER_GET) {
      rect_out->evicted_keys[rect_out->evicted_count++] = evicted_key;
   }
   return 1;
}

surf_atlas_rect_t
surf_atlas_get(surf_atlas_t *a,
               const void   *key,
               uint16_t      req_w,
               uint16_t      req_h)
{
   surf_atlas_rect_t rect;
   int32_t           idx;
   int32_t           preferred_strip;
   int32_t           free_slot;
   surf_atlas_entry_t *e;
   uint16_t          alloc_x;
   uint16_t          alloc_y;
   uint16_t          strip_used;
   int               allocated;

   memset(&rect, 0, sizeof(rect));

   if (!a || !key || req_w == 0 || req_h == 0)
      return rect;

   a->stats.get_calls_total++;

   if (req_w > a->width)
      return rect;

   /* Hit path: existing entry for this key.  If its rect's
    * dimensions match the request, return it as a clean cache
    * hit (fresh=0).  If they DON'T match, the SW source
    * underlying this key has been reused for a different
    * surface (the typical scenario: D_SCAlloc reclaiming a
    * surfcache_t slot for a different brush surface, or a
    * miplevel change rebuilding the cache at the same slot)
    * and the cached rect is stale.  Auto-evict and fall
    * through to the alloc path so the caller gets a properly-
    * sized fresh rect.  We don't surface this in evicted_keys
    * [] -- the caller's response to fresh=1 (re-upload) already
    * covers any per-key state it kept alongside the atlas. */
   idx = find_entry_by_key(a, key);
   if (idx >= 0) {
      e = &a->entries[idx];
      if (e->w == req_w && e->h == req_h) {
         e->last_used_frame = a->cur_frame;
         rect.x     = e->x;
         rect.y     = e->y;
         rect.w     = e->w;
         rect.h     = e->h;
         rect.fresh = 0;
         a->stats.get_hits_total++;
         return rect;
      }
      /* Dim mismatch.  Release the stale entry, fall through. */
      release_entry(a, idx);
      a->stats.evictions_total++;
   }

   /* Miss: allocate.  Pick the preferred strip. */
   preferred_strip = pick_strip_idx(a, req_h);
   if (preferred_strip < 0) {
      a->stats.hard_failures_total++;
      return rect;
   }

   /* Reserve an entry slot first.  If none is free, try
    * evicting in the preferred strip (LRU there frees up an
    * entry slot too). */
   free_slot = find_free_entry_slot(a);
   if (free_slot < 0) {
      if (!evict_lru_from_strip(a, (uint16_t)preferred_strip, &rect)) {
         /* Fall back to evicting from any strip with a
          * not-in-use entry. */
         uint16_t si;
         int      ok = 0;
         for (si = 0; si < a->strip_count && !ok; si++) {
            if ((int32_t)si == preferred_strip)
               continue;
            ok = evict_lru_from_strip(a, si, &rect);
         }
         if (!ok) {
            a->stats.hard_failures_total++;
            return rect;
         }
      }
      free_slot = find_free_entry_slot(a);
      if (free_slot < 0) {
         a->stats.hard_failures_total++;
         return rect;
      }
   }

   /* Try the preferred strip first.  On no fit, evict LRU
    * in that strip until either we fit or there's nothing
    * left to evict.  Then walk to less-preferred (taller)
    * strips. */
   allocated  = try_alloc_in_strip(a, (uint16_t)preferred_strip,
                                   req_w, req_h, &alloc_x, &alloc_y);
   strip_used = (uint16_t)preferred_strip;
   while (!allocated) {
      if (!evict_lru_from_strip(a, strip_used, &rect))
         break;
      allocated = try_alloc_in_strip(a, strip_used,
                                     req_w, req_h, &alloc_x, &alloc_y);
   }
   if (!allocated) {
      /* Try taller strips, smallest-first. */
      uint16_t si;
      uint16_t best     = a->strip_count;
      uint16_t best_h   = 0xFFFF;
      for (si = 0; si < a->strip_count; si++) {
         if (si == preferred_strip)
            continue;
         if (a->strips[si].h >= req_h && a->strips[si].h < best_h) {
            best   = si;
            best_h = a->strips[si].h;
         }
      }
      if (best < a->strip_count) {
         strip_used = best;
         allocated  = try_alloc_in_strip(a, strip_used,
                                         req_w, req_h,
                                         &alloc_x, &alloc_y);
         while (!allocated) {
            if (!evict_lru_from_strip(a, strip_used, &rect))
               break;
            allocated = try_alloc_in_strip(a, strip_used,
                                           req_w, req_h,
                                           &alloc_x, &alloc_y);
         }
      }
   }

   if (!allocated) {
      a->stats.hard_failures_total++;
      return rect;
   }

   /* Commit. */
   e                    = &a->entries[free_slot];
   e->key               = key;
   e->x                 = alloc_x;
   e->y                 = alloc_y;
   e->w                 = req_w;
   e->h                 = req_h;
   e->strip_idx         = strip_used;
   e->last_used_frame   = a->cur_frame;
   strip_list_insert(a, &a->strips[strip_used], free_slot);

   a->stats.entries_used++;
   a->stats.pixels_allocated += (uint64_t)req_w * (uint64_t)req_h;

   rect.x     = alloc_x;
   rect.y     = alloc_y;
   rect.w     = req_w;
   rect.h     = req_h;
   rect.fresh = 1;
   return rect;
}

void
surf_atlas_evict(surf_atlas_t *a, const void *key)
{
   int32_t idx;
   if (!a || !key)
      return;
   idx = find_entry_by_key(a, key);
   if (idx >= 0) {
      release_entry(a, idx);
      a->stats.evictions_total++;
   }
}

void
surf_atlas_get_stats(const surf_atlas_t *a, surf_atlas_stats_t *out)
{
   if (!a || !out)
      return;
   *out = a->stats;
}
