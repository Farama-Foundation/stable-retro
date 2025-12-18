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

#include <stdint.h>
#include <stdlib.h>
#include "glide.h"
#include "glitchmain.h"
#include "uthash.h"

/* Napalm extensions to GrTextureFormat_t */
#define GR_TEXFMT_ARGB_8888               0x12
#define GR_TEXFMT_YUYV_422                0x13
#define GR_TEXFMT_UYVY_422                0x14
#define GR_TEXFMT_AYUV_444                0x15
#define GR_TEXTFMT_RGB_888                0xFF

int packed_pixels_support = -1;

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

int tex_width[2], tex_height[2];
int tex_exactWidth[2],tex_exactHeight[2];
int three_point_filter[2];
float lambda;

static int min_filter[2] = { GL_NEAREST, GL_NEAREST },
mag_filter[2] = { GL_NEAREST, GL_NEAREST },
wrap_s[2] = { GL_REPEAT, GL_REPEAT },
wrap_t[2] = { GL_REPEAT, GL_REPEAT };

unsigned char *filter(unsigned char *source, int width, int height, int *width2, int *height2);

typedef struct _texlist
{
   unsigned int id;
   GLuint tex_id;
   UT_hash_handle hh;
} texlist;

static texlist *list = NULL;

//#define LOG_TEXTUREMEM 1

static void remove_tex(unsigned int idmin, unsigned int idmax)
{
   GLuint *t;
   unsigned int n = 0;
   texlist *current, *tmp;

   unsigned count = HASH_COUNT(list);

   if (!count)
       return;

   t = (GLuint*)malloc(count * sizeof(GLuint));
   HASH_ITER(hh, list, current, tmp)
   {
      if (current->id >= idmin && current->id < idmax)
      {
         t[n++] = current->tex_id;
         HASH_DEL(list, current);
         free(current);
      }
   }
   glDeleteTextures(n, t);
   free(t);
#ifdef LOG_TEXTUREMEM
   if (log_cb)
      log_cb(RETRO_LOG_DEBUG, "RMVTEX nbtex is now %d (%06x - %06x)\n", HASH_COUNT(list), idmin, idmax);
#endif
}


static void add_tex(unsigned int id)
{
   texlist *entry;
   
   HASH_FIND_INT(list, &id, entry);

   if (!entry)
   {
      entry = malloc(sizeof(texlist));
      entry->id = id;
      glGenTextures(1, &entry->tex_id);
      HASH_ADD_INT(list, id, entry);
   }

#ifdef LOG_TEXTUREMEM
  if (log_cb)
     log_cb(RETRO_LOG_DEBUG, "ADDTEX nbtex is now %d (%06x)\n", HASH_COUNT(list), id);
#endif
}

static GLuint get_tex_id(unsigned int id)
{
   texlist *entry;

   HASH_FIND_INT(list, &id, entry);

   if (!entry)
   {
#ifdef LOG_TEXTUREMEM
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "get_tex_id for %08x failed!\n", id);
#endif
      return 0;
   }

   return entry->tex_id;
}

void init_textures(void)
{
   list = NULL;
}

void free_textures(void)
{
  remove_tex(0x00000000, 0xFFFFFFFF);
}

uint32_t grTexCalcMemRequired(int32_t lodmax,
      int32_t aspect, int32_t fmt)
{
   int width  = 1 << lodmax;
   int height = 1 << lodmax;

   if (aspect < 0)
      width = height >> -aspect;
   else
      height = width >> aspect;

   switch(fmt)
   {
      case GR_TEXFMT_ALPHA_8:
      case GR_TEXFMT_INTENSITY_8: // I8 support - H.Morii
      case GR_TEXFMT_ALPHA_INTENSITY_44:
         return width*height;
      case GR_TEXFMT_ARGB_1555:
      case GR_TEXFMT_ARGB_4444:
      case GR_TEXFMT_ALPHA_INTENSITY_88:
      case GR_TEXFMT_RGB_565:
         return (width*height)<<1;
      case GR_TEXFMT_ARGB_8888:
         return (width*height)<<2;
   }

   return 0;
}

