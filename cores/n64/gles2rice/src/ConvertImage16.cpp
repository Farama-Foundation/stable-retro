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
#include <stdint.h>
#include "Config.h"
#include "ConvertImage.h"
#include "RenderBase.h"

// Still to be swapped:
// IA16

ConvertFunction     gConvertFunctions_16_FullTMEM[ 8 ][ 4 ] = 
{
    // 4bpp             8bpp            16bpp               32bpp
    {  Convert4b_16,    Convert8b_16,   Convert16b_16,      ConvertRGBA32_16 },     // RGBA
    {  NULL,            NULL,           ConvertYUV_16,      NULL },                 // YUV
    {  Convert4b_16,    Convert8b_16,   NULL,               NULL },                 // CI
    {  Convert4b_16,    Convert8b_16,   Convert16b_16,      NULL },                 // IA
    {  Convert4b_16,    Convert8b_16,   Convert16b_16,      NULL },                 // I
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL }                  // ?
};
ConvertFunction     gConvertFunctions_16[ 8 ][ 4 ] = 
{
    // 4bpp             8bpp            16bpp               32bpp
    {  ConvertCI4_16,   ConvertCI8_16,  ConvertRGBA16_16,   ConvertRGBA32_16 },     // RGBA
    {  NULL,            NULL,           ConvertYUV_16,      NULL },                 // YUV
    {  ConvertCI4_16,   ConvertCI8_16,  NULL,               NULL },                 // CI
    {  ConvertIA4_16,   ConvertIA8_16,  ConvertIA16_16,     NULL },                 // IA
    {  ConvertI4_16,    ConvertI8_16,   ConvertRGBA16_16,   NULL },                 // I
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL }                  // ?
};

ConvertFunction     gConvertTlutFunctions_16[ 8 ][ 4 ] = 
{
    // 4bpp             8bpp            16bpp               32bpp
    {  ConvertCI4_16,   ConvertCI8_16,  ConvertRGBA16_16,   ConvertRGBA32_16 },     // RGBA
    {  NULL,            NULL,           ConvertYUV_16,      NULL },                 // YUV
    {  ConvertCI4_16,   ConvertCI8_16,  NULL,               NULL },                 // CI
    {  ConvertCI4_16,   ConvertCI8_16,  ConvertIA16_16,     NULL },                 // IA
    {  ConvertCI4_16,   ConvertCI8_16,  ConvertRGBA16_16,   NULL },                 // I
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL },                 // ?
    {  NULL,            NULL,           NULL,               NULL }                  // ?
};

extern bool conkerSwapHack;

