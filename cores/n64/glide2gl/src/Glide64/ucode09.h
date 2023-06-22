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
// December 2008 Created by Gonetz (Gonetz@ngs.ru)
//
//****************************************************************

#include <stdint.h>

#include "../../Graphics/HLE/Microcode/ZSort.h"
#include "../../Graphics/RSP/gSP_state.h"

ZSORTRDP zSortRdp = {{0, 0}, {0, 0}, 0, 0};

static void uc9_draw_object (uint8_t * addr, uint32_t type)
{
   uint32_t i;
   VERTEX vtx[4], *pV[4];
   uint32_t textured = 0;
   uint32_t vnum     = 0;
   uint32_t vsize    = 0;

   switch (type)
   {
      case ZH_NULL:
         break;
      case ZH_SHTRI:
         vnum     = 3;
         vsize    = 8;
         break;
      case ZH_TXTRI:
         textured = 1;
         vnum     = 3;
         vsize    = 16;
         break;
      case ZH_SHQUAD:
         vnum     = 4;
         vsize    = 8;
         break;
      case ZH_TXQUAD:
         textured = 1;
         vnum     = 4;
         vsize    = 16;
         break;
   }

   for (i = 0; i < vnum; i++)
   {
      VERTEX *v = (VERTEX*)&vtx[i];
      v->sx = zSortRdp.scale_x * ((int16_t*)addr)[0^1];
      v->sy = zSortRdp.scale_y * ((int16_t*)addr)[1^1];
      v->sz = 1.0f;
      v->r = addr[4^3];
      v->g = addr[5^3];
      v->b = addr[6^3];
      v->a = addr[7^3];
      v->flags = 0;
      v->uv_scaled = 0;
      v->uv_calculated = 0xFFFFFFFF;
      v->shade_mod = 0;
      v->scr_off = 0;
      v->screen_translated = 2;
      v->oow = 1.0f;
      v->w   = 1.0f;

      if (textured)
      {
         v->ou  = ((int16_t*)addr)[4^1];
         v->ov  = ((int16_t*)addr)[5^1];
         v->w   = ZSort_Calc_invw(((int*)addr)[3]) / 31.0f;
         v->oow = 1.0f / v->w;
      }
      addr += vsize;
   }

   pV[0] = &vtx[0];
   pV[1] = &vtx[1];
   pV[2] = &vtx[2];
   pV[3] = &vtx[3];

   cull_trianglefaces(pV, 1, false, false, 0); 

   if (vnum != 3)
      cull_trianglefaces(pV + 1, 1, false, false, 0); 
}

static uint32_t uc9_load_object (uint32_t zHeader, uint32_t * rdpcmds)
{
   uint32_t w0   = __RSP.w0;
   uint32_t w1   = __RSP.w1;
   uint32_t type = zHeader & 7;
   uint8_t *addr = gfx_info.RDRAM + (zHeader&0xFFFFFFF8);

   switch (type)
   {
      case ZH_SHTRI:
      case ZH_SHQUAD:
         __RSP.w1 = ((uint32_t*)addr)[1];
         if (__RSP.w1 != rdpcmds[0])
         {
            rdpcmds[0] = __RSP.w1;
            uc9_rpdcmd(w0, w1);
         }
         update();
         uc9_draw_object(addr + 8, type);
         break;
      case ZH_NULL:
      case ZH_TXTRI:
      case ZH_TXQUAD:
         __RSP.w1 = ((uint32_t*)addr)[1];
         if (__RSP.w1 != rdpcmds[0])
         {
            rdpcmds[0] = __RSP.w1;
            uc9_rpdcmd(w0, w1);
         }
         __RSP.w1 = ((uint32_t*)addr)[2];
         if (__RSP.w1 != rdpcmds[1])
         {
            uc9_rpdcmd(w0, w1);
            rdpcmds[1] = __RSP.w1;
         }
         __RSP.w1 = ((uint32_t*)addr)[3];
         if (__RSP.w1 != rdpcmds[2])
         {
            uc9_rpdcmd(w0, w1);
            rdpcmds[2] = __RSP.w1;
         }
         if (type)
         {
            update();
            uc9_draw_object(addr + 16, type);
         }
         break;
   }
   return RSP_SegmentToPhysical(((uint32_t*)addr)[0]);
}