static int grTexFormat2GLPackedFmt(GrTexInfo *info, int fmt, int * gltexfmt, int * glpixfmt, int * glpackfmt)
{
   int factor        = -1;
   unsigned size_tex = width * height;

   if (fmt == GR_TEXFMT_ALPHA_INTENSITY_44)
   {
      uint16_t *texture_ptr = &((uint16_t*)info->data)[size_tex];
      uint8_t *texture_ptr8 = &((uint8_t*)info->data)[size_tex];
      //FIXME - still CPU software color conversion
      do{
         uint16_t texel = (uint16_t)*texture_ptr8--;
         // Replicate glide's ALPHA_INTENSITY_44 to match gl's LUMINANCE_ALPHA
         texel = (texel & 0x00F0) << 4 | (texel & 0x000F);
         *texture_ptr-- = (texel << 4) | texel;
      }while(size_tex--);
      factor = 1;
      *gltexfmt = GL_LUMINANCE_ALPHA;
      *glpixfmt = GL_LUMINANCE_ALPHA;
      *glpackfmt = GL_UNSIGNED_BYTE;
   }
#ifndef HAVE_OPENGLES2
   else if (packed_pixels_support)
   {
      switch(fmt)
      {
         case GR_TEXFMT_ALPHA_8:
            factor = 1;
            *gltexfmt = GL_INTENSITY8;
            *glpixfmt = GL_LUMINANCE;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_INTENSITY_8: // I8 support - H.Morii
            factor = 1;
            *gltexfmt = GL_LUMINANCE8;
            *glpixfmt = GL_LUMINANCE;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_RGB_565:
            factor = 2;
            *gltexfmt = GL_RGB;
            *glpixfmt = GL_RGB;
            *glpackfmt = GL_UNSIGNED_SHORT_5_6_5;
            break;
         case GR_TEXFMT_ARGB_1555:
            factor = 2;
            *gltexfmt = GL_RGB5_A1;
            *glpixfmt = GL_BGRA;
            *glpackfmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            break;
         case GR_TEXFMT_ALPHA_INTENSITY_88:
            factor = 2;
            *gltexfmt = GL_LUMINANCE8_ALPHA8;
            *glpixfmt = GL_LUMINANCE_ALPHA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_ARGB_4444:
            factor = 2;
            *gltexfmt = GL_RGBA4;
            *glpixfmt = GL_BGRA;
            *glpackfmt = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            break;
         case GR_TEXFMT_ARGB_8888:
            factor = 4;
            *gltexfmt = GL_RGBA8;
            *glpixfmt = GL_BGRA;
            *glpackfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
            break;
      }
   }
#endif
   else
   {
      // VP fixed the texture conversions to be more accurate, also swapped
      // the for i/j loops so that is is less likely to break the memory cache
      unsigned size_tex = width * height;
      switch(info->format)
      {
         case GR_TEXFMT_ALPHA_8:
            do
            {
               uint16_t texel = (uint16_t)((uint8_t*)info->data)[size_tex];

               // Replicate glide's ALPHA_8 to match gl's LUMINANCE_ALPHA
               // This is to make up for the lack of INTENSITY in gles
               ((uint16_t*)info->data)[size_tex] = texel | (texel << 8);
            }while(size_tex--);
            factor = 1;
            *glpixfmt = GL_LUMINANCE_ALPHA;
            *gltexfmt = GL_LUMINANCE_ALPHA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_INTENSITY_8: // I8 support - H.Morii
            do
            {
               unsigned int texel = (unsigned int)((uint8_t*)info->data)[size_tex];
               texel |= (0xFF000000 | (texel << 16) | (texel << 8));
               ((unsigned int*)info->data)[size_tex] = texel;
            }while(size_tex--);
            factor = 1;
            *glpixfmt = GL_RGBA;
            *gltexfmt = GL_RGBA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_RGB_565:
            do
            {
               unsigned int texel = (unsigned int)((uint16_t*)info->data)[size_tex];
               unsigned int B = texel & 0x0000F800;
               unsigned int G = texel & 0x000007E0;
               unsigned int R = texel & 0x0000001F;
               ((unsigned int*)info->data)[size_tex] = 0xFF000000 | (R << 19) | (G << 5) | (B >> 8);
            }while(size_tex--);
            factor = 2;
            *glpixfmt = GL_RGBA;
            *gltexfmt = GL_RGBA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_ARGB_1555:
            do
            {
               uint16_t texel = ((uint16_t*)info->data)[size_tex];

               // Shift-rotate glide's ARGB_1555 to match gl's RGB5_A1
               ((uint16_t*)info->data)[size_tex] = (texel << 1) | (texel >> 15);
            }while(size_tex--);
            factor = 2;
            *glpixfmt = GL_RGBA;
            *gltexfmt = GL_RGBA;
            *glpackfmt = GL_UNSIGNED_SHORT_5_5_5_1;
            break;
         case GR_TEXFMT_ALPHA_INTENSITY_88:
            do
            {
               unsigned int AI = (unsigned int)((uint16_t*)info->data)[size_tex];
               unsigned int I = (unsigned int)(AI & 0x000000FF);
               ((unsigned int*)info->data)[size_tex] = (AI << 16) | (I << 8) | I;
            }while(size_tex--);
            factor = 2;
            *glpixfmt = GL_RGBA;
            *gltexfmt = GL_RGBA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_ARGB_4444:
            do
            {
               unsigned int texel = (unsigned int)((uint16_t*)info->data)[size_tex];
               unsigned int A = texel & 0x0000F000;
               unsigned int B = texel & 0x00000F00;
               unsigned int G = texel & 0x000000F0;
               unsigned int R = texel & 0x0000000F;
               ((unsigned int*)info->data)[size_tex] = (A << 16) | (R << 20) | (G << 8) | (B >> 4);
            }while(size_tex--);
            factor = 2;
            *glpixfmt = GL_RGBA;
            *gltexfmt = GL_RGBA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         case GR_TEXFMT_ARGB_8888:
#ifdef HAVE_OPENGLES2
            if (bgra8888_support)
            {
               factor = 4;
               *gltexfmt = GL_BGRA_EXT;
               *glpixfmt = GL_BGRA_EXT;
               *glpackfmt = GL_UNSIGNED_BYTE;
               break;
            }
#endif
            do
            {
               unsigned int texel = ((unsigned int*)info->data)[size_tex];
               unsigned int A = texel & 0xFF000000;
               unsigned int B = texel & 0x00FF0000;
               unsigned int G = texel & 0x0000FF00;
               unsigned int R = texel & 0x000000FF;
               ((unsigned int*)info->data)[size_tex] = A | (R << 16) | G | (B >> 16);
            }while(size_tex--);
            factor = 4;
            *glpixfmt = GL_RGBA;
            *gltexfmt = GL_RGBA;
            *glpackfmt = GL_UNSIGNED_BYTE;
            break;
         default:
            factor = 0;
      }
   }
   return factor;
}

