/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <assert.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

extern const unsigned char cmd_lengths[256];
#define command_lengths cmd_lengths

static unsigned int *ex_regs;
static int initialized;

#define PCSX
#define SET_Ex(r, v) \
  ex_regs[r] = v

static __attribute__((noinline)) void
sync_enhancement_buffers(int x, int y, int w, int h);

#include "../gpulib/gpu.h"
#include "psx_gpu/psx_gpu.c"
#include "psx_gpu/psx_gpu_parse.c"

static psx_gpu_struct egpu __attribute__((aligned(256)));

int do_cmd_list(uint32_t *list, int count,
 int *cycles_sum, int *cycles_last, int *last_cmd)
{
  int ret;

  if (gpu.state.enhancement_active)
    ret = gpu_parse_enhanced(&egpu, list, count * 4,
            cycles_sum, cycles_last, (u32 *)last_cmd);
  else
    ret = gpu_parse(&egpu, list, count * 4,
            cycles_sum, cycles_last, (u32 *)last_cmd);

  ex_regs[1] &= ~0x1ff;
  ex_regs[1] |= egpu.texture_settings & 0x1ff;
  return ret;
}

#define ENHANCEMENT_BUF_SIZE (1024 * 1024 * 2 * 4 + 4096 * 2)

static void *get_enhancement_bufer(int *x, int *y, int *w, int *h,
 int *vram_h)
{
  uint16_t *ret = select_enhancement_buf_ptr(&egpu, *x, *y);
  if (ret == NULL)
    return NULL;

  *x *= 2;
  *y *= 2;
  *w = *w * 2;
  *h = *h * 2;
  *vram_h = 1024;
  return ret;
}

static void map_enhancement_buffer(void)
{
  // currently we use 4x 1024*1024 buffers instead of single 2048*1024
  // to be able to reuse 1024-width code better (triangle setup,
  // dithering phase, lines).
  egpu.enhancement_buf_ptr = gpu.mmap(ENHANCEMENT_BUF_SIZE);
  if (egpu.enhancement_buf_ptr == NULL || egpu.enhancement_buf_ptr == (void *)(intptr_t)-1) {
    fprintf(stderr, "failed to map enhancement buffer\n");
    egpu.enhancement_buf_ptr = NULL;
    gpu.get_enhancement_bufer = NULL;
  }
  else {
    egpu.enhancement_buf_ptr += 4096 / 2;
    gpu.get_enhancement_bufer = get_enhancement_bufer;
  }
}

int renderer_init(void)
{
  if (gpu.vram != NULL) {
    initialize_psx_gpu(&egpu, gpu.vram);
    initialized = 1;
  }

  if (gpu.mmap != NULL && egpu.enhancement_buf_ptr == NULL)
    map_enhancement_buffer();

  ex_regs = gpu.ex_regs;
  return 0;
}

void renderer_finish(void)
{
  if (egpu.enhancement_buf_ptr != NULL) {
    egpu.enhancement_buf_ptr -= 4096 / 2;
    gpu.munmap(egpu.enhancement_buf_ptr, ENHANCEMENT_BUF_SIZE);
  }
  egpu.enhancement_buf_ptr = NULL;
  egpu.enhancement_current_buf_ptr = NULL;
  initialized = 0;
}

static __attribute__((noinline)) void
sync_enhancement_buffers(int x, int y, int w, int h)
{
  int i, right = x + w, bottom = y + h;
  const u16 *src = gpu.vram;
  // use these because the scanout struct may hold reduced w, h
  // due to intersection stuff, see the update_enhancement_buf_scanouts() mess
  int s_w = max(gpu.screen.hres, gpu.screen.w);
  int s_h = gpu.screen.vres;
  if (gpu.screen.y < 0)
    s_h -= gpu.screen.y;
  s_w = min(s_w, 512);
  for (i = 0; i < ARRAY_SIZE(egpu.enhancement_scanouts); i++) {
    const struct psx_gpu_scanout *s = &egpu.enhancement_scanouts[i];
    u16 *dst = select_enhancement_buf_by_index(&egpu, i);
    int x1, x2, y1, y2;
    if (s->w == 0) continue;
    if (s->x >= right) continue;
    if (s->x + s_w <= x) continue;
    if (s->y >= bottom) continue;
    if (s->y + s_h <= y) continue;
    x1 = max(x, s->x);
    x2 = min(right, s->x + s_w);
    y1 = max(y, s->y);
    y2 = min(bottom, s->y + s_h);
    // 16-byte align for the asm version
    x2 += x1 & 7;
    x1 &= ~7;
    scale2x_tiles8(dst + y1 * 1024*2 + x1 * 2,
        src + y1 * 1024 + x1, (x2 - x1 + 7) / 8u, y2 - y1);
  }
}

