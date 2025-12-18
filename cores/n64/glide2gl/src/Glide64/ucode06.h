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
// * Do NOT send me the whole project or file that you modified->  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

#include "../../Graphics/HLE/Microcode/S2DEX.h"
#include "../../Graphics/image_convert.h"


typedef struct DRAWIMAGE_t
{
  float frameX;
  float frameY;
  uint16_t frameW;
  uint16_t frameH;
  uint16_t imageX;
  uint16_t imageY;
  uint16_t imageW;
  uint16_t imageH;
  uint32_t imagePtr;
  uint8_t imageFmt;
  uint8_t imageSiz;
  uint16_t imagePal;
  uint8_t flipX;
  uint8_t flipY;
  float scaleX;
  float scaleY;
} DRAWIMAGE;

static void DrawDepthImage (const DRAWIMAGE *d)
{
   float scale_x_src, scale_y_src, scale_x_dst, scale_y_dst;
   uint16_t *src, *dst;
   int32_t x, y, src_width, src_height, dst_width, dst_height;

   if (!fb_depth_render_enabled || d->imageH > d->imageW)
      return;

   scale_x_dst = rdp.scale_x;
   scale_y_dst = rdp.scale_y;
   scale_x_src = 1.0f/rdp.scale_x;
   scale_y_src = 1.0f/rdp.scale_y;
   src_width   = d->imageW;
   src_height  = d->imageH;
   dst_width   = MIN((int)(src_width*scale_x_dst), (int)settings.scr_res_x);
   dst_height  = MIN((int)(src_height*scale_y_dst), (int)settings.scr_res_y);
   src         = (uint16_t*)(gfx_info.RDRAM+d->imagePtr);
   dst         = (uint16_t*)malloc(dst_width * dst_height * sizeof(uint16_t));

   for (y = 0; y < dst_height; y++)
   {
      for (x = 0; x < dst_width; x++)
         dst[x + y * dst_width] = src[((int)(x * scale_x_src) + (int)(y*scale_y_src) * src_width)^1];
   }
   grLfbWriteRegion(GR_BUFFER_AUXBUFFER,
         0,
         0,
         GR_LFB_SRC_FMT_ZA16,
         dst_width,
         dst_height,
         FXFALSE,
         dst_width<<1,
         dst);
   free(dst);
}

