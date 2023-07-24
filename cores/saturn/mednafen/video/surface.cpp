/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "../mednafen.h"
#include "surface.h"

MDFN_PixelFormat::MDFN_PixelFormat()
{
   bpp = 0;
   colorspace = 0;

   Rshift = 0;
   Gshift = 0;
   Bshift = 0;
   Ashift = 0;
}

MDFN_PixelFormat::MDFN_PixelFormat(const unsigned int p_colorspace, const uint8 p_rs, const uint8 p_gs, const uint8 p_bs, const uint8 p_as)
{
   bpp = 32;
   colorspace = p_colorspace;

   Rshift = p_rs;
   Gshift = p_gs;
   Bshift = p_bs;
   Ashift = p_as;
}

MDFN_Surface::MDFN_Surface()
{
   memset(&format, 0, sizeof(format));

   pixels = NULL;
   pitchinpix = 0;
   w = 0;
   h = 0;
}

MDFN_Surface::MDFN_Surface(void *const p_pixels, const uint32 p_width, const uint32 p_height, const uint32 p_pitchinpix, const MDFN_PixelFormat &nf)
{
   Init(p_pixels, p_width, p_height, p_pitchinpix, nf);
}

bool MDFN_Surface::Init(void *const p_pixels, const uint32 p_width, const uint32 p_height, const uint32 p_pitchinpix, const MDFN_PixelFormat &nf)
{
   void *rpix = NULL;
   assert(nf.bpp == 16 || nf.bpp == 32);

   format = nf;

   pixels = NULL;

   rpix = calloc(1, p_pitchinpix * p_height * (nf.bpp / 8));
   if(!rpix)
      return false;

   pixels = (uint32 *)rpix;

   w = p_width;
   h = p_height;

   pitchinpix = p_pitchinpix;

   return true;
}

// When we're converting, only convert the w*h area(AKA leave the last part of the line, pitch32 - w, alone),
// for places where we store auxillary information there(graphics viewer in the debugger), and it'll be faster
// to boot.
void MDFN_Surface::SetFormat(const MDFN_PixelFormat &nf, bool convert)
{
   format = nf;
}

MDFN_Surface::~MDFN_Surface()
{
   if(pixels)
      free(pixels);
}

