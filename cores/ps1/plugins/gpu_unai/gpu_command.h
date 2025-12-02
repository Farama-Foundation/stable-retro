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

#ifndef __GPU_UNAI_GPU_COMMAND_H__
#define __GPU_UNAI_GPU_COMMAND_H__

///////////////////////////////////////////////////////////////////////////////
void gpuSetTexture(u16 tpage)
{
	u32 tmode, tx, ty;
	gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 & ~0x1FF) | (tpage & 0x1FF);
	gpu_unai.TextureWindow[0]&= ~gpu_unai.TextureWindow[2];
	gpu_unai.TextureWindow[1]&= ~gpu_unai.TextureWindow[3];

	tmode = (tpage >> 7) & 3;  // 16bpp, 8bpp, or 4bpp texture colors?
	                           // 0: 4bpp     1: 8bpp     2/3: 16bpp

	// Nocash PSX docs state setting of 3 is same as setting of 2 (16bpp):
	// Note: DrHell assumes 3 is same as 0.. TODO: verify which is correct?
	if (tmode == 3) tmode = 2;

	tx = (tpage & 0x0F) << 6;
	ty = (tpage & 0x10) << 4;

	tx += (gpu_unai.TextureWindow[0] >> (2 - tmode));
	ty += gpu_unai.TextureWindow[1];
	
	gpu_unai.BLEND_MODE  = ((tpage>>5) & 3) << 3;
	gpu_unai.TEXT_MODE   = (tmode + 1) << 5; // gpu_unai.TEXT_MODE should be values 1..3, so add one
	gpu_unai.inn.TBA = &gpu_unai.vram[FRAME_OFFSET(tx, ty)];
}

///////////////////////////////////////////////////////////////////////////////
INLINE void gpuSetCLUT(u16 clut)
{
	gpu_unai.inn.CBA = &gpu_unai.vram[(clut & 0x7FFF) << 4];
}

#ifdef  ENABLE_GPU_NULL_SUPPORT
#define NULL_GPU() break
#else
#define NULL_GPU()
#endif

#ifdef  ENABLE_GPU_LOG_SUPPORT
#define DO_LOG(expr) printf expr
#else
#define DO_LOG(expr) {}
#endif

#define Blending      (((PRIM&0x2) && BlendingEnabled()) ? (PRIM&0x2) : 0)
#define Blending_Mode (((PRIM&0x2) && BlendingEnabled()) ? gpu_unai.BLEND_MODE : 0)
#define Lighting      (((~PRIM)&0x1) && LightingEnabled())
// Dithering applies only to Gouraud-shaded polys or texture-blended polys:
#define Dithering     (((((~PRIM)&0x1) || (PRIM&0x10)) && DitheringEnabled()) ?            \
                       (ForcedDitheringEnabled() ? (1<<9) : (gpu_unai.GPU_GP1 & (1 << 9))) \
                       : 0)

///////////////////////////////////////////////////////////////////////////////
//Now handled by Rearmed's gpulib and gpu_unai/gpulib_if.cpp:
///////////////////////////////////////////////////////////////////////////////
#ifndef USE_GPULIB

