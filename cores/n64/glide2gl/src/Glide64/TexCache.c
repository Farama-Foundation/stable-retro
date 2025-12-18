/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include "Gfx_1.3.h"
#include "TexCache.h"
#include "TexLoad.h"
#include "Combine.h"
#include "Util.h"
#include "libretro.h"
#include "GlideExtensions.h"
#include "MiClWr.h"
#include "CRC.h"

#include <clamping.h>
#include <encodings/crc32.h>

#include "../../../Graphics/GBI.h"
#include "../../../Graphics/RDP/gDP_state.h"
#include "../../../Graphics/image_convert.h"

int GetTexAddrUMA(int tmu, int texsize);
static void LoadTex (int id, int tmu);

uint32_t tex1[2048*2048];		// temporary texture
uint32_t tex2[2048*2048];
uint8_t *texture;
uint8_t *texture_buffer = (uint8_t *)tex1;

typedef struct TEXINFO_t
{
   int real_image_width, real_image_height;	// FOR ALIGNMENT PURPOSES ONLY!!!
   int tile_width, tile_height;
   int mask_width, mask_height;
   int width, height;
   int wid_64, line;
   uint32_t crc;
   uint32_t flags;
   int splitheight;
} TEXINFO;

TEXINFO texinfo[2];
int tex_found[2][MAX_TMU];

//****************************************************************
// List functions

typedef struct NODE_t
{
   uint32_t	crc;
   uintptr_t	data;
   int		tmu;
   int		number;
   struct NODE_t	*pNext;
} NODE;

NODE *cachelut[65536];

static void AddToList (NODE **list, uint32_t crc, uintptr_t data, int tmu, int number)
{
   NODE *node = (NODE*)malloc(sizeof(NODE));
   node->crc = crc;
   node->data = data;
   node->tmu = tmu;
   node->number = number;
   node->pNext = *list;
   *list = node;
   rdp.n_cached[tmu] ++;
   rdp.n_cached[tmu^1] = rdp.n_cached[tmu];
}

static void DeleteList (NODE **list)
{
   while (*list)
   {
      NODE *next = (*list)->pNext;
      free(*list);
      *list = next;
   }
}

void TexCacheInit(void)
{
   int i;
   for (i = 0; i < 65536; i++)
      cachelut[i] = NULL;
}

// Clear the texture cache for both TMUs
// TMU : Texture Memory Unit (3Dfx Voodoo term)
void ClearCache(void)
{
   int i;
   voodoo.tmem_ptr[0] = offset_textures;
   rdp.n_cached[0] = 0;
   voodoo.tmem_ptr[1] = offset_textures;
   rdp.n_cached[1] = 0;

   for (i = 0; i < 65536; i++)
      DeleteList(&cachelut[i]);
}

static uint32_t textureCRC(uint32_t crc, uint8_t *addr, int width, int height, int line)
{
   const size_t len = sizeof(uint32_t) * 2 * width;

   while (height--)
   {
      crc = encoding_crc32(crc, addr, len);
      addr += len + line;
   }

   return crc;
}

/* Gets information for either t0 or t1, checks if in cache & fills tex_found */
static void GetTexInfo (int id, int tile)
{
   int t, tile_width, tile_height, mask_width, mask_height, width, height, wid_64, line;
   int real_image_width, real_image_height, crc_height;
   uint32_t crc, flags, mod, modcolor, modcolor1, modcolor2, modfactor, mod_mask;
   NODE *node;
   CACHE_LUT *cache;
   TEXINFO *info;

   // this is the NEW cache searching, searches only textures with similar crc's
   for (t = 0; t < MAX_TMU; t++)
      tex_found[id][t] = -1;

   info = (TEXINFO*)&texinfo[id];

   // Get width and height
   tile_width  = g_gdp.tile[tile].sl - g_gdp.tile[tile].sh + 1;
   tile_height = g_gdp.tile[tile].tl - g_gdp.tile[tile].th + 1;

   mask_width = (g_gdp.tile[tile].mask_s==0)?(tile_width):(1 << g_gdp.tile[tile].mask_s);
   mask_height = (g_gdp.tile[tile].mask_t==0)?(tile_height):(1 << g_gdp.tile[tile].mask_t);

   if (settings.alt_tex_size)
   {
      // ** ALTERNATE TEXTURE SIZE METHOD **
      // Helps speed in some games that loaded weird-sized textures, but could break other
      //  textures.

      // wrap all the way
      width = MIN(mask_width, tile_width);	// changed from mask_width only
      gDP.tiles[tile].width = width;

      // Get the width/height to load
      if ((g_gdp.tile[tile].cs && tile_width <= 256) || (mask_width > 256))   // actual width
         gDP.tiles[tile].width = tile_width;

      height = MIN(mask_height, tile_height);
      gDP.tiles[tile].height = height;

      // actual height
      if ((g_gdp.tile[tile].ct && tile_height <= 256) || (mask_height > 256))
         gDP.tiles[tile].height = tile_height;
   }
   else
   {
      // ** NORMAL TEXTURE SIZE METHOD **
      // This is the 'correct' method for determining texture size, but may cause certain
      //  textures to load too large & make the whole game go slow.

      if (mask_width > 256 && mask_height > 256)
      {
         mask_width = tile_width;
         mask_height = tile_height;
      }

      width = mask_width;
      gDP.tiles[tile].width = mask_width;
      // Get the width/height to load
      if ((g_gdp.tile[tile].cs && tile_width <= 256) )//|| (mask_width > 256))
      {
         // loading width
         width = MIN(mask_width, tile_width);
         // actual width
         gDP.tiles[tile].width = tile_width;
      }

      height = mask_height;
      gDP.tiles[tile].height = mask_height;

      if ((g_gdp.tile[tile].ct && tile_height <= 256) || (mask_height > 256))
      {
         // loading height
         height = MIN(mask_height, tile_height);
         // actual height
         gDP.tiles[tile].height = tile_height;
      }
   }

   // without any large texture fixing-up; for alignment
   real_image_width  = gDP.tiles[tile].width;
   real_image_height = gDP.tiles[tile].height;
   crc_height = height;
   if (rdp.timg.set_by == 1)
      crc_height = tile_height;

#ifndef NDEBUG
   LRDP(" | | |-+ Texture approved:\n");
   FRDP (" | | | |- tmem: %08lx\n", g_gdp.tile[tile].tmem);
   FRDP (" | | | |- load width: %d\n", width);
   FRDP (" | | | |- load height: %d\n", height);
   FRDP (" | | | |- actual width: %d\n", gDP.tiles[tile].width);
   FRDP (" | | | |- actual height: %d\n", gDP.tiles[tile].height);
   FRDP (" | | | |- size: %d\n", g_gdp.tile[tile].size);
   FRDP (" | | | +- format: %d\n", g_gdp.tile[tile].format);
   LRDP(" | | |- Calculating CRC... ");
#endif

   // ** CRC CHECK

   wid_64 = width << (g_gdp.tile[tile].size) >> 1;
   if (g_gdp.tile[tile].size == G_IM_SIZ_32b)
   {
      if (wid_64 & 15) wid_64 += 16;
      wid_64 &= 0xFFFFFFF0;
   }
   else
   {
      if (wid_64 & 7) wid_64 += 8;	// round up
   }
   wid_64 = wid_64>>3;

   // Texture too big for tmem & needs to wrap? (trees in mm)
   if (g_gdp.tile[tile].tmem + MIN(height, tile_height) * (g_gdp.tile[tile].line << 3) > 4096)
   {
      int y, shift;
      LRDP("TEXTURE WRAPS TMEM!!! ");

      // calculate the y value that intersects at 4096 bytes
      y = (4096 - g_gdp.tile[tile].tmem) / (g_gdp.tile[tile].line<<3);

      g_gdp.tile[tile].ct = 0;
      g_gdp.tile[tile].tl = g_gdp.tile[tile].th + y - 1;

      // calc mask
      for (shift=0; (1<<shift)<y; shift++);
      g_gdp.tile[tile].mask_t = shift;

      // restart the function
      LRDP("restarting...\n");
      GetTexInfo (id, tile);
      return;
   }

   line = g_gdp.tile[tile].line;
   if (g_gdp.tile[tile].size == G_IM_SIZ_32b)
      line <<= 1;

   crc = 0;

   if ((g_gdp.tile[tile].size < 2) && (rdp.tlut_mode || g_gdp.tile[tile].format == G_IM_FMT_CI))
   {
      if (g_gdp.tile[tile].size == G_IM_SIZ_4b)
         crc = rdp.pal_8_crc[g_gdp.tile[tile].palette];
      else
         crc = gDP.paletteCRC256;
   }

   {
      uint8_t *addr;
      line = (line - wid_64) << 3;
      if (wid_64 < 1)
         wid_64 = 1;
      addr = (uint8_t*)((((uint8_t*)g_gdp.tmem) + (g_gdp.tile[tile].tmem << 3)));
      if (crc_height > 0) // Check the CRC
      {
         if (g_gdp.tile[tile].size < 3)
            crc = textureCRC(crc, addr, wid_64, crc_height, line);
         else //32b texture
         {
            int line_2, wid_64_2;
            line_2 = line >> 1;
            wid_64_2 = MAX(1, wid_64 >> 1);
            crc = textureCRC(crc, addr, wid_64_2, crc_height, line_2);
            crc = textureCRC(crc, addr+0x800, wid_64_2, crc_height, line_2);
         }
      }
   }


   FRDP ("Done.  CRC is: %08lx.\n", crc);

   flags = (g_gdp.tile[tile].cs << 23) | (g_gdp.tile[tile].ms << 22) |
      (g_gdp.tile[tile].mask_s << 18) | (g_gdp.tile[tile].ct << 17) |
      (g_gdp.tile[tile].mt << 16) | (g_gdp.tile[tile].mask_t << 12);

   info->real_image_width = real_image_width;
   info->real_image_height = real_image_height;
   info->tile_width = tile_width;
   info->tile_height = tile_height;
   info->mask_width = mask_width;
   info->mask_height = mask_height;
   info->width = width;
   info->height = height;
   info->wid_64 = wid_64;
   info->line = line;
   info->crc = crc;
   info->flags = flags;

   // Search the texture cache for this texture
   LRDP(" | | |-+ Checking cache...\n");

   if (rdp.noise == NOISE_MODE_TEXTURE)
      return;

   if (id == 0)
   {
      mod = cmb.mod_0;
      modcolor = cmb.modcolor_0;
      modcolor1 = cmb.modcolor1_0;
      modcolor2 = cmb.modcolor2_0;
      modfactor = cmb.modfactor_0;
   }
   else
   {
      mod = cmb.mod_1;
      modcolor = cmb.modcolor_1;
      modcolor1 = cmb.modcolor1_1;
      modcolor2 = cmb.modcolor2_1;
      modfactor = cmb.modfactor_1;
   }

   node = (NODE*)cachelut[crc>>16];
   mod_mask = (g_gdp.tile[tile].format == G_IM_FMT_CI) ? 0xFFFFFFFF : 0xF0F0F0F0;
   while (node)
   {
      if (node->crc == crc)
      {
         cache = (CACHE_LUT*)node->data;
         if (/*tex_found[id][node->tmu] == -1 &&
               g_gdp.tile[tile].palette == cache->palette &&
               g_gdp.tile[tile].format == cache->format &&
               g_gdp.tile[tile].size == cache->size &&*/
               gDP.tiles[tile].width == cache->width &&
               gDP.tiles[tile].height == cache->height &&
               flags == cache->flags)
         {
            if (!(mod+cache->mod) || (cache->mod == mod &&
                     (cache->mod_color&mod_mask) == (modcolor&mod_mask) &&
                     (cache->mod_color1&mod_mask) == (modcolor1&mod_mask) &&
                     (cache->mod_color2&mod_mask) == (modcolor2&mod_mask) &&
                     abs((int)(cache->mod_factor - modfactor)) < 8))
            {
               FRDP (" | | | |- Texture found in cache (tmu=%d).\n", node->tmu);
               tex_found[id][node->tmu] = node->number;
               tex_found[id][node->tmu^1] = node->number;
               return;
            }
         }
      }
      node = node->pNext;
   }
}