void ConvertRGBA16_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t x, y;
    uint32_t nFiddle;

    // Copy of the base pointer
    uint16_t * pSrc = (uint16_t*)(tinfo.pPhysicalAddress);
    uint8_t * pByteSrc = (uint8_t *)pSrc;

    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (y = 0; y < tinfo.HeightToLoad; y++)
        {
            if ((y%2) == 0)
                nFiddle = 0x2;
            else
                nFiddle = 0x2 | 0x4;

            // dwDst points to start of destination row
            uint16_t * wDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

            // DWordOffset points to the current dword we're looking at
            // (process 2 pixels at a time). May be a problem if we don't start on even pixel
            uint32_t dwWordOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2);

            for (x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint16_t w = *(uint16_t *)&pByteSrc[dwWordOffset ^ nFiddle];

                wDst[x] = Convert555ToR4G4B4A4(w);
                
                // Increment word offset to point to the next two pixels
                dwWordOffset += 2;
            }
        }
    }
    else
    {
        for (y = 0; y < tinfo.HeightToLoad; y++)
        {
            // dwDst points to start of destination row
            uint16_t * wDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

            // DWordOffset points to the current dword we're looking at
            // (process 2 pixels at a time). May be a problem if we don't start on even pixel
            uint32_t dwWordOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2);

            for (x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint16_t w = *(uint16_t *)&pByteSrc[dwWordOffset ^ 0x2];

                wDst[x] = Convert555ToR4G4B4A4(w);
                
                // Increment word offset to point to the next two pixels
                dwWordOffset += 2;
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

void ConvertRGBA32_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t * pSrc = (uint32_t*)(tinfo.pPhysicalAddress);
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if( options.bUseFullTMEM )
    {
        gDPTile *tile = &gDP.tiles[tinfo.tileNo];

        uint32_t *pWordSrc;
        if( tinfo.tileNo >= 0 )
        {
            pWordSrc = (uint32_t*)&g_Tmem.g_Tmem64bit[tile->tmem];

            for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
            {
                uint16_t * dwDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

                uint32_t nFiddle = ( y&1 )? 0x2 : 0;
                int idx = tile->line * 4 * y;

                for (uint32_t x = 0; x < tinfo.WidthToLoad; x++, idx++)
                {
                    uint32_t w   = pWordSrc[idx^nFiddle];
                    uint8_t* psw = (uint8_t*)&w;
                    dwDst[x]     = PAL8toR4G4B4A4( (psw[0]>>4), (psw[1]>>4), (psw[2]>>4), (psw[3]>>4));
                }
            }
        }
    }
    else
    {
        if (tinfo.bSwapped)
        {
            for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
            {
                if ((y%2) == 0)
                {
                    uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);
                    uint8_t *pS = (uint8_t *)pSrc + (y+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad*4);

                    for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
                    {

                        *pDst++ = PAL8toR4G4B4A4((pS[3]>>4),  // Red
                                                (pS[2]>>4),  // Green
                                                (pS[1]>>4),  // Blue
                                                (pS[0]>>4)); // Alpha
                        pS+=4;
                    }
                }
                else
                {
                    uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);
                    uint8_t *pS = (uint8_t *)pSrc + (y+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad*4);
                    int n;

                    n = 0;
                    for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
                    {
                        *pDst++ = PAL8toR4G4B4A4((pS[(n^0x8) + 3]>>4),   // Red
                                                (pS[(n^0x8) + 2]>>4),   // Green
                                                (pS[(n^0x8) + 1]>>4),   // Blue
                                                (pS[(n^0x8) + 0]>>4));  // Alpha

                        n += 4;
                    }
                }
            }
        }
        else
        {
            for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
            {
                uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);
                uint8_t *pS = (uint8_t *)pSrc + (y+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad*4);

                for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
                {
                    *pDst++ = PAL8toR4G4B4A4((pS[3]>>4),     // Red
                                            (pS[2]>>4),     // Green
                                            (pS[1]>>4),     // Blue
                                            (pS[0]>>4));    // Alpha
                    pS+=4;
                }
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();

}

// E.g. Dear Mario text
// Copy, Score etc
void ConvertIA4_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            // For odd lines, swap words too
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;


            // This may not work if X is not even?
            uint32_t dwByteOffset = (y+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2);

            // Do two pixels at a time
            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                // Even
                *pDst++ = PAL8toR4G4B4A4(ThreeToFour[(b & 0xE0) >> 5],
                                        ThreeToFour[(b & 0xE0) >> 5],
                                        ThreeToFour[(b & 0xE0) >> 5],
                                        OneToFour[(b & 0x10) >> 4]);
    
                // Odd
                *pDst++ = PAL8toR4G4B4A4(ThreeToFour[(b & 0x0E) >> 1],
                                        ThreeToFour[(b & 0x0E) >> 1],
                                        ThreeToFour[(b & 0x0E) >> 1],
                                        OneToFour[(b & 0x01)]     );

                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            // This may not work if X is not even?
            uint32_t dwByteOffset = (y+tinfo.TopToLoad) * tinfo.Pitch + (tinfo.LeftToLoad/2);

            // Do two pixels at a time
            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];

                // Even
                *pDst++ = PAL8toR4G4B4A4(ThreeToFour[(b & 0xE0) >> 5],
                                        ThreeToFour[(b & 0xE0) >> 5],
                                        ThreeToFour[(b & 0xE0) >> 5],
                                        OneToFour[(b & 0x10) >> 4]);
    
                // Odd
                *pDst++ = PAL8toR4G4B4A4(ThreeToFour[(b & 0x0E) >> 1],
                                        ThreeToFour[(b & 0x0E) >> 1],
                                        ThreeToFour[(b & 0x0E) >> 1],
                                        OneToFour[(b & 0x01)]     );

                dwByteOffset++;
            }
        }
    }
    
    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

