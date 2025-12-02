/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2011 notaz                                              *
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gpulib/gpu.h"
#include "old/if.h"

#ifdef THREAD_RENDERING
#include "../gpulib/gpulib_thread_if.h"
#define do_cmd_list real_do_cmd_list
#define renderer_init real_renderer_init
#define renderer_finish real_renderer_finish
#define renderer_sync_ecmds real_renderer_sync_ecmds
#define renderer_update_caches real_renderer_update_caches
#define renderer_flush_queues real_renderer_flush_queues
#define renderer_set_interlace real_renderer_set_interlace
#define renderer_set_config real_renderer_set_config
#define renderer_notify_res_change real_renderer_notify_res_change
#define renderer_notify_update_lace real_renderer_notify_update_lace
#define renderer_sync real_renderer_sync
#define ex_regs scratch_ex_regs
#endif

//#include "port.h"
#include "gpu_unai.h"

// GPU fixed point math
#include "gpu_fixedpoint.h"

// Inner loop driver instantiation file
#include "gpu_inner.h"

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

#ifndef GPU_UNAI_NO_OLD
#define IS_OLD_RENDERER() gpu_unai.config.old_renderer
#else
#define IS_OLD_RENDERER() false
#endif

#define DOWNSCALE_VRAM_SIZE (1024 * 512 * 2 * 2 + 4096)

INLINE void scale_640_to_320(le16_t *dest, const le16_t *src, bool isRGB24) {
  size_t uCount = 320;

  if(isRGB24) {
    const uint8_t* src8 = (const uint8_t *)src;
    uint8_t* dst8 = (uint8_t *)dest;

    do {
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8;
      src8 += 4;
    } while(--uCount);
  } else {
    const le16_t* src16 = src;
    le16_t* dst16 = dest;

    do {
      *dst16++ = *src16;
      src16 += 2;
    } while(--uCount);
  }
}

INLINE void scale_512_to_320(le16_t *dest, const le16_t *src, bool isRGB24) {
  size_t uCount = 64;

  if(isRGB24) {
    const uint8_t* src8 = (const uint8_t *)src;
    uint8_t* dst8 = (uint8_t *)dest;

    do {
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8;
      src8 += 4;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8;
      src8 += 4;
      *dst8++ = *src8++;
      *dst8++ = *src8++;
      *dst8++ = *src8;
      src8 += 4;
    } while(--uCount);
  } else {
    const le16_t* src16 = src;
    le16_t* dst16 = dest;

    do {
      *dst16++ = *src16++;
      *dst16++ = *src16;
      src16 += 2;
      *dst16++ = *src16++;
      *dst16++ = *src16;
      src16 += 2;
      *dst16++ = *src16;
      src16 += 2;
    } while(--uCount);
  }
}

static uint16_t *get_downscale_buffer(int *x, int *y, int *w, int *h, int *vram_h)
{
  le16_t *dest = gpu_unai.downscale_vram;
  const le16_t *src = gpu_unai.vram;
  bool isRGB24 = (gpu_unai.GPU_GP1 & 0x00200000 ? true : false);
  int stride = 1024, dstride = 1024, lines = *h, orig_w = *w;

  // PS1 fb read wraps around (fixes black screen in 'Tobal no. 1')
  unsigned int fb_mask = 1024 * 512 - 1;

  if (*h > 240) {
    *h /= 2;
    stride *= 2;
    lines = *h;

    // Ensure start at a non-skipped line
    while (*y & gpu_unai.inn.ilace_mask) ++*y;
  }

  unsigned int fb_offset_src = (*y * dstride + *x) & fb_mask;
  unsigned int fb_offset_dest = fb_offset_src;

  if (*w == 512 || *w == 640) {
    *w = 320;
  }

  switch(orig_w) {
  case 640:
    do {
      scale_640_to_320(dest + fb_offset_dest, src + fb_offset_src, isRGB24);
      fb_offset_src = (fb_offset_src + stride) & fb_mask;
      fb_offset_dest = (fb_offset_dest + dstride) & fb_mask;
    } while(--lines);

    break;
  case 512:
    do {
      scale_512_to_320(dest + fb_offset_dest, src + fb_offset_src, isRGB24);
      fb_offset_src = (fb_offset_src + stride) & fb_mask;
      fb_offset_dest = (fb_offset_dest + dstride) & fb_mask;
    } while(--lines);
    break;
  default:
    size_t size = isRGB24 ? *w * 3 : *w * 2;

    do {
      memcpy(dest + fb_offset_dest, src + fb_offset_src, size);
      fb_offset_src = (fb_offset_src + stride) & fb_mask;
      fb_offset_dest = (fb_offset_dest + dstride) & fb_mask;
    } while(--lines);
    break;
  }

  return (uint16_t *)gpu_unai.downscale_vram;
}

