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

#include "TextureManager.h"


//////////////////////////////////////////
// Constructors / Deconstructors

// Probably shouldn't need more than 4096 * 4096

CTexture::CTexture(uint32_t dwWidth, uint32_t dwHeight, TextureUsage usage) :
    m_dwWidth(dwWidth),
    m_dwHeight(dwHeight),
    m_dwCreatedTextureWidth(dwWidth),
    m_dwCreatedTextureHeight(dwHeight),
    m_fXScale(1.0f),
    m_fYScale(1.0f),
    m_bScaledS(false),
    m_bScaledT(false),
    m_bClampedS(false),
    m_bClampedT(false),
    m_bIsEnhancedTexture(false),
    m_Usage(usage),
        m_pTexture(NULL),
        m_dwTextureFmt(TEXTURE_FMT_A8R8G8B8)
{
   // fix me, do something here
}


CTexture::~CTexture(void)
{
}

TextureFmt CTexture::GetSurfaceFormat(void)
{
   if (m_pTexture == NULL)
      return TEXTURE_FMT_UNKNOWN;
   return m_dwTextureFmt;
}

uint32_t CTexture::GetPixelSize()
{
   if( m_dwTextureFmt == TEXTURE_FMT_A8R8G8B8 )
      return 4;
   return 2;
}


// There are reasons to create this function. D3D and OGL will only create surface of width and height
// as 2's pow, for example, N64's 20x14 image, D3D and OGL will create a 32x16 surface.
// When we using such a surface as D3D texture, and the U and V address is for the D3D and OGL surface
// width and height. It is still OK if the U and V addr value is less than the real image within
// the D3D surface. But we will have problems if the U and V addr value is larger than it, or even
// large then 1.0.
// In such a case, we need to scale the image to the D3D surface dimension, to ease the U/V addr
// limition
void CTexture::ScaleImageToSurface(bool scaleS, bool scaleT)
{
   uint8_t g_ucTempBuffer[1024*1024*4];

   if( scaleS==false && scaleT==false)
      return;

   // If the image is not scaled, call this function to scale the real image to
   // the D3D given dimension

   uint32_t width = scaleS ? m_dwWidth : m_dwCreatedTextureWidth;
   uint32_t height = scaleT ? m_dwHeight : m_dwCreatedTextureHeight;

   uint32_t xDst, yDst;
   uint32_t xSrc, ySrc;

   DrawInfo di;

   if (!StartUpdate(&di))
      return;

   int pixSize = GetPixelSize();

   // Copy across from the temp buffer to the surface
   switch (pixSize)
   {
      case 4:
         {
            memcpy((uint8_t*)g_ucTempBuffer, (uint8_t*)(di.lpSurface), m_dwHeight*m_dwCreatedTextureWidth*4);

            uint32_t * pDst;
            uint32_t * pSrc;

            for (yDst = 0; yDst < m_dwCreatedTextureHeight; yDst++)
            {
               // ySrc ranges from 0..m_dwHeight
               // I'd rather do this but sometimes very narrow (i.e. 1 pixel)
               // surfaces are created which results in  /0...
               //ySrc = (yDst * (m_dwHeight-1)) / (d3dTextureHeight-1);
               ySrc = (uint32_t)((yDst * height) / m_dwCreatedTextureHeight+0.49f);

               pSrc = (uint32_t*)((uint8_t*)g_ucTempBuffer + (ySrc * m_dwCreatedTextureWidth * 4));
               pDst = (uint32_t*)((uint8_t*)di.lpSurface + (yDst * di.lPitch));

               for (xDst = 0; xDst < m_dwCreatedTextureWidth; xDst++)
               {
                  xSrc = (uint32_t)((xDst * width) / m_dwCreatedTextureWidth+0.49f);
                  pDst[xDst] = pSrc[xSrc];
               }
            }
         }

         break;
      case 2:
         {
            memcpy((uint8_t*)g_ucTempBuffer, (uint8_t*)(di.lpSurface), m_dwHeight*m_dwCreatedTextureWidth*2);

            uint16_t * pDst;
            uint16_t * pSrc;

            for (yDst = 0; yDst < m_dwCreatedTextureHeight; yDst++)
            {
               // ySrc ranges from 0..m_dwHeight
               ySrc = (yDst * height) / m_dwCreatedTextureHeight;

               pSrc = (uint16_t*)((uint8_t*)g_ucTempBuffer + (ySrc * m_dwCreatedTextureWidth * 2));
               pDst = (uint16_t*)((uint8_t*)di.lpSurface + (yDst * di.lPitch));

               for (xDst = 0; xDst < m_dwCreatedTextureWidth; xDst++)
               {
                  xSrc = (xDst * width) / m_dwCreatedTextureWidth;
                  pDst[xDst] = pSrc[xSrc];
               }
            }
         }
         break;
   }

   EndUpdate(&di);

   if( scaleS ) m_bScaledS = true;
   if( scaleT ) m_bScaledT = true;
}

