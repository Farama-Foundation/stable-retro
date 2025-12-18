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
#include <stdint.h>
#include <string.h>

#include <retro_inline.h>

/* 8-bit Horizontal Mirror */
static void Mirror8bS (uint8_t *tex, uint32_t mask,
      uint32_t max_width, uint32_t real_width, uint32_t height)
{
   uint32_t mask_width = (1 << mask);
   uint32_t mask_mask  = (mask_width-1);
   int32_t count       = max_width - mask_width;
   int32_t line_full   = real_width;
   int32_t line        = line_full - (count);
   uint8_t *start      = (uint8_t*)(tex + (mask_width));

   do
   {
      int v10 = 0;
      do
      {
         if ( mask_width & (v10 + mask_width) )
            *start++ = *(&tex[mask_mask] - (mask_mask & v10));
         else
            *start++ = tex[mask_mask & v10];
      }while ( ++v10 != count );
      start += line;
      tex += line_full;
   }while ( --height);
}

/* 8-bit Horizontal Clamp */
static void Clamp8bS (uint8_t *tex, uint32_t width,
      uint32_t clamp_to, uint32_t real_width, uint32_t real_height)
{
   uint8_t *dest     = tex + (width);
   uint8_t *constant = dest-1;
   int32_t count     = clamp_to - width;
   int32_t line_full = real_width;
   int32_t line      = width;

   do
   {
      unsigned i = count;
      do
      {
         *dest++ = *constant;
      }while(--i);
      constant += line_full;
      dest += line;
   }while (--real_height);
}

/* 16-bit Horizontal Mirror */
static void Mirror16bS (uint8_t *tex, uint32_t mask,
      uint32_t max_width, uint32_t real_width, uint32_t height)
{
   uint32_t mask_width = (1 << mask);
   uint32_t mask_mask  = (mask_width-1) << 1;
   int32_t count       = max_width - mask_width;
   int32_t line_full   = real_width << 1;
   int32_t line        = line_full - (count << 1);
   uint16_t *v8        = (uint16_t *)(uint8_t*)(tex + (mask_width << 1));
   do
   {
      int v10 = 0;
      do
      {
         if ( mask_width & (v10 + mask_width) )
            *v8++ = *(uint16_t *)(&tex[mask_mask] - (mask_mask & 2 * v10));
         else
            *v8++ = *(uint16_t *)&tex[mask_mask & 2 * v10];
      }while ( ++v10 != count );
      v8 = (uint16_t *)((int8_t*)v8 + line);
      tex += line_full;
   }while (--height);
}

/* 16-bit Horizontal Clamp */

static void Clamp16bS (uint8_t *tex, uint32_t width,
      uint32_t clamp_to, uint32_t real_width, uint32_t real_height)
{
   uint8_t *dest     = (uint8_t*)(tex + (width << 1));
   uint8_t *constant = (uint8_t*)(dest-2);
   int32_t count     = clamp_to - width;
   int32_t line_full = real_width << 1;
   int32_t line      = width << 1;
   uint16_t *v6      = (uint16_t *)constant;
   uint16_t *v7      = (uint16_t *)dest;

   do
   {
      int v10 = count;
      do
      {
         *v7++ = *v6;
      }while (--v10 );
      v6 = (uint16_t *)((int8_t*)v6 + line_full);
      v7 = (uint16_t *)((int8_t*)v7 + line);
   }while(--real_height);
}

static INLINE void clamp32bS(uint8_t *tex,
      uint8_t *constant, int height, int line, int full, int count)
{
   uint32_t *v6 = (uint32_t *)constant;
   uint32_t *v7 = (uint32_t *)tex;

   do
   {
      int v10 = count;
      do
      {
         *v7++ = *v6;
      }while (--v10);
      v6 = (uint32_t *)((int8_t*)v6 + full);
      v7 = (uint32_t *)((int8_t*)v7 + line);
   }while (--height);
}

/* 32-bit Horizontal Mirror */
static void Mirror32bS (uint8_t *tex, uint32_t mask, uint32_t max_width,
      uint32_t real_width, uint32_t height)
{
   uint32_t mask_width = (1 << mask);
   uint32_t mask_mask  = (mask_width-1) << 2;
   int32_t count       = max_width - mask_width;
   int32_t line_full   = real_width << 2;
   int32_t line        = line_full - (count << 2);
   uint8_t *start      = (uint8_t*)(tex + (mask_width << 2));
   uint32_t *v8        = (uint32_t *)start;

   do
   {
      int v10 = 0;
      do
      {
         if ( mask_width & (v10 + mask_width) )
            *v8++ = *(uint32_t *)(&tex[mask_mask] - (mask_mask & 4 * v10));
         else
            *v8++ = *(uint32_t *)&tex[mask_mask & 4 * v10];
      }while (++v10 != count );
      v8 = (uint32_t *)((int8_t*)v8 + line);
      tex += line_full;
   }while (--height);
}

//****************************************************************
// 32-bit Horizontal Clamp

static void Clamp32bS (uint8_t *tex, uint32_t width, uint32_t clamp_to,
      uint32_t real_width, uint32_t real_height)
{
	uint8_t *dest = (uint8_t*)(tex + (width << 2));
	uint8_t *constant = (uint8_t*)(dest-4);
	int32_t count = clamp_to - width;
	int32_t line_full = real_width << 2;
	int32_t line = width << 2;
	if (real_width > width)
      clamp32bS (dest, constant, real_height, line, line_full, count);
}

void MirrorTex (uint8_t *tex, uint32_t mask, uint32_t max_width,
      uint32_t real_width, uint32_t height, uint32_t size)
{
   switch (size)
   {
      case 1:
         Mirror16bS (tex, mask, max_width, real_width, height);
         break;
      case 2:
         Mirror32bS (tex, mask, max_width, real_width, height);
         break;
      default:
         Mirror8bS (tex, mask, max_width, real_width, height);
         break;
   }
}

void ClampTex(uint8_t *tex, uint32_t width,
      uint32_t clamp_to, uint32_t real_width, uint32_t real_height,
      uint32_t size)
{
   switch (size)
   {
      case 1:
         Clamp16bS (tex, width, clamp_to, real_width, real_height);
         break;
      case 2:
         Clamp32bS (tex, width, clamp_to, real_width, real_height);
         break;
      default:
         Clamp8bS (tex,  width, clamp_to, real_width, real_height);
         break;
   }
}
