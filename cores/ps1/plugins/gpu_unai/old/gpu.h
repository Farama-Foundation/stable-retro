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

#ifndef NEW_GPU_H
#define NEW_GPU_H

///////////////////////////////////////////////////////////////////////////////
//  GPU global definitions
#define	FRAME_BUFFER_SIZE	(1024*512*2)
#define	FRAME_WIDTH			  1024
#define	FRAME_HEIGHT		  512
#define	FRAME_OFFSET(x,y)	(((y)<<10)+(x))

#define VIDEO_WIDTH 320

typedef char				s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long	s64;

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

#include "gpu_fixedpoint.h"

///////////////////////////////////////////////////////////////////////////////
//  Tweaks and Hacks
extern  int  skipCount;
extern  bool enableAbbeyHack;
extern  bool show_fps;
extern  bool alt_fps;

///////////////////////////////////////////////////////////////////////////////
//  interlaced rendering
extern  int linesInterlace_user;
extern  bool progressInterlace;

extern  bool light;
extern  bool blend;

typedef struct {
	u32 Version;
	u32 GPU_gp1;
	u32 Control[256];
	unsigned char FrameBuffer[1024*512*2];
} GPUFreeze_t;

struct  GPUPacket
{
	union
	{
		u32 U4[16];
		s32 S4[16];
		u16 U2[32];
		s16 S2[32];
		u8  U1[64];
		s8  S1[64];
	};
};

///////////////////////////////////////////////////////////////////////////////
//  Compile Options

//#define ENABLE_GPU_NULL_SUPPORT   // Enables NullGPU support
//#define ENABLE_GPU_LOG_SUPPORT    // Enables gpu logger, very slow only for windows debugging

///////////////////////////////////////////////////////////////////////////////
#endif  // NEW_GPU_H
