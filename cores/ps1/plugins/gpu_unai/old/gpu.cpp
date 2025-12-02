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

#include "port.h"
#include "gpu.h"
#include "profiler.h"
#include "debug.h"

int skipCount = 2; /* frame skip (0,1,2,3...) */
int skCount = 0; /* internal frame skip */
int linesInterlace = 0;  /* internal lines interlace */
int linesInterlace_user = 0; /* Lines interlace */

bool isSkip = false; /* skip frame (info coming from GPU) */
bool wasSkip = false;
bool skipFrame = false; /* skip frame (according to frame skip) */
bool alt_fps = false; /* Alternative FPS algorithm */
bool show_fps = false; /* Show FPS statistics */

bool isPAL = false; /* PAL video timing */
bool progressInterlace_flag = false; /* Progressive interlace flag */
bool progressInterlace = false; /* Progressive interlace option*/
bool frameLimit = false; /* frames to wait */

bool light = true; /* lighting */
bool blend = true; /* blending */
bool FrameToRead = false; /* load image in progress */
bool FrameToWrite = false; /* store image in progress */
bool fb_dirty = false;

bool enableAbbeyHack = false; /* Abe's Odyssey hack */

u8 BLEND_MODE;
u8 TEXT_MODE;
u8 Masking;

u16 PixelMSB;
u16 PixelData;

///////////////////////////////////////////////////////////////////////////////
//  GPU Global data
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Dma Transfers info
s32		px,py;
s32		x_end,y_end;
u16*  pvram;

u32 GP0;
s32 PacketCount;
s32 PacketIndex;

///////////////////////////////////////////////////////////////////////////////
//  Display status
u32 DisplayArea   [6];

///////////////////////////////////////////////////////////////////////////////
//  Rasterizer status
u32 TextureWindow [4];
u32 DrawingArea   [4];
u32 DrawingOffset [2];

///////////////////////////////////////////////////////////////////////////////
//  Rasterizer status

u16* TBA;
u16* CBA;

///////////////////////////////////////////////////////////////////////////////
//  Inner Loops
s32   u4, du4;
s32   v4, dv4;
s32   r4, dr4;
s32   g4, dg4;
s32   b4, db4;
u32   lInc;
u32   tInc, tMsk;

GPUPacket PacketBuffer;
// FRAME_BUFFER_SIZE is defined in bytes; 512K is guard memory for out of range reads
u16   GPU_FrameBuffer[(FRAME_BUFFER_SIZE+512*1024)/2] __attribute__((aligned(2048)));
u32   GPU_GP1;

///////////////////////////////////////////////////////////////////////////////
//  Inner loop driver instanciation file
#include "gpu_inner.h"

///////////////////////////////////////////////////////////////////////////////
//  GPU Raster Macros
#define	GPU_RGB16(rgb)        ((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

#define GPU_EXPANDSIGN(x)  (((s32)(x)<<21)>>21)

#define CHKMAX_X 1024
#define CHKMAX_Y 512

#define	GPU_SWAP(a,b,t)	{(t)=(a);(a)=(b);(b)=(t);}

///////////////////////////////////////////////////////////////////////////////
// GPU internal image drawing functions
#include "gpu_raster_image.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal line drawing functions
#include "gpu_raster_line.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal polygon drawing functions
#include "gpu_raster_polygon.h"

///////////////////////////////////////////////////////////////////////////////
// GPU internal sprite drawing functions
#include "gpu_raster_sprite.h"

///////////////////////////////////////////////////////////////////////////////
// GPU command buffer execution/store
#include "gpu_command.h"

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuReset(void)
{
	GPU_GP1 = 0x14802000;
	TextureWindow[0] = 0;
	TextureWindow[1] = 0;
	TextureWindow[2] = 255;
	TextureWindow[3] = 255;
	DrawingArea[2] = 256;
	DrawingArea[3] = 240;
	DisplayArea[2] = 256;
	DisplayArea[3] = 240;
	DisplayArea[5] = 240;
}