static void map_downscale_buffer(void)
{
  if (gpu_unai.downscale_vram)
    return;

  gpu_unai.downscale_vram = (le16_t*)gpu.mmap(DOWNSCALE_VRAM_SIZE);

  if (gpu_unai.downscale_vram == NULL || gpu_unai.downscale_vram == (le16_t *)(intptr_t)-1) {
    fprintf(stderr, "failed to map downscale buffer\n");
    gpu_unai.downscale_vram = NULL;
    gpu.get_downscale_buffer = NULL;
  }
  else {
    gpu.get_downscale_buffer = get_downscale_buffer;
  }
}

static void unmap_downscale_buffer(void)
{
  if (gpu_unai.downscale_vram == NULL)
    return;

  gpu.munmap(gpu_unai.downscale_vram, DOWNSCALE_VRAM_SIZE);
  gpu_unai.downscale_vram = NULL;
  gpu.get_downscale_buffer = NULL;
}

int renderer_init(void)
{
  memset((void*)&gpu_unai, 0, sizeof(gpu_unai));
  gpu_unai.vram = (le16_t *)gpu.vram;

  // Original standalone gpu_unai initialized TextureWindow[]. I added the
  //  same behavior here, since it seems unsafe to leave [2],[3] unset when
  //  using HLE and Rearmed gpu_neon sets this similarly on init. -senquack
  gpu_unai.TextureWindow[0] = 0;
  gpu_unai.TextureWindow[1] = 0;
  gpu_unai.TextureWindow[2] = 255;
  gpu_unai.TextureWindow[3] = 255;
  //senquack - new vars must be updated whenever texture window is changed:
  //           (used for polygon-drawing in gpu_inner.h, gpu_raster_polygon.h)
  const u32 fb = FIXED_BITS;  // # of fractional fixed-pt bits of u4/v4
  gpu_unai.inn.u_msk = (((u32)gpu_unai.TextureWindow[2]) << fb) | ((1 << fb) - 1);
  gpu_unai.inn.v_msk = (((u32)gpu_unai.TextureWindow[3]) << fb) | ((1 << fb) - 1);

  // Configuration options
  gpu_unai.config = gpu_unai_config_ext;
  //senquack - disabled, not sure this is needed and would require modifying
  // sprite-span functions, perhaps unnecessarily. No Abe Oddysey hack was
  // present in latest PCSX4ALL sources we were using.
  //gpu_unai.config.enableAbbeyHack = gpu_unai_config_ext.abe_hack;
  gpu_unai.inn.ilace_mask = gpu_unai.config.ilace_force;

#if defined(GPU_UNAI_USE_INT_DIV_MULTINV) || (!defined(GPU_UNAI_NO_OLD) && !defined(GPU_UNAI_USE_FLOATMATH))
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

  SetupLightLUT();
  SetupDitheringConstants();

  if (gpu_unai.config.scale_hires) {
    map_downscale_buffer();
  }

  return 0;
}

void renderer_finish(void)
{
  unmap_downscale_buffer();
}

void renderer_notify_res_change(void)
{
  gpu_unai.inn.ilace_mask = gpu_unai.config.ilace_force;

#ifndef HAVE_PRE_ARMV7 /* XXX */
  if (gpu_unai.config.scale_hires)
#endif
  {
    gpu_unai.inn.ilace_mask |= !!(gpu.status & PSX_GPU_STATUS_INTERLACE);
  }

  /*
  printf("res change hres: %d   vres: %d   depth: %d   ilace_mask: %d\n",
      gpu.screen.hres, gpu.screen.vres, (gpu.status & PSX_GPU_STATUS_RGB24) ? 24 : 15,
      gpu_unai.ilace_mask);
  */
}

