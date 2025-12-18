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
//
// Draw N64 frame buffer to screen.
// Created by Gonetz, 2007
//
//****************************************************************


#include <math.h>
#include "Gfx_1.3.h"

#include "../../../Graphics/GBI.h"
#include "../../../Graphics/image_convert.h"
#include "../../../Graphics/RDP/gDP_state.h"

#include "Framebuffer_glide64.h"
#include "TexCache.h"
#include "../Glitch64/glide.h"
#include "GlideExtensions.h"
#include "rdp.h"

#define ZLUT_SIZE 0x40000

/* (x * y) >> 16 */
#define IMUL16(x, y) ((((int64_t)x) * ((int64_t)y)) >> 16)
/* (x * y) >> 14 */
#define IMUL14(x, y) ((((int64_t)x) * ((int64_t)y)) >> 14)

static uint16_t *zLUT;
uint16_t *glide64_frameBuffer;

extern int dzdx;

static struct vertexi *max_vtx;                   // Max y vertex (ending vertex)
static struct vertexi *start_vtx, *end_vtx;      // First and last vertex in array
static struct vertexi *right_vtx, *left_vtx;     // Current right and left vertex

static int right_height, left_height;
static int right_x, right_dxdy, left_x, left_dxdy;
static int left_z, left_dzdy;

void ZLUT_init(void)
{
   int i;
   if (zLUT)
      return;

   zLUT = (uint16_t*)malloc(ZLUT_SIZE * sizeof(uint16_t));

   for(i = 0; i< ZLUT_SIZE; i++)
   {
      uint32_t mantissa;
      uint32_t exponent = 0;
      uint32_t testbit  = 1 << 17;

      while((i & testbit) && (exponent < 7))
      {
         exponent++;
         testbit = 1 << (17 - exponent);
      }

      mantissa = (i >> (6 - (6 < exponent ? 6 : exponent))) & 0x7ff;
      zLUT[i] = (uint16_t)(((exponent << 11) | mantissa) << 2);
   }
}

void ZLUT_release(void)
{
   if (zLUT)
      free(zLUT);
   zLUT = NULL;
}


static INLINE int idiv16(int x, int y)
{
   const int64_t m = (int64_t)(x);
   const int64_t n = (int64_t)(y);
   int64_t result = (m << 16) / n;

   return (int)(result);
}

static INLINE int iceil(int x)
{
   return ((x + 0xffff)  >> 16);
}

static void RightSection(void)
{
   int prestep;
   // Walk backwards trough the vertex array
   struct vertexi *v1 = (struct vertexi*)right_vtx;
   struct vertexi *v2 = end_vtx;         // Wrap to end of array

   if(right_vtx > start_vtx)
      v2 = right_vtx-1;     

   right_vtx = v2;

   // v1 = top vertex
   // v2 = bottom vertex 

   // Calculate number of scanlines in this section

   right_height = iceil(v2->y) - iceil(v1->y);
   if(right_height <= 0)
      return;

   // Guard against possible div overflows

   if(right_height > 1)
   {
      // OK, no worries, we have a section that is at least
      // one pixel high. Calculate slope as usual.

      int height = v2->y - v1->y;
      right_dxdy  = idiv16(v2->x - v1->x, height);
   }
   else
   {
      // Height is less or equal to one pixel.
      // Calculate slope = width * 1/height
      // using 18:14 bit precision to avoid overflows.

      int inv_height = (0x10000 << 14) / (v2->y - v1->y);  
      right_dxdy     = IMUL14(v2->x - v1->x, inv_height);
   }

   // Prestep initial values

   prestep = (iceil(v1->y) << 16) - v1->y;
   right_x = v1->x + IMUL16(prestep, right_dxdy);
}

static void LeftSection(void)
{
   int prestep;
   // Walk forward through the vertex array
   struct vertexi *v1 = (struct vertexi*)left_vtx;
   struct vertexi *v2 = start_vtx;      // Wrap to start of array

   if(left_vtx < end_vtx)
      v2 = left_vtx+1;
   left_vtx = v2;

   // v1 = top vertex
   // v2 = bottom vertex 

   // Calculate number of scanlines in this section

   left_height = iceil(v2->y) - iceil(v1->y);

   if(left_height <= 0)
      return;

   // Guard against possible div overflows

   if(left_height > 1)
   {
      // OK, no worries, we have a section that is at least
      // one pixel high. Calculate slope as usual.

      int height = v2->y - v1->y;
      left_dxdy = idiv16(v2->x - v1->x, height);
      left_dzdy = idiv16(v2->z - v1->z, height);
   }
   else
   {
      // Height is less or equal to one pixel.
      // Calculate slope = width * 1/height
      // using 18:14 bit precision to avoid overflows.

      int inv_height = (0x10000 << 14) / (v2->y - v1->y);
      left_dxdy      = IMUL14(v2->x - v1->x, inv_height);
      left_dzdy      = IMUL14(v2->z - v1->z, inv_height);
   }

   // Prestep initial values

   prestep = (iceil(v1->y) << 16) - v1->y;
   left_x = v1->x + IMUL16(prestep, left_dxdy);
   left_z = v1->z + IMUL16(prestep, left_dzdy);
}