// E.g Mario's head textures
void ConvertIA8_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            // For odd lines, swap words too
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;


            uint16_t *pDst = (uint16_t *)((uint8_t*)dInfo.lpSurface + y * dInfo.lPitch);
            // Points to current byte
            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                *pDst++ = PAL8toR4G4B4A4( ((b&0xf0)>>4),((b&0xf0)>>4),((b&0xf0)>>4),(b&0x0f));

                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            // Points to current byte
            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];
                *pDst++   = PAL8toR4G4B4A4(((b&0xf0)>>4),((b&0xf0)>>4),((b&0xf0)>>4),(b&0x0f));

                dwByteOffset++;
            }
        }
    }
    
    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();

}

// E.g. camera's clouds, shadows
void ConvertIA16_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;

    uint16_t * pSrc = (uint16_t*)(tinfo.pPhysicalAddress);
    uint8_t * pByteSrc = (uint8_t *)pSrc;

    if (!pTexture->StartUpdate(&dInfo))
        return;
        
    for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
    {
        uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

        // Points to current word
        uint32_t dwWordOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2);

        for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
        {
            uint16_t w    = *(uint16_t *)&pByteSrc[dwWordOffset^0x2];
            uint8_t i     = (uint8_t)(w >> 12);
            uint8_t a     = (uint8_t)(w & 0xFF);
            *pDst++       = PAL8toR4G4B4A4(i, i, i, (a>>4));
            dwWordOffset += 2;
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}



// Used by MarioKart 
void ConvertI4_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            // Might not work with non-even starting X
            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

            // For odd lines, swap words too
            if( !conkerSwapHack || (y&4) == 0 )
            {
                if ((y%2) == 0)
                    nFiddle = 0x3;
                else
                    nFiddle = 0x7;
            }
            else
            {
                if ((y%2) == 1)
                    nFiddle = 0x3;
                else
                    nFiddle = 0x7;
            }

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                // Even
                //*pDst++ = PAL8toR4G4B4A4(b>>4, b>>4, b>>4, b>>4);
                *pDst++   = FourToSixteen[(b & 0xF0)>>4];
                // Odd
                //*pDst++ = PAL8toR4G4B4A4(b & 0x0f, b & 0x0f, b & 0x0f, b & 0x0f);
                *pDst++ = FourToSixteen[b & 0x0f];

                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint8_t *pDst = (uint8_t *)dInfo.lpSurface + y * dInfo.lPitch;

            // Might not work with non-even starting X
            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];

                // Even
                //*pDst++ = PAL8toR4G4B4A4(b>>4, b>>4, b>>4, b>>4);
                *pDst++ = FourToEight[(b & 0xF0)>>4];

                // Odd
                //*pDst++ = PAL8toR4G4B4A4(b & 0x0f, b & 0x0f, b & 0x0f, b & 0x0f);
                *pDst++ = FourToEight[b & 0x0f];

                dwByteOffset++;
            }
        }
    }
    
    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

// Used by MarioKart
void ConvertI8_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    long long pSrc = (long long) (tinfo.pPhysicalAddress);
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;

            uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = *(uint8_t*)((pSrc+dwByteOffset)^nFiddle);

                *pDst++ = PAL8toR4G4B4A4(b>>4,
                                        b>>4,
                                        b>>4,
                                        b>>4);

                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t*)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = *(uint8_t*)((pSrc+dwByteOffset)^0x3);

                *pDst++ = PAL8toR4G4B4A4(b>>4,
                                        b>>4,
                                        b>>4,
                                        b>>4);

                dwByteOffset++;
            }
        }

    }
    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();

}