void renderer_notify_scanout_change(int x, int y)
{
}

#ifdef USE_GPULIB
// Handles GP0 draw settings commands 0xE1...0xE6
static void gpuGP0Cmd_0xEx(gpu_unai_t &gpu_unai, u32 cmd_word)
{
  // Assume incoming GP0 command is 0xE1..0xE6, convert to 1..6
  u8 num = (cmd_word >> 24) & 7;
  gpu.ex_regs[num] = cmd_word; // Update gpulib register
  switch (num) {
    case 1: {
      // GP0(E1h) - Draw Mode setting (aka "Texpage")
      u32 cur_texpage = gpu_unai.GPU_GP1 & 0x7FF;
      u32 new_texpage = cmd_word & 0x7FF;
      if (cur_texpage != new_texpage) {
        gpu_unai.GPU_GP1 = (gpu_unai.GPU_GP1 & ~0x7FF) | new_texpage;
        gpuSetTexture(gpu_unai.GPU_GP1);
      }
    } break;

    case 2: {
      // GP0(E2h) - Texture Window setting
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
        gpu_unai.inn.u_msk = (((u32)gpu_unai.TextureWindow[2]) << fb) | ((1 << fb) - 1);
        gpu_unai.inn.v_msk = (((u32)gpu_unai.TextureWindow[3]) << fb) | ((1 << fb) - 1);

        gpuSetTexture(gpu_unai.GPU_GP1);
      }
    } break;

    case 3: {
      // GP0(E3h) - Set Drawing Area top left (X1,Y1)
      gpu_unai.DrawingArea[0] = cmd_word         & 0x3FF;
      gpu_unai.DrawingArea[1] = (cmd_word >> 10) & 0x3FF;
    } break;

    case 4: {
      // GP0(E4h) - Set Drawing Area bottom right (X2,Y2)
      gpu_unai.DrawingArea[2] = (cmd_word         & 0x3FF) + 1;
      gpu_unai.DrawingArea[3] = ((cmd_word >> 10) & 0x3FF) + 1;
    } break;

    case 5: {
      // GP0(E5h) - Set Drawing Offset (X,Y)
      gpu_unai.DrawingOffset[0] = GPU_EXPANDSIGN(cmd_word);
      gpu_unai.DrawingOffset[1] = GPU_EXPANDSIGN(cmd_word >> 11);
    } break;

    case 6: {
      // GP0(E6h) - Mask Bit Setting
      gpu_unai.Masking  = (cmd_word & 0x2) <<  1;
      gpu_unai.PixelMSB = (cmd_word & 0x1) <<  8;
    } break;
  }
}
#endif

#include "../gpulib/gpu_timing.h"

// Strip lower 3 bits of each color and determine if lighting should be used:
static inline bool need_lighting(u32 rgb_raw)
{
  return (rgb_raw & HTOLE32(0xF8F8F8)) != HTOLE32(0x808080);
}

static inline void textured_sprite(int &cpu_cycles_sum, int &cpu_cycles)
{
  u32 PRIM = le32_to_u32(gpu_unai.PacketBuffer.U4[0]) >> 24;
  gpuSetCLUT(le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
  u32 driver_idx = Blending_Mode | gpu_unai.TEXT_MODE | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>1);
  s32 w = 0, h = 0;

  //senquack - Only color 808080h-878787h allows skipping lighting calculation:
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
  if (need_lighting(le32_raw(gpu_unai.PacketBuffer.U4[0])))
    driver_idx |= Lighting;
  PS driver = gpuSpriteDrivers[driver_idx];
  PtrUnion packet = { .ptr = (void*)&gpu_unai.PacketBuffer };
  gpuDrawS(packet, driver, &w, &h);
  gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(w, h));
}

extern const unsigned char cmd_lengths[256];