static void DepthBufferRasterize(struct vertexi * vtx, int vertices, int dzdx)
{
   int n, min_y, max_y, y1;
   struct vertexi *min_vtx;
   start_vtx = vtx;        // First vertex in array

   // Search trough the vtx array to find min y, max y
   // and the location of these structures.

   min_vtx = (struct vertexi*)vtx;
   max_vtx = vtx;

   min_y = vtx->y;
   max_y = vtx->y;

   vtx++;

   for (n = 1; n < vertices; n++)
   {
      if(vtx->y < min_y)
      {
         min_y = vtx->y;
         min_vtx = vtx;
      }
      else if(vtx->y > max_y)
      {
         max_y = vtx->y;
         max_vtx = vtx;
      }
      vtx++;
   }

   // OK, now we know where in the array we should start and
   // where to end while scanning the edges of the polygon

   left_vtx  = min_vtx;    // Left side starting vertex
   right_vtx = min_vtx;    // Right side starting vertex
   end_vtx   = vtx-1;      // Last vertex in array

   // Search for the first usable right section

   do {
      if(right_vtx == max_vtx)
         return;
      RightSection();
   } while(right_height <= 0);

   // Search for the first usable left section

   do {
      if(left_vtx == max_vtx)
         return;
      LeftSection();
   } while(left_height <= 0);

   y1      = iceil(min_y);

   if (y1 >= g_gdp.__clip.yl)
      return;

   for(;;)
   {
      int width;
      int x1 = iceil(left_x);

      if (x1 < g_gdp.__clip.xh)
         x1 = g_gdp.__clip.xh;
      width = iceil(right_x) - x1;
      if (x1+width >= g_gdp.__clip.xl)
         width = g_gdp.__clip.xl - x1 - 1;

      if(width > 0 && y1 >= g_gdp.__clip.yh)
      {
         /* Prestep initial z */

         unsigned x;
         int       prestep = (x1 << 16) - left_x;
         int             z = left_z + IMUL16(prestep, dzdx);
         uint16_t *ptr_dst = (uint16_t*)(gfx_info.RDRAM + g_gdp.zb_address);
         int         shift = x1 + y1 * rdp.zi_width;

         /* draw to depth buffer */
         for (x = 0; x < width; x++)
         {
            int idx;
            uint16_t encodedZ;
            int trueZ = z / 8192;
            if (trueZ < 0)
               trueZ = 0;
            else if (trueZ > 0x3FFFF)
               trueZ = 0x3FFFF;
            encodedZ = zLUT[trueZ];
            idx = (shift+x)^1;
            if(encodedZ < ptr_dst[idx]) 
               ptr_dst[idx] = encodedZ;
            z += dzdx;
         }
      }

      y1++;
      if (y1 >= g_gdp.__clip.yl)
         return;

      // Scan the right side

      if(--right_height <= 0) // End of this section?
      {
         do
         {
            if(right_vtx == max_vtx)
               return;
            RightSection();
         } while(right_height <= 0);
      }
      else 
         right_x += right_dxdy;

      // Scan the left side

      if(--left_height <= 0) // End of this section ?
      {
         do
         {
            if(left_vtx == max_vtx)
               return;
            LeftSection();
         } while(left_height <= 0);
      }
      else
      {
         left_x += left_dxdy;
         left_z += left_dzdy;
      }
   }
}

static void glide64_draw_fb(float ul_x, float ul_y, float lr_x,
      float lr_y, float lr_u, float lr_v, float zero)
{
   VERTEX v[4], vout[4];
   /* Make the vertices */

   v[0].x  = ul_x;
   v[0].y  = ul_y;
   v[0].z  = 1.0f;
   v[0].q  = 1.0f;
   v[0].u[0] = zero;
   v[0].v[0] = zero;
   v[0].u[1] = zero;
   v[0].v[1] = zero;
   v[0].coord[0] = zero;
   v[0].coord[1] = zero;
   v[0].coord[2] = zero;
   v[0].coord[3] = zero;

   v[1].x  = lr_x;
   v[1].y  = ul_y;
   v[1].z  = 1.0f;
   v[1].q  = 1.0f;
   v[1].u[0] = lr_u;
   v[1].v[0] = zero;
   v[1].u[1] = lr_u;
   v[1].v[1] = zero;
   v[1].coord[0] = lr_u;
   v[1].coord[1] = zero;
   v[1].coord[2] = lr_u;
   v[1].coord[3] = zero;

   v[2].x  = ul_x;
   v[2].y  = lr_y;
   v[2].z  = 1.0f;
   v[2].q  = 1.0f;
   v[2].u[0] = zero; 
   v[2].v[0] = lr_v;
   v[2].u[1] = zero;
   v[2].v[1] = lr_v;
   v[2].coord[0] = zero;
   v[2].coord[1] = lr_v;
   v[2].coord[2] = zero;
   v[2].coord[3] = lr_v;

   v[3].x  = lr_x;
   v[3].y  = lr_y;
   v[3].z  = 1.0f;
   v[3].q  = 1.0f;
   v[3].u[0] = lr_u; 
   v[3].v[0] = lr_v;
   v[3].u[1] = lr_u;
   v[3].v[1] = lr_v;
   v[3].coord[0] = lr_u;
   v[3].coord[1] = lr_v;
   v[3].coord[2] = lr_u;
   v[3].coord[3] = lr_v;

   vout[0] = v[0];
   vout[1] = v[2];
   vout[2] = v[1];
   vout[3] = v[3];

   grDrawVertexArrayContiguous(GR_TRIANGLE_STRIP, 4, &vout[0]);
}

