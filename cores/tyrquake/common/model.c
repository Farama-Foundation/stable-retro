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
/* models.c -- model loading and caching */

/* models are the only shared resource between a client and server running */
/* on the same machine. */

#include "compat/strl.h"
#include <float.h>
#include <stdint.h>

#include "cmd.h"
#include "common.h"
#include "console.h"
#include "model.h"

#include "quakedef.h"
#include "render.h"
#include "sys.h"
/* FIXME - quick hack to enable merging of NQ/QWSV shared code */
#define SV_Error Sys_Error

static model_t *loadmodel;

static void Mod_LoadBrushModel(model_t *mod, void *buffer, unsigned long size);
static model_t *Mod_LoadModel(model_t *mod, qboolean crash);

#define MAX_MOD_KNOWN 512
static model_t mod_known[MAX_MOD_KNOWN];
static int mod_numknown;

static const model_loader_t *mod_loader;

static void PVSCache_f(void);

/* coloredlights tracks the current map's lightdata format (1 if .lit
 * file was loaded with RGB lightmap data, 0 if the BSP's grayscale
 * lightdata is in use).  It is the runtime mirror of the user's
 * r_coloredlight cvar, but only resyncs at map load time -- the
 * lightdata stride is baked into the loaded map and cannot change
 * mid-level without re-reading the file.  Toggling r_coloredlight
 * mid-game persists the preference; the new value takes effect on
 * the next Mod_LoadLighting. */

extern cvar_t r_coloredlight;

int coloredlights = 0;


/*
===============
Mod_Init
===============
*/
void
Mod_Init(const model_loader_t *loader)
{
    Cmd_AddCommand("pvscache", PVSCache_f);
    mod_loader = loader;
}

/*
===============
Mod_Shutdown

Clear the loaded-model registry.  Each entry in mod_known has fields
that point into the hunk -- nodes, lightdata, surfaces, texture data,
the alias-model cache_user_t, and so on -- all of which are
invalidated when the libretro frontend frees the backing heap between
deinit and the next load_game.

Without this reset, on the next session Mod_FindName() would find
matching entries by name (mod_numknown is non-zero), and Mod_LoadModel
would either:
  - return the cached model directly for brush/sprite types (line
    "return mod" for non-alias not-needing-load), so the caller
    dereferences dangling Hunk pointers, or
  - call Cache_Check() for alias models, which reads mod->cache.data
    -- still non-NULL from the previous session -- and dereferences
    it as a cache_system_t.

Both are use-after-free.  Zeroing mod_known is enough; the memory it
points at is reclaimed wholesale by the heap free, so we don't need
to (and can't) free individual fields.
===============
*/
void
Mod_Shutdown(void)
{
    memset(mod_known, 0, sizeof(mod_known));
    mod_numknown = 0;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t * Mod_PointInLeaf(const model_t *model, const vec3_t point)
{
   mnode_t *node;

   if (!model || !model->nodes)
      SV_Error("%s: bad model", __func__);

   node = model->nodes;

   while (1)
   {
      float dist;
      mplane_t *plane;
      if (node->contents < 0)
         return (mleaf_t *)node;
      plane = node->plane;
      dist = DotProduct(point, plane->normal) - plane->dist;
      if (dist > 0)
         node = node->children[0];
      else
         node = node->children[1];
   }

   return NULL;		/* never reached */
}

void
Mod_AddLeafBits(leafbits_t *dst, const leafbits_t *src)
{
    int i, leafblocks;
    const leafblock_t *srcblock;
    leafblock_t *dstblock;

    if (src->numleafs != dst->numleafs)
	SV_Error("%s: src->numleafs (%d) != dst->numleafs (%d)",
		 __func__, src->numleafs, dst->numleafs);

    srcblock = src->bits;
    dstblock = dst->bits;
    leafblocks = (src->numleafs + LEAFMASK) >> LEAFSHIFT;
    for (i = 0; i < leafblocks; i++)
	*dstblock++ |= *srcblock++;
}

/*
 * Simple LRU cache for decompressed vis data
 */
typedef struct {
    const model_t *model;
    const mleaf_t *leaf;
    leafbits_t *leafbits;
} pvscache_t;
static pvscache_t pvscache[2];
static leafbits_t *fatpvs;
static int pvscache_numleafs;
static int pvscache_bytes;
static int pvscache_blocks;

static int c_cachehit, c_cachemiss;

#define PVSCACHE_SIZE ARRAY_SIZE(pvscache)

static void
Mod_InitPVSCache(int numleafs)
{
    int i;
    int memsize;
    byte *leafmem;

    pvscache_numleafs = numleafs;
    pvscache_bytes = ((numleafs + LEAFMASK) & ~LEAFMASK) >> 3;
    pvscache_blocks = pvscache_bytes / sizeof(leafblock_t);
    memsize = Mod_LeafbitsSize(numleafs);
    fatpvs = (leafbits_t*)Hunk_Alloc(memsize);

    memset(pvscache, 0, sizeof(pvscache));
    leafmem = (byte*)Hunk_Alloc(PVSCACHE_SIZE * memsize);
    for (i = 0; i < PVSCACHE_SIZE; i++)
	pvscache[i].leafbits = (leafbits_t *)(leafmem + i * memsize);
}

/*
===================
Mod_DecompressVis
===================
*/

static void
Mod_DecompressVis(const byte *in, const model_t *model, leafbits_t *dest)
{
    leafblock_t *out;
    int num_out;
    int shift;
    int count;

    dest->numleafs = model->numleafs;
    out = dest->bits;

    if (!in) {
	/* no vis info, so make all visible */
	memset(out, 0xff, pvscache_bytes);
	return;
    }

    memset(out, 0, pvscache_bytes);
    num_out = 0;
    shift = 0;
    do {
	if (*in) {
	    *out |= (leafblock_t)*in++ << shift;
	    shift += 8;
	    num_out += 8;
	    if (shift == (1 << LEAFSHIFT)) {
		shift = 0;
		out++;
	    }
	    continue;
	}

	/* Run of zeros - skip over */
	count = in[1];
	in += 2;
	out += count / sizeof(leafblock_t);
	shift += (count % sizeof(leafblock_t)) << 3;
	num_out += count << 3;
	if (shift >= (1 << LEAFSHIFT)) {
	    shift -= (1 << LEAFSHIFT);
	    out++;
	}
    } while (num_out < dest->numleafs);
}

const leafbits_t *
Mod_LeafPVS(const model_t *model, const mleaf_t *leaf)
{
    int slot;
    pvscache_t tmp;

    for (slot = 0; slot < PVSCACHE_SIZE; slot++)
	if (pvscache[slot].model == model && pvscache[slot].leaf == leaf) {
	    c_cachehit++;
	    break;
	}

    if (slot) {
	if (slot == PVSCACHE_SIZE) {
	    slot--;
	    tmp.model = model;
	    tmp.leaf = leaf;
	    tmp.leafbits = pvscache[slot].leafbits;
	    if (leaf == model->leafs) {
		/* return set with everything visible */
		tmp.leafbits->numleafs = model->numleafs;
		memset(tmp.leafbits->bits, 0xff, pvscache_bytes);
	    } else {
		Mod_DecompressVis(leaf->compressed_vis, model, tmp.leafbits);
	    }
	    c_cachemiss++;
	} else {
	    tmp = pvscache[slot];
	}
	memmove(pvscache + 1, pvscache, slot * sizeof(pvscache_t));
	pvscache[0] = tmp;
    }

    return pvscache[0].leafbits;
}

static void
PVSCache_f(void)
{
    Con_Printf("PVSCache: %7d hits %7d misses\n", c_cachehit, c_cachemiss);
}

static void Mod_AddToFatPVS(const model_t *model, const vec3_t point, const mnode_t *node)
{
   while (1)
   {
      float d;
      mplane_t *plane;

      /* if this is a leaf, accumulate the pvs bits */
      if (node->contents < 0)
      {
         if (node->contents != CONTENTS_SOLID)
         {
            const leafbits_t *pvs = Mod_LeafPVS(model, (const mleaf_t *)node);
            Mod_AddLeafBits(fatpvs, pvs);
         }
         return;
      }

      plane = node->plane;
      d = DotProduct(point, plane->normal) - plane->dist;

      if (d > 8)
         node = node->children[0];
      else if (d < -8)
         node = node->children[1];
      else
      {			/* go down both */
         Mod_AddToFatPVS(model, point, node->children[0]);
         node = node->children[1];
      }
   }
}

/*
=============
Mod_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.

The FatPVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.
=============
*/
const leafbits_t *
Mod_FatPVS(const model_t *model, const vec3_t point)
{
    fatpvs->numleafs = model->numleafs;
    memset(fatpvs->bits, 0, pvscache_bytes);
    Mod_AddToFatPVS(model, point, model->nodes);

    return fatpvs;
}

/*
=============
Mod_FatPVSThroughLiquid

Like Mod_FatPVS, but additionally extends the visible set across
liquid surfaces so entities on the other side of translucent
water/lava/slime/teleporter surfaces are sent to the client.

Vanilla Quake's PVS treats liquid surfaces as opaque -- a leaf
above water has no PVS overlap with the leaf below it.  When the
client renders translucent liquids and the player is on one side,
SV_WriteEntitiesToClient with plain Mod_FatPVS would silently drop
every entity on the other side (doors, pickups, monsters).  No
amount of client-side rendering can recover entities the server
never sent.

This variant probes vertically from the source point, sampling
leaves at ±64, ±128, ±192, and ±256 units along Z.  Every probe
that lands in a non-solid leaf with a different content type than
the source has its containing leaf's PVS folded in via the
standard Mod_AddToFatPVS path.  Probes into solid or same-content
leaves are skipped.

Coverage: vertical-only is sufficient because Quake water surfaces
are nearly always horizontal (the engine's water/swimming/contents
logic assumes this).  ±256 covers the typical vertical extent of a
water/air pair in stock and community maps.

Cost: the per-probe early-out via Mod_PointInLeaf (log-n BSP
descent, no PVS work) makes the typical case very cheap -- only
probes that actually cross a content boundary trigger the
recursive Mod_AddToFatPVS.  When the player is far from any
liquid, every probe lands in the same content type and the work
collapses to one extra Mod_PointInLeaf per probe (~8 of them,
each O(log n)).

Use Mod_FatPVS when liquid translucency is off; Mod_FatPVSThroughLiquid
adds latency to entity-update generation that isn't worth paying
when the result would be byte-identical.
=============
*/
const leafbits_t *
Mod_FatPVSThroughLiquid(const model_t *model, const vec3_t point)
{
    static const float probe_offsets[] = {
	 64.0f,  128.0f,  192.0f,  256.0f,
	-64.0f, -128.0f, -192.0f, -256.0f,
    };
    const int num_probes = (int)(sizeof(probe_offsets) / sizeof(probe_offsets[0]));
    int starting_contents;
    int i;

    /* Build the base FatPVS first. */
    fatpvs->numleafs = model->numleafs;
    memset(fatpvs->bits, 0, pvscache_bytes);
    Mod_AddToFatPVS(model, point, model->nodes);

    /* Identify the source point's medium so probes can detect
     * boundary crossings.  Bail if the source lookup fails (should
     * not happen on a loaded map, but being defensive keeps the
     * function pure-additive). */
    {
	const mleaf_t *src_leaf = Mod_PointInLeaf(model, point);
	if (!src_leaf)
	    return fatpvs;
	starting_contents = src_leaf->contents;
    }

    /* Probe vertically.  Only different-medium hits OR in their
     * leaf's PVS; same-medium probes were already covered by the
     * base FatPVS above. */
    for (i = 0; i < num_probes; i++) {
	vec3_t probe;
	const mleaf_t *probe_leaf;

	VectorCopy(point, probe);
	probe[2] += probe_offsets[i];

	probe_leaf = Mod_PointInLeaf(model, probe);
	if (!probe_leaf)
	    continue;
	if (probe_leaf->contents == CONTENTS_SOLID)
	    continue;
	if (probe_leaf->contents == starting_contents)
	    continue;

	Mod_AddToFatPVS(model, probe, model->nodes);
    }

    return fatpvs;
}

/*
===================
Mod_ClearAll
===================
*/
void
Mod_ClearAll(void)
{
    int i;
    model_t *mod;

    for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++) {
	if (mod->type != mod_alias)
	    mod->needload = true;
	/*
	 * FIXME: sprites use the cache data pointer for their own purposes,
	 *        bypassing the Cache_Alloc/Free functions.
	 */
	if (mod->type == mod_sprite)
	    mod->cache.data = NULL;
    }

    fatpvs = NULL;
    memset(pvscache, 0, sizeof(pvscache));
    pvscache_numleafs = 0;
    pvscache_bytes = pvscache_blocks = 0;
    c_cachehit = c_cachemiss = 0;
}

