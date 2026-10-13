/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* d_sky.c */

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

#define SKY_SPAN_SHIFT	5
#define SKY_SPAN_MAX	(1 << SKY_SPAN_SHIFT)

static float   timespeed1, timespeed2; /* Manoel Kasimier - smooth sky */

byte *skyunderlay;
byte *skyoverlay;

/*
=================
D_Sky_uv_To_st
=================
*/
static void D_Sky_uv_To_st (int u, int v, fixed16_t *s, fixed16_t *t
               , fixed16_t *s2, fixed16_t *t2)
{
   vec3_t   end;

   /* ToChriS - begin */
   float wu = (u - xcenter) / xscale;
   float wv = (ycenter - v) / yscale;

   end[0] = vpn[0] + wu*vright[0] + wv*vup[0];
   end[1] = vpn[1] + wu*vright[1] + wv*vup[1];
   end[2] = vpn[2] + wu*vright[2] + wv*vup[2];
   /* ToChriS - end */
   end[2] *= 3;
   VectorNormalize (end);
   *s = (int)((timespeed1 + 6*(SKYSIZE/2-1)*end[0]) * 0x10000); /* Manoel Kasimier - smooth sky - edited */
   *t = (int)((timespeed1 + 6*(SKYSIZE/2-1)*end[1]) * 0x10000); /* Manoel Kasimier - smooth sky - edited */
   *s2 = (int)((timespeed2 + 6*(SKYSIZE/2-1)*end[0]) * 0x10000); /* Manoel Kasimier - smooth sky */
   *t2 = (int)((timespeed2 + 6*(SKYSIZE/2-1)*end[1]) * 0x10000); /* Manoel Kasimier - smooth sky */
}


static unsigned char D_DrawSkyPixelOpaque (unsigned char pixel1, unsigned char pixel2)
{
   return pixel2 ? pixel2 : pixel1;
}

/*
=================
D_DrawSkyScans8
=================
*/
void D_DrawSkyScans8 (espan_t *pspan)
{
   fixed16_t      s, t;
   fixed16_t sstep = 0;   /* keep compiler happy */
   fixed16_t tstep = 0;   /* ditto */

   timespeed1=skytime*skyspeed;
   timespeed2=timespeed1*2.0;

   do
   {
      fixed16_t      snext, tnext, s2, t2;
      uint8_t *pdest = (uint8_t*)((byte *)d_viewbuffer +
            (screenwidth * pspan->v) + pspan->u);
      int count      = pspan->count;

      /* calculate the initial s & t */
      int u = pspan->u;
      int v = pspan->v;

      D_Sky_uv_To_st (u, v, &s, &t, &s2, &t2); /* Manoel Kasimier - smooth sky - edited */

      do
      {
         int sstep2 = 0;
         int tstep2 = 0;
         fixed16_t snext2 = 0;
         fixed16_t tnext2 = 0;
         int spancount    = count;

         if (count >= SKY_SPAN_MAX)
            spancount = SKY_SPAN_MAX;

         count -= spancount;

         if (count)
         {
            u += spancount;

            /* calculate s and t at far end of span, */
            /* calculate s and t steps across span by shifting */
            D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); /* Manoel Kasimier - smooth sky - edited */

            sstep = (snext - s) >> SKY_SPAN_SHIFT;
            tstep = (tnext - t) >> SKY_SPAN_SHIFT;

            sstep2 = (snext2 - s2) >> SKY_SPAN_SHIFT; /* Manoel Kasimier - smooth sky */
            tstep2 = (tnext2 - t2) >> SKY_SPAN_SHIFT; /* Manoel Kasimier - smooth sky */

         }
         else
         {
            /* calculate s and t at last pixel in span, */
            /* calculate s and t steps across span by division */
            int spancountminus1 = (float)(spancount - 1);

            if (spancountminus1 > 0)
            {
               u += spancountminus1;
               D_Sky_uv_To_st (u, v, &snext, &tnext, &snext2, &tnext2); /* Manoel Kasimier - smooth sky - edited */

               sstep = (snext - s) / spancountminus1;
               tstep = (tnext - t) / spancountminus1;
               sstep2 = (snext2 - s2) / spancountminus1; /* Manoel Kasimier - smooth sky */
               tstep2 = (tnext2 - t2) / spancountminus1; /* Manoel Kasimier - smooth sky */
            }
         }

         do
         {
            /* Manoel Kasimier - smooth sky - begin */
            *pdest++ = D_DrawSkyPixelOpaque(skyunderlay[((t  & R_SKY_TMASK) >> 8) + ((s  & R_SKY_SMASK) >> 16)],
                  skyoverlay [((t2 & R_SKY_TMASK) >> 8) + ((s2 & R_SKY_SMASK) >> 16)]);
            /* Manoel Kasimier - smooth sky - end */
            s += sstep;
            t += tstep;
            s2 += sstep2; /* Manoel Kasimier - smooth sky */
            t2 += tstep2; /* Manoel Kasimier - smooth sky */
         } while (--spancount > 0);

         s = snext;
         t = tnext;
         s2 = snext2; /* Manoel Kasimier - smooth sky */
         t2 = tnext2; /* Manoel Kasimier - smooth sky */

      } while (count > 0);
   } while ((pspan = pspan->pnext) != NULL);
}
