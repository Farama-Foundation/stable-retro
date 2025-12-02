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

#ifndef _INNER_BLIT_H_
#define _INNER_BLIT_H_

#ifndef USE_BGR15
#define RGB24(R,G,B)	(((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
#define RGB16X2(C)      (((C)&(0x1f001f<<10))>>10) | (((C)&(0x1f001f<<5))<<1) | (((C)&(0x1f001f<<0))<<11)
#define RGB16(C)		(((C)&(0x1f<<10))>>10) | (((C)&(0x1f<<5))<<1) | (((C)&(0x1f<<0))<<11)
#else
#define RGB24(R,G,B)  	((((R)&0xF8)>>3)|(((G)&0xF8)<<2)|(((B)&0xF8)<<7))
#endif

///////////////////////////////////////////////////////////////////////////////
//  GPU Blitting code with rescale and interlace support.

INLINE void GPU_BlitWW(const void* src, u16* dst16, bool isRGB24)
{
	u32 uCount;
	if(!isRGB24)
	{
		#ifndef USE_BGR15
			uCount = 20;
			const u32* src32 = (const u32*) src; 
			u32* dst32 = (u32*)(void*) dst16;
			do{
				dst32[0] = RGB16X2(src32[0]);
				dst32[1] = RGB16X2(src32[1]);
				dst32[2] = RGB16X2(src32[2]);
				dst32[3] = RGB16X2(src32[3]);
				dst32[4] = RGB16X2(src32[4]);
				dst32[5] = RGB16X2(src32[5]);
				dst32[6] = RGB16X2(src32[6]);
				dst32[7] = RGB16X2(src32[7]);
				dst32 += 8;
				src32 += 8;
			}while(--uCount);
		#else
			memcpy(dst16,src,640);
		#endif
	}
	else
	{
		uCount = 20;
		const u8* src8 = (const u8*)src;
		do{
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 5] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 6] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 7] = RGB24(src8[21], src8[22], src8[23] );

			dst16[ 8] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 9] = RGB24(src8[27], src8[28], src8[29] );
			dst16[10] = RGB24(src8[30], src8[31], src8[32] );
			dst16[11] = RGB24(src8[33], src8[34], src8[35] );
			dst16[12] = RGB24(src8[36], src8[37], src8[38] );
			dst16[13] = RGB24(src8[39], src8[40], src8[41] );
			dst16[14] = RGB24(src8[42], src8[43], src8[44] );
			dst16[15] = RGB24(src8[45], src8[46], src8[47] );
			dst16 += 16;
			src8  += 48;
		}while(--uCount);
	}
}

INLINE void GPU_BlitWWSWWSWS(const void* src, u16* dst16, bool isRGB24)
{
	u32 uCount;
	if(!isRGB24)
	{
		#ifndef USE_BGR15
			uCount = 32;
			const u16* src16 = (const u16*) src; 
			do{
				dst16[ 0] = RGB16(src16[0]);
				dst16[ 1] = RGB16(src16[1]);
				dst16[ 2] = RGB16(src16[3]);
				dst16[ 3] = RGB16(src16[4]);
				dst16[ 4] = RGB16(src16[6]);
				dst16[ 5] = RGB16(src16[8]);
				dst16[ 6] = RGB16(src16[9]);
				dst16[ 7] = RGB16(src16[11]);
				dst16[ 8] = RGB16(src16[12]);
				dst16[ 9] = RGB16(src16[14]);
				dst16 += 10;
				src16 += 16;
			}while(--uCount);
		#else
			uCount = 64;
			const u16* src16 = (const u16*) src; 
			do{
				*dst16++ = *src16++;
				*dst16++ = *src16;
				src16+=2;
				*dst16++ = *src16++;
				*dst16++ = *src16;
				src16+=2;
				*dst16++ = *src16;
				src16+=2;
			}while(--uCount);
		#endif
	}
	else
	{
		uCount = 32;
		const u8* src8 = (const u8*)src;
		do{
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 3] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 4] = RGB24(src8[18], src8[19], src8[20] );

			dst16[ 5] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 6] = RGB24(src8[27], src8[28], src8[29] );
			dst16[ 7] = RGB24(src8[33], src8[34], src8[35] );
			dst16[ 8] = RGB24(src8[36], src8[37], src8[38] );
			dst16[ 9] = RGB24(src8[42], src8[43], src8[44] );

			dst16 += 10;
			src8  += 48;
		}while(--uCount);
	}
}