/*
===================
Mod_FlushAlias

Drop every cached alias model and mark it for reload.  Used when a
mesh-affecting render option (currently r_polysubdiv) changes value:
alias models normally outlive map changes and live in the cache
indefinitely, so without this helper a setting change would only
become visible when the cache happens to evict the old entry.

Each alias model's Hunk image (created by Mod_LoadAliasModel via
Cache_AllocPadded) is released with Cache_Free, then needload is set
so Mod_LoadModel re-reads the file the next time the model is
referenced.  Brush and sprite models are left alone; they don't
participate in alias subdivision and have their own load semantics.
===================
*/
void
Mod_FlushAlias(void)
{
    int i;
    model_t *mod;

    for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++) {
	if (mod->type != mod_alias)
	    continue;
	if (Cache_Check(&mod->cache))
	    Cache_Free(&mod->cache);
	mod->needload = true;
    }
}

/*
==================
Mod_FindName

==================
*/
static model_t *
Mod_FindName(const char *name)
{
    int i;
    model_t *mod;

    if (!name[0])
	SV_Error("%s: NULL name", __func__);

/**/
/* search the currently loaded models */
/**/
    for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	if (!strcmp(mod->name, name))
	    break;

    if (i == mod_numknown) {
	if (mod_numknown == MAX_MOD_KNOWN)
	    SV_Error("mod_numknown == MAX_MOD_KNOWN");
	strlcpy(mod->name, name, sizeof(mod->name));
	mod->needload = true;
	mod_numknown++;
    }

    return mod;
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
static model_t *
Mod_LoadModel(model_t *mod, qboolean crash)
{
    unsigned *buf;
    byte stackbuf[1024];	/* avoid dirtying the cache heap */
    unsigned long size;

    if (!mod->needload) {
	if (mod->type == mod_alias) {
	    if (Cache_Check(&mod->cache))
		return mod;
	} else
	    return mod;		/* not cached at all */
    }

    /* load the file */
    buf = (unsigned int*)COM_LoadStackFile(mod->name, stackbuf, sizeof(stackbuf), &size);
    if (!buf) {
	if (crash)
	    SV_Error("%s: %s not found", __func__, mod->name);
	return NULL;
    }

    /* allocate a new model */
    loadmodel = mod;

/**/
/* fill it in */
/**/

/* call the apropriate loader */
    mod->needload = false;

    switch (LittleLong(*(unsigned *)buf))
    {
       case IDPOLYHEADER:
          Mod_LoadAliasModel(mod_loader, mod, buf, loadmodel);
          break;

       case IDSPRITEHEADER:
          Mod_LoadSpriteModel(mod, buf);
          break;
       default:
          Mod_LoadBrushModel(mod, buf, size);
          break;
    }

    return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *
Mod_ForName(const char *name, qboolean crash)
{
    model_t *mod;

    mod = Mod_FindName(name);

    return Mod_LoadModel(mod, crash);
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

static byte *mod_base;


/*
=================
Mod_BoundLoaderCount

Validates the (count + reserve_slots) * record_size product that the
brush-model loaders pass to Hunk_Alloc, SV_Erroring before the
multiplication wraps in 32-bit signed arithmetic.

The lump-extent checks in Mod_LoadBrushModel bound `l->filelen` against
the file size (capped at INT_MAX = ~2 GiB by COM_filelength), but the
per-record output size in some loaders is significantly larger than the
on-disk record (e.g. bsp29_dface_t = 20 B versus msurface_t = 128 B, a
6.4x growth).  With a maliciously crafted BSP at 670 MiB the face
loader's `count * 128` wraps to 0, Hunk_Alloc accepts a zero-byte
request, and the rasterizer loop walks `count` records past the
allocation -- d0f7e07 (Hunk_Alloc INT_MAX bound) only catches the
negative or near-INT_MAX wrap, not the zero-wrap case.

`reserve_slots` covers loaders that allocate extra trailing records
(Mod_LoadEdges allocates one extra medge_t); `record_size` should be
the full per-input-record output size including any compile-time
multiplier (Mod_LoadPlanes passes 2 * sizeof(mplane_t)).

No behaviour change on well-formed BSPs; failures identify the
offending lump count in the style of the existing audit checks.
=================
*/
static void
Mod_BoundLoaderCount(int count, int reserve_slots, size_t record_size,
                     const char *fnname, const char *modname)
{
   size_t total_records;
   if (count < 0 || reserve_slots < 0)
      SV_Error("%s: bad count %d / reserve %d in %s",
               fnname, count, reserve_slots, modname);
   if ((size_t)count > (size_t)INT_MAX - (size_t)reserve_slots)
      SV_Error("%s: count %d + reserve %d overflows int in %s",
               fnname, count, reserve_slots, modname);
   total_records = (size_t)count + (size_t)reserve_slots;
   if (record_size == 0 || total_records > (size_t)INT_MAX / record_size)
      SV_Error("%s: %zu records * %zu bytes exceeds INT_MAX in %s",
               fnname, total_records, record_size, modname);
}


/*
=================
Mod_LoadTextures
=================
*/
static void
Mod_LoadTextures(lump_t *l)
{
   int i, j, pixels, num, max, altmax = 0;
   miptex_t *mt;
   texture_t *tx, *tx2;
   texture_t *anims[10];
   texture_t *altanims[10];
   dmiptexlump_t *m;

   if (!l->filelen) {
      loadmodel->textures = NULL;
      return;
   }
   /* l->filelen has already been bounded against mod_base size
    * by the dheader_t lump validation, but the contents of the
    * texture lump are file-controlled.  Defend each step. */
   if (l->filelen < (int)sizeof(int32_t))
      SV_Error("%s: texture lump too small for nummiptex (%d)",
               __func__, l->filelen);
   m = (dmiptexlump_t *)(mod_base + l->fileofs);

   m->nummiptex = LittleLong(m->nummiptex);

   /* nummiptex bounds.  Need (nummiptex >= 0) and the dataofs
    * array of int32_t * nummiptex to fit in the lump after the
    * 4-byte nummiptex header. */
   if (m->nummiptex < 0
       || (long long)m->nummiptex
              > ((long long)l->filelen - (long long)sizeof(int32_t))
                / (long long)sizeof(int32_t))
      SV_Error("%s: bad nummiptex %d (lump size %d)", __func__,
               m->nummiptex, l->filelen);

   loadmodel->numtextures = m->nummiptex;
   loadmodel->textures = (texture_t**)Hunk_Alloc(m->nummiptex * sizeof(*loadmodel->textures));

   for (i = 0; i < m->nummiptex; i++)
   {
      m->dataofs[i] = LittleLong(m->dataofs[i]);
      if (m->dataofs[i] == -1)
         continue;
      /* dataofs[i] is a byte offset within the texture lump.
       * Require room for at least the miptex_t header. */
      if (m->dataofs[i] < 0
          || (long long)m->dataofs[i] + (long long)sizeof(miptex_t)
                 > (long long)l->filelen)
         SV_Error("%s: bad dataofs[%d] = %d (lump size %d)",
                  __func__, i, m->dataofs[i], l->filelen);
      mt = (miptex_t *)((byte *)m + m->dataofs[i]);
      mt->width = (uint32_t)LittleLong(mt->width);
      mt->height = (uint32_t)LittleLong(mt->height);
      for (j = 0; j < MIPLEVELS; j++)
         mt->offsets[j] = (uint32_t)LittleLong(mt->offsets[j]);

      if ((mt->width & 15) || (mt->height & 15))
         SV_Error("Texture %s is not 16 aligned", mt->name);
      /* width, height are uint32_t.  pixels = w*h/64*85 (the
       * mip-chain expansion factor) must not overflow.  Reject
       * any combination that would produce a >= INT_MAX pixel
       * count, which would also fail Hunk_Alloc cleanly but
       * only after a memcpy-sized read past the lump. */
      if (mt->width == 0 || mt->height == 0
          || mt->width > 32768 || mt->height > 32768
          || (uint64_t)mt->width * (uint64_t)mt->height
                 > (uint64_t)INT_MAX / 85 * 64)
         SV_Error("%s: texture %s has bad dimensions %ux%u",
                  __func__, mt->name, mt->width, mt->height);
      pixels = (int)((uint64_t)mt->width * (uint64_t)mt->height / 64 * 85);
      /* The miptex header plus its mip chain occupies
       * sizeof(miptex_t) + pixels bytes in the lump; require
       * that range to fit. */
      if ((long long)m->dataofs[i] + (long long)sizeof(miptex_t)
              + (long long)pixels > (long long)l->filelen)
         SV_Error("%s: texture %s pixels (%d) extend past lump end",
                  __func__, mt->name, pixels);
      tx = (texture_t*)Hunk_Alloc(sizeof(texture_t) + pixels);
      loadmodel->textures[i] = tx;

      memcpy(tx->name, mt->name, sizeof(tx->name));
      tx->width = mt->width;
      tx->height = mt->height;
      /* mt->offsets[j] are file-controlled byte offsets pointing
       * from the miptex_t header to each of the four mip levels.
       * The renderer (D_DrawSurface, R_DrawSubdividedSurfaces,
       * R_InitSky's memcpy-into-tx) consumes the post-translation
       * tx->offsets[j] as 'data = (byte*)tx + tx->offsets[j]' and
       * walks the resulting pointer mip_w*mip_h bytes deep. A
       * hostile or corrupt BSP can set any of the four offsets to
       * a value outside the [sizeof(miptex_t),
       * sizeof(miptex_t)+pixels) region we just memcpy'd, and the
       * renderer reads (or, via R_InitSky, writes through) a wild
       * pointer.
       *
       * Validate per level: the level's mip is (w >> j) * (h >> j)
       * bytes (textures are required multiples of 16 so the
       * right-shifts stay >= 2 for j up to 3, no max(1,) fallback
       * needed).  Both the start of the level and its end must lie
       * within the loaded mip range. */
      for (j = 0; j < MIPLEVELS; j++) {
         uint32_t mip_w = mt->width >> j;
         uint32_t mip_h = mt->height >> j;
         uint64_t mip_size = (uint64_t)mip_w * (uint64_t)mip_h;
         if (mt->offsets[j] < sizeof(miptex_t)
             || (uint64_t)mt->offsets[j] + mip_size
                  > (uint64_t)sizeof(miptex_t) + (uint64_t)pixels)
            SV_Error("%s: texture %s mip %d offset %u out of "
                  "range [%zu, %zu) in %s",
                  __func__, mt->name, j,
                  (unsigned)mt->offsets[j],
                  sizeof(miptex_t),
                  (size_t)(sizeof(miptex_t) + pixels),
                  loadmodel->name);
      }
      for (j = 0; j < MIPLEVELS; j++)
         tx->offsets[j] =
            mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
      /* the pixels immediately follow the structures */
      memcpy(tx + 1, mt + 1, pixels);

      if (!strncmp(mt->name, "sky", 3))
         R_InitSky(tx);
   }

   /**/
   /* sequence the animations */
   /**/
   for (i = 0; i < m->nummiptex; i++) {
      tx = loadmodel->textures[i];
      if (!tx || tx->name[0] != '+')
         continue;
      if (tx->anim_next)
         continue;		/* allready sequenced */

      /* find the number of frames in the animation */
      memset(anims, 0, sizeof(anims));
      memset(altanims, 0, sizeof(altanims));

      max = tx->name[1];
      if (max >= 'a' && max <= 'z')
         max -= 'a' - 'A';
      if (max >= '0' && max <= '9') {
         max -= '0';
         altmax = 0;
         anims[max] = tx;
         max++;
      } else if (max >= 'A' && max <= 'J') {
         altmax = max - 'A';
         max = 0;
         altanims[altmax] = tx;
         altmax++;
      } else
         SV_Error("Bad animating texture %s", tx->name);

      for (j = i + 1; j < m->nummiptex; j++) {
         tx2 = loadmodel->textures[j];
         if (!tx2 || tx2->name[0] != '+')
            continue;
         if (strcmp(tx2->name + 2, tx->name + 2))
            continue;

         num = tx2->name[1];
         if (num >= 'a' && num <= 'z')
            num -= 'a' - 'A';
         if (num >= '0' && num <= '9') {
            num -= '0';
            anims[num] = tx2;
            if (num + 1 > max)
               max = num + 1;
         } else if (num >= 'A' && num <= 'J') {
            num = num - 'A';
            altanims[num] = tx2;
            if (num + 1 > altmax)
               altmax = num + 1;
         } else
            SV_Error("Bad animating texture %s", tx->name);
      }

#define	ANIM_CYCLE	2
      /* link them all together */
      for (j = 0; j < max; j++) {
         tx2 = anims[j];
         if (!tx2)
            SV_Error("Missing frame %i of %s", j, tx->name);
         tx2->anim_total = max * ANIM_CYCLE;
         tx2->anim_min = j * ANIM_CYCLE;
         tx2->anim_max = (j + 1) * ANIM_CYCLE;
         tx2->anim_next = anims[(j + 1) % max];
         if (altmax)
            tx2->alternate_anims = altanims[0];
      }
      for (j = 0; j < altmax; j++) {
         tx2 = altanims[j];
         if (!tx2)
            SV_Error("Missing frame %i of %s", j, tx->name);
         tx2->anim_total = altmax * ANIM_CYCLE;
         tx2->anim_min = j * ANIM_CYCLE;
         tx2->anim_max = (j + 1) * ANIM_CYCLE;
         tx2->anim_next = altanims[(j + 1) % altmax];
         if (max)
            tx2->alternate_anims = anims[0];
      }
   }
}

/*
=================
Mod_LoadLighting
=================
*/


static void
Mod_LoadLighting(lump_t *l)
{
	int		i;
	byte	*data;
	char	litname[1024];
	byte 	*lightmapfile;

	/* Sync coloredlights to the user's cvar choice at map-load time.
	 * The lightdata format (grayscale vs RGB) is determined here for
	 * the lifetime of this map, so all hot-path consumers can keep
	 * reading the int directly without a per-pixel cvar lookup. */
	coloredlights = (r_coloredlight.value != 0.0f) ? 1 : 0;

	if (!l->filelen) {
		loadmodel->lightdata = NULL;
		loadmodel->lightdatasize = 0;
		return;
	}

	if (coloredlights)	/* look for a .lit file to load */
	{
		strlcpy(litname, loadmodel->name, sizeof(litname));
		COM_StripExtension(litname);
		COM_DefaultExtension(litname, sizeof(litname), ".lit");
		lightmapfile = COM_LoadHunkFile(litname);
		if (lightmapfile)
		{
			/* The .lit format: 8-byte header
			 *   bytes 0..3: ASCII "QLIT"
			 *   bytes 4..7: int32 version (LittleLong, == 1)
			 * followed by 3 * (number of bsp lightmap samples)
			 * bytes of RGB pixel data.  com_filesize is set by
			 * COM_LoadHunkFile.  Reject any file too short to
			 * contain even the header so the reads below stay
			 * inside the allocation. */
			if (com_filesize < 8) {
				Con_Printf("Truncated .LIT file (size %i), ignoring\n", com_filesize);
			} else
			{
			data = lightmapfile;	
			if (data[0] == 'Q' && data[1] == 'L' && data[2] == 'I' && data[3] == 'T')
			{
				i = LittleLong(((int *)data)[1]);
				if (i == 1)
				{
					/* A .lit file is a sidecar replacement for the
					 * BSP's grayscale lightdata lump: 1 byte per
					 * sample in the BSP becomes 3 RGB bytes here.
					 * If the payload size doesn't match the BSP's
					 * expectation, individual lightofs values stored
					 * in the BSP's surfaces still point inside the
					 * .lit allocation (the per-surface bounds check
					 * later in Mod_LoadFaces enforces that), but
					 * they alias to the WRONG samples -- the .lit's
					 * pixel layout is keyed to a particular BSP, so
					 * a mismatched-size sidecar gives every surface
					 * a deterministic-but-wrong RGB triplet. Visual
					 * symptom is whole-room colour shifts that
					 * don't bisect to any code change. Reject and
					 * fall through to the grayscale->RGB expansion
					 * of the BSP's own lightdata. */
					if ((long long)com_filesize - 8
					    != (long long)l->filelen * 3) {
						Con_Printf("Mismatched .LIT file size "
						           "(%i, expected %i), "
						           "ignoring\n",
						           com_filesize - 8,
						           l->filelen * 3);
					} else {
						loadmodel->lightdata = data + 8;	
						/* The .lit payload starts after the 8-byte
						 * QLIT header. */
						loadmodel->lightdatasize = com_filesize - 8;
						return;
					}
				}
				else
					Con_Printf("Unknown .LIT file version (%d)\n", i);
			}
			else
				Con_Printf("Corrupt .LIT file (old version?), ignoring\n");
			}

		}
		else
		{
		/* expand the mono lighting to 24 bit */
			int i;
			byte *dest, *src = mod_base + l->fileofs;
			/* l->filelen is bounded by the BSP loader's input-size
			 * check, but l->filelen * 3 must still be checked for
			 * int overflow against INT_MAX before passing to the
			 * Hunk_Alloc int parameter. */
			if (l->filelen > 0 && (size_t)l->filelen > (size_t)INT_MAX / 3)
				SV_Error("%s: lightdata too large in %s", __func__, loadmodel->name);
			loadmodel->lightdata = Hunk_Alloc( l->filelen*3);
			loadmodel->lightdatasize = l->filelen * 3;
			dest = loadmodel->lightdata;
			for (i = 0; i<l->filelen; i++)
			{
				dest[0] = *src;
				dest[1] = *src;
				dest[2] = *src;

				src++;
				dest+=3;
		
			}
				
	
		}
	}
	else		/* mono lights */
	{
	    loadmodel->lightdata = (byte*)Hunk_Alloc(l->filelen);
	    loadmodel->lightdatasize = l->filelen;
	    memcpy(loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
	}
}


/*
=================
Mod_LoadVisibility
=================
*/
static void
Mod_LoadVisibility(lump_t *l)
{
    if (!l->filelen) {
	loadmodel->visdata = NULL;
	loadmodel->visdatasize = 0;
	return;
    }
    loadmodel->visdata = (byte*)Hunk_Alloc(l->filelen);
    loadmodel->visdatasize = l->filelen;
    memcpy(loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
static void
Mod_LoadEntities(lump_t *l)
{
    if (!l->filelen) {
	loadmodel->entities = NULL;
	return;
    }
    /* Reserve +1 for an explicit NUL terminator.  COM_Parse
     * (the consumer in ED_LoadFromFile) walks the entity
     * string until *data == 0; if the file's entity lump
     * happens to contain no trailing NUL within filelen
     * bytes, the parser walks past the allocation into the
     * next hunk lump (typically submodels).  qbsp-produced
     * BSPs include a NUL but a malformed or hostile BSP
     * could omit it. */
    loadmodel->entities = (char*)Hunk_Alloc(l->filelen + 1);
    memcpy(loadmodel->entities, mod_base + l->fileofs, l->filelen);
    loadmodel->entities[l->filelen] = 0;
}


/*
=================
Mod_LoadVertexes
=================
*/
static void
Mod_LoadVertexes(lump_t *l)
{
   dvertex_t *in;
   mvertex_t *out;
   int i, count;

   in = (dvertex_t*)(void *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   out = (mvertex_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->vertexes = out;
   loadmodel->numvertexes = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      out->position[0] = LittleFloat(in->point[0]);
      out->position[1] = LittleFloat(in->point[1]);
      out->position[2] = LittleFloat(in->point[2]);
      /* Vertex coordinates are loaded raw from the BSP as
       * float bit patterns (LittleFloat preserves the
       * pattern); a malformed or hostile file can put
       * NaN / Inf here.  Bad positions propagate into
       * CalcSurfaceExtents (caught there by IS_NAN on
       * mins/maxs), into SV_RecursiveHullCheck via brush
       * collision (caught at the SV_Move boundary), and
       * into renderer transforms.  All three downstream
       * paths recover, but each has to re-discover the
       * problem and each does it a little differently.
       * Reject at the loader so every consumer can trust
       * that loadmodel->vertexes[i].position is finite.
       * Same defense pattern as the plane normal/dist
       * check in Mod_LoadPlanes and the submodel
       * mins/maxs/origin check in Mod_LoadSubmodels. */
      if (IS_NAN(out->position[0]) || IS_NAN(out->position[1]) ||
          IS_NAN(out->position[2]))
         SV_Error("%s: non-finite vertex %d in %s",
                  __func__, i, loadmodel->name);
   }
}

/*
=================
Mod_LoadSubmodels
=================
*/
static void
Mod_LoadSubmodels(lump_t *l)
{
   dmodel_t *in;
   dmodel_t *out;
   int i, j, count;

   in = (dmodel_t*)(void *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   out = (dmodel_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->submodels = out;
   loadmodel->numsubmodels = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      for (j = 0; j < 3; j++)
      {	/* spread the mins / maxs by a pixel */
         out->mins[j]   = LittleFloat(in->mins[j]) - 1;
         out->maxs[j]   = LittleFloat(in->maxs[j]) + 1;
         out->origin[j] = LittleFloat(in->origin[j]);
         /* mins/maxs/origin are the entity bounding box and
          * spawn origin for every cl.model_precache submodel
          * (movers, doors, plats, world brushes parented to
          * func_* entities). They flow straight into
          * cl_entities[].origin and bbox tests in the client
          * and into edict_t->v.mins / maxs / origin and the
          * server's collision routines. A NaN/Inf here makes
          * the entity's bbox unbounded -- NaN-comparisons all
          * return false so SV_AreaEdicts can't decide whether
          * the entity overlaps the trace bounds, and the entity
          * gets included or excluded inconsistently across
          * traces. Reject the BSP at load. */
         if (IS_NAN(out->mins[j]) || IS_NAN(out->maxs[j]) ||
               IS_NAN(out->origin[j]))
            SV_Error("%s: non-finite submodel bbox in %s",
                  __func__, loadmodel->name);
      }
      for (j = 0; j < MAX_MAP_HULLS; j++)
      {
         out->headnode[j] = LittleLong(in->headnode[j]);
      }
      out->visleafs  = LittleLong(in->visleafs);
      out->firstface = LittleLong(in->firstface);
      out->numfaces  = LittleLong(in->numfaces);

      /* headnode[0] is the root mnode_t for hull 0, used by
       * the renderer's BSP traversal.  headnode[1..] feed
       * mclipnode_t-based collision hulls.  Mod_LoadBrushModel
       * later copies these unchecked into mod->hulls[j].
       * firstclipnode, so any out-of-range value here turns
       * into a stale pointer the moment the player moves. */
      if (out->headnode[0] < 0 || out->headnode[0] >= loadmodel->numnodes)
         SV_Error("%s: bad submodel headnode[0] %i (numnodes=%i) in %s",
                  __func__, out->headnode[0], loadmodel->numnodes,
                  loadmodel->name);
      for (j = 1; j < MAX_MAP_HULLS; j++) {
         /* Hull 0 is the rendered BSP, hulls 1+ are clipnode
          * hulls.  An unused hull slot is conventionally 0;
          * accept that.  Any other negative value, or anything
          * past numclipnodes, is corrupt. */
         if (out->headnode[j] < 0 || out->headnode[j] >= loadmodel->numclipnodes)
            SV_Error("%s: bad submodel headnode[%i] %i (numclipnodes=%i) in %s",
                     __func__, j, out->headnode[j], loadmodel->numclipnodes,
                     loadmodel->name);
      }
      /* firstface + numfaces feed the surface walk for this
       * submodel; out-of-range walks past the surfaces hunk
       * allocation. */
      if (out->firstface < 0 ||
          out->firstface > loadmodel->numsurfaces ||
          out->numfaces < 0 ||
          out->numfaces > loadmodel->numsurfaces - out->firstface)
         SV_Error("%s: bad submodel face range (first=%i, num=%i; "
                  "numsurfaces=%i) in %s",
                  __func__, out->firstface, out->numfaces,
                  loadmodel->numsurfaces, loadmodel->name);
   }
}

/*
=================
Mod_LoadEdges
 => Two versions for the different BSP file formats
=================
*/
static void
Mod_LoadEdges_BSP29(lump_t *l)
{
   bsp29_dedge_t *in;
   medge_t *out;
   int i, count;

   in = (bsp29_dedge_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 1, sizeof(*out), __func__, loadmodel->name);
   out = (medge_t*)Hunk_Alloc((count + 1) * sizeof(*out));

   loadmodel->edges = out;
   loadmodel->numedges = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      out->v[0] = (uint16_t)LittleShort(in->v[0]);
      out->v[1] = (uint16_t)LittleShort(in->v[1]);
      /* Edge endpoints are indices into loadmodel->vertexes[].
       * The renderer indexes vertexes[] without rebounds-checking,
       * so a corrupt or malicious BSP can drive that read off
       * the end of the hunk-allocated array.  Fail at load time. */
      if (out->v[0] >= (uint32_t)loadmodel->numvertexes ||
          out->v[1] >= (uint32_t)loadmodel->numvertexes)
         SV_Error("%s: bad vertex index (%u, %u; numvertexes=%i) in %s",
                  __func__, (unsigned)out->v[0], (unsigned)out->v[1],
                  loadmodel->numvertexes, loadmodel->name);
   }
}

static void
Mod_LoadEdges_BSP2(lump_t *l)
{
   bsp2_dedge_t *in;
   medge_t *out;
   int i, count;

   in = (bsp2_dedge_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 1, sizeof(*out), __func__, loadmodel->name);
   out = (medge_t*)Hunk_Alloc((count + 1) * sizeof(*out));

   loadmodel->edges = out;
   loadmodel->numedges = count;

   for (i = 0; i < count; i++, in++, out++) {
      out->v[0] = (uint32_t)LittleLong(in->v[0]);
      out->v[1] = (uint32_t)LittleLong(in->v[1]);
      /* See Mod_LoadEdges_BSP29 above. */
      if (out->v[0] >= (uint32_t)loadmodel->numvertexes ||
          out->v[1] >= (uint32_t)loadmodel->numvertexes)
         SV_Error("%s: bad vertex index (%u, %u; numvertexes=%i) in %s",
                  __func__, (unsigned)out->v[0], (unsigned)out->v[1],
                  loadmodel->numvertexes, loadmodel->name);
   }
}

/*
=================
Mod_LoadTexinfo
=================
*/
static void Mod_LoadTexinfo(lump_t *l)
{
   texinfo_t *in;
   mtexinfo_t *out;
   int i, j, count;
   int miptex;
   float len1, len2;

   in = (texinfo_t*)(void *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (mtexinfo_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->texinfo = out;
   loadmodel->numtexinfo = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      for (j = 0; j < 4; j++)
      {
         out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
         out->vecs[1][j] = LittleFloat(in->vecs[1][j]);
      }
      len1 = Length(out->vecs[0]);
      len2 = Length(out->vecs[1]);
      len1 = (len1 + len2) / 2;
      if (len1 < 0.32)
         out->mipadjust = 4;
      else if (len1 < 0.49)
         out->mipadjust = 3;
      else if (len1 < 0.99)
         out->mipadjust = 2;
      else
         out->mipadjust = 1;

      miptex     = LittleLong(in->miptex);
      out->flags = LittleLong(in->flags);

      if (!loadmodel->textures) {
         out->texture = r_notexture_mip;	/* checkerboard texture */
         out->flags = 0;
      } else {
         if (miptex >= loadmodel->numtextures)
            SV_Error("miptex >= loadmodel->numtextures");
         out->texture = loadmodel->textures[miptex];
         if (!out->texture) {
            out->texture = r_notexture_mip;	/* texture not found */
            out->flags = 0;
         }
      }
   }
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
static void
CalcSurfaceExtents(msurface_t *s)
{
    float mins[2], maxs[2], val;
    int i, j, e;
    mvertex_t *v;
    mtexinfo_t *tex;
    int bmins[2], bmaxs[2];

    mins[0] = mins[1] = FLT_MAX;
    maxs[0] = maxs[1] = -FLT_MAX;

    tex = s->texinfo;

    for (i = 0; i < s->numedges; i++) {
	e = loadmodel->surfedges[s->firstedge + i];
	if (e >= 0)
	    v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
	else
	    v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

/*
	 * The (long double) casts below are important: The original code was
	 * written for x87 floating-point which uses 80-bit floats for
	 * intermediate calculations. But if you compile it without the casts
	 * for modern x86_64, the compiler will round each intermediate result
	 * to a 32-bit float, which introduces extra rounding error.
	 *
	 * This becomes a problem if the rounding error causes the light
	 * utilities and the engine to disagree about the lightmap size for
	 * some surfaces.
	 *
	 * Casting to (long double) keeps the intermediate values at at least
	 * 64 bits of precision, probably 128.
	 */

	for (j = 0; j < 2; j++) {
       val =
		(long double)v->position[0] * tex->vecs[j][0] +
		(long double)v->position[1] * tex->vecs[j][1] +
		(long double)v->position[2] * tex->vecs[j][2] +
		                                   tex->vecs[j][3];
	    if (val < mins[j])
		mins[j] = val;
	    if (val > maxs[j])
		maxs[j] = val;
	}
    }

    for (i = 0; i < 2; i++) {
	/* mins[i] / maxs[i] are accumulated from vertex positions
	 * dotted with texinfo vecs; vertex positions and tex->vecs
	 * are loaded raw from the BSP (LittleFloat -- bit pattern
	 * preserved).  A NaN / Inf there propagates into mins/maxs
	 * here, and floorf(NaN) is NaN.  NaN converted to int is
	 * undefined behaviour and typically yields INT_MIN; the
	 * subsequent (bmaxs-bmins)*16 then signed-overflows.  The
	 * extents bound below catches positive overflow but a
	 * NaN-derived junk value can land coincidentally in
	 * [0, 256].  Reject early. */
	if (IS_NAN((float)mins[i]) || IS_NAN((float)maxs[i]))
	    SV_Error("Bad surface extents (NaN/Inf vertex position)");

	bmins[i] = floorf(mins[i] / 16);
	bmaxs[i] = ceilf(maxs[i] / 16);

	s->texturemins[i] = bmins[i] * 16;
	s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	/* Vanilla allowed TEX_SPECIAL surfaces to skip the
	 * extents bound because in the original engine those
	 * were always sky or liquid surfaces, which the loader
	 * later overrode with extents = 16384 and routed through
	 * the SURF_DRAWSKY / SURF_DRAWTURB code paths
	 * (D_DrawSkyScans8 / Turbulent8 -- not lightmapped).
	 * However, the SURF_DRAWSKY / SURF_DRAWTURB routing is
	 * decided by texture-name prefix ("sky"/"*"), not the
	 * TEX_SPECIAL flag.  A hostile BSP with TEX_SPECIAL set
	 * on a normally-named texture and absurd vertex positions
	 * makes this surface enter R_DrawSurface ->
	 * R_BuildLightMap with smax*tmax far exceeding the
	 * size of the static blocklights[18*18*3] buffer,
	 * producing an OOB write.
	 *
	 * Bound regardless of TEX_SPECIAL: an extent of 256
	 * means smax = (256>>4)+1 = 17, so 17*17*3 = 867 fits
	 * comfortably in blocklights[972].  The 16384-extent
	 * override at Mod_LoadFaces still happens for water
	 * surfaces, but those are routed away from the
	 * lightmap path by the texture-name check. */
	if (s->extents[i] < 0 || s->extents[i] > 256)
	    SV_Error("Bad surface extents (%d)", s->extents[i]);
    }
}

static void
CalcSurfaceBounds(msurface_t *surf)
{
    int i, j, edgenum;
    medge_t *edge;
    mvertex_t *v;

    surf->mins[0] = surf->mins[1] = surf->mins[2] = FLT_MAX;
    surf->maxs[0] = surf->maxs[1] = surf->maxs[2] = -FLT_MAX;

    for (i = 0; i < surf->numedges; i++) {
	edgenum = loadmodel->surfedges[surf->firstedge + i];
	if (edgenum >= 0) {
	    edge = &loadmodel->edges[edgenum];
	    v = &loadmodel->vertexes[edge->v[0]];
	} else {
	    edge = &loadmodel->edges[-edgenum];
	    v = &loadmodel->vertexes[edge->v[1]];
	}

	for (j = 0; j < 3; j++) {
	    if (surf->mins[j] > v->position[j])
		surf->mins[j] = v->position[j];
	    if (surf->maxs[j] < v->position[j])
		surf->maxs[j] = v->position[j];
	}
    }
}

/*
=================
Mod_LoadFaces
 => Two versions for the different BSP file formats
=================
*/
static void
Mod_LoadFaces_BSP29(lump_t *l)
{
   bsp29_dface_t *in;
   msurface_t *out;
   int i, count, surfnum;
   int planenum, side;

   in = (bsp29_dface_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (msurface_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->surfaces = out;
   loadmodel->numsurfaces = count;

   for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
   {
      out->firstedge = LittleLong(in->firstedge);
      out->numedges  = LittleShort(in->numedges);
      out->flags = 0;

      /* firstedge / numedges feed the surface's vertex walk
       * via loadmodel->surfedges[].  An out-of-range value
       * here lets renderer code read past the surfedges hunk
       * allocation. */
      if (out->numedges <= 0)
         SV_Error("%s: bmodel %s has surface with no edges", __func__,
               loadmodel->name);
      if (out->firstedge < 0 ||
          out->firstedge > loadmodel->numsurfedges ||
          out->numedges > loadmodel->numsurfedges - out->firstedge)
         SV_Error("%s: bad surface edge range (first=%i, num=%i; "
                  "numsurfedges=%i) in %s",
                  __func__, out->firstedge, out->numedges,
                  loadmodel->numsurfedges, loadmodel->name);

      planenum = LittleShort(in->planenum);
      side     = LittleShort(in->side);
      if (side)
         out->flags |= SURF_PLANEBACK;

      /* Defensive: planenum and texinfo come from the BSP
       * file as raw shorts.  A truncated, corrupt, or
       * malicious map can leave them out of range -- the
       * resulting out->plane / out->texinfo points outside
       * the loaded arrays, and later renderer / collision
       * code dereferences garbage.  This is the surface-side
       * of the same crash class hardened on the consumer side
       * by the SV_HullPointContents null-guards (commit
       * 6042e45). */
      if (planenum < 0 || planenum >= loadmodel->numplanes)
         SV_Error("%s: bad planenum %i (numplanes=%i) in %s",
               __func__, planenum, loadmodel->numplanes,
               loadmodel->name);

      i = LittleShort(in->texinfo);
      if (i < 0 || i >= loadmodel->numtexinfo)
         SV_Error("%s: bad texinfo index %i (numtexinfo=%i) in %s",
               __func__, i, loadmodel->numtexinfo, loadmodel->name);

      out->plane = loadmodel->planes + planenum;
      out->texinfo = loadmodel->texinfo + i;

      CalcSurfaceExtents(out);
      CalcSurfaceBounds(out);

      /* lighting info */

      for (i = 0; i < MAXLIGHTMAPS; i++)
         out->styles[i] = in->styles[i];
      i = LittleLong(in->lightofs);
      /* lightofs is an int byte-offset into loadmodel->lightdata,
       * or -1 if the surface has no lighting.  Any other
       * out-of-range value would make out->samples point
       * outside the lightdata hunk allocation; the rasterizer
       * would then read uninitialised / unmapped memory.  In
       * the coloredlights branch the offset is also multiplied
       * by 3, so guard against integer overflow before the
       * pointer arithmetic. */
      if (i == -1)
      {
         out->samples = NULL;
      }
      else if (coloredlights)
      {
         if (i < 0 ||
             (size_t)i > (size_t)INT_MAX / 3 ||
             i * 3 >= loadmodel->lightdatasize)
            SV_Error("%s: bad lightofs %i (lightdatasize=%i) in %s",
                     __func__, i, loadmodel->lightdatasize, loadmodel->name);
         out->samples = loadmodel->lightdata + i * 3;
      }
      else
      {
         if (i < 0 || i >= loadmodel->lightdatasize)
            SV_Error("%s: bad lightofs %i (lightdatasize=%i) in %s",
                     __func__, i, loadmodel->lightdatasize, loadmodel->name);
         out->samples = loadmodel->lightdata + i;
      }

      /* set the surface drawing flags */
      if (!strncmp(out->texinfo->texture->name, "sky", 3)) {
         out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
      } else if (!strncmp(out->texinfo->texture->name, "*", 1)) {
         out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
         for (i = 0; i < 2; i++) {
            out->extents[i] = 16384;
            out->texturemins[i] = -8192;
         }
      }
   }
}

static void Mod_LoadFaces_BSP2(lump_t *l)
{
   msurface_t *out;
   int i, count, surfnum;
   int planenum, side;
   bsp2_dface_t *in = (bsp2_dface_t *)(mod_base + l->fileofs);

   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);

   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (msurface_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->surfaces = out;
   loadmodel->numsurfaces = count;

   for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
   {
      out->firstedge = LittleLong(in->firstedge);
      out->numedges  = LittleLong(in->numedges);
      out->flags     = 0;

      /* See Mod_LoadFaces_BSP29 above. */
      if (out->numedges <= 0)
         SV_Error("%s: bmodel %s has surface with no edges", __func__,
               loadmodel->name);
      if (out->firstedge < 0 ||
          out->firstedge > loadmodel->numsurfedges ||
          out->numedges > loadmodel->numsurfedges - out->firstedge)
         SV_Error("%s: bad surface edge range (first=%i, num=%i; "
                  "numsurfedges=%i) in %s",
                  __func__, out->firstedge, out->numedges,
                  loadmodel->numsurfedges, loadmodel->name);

      planenum       = LittleLong(in->planenum);
      side           = LittleLong(in->side);
      if (side)
         out->flags |= SURF_PLANEBACK;

      /* Defensive: see Mod_LoadFaces_BSP29 above. */
      if (planenum < 0 || planenum >= loadmodel->numplanes)
         SV_Error("%s: bad planenum %i (numplanes=%i) in %s",
               __func__, planenum, loadmodel->numplanes,
               loadmodel->name);

      i = LittleLong(in->texinfo);
      if (i < 0 || i >= loadmodel->numtexinfo)
         SV_Error("%s: bad texinfo index %i (numtexinfo=%i) in %s",
               __func__, i, loadmodel->numtexinfo, loadmodel->name);

      out->plane = loadmodel->planes + planenum;
      out->texinfo = loadmodel->texinfo + i;

      CalcSurfaceExtents(out);
      CalcSurfaceBounds(out);

      /* lighting info */

      for (i = 0; i < MAXLIGHTMAPS; i++)
         out->styles[i] = in->styles[i];
      i = LittleLong(in->lightofs);
      /* See Mod_LoadFaces_BSP29 above. */
      if (i == -1)
      {
         out->samples = NULL;
      }
      else if (coloredlights)
      {
         if (i < 0 ||
             (size_t)i > (size_t)INT_MAX / 3 ||
             i * 3 >= loadmodel->lightdatasize)
            SV_Error("%s: bad lightofs %i (lightdatasize=%i) in %s",
                     __func__, i, loadmodel->lightdatasize, loadmodel->name);
         out->samples = loadmodel->lightdata + i * 3;
      }
      else
      {
         if (i < 0 || i >= loadmodel->lightdatasize)
            SV_Error("%s: bad lightofs %i (lightdatasize=%i) in %s",
                     __func__, i, loadmodel->lightdatasize, loadmodel->name);
         out->samples = loadmodel->lightdata + i;
      }

      /* set the surface drawing flags */
      if (!strncmp(out->texinfo->texture->name, "sky", 3))
         out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
      else if (!strncmp(out->texinfo->texture->name, "*", 1))
      {
         out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
         for (i = 0; i < 2; i++)
         {
            out->extents[i] = 16384;
            out->texturemins[i] = -8192;
         }
      }
   }
}

/*
=================
Mod_SetParent
=================
*/
static void
Mod_SetParent(mnode_t *node, mnode_t *parent)
{
    node->parent = parent;
    if (node->contents < 0)
	return;
    Mod_SetParent(node->children[0], node);
    Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
 => Two versions for the different BSP file formats
=================
*/
static void
Mod_LoadNodes_BSP29(lump_t *l)
{
   int i, j, count, p;
   bsp29_dnode_t *in;
   mnode_t *out;

   in = (bsp29_dnode_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (mnode_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->nodes = out;
   loadmodel->numnodes = count;

   for (i = 0; i < count; i++, in++, out++) {
      for (j = 0; j < 3; j++) {
         out->mins[j] = LittleShort(in->mins[j]);
         out->maxs[j] = LittleShort(in->maxs[j]);
      }

      p = LittleLong(in->planenum);
      /* Defensive: planenum and child indices come from the
       * BSP file as raw integers.  Walked at runtime by the
       * BSP collision code; an out-of-range value yields a
       * pointer into garbage that crashes deep in
       * SV_PointContents / R_RecursiveWorldNode. */
      if (p < 0 || p >= loadmodel->numplanes)
         SV_Error("%s: bad node planenum %i (numplanes=%i) in %s",
               __func__, p, loadmodel->numplanes, loadmodel->name);
      out->plane = loadmodel->planes + p;

      out->firstsurface = (uint16_t)LittleShort(in->firstface);
      out->numsurfaces = (uint16_t)LittleShort(in->numfaces);
      /* firstsurface and numsurfaces are file-controlled
       * 16-bit values, walked by the renderer as
       *   surf = cl.worldmodel->surfaces + node->firstsurface;
       *   for (count = node->numsurfaces; count; count--, surf++)
       * (r_bsp.c R_RecursiveWorldNode, r_light.c lightmap walk).
       * Out-of-range values let that loop read past the
       * surfaces hunk allocation, dereferencing junk for
       * surf->visframe / surf->clipflags.  Both fields are
       * unsigned-narrowed at load (so individually <=
       * 0xffff) but their sum can still exceed numsurfaces.
       * Reject any node whose surface range walks past the
       * end of the surfaces array.  Order: this runs after
       * Mod_LoadFaces (which set loadmodel->numsurfaces), so
       * the comparison is well-defined. */
      if ((int)out->firstsurface > loadmodel->numsurfaces ||
          (int)out->numsurfaces > loadmodel->numsurfaces - (int)out->firstsurface)
         SV_Error("%s: bad node surface range (first=%u, num=%u; "
                  "numsurfaces=%i) in %s",
                  __func__, (unsigned)out->firstsurface,
                  (unsigned)out->numsurfaces,
                  loadmodel->numsurfaces, loadmodel->name);

      for (j = 0; j < 2; j++)
      {
         p = LittleShort(in->children[j]);
         if (p >= 0) {
            if (p >= loadmodel->numnodes)
               SV_Error("%s: bad node child index %i (numnodes=%i) in %s",
                     __func__, p, loadmodel->numnodes, loadmodel->name);
            out->children[j] = loadmodel->nodes + p;
         } else {
            int leafidx = -1 - p;
            if (leafidx < 0 || leafidx >= loadmodel->numleafs)
               SV_Error("%s: bad node leaf index %i (numleafs=%i) in %s",
                     __func__, leafidx, loadmodel->numleafs,
                     loadmodel->name);
            out->children[j] = (mnode_t *)(loadmodel->leafs + leafidx);
         }
      }
   }

   Mod_SetParent(loadmodel->nodes, NULL);	/* sets nodes and leafs */
}

static void Mod_LoadNodes_BSP2(lump_t *l)
{
   int i, count;
   mnode_t *out;
   bsp2_dnode_t *in = (bsp2_dnode_t *)(mod_base + l->fileofs);

   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);

   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out   = (mnode_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->nodes    = out;
   loadmodel->numnodes = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      int j, p;

      for (j = 0; j < 3; j++)
      {
         out->mins[j] = LittleShort(in->mins[j]);
         out->maxs[j] = LittleShort(in->maxs[j]);
      }

      p = LittleLong(in->planenum);
      /* Defensive: see Mod_LoadNodes_BSP29 above. */
      if (p < 0 || p >= loadmodel->numplanes)
         SV_Error("%s: bad node planenum %i (numplanes=%i) in %s",
               __func__, p, loadmodel->numplanes, loadmodel->name);
      out->plane = loadmodel->planes + p;

      out->firstsurface = (uint32_t)LittleLong(in->firstface);
      out->numsurfaces = (uint32_t)LittleLong(in->numfaces);
      /* See Mod_LoadNodes_BSP29 above.  BSP2 widens these to
       * uint32, so individually they can be up to 4G; the
       * sum-overflow / past-end check still applies. */
      if ((int)out->firstsurface > loadmodel->numsurfaces ||
          (int)out->numsurfaces > loadmodel->numsurfaces - (int)out->firstsurface)
         SV_Error("%s: bad node surface range (first=%u, num=%u; "
                  "numsurfaces=%i) in %s",
                  __func__, (unsigned)out->firstsurface,
                  (unsigned)out->numsurfaces,
                  loadmodel->numsurfaces, loadmodel->name);

      for (j = 0; j < 2; j++)
      {
         p = LittleLong(in->children[j]);
         if (p >= 0) {
            if (p >= loadmodel->numnodes)
               SV_Error("%s: bad node child index %i (numnodes=%i) in %s",
                     __func__, p, loadmodel->numnodes, loadmodel->name);
            out->children[j] = loadmodel->nodes + p;
         } else {
            int leafidx = -1 - p;
            if (leafidx < 0 || leafidx >= loadmodel->numleafs)
               SV_Error("%s: bad node leaf index %i (numleafs=%i) in %s",
                     __func__, leafidx, loadmodel->numleafs,
                     loadmodel->name);
            out->children[j] = (mnode_t *)(loadmodel->leafs + leafidx);
         }
      }
   }

   Mod_SetParent(loadmodel->nodes, NULL);	/* sets nodes and leafs */
}

/*
=================
Mod_LoadLeafs
 => Two versions for the different BSP file formats
=================
*/
static void
Mod_LoadLeafs_BSP29(lump_t *l)
{
   bsp29_dleaf_t *in;
   mleaf_t *out;
   int i, j, count, p;

   in = (bsp29_dleaf_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   /* numleafs feeds Mod_InitPVSCache which allocates
    * PVSCACHE_SIZE * Mod_LeafbitsSize(numleafs) bytes; at very
    * large counts the multiplication can stress the hunk.  The
    * BSP format reserves negative shorts for content codes, so
    * MAX_MAP_LEAFS is the natural upper bound. */
   if (count < 0 || count > MAX_MAP_LEAFS)
      SV_Error("%s: bad leaf count %i (max %i) in %s", __func__,
               count, MAX_MAP_LEAFS, loadmodel->name);
   out = (mleaf_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->leafs = out;
   loadmodel->numleafs = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      for (j = 0; j < 3; j++)
      {
         out->mins[j] = LittleShort(in->mins[j]);
         out->maxs[j] = LittleShort(in->maxs[j]);
      }

      p = LittleLong(in->contents);
      out->contents = p;

      {
         uint16_t fms = (uint16_t)LittleShort(in->firstmarksurface);
         uint16_t nms = (uint16_t)LittleShort(in->nummarksurfaces);
         /* fms+nms must lie within marksurfaces[]. */
         if ((int)fms > loadmodel->nummarksurfaces ||
             (int)nms > loadmodel->nummarksurfaces - (int)fms)
            SV_Error("%s: bad marksurface range (first=%u, num=%u; "
                     "nummarksurfaces=%i) in %s",
                     __func__, fms, nms, loadmodel->nummarksurfaces,
                     loadmodel->name);
         out->firstmarksurface = loadmodel->marksurfaces + fms;
         out->nummarksurfaces = nms;
      }

      p = LittleLong(in->visofs);
      /* visofs is a byte-offset into loadmodel->visdata, or -1
       * for "no PVS data".  Out-of-range values cause the PVS
       * decompressor (Mod_DecompressVis) to walk off the end
       * of the visdata hunk allocation. */
      if (p == -1)
         out->compressed_vis = NULL;
      else
      {
         if (p < 0 || p >= loadmodel->visdatasize)
            SV_Error("%s: bad visofs %i (visdatasize=%i) in %s",
                     __func__, p, loadmodel->visdatasize, loadmodel->name);
         out->compressed_vis = loadmodel->visdata + p;
      }
      out->efrags = NULL;

      for (j = 0; j < 4; j++)
         out->ambient_sound_level[j] = in->ambient_level[j];
   }
}

static void
Mod_LoadLeafs_BSP2(lump_t *l)
{
   bsp2_dleaf_t *in;
   mleaf_t *out;
   int i, j, count, p;

   in = (bsp2_dleaf_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   /* See Mod_LoadLeafs_BSP29 above. */
   if (count < 0 || count > MAX_MAP_LEAFS)
      SV_Error("%s: bad leaf count %i (max %i) in %s", __func__,
               count, MAX_MAP_LEAFS, loadmodel->name);
   out = (mleaf_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->leafs = out;
   loadmodel->numleafs = count;

   for (i = 0; i < count; i++, in++, out++) {
      for (j = 0; j < 3; j++) {
         out->mins[j] = LittleShort(in->mins[j]);
         out->maxs[j] = LittleShort(in->maxs[j]);
      }

      p = LittleLong(in->contents);
      out->contents = p;

      {
         uint32_t fms = (uint32_t)LittleLong(in->firstmarksurface);
         uint32_t nms = (uint32_t)LittleLong(in->nummarksurfaces);
         /* See Mod_LoadLeafs_BSP29 above. */
         if (fms > (uint32_t)loadmodel->nummarksurfaces ||
             nms > (uint32_t)loadmodel->nummarksurfaces - fms)
            SV_Error("%s: bad marksurface range (first=%u, num=%u; "
                     "nummarksurfaces=%i) in %s",
                     __func__, fms, nms, loadmodel->nummarksurfaces,
                     loadmodel->name);
         out->firstmarksurface = loadmodel->marksurfaces + fms;
         out->nummarksurfaces = nms;
      }

      p = LittleLong(in->visofs);

      if (p == -1)
         out->compressed_vis = NULL;
      else
      {
         if (p < 0 || p >= loadmodel->visdatasize)
            SV_Error("%s: bad visofs %i (visdatasize=%i) in %s",
                     __func__, p, loadmodel->visdatasize, loadmodel->name);
         out->compressed_vis = loadmodel->visdata + p;
      }
      out->efrags = NULL;

      for (j = 0; j < 4; j++)
         out->ambient_sound_level[j] = in->ambient_level[j];
   }
}

/*
=================
Mod_LoadClipnodes
 => Two versions for the different BSP file formats
=================
*/
static void
Mod_LoadClipnodes_BSP29(lump_t *l)
{
   bsp29_dclipnode_t *in;
   mclipnode_t *out;
   int i, j, count;
   hull_t *hull;

   in = (bsp29_dclipnode_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (mclipnode_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->clipnodes = out;
   loadmodel->numclipnodes = count;

   hull = &loadmodel->hulls[1];
   hull->clipnodes = out;
   hull->firstclipnode = 0;
   hull->lastclipnode = count - 1;
   hull->planes = loadmodel->planes;
   hull->clip_mins[0] = -16;
   hull->clip_mins[1] = -16;
   hull->clip_mins[2] = -24;
   hull->clip_maxs[0] = 16;
   hull->clip_maxs[1] = 16;
   hull->clip_maxs[2] = 32;

   hull = &loadmodel->hulls[2];
   hull->clipnodes = out;
   hull->firstclipnode = 0;
   hull->lastclipnode = count - 1;
   hull->planes = loadmodel->planes;
   hull->clip_mins[0] = -32;
   hull->clip_mins[1] = -32;
   hull->clip_mins[2] = -24;
   hull->clip_maxs[0] = 32;
   hull->clip_maxs[1] = 32;
   hull->clip_maxs[2] = 64;

   for (i = 0; i < count; i++, out++, in++)
   {
      out->planenum = LittleLong(in->planenum);
      /* Defensive: planenum dereferenced as
       * hull->planes[node->planenum] in the BSP traversal
       * (SV_HullPointContents).  Same crash class as the
       * face planenum check above. */
      if (out->planenum < 0 || out->planenum >= loadmodel->numplanes)
         SV_Error("%s: bad planenum %i (numplanes=%i) in %s",
               __func__, out->planenum, loadmodel->numplanes,
               loadmodel->name);
      for (j = 0; j < 2; j++) {
         out->children[j] = (uint16_t)LittleShort(in->children[j]);
         if (out->children[j] > 0xfff0)
            out->children[j] -= 0x10000;
         if (out->children[j] >= count)
            SV_Error("%s: bad clipnode child number", __func__);
      }
   }
}

static void
Mod_LoadClipnodes_BSP2(lump_t *l)
{
   bsp2_dclipnode_t *in;
   mclipnode_t *out;
   int i, j, count;
   hull_t *hull;

   in = (bsp2_dclipnode_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   out = (mclipnode_t*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->clipnodes = out;
   loadmodel->numclipnodes = count;

   hull = &loadmodel->hulls[1];
   hull->clipnodes = out;
   hull->firstclipnode = 0;
   hull->lastclipnode = count - 1;
   hull->planes = loadmodel->planes;
   hull->clip_mins[0] = -16;
   hull->clip_mins[1] = -16;
   hull->clip_mins[2] = -24;
   hull->clip_maxs[0] = 16;
   hull->clip_maxs[1] = 16;
   hull->clip_maxs[2] = 32;

   hull = &loadmodel->hulls[2];
   hull->clipnodes = out;
   hull->firstclipnode = 0;
   hull->lastclipnode = count - 1;
   hull->planes = loadmodel->planes;
   hull->clip_mins[0] = -32;
   hull->clip_mins[1] = -32;
   hull->clip_mins[2] = -24;
   hull->clip_maxs[0] = 32;
   hull->clip_maxs[1] = 32;
   hull->clip_maxs[2] = 64;

   for (i = 0; i < count; i++, out++, in++) {
      out->planenum = LittleLong(in->planenum);
      /* Defensive: see Mod_LoadClipnodes_BSP29 above. */
      if (out->planenum < 0 || out->planenum >= loadmodel->numplanes)
         SV_Error("%s: bad planenum %i (numplanes=%i) in %s",
               __func__, out->planenum, loadmodel->numplanes,
               loadmodel->name);
      for (j = 0; j < 2; j++) {
         out->children[j] = LittleLong(in->children[j]);
         if (out->children[j] >= count)
            SV_Error("%s: bad clipnode child number", __func__);
      }
   }
}

/*
=================
Mod_MakeHull0

Duplicate the drawing hull structure as a clipping hull
=================
*/
static void
Mod_MakeHull0(void)
{
    mnode_t *in, *child;
    mclipnode_t *out;
    int i, j, count;
    hull_t *hull;

    hull = &loadmodel->hulls[0];

    in = loadmodel->nodes;
    count = loadmodel->numnodes;
    out = (mclipnode_t*)Hunk_Alloc(count * sizeof(*out));

    hull->clipnodes = out;
    hull->firstclipnode = 0;
    hull->lastclipnode = count - 1;
    hull->planes = loadmodel->planes;

    for (i = 0; i < count; i++, out++, in++) {
	out->planenum = in->plane - loadmodel->planes;
	for (j = 0; j < 2; j++) {
	    child = in->children[j];
	    if (child->contents < 0)
		out->children[j] = child->contents;
	    else
		out->children[j] = child - loadmodel->nodes;
	}
    }
}

/*
=================
Mod_LoadMarksurfaces
 => Two versions for the different BSP file formats
=================
*/
static void
Mod_LoadMarksurfaces_BSP29(lump_t *l)
{
   int i, j, count;
   uint16_t *in;
   msurface_t **out;

   in = (uint16_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (msurface_t**)Hunk_Alloc(count * sizeof(*out));

   loadmodel->marksurfaces = out;
   loadmodel->nummarksurfaces = count;

   for (i = 0; i < count; i++)
   {
      j = (uint16_t)LittleShort(in[i]);
      if (j >= loadmodel->numsurfaces)
         SV_Error("%s: bad surface number", __func__);
      out[i] = loadmodel->surfaces + j;
   }
}

static void
Mod_LoadMarksurfaces_BSP2(lump_t *l)
{
   int i, j, count;
   uint32_t *in;
   msurface_t **out;

   in = (uint32_t *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   Mod_BoundLoaderCount(count, 0, sizeof(*out), __func__, loadmodel->name);
   out = (msurface_t**)Hunk_Alloc(count * sizeof(*out));

   loadmodel->marksurfaces = out;
   loadmodel->nummarksurfaces = count;

   for (i = 0; i < count; i++) {
      j = (uint32_t)LittleLong(in[i]);
      if (j >= loadmodel->numsurfaces)
         SV_Error("%s: bad surface number", __func__);
      out[i] = loadmodel->surfaces + j;
   }
}

/*
=================
Mod_LoadSurfedges
=================
*/
static void
Mod_LoadSurfedges(lump_t *l)
{
   int i, count;
   int *in, *out;

   in = (int*)(void *)(mod_base + l->fileofs);
   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);
   count = l->filelen / sizeof(*in);
   out = (int*)Hunk_Alloc(count * sizeof(*out));

   loadmodel->surfedges = out;
   loadmodel->numsurfedges = count;

   for (i = 0; i < count; i++) {
      out[i] = LittleLong(in[i]);
      /* surfedges are signed indices into edges[]: positive
       * value means walk edge[i].v[0]->v[1], negative means
       * walk edge[-i].v[1]->v[0].  Out-of-range values cause
       * the renderer's R_RecursiveWorldNode / R_RenderFace
       * lookup of edges[abs(out[i])] to walk off the end of
       * the edges hunk allocation.  Be defensive: also catch
       * INT_MIN, whose abs() is undefined. */
      {
         int e = out[i];
         int a = (e < 0) ? -e : e;
         if (e == INT_MIN || a >= loadmodel->numedges)
            SV_Error("%s: bad surfedge %i (numedges=%i) in %s",
                     __func__, e, loadmodel->numedges, loadmodel->name);
      }
   }
}

/*
=================
Mod_LoadPlanes
=================
*/
static void Mod_LoadPlanes(lump_t *l)
{
   int i, j, count;
   mplane_t *out;
   dplane_t *in = (dplane_t*)(void *)(mod_base + l->fileofs);

   if (l->filelen % sizeof(*in))
      SV_Error("%s: funny lump size in %s", __func__, loadmodel->name);

   count = l->filelen / sizeof(*in);
   /* Mod_LoadPlanes allocates 2 mplane_t per dplane_t (used by the
    * server-side clipping path).  Pass 2 * sizeof(*out) as the
    * effective per-record size to the bound check. */
   Mod_BoundLoaderCount(count, 0, 2 * sizeof(*out), __func__, loadmodel->name);
   out   = (mplane_t*)
      Hunk_Alloc(count * 2 * sizeof(*out));

   loadmodel->planes    = out;
   loadmodel->numplanes = count;

   for (i = 0; i < count; i++, in++, out++)
   {
      int bits = 0;
      for (j = 0; j < 3; j++)
      {
         out->normal[j] = LittleFloat(in->normal[j]);
         if (out->normal[j] < 0)
            bits |= 1 << j;
      }

      out->dist = LittleFloat(in->dist);
      out->type = LittleLong(in->type);
      out->signbits = bits;

      /* Plane normals and distances feed BSP traversal
       * (SV_RecursiveHullCheck, SV_HullPointContents) and
       * visibility / PVS culling. A NaN/Inf in normal[] or dist
       * collapses every 'DotProduct(plane->normal, p) - plane->
       * dist' on either side of zero (NaN comparisons all return
       * false), so PointContents returns CONTENTS_SOLID
       * unconditionally for one side and CONTENTS_EMPTY for the
       * other depending on which branch the traversal takes.
       * Collision then either lets the player walk through every
       * brush or wedges them against the first plane they touch.
       * 'normal[j] < 0' above also missed the NaN case before
       * the signbits computation, so signbits is wrong as well.
       * Reject the BSP. */
      if (IS_NAN(out->normal[0]) || IS_NAN(out->normal[1]) ||
            IS_NAN(out->normal[2]) || IS_NAN(out->dist))
         SV_Error("%s: non-finite plane in %s", __func__,
               loadmodel->name);
   }
}

/*
=================
RadiusFromBounds
=================
*/
static float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
   int i;
   vec3_t corner;

   for (i = 0; i < 3; i++)
      corner[i] = qmax(fabsf(mins[i]), fabsf(maxs[i]));

   return Length(corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
static void Mod_LoadBrushModel(model_t *mod, void *buffer, unsigned long size)
{
   int i, j;
   dheader_t *header;
   dmodel_t *bm;

   loadmodel->type = mod_brush;

   /* The header itself must fit in the loaded buffer before
    * we can byte-swap or read its fields.  Without this
    * check, a truncated or malformed BSP shorter than
    * sizeof(dheader_t) (typically 116 bytes) walks past
    * `buffer` during the LittleLong loop below, reading
    * uninitialised heap memory.  com_filesize is the
    * authority on the actual size; the `size` parameter is
    * the same value plumbed through Mod_LoadModel. */
   if (size < sizeof(dheader_t))
      SV_Error("%s: %s truncated (size %lu < %lu)", __func__,
               mod->name, size, (unsigned long)sizeof(dheader_t));

   header = (dheader_t *)buffer;

   /* swap all the header entries */
   header->version = LittleLong(header->version);
   for (i = 0; i < HEADER_LUMPS; i++) {
      header->lumps[i].fileofs = LittleLong(header->lumps[i].fileofs);
      header->lumps[i].filelen = LittleLong(header->lumps[i].filelen);
   }

   if (header->version != BSPVERSION && header->version != BSP2VERSION)
      SV_Error("%s: %s has wrong version number (%i should be %i or %i)",
            __func__, mod->name, header->version, BSPVERSION, BSP2VERSION);

   mod_base = (byte *)header;

   /*
    * Check the lump extents
    * FIXME - do this more generally... cleanly...?
    */
   for (i = 0; i < HEADER_LUMPS; ++i)
   {
      int b1 = header->lumps[i].fileofs;
      int e1 = b1 + header->lumps[i].filelen;

      /*
       * Sanity checks
       * - begin and end >= 0 (end might overflow).
       * - end > begin (again, overflow reqd.)
       * - end < size of file.
       */
      if (b1 > e1 || e1 > size || b1 < 0 || e1 < 0)
         SV_Error("%s: bad lump extents in %s", __func__,
               loadmodel->name);

      /* Now, check that it doesn't overlap any other lumps */
      for (j = 0; j < HEADER_LUMPS; ++j)
      {
         int b2 = header->lumps[j].fileofs;
         int e2 = b2 + header->lumps[j].filelen;

         if ((b1 < b2 && e1 > b2) || (b2 < b1 && e2 > b1))
            SV_Error("%s: overlapping lumps in %s", __func__,
                  loadmodel->name);
      }
   }

   /* load into heap */
   Mod_LoadVertexes(&header->lumps[LUMP_VERTEXES]);
   if (header->version == BSPVERSION)
      Mod_LoadEdges_BSP29(&header->lumps[LUMP_EDGES]);
   else
      Mod_LoadEdges_BSP2(&header->lumps[LUMP_EDGES]);
   Mod_LoadSurfedges(&header->lumps[LUMP_SURFEDGES]);
   Mod_LoadTextures(&header->lumps[LUMP_TEXTURES]);
   Mod_LoadLighting(&header->lumps[LUMP_LIGHTING]);
   Mod_LoadPlanes(&header->lumps[LUMP_PLANES]);
   Mod_LoadTexinfo(&header->lumps[LUMP_TEXINFO]);
   if (header->version == BSPVERSION) {
      Mod_LoadFaces_BSP29(&header->lumps[LUMP_FACES]);
      Mod_LoadMarksurfaces_BSP29(&header->lumps[LUMP_MARKSURFACES]);
   } else {
      Mod_LoadFaces_BSP2(&header->lumps[LUMP_FACES]);
      Mod_LoadMarksurfaces_BSP2(&header->lumps[LUMP_MARKSURFACES]);
   }
   Mod_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
   if (header->version == BSPVERSION) {
      Mod_LoadLeafs_BSP29(&header->lumps[LUMP_LEAFS]);
      Mod_LoadNodes_BSP29(&header->lumps[LUMP_NODES]);
      Mod_LoadClipnodes_BSP29(&header->lumps[LUMP_CLIPNODES]);
   } else {
      Mod_LoadLeafs_BSP2(&header->lumps[LUMP_LEAFS]);
      Mod_LoadNodes_BSP2(&header->lumps[LUMP_NODES]);
      Mod_LoadClipnodes_BSP2(&header->lumps[LUMP_CLIPNODES]);
   }
   Mod_LoadEntities(&header->lumps[LUMP_ENTITIES]);
   Mod_LoadSubmodels(&header->lumps[LUMP_MODELS]);

   Mod_MakeHull0();

   mod->numframes = 2;		/* regular and alternate animation */
   mod->flags = 0;

   /*
    * Create space for the decompressed vis data
    * - We assume the main map is the first BSP file loaded (should be)
    * - If any other model has more leafs, then we may be in trouble...
    */
         if (mod->numleafs > pvscache_numleafs) {
            if (pvscache[0].leafbits)
               SV_Error("%s: %d allocated for visdata, but model %s has %d leafs",
                     __func__, pvscache_numleafs, loadmodel->name, mod->numleafs);
            Mod_InitPVSCache(mod->numleafs);
         }

         /**/
         /* set up the submodels (FIXME: this is confusing) */
         /**/
         for (i = 0; i < mod->numsubmodels; i++) {
            bm = &mod->submodels[i];

            mod->hulls[0].firstclipnode = bm->headnode[0];
            for (j = 1; j < MAX_MAP_HULLS; j++) {
               mod->hulls[j].firstclipnode = bm->headnode[j];
               mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
            }

            mod->firstmodelsurface = bm->firstface;
            mod->nummodelsurfaces = bm->numfaces;

            VectorCopy(bm->maxs, mod->maxs);
            VectorCopy(bm->mins, mod->mins);

            mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
            mod->numleafs = bm->visleafs;

            /* duplicate the basic information */
            if (i < mod->numsubmodels - 1) {
               /* "*" + INT_MAX (10 digits) + NUL = 12; round to 16. */
               char name[16];

               snprintf(name, sizeof(name), "*%i", i + 1);
               loadmodel = Mod_FindName(name);
               *loadmodel = *mod;
               strlcpy(loadmodel->name, name, sizeof(loadmodel->name));
               mod = loadmodel;
            }
         }
}

/*
 * =========================================================================
 *                          CLIENT ONLY FUNCTIONS
 * =========================================================================
 */

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void *Mod_Extradata(model_t *mod)
{
   void *r = Cache_Check(&mod->cache);
   if (r)
      return r;

   Mod_LoadModel(mod, true);

   if (!mod->cache.data)
      Sys_Error("%s: caching failed", __func__);
   return mod->cache.data;
}

/*
================
Mod_Print
================
*/
void Mod_Print(void)
{
   int i;
   model_t *mod;

   Con_Printf("Cached models:\n");
   for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
      Con_Printf("%*p : %s\n", (int)sizeof(void *) * 2 + 2,
            mod->cache.data, mod->name);
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel(char *name)
{
   model_t *mod = Mod_FindName(name);

   if (!mod->needload)
   {
      if (mod->type == mod_alias)
         Cache_Check(&mod->cache);
   }
}

