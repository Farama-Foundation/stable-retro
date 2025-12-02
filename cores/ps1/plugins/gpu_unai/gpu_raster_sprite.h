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

#ifndef __GPU_UNAI_GPU_RASTER_SPRITE_H__
#define __GPU_UNAI_GPU_RASTER_SPRITE_H__

///////////////////////////////////////////////////////////////////////////////
//  GPU internal sprite drawing functions

void gpuDrawS(PtrUnion packet, const PS gpuSpriteDriver, s32 *w_out, s32 *h_out)
{
	s32 x0, x1, y0, y1;
	u32 u0, v0;

	//NOTE: Must 11-bit sign-extend the whole sum here, not just packet X/Y,
	// or sprites in 1st level of SkullMonkeys disappear when walking right.
	// This now matches behavior of Mednafen and PCSX Rearmed's gpu_neon:
	x0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[2]) + gpu_unai.DrawingOffset[0]);
	y0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[3]) + gpu_unai.DrawingOffset[1]);

	u32 w = le16_to_u16(packet.U2[6]) & 0x3ff; // Max width is 1023
	u32 h = le16_to_u16(packet.U2[7]) & 0x1ff; // Max height is 511
	x1 = x0 + w;
	y1 = y0 + h;

	s32 xmin, xmax, ymin, ymax;
	xmin = gpu_unai.DrawingArea[0];	xmax = gpu_unai.DrawingArea[2];
	ymin = gpu_unai.DrawingArea[1];	ymax = gpu_unai.DrawingArea[3];

	u0 = packet.U1[8];
	v0 = packet.U1[9];

	s32 temp;
	temp = ymin - y0;
	if (temp > 0) { y0 = ymin; v0 += temp; }
	if (y1 > ymax) y1 = ymax;
	if (y1 <= y0) return;

	temp = xmin - x0;
	if (temp > 0) { x0 = xmin; u0 += temp; }
	if (x1 > xmax) x1 = xmax;
	x1 -= x0;
	if (x1 <= 0) return;
	*w_out = x1;
	*h_out = y1 - y0;

	le16_t *Pixel = &gpu_unai.vram[FRAME_OFFSET(x0, y0)];

	gpu_unai.inn.r5 = packet.U1[0] >> 3;
	gpu_unai.inn.g5 = packet.U1[1] >> 3;
	gpu_unai.inn.b5 = packet.U1[2] >> 3;
	gpu_unai.inn.u = u0;
	gpu_unai.inn.v = v0;
	gpu_unai.inn.y0 = y0;
	gpu_unai.inn.y1 = y1;
	gpuSpriteDriver(Pixel, x1, (u8 *)gpu_unai.inn.TBA, gpu_unai.inn);
}

void gpuDrawT(PtrUnion packet, const PT gpuTileDriver, s32 *w_out, s32 *h_out)
{
	s32 x0, x1, y0, y1;

	// This now matches behavior of Mednafen and PCSX Rearmed's gpu_neon:
	x0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[2]) + gpu_unai.DrawingOffset[0]);
	y0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[3]) + gpu_unai.DrawingOffset[1]);

	u32 w = le16_to_u16(packet.U2[4]) & 0x3ff; // Max width is 1023
	u32 h = le16_to_u16(packet.U2[5]) & 0x1ff; // Max height is 511
	x1 = x0 + w;
	y1 = y0 + h;

	s32 xmin, xmax, ymin, ymax;
	xmin = gpu_unai.DrawingArea[0];	xmax = gpu_unai.DrawingArea[2];
	ymin = gpu_unai.DrawingArea[1];	ymax = gpu_unai.DrawingArea[3];

	if (y0 < ymin) y0 = ymin;
	if (y1 > ymax) y1 = ymax;
	if (y1 <= y0) return;

	if (x0 < xmin) x0 = xmin;
	if (x1 > xmax) x1 = xmax;
	x1 -= x0;
	if (x1 <= 0) return;
	*w_out = x1;
	*h_out = y1 - y0;

	const u16 Data = GPU_RGB16(le32_to_u32(packet.U4[0]));
	le16_t *Pixel = &gpu_unai.vram[FRAME_OFFSET(x0, y0)];

	gpu_unai.inn.y0 = y0;
	gpu_unai.inn.y1 = y1;
	gpuTileDriver(Pixel, Data, x1, gpu_unai.inn);
}

#endif /* __GPU_UNAI_GPU_RASTER_SPRITE_H__ */