// Used by Starfox intro
void ConvertCI4_RGBA16_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    uint16_t * pPal = (uint16_t *)tinfo.PalAddress;
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {

        for (uint32_t y = 0; y <  tinfo.HeightToLoad; y++)
        {
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;


            uint16_t * pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                uint8_t bhi = (b&0xf0)>>4;
                uint8_t blo = (b&0x0f);

                pDst[0] = Convert555ToR4G4B4A4(pPal[bhi^1]);    // Remember palette is in different endian order!
                pDst[1] = Convert555ToR4G4B4A4(pPal[blo^1]);    // Remember palette is in different endian order!
                pDst+=2;

                dwByteOffset++;
            }
        }   
        
    }
    else
    {
        for (uint32_t y = 0; y <  tinfo.HeightToLoad; y++)
        {
            uint16_t * pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];

                uint8_t bhi = (b&0xf0)>>4;
                uint8_t blo = (b&0x0f);

                pDst[0] = Convert555ToR4G4B4A4(pPal[bhi^1]);    // Remember palette is in different endian order!
                pDst[1] = Convert555ToR4G4B4A4(pPal[blo^1]);    // Remember palette is in different endian order!
                pDst+=2;

                dwByteOffset++;
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

//*****************************************************************************
// Convert CI4 images. We need to switch on the palette type
//*****************************************************************************
void ConvertCI4_16( CTexture * p_texture, const TxtrInfo & tinfo )
{
    if ( tinfo.TLutFmt == TLUT_FMT_RGBA16 )
    {
        ConvertCI4_RGBA16_16( p_texture, tinfo );   
    }
    else if ( tinfo.TLutFmt == TLUT_FMT_IA16 )
    {
        ConvertCI4_IA16_16( p_texture, tinfo );                 
    }
}

//*****************************************************************************
// Convert CI8 images. We need to switch on the palette type
//*****************************************************************************
void ConvertCI8_16( CTexture * p_texture, const TxtrInfo & tinfo )
{
    if ( tinfo.TLutFmt == TLUT_FMT_RGBA16 )
    {
        ConvertCI8_RGBA16_16( p_texture, tinfo );   
    }
    else if ( tinfo.TLutFmt == TLUT_FMT_IA16 )
    {
        ConvertCI8_IA16_16( p_texture, tinfo );                 
    }
}

// Used by Starfox intro
void ConvertCI4_IA16_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    uint16_t * pPal = (uint16_t *)tinfo.PalAddress;
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y <  tinfo.HeightToLoad; y++)
        {
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;

            uint16_t * pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                uint8_t bhi = (b&0xf0)>>4;
                uint8_t blo = (b&0x0f);

                pDst[0] = ConvertIA16ToR4G4B4A4(pPal[bhi^1]);   // Remember palette is in different endian order!
                pDst[1] = ConvertIA16ToR4G4B4A4(pPal[blo^1]);   // Remember palette is in different endian order!
                pDst += 2;
                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y <  tinfo.HeightToLoad; y++)
        {
            uint16_t * pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

            for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];

                uint8_t bhi = (b&0xf0)>>4;
                uint8_t blo = (b&0x0f);

                pDst[0] = ConvertIA16ToR4G4B4A4(pPal[bhi^1]);   // Remember palette is in different endian order!
                pDst[1] = ConvertIA16ToR4G4B4A4(pPal[blo^1]);   // Remember palette is in different endian order!
                pDst+=2;

                dwByteOffset++;
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}


// Used by MarioKart for Cars etc
void ConvertCI8_RGBA16_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    uint16_t * pPal = (uint16_t *)tinfo.PalAddress;
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;

            uint16_t *pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;
            
            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                *pDst++ = Convert555ToR4G4B4A4(pPal[b^1]);  // Remember palette is in different endian order!

                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;
            
            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];

                *pDst++ = Convert555ToR4G4B4A4(pPal[b^1]);  // Remember palette is in different endian order!

                dwByteOffset++;
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}


