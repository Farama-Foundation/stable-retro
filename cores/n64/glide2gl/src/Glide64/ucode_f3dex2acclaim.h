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
static void glide64gSPLightAcclaim(uint32_t l, int32_t n)
{
   int16_t *rdram     = (int16_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   uint8_t *rdram_u8  = (uint8_t*)(gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   int8_t  *rdram_s8  = (int8_t*) (gfx_info.RDRAM  + RSP_SegmentToPhysical(l));
   const uint32_t addrByte = RSP_SegmentToPhysical(l);

   --n;

   if (n < 10)
   {
      const uint32_t addrShort = addrByte >> 1;

      rdp.light[n].x         = (float)rdram[(addrShort + 0) ^ 1];
      rdp.light[n].y         = (float)rdram[(addrShort + 1) ^ 1];
      rdp.light[n].z         = (float)rdram[(addrShort + 2) ^ 1];
      rdp.light[n].ca        = (float)rdram[(addrShort + 5) ^ 1];
      rdp.light[n].la        = (float)rdram[(addrShort + 6) ^ 1] / 16.0f;
      rdp.light[n].qa        = (float)rdram[(addrShort + 7) ^ 1];

      rdp.light[n].col[0]    = rdram_u8[(addrByte + 6) ^ 3] / 255.0f;
      rdp.light[n].col[1]    = rdram_u8[(addrByte + 7) ^ 3] / 255.0f;
      rdp.light[n].col[2]    = rdram_u8[(addrByte + 8) ^ 3] / 255.0f;
      rdp.light[n].col[3]    = 1.0f;

   }

   //g_gdp.flags |= UPDATE_LIGHTS;
}

static void F3DEX2ACCLAIM_MoveMem(uint32_t w0, uint32_t w1)
{
   switch (_SHIFTR(w0, 0, 8))
   {
      case 0:
      case 2:
         uc6_obj_movemem(w0, w1);
         break;
      case F3DEX2_MV_VIEWPORT:
         gSPViewport( w1 );
         break;
      case G_MV_MATRIX:
         glide64gSPForceMatrix(w1);

         /* force matrix takes two commands */
         __RSP.PC[__RSP.PCi] += 8; 
         break;

      case G_MV_LIGHT:
         {
            const uint32_t offset = (_SHIFTR( w0, 5, 11)) & 0x7F8;
            if (offset <= 24 * 3)
            {
               uint32_t n = offset / 24;
               if (n < 2)
                  gSPLookAt(w1, n);
               else
                  gSPLight(w1, n - 1);
            }
            else
            {
               const uint32_t n = 2 + (offset - 24 * 4) / 16;
               glide64gSPLightAcclaim(w1, n);
            }

         }
         break;

   }
}

void glide64gSPPointLightVertex_Acclaim(void *data)
{
   uint32_t l;
   VERTEX *v      = (VERTEX*)data;

   for (l = 2; l < 10; l++)
   {
      if (rdp.light[l].ca < 0)
         continue;

      {
         const float dX              = fabsf(rdp.light[l].x - v->x);
         const float dY              = fabsf(rdp.light[l].y - v->y);
         const float dZ              = fabsf(rdp.light[l].z - v->z);
         const float distance        = dX + dY + dZ - rdp.light[l].ca;
         const float light_intensity = -distance * rdp.light[l].la;
         if (distance >= 0.0f)
            continue;

         if (light_intensity > 0.0f)
         {
            v->r += rdp.light[l].col[0] * light_intensity;
            v->g += rdp.light[l].col[1] * light_intensity;
            v->b += rdp.light[l].col[2] * light_intensity;
         }
      }
   }

   if (v->r > 1.0f) v->r = 1.0f;
   if (v->g > 1.0f) v->g = 1.0f;
   if (v->b > 1.0f) v->b = 1.0f;
}