INLINE void GPU_BlitWWWWWS(const void* src, u16* dst16, bool isRGB24)
{
	u32 uCount;
	if(!isRGB24)
	{
		#ifndef USE_BGR15
			uCount = 32;
			const u16* src16 = (const u16*) src; 
			do{
				dst16[ 0] = RGB16(src16[0]);
				dst16[ 1] = RGB16(src16[1]);
				dst16[ 2] = RGB16(src16[2]);
				dst16[ 3] = RGB16(src16[3]);
				dst16[ 4] = RGB16(src16[4]);
				dst16[ 5] = RGB16(src16[6]);
				dst16[ 6] = RGB16(src16[7]);
				dst16[ 7] = RGB16(src16[8]);
				dst16[ 8] = RGB16(src16[9]);
				dst16[ 9] = RGB16(src16[10]);
				dst16 += 10;
				src16 += 12;
			}while(--uCount);
		#else
			uCount = 64;
			const u16* src16 = (const u16*) src; 
			do{
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16;
				src16+=2;
			}while(--uCount);
		#endif
	}
	else
	{
		uCount = 32;
		const u8* src8 = (const u8*)src;
		do{
			dst16[0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[5] = RGB24(src8[18], src8[19], src8[20] );
			dst16[6] = RGB24(src8[21], src8[22], src8[23] );
			dst16[7] = RGB24(src8[24], src8[25], src8[26] );
			dst16[8] = RGB24(src8[27], src8[28], src8[29] );
			dst16[9] = RGB24(src8[30], src8[31], src8[32] );
			dst16 += 10;
			src8  += 36;
		}while(--uCount);
	}
}

INLINE void GPU_BlitWWWWWWWWS(const void* src, u16* dst16, bool isRGB24, u32 uClip_src)
{
	u32 uCount;
	if(!isRGB24)
	{
		#ifndef USE_BGR15
			uCount = 20;
			const u16* src16 = ((const u16*) src) + uClip_src; 
			do{
				dst16[ 0] = RGB16(src16[0]);
				dst16[ 1] = RGB16(src16[1]);
				dst16[ 2] = RGB16(src16[2]);
				dst16[ 3] = RGB16(src16[3]);
				dst16[ 4] = RGB16(src16[4]);
				dst16[ 5] = RGB16(src16[5]);
				dst16[ 6] = RGB16(src16[6]);
				dst16[ 7] = RGB16(src16[7]);

				dst16[ 8] = RGB16(src16[9]);
				dst16[ 9] = RGB16(src16[10]);
				dst16[10] = RGB16(src16[11]);
				dst16[11] = RGB16(src16[12]);
				dst16[12] = RGB16(src16[13]);
				dst16[13] = RGB16(src16[14]);
				dst16[14] = RGB16(src16[15]);
				dst16[15] = RGB16(src16[16]);
				dst16 += 16;
				src16 += 18;
			}while(--uCount);
		#else
			uCount = 40;
			const u16* src16 = ((const u16*) src) + uClip_src; 
			do{
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16;
				src16+=2;
			}while(--uCount);
		#endif
	}
	else
	{
		uCount = 20;
		const u8* src8 = (const u8*)src + (uClip_src<<1) + uClip_src;
		do{
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 5] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 6] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 7] = RGB24(src8[21], src8[22], src8[23] );

			dst16[ 8] = RGB24(src8[27], src8[28], src8[29] );
			dst16[ 9] = RGB24(src8[30], src8[31], src8[32] );
			dst16[10] = RGB24(src8[33], src8[34], src8[35] );
			dst16[11] = RGB24(src8[36], src8[37], src8[38] );
			dst16[12] = RGB24(src8[39], src8[40], src8[41] );
			dst16[13] = RGB24(src8[42], src8[43], src8[44] );
			dst16[14] = RGB24(src8[45], src8[46], src8[47] );
			dst16[15] = RGB24(src8[48], src8[49], src8[50] );
			dst16 += 16;
			src8  += 54;
		}while(--uCount);
	}
}