///////////////////////////////////////////////////////////////////////////////
bool  GPU_init(void)
{
	gpuReset();
	
	// s_invTable
	for(int i=1;i<=(1<<TABLE_BITS);++i)
	{
		double v = 1.0 / double(i);
		#ifdef GPU_TABLE_10_BITS
		v *= double(0xffffffff>>1);
		#else
		v *= double(0x80000000);
		#endif
		s_invTable[i-1]=s32(v);
	}
	return (0);
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_shutdown(void)
{
}

///////////////////////////////////////////////////////////////////////////////
long  GPU_freeze(unsigned int bWrite, GPUFreeze_t* p2)
{
	if (!p2) return (0);
	if (p2->Version != 1) return (0);

	if (bWrite)
	{
		p2->GPU_gp1 = GPU_GP1;
		memset(p2->Control, 0, sizeof(p2->Control));
		// save resolution and registers for P.E.Op.S. compatibility
		p2->Control[3] = (3 << 24) | ((GPU_GP1 >> 23) & 1);
		p2->Control[4] = (4 << 24) | ((GPU_GP1 >> 29) & 3);
		p2->Control[5] = (5 << 24) | (DisplayArea[0] | (DisplayArea[1] << 10));
		p2->Control[6] = (6 << 24) | (2560 << 12);
		p2->Control[7] = (7 << 24) | (DisplayArea[4] | (DisplayArea[5] << 10));
		p2->Control[8] = (8 << 24) | ((GPU_GP1 >> 17) & 0x3f) | ((GPU_GP1 >> 10) & 0x40);
		memcpy(p2->FrameBuffer, (u16*)GPU_FrameBuffer, FRAME_BUFFER_SIZE);
		return (1);
	}
	else
	{
		GPU_GP1 = p2->GPU_gp1;
		memcpy((u16*)GPU_FrameBuffer, p2->FrameBuffer, FRAME_BUFFER_SIZE);
		GPU_writeStatus((5 << 24) | p2->Control[5]);
		GPU_writeStatus((7 << 24) | p2->Control[7]);
		GPU_writeStatus((8 << 24) | p2->Control[8]);
		gpuSetTexture(GPU_GP1);
		return (1);
	}
	return (0);
}

///////////////////////////////////////////////////////////////////////////////
//  GPU DMA comunication

///////////////////////////////////////////////////////////////////////////////
u8 PacketSize[256] =
{
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		0-15
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		16-31
	3, 3, 3, 3, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 8, 8,	//		32-47
	5, 5, 5, 5, 8, 8, 8, 8, 7, 7, 7, 7, 11, 11, 11, 11,	//	48-63
	2, 2, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,	//		64-79
	3, 3, 3, 3, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4,	//		80-95
	2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2,	//		96-111
	1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,	//		112-127
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		128-
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		144
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	//
};

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuSendPacket()
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_sendPacket++;
#endif
	gpuSendPacketFunction(PacketBuffer.U4[0]>>24);
}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuCheckPacket(u32 uData)
{
	if (PacketCount)
	{
		PacketBuffer.U4[PacketIndex++] = uData;
		--PacketCount;
	}
	else
	{
		PacketBuffer.U4[0] = uData;
		PacketCount = PacketSize[uData >> 24];
		PacketIndex = 1;
	}
	if (!PacketCount) gpuSendPacket();
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_writeDataMem(u32* dmaAddress, s32 dmaCount)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_writeDataMem++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	u32 data;
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
	GPU_GP1 &= ~0x14000000;

	while (dmaCount) 
	{
		if (FrameToWrite) 
		{
			while (dmaCount)
			{
				dmaCount--;
				data = *dmaAddress++;
				if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
				pvram[px] = data;
				if (++px>=x_end) 
				{
					px = 0;
					pvram += 1024;
					if (++py>=y_end) 
					{
						FrameToWrite = false;
						GPU_GP1 &= ~0x08000000;
						break;
					}
				}
				if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
				pvram[px] = data>>16;
				if (++px>=x_end) 
				{
					px = 0;
					pvram += 1024;
					if (++py>=y_end) 
					{
						FrameToWrite = false;
						GPU_GP1 &= ~0x08000000;
						break;
					}
				}
			}
		}
		else
		{
			data = *dmaAddress++;
			dmaCount--;
			gpuCheckPacket(data);
		}
	}

	GPU_GP1 = (GPU_GP1 | 0x14000000) & ~0x60000000;
	fb_dirty = true;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
}

