/*
 * video output handling using plugin_lib
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include <string.h>
#include "gpu.h"
#include "../../frontend/plugin_lib.h"

static const struct rearmed_cbs *cbs;

int vout_init(void)
{
  return 0;
}

int vout_finish(void)
{
  return 0;
}

static void check_mode_change(int force)
{
  int w = gpu.screen.hres;
  int h = gpu.screen.vres;
  int w_out, h_out, bpp = 16;

  if (gpu.state.screen_centering_type == C_BORDERLESS)
    h = gpu.screen.h;
  w_out = w, h_out = h;
#ifdef RAW_FB_DISPLAY
  w = w_out = (gpu.status & PSX_GPU_STATUS_RGB24) ? 2048/3 : 1024;
  h = h_out = 512;
#endif
  gpu.state.enhancement_active =
    gpu.get_enhancement_bufer != NULL && gpu.state.enhancement_enable
    && w <= 512 && h <= 256 && !(gpu.status & PSX_GPU_STATUS_RGB24);

  if (gpu.state.enhancement_active) {
    w_out *= 2;
    h_out *= 2;
  }
  if (gpu.status & PSX_GPU_STATUS_RGB24) {
    // some asm relies on this alignment
    w_out = (w_out + 7) & ~7;
    bpp = 24;
  }

  gpu.state.downscale_active =
    gpu.get_downscale_buffer != NULL && gpu.state.downscale_enable
    && (w >= 512 || h >= 256);

  if (gpu.state.downscale_active) {
    w_out = w < 512 ? w : 320;
    h_out = h < 256 ? h : h / 2;
  }

  // width|rgb24 change?
  if (force || (gpu.status ^ gpu.state.status_vo_old) & ((7<<16)|(1<<21))
      || w_out != gpu.state.w_out_old || h_out != gpu.state.h_out_old)
  {
    gpu.state.status_vo_old = gpu.status;
    gpu.state.w_out_old = w_out;
    gpu.state.h_out_old = h_out;

    if (w_out != 0 && h_out != 0)
      cbs->pl_vout_set_mode(w_out, h_out, w, h, bpp);
  }
}

int vout_update(void)
{
  int bpp = (gpu.status & PSX_GPU_STATUS_RGB24) ? 24 : 16;
  uint8_t *vram = (uint8_t *)gpu.vram;
  int src_x = gpu.screen.src_x;
  int src_y = gpu.screen.src_y;
  int x = gpu.screen.x;
  int y = gpu.screen.y;
  int w = gpu.screen.w;
  int h = gpu.screen.h;
  int vram_h = 512;
  int src_x2 = 0;
  int offset;

#ifdef RAW_FB_DISPLAY
  w = (gpu.status & PSX_GPU_STATUS_RGB24) ? 2048/3 : 1024;
  h = 512, x = src_x = y = src_y = 0;
#endif
  if (x < 0) { w += x; src_x2 = -x; x = 0; }
  if (y < 0) { h += y; src_y -=  y; y = 0; }

  if (w <= 0 || h <= 0)
    return 0;

  check_mode_change(0);
  if (gpu.state.enhancement_active) {
    if (!gpu.state.enhancement_was_active)
      return 0; // buffer not ready yet
    vram = gpu.get_enhancement_bufer(&src_x, &src_y, &w, &h, &vram_h);
    if (vram == NULL)
      return 0;
    x *= 2; y *= 2;
    src_x2 *= 2;
  }

  if (gpu.state.downscale_active)
    vram = (void *)gpu.get_downscale_buffer(&src_x, &src_y, &w, &h, &vram_h);

  if (src_y + h > vram_h) {
    if (src_y + h - vram_h > h / 2) {
      // wrap
      h -= vram_h - src_y;
      src_y = 0;
    }
    else
      // clip
      h = vram_h - src_y;
  }

  offset = (src_y * 1024 + src_x) * 2;
  offset += src_x2 * bpp / 8;

  cbs->pl_vout_flip(vram, offset, !!(gpu.status & PSX_GPU_STATUS_RGB24),
      x, y, w, h, gpu.state.dims_changed);
  gpu.state.dims_changed = 0;
  return 1;
}

void vout_blank(void)
{
  int w = gpu.screen.hres;
  int h = gpu.screen.vres;

  check_mode_change(0);
  if (gpu.state.enhancement_active) {
    w *= 2;
    h *= 2;
  }
  cbs->pl_vout_flip(NULL, 0, !!(gpu.status & PSX_GPU_STATUS_RGB24), 0, 0, w, h, 0);
}

long GPUopen(unsigned long *disp, char *cap, char *cfg)
{
  gpu.frameskip.active = 0;
  gpu.frameskip.frame_ready = 1;

  cbs->pl_vout_open();
  check_mode_change(1);
  vout_update();
  return 0;
}

long GPUclose(void)
{
  cbs->pl_vout_close();
  return 0;
}

void vout_set_config(const struct rearmed_cbs *cbs_)
{
  cbs = cbs_;
}

// vim:shiftwidth=2:expandtab