static void DrawImage (DRAWIMAGE *d)
{
   int x_size, y_size, x_shift, y_shift, line;
   int min_wrap_u, min_wrap_v, min_256_u, min_256_v;
   float nul_y, nlr_x;
   int nul_v, nlr_u;
   int cur_wrap_v, cur_v;
   int cb_v; // coordinate-base
   int tb_v; // texture-base
   float ful_u, ful_v, flr_u, flr_v;
   float ful_x, ful_y, flr_x, flr_y, mx, bx, my, by;
   float Z;
   struct gDPTile *tile;
   int ul_u, ul_v, lr_u, lr_v;
   float ul_x, ul_y, lr_x, lr_y;

   if (d->imageW == 0 || d->imageH == 0 || d->frameH == 0)
      return;

   // choose optimum size for the format/size
   switch (d->imageSiz)
   {
      case 0:
         y_size  = 32;
         y_shift = 5;
         if (rdp.tlut_mode < G_TT_RGBA16)
         {
            y_size  = 64;
            y_shift = 6;
         }
         x_size  = 128;
         x_shift = 7;
         line    = 8;
         break;
      case 1:
         y_size  = 32;
         y_shift = 5;
         if (rdp.tlut_mode < G_TT_RGBA16)
         {
            y_size  = 64;
            y_shift = 6;
         }
         x_size  = 64;
         x_shift = 6;
         line    = 8;
         break;
      case 2:
         x_size  = 64;
         y_size  = 32;
         x_shift = 6;
         y_shift = 5;
         line    = 16;
         break;
      case 3:
         x_size  = 32;
         y_size  = 16;
         x_shift = 4;
         y_shift = 3;
         line    = 16;
         break;
      default:
         FRDP("DrawImage. unknown image size: %d\n", d->imageSiz);
         return;
   }

   if (gDP.colorImage.width == 512 && !no_dlist) //RE2
   {
      uint16_t width = (uint16_t)(*gfx_info.VI_WIDTH_REG & 0xFFF);
      d->frameH      = d->imageH = (d->frameW * d->frameH)/width;
      d->frameW      = d->imageW = width;

      if (g_gdp.zb_address == gDP.colorImage.address)
      {
         DrawDepthImage(d);
         g_gdp.flags |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE |
            UPDATE_ALPHA_COMPARE | UPDATE_VIEWPORT;
         return;
      }
   }

   if ((settings.hacks&hack_PPL) > 0)
   {
      if (d->imageY > d->imageH)
         d->imageY = (d->imageY % d->imageH);
   }
   else
   if ((settings.hacks&hack_Starcraft) > 0)
   {
      if (d->imageH%2 == 1)
         d->imageH -= 1;
   }
   else
   {
      if ( (d->frameX > 0) && (d->frameW == gDP.colorImage.width) )
         d->frameW -= (uint16_t)(2.0f*d->frameX);
      if ( (d->frameY > 0) && (d->frameH == gDP.colorImage.height) )
         d->frameH -= (uint16_t)(2.0f*d->frameY);
   }


   ul_u = (int)d->imageX;
   ul_v = (int)d->imageY;
   lr_u = (int)d->imageX + (int)(d->frameW * d->scaleX);
   lr_v = (int)d->imageY + (int)(d->frameH * d->scaleY);

   ul_x = d->frameX;
   lr_x = d->frameX + d->frameW;
   ul_y = d->frameY;
   lr_y = d->frameY + d->frameH;

   if (d->flipX)
   {
      ul_x = d->frameX + d->frameW;
      lr_x = d->frameX;
   }
   if (d->flipY)
   {
      ul_y = d->frameY + d->frameH;
      lr_y = d->frameY;
   }

   min_wrap_u = ul_u / d->imageW;
   min_wrap_v = ul_v / d->imageH;
   min_256_u  = ul_u >> x_shift;
   min_256_v  = ul_v >> y_shift;

   glide64gDPSetTextureImage(
         d->imageFmt,                              /* format - RGBA */
         d->imageSiz,                              /* size   - 16bit */
         (d->imageW%2)?d->imageW-1:d->imageW,      /* width */
         d->imagePtr                               /* address */
         );

   rdp.timg.set_by        = 0;

   glide64gDPSetTile(
         d->imageFmt,               /* RGBA */
         d->imageSiz,               /* 16bit */
         line,                      /* line */
         0,                         /* tmem */
         0,                         /* tile */
         (uint8_t)d->imagePal,      /* palette */
         0,                         /* cmt */
         0,                         /* mask_t */
         0,                         /* shift_t */
         0,                         /* cms */
         0,                         /* mask_s */
         0                          /* shift_s */
         );

   glide64gDPSetTileSize(
         0,                /* tile */
         0,                /* ulx  */
         0,                /* uly  */
         x_size - 1,       /* lrs  */
         y_size - 1        /* lrt  */
         );

   Z = set_sprite_combine_mode ();
   if (((gDP.otherMode.h & RDP_CYCLE_TYPE) >> 20) == G_CYC_COPY)
      rdp.allow_combine = 0;

   {
      uint32_t minx = 0;
      uint32_t miny = 0;
      uint32_t maxx, maxy;
      if (gDP.colorImage.width == 512 && !no_dlist)
      {
         maxx = settings.scr_res_x;
         maxy = settings.scr_res_y;
      }
      else if (d->scaleX == 1.0f && d->scaleY == 1.0f)
      {
         minx = rdp.scissor.ul_x;
         miny = rdp.scissor.ul_y;
         maxx = rdp.scissor.lr_x;
         maxy = rdp.scissor.lr_y;
      }
      else
      {
         minx = rdp.scissor.ul_x;
         miny = rdp.scissor.ul_y;
         maxx = MIN(rdp.scissor.lr_x, (uint32_t)((d->frameX+d->imageW/d->scaleX+0.5f)*rdp.scale_x));
         maxy = MIN(rdp.scissor.lr_y, (uint32_t)((d->frameY+d->imageH/d->scaleY+0.5f)*rdp.scale_y));
      }
      grClipWindow(minx, miny, maxx, maxy);
      g_gdp.flags |= UPDATE_SCISSOR;
   }

   // Texture ()
   rdp.cur_tile = 0;

   mx    = (float)(lr_x - ul_x) / (float)(lr_u - ul_u);
   bx    = ul_x - mx * ul_u;

   my    = (float)(lr_y - ul_y) / (float)(lr_v - ul_v);
   by    = ul_y - my * ul_v;


   nul_v = ul_v;
   nul_y = ul_y;

   // #162

   cur_wrap_v = min_wrap_v + 1;
   cur_v      = min_256_v + 1;
   cb_v       = ((cur_v-1)<<y_shift);
   while (cb_v >= d->imageH) cb_v -= d->imageH;
   tb_v       = cb_v;
   rdp.bg_image_height = d->imageH;

   while (1)
   {
      int nul_u, cb_u, tb_u;
      float nul_x, nlr_y;
      int cur_wrap_u = min_wrap_u + 1;
      int cur_u      = min_256_u + 1;

      // calculate intersection with this point
      int nlr_v      = MIN (MIN (cur_wrap_v*d->imageH, (cur_v<<y_shift)), lr_v);
      nlr_y          = my * nlr_v + by;

      nul_u          = ul_u;
      nul_x          = ul_x;
      cb_u           = ((cur_u-1)<<x_shift);
      while (cb_u >= d->imageW)
         cb_u       -= d->imageW;
      tb_u           = cb_u;

      while (1)
      {
         // calculate intersection with this point
         nlr_u = MIN (MIN (cur_wrap_u * d->imageW, (cur_u<<x_shift)), lr_u);
         nlr_x = mx * nlr_u + bx;

         // ** Load the texture, constant portions have been set above
         glide64gDPSetTileSize(
               0,                /* tile */
               tb_u,             /* uls  */
               tb_v,             /* ult  */
               tb_u+x_size-1,    /* lrs  */
               tb_v+y_size-1     /* lrt  */
               );

         __RSP.w0 = ((int)g_gdp.tile[0].sh << 14) | ((int)g_gdp.tile[0].th << 2);
         __RSP.w1 = ((int)g_gdp.tile[0].sl << 14) | ((int)g_gdp.tile[0].tl << 2);
         glide64gDPLoadTile(
               (uint32_t)((__RSP.w1 >> 24) & 0x07),      /* tile */
               (uint32_t)((__RSP.w0 >> 14) & 0x03FF),    /* ul_s */
               (uint32_t)((__RSP.w0 >> 2 ) & 0x03FF),    /* ul_t */
               (uint32_t)((__RSP.w1 >> 14) & 0x03FF),    /* lr_s */
               (uint32_t)((__RSP.w1 >> 2 ) & 0x03FF)     /* lr_t */
               );
         TexCache ();

         ful_u = (float)nul_u - cb_u;
         flr_u = (float)nlr_u - cb_u;
         ful_v = (float)nul_v - cb_v;
         flr_v = (float)nlr_v - cb_v;

         ful_u *= rdp.cur_cache[0]->c_scl_x;
         ful_v *= rdp.cur_cache[0]->c_scl_y;
         flr_u *= rdp.cur_cache[0]->c_scl_x;
         flr_v *= rdp.cur_cache[0]->c_scl_y;

         ful_x = nul_x * rdp.scale_x + rdp.offset_x;
         flr_x = nlr_x * rdp.scale_x + rdp.offset_x;
         ful_y = nul_y * rdp.scale_y + rdp.offset_y;
         flr_y = nlr_y * rdp.scale_y + rdp.offset_y;

         /* Make the vertices */

         if ((flr_x <= rdp.scissor.lr_x) || (ful_x < rdp.scissor.lr_x))
         {
            VERTEX v[4];

            v[0].x = ful_x;
            v[0].y = ful_y;
            v[0].z = Z;
            v[0].q = 1.0f;
            v[0].u[0] = ful_u;
            v[0].v[0] = ful_v;

            v[1].x = flr_x;
            v[1].y = ful_y;
            v[1].z = Z;
            v[1].q = 1.0f;
            v[1].u[0] = flr_u;
            v[1].v[0] = ful_v;

            v[2].x = ful_x;
            v[2].y = flr_y;
            v[2].z = Z;
            v[2].q = 1.0f;
            v[2].u[0] = ful_u;
            v[2].v[0] = flr_v;

            v[3].x = flr_x;
            v[3].y = flr_y;
            v[3].z = Z;
            v[3].q = 1.0f;
            v[3].u[0] = flr_u;
            v[3].v[0] = flr_v;

            apply_shading(v);
            ConvertCoordsConvert (v, 4);
            grDrawVertexArrayContiguous (GR_TRIANGLE_STRIP, 4, v);
         }

         // increment whatever caused this split
         tb_u += x_size - (x_size-(nlr_u-cb_u));
         cb_u  = nlr_u;
         if (nlr_u == cur_wrap_u * d->imageW)
         {
            cur_wrap_u ++;
            tb_u = 0;
         }
         if (nlr_u == (cur_u<<x_shift))
            cur_u ++;
         if (nlr_u == lr_u)
            break;
         nul_u = nlr_u;
         nul_x = nlr_x;
      }

      tb_v += y_size - (y_size-(nlr_v-cb_v));
      cb_v = nlr_v;
      if (nlr_v == cur_wrap_v* d->imageH)
      {
         cur_wrap_v ++;
         tb_v = 0;
      }
      if (nlr_v == (cur_v<<y_shift))
         cur_v ++;
      if (nlr_v == lr_v)
         break;
      nul_v = nlr_v;
      nul_y = nlr_y;
   }

   rdp.allow_combine = 1;
   rdp.bg_image_height = 0xFFFF;
}

