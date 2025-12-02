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
INLINE void gpuLoadImage(void)
{
	u16 x0, y0, w0, h0;
	x0 = PacketBuffer.U2[2] & 1023;
	y0 = PacketBuffer.U2[3] & 511;
	w0 = PacketBuffer.U2[4];
	h0 = PacketBuffer.U2[5];

	if ((y0 + h0) > FRAME_HEIGHT)
	{
		h0 = FRAME_HEIGHT - y0;
	}

	FrameToWrite = ((w0)&&(h0));

	px = 0;
	py = 0;
	x_end = w0;
	y_end = h0;
	pvram = &((u16*)GPU_FrameBuffer)[x0+(y0*1024)];

	GPU_GP1 |= 0x08000000;
}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuStoreImage(void)
{
	u16 x0, y0, w0, h0;
	x0 = PacketBuffer.U2[2] & 1023;
	y0 = PacketBuffer.U2[3] & 511;
	w0 = PacketBuffer.U2[4];
	h0 = PacketBuffer.U2[5];

	if ((y0 + h0) > FRAME_HEIGHT)
	{
		h0 = FRAME_HEIGHT - y0;
	}
	FrameToRead = ((w0)&&(h0));

	px = 0;
	py = 0;
	x_end = w0;
	y_end = h0;
	pvram = &((u16*)GPU_FrameBuffer)[x0+(y0*1024)];
	
	GPU_GP1 |= 0x08000000;
}

INLINE void gpuMoveImage(void)
{
	u32 x0, y0, x1, y1;
	s32 w0, h0;
	x0 = PacketBuffer.U2[2] & 1023;
	y0 = PacketBuffer.U2[3] & 511;
	x1 = PacketBuffer.U2[4] & 1023;
	y1 = PacketBuffer.U2[5] & 511;
	w0 = PacketBuffer.U2[6];
	h0 = PacketBuffer.U2[7];

	if( (x0==x1) && (y0==y1) ) return;
	if ((w0<=0) || (h0<=0)) return;
	
	if (((y0+h0)>512)||((x0+w0)>1024)||((y1+h0)>512)||((x1+w0)>1024))
	{
		u16 *psxVuw=GPU_FrameBuffer;
		s32 i,j;
	    for(j=0;j<h0;j++)
		 for(i=0;i<w0;i++)
		  psxVuw [(1024*((y1+j)&511))+((x1+i)&0x3ff)]=
		   psxVuw[(1024*((y0+j)&511))+((x0+i)&0x3ff)];
	}
	else if ((x0&1)||(x1&1))
	{
		u16 *lpDst, *lpSrc;
		lpDst = lpSrc = (u16*)GPU_FrameBuffer;
		lpSrc += FRAME_OFFSET(x0, y0);
		lpDst += FRAME_OFFSET(x1, y1);
		x1 = FRAME_WIDTH - w0;
		do {
			x0=w0;
			do { *lpDst++ = *lpSrc++; } while (--x0);
			lpDst += x1;
			lpSrc += x1;
		} while (--h0);
	}
	else
	{
		u32 *lpDst, *lpSrc;
		lpDst = lpSrc = (u32*)(void*)GPU_FrameBuffer;
		lpSrc += ((FRAME_OFFSET(x0, y0))>>1);
		lpDst += ((FRAME_OFFSET(x1, y1))>>1);
		if (w0&1)
		{
			x1 = (FRAME_WIDTH - w0 +1)>>1;
			w0>>=1;
			if (!w0) {
				do {
					*((u16*)lpDst) = *((u16*)lpSrc);
					lpDst += x1;
					lpSrc += x1;
				} while (--h0);
			} else
			do {
				x0=w0;
				do { *lpDst++ = *lpSrc++; } while (--x0);
				*((u16*)lpDst) = *((u16*)lpSrc);
				lpDst += x1;
				lpSrc += x1;
			} while (--h0);
		}
		else
		{
			x1 = (FRAME_WIDTH - w0)>>1;
			w0>>=1;
			do {
				x0=w0;
				do { *lpDst++ = *lpSrc++; } while (--x0);
				lpDst += x1;
				lpSrc += x1;
			} while (--h0);
		}
	}
}

INLINE void gpuClearImage(void)
{
	s32   x0, y0, w0, h0;
	x0 = PacketBuffer.S2[2];
	y0 = PacketBuffer.S2[3];
	w0 = PacketBuffer.S2[4] & 0x3ff;
	h0 = PacketBuffer.S2[5] & 0x3ff;
	 
	x0 &= ~0xF;
	w0 = ((w0 + 0xF) & ~0xF);

	w0 += x0;
	if (x0 < 0) x0 = 0;
	if (w0 > FRAME_WIDTH) w0 = FRAME_WIDTH;
	w0 -= x0;
	if (w0 <= 0) return;
	h0 += y0;
	if (y0 < 0) y0 = 0;
	if (h0 > FRAME_HEIGHT) h0 = FRAME_HEIGHT;
	h0 -= y0;
	if (h0 <= 0) return;

	{
		u32* pixel = (u32*)(void*)GPU_FrameBuffer + ((FRAME_OFFSET(x0, y0))>>1);
		u32 rgb = GPU_RGB16(PacketBuffer.S4[0]);
		rgb |= (rgb<<16);
		{
			y0 = (FRAME_WIDTH - w0)>>1;
			w0>>=3;
			do {
				x0=w0;
				do {
					pixel[0] = rgb;
					pixel[1] = rgb;
					pixel[2] = rgb;
					pixel[3] = rgb;
					pixel += 4;
				} while (--x0);
				pixel += y0;
			} while (--h0);
		}
	}
}