static int SetupFBtoScreenCombiner(uint32_t texture_size, uint32_t opaque)
{
   int tmu, filter;

   if (voodoo.tmem_ptr[GR_TMU0]+texture_size < voodoo.tex_max_addr)
   {
      tmu = GR_TMU0;
      grTexCombine( GR_TMU1,
            GR_COMBINE_FUNCTION_NONE,
            GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_NONE,
            GR_COMBINE_FACTOR_NONE,
            FXFALSE,
            FXFALSE );
      grTexCombine( GR_TMU0,
            GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            FXFALSE,
            FXFALSE );
   }
   else
   {
      if (voodoo.tmem_ptr[GR_TMU1]+texture_size >= voodoo.tex_max_addr)
         ClearCache ();
      tmu = GR_TMU1;
      grTexCombine( GR_TMU1,
            GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            GR_COMBINE_FUNCTION_LOCAL,
            GR_COMBINE_FACTOR_NONE,
            FXFALSE,
            FXFALSE );
      grTexCombine( GR_TMU0,
            GR_COMBINE_FUNCTION_SCALE_OTHER,
            GR_COMBINE_FACTOR_ONE,
            GR_COMBINE_FUNCTION_SCALE_OTHER,
            GR_COMBINE_FACTOR_ONE,
            FXFALSE,
            FXFALSE );
   }
   filter = (gDP.otherMode.textureFilter == 2) ? GR_TEXTUREFILTER_BILINEAR : GR_TEXTUREFILTER_POINT_SAMPLED;

   grTexFilterClampMode (tmu,
         GR_TEXTURECLAMP_CLAMP,
         GR_TEXTURECLAMP_CLAMP,
         filter,
         filter);
   grColorCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_LOCAL_NONE,
         GR_COMBINE_OTHER_TEXTURE,
         //    GR_COMBINE_OTHER_CONSTANT,
         FXFALSE);
   grAlphaCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_LOCAL_NONE,
         GR_COMBINE_OTHER_TEXTURE,
         FXFALSE);
   if (opaque)
   {
      grAlphaTestFunction (GR_CMP_ALWAYS, 0x00, 0);
      grAlphaBlendFunction( GR_BLEND_ONE,
            GR_BLEND_ZERO,
            GR_BLEND_ONE,
            GR_BLEND_ZERO);
   }
   else
   {
      grAlphaBlendFunction( GR_BLEND_SRC_ALPHA,
            GR_BLEND_ONE_MINUS_SRC_ALPHA,
            GR_BLEND_ONE,
            GR_BLEND_ZERO);
   }
   grDepthBufferFunction (GR_CMP_ALWAYS);
   grCullMode(GR_CULL_DISABLE);
   grDepthMask (FXFALSE);
   g_gdp.flags |= UPDATE_COMBINE | UPDATE_ZBUF_ENABLED | UPDATE_CULL_MODE;
   return tmu;
}

static void DrawDepthBufferToScreen256(FB_TO_SCREEN_INFO *fb_info)
{
   uint32_t h, w, x, y, tex_size;
   uint32_t w_tail, h_tail, tex_adr;
   int tmu;
   GrTexInfo t_info;
   uint32_t width         = fb_info->lr_x - fb_info->ul_x + 1;
   uint32_t height        = fb_info->lr_y - fb_info->ul_y + 1;
   uint8_t *image         = (uint8_t*)(gfx_info.RDRAM + fb_info->addr);
   uint32_t width256      = ((width-1) >> 8) + 1;
   uint32_t height256     = ((height-1) >> 8) + 1;
   uint16_t *tex          = (uint16_t*)texture_buffer;
   uint16_t *src          = (uint16_t*)(image + fb_info->ul_x + fb_info->ul_y * fb_info->width);

   t_info.smallLodLog2    = t_info.largeLodLog2 = GR_LOD_LOG2_256;
   t_info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
   t_info.format          = GR_TEXFMT_ALPHA_INTENSITY_88;
   t_info.data            = tex;

   tex_size               = grTexCalcMemRequired(t_info.largeLodLog2, t_info.aspectRatioLog2, t_info.format);
   tmu                    = SetupFBtoScreenCombiner(tex_size*width256*height256, fb_info->opaque);

   grConstantColorValue (g_gdp.fog_color.total);
   grColorCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_LOCAL_NONE,
         GR_COMBINE_OTHER_CONSTANT,
         FXFALSE);
   w_tail                 = width % 256;
   h_tail                 = height % 256;
   tex_adr                = voodoo.tmem_ptr[tmu];

   for (h = 0; h < height256; h++)
   {
      for (w = 0; w < width256; w++)
      {
         float ul_x, ul_y, lr_x, lr_y, lr_u, lr_v;

         uint32_t cur_width  = (256 * (w + 1) < width) ? 256 : w_tail;
         uint32_t cur_height = (256 * (h + 1) < height) ? 256 : h_tail;
         uint32_t cur_tail   = 256 - cur_width;
         uint16_t *dst       = tex;

         for (y=0; y < cur_height; y++)
         {
            for (x=0; x < cur_width; x++)
               *(dst++) = rdp.pal_8[src[(x + 256 * w + (y + 256 * h) * fb_info->width) ^ 1]>>8];
            dst += cur_tail;
         }
         grTexSource (tmu, tex_adr, GR_MIPMAPLEVELMASK_BOTH, &t_info, true);
         tex_adr += tex_size;
         ul_x     = (float)(fb_info->ul_x + 256 * w);
         ul_y     = (float)(fb_info->ul_y + 256 * h);
         lr_x     = (ul_x + (float)(cur_width)) * rdp.scale_x + rdp.offset_x;
         lr_y     = (ul_y + (float)(cur_height)) * rdp.scale_y + rdp.offset_y;
         ul_x     = ul_x * rdp.scale_x + rdp.offset_x;
         ul_y     = ul_y * rdp.scale_y + rdp.offset_y;
         lr_u     = (float)(cur_width-1);
         lr_v     = (float)(cur_height-1);

         glide64_draw_fb(ul_x, ul_y, lr_x,
               lr_y, lr_u, lr_v, 0.5f);
      }
   }
}