// Used by MarioKart for Cars etc
void ConvertCI8_IA16_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    uint32_t nFiddle;

    uint8_t * pSrc = (uint8_t*)(tinfo.pPhysicalAddress);
    uint16_t * pPal = (uint16_t *)tinfo.PalAddress;
    if (!pTexture->StartUpdate(&dInfo))
        return;

    if (tinfo.bSwapped)
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            if ((y%2) == 0)
                nFiddle = 0x3;
            else
                nFiddle = 0x7;

            uint16_t *pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;
            
            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = pSrc[dwByteOffset ^ nFiddle];

                *pDst++ = ConvertIA16ToR4G4B4A4(pPal[b^1]); // Remember palette is in different endian order!

                dwByteOffset++;
            }
        }
    }
    else
    {
        for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
        {
            uint16_t *pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

            uint32_t dwByteOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;
            
            for (uint32_t x = 0; x < tinfo.WidthToLoad; x++)
            {
                uint8_t b = pSrc[dwByteOffset ^ 0x3];

                *pDst++ = ConvertIA16ToR4G4B4A4(pPal[b^1]); // Remember palette is in different endian order!

                dwByteOffset++;
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}


void ConvertYUV_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    if (!pTexture->StartUpdate(&dInfo))
        return;

    uint32_t x, y;
    uint32_t nFiddle;

    if( options.bUseFullTMEM )
    {
        gDPTile *tile = &gDP.tiles[tinfo.tileNo];

        uint16_t * pSrc;
        if( tinfo.tileNo >= 0 )
            pSrc = (uint16_t*)&g_Tmem.g_Tmem64bit[tile->tmem];
        else
            pSrc = (uint16_t*)(tinfo.pPhysicalAddress);

        uint8_t * pByteSrc = (uint8_t *)pSrc;
        for (y = 0; y < tinfo.HeightToLoad; y++)
        {
            nFiddle = ( y&1 )? 0x4 : 0;
            int dwWordOffset = tinfo.tileNo>=0? tile->line * 8 * y : ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2);
            uint16_t * wDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

            for (x = 0; x < tinfo.WidthToLoad/2; x++)
            {
                int y0 = *(uint8_t*)&pByteSrc[(dwWordOffset+1)^nFiddle];
                int y1 = *(uint8_t*)&pByteSrc[(dwWordOffset+3)^nFiddle];
                int u0 = *(uint8_t*)&pByteSrc[(dwWordOffset  )^nFiddle];
                int v0 = *(uint8_t*)&pByteSrc[(dwWordOffset+2)^nFiddle];

                wDst[x*2+0] = ConvertYUV16ToR4G4B4(y0,u0,v0);
                wDst[x*2+1] = ConvertYUV16ToR4G4B4(y1,u0,v0);

                dwWordOffset += 4;
            }
        }
    }
    else
    {
        // Copy of the base pointer
        uint16_t * pSrc = (uint16_t*)(tinfo.pPhysicalAddress);
        uint8_t * pByteSrc = (uint8_t *)pSrc;


        if (tinfo.bSwapped)
        {
            for (y = 0; y < tinfo.HeightToLoad; y++)
            {
                if ((y%2) == 0)
                    nFiddle = 0x2;
                else
                    nFiddle = 0x2 | 0x4;

                // dwDst points to start of destination row
                uint16_t * wDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

                // DWordOffset points to the current dword we're looking at
                // (process 2 pixels at a time). May be a problem if we don't start on even pixel
                uint32_t dwWordOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2);

                for (x = 0; x < tinfo.WidthToLoad/2; x++)
                {
                    uint32_t y0 = *(uint8_t*)&pByteSrc[(dwWordOffset+1)^nFiddle];
                    uint32_t y1 = *(uint8_t*)&pByteSrc[(dwWordOffset+3)^nFiddle];
                    uint32_t u0 = *(uint8_t*)&pByteSrc[(dwWordOffset  )^nFiddle];
                    uint32_t v0 = *(uint8_t*)&pByteSrc[(dwWordOffset+2)^nFiddle];

                    wDst[x*2+0] = ConvertYUV16ToR4G4B4(y0,u0,v0);
                    wDst[x*2+1] = ConvertYUV16ToR4G4B4(y1,u0,v0);

                    dwWordOffset += 4;
                }
            }
        }
        else
        {
            for (y = 0; y < tinfo.HeightToLoad; y++)
            {
                // dwDst points to start of destination row
                uint16_t * wDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

                // DWordOffset points to the current dword we're looking at
                // (process 2 pixels at a time). May be a problem if we don't start on even pixel
                uint32_t dwWordOffset = ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad * 2);

                for (x = 0; x < tinfo.WidthToLoad/2; x++)
                {
                    uint32_t y0 = *(uint8_t*)&pByteSrc[(dwWordOffset+1)^3];
                    uint32_t y1 = *(uint8_t*)&pByteSrc[(dwWordOffset+3)^3];
                    uint32_t u0 = *(uint8_t*)&pByteSrc[(dwWordOffset  )^3];
                    uint32_t v0 = *(uint8_t*)&pByteSrc[(dwWordOffset+2)^3];

                    wDst[x*2+0] = ConvertYUV16ToR4G4B4(y0,u0,v0);
                    wDst[x*2+1] = ConvertYUV16ToR4G4B4(y1,u0,v0);

                    dwWordOffset += 4;
                }
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

uint16_t ConvertYUV16ToR4G4B4(int Y, int U, int V)
{
    uint32_t A=1;
    uint32_t R1 = Y + g_convk0 * V;
    uint32_t G1 = Y + g_convk1 * U + g_convk2 * V;
    uint32_t B1 = Y + g_convk3 * U;
    uint32_t R = (R1 - g_convk4) * g_convk5 + R1;
    uint32_t G = (G1 - g_convk4) * g_convk5 + G1;
    uint32_t B = (B1 - g_convk4) * g_convk5 + B1;
    return (uint16_t)PAL8toR4G4B4A4((R>>4), (G>>4), (B>>4), 0xF*A);
}


// Used by Starfox intro
void Convert4b_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;

    if (!pTexture->StartUpdate(&dInfo)) 
        return;

    uint16_t * pPal = (uint16_t *)tinfo.PalAddress;
    bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_UNKNOWN);
    if( tinfo.Format <= G_IM_FMT_CI ) bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_NONE);

    gDPTile *tile = &gDP.tiles[tinfo.tileNo];

    uint8_t *pByteSrc = tinfo.tileNo >= 0 ? (uint8_t*)&g_Tmem.g_Tmem64bit[tile->tmem] : (uint8_t*)(tinfo.pPhysicalAddress);

    for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
    {
        uint16_t * pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

        uint32_t nFiddle;
        if( tinfo.tileNo < 0 )  
        {
            if (tinfo.bSwapped)
            {
                if ((y%2) == 0)
                    nFiddle = 0x3;
                else
                    nFiddle = 0x7;
            }
            else
            {
                nFiddle = 3;
            }
        }
        else
        {
            nFiddle = ( y&1 )? 0x4 : 0;
        }

        int idx = tinfo.tileNo>=0 ? tile->line * 8 * y : ((y+tinfo.TopToLoad) * tinfo.Pitch) + (tinfo.LeftToLoad / 2);

        for (uint32_t x = 0; x < tinfo.WidthToLoad; x+=2, idx++)
        {
            uint8_t b = pByteSrc[idx^nFiddle];
            uint8_t bhi = (b&0xf0)>>4;
            uint8_t blo = (b&0x0f);

            if( gRDP.otherMode.text_tlut>=2 || ( tinfo.Format != G_IM_FMT_IA && tinfo.Format != G_IM_FMT_I) )
            {
                if( tinfo.TLutFmt == TLUT_FMT_IA16 )
                {
                    if( tinfo.tileNo>=0 )
                    {
                        pDst[0] = ConvertIA16ToR4G4B4A4(g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(bhi<<2)]);
                        pDst[1] = ConvertIA16ToR4G4B4A4(g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(blo<<2)]);
                    }
                    else
                    {
                        pDst[0] = ConvertIA16ToR4G4B4A4(pPal[bhi^1]);
                        pDst[1] = ConvertIA16ToR4G4B4A4(pPal[blo^1]);
                    }
                }
                else
                {
                    if( tinfo.tileNo>=0 )
                    {
                        pDst[0] = Convert555ToR4G4B4A4(g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(bhi<<2)]);
                        pDst[1] = Convert555ToR4G4B4A4(g_Tmem.g_Tmem16bit[0x400+tinfo.Palette*0x40+(blo<<2)]);
                    }
                    else
                    {
                        pDst[0] = Convert555ToR4G4B4A4(pPal[bhi^1]);
                        pDst[1] = Convert555ToR4G4B4A4(pPal[blo^1]);
                    }
                }
            }
            else if( tinfo.Format == G_IM_FMT_IA )
            {
                pDst[0] = ConvertIA4ToR4G4B4A4(b>>4);
                pDst[1] = ConvertIA4ToR4G4B4A4(b&0xF);
            }
            else //if( tinfo.Format == G_IM_FMT_I )
            {
                pDst[0] = ConvertI4ToR4G4B4A4(b>>4);
                pDst[1] = ConvertI4ToR4G4B4A4(b&0xF);
            }

            if( bIgnoreAlpha )
            {
                pDst[0] |= 0xF000;
                pDst[1] |= 0xF000;
            }
            pDst+=2;
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