void CTexture::ClampImageToSurfaceS()
{
   if( !m_bClampedS && m_dwWidth < m_dwCreatedTextureWidth )
   {       
      DrawInfo di;
      if( StartUpdate(&di) )
      {
         if(  m_dwTextureFmt == TEXTURE_FMT_A8R8G8B8 )
         {
            for( uint32_t y = 0; y<m_dwHeight; y++ )
            {
               uint32_t* line = (uint32_t*)((uint8_t*)di.lpSurface+di.lPitch*y);
               uint32_t val = line[m_dwWidth-1];
               for( uint32_t x=m_dwWidth; x<m_dwCreatedTextureWidth; x++ )
                  line[x] = val;
            }
         }
         else
         {
            for( uint32_t y = 0; y<m_dwHeight; y++ )
            {
               uint16_t* line = (uint16_t*)((uint8_t*)di.lpSurface+di.lPitch*y);
               uint16_t val = line[m_dwWidth-1];
               for( uint32_t x=m_dwWidth; x<m_dwCreatedTextureWidth; x++ )
                  line[x] = val;
            }
         }
         EndUpdate(&di);
      }
   }
   m_bClampedS = true;
}

void CTexture::ClampImageToSurfaceT()
{
   if( !m_bClampedT && m_dwHeight < m_dwCreatedTextureHeight )
   {
      DrawInfo di;
      if( StartUpdate(&di) )
      {
         if(  m_dwTextureFmt == TEXTURE_FMT_A8R8G8B8 )
         {
            uint32_t* linesrc = (uint32_t*)((uint8_t*)di.lpSurface+di.lPitch*(m_dwHeight-1));
            for( uint32_t y = m_dwHeight; y<m_dwCreatedTextureHeight; y++ )
            {
               uint32_t* linedst = (uint32_t*)((uint8_t*)di.lpSurface+di.lPitch*y);
               for( uint32_t x=0; x<m_dwCreatedTextureWidth; x++ )
                  linedst[x] = linesrc[x];
            }
         }
         else
         {
            uint16_t* linesrc = (uint16_t*)((uint8_t*)di.lpSurface+di.lPitch*(m_dwHeight-1));
            for( uint32_t y = m_dwHeight; y<m_dwCreatedTextureHeight; y++ )
            {
               uint16_t* linedst = (uint16_t*)((uint8_t*)di.lpSurface+di.lPitch*y);
               for( uint32_t x=0; x<m_dwCreatedTextureWidth; x++ )
                  linedst[x] = linesrc[x];
            }
         }
         EndUpdate(&di);
      }
   }
   m_bClampedT = true;
}

void CTexture::RestoreAlphaChannel(void)
{
   DrawInfo di;

   if ( StartUpdate(&di) )
   {
      uint32_t *pSrc = (uint32_t *)di.lpSurface;
      int lPitch = di.lPitch;

      for (uint32_t y = 0; y < m_dwHeight; y++)
      {
         uint32_t * dwSrc = (uint32_t *)((uint8_t *)pSrc + y*lPitch);
         for (uint32_t x = 0; x < m_dwWidth; x++)
         {
            uint32_t dw = dwSrc[x];
            uint32_t dwRed   = (uint8_t)((dw & 0x00FF0000)>>16);
            uint32_t dwGreen = (uint8_t)((dw & 0x0000FF00)>>8 );
            uint32_t dwBlue  = (uint8_t)((dw & 0x000000FF)    );
            uint32_t dwAlpha = (dwRed+dwGreen+dwBlue)/3;
            dw &= 0x00FFFFFF;
            dw |= (dwAlpha<<24);

            /*
               uint32_t dw = dwSrc[x];
               if( (dw&0x00FFFFFF) > 0 )
               dw |= 0xFF000000;
               else
               dw &= 0x00FFFFFF;
               */
         }
      }
      EndUpdate(&di);
   }
}

