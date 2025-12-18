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

#ifndef _GLIDE64_UTIL_H
#define _GLIDE64_UTIL_H

#include "../Glitch64/glide.h"
#include "rdp.h"
#include "../../../mupen64plus-core/src/main/util.h"

#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/RDP/RDP_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NOT_TMU0	0x00
#define NOT_TMU1	0x01
#define NOT_TMU2	0x02

#define clip_tri_uv(first, second, index, percent) \
   rdp.vtxbuf[index].u[0] = first->u[0] + (second->u[0] - first->u[0]) * percent; \
   rdp.vtxbuf[index].v[0] = first->v[0] + (second->v[0] - first->v[0]) * percent; \
   rdp.vtxbuf[index].u[1] = first->u[1] + (second->u[1] - first->u[1]) * percent; \
   rdp.vtxbuf[index].v[1] = first->v[1] + (second->v[1] - first->v[1]) * percent

#define clip_tri_interp_colors(first, second, index, percent, val, interpolate_colors) \
   if (interpolate_colors) \
      glide64_interpolate_colors(first, second, &rdp.vtxbuf[index++], percent, 1.0f, 1.0f, 1.0f); \
   else \
      rdp.vtxbuf[index++].number = first->number | second->number | val

void do_triangle_stuff(uint16_t linew, int old_interpolate);
void do_triangle_stuff_2(uint16_t linew, uint8_t no_clip, int old_interpolate);
void apply_shade_modulation(VERTEX *v);

void update(void);
void update_scissor(bool set_scissor);

float ScaleZ(float z);

// rotate left
#define __ROL__(value, count, nbits) ((value << (count % (nbits))) | (value >> ((nbits) - (count % (nbits)))))

static INLINE void draw_tri_uv_calculation_update_shift(unsigned cur_tile, unsigned index, VERTEX *v)
{
   if (g_gdp.tile[cur_tile].shift_s)
   {
      if (g_gdp.tile[cur_tile].shift_s > 10)
         v->u[index] *= (float)(1 << (16 - g_gdp.tile[cur_tile].shift_s));
      else
         v->u[index] /= (float)(1 << g_gdp.tile[cur_tile].shift_s);
   }

   if (g_gdp.tile[cur_tile].shift_t)
   {
      if (g_gdp.tile[cur_tile].shift_t > 10)
         v->v[index] *= (float)(1 << (16 - g_gdp.tile[cur_tile].shift_t));
      else
         v->v[index] /= (float)(1 << g_gdp.tile[cur_tile].shift_t);
   }

   v->u[index]   -= gDP.tiles[cur_tile].fuls;
   v->v[index]   -= gDP.tiles[cur_tile].fult;
   v->u[index]    = rdp.cur_cache[index]->c_off + rdp.cur_cache[index]->c_scl_x * v->u[index];
   v->v[index]    = rdp.cur_cache[index]->c_off + rdp.cur_cache[index]->c_scl_y * v->v[index];
   v->u_w[index]  = v->u[index] / v->w;
   v->v_w[index]  = v->v[index] / v->w;
}

static INLINE uint32_t rol32(uint32_t value, uint32_t amount)
{
    return (value << amount) | (value >> (-(int32_t)amount & 31));
}

static INLINE uint32_t ror32(uint32_t value, uint32_t amount)
{
    return (value << (-(int32_t)amount & 31)) | (value >> amount);
}

static INLINE uint16_t rol16(uint16_t value, uint16_t amount)
{
    return (value << amount) | (value >> (-(int16_t)amount & 15));
}

static INLINE uint16_t ror16(uint16_t value, uint16_t amount)
{
    return (value << (-(int16_t)amount & 15)) | (value >> amount);
}

void apply_shading(void *data);

#ifdef __cplusplus
}
#endif

#endif  // ifndef Util_H