//****************************************************************

static void uc6_read_background_data (DRAWIMAGE *d, bool bReadScale)
{
   int imageYorig;
   uint16_t imageFlip;
   uint32_t addr  = RSP_SegmentToPhysical(__RSP.w1) >> 1;

   d->imageX      = (((uint16_t *)gfx_info.RDRAM)[(addr+0)^1] >> 5);   // 0
   d->imageW      = (((uint16_t *)gfx_info.RDRAM)[(addr+1)^1] >> 2);   // 1
   d->frameX      = ((int16_t*)gfx_info.RDRAM)[(addr+2)^1] / 4.0f;       // 2
   d->frameW      = ((uint16_t *)gfx_info.RDRAM)[(addr+3)^1] >> 2;             // 3

   d->imageY      = (((uint16_t *)gfx_info.RDRAM)[(addr+4)^1] >> 5);   // 4
   d->imageH      = (((uint16_t *)gfx_info.RDRAM)[(addr+5)^1] >> 2);   // 5
   d->frameY      = ((int16_t*)gfx_info.RDRAM)[(addr+6)^1] / 4.0f;       // 6
   d->frameH      = ((uint16_t *)gfx_info.RDRAM)[(addr+7)^1] >> 2;             // 7

   d->imagePtr    = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr+8)>>1]);       // 8,9
   d->imageFmt    = ((uint8_t *)gfx_info.RDRAM)[(((addr+11)<<1)+0)^3]; // 11
   d->imageSiz    = ((uint8_t *)gfx_info.RDRAM)[(((addr+11)<<1)+1)^3]; // |
   d->imagePal    = ((uint16_t *)gfx_info.RDRAM)[(addr+12)^1]; // 12
   imageFlip      = ((uint16_t *)gfx_info.RDRAM)[(addr+13)^1];    // 13;
   d->flipX       = (uint8_t)imageFlip & G_BG_FLAG_FLIPS;

   if (bReadScale)
   {
      d->scaleX   = ((int16_t*)gfx_info.RDRAM)[(addr+14)^1] / 1024.0f;  // 14
      d->scaleY   = ((int16_t*)gfx_info.RDRAM)[(addr+15)^1] / 1024.0f;  // 15
   }
   else
      d->scaleX   = d->scaleY = 1.0f;

   d->flipY       = 0;
   imageYorig     = ((int *)gfx_info.RDRAM)[(addr+16)>>1] >> 5;
   rdp.last_bg    = d->imagePtr;
}

static void uc6_bg(bool first_cycle)
{
   DRAWIMAGE d;

   if (rdp.skip_drawing)
      return;

   uc6_read_background_data(&d, first_cycle);

   if (settings.ucode == ucode_F3DEX2 || (settings.hacks&hack_PPL))
   {
      /* can't draw from framebuffer */
      if (d.imagePtr == gDP.colorImage.address || d.imagePtr == rdp.ocimg)
         return;
      if (!d.imagePtr)
         return;
   }

   DrawImage(&d);
}

static void uc6_bg_1cyc(uint32_t w0, uint32_t w1)
{
   uc6_bg(true);
}

static void uc6_bg_copy(uint32_t w0, uint32_t w1)
{
   uc6_bg(false);
}