static void DrawDepthBufferToScreen(FB_TO_SCREEN_INFO *fb_info)
{
   uint32_t x, y;
   int tmu;
   float ul_x, ul_y, lr_x, lr_y, lr_u, lr_v, zero;
   GrTexInfo t_info;
   uint32_t width    = fb_info->lr_x - fb_info->ul_x + 1;
   uint32_t height   = fb_info->lr_y - fb_info->ul_y + 1;
   uint8_t *image    = (uint8_t*)(gfx_info.RDRAM + fb_info->addr);
   uint32_t texwidth = 512;
   float scale       = 0.5f;
   uint16_t *tex     = (uint16_t*)texture_buffer;
   uint16_t *dst     = (uint16_t*)tex;
   uint16_t *src     = (uint16_t*)(image + fb_info->ul_x + fb_info->ul_y * fb_info->width);

   if (width > 512)
   {
      DrawDepthBufferToScreen256(fb_info);
      return;
   }

   t_info.smallLodLog2 = t_info.largeLodLog2 = GR_LOD_LOG2_512;
   t_info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;

   if (width <= 256)
   {
      texwidth = 256;
      scale = 1.0f;
      t_info.smallLodLog2 = t_info.largeLodLog2 = GR_LOD_LOG2_256;
   }

   if (height <= (texwidth>>1))
      t_info.aspectRatioLog2 = GR_ASPECT_LOG2_2x1;


   for (y=0; y < height; y++)
   {
      for (x = 0; x < width; x++)
         *(dst++) = rdp.pal_8[src[(x+y*fb_info->width)^1]>>8];
      dst += texwidth-width;
   }
   t_info.format = GR_TEXFMT_ALPHA_INTENSITY_88;
   t_info.data = tex;

   tmu = SetupFBtoScreenCombiner(grTexCalcMemRequired(t_info.largeLodLog2, t_info.aspectRatioLog2, t_info.format), fb_info->opaque);
   grConstantColorValue (g_gdp.fog_color.total);
   grColorCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_LOCAL_NONE,
         GR_COMBINE_OTHER_CONSTANT,
         FXFALSE);
   grTexSource (tmu,
         voodoo.tmem_ptr[tmu],
         GR_MIPMAPLEVELMASK_BOTH,
         &t_info, true);
   ul_x = fb_info->ul_x * rdp.scale_x + rdp.offset_x;
   ul_y = fb_info->ul_y * rdp.scale_y + rdp.offset_y;
   lr_x = fb_info->lr_x * rdp.scale_x + rdp.offset_x;
   lr_y = fb_info->lr_y * rdp.scale_y + rdp.offset_y;
   lr_u = (width  - 1)  * scale;
   lr_v = (height - 1)  * scale;
   zero = scale * 0.5f;

   glide64_draw_fb(ul_x, ul_y, lr_x,
         lr_y, lr_u, lr_v, zero);
}



static void DrawRE2Video(FB_TO_SCREEN_INFO *fb_info, float scale)
{
   float scale_y = (float)fb_info->width / rdp.vi_height;
   float height  = settings.scr_res_x / scale_y;
   float ul_x    = 0.5f;
   float ul_y    = (settings.scr_res_y - height) / 2.0f;
   float lr_y    = settings.scr_res_y - ul_y - 1.0f;
   float lr_x    = settings.scr_res_x - 1.0f;
   float lr_u    = (fb_info->width - 1) * scale;
   float lr_v    = (fb_info->height - 1) * scale;

   glide64_draw_fb(ul_x, ul_y, lr_x,
         lr_y, lr_u, lr_v, 0.5f);
}

