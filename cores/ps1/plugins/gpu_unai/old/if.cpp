/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2011 notaz                                              *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../gpulib/gpu.h"
#include "arm_features.h"
#include "if.h"

#define u8 uint8_t
#define s8 int8_t
#define u16 uint16_t
#define s16 int16_t
#define u32 uint32_t
#define s32 int32_t
#define s64 int64_t

#define INLINE static

#define	FRAME_BUFFER_SIZE  (1024*512*2)
#define	FRAME_WIDTH        1024
#define	FRAME_HEIGHT       512
#define	FRAME_OFFSET(x,y)  (((y)<<10)+(x))

#define isSkip 0 /* skip frame (info coming from GPU) */
#define alt_fps 0
static int linesInterlace;  /* internal lines interlace */
static int force_interlace;

static bool light = true; /* lighting */
static bool blend = true; /* blending */
static bool FrameToRead = false; /* load image in progress */
static bool FrameToWrite = false; /* store image in progress */

//static bool enableAbbeyHack = false; /* Abe's Odyssey hack */
#define enableAbbeyHack false

static u8 BLEND_MODE;
static u8 TEXT_MODE;
static u8 Masking;

static u16 PixelMSB;
static u16 PixelData;

///////////////////////////////////////////////////////////////////////////////
//  GPU Global data
///////////////////////////////////////////////////////////////////////////////

//  Dma Transfers info
static s32		px,py;
static s32		x_end,y_end;
static u16*  pvram;

static s32 PacketCount;
static s32 PacketIndex;

//  Rasterizer status
static u32 TextureWindow [4];
static u32 DrawingArea   [4];
static u32 DrawingOffset [2];

static u16* TBA;
static u16* CBA;

//  Inner Loops
static s32   u4, du4;
static s32   v4, dv4;
static s32   r4, dr4;
static s32   g4, dg4;
static s32   b4, db4;
static u32   lInc;
static u32   tInc, tMsk;

union GPUPacket
{
	u32 U4[16];
	s32 S4[16];
	u16 U2[32];
	s16 S2[32];
	u8  U1[64];
	s8  S1[64];
};

static GPUPacket PacketBuffer;
static u16  *GPU_FrameBuffer;
static u32   GPU_GP1;

///////////////////////////////////////////////////////////////////////////////

#include "gpu_fixedpoint.h"

//  Inner loop driver instanciation file
#include "gpu_inner.h"

//  GPU Raster Macros
#define	GPU_RGB16(rgb)        ((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

#define GPU_EXPANDSIGN(x)  (((s32)(x)<<21)>>21)

#define CHKMAX_X 1024
#define CHKMAX_Y 512

#define	GPU_SWAP(a,b,t)	{(t)=(a);(a)=(b);(b)=(t);}

// GPU internal image drawing functions
#include "gpu_raster_image.h"

// GPU internal line drawing functions
#include "gpu_raster_line.h"

// GPU internal polygon drawing functions
#include "gpu_raster_polygon.h"

// GPU internal sprite drawing functions
#include "gpu_raster_sprite.h"

// GPU command buffer execution/store
#include "gpu_command.h"

/////////////////////////////////////////////////////////////////////////////

void oldunai_renderer_init(void)
{
	GPU_FrameBuffer = (u16 *)gpu.vram;

#if 0 // shared with "new" unai
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
#endif
}

extern const unsigned char cmd_lengths[256];