static void draw_split_triangle(VERTEX **vtx)
{
  int index,i,j, min_256,max_256, cur_256;
  float percent;

  vtx[0]->not_zclipped = 1;
  vtx[1]->not_zclipped = 1;
  vtx[2]->not_zclipped = 1;

  min_256 = MIN((int)vtx[0]->u[0],(int)vtx[1]->u[0]); // bah, don't put two mins on one line
  min_256 = MIN(min_256,(int)vtx[2]->u[0]) >> 8;  // or it will be calculated twice

  max_256 = MAX((int)vtx[0]->u[0],(int)vtx[1]->u[0]); // not like it makes much difference
  max_256 = MAX(max_256,(int)vtx[2]->u[0]) >> 8;  // anyway :P

  for (cur_256=min_256; cur_256<=max_256; cur_256++)
  {
    int left_256 = cur_256 << 8;
    int right_256 = (cur_256+1) << 8;

    // Set vertex buffers
    rdp.vtxbuf = rdp.vtx1;  // copy from v to rdp.vtx1
    rdp.vtxbuf2 = rdp.vtx2;
    rdp.vtx_buffer = 0;
    rdp.n_global   = 3;
    index          = 0;

    // ** Left plane **
    for (i=0; i<3; i++)
    {
       VERTEX *v2 = NULL;
       VERTEX *v1 = (VERTEX*)vtx[i];

       j = i+1;
       if (j == 3)
          j = 0;

       v2 = (VERTEX*)vtx[j];

       if (v1->u[0] >= left_256)
       {
          if (v2->u[0] >= left_256)   // Both are in, save the last one
          {
             rdp.vtxbuf[index]         = *v2;
             rdp.vtxbuf[index].u[0]   -= left_256;
             rdp.vtxbuf[index++].v[0] += rdp.cur_cache[0]->c_scl_y * (cur_256 * rdp.cur_cache[0]->splitheight);
          }
          else      // First is in, second is out, save intersection
          {
             percent = (left_256 - v1->u[0]) / (v2->u[0] - v1->u[0]);
             rdp.vtxbuf[index].x    = v1->x + (v2->x - v1->x) * percent;
             rdp.vtxbuf[index].y    = v1->y + (v2->y - v1->y) * percent;
             rdp.vtxbuf[index].z    = 1;
             rdp.vtxbuf[index].q    = 1;
             rdp.vtxbuf[index].u[0] = 0.5f;
             rdp.vtxbuf[index].v[0] = v1->v[0] + (v2->v[0] - v1->v[0]) * percent +
                rdp.cur_cache[0]->c_scl_y * cur_256 * rdp.cur_cache[0]->splitheight;
             rdp.vtxbuf[index].b    = (uint8_t)(v1->b + (v2->b - v1->b) * percent);
             rdp.vtxbuf[index].g    = (uint8_t)(v1->g + (v2->g - v1->g) * percent);
             rdp.vtxbuf[index].r    = (uint8_t)(v1->r + (v2->r - v1->r) * percent);
             rdp.vtxbuf[index++].a  = (uint8_t)(v1->a + (v2->a - v1->a) * percent);
          }
       }
       else
       {
          if (v2->u[0] >= left_256) // First is out, second is in, save intersection & in point
          {
             percent = (left_256 - v2->u[0]) / (v1->u[0] - v2->u[0]);
             rdp.vtxbuf[index].x = v2->x + (v1->x - v2->x) * percent;
             rdp.vtxbuf[index].y = v2->y + (v1->y - v2->y) * percent;
             rdp.vtxbuf[index].z = 1;
             rdp.vtxbuf[index].q = 1;
             rdp.vtxbuf[index].u[0] = 0.5f;
             rdp.vtxbuf[index].v[0] = v2->v[0] + (v1->v[0] - v2->v[0]) * percent +
                rdp.cur_cache[0]->c_scl_y * cur_256 * rdp.cur_cache[0]->splitheight;
             rdp.vtxbuf[index].b = (uint8_t)(v2->b + (v1->b - v2->b) * percent);
             rdp.vtxbuf[index].g = (uint8_t)(v2->g + (v1->g - v2->g) * percent);
             rdp.vtxbuf[index].r = (uint8_t)(v2->r + (v1->r - v2->r) * percent);
             rdp.vtxbuf[index++].a = (uint8_t)(v2->a + (v1->a - v2->a) * percent);

             // Save the in point
             rdp.vtxbuf[index] = *v2;
             rdp.vtxbuf[index].u[0] -= left_256;
             rdp.vtxbuf[index++].v[0] += rdp.cur_cache[0]->c_scl_y * (cur_256 * rdp.cur_cache[0]->splitheight);
          }
       }
    }
    rdp.n_global = index;

    rdp.vtxbuf = rdp.vtx2;  // now vtx1 holds the value, & vtx2 is the destination
    rdp.vtxbuf2 = rdp.vtx1;
    rdp.vtx_buffer ^= 1;
    index = 0;

    for (i=0; i<rdp.n_global; i++)
    {
       VERTEX *v1 = (VERTEX*)&rdp.vtxbuf2[i];
       VERTEX *v2 = NULL;

       j = i+1;
       if (j == rdp.n_global)
          j = 0;

       v2 = (VERTEX*)&rdp.vtxbuf2[j];

       // ** Right plane **
       if (v1->u[0] <= 256.0f)
       {
          if (v2->u[0] <= 256.0f)   // Both are in, save the last one
          {
             rdp.vtxbuf[index++] = *v2;
          }
          else      // First is in, second is out, save intersection
          {
             percent = (right_256 - v1->u[0]) / (v2->u[0] - v1->u[0]);
             rdp.vtxbuf[index].x = v1->x + (v2->x - v1->x) * percent;
             rdp.vtxbuf[index].y = v1->y + (v2->y - v1->y) * percent;
             rdp.vtxbuf[index].z = 1;
             rdp.vtxbuf[index].q = 1;
             rdp.vtxbuf[index].u[0] = 255.5f;
             rdp.vtxbuf[index].v[0] = v1->v[0] + (v2->v[0] - v1->v[0]) * percent;
             rdp.vtxbuf[index].b = (uint8_t)(v1->b + (v2->b - v1->b) * percent);
             rdp.vtxbuf[index].g = (uint8_t)(v1->g + (v2->g - v1->g) * percent);
             rdp.vtxbuf[index].r = (uint8_t)(v1->r + (v2->r - v1->r) * percent);
             rdp.vtxbuf[index++].a = (uint8_t)(v1->a + (v2->a - v1->a) * percent);
          }
       }
       else
       {
          if (v2->u[0] <= 256.0f) // First is out, second is in, save intersection & in point
          {
             percent = (right_256 - v2->u[0]) / (v1->u[0] - v2->u[0]);
             rdp.vtxbuf[index].x = v2->x + (v1->x - v2->x) * percent;
             rdp.vtxbuf[index].y = v2->y + (v1->y - v2->y) * percent;
             rdp.vtxbuf[index].z = 1;
             rdp.vtxbuf[index].q = 1;
             rdp.vtxbuf[index].u[0] = 255.5f;
             rdp.vtxbuf[index].v[0] = v2->v[0] + (v1->v[0] - v2->v[0]) * percent;
             rdp.vtxbuf[index].b = (uint8_t)(v2->b + (v1->b - v2->b) * percent);
             rdp.vtxbuf[index].g = (uint8_t)(v2->g + (v1->g - v2->g) * percent);
             rdp.vtxbuf[index].r = (uint8_t)(v2->r + (v1->r - v2->r) * percent);
             rdp.vtxbuf[index++].a = (uint8_t)(v2->a + (v1->a - v2->a) * percent);

             // Save the in point
             rdp.vtxbuf[index++] = *v2;
          }
       }
    }
    rdp.n_global = index;

    do_triangle_stuff_2 (0, 1, 1);
  }
}

static void uc6_draw_polygons (VERTEX v[4])
{
   apply_shading(v);

   {
      rdp.vtxbuf = rdp.vtx1; // copy from v to rdp.vtx1
      rdp.vtxbuf2 = rdp.vtx2;
      rdp.vtx_buffer = 0;
      rdp.n_global = 3;
      memcpy (rdp.vtxbuf, v, sizeof(VERTEX)*3);
      do_triangle_stuff_2 (0, 1, 1);

      rdp.vtxbuf = rdp.vtx1; // copy from v to rdp.vtx1
      rdp.vtxbuf2 = rdp.vtx2;
      rdp.vtx_buffer = 0;
      rdp.n_global = 3;
      memcpy (rdp.vtxbuf, v+1, sizeof(VERTEX)*3);
      do_triangle_stuff_2 (0, 1, 1);
   }
   g_gdp.flags |= UPDATE_ZBUF_ENABLED | UPDATE_VIEWPORT;

   if (settings.fog && (rdp.flags & FOG_ENABLED))
      grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT, g_gdp.fog_color.total);
}

