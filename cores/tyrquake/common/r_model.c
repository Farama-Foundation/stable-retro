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

/* Shared model functions for renderers */

#include "model.h"

/*
 * Find the correct interval based on time
 * Used for Alias model frame/sprite/skin group animations
 */
int Mod_FindInterval(const float *intervals, int numintervals, double time)
{
   int i;
   /*
    * when loading models/skins/sprites, we guaranteed all interval values
    * are positive, so we don't have to worry about division by 0
    *
    * Compute (time mod fullinterval) in double so cl.time-derived inputs
    * keep full precision regardless of how long the engine has been
    * running, then cast the bounded result (< fullinterval) to float for
    * the comparison loop against the (small) interval table.
    */
   float fullinterval = intervals[numintervals - 1];
   float targettime   = (float)(time - (int)(time / fullinterval) * fullinterval);

   for (i = 0; i < numintervals - 1; i++)
      if (intervals[i] > targettime)
         break;

   return i;
}