int do_cmd_list(u32 *list_, int list_len,
 int *cycles_sum_out, int *cycles_last, int *last_cmd)
{
  int cpu_cycles_sum = 0, cpu_cycles = *cycles_last;
  u32 cmd = 0, len, i;
  le32_t *list = (le32_t *)list_;
  le32_t *list_start = list;
  le32_t *list_end = list + list_len;

  if (IS_OLD_RENDERER())
    return oldunai_do_cmd_list(list_, list_len, cycles_sum_out, cycles_last, last_cmd);

  for (; list < list_end; list += 1 + len)
  {
    cmd = le32_to_u32(*list) >> 24;
    len = cmd_lengths[cmd];
    if (list + 1 + len > list_end) {
      cmd = -1;
      break;
    }

    #define PRIM cmd
    gpu_unai.PacketBuffer.U4[0] = list[0];
    for (i = 1; i <= len; i++)
      gpu_unai.PacketBuffer.U4[i] = list[i];

    PtrUnion packet = { .ptr = (void*)&gpu_unai.PacketBuffer };

    switch (cmd)
    {
      case 0x02:
        gpuClearImage(packet);
        gput_sum(cpu_cycles_sum, cpu_cycles,
           gput_fill(le16_to_s16(packet.U2[4]) & 0x3ff, le16_to_s16(packet.U2[5]) & 0x1ff));
        break;

      case 0x20:
      case 0x21:
      case 0x22:
      case 0x23: {          // Monochrome 3-pt poly
        PP driver = gpuPolySpanDrivers[
          //(gpu_unai.blit_mask?1024:0) |
          Blending_Mode |
          gpu_unai.Masking | Blending | gpu_unai.PixelMSB
        ];
        gpuDrawPolyF(packet, driver, false);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base());
      } break;

      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27: {          // Textured 3-pt poly
        gpuSetCLUT   (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
        gpuSetTexture(le32_to_u32(gpu_unai.PacketBuffer.U4[4]) >> 16);

        u32 driver_idx =
          //(gpu_unai.blit_mask?1024:0) |
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
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_t());
      } break;

      case 0x28:
      case 0x29:
      case 0x2A:
      case 0x2B: {          // Monochrome 4-pt poly
        PP driver = gpuPolySpanDrivers[
          //(gpu_unai.blit_mask?1024:0) |
          Blending_Mode |
          gpu_unai.Masking | Blending | gpu_unai.PixelMSB
        ];
        gpuDrawPolyF(packet, driver, true); // is_quad = true
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base());
      } break;

      case 0x2C:
      case 0x2D:
      case 0x2E:
      case 0x2F: {          // Textured 4-pt poly
        u32 simplified_count;
        gpuSetTexture(le32_to_u32(gpu_unai.PacketBuffer.U4[4]) >> 16);
        if ((simplified_count = prim_try_simplify_quad_t(gpu_unai.PacketBuffer.U4,
              gpu_unai.PacketBuffer.U4)))
        {
          for (i = 0;; ) {
            textured_sprite(cpu_cycles_sum, cpu_cycles);
            if (++i >= simplified_count)
              break;
            memcpy(&gpu_unai.PacketBuffer.U4[0], &gpu_unai.PacketBuffer.U4[i * 4], 16);
          }
          break;
        }
        gpuSetCLUT(le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);

        u32 driver_idx =
          //(gpu_unai.blit_mask?1024:0) |
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
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_t());
      } break;

      case 0x30:
      case 0x31:
      case 0x32:
      case 0x33: {          // Gouraud-shaded 3-pt poly
        //NOTE: The '129' here is CF_GOURAUD | CF_LIGHT, however
        // this is an untextured poly, so CF_LIGHT (texture blend)
        // shouldn't apply. Until the original array of template
        // instantiation ptrs is fixed, we're stuck with this. (TODO)
        u8 gouraud = 129;
        u32 xor_ = 0, rgb0 = le32_raw(gpu_unai.PacketBuffer.U4[0]);
        for (i = 1; i < 3; i++)
          xor_ |= rgb0 ^ le32_raw(gpu_unai.PacketBuffer.U4[i * 2]);
        if ((xor_ & HTOLE32(0xf8f8f8)) == 0)
          gouraud = 0;
        PP driver = gpuPolySpanDrivers[
          //(gpu_unai.blit_mask?1024:0) |
          Dithering |
          Blending_Mode |
          gpu_unai.Masking | Blending | gouraud | gpu_unai.PixelMSB
        ];
        if (gouraud)
          gpuDrawPolyG(packet, driver, false);
        else
          gpuDrawPolyF(packet, driver, false, POLYTYPE_G);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_g());
      } break;

      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37: {          // Gouraud-shaded, textured 3-pt poly
        gpuSetCLUT    (le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
        gpuSetTexture (le32_to_u32(gpu_unai.PacketBuffer.U4[5]) >> 16);
        u8 lighting = Lighting;
        u8 gouraud = lighting ? (1<<7) : 0;
        if (lighting) {
          u32 xor_ = 0, rgb0 = le32_raw(gpu_unai.PacketBuffer.U4[0]);
          for (i = 1; i < 3; i++)
            xor_ |= rgb0 ^ le32_raw(gpu_unai.PacketBuffer.U4[i * 3]);
          if ((xor_ & HTOLE32(0xf8f8f8)) == 0) {
            gouraud = 0;
            if (!need_lighting(rgb0))
              lighting = 0;
          }
        }
        PP driver = gpuPolySpanDrivers[
          //(gpu_unai.blit_mask?1024:0) |
          Dithering |
          Blending_Mode | gpu_unai.TEXT_MODE |
          gpu_unai.Masking | Blending | gouraud | lighting | gpu_unai.PixelMSB
        ];
        if (gouraud)
          gpuDrawPolyGT(packet, driver, false); // is_quad = true
        else
          gpuDrawPolyFT(packet, driver, false, POLYTYPE_GT);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_gt());
      } break;

      case 0x38:
      case 0x39:
      case 0x3A:
      case 0x3B: {          // Gouraud-shaded 4-pt poly
        // See notes regarding '129' for 0x30..0x33 further above -senquack
        u8 gouraud = 129;
        u32 xor_ = 0, rgb0 = le32_raw(gpu_unai.PacketBuffer.U4[0]);
        for (i = 1; i < 4; i++)
          xor_ |= rgb0 ^ le32_raw(gpu_unai.PacketBuffer.U4[i * 2]);
        if ((xor_ & HTOLE32(0xf8f8f8)) == 0)
          gouraud = 0;
        PP driver = gpuPolySpanDrivers[
          //(gpu_unai.blit_mask?1024:0) |
          Dithering |
          Blending_Mode |
          gpu_unai.Masking | Blending | gouraud | gpu_unai.PixelMSB
        ];
        if (gouraud)
          gpuDrawPolyG(packet, driver, true); // is_quad = true
        else
          gpuDrawPolyF(packet, driver, true, POLYTYPE_G);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_g());
      } break;

      case 0x3C:
      case 0x3D:
      case 0x3E:
      case 0x3F: {          // Gouraud-shaded, textured 4-pt poly
        u32 simplified_count;
        gpuSetTexture(le32_to_u32(gpu_unai.PacketBuffer.U4[5]) >> 16);
        if ((simplified_count = prim_try_simplify_quad_gt(gpu_unai.PacketBuffer.U4,
              gpu_unai.PacketBuffer.U4)))
        {
          for (i = 0;; ) {
            textured_sprite(cpu_cycles_sum, cpu_cycles);
            if (++i >= simplified_count)
              break;
            memcpy(&gpu_unai.PacketBuffer.U4[0], &gpu_unai.PacketBuffer.U4[i * 4], 16);
          }
          break;
        }
        gpuSetCLUT(le32_to_u32(gpu_unai.PacketBuffer.U4[2]) >> 16);
        u8 lighting = Lighting;
        u8 gouraud = lighting ? (1<<7) : 0;
        if (lighting) {
          u32 xor_ = 0, rgb0 = le32_raw(gpu_unai.PacketBuffer.U4[0]);
          for (i = 1; i < 4; i++)
            xor_ |= rgb0 ^ le32_raw(gpu_unai.PacketBuffer.U4[i * 3]);
          if ((xor_ & HTOLE32(0xf8f8f8)) == 0) {
            gouraud = 0;
            if (!need_lighting(rgb0))
              lighting = 0;
          }
        }
        PP driver = gpuPolySpanDrivers[
          //(gpu_unai.blit_mask?1024:0) |
          Dithering |
          Blending_Mode | gpu_unai.TEXT_MODE |
          gpu_unai.Masking | Blending | gouraud | lighting | gpu_unai.PixelMSB
        ];
        if (gouraud)
          gpuDrawPolyGT(packet, driver, true); // is_quad = true
        else
          gpuDrawPolyFT(packet, driver, true, POLYTYPE_GT);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_gt());
      } break;

      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43: {          // Monochrome line
        // Shift index right by one, as untextured prims don't use lighting
        u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
        PSD driver = gpuPixelSpanDrivers[driver_idx];
        gpuDrawLineF(packet, driver);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));
      } break;

      case 0x48 ... 0x4F: { // Monochrome line strip
        u32 num_vertexes = 1;
        le32_t *list_position = &list[2];

        // Shift index right by one, as untextured prims don't use lighting
        u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
        PSD driver = gpuPixelSpanDrivers[driver_idx];
        gpuDrawLineF(packet, driver);

        while(1)
        {
          gpu_unai.PacketBuffer.U4[1] = gpu_unai.PacketBuffer.U4[2];
          gpu_unai.PacketBuffer.U4[2] = *list_position++;
          gpuDrawLineF(packet, driver);
          gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));

          num_vertexes++;
          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }
          if((le32_raw(*list_position) & HTOLE32(0xf000f000)) == HTOLE32(0x50005000))
            break;
        }

        len += (num_vertexes - 2);
      } break;

      case 0x50:
      case 0x51:
      case 0x52:
      case 0x53: {          // Gouraud-shaded line
        // Shift index right by one, as untextured prims don't use lighting
        u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
        // Index MSB selects Gouraud-shaded PixelSpanDriver:
        driver_idx |= (1 << 5);
        PSD driver = gpuPixelSpanDrivers[driver_idx];
        gpuDrawLineG(packet, driver);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));
      } break;

      case 0x58 ... 0x5F: { // Gouraud-shaded line strip
        u32 num_vertexes = 1;
        le32_t *list_position = &list[2];

        // Shift index right by one, as untextured prims don't use lighting
        u32 driver_idx = (Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1;
        // Index MSB selects Gouraud-shaded PixelSpanDriver:
        driver_idx |= (1 << 5);
        PSD driver = gpuPixelSpanDrivers[driver_idx];
        gpuDrawLineG(packet, driver);

        while(1)
        {
          gpu_unai.PacketBuffer.U4[0] = gpu_unai.PacketBuffer.U4[2];
          gpu_unai.PacketBuffer.U4[1] = gpu_unai.PacketBuffer.U4[3];
          gpu_unai.PacketBuffer.U4[2] = *list_position++;
          gpu_unai.PacketBuffer.U4[3] = *list_position++;
          gpuDrawLineG(packet, driver);
          gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));

          num_vertexes++;
          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }
          if((le32_raw(*list_position) & HTOLE32(0xf000f000)) == HTOLE32(0x50005000))
            break;
        }

        len += (num_vertexes - 2) * 2;
      } break;

      case 0x60:
      case 0x61:
      case 0x62:
      case 0x63: {          // Monochrome rectangle (variable size)
        PT driver = gpuTileDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
        s32 w = 0, h = 0;
        gpuDrawT(packet, driver, &w, &h);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(w, h));
      } break;

      case 0x64:
      case 0x65:
      case 0x66:
      case 0x67:            // Textured rectangle (variable size)
        textured_sprite(cpu_cycles_sum, cpu_cycles);
        break;

      case 0x68:
      case 0x69:
      case 0x6A:
      case 0x6B: {          // Monochrome rectangle (1x1 dot)
        gpu_unai.PacketBuffer.U4[2] = u32_to_le32(0x00010001);
        PT driver = gpuTileDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
        s32 w = 0, h = 0;
        gpuDrawT(packet, driver, &w, &h);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(1, 1));
      } break;

      case 0x70:
      case 0x71:
      case 0x72:
      case 0x73: {          // Monochrome rectangle (8x8)
        gpu_unai.PacketBuffer.U4[2] = u32_to_le32(0x00080008);
        PT driver = gpuTileDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
        s32 w = 0, h = 0;
        gpuDrawT(packet, driver, &w, &h);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(w, h));
      } break;

      case 0x74:
      case 0x75:
      case 0x76:
      case 0x77: {          // Textured rectangle (8x8)
        gpu_unai.PacketBuffer.U4[3] = u32_to_le32(0x00080008);
        textured_sprite(cpu_cycles_sum, cpu_cycles);
      } break;

      case 0x78:
      case 0x79:
      case 0x7A:
      case 0x7B: {          // Monochrome rectangle (16x16)
        gpu_unai.PacketBuffer.U4[2] = u32_to_le32(0x00100010);
        PT driver = gpuTileDrivers[(Blending_Mode | gpu_unai.Masking | Blending | (gpu_unai.PixelMSB>>3)) >> 1];
        s32 w = 0, h = 0;
        gpuDrawT(packet, driver, &w, &h);
        gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(w, h));
      } break;

      case 0x7C:
      case 0x7D:
      case 0x7E:
      case 0x7F: {          // Textured rectangle (16x16)
        gpu_unai.PacketBuffer.U4[3] = u32_to_le32(0x00100010);
        textured_sprite(cpu_cycles_sum, cpu_cycles);
      } break;