static void DrawRE2Video256(FB_TO_SCREEN_INFO *fb_info)
{
   uint32_t h, w;
   int tmu;
   GrTexInfo t_info;
   uint32_t *src          = (uint32_t*)(gfx_info.RDRAM + fb_info->addr);
   uint16_t *tex          = (uint16_t*)texture_buffer;
   uint16_t *dst          = (uint16_t*)tex;

   t_info.smallLodLog2    = GR_LOD_LOG2_256;
   t_info.largeLodLog2    = GR_LOD_LOG2_256;
   t_info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;

   fb_info->height        = MIN(256, fb_info->height);

   for (h = 0; h < fb_info->height; h++)
   {
      for (w = 0; w < 256; w++)
      {
         uint32_t col = *(src++);
         uint8_t r    = (uint8_t)((col >> 24)&0xFF);
         uint8_t g    = (uint8_t)((col >> 16)&0xFF);
         uint8_t b    = (uint8_t)((col >> 8)&0xFF);

         r = (uint8_t)((float)r / 255.0f * 31.0f);
         g = (uint8_t)((float)g / 255.0f * 63.0f);
         b = (uint8_t)((float)b / 255.0f * 31.0f);
         *(dst++) = PAL8toRGB565(r, g, b, 0);
      }
      src += (fb_info->width - 256);
   }
   t_info.format = GR_TEXFMT_RGB_565;
   t_info.data   = tex;
   tmu           = SetupFBtoScreenCombiner(
         grTexCalcMemRequired(t_info.largeLodLog2, t_info.aspectRatioLog2, t_info.format), fb_info->opaque);

   grTexSource (tmu,
         voodoo.tmem_ptr[tmu],
         GR_MIPMAPLEVELMASK_BOTH,
         &t_info, 
         true);
   DrawRE2Video(fb_info, 1.0f);
}

static void DrawFrameBufferToScreen256(FB_TO_SCREEN_INFO *fb_info)
{
  uint32_t w, h, x, y, width, height, width256, height256, tex_size, *src32;
  uint32_t tex_adr, w_tail, h_tail, bound, c32, idx;
  uint8_t r, g, b, a, *image;
  uint16_t *tex, *src, c;
  float ul_x, ul_y, lr_x, lr_y, lr_u, lr_v;
  int tmu;
  GrTexInfo t_info;

  if (settings.hacks & hack_RE2)
  {
     DrawRE2Video256(fb_info);
     return;
  }

  width                  = fb_info->lr_x - fb_info->ul_x + 1;
  height                 = fb_info->lr_y - fb_info->ul_y + 1;
  image                  = (uint8_t*)(gfx_info.RDRAM + fb_info->addr);
  width256               = ((width - 1) >> 8) + 1;
  height256              = ((height - 1) >> 8) + 1;
  t_info.smallLodLog2    = t_info.largeLodLog2 = GR_LOD_LOG2_256;
  t_info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
  t_info.format          = GR_TEXFMT_ARGB_1555;

  tex                    = (uint16_t*)texture_buffer;
  t_info.data            = tex;
  tex_size               = grTexCalcMemRequired(t_info.largeLodLog2, t_info.aspectRatioLog2, t_info.format);
  tmu                    = SetupFBtoScreenCombiner(tex_size * width256 * height256, fb_info->opaque);
  src                    = (uint16_t*)(image + fb_info->ul_x + fb_info->ul_y * fb_info->width);
  src32                  = (uint32_t*)(image + fb_info->ul_x + fb_info->ul_y * fb_info->width);
  w_tail                 = width % 256;
  h_tail                 = height % 256;
  bound                  = (BMASK + 1) - fb_info->addr;
  bound                  = fb_info->size == 2 ? (bound >> 1) : (bound >> 2);
  tex_adr                = voodoo.tmem_ptr[tmu];

  for (h = 0; h < height256; h++)
  {
    for (w = 0; w < width256; w++)
    {
      uint32_t cur_width  = (256 *  (w + 1) < width) ? 256 : w_tail;
      uint32_t cur_height = (256 * (h + 1) < height) ? 256 : h_tail;
      uint32_t cur_tail   = 256 - cur_width;
      uint16_t *dst_ptr   = (uint16_t*)tex;

      if (fb_info->size == 2)
      {
        for (y=0; y < cur_height; y++)
        {
          for (x=0; x < cur_width; x++)
          {
            idx = (x + 256 * w + (y + 256 * h) * fb_info->width) ^ 1;
            if (idx >= bound)
              break;
            c = src[idx];
            *(dst_ptr++) = (c >> 1) | ((c&1)<<15);
          }
          dst_ptr += cur_tail;
        }
      }
      else
      {
        for (y=0; y < cur_height; y++)
        {
          for (x=0; x < cur_width; x++)
          {
            idx = (x+256*w+(y+256*h)*fb_info->width);
            if (idx >= bound)
              break;
            c32 = src32[idx];
            r = (uint8_t)((c32 >> 24) & 0xFF);
            r = (uint8_t)((float)r / 255.0f * 31.0f);
            g = (uint8_t)((c32 >> 16) & 0xFF);
            g = (uint8_t)((float)g / 255.0f * 63.0f);
            b = (uint8_t)((c32 >> 8) & 0xFF);
            b = (uint8_t)((float)b / 255.0f * 31.0f);
            a = (c32 & 0xFF) ? 1 : 0;
            *(dst_ptr++) = (a<<15) | (r << 10) | (g << 5) | b;
          }
          dst_ptr += cur_tail;
        }
      }
      grTexSource (tmu, tex_adr, GR_MIPMAPLEVELMASK_BOTH, &t_info, true);
      tex_adr += tex_size;

      ul_x     = (fb_info->ul_x + 256 * w)    * rdp.scale_x + rdp.offset_x;
      ul_y     = (fb_info->ul_y + 256 * h)    * rdp.scale_y + rdp.offset_y;
      lr_x     = (ul_x + (float)(cur_width))  * rdp.scale_x + rdp.offset_x;
      lr_y     = (ul_y + (float)(cur_height)) * rdp.scale_y + rdp.offset_y;

      lr_u     = (float)(cur_width - 1);
      lr_v     = (float)(cur_height - 1);

      glide64_draw_fb(ul_x, ul_y, lr_x, lr_y, lr_u, lr_v, 0.5f);
    }
  }
}

