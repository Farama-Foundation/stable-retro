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
/* d_surf.c: rasterization driver surface heap manager */

#include <stdint.h>
#include <stddef.h>

#include "console.h"
#include "d_local.h"
#include "quakedef.h"
#include "r_local.h"
#include "sys.h"

#include "host.h"

float surfscale;

int sc_size;
surfcache_t *sc_rover, *sc_base;

#define GUARDSIZE       4

int D_SurfaceCacheForRes(int width, int height)
{
   int size, pix;

   if (COM_CheckParm("-surfcachesize")) {
      int kb = Q_atoi(com_argv[COM_CheckParm("-surfcachesize") + 1]);
      /* Command-line value is in KB; multiplied by 1024 to give
       * the cache size in bytes.  A negative or absurdly large
       * value would overflow the int multiplication; the cache
       * allocator (D_InitCaches) would then receive a wrapped
       * size and the rover walk would corrupt unrelated memory. */
      if (kb < 1 || kb > INT_MAX / 1024)
         Sys_Error("-surfcachesize: bad value %i (KB)", kb);
      size = kb * 1024;
      return size;
   }

   size = SURFCACHE_SIZE_AT_320X200;
   /* width and height come from VID_Init (35783da clamped them
    * to [320, MAXWIDTH] and [200, MAXHEIGHT]); width*height is
    * at most 1920*1200 = 2.3M pixels, easily fits in int.  The
    * (pix - 64000) * 3 increment is at most ~7M.  Total well
    * under INT_MAX, no overflow possible. */
   pix  = width * height;
   if (pix > 64000)
      size += (pix - 64000) * 3;

   return size;
}

static void D_ClearCacheGuard(void)
{
   byte *s;
   int i;

   s = (byte *)sc_base + sc_size;
   for (i = 0; i < GUARDSIZE; i++)
      s[i] = (byte)i;
}


/*
================
D_InitCaches

================
*/
void
D_InitCaches(void *buffer, int size)
{
    Con_Printf("%ik surface cache\n", size / 1024);

    sc_size = size - GUARDSIZE;
    sc_base = (surfcache_t *)buffer;
    sc_rover = sc_base;

    sc_base->next = NULL;
    sc_base->owner = NULL;
    sc_base->size = sc_size;

    D_ClearCacheGuard();
}


/*
==================
D_FlushCaches
==================
*/
void
D_FlushCaches(void)
{
   surfcache_t *c;

   if (!sc_base)
      return;

   for (c = sc_base; c; c = c->next)
   {
      if (c->owner)
         *c->owner = NULL;
   }

   sc_rover = sc_base;
   sc_base->next = NULL;
   sc_base->owner = NULL;
   sc_base->size = sc_size;
}

/*
=================
D_SCAlloc
=================
*/
static surfcache_t *
D_SCAlloc(int width, int size)
{
   surfcache_t *new_surf;

   if ((width < 0) || (width > 256))
      Sys_Error("%s: bad cache width %d", __func__, width);

   if ((size <= 0) || (size > 0x10000))
      Sys_Error("%s: bad cache size %d", __func__, size);

   size = offsetof(surfcache_t, data) + size;
   size = (size + 3) & ~3;
   if (size > sc_size)
      Sys_Error("%s: %i > cache size", __func__, size);

   if (!sc_rover || (byte *)sc_rover - (byte *)sc_base > sc_size - size)
      sc_rover = sc_base;
   /* colect and free surfcache_t blocks until the rover block is large enough */
   new_surf = sc_rover;
   if (sc_rover->owner)
      *sc_rover->owner = NULL;

   while (new_surf->size < size) {
      /* free another */
      sc_rover = sc_rover->next;
      if (!sc_rover)
         Sys_Error("%s: hit the end of memory", __func__);
      if (sc_rover->owner)
         *sc_rover->owner = NULL;

      new_surf->size += sc_rover->size;
      new_surf->next = sc_rover->next;
   }

   /* create a fragment out of any leftovers */
   if (new_surf->size - size > 256) {
      sc_rover = (surfcache_t *)((byte *)new_surf + size);
      sc_rover->size = new_surf->size - size;
      sc_rover->next = new_surf->next;
      sc_rover->width = 0;
      sc_rover->owner = NULL;
      new_surf->next = sc_rover;
      new_surf->size = size;
   } else
      sc_rover = new_surf->next;

   new_surf->width = width;
   if (width > 0)
      new_surf->height = (size - sizeof(*new_surf) + sizeof(new_surf->data)) / width;

   new_surf->owner = NULL;		/* should be set properly after return */

   return new_surf;
}

/* ============================================================================= */

/*
================
D_CacheSurface
================
*/
surfcache_t *
D_CacheSurface(const entity_t *e, msurface_t *surface, int miplevel)
{
   surfcache_t *cache;

   /* if the surface is animating or flashing, flush the cache */
   r_drawsurf.texture = R_TextureAnimation(e, surface->texinfo->texture);
   r_drawsurf.lightadj[0] = d_lightstylevalue[surface->styles[0]];
   r_drawsurf.lightadj[1] = d_lightstylevalue[surface->styles[1]];
   r_drawsurf.lightadj[2] = d_lightstylevalue[surface->styles[2]];
   r_drawsurf.lightadj[3] = d_lightstylevalue[surface->styles[3]];

   /* see if the cache holds apropriate data */
   cache = surface->cachespots[miplevel];

   if (cache && !cache->dlight && surface->dlightframe != r_framecount
         && cache->texture == r_drawsurf.texture
         && cache->lightadj[0] == r_drawsurf.lightadj[0]
         && cache->lightadj[1] == r_drawsurf.lightadj[1]
         && cache->lightadj[2] == r_drawsurf.lightadj[2]
         && cache->lightadj[3] == r_drawsurf.lightadj[3])
      return cache;

   /* determine shape of surface */
   surfscale = 1.0 / (1 << miplevel);
   r_drawsurf.surfmip = miplevel;
   r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
   r_drawsurf.rowbytes = r_drawsurf.surfwidth;
   r_drawsurf.surfheight = surface->extents[1] >> miplevel;

   /* allocate memory if needed */
   /* if a texture just animated, don't reallocate it */
   if (!cache)			
   {
      cache = D_SCAlloc(r_drawsurf.surfwidth,
            r_drawsurf.surfwidth * r_drawsurf.surfheight);
      surface->cachespots[miplevel] = cache;
      cache->owner                  = &surface->cachespots[miplevel];
      cache->mipscale               = surfscale;
   }

   if (surface->dlightframe == r_framecount)
      cache->dlight = 1;
   else
      cache->dlight = 0;

   r_drawsurf.surfdat = (pixel_t *)cache->data;

   cache->texture = r_drawsurf.texture;
   cache->lightadj[0] = r_drawsurf.lightadj[0];
   cache->lightadj[1] = r_drawsurf.lightadj[1];
   cache->lightadj[2] = r_drawsurf.lightadj[2];
   cache->lightadj[3] = r_drawsurf.lightadj[3];

   /* draw and light the surface texture */
   r_drawsurf.surf = surface;

   c_surf++;
   R_DrawSurface();

   return surface->cachespots[miplevel];
}