#ifdef TEST
      case 0x80:          //  vid -> vid
        gpuMoveImage(packet);
        break;

      case 0xA0:          //  sys -> vid
      {
        u32 load_width = list[2] & 0xffff;
        u32 load_height = list[2] >> 16;
        u32 load_size = load_width * load_height;

        len += load_size / 2;
      } break;

      case 0xC0:
        break;
#else
      case 0x1F:                   //  irq?
      case 0x80 ... 0x9F:          //  vid -> vid
      case 0xA0 ... 0xBF:          //  sys -> vid
      case 0xC0 ... 0xDF:          //  vid -> sys
        // Handled by gpulib
        goto breakloop;
#endif
      case 0xE1 ... 0xE6: { // Draw settings
        gpuGP0Cmd_0xEx(gpu_unai, le32_to_u32(gpu_unai.PacketBuffer.U4[0]));
      } break;
    }
  }

breakloop:
  gpu.ex_regs[1] &= ~0x1ff;
  gpu.ex_regs[1] |= gpu_unai.GPU_GP1 & 0x1ff;

  *cycles_sum_out += cpu_cycles_sum;
  *cycles_last = cpu_cycles;
  *last_cmd = cmd;
  return list - list_start;
}

void renderer_sync_ecmds(u32 *ecmds)
{
  if (!IS_OLD_RENDERER()) {
    int dummy;
    do_cmd_list(&ecmds[1], 6, &dummy, &dummy, &dummy);
  }
  else
    oldunai_renderer_sync_ecmds(ecmds);
}