INLINE void GPU_BlitWWDWW(const void* src, u16* dst16, bool isRGB24)
{
	u32 uCount;
	if(!isRGB24)
	{
		#ifndef USE_BGR15
			uCount = 32;
			const u16* src16 = (const u16*) src; 
			do{
				dst16[ 0] = RGB16(src16[0]);
				dst16[ 1] = RGB16(src16[1]);
				dst16[ 2] = dst16[1];
				dst16[ 3] = RGB16(src16[2]);
				dst16[ 4] = RGB16(src16[3]);
				dst16[ 5] = RGB16(src16[4]);
				dst16[ 6] = RGB16(src16[5]);
				dst16[ 7] = dst16[6];
				dst16[ 8] = RGB16(src16[6]);
				dst16[ 9] = RGB16(src16[7]);
				dst16 += 10;
				src16 +=  8;
			}while(--uCount);
		#else
			uCount = 64;
			const u16* src16 = (const u16*) src; 
			do{
				*dst16++ = *src16++;
				*dst16++ = *src16;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
				*dst16++ = *src16++;
			}while(--uCount);
		#endif
	}
	else
	{
		uCount = 32;
		const u8* src8 = (const u8*)src;
		do{
			dst16[ 0] = RGB24(src8[0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[3], src8[ 4], src8[ 5] );
			dst16[ 2] = dst16[1];
			dst16[ 3] = RGB24(src8[6], src8[ 7], src8[ 8] );
			dst16[ 4] = RGB24(src8[9], src8[10], src8[11] );

			dst16[ 5] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 6] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 7] = dst16[6];
			dst16[ 8] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 9] = RGB24(src8[21], src8[22], src8[23] );
			dst16 += 10;
			src8  += 24;
		}while(--uCount);
	}
}


INLINE void GPU_BlitWS(const void* src, u16* dst16, bool isRGB24)
{
	u32 uCount;
	if(!isRGB24)
	{
		#ifndef USE_BGR15
			uCount = 20;
			const u16* src16 = (const u16*) src; 
			do{
				dst16[ 0] = RGB16(src16[0]);
				dst16[ 1] = RGB16(src16[2]);
				dst16[ 2] = RGB16(src16[4]);
				dst16[ 3] = RGB16(src16[6]);

				dst16[ 4] = RGB16(src16[8]);
				dst16[ 5] = RGB16(src16[10]);
				dst16[ 6] = RGB16(src16[12]);
				dst16[ 7] = RGB16(src16[14]);

				dst16[ 8] = RGB16(src16[16]);
				dst16[ 9] = RGB16(src16[18]);
				dst16[10] = RGB16(src16[20]);
				dst16[11] = RGB16(src16[22]);

				dst16[12] = RGB16(src16[24]);
				dst16[13] = RGB16(src16[26]);
				dst16[14] = RGB16(src16[28]);
				dst16[15] = RGB16(src16[30]);

				dst16 += 16;
				src16 += 32;
			}while(--uCount);
		#else
			uCount = 320;
			const u16* src16 = (const u16*) src; 
			do{
				*dst16++ = *src16; src16+=2;
			}while(--uCount);
		#endif
	}
	else
	{
		uCount = 20;
		const u8* src8 = (const u8*) src; 
		do{
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 2] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 3] = RGB24(src8[18], src8[19], src8[20] );

			dst16[ 4] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 5] = RGB24(src8[30], src8[31], src8[32] );
			dst16[ 6] = RGB24(src8[36], src8[37], src8[38] );
			dst16[ 7] = RGB24(src8[42], src8[43], src8[44] );

			dst16[ 8] = RGB24(src8[48], src8[49], src8[50] );
			dst16[ 9] = RGB24(src8[54], src8[55], src8[56] );
			dst16[10] = RGB24(src8[60], src8[61], src8[62] );
			dst16[11] = RGB24(src8[66], src8[67], src8[68] );

			dst16[12] = RGB24(src8[72], src8[73], src8[74] );
			dst16[13] = RGB24(src8[78], src8[79], src8[80] );
			dst16[14] = RGB24(src8[84], src8[85], src8[86] );
			dst16[15] = RGB24(src8[90], src8[91], src8[92] );

			dst16 += 16;
			src8  += 96;
		}while(--uCount);
	}
}

#endif //_INNER_BLIT_H_