static bool DrawFrameBufferToScreen(FB_TO_SCREEN_INFO *fb_info)
{
   uint32_t x, y, width, height, texwidth;
   uint8_t *image;
   int tmu;
   float scale;
   GrTexInfo t_info;

   if (fb_info->width < 200 || fb_info->size < 2)
      return false;

   width  = fb_info->lr_x - fb_info->ul_x + 1;
   height = fb_info->lr_y - fb_info->ul_y + 1;

   if (width > 512 || height > 512)
   {
      DrawFrameBufferToScreen256(fb_info);
      return true;
   }

   image = (uint8_t*)(gfx_info.RDRAM + fb_info->addr);

   texwidth               = 512;
   scale                  = 0.5f;
   t_info.smallLodLog2    = GR_LOD_LOG2_512;
   t_info.largeLodLog2    = GR_LOD_LOG2_512;
   t_info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;

   if (width <= 256)
   {
      texwidth = 256;
      scale = 1.0f;
      t_info.smallLodLog2 = t_info.largeLodLog2 = GR_LOD_LOG2_256;
   }

   if (height <= (texwidth>>1))
      t_info.aspectRatioLog2 = GR_ASPECT_LOG2_2x1;

   if (fb_info->size == 2)
   {
      uint16_t c;
      uint32_t idx;

      uint16_t *tex  = (uint16_t*)texture_buffer;
      uint16_t *dst  = (uint16_t*)tex;
      uint16_t *src  = (uint16_t*)(image + fb_info->ul_x + fb_info->ul_y * fb_info->width);

      uint32_t bound = (BMASK+1 - fb_info->addr) >> 1;
      bool empty     = true;

      for (y = 0; y < height; y++)
      {
         for (x = 0; x < width; x++)
         {
            idx = (x + y * fb_info->width) ^ 1;
            if (idx >= bound)
               break;
            c = src[idx];
            if (c)
               empty = false;
            *(dst++) = (c >> 1) | ((c & 1) << 15);
         }
         dst += texwidth-width;
      }
      if (empty)
         return false;
      t_info.format = GR_TEXFMT_ARGB_1555;
      t_info.data   = tex;
   }
   else
   {
      uint32_t *tex  = (uint32_t*)texture_buffer;
      uint32_t *dst  = (uint32_t*)tex;
      uint32_t *src  = (uint32_t*)(image + fb_info->ul_x + fb_info->ul_y * fb_info->width);
      uint32_t bound = (BMASK + 1 - fb_info->addr) >> 2;

      for (y = 0; y < height; y++)
      {
         for (x = 0; x < width; x++)
         {
            uint32_t col;
            uint32_t idx = x + y * fb_info->width;
            if (idx >= bound)
               break;
            col = src[idx];
            *(dst++) = (col >> 8) | 0xFF000000;
         }
         dst += texwidth-width;
      }
      t_info.format = GR_TEXFMT_ARGB_8888;
      t_info.data   = tex;
   }

   tmu = SetupFBtoScreenCombiner(
         grTexCalcMemRequired(t_info.largeLodLog2,
            t_info.aspectRatioLog2,
            t_info.format),
         fb_info->opaque);

   grTexSource (tmu,
         voodoo.tmem_ptr[tmu],
         GR_MIPMAPLEVELMASK_BOTH,
         &t_info, true);

   if (settings.hacks&hack_RE2)
      DrawRE2Video(fb_info, scale);
   else
   {
      float ul_x = fb_info->ul_x * rdp.scale_x + rdp.offset_x;
      float ul_y = fb_info->ul_y * rdp.scale_y + rdp.offset_y;
      float lr_x = fb_info->lr_x * rdp.scale_x + rdp.offset_x;
      float lr_y = fb_info->lr_y * rdp.scale_y + rdp.offset_y;
      float lr_u = (width  - 1) * scale;
      float lr_v = (height - 1) * scale;

      glide64_draw_fb(ul_x, ul_y, lr_x,
            lr_y, lr_u, lr_v, 0.5f);
   }
   return true;
}

void copyWhiteToRDRAM(void)
{
   uint32_t y, x;
   if(g_gdp.fb_width == 0)
      return;

   if(g_gdp.fb_size == G_IM_SIZ_32b)
   {
      uint32_t *ptr_dst = (uint32_t*)(gfx_info.RDRAM + gDP.colorImage.address);
      for(y = 0; y < gDP.colorImage.height; y++)
      {
         for(x = 0; x < g_gdp.fb_width; x++)
            ptr_dst[x + y * g_gdp.fb_width] = 0xFFFFFFFF;
      }
   }
   else
   {
      uint16_t *ptr_dst = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);
      for(y = 0; y < gDP.colorImage.height; y++)
      {
         for(x = 0; x < g_gdp.fb_width; x++)
            ptr_dst[(x + y * g_gdp.fb_width) ^ 1] = 0xFFFF;
      }
   }
}