void renderer_update_caches(int x, int y, int w, int h, int state_changed)
{
}

void renderer_flush_queues(void)
{
}

void renderer_set_interlace(int enable, int is_odd)
{
  renderer_notify_res_change();
}

#include "../../frontend/plugin_lib.h"
// Handle any gpulib settings applicable to gpu_unai:
void renderer_set_config(const struct rearmed_cbs *cbs)
{
  gpu_unai.vram = (le16_t *)gpu.vram;
  gpu_unai.config.old_renderer  = cbs->gpu_unai.old_renderer;
  gpu_unai.config.ilace_force   = cbs->gpu_unai.ilace_force;
  gpu_unai.config.lighting      = cbs->gpu_unai.lighting;
  gpu_unai.config.fast_lighting = cbs->gpu_unai.fast_lighting;
  gpu_unai.config.blending      = cbs->gpu_unai.blending;
  gpu_unai.config.scale_hires   = cbs->gpu_unai.scale_hires;
  gpu_unai.config.dithering     = cbs->dithering != 0;
  gpu_unai.config.force_dithering = cbs->dithering >> 1;

  gpu.state.downscale_enable    = gpu_unai.config.scale_hires;
  if (gpu_unai.config.scale_hires) {
    map_downscale_buffer();
  } else {
    unmap_downscale_buffer();
  }
  oldunai_renderer_set_config(cbs);
}

void renderer_sync(void)
{
}

void renderer_notify_update_lace(int updated)
{
}

// vim:shiftwidth=2:expandtab