static void uc9_object(uint32_t w0, uint32_t w1)
{
   uint32_t cmd1, zHeader;
   uint32_t rdpcmds[3];

   rdpcmds[0] = 0;
   rdpcmds[1] = 0;
   rdpcmds[2] = 0;

   cmd1    = w1;
   zHeader = RSP_SegmentToPhysical(w0);

   while (zHeader)
      zHeader = uc9_load_object(zHeader, rdpcmds);
   zHeader = RSP_SegmentToPhysical(cmd1);
   while (zHeader)
      zHeader = uc9_load_object(zHeader, rdpcmds);
}

static void uc9_mix(uint32_t w0, uint32_t w1)
{
}

static void uc9_fmlight(uint32_t w0, uint32_t w1)
{
   uint32_t i;
   M44 *m;
   int    mid = w0 & 0xFF;
   uint32_t a = -1024 + (w1 & 0xFFF);

   glide64gSPNumLights(1 + _SHIFTR(w1, 12, 8));

   switch (mid)
   {
      case 4:
         m = (M44*)rdp.model;
         break;
      case 6:
         m = (M44*)rdp.proj;
         break;
      case 8:
         m = (M44*)rdp.combined;
         break;
   }

   rdp.light[gSP.numLights].col[0] = (float)(((uint8_t*)gfx_info.DMEM)[(a+0)^3]) / 255.0f;
   rdp.light[gSP.numLights].col[1] = (float)(((uint8_t*)gfx_info.DMEM)[(a+1)^3]) / 255.0f;
   rdp.light[gSP.numLights].col[2] = (float)(((uint8_t*)gfx_info.DMEM)[(a+2)^3]) / 255.0f;
   rdp.light[gSP.numLights].col[3] = 1.0f;
   //FRDP ("ambient light: r: %.3f, g: %.3f, b: %.3f\n", rdp.light[gSP.numLights].r, rdp.light[gSP.numLights].g, rdp.light[gSP.numLights].b);
   a += 8;

   for (i = 0; i < gSP.numLights; i++)
   {
      rdp.light[i].col[0] = (float)(((uint8_t*)gfx_info.DMEM)[(a+0)^3]) / 255.0f;
      rdp.light[i].col[1] = (float)(((uint8_t*)gfx_info.DMEM)[(a+1)^3]) / 255.0f;
      rdp.light[i].col[2] = (float)(((uint8_t*)gfx_info.DMEM)[(a+2)^3]) / 255.0f;
      rdp.light[i].col[3] = 1.0f;
      rdp.light[i].dir[0] = (float)(((int8_t*)gfx_info.DMEM)[(a+8)^3]) / 127.0f;
      rdp.light[i].dir[1] = (float)(((int8_t*)gfx_info.DMEM)[(a+9)^3]) / 127.0f;
      rdp.light[i].dir[2] = (float)(((int8_t*)gfx_info.DMEM)[(a+10)^3]) / 127.0f;
      //FRDP ("light: n: %d, r: %.3f, g: %.3f, b: %.3f, x: %.3f, y: %.3f, z: %.3f\n", i, rdp.light[i].r, rdp.light[i].g, rdp.light[i].b, rdp.light[i].dir_x, rdp.light[i].dir_y, rdp.light[i].dir_z);
      InverseTransformVector(&rdp.light[i].dir[0], rdp.light_vector[i], *m);
      NormalizeVector (rdp.light_vector[i]);
      //FRDP ("light vector: n: %d, x: %.3f, y: %.3f, z: %.3f\n", i, rdp.light_vector[i][0], rdp.light_vector[i][1], rdp.light_vector[i][2]);
      a += 24;
   }

   for (i = 0; i < 2; i++)
   {
      float dir_x = (float)(((int8_t*)gfx_info.DMEM)[(a+8)^3]) / 127.0f;
      float dir_y = (float)(((int8_t*)gfx_info.DMEM)[(a+9)^3]) / 127.0f;
      float dir_z = (float)(((int8_t*)gfx_info.DMEM)[(a+10)^3]) / 127.0f;
      if (sqrt(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z) < 0.98)
      {
         gSP.lookatEnable = false;
         return;
      }
      rdp.lookat[i][0] = dir_x;
      rdp.lookat[i][1] = dir_y;
      rdp.lookat[i][2] = dir_z;
      a += 24;
   }
   gSP.lookatEnable = true;
}

