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
// vertex - loads vertices
//

static void uc3_vertex(uint32_t w0, uint32_t w1)
{
   int v0 = _SHIFTR(w0, 16, 8) / 5;      // Current vertex
   int n  = (uint16_t)((w0 & 0xFFFF) + 1) / 0x210;    // Number to copy

   if (v0 >= 32)
      v0 = 31;

   if ((v0 + n) > 32)
      n = 32 - v0;

   glide64gSPVertex(
         RSP_SegmentToPhysical(w1),        /* v - Current vertex */
         n,                                /* n - Number of vertices to copy */
         v0                                /* v0 */
         );
}

static void uc3_tri1(uint32_t w0, uint32_t w1)
{
	glide64gSP1Triangle( _SHIFTR( w1, 16, 8 ) / 5,
				  _SHIFTR( w1, 8, 8 ) / 5,
				  _SHIFTR( w1, 0, 8 ) / 5, 0);
}

static void uc3_tri2(uint32_t w0, uint32_t w1)
{
	glide64gSP2Triangles(
         _SHIFTR( w0, 16, 8 ) / 5,     /* v00 */
         _SHIFTR( w0, 8, 8 ) / 5,      /* v01 */
         _SHIFTR( w0, 0, 8 ) / 5,      /* v02 */
         0,                            /* flag0 */
         _SHIFTR( w1, 16, 8 ) / 5,     /* v10 */
         _SHIFTR( w1, 8, 8 ) / 5,      /* v11 */
         _SHIFTR( w1, 0, 8 ) / 5,      /* v12 */
         0
         );
}

static void uc3_quad3d(uint32_t w0, uint32_t w1)
{
   glide64gSP1Quadrangle(
         _SHIFTR( w1, 24, 8 ) / 5,     /* v0 */
         _SHIFTR( w1, 16, 8 ) / 5,     /* v1 */
         _SHIFTR( w1, 8, 8  ) / 5,     /* v2 */
         _SHIFTR( w1, 0, 8 )  / 5      /* v3 */
         );
}
