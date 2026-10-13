/*
 * surf_atlas.h -- RHI-agnostic surface texture atlas
 *
 * Manages a single fixed-size 2D atlas storing brush-surface
 * texture+lightmap composites (the output of D_CacheSurface +
 * R_DrawSurface).  Pure C, no backend-specific types -- both
 * the compute renderer (consumes via storage-image imageLoad)
 * and future rasterized renderers (consume via texture sample)
 * share this module.  The backend owns the actual GPU
 * resources (VkImage, ID3D11Texture2D, GLuint, etc.) and
 * uploads pixels via its native API; this module only tracks
 * which atlas regions hold which cache entries.
 *
 * Storage model.  The atlas is partitioned into fixed-height
 * horizontal strips at create time.  Each strip has its own
 * free-list of (x, width) extents.  An incoming surface of
 * height H is placed in the strip with the smallest height
 * >= H (rounded up to the strip-set's smallest power of two
 * that fits H).  Variable-width allocation within the strip
 * is first-fit + immediate coalesce on free.  This trades a
 * small amount of vertical-axis fragmentation (a 40-tall
 * surface in a 64-tall strip wastes 24 px of height) for
 * predictable allocation cost and zero allocator complexity.
 *
 * Eviction.  Each cached entry tracks the frame number of its
 * most recent surf_atlas_get hit.  When allocation fails, the
 * LRU entry in any strip is evicted (its rect returned to
 * its strip's free-list) and allocation retries.  If the new
 * entry's height doesn't match the LRU's strip, further LRU
 * entries are evicted until a suitable rect is freed.  Hard
 * failure (return rect{0,0,0,0,fresh=0}) occurs only if all
 * strips matching the requested height contain only entries
 * marked-in-use this frame.
 *
 * Cache key.  An opaque `const void *key`, typically the
 * surfcache_t * from D_CacheSurface.  The atlas tracks one
 * entry per unique key.  Duplicate keys are coalesced -- a
 * second get() with the same key returns the same rect.
 *
 * Lifecycle.  Caller drives begin_frame at the start of each
 * frame (advances the LRU clock).  Within the frame, each
 * brush-surface dispatch path calls surf_atlas_get to look
 * up the surface's atlas slot.  The .fresh field indicates
 * "newly allocated this call" -- the caller's responsibility
 * to upload the source pixels into the backend's GPU
 * resource at the returned (x, y, w, h).  The atlas itself
 * stores no pixel data.
 *
 * Stale entries.  D_CacheSurface may reuse a surfcache_t
 * slot pointer for a different surface (the underlying
 * D_SCAlloc heap is LRU and reclaims slots when CPU memory
 * pressure forces it).  When the caller detects this (via
 * cache->texture / lightadj[] state changing for the same
 * pointer) it calls surf_atlas_evict(key) and a subsequent
 * surf_atlas_get will allocate fresh.  The atlas itself
 * doesn't track content-validity -- the caller knows the
 * SW cache state and is responsible for invalidation.
 *
 * Thread safety.  Not thread-safe; intended for the single
 * render thread.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 The libretro team
 */
#ifndef SURF_ATLAS_H
#define SURF_ATLAS_H

#include <stddef.h>
#include <stdint.h>

#include "qtypes.h"   /* qboolean */

typedef struct surf_atlas_s surf_atlas_t;

/*
 * Strip-set description.  One descriptor per strip-height
 * tier.  The atlas allocates `count` strips of `height`
 * pixels at the start of construction; total strip height
 * must equal the atlas height.  Strips are stacked top-down
 * in descriptor order.
 *
 * Recommended descriptor for Quake brush surfaces at a 4096
 * wide atlas (fits the cross-API minimum guaranteed texture
 * dimension of 4096 across D3D9 SM2 / GL 2.x / Vulkan):
 *
 *     {  16, 32 }  =  512 px
 *     {  32, 16 }  =  512 px
 *     {  64, 16 }  = 1024 px
 *     { 128,  8 }  = 1024 px
 *     { 256,  2 }  =  512 px
 *     { 512,  1 }  =  512 px
 *                   ----
 *                   4096 px
 *
 * This distribution is empirical for id1 maps -- most surface
 * extents at typical miplevels fall in the 16..128 height
 * range; 256 and 512 are reserve for floor/ceiling at mip-0
 * close to the camera.
 */