void renderer_sync_ecmds(uint32_t *ecmds)
{
  s32 dummy0 = 0;
  u32 dummy1 = 0;
  gpu_parse(&egpu, ecmds + 1, 6 * 4, &dummy0, &dummy0, &dummy1);
}

void renderer_update_caches(int x, int y, int w, int h, int state_changed)
{
  update_texture_cache_region(&egpu, x, y, x + w - 1, y + h - 1);

  if (gpu.state.enhancement_active) {
    if (state_changed) {
      int vres = gpu.screen.vres;
      if (gpu.screen.y < 0)
        vres -= gpu.screen.y;
      memset(egpu.enhancement_scanouts, 0, sizeof(egpu.enhancement_scanouts));
      egpu.enhancement_scanout_eselect = 0;
      update_enhancement_buf_scanouts(&egpu,
        gpu.screen.src_x, gpu.screen.src_y, gpu.screen.hres, vres);
      return;
    }
    sync_enhancement_buffers(x, y, w, h);
  }
}

void renderer_flush_queues(void)
{
  flush_render_block_buffer(&egpu);
}

void renderer_set_interlace(int enable, int is_odd)
{
  egpu.render_mode &= ~(RENDER_INTERLACE_ENABLED|RENDER_INTERLACE_ODD);
  if (enable)
    egpu.render_mode |= RENDER_INTERLACE_ENABLED;
  if (is_odd)
    egpu.render_mode |= RENDER_INTERLACE_ODD;
}

void renderer_notify_res_change(void)
{
  renderer_notify_scanout_change(gpu.screen.src_x, gpu.screen.src_y);
}

void renderer_notify_scanout_change(int x, int y)
{
  int vres = gpu.screen.vres;
  if (!gpu.state.enhancement_active || !egpu.enhancement_buf_ptr)
    return;

  if (gpu.screen.y < 0)
    vres -= gpu.screen.y;
  update_enhancement_buf_scanouts(&egpu, x, y, gpu.screen.hres, vres);
}

#include "../../frontend/plugin_lib.h"

void renderer_set_config(const struct rearmed_cbs *cbs)
{
  if (!initialized) {
    initialize_psx_gpu(&egpu, gpu.vram);
    initialized = 1;
  }
  if (cbs->pl_set_gpu_caps)
    cbs->pl_set_gpu_caps(GPU_CAP_SUPPORTS_2X);
  
  egpu.allow_dithering = cbs->dithering;
  egpu.force_dithering = cbs->dithering >> 1;
  /*
  if (!egpu.allow_dithering) {
    egpu.dither_table[0] = dither_table_row(0, 0, 0, 0);
    egpu.dither_table[1] = dither_table_row(0, 0, 0, 0);
    egpu.dither_table[2] = dither_table_row(0, 0, 0, 0);
    egpu.dither_table[3] = dither_table_row(0, 0, 0, 0);
  } else {
    egpu.dither_table[0] = dither_table_row(-4, 0, -3, 1);
    egpu.dither_table[1] = dither_table_row(2, -2, 3, -1);
    egpu.dither_table[2] = dither_table_row(-3, 1, -4, 0);
    egpu.dither_table[3] = dither_table_row(3, -1, 2, -2); 
  }
  */

  egpu.hack_disable_main = cbs->gpu_neon.enhancement_no_main;
  egpu.hack_texture_adj = cbs->gpu_neon.enhancement_tex_adj;
  if (gpu.state.enhancement_enable) {
    if (gpu.mmap != NULL && egpu.enhancement_buf_ptr == NULL)
      map_enhancement_buffer();
  }
}

void renderer_sync(void)
{
}

void renderer_notify_update_lace(int updated)
{
}

// vim:ts=2:sw=2:expandtab