static void uc6_read_object_data (DRAWOBJECT *d)
{
   uint32_t addr  = RSP_SegmentToPhysical(__RSP.w1) >> 1;

   d->objX        = ((int16_t*)gfx_info.RDRAM)[(addr+0)^1] / 4.0f;               // 0
   d->scaleW      = ((uint16_t *)gfx_info.RDRAM)[(addr+1)^1] / 1024.0f;        // 1
   d->imageW      = ((int16_t*)gfx_info.RDRAM)[(addr+2)^1] >> 5;                 // 2, 3 is padding
   d->objY        = ((int16_t*)gfx_info.RDRAM)[(addr+4)^1] / 4.0f;               // 4
   d->scaleH      = ((uint16_t *)gfx_info.RDRAM)[(addr+5)^1] / 1024.0f;        // 5
   d->imageH      = ((int16_t*)gfx_info.RDRAM)[(addr+6)^1] >> 5;                 // 6, 7 is padding

   d->imageStride = ((uint16_t *)gfx_info.RDRAM)[(addr+8)^1];                  // 8
   d->imageAdrs   = ((uint16_t *)gfx_info.RDRAM)[(addr+9)^1];                  // 9
   d->imageFmt    = ((uint8_t *)gfx_info.RDRAM)[(((addr+10)<<1)+0)^3]; // 10
   d->imageSiz    = ((uint8_t *)gfx_info.RDRAM)[(((addr+10)<<1)+1)^3]; // |
   d->imagePal    = ((uint8_t *)gfx_info.RDRAM)[(((addr+10)<<1)+2)^3]; // 11
   d->imageFlags  = ((uint8_t *)gfx_info.RDRAM)[(((addr+10)<<1)+3)^3]; // |

   if (d->imageW < 0)
      d->imageW = (int16_t)g_gdp.__clip.xl - (int16_t)d->objX - d->imageW;
   if (d->imageH < 0)
      d->imageH = (int16_t)g_gdp.__clip.yl - (int16_t)d->objY - d->imageH;
}

static void uc6_init_tile(const DRAWOBJECT *d)
{
   glide64gDPSetTile(
         d->imageFmt,       /* fmt - RGBA */
         d->imageSiz,       /* siz - 16bit */
         d->imageStride,    /* line */
         d->imageAdrs,      /* tmem */
         0,                 /* tile */
         d->imagePal,       /* palette */
         0,                 /* cmt */
         0,                 /* maskt */
         0,                 /* shift_t */
         0,                 /* cms */
         0,                 /* mask_s */
         0                  /* shift_s */
         );

   glide64gDPSetTileSize(
         0,                                  /* tile */
         0,                                  /* uls  */
         0,                                  /* ult  */
         (d->imageW>0) ? (d->imageW-1) : 0,  /* lrs */
         (d->imageH>0) ? (d->imageH-1) : 0   /* lrt */
         );
}

static void uc6_obj_rectangle(uint32_t w0, uint32_t w1)
{
   int i;
   float Z, ul_x, lr_x, ul_y, lr_y, ul_u, ul_v, lr_u, lr_v;
   VERTEX v[4];
   DRAWOBJECT d;

   uc6_read_object_data(&d);

   if (d.imageAdrs > 4096)
   {
      FRDP("tmem: %08lx is out of bounds! return\n", d.imageAdrs);
      return;
   }
   if (!rdp.s2dex_tex_loaded)
   {
      LRDP("Texture was not loaded! return\n");
      return;
   }

   uc6_init_tile(&d);

   Z = set_sprite_combine_mode();

   ul_x = d.objX;
   lr_x = d.objX + d.imageW / d.scaleW;
   ul_y = d.objY;
   lr_y = d.objY + d.imageH / d.scaleH;
   lr_u = 255.0f*rdp.cur_cache[0]->scale_x;
   lr_v = 255.0f*rdp.cur_cache[0]->scale_y;
   ul_u = 0.5f;
   ul_v = 0.5f;

   if (d.imageFlags & G_BG_FLAG_FLIPS) /* flipS */
   {
      ul_u = lr_u;
      lr_u = 0.5f;
   }

   if (d.imageFlags & G_BG_FLAG_FLIPT) //flipT
   {
      ul_v = lr_v;
      lr_v = 0.5f;
   }

   /* Make the vertices */

   v[0].x = ul_x;
   v[0].y = ul_y;
   v[0].z = Z;
   v[0].q = 1.0f;
   v[0].u[0] = ul_u;
   v[0].v[0] = ul_v;

   v[1].x = lr_x;
   v[1].y = ul_y;
   v[1].z = Z;
   v[1].q = 1.0f;
   v[1].u[0] = lr_u;
   v[1].v[0] = ul_v;

   v[2].x = ul_x;
   v[2].y = lr_y;
   v[2].z = Z;
   v[2].q = 1.0f;
   v[2].u[0] = ul_u;
   v[2].v[0] = lr_v;

   v[3].x = lr_x;
   v[3].y = lr_y;
   v[3].z = Z;
   v[3].q = 1.0f;
   v[3].u[0] = lr_u;
   v[3].v[0] = lr_v;

   for (i = 0; i < 4; i++)
   {
      v[i].x = (v[i].x * rdp.scale_x) + rdp.offset_x;
      v[i].y = (v[i].y * rdp.scale_y) + rdp.offset_y;
   }

   uc6_draw_polygons (v);
}

