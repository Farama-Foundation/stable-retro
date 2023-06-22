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

/* Fast3DEX */

/* vertex - loads vertices */

static void uc1_vertex(uint32_t w0, uint32_t w1)
{
   glide64gSPVertex(
         RSP_SegmentToPhysical(__RSP.w1),       /* Address */
         (w0 >> 10) & 0x3F,                     /* Number of vertices to copy */
         (w0 >> 17) & 0x7F                      /* Current vertex */
         );
}

static void uc1_tri1(uint32_t w0, uint32_t w1)
{
   glide64gSP1Triangle( _SHIFTR( w1, 17, 7 ), _SHIFTR( w1, 9, 7 ), _SHIFTR( w1, 1, 7 ), 0 );
}

static void uc1_tri2(uint32_t w0, uint32_t w1)
{
   glide64gSP2Triangles( _SHIFTR( w0, 17, 7 ), _SHIFTR( w0, 9, 7 ), _SHIFTR( w0, 1, 7 ), 0,
         _SHIFTR( w1, 17, 7 ), _SHIFTR( w1, 9, 7 ), _SHIFTR( w1, 1, 7 ), 0);
}

static void uc1_line3d(uint32_t w0, uint32_t w1)
{
   if (!settings.force_quad3d && ((w1 & 0xFF000000) == 0) && ((w0 & 0x00FFFFFF) == 0))
   {
      uint32_t mode = (rdp.flags & G_CULL_BOTH) >> CULLSHIFT;
      rdp.flags    |= G_CULL_BOTH;
      g_gdp.flags  |= UPDATE_CULL_MODE;

      glide64gSP1Triangle(
            (w1 >> 17) & 0x7F,
            (w1 >> 9) & 0x7F,
            (w1 >> 9) & 0x7F,
            (uint16_t)(w1 & 0xFF) + 3);

      rdp.flags    ^= G_CULL_BOTH;
      rdp.flags    |= mode << CULLSHIFT;
      g_gdp.flags  |= UPDATE_CULL_MODE;

      return;
   }

   glide64gSP2Triangles(
         (w1 >> 25) & 0x7F,
         (w1 >> 17) & 0x7F,
         (w1 >> 9) & 0x7F,
         0,
         (w1 >> 1) & 0x7F,
         (w1 >> 25) & 0x7F,
         (w1 >> 9) & 0x7F,
         0);
}

static void uc1_rdphalf_1(uint32_t w0, uint32_t w1)
{
   gDP.half_1 = w1;
   rdphalf_1(w0, w1);
}

static void uc1_branch_z(uint32_t w0, uint32_t w1)
{
   uint32_t branchdl    = gDP.half_1;
   uint32_t address     = RSP_SegmentToPhysical(branchdl);
   uint32_t vtx         = (w0 >> 1) & 0x7FF;
   const uint32_t zTest = (uint32_t)((rdp.vtx[vtx].z / rdp.vtx[vtx].w) * 1023.0f);
   if (zTest > 0x03FF || zTest <= w1)
      __RSP.PC[__RSP.PCi] = address;
}
