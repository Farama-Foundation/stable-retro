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
// January 2004 Created by Gonetz (Gonetz@ngs.ru)
//
//****************************************************************

static void uc8_vertex(uint32_t w0, uint32_t w1)
{
   uint32_t n = _SHIFTR(w0, 12, 8);

   glide64gSPCBFDVertex(w1, n, _SHIFTR(w0, 1, 7) - n);
}

static void uc8_moveword(uint32_t w0, uint32_t w1)
{
   int k;
   uint8_t index   = (uint8_t)((w0 >> 16) & 0xFF);
   uint16_t offset = (uint16_t)(w0 & 0xFFFF);

   switch (index)
   {
      // NOTE: right now it's assuming that it sets the integer part first.  This could
      //  be easily fixed, but only if i had something to test with.

      case G_MW_NUMLIGHT:
         glide64gSPNumLights(w1 / 48);
         break;

      case G_MW_CLIP:
         if (offset == 0x04)
            glide64gSPClipRatio(w1);
         break;

      case G_MW_SEGMENT:
         glide64gSPSegment((offset >> 2) & 0xF, w1);
         break;

      case G_MW_FOG:
         glide64gSPFogFactor((int16_t)_SHIFTR(w1, 16, 16), (int16_t)_SHIFTR(w1, 0, 16));
         break;

      case G_MW_PERSPNORM:
         break;

      case G_MV_COORDMOD:  // moveword coord mod
         glide64gSPCoordMod(w0, w1);
         break;
   }
}

static void uc8_movemem(uint32_t w0, uint32_t w1)
{
   int i, t;
   uint32_t addr = RSP_SegmentToPhysical(w1);
   int ofs       = _SHIFTR(w0, 5, 14);

   switch (_SHIFTR(w0, 0, 8))
   {
      case F3DCBFD_MV_VIEWPORT:
         gSPViewport(w1);
         break;

      case F3DCBFD_MV_LIGHT:  // LIGHT
         {
            int n = (ofs / 48);

            if (n < 2)
               gSPLookAt(w1, n);
            else
               glide64gSPLightCBFD(w1, n - 2);
         }
         break;

      case F3DCBFD_MV_NORMAL: //Normals
         glide64gSPSetVertexNormaleBase(w1);
         break;
   }
}

static void uc8_tri4(uint32_t w0, uint32_t w1)
{
	glide64gSP4Triangles( _SHIFTR( w0, 23, 5 ), _SHIFTR( w0, 18, 5 ), (_SHIFTR( w0, 15, 3 )<<2)|_SHIFTR( w1, 30, 2 ),
				   _SHIFTR( w0, 10, 5 ), _SHIFTR( w0,  5, 5 ), _SHIFTR( w0,  0, 5 ),
				   _SHIFTR( w1, 25, 5 ), _SHIFTR( w1, 20, 5 ), _SHIFTR( w1, 15, 5 ),
				   _SHIFTR( w1, 10, 5 ), _SHIFTR( w1,  5, 5 ), _SHIFTR( w1,  0, 5 ));
}
