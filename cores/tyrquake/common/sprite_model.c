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

#include <string.h>

#include "common.h"
#include "console.h"
#include "mathlib.h"
#include "model.h"
#include "sys.h"
#include "zone.h"

#include "d_iface.h"
#include "render.h"

/*
=================
Mod_LoadSpriteFrame
=================
*/
static void * Mod_LoadSpriteFrame(void *pin, mspriteframe_t **ppframe,
      int framenum, const byte *bufend, const char *modname)
{
   int origin[2];
   dspriteframe_t *pinframe = (dspriteframe_t *)pin;
   int width;
   int height;
   int numpixels;
   int size;
   mspriteframe_t *pspriteframe;

   /* dspriteframe_t header itself must fit before we read
    * width/height. */
   if ((const byte *)pinframe + sizeof(dspriteframe_t) > bufend)
      Sys_Error("%s: %s: frame header past EOF", __func__, modname);

   width = LittleLong(pinframe->width);
   height = LittleLong(pinframe->height);

   /* Defensive: width/height come from the .spr file directly.
    * Without bounds checking, width*height can overflow the
    * signed int and produce a tiny allocation whose subsequent
    * memcpy in R_SpriteDataStore reads many MB beyond the
    * source.  Cap each dimension to a value well above any
    * legitimate Quake sprite (hellknight nail trail is 32x32). */
   if (width <= 0 || height <= 0 || width > 4096 || height > 4096)
      Sys_Error("%s: bad sprite dimensions %dx%d", __func__, width, height);

   numpixels = width * height;
   /* dspriteframe_t header + numpixels bytes of pixel data
    * must all fit in the input buffer.  R_SpriteDataStore
    * memcpy's exactly numpixels bytes from (pinframe + 1);
    * with the source past EOF it walks adjacent host memory
    * into the persistent sprite cache. */
   if ((const byte *)(pinframe + 1) + numpixels > bufend)
      Sys_Error("%s: %s: pixel data past EOF (%dx%d)",
                __func__, modname, width, height);

   size = sizeof(mspriteframe_t) + R_SpriteDataSize(numpixels);
   pspriteframe = (mspriteframe_t*)Hunk_Alloc(size);

   memset(pspriteframe, 0, size);
   *ppframe = pspriteframe;

   pspriteframe->width = width;
   pspriteframe->height = height;
   origin[0] = LittleLong(pinframe->origin[0]);
   origin[1] = LittleLong(pinframe->origin[1]);

   pspriteframe->up = origin[1];
   pspriteframe->down = origin[1] - height;
   pspriteframe->left = origin[0];
   pspriteframe->right = width + origin[0];

   /* Let the renderer process the pixel data as needed */
   R_SpriteDataStore(pspriteframe, framenum, (byte *)(pinframe + 1));

   return (byte *)pinframe + sizeof(dspriteframe_t) + numpixels;
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
static void * Mod_LoadSpriteGroup(void *pin, mspriteframe_t **ppframe,
		    int framenum, const byte *bufend, const char *modname)
{
   int i;
   dspriteinterval_t *pin_intervals;
   float *poutintervals;
   void *ptemp;
   dspritegroup_t *pingroup = (dspritegroup_t *)pin;
   int numframes;
   mspritegroup_t *pspritegroup;

   /* dspritegroup_t header must fit before we read numframes */
   if ((const byte *)pingroup + sizeof(dspritegroup_t) > bufend)
      Sys_Error("%s: %s: group header past EOF", __func__, modname);

   numframes = LittleLong(pingroup->numframes);

   /* Defensive: numframes is file-controlled.  Negative or
    * huge values either underflow Hunk_Alloc or produce a
    * runaway loop that walks far past the .spr buffer. */
   if (numframes < 1 || numframes > 1024)
      Sys_Error("%s: bad numframes %d", __func__, numframes);

   pspritegroup = (mspritegroup_t*)Hunk_Alloc(sizeof(*pspritegroup) +
         numframes * sizeof(pspritegroup->frames[0]));

   pspritegroup->numframes = numframes;
   *ppframe = (mspriteframe_t *)pspritegroup;
   pin_intervals = (dspriteinterval_t *)(pingroup + 1);
   /* numframes * dspriteinterval_t must fit. */
   if ((const byte *)&pin_intervals[numframes] > bufend)
      Sys_Error("%s: %s: group intervals past EOF (numframes %d)",
                __func__, modname, numframes);
   poutintervals = (float*)Hunk_Alloc(numframes * sizeof(float));
   pspritegroup->intervals = poutintervals;

   for (i = 0; i < numframes; i++) {
      *poutintervals = LittleFloat(pin_intervals->interval);
      /* Sprite-group frame intervals share the same loader gap as
       * MDL pose/skin intervals: 'interval <= 0.0' lets NaN/Inf
       * through (NaN <= 0 is false, Inf > 0). Downstream
       * R_GetSpriteFrame divides cl.time by intervals[N-1] and
       * compares element-by-element; NaN/Inf returns false on every
       * comparison and the frame walk falls off the loop with
       * undefined selection. Reject. */
      if (IS_NAN(*poutintervals) || *poutintervals <= 0.0)
         Sys_Error("%s: interval <= 0", __func__);

      poutintervals++;
      pin_intervals++;
   }

   ptemp = (void *)pin_intervals;

   for (i = 0; i < numframes; i++) {
      ptemp = Mod_LoadSpriteFrame(ptemp, &pspritegroup->frames[i],
                                  framenum * 100 + i, bufend, modname);
   }

   return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel(model_t *mod, void *buffer)
{
   int i;
   msprite_t *psprite;
   int numframes;
   int size;
   dspriteframetype_t *pframetype;
   dsprite_t *pin = (dsprite_t *)buffer;
   const byte *bufend;
   int version;

   /* bufend bounds the input buffer extent (com_filesize bytes
    * loaded).  See 1301598 for the equivalent treatment of
    * Mod_LoadAliasModel.  Without these checks a hostile .spr
    * with a header that lies about numframes / per-frame
    * dimensions causes the running pointer cursor (pframetype,
    * Mod_LoadSpriteFrame's ptemp) to walk past the buffer; the
    * R_SpriteDataStore memcpy at sprite_model.c:86 then reads
    * width*height bytes from past-EOF heap memory into the
    * persistent sprite cache (game-renderable disclosure). */
   if (com_filesize < (int)sizeof(dsprite_t))
      Sys_Error("%s: %s truncated (filesize %d < %d)",
                __func__, mod->name, com_filesize, (int)sizeof(dsprite_t));
   bufend = (const byte *)buffer + com_filesize;

   version = LittleLong(pin->version);
   if (version != SPRITE_VERSION)
      Sys_Error("%s: %s has wrong version number (%i should be %i)",
            __func__, mod->name, version, SPRITE_VERSION);

   numframes = LittleLong(pin->numframes);

   /* Defensive: bound numframes BEFORE the alloc so the
    * size = sizeof(*psprite) + numframes * sizeof(...)
    * arithmetic can't overflow.  The original numframes < 1
    * check was after the alloc had already used the bad size. */
   if (numframes < 1 || numframes > 1024)
      Sys_Error("%s: %s has bad numframes %d",
                __func__, mod->name, numframes);

   size = sizeof(*psprite) + numframes * sizeof(psprite->frames[0]);
   psprite = (msprite_t*)Hunk_Alloc(size);
   mod->cache.data = psprite;

   psprite->type = LittleLong(pin->type);
   psprite->maxwidth = LittleLong(pin->width);
   psprite->maxheight = LittleLong(pin->height);
   psprite->beamlength = LittleFloat(pin->beamlength);
   /* beamlength is loaded raw from the SPR file and threaded into
    * R_RotateSprite (r_sprite.c) as the orientation length for
    * SPR_ORIENTED-style sprites. The existing 'if (beamlength ==
    * 0.0) return;' fast-path at the top of R_RotateSprite catches
    * the zero case, but NaN != 0 and Inf != 0, so a malformed SPR
    * with non-finite beamlength makes it through and feeds
    * VectorScale(vpn, -beamlength, vec) where the scaled vector
    * lands non-finite, then propagates through the per-vertex
    * world->screen transform. Reject at the loader. */
   if (IS_NAN(psprite->beamlength))
      Sys_Error("%s: non-finite beamlength", __func__);
   mod->synctype = (synctype_t)LittleLong(pin->synctype);
   psprite->numframes = numframes;

   mod->mins[0] = mod->mins[1] = -psprite->maxwidth / 2;
   mod->maxs[0] = mod->maxs[1] = psprite->maxwidth / 2;
   mod->mins[2] = -psprite->maxheight / 2;
   mod->maxs[2] = psprite->maxheight / 2;

   /**/
   /* load the frames */
   /**/
   mod->numframes = numframes;
   mod->flags = 0;

   pframetype = (dspriteframetype_t *)(pin + 1);

   for (i = 0; i < numframes; i++) {
      spriteframetype_t frametype;

      /* dspriteframetype_t (just an int) must fit before
       * we read pframetype->type. */
      if ((const byte *)pframetype + sizeof(dspriteframetype_t) > bufend)
         Sys_Error("%s: %s: frame %d type past EOF",
                   __func__, mod->name, i);

      frametype = (spriteframetype_t)LittleLong(pframetype->type);
      psprite->frames[i].type = frametype;

      if (frametype == SPR_SINGLE) {
         pframetype = (dspriteframetype_t *)
            Mod_LoadSpriteFrame(pframetype + 1,
                  &psprite->frames[i].frameptr, i, bufend, mod->name);
      } else {
         pframetype = (dspriteframetype_t *)
            Mod_LoadSpriteGroup(pframetype + 1,
                  &psprite->frames[i].frameptr, i, bufend, mod->name);
      }
      /* check returned cursor stays within the buffer */
      if ((const byte *)pframetype > bufend)
         Sys_Error("%s: %s: frame %d walked past EOF",
                   __func__, mod->name, i);
   }

   mod->type = mod_sprite;
}

/*
==================
Mod_GetSpriteFrame
==================
*/
mspriteframe_t *Mod_GetSpriteFrame(const entity_t *e, msprite_t *psprite, double time)
{
   mspriteframe_t *pspriteframe;
   int i;
   int frame = e->frame;

   if ((frame >= psprite->numframes) || (frame < 0))
   {
      Con_Printf("R_DrawSprite: no such frame %d\n", frame);
      frame = 0;
   }

   if (psprite->frames[frame].type == SPR_SINGLE)
      pspriteframe = psprite->frames[frame].frameptr;
   else
   {
      mspritegroup_t *pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
      float *pintervals            = pspritegroup->intervals;
      int numframes                = pspritegroup->numframes;
      float fullinterval           = pintervals[numframes - 1];

      /* when loading in Mod_LoadSpriteGroup, we guaranteed all interval */
      /* values are positive, so we don't have to worry about division by 0 */
      /* Compute (time mod fullinterval) in double so cl.time-derived */
      /* inputs keep full precision regardless of how long the engine */
      /* has been running, then cast the bounded result (< fullinterval) */
      /* to float for the comparison loop against the interval table. */
      float targettime             = (float)(time - ((int)(time / fullinterval)) * fullinterval);

      for (i = 0; i < (numframes - 1); i++)
      {
         if (pintervals[i] > targettime)
            break;
      }
      pspriteframe = pspritegroup->frames[i];
   }

   return pspriteframe;
}