void uc6_obj_sprite(uint32_t w0, uint32_t w1)
{
   DRAWOBJECT d;
   VERTEX v[4];
   int i;
   float Z, ul_x, lr_x, ul_y, lr_y, ul_u, lr_u, ul_v, lr_v;

   uc6_read_object_data(&d);
   uc6_init_tile(&d);

   Z = set_sprite_combine_mode ();

   ul_x = d.objX;
   lr_x = d.objX + d.imageW/d.scaleW;
   ul_y = d.objY;
   lr_y = d.objY + d.imageH/d.scaleH;

#if 0
   if (rdp.cur_cache[0]->splits > 1)
   {
      lr_u = (float)(d.imageW-1);
      lr_v = (float)(d.imageH-1);
   }
   else
#endif
   {
      lr_u = 255.0f*rdp.cur_cache[0]->scale_x;
      lr_v = 255.0f*rdp.cur_cache[0]->scale_y;
   }

   if (d.imageFlags&0x01) //flipS
   {
      ul_u = lr_u;
      lr_u = 0.5f;
   }
   else
      ul_u = 0.5f;
   if (d.imageFlags&0x10) //flipT
   {
      ul_v = lr_v;
      lr_v = 0.5f;
   }
   else
      ul_v = 0.5f;

   /* Make the vertices */
   // FRDP("scale_x: %f, scale_y: %f\n", rdp.cur_cache[0]->scale_x, rdp.cur_cache[0]->scale_y);

   v[0].x = ul_x;
   v[0].y = ul_y;
   v[0].z = Z;
   v[0].q = 1.0f;
   v[0].u[0] = ul_u;
   v[0].v[0] = ul_v;

   v[1].x = lr_x;
   v[1].y = ul_y;
   v[1].z = Z;
   v[1].q = 1.0f;
   v[1].u[0] = lr_u;
   v[1].v[0] = ul_v;

   v[2].x = ul_x;
   v[2].y = lr_y;
   v[2].z = Z;
   v[2].q = 1.0f;
   v[2].u[0] = ul_u;
   v[2].v[0] = lr_v;

   v[3].x = lr_x;
   v[3].y = lr_y;
   v[3].z = Z;
   v[3].q = 1.0f;
   v[3].u[0] = lr_u;
   v[3].v[0] = lr_v;
  
   for (i = 0; i < 4; i++)
   {
      float x = v[i].x;
      float y = v[i].y;
      v[i].x = ((x * gSP.objMatrix.A + y * gSP.objMatrix.B + gSP.objMatrix.X) * rdp.scale_x) + rdp.offset_x;
      v[i].y = ((x * gSP.objMatrix.C + y * gSP.objMatrix.D + gSP.objMatrix.Y) * rdp.scale_y) + rdp.offset_y;
   }

   uc6_draw_polygons (v);
}

static void uc6_obj_movemem(uint32_t w0, uint32_t w1)
{
   switch (_SHIFTR( w0, 0, 16 ))
   {
      case S2DEX_MV_MATRIX:
         glide64gSPObjMatrix( w1 );
         break;
		case S2DEX_MV_SUBMATRIX:
			glide64gSPObjSubMatrix( w1 );
			break;
   }
}

static void uc6_select_dl(uint32_t w0, uint32_t w1)
{
}

static void uc6_obj_rendermode(uint32_t w0, uint32_t w1)
{
}

static void DrawYUVImageToFrameBuffer(uint16_t ul_x, uint16_t ul_y, uint16_t lr_x, uint16_t lr_y)
{
   uint16_t h, w, *dst;
   uint32_t width, height, *mb;

   uint32_t ci_width  = gDP.colorImage.width;
   uint32_t ci_height = rdp.ci_lower_bound;

   if (ul_x >= ci_width)
      return;
   if (ul_y >= ci_height)
      return;

   width  = 16;
   height = 16;

   if (lr_x > ci_width)
      width  = ci_width - ul_x;
   if (lr_y > ci_height)
      height = ci_height - ul_y;

   mb   = (uint32_t*)(gfx_info.RDRAM + g_gdp.ti_address); //pointer to the first macro block
   dst  = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);
   dst += ul_x + ul_y * ci_width;

   /* YUV macro block contains 16x16 texture. 
    * we need to put it in the proper place inside cimg */
   for (h = 0; h < 16; h++)
   {
      for (w = 0; w < 16; w+=2)
      {
         uint32_t t = *(mb++); /* each uint32_t contains 2 pixels */
         if ((h < height) && (w < width)) /* clipping. texture image may be larger than color image */
         {
            uint8_t y0 = (uint8_t)t&0xFF;
            uint8_t v  = (uint8_t)(t>>8)&0xFF;
            uint8_t y1 = (uint8_t)(t>>16)&0xFF;
            uint8_t u  = (uint8_t)(t>>24)&0xFF;
            *(dst++)   = YUVtoRGBA16(y0, u, v);
            *(dst++)   = YUVtoRGBA16(y1, u, v);
         }
      }
      dst += ci_width - 16;
   }
}

static void uc6_obj_rectangle_r(uint32_t w0, uint32_t w1)
{
   int i;
   float Z, ul_x, lr_x, ul_y, lr_y, ul_u, ul_v, lr_u, lr_v;
   VERTEX v[4];
   DRAWOBJECT d;

   uc6_read_object_data(&d);

   if (d.imageFmt == G_IM_FMT_YUV && (settings.hacks&hack_Ogre64)) //Ogre Battle needs to copy YUV texture to frame buffer
   {
      ul_x = d.objX / gSP.objMatrix.baseScaleX + gSP.objMatrix.X;
      lr_x = (d.objX + d.imageW / d.scaleW) / gSP.objMatrix.baseScaleX + gSP.objMatrix.X;
      ul_y = d.objY / gSP.objMatrix.baseScaleY + gSP.objMatrix.Y;
      lr_y = (d.objY + d.imageH / d.scaleH) / gSP.objMatrix.baseScaleY + gSP.objMatrix.Y;
      DrawYUVImageToFrameBuffer((uint16_t)ul_x, (uint16_t)ul_y, (uint16_t)lr_x, (uint16_t)lr_y);
      return;
   }

   uc6_init_tile(&d);

   Z    = set_sprite_combine_mode();

   ul_x = d.objX / gSP.objMatrix.baseScaleX;
   lr_x = (d.objX + d.imageW / d.scaleW) / gSP.objMatrix.baseScaleX;
   ul_y = d.objY / gSP.objMatrix.baseScaleY;
   lr_y = (d.objY + d.imageH / d.scaleH) / gSP.objMatrix.baseScaleY;
   lr_u = 255.0f * rdp.cur_cache[0]->scale_x;
   lr_v = 255.0f * rdp.cur_cache[0]->scale_y;

   if (d.imageFlags & G_BG_FLAG_FLIPS) //flipS
   {
      ul_u = lr_u;
      lr_u = 0.5f;
   }
   else
      ul_u = 0.5f;
   if (d.imageFlags & G_BG_FLAG_FLIPT) //flipT
   {
      ul_v = lr_v;
      lr_v = 0.5f;
   }
   else
      ul_v = 0.5f;

   /* Make the vertices */

   v[0].x = ul_x;
   v[0].y = ul_y;
   v[0].z = Z;
   v[0].q = 1.0f;
   v[0].u[0] = ul_u;
   v[0].v[0] = ul_v;

   v[1].x = lr_x;
   v[1].y = ul_y;
   v[1].z = Z;
   v[1].q = 1.0f;
   v[1].u[0] = lr_u;
   v[1].v[0] = ul_v;

   v[2].x = ul_x;
   v[2].y = lr_y;
   v[2].z = Z;
   v[2].q = 1.0f;
   v[2].u[0] = ul_u;
   v[2].v[0] = lr_v;

   v[3].x = lr_x;
   v[3].y = lr_y;
   v[3].z = Z;
   v[3].q = 1.0f;
   v[3].u[0] = lr_u;
   v[3].v[0] = lr_v;

   for (i = 0; i < 4; i++)
   {
      v[i].x = ((v[i].x + gSP.objMatrix.X) * rdp.scale_x) + rdp.offset_x;
      v[i].y = ((v[i].y + gSP.objMatrix.Y) * rdp.scale_y) + rdp.offset_y;
   }

   uc6_draw_polygons (v);
}