#define TMUMODE_NORMAL		0
#define TMUMODE_PASSTHRU	1
#define TMUMODE_NONE		2

int SwapTextureBuffer(void); //forward decl

// Does texture loading after combiner is set
void TexCache(void)
{
   int i, tmu_0_mode, tmu_1_mode, tmu_0, tmu_1;

   if (rdp.tex & 1)
      GetTexInfo (0, rdp.cur_tile);
   if (rdp.tex & 2)
      GetTexInfo (1, rdp.cur_tile+1);

   tmu_0_mode = 0;
   tmu_1_mode = 0;

   // Select the best TMUs to use (removed 3 tmu support, unnecessary)
   if (rdp.tex == 3)	// T0 and T1
   {
      tmu_0 = 0;
      tmu_1 = 1;
   }
   else if (rdp.tex == 2)	// T1
   {
      if (tex_found[1][0] != -1)	// T1 found in tmu 0
         tmu_1 = 0;
      else if (tex_found[1][1] != -1)	// T1 found in tmu 1
         tmu_1 = 1;
      else	// T1 not found
         tmu_1 = 0;

      tmu_0 = !tmu_1;
      tmu_0_mode = (tmu_0==1)?TMUMODE_NONE:TMUMODE_PASSTHRU;
   }
   else if (rdp.tex == 1)	// T0
   {
      if (tex_found[0][0] != -1)	// T0 found in tmu 0
         tmu_0 = 0;
      else if (tex_found[0][1] != -1)	// T0 found in tmu 1
         tmu_0 = 1;
      else	// T0 not found
         tmu_0 = 0;

      tmu_1 = !tmu_0;
      tmu_1_mode = (tmu_1==1)?TMUMODE_NONE:TMUMODE_PASSTHRU;
   }
   else	// no texture
   {
      tmu_0 = 0;
      tmu_0_mode = TMUMODE_NONE;
      tmu_1 = 0;
      tmu_1_mode = TMUMODE_NONE;
   }

   FRDP (" | |-+ Modes set:\n | | |- tmu_0 = %d\n | | |- tmu_1 = %d\n",
         tmu_0, tmu_1);
   FRDP (" | | |- tmu_0_mode = %d\n | | |- tmu_1_mode = %d\n",
         tmu_0_mode, tmu_1_mode);

   if (tmu_0_mode == TMUMODE_PASSTHRU)
   {
      cmb.tmu0_func = cmb.tmu0_a_func = GR_COMBINE_FUNCTION_SCALE_OTHER;
      cmb.tmu0_fac = cmb.tmu0_a_fac = GR_COMBINE_FACTOR_ONE;
      if (cmb.tex_cmb_ext_use)
      {
         cmb.t0c_ext_a = GR_CMBX_OTHER_TEXTURE_RGB;
         cmb.t0c_ext_a_mode = GR_FUNC_MODE_X;
         cmb.t0c_ext_b = GR_CMBX_LOCAL_TEXTURE_RGB;
         cmb.t0c_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t0c_ext_c = GR_CMBX_ZERO;
         cmb.t0c_ext_c_invert = 1;
         cmb.t0c_ext_d = GR_CMBX_ZERO;
         cmb.t0c_ext_d_invert = 0;
         cmb.t0a_ext_a = GR_CMBX_OTHER_TEXTURE_ALPHA;
         cmb.t0a_ext_a_mode = GR_FUNC_MODE_X;
         cmb.t0a_ext_b = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         cmb.t0a_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t0a_ext_c = GR_CMBX_ZERO;
         cmb.t0a_ext_c_invert = 1;
         cmb.t0a_ext_d = GR_CMBX_ZERO;
         cmb.t0a_ext_d_invert = 0;
      }
   }
   else if (tmu_0_mode == TMUMODE_NONE)
   {
      cmb.tmu0_func = cmb.tmu0_a_func = GR_COMBINE_FUNCTION_NONE;
      cmb.tmu0_fac = cmb.tmu0_a_fac = GR_COMBINE_FACTOR_NONE;
      if (cmb.tex_cmb_ext_use)
      {
         cmb.t0c_ext_a = GR_CMBX_LOCAL_TEXTURE_RGB;
         cmb.t0c_ext_a_mode = GR_FUNC_MODE_ZERO;
         cmb.t0c_ext_b = GR_CMBX_LOCAL_TEXTURE_RGB;
         cmb.t0c_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t0c_ext_c = GR_CMBX_ZERO;
         cmb.t0c_ext_c_invert = 0;
         cmb.t0c_ext_d = GR_CMBX_ZERO;
         cmb.t0c_ext_d_invert = 0;
         cmb.t0a_ext_a = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         cmb.t0a_ext_a_mode = GR_FUNC_MODE_ZERO;
         cmb.t0a_ext_b = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         cmb.t0a_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t0a_ext_c = GR_CMBX_ZERO;
         cmb.t0a_ext_c_invert = 0;
         cmb.t0a_ext_d = GR_CMBX_ZERO;
         cmb.t0a_ext_d_invert = 0;
      }
   }
   if (tmu_1_mode == TMUMODE_PASSTHRU)
   {
      cmb.tmu1_func = cmb.tmu1_a_func = GR_COMBINE_FUNCTION_SCALE_OTHER;
      cmb.tmu1_fac = cmb.tmu1_a_fac = GR_COMBINE_FACTOR_ONE;
      if (cmb.tex_cmb_ext_use)
      {
         cmb.t1c_ext_a = GR_CMBX_OTHER_TEXTURE_RGB;
         cmb.t1c_ext_a_mode = GR_FUNC_MODE_X;
         cmb.t1c_ext_b = GR_CMBX_LOCAL_TEXTURE_RGB;
         cmb.t1c_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t1c_ext_c = GR_CMBX_ZERO;
         cmb.t1c_ext_c_invert = 1;
         cmb.t1c_ext_d = GR_CMBX_ZERO;
         cmb.t1c_ext_d_invert = 0;
         cmb.t1a_ext_a = GR_CMBX_OTHER_TEXTURE_ALPHA;
         cmb.t1a_ext_a_mode = GR_FUNC_MODE_X;
         cmb.t1a_ext_b = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         cmb.t1a_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t1a_ext_c = GR_CMBX_ZERO;
         cmb.t1a_ext_c_invert = 1;
         cmb.t1a_ext_d = GR_CMBX_ZERO;
         cmb.t1a_ext_d_invert = 0;
      }
   }
   else if (tmu_1_mode == TMUMODE_NONE)
   {
      cmb.tmu1_func = cmb.tmu1_a_func = GR_COMBINE_FUNCTION_NONE;
      cmb.tmu1_fac = cmb.tmu1_a_fac = GR_COMBINE_FACTOR_NONE;
      if (cmb.tex_cmb_ext_use)
      {
         cmb.t1c_ext_a = GR_CMBX_LOCAL_TEXTURE_RGB;
         cmb.t1c_ext_a_mode = GR_FUNC_MODE_ZERO;
         cmb.t1c_ext_b = GR_CMBX_LOCAL_TEXTURE_RGB;
         cmb.t1c_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t1c_ext_c = GR_CMBX_ZERO;
         cmb.t1c_ext_c_invert = 0;
         cmb.t1c_ext_d = GR_CMBX_ZERO;
         cmb.t1c_ext_d_invert = 0;
         cmb.t1a_ext_a = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         cmb.t1a_ext_a_mode = GR_FUNC_MODE_ZERO;
         cmb.t1a_ext_b = GR_CMBX_LOCAL_TEXTURE_ALPHA;
         cmb.t1a_ext_b_mode = GR_FUNC_MODE_ZERO;
         cmb.t1a_ext_c = GR_CMBX_ZERO;
         cmb.t1a_ext_c_invert = 0;
         cmb.t1a_ext_d = GR_CMBX_ZERO;
         cmb.t1a_ext_d_invert = 0;
      }
   }

   rdp.t0 = tmu_0;
   rdp.t1 = tmu_1;

   // SET the combiner
   {
      if (rdp.allow_combine)
      {
         // Now actually combine
         if (cmb.cmb_ext_use)
         {
            LRDP(" | | | |- combiner extension\n");
            if (!(cmb.cmb_ext_use & COMBINE_EXT_COLOR))
               ColorCombinerToExtension ();
            if (!(cmb.cmb_ext_use & COMBINE_EXT_ALPHA))
               AlphaCombinerToExtension ();
            grColorCombineExt(cmb.c_ext_a, cmb.c_ext_a_mode,
                  cmb.c_ext_b, cmb.c_ext_b_mode,
                  cmb.c_ext_c, cmb.c_ext_c_invert,
                  cmb.c_ext_d, cmb.c_ext_d_invert, 0, 0);
            grAlphaCombineExt(cmb.a_ext_a, cmb.a_ext_a_mode,
                  cmb.a_ext_b, cmb.a_ext_b_mode,
                  cmb.a_ext_c, cmb.a_ext_c_invert,
                  cmb.a_ext_d, cmb.a_ext_d_invert, 0, 0);
         }
         else
         {
            grColorCombine (cmb.c_fnc, cmb.c_fac, cmb.c_loc, cmb.c_oth, FXFALSE);
            grAlphaCombine (cmb.a_fnc, cmb.a_fac, cmb.a_loc, cmb.a_oth, FXFALSE);
         }
         grConstantColorValue (cmb.ccolor);
         grAlphaBlendFunction (cmb.abf1, cmb.abf2, GR_BLEND_ZERO, GR_BLEND_ZERO);
         if (!rdp.tex) //nothing more to do
            return;
      }

      if (tmu_1 < NUM_TMU)
      {
         if (cmb.tex_cmb_ext_use)
         {
            LRDP(" | | | |- combiner extension tmu1\n");
            if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_COLOR))
               TexColorCombinerToExtension (GR_TMU1);
            if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_ALPHA))
               TexAlphaCombinerToExtension (GR_TMU1);
            grTexColorCombineExt(tmu_1, cmb.t1c_ext_a, cmb.t1c_ext_a_mode,
                  cmb.t1c_ext_b, cmb.t1c_ext_b_mode,
                  cmb.t1c_ext_c, cmb.t1c_ext_c_invert,
                  cmb.t1c_ext_d, cmb.t1c_ext_d_invert, 0, 0);
            grTexAlphaCombineExt(tmu_1, cmb.t1a_ext_a, cmb.t1a_ext_a_mode,
                  cmb.t1a_ext_b, cmb.t1a_ext_b_mode,
                  cmb.t1a_ext_c, cmb.t1a_ext_c_invert,
                  cmb.t1a_ext_d, cmb.t1a_ext_d_invert, 0, 0, cmb.tex_ccolor);
         }
         else
            grTexCombine (tmu_1, cmb.tmu1_func, cmb.tmu1_fac, cmb.tmu1_a_func, cmb.tmu1_a_fac, cmb.tmu1_invert, cmb.tmu1_a_invert);
         grTexDetailControl (tmu_1, cmb.dc1_lodbias, cmb.dc1_detailscale, cmb.dc1_detailmax);
      }
      if (tmu_0 < NUM_TMU)
      {
         if (cmb.tex_cmb_ext_use)
         {
            LRDP(" | | | |- combiner extension tmu0\n");
            if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_COLOR))
               TexColorCombinerToExtension (GR_TMU0);
            if (!(cmb.tex_cmb_ext_use & TEX_COMBINE_EXT_ALPHA))
               TexAlphaCombinerToExtension (GR_TMU0);
            grTexColorCombineExt(tmu_0, cmb.t0c_ext_a, cmb.t0c_ext_a_mode,
                  cmb.t0c_ext_b, cmb.t0c_ext_b_mode,
                  cmb.t0c_ext_c, cmb.t0c_ext_c_invert,
                  cmb.t0c_ext_d, cmb.t0c_ext_d_invert, 0, 0);
            grTexAlphaCombineExt(tmu_0, cmb.t0a_ext_a, cmb.t0a_ext_a_mode,
                  cmb.t0a_ext_b, cmb.t0a_ext_b_mode,
                  cmb.t0a_ext_c, cmb.t0a_ext_c_invert,
                  cmb.t0a_ext_d, cmb.t0a_ext_d_invert, 0, 0, cmb.tex_ccolor);
         }
         else
            grTexCombine (tmu_0, cmb.tmu0_func, cmb.tmu0_fac, cmb.tmu0_a_func, cmb.tmu0_a_fac, cmb.tmu0_invert, cmb.tmu0_a_invert);
         grTexDetailControl (tmu_0, cmb.dc0_lodbias, cmb.dc0_detailscale, cmb.dc0_detailmax);
      }
   }

   if ((rdp.tex & 1) && tmu_0 < NUM_TMU)
   {
      if (tex_found[0][tmu_0] != -1)
      {
         CACHE_LUT *cache;
         LRDP(" | |- T0 found in cache.\n");
         cache = (CACHE_LUT*)&rdp.cache[0][tex_found[0][0]];
         rdp.cur_cache[0] = cache;
         rdp.cur_cache[0]->last_used = frame_count;
         rdp.cur_cache[0]->uses = 0;
         grTexSource (tmu_0,
               cache->tmem_addr,
               GR_MIPMAPLEVELMASK_BOTH,
               &cache->t_info,
               false);
      }
      else
         LoadTex (0, tmu_0);
   }
   if ((rdp.tex & 2) && tmu_1 < NUM_TMU)
   {
      if (tex_found[1][tmu_1] != -1)
      {
         CACHE_LUT *cache;
         LRDP(" | |- T1 found in cache.\n");
         cache = (CACHE_LUT*)&rdp.cache[0][tex_found[1][0]];
         rdp.cur_cache[1] = cache;
         rdp.cur_cache[1]->last_used = frame_count;
         rdp.cur_cache[1]->uses = 0;
         grTexSource (tmu_1,
               cache->tmem_addr,
               GR_MIPMAPLEVELMASK_BOTH,
               &cache->t_info,
               false);
      }
      else
         LoadTex (1, tmu_1);
   }

   {
      int tmu_v[2];

      tmu_v[0] = tmu_0;
      tmu_v[1] = tmu_1;
      for (i = 0; i < NUM_TMU; i++)
      {
         int tile;
         const int tmu = tmu_v[i];

         if (tmu >= NUM_TMU)
            continue;

         tile = rdp.cur_tile + i;


         if (rdp.cur_cache[i])
         {
            uint32_t mode_s, mode_t;
            int cs, ct, filter;
            if (rdp.force_wrap && !rdp.texrecting)
            {
               cs = g_gdp.tile[tile].cs && g_gdp.tile[tile].sl - g_gdp.tile[tile].sh < 256;
               ct = g_gdp.tile[tile].ct && g_gdp.tile[tile].tl - g_gdp.tile[tile].th < 256;
            }
            else
            {
               cs = (g_gdp.tile[tile].cs || g_gdp.tile[tile].mask_s == 0) &&
                  g_gdp.tile[tile].sl - g_gdp.tile[tile].sh < 256;
               ct = (g_gdp.tile[tile].ct || g_gdp.tile[tile].mask_t == 0) &&
                  g_gdp.tile[tile].tl - g_gdp.tile[tile].th < 256;
            }

            mode_s = GR_TEXTURECLAMP_WRAP;
            mode_t = GR_TEXTURECLAMP_WRAP;

            if (cs)
               mode_s = GR_TEXTURECLAMP_CLAMP;
            else if (g_gdp.tile[tile].ms)
               mode_s = GR_TEXTURECLAMP_MIRROR_EXT;

            if (ct)
               mode_t = GR_TEXTURECLAMP_CLAMP;
            else if (g_gdp.tile[tile].mt)
               mode_t = GR_TEXTURECLAMP_MIRROR_EXT;

            if (settings.filtering == 0)
               filter = (gDP.otherMode.textureFilter == 2)? GR_TEXTUREFILTER_3POINT_LINEAR : GR_TEXTUREFILTER_POINT_SAMPLED;
            else
               filter = (settings.filtering==1)? GR_TEXTUREFILTER_3POINT_LINEAR : (settings.filtering==2)?GR_TEXTUREFILTER_POINT_SAMPLED:GR_TEXTUREFILTER_BILINEAR;
            grTexFilterClampMode (tmu, mode_s, mode_t, filter, filter);
         }
      }
   }
}