void Convert8b_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    if (!pTexture->StartUpdate(&dInfo)) 
        return;


    uint16_t * pPal = (uint16_t *)tinfo.PalAddress;
    bool bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_UNKNOWN);
    if( tinfo.Format <= G_IM_FMT_CI ) bIgnoreAlpha = (tinfo.TLutFmt==TLUT_FMT_NONE);

    gDPTile *tile = &gDP.tiles[tinfo.tileNo];

    uint8_t *pByteSrc;
    if( tinfo.tileNo >= 0 )
        pByteSrc = (uint8_t*)&g_Tmem.g_Tmem64bit[tile->tmem];
    else
        pByteSrc = (uint8_t*)(tinfo.pPhysicalAddress);


    for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
    {
        uint16_t * pDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y * dInfo.lPitch);

        uint32_t nFiddle;
        if( tinfo.tileNo < 0 )  
        {
            if (tinfo.bSwapped)
            {
                if ((y%2) == 0)
                    nFiddle = 0x3;
                else
                    nFiddle = 0x7;
            }
            else
                nFiddle = 3;
        }
        else
            nFiddle = ( y&1 )? 0x4 : 0;

        int idx = tinfo.tileNo>=0? tile->line * 8 * y : ((y+tinfo.TopToLoad) * tinfo.Pitch) + tinfo.LeftToLoad;

        for (uint32_t x = 0; x < tinfo.WidthToLoad; x++, idx++)
        {
            uint8_t b = pByteSrc[idx^nFiddle];

            if( gRDP.otherMode.text_tlut>=2 || ( tinfo.Format != G_IM_FMT_IA && tinfo.Format != G_IM_FMT_I) )
            {
                if( tinfo.TLutFmt == TLUT_FMT_IA16 )
                {
                    if( tinfo.tileNo>=0 )
                        *pDst = ConvertIA16ToR4G4B4A4(g_Tmem.g_Tmem16bit[0x400+(b<<2)]);
                    else
                        *pDst = ConvertIA16ToR4G4B4A4(pPal[b^1]);
                }
                else
                {
                    if( tinfo.tileNo>=0 )
                        *pDst = Convert555ToR4G4B4A4(g_Tmem.g_Tmem16bit[0x400+(b<<2)]);
                    else
                        *pDst = Convert555ToR4G4B4A4(pPal[b^1]);
                }
            }
            else if( tinfo.Format == G_IM_FMT_IA )
            {
                *pDst = PAL8toR4G4B4A4( ((b&0xf0)>>4),((b&0xf0)>>4),((b&0xf0)>>4),(b&0x0f));
            }
            else //if( tinfo.Format == G_IM_FMT_I )
            {
                *pDst = PAL8toR4G4B4A4(b>>4, b>>4, b>>4, b>>4);
            }

            if( bIgnoreAlpha )
            {
                *pDst |= 0xFF000000;
            }
            pDst++;
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}