u32 *lUsedAddr[3];
INLINE int CheckForEndlessLoop(u32 *laddr)
{
	if(laddr==lUsedAddr[1]) return 1;
	if(laddr==lUsedAddr[2]) return 1;

	if(laddr<lUsedAddr[0]) lUsedAddr[1]=laddr;
	else                   lUsedAddr[2]=laddr;
	lUsedAddr[0]=laddr;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
long GPU_dmaChain(u32* baseAddr, u32 dmaVAddr)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_dmaChain++;
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	u32 data, *address, count, offset;
	unsigned int DMACommandCounter = 0;
	long dma_words = 0;

	GPU_GP1 &= ~0x14000000;
	lUsedAddr[0]=lUsedAddr[1]=lUsedAddr[2]=(u32*)0x1fffff;
	dmaVAddr &= 0x001FFFFF;
	while (dmaVAddr != 0x1FFFFF)
	{
		address = (baseAddr + (dmaVAddr >> 2));
		if(DMACommandCounter++ > 2000000) break;
		if(CheckForEndlessLoop(address)) break;
		data = *address++;
		count = (data >> 24);
		offset = data & 0x001FFFFF;
		if (dmaVAddr != offset) dmaVAddr = offset;
		else dmaVAddr = 0x1FFFFF;

		if(count>0) GPU_writeDataMem(address,count);
		dma_words += 1 + count;
	}
	GPU_GP1 = (GPU_GP1 | 0x14000000) & ~0x60000000;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);

	return dma_words;
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_writeData(u32 data)
{
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_writeData++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	GPU_GP1 &= ~0x14000000;

	if (FrameToWrite)
	{
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		pvram[px]=(u16)data;
		if (++px>=x_end)
		{
			px = 0;
			pvram += 1024;
			if (++py>=y_end) 
			{
				FrameToWrite = false;
				GPU_GP1 &= ~0x08000000;
			}
		}
		if (FrameToWrite)
		{
			if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
			pvram[px]=data>>16;
			if (++px>=x_end)
			{
				px = 0;
				pvram += 1024;
				if (++py>=y_end) 
				{
					FrameToWrite = false;
					GPU_GP1 &= ~0x08000000;
				}
			}
		}
	}
	else
	{
		gpuCheckPacket(data);
	}
	GPU_GP1 |= 0x14000000;
	fb_dirty = true;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);

}


///////////////////////////////////////////////////////////////////////////////
void  GPU_readDataMem(u32* dmaAddress, s32 dmaCount)
{
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_readDataMem++;
#endif
	if(!FrameToRead) return;

	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	GPU_GP1 &= ~0x14000000;
	do 
	{
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		// lower 16 bit
		u32 data = pvram[px];

		if (++px>=x_end) 
		{
			px = 0;
			pvram += 1024;
		}

		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		// higher 16 bit (always, even if it's an odd width)
		data |= (u32)(pvram[px])<<16;
		
		*dmaAddress++ = data;

		if (++px>=x_end) 
		{
			px = 0;
			pvram += 1024;
			if (++py>=y_end) 
			{
				FrameToRead = false;
				GPU_GP1 &= ~0x08000000;
				break;
			}
		}
	} while (--dmaCount);

	GPU_GP1 = (GPU_GP1 | 0x14000000) & ~0x60000000;
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
}



///////////////////////////////////////////////////////////////////////////////
u32  GPU_readData(void)
{
	const u16 *VIDEO_END=(GPU_FrameBuffer+(FRAME_BUFFER_SIZE/2)-1);
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_readData++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_READ);
	GPU_GP1 &= ~0x14000000;
	if (FrameToRead)
	{
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		GP0 = pvram[px];
		if (++px>=x_end)
		{
			px = 0;
			pvram += 1024;
			if (++py>=y_end) 
			{
				FrameToRead = false;
				GPU_GP1 &= ~0x08000000;
			}
		}
		if ((&pvram[px])>(VIDEO_END)) pvram-=512*1024;
		GP0 |= pvram[px]<<16;
		if (++px>=x_end)
		{
			px = 0;
			pvram +=1024;
			if (++py>=y_end) 
			{
				FrameToRead = false;
				GPU_GP1 &= ~0x08000000;
			}
		}

	}
	GPU_GP1 |= 0x14000000;

	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_READ);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
	return (GP0);
}