static void uc6_obj_loadtxtr(uint32_t w0, uint32_t w1)
{
   rdp.s2dex_tex_loaded = true;
   g_gdp.flags |= UPDATE_TEXTURE;

   glide64gSPObjLoadTxtr(w1);
}

static void uc6_obj_ldtx_sprite(uint32_t w0, uint32_t w1)
{
   uint32_t addr = w1;
   uc6_obj_loadtxtr(__RSP.w0, __RSP.w1);
   __RSP.w1 = addr + 24;
   uc6_obj_sprite(__RSP.w0, __RSP.w1);
}

static void uc6_obj_ldtx_rect(uint32_t w0, uint32_t w1)
{
   uint32_t addr = w1;
   uc6_obj_loadtxtr(__RSP.w0, __RSP.w1);
   __RSP.w1 = addr + 24;
   uc6_obj_rectangle(__RSP.w0, __RSP.w1);
}

static void uc6_ldtx_rect_r(uint32_t w0, uint32_t w1)
{
   uint32_t addr = w1;
   uc6_obj_loadtxtr(__RSP.w0, __RSP.w1);
   __RSP.w1 = addr + 24;
   uc6_obj_rectangle_r(__RSP.w0, __RSP.w1);
}

static void uc6_loaducode(uint32_t w0, uint32_t w1)
{
   /* copy the microcode data */
   uint32_t addr = RSP_SegmentToPhysical(w1);
   uint32_t size = (w0 & 0xFFFF) + 1;
   memcpy (microcode, gfx_info.RDRAM+addr, size);

   microcheck ();
}