static void uc9_light(uint32_t w0, uint32_t w1)
{
   VERTEX v;
   uint32_t i;
   uint32_t csrs    = -1024 + ((w0 >> 12) & 0xFFF);
   uint32_t nsrs    = -1024 + (w0 & 0xFFF);
   uint32_t num     = 1 + ((w1 >> 24) & 0xFF);
   uint32_t cdest   = -1024 + ((w1 >> 12) & 0xFFF);
   uint32_t tdest   = -1024 + (w1 & 0xFFF);
   int use_material = (csrs != 0x0ff0);
   tdest >>= 1;

   for (i = 0; i < num; i++)
   {
      v.vec[0] = ((int8_t*)gfx_info.DMEM)[(nsrs++)^3];
      v.vec[1] = ((int8_t*)gfx_info.DMEM)[(nsrs++)^3];
      v.vec[2] = ((int8_t*)gfx_info.DMEM)[(nsrs++)^3];

      calc_sphere (&v);

      NormalizeVector (v.vec);
      glide64gSPLightVertex(&v);

      v.a = 0xFF;

      if (use_material)
      {
         v.r = (uint8_t)(((uint32_t)v.r * gfx_info.DMEM[(csrs++)^3]) >> 8);
         v.g = (uint8_t)(((uint32_t)v.g * gfx_info.DMEM[(csrs++)^3]) >> 8);
         v.b = (uint8_t)(((uint32_t)v.b * gfx_info.DMEM[(csrs++)^3]) >> 8);
         v.a = gfx_info.DMEM[(csrs++)^3];
      }
      gfx_info.DMEM[(cdest++)^3] = v.r;
      gfx_info.DMEM[(cdest++)^3] = v.g;
      gfx_info.DMEM[(cdest++)^3] = v.b;
      gfx_info.DMEM[(cdest++)^3] = v.a;
      ((int16_t*)gfx_info.DMEM)[(tdest++)^1] = (int16_t)v.ou;
      ((int16_t*)gfx_info.DMEM)[(tdest++)^1] = (int16_t)v.ov;
   }
}

static void uc9_mtxtrnsp(uint32_t w0, uint32_t w1)
{
   /*
   M44 *s;
   switch (w1 & 0xF)
   {
      case 4:
         s = (M44*)rdp.model;
         LRDP("Model\n");
         break;
      case 6:
         s = (M44*)rdp.proj;
         LRDP("Proj\n");
         break;
      case 8:
         s = (M44*)rdp.combined;
         LRDP("Comb\n");
         break;
    }
    float m = *s[1][0];
    *s[1][0] = *s[0][1];
    *s[0][1] = m;
    m = *s[2][0];
    *s[2][0] = *s[0][2];
    *s[0][2] = m;
    m = *s[2][1];
    *s[2][1] = *s[1][2];
    *s[1][2] = m;
    */
}

static void uc9_mtxcat(uint32_t w0, uint32_t w1)
{
   M44 *s;
   M44 *t;
   DECLAREALIGN16VAR(m[4][4]);
   uint32_t S = w0 & 0xF;
   uint32_t T = (w1 >> 16) & 0xF;
   uint32_t D = w1 & 0xF;

   switch (S)
   {
      case 4:
         s = (M44*)rdp.model;
         LRDP("Model * ");
         break;
      case 6:
         s = (M44*)rdp.proj;
         LRDP("Proj * ");
         break;
      case 8:
         s = (M44*)rdp.combined;
         LRDP("Comb * ");
         break;
   }

   switch (T)
   {
      case 4:
         t = (M44*)rdp.model;
         LRDP("Model -> ");
         break;
      case 6:
         t = (M44*)rdp.proj;
         LRDP("Proj -> ");
         break;
      case 8:
         LRDP("Comb -> ");
         t = (M44*)rdp.combined;
         break;
   }
   MulMatrices(*s, *t, m);

   switch (D)
   {
      case 4:
         CopyMatrix(rdp.model, m);
         break;
      case 6:
         CopyMatrix(rdp.proj, m);
         break;
      case 8:
         CopyMatrix(rdp.combined, m);
         break;
   }
}

typedef struct
{
   int16_t sy;
   int16_t sx;
   int   invw;
   int16_t yi;
   int16_t xi;
   int16_t wi;
   uint8_t fog;
   uint8_t cc;
} zSortVDest;