///////////////////////////////////////////////////////////////////////////////
u32     GPU_readStatus(void)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_readStatus++;
#endif
	return GPU_GP1;
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_writeStatus(u32 data)
{
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_writeStatus++;
#endif
	pcsx4all_prof_pause(PCSX4ALL_PROF_CPU);
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	switch (data >> 24) {
	case 0x00:
		gpuReset();
		break;
	case 0x01:
		GPU_GP1 &= ~0x08000000;
		PacketCount = 0; FrameToRead = FrameToWrite = false;
		break;
	case 0x02:
		GPU_GP1 &= ~0x08000000;
		PacketCount = 0; FrameToRead = FrameToWrite = false;
		break;
	case 0x03:
		GPU_GP1 = (GPU_GP1 & ~0x00800000) | ((data & 1) << 23);
		break;
	case 0x04:
		if (data == 0x04000000)
		PacketCount = 0;
		GPU_GP1 = (GPU_GP1 & ~0x60000000) | ((data & 3) << 29);
		break;
	case 0x05:
		DisplayArea[0] = (data & 0x000003FF); //(short)(data & 0x3ff);
		DisplayArea[1] = ((data & 0x0007FC00)>>10); //(data & 0x000FFC00) >> 10; //(short)((data>>10)&0x1ff);
		fb_dirty = true;
		wasSkip = isSkip;
		if (isSkip)
			isSkip = false;
		else
			isSkip = skipFrame;
		break;
	case 0x07:
		DisplayArea[4] = data & 0x000003FF; //(short)(data & 0x3ff);
		DisplayArea[5] = (data & 0x000FFC00) >> 10; //(short)((data>>10) & 0x3ff);
		fb_dirty = true;
		break;
	case 0x08:
		{
			GPU_GP1 = (GPU_GP1 & ~0x007F0000) | ((data & 0x3F) << 17) | ((data & 0x40) << 10);
			static u32 HorizontalResolution[8] = { 256, 368, 320, 384, 512, 512, 640, 640 };
			DisplayArea[2] = HorizontalResolution[(GPU_GP1 >> 16) & 7];
			static u32 VerticalResolution[4] = { 240, 480, 256, 480 };
			DisplayArea[3] = VerticalResolution[(GPU_GP1 >> 19) & 3];
			isPAL = (data & 0x08) ? true : false; // if 1 - PAL mode, else NTSC
		}
		fb_dirty = true;
		break;
	case 0x10:
		switch (data & 0xffff) {
		case 0:
		case 1:
		case 3:
			GP0 = (DrawingArea[1] << 10) | DrawingArea[0];
			break;
		case 4:
			GP0 = ((DrawingArea[3]-1) << 10) | (DrawingArea[2]-1);
			break;
		case 6:
		case 5:
			GP0 = (DrawingOffset[1] << 11) | DrawingOffset[0];
			break;
		case 7:
			GP0 = 2;
			break;
		default:
			GP0 = 0;
		}
		break;
	}
	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_HW_WRITE);
	pcsx4all_prof_resume(PCSX4ALL_PROF_CPU);
}

#ifndef REARMED

// Blitting functions
#include "gpu_blit.h"