static void uc6_sprite2d(uint32_t w0, uint32_t w1)
{
   uint16_t stride;
   uint32_t addr, tlut;
   int i;
   DRAWIMAGE d;
   uint32_t    a = __RSP.PC[__RSP.PCi] & BMASK;
   uint32_t cmd0 = ((uint32_t*)gfx_info.RDRAM)[a>>2]; //check next command

   if ( (cmd0>>24) != 0xBE )
      return;

   addr = RSP_SegmentToPhysical(w1) >> 1;

   d.imagePtr = RSP_SegmentToPhysical(((uint32_t*)gfx_info.RDRAM)[(addr+0)>>1]); // 0,1
   stride     = (((uint16_t *)gfx_info.RDRAM)[(addr+4)^1]); // 4
   d.imageW   = (((uint16_t *)gfx_info.RDRAM)[(addr+5)^1]); // 5
   d.imageH   = (((uint16_t *)gfx_info.RDRAM)[(addr+6)^1]); // 6
   d.imageFmt = ((uint8_t *)gfx_info.RDRAM)[(((addr+7)<<1)+0)^3]; // 7
   d.imageSiz = ((uint8_t *)gfx_info.RDRAM)[(((addr+7)<<1)+1)^3]; // |
   d.imagePal = 0;
   d.imageX   = (((uint16_t *)gfx_info.RDRAM)[(addr+8)^1]); // 8
   d.imageY   = (((uint16_t *)gfx_info.RDRAM)[(addr+9)^1]); // 9
   tlut       = ((uint32_t*)gfx_info.RDRAM)[(addr + 2) >> 1]; // 2, 3

   /*low-level implementation of sprite2d apparently calls setothermode command to set tlut mode		
    *However, description of sprite2d microcode just says that		
    *TlutPointer should be Null when CI images will not be used.		
    *HLE implementation sets rdp.tlut_mode=2 if TlutPointer is not null, and rdp.tlut_mode=0 otherwise		
    *Alas, it is not sufficient, since WCW Nitro uses non-Null TlutPointer for rgba textures.		
    *So, additional check added.		
    */
   rdp.tlut_mode = 0;		
   if (tlut)		
   {		
      load_palette (RSP_SegmentToPhysical(tlut), 0, 256);		
      if (d.imageFmt > G_IM_FMT_RGBA)		
         rdp.tlut_mode = 2;		
      else		
         rdp.tlut_mode = 0;		
   }

   if (d.imageW == 0)
      return;// d.imageW = stride;

   cmd0 = ((uint32_t*)gfx_info.RDRAM)[a>>2]; //check next command
   while (1)
   {
      uint32_t texsize, maxTexSize;
      if ( (cmd0>>24) == 0xBE )
      {
         uint32_t cmd1    = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+1];
         __RSP.PC[__RSP.PCi] = (a+8) & BMASK;

         d.scaleX = ((cmd1 >> 16)&0xFFFF)/1024.0f;
         d.scaleY = (cmd1  &  0xFFFF)/1024.0f;

         //the code below causes wrong background height in super robot spirit, so it is disabled.
         //need to find, for which game this hack was made
         //if( (cmd1&0xFFFF) < 0x100 )
         // d.scaleY = d.scaleX;
         d.flipX          = (uint8_t)((cmd0>>8)&0xFF);
         d.flipY          = (uint8_t)(cmd0&0xFF);

         a                   = __RSP.PC[__RSP.PCi] & BMASK;
         __RSP.PC[__RSP.PCi] = (a+8) & BMASK;
         cmd0                = ((uint32_t*)gfx_info.RDRAM)[a>>2]; //check next command
      }
      if ( (cmd0>>24) == 0xBD )
      {
         uint32_t cmd1 = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+1];

         d.frameX = ((int16_t)((cmd1>>16)&0xFFFF)) / 4.0f;
         d.frameY = ((int16_t)(cmd1&0xFFFF)) / 4.0f;
         d.frameW = (uint16_t) (d.imageW / d.scaleX);
         d.frameH = (uint16_t) (d.imageH / d.scaleY);
         if (settings.hacks&hack_WCWnitro)
         {
            int scaleY = (int)d.scaleY;
            d.imageH /= scaleY;
            d.imageY /= scaleY;
            stride *= scaleY;
            d.scaleY = 1.0f;
         }
#if 0
         FRDP ("imagePtr: %08lx\n", d.imagePtr);
         FRDP ("frameX: %f, frameW: %d, frameY: %f, frameH: %d\n", d.frameX, d.frameW, d.frameY, d.frameH);
         FRDP ("imageX: %d, imageW: %d, imageY: %d, imageH: %d\n", d.imageX, d.imageW, d.imageY, d.imageH);
         FRDP ("imageFmt: %d, imageSiz: %d, imagePal: %d, imageStride: %d\n", d.imageFmt, d.imageSiz, d.imagePal, stride);
         FRDP ("scaleX: %f, scaleY: %f\n", d.scaleX, d.scaleY);
#endif
      }
      else
         return;
 
      texsize = (d.imageW * d.imageH) << d.imageSiz >> 1;
      maxTexSize = rdp.tlut_mode < G_TT_RGBA16 ? 4096 : 2048;

      if (texsize > maxTexSize)
      {
         if (d.scaleX != 1)
            d.scaleX *= (float)stride/(float)d.imageW;
         d.imageW = stride;
         d.imageH += d.imageY;
         DrawImage(&d);
      }
      else
      {
         float Z, ul_x, ul_y, lr_x, lr_y, lr_u, lr_v;
         struct gDPTile *tile;
         VERTEX v[4];
         uint16_t line = d.imageW;

         if (line & 7) line += 8; // round up
         line >>= 3;
         if (d.imageSiz == 0)
         {
            if (line%2)
               line++;
            line >>= 1;
         }
         else
         {
            line <<= (d.imageSiz-1);
         }
         if (line == 0)
            line = 1;

         glide64gDPSetTextureImage(
               g_gdp.ti_format,        /* format */
               g_gdp.ti_size,          /* siz */
               stride,                 /* width */
               d.imagePtr              /* address */
               );

         g_gdp.tile[7].tmem = 0;
         g_gdp.tile[7].line = line;//(d.imageW>>3);
         g_gdp.tile[7].size = d.imageSiz;
         __RSP.w0           = (d.imageX << 14) | (d.imageY << 2);
         __RSP.w1           = 0x07000000 | ((d.imageX+d.imageW-1) << 14) | ((d.imageY+d.imageH-1) << 2);
         rdp_loadtile(__RSP.w0, __RSP.w1);

         glide64gDPSetTile(
               d.imageFmt,
               d.imageSiz,
               line, /* (d.imageW>>3)  */
               0,
               0,
               0,
               0,
               0,
               0,
               0,
               0,
               0);

         glide64gDPSetTileSize(
               0,                      /* tile */
               d.imageX,               /* uls  */
               d.imageY,               /* ult  */
               d.imageX+d.imageW-1,    /* lrs  */
               d.imageY+d.imageH-1     /* lrt  */
               );

         Z = set_sprite_combine_mode ();

         if (d.flipX)
         {
            ul_x = d.frameX + d.frameW;
            lr_x = d.frameX;
         }
         else
         {
            ul_x = d.frameX;
            lr_x = d.frameX + d.frameW;
         }
         if (d.flipY)
         {
            ul_y = d.frameY + d.frameH;
            lr_y = d.frameY;
         }
         else
         {
            ul_y = d.frameY;
            lr_y = d.frameY + d.frameH;
         }

#if 0
         if (rdp.cur_cache[0]->splits > 1)
         {
            lr_u = (float)(d.imageW-1);
            lr_v = (float)(d.imageH-1);
         }
         else
#endif
         {
            lr_u = 255.0f*rdp.cur_cache[0]->scale_x;
            lr_v = 255.0f*rdp.cur_cache[0]->scale_y;
         }

         /* Make the vertices */

         v[0].x = ul_x;
         v[0].y = ul_y;
         v[0].z = Z;
         v[0].q = 1.0f;
         v[0].u[0] = 0.5f;
         v[0].v[0] = 0.5f;

         v[1].x = lr_x;
         v[1].y = ul_y;
         v[1].z = Z;
         v[1].q = 1.0f;
         v[1].u[0] = lr_u;
         v[1].v[0] = 0.5f;

         v[2].x = ul_x;
         v[2].y = lr_y;
         v[2].z = Z;
         v[2].q = 1.0f;
         v[2].u[0] = 0.5f;
         v[2].v[0] = lr_v;

         v[3].x = lr_x;
         v[3].y = lr_y;
         v[3].z = Z;
         v[3].q = 1.0f;
         v[3].u[0] = lr_u;
         v[3].v[0] = lr_v;

         for (i=0; i<4; i++)
         {
            v[i].x = (v[i].x * rdp.scale_x) + rdp.offset_x;
            v[i].y = (v[i].y * rdp.scale_y) + rdp.offset_y;
         }

         apply_shading(v);

#if 0
         // Set vertex buffers
         if (rdp.cur_cache[0]->splits > 1)
         {
            VERTEX *vptr[3];
            for (i = 0; i < 3; i++)
               vptr[i] = &v[i];
            draw_split_triangle(vptr);

            for (i = 0; i < 3; i++)
               vptr[i] = &v[i+1];
            draw_split_triangle(vptr);
         }
         else
#endif
         {
            rdp.vtxbuf = rdp.vtx1; // copy from v to rdp.vtx1
            rdp.vtxbuf2 = rdp.vtx2;
            rdp.vtx_buffer = 0;
            rdp.n_global = 3;
            memcpy (rdp.vtxbuf, v, sizeof(VERTEX)*3);
            do_triangle_stuff_2 (0, 1, 1);

            rdp.vtxbuf = rdp.vtx1; // copy from v to rdp.vtx1
            rdp.vtxbuf2 = rdp.vtx2;
            rdp.vtx_buffer = 0;
            rdp.n_global = 3;
            memcpy (rdp.vtxbuf, v+1, sizeof(VERTEX)*3);
            do_triangle_stuff_2 (0, 1, 1);
         }
         g_gdp.flags |= UPDATE_ZBUF_ENABLED | UPDATE_VIEWPORT;

         if (settings.fog && (rdp.flags & FOG_ENABLED))
            grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT, g_gdp.fog_color.total);

      }
      a    = __RSP.PC[__RSP.PCi] & BMASK;
      cmd0 = ((uint32_t*)gfx_info.RDRAM)[a>>2]; //check next command
      if (( (cmd0>>24) == 0xBD ) || ( (cmd0>>24) == 0xBE ))
         __RSP.PC[__RSP.PCi] = (a+8) & BMASK;
      else
         return;
   }
}
