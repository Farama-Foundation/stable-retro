/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//  Inner loop driver instanciation file

///////////////////////////////////////////////////////////////////////////////
//  Option Masks
#define   L ((CF>>0)&1)
#define   B ((CF>>1)&1)
#define   M ((CF>>2)&1)
#define  BM ((CF>>3)&3)
#define  TM ((CF>>5)&3)
#define   G ((CF>>7)&1)

#define  AH ((CF>>7)&1)

#define  MB ((CF>>8)&1)

#include "gpu_inner_blend.h"
#include "gpu_inner_light.h"

///////////////////////////////////////////////////////////////////////////////
//  GPU Pixel opperations generator
template<const int CF>
INLINE void gpuPixelFn(u16 *pixel,const u16 data)
{
	if ((!M)&&(!B))
	{
		if(MB) { *pixel = data | 0x8000; }
		else   { *pixel = data; }
	}
	else if ((M)&&(!B))
	{
		if (!(*pixel&0x8000))
		{
			if(MB) { *pixel = data | 0x8000; }
			else   { *pixel = data; }
		}
	}
	else
	{
		u16 uDst = *pixel;
		if(M) { if (uDst&0x8000) return; }
		u16 uSrc = data;
		u32 uMsk; if (BM==0) uMsk=0x7BDE;
		if (BM==0) gpuBlending00(uSrc, uDst);
		if (BM==1) gpuBlending01(uSrc, uDst);
		if (BM==2) gpuBlending02(uSrc, uDst);
		if (BM==3) gpuBlending03(uSrc, uDst);
		if(MB) { *pixel = uSrc | 0x8000; }
		else   { *pixel = uSrc; }
	}
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Pixel drawing drivers, for lines (only blending)
typedef void (*PD)(u16 *pixel,const u16 data);
const PD  gpuPixelDrivers[32] =   //  We only generate pixel op for MASKING/BLEND_ENABLE/BLEND_MODE
{ 
	gpuPixelFn<0x00<<1>,gpuPixelFn<0x01<<1>,gpuPixelFn<0x02<<1>,gpuPixelFn<0x03<<1>,  
	NULL,gpuPixelFn<0x05<<1>,NULL,gpuPixelFn<0x07<<1>,
	NULL,gpuPixelFn<0x09<<1>,NULL,gpuPixelFn<0x0B<<1>,
	NULL,gpuPixelFn<0x0D<<1>,NULL,gpuPixelFn<0x0F<<1>,

	gpuPixelFn<(0x00<<1)|256>,gpuPixelFn<(0x01<<1)|256>,gpuPixelFn<(0x02<<1)|256>,gpuPixelFn<(0x03<<1)|256>,  
	NULL,gpuPixelFn<(0x05<<1)|256>,NULL,gpuPixelFn<(0x07<<1)|256>,
	NULL,gpuPixelFn<(0x09<<1)|256>,NULL,gpuPixelFn<(0x0B<<1)|256>,
	NULL,gpuPixelFn<(0x0D<<1)|256>,NULL,gpuPixelFn<(0x0F<<1)|256>
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Tiles innerloops generator

template<const int CF>
INLINE void  gpuTileSpanFn(u16 *pDst, u32 count, u16 data)
{
	if ((!M)&&(!B))
	{
		if (MB) { data = data | 0x8000; }
		do { *pDst++ = data; } while (--count);
	}
	else if ((M)&&(!B))
	{
		if (MB) { data = data | 0x8000; }
		do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
	}
	else
	{
		u16 uSrc;
		u16 uDst;
		u32 uMsk; if (BM==0) uMsk=0x7BDE;
		do
		{
			//  MASKING
			uDst = *pDst;
			if(M) { if (uDst&0x8000) goto endtile;  }
			uSrc = data;

			//  BLEND
			if (BM==0) gpuBlending00(uSrc, uDst);
			if (BM==1) gpuBlending01(uSrc, uDst);
			if (BM==2) gpuBlending02(uSrc, uDst);
			if (BM==3) gpuBlending03(uSrc, uDst);

			if (MB) { *pDst = uSrc | 0x8000; }
			else    { *pDst = uSrc; }
			endtile: pDst++;
		}
		while (--count);
	}
}

///////////////////////////////////////////////////////////////////////////////
//  Tiles innerloops driver
typedef void (*PT)(u16 *pDst, u32 count, u16 data);
const PT gpuTileSpanDrivers[64] = 
{
	gpuTileSpanFn<0x00>,NULL,gpuTileSpanFn<0x02>,NULL,  gpuTileSpanFn<0x04>,NULL,gpuTileSpanFn<0x06>,NULL,  NULL,NULL,gpuTileSpanFn<0x0A>,NULL,  NULL,NULL,gpuTileSpanFn<0x0E>,NULL,
	NULL,NULL,gpuTileSpanFn<0x12>,NULL,  NULL,NULL,gpuTileSpanFn<0x16>,NULL,  NULL,NULL,gpuTileSpanFn<0x1A>,NULL,  NULL,NULL,gpuTileSpanFn<0x1E>,NULL,

	gpuTileSpanFn<0x100>,NULL,gpuTileSpanFn<0x102>,NULL,  gpuTileSpanFn<0x104>,NULL,gpuTileSpanFn<0x106>,NULL,  NULL,NULL,gpuTileSpanFn<0x10A>,NULL,  NULL,NULL,gpuTileSpanFn<0x10E>,NULL,
	NULL,NULL,gpuTileSpanFn<0x112>,NULL,  NULL,NULL,gpuTileSpanFn<0x116>,NULL,  NULL,NULL,gpuTileSpanFn<0x11A>,NULL,  NULL,NULL,gpuTileSpanFn<0x11E>,NULL,
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Sprites innerloops generator

template<const int CF>
INLINE void  gpuSpriteSpanFn(u16 *pDst, u32 count, u32 u0, const u32 mask)
{
	u16 uSrc;
	u16 uDst;
	const u16* pTxt = TBA+(u0&~0x1ff); u0=u0&0x1ff;
	const u16 *_CBA; if(TM!=3) _CBA=CBA;
	u32 lCol; if(L)  { lCol = ((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21));  }
	u8 rgb; if (TM==1) rgb = ((u8*)pTxt)[u0>>1];
	u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;

	do
	{
		//  MASKING
		if(M)   { uDst = *pDst;   if (uDst&0x8000) { u0=(u0+1)&mask; goto endsprite; }  }

		//  TEXTURE MAPPING
		if (TM==1) { if (!(u0&1)) rgb = ((u8*)pTxt)[u0>>1]; uSrc = _CBA[(rgb>>((u0&1)<<2))&0xf]; u0=(u0+1)&mask; }
		if (TM==2) { uSrc = _CBA[((u8*)pTxt)[u0]]; u0=(u0+1)&mask; }
		if (TM==3) { uSrc = pTxt[u0]; u0=(u0+1)&mask; }
		if(!AH) { if (!uSrc) goto endsprite; }

		//  BLEND
		if(B)
		{
			if(uSrc&0x8000)
			{
				//  LIGHTING CALCULATIONS
				if(L)  { gpuLightingTXT(uSrc, lCol);   }

				if(!M)    { uDst = *pDst; }
				if (BM==0) gpuBlending00(uSrc, uDst);
				if (BM==1) gpuBlending01(uSrc, uDst);
				if (BM==2) gpuBlending02(uSrc, uDst);
				if (BM==3) gpuBlending03(uSrc, uDst);
			}
			else
			{
				//  LIGHTING CALCULATIONS
				if(L)  { gpuLightingTXT(uSrc, lCol); }
			}
		}
		else
		{
			//  LIGHTING CALCULATIONS
			if(L)  { gpuLightingTXT(uSrc, lCol);   } else
			{ if(!MB) uSrc&= 0x7fff;               }
		}

		if (MB) { *pDst = uSrc | 0x8000; }
		else    { *pDst = uSrc; }
		
		endsprite: pDst++;
	}
	while (--count);
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Sprite innerloops driver
typedef void (*PS)(u16 *pDst, u32 count, u32 u0, const u32 mask);
const PS gpuSpriteSpanDrivers[512] = 
{
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	gpuSpriteSpanFn<0x20>,gpuSpriteSpanFn<0x21>,gpuSpriteSpanFn<0x22>,gpuSpriteSpanFn<0x23>,  gpuSpriteSpanFn<0x24>,gpuSpriteSpanFn<0x25>,gpuSpriteSpanFn<0x26>,gpuSpriteSpanFn<0x27>,  NULL,NULL,gpuSpriteSpanFn<0x2A>,gpuSpriteSpanFn<0x2B>,  NULL,NULL,gpuSpriteSpanFn<0x2E>,gpuSpriteSpanFn<0x2F>,
	NULL,NULL,gpuSpriteSpanFn<0x32>,gpuSpriteSpanFn<0x33>,  NULL,NULL,gpuSpriteSpanFn<0x36>,gpuSpriteSpanFn<0x37>,  NULL,NULL,gpuSpriteSpanFn<0x3A>,gpuSpriteSpanFn<0x3B>,  NULL,NULL,gpuSpriteSpanFn<0x3E>,gpuSpriteSpanFn<0x3F>,
	gpuSpriteSpanFn<0x40>,gpuSpriteSpanFn<0x41>,gpuSpriteSpanFn<0x42>,gpuSpriteSpanFn<0x43>,  gpuSpriteSpanFn<0x44>,gpuSpriteSpanFn<0x45>,gpuSpriteSpanFn<0x46>,gpuSpriteSpanFn<0x47>,  NULL,NULL,gpuSpriteSpanFn<0x4A>,gpuSpriteSpanFn<0x4B>,  NULL,NULL,gpuSpriteSpanFn<0x4E>,gpuSpriteSpanFn<0x4F>,
	NULL,NULL,gpuSpriteSpanFn<0x52>,gpuSpriteSpanFn<0x53>,  NULL,NULL,gpuSpriteSpanFn<0x56>,gpuSpriteSpanFn<0x57>,  NULL,NULL,gpuSpriteSpanFn<0x5A>,gpuSpriteSpanFn<0x5B>,  NULL,NULL,gpuSpriteSpanFn<0x5E>,gpuSpriteSpanFn<0x5F>,
	gpuSpriteSpanFn<0x60>,gpuSpriteSpanFn<0x61>,gpuSpriteSpanFn<0x62>,gpuSpriteSpanFn<0x63>,  gpuSpriteSpanFn<0x64>,gpuSpriteSpanFn<0x65>,gpuSpriteSpanFn<0x66>,gpuSpriteSpanFn<0x67>,  NULL,NULL,gpuSpriteSpanFn<0x6A>,gpuSpriteSpanFn<0x6B>,  NULL,NULL,gpuSpriteSpanFn<0x6E>,gpuSpriteSpanFn<0x6F>,
	NULL,NULL,gpuSpriteSpanFn<0x72>,gpuSpriteSpanFn<0x73>,  NULL,NULL,gpuSpriteSpanFn<0x76>,gpuSpriteSpanFn<0x77>,  NULL,NULL,gpuSpriteSpanFn<0x7A>,gpuSpriteSpanFn<0x7B>,  NULL,NULL,gpuSpriteSpanFn<0x7E>,gpuSpriteSpanFn<0x7F>,

	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	gpuSpriteSpanFn<0xa0>,gpuSpriteSpanFn<0xa1>,gpuSpriteSpanFn<0xa2>,gpuSpriteSpanFn<0xa3>,  gpuSpriteSpanFn<0xa4>,gpuSpriteSpanFn<0xa5>,gpuSpriteSpanFn<0xa6>,gpuSpriteSpanFn<0xa7>,  NULL,NULL,gpuSpriteSpanFn<0xaA>,gpuSpriteSpanFn<0xaB>,  NULL,NULL,gpuSpriteSpanFn<0xaE>,gpuSpriteSpanFn<0xaF>,
	NULL,NULL,gpuSpriteSpanFn<0xb2>,gpuSpriteSpanFn<0xb3>,  NULL,NULL,gpuSpriteSpanFn<0xb6>,gpuSpriteSpanFn<0xb7>,  NULL,NULL,gpuSpriteSpanFn<0xbA>,gpuSpriteSpanFn<0xbB>,  NULL,NULL,gpuSpriteSpanFn<0xbE>,gpuSpriteSpanFn<0xbF>,
	gpuSpriteSpanFn<0xc0>,gpuSpriteSpanFn<0xc1>,gpuSpriteSpanFn<0xc2>,gpuSpriteSpanFn<0xc3>,  gpuSpriteSpanFn<0xc4>,gpuSpriteSpanFn<0xc5>,gpuSpriteSpanFn<0xc6>,gpuSpriteSpanFn<0xc7>,  NULL,NULL,gpuSpriteSpanFn<0xcA>,gpuSpriteSpanFn<0xcB>,  NULL,NULL,gpuSpriteSpanFn<0xcE>,gpuSpriteSpanFn<0xcF>,
	NULL,NULL,gpuSpriteSpanFn<0xd2>,gpuSpriteSpanFn<0xd3>,  NULL,NULL,gpuSpriteSpanFn<0xd6>,gpuSpriteSpanFn<0xd7>,  NULL,NULL,gpuSpriteSpanFn<0xdA>,gpuSpriteSpanFn<0xdB>,  NULL,NULL,gpuSpriteSpanFn<0xdE>,gpuSpriteSpanFn<0xdF>,
	gpuSpriteSpanFn<0xe0>,gpuSpriteSpanFn<0xe1>,gpuSpriteSpanFn<0xe2>,gpuSpriteSpanFn<0xe3>,  gpuSpriteSpanFn<0xe4>,gpuSpriteSpanFn<0xe5>,gpuSpriteSpanFn<0xe6>,gpuSpriteSpanFn<0xe7>,  NULL,NULL,gpuSpriteSpanFn<0xeA>,gpuSpriteSpanFn<0xeB>,  NULL,NULL,gpuSpriteSpanFn<0xeE>,gpuSpriteSpanFn<0xeF>,
	NULL,NULL,gpuSpriteSpanFn<0xf2>,gpuSpriteSpanFn<0xf3>,  NULL,NULL,gpuSpriteSpanFn<0xf6>,gpuSpriteSpanFn<0xf7>,  NULL,NULL,gpuSpriteSpanFn<0xfA>,gpuSpriteSpanFn<0xfB>,  NULL,NULL,gpuSpriteSpanFn<0xfE>,gpuSpriteSpanFn<0xfF>,

	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	gpuSpriteSpanFn<0x120>,gpuSpriteSpanFn<0x121>,gpuSpriteSpanFn<0x122>,gpuSpriteSpanFn<0x123>,  gpuSpriteSpanFn<0x124>,gpuSpriteSpanFn<0x125>,gpuSpriteSpanFn<0x126>,gpuSpriteSpanFn<0x127>,  NULL,NULL,gpuSpriteSpanFn<0x12A>,gpuSpriteSpanFn<0x12B>,  NULL,NULL,gpuSpriteSpanFn<0x12E>,gpuSpriteSpanFn<0x12F>,
	NULL,NULL,gpuSpriteSpanFn<0x132>,gpuSpriteSpanFn<0x133>,  NULL,NULL,gpuSpriteSpanFn<0x136>,gpuSpriteSpanFn<0x137>,  NULL,NULL,gpuSpriteSpanFn<0x13A>,gpuSpriteSpanFn<0x13B>,  NULL,NULL,gpuSpriteSpanFn<0x13E>,gpuSpriteSpanFn<0x13F>,
	gpuSpriteSpanFn<0x140>,gpuSpriteSpanFn<0x141>,gpuSpriteSpanFn<0x142>,gpuSpriteSpanFn<0x143>,  gpuSpriteSpanFn<0x144>,gpuSpriteSpanFn<0x145>,gpuSpriteSpanFn<0x146>,gpuSpriteSpanFn<0x147>,  NULL,NULL,gpuSpriteSpanFn<0x14A>,gpuSpriteSpanFn<0x14B>,  NULL,NULL,gpuSpriteSpanFn<0x14E>,gpuSpriteSpanFn<0x14F>,
	NULL,NULL,gpuSpriteSpanFn<0x152>,gpuSpriteSpanFn<0x153>,  NULL,NULL,gpuSpriteSpanFn<0x156>,gpuSpriteSpanFn<0x157>,  NULL,NULL,gpuSpriteSpanFn<0x15A>,gpuSpriteSpanFn<0x15B>,  NULL,NULL,gpuSpriteSpanFn<0x15E>,gpuSpriteSpanFn<0x15F>,
	gpuSpriteSpanFn<0x160>,gpuSpriteSpanFn<0x161>,gpuSpriteSpanFn<0x162>,gpuSpriteSpanFn<0x163>,  gpuSpriteSpanFn<0x164>,gpuSpriteSpanFn<0x165>,gpuSpriteSpanFn<0x166>,gpuSpriteSpanFn<0x167>,  NULL,NULL,gpuSpriteSpanFn<0x16A>,gpuSpriteSpanFn<0x16B>,  NULL,NULL,gpuSpriteSpanFn<0x16E>,gpuSpriteSpanFn<0x16F>,
	NULL,NULL,gpuSpriteSpanFn<0x172>,gpuSpriteSpanFn<0x173>,  NULL,NULL,gpuSpriteSpanFn<0x176>,gpuSpriteSpanFn<0x177>,  NULL,NULL,gpuSpriteSpanFn<0x17A>,gpuSpriteSpanFn<0x17B>,  NULL,NULL,gpuSpriteSpanFn<0x17E>,gpuSpriteSpanFn<0x17F>,
                                                                                                                                                                                                                                                                                                                                                                                      
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	gpuSpriteSpanFn<0x1a0>,gpuSpriteSpanFn<0x1a1>,gpuSpriteSpanFn<0x1a2>,gpuSpriteSpanFn<0x1a3>,  gpuSpriteSpanFn<0x1a4>,gpuSpriteSpanFn<0x1a5>,gpuSpriteSpanFn<0x1a6>,gpuSpriteSpanFn<0x1a7>,  NULL,NULL,gpuSpriteSpanFn<0x1aA>,gpuSpriteSpanFn<0x1aB>,  NULL,NULL,gpuSpriteSpanFn<0x1aE>,gpuSpriteSpanFn<0x1aF>,
	NULL,NULL,gpuSpriteSpanFn<0x1b2>,gpuSpriteSpanFn<0x1b3>,  NULL,NULL,gpuSpriteSpanFn<0x1b6>,gpuSpriteSpanFn<0x1b7>,  NULL,NULL,gpuSpriteSpanFn<0x1bA>,gpuSpriteSpanFn<0x1bB>,  NULL,NULL,gpuSpriteSpanFn<0x1bE>,gpuSpriteSpanFn<0x1bF>,
	gpuSpriteSpanFn<0x1c0>,gpuSpriteSpanFn<0x1c1>,gpuSpriteSpanFn<0x1c2>,gpuSpriteSpanFn<0x1c3>,  gpuSpriteSpanFn<0x1c4>,gpuSpriteSpanFn<0x1c5>,gpuSpriteSpanFn<0x1c6>,gpuSpriteSpanFn<0x1c7>,  NULL,NULL,gpuSpriteSpanFn<0x1cA>,gpuSpriteSpanFn<0x1cB>,  NULL,NULL,gpuSpriteSpanFn<0x1cE>,gpuSpriteSpanFn<0x1cF>,
	NULL,NULL,gpuSpriteSpanFn<0x1d2>,gpuSpriteSpanFn<0x1d3>,  NULL,NULL,gpuSpriteSpanFn<0x1d6>,gpuSpriteSpanFn<0x1d7>,  NULL,NULL,gpuSpriteSpanFn<0x1dA>,gpuSpriteSpanFn<0x1dB>,  NULL,NULL,gpuSpriteSpanFn<0x1dE>,gpuSpriteSpanFn<0x1dF>,
	gpuSpriteSpanFn<0x1e0>,gpuSpriteSpanFn<0x1e1>,gpuSpriteSpanFn<0x1e2>,gpuSpriteSpanFn<0x1e3>,  gpuSpriteSpanFn<0x1e4>,gpuSpriteSpanFn<0x1e5>,gpuSpriteSpanFn<0x1e6>,gpuSpriteSpanFn<0x1e7>,  NULL,NULL,gpuSpriteSpanFn<0x1eA>,gpuSpriteSpanFn<0x1eB>,  NULL,NULL,gpuSpriteSpanFn<0x1eE>,gpuSpriteSpanFn<0x1eF>,
	NULL,NULL,gpuSpriteSpanFn<0x1f2>,gpuSpriteSpanFn<0x1f3>,  NULL,NULL,gpuSpriteSpanFn<0x1f6>,gpuSpriteSpanFn<0x1f7>,  NULL,NULL,gpuSpriteSpanFn<0x1fA>,gpuSpriteSpanFn<0x1fB>,  NULL,NULL,gpuSpriteSpanFn<0x1fE>,gpuSpriteSpanFn<0x1fF>
};

///////////////////////////////////////////////////////////////////////////////
//  GPU Polygon innerloops generator
template<const int CF>
INLINE void  gpuPolySpanFn(u16 *pDst, u32 count)
{
	if (!TM)
	{	
		// NO TEXTURE
		if (!G)
		{
			// NO GOURAUD
			u16 data;
			if (L) { u32 lCol=((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); gpuLightingRGB(data,lCol); }
			else data=PixelData;
			if ((!M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { *pDst++ = data; } while (--count);
			}
			else if ((M)&&(!B))
			{
				if (MB) { data = data | 0x8000; }
				do { if (!(*pDst&0x8000)) { *pDst = data; } pDst++; } while (--count);
			}
			else
			{
				u16 uSrc;
				u16 uDst;
				u32 uMsk; if (BM==0) uMsk=0x7BDE;
				do
				{
					//  masking
					uDst = *pDst;
					if(M) { if (uDst&0x8000) goto endtile;  }
					uSrc = data;
					//  blend
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
					if (MB) { *pDst = uSrc | 0x8000; }
					else    { *pDst = uSrc; }
					endtile: pDst++;
				}
				while (--count);
			}
		}
		else
		{
			// GOURAUD
			u16 uDst;
			u16 uSrc;
			u32 linc=lInc;
			u32 lCol=((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21));
			u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
			do
			{
				//  masking
				if(M) { uDst = *pDst;  if (uDst&0x8000) goto endgou;  }
				//  blend
				if(B)
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					//  light
					gpuLightingRGB(uSrc,lCol);
				}
				if (MB) { *pDst = uSrc | 0x8000; }
				else    { *pDst = uSrc; }
				endgou: pDst++; lCol=(lCol+linc);
			}
			while (--count);
		}
	}
	else
	{
		// TEXTURE
		u16 uDst;
		u16 uSrc;
		u32 linc; if (L&&G) linc=lInc;
		u32 tinc=tInc;
		u32 tmsk=tMsk;
		u32 tCor = ((u32)( u4<<7)&0x7fff0000) | ((u32)( v4>>9)&0x00007fff); tCor&= tmsk;
		const u16* _TBA=TBA;
		const u16* _CBA; if (TM!=3) _CBA=CBA;
		u32 lCol;
		if(L && !G) { lCol = ((u32)(b4<< 2)&(0x03ff)) | ((u32)(g4<<13)&(0x07ff<<10)) | ((u32)(r4<<24)&(0x07ff<<21)); }
		else if(L && G) { lCol = ((u32)(b4>>14)&(0x03ff)) | ((u32)(g4>>3)&(0x07ff<<10)) | ((u32)(r4<<8)&(0x07ff<<21)); 	}
		u32 uMsk; if ((B)&&(BM==0)) uMsk=0x7BDE;
		do
		{
			//  masking
			if(M) { uDst = *pDst;  if (uDst&0x8000) goto endpoly;  }
			//  texture
			if (TM==1) { u32 tu=(tCor>>23); u32 tv=(tCor<<4)&(0xff<<11); u8 rgb=((u8*)_TBA)[tv+(tu>>1)]; uSrc=_CBA[(rgb>>((tu&1)<<2))&0xf]; if(!uSrc) goto endpoly; }
			if (TM==2) { uSrc = _CBA[(((u8*)_TBA)[(tCor>>23)+((tCor<<4)&(0xff<<11))])]; if(!uSrc)  goto endpoly; }
			if (TM==3) { uSrc = _TBA[(tCor>>23)+((tCor<<3)&(0xff<<10))]; if(!uSrc)  goto endpoly; }
			//  blend
			if(B)
			{
				if (uSrc&0x8000)
				{
					//  light
					if(L) gpuLightingTXT(uSrc, lCol);
					if(!M)    { uDst = *pDst; }
					if (BM==0) gpuBlending00(uSrc, uDst);
					if (BM==1) gpuBlending01(uSrc, uDst);
					if (BM==2) gpuBlending02(uSrc, uDst);
					if (BM==3) gpuBlending03(uSrc, uDst);
				}
				else
				{
					// light
					if(L) gpuLightingTXT(uSrc, lCol);
				}
			}
			else
			{
				//  light
				if(L)  { gpuLightingTXT(uSrc, lCol); } else if(!MB) { uSrc&= 0x7fff; }
			}
			if (MB) { *pDst = uSrc | 0x8000; }
			else    { *pDst = uSrc; }
			endpoly: pDst++;
			tCor=(tCor+tinc)&tmsk;
			if (L&&G) lCol=(lCol+linc);
		}
		while (--count);
	}
}

// supposedly shouldn't be called?
static void gpuPolySpanFn_NULL_(u16 *pDst, u32 count)
{
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Polygon innerloops driver
typedef void (*PP)(u16 *pDst, u32 count);
const PP gpuPolySpanDrivers[512] =
{
	gpuPolySpanFn<0x00>,gpuPolySpanFn<0x01>,gpuPolySpanFn<0x02>,gpuPolySpanFn<0x03>,  gpuPolySpanFn<0x04>,gpuPolySpanFn<0x05>,gpuPolySpanFn<0x06>,gpuPolySpanFn<0x07>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x0A>,gpuPolySpanFn<0x0B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x0E>,gpuPolySpanFn<0x0F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x12>,gpuPolySpanFn<0x13>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x16>,gpuPolySpanFn<0x17>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1A>,gpuPolySpanFn<0x1B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1E>,gpuPolySpanFn<0x1F>,
	gpuPolySpanFn<0x20>,gpuPolySpanFn<0x21>,gpuPolySpanFn<0x22>,gpuPolySpanFn<0x23>,  gpuPolySpanFn<0x24>,gpuPolySpanFn<0x25>,gpuPolySpanFn<0x26>,gpuPolySpanFn<0x27>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x2A>,gpuPolySpanFn<0x2B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x2E>,gpuPolySpanFn<0x2F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x32>,gpuPolySpanFn<0x33>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x36>,gpuPolySpanFn<0x37>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x3A>,gpuPolySpanFn<0x3B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x3E>,gpuPolySpanFn<0x3F>,
	gpuPolySpanFn<0x40>,gpuPolySpanFn<0x41>,gpuPolySpanFn<0x42>,gpuPolySpanFn<0x43>,  gpuPolySpanFn<0x44>,gpuPolySpanFn<0x45>,gpuPolySpanFn<0x46>,gpuPolySpanFn<0x47>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x4A>,gpuPolySpanFn<0x4B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x4E>,gpuPolySpanFn<0x4F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x52>,gpuPolySpanFn<0x53>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x56>,gpuPolySpanFn<0x57>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x5A>,gpuPolySpanFn<0x5B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x5E>,gpuPolySpanFn<0x5F>,
	gpuPolySpanFn<0x60>,gpuPolySpanFn<0x61>,gpuPolySpanFn<0x62>,gpuPolySpanFn<0x63>,  gpuPolySpanFn<0x64>,gpuPolySpanFn<0x65>,gpuPolySpanFn<0x66>,gpuPolySpanFn<0x67>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x6A>,gpuPolySpanFn<0x6B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x6E>,gpuPolySpanFn<0x6F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x72>,gpuPolySpanFn<0x73>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x76>,gpuPolySpanFn<0x77>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x7A>,gpuPolySpanFn<0x7B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x7E>,gpuPolySpanFn<0x7F>,

	gpuPolySpanFn_NULL_,gpuPolySpanFn<0x81>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x83>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0x85>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x87>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x8B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x8F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x93>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x97>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x9B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x9F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0xa1>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xa3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0xa5>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xa7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xaB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xaF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xb3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xb7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xbB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xbF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0xc1>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xc3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0xc5>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xc7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xcB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xcF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xd3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xd7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xdB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xdF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0xe1>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xe3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0xe5>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xe7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xeB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xeF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xf3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xf7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xfB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0xfF>,

	gpuPolySpanFn<0x100>,gpuPolySpanFn<0x101>,gpuPolySpanFn<0x102>,gpuPolySpanFn<0x103>,  gpuPolySpanFn<0x104>,gpuPolySpanFn<0x105>,gpuPolySpanFn<0x106>,gpuPolySpanFn<0x107>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x10A>,gpuPolySpanFn<0x10B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x10E>,gpuPolySpanFn<0x10F>,
	gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x112>,gpuPolySpanFn<0x113>,  gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x116>,gpuPolySpanFn<0x117>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x11A>,gpuPolySpanFn<0x11B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x11E>,gpuPolySpanFn<0x11F>,
	gpuPolySpanFn<0x120>,gpuPolySpanFn<0x121>,gpuPolySpanFn<0x122>,gpuPolySpanFn<0x123>,  gpuPolySpanFn<0x124>,gpuPolySpanFn<0x125>,gpuPolySpanFn<0x126>,gpuPolySpanFn<0x127>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x12A>,gpuPolySpanFn<0x12B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x12E>,gpuPolySpanFn<0x12F>,
	gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x132>,gpuPolySpanFn<0x133>,  gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x136>,gpuPolySpanFn<0x137>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x13A>,gpuPolySpanFn<0x13B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x13E>,gpuPolySpanFn<0x13F>,
	gpuPolySpanFn<0x140>,gpuPolySpanFn<0x141>,gpuPolySpanFn<0x142>,gpuPolySpanFn<0x143>,  gpuPolySpanFn<0x144>,gpuPolySpanFn<0x145>,gpuPolySpanFn<0x146>,gpuPolySpanFn<0x147>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x14A>,gpuPolySpanFn<0x14B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x14E>,gpuPolySpanFn<0x14F>,
	gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x152>,gpuPolySpanFn<0x153>,  gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x156>,gpuPolySpanFn<0x157>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x15A>,gpuPolySpanFn<0x15B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x15E>,gpuPolySpanFn<0x15F>,
	gpuPolySpanFn<0x160>,gpuPolySpanFn<0x161>,gpuPolySpanFn<0x162>,gpuPolySpanFn<0x163>,  gpuPolySpanFn<0x164>,gpuPolySpanFn<0x165>,gpuPolySpanFn<0x166>,gpuPolySpanFn<0x167>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x16A>,gpuPolySpanFn<0x16B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x16E>,gpuPolySpanFn<0x16F>,
	gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x172>,gpuPolySpanFn<0x173>,  gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_, gpuPolySpanFn<0x176>,gpuPolySpanFn<0x177>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x17A>,gpuPolySpanFn<0x17B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x17E>,gpuPolySpanFn<0x17F>,
                                                                                                                                                                                                                                                                                                                                                                                      
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0x181>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x183>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0x185>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x187>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x18B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x18F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x193>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x197>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x19B>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x19F>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1a1>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1a3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1a5>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1a7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1aB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1aF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1b3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1b7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1bB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1bF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1c1>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1c3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1c5>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1c7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1cB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1cF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1d3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1d7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1dB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1dF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1e1>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1e3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1e5>,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1e7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1eB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1eF>,
	gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1f3>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_, gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1f7>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1fB>,  gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn_NULL_,gpuPolySpanFn<0x1fF>
};