INLINE void gpuVideoOutput(void)
{
	static s16 old_res_horz, old_res_vert, old_rgb24;
	s16 h0, x0, y0, w0, h1;

	x0 = DisplayArea[0];
	y0 = DisplayArea[1];

	w0 = DisplayArea[2];
	h0 = DisplayArea[3];  // video mode

	h1 = DisplayArea[5] - DisplayArea[4]; // display needed
	if (h0 == 480) h1 = Min2(h1*2,480);

	u16* dest_screen16 = SCREEN;
	u16* src_screen16  = &((u16*)GPU_FrameBuffer)[FRAME_OFFSET(x0,y0)];
	u32 isRGB24 = (GPU_GP1 & 0x00200000 ? 32 : 0);

	/* Clear the screen if resolution changed to prevent interlacing and clipping to clash */
	if( (w0 != old_res_horz || h1 != old_res_vert || (s16)isRGB24 != old_rgb24) )
	{
		// Update old resolution
		old_res_horz = w0;
		old_res_vert = h1;
		old_rgb24 = (s16)isRGB24;
		// Finally, clear the screen for this special case
		video_clear();
	}

	//  Height centering
	int sizeShift = 1;
	if(h0==256) h0 = 240; else if(h0==480) sizeShift = 2;
	if(h1>h0) { src_screen16 += ((h1-h0)>>sizeShift)*1024; h1 = h0; }
	else if(h1<h0) dest_screen16 += ((h0-h1)>>sizeShift)*VIDEO_WIDTH;

	/* Main blitter */
	int incY = (h0==480) ? 2 : 1;
	h0=(h0==480 ? 2048 : 1024);

	{
		const int li=linesInterlace;
		bool pi=progressInterlace;
		bool pif=progressInterlace_flag;
		switch ( w0 )
		{
			case 256:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWDWW(	src_screen16,	dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 368:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWWWWWWWS(	src_screen16,	dest_screen16, isRGB24, 4);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 320:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWW(	src_screen16,	dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 384:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWWWWS(	src_screen16,	dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 512:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWWSWWSWS(	src_screen16, dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
			case 640:
				for(int y1=y0+h1; y0<y1; y0+=incY)
				{
					if(( 0 == (y0&li) ) && ((!pi) || (pif=!pif))) GPU_BlitWS(	src_screen16, dest_screen16, isRGB24);
					dest_screen16 += VIDEO_WIDTH;
					src_screen16  += h0;
				}
				break;
		}
		progressInterlace_flag=!progressInterlace_flag;
	}
	video_flip();
}

///////////////////////////////////////////////////////////////////////////////
void  GPU_updateLace(void)
{
#ifdef  ENABLE_GPU_LOG_SUPPORT
	fprintf(stdout,"GPU_updateLace()\n");
#endif
#ifdef DEBUG_ANALYSIS
	dbg_anacnt_GPU_updateLace++;
#endif
	pcsx4all_prof_start_with_pause(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_COUNTERS);
#ifdef PROFILER_PCSX4ALL
	pcsx4all_prof_frames++;
#endif
#ifdef DEBUG_FRAME
	if(isdbg_frame())
	{
		static int passed=0;
		if (!passed) dbg_enable();
		else pcsx4all_exit();
		passed++;
	}
#endif

	// Frame skip table
	static const unsigned char skipTable[12][12] =
	{
		{ 0,0,0,0,0,0,0,0,0,0,0,0 },
		{ 0,0,0,0,0,0,0,0,0,0,0,1 },
		{ 0,0,0,0,0,1,0,0,0,0,0,1 },
		{ 0,0,0,1,0,0,0,1,0,0,0,1 },
		{ 0,0,1,0,0,1,0,0,1,0,0,1 },
		{ 0,1,0,0,1,0,1,0,0,1,0,1 },
		{ 0,1,0,1,0,1,0,1,0,1,0,1 },
		{ 0,1,0,1,1,0,1,0,1,1,0,1 },
		{ 0,1,1,0,1,1,0,1,1,0,1,1 },
		{ 0,1,1,1,0,1,1,1,0,1,1,1 },
		{ 0,1,1,1,1,1,0,1,1,1,1,1 },
		{ 0,1,1,1,1,1,1,1,1,1,1,1 }
	};
	
	// Interlace bit toggle
	GPU_GP1 ^= 0x80000000;

	// Update display
	if ((!skipFrame) && (!isSkip) && (fb_dirty) && (!(((GPU_GP1&0x08000000))||((GPU_GP1&0x00800000)))))
	{
		gpuVideoOutput(); // Display updated

		if (DisplayArea[3] == 480)
		{
			if (linesInterlace_user) linesInterlace = 3; // 1/4 of lines
			else linesInterlace = 1; // if 480 we only need half of lines
		}
		else if (linesInterlace != linesInterlace_user)
		{
			linesInterlace = linesInterlace_user; // resolution changed from 480 to lower one
			video_clear();
		}
	}

	// Limit FPS
	if (frameLimit)
	{
		static unsigned next=get_ticks();
		if (!skipFrame)
		{
			unsigned now=get_ticks();
			if (now<next) wait_ticks(next-now);
		}
		next+=(isPAL?(1000000/50):((unsigned)(1000000.0/59.94)));
	}

	// Show FPS statistics
	if (show_fps)
	{
		static u32 real_fps=0;
		static u32 prev=get_ticks();
		static char msg[32]="FPS=000/00 SPD=000%";
		u32 now=get_ticks();
		real_fps++;
		if ((now-prev)>=1000000)
		{
			u32 expected_fps=(isPAL?50:60);
			sprintf(msg,"FPS=%3d/%2d SPD=%3d%%",((real_fps*(12-skipCount))/12),((expected_fps*(12-skipCount))/12),((real_fps*100)/expected_fps));
			prev=now;
			real_fps=0;
		}
		port_printf(5,5,msg);
	}

	// Update frame-skip
	if (!alt_fps)
	{
		// Video frame-skip
		skipFrame=skipTable[skipCount][skCount];
		skCount--; if (skCount<0) skCount=11;
		isSkip=skipFrame;
	}
	else
	{
		// Game frame-skip
		if (!isSkip)
		{
			skipFrame=skipTable[skipCount][skCount];
			skCount--; if (skCount<0) skCount=11;
			isSkip=true;
		}
	}
	fb_dirty=false;

	pcsx4all_prof_end_with_resume(PCSX4ALL_PROF_GPU,PCSX4ALL_PROF_COUNTERS);
}

#else

#include "../../frontend/plugin_lib.h"

extern "C" {

static const struct rearmed_cbs *cbs;
static s16 old_res_horz, old_res_vert, old_rgb24;

static void blit(void)
{
	u16 *base = (u16 *)GPU_FrameBuffer;
	s16 isRGB24 = (GPU_GP1 & 0x00200000) ? 1 : 0;
	s16 h0, x0, y0, w0, h1;

	x0 = DisplayArea[0] & ~1; // alignment needed by blitter
	y0 = DisplayArea[1];
	base += FRAME_OFFSET(x0, y0);

	w0 = DisplayArea[2];
	h0 = DisplayArea[3];  // video mode

	h1 = DisplayArea[5] - DisplayArea[4]; // display needed
	if (h0 == 480) h1 = Min2(h1*2,480);

	if (h1 <= 0)
		return;

	if (w0 != old_res_horz || h1 != old_res_vert || isRGB24 != old_rgb24)
	{
		old_res_horz = w0;
		old_res_vert = h1;
		old_rgb24 = (s16)isRGB24;
		cbs->pl_vout_set_mode(w0, h1, w0, h1, isRGB24 ? 24 : 16);
	}

#error out of date
	cbs->pl_vout_flip(base, 1024, isRGB24, w0, h1);
}

void GPU_updateLace(void)
{
	// Interlace bit toggle
	GPU_GP1 ^= 0x80000000;

	if (!fb_dirty || (GPU_GP1&0x08800000))
		return;

	if (!wasSkip) {
		blit();
		fb_dirty = false;
		skCount = 0;
	}
	else {
		skCount++;
		if (skCount >= 8)
			wasSkip = isSkip = 0;
	}

	skipFrame = cbs->fskip_advice || cbs->frameskip == 1;
}

long GPUopen(unsigned long *, char *, char *)
{
	cbs->pl_vout_open();
	return 0;
}

long GPUclose(void)
{
	cbs->pl_vout_close();
	return 0;
}

long GPUfreeze(unsigned int ulGetFreezeData, GPUFreeze_t* p2)
{
	if (ulGetFreezeData > 1)
		return 0;

	return GPU_freeze(ulGetFreezeData, p2);
}

void GPUrearmedCallbacks(const struct rearmed_cbs *cbs_)
{
	enableAbbeyHack = cbs_->gpu_unai.abe_hack;
	light = !cbs_->gpu_unai.no_light;
	blend = !cbs_->gpu_unai.no_blend;
	if (cbs_->pl_vout_set_raw_vram)
		cbs_->pl_vout_set_raw_vram((void *)GPU_FrameBuffer);

	cbs = cbs_;
	if (cbs->pl_set_gpu_caps)
		cbs->pl_set_gpu_caps(0);
}

} /* extern "C" */

#endif
