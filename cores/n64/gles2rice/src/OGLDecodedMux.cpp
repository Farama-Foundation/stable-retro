/*
Copyright (C) 2003 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <retro_miscellaneous.h>

#include "OGLDecodedMux.h"

//========================================================================
void COGLDecodedMux::Simplify(void)
{
    DecodedMux::Simplify();
}

void COGLDecodedMux::Reformat(void)
{
    DecodedMux::Reformat(true);
    signed cond3 = MAX(splitType[0], splitType[1]);
    signed cond2 = MAX(cond3, splitType[2]);
    mType = (CombinerFormatType)MAX(cond2, splitType[3]);
}

void COGLExtDecodedMux::Simplify(void)
//========================================================================
{
    COGLDecodedMux::Simplify();
    FurtherFormatForOGL2();
    Reformat();     // Reformat again
}

void COGLExtDecodedMux::FurtherFormatForOGL2(void)
{
    // This function is used by OGL 1.2, no need to call this function
    // for Nvidia register combiner
    
    // And OGL 1.2 extension only supports 1 constant color, we can not use both PRIM and ENV
    // constant color, and we can not use SPECULAR color as the 2nd color.

    // To further format the mux.
    // - For each stage, allow only 1 texel, change the 2nd texel in the same stage to MUX_SHADE
    // - Only allow 1 constant color. Count PRIM and ENV, left the most used one, and change
    //   the 2nd one to MUX_SHADE

    if (Count(MUX_PRIM, -1, MUX_MASK) >= Count(MUX_ENV, -1, MUX_MASK))
    {
        ReplaceVal(MUX_ENV, MUX_PRIM, -1, MUX_MASK);
        //ReplaceVal(MUX_ENV, MUX_SHADE, -1, MUX_MASK);
        //ReplaceVal(MUX_ENV, MUX_1, -1, MUX_MASK);
        //ReplaceVal(MUX_PRIM, MUX_0, -1, MUX_MASK);
    }
    else
    {
        //ReplaceVal(MUX_PRIM, MUX_ENV, -1, MUX_MASK);
        //ReplaceVal(MUX_PRIM, MUX_SHADE, -1, MUX_MASK);
        ReplaceVal(MUX_PRIM, MUX_0, -1, MUX_MASK);
    }

    /*
    // Because OGL 1.2, we may use more than 1 texture unit, but for each texture unit,
    // we can not use multitexture to do color combiner. Each combiner stage can only
    // use 1 texture.

    if (IsUsed(MUX_TEXEL0, MUX_MASK) && IsUsed(MUX_TEXEL1, MUX_MASK))
    {
        if (Count(MUX_TEXEL0, 0, MUX_MASK)+Count(MUX_TEXEL0, 1, MUX_MASK) >= Count(MUX_TEXEL1, 0, MUX_MASK)+Count(MUX_TEXEL1, 1, MUX_MASK))
        {
            ReplaceVal(MUX_TEXEL1, MUX_TEXEL0, 0, MUX_MASK);
            ReplaceVal(MUX_TEXEL1, MUX_TEXEL0, 1, MUX_MASK);
        }
        else
        {
            ReplaceVal(MUX_TEXEL0, MUX_TEXEL1, 0, MUX_MASK);
            ReplaceVal(MUX_TEXEL0, MUX_TEXEL1, 1, MUX_MASK);
        }

        if (Count(MUX_TEXEL0, 2, MUX_MASK)+Count(MUX_TEXEL0, 3, MUX_MASK) >= Count(MUX_TEXEL1, 2, MUX_MASK)+Count(MUX_TEXEL1, 3, MUX_MASK))
        {
            ReplaceVal(MUX_TEXEL1, MUX_TEXEL0, 2, MUX_MASK);
            ReplaceVal(MUX_TEXEL1, MUX_TEXEL0, 3, MUX_MASK);
        }
        else
        {
            ReplaceVal(MUX_TEXEL0, MUX_TEXEL1, 2, MUX_MASK);
            ReplaceVal(MUX_TEXEL0, MUX_TEXEL1, 3, MUX_MASK);
        }
    }
    */
}