// Does the actual texture loading after everything is prepared
static void LoadTex(int id, int tmu)
{
   int lod, aspect, shift, size_max, wid, hei, modifyPalette;
   uint32_t size_x, size_y, real_x, real_y, result;
   uint32_t mod, modcolor, modcolor1, modcolor2, modfactor;
   CACHE_LUT *cache;
   int td = rdp.cur_tile + id;

   if (texinfo[id].width < 0 || texinfo[id].height < 0)
      return;

   // Clear the cache if it's full
   if (rdp.n_cached[tmu] >= MAX_CACHE)
   {
      LRDP("Cache count reached, clearing...\n");
      ClearCache ();
      if (id == 1 && rdp.tex == 3)
         LoadTex (0, rdp.t0);
   }

   // Get this cache object
   cache = &rdp.cache[0][rdp.n_cached[0]];
   rdp.cur_cache[id] = cache;

   //!Hackalert
   //GoldenEye water texture. It has CI format in fact, but the game set it to RGBA
   if ((settings.hacks&hack_GoldenEye) && g_gdp.tile[td].format == G_IM_FMT_RGBA && rdp.tlut_mode == 2 && g_gdp.tile[td].size == G_IM_SIZ_16b)
   {
      g_gdp.tile[td].format = G_IM_FMT_CI;
      g_gdp.tile[td].size = G_IM_SIZ_8b;
   }

   // Set the data
   cache->line       = g_gdp.tile[td].line;
   cache->addr       = rdp.addr[g_gdp.tile[td].tmem];
   cache->crc        = texinfo[id].crc;
   cache->palette    = g_gdp.tile[td].palette;
   cache->width      = gDP.tiles[td].width;
   cache->height     = gDP.tiles[td].height;
   cache->format     = g_gdp.tile[td].format;
   cache->size       = g_gdp.tile[td].size;
   cache->tmem_addr  = voodoo.tmem_ptr[tmu];
   cache->set_by     = rdp.timg.set_by;
   cache->texrecting = rdp.texrecting;
   cache->last_used  = frame_count;
   cache->uses = 0;
   cache->flags = texinfo[id].flags;

   // Add this cache to the list
   AddToList (&cachelut[cache->crc>>16], cache->crc, (uintptr_t)(cache), tmu, rdp.n_cached[tmu]);

   // temporary
   cache->t_info.format = GR_TEXFMT_ARGB_1555;

   // Calculate lod and aspect
   size_x = gDP.tiles[td].width;
   size_y = gDP.tiles[td].height;

   for (shift=0; (1<<shift) < (int)size_x; shift++);
   size_x = 1 << shift;
   for (shift=0; (1<<shift) < (int)size_y; shift++);
   size_y = 1 << shift;

   // Calculate the maximum size
   size_max = MAX(size_x, size_y);
   real_x = size_max;
   real_y = size_max;
   switch (size_max)
   {
      case 1:
         lod = GR_LOD_LOG2_1;
         cache->scale = 256.0f;
         break;
      case 2:
         lod = GR_LOD_LOG2_2;
         cache->scale = 128.0f;
         break;
      case 4:
         lod = GR_LOD_LOG2_4;
         cache->scale = 64.0f;
         break;
      case 8:
         lod = GR_LOD_LOG2_8;
         cache->scale = 32.0f;
         break;
      case 16:
         lod = GR_LOD_LOG2_16;
         cache->scale = 16.0f;
         break;
      case 32:
         lod = GR_LOD_LOG2_32;
         cache->scale = 8.0f;
         break;
      case 64:
         lod = GR_LOD_LOG2_64;
         cache->scale = 4.0f;
         break;
      case 128:
         lod = GR_LOD_LOG2_128;
         cache->scale = 2.0f;
         break;
      case 256:
         lod = GR_LOD_LOG2_256;
         cache->scale = 1.0f;
         break;
      case 512:
         lod = GR_LOD_LOG2_512;
         cache->scale = 0.5f;
         break;
      default:
         lod = GR_LOD_LOG2_1024;
         cache->scale = 0.25f;
         break;
   }

   // Calculate the aspect ratio
   if (size_x >= size_y)
   {
      int ratio = size_x / size_y;
      switch (ratio)
      {
         case 1:
            aspect = GR_ASPECT_LOG2_1x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 1.0f;
            break;
         case 2:
            aspect = GR_ASPECT_LOG2_2x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 0.5f;
            real_y >>= 1;
            break;
         case 4:
            aspect = GR_ASPECT_LOG2_4x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 0.25f;
            real_y >>= 2;
            break;
         default:
            aspect = GR_ASPECT_LOG2_8x1;
            cache->scale_x = 1.0f;
            cache->scale_y = 0.125f;
            real_y >>= 3;
            break;
      }
   }
   else
   {
      int ratio = size_y / size_x;
      switch (ratio)
      {
         case 2:
            aspect = GR_ASPECT_LOG2_1x2;
            cache->scale_x = 0.5f;
            cache->scale_y = 1.0f;
            real_x >>= 1;
            break;
         case 4:
            aspect = GR_ASPECT_LOG2_1x4;
            cache->scale_x = 0.25f;
            cache->scale_y = 1.0f;
            real_x >>= 2;
            break;
         default:
            aspect = GR_ASPECT_LOG2_1x8;
            cache->scale_x = 0.125f;
            cache->scale_y = 1.0f;
            real_x >>= 3;
            break;
      }
   }

   if (real_x != cache->width || real_y != cache->height)
   {
      cache->scale_x *= (float)cache->width / (float)real_x;
      cache->scale_y *= (float)cache->height / (float)real_y;
   }

   cache->splitheight = real_y;
   if (cache->splitheight < texinfo[id].splitheight)
      cache->splitheight = texinfo[id].splitheight;

   // ** Calculate alignment values
   wid = cache->width;
   hei = cache->height;

   cache->c_off = cache->scale * 0.5f;
   if (wid != 1) cache->c_scl_x = cache->scale;
   else cache->c_scl_x = 0.0f;
   if (hei != 1) cache->c_scl_y = cache->scale;
   else cache->c_scl_y = 0.0f;

   if (id == 0)
   {
      mod = cmb.mod_0;
      modcolor = cmb.modcolor_0;
      modcolor1 = cmb.modcolor1_0;
      modcolor2 = cmb.modcolor2_0;
      modfactor = cmb.modfactor_0;
   }
   else
   {
      mod = cmb.mod_1;
      modcolor = cmb.modcolor_1;
      modcolor1 = cmb.modcolor1_1;
      modcolor2 = cmb.modcolor2_1;
      modfactor = cmb.modfactor_1;
   }

   modifyPalette = (mod && (cache->format == G_IM_FMT_CI) && (rdp.tlut_mode == 2));

   if (modifyPalette)
   {
      uint16_t tmp_pal[256];

      uint8_t cr0       = ((modcolor >> 24) & 0xFF);
      uint8_t cg0       = ((modcolor >> 16) & 0xFF);
      uint8_t cb0       = ((modcolor >> 8) & 0xFF);
      uint8_t ca0       = (modcolor & 0xFF);
      uint8_t cr1       = ((modcolor1 >> 24) & 0xFF);
      uint8_t cg1       = ((modcolor1 >> 16) & 0xFF);
      uint8_t cb1       = ((modcolor1 >> 8)  & 0xFF);
      int32_t size      = 256;
      float   percent_r = ((modcolor1 >> 24) & 0xFF) / 255.0f;
      float   percent_g = ((modcolor1 >> 16) & 0xFF) / 255.0f;
      float   percent_b = ((modcolor1 >> 8)  & 0xFF) / 255.0f;
      uint16_t *col     = (uint16_t*)&rdp.pal_8[0];

      memcpy(tmp_pal, rdp.pal_8, 512);

      switch (mod)
      {
         case TMOD_TEX_INTER_COLOR_USING_FACTOR:
            percent_r = percent_g = percent_b = modfactor / 255.0f;
         case TMOD_TEX_INTER_COL_USING_COL1:
            while(--size)
            {
               uint8_t a = (*col & 0x0001);
               uint8_t r = (uint8_t)
                  ((1-percent_r) * (((*col & 0xF800) >> 11)) + percent_r * cr0);
               uint8_t g = (uint8_t)
                  ((1-percent_g) * (((*col & 0x07C0) >>  6)) + percent_g * cg0);
               uint8_t b = (uint8_t)
                  ((1-percent_b) * (((*col & 0x003E) >>  1)) + percent_b * cb0);

               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_FULL_COLOR_SUB_TEX:
            while(--size)
            {
               uint8_t a = ca0 - (*col & 0x0001);
               uint8_t r = cr0 - (((*col & 0xF800) >> 11));
               uint8_t g = cg0 - (((*col & 0x07C0) >> 6));
               uint8_t b = cb0 - (((*col & 0x003E) >> 1));
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_TEX_SUB_COL:
            while(--size)
            {
               uint8_t a = (*col & 0x0001);
               uint8_t r = (((*col & 0xF800) >> 11)) - cr0;
               uint8_t g = (((*col & 0x07C0) >> 6)) - cg0;
               uint8_t b = (((*col & 0x003E) >> 1)) - cb0;
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_COL_INTER_COL1_USING_TEX:
            while(--size)
            {
               float percent_r = ((*col & 0xF800) >> 11) / 31.0f;
               float percent_g = ((*col & 0x07C0) >> 6) / 31.0f;
               float percent_b = ((*col & 0x003E) >> 1) / 31.0f;
               uint8_t a = (*col & 0x0001);
               uint8_t r = (uint8_t)((1.0f-percent_r) * cr0 + percent_r * cr1);
               uint8_t g = (uint8_t)((1.0f-percent_g) * cg0 + percent_g * cg1);
               uint8_t b = (uint8_t)((1.0f-percent_b) * cb0 + percent_b * cb1);
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_TEX_SUB_COL_MUL_FAC_ADD_TEX:
            {
               float percent = modfactor / 255.0f;

               while(--size)
               {
                  uint8_t a = (*col & 0x0001);
                  float r = (uint8_t)((float)((*col & 0xF800) >> 11));
                  float g = (uint8_t)((float)((*col & 0x07C0) >> 6));
                  float b = (uint8_t)((float)((*col & 0x003E) >> 1));
                  r = (r - cr0) * percent + r;
                  g = (g - cg0) * percent + g;
                  b = (b - cb0) * percent + b;

                  /* clipping the result */
                  r = clamp_float(r, 0.0f, 255.0f);
                  g = clamp_float(g, 0.0f, 255.0f);
                  b = clamp_float(b, 0.0f, 255.0f);
                  *col++    = PAL8toRGBA16(r, g, b, a);
               };
            }
            break;
         case TMOD_TEX_SUB_COL_MUL_FAC:
            {
               float percent = modfactor / 255.0f;

               while(--size)
               {
                  uint8_t a = (*col & 0x0001);
                  float r   = (((float)((*col & 0xF800) >> 11)) - cr0) * percent;
                  float g   = (((float)((*col & 0x07C0) >> 6)) - cg0) * percent;
                  float b   = ((float)((*col & 0x003E) >> 1) - cb0) * percent;
                  r         = clamp_float(r, 0.0f, 255.0f);
                  g         = clamp_float(g, 0.0f, 255.0f);
                  b         = clamp_float(b, 0.0f, 255.0f);
                  *col++    = PAL8toRGBA16(r, g, b, a);
               };
            }
         case TMOD_TEX_SCALE_COL_ADD_COL:
            percent_r = ((modcolor1 >> 24) & 0xFF) / 255.0f;
            percent_g = ((modcolor1 >> 16) & 0xFF) / 255.0f;
            percent_b = ((modcolor1 >> 8)  & 0xFF) / 255.0f;

            while(--size)
            {
               uint8_t a = (*col & 0x0001);
               uint8_t r = (uint8_t)(percent_r * ((*col & 0xF800) >> 11)) + cr0;
               uint8_t g = (uint8_t)(percent_g * ((*col & 0x07C0) >> 6)) + cg0;
               uint8_t b = (uint8_t)(percent_b * ((*col & 0x003E) >> 1)) + cb0;
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_TEX_ADD_COL:
            while(--size)
            {
               uint8_t a = (*col & 0x0001);
               uint8_t r = cr0 + (((*col & 0xF800) >> 11));
               uint8_t g = cg0 + (((*col & 0x07C0) >> 6));
               uint8_t b = cb0 + (((*col & 0x003E) >> 1));
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_COL_INTER_TEX_USING_COL1:
            while(--size)
            {
               uint8_t a = (*col & 0x0001);
               uint8_t r = (uint8_t)
                  (uint8_t)(percent_r * ((*col & 0xF800) >> 11) + (1-percent_r) * cr0);
               uint8_t g =
                  (uint8_t)(percent_g * ((*col & 0x07C0) >>  6) + (1-percent_g) * cg0);
               uint8_t b =
                  (uint8_t)(percent_b * ((*col & 0x003E) >>  1) + (1-percent_b) * cb0);
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
         case TMOD_TEX_INTER_COL_USING_TEXA:
            {
               uint8_t r = (uint8_t)(((modcolor >> 24) & 0xFF) / 255.f * 31.f);
               uint8_t g = (uint8_t)(((modcolor >> 16) & 0xFF) / 255.f * 31.f);
               uint8_t b = (uint8_t)(((modcolor >>  8) & 0xFF) / 255.f * 31.f);
               uint8_t a = (modcolor & 0xFF) ? 1 : 0;
               uint16_t col16 = ((r << 11)|(g << 6)|(b << 1) | a);

               while(--size)
               {
                  *col = (*col & 1) ? col16 : *col;
                  col++;
               };
            }
            break;
         case TMOD_TEX_MUL_COL:
            while(--size)
            {
               uint8_t a = (*col & 0x0001);
               uint8_t r = (((*col & 0xF800) >> 11) * cr0);
               uint8_t g = (((*col & 0x07C0) >> 6) * cg0);
               uint8_t b = (((*col & 0x003E) >> 1) * cb0);
               *col++    = PAL8toRGBA16(r, g, b, a);
            };
            break;
      }

      memcpy(rdp.pal_8, tmp_pal, 512);
   }

   cache->mod        = mod;
   cache->mod_color  = modcolor;
   cache->mod_color1 = modcolor1;
   cache->mod_factor = modfactor;

   result = 0;	// keep =0 so it doesn't mess up on the first split

   texture = (uint8_t *)tex1;

   {
      uint32_t size;
      int min_x, min_y;

      result = load_table[g_gdp.tile[td].size][g_gdp.tile[td].format]
         ((uintptr_t)(texture), (uintptr_t)(g_gdp.tmem)+(g_gdp.tile[td].tmem<<3),
          texinfo[id].wid_64, texinfo[id].height, texinfo[id].line, real_x, td);

      size = HIWORD(result);

      if (g_gdp.tile[td].mask_s != 0)
         min_x = MIN((int)real_x, 1 << g_gdp.tile[td].mask_s);
      else
         min_x = real_x;
      if (g_gdp.tile[td].mask_t != 0)
         min_y  = MIN((int)real_y, 1 << g_gdp.tile[td].mask_t);
      else
         min_y = real_y;

      // Load using mirroring/clamping
      if (min_x > texinfo[id].width && (signed)real_x > texinfo[id].width) /* real_x unsigned just for right shift */
         ClampTex(texture, texinfo[id].width, min_x, real_x, texinfo[id].height, size);

      if (texinfo[id].width < (int)real_x)
      {
         bool cond_true = g_gdp.tile[td].mask_s != 0
            && (real_x > (1U << g_gdp.tile[td].mask_s));

         if (g_gdp.tile[td].ms && cond_true)
         {
            MirrorTex((texture), g_gdp.tile[td].mask_s,
                  real_x, real_x, texinfo[id].height, size);
         }
         else if (cond_true)
         {
            // Horizontal Wrap (like mirror) ** UNTESTED **
            uint8_t *tex        = (uint8_t*)texture;
            uint32_t max_height = texinfo[id].height;
            uint8_t shift_a     = (size == 0) ? 2 : (size == 1) ? 1 : 0;
            uint32_t mask_width = (1 << g_gdp.tile[td].mask_s);
            uint32_t mask_mask  = (mask_width-1) >> shift_a;
            int32_t count       = (real_x - mask_width) >> shift_a;
            int32_t line_full   = real_x << size;
            int32_t line        = line_full - (count << 2);
            uint8_t *start      = (uint8_t*)(tex + (mask_width << size));
            uint32_t *v7        = (uint32_t *)start;

            do
            {
               int v9 = 0;
               do
               {
                  *v7++ = *(uint32_t *)&tex[4 * (mask_mask & v9++)];
               }while ( v9 != count );
               v7 = (uint32_t *)((int8_t*)v7 + line);
               tex += line_full;
            }while (--max_height);
         }
      }

      if (min_y > texinfo[id].height)
      {
         // Vertical Clamp
         int32_t line_full   = real_x << size;
         uint8_t *dst        = (uint8_t*)(texture + texinfo[id].height * line_full);
         uint8_t *const_line = (uint8_t*)(dst - line_full);
         int y               = texinfo[id].height;

         for (; y < min_y; y++)
         {
            memcpy ((void*)dst, (void*)const_line, line_full);
            dst += line_full;
         }
      }

      if (texinfo[id].height < (int)real_y)
      {
         if (g_gdp.tile[td].mt)
         {
            // Vertical Mirror
            if (g_gdp.tile[td].mask_t != 0 && (real_y > (1U << g_gdp.tile[td].mask_t)))
            {
               uint32_t mask_height = (1 << g_gdp.tile[td].mask_t);
               uint32_t mask_mask   = mask_height-1;
               int32_t line_full    = real_x << size;
               uint8_t *dst         = (uint8_t*)(texture + mask_height * line_full);
               uint32_t y           = mask_height;

               for (; y < real_y; y++)
               {
                  if (y & mask_height)   // mirrored
                     memcpy ((void*)dst, (void*)(texture + (mask_mask - (y & mask_mask)) * line_full), line_full);
                  else                   // not mirrored
                     memcpy ((void*)dst, (void*)(texture + (y & mask_mask) * line_full), line_full);

                  dst += line_full;
               }
            }
         }
         else if (g_gdp.tile[td].mask_t != 0 && real_y > (1U << g_gdp.tile[td].mask_t))
         {
            // Vertical Wrap
            uint32_t wrap_size   = size;
            uint32_t mask_height = (1 << g_gdp.tile[td].mask_t);
            uint32_t y           = mask_height;
            uint32_t mask_mask   = mask_height-1;
            int32_t line_full    = real_x << wrap_size;
            uint8_t *dst         = (uint8_t*)(texture + mask_height * line_full);

            if (wrap_size == 1)
               wrap_size = 0;

            for ( ;y < real_y; y++)
            {
               // not mirrored
               memcpy ((void*)dst, (void*)(texture + (y & mask_mask) * (line_full >> wrap_size)), (line_full >> wrap_size));
               dst += line_full;
            }
         }
      }
   }


   if (mod && !modifyPalette)
   {
	   int size        = real_x * real_y;
      uint32_t *src   = (uint32_t*)texture;
      uint32_t *dst   = tex2;
      unsigned texfmt = LOWORD(result);

      /* Convert the texture to ARGB 4444 */

      switch (texfmt)
      {
         case GR_TEXFMT_ARGB_1555:
            /* 2 pixels are converted in one loop
             * NOTE: width * height must be a multiple of 2 */

            size >>= 1;

            while (size--)
            {
               uint32_t col = *src++;
               *dst++ = ((col & 0x1E001E) >> 1) | ((col & 0x3C003C0) >> 2) | ((col & 0x78007800) >> 3) | ((col & 0x80008000) >> 3) | ((col & 0x80008000) >> 2) | ((col & 0x80008000) >> 1) | (col & 0x80008000);
            }

            texture = (uint8_t *)tex2;
            break;
         case GR_TEXFMT_ALPHA_INTENSITY_88:
            /* 2 pixels are converted in one loop
             * NOTE: width * height must be a multiple of 2 */
            size >>= 1;

            while (size--)
            {
               uint32_t col = *src++;
               *dst++ = (16 * (col & 0xF000F0) >> 8) | (col & 0xF000F0) | (16 * (col & 0xF000F0)) | (col & 0xF000F000);
            }
            texture = (uint8_t *)tex2;
            break;
         case GR_TEXFMT_ALPHA_INTENSITY_44:
            /* 4 pixels are converted in one loop
             * NOTE: width * height must be a multiple of 4 */

            size >>= 2;

            while (size--)
            {
               uint32_t col = *src++;
               *dst++ = ((((uint16_t)col << 8) & 0xFF00 & 0xF00u) >> 8) | ((((uint16_t)col << 8) & 0xFF00 & 0xF00u) >> 4) | (uint16_t)(((uint16_t)col << 8) & 0xFF00) | (((col << 16) & 0xF000000) >> 8) | (((col << 16) & 0xF000000) >> 4) | ((col << 16) & 0xFF000000);
               *dst++ = (((col >> 8) & 0xF00) >> 8) | (((col >> 8) & 0xF00) >> 4) | ((col >> 8) & 0xFF00) | ((col & 0xF000000) >> 8) | ((col & 0xF000000) >> 4) | (col & 0xFF000000);
            }

            texture = (uint8_t *)tex2;
            break;
         case GR_TEXFMT_ALPHA_8:
            /* 4 pixels are converted in one loop
             * NOTE: width * height must be a multiple of 4 */

            size >>= 2;

            while (size--)
            {
               uint32_t col = *src++;
               *dst++ = ((col & 0xF0) << 8 >> 12) | (uint8_t)(col & 0xF0) | (16 * (uint8_t)(col & 0xF0) & 0xFFFFFFF) | ((uint8_t)(col & 0xF0) << 8) | (16 * (uint16_t)(col & 0xF000) & 0xFFFFF) | (((uint16_t)(col & 0xF000) << 8) & 0xFFFFFF) | (((uint16_t)(col & 0xF000) << 12) & 0xFFFFFFF) | ((uint16_t)(col & 0xF000) << 16);
               *dst++ = ((col & 0xF00000) >> 20) | ((col & 0xF00000) >> 16) | ((col & 0xF00000) >> 12) | ((col & 0xF00000) >> 8) | ((col & 0xF0000000) >> 12) | ((col & 0xF0000000) >> 8) | ((col & 0xF0000000) >> 4) | (col & 0xF0000000);
            }

            texture = (uint8_t *)tex2;
            break;
      }

      result = (1 << 16) | GR_TEXFMT_ARGB_4444;

      // Now convert the color to the same
      modcolor = ((modcolor & 0xF0000000) >> 16) | ((modcolor & 0x00F00000) >> 12) |
         ((modcolor & 0x0000F000) >> 8) | ((modcolor & 0x000000F0) >> 4);
      modcolor1 = ((modcolor1 & 0xF0000000) >> 16) | ((modcolor1 & 0x00F00000) >> 12) |
         ((modcolor1 & 0x0000F000) >> 8) | ((modcolor1 & 0x000000F0) >> 4);
      modcolor2 = ((modcolor2 & 0xF0000000) >> 16) | ((modcolor2 & 0x00F00000) >> 12) |
         ((modcolor2 & 0x0000F000) >> 8) | ((modcolor2 & 0x000000F0) >> 4);

      {
		  uint16_t *dst;
		  uint32_t cr0, cg0, cb0, cr1, cg1, cb1, cr2, cg2, cb2;
         size = (real_x * real_y) << 1;
         dst = (uint16_t*)texture;
         cr0 = (modcolor >> 12) & 0xF;
         cg0 = (modcolor >> 8) & 0xF;
         cb0 = (modcolor >> 4) & 0xF;
         cr1 = (modcolor1 >> 12) & 0xF;
         cg1 = (modcolor1 >> 8) & 0xF;
         cb1 = (modcolor1 >> 4) & 0xF;
         cr2 = (modcolor2 >> 12) & 0xF;
         cg2 = (modcolor2 >> 8) & 0xF;
         cb2 = (modcolor2 >> 4) & 0xF;

         switch (mod)
         {
            case TMOD_TEX_INTER_COLOR_USING_FACTOR:
               {
                  float percent = modfactor / 255.0f;

                  while (size--)
                  {
                     uint8_t r = (uint8_t)
                        ((1 - percent) * (((*dst) >> 8) & 0xF) + percent * cr0);
                     uint8_t g = (uint8_t)
                        ((1 - percent) * (((*dst) >> 4) & 0xF) + percent * cg0);
                     uint8_t b = (uint8_t)
                        ((1 - percent) * ((*dst) & 0xF) + percent * cb0);
                     *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_TEX_INTER_COL_USING_COL1:
               {
                  float percent_r = cr1 / 15.0f;
                  float percent_g = cg1 / 15.0f;
                  float percent_b = cb1 / 15.0f;

                  while (size--)
                  {
                     uint8_t r = (uint8_t)
                        ((1 - percent_r) * (((*dst) >> 8) & 0xF) + percent_r * cr0);
                     uint8_t g = (uint8_t)
                        ((1 - percent_g) * (((*dst) >> 4) & 0xF) + percent_g * cg0);
                     uint8_t b = (uint8_t)
                        ((1 - percent_b) * ((*dst) & 0xF) + percent_b * cb0);
                     *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_FULL_COLOR_SUB_TEX:
               {
                  while (size--)
                  {
                     uint8_t a = ((modcolor & 0xF) - (((*dst) >> 12) & 0xF));
                     uint8_t r = (cr0 - (((*dst) >> 8) & 0xF));
                     uint8_t g = (cg0 - (((*dst) >> 4) & 0xF));
                     uint8_t b = (cb0 - (*dst & 0xF));
                     *dst = (a << 12) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_COL_INTER_COL1_USING_TEX:
               while (size--)
               {
                  float percent_r = (((*dst) >> 8) & 0xF) / 15.0f;
                  float percent_g = (((*dst) >> 4) & 0xF) / 15.0f;
                  float percent_b = ((*dst) & 0xF) / 15.0f;
                  uint8_t r = (uint8_t)((1.0f-percent_r) * cr0 + percent_r * cr1 + 0.0001f);
                  uint8_t g = (uint8_t)((1.0f-percent_g) * cg0 + percent_g * cg1 + 0.0001f);
                  uint8_t b = (uint8_t)((1.0f-percent_b) * cb0 + percent_b * cb1 + 0.0001f);
                  *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                  dst++;
               }
               break;
            case TMOD_COL_INTER_COL1_USING_TEXA:
            case TMOD_COL_INTER_COL1_USING_TEXA__MUL_TEX:
               {

                  if (mod == TMOD_COL_INTER_COL1_USING_TEXA__MUL_TEX)
                  {
                     while (size--)
                     {
                        float percent = ((*dst & 0xF000) >> 12) / 15.0f;
                        uint8_t r = (uint8_t)
                           ((((1 - percent) * cr0 + percent * cr1) / 15.0f) * ((((*dst) & 0x0F00) >> 8) / 15.0f) * 15.0f);
                        uint8_t g = (uint8_t)
                           ((((1 - percent) * cg0 + percent * cg1) / 15.0f) * ((((*dst) & 0x00F0) >> 4) / 15.0f) * 15.0f);
                        uint8_t b = (uint8_t)
                           ((((1 - percent) * cb0 + percent * cb1) / 15.0f) * (((*dst) & 0x000F) / 15.0f) * 15.0f);
                        *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                        dst++;
                     }
                  }
                  else
                  {
                     while (size--)
                     {
                        float percent = ((*dst & 0xF000) >> 12) / 15.0f;
                        uint8_t r = (uint8_t)((1 - percent)*cr0 + percent*cr1);
                        uint8_t g = (uint8_t)((1 - percent)*cg0 + percent*cg1);
                        uint8_t b = (uint8_t)((1 - percent)*cb0 + percent*cb1);
                        *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                        dst++;
                     }
                  }
               }
               break;
            case TMOD_COL_INTER_TEX_USING_TEX:
            case TMOD_COL_INTER_TEX_USING_TEXA:
               {
                  if (mod == TMOD_COL_INTER_TEX_USING_TEX)
                  {
                     while (size--)
                     {
                        float percent_r = (((*dst) >> 8) & 0xF) / 15.0f;
                        float percent_g = (((*dst) >> 4) & 0xF) / 15.0f;
                        float percent_b = ((*dst) & 0xF) / 15.0f;
                        uint8_t r = (uint8_t)
                           ((1.0f-percent_r) * cr0 + percent_r * (((*dst) & 0x0F00) >> 8));
                        uint8_t g = (uint8_t)
                           ((1.0f-percent_g) * cg0 + percent_g * (((*dst) & 0x00F0) >> 4));
                        uint8_t b = (uint8_t)
                           ((1.0f-percent_b) * cb0 + percent_b * ((*dst) & 0x000F));
                        *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                        dst++;
                     }
                  }
                  else
                  {
                     while (size--)
                     {
                        float percent = ((*dst & 0xF000) >> 12) / 15.0f;
                        uint8_t r = (uint8_t)
                           ((1 - percent)*cr0 + percent*((*dst & 0x0F00) >> 8));
                        uint8_t g = (uint8_t)
                           ((1 - percent)*cg0 + percent*((*dst & 0x00F0) >> 4));
                        uint8_t b = (uint8_t)
                           ((1 - percent)*cb0 + percent*(*dst & 0x000F));
                        *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                        dst++;
                     }
                  }
               }
               break;
            case TMOD_COL2_INTER__COL_INTER_COL1_USING_TEX__USING_TEXA:
               {
                  while (size--)
                  {
                     float percent_a = ((*dst & 0xF000) >> 12) / 15.0f;
                     float percent_r = (((*dst) >> 8) & 0xF) / 15.0f;
                     float percent_g = (((*dst) >> 4) & 0xF) / 15.0f;
                     float percent_b = (*dst & 0xF) / 15.0f;
                     uint8_t r = (uint8_t)
                        (((1.0f-percent_r)*cr0 + percent_r*cr1)*percent_a + cr2*(1.0f-percent_a));
                     uint8_t g = (uint8_t)
                        (((1.0f-percent_g)*cg0 + percent_g*cg1)*percent_a + cg2*(1.0f-percent_a));
                     uint8_t b = (uint8_t)
                        (((1.0f-percent_b)*cb0 + percent_b*cb1)*percent_a + cb2*(1.0f-percent_a));
                     *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_TEX_SCALE_FAC_ADD_FAC:
               {
                  float base_a_plus_percent = ((1.0f - (modfactor / 255.0f)) * 15.0f) + (modfactor / 255.0f);

                  while (size--)
                  {
                     *dst = ((uint16_t)(base_a_plus_percent * ((*dst) >> 12)) << 12) | ((*dst) & 0x0FFF);
                      dst++;
                  }
               }
               break;
            case TMOD_TEX_SUB_COL_MUL_FAC_ADD_TEX:
               {
                  float percent = modfactor / 255.0f;

                  while (size--)
                  {
                     float r = (float)(((*dst) >> 8) & 0xF);
                     float g = (float)(((*dst) >> 4) & 0xF);
                     float b = (float)(*dst & 0xF);
                     r       = (r - cr0) * percent + r;
                     r       = clamp_float(r, 0.0f, 15.0f);
                     g       = (g - cg0) * percent + g;
                     g       = clamp_float(g, 0.0f, 15.0f);
                     b       = (b - cb0) * percent + b;
                     b       = clamp_float(b, 0.0f, 15.0f);

                     *dst    = (*dst & 0xF000) | ((uint16_t)r << 8) | ((uint16_t)g << 4) | (uint16_t)b;
                     dst++;
                  }
               }
               break;
            case TMOD_TEX_SCALE_COL_ADD_COL:
               while (size--)
               {
                  float percent_r = (((*dst) >> 8) & 0xF) / 15.0f;
                  float percent_g = (((*dst) >> 4) & 0xF) / 15.0f;
                  float percent_b = ((*dst) & 0xF) / 15.0f;
                  uint8_t r = (uint8_t)(percent_r * cr0 + cr1 + 0.0001f);
                  uint8_t g = (uint8_t)(percent_g * cg0 + cg1 + 0.0001f);
                  uint8_t b = (uint8_t)(percent_b * cb0 + cb1 + 0.0001f);
                  *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                  dst++;
               }
               break;
            case TMOD_TEX_ADD_COL:
               while (size--)
               {
                  uint8_t r = (uint8_t)(cr0 + (((*dst) >> 8) & 0xF))&0xF;
                  uint8_t g = (uint8_t)(cg0 + (((*dst) >> 4) & 0xF))&0xF;
                  uint8_t b = (uint8_t)(cb0 + ((*dst) & 0xF))&0xF;
                  *dst = ((((*dst) >> 12) & 0xF) << 12) | (r << 8) | (g << 4) | b;
                  dst++;
               }
               break;
            case TMOD_TEX_SUB_COL:
               {
                  while (size--)
                  {
                     uint8_t r = (((*dst) >> 8) & 0xF) - cr0;
                     uint8_t g = (((*dst) >> 4) & 0xF) - cg0;
                     uint8_t b = (((*dst) & 0xF) - cb0) - cb0;
                     *dst = (((*dst) & 0xF000) << 12) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_TEX_SUB_COL_MUL_FAC:
               {
                  float percent = modfactor / 255.0f;

                  while (size--)
                  {
                     float r = ((float)(((*dst) >> 8) & 0xF) - cr0) * percent;
                     float g = ((float)(((*dst) >> 4) & 0xF) - cg0) * percent;
                     float b = ((float)(*dst & 0xF) - cb0) * percent;

                     r       = clamp_float(r, 0.0f, 15.0f);
                     g       = clamp_float(g, 0.0f, 15.0f);
                     b       = clamp_float(b, 0.0f, 15.0f);

                     *dst = ((((*dst) >> 12) & 0xF) << 12) | ((uint16_t)r << 8) | ((uint16_t)g << 4) | (uint16_t)b;
                     dst++;
                  }
               }
               break;
            case TMOD_COL_INTER_TEX_USING_COL1:
               {
                  float percent_r = cr1 / 15.0f;
                  float percent_g = cg1 / 15.0f;
                  float percent_b = cb1 / 15.0f;

                  while (size--)
                  {
                     uint8_t r = (uint8_t)
                        (percent_r * (((*dst) >> 8) & 0xF) + (1 - percent_r) * cr0);
                     uint8_t g = (uint8_t)
                        (percent_g * (((*dst) >> 4) & 0xF) + (1 - percent_g) * cg0);
                     uint8_t b = (uint8_t)
                        (percent_b * ((*dst) & 0xF) + (1 - percent_b) * cb0);
                     *dst = ((((*dst) >> 12) & 0xF) << 12) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_COL_MUL_TEXA_ADD_TEX:
               while (size--)
               {
                  float factor = ((*dst & 0xF000) >> 12) / 15.0f;
                  uint8_t r = (uint8_t)(cr0 * factor + (((*dst) >> 8) & 0xF))&0xF;
                  uint8_t g = (uint8_t)(cg0 * factor + (((*dst) >> 4) & 0xF))&0xF;
                  uint8_t b = (uint8_t)(cb0 * factor + ((*dst) & 0xF))&0xF;
                  *dst = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                  dst++;
               }
               break;
            case TMOD_TEX_INTER_NOISE_USING_COL:
               {
                  float percent_r = cr0 / 15.0f;
                  float percent_g = cg0 / 15.0f;
                  float percent_b = cb0 / 15.0f;

                  while (size--)
                  {
                     uint8_t noise = rand()%16;
                     uint8_t r = (uint8_t)
                        ((1 - percent_r)*(((*dst) >> 8) & 0xF) + percent_r*noise);
                     uint8_t g = (uint8_t)
                        ((1 - percent_g)*(((*dst) >> 4) & 0xF) + percent_g*noise);
                     uint8_t b = (uint8_t)
                        ((1 - percent_b)*(*dst & 0xF) + percent_b*noise);
                     *dst      = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
            case TMOD_TEX_INTER_COL_USING_TEXA:
               while (size--)
               {
                  float percent = ((*dst & 0xF000) >> 12) / 15.0f;
                  uint8_t r = (uint8_t)
                     (percent*cr0 + (1 - percent)*((*dst & 0x0F00) >> 8));
                  uint8_t g = (uint8_t)
                     (percent*cg0 + (1 - percent)*((*dst & 0x00F0) >> 4));
                  uint8_t b = (uint8_t)
                     (percent*cb0 + (1 - percent)*(*dst & 0x000F));
                  *dst      = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                  dst++;
               }
               break;
            case TMOD_TEX_MUL_COL:
               while (size--)
               {
                  uint8_t r = (cr0 * ((*dst & 0x0F00) >> 8));
                  uint8_t g = (cg0 * ((*dst & 0x00F0) >> 4));
                  uint8_t b = (cb0 * (*dst & 0x000F));
                  *dst      = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                  dst++;
               }
               break;
            case TMOD_TEX_SCALE_FAC_ADD_COL:
               {
                  float percent = modfactor / 255.0f;

                  while (size--)
                  {
                     uint8_t r = (uint8_t)(cr0 + percent*(((*dst) >> 8) & 0xF));
                     uint8_t g = (uint8_t)(cg0 + percent*(((*dst) >> 4) & 0xF));
                     uint8_t b = (uint8_t)(cb0 + percent*(((*dst) >> 0) & 0xF));
                     *dst      = (*dst & 0xF000) | (r << 8) | (g << 4) | b;
                     dst++;
                  }
               }
               break;
         }
      }
   }


   cache->t_info.format = LOWORD(result);
   cache->realwidth     = real_x;
   cache->realheight    = real_y;
   cache->lod           = lod;
   cache->aspect        = aspect;

   {
	   uint32_t texture_size, tex_addr;
	   GrTexInfo *t_info;

      /* Load the texture into texture memory */
      t_info                  = (GrTexInfo*)&cache->t_info;
      t_info->data            = texture;
      t_info->smallLodLog2    = lod;
      t_info->largeLodLog2    = lod;
      t_info->aspectRatioLog2 = aspect;

      texture_size            = grTexCalcMemRequired (t_info->largeLodLog2, t_info->aspectRatioLog2, t_info->format);

      /* Check for end of memory (too many textures to fit, clear cache) */
      if (voodoo.tmem_ptr[tmu]+texture_size >= voodoo.tex_max_addr)
      {
         LRDP("Cache size reached, clearing...\n");
         ClearCache ();

         if (id == 1 && rdp.tex == 3)
            LoadTex (0, rdp.t0);

         LoadTex (id, tmu);
         /* Don't continue (already done) */
         return;

      }

      tex_addr = GetTexAddrUMA(tmu, texture_size);
      grTexSource (tmu,
            tex_addr,
            GR_MIPMAPLEVELMASK_BOTH,
            t_info,
            true);
   }
}