static void uc9_mult_mpmtx(uint32_t w0, uint32_t w1)
{
   int i, idx, num, src, dst;
   int16_t *saddr;
   zSortVDest v, *daddr;
   num = 1+ ((w1 >> 24) & 0xFF);
   src = -1024 + ((w1 >> 12) & 0xFFF);
   dst = -1024 + (w1 & 0xFFF);
   FRDP ("uc9:mult_mpmtx from: %04lx  to: %04lx n: %d\n", src, dst, num);
   saddr = (int16_t*)(gfx_info.DMEM+src);
   daddr = (zSortVDest*)(gfx_info.DMEM+dst);
   idx = 0;
   memset(&v, 0, sizeof(zSortVDest));
   //float scale_x = 4.0f/rdp.scale_x;
   //float scale_y = 4.0f/rdp.scale_y;
   for (i = 0; i < num; i++)
   {
      int16_t sx   = saddr[(idx++)^1];
      int16_t sy   = saddr[(idx++)^1];
      int16_t sz   = saddr[(idx++)^1];
      float x = sx*rdp.combined[0][0] + sy*rdp.combined[1][0] + sz*rdp.combined[2][0] + rdp.combined[3][0];
      float y = sx*rdp.combined[0][1] + sy*rdp.combined[1][1] + sz*rdp.combined[2][1] + rdp.combined[3][1];
      float z = sx*rdp.combined[0][2] + sy*rdp.combined[1][2] + sz*rdp.combined[2][2] + rdp.combined[3][2];
      float w = sx*rdp.combined[0][3] + sy*rdp.combined[1][3] + sz*rdp.combined[2][3] + rdp.combined[3][3];
      v.sx    = (int16_t)(zSortRdp.view_trans[0] + x / w * zSortRdp.view_scale[0]);
      v.sy    = (int16_t)(zSortRdp.view_trans[1] + y / w * zSortRdp.view_scale[1]);

      v.xi    = (int16_t)x;
      v.yi    = (int16_t)y;
      v.wi    = (int16_t)w;
      v.invw  = ZSort_Calc_invw((int)(w * 31.0));

      if (w < 0.0f)
         v.fog = 0;
      else
      {
         int fog = (int)(z / w * gSP.fog.multiplier + gSP.fog.offset);
         if (fog > 255)
            fog = 255;
         v.fog = (fog >= 0) ? (uint8_t)fog : 0;
      }

      v.cc = 0;
      if (x < -w)
         v.cc |= Z_CLIP_MAX;
      if (x > w)
         v.cc |= X_CLIP_MAX;
      if (y < -w)
         v.cc |= Z_CLIP_MIN;
      if (y > w)
         v.cc |= X_CLIP_MIN;
      if (w < 0.1f)
         v.cc |= Y_CLIP_MAX;

      daddr[i] = v;
   }
}

static void uc9_link_subdl(uint32_t w0, uint32_t w1)
{
}

static void uc9_set_subdl(uint32_t w0, uint32_t w1)
{
}

static void uc9_wait_signal(uint32_t w0, uint32_t w1)
{
}

static void uc9_send_signal(uint32_t w0, uint32_t w1)
{
}