void DrawWholeFrameBufferToScreen(void)
{
  FB_TO_SCREEN_INFO fb_info;
  static uint32_t toScreenCI = 0;

  if (gDP.colorImage.width < 200)
    return;
  if (gDP.colorImage.address == toScreenCI)
    return;
  if (gDP.colorImage.height == 0)
     return;
  toScreenCI = gDP.colorImage.address;

  fb_info.addr   = gDP.colorImage.address;
  fb_info.size   = g_gdp.fb_size;
  fb_info.width  = gDP.colorImage.width;
  fb_info.height = gDP.colorImage.height;
  fb_info.ul_x   = 0;
  fb_info.lr_x   = gDP.colorImage.width-1;
  fb_info.ul_y   = 0;
  fb_info.lr_y   = gDP.colorImage.height-1;
  fb_info.opaque = 0;

  DrawFrameBufferToScreen(&fb_info);

  if (!(settings.frame_buffer & fb_ref))
    memset(gfx_info.RDRAM + gDP.colorImage.address, 0,
          (gDP.colorImage.width * gDP.colorImage.height) << g_gdp.fb_size >> 1);
}

void CopyFrameBuffer(int32_t buffer)
{
   uint32_t height = 0;
   uint32_t width  = gDP.colorImage.width;//*gfx_info.VI_WIDTH_REG;

   if (fb_emulation_enabled && !(settings.hacks&hack_PPL))
   {
      int ind = (rdp.ci_count > 0) ? (rdp.ci_count-1) : 0;
      height = rdp.frame_buffers[ind].height;
   }
   else
   {
      height = rdp.ci_lower_bound;

      if (settings.hacks&hack_PPL)
         height -= rdp.ci_upper_bound;
   }

   if (rdp.scale_x < 1.1f)
   {
      uint16_t * ptr_src = (uint16_t*)glide64_frameBuffer;
      if (grLfbReadRegion(buffer,
               (uint32_t)rdp.offset_x,
               (uint32_t)rdp.offset_y,//rdp.ci_upper_bound,
               width,
               height,
               width<<1,
               ptr_src))
      {
         uint32_t y, x;

         if (g_gdp.fb_size == G_IM_SIZ_16b)
         {
            uint16_t *ptr_dst   = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);

            for (y = 0; y < height; y++)
            {
               for (x = 0; x < width; x++)
               {
                  uint16_t c = ptr_src[x + y * width];
                  if ((settings.frame_buffer & fb_read_alpha) && c <= 0) {}
                  else
                     c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
                  ptr_dst[(x + y * width)^1] = c;
               }
            }
         }
         else
         {
            uint32_t *ptr_dst = (uint32_t*)(gfx_info.RDRAM + gDP.colorImage.address);
            for (y = 0; y < height; y++)
            {
               for (x = 0; x < width; x++)
               {
                  uint16_t c = ptr_src[x + y * width];
                  if ((settings.frame_buffer & fb_read_alpha) && c <= 0) {}
                  else
                     c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
                  ptr_dst[x + y * width]   = RGBA16toRGBA32(c);
               }
            }
         }
      }
   }
   else
   {
      GrLfbInfo_t info;
      float scale_x = (settings.scr_res_x - rdp.offset_x*2.0f)  / MAX(width, rdp.vi_width);
      float scale_y = (settings.scr_res_y - rdp.offset_y*2.0f) / MAX(height, rdp.vi_height);

      FRDP("width: %d, height: %d, ul_y: %d, lr_y: %d, scale_x: %f, scale_y: %f, ci_width: %d, ci_height: %d\n",width, height, rdp.ci_upper_bound, rdp.ci_lower_bound, scale_x, scale_y, gDP.colorImage.width, gDP.colorImage.height);
      info.size = sizeof(GrLfbInfo_t);

      if (grLfbLock (GR_LFB_READ_ONLY,
               buffer,
               GR_LFBWRITEMODE_565,
               GR_ORIGIN_UPPER_LEFT,
               FXFALSE,
               &info))
      {
         int        x_start  = 0;
         int        y_start  = 0;
         int         x_end   = width;
         int         y_end   = height;
         uint32_t stride     = info.strideInBytes>>1;
         int read_alpha      = settings.frame_buffer & fb_read_alpha;

         if ((settings.hacks&hack_PMario) && rdp.ci_count > 0 && rdp.frame_buffers[rdp.ci_count-1].status != CI_AUX)
            read_alpha = false;

         if (settings.hacks&hack_BAR)
         {
            x_start = 80;
            y_start = 24;
            x_end   = 240;
            y_end   = 86;
         }

         if (g_gdp.fb_size <= G_IM_SIZ_16b)
         {
            int y, x;
            uint16_t *ptr_src   = (uint16_t*)info.lfbPtr;
            uint16_t *ptr_dst   = (uint16_t*)(gfx_info.RDRAM + gDP.colorImage.address);

            for (y = y_start; y < y_end; y++)
            {
               for (x = x_start; x < x_end; x++)
               {
                  uint16_t c = ptr_src[(int)(x*scale_x + rdp.offset_x) + (int)(y * scale_y + rdp.offset_y) * stride];
                  c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
                  if (read_alpha && c == 1)
                     c = 0;
                  ptr_dst[(x + y * width)^1] = c;
               }
            }
         }
         else
         {
            int y, x;
            uint16_t *ptr_src   = (uint16_t*)info.lfbPtr;
            uint32_t *ptr_dst = (uint32_t*)(gfx_info.RDRAM + gDP.colorImage.address);

            for (y = y_start; y < y_end; y++)
            {
               for (x = x_start; x < x_end; x++)
               {
                  uint16_t c = ptr_src[(int)(x*scale_x + rdp.offset_x) + (int)(y * scale_y + rdp.offset_y) * stride];
                  c = (c&0xFFC0) | ((c&0x001F) << 1) | 1;
                  if (read_alpha && c == 1)
                     c = 0;
                  ptr_dst[x + y * width] = RGBA16toRGBA32(c);
               }
            }
         }

         // Unlock the backbuffer
         grLfbUnlock (GR_LFB_READ_ONLY, buffer);
         LRDP("LfbLock.  Framebuffer copy complete.\n");
      }
   }
}

