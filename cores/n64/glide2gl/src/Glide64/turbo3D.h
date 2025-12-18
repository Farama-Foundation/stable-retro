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
// Created by Gonetz, 2008
//
//****************************************************************
#include "../../Graphics/GBI.h"
#include "../../Graphics/HLE/Microcode/turbo3d.h"

/******************Turbo3D microcode*************************/

static void t3dProcessRDP(uint32_t cmds)
{
   if (cmds)
   {
      __RSP.bLLE  = 1;
      __RSP.w0    = ((uint32_t*)gfx_info.RDRAM)[cmds++];
      __RSP.w1    = ((uint32_t*)gfx_info.RDRAM)[cmds++];
      while (__RSP.w0 + __RSP.w1)
      {
         uint32_t cmd;
         gfx_instruction[0][__RSP.w0 >> 24](__RSP.w0, __RSP.w1);
         __RSP.w0 = ((uint32_t*)gfx_info.RDRAM)[cmds++];
         __RSP.w1 = ((uint32_t*)gfx_info.RDRAM)[cmds++];
         cmd      = __RSP.w0 >> 24;

         if (cmd == G_TEXRECT || cmd == G_TEXRECTFLIP)
         {
            __RDP.w2 = ((uint32_t*)gfx_info.RDRAM)[cmds++];
            __RDP.w3 = ((uint32_t*)gfx_info.RDRAM)[cmds++];
         }
      }
      __RSP.bLLE = 0;
   }
}

static void t3dLoadGlobState(uint32_t pgstate)
{
   unsigned s;
   struct T3DGlobState *gstate = (struct T3DGlobState*)&gfx_info.RDRAM[RSP_SegmentToPhysical(pgstate)];
   const uint32_t w0           = gstate->othermode0;
   const uint32_t w1           = gstate->othermode1;

   rdp_setothermode(w0, w1);

   for (s = 0; s < 16; s++)
      glide64gSPSegment(s, gstate->segBases[s]);

   gSPViewport(pgstate + 80);

   t3dProcessRDP(RSP_SegmentToPhysical(gstate->rdpCmds) >> 2);
}

static void t3d_vertex(uint32_t addr, uint32_t v0, uint32_t n)
{
   uint32_t i;
   float x, y, z;

   n <<= 4;

   for (i = 0; i < n; i+=16)
   {
      VERTEX *v    = &rdp.vtx[v0 + (i>>4)];
      x            = (float)((int16_t*)gfx_info.RDRAM)[(((addr+i) >> 1) + 0)^1];
      y            = (float)((int16_t*)gfx_info.RDRAM)[(((addr+i) >> 1) + 1)^1];
      z            = (float)((int16_t*)gfx_info.RDRAM)[(((addr+i) >> 1) + 2)^1];
      v->flags     = ((uint16_t*)gfx_info.RDRAM)[(((addr+i) >> 1) + 3)^1];
      v->ou        = 2.0f * (float)((int16_t*)gfx_info.RDRAM)[(((addr+i) >> 1) + 4)^1];
      v->ov        = 2.0f * (float)((int16_t*)gfx_info.RDRAM)[(((addr+i) >> 1) + 5)^1];
      v->uv_scaled = 0;
      v->r         = ((uint8_t*)gfx_info.RDRAM)[(addr+i + 12)^3];
      v->g         = ((uint8_t*)gfx_info.RDRAM)[(addr+i + 13)^3];
      v->b         = ((uint8_t*)gfx_info.RDRAM)[(addr+i + 14)^3];
      v->a         = ((uint8_t*)gfx_info.RDRAM)[(addr+i + 15)^3];

      v->x         = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
      v->y         = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
      v->z         = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
      v->w         = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];

      if (fabs(v->w) < 0.001)
         v->w              = 0.001f;
      v->oow               = 1.0f / v->w;
      v->x_w               = v->x * v->oow;
      v->y_w               = v->y * v->oow;
      v->z_w               = v->z * v->oow;

      v->uv_calculated     = 0xFFFFFFFF;
      v->screen_translated = 0;
      v->shade_mod         = 0;

      v->scr_off = 0;
      if (v->x < -v->w)
         v->scr_off |= X_CLIP_MAX;
      if (v->x > v->w)
         v->scr_off |= X_CLIP_MIN;
      if (v->y < -v->w)
         v->scr_off |= Y_CLIP_MAX;
      if (v->y > v->w)
         v->scr_off |= Y_CLIP_MIN;
      if (v->w < 0.1f)
         v->scr_off |= Z_CLIP_MAX;
   }
}

static void t3dLoadObject(uint32_t pstate, uint32_t pvtx, uint32_t ptri)
{
   int t;
   struct T3DState *ostate = (struct T3DState*)&gfx_info.RDRAM[RSP_SegmentToPhysical(pstate)];
   rdp.cur_tile = (ostate->textureState)&7;

   if (gSP.texture.scales < 0.001f)
      gSP.texture.scales = 0.015625;
   if (gSP.texture.scalet < 0.001f)
      gSP.texture.scalet = 0.015625;

   __RSP.w0 = ostate->othermode0;
   __RSP.w1 = ostate->othermode1;
   rdp_setothermode(__RSP.w0, __RSP.w1);

   __RSP.w1 = ostate->renderState;
   uc0_setgeometrymode(__RSP.w0, __RSP.w1);

   if (!(ostate->flag&1)) //load matrix
   {
      uint32_t addr = RSP_SegmentToPhysical(pstate+sizeof(struct T3DState)) & BMASK;
      load_matrix(rdp.combined, addr);
   }

   gSP.geometryMode &= ~G_LIGHTING;

   glide64gSPSetGeometryMode(UPDATE_SCISSOR);

   if (pvtx) /* load vtx */
      t3d_vertex(RSP_SegmentToPhysical(pvtx), ostate->vtxV0, ostate->vtxCount);

   t3dProcessRDP(RSP_SegmentToPhysical(ostate->rdpCmds) >> 2);

   if (ptri)
   {
      uint32_t addr = RSP_SegmentToPhysical(ptri);

      update();

      for (t = 0; t < ostate->triCount; t++)
      {
         struct T3DTriN *tri = (struct T3DTriN*)&gfx_info.RDRAM[addr];
         addr += 4;
			glide64gSP1Triangle(tri->v0, tri->v1, tri->v2, 0);
      }
   }
}

static void Turbo3D(void)
{
   settings.ucode = ucode_Fast3D;

   do
   {
      uint32_t          a = __RSP.PC[__RSP.PCi] & BMASK;
      uint32_t    pgstate = ((uint32_t*)gfx_info.RDRAM)[a>>2];
      uint32_t       ptri = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+3];
      uint32_t       pvtx = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+2];

      uint32_t     pstate = ((uint32_t*)gfx_info.RDRAM)[(a>>2)+1];

      if (!pstate)
      {
         __RSP.halt = 1;
         break;
      }

      if (pgstate)
         t3dLoadGlobState(pgstate);
      t3dLoadObject(pstate, pvtx, ptri);

      // Go to the next instruction
      __RSP.PC[__RSP.PCi] += 16;
   }while(1);

   // rdp_fullsync();
   
   settings.ucode = ucode_Turbo3d;
}