void grTexSource( int32_t tmu,
      uint32_t      startAddress,
      uint32_t      evenOdd,
      GrTexInfo  *info,
      int do_download)
{
   int width, height;
   int factor;
   int gltexfmt, glpixfmt, glpackfmt;
   unsigned index = (tmu == GR_TMU1) ? 0 : 1;

   glActiveTexture((tmu == GR_TMU1) ? GL_TEXTURE0 : GL_TEXTURE1);
   if (!do_download)
      goto grtexsource;

   height = 1 << info->largeLodLog2;
   width = 1 << info->largeLodLog2;
   if (info->aspectRatioLog2 < 0)
      width = height >> -info->aspectRatioLog2;
   else
      height = width >> info->aspectRatioLog2;

   factor = grTexFormat2GLPackedFmt(info, info->format, &gltexfmt, &glpixfmt, &glpackfmt);

   remove_tex(startAddress+1, startAddress+1+(width * height * factor));

   add_tex(startAddress+1);
   glBindTexture(GL_TEXTURE_2D, get_tex_id(startAddress+1));

   glTexImage2D(GL_TEXTURE_2D, 0, gltexfmt, width, height, 0, glpixfmt, glpackfmt, info->data);
   info->width = width;
   info->height = height;

grtexsource:
   tex_height[index] = 256;
   tex_width[index] = 256;
   if (info->aspectRatioLog2 < 0)
      tex_width[index] = tex_height[index] >> -info->aspectRatioLog2;
   else
      tex_height[index] = tex_width[index] >> info->aspectRatioLog2;

   if (!do_download)
      glBindTexture(GL_TEXTURE_2D, get_tex_id(startAddress+1));
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter[index]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter[index]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s[index]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t[index]);
   tex_exactWidth[index] = info->width;
   tex_exactHeight[index] = info->height;
}


void grTexDetailControl(
      int32_t tmu,
      int lod_bias,
      uint8_t detail_scale,
      float detail_max
      )
{
   if (lod_bias != 31 && detail_scale != 7)
   {
      if (!lod_bias && !detail_scale && !detail_max)
         return;
   }
   lambda = detail_max;
   if(lambda > 1.0f)
      lambda = 1.0f - (255.0f - lambda);

   set_lambda();
}

void grTexFilterClampMode(
      int32_t tmu,
      int32_t s_clampmode,
      int32_t t_clampmode,
      int32_t minfilter_mode,
      int32_t magfilter_mode
      )
{
   unsigned active_texindex = GL_TEXTURE1;
   unsigned index = 1;

   if (tmu == GR_TMU1)
   {
      index = 0;
      active_texindex = GL_TEXTURE0;
   }

   glActiveTexture(active_texindex);

   wrap_s[index] = s_clampmode;
   wrap_t[index] = t_clampmode;
   min_filter[index] = (minfilter_mode == GR_TEXTUREFILTER_POINT_SAMPLED) ? GL_NEAREST : GL_LINEAR;
   mag_filter[index] = (magfilter_mode == GR_TEXTUREFILTER_POINT_SAMPLED) ? GL_NEAREST : GL_LINEAR;

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s[index]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t[index]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter[index]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter[index]);

#ifdef DISABLE_3POINT
   three_point_filter[index] =false;
#else
   three_point_filter[index] = (magfilter_mode == GR_TEXTUREFILTER_3POINT_LINEAR);
#endif

   need_to_compile = 1;
}