static void uc9_movemem(uint32_t w0, uint32_t w1)
{
   int idx       = w0 & 0x0E;
   int ofs       = ((w0 >> 6) & 0x1ff)<<3;
   int len       = (1 + ((w0 >> 15) & 0x1ff))<<3;
   int flag      = w0 & 0x01;
   uint32_t addr = RSP_SegmentToPhysical(w1);

   switch (idx)
   {
      case GZF_LOAD:
         if (flag == 0)
         {
            int dmem_addr = (idx<<3) + ofs;
            memcpy(gfx_info.DMEM + dmem_addr, gfx_info.RDRAM + addr, len);
         }
         else
         {
            int dmem_addr = (idx<<3) + ofs;
            memcpy(gfx_info.RDRAM + addr, gfx_info.DMEM + dmem_addr, len);
         }
         break;

      case GZM_MMTX:
      case GZM_PMTX:
      case GZM_MPMTX:
         {
            DECLAREALIGN16VAR(m[4][4]);
            load_matrix(m, addr);
            switch (idx)
            {
               case GZM_MMTX:
                  modelview_load (m);
                  break;
               case GZM_PMTX:
                  projection_load (m);
                  break;
               case GZM_MPMTX:
                  LRDP("Combined load\n");
                  g_gdp.flags &= ~UPDATE_MULT_MAT;
                  CopyMatrix(rdp.combined, m);
                  break;
            }
         }
         break;

      case GZM_OTHERMODE:
         LRDP("Othermode - IGNORED\n");
         break;

      case GZM_VIEWPORT:
         {
            uint32_t      a = addr >> 1;
            int16_t scale_x = ((int16_t*)gfx_info.RDRAM)[(a+0)^1] >> 2;
            int16_t scale_y = ((int16_t*)gfx_info.RDRAM)[(a+1)^1] >> 2;
            int16_t scale_z = ((int16_t*)gfx_info.RDRAM)[(a+2)^1];
            int16_t trans_x = ((int16_t*)gfx_info.RDRAM)[(a+4)^1] >> 2;
            int16_t trans_y = ((int16_t*)gfx_info.RDRAM)[(a+5)^1] >> 2;
            int16_t trans_z = ((int16_t*)gfx_info.RDRAM)[(a+6)^1];

            glide64gSPFogFactor(
                  ((int16_t*)gfx_info.RDRAM)[(a+3)^1],   /* fm */
                  ((int16_t*)gfx_info.RDRAM)[(a+7)^1]    /* fo */
                  );

            gSP.viewport.vscale[0] = scale_x * rdp.scale_x;
            gSP.viewport.vscale[1] = scale_y * rdp.scale_y;
            gSP.viewport.vscale[2] = 32.0f * scale_z;
            gSP.viewport.vtrans[0] = trans_x * rdp.scale_x;
            gSP.viewport.vtrans[1] = trans_y * rdp.scale_y;
            gSP.viewport.vtrans[2] = 32.0f * trans_z;

            zSortRdp.view_scale[0] = (float)(scale_x*4);
            zSortRdp.view_scale[1] = (float)(scale_y*4);
            zSortRdp.view_trans[0] = (float)(trans_x*4);
            zSortRdp.view_trans[1] = (float)(trans_y*4);
            zSortRdp.scale_x = rdp.scale_x / 4.0f;
            zSortRdp.scale_y = rdp.scale_y / 4.0f;

            g_gdp.flags |= UPDATE_VIEWPORT;

            glide64gSPTexture(
                  0xFFFF,     /* sc */
                  0xFFFF,     /* tc */
                  0,          /* level */
                  0,          /* tile  */
                  1           /* on */
                  );

            glide64gSPSetGeometryMode(0x0200);

            FRDP ("viewport scale(%d, %d, %d), trans(%d, %d, %d), from:%08lx\n", scale_x, scale_y, scale_z,
                  trans_x, trans_y, trans_z, a);
            FRDP ("fog: multiplier: %f, offset: %f\n", gSP.fog.multiplier, gSP.fog.offset);
         }
         break;

      default:
         FRDP ("** UNKNOWN %d\n", idx);
   }
}

static void uc9_setscissor(uint32_t w0, uint32_t w1)
{
   rdp_setscissor(w0, w1);

   if ((g_gdp.__clip.xl - g_gdp.__clip.xh) > (zSortRdp.view_scale[0] - zSortRdp.view_trans[0]))
   {
      float w = (g_gdp.__clip.xl - g_gdp.__clip.xh) / 2.0f;
      float h = (g_gdp.__clip.yl - g_gdp.__clip.yh) / 2.0f;

      gSP.viewport.vscale[0] = w * rdp.scale_x;
      gSP.viewport.vscale[1] = h * rdp.scale_y;
      gSP.viewport.vtrans[0] = w * rdp.scale_x;
      gSP.viewport.vtrans[1] = h * rdp.scale_y;

      zSortRdp.view_scale[0] = w * 4.0f;
      zSortRdp.view_scale[1] = h * 4.0f;
      zSortRdp.view_trans[0] = w * 4.0f;
      zSortRdp.view_trans[1] = h * 4.0f;
      zSortRdp.scale_x = rdp.scale_x / 4.0f;
      zSortRdp.scale_y = rdp.scale_y / 4.0f;

      g_gdp.flags |= UPDATE_VIEWPORT;

      glide64gSPTexture(
            0xFFFF,     /* sc */
            0xFFFF,     /* tc */
            0,          /* level */
            0,          /* tile  */
            1           /* on */
            );

      glide64gSPSetGeometryMode(0x0200);
   }
}