int oldunai_do_cmd_list(uint32_t *list, int list_len,
 int *cycles_sum_out, int *cycles_last, int *last_cmd)
{
  unsigned int cmd = 0, len, i;
  uint32_t *list_start = list;
  uint32_t *list_end = list + list_len;

  linesInterlace = force_interlace;
#ifdef HAVE_PRE_ARMV7 /* XXX */
  linesInterlace |= !!(gpu.status & PSX_GPU_STATUS_INTERLACE);
#endif

  for (; list < list_end; list += 1 + len)
  {
    cmd = *list >> 24;
    len = cmd_lengths[cmd];
    if (list + 1 + len > list_end) {
      cmd = -1;
      break;
    }

    #define PRIM cmd
    PacketBuffer.U4[0] = list[0];
    for (i = 1; i <= len; i++)
      PacketBuffer.U4[i] = list[i];

    switch (cmd)
    {
      case 0x02:
        gpuClearImage();
        break;

      case 0x20:
      case 0x21:
      case 0x22:
      case 0x23:
        gpuDrawF3(gpuPolySpanDrivers [Blending_Mode | Masking | Blending | PixelMSB]);
        break;

      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
        gpuSetCLUT   (PacketBuffer.U4[2] >> 16);
        gpuSetTexture(PacketBuffer.U4[4] >> 16);
        if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
          gpuDrawFT3(gpuPolySpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | PixelMSB]);
        else
          gpuDrawFT3(gpuPolySpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | PixelMSB]);
        break;

      case 0x28:
      case 0x29:
      case 0x2A:
      case 0x2B: {
        const PP gpuPolySpanDriver = gpuPolySpanDrivers [Blending_Mode | Masking | Blending | PixelMSB];
        gpuDrawF3(gpuPolySpanDriver);
        PacketBuffer.U4[1] = PacketBuffer.U4[4];
        gpuDrawF3(gpuPolySpanDriver);
        break;
      }

      case 0x2C:
      case 0x2D:
      case 0x2E:
      case 0x2F: {
        gpuSetCLUT   (PacketBuffer.U4[2] >> 16);
        gpuSetTexture(PacketBuffer.U4[4] >> 16);
        PP gpuPolySpanDriver;
        if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
          gpuPolySpanDriver = gpuPolySpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | PixelMSB];
        else
          gpuPolySpanDriver = gpuPolySpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | PixelMSB];
        gpuDrawFT3(gpuPolySpanDriver);
        PacketBuffer.U4[1] = PacketBuffer.U4[7];
        PacketBuffer.U4[2] = PacketBuffer.U4[8];
        gpuDrawFT3(gpuPolySpanDriver);
        break;
      }

      case 0x30:
      case 0x31:
      case 0x32:
      case 0x33:
        gpuDrawG3(gpuPolySpanDrivers [Blending_Mode | Masking | Blending | 129 | PixelMSB]);
        break;

      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
        gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
        gpuSetTexture (PacketBuffer.U4[5] >> 16);
        gpuDrawGT3(gpuPolySpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | ((Lighting)?129:0) | PixelMSB]);
        break;

      case 0x38:
      case 0x39:
      case 0x3A:
      case 0x3B: {
        const PP gpuPolySpanDriver  = gpuPolySpanDrivers [Blending_Mode | Masking | Blending | 129 | PixelMSB];
        gpuDrawG3(gpuPolySpanDriver);
        PacketBuffer.U4[0] = PacketBuffer.U4[6];
        PacketBuffer.U4[1] = PacketBuffer.U4[7];
        gpuDrawG3(gpuPolySpanDriver);
        break;
      }

      case 0x3C:
      case 0x3D:
      case 0x3E:
      case 0x3F: {
        gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
        gpuSetTexture (PacketBuffer.U4[5] >> 16);
        const PP gpuPolySpanDriver  = gpuPolySpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | ((Lighting)?129:0) | PixelMSB];
        gpuDrawGT3(gpuPolySpanDriver);
        PacketBuffer.U4[0] = PacketBuffer.U4[9];
        PacketBuffer.U4[1] = PacketBuffer.U4[10];
        PacketBuffer.U4[2] = PacketBuffer.U4[11];
        gpuDrawGT3(gpuPolySpanDriver);
        break;
      }

      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
        gpuDrawLF(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);
        break;

      case 0x48 ... 0x4F:
      {
        u32 num_vertexes = 1;
        u32 *list_position = &(list[2]);

        gpuDrawLF(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);

        while(1)
        {
          PacketBuffer.U4[1] = PacketBuffer.U4[2];
          PacketBuffer.U4[2] = *list_position++;
          gpuDrawLF(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);

          num_vertexes++;
          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }
          if((*list_position & 0xf000f000) == 0x50005000)
            break;
        }

        len += (num_vertexes - 2);
        break;
      }

      case 0x50:
      case 0x51:
      case 0x52:
      case 0x53:
        gpuDrawLG(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);
        break;

      case 0x58 ... 0x5F:
      {
        u32 num_vertexes = 1;
        u32 *list_position = &(list[2]);

        gpuDrawLG(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);

        while(1)
        {
          PacketBuffer.U4[0] = PacketBuffer.U4[2];
          PacketBuffer.U4[1] = PacketBuffer.U4[3];
          PacketBuffer.U4[2] = *list_position++;
          PacketBuffer.U4[3] = *list_position++;
          gpuDrawLG(gpuPixelDrivers [ (Blending_Mode | Masking | Blending | (PixelMSB>>3)) >> 1]);

          num_vertexes++;
          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }
          if((*list_position & 0xf000f000) == 0x50005000)
            break;
        }

        len += (num_vertexes - 2) * 2;
        break;
      }

      case 0x60:
      case 0x61:
      case 0x62:
      case 0x63:
        gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
        break;

      case 0x64:
      case 0x65:
      case 0x66:
      case 0x67:
        gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
        gpuSetTexture (GPU_GP1);
        if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
          gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | (enableAbbeyHack<<7)  | PixelMSB]);
        else
          gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | (enableAbbeyHack<<7)  | PixelMSB]);
        break;

      case 0x68:
      case 0x69:
      case 0x6A:
      case 0x6B:
        PacketBuffer.U4[2] = 0x00010001;
        gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
        break;

      case 0x70:
      case 0x71:
      case 0x72:
      case 0x73:
        PacketBuffer.U4[2] = 0x00080008;
        gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
        break;

      case 0x74:
      case 0x75:
      case 0x76:
      case 0x77:
        PacketBuffer.U4[3] = 0x00080008;
        gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
        gpuSetTexture (GPU_GP1);
        if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
          gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | (enableAbbeyHack<<7)  | PixelMSB]);
        else
          gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | (enableAbbeyHack<<7)  | PixelMSB]);
        break;

      case 0x78:
      case 0x79:
      case 0x7A:
      case 0x7B:
        PacketBuffer.U4[2] = 0x00100010;
        gpuDrawT(gpuTileSpanDrivers [Blending_Mode | Masking | Blending | (PixelMSB>>3)]);
        break;

      case 0x7C:
      case 0x7D:
#ifdef __arm__
        if ((GPU_GP1 & 0x180) == 0 && (Masking | PixelMSB) == 0)
        {
          gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
          gpuSetTexture (GPU_GP1);
          gpuDrawS16();
          break;
        }
        // fallthrough
#endif
      case 0x7E:
      case 0x7F:
        PacketBuffer.U4[3] = 0x00100010;
        gpuSetCLUT    (PacketBuffer.U4[2] >> 16);
        gpuSetTexture (GPU_GP1);
        if ((PacketBuffer.U1[0]>0x5F) && (PacketBuffer.U1[1]>0x5F) && (PacketBuffer.U1[2]>0x5F))
          gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | (enableAbbeyHack<<7)  | PixelMSB]);
        else
          gpuDrawS(gpuSpriteSpanDrivers [Blending_Mode | TEXT_MODE | Masking | Blending | Lighting | (enableAbbeyHack<<7)  | PixelMSB]);
        break;

#ifdef TEST
      case 0x80:          //  vid -> vid
        gpuMoveImage();   //  prim handles updateLace && skip
        break;
      case 0xA0:          //  sys -> vid
      {
        u32 load_width = list[2] & 0xffff;
        u32 load_height = list[2] >> 16;
        u32 load_size = load_width * load_height;

        len += load_size / 2;
        break;
      }
      case 0xC0:
        break;
#else
      case 0x80 ... 0x9F:          //  vid -> vid
      case 0xA0 ... 0xBF:          //  sys -> vid
      case 0xC0 ... 0xDF:          //  vid -> sys
        // Handled by gpulib
        goto breakloop;
#endif
      case 0xE1: {
        const u32 temp = PacketBuffer.U4[0];
        GPU_GP1 = (GPU_GP1 & ~0x000007FF) | (temp & 0x000007FF);
        gpuSetTexture(temp);
        gpu.ex_regs[1] = temp;
        break;
      }
      case 0xE2: {
        static const u8  TextureMask[32] = {
          255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,
          127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7
        };
        const u32 temp = PacketBuffer.U4[0];
        TextureWindow[0] = ((temp >> 10) & 0x1F) << 3;
        TextureWindow[1] = ((temp >> 15) & 0x1F) << 3;
        TextureWindow[2] = TextureMask[(temp >> 0) & 0x1F];
        TextureWindow[3] = TextureMask[(temp >> 5) & 0x1F];
        gpuSetTexture(GPU_GP1);
        gpu.ex_regs[2] = temp;
        break;
      }
      case 0xE3: {
        const u32 temp = PacketBuffer.U4[0];
        DrawingArea[0] = temp         & 0x3FF;
        DrawingArea[1] = (temp >> 10) & 0x3FF;
        gpu.ex_regs[3] = temp;
        break;
      }
      case 0xE4: {
        const u32 temp = PacketBuffer.U4[0];
        DrawingArea[2] = (temp         & 0x3FF) + 1;
        DrawingArea[3] = ((temp >> 10) & 0x3FF) + 1;
        gpu.ex_regs[4] = temp;
        break;
      }
      case 0xE5: {
        const u32 temp = PacketBuffer.U4[0];
        DrawingOffset[0] = ((s32)temp<<(32-11))>>(32-11);
        DrawingOffset[1] = ((s32)temp<<(32-22))>>(32-11);
        gpu.ex_regs[5] = temp;
        break;
      }
      case 0xE6: {
        const u32 temp = PacketBuffer.U4[0];
        Masking = (temp & 0x2) <<  1;
        PixelMSB =(temp & 0x1) <<  8;
        gpu.ex_regs[6] = temp;
        break;
      }
    }
  }

breakloop:
  gpu.ex_regs[1] &= ~0x1ff;
  gpu.ex_regs[1] |= GPU_GP1 & 0x1ff;

  *last_cmd = cmd;
  return list - list_start;
}

void oldunai_renderer_sync_ecmds(uint32_t *ecmds)
{
  int dummy;
  do_cmd_list(&ecmds[1], 6, &dummy, &dummy, &dummy);
}

#ifndef TEST

#include "../../../frontend/plugin_lib.h"

void oldunai_renderer_set_config(const struct rearmed_cbs *cbs)
{
  force_interlace = cbs->gpu_unai.ilace_force;
  //enableAbbeyHack = cbs->gpu_unai_old.abe_hack;
  light = cbs->gpu_unai.lighting;
  blend = cbs->gpu_unai.blending;

  GPU_FrameBuffer = (u16 *)gpu.vram;
}

#endif

// vim:shiftwidth=2:expandtab