typedef struct {
   uint16_t height;
   uint16_t count;
} surf_atlas_strip_desc_t;

typedef struct {
   uint16_t                       width;
   uint16_t                       height;
   uint16_t                       max_entries;
   uint16_t                       strip_desc_count;
   const surf_atlas_strip_desc_t *strip_desc;
} surf_atlas_config_t;

/*
 * Lookup-or-alloc result.  rect coordinates and dimensions
 * are atlas-local pixel units.  If w == 0 the lookup failed
 * hard (atlas exhausted with no evictable LRU entries) and
 * the caller must fall through to a non-atlas path
 * (typically SW raster).
 *
 * fresh == 1 means this lookup allocated a new slot; the
 * caller must upload the source pixels into the backend's
 * resource at the returned rect.  fresh == 0 means a
 * cache hit; no upload needed.
 *
 * evicted_keys[] (if non-empty) reports keys whose entries
 * were evicted by this allocation, so the caller can drop
 * any per-key state it keeps alongside the atlas (e.g. a
 * "already-uploaded this session" tracker).  evicted_count
 * is the number of valid entries in evicted_keys[].
 */
#define SURF_ATLAS_MAX_EVICTIONS_PER_GET 8

typedef struct {
   uint16_t    x;
   uint16_t    y;
   uint16_t    w;
   uint16_t    h;
   uint8_t     fresh;
   uint8_t     evicted_count;
   const void *evicted_keys[SURF_ATLAS_MAX_EVICTIONS_PER_GET];
} surf_atlas_rect_t;

typedef struct {
   uint32_t entries_used;
   uint32_t entries_total;
   uint64_t pixels_allocated;
   uint64_t pixels_total;
   uint32_t evictions_total;
   uint32_t hard_failures_total;
   uint32_t get_calls_total;
   uint32_t get_hits_total;
} surf_atlas_stats_t;

/*
 * Construct an atlas with the given configuration.  Returns
 * NULL on out-of-memory or invalid config (zero dims, strip
 * heights not summing to atlas height, etc.).  Caller must
 * pair with surf_atlas_destroy.
 */
surf_atlas_t *surf_atlas_create(const surf_atlas_config_t *cfg);

void surf_atlas_destroy(surf_atlas_t *a);

/*
 * Advance the frame counter.  Call at the start of each
 * render frame.  Existing entries' last_used_frame becomes
 * "older" relative to this frame, making them more
 * eviction-eligible.
 */
void surf_atlas_begin_frame(surf_atlas_t *a);

/*
 * Lookup or allocate a rect for `key` with content dimensions
 * (w, h).  See surf_atlas_rect_t for return semantics.
 *
 * If an existing entry for `key` is found with matching
 * dimensions, its rect is returned with fresh=0 (cache hit).
 * If found with DIFFERENT dimensions (the underlying source
 * was reused for a different surface, or its miplevel changed
 * and the cache was rebuilt at the same key), the stale entry
 * is auto-evicted and a fresh rect at the requested dimensions
 * is allocated -- caller sees rect.fresh=1 in this case and
 * should treat it as a fresh allocation (re-upload pixels,
 * etc.).  The auto-eviction is NOT reported in
 * evicted_keys[]; the caller's response to fresh=1 already
 * covers any per-key state kept alongside the atlas.
 */
surf_atlas_rect_t surf_atlas_get(surf_atlas_t *a,
                                 const void   *key,
                                 uint16_t      w,
                                 uint16_t      h);

/*
 * Force-evict an entry by key.  No-op if key isn't cached.
 * Use when the caller knows the SW-side content for this
 * key was invalidated (D_CacheSurface reused the pointer
 * for a different surface, or content-dimensions changed).
 */
void surf_atlas_evict(surf_atlas_t *a, const void *key);

/*
 * Read out current statistics.
 */
void surf_atlas_get_stats(const surf_atlas_t *a,
                          surf_atlas_stats_t *out);

#endif /* SURF_ATLAS_H */
