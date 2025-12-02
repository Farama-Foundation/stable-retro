/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2016 Senquack (dansilsby <AT> gmail <DOT> com)          *
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

#ifndef GPU_UNAI_GPU_H
#define GPU_UNAI_GPU_H

struct gpu_unai_config_t {
	uint8_t pixel_skip:1;     // If 1, allows skipping rendering pixels that
	                          //  would not be visible when a high horizontal
	                          //  resolution PS1 video mode is set.
	                          //  Only applies to devices with low resolutions
	                          //  like 320x240. Should not be used if a
	                          //  down-scaling framebuffer blitter is in use.
	                          //  Can cause gfx artifacts if game reads VRAM
	                          //  to do framebuffer effects.

	uint8_t ilace_force:3;    // Option to force skipping rendering of lines,
	                          //  for very slow platforms. Value will be
	                          //  assigned to 'ilace_mask' in gpu_unai struct.
	                          //  Normally 0. Value '1' will skip rendering
	                          //  odd lines.

	uint8_t scale_hires:1;    // If 1, will scale hi-res output to
	                          //  320x240 when gpulib reads the frame.
	                          //  Implies pixel_skip and ilace_force
	                          //  (when height > 240).
	uint8_t lighting:1;
	uint8_t fast_lighting:1;
	uint8_t blending:1;
	uint8_t dithering:1;
	uint8_t force_dithering:1;
	uint8_t old_renderer:1;

	//senquack Only PCSX Rearmed's version of gpu_unai had this, and I
	// don't think it's necessary. It would require adding 'AH' flag to
	// gpuSpriteSpanFn() increasing size of sprite span function array.
	//uint8_t enableAbbeyHack:1;  // Abe's Odyssey hack

	////////////////////////////////////////////////////////////////////////////
	// Variables used only by older standalone version of gpu_unai (gpu.cpp)
#ifndef USE_GPULIB
	uint8_t prog_ilace:1;         // Progressive interlace option (old option)
	                              //  This option was somewhat oddly named:
	                              //  When in interlaced video mode, on a low-res
	                              //  320x240 device, only the even lines are
	                              //  rendered. This option will take that one
	                              //  step further and only render half the even
	                              //  even lines one frame, and then the other half.
	uint8_t frameskip_count:3;    // Frame skip (0..7)
#endif
};

extern gpu_unai_config_t gpu_unai_config_ext;

// TODO: clean up show_fps frontend option
extern  bool show_fps;

#endif // GPU_UNAI_GPU_H