float ScaleZ(float z)
{
   if (settings.n64_z_scale)
   {
      int iz = (int)(z*8.0f+0.5f);
      if (iz < 0)
         iz = 0;
      else if (iz >= ZLUT_SIZE)
         iz = ZLUT_SIZE - 1;
      return (float)zLUT[iz];
   }
   if (z  < 0.0f)
      return 0.0f;
   z *= 1.9f;
   if (z > 65535.0f)
      return 65535.0f;
   return z;
}

void DrawDepthBuffer(VERTEX * vtx, int n)
{
   unsigned i     = 0;
   if ( gfx_plugin_accuracy < 3)
       return;

   if (fb_depth_render_enabled && dzdx && (rdp.flags & ZBUF_UPDATE))
   {
      struct vertexi v[12];
      int index = 0;
      int inc   = 1;

      if (rdp.u_cull_mode == 1) //cull front
      {
         index   = n - 1;
         inc     = -1;
      }

      for (i = 0; i < n; i++)
      {
         v[i].x = (int)((vtx[index].x - rdp.offset_x) / rdp.scale_x * 65536.0);
         v[i].y = (int)((vtx[index].y - rdp.offset_y) / rdp.scale_y * 65536.0);
         v[i].z = (int)(vtx[index].z * 65536.0);
         index += inc;
      }
      DepthBufferRasterize(v, n, dzdx);
   }

   for (i = 0; i < n; i++)
      vtx[i].z = ScaleZ(vtx[i].z);
}

void DrawDepthBufferFog(void)
{
   FB_TO_SCREEN_INFO fb_info;
   if (rdp.zi_width < 200)
      return;

   fb_info.addr   = g_gdp.zb_address;
   fb_info.size   = 2;
   fb_info.width  = rdp.zi_width;
   fb_info.height = gDP.colorImage.height;
   fb_info.ul_x   = g_gdp.__clip.xh;
   fb_info.lr_x   = g_gdp.__clip.xl;
   fb_info.ul_y   = g_gdp.__clip.yh;
   fb_info.lr_y   = g_gdp.__clip.yl;
   fb_info.opaque = 0;

   DrawDepthBufferToScreen(&fb_info);
}

void drawViRegBG(void)
{
   bool drawn;
   FB_TO_SCREEN_INFO fb_info;

   fb_info.width  = *gfx_info.VI_WIDTH_REG;
   fb_info.height = (uint32_t)rdp.vi_height;
   fb_info.ul_x   = 0;
   fb_info.lr_x   = fb_info.width - 1;
   fb_info.ul_y   = 0;
   fb_info.lr_y   = fb_info.height - 1;
   fb_info.opaque = 1;
   fb_info.addr   = *gfx_info.VI_ORIGIN_REG;
   fb_info.size   = *gfx_info.VI_STATUS_REG & 3;

   rdp.last_bg    = fb_info.addr;

   drawn          = DrawFrameBufferToScreen(&fb_info);

   if (settings.hacks&hack_Lego && drawn)
   {
      rdp.updatescreen = 1;
      newSwapBuffers ();
      DrawFrameBufferToScreen(&fb_info);
   }
}

part_framebuffer part_framebuf;

void DrawPartFrameBufferToScreen(void)
{
   FB_TO_SCREEN_INFO fb_info;

   fb_info.addr   = gDP.colorImage.address;
   fb_info.size   = g_gdp.fb_size;
   fb_info.width  = gDP.colorImage.width;
   fb_info.height = gDP.colorImage.height;
   fb_info.ul_x   = part_framebuf.d_ul_x;
   fb_info.lr_x   = part_framebuf.d_lr_x;
   fb_info.ul_y   = part_framebuf.d_ul_y;
   fb_info.lr_y   = part_framebuf.d_lr_y;
   fb_info.opaque = 0;

   DrawFrameBufferToScreen(&fb_info);
   memset(gfx_info.RDRAM + gDP.colorImage.address, 0, (gDP.colorImage.width * gDP.colorImage.height) << g_gdp.fb_size >> 1);
}
