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
extern int vtx_last;

static void uc5_dma_offsets(uint32_t w0, uint32_t w1)
{
   glide64gSPSetDMAOffsets(_SHIFTR( w0, 0, 24), _SHIFTR( w1, 0, 24));
}

static void uc5_matrix(uint32_t w0, uint32_t w1)
{
   uint8_t multiply = 0;
   uint8_t index    = _SHIFTR(w0, 16, 4);

   if (index == 0) /* Diddy Kong Racing */
      index = _SHIFTR( w0, 22, 2 );
   else            /* Jet Force Gemini */
      multiply = _SHIFTR( w0, 23, 1 );

   glide64gSPDMAMatrix(w1, index, multiply);

}

static void uc5_vertex(uint32_t w0, uint32_t w1)
{
   uint32_t n;

   if (w0 & G_FOG)
   {
      if (gSP.matrix.billboard)
         vtx_last = 1;
   }
   else
      vtx_last = 0;

   n = _SHIFTR( w0, 19, 5);
   if (settings.hacks&hack_Diddy)
      n++;

   glide64gSPDMAVertex(w1, n, vtx_last + _SHIFTR(w0, 9, 5));

   vtx_last += n;
}

static void uc5_tridma(uint32_t w0, uint32_t w1)
{
   glide64gSPDMATriangles(w1, _SHIFTR( w0, 4, 12));
}

static void uc5_dl_in_mem(uint32_t w0, uint32_t w1)
{
	glide64gSPDlistCount(_SHIFTR(w0, 16, 8), w1);
}

static void uc5_moveword(uint32_t w0, uint32_t w1)
{
   switch (_SHIFTR( w0, 0, 8))
   {
      case 0x02:
         gSP.matrix.billboard = w1 & 1;
         break;
      case G_MW_CLIP:
         if (((__RSP.w0 >> 8)&0xFFFF) == 0x04)
            glide64gSPClipRatio( w1 );
         break;
      case G_MW_SEGMENT:
         glide64gSPSegment((w0 >> 10) & 0x0F, w1);
         break;
      case G_MW_FOG:
         glide64gSPFogFactor( (int16_t)_SHIFTR( w1, 16, 16 ), (int16_t)_SHIFTR( w1, 0, 16 ) );
         break;

      case 0x0a:  // moveword matrix select
         gSP.matrix.modelViewi = _SHIFTR( w1, 6, 2);
         FRDP ("matrix select - mtx: %d\n", gSP.matrix.modelViewi);
         break;
   }
}

static void uc5_setgeometrymode(uint32_t w0, uint32_t w1)
{
   glide64gSPSetGeometryMode(w1);

   if (w1 & G_ZBUFFER)
   {
      if (!(rdp.flags & ZBUF_ENABLED))
      {
         rdp.flags |= ZBUF_ENABLED;
         g_gdp.flags |= UPDATE_ZBUF_ENABLED;
      }
   }

   if (w1 & G_FOG)
   {
      if (!(rdp.flags & FOG_ENABLED))
      {
         rdp.flags |= FOG_ENABLED;
         g_gdp.flags |= UPDATE_FOG_ENABLED;
      }
   }
}

static void uc5_cleargeometrymode(uint32_t w0, uint32_t w1)
{
   glide64gSPClearGeometryMode(w1);

   if (w1 & G_ZBUFFER)
   {
      if (rdp.flags & ZBUF_ENABLED)
      {
         rdp.flags ^= ZBUF_ENABLED;
         g_gdp.flags |= UPDATE_ZBUF_ENABLED;
      }
   }

   if (w1 & G_FOG)
   {
      if (rdp.flags & FOG_ENABLED)
      {
         rdp.flags ^= FOG_ENABLED;
         g_gdp.flags |= UPDATE_FOG_ENABLED;
      }
   }
}