// Handles GP0 draw settings commands 0xE1...0xE6
static void gpuGP0Cmd_0xEx(gpu_unai_t &gpu_unai, u32 cmd_word)
{
	// Assume incoming GP0 command is 0xE1..0xE6, convert to 1..6
	u8 num = (cmd_word >> 24) & 7;
	switch (num) {
		case 1: {
			// GP0(E1h) - Draw Mode setting (aka "Texpage")
			DO_LOG(("GP0(0xE1) DrawMode TexPage(0x%x)\n", cmd_word));
			u32 cur_texpage = gpu_unai.GPU_GP1 & 0x7FF;
			u32 new_texpage = cmd_word & 0x7FF;
			if (cur_texpage != new_texpage) {
				gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 & ~0x7FF) | new_texpage;
				gpuSetTexture(gpu_unai.GPU_GP1);
			}
		} break;

		case 2: {
			// GP0(E2h) - Texture Window setting
			DO_LOG(("GP0(0xE2) TextureWindow(0x%x)\n", cmd_word));
			if (cmd_word != gpu_unai.TextureWindowCur) {
				static const u8 TextureMask[32] = {
					255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,
					127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7
				};
				gpu_unai.TextureWindowCur = cmd_word;
				gpu_unai.TextureWindow[0] = ((cmd_word >> 10) & 0x1F) << 3;
				gpu_unai.TextureWindow[1] = ((cmd_word >> 15) & 0x1F) << 3;
				gpu_unai.TextureWindow[2] = TextureMask[(cmd_word >> 0) & 0x1F];
				gpu_unai.TextureWindow[3] = TextureMask[(cmd_word >> 5) & 0x1F];
				gpu_unai.TextureWindow[0] &= ~gpu_unai.TextureWindow[2];
				gpu_unai.TextureWindow[1] &= ~gpu_unai.TextureWindow[3];

				// Inner loop vars must be updated whenever texture window is changed:
				const u32 fb = FIXED_BITS;  // # of fractional fixed-pt bits of u4/v4
				gpu_unai.u_msk = (((u32)gpu_unai.TextureWindow[2]) << fb) | ((1 << fb) - 1);
				gpu_unai.v_msk = (((u32)gpu_unai.TextureWindow[3]) << fb) | ((1 << fb) - 1);

				gpuSetTexture(gpu_unai.GPU_GP1);
			}
		} break;

		case 3: {
			// GP0(E3h) - Set Drawing Area top left (X1,Y1)
			DO_LOG(("GP0(0xE3) DrawingArea Pos(0x%x)\n", cmd_word));
			gpu_unai.DrawingArea[0] = cmd_word         & 0x3FF;
			gpu_unai.DrawingArea[1] = (cmd_word >> 10) & 0x3FF;
		} break;

		case 4: {
			// GP0(E4h) - Set Drawing Area bottom right (X2,Y2)
			DO_LOG(("GP0(0xE4) DrawingArea Size(0x%x)\n", cmd_word));
			gpu_unai.DrawingArea[2] = (cmd_word         & 0x3FF) + 1;
			gpu_unai.DrawingArea[3] = ((cmd_word >> 10) & 0x3FF) + 1;
		} break;

		case 5: {
			// GP0(E5h) - Set Drawing Offset (X,Y)
			DO_LOG(("GP0(0xE5) DrawingOffset(0x%x)\n", cmd_word));
			gpu_unai.DrawingOffset[0] = ((s32)cmd_word<<(32-11))>>(32-11);
			gpu_unai.DrawingOffset[1] = ((s32)cmd_word<<(32-22))>>(32-11);
		} break;

		case 6: {
			// GP0(E6h) - Mask Bit Setting
			DO_LOG(("GP0(0xE6) SetMask(0x%x)\n", cmd_word));
			gpu_unai.Masking  = (cmd_word & 0x2) <<  1;
			gpu_unai.PixelMSB = (cmd_word & 0x1) <<  8;
		} break;
	}
}