void Convert16b_16(CTexture *pTexture, const TxtrInfo &tinfo)
{
    DrawInfo dInfo;
    if (!pTexture->StartUpdate(&dInfo)) 
        return;

    gDPTile *tile = &gDP.tiles[tinfo.tileNo];

    uint16_t *pWordSrc;
    if( tinfo.tileNo >= 0 )
        pWordSrc = (uint16_t*)&g_Tmem.g_Tmem64bit[tile->tmem];
    else
        pWordSrc = (uint16_t*)(tinfo.pPhysicalAddress);


    for (uint32_t y = 0; y < tinfo.HeightToLoad; y++)
    {
        uint16_t * dwDst = (uint16_t *)((uint8_t *)dInfo.lpSurface + y*dInfo.lPitch);

        uint32_t nFiddle;
        if( tinfo.tileNo < 0 )  
        {
            if (tinfo.bSwapped)
            {
                if ((y&1) == 0)
                    nFiddle = 0x1;
                else
                    nFiddle = 0x3;
            }
            else
                nFiddle = 0x1;
        }
        else
        {
            nFiddle = ( y&1 )? 0x2 : 0;
        }

        int idx = tinfo.tileNo>=0? tile->line * 4 * y : (((y+tinfo.TopToLoad) * tinfo.Pitch)>>1) + tinfo.LeftToLoad;

        for (uint32_t x = 0; x < tinfo.WidthToLoad; x++, idx++)
        {
            uint16_t w = pWordSrc[idx^nFiddle];
            uint16_t w2 = tinfo.tileNo>=0? ((w>>8)|(w<<8)) : w;

            if( tinfo.Format == G_IM_FMT_RGBA )
            {
                dwDst[x] = Convert555ToR4G4B4A4(w2);
            }
            else if( tinfo.Format == G_IM_FMT_YUV )
            {
            }
            else if( tinfo.Format >= G_IM_FMT_IA )
            {
                uint8_t i = (uint8_t)(w2 >> 12);
                uint8_t a = (uint8_t)(w2 & 0xFF);
                dwDst[x] = PAL8toR4G4B4A4(i, i, i, (a>>4));
            }
        }
    }

    pTexture->EndUpdate(&dInfo);
    pTexture->SetOthersVariables();
}