void gpuSendPacketFunction(const int PRIM)
{
	//printf("0x%x\n",PRIM);

	//senquack - TODO: optimize this (packet pointer union as prim draw parameter
	// introduced as optimization for gpulib command-list processing)
	PtrUnion packet = { .ptr = (void*)&gpu_unai.PacketBuffer };

	switch (PRIM)
	{
		case 0x02: {
			NULL_GPU();
			gpuClearImage(packet);    //  prim handles updateLace && skip
			gpu_unai.fb_dirty = true;
			DO_LOG(("gpuClearImage(0x%x)\n",PRIM));
		} break;

		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23: {          // Monochrome 3-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				PP driver = gpuPolySpanDrivers[
					(gpu_unai.blit_mask?1024:0) |
					Blending_Mode |
					gpu_unai.Masking | Blending | gpu_unai.PixelMSB
				];
				gpuDrawPolyF(packet, driver, false);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyF(0x%x)\n",PRIM));
			}
		} break;

		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27: {          // Textured 3-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				gpuSetTexture (le32_to_u32(gpu_unai.PacketBuffer.U4[4]) >> 16);

				u32 driver_idx =
					(gpu_unai.blit_mask?1024:0) |
					Dithering |
					Blending_Mode | gpu_unai.TEXT_MODE |
					gpu_unai.Masking | Blending | gpu_unai.PixelMSB;

				if (!FastLightingEnabled()) {
					driver_idx |= Lighting;
				} else {
					if (!((gpu_unai.PacketBuffer.U1[0]>0x5F) && (gpu_unai.PacketBuffer.U1[1]>0x5F) && (gpu_unai.PacketBuffer.U1[2]>0x5F)))
						driver_idx |= Lighting;
				}

				PP driver = gpuPolySpanDrivers[driver_idx];
				gpuDrawPolyFT(packet, driver, false);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyFT(0x%x)\n",PRIM));
			}
		} break;

		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B: {          // Monochrome 4-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				PP driver = gpuPolySpanDrivers[
					(gpu_unai.blit_mask?1024:0) |
					Blending_Mode |
					gpu_unai.Masking | Blending | gpu_unai.PixelMSB
				];
				gpuDrawPolyF(packet, driver, true); // is_quad = true
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyF(0x%x) (4-pt QUAD)\n",PRIM));
			}
		} break;

		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F: {          // Textured 4-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				gpuSetTexture (le32_to_u32(gpu_unai.PacketBuffer.U4[4]) >> 16);

				u32 driver_idx =
					(gpu_unai.blit_mask?1024:0) |
					Dithering |
					Blending_Mode | gpu_unai.TEXT_MODE |
					gpu_unai.Masking | Blending | gpu_unai.PixelMSB;

				if (!FastLightingEnabled()) {
					driver_idx |= Lighting;
				} else {
					if (!((gpu_unai.PacketBuffer.U1[0]>0x5F) && (gpu_unai.PacketBuffer.U1[1]>0x5F) && (gpu_unai.PacketBuffer.U1[2]>0x5F)))
						driver_idx |= Lighting;
				}

				PP driver = gpuPolySpanDrivers[driver_idx];
				gpuDrawPolyFT(packet, driver, true); // is_quad = true
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyFT(0x%x) (4-pt QUAD)\n",PRIM));
			}
		} break;

		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33: {          // Gouraud-shaded 3-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				//NOTE: The '129' here is CF_GOURAUD | CF_LIGHT, however
				// this is an untextured poly, so CF_LIGHT (texture blend)
				// shouldn't apply. Until the original array of template
				// instantiation ptrs is fixed, we're stuck with this. (TODO)
				PP driver = gpuPolySpanDrivers[
					(gpu_unai.blit_mask?1024:0) |
					Dithering |
					Blending_Mode |
					gpu_unai.Masking | Blending | 129 | gpu_unai.PixelMSB
				];
				gpuDrawPolyG(packet, driver, false);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyG(0x%x)\n",PRIM));
			}
		} break;

		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37: {          // Gouraud-shaded, textured 3-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				gpuSetTexture (le32_to_u32(gpu_unai.PacketBuffer.U4[5]) >> 16);
				PP driver = gpuPolySpanDrivers[
					(gpu_unai.blit_mask?1024:0) |
					Dithering |
					Blending_Mode | gpu_unai.TEXT_MODE |
					gpu_unai.Masking | Blending | ((Lighting)?129:0) | gpu_unai.PixelMSB
				];
				gpuDrawPolyGT(packet, driver, false);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyGT(0x%x)\n",PRIM));
			}
		} break;

		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B: {          // Gouraud-shaded 4-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				// See notes regarding '129' for 0x30..0x33 further above -senquack
				PP driver = gpuPolySpanDrivers[
					(gpu_unai.blit_mask?1024:0) |
					Dithering |
					Blending_Mode |
					gpu_unai.Masking | Blending | 129 | gpu_unai.PixelMSB
				];
				gpuDrawPolyG(packet, driver, true); // is_quad = true
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyG(0x%x) (4-pt QUAD)\n",PRIM));
			}
		} break;

		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F: {          // Gouraud-shaded, textured 4-pt poly
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				gpuSetTexture (le32_to_u32(gpu_unai.PacketBuffer.U4[5]) >> 16);
				PP driver = gpuPolySpanDrivers[
					(gpu_unai.blit_mask?1024:0) |
					Dithering |
					Blending_Mode | gpu_unai.TEXT_MODE |
					gpu_unai.Masking | Blending | ((Lighting)?129:0) | gpu_unai.PixelMSB
				];
				gpuDrawPolyGT(packet, driver, true); // is_quad = true
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawPolyGT(0x%x) (4-pt QUAD)\n",PRIM));
			}
		} break;

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43: {          // Monochrome line
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				// Shift index right by one, as untextured prims don't use lighting
				u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
				PSD driver = gpuPixelSpanDrivers[driver_idx];
				gpuDrawLineF(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawLineF(0x%x)\n",PRIM));
			}
		} break;

		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F: { // Monochrome line strip
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				// Shift index right by one, as untextured prims don't use lighting
				u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
				PSD driver = gpuPixelSpanDrivers[driver_idx];
				gpuDrawLineF(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawLineF(0x%x)\n",PRIM));
			}
			if ((le32_raw(gpu_unai.PacketBuffer.U4[3]) & HTOLE32(0xF000F000)) != HTOLE32(0x50005000))
			{
				gpu_unai.PacketBuffer.U4[1] = gpu_unai.PacketBuffer.U4[2];
				gpu_unai.PacketBuffer.U4[2] = gpu_unai.PacketBuffer.U4[3];
				gpu_unai.PacketCount = 1;
				gpu_unai.PacketIndex = 3;
			}
		} break;

		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53: {          // Gouraud-shaded line
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				// Shift index right by one, as untextured prims don't use lighting
				u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
				// Index MSB selects Gouraud-shaded PixelSpanDriver:
				driver_idx |= (1 << 5);
				PSD driver = gpuPixelSpanDrivers[driver_idx];
				gpuDrawLineG(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawLineG(0x%x)\n",PRIM));
			}
		} break;

		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F: { // Gouraud-shaded line strip
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				// Shift index right by one, as untextured prims don't use lighting
				u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
				// Index MSB selects Gouraud-shaded PixelSpanDriver:
				driver_idx |= (1 << 5);
				PSD driver = gpuPixelSpanDrivers[driver_idx];
				gpuDrawLineG(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawLineG(0x%x)\n",PRIM));
			}
			if ((le32_raw(gpu_unai.PacketBuffer.U4[4]) & HTOLE32(0xF000F000)) != HTOLE32(0x50005000))
			{
				gpu_unai.PacketBuffer.U1[3 + (2 * 4)] = gpu_unai.PacketBuffer.U1[3 + (0 * 4)];
				gpu_unai.PacketBuffer.U4[0] = gpu_unai.PacketBuffer.U4[2];
				gpu_unai.PacketBuffer.U4[1] = gpu_unai.PacketBuffer.U4[3];
				gpu_unai.PacketBuffer.U4[2] = gpu_unai.PacketBuffer.U4[4];
				gpu_unai.PacketCount = 2;
				gpu_unai.PacketIndex = 3;
			}
		} break;

		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63: {          // Monochrome rectangle (variable size)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				PT driver = gpuTileSpanDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
				gpuDrawT(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67: {          // Textured rectangle (variable size)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				u32 driver_idx = Blending_Mode | gpu_unai.TEXT_MODE | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>1);

				// This fixes Silent Hill running animation on loading screens:
				// (On PSX, color values 0x00-0x7F darken the source texture's color,
				//  0x81-FF lighten textures (ultimately clamped to 0x1F),
				//  0x80 leaves source texture color unchanged, HOWEVER,
				//   gpu_unai uses a simple lighting LUT whereby only the upper
				//   5 bits of an 8-bit color are used, so 0x80-0x87 all behave as
				//   0x80.
				// 
				// NOTE: I've changed all textured sprite draw commands here and
				//  elsewhere to use proper behavior, but left poly commands
				//  alone, I don't want to slow rendering down too much. (TODO)
				//if ((gpu_unai.PacketBuffer.U1[0]>0x5F) && (gpu_unai.PacketBuffer.U1[1]>0x5F) && (gpu_unai.PacketBuffer.U1[2]>0x5F))
				// Strip lower 3 bits of each color and determine if lighting should be used:
				if ((le32_raw(gpu_unai.PacketBuffer.U4[0]) & HTOLE32(0xF8F8F8)) != HTOLE32(0x808080))
					driver_idx |= Lighting;
				PS driver = gpuSpriteSpanDrivers[driver_idx];
				gpuDrawS(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
		} break;

		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B: {          // Monochrome rectangle (1x1 dot)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpu_unai.PacketBuffer.U4[2] = u32_to_le32(0x00010001);
				PT driver = gpuTileSpanDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
				gpuDrawT(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73: {          // Monochrome rectangle (8x8)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpu_unai.PacketBuffer.U4[2] = u32_to_le32(0x00080008);
				PT driver = gpuTileSpanDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
				gpuDrawT(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77: {          // Textured rectangle (8x8)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpu_unai.PacketBuffer.U4[3] = u32_to_le32(0x00080008);
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				u32 driver_idx = Blending_Mode | gpu_unai.TEXT_MODE | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>1);

				//senquack - Only color 808080h-878787h allows skipping lighting calculation:
				//if ((gpu_unai.PacketBuffer.U1[0]>0x5F) && (gpu_unai.PacketBuffer.U1[1]>0x5F) && (gpu_unai.PacketBuffer.U1[2]>0x5F))
				// Strip lower 3 bits of each color and determine if lighting should be used:
				if ((le32_raw(gpu_unai.PacketBuffer.U4[0]) & HTOLE32(0xF8F8F8)) != HTOLE32(0x808080))
					driver_idx |= Lighting;
				PS driver = gpuSpriteSpanDrivers[driver_idx];
				gpuDrawS(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
		} break;

		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B: {          // Monochrome rectangle (16x16)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpu_unai.PacketBuffer.U4[2] = u32_to_le32(0x00100010);
				PT driver = gpuTileSpanDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
				gpuDrawT(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawT(0x%x)\n",PRIM));
			}
		} break;

		case 0x7C:
		case 0x7D:
			#ifdef __arm__
			/* Notaz 4bit sprites optimization */
			if ((!gpu_unai.frameskip.skipGPU) && (!(gpu_unai.GPU_GP1&0x180)) && (!(gpu_unai.Masking|gpu_unai.PixelMSB)))
			{
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				gpuDrawS16(packet);
				gpu_unai.fb_dirty = true;
				break;
			}
			#endif
		case 0x7E:
		case 0x7F: {          // Textured rectangle (16x16)
			if (!gpu_unai.frameskip.skipGPU)
			{
				NULL_GPU();
				gpu_unai.PacketBuffer.U4[3] = u32_to_le32(0x00100010);
				gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
				u32 driver_idx = Blending_Mode | gpu_unai.TEXT_MODE | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>1);

				//senquack - Only color 808080h-878787h allows skipping lighting calculation:
				//if ((gpu_unai.PacketBuffer.U1[0]>0x5F) && (gpu_unai.PacketBuffer.U1[1]>0x5F) && (gpu_unai.PacketBuffer.U1[2]>0x5F))
				// Strip lower 3 bits of each color and determine if lighting should be used:
				if ((le32_raw(gpu_unai.PacketBuffer.U4[0]) & HTOLE32(0xF8F8F8)) != HTOLE32(0x808080))
					driver_idx |= Lighting;
				PS driver = gpuSpriteSpanDrivers[driver_idx];
				gpuDrawS(packet, driver);
				gpu_unai.fb_dirty = true;
				DO_LOG(("gpuDrawS(0x%x)\n",PRIM));
			}
		} break;

		case 0x80:          //  vid -> vid
			gpuMoveImage(packet);   //  prim handles updateLace && skip
			if ((!gpu_unai.frameskip.skipCount) && (gpu_unai.DisplayArea[3] == 480)) // Tekken 3 hack
			{
				if (!gpu_unai.frameskip.skipGPU) gpu_unai.fb_dirty = true;
			}
			else
			{
				gpu_unai.fb_dirty = true;
			}
			DO_LOG(("gpuMoveImage(0x%x)\n",PRIM));
			break;
		case 0xA0:          //  sys ->vid
			gpuLoadImage(packet);   //  prim handles updateLace && skip
			DO_LOG(("gpuLoadImage(0x%x)\n",PRIM));
			break;
		case 0xC0:          //  vid -> sys
			gpuStoreImage(packet);  //  prim handles updateLace && skip
			DO_LOG(("gpuStoreImage(0x%x)\n",PRIM));
			break;
		case 0xE1 ... 0xE6: { // Draw settings
			gpuGP0Cmd_0xEx(gpu_unai, le32_to_u32(gpu_unai.PacketBuffer.U4[0]));
		} break;
	}
}
#endif //!USE_GPULIB
///////////////////////////////////////////////////////////////////////////////
// End of code specific to non-gpulib standalone version of gpu_unai
///////////////////////////////////////////////////////////////////////////////

#endif /* __GPU_UNAI_GPU_COMMAND_H__ */
